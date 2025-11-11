# Critical Fix: Error Handling in processBlock

## Issue Overview

The `processBlock()` method in `PluginProcessor.cpp` (lines 580-730) lacks comprehensive error handling, which can lead to crashes when:
- Invalid buffer data is received
- Engine processing throws exceptions
- Parameter values are out of range
- Null pointer access occurs
- NaN or Inf values propagate through the signal chain

### Current State
- **No try-catch blocks** around engine processing
- **No validation** of input buffer state
- **No NaN/Inf checking** before or after processing
- **No graceful degradation** when errors occur
- **Debug file I/O** operations that can fail silently

## System Context

### Architecture
The Phoenix-Chimera plugin processes audio through up to 4 slots, each containing one of 57 DSP engines. The signal flow is:
1. Input buffer → Dry signal copy
2. For each slot (0-3):
   - Check bypass/solo states
   - Get 15 parameters per slot
   - Lock mutex for thread safety
   - Call engine->updateParameters()
   - Call engine->process()
   - Apply wet/dry mix
3. Apply gain compensation
4. Apply output limiting

### Known Problem Areas
- Line 669: `m_activeEngines[slot]->process(wetBuffer)` - No error handling
- Line 668: `m_activeEngines[slot]->updateParameters(params)` - No validation
- Line 586: `buffer.getMagnitude()` - Can throw if buffer invalid
- Lines 649-654: Debug file I/O can fail and leave files open

## Agent Task

### Objective
Add comprehensive error handling to `processBlock()` to prevent crashes and provide graceful degradation when errors occur.

### Requirements
1. **Validate input buffer** before processing
2. **Wrap engine calls** in try-catch blocks
3. **Check for NaN/Inf** at critical points
4. **Implement fallback** behavior when engines fail
5. **Add error reporting** without affecting real-time performance
6. **Ensure RAII** for all resources

### Implementation Steps

1. **First, analyze the current code** in `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/PluginProcessor.cpp`
   - Review lines 580-730 of `processBlock()`
   - Identify all potential failure points
   - Note any existing partial error handling

2. **Create a safe wrapper function** for engine processing:
   ```cpp
   bool safeProcessEngine(int slot, juce::AudioBuffer<float>& buffer,
                          const std::map<int, float>& params);
   ```

3. **Add input validation** at the start of processBlock:
   ```cpp
   // Validate buffer state
   if (!validateAudioBuffer(buffer)) {
       buffer.clear(); // Safe fallback
       return;
   }
   ```

4. **Implement NaN/Inf detection**:
   ```cpp
   // After each engine process
   if (containsNaNOrInf(wetBuffer)) {
       handleCorruptedAudio(wetBuffer, slot);
   }
   ```

5. **Add error recovery mechanism**:
   - Track failed engines
   - Bypass failed engines automatically
   - Log errors to a ring buffer (not file I/O)
   - Optionally reset failed engines

6. **Create unit tests** to verify error handling:
   - Test with null buffers
   - Test with NaN/Inf input
   - Test with exceptions from engines
   - Test with invalid parameters

### Validation Checklist
- [ ] No crashes with corrupt input data
- [ ] Graceful handling of engine exceptions
- [ ] NaN/Inf values don't propagate
- [ ] Failed engines auto-bypass
- [ ] Error reporting doesn't affect performance
- [ ] All file handles properly closed
- [ ] Thread-safe error handling

### Example Implementation Pattern

```cpp
void ChimeraAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer&) {
    // Early validation
    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) {
        return;
    }

    // Protect against NaN/Inf input
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            if (!std::isfinite(data[s])) {
                data[s] = 0.0f;
            }
        }
    }

    try {
        // Main processing...
        for (int slot = 0; slot < NUM_SLOTS; ++slot) {
            if (!safeProcessSlot(slot, buffer)) {
                // Mark slot as failed, continue processing
                m_failedSlots[slot] = true;
            }
        }
    } catch (const std::exception& e) {
        // Log to ring buffer, not file
        logError("processBlock exception: " + std::string(e.what()));
        // Clear output to prevent noise
        buffer.clear();
    }
}
```

## Success Criteria

1. **Robustness**: Plugin never crashes due to bad input or engine failures
2. **Transparency**: Error handling doesn't affect normal operation performance
3. **Debuggability**: Errors are logged for diagnosis without file I/O
4. **Recovery**: Failed engines can be reset without restarting
5. **Safety**: No undefined behavior or memory corruption

## Files to Modify

1. `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/PluginProcessor.cpp`
2. `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/PluginProcessor.h`
3. Create new: `ErrorHandling.h` for error handling utilities

## Testing Approach

1. Create a test harness that feeds corrupt data
2. Use fuzzing to find edge cases
3. Verify with tools like Valgrind/AddressSanitizer
4. Test in actual DAW with stress scenarios

---

**Priority**: 🔴 CRITICAL - This must be fixed before any public release
**Estimated Time**: 1-2 days
**Risk if Not Fixed**: Crashes in production, data loss, reputation damage