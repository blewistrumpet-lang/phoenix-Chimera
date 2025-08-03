# Critical Engine Fixes Required

## Current Status
- Only 1/53 engines working (K-Style Overdrive)
- All other engines output silence
- Test framework is working correctly

## Root Causes

### 1. Missing Dry/Wet Mix
Engines are replacing input with processed output entirely. If processing produces silence initially (like reverbs need time to build up), output is silent.

### 2. Gain Staging Issue
The processBlock applies 0.9x gain after each engine, potentially accumulating to silence.

### 3. Parameter Initialization
Engines may start with parameters that produce no output (e.g., filters with cutoff at 0).

## Quick Fix for All Engines

Add this to each engine's process() method:

```cpp
void process(juce::AudioBuffer<float>& buffer) {
    // Store dry signal
    juce::AudioBuffer<float> wetBuffer(buffer);
    
    // ... existing processing on wetBuffer ...
    
    // Mix dry and wet (50/50 for testing)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* dry = buffer.getWritePointer(ch);
        const float* wet = wetBuffer.getReadPointer(ch);
        
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            dry[s] = dry[s] * 0.5f + wet[s] * 0.5f;
        }
    }
}
```

## Test in Logic Pro

1. Load ChimeraPhoenix on an audio track
2. Play audio through it (not silence)
3. Select each engine in Slot 1
4. Adjust parameters to hear changes

## Engines to Fix First (High Priority)

1. **Classic Compressor** - Should work with proper threshold
2. **Plate Reverb** - Needs dry/wet mix
3. **Parametric EQ** - Needs frequency/gain settings
4. **Tape Echo** - Needs dry/wet mix
5. **Stereo Chorus** - Needs dry/wet mix

## Testing Protocol

For each engine:
1. Set Slot 1 to the engine
2. Set all other slots to Bypass
3. Play a drum loop or music through it
4. Adjust each parameter 1-10
5. Document which parameters affect the sound

## Expected Behavior

- **Bypass**: Audio passes through unchanged ✓
- **K-Style Overdrive**: Adds distortion ✓
- **Classic Compressor**: Reduces dynamic range
- **Plate Reverb**: Adds ambience
- **Parametric EQ**: Changes frequency balance
- **Tape Echo**: Adds delays
- **Stereo Chorus**: Adds width and modulation

## Next Steps

1. Test in Logic Pro with real audio (not test signals)
2. Add dry/wet mix to 5 high-priority engines
3. Fix parameter initialization
4. Re-test with comprehensive test suite
5. Document working engines