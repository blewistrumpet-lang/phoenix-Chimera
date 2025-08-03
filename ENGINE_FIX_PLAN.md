# Chimera Engine Fix Plan

## Current Issues Identified

### 1. Audio Processing Chain Problem
- Engines are processing in series through all 6 slots
- Gain reduction is being applied incorrectly
- Bypass engines still affect the signal path

### 2. Parameter Mapping Issues
- Parameters are 1-based in UI (slot1_param1 through slot1_param10)
- Parameters are 0-based in engines (index 0-9)
- Mix parameter (usually index 3) is not being set correctly for many engines

### 3. Missing Dry/Wet Mix Implementation
Most engines process audio in-place without preserving the dry signal. They need:
- Store dry signal before processing
- Process to create wet signal
- Mix based on a mix parameter

## Required Fixes

### Fix 1: Update Each Engine Class
Each engine needs to implement proper dry/wet mixing:

```cpp
void process(juce::AudioBuffer<float>& buffer) {
    // Store dry signal
    juce::AudioBuffer<float> dryBuffer(buffer);
    
    // Process wet signal
    // ... existing processing code ...
    
    // Mix dry and wet based on mix parameter
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* wet = buffer.getWritePointer(ch);
        const float* dry = dryBuffer.getReadPointer(ch);
        
        for (int s = 0; s < buffer.getNumSamples(); ++s) {
            wet[s] = dry[s] * (1.0f - mixAmount) + wet[s] * mixAmount;
        }
    }
}
```

### Fix 2: Correct Parameter Defaults
Update PluginProcessor.cpp applyDefaultParameters() to set correct indices:
- Most effects expect mix at parameter index 3 (param4 in UI)
- Ensure all parameters are initialized to sensible defaults

### Fix 3: Fix Audio Processing Chain
Update processBlock() to handle bypass engines correctly and avoid cumulative gain reduction.

## Testing Requirements

### In-DAW Testing Protocol
1. Load ChimeraPhoenix in Logic Pro
2. Create an audio track with test signal (sine wave, drums, etc.)
3. For each engine:
   - Select engine in Slot 1
   - Set all other slots to Bypass
   - Sweep each parameter from 0 to 1
   - Verify audio changes appropriately
   - Document working/non-working status

### Expected Behavior by Engine Type

#### Dynamics (Compressors, Gates, Limiters)
- Should reduce gain on loud signals
- Attack/Release should affect timing
- Threshold should set trigger point

#### Filters (EQ, Filters)
- Should change frequency content
- Cutoff should sweep frequency
- Resonance should add emphasis

#### Time-Based (Reverbs, Delays)
- Should add echoes/ambience
- Time/Size parameters should change delay/decay
- Feedback should increase repetitions

#### Modulation (Chorus, Phaser, Tremolo)
- Should add movement/animation
- Rate should control speed
- Depth should control intensity

#### Distortion (Overdrive, Fuzz)
- Should add harmonics/saturation
- Drive should increase distortion
- Tone should affect brightness

## Implementation Priority

1. **High Priority** (Core effects users expect):
   - Classic Compressor
   - Parametric EQ
   - Plate Reverb
   - Tape Echo
   - K-Style Overdrive
   - Stereo Chorus

2. **Medium Priority** (Popular effects):
   - Vintage Tube Preamp
   - Analog Phaser
   - Classic Tremolo
   - Ladder Filter
   - Shimmer Reverb
   - Bit Crusher

3. **Low Priority** (Specialty effects):
   - Spectral Freeze
   - Granular Cloud
   - Chaos Generator
   - Feedback Network

## Verification Checklist

For each engine, verify:
- [ ] Audio passes through when bypassed
- [ ] Audio is affected when active
- [ ] Each parameter changes the sound
- [ ] Mix/blend control works (dry/wet)
- [ ] No clicks, pops, or artifacts
- [ ] CPU usage is reasonable
- [ ] Presets sound musical

## Next Steps

1. Test the plugin in Logic Pro to verify basic functionality
2. Implement dry/wet mixing in high-priority engines
3. Fix parameter mapping issues
4. Test each engine systematically
5. Create presets that showcase each engine