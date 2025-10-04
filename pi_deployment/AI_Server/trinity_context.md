# Trinity Context - Complete Engine & Parameter Documentation

**Version:** 3.0
**Last Updated:** 2025-09-19
**Status:** PRODUCTION - Verified against C++ source

This document is the authoritative knowledge base for the Trinity AI pipeline.
All parameter descriptions have been verified against UnifiedDefaultParameters.cpp


## Engine Definitions (57 Total)

Each engine has up to 16 parameters (param0-param15).
Parameter counts and descriptions are exact matches to the C++ implementation.


### ENGINE_NONE (ID: 0)
**Name:** None
**Category:** Special
**Parameter Count:** 0

**Parameters:** None (bypass)


### ENGINE_VINTAGE_OPTO_COMPRESSOR (ID: 1)
**Name:** Vintage Opto Compressor
**Category:** Dynamics & Compression
**Parameter Count:** 8

**Parameters:**
- param0: Input Gain (-∞ to +12dB)
- param1: Peak Reduction (0.0 to 1.0)
- param2: HF Emphasis (0.0 to 1.0)
- param3: Output Gain (-∞ to +12dB)
- param4: Mix (0% to 100%)
- param5: Knee (0.0 to 1.0)
- param6: Tube Harmonics (0.0 to 1.0)
- param7: Stereo Link (0.0 to 1.0)


### ENGINE_CLASSIC_COMPRESSOR (ID: 2)
**Name:** Classic Compressor
**Category:** Dynamics & Compression
**Parameter Count:** 7

**Parameters:**
- param0: Threshold (-60dB to 0dB)
- param1: Ratio (1:1 to 20:1)
- param2: Attack (0.1ms to 1000ms)
- param3: Release (0.1ms to 1000ms)
- param4: Knee (0.0 to 1.0)
- param5: Makeup Gain (-∞ to +12dB)
- param6: Mix (0% to 100%)


### ENGINE_TRANSIENT_SHAPER (ID: 3)
**Name:** Transient Shaper
**Category:** Dynamics & Compression
**Parameter Count:** 4

**Parameters:**
- param0: Attack (0.1ms to 1000ms)
- param1: Sustain (0.0 to 1.0)
- param2: Sensitivity (0.0 to 1.0)
- param3: Output (0.0 to 1.0)


### ENGINE_NOISE_GATE (ID: 4)
**Name:** Noise Gate
**Category:** Dynamics & Compression
**Parameter Count:** 5

**Parameters:**
- param0: Threshold (-60dB to 0dB)
- param1: Attack (0.1ms to 1000ms)
- param2: Hold (0.0 to 1.0)
- param3: Release (0.1ms to 1000ms)
- param4: Range (0.0 to 1.0)


### ENGINE_MASTERING_LIMITER (ID: 5)
**Name:** Mastering Limiter
**Category:** Dynamics & Compression
**Parameter Count:** 4

**Parameters:**
- param0: Threshold (-60dB to 0dB)
- param1: Release (0.1ms to 1000ms)
- param2: Knee (0.0 to 1.0)
- param3: Lookahead (0.0 to 1.0)


### ENGINE_DYNAMIC_EQ (ID: 6)
**Name:** Dynamic EQ
**Category:** Dynamics & Compression
**Parameter Count:** 8

**Parameters:**
- param0: Frequency (20Hz to 20kHz)
- param1: Threshold (-60dB to 0dB)
- param2: Ratio (1:1 to 20:1)
- param3: Attack (0.1ms to 1000ms)
- param4: Release (0.1ms to 1000ms)
- param5: Gain (-∞ to +12dB)
- param6: Mix (0% to 100%)
- param7: Mode (0.0 to 1.0)


### ENGINE_PARAMETRIC_EQ (ID: 7)
**Name:** Parametric EQ
**Category:** Filters & EQ
**Parameter Count:** 9

**Parameters:**
- param0: Low Frequency (20Hz to 20kHz)
- param1: Low Gain (-∞ to +12dB)
- param2: Low Q (0.1 to 20)
- param3: Mid Frequency (20Hz to 20kHz)
- param4: Mid Gain (-∞ to +12dB)
- param5: Mid Q (0.1 to 20)
- param6: High Frequency (20Hz to 20kHz)
- param7: High Gain (-∞ to +12dB)
- param8: High Q (0.1 to 20)


### ENGINE_VINTAGE_CONSOLE_EQ (ID: 8)
**Name:** Vintage Console EQ
**Category:** Filters & EQ
**Parameter Count:** 7

**Parameters:**
- param0: Low Shelf (0.0 to 1.0)
- param1: Low Mid Frequency (20Hz to 20kHz)
- param2: Low Mid Gain (-∞ to +12dB)
- param3: High Mid Frequency (20Hz to 20kHz)
- param4: High Mid Gain (-∞ to +12dB)
- param5: High Shelf (0.0 to 1.0)
- param6: Drive (0% to 100%)


### ENGINE_LADDER_FILTER (ID: 9)
**Name:** Ladder Filter
**Category:** Filters & EQ
**Parameter Count:** 6

**Parameters:**
- param0: Cutoff (0.0 to 1.0)
- param1: Resonance (0.1 to 20)
- param2: Drive (0% to 100%)
- param3: Mode (0.0 to 1.0)
- param4: Envelope Amount (0% to 100%)
- param5: Mix (0% to 100%)


### ENGINE_STATE_VARIABLE_FILTER (ID: 10)
**Name:** State Variable Filter
**Category:** Filters & EQ
**Parameter Count:** 4

**Parameters:**
- param0: Frequency (20Hz to 20kHz)
- param1: Resonance (0.1 to 20)
- param2: Mode (0.0 to 1.0)
- param3: Mix (0% to 100%)


### ENGINE_FORMANT_FILTER (ID: 11)
**Name:** Formant Filter
**Category:** Filters & EQ
**Parameter Count:** 3

**Parameters:**
- param0: Formant (0.0 to 1.0)
- param1: Size (0.0 to 1.0)
- param2: Mix (0% to 100%)


### ENGINE_ENVELOPE_FILTER (ID: 12)
**Name:** Envelope Filter
**Category:** Filters & EQ
**Parameter Count:** 6

**Parameters:**
- param0: Sensitivity (0.0 to 1.0)
- param1: Range (0.0 to 1.0)
- param2: Resonance (0.1 to 20)
- param3: Attack (0.1ms to 1000ms)
- param4: Release (0.1ms to 1000ms)
- param5: Mix (0% to 100%)


### ENGINE_COMB_RESONATOR (ID: 13)
**Name:** Comb Resonator
**Category:** Filters & EQ
**Parameter Count:** 4

**Parameters:**
- param0: Frequency (20Hz to 20kHz)
- param1: Feedback (0% to 95%)
- param2: Damping (0.0 to 1.0)
- param3: Mix (0% to 100%)


### ENGINE_VOCAL_FORMANT_FILTER (ID: 14)
**Name:** Vocal Formant Filter
**Category:** Filters & EQ
**Parameter Count:** 4

**Parameters:**
- param0: Vowel (0.0 to 1.0)
- param1: Size (0.0 to 1.0)
- param2: Brightness (0.0 to 1.0)
- param3: Mix (0% to 100%)


### ENGINE_VINTAGE_TUBE_PREAMP (ID: 15)
**Name:** Vintage Tube Preamp
**Category:** Distortion & Saturation
**Parameter Count:** 8

**Parameters:**
- param0: Input Gain (-∞ to +12dB)
- param1: Drive (0% to 100%)
- param2: Bias (0.0 to 1.0)
- param3: Bass (0.0 to 1.0)
- param4: Mid (0.0 to 1.0)
- param5: Treble (0.0 to 1.0)
- param6: Presence (0.0 to 1.0)
- param7: Output Gain (-∞ to +12dB)


### ENGINE_WAVE_FOLDER (ID: 16)
**Name:** Wave Folder
**Category:** Distortion & Saturation
**Parameter Count:** 5

**Parameters:**
- param0: Drive (0% to 100%)
- param1: Fold Amount (0% to 100%)
- param2: Symmetry (0.0 to 1.0)
- param3: Output (0.0 to 1.0)
- param4: Mix (0% to 100%)


### ENGINE_HARMONIC_EXCITER (ID: 17)
**Name:** Harmonic Exciter
**Category:** Distortion & Saturation
**Parameter Count:** 3

**Parameters:**
- param0: Harmonics (0.0 to 1.0)
- param1: Frequency (20Hz to 20kHz)
- param2: Mix (0% to 100%)


### ENGINE_BIT_CRUSHER (ID: 18)
**Name:** Bit Crusher
**Category:** Distortion & Saturation
**Parameter Count:** 3

**Parameters:**
- param0: Bits (1 to 16 bits)
- param1: Downsample (0.0 to 1.0)
- param2: Mix (0% to 100%)


### ENGINE_MULTIBAND_SATURATOR (ID: 19)
**Name:** Multiband Saturator
**Category:** Distortion & Saturation
**Parameter Count:** 6

**Parameters:**
- param0: Low Drive (0% to 100%)
- param1: Mid Drive (0% to 100%)
- param2: High Drive (0% to 100%)
- param3: Low Mix (0% to 100%)
- param4: Mid Mix (0% to 100%)
- param5: High Mix (0% to 100%)


### ENGINE_MUFF_FUZZ (ID: 20)
**Name:** Muff Fuzz
**Category:** Distortion & Saturation
**Parameter Count:** 3

**Parameters:**
- param0: Sustain (0.0 to 1.0)
- param1: Tone (0.0 to 1.0)
- param2: Volume (-∞ to +12dB)


### ENGINE_RODENT_DISTORTION (ID: 21)
**Name:** Rodent Distortion
**Category:** Distortion & Saturation
**Parameter Count:** 3

**Parameters:**
- param0: Distortion (0% to 100%)
- param1: Filter (0.0 to 1.0)
- param2: Volume (-∞ to +12dB)


### ENGINE_K_STYLE_OVERDRIVE (ID: 22)
**Name:** K-Style Overdrive
**Category:** Distortion & Saturation
**Parameter Count:** 3

**Parameters:**
- param0: Drive (0% to 100%)
- param1: Tone (0.0 to 1.0)
- param2: Level (-∞ to +12dB)


### ENGINE_DIGITAL_CHORUS (ID: 23)
**Name:** Digital Chorus
**Category:** Modulation Effects
**Parameter Count:** 5

**Parameters:**
- param0: Rate (0.01Hz to 20Hz)
- param1: Depth (0% to 100%)
- param2: Delay (1ms to 5000ms)
- param3: Feedback (0% to 95%)
- param4: Mix (0% to 100%)


### ENGINE_RESONANT_CHORUS (ID: 24)
**Name:** Resonant Chorus
**Category:** Modulation Effects
**Parameter Count:** 7

**Parameters:**
- param0: Rate (0.01Hz to 20Hz)
- param1: Depth (0% to 100%)
- param2: Resonance (0.1 to 20)
- param3: Width (0.0 to 1.0)
- param4: Feedback (0% to 95%)
- param5: HP Filter (0.0 to 1.0)
- param6: Mix (0% to 100%)


### ENGINE_ANALOG_PHASER (ID: 25)
**Name:** Analog Phaser
**Category:** Modulation Effects
**Parameter Count:** 7

**Parameters:**
- param0: Rate (0.01Hz to 20Hz)
- param1: Depth (0% to 100%)
- param2: Feedback (0% to 95%)
- param3: Stages (0.0 to 1.0)
- param4: Frequency (20Hz to 20kHz)
- param5: Width (0.0 to 1.0)
- param6: Mix (0% to 100%)


### ENGINE_RING_MODULATOR (ID: 26)
**Name:** Ring Modulator
**Category:** Modulation Effects
**Parameter Count:** 3

**Parameters:**
- param0: Frequency (20Hz to 20kHz)
- param1: Fine Tune (0.0 to 1.0)
- param2: Mix (0% to 100%)


### ENGINE_FREQUENCY_SHIFTER (ID: 27)
**Name:** Frequency Shifter
**Category:** Modulation Effects
**Parameter Count:** 3

**Parameters:**
- param0: Shift Amount (0% to 100%)
- param1: Fine Tune (0.0 to 1.0)
- param2: Mix (0% to 100%)


### ENGINE_HARMONIC_TREMOLO (ID: 28)
**Name:** Harmonic Tremolo
**Category:** Modulation Effects
**Parameter Count:** 5

**Parameters:**
- param0: Rate (0.01Hz to 20Hz)
- param1: Depth (0% to 100%)
- param2: Crossover Frequency (20Hz to 20kHz)
- param3: LFO Shape (0.0 to 1.0)
- param4: Mix (0% to 100%)


### ENGINE_CLASSIC_TREMOLO (ID: 29)
**Name:** Classic Tremolo
**Category:** Modulation Effects
**Parameter Count:** 4

**Parameters:**
- param0: Rate (0.01Hz to 20Hz)
- param1: Depth (0% to 100%)
- param2: Shape (0.0 to 1.0)
- param3: Mix (0% to 100%)


### ENGINE_ROTARY_SPEAKER (ID: 30)
**Name:** Rotary Speaker
**Category:** Modulation Effects
**Parameter Count:** 5

**Parameters:**
- param0: Speed (0.0 to 1.0)
- param1: Doppler (0.0 to 1.0)
- param2: Horn/Drum Mix (0% to 100%)
- param3: Distance (0.0 to 1.0)
- param4: Mix (0% to 100%)


### ENGINE_PITCH_SHIFTER (ID: 31)
**Name:** Pitch Shifter
**Category:** Modulation Effects
**Parameter Count:** 4

**Parameters:**
- param0: Pitch (-24 to +24 semitones)
- param1: Fine Tune (0.0 to 1.0)
- param2: Formant (0.0 to 1.0)
- param3: Mix (0% to 100%)


### ENGINE_DETUNE_DOUBLER (ID: 32)
**Name:** Detune Doubler
**Category:** Modulation Effects
**Parameter Count:** 5

**Parameters:**
- param0: Detune 1 (0.0 to 1.0)
- param1: Detune 2 (0.0 to 1.0)
- param2: Delay (1ms to 5000ms)
- param3: Width (0.0 to 1.0)
- param4: Mix (0% to 100%)


### ENGINE_INTELLIGENT_HARMONIZER (ID: 33)
**Name:** Intelligent Harmonizer
**Category:** Modulation Effects
**Parameter Count:** 7

**Parameters:**
- param0: Interval 1 (0.0 to 1.0)
- param1: Interval 2 (0.0 to 1.0)
- param2: Key (0.0 to 1.0)
- param3: Scale (0.0 to 1.0)
- param4: Mix 1 (0% to 100%)
- param5: Mix 2 (0% to 100%)
- param6: Feedback (0% to 95%)


### ENGINE_TAPE_ECHO (ID: 34)
**Name:** Tape Echo
**Category:** Reverb & Delay
**Parameter Count:** 6

**Parameters:**
- param0: Delay Time (1ms to 5000ms)
- param1: Feedback (0% to 95%)
- param2: Wow (0.0 to 1.0)
- param3: Flutter (0.0 to 1.0)
- param4: Saturation (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_DIGITAL_DELAY (ID: 35)
**Name:** Digital Delay
**Category:** Reverb & Delay
**Parameter Count:** 6

**Parameters:**
- param0: Delay Time (1ms to 5000ms)
- param1: Feedback (0% to 95%)
- param2: HP Filter (0.0 to 1.0)
- param3: LP Filter (0.0 to 1.0)
- param4: Modulation (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_MAGNETIC_DRUM_ECHO (ID: 36)
**Name:** Magnetic Drum Echo
**Category:** Reverb & Delay
**Parameter Count:** 5

**Parameters:**
- param0: Delay Time (1ms to 5000ms)
- param1: Feedback (0% to 95%)
- param2: Drum Speed (0.0 to 1.0)
- param3: Age (0.0 to 1.0)
- param4: Mix (0% to 100%)


### ENGINE_BUCKET_BRIGADE_DELAY (ID: 37)
**Name:** Bucket Brigade Delay
**Category:** Reverb & Delay
**Parameter Count:** 6

**Parameters:**
- param0: Delay Time (1ms to 5000ms)
- param1: Feedback (0% to 95%)
- param2: Clock Noise (0.0 to 1.0)
- param3: LP Filter (0.0 to 1.0)
- param4: Modulation (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_BUFFER_REPEAT (ID: 38)
**Name:** Buffer Repeat
**Category:** Reverb & Delay
**Parameter Count:** 5

**Parameters:**
- param0: Buffer Size (0.0 to 1.0)
- param1: Pitch (-24 to +24 semitones)
- param2: Reverse (0.0 to 1.0)
- param3: Feedback (0% to 95%)
- param4: Mix (0% to 100%)


### ENGINE_PLATE_REVERB (ID: 39)
**Name:** Plate Reverb
**Category:** Reverb & Delay
**Parameter Count:** 6

**Parameters:**
- param0: Size (0.0 to 1.0)
- param1: Decay (0.0 to 1.0)
- param2: Damping (0.0 to 1.0)
- param3: Pre-Delay (1ms to 5000ms)
- param4: Modulation (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_SPRING_REVERB (ID: 40)
**Name:** Spring Reverb
**Category:** Reverb & Delay
**Parameter Count:** 6

**Parameters:**
- param0: Tension (0.0 to 1.0)
- param1: Springs (0.0 to 1.0)
- param2: Damping (0.0 to 1.0)
- param3: Boing (0.0 to 1.0)
- param4: Tone (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_CONVOLUTION_REVERB (ID: 41)
**Name:** Convolution Reverb
**Category:** Reverb & Delay
**Parameter Count:** 5

**Parameters:**
- param0: IR Selection (0.0 to 1.0)
- param1: Size (0.0 to 1.0)
- param2: Pre-Delay (1ms to 5000ms)
- param3: Damping (0.0 to 1.0)
- param4: Mix (0% to 100%)


### ENGINE_SHIMMER_REVERB (ID: 42)
**Name:** Shimmer Reverb
**Category:** Reverb & Delay
**Parameter Count:** 6

**Parameters:**
- param0: Size (0.0 to 1.0)
- param1: Decay (0.0 to 1.0)
- param2: Shimmer (0.0 to 1.0)
- param3: Pitch (-24 to +24 semitones)
- param4: Damping (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_GATED_REVERB (ID: 43)
**Name:** Gated Reverb
**Category:** Reverb & Delay
**Parameter Count:** 5

**Parameters:**
- param0: Size (0.0 to 1.0)
- param1: Gate Time (1ms to 5000ms)
- param2: Pre-Delay (1ms to 5000ms)
- param3: Damping (0.0 to 1.0)
- param4: Mix (0% to 100%)


### ENGINE_STEREO_WIDENER (ID: 44)
**Name:** Stereo Widener
**Category:** Spatial & Special Effects
**Parameter Count:** 4

**Parameters:**
- param0: Width (0.0 to 1.0)
- param1: Bass Mono (0.0 to 1.0)
- param2: High Frequency Width (20Hz to 20kHz)
- param3: Mix (0% to 100%)


### ENGINE_STEREO_IMAGER (ID: 45)
**Name:** Stereo Imager
**Category:** Spatial & Special Effects
**Parameter Count:** 6

**Parameters:**
- param0: Low Width (0.0 to 1.0)
- param1: Mid Width (0.0 to 1.0)
- param2: High Width (0.0 to 1.0)
- param3: Low Freq (20Hz to 20kHz)
- param4: High Freq (20Hz to 20kHz)
- param5: Mix (0% to 100%)


### ENGINE_DIMENSION_EXPANDER (ID: 46)
**Name:** Dimension Expander
**Category:** Spatial & Special Effects
**Parameter Count:** 4

**Parameters:**
- param0: Size (0.0 to 1.0)
- param1: Amount (0% to 100%)
- param2: Brightness (0.0 to 1.0)
- param3: Mix (0% to 100%)


### ENGINE_SPECTRAL_FREEZE (ID: 47)
**Name:** Spectral Freeze
**Category:** Spatial & Special Effects
**Parameter Count:** 4

**Parameters:**
- param0: Freeze (0.0 to 1.0)
- param1: Smoothness (0.0 to 1.0)
- param2: Spectral Shift (0.0 to 1.0)
- param3: Mix (0% to 100%)


### ENGINE_SPECTRAL_GATE (ID: 48)
**Name:** Spectral Gate
**Category:** Spatial & Special Effects
**Parameter Count:** 5

**Parameters:**
- param0: Threshold (-60dB to 0dB)
- param1: Attack (0.1ms to 1000ms)
- param2: Release (0.1ms to 1000ms)
- param3: Frequency Range (20Hz to 20kHz)
- param4: Mix (0% to 100%)


### ENGINE_PHASED_VOCODER (ID: 49)
**Name:** Phased Vocoder
**Category:** Spatial & Special Effects
**Parameter Count:** 5

**Parameters:**
- param0: Bands (0.0 to 1.0)
- param1: Formant Shift (0.0 to 1.0)
- param2: Pitch Shift (-24 to +24 semitones)
- param3: Time Stretch (1ms to 5000ms)
- param4: Mix (0% to 100%)


### ENGINE_GRANULAR_CLOUD (ID: 50)
**Name:** Granular Cloud
**Category:** Spatial & Special Effects
**Parameter Count:** 6

**Parameters:**
- param0: Grain Size (0.0 to 1.0)
- param1: Position (0.0 to 1.0)
- param2: Density (0.0 to 1.0)
- param3: Pitch Variance (-24 to +24 semitones)
- param4: Reverse (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_CHAOS_GENERATOR (ID: 51)
**Name:** Chaos Generator
**Category:** Spatial & Special Effects
**Parameter Count:** 5

**Parameters:**
- param0: Chaos Amount (0% to 100%)
- param1: Rate (0.01Hz to 20Hz)
- param2: Smoothness (0.0 to 1.0)
- param3: Feedback (0% to 95%)
- param4: Mix (0% to 100%)


### ENGINE_FEEDBACK_NETWORK (ID: 52)
**Name:** Feedback Network
**Category:** Spatial & Special Effects
**Parameter Count:** 6

**Parameters:**
- param0: Feedback (0% to 95%)
- param1: Delay 1 (1ms to 5000ms)
- param2: Delay 2 (1ms to 5000ms)
- param3: Delay 3 (1ms to 5000ms)
- param4: Filter (0.0 to 1.0)
- param5: Mix (0% to 100%)


### ENGINE_MID_SIDE_PROCESSOR (ID: 53)
**Name:** Mid-Side Processor
**Category:** Utility
**Parameter Count:** 5

**Parameters:**
- param0: Mid Level (-∞ to +12dB)
- param1: Side Level (-∞ to +12dB)
- param2: Mid EQ (0.1 to 20)
- param3: Side EQ (0.1 to 20)
- param4: Width (0.0 to 1.0)


### ENGINE_GAIN_UTILITY (ID: 54)
**Name:** Gain Utility
**Category:** Utility
**Parameter Count:** 4

**Parameters:**
- param0: Input Gain (-∞ to +12dB)
- param1: Output Gain (-∞ to +12dB)
- param2: Pan (0.0 to 1.0)
- param3: Phase Invert (0.0 to 1.0)


### ENGINE_MONO_MAKER (ID: 55)
**Name:** Mono Maker
**Category:** Utility
**Parameter Count:** 3

**Parameters:**
- param0: Frequency (20Hz to 20kHz)
- param1: Amount (0% to 100%)
- param2: Phase Coherence (0.0 to 1.0)


### ENGINE_PHASE_ALIGN (ID: 56)
**Name:** Phase Align
**Category:** Utility
**Parameter Count:** 4

**Parameters:**
- param0: Delay (1ms to 5000ms)
- param1: Phase Rotation (0.0 to 1.0)
- param2: All-Pass Filter (0.0 to 1.0)
- param3: Mix (0% to 100%)


## System Metadata

### Slot Configuration
- **Total Slots:** 6
- **Parameters per Slot:** 16 (param0-param15)
- **Total Parameters:** 96 (6 slots × 16 params)

### Parameter Naming Convention
- Format: `slot{N}_param{M}` where N=[1-6], M=[0-15]
- Example: `slot3_param7` = Slot 3, Parameter 7

### Engine Categories
1. **Dynamics & Compression** (IDs 1-6): Control dynamics, compression, limiting
2. **Filters & EQ** (IDs 7-14): Frequency shaping and filtering
3. **Distortion & Saturation** (IDs 15-22): Harmonic generation and saturation
4. **Modulation Effects** (IDs 23-33): Time-based modulation and pitch effects
5. **Reverb & Delay** (IDs 34-43): Spatial and time-based effects
6. **Spatial & Special Effects** (IDs 44-52): Advanced spatial and experimental
7. **Utility** (IDs 53-56): Technical utilities and processing

### Validation Rules
- Engine IDs must be in range [0, 56]
- Parameter values must be in range [0.0, 1.0]
- Empty slots should use ENGINE_NONE (ID: 0)
- All slots must have an engine_id, even if 0

### Source Files
- **C++ Authority:** `/JUCE_Plugin/Source/EngineTypes.h`
- **C++ Parameters:** `/JUCE_Plugin/Source/UnifiedDefaultParameters.cpp`
- **Python Mirror:** `engine_mapping_authoritative.py`
- **Preset Corpus:** `/JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json`

---

*This document is automatically generated from the C++ source files.*
*Do not edit manually - regenerate using fix_trinity_parameters.py*