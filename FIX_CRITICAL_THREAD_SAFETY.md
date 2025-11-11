# Critical Fix: Thread Safety Issues with Engine Swapping

## Issue Overview

The plugin has race conditions between the audio thread (processBlock) and the message thread (parameter changes) when swapping engines. This can cause:
- **Crashes** when an engine is deleted while being processed
- **Use-after-free** errors accessing deleted engine memory
- **Incomplete initialization** when processing starts before prepareToPlay
- **Data corruption** from simultaneous parameter updates
- **Deadlocks** from improper mutex usage

### Current State
- Single `m_engineMutex` protects engine array
- Mutex held during entire process() call (performance issue)
- Engine swapping not atomic
- No lock-free alternatives used
- Parameter updates can race with processing

## System Context

### Threading Model
1. **Audio Thread**: High-priority, real-time
   - Calls processBlock() continuously
   - Must never block or allocate memory
   - Typical callback rate: 44100Hz / 128 samples = ~345 times/second

2. **Message Thread**: Normal priority
   - Handles parameter changes
   - Creates/destroys engines
   - Can block and allocate

3. **Race Condition Scenario**:
   ```
   Audio Thread                Message Thread
   ------------                --------------
   Lock mutex
   Check engine exists
   Start processing
                              User changes engine
                              Waits for mutex...
   Call engine->process()
   Unlock mutex
                              Lock mutex
                              Delete old engine  <-- CRASH! Still processing
                              Create new engine
                              Unlock mutex
   ```

### Current Problematic Code

```cpp
// In processBlock() - holds lock too long
{
    std::lock_guard<std::mutex> lock(m_engineMutex);
    if (m_activeEngines[slot]) {
        m_activeEngines[slot]->updateParameters(params);
        m_activeEngines[slot]->process(wetBuffer);  // Lock held during processing!
    }
}

// In parameterChanged() - can delete while processing
{
    std::lock_guard<std::mutex> lock(m_engineMutex);
    m_activeEngines[slot] = std::move(newEngine);  // Deletes old engine!
}
```

## Agent Task

### Objective
Implement thread-safe engine swapping without blocking the audio thread or causing race conditions.

### Requirements

1. **Never block audio thread** - Use lock-free techniques where possible
2. **Prevent use-after-free** - Ensure engines aren't deleted while processing
3. **Atomic engine swaps** - Make engine replacement atomic
4. **Safe parameter updates** - Prevent parameter races
5. **No priority inversion** - Audio thread must not wait for message thread
6. **Maintain low latency** - < 0.1ms overhead

### Implementation Steps

1. **Analyze current threading issues**:
   ```bash
   # Review all mutex usage
   grep -n "m_engineMutex\|std::lock_guard\|std::mutex" \
     /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/PluginProcessor.cpp
   ```

2. **Implement double-buffering for engine swaps**:
   ```cpp
   class EngineSlot {
   private:
       std::atomic<EngineBase*> currentEngine{nullptr};
       std::unique_ptr<EngineBase> pendingEngine;
       std::atomic<bool> swapPending{false};
       std::unique_ptr<EngineBase> oldEngine;  // For deferred deletion

   public:
       void requestSwap(std::unique_ptr<EngineBase> newEngine);
       void performSwapIfNeeded();  // Called from audio thread
       EngineBase* getEngine() const;
   };
   ```

3. **Use lock-free queue for parameter updates**:
   ```cpp
   class LockFreeParameterQueue {
   private:
       struct ParameterUpdate {
           int slotIndex;
           int paramIndex;
           float value;
       };

       moodycamel::ReaderWriterQueue<ParameterUpdate> queue;

   public:
       void pushUpdate(int slot, int param, float value);
       bool popUpdate(ParameterUpdate& update);
   };
   ```

4. **Implement safe engine lifecycle**:
   ```cpp
   class ThreadSafeEngineManager {
   private:
       struct EngineState {
           std::atomic<EngineBase*> engine{nullptr};
           std::atomic<bool> preparedToPlay{false};
           std::atomic<int> referenceCount{0};
       };

       std::array<EngineState, NUM_SLOTS> engineStates;
       std::vector<std::unique_ptr<EngineBase>> deferredDeletions;

   public:
       void swapEngine(int slot, std::unique_ptr<EngineBase> newEngine);
       EngineBase* getEngineForProcessing(int slot);
       void releaseEngine(int slot);
       void cleanupDeferredDeletions();  // Call from message thread
   };
   ```

5. **Refactor processBlock for lock-free operation**:
   ```cpp
   void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
       // Apply pending parameter updates (lock-free)
       applyPendingParameterUpdates();

       // Process each slot without locks
       for (int slot = 0; slot < NUM_SLOTS; ++slot) {
           // Get engine with reference counting
           if (auto* engine = engineManager.getEngineForProcessing(slot)) {
               // Create scope guard for automatic release
               ScopeGuard guard([&]{ engineManager.releaseEngine(slot); });

               // Safe to process - engine won't be deleted
               engine->process(buffer);
           }
       }

       // Perform any pending engine swaps
       engineManager.performPendingSwaps();
   }
   ```

6. **Add memory barriers and atomic operations**:
   ```cpp
   class AtomicEnginePointer {
   private:
       std::atomic<EngineBase*> ptr{nullptr};

   public:
       void store(EngineBase* engine) {
           ptr.store(engine, std::memory_order_release);
       }

       EngineBase* load() const {
           return ptr.load(std::memory_order_acquire);
       }

       bool compare_exchange(EngineBase*& expected, EngineBase* desired) {
           return ptr.compare_exchange_strong(expected, desired,
                                             std::memory_order_acq_rel);
       }
   };
   ```

### Specific Patterns to Implement

1. **Pattern: Read-Copy-Update (RCU)**
   ```cpp
   // Message thread prepares new engine
   auto newEngine = createEngine();
   newEngine->prepareToPlay(sampleRate, blockSize);

   // Atomically swap pointers
   EngineBase* oldEngine = enginePointer.exchange(newEngine);

   // Defer deletion until safe
   if (oldEngine) {
       deferredDeletions.push_back(oldEngine);
   }
   ```

2. **Pattern: Hazard Pointers**
   ```cpp
   class HazardPointer {
       std::atomic<EngineBase*> hazard{nullptr};

   public:
       EngineBase* acquire(std::atomic<EngineBase*>& ptr) {
           EngineBase* p;
           do {
               p = ptr.load();
               hazard.store(p);
           } while (p != ptr.load());
           return p;
       }

       void release() {
           hazard.store(nullptr);
       }
   };
   ```

3. **Pattern: Epoch-Based Reclamation**
   ```cpp
   class EpochManager {
       std::atomic<uint64_t> globalEpoch{0};
       thread_local uint64_t localEpoch{0};

   public:
       void enterCritical() {
           localEpoch = globalEpoch.load();
       }

       void exitCritical() {
           // Mark safe to reclaim
       }

       bool canDelete(uint64_t creationEpoch) {
           return creationEpoch < getMinEpoch();
       }
   };
   ```

### Testing Thread Safety

1. **Thread Sanitizer**:
   ```bash
   # Compile with ThreadSanitizer
   clang++ -fsanitize=thread -g ...

   # Look for race conditions
   ./test_program 2>&1 | grep "WARNING: ThreadSanitizer"
   ```

2. **Stress test scenarios**:
   ```cpp
   void stressTestEngineSwapping() {
       // Thread 1: Continuously process audio
       std::thread audioThread([&] {
           while (running) {
               processBlock(buffer, midi);
           }
       });

       // Thread 2: Rapidly swap engines
       std::thread swapThread([&] {
           while (running) {
               for (int i = 0; i < ENGINE_COUNT; ++i) {
                   swapEngine(randomSlot(), randomEngine());
                   std::this_thread::sleep_for(1ms);
               }
           }
       });

       // Thread 3: Rapidly change parameters
       std::thread paramThread([&] {
           while (running) {
               changeRandomParameter();
               std::this_thread::sleep_for(0.1ms);
           }
       });
   }
   ```

3. **Helgrind validation**:
   ```bash
   valgrind --tool=helgrind ./test_program
   ```

## Implementation Priority

### Phase 1: Immediate Safety (Day 1)
1. Add reference counting to prevent use-after-free
2. Reduce mutex hold time
3. Add memory barriers

### Phase 2: Lock-Free Parameters (Day 2)
1. Implement parameter queue
2. Remove parameter mutex locks
3. Test with ThreadSanitizer

### Phase 3: Lock-Free Engine Swapping (Day 3)
1. Implement RCU or hazard pointers
2. Remove engine mutex from audio thread
3. Extensive stress testing

## Files to Modify

1. `/JUCE_Plugin/Source/PluginProcessor.h` - Add thread-safe structures
2. `/JUCE_Plugin/Source/PluginProcessor.cpp` - Refactor processBlock
3. Create `/JUCE_Plugin/Source/ThreadSafeEngineManager.h`
4. Create `/JUCE_Plugin/Source/LockFreeParameterQueue.h`

## Success Criteria

1. **No data races** detected by ThreadSanitizer
2. **No deadlocks** under any load
3. **Audio thread never blocks** (verified with profiler)
4. **< 0.1ms overhead** from synchronization
5. **10,000+ engine swaps** without crashes
6. **Clean Helgrind report**

## Example Thread-Safe Implementation

```cpp
class ThreadSafeProcessor {
private:
    // Lock-free engine pointers
    struct EngineSlot {
        std::atomic<EngineBase*> engine{nullptr};
        std::atomic<int> refCount{0};
    };
    std::array<EngineSlot, NUM_SLOTS> slots;

    // Lock-free parameter updates
    struct ParamUpdate {
        uint8_t slot;
        uint8_t param;
        float value;
    };
    lockfree::spsc_queue<ParamUpdate, 1024> paramQueue;

    // Deferred deletions (message thread only)
    std::vector<std::unique_ptr<EngineBase>> graveyard;
    std::mutex graveyardMutex;

public:
    void processBlock(juce::AudioBuffer<float>& buffer) {
        // Apply parameter updates (lock-free)
        ParamUpdate update;
        while (paramQueue.pop(update)) {
            if (auto* engine = slots[update.slot].engine.load()) {
                engine->setParameter(update.param, update.value);
            }
        }

        // Process each slot (lock-free)
        for (auto& slot : slots) {
            if (auto* engine = slot.engine.load(std::memory_order_acquire)) {
                slot.refCount.fetch_add(1);

                engine->process(buffer);

                if (slot.refCount.fetch_sub(1) == 1) {
                    // Last reference, safe to delete if marked
                    cleanupIfNeeded(slot);
                }
            }
        }
    }

    void changeEngine(int slotIdx, std::unique_ptr<EngineBase> newEngine) {
        // Prepare new engine (message thread)
        newEngine->prepareToPlay(sampleRate, blockSize);

        // Atomic swap
        auto* oldEngine = slots[slotIdx].engine.exchange(newEngine.release());

        // Defer deletion
        if (oldEngine) {
            std::lock_guard<std::mutex> lock(graveyardMutex);
            graveyard.push_back(std::unique_ptr<EngineBase>(oldEngine));
        }
    }
};
```

---

**Priority**: 🔴 CRITICAL - Race conditions cause unpredictable crashes
**Estimated Time**: 2-3 days (complex refactoring required)
**Risk if Not Fixed**: Random crashes, audio glitches, corrupted state, poor user experience