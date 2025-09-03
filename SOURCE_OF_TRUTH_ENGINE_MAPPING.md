# SOURCE OF TRUTH - Chimera Phoenix v3.0 Engine & Parameter Mapping

## Official Sources
1. **Engine IDs**: `/JUCE_Plugin/Source/EngineTypes.h` (marked as "Single source of truth")
2. **Factory Implementation**: `/JUCE_Plugin/Source/EngineFactory.cpp`
3. **APVTS Parameters**: `/JUCE_Plugin/Source/PluginProcessor.cpp`

## Plugin Architecture
- **Name**: Chimera v3.0
- **Slots**: 6 effect slots (serial processing chain)
- **Parameters per slot**: 15
- **Total parameters**: 102
  - 90 effect parameters (6 slots × 15)
  - 6 engine selection parameters
  - 6 master/utility parameters

## Complete Engine Mapping (57 Total: 0 + 56 effects)

### ID 0: Special
- `ENGINE_NONE` - None/Bypass

### IDs 1-6: DYNAMICS & COMPRESSION
- `ENGINE_OPTO_COMPRESSOR` (1) - Vintage Opto Compressor
- `ENGINE_VCA_COMPRESSOR` (2) - VCA/Classic Compressor
- `ENGINE_TRANSIENT_SHAPER` (3) - Transient Shaper
- `ENGINE_NOISE_GATE` (4) - Noise Gate
- `ENGINE_MASTERING_LIMITER` (5) - Mastering Limiter
- `ENGINE_DYNAMIC_EQ` (6) - Dynamic EQ

### IDs 7-14: FILTERS & EQ
- `ENGINE_PARAMETRIC_EQ` (7) - Parametric EQ
- `ENGINE_VINTAGE_CONSOLE_EQ` (8) - Vintage Console EQ
- `ENGINE_LADDER_FILTER` (9) - Ladder Filter
- `ENGINE_STATE_VARIABLE_FILTER` (10) - State Variable Filter
- `ENGINE_FORMANT_FILTER` (11) - Formant Filter
- `ENGINE_ENVELOPE_FILTER` (12) - Envelope Filter
- `ENGINE_COMB_RESONATOR` (13) - Comb Resonator
- `ENGINE_VOCAL_FORMANT` (14) - Vocal Formant Filter

### IDs 15-22: DISTORTION & SATURATION
- `ENGINE_VINTAGE_TUBE` (15) - Vintage Tube Preamp
- `ENGINE_WAVE_FOLDER` (16) - Wave Folder
- `ENGINE_HARMONIC_EXCITER` (17) - Harmonic Exciter
- `ENGINE_BIT_CRUSHER` (18) - Bit Crusher
- `ENGINE_MULTIBAND_SATURATOR` (19) - Multiband Saturator
- `ENGINE_MUFF_FUZZ` (20) - Muff Fuzz
- `ENGINE_RODENT_DISTORTION` (21) - Rodent Distortion
- `ENGINE_K_STYLE` (22) - K-Style Overdrive

### IDs 23-33: MODULATION EFFECTS
- `ENGINE_DIGITAL_CHORUS` (23) - Digital/Stereo Chorus
- `ENGINE_RESONANT_CHORUS` (24) - Resonant Chorus
- `ENGINE_ANALOG_PHASER` (25) - Analog Phaser
- `ENGINE_RING_MODULATOR` (26) - Ring Modulator
- `ENGINE_FREQUENCY_SHIFTER` (27) - Frequency Shifter
- `ENGINE_HARMONIC_TREMOLO` (28) - Harmonic Tremolo
- `ENGINE_CLASSIC_TREMOLO` (29) - Classic Tremolo
- `ENGINE_ROTARY_SPEAKER` (30) - Rotary Speaker
- `ENGINE_PITCH_SHIFTER` (31) - Pitch Shifter
- `ENGINE_DETUNE_DOUBLER` (32) - Detune Doubler
- `ENGINE_INTELLIGENT_HARMONIZER` (33) - Intelligent Harmonizer

### IDs 34-43: REVERB & DELAY
- `ENGINE_TAPE_ECHO` (34) - Tape Echo
- `ENGINE_DIGITAL_DELAY` (35) - Digital Delay
- `ENGINE_MAGNETIC_DRUM_ECHO` (36) - Magnetic Drum Echo
- `ENGINE_BUCKET_BRIGADE_DELAY` (37) - Bucket Brigade Delay
- `ENGINE_BUFFER_REPEAT` (38) - Buffer Repeat
- `ENGINE_PLATE_REVERB` (39) - Plate Reverb
- `ENGINE_SPRING_REVERB` (40) - Spring Reverb
- `ENGINE_CONVOLUTION_REVERB` (41) - Convolution Reverb
- `ENGINE_SHIMMER_REVERB` (42) - Shimmer Reverb
- `ENGINE_GATED_REVERB` (43) - Gated Reverb

### IDs 44-52: SPATIAL & SPECIAL EFFECTS
- `ENGINE_STEREO_WIDENER` (44) - Stereo Widener
- `ENGINE_STEREO_IMAGER` (45) - Stereo Imager
- `ENGINE_DIMENSION_EXPANDER` (46) - Dimension Expander
- `ENGINE_SPECTRAL_FREEZE` (47) - Spectral Freeze
- `ENGINE_SPECTRAL_GATE` (48) - Spectral Gate
- `ENGINE_PHASED_VOCODER` (49) - Phased Vocoder
- `ENGINE_GRANULAR_CLOUD` (50) - Granular Cloud
- `ENGINE_CHAOS_GENERATOR` (51) - Chaos Generator ✅ (You were right!)
- `ENGINE_FEEDBACK_NETWORK` (52) - Feedback Network

### IDs 53-56: UTILITY
- `ENGINE_MID_SIDE_PROCESSOR` (53) - Mid-Side Processor
- `ENGINE_GAIN_UTILITY` (54) - Gain Utility
- `ENGINE_MONO_MAKER` (55) - Mono Maker
- `ENGINE_PHASE_ALIGN` (56) - Phase Align

## APVTS Parameter Structure

### Per Slot (17 parameters each):
```
slot[N]_param1    - Parameter 1 (mapped to engine's param index 0)
slot[N]_param2    - Parameter 2 (mapped to engine's param index 1)
...
slot[N]_param15   - Parameter 15 (mapped to engine's param index 14)
slot[N]_engine    - Engine selection (0-56)
slot[N]_bypass    - Slot bypass (true/false)
```

### Parameter Mapping Implementation:
The PluginProcessor::updateEngineParameters() function maps APVTS parameters to engine parameters:
- APVTS `slot[N]_param1` → Engine parameter index 0
- APVTS `slot[N]_param2` → Engine parameter index 1
- ... and so on up to index 14

### Parameter Mapping Example:
When ShimmerReverb (ID 42) is loaded into Slot 1:
- `slot1_param1` → ShimmerReverb param 0 ("Size")
- `slot1_param2` → ShimmerReverb param 1 ("Shimmer")
- `slot1_param3` → ShimmerReverb param 2 ("Pitch")
- etc.

### Master Parameters:
- Global bypass
- Global mix
- Input gain
- Output gain
- CPU limit
- Quality mode

### Verified APVTS Implementation (PluginProcessor.cpp):
✅ Engine choices array contains all 57 engines (0-56)
✅ Direct 1:1 mapping where engine ID = dropdown index
✅ Each slot has 15 parameter floats (0.0-1.0 range, 0.5 default)
✅ Each slot has engine selector (choice 0-56)
✅ Each slot has bypass switch (bool)

## Implementation Status (from our tests):
- ✅ 52+ engines confirmed working
- ✅ ChaosGenerator (51) confirmed implemented
- ✅ Parameter mapping system working
- ⚠️ HarmonicExciter (17) - parameters not affecting output
- ⚠️ KStyleOverdrive (22) - parameters not affecting output

## Notes:
- Engine IDs are DIRECT - no translation needed
- Choice index in dropdown = Engine ID
- All engines inherit from `EngineBase` class
- Each engine implements its own `updateParameters()` mapping