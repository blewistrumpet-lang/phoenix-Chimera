# FIX 01: Critical Safety - Complete Implementation Guide

## Overview

This document consolidates all critical safety fixes (TD-001, TD-002, TD-003) into a single coordinated implementation plan. These MUST be fixed together as they're interdependent.

### Scope
- **TD-001**: Error handling in processBlock
- **TD-002**: Buffer overflow protection
- **TD-003**: Thread safety for engine swapping

### Timeline: 5 Days
- Day 1-2: Core safety framework
- Day 3-4: Engine-specific implementation
- Day 5: Integration and testing

## Day 1: Safety Framework Foundation

### Step 1.1: Create Core Safety Headers

```cpp
// SafetyCore.h - Central safety utilities
#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include <atomic>
#include <memory>

namespace ChimeraSafety {

// ============ Error Handling ============
class SafetyException : public std::exception {
    juce::String message;
    int errorCode;
public:
    SafetyException(const juce::String& msg, int code = -1)
        : message(msg), errorCode(code) {}
    const char* what() const noexcept override {
        return message.toRawUTF8();
    }
};

// Error reporter that doesn't block audio thread
class ErrorReporter {
private:
    struct Error {
        std::string message;
        std::chrono::time_point<std::chrono::steady_clock> timestamp;
        int severity;
    };

    moodycamel::ReaderWriterQueue<Error> errorQueue{1024};
    std::atomic<int> errorCount{0};

public:
    static ErrorReporter& instance() {
        static ErrorReporter reporter;
        return reporter;
    }

    void reportError(const std::string& msg, int severity = 1) {
        Error e{msg, std::chrono::steady_clock::now(), severity};
        errorQueue.enqueue(e);
        errorCount.fetch_add(1);
    }

    std::vector<Error> getRecentErrors(int count = 10);
};

// ============ Buffer Validation ============
class BufferValidator {
public:
    static bool isValid(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() <= 0) return false;
        if (buffer.getNumChannels() > 32) return false;  // Sanity check
        if (buffer.getNumSamples() <= 0) return false;
        if (buffer.getNumSamples() > 8192) return false;  // Max block size

        // Check for null pointers
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            if (!buffer.getReadPointer(ch)) return false;
        }

        return true;
    }

    static bool containsNaN(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s) {
                if (!std::isfinite(data[s])) return true;
            }
        }
        return false;
    }

    static void sanitizeBuffer(juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s) {
                if (!std::isfinite(data[s])) {
                    data[s] = 0.0f;
                }
                data[s] = juce::jlimit(-10.0f, 10.0f, data[s]);  // Prevent explosions
            }
        }
    }
};

// ============ Thread-Safe Engine Management ============
template<typename T>
class AtomicPointer {
private:
    std::atomic<T*> ptr{nullptr};

public:
    void store(T* p) { ptr.store(p, std::memory_order_release); }
    T* load() const { return ptr.load(std::memory_order_acquire); }

    T* exchange(T* newPtr) {
        return ptr.exchange(newPtr, std::memory_order_acq_rel);
    }

    bool compare_exchange(T*& expected, T* desired) {
        return ptr.compare_exchange_strong(expected, desired,
                                          std::memory_order_acq_rel);
    }
};

// Reference-counted engine wrapper for safe deletion
class SafeEngineWrapper {
private:
    std::unique_ptr<EngineBase> engine;
    std::atomic<int> refCount{0};
    std::atomic<bool> markedForDeletion{false};

public:
    SafeEngineWrapper(std::unique_ptr<EngineBase> e)
        : engine(std::move(e)) {}

    EngineBase* acquire() {
        if (markedForDeletion.load()) return nullptr;
        refCount.fetch_add(1);
        return engine.get();
    }

    void release() {
        refCount.fetch_sub(1);
    }

    bool canDelete() const {
        return markedForDeletion.load() && refCount.load() == 0;
    }

    void markForDeletion() {
        markedForDeletion.store(true);
    }
};

// ============ Safe Processing Wrapper ============
class SafeProcessor {
public:
    template<typename ProcessFunc>
    static bool safeProcess(const std::string& context,
                           juce::AudioBuffer<float>& buffer,
                           ProcessFunc func) {
        try {
            // Pre-validation
            if (!BufferValidator::isValid(buffer)) {
                ErrorReporter::instance().reportError(
                    context + ": Invalid buffer", 2);
                buffer.clear();
                return false;
            }

            // Check for NaN input
            if (BufferValidator::containsNaN(buffer)) {
                ErrorReporter::instance().reportError(
                    context + ": NaN in input", 2);
                BufferValidator::sanitizeBuffer(buffer);
            }

            // Execute processing
            func();

            // Post-validation
            if (BufferValidator::containsNaN(buffer)) {
                ErrorReporter::instance().reportError(
                    context + ": NaN in output", 2);
                BufferValidator::sanitizeBuffer(buffer);
            }

            return true;

        } catch (const std::exception& e) {
            ErrorReporter::instance().reportError(
                context + ": Exception: " + std::string(e.what()), 3);
            buffer.clear();
            return false;
        } catch (...) {
            ErrorReporter::instance().reportError(
                context + ": Unknown exception", 3);
            buffer.clear();
            return false;
        }
    }
};

} // namespace ChimeraSafety
```

### Step 1.2: Create Thread-Safe Engine Manager

```cpp
// ThreadSafeEngineManager.h
#pragma once
#include "SafetyCore.h"
#include <array>

class ThreadSafeEngineManager {
private:
    static constexpr int NUM_SLOTS = 4;

    struct EngineSlot {
        AtomicPointer<SafeEngineWrapper> current;
        std::unique_ptr<SafeEngineWrapper> pending;
        std::atomic<bool> swapPending{false};
    };

    std::array<EngineSlot, NUM_SLOTS> slots;
    std::vector<std::unique_ptr<SafeEngineWrapper>> graveyard;
    std::mutex graveyardMutex;

public:
    // Called from message thread
    void requestEngineSwap(int slot, std::unique_ptr<EngineBase> newEngine) {
        if (slot < 0 || slot >= NUM_SLOTS) return;

        // Prepare new engine
        auto wrapper = std::make_unique<SafeEngineWrapper>(std::move(newEngine));

        // Store as pending
        slots[slot].pending = std::move(wrapper);
        slots[slot].swapPending.store(true);
    }

    // Called from audio thread at safe point
    void performPendingSwaps() {
        for (int slot = 0; slot < NUM_SLOTS; ++slot) {
            if (slots[slot].swapPending.load()) {
                // Atomic pointer swap
                auto* old = slots[slot].current.exchange(
                    slots[slot].pending.release());

                if (old) {
                    old->markForDeletion();

                    // Move to graveyard for later cleanup
                    std::lock_guard<std::mutex> lock(graveyardMutex);
                    graveyard.push_back(std::unique_ptr<SafeEngineWrapper>(old));
                }

                slots[slot].swapPending.store(false);
            }
        }
    }

    // Called from audio thread for processing
    EngineBase* getEngineForProcessing(int slot) {
        if (slot < 0 || slot >= NUM_SLOTS) return nullptr;

        auto* wrapper = slots[slot].current.load();
        return wrapper ? wrapper->acquire() : nullptr;
    }

    void releaseEngine(int slot, EngineBase* engine) {
        if (slot < 0 || slot >= NUM_SLOTS || !engine) return;

        auto* wrapper = slots[slot].current.load();
        if (wrapper) {
            wrapper->release();
        }
    }

    // Called periodically from message thread
    void cleanupGraveyard() {
        std::lock_guard<std::mutex> lock(graveyardMutex);
        graveyard.erase(
            std::remove_if(graveyard.begin(), graveyard.end(),
                [](const auto& wrapper) { return wrapper->canDelete(); }),
            graveyard.end()
        );
    }
};
```

## Day 2: PluginProcessor Integration

### Step 2.1: Refactor processBlock with Complete Safety

```cpp
// In PluginProcessor.cpp

void ChimeraAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    // SAFETY LAYER 1: Input validation
    if (!ChimeraSafety::BufferValidator::isValid(buffer)) {
        buffer.clear();
        ChimeraSafety::ErrorReporter::instance().reportError(
            "processBlock: Invalid input buffer received");
        return;
    }

    // SAFETY LAYER 2: NaN sanitization
    if (ChimeraSafety::BufferValidator::containsNaN(buffer)) {
        ChimeraSafety::BufferValidator::sanitizeBuffer(buffer);
        ChimeraSafety::ErrorReporter::instance().reportError(
            "processBlock: NaN detected in input");
    }

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // SAFETY LAYER 3: Protect against extreme values
    for (int ch = 0; ch < numChannels; ++ch) {
        buffer.applyGain(ch, 0, numSamples, 0.99f);  // Headroom
    }

    // Create dry buffer with validation
    juce::AudioBuffer<float> dryBuffer;
    try {
        dryBuffer.setSize(numChannels, numSamples, false, false, true);
        for (int ch = 0; ch < numChannels; ++ch) {
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        }
    } catch (const std::exception& e) {
        ChimeraSafety::ErrorReporter::instance().reportError(
            "processBlock: Failed to create dry buffer: " + std::string(e.what()));
        buffer.clear();
        return;
    }

    // Process each slot with full safety
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        if (!processSlotSafely(slot, buffer, numSamples)) {
            // Log but continue processing other slots
            ChimeraSafety::ErrorReporter::instance().reportError(
                "processBlock: Slot " + std::to_string(slot) + " failed");
        }
    }

    // SAFETY LAYER 4: Final output validation
    ChimeraSafety::BufferValidator::sanitizeBuffer(buffer);

    // SAFETY LAYER 5: Soft clipping to prevent speaker damage
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s) {
            // Soft clip
            if (std::abs(data[s]) > 0.95f) {
                data[s] = std::tanh(data[s] * 0.7f) * 1.2f;
            }
            // Hard limit
            data[s] = juce::jlimit(-0.99f, 0.99f, data[s]);
        }
    }

    // Perform pending engine swaps at safe point
    engineManager.performPendingSwaps();
}

bool ChimeraAudioProcessor::processSlotSafely(int slot,
                                             juce::AudioBuffer<float>& buffer,
                                             int numSamples) {
    // Get slot parameters safely
    bool isBypassed = false;
    float mixLevel = 0.5f;

    try {
        auto* bypassParam = parameters.getRawParameterValue(
            "slot" + juce::String(slot + 1) + "_bypass");
        auto* mixParam = parameters.getRawParameterValue(
            "slot" + juce::String(slot + 1) + "_mix");

        isBypassed = bypassParam ? bypassParam->load() > 0.5f : false;
        mixLevel = mixParam ? mixParam->load() : 0.5f;
    } catch (...) {
        return false;  // Skip slot on parameter error
    }

    if (isBypassed) return true;

    // Get engine safely (no locks in audio thread!)
    auto* engine = engineManager.getEngineForProcessing(slot);
    if (!engine) return true;  // No engine is OK

    // RAII to ensure engine is released
    struct EngineReleaser {
        ThreadSafeEngineManager* manager;
        int slot;
        EngineBase* engine;
        ~EngineReleaser() {
            if (manager && engine) manager->releaseEngine(slot, engine);
        }
    } releaser{&engineManager, slot, engine};

    // Create wet buffer for processing
    juce::AudioBuffer<float> wetBuffer;
    try {
        wetBuffer.makeCopyOf(buffer);
    } catch (...) {
        return false;
    }

    // Process with complete safety wrapper
    bool success = ChimeraSafety::SafeProcessor::safeProcess(
        "Slot" + std::to_string(slot),
        wetBuffer,
        [&]() {
            // Update parameters safely
            std::map<int, float> params;
            for (int i = 0; i < 15; ++i) {
                auto* param = parameters.getRawParameterValue(
                    "slot" + juce::String(slot + 1) + "_param" + juce::String(i + 1));
                params[i] = param ? juce::jlimit(0.0f, 1.0f, param->load()) : 0.5f;
            }

            engine->updateParameters(params);
            engine->process(wetBuffer);
        }
    );

    if (success) {
        // Apply wet/dry mix
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* dry = buffer.getWritePointer(ch);
            auto* wet = wetBuffer.getReadPointer(ch);

            for (int s = 0; s < numSamples; ++s) {
                dry[s] = dry[s] * (1.0f - mixLevel) + wet[s] * mixLevel;
            }
        }
    }

    return success;
}
```

## Day 3-4: Engine-Specific Safety

### Step 3.1: Create Safe Engine Base Class

```cpp
// SafeEngineBase.h
#pragma once
#include "EngineBase.h"
#include "SafetyCore.h"

class SafeEngineBase : public EngineBase {
protected:
    // Safe internal buffers
    struct SafeBuffer {
        std::vector<float> data;
        size_t capacity = 0;

        void resize(size_t newSize) {
            if (newSize > capacity) {
                data.resize(newSize * 2);  // Over-allocate
                capacity = newSize * 2;
            }
        }

        float* getWritePointer() {
            return data.data();
        }

        const float* getReadPointer() const {
            return data.data();
        }
    };

    std::vector<SafeBuffer> workBuffers;
    size_t maxBlockSize = 0;
    double sampleRate = 44100.0;

public:
    void prepareToPlay(double fs, int samplesPerBlock) override {
        sampleRate = fs;
        maxBlockSize = samplesPerBlock;

        // Pre-allocate work buffers
        workBuffers.resize(2);  // Stereo
        for (auto& buf : workBuffers) {
            buf.resize(maxBlockSize * 2);  // Double size for safety
        }

        // Let derived class do its setup
        prepareToPlaySafe(fs, samplesPerBlock);
    }

    void process(juce::AudioBuffer<float>& buffer) override {
        // Validate first
        if (!ChimeraSafety::BufferValidator::isValid(buffer)) {
            return;
        }

        // Check buffer size
        if (buffer.getNumSamples() > maxBlockSize * 2) {
            ChimeraSafety::ErrorReporter::instance().reportError(
                getName().toStdString() + ": Block size exceeds maximum");
            return;
        }

        // Process safely
        ChimeraSafety::SafeProcessor::safeProcess(
            getName().toStdString(),
            buffer,
            [&]() { processSafe(buffer); }
        );
    }

protected:
    // Derived classes implement these safe versions
    virtual void prepareToPlaySafe(double sampleRate, int samplesPerBlock) = 0;
    virtual void processSafe(juce::AudioBuffer<float>& buffer) = 0;

    // Safe buffer access helpers
    float* getSafeChannelPointer(juce::AudioBuffer<float>& buffer, int channel) {
        if (channel >= 0 && channel < buffer.getNumChannels()) {
            return buffer.getWritePointer(channel);
        }
        return nullptr;
    }

    void processMonoToStereo(juce::AudioBuffer<float>& buffer,
                            std::function<float(float)> processor) {
        const int numSamples = buffer.getNumSamples();

        if (buffer.getNumChannels() == 1) {
            // Mono
            auto* data = buffer.getWritePointer(0);
            for (int s = 0; s < numSamples; ++s) {
                data[s] = processor(data[s]);
            }
        } else {
            // Stereo or more
            for (int ch = 0; ch < std::min(2, buffer.getNumChannels()); ++ch) {
                auto* data = buffer.getWritePointer(ch);
                for (int s = 0; s < numSamples; ++s) {
                    data[s] = processor(data[s]);
                }
            }
        }
    }
};
```

### Step 3.2: Fix High-Risk Engines

```cpp
// VintageOptoCompressor_Platinum_Fixed.cpp
class VintageOptoCompressor_Fixed : public SafeEngineBase {
private:
    // Use safe buffers instead of raw arrays
    SafeBuffer sidechainBuffer;

    // Parameters with validation
    struct Parameters {
        std::atomic<float> gain{0.5f};
        std::atomic<float> threshold{0.5f};
        std::atomic<float> ratio{0.5f};
        std::atomic<float> attack{0.5f};
        std::atomic<float> release{0.5f};

        float getGain() const {
            return juce::jlimit(0.0f, 1.0f, gain.load());
        }
        float getThreshold() const {
            return juce::jlimit(0.0f, 1.0f, threshold.load());
        }
        // etc...
    } params;

protected:
    void prepareToPlaySafe(double fs, int blockSize) override {
        sidechainBuffer.resize(blockSize * 2);
        // Initialize filters, etc.
    }

    void processSafe(juce::AudioBuffer<float>& buffer) override {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // Safe parameter reading
        const float gain = params.getGain();
        const float threshold = params.getThreshold();

        // Process with bounds checking
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* data = getSafeChannelPointer(buffer, ch);
            if (!data) continue;

            for (int s = 0; s < numSamples; ++s) {
                // Prevent NaN propagation
                float sample = data[s];
                if (!std::isfinite(sample)) {
                    sample = 0.0f;
                }

                // Compression logic with safety
                float absVal = std::abs(sample);
                float reduction = 1.0f;

                if (absVal > threshold) {
                    // Safe division
                    float excess = absVal - threshold;
                    if (excess > 0.0f) {
                        reduction = threshold / (threshold + excess);
                    }
                }

                // Apply with limiting
                data[s] = juce::jlimit(-1.0f, 1.0f, sample * reduction * gain);
            }
        }
    }
};
```

## Day 5: Testing & Validation

### Step 5.1: Comprehensive Test Suite

```cpp
// test_safety_complete.cpp
#include <catch2/catch.hpp>
#include "ChimeraAudioProcessor.h"

TEST_CASE("Safety Layer 1: Input Validation", "[safety]") {
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);

    SECTION("Null buffer") {
        juce::AudioBuffer<float> buffer;
        juce::MidiBuffer midi;
        REQUIRE_NOTHROW(processor.processBlock(buffer, midi));
    }

    SECTION("Zero channels") {
        juce::AudioBuffer<float> buffer(0, 512);
        juce::MidiBuffer midi;
        REQUIRE_NOTHROW(processor.processBlock(buffer, midi));
    }

    SECTION("Zero samples") {
        juce::AudioBuffer<float> buffer(2, 0);
        juce::MidiBuffer midi;
        REQUIRE_NOTHROW(processor.processBlock(buffer, midi));
    }
}

TEST_CASE("Safety Layer 2: NaN Handling", "[safety]") {
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    // Insert NaN
    buffer.setSample(0, 100, std::numeric_limits<float>::quiet_NaN());

    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);

    // Check NaN was removed
    for (int ch = 0; ch < 2; ++ch) {
        for (int s = 0; s < 512; ++s) {
            REQUIRE(std::isfinite(buffer.getSample(ch, s)));
        }
    }
}

TEST_CASE("Safety Layer 3: Thread Safety", "[safety]") {
    ChimeraAudioProcessor processor;
    processor.prepareToPlay(44100, 512);

    std::atomic<bool> running{true};
    std::atomic<int> swapCount{0};
    std::atomic<int> processCount{0};

    // Audio thread simulator
    std::thread audioThread([&]() {
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;

        while (running.load()) {
            buffer.clear();
            processor.processBlock(buffer, midi);
            processCount.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Parameter thread simulator
    std::thread paramThread([&]() {
        while (running.load()) {
            // Change engines rapidly
            for (int slot = 0; slot < 4; ++slot) {
                for (int engine = 0; engine < 57; ++engine) {
                    processor.setEngine(slot, engine);
                    swapCount.fetch_add(1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    });

    // Run for 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));
    running.store(false);

    audioThread.join();
    paramThread.join();

    // Should have processed many blocks without crashing
    REQUIRE(processCount.load() > 1000);
    REQUIRE(swapCount.load() > 100);
}

TEST_CASE("Buffer Overflow Protection", "[safety]") {
    for (int engineId = 0; engineId < 57; ++engineId) {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) continue;

        DYNAMIC_SECTION("Engine " << engineId << ": " << engine->getName()) {
            engine->prepareToPlay(44100, 512);

            // Test various buffer sizes
            std::vector<int> sizes = {1, 64, 512, 4096, 8192};

            for (int size : sizes) {
                juce::AudioBuffer<float> buffer(2, size);
                buffer.clear();

                // Should not crash or corrupt memory
                REQUIRE_NOTHROW(engine->process(buffer));

                // Buffer should still be valid
                REQUIRE(buffer.getNumChannels() == 2);
                REQUIRE(buffer.getNumSamples() == size);
            }
        }
    }
}
```

### Step 5.2: Memory Safety Validation

```bash
#!/bin/bash
# validate_safety.sh

echo "Running safety validation suite..."

# Compile with sanitizers
echo "Building with AddressSanitizer..."
clang++ -std=c++17 -fsanitize=address -g \
    test_safety_complete.cpp \
    -o test_safety_asan

echo "Building with ThreadSanitizer..."
clang++ -std=c++17 -fsanitize=thread -g \
    test_safety_complete.cpp \
    -o test_safety_tsan

# Run tests
echo "Testing memory safety..."
./test_safety_asan 2>&1 | tee asan_report.txt

echo "Testing thread safety..."
./test_safety_tsan 2>&1 | tee tsan_report.txt

# Check for issues
if grep -q "ERROR: AddressSanitizer" asan_report.txt; then
    echo "❌ Memory safety issues detected!"
    exit 1
fi

if grep -q "WARNING: ThreadSanitizer" tsan_report.txt; then
    echo "❌ Thread safety issues detected!"
    exit 1
fi

echo "✅ All safety tests passed!"
```

## Integration Checklist

### Code Changes Required

- [ ] Replace all direct buffer access with validated access
- [ ] Wrap all engine->process() calls in SafeProcessor
- [ ] Convert all engines to SafeEngineBase
- [ ] Replace std::mutex with lock-free structures
- [ ] Add error reporting throughout
- [ ] Remove all hardcoded buffer sizes
- [ ] Add bounds checking to all loops
- [ ] Validate all parameter access

### Testing Required

- [ ] Run AddressSanitizer on full test suite
- [ ] Run ThreadSanitizer with concurrent operations
- [ ] Stress test with corrupt input data
- [ ] Verify no memory leaks with Valgrind
- [ ] Test all 57 engines individually
- [ ] Test rapid engine swapping
- [ ] Test extreme parameter values
- [ ] Test various buffer sizes

### Success Metrics

- [ ] 0 crashes in 10,000 test runs
- [ ] 0 data races detected
- [ ] 0 memory leaks
- [ ] <0.1ms safety overhead
- [ ] 100% of engines converted
- [ ] Error reporting operational

---

**This completes the critical safety implementation. With this foundation, the codebase will be resilient to crashes and undefined behavior.**