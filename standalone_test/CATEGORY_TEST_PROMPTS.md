# ChimeraPhoenix Category-Specific Test Prompts

**Purpose**: Self-contained prompts for deep testing of each DSP engine category
**Method**: Each prompt can be given to a fresh Claude instance for specialized testing

---

## PROMPT 1: Distortion & Saturation Testing

```
You are testing the Distortion & Saturation engines (15-22) in the ChimeraPhoenix audio plugin.

CONTEXT:
- Standalone test framework exists at: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
- Build system (build_v2.sh) and object files are already compiled
- You need to create distortion-specific tests

ENGINES TO TEST:
15. Vintage Tube Preamp Studio (KNOWN ISSUE: Hangs/infinite loop)
16. Wave Folder
17. Harmonic Exciter Platinum
18. Bit Crusher
19. Multiband Saturator
20. Muff Fuzz (KNOWN ISSUE: CPU 5.19%)
21. Rodent Distortion
22. K-Style Overdrive

DISTORTION-SPECIFIC METRICS YOU MUST MEASURE:

1. **Harmonic Content Analysis**
   - Measure THD (Total Harmonic Distortion)
   - Identify odd vs even harmonic ratios
   - Detect harmonic series up to 10th harmonic
   - Classify as "warm" (even harmonics) vs "harsh" (odd harmonics)

2. **Transfer Function (Input/Output Curve)**
   - Test input levels: -40dB, -20dB, -10dB, -6dB, 0dB, +6dB
   - Measure output level at each input
   - Detect soft-clipping vs hard-clipping
   - Identify compression ratio

3. **Frequency-Dependent Distortion**
   - Test at: 100Hz, 500Hz, 1kHz, 4kHz, 10kHz
   - Measure THD at each frequency
   - Detect intermodulation distortion (IMD)
   - Use dual-tone test (e.g., 1kHz + 1.1kHz)

4. **Dynamic Response**
   - Test with sine sweep (20Hz-20kHz)
   - Test with transient-rich signals (drums)
   - Measure attack/release characteristics
   - Detect aliasing artifacts

5. **Saturation Curve Analysis**
   - Plot input vs output amplitude
   - Measure where soft-clipping begins
   - Identify maximum output level
   - Detect asymmetric clipping

CRITICAL TESTS:

1. **Vintage Tube Preamp (15) - Debug Hang**
   - Instrument the code to find infinite loop
   - Test with timeout protection
   - Try different parameter combinations
   - Identify which parameter causes hang

2. **Bit Crusher (18) - Aliasing Detection**
   - Measure aliasing artifacts
   - Test bit depth reduction artifacts
   - Test sample rate reduction artifacts
   - Spectral analysis of aliasing

3. **Multiband Saturator (19) - Crossover Quality**
   - Measure crossover frequency accuracy
   - Test phase response at crossovers
   - Detect crossover artifacts
   - Verify independent band processing

YOUR DELIVERABLES:

1. Create `distortion_test.cpp` with:
   - Harmonic analysis functions
   - Transfer function measurement
   - IMD testing
   - Spectral analysis

2. Generate CSV files for each engine:
   - `distortion_engine_XX_harmonics.csv`
   - `distortion_engine_XX_transfer.csv`
   - `distortion_engine_XX_spectrum.csv`

3. Create `DISTORTION_QUALITY_REPORT.md` with:
   - Quality rating (1-5 stars) per engine
   - Harmonic character description
   - Comparison to classic hardware (e.g., "Similar to Tube Screamer")
   - Issues found with severity ratings
   - Recommendations for fixes

BUILD SYSTEM:
- Compile: `clang++ -std=c++17 -O2 -I. -I../JUCE_Plugin/Source -I/Users/Branden/JUCE/modules distortion_test.cpp [link with existing objects]`
- Use existing pattern from reverb_test.cpp and build_reverb_test.sh

SUCCESS CRITERIA:
- All 8 engines tested (skip Vintage Tube if it hangs unrecoverably)
- Harmonic content characterized for each
- Transfer curves plotted
- Quality assessment completed
- Critical bugs documented with reproduction steps
```

---

## PROMPT 2: Dynamics & Compression Testing

```
You are testing the Dynamics & Compression engines (1-6) in the ChimeraPhoenix audio plugin.

CONTEXT:
- Standalone test framework exists at: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
- Build system ready, object files compiled
- Some engines have known issues (heap allocation, file I/O in process())

ENGINES TO TEST:
1. Vintage Opto Compressor Platinum (KNOWN: File I/O in process())
2. Classic Compressor Pro
3. Transient Shaper Platinum (KNOWN: Debug printf in process())
4. Noise Gate Platinum (KNOWN: Heap allocation in process())
5. Mastering Limiter Platinum (KNOWN: Debug printf)
6. Dynamic EQ (KNOWN: THD 0.759%)

DYNAMICS-SPECIFIC METRICS YOU MUST MEASURE:

1. **Compression Characteristics**
   - Measure compression ratio at different input levels
   - Calculate attack time (10%-90% of compression)
   - Calculate release time (90%-10% of release)
   - Measure knee behavior (hard vs soft knee)
   - Test threshold accuracy

2. **Gain Reduction Metering**
   - Measure GR at: -40dB, -30dB, -20dB, -10dB, 0dB input
   - Plot GR curve (input vs output)
   - Detect pumping/breathing artifacts
   - Measure makeup gain accuracy

3. **Transient Preservation**
   - Test with drum hits (sharp transients)
   - Measure transient peak preservation
   - Calculate transient-to-sustain ratio
   - Detect overshoot/undershoot

4. **Sidechain Response** (if applicable)
   - Test sidechain filtering
   - Measure detection circuit response
   - Test RMS vs peak detection
   - Verify stereo linking behavior

5. **Limiting/Brick-Wall Performance**
   - Test with signal >0dBFS
   - Measure output ceiling accuracy (should be exactly 0dBFS or target)
   - Detect clipping/distortion at limit
   - Measure lookahead effectiveness

6. **Gate/Expander Behavior**
   - Measure gate threshold accuracy
   - Test hysteresis (open vs close threshold)
   - Measure gate attack/release times
   - Detect gate chattering

CRITICAL TESTS:

1. **Vintage Opto Compressor (1) - Real-time Safety**
   - Verify file I/O is disabled in release builds
   - Test with rapid parameter changes
   - Measure CPU usage consistency
   - Check for buffer overruns

2. **Noise Gate (4) - Heap Allocation Issue**
   - Profile memory allocation in process()
   - Test with varying channel counts
   - Measure real-time safety violations
   - Recommend fixed-size buffer solution

3. **Mastering Limiter (5) - Ceiling Accuracy**
   - Test with 0dBFS, +3dBFS, +6dBFS input
   - Measure output: should NEVER exceed ceiling
   - Test lookahead effectiveness
   - Measure THD when limiting

4. **Dynamic EQ (6) - High THD Issue**
   - Measure THD at neutral settings (should be <0.1%)
   - Test frequency-dependent THD
   - Identify which band causes distortion
   - Test with different Q values

YOUR DELIVERABLES:

1. Create `dynamics_test.cpp` with:
   - Compression ratio calculation
   - Attack/release time measurement
   - GR curve plotting
   - Transient analysis functions
   - Limiting ceiling verification

2. Generate CSV files:
   - `dynamics_engine_XX_gr_curve.csv` (input vs output levels)
   - `dynamics_engine_XX_envelope.csv` (attack/release behavior)
   - `dynamics_engine_XX_transients.csv` (transient response)

3. Create `DYNAMICS_QUALITY_REPORT.md` with:
   - Compression character (transparent, colored, aggressive, etc.)
   - Attack/release time measurements
   - Comparison to classic compressors (1176, LA-2A, SSL, etc.)
   - Real-time safety assessment
   - Critical bugs and fixes needed

BUILD PATTERN:
- Follow reverb_test.cpp structure
- Use build_reverb_test.sh as template

SUCCESS CRITERIA:
- Compression ratios measured accurately
- Attack/release times documented
- Limiting accuracy verified (no overs)
- Real-time safety issues documented
- Quality ratings assigned
```

---

## PROMPT 3: Filters & EQ Testing

```
You are testing the Filters & EQ engines (7-14) in the ChimeraPhoenix audio plugin.

CONTEXT:
- Standalone test framework at: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
- Build system operational
- LadderFilter (9) has known high THD (3.512%)

ENGINES TO TEST:
7. Parametric EQ Studio
8. Vintage Console EQ Studio
9. Ladder Filter Pro (KNOWN: THD 3.512%)
10. State Variable Filter
11. Formant Filter Pro
12. Envelope Filter
13. Comb Resonator
14. Vocal Formant Filter

FILTER-SPECIFIC METRICS YOU MUST MEASURE:

1. **Frequency Response**
   - Measure magnitude response: 20Hz-20kHz (logarithmic)
   - Test at 100 frequencies minimum
   - Generate Bode plot data
   - Measure -3dB cutoff frequency accuracy
   - Test flatness of passband (should be ±0.5dB)

2. **Phase Response**
   - Measure phase shift across frequency spectrum
   - Calculate group delay
   - Detect phase distortion
   - Test minimum-phase vs linear-phase behavior

3. **Filter Slope/Roll-off**
   - Measure slope in dB/octave
   - Verify filter order (12dB/oct = 2-pole, 24dB/oct = 4-pole, etc.)
   - Test stopband attenuation
   - Detect resonance peaks

4. **Resonance/Q Behavior**
   - Test Q values: 0.5, 0.707, 1.0, 2.0, 5.0, 10.0
   - Measure resonant peak magnitude
   - Detect self-oscillation threshold
   - Test stability at high Q

5. **THD at Different Frequencies**
   - Measure THD: 50Hz, 100Hz, 500Hz, 1kHz, 5kHz, 10kHz
   - Test with different Q values
   - Identify non-linear behavior
   - Compare to professional standard (<0.01% THD for clean filters)

6. **Transient Response** (for resonant filters)
   - Impulse response analysis
   - Ringing duration measurement
   - Step response overshoot
   - Settling time calculation

7. **EQ-Specific Tests**
   - Bell filter: bandwidth accuracy, gain accuracy
   - Shelving: transition frequency, slope accuracy
   - High/low-pass: cutoff accuracy, slope verification
   - Notch: depth measurement, Q accuracy

CRITICAL TESTS:

1. **Ladder Filter (9) - High THD Investigation**
   - Measure THD vs frequency (is it frequency-dependent?)
   - Test THD vs Q (resonance-dependent?)
   - Test THD vs input level
   - Identify root cause (coefficient quantization? instability?)
   - Compare to Moog ladder reference

2. **State Variable Filter (10) - Topology Verification**
   - Verify simultaneous LP/BP/HP outputs
   - Test mode switching stability
   - Measure inter-mode phase relationships
   - Verify filter topology correctness

3. **Formant Filters (11, 14) - Formant Accuracy**
   - Measure formant peak frequencies
   - Compare to vowel formant standards (IPA chart)
   - Test formant bandwidth
   - Verify vowel sound accuracy

4. **Comb Resonator (13) - Harmonic Series**
   - Verify harmonic spacing (should be exact integer multiples)
   - Measure peak-to-notch depth
   - Test feedback vs feedforward combs
   - Detect tuning accuracy

5. **Envelope Filter (12) - Envelope Follower**
   - Measure envelope detection accuracy
   - Test attack/release of filter modulation
   - Verify filter sweeps musically
   - Test with different input dynamics

YOUR DELIVERABLES:

1. Create `filter_test.cpp` with:
   - Frequency response sweep (log scale)
   - Phase response measurement
   - THD vs frequency analysis
   - Q factor verification
   - Impulse response capture
   - Resonance peak detection

2. Generate CSV files:
   - `filter_engine_XX_magnitude.csv` (frequency response)
   - `filter_engine_XX_phase.csv`
   - `filter_engine_XX_thd_vs_freq.csv`
   - `filter_engine_XX_impulse.csv`

3. Create `FILTER_QUALITY_REPORT.md` with:
   - Filter type classification (Butterworth, Chebyshev, Moog, etc.)
   - Frequency response accuracy rating
   - THD measurements and comparison to professional (<0.01%)
   - Phase linearity assessment
   - Musical character description
   - Comparison to classic filters (Moog ladder, MS-20, SEM, etc.)

BUILD PATTERN:
- Follow reverb_test.cpp
- Add FFT-based frequency response measurement
- Use logarithmic frequency sweep

SUCCESS CRITERIA:
- Frequency response plotted for all 8 engines
- Cutoff accuracy verified (±2% tolerance)
- THD measured and compared to <0.01% professional standard
- LadderFilter high THD root cause identified
- Filter slope verified (dB/octave)
- Quality ratings assigned
```

---

## PROMPT 4: Pitch & Time-Based Effects Testing

```
You are testing Pitch Shifting, Harmonizer, and Time-based engines in ChimeraPhoenix.

CONTEXT:
- Standalone test framework at: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
- Build system operational
- PitchShifter (32) has CRITICAL high THD (8.673%)
- IntelligentHarmonizer (33) crashed during testing

ENGINES TO TEST:
31. Detune Doubler
32. Pitch Shifter (KNOWN: THD 8.673% - CRITICAL)
33. Intelligent Harmonizer (KNOWN: Test crashed)
49. Pitch Shifter (duplicate? may be broken)
34-38. Delay engines (Tape, Digital, Magnetic Drum, BBD, Buffer Repeat)

PITCH-SPECIFIC METRICS YOU MUST MEASURE:

1. **Pitch Accuracy**
   - Input: 440Hz sine wave
   - Shift by: -12, -7, -5, -2, 0, +2, +5, +7, +12 semitones
   - Measure output frequency (should be exact: 440 * 2^(semitones/12))
   - Tolerance: ±1 cent (1/100th semitone)
   - Test with different input frequencies: 100Hz, 440Hz, 1kHz, 4kHz

2. **Formant Preservation**
   - Test with vocal recording or synthetic vowel
   - Measure formant peak frequencies before/after shift
   - Formants should NOT shift with pitch (for formant-preserved mode)
   - Compare formant-preserved vs non-formant modes

3. **Artifact Analysis**
   - Measure THD for pure sine wave input
   - Detect phasiness/chorus artifacts
   - Identify pre-echo/post-echo
   - Measure transient smearing
   - Spectral analysis for alien/robotic artifacts

4. **Latency Measurement**
   - Measure input-to-output delay
   - Test if latency is constant or pitch-dependent
   - Report latency compensation needed for DAW

5. **Polyphonic Performance** (for harmonizers)
   - Test with chord input (3+ notes)
   - Verify each note tracked correctly
   - Measure harmonic accuracy for each note
   - Detect note tracking errors

6. **Algorithm Classification**
   - Identify algorithm: Time-domain (PSOLA), Frequency-domain (Phase Vocoder), Hybrid
   - Measure grain/window size
   - Detect pitch detection method
   - Compare to known algorithms (Elastique, Zynaptiq, Serato)

TIME-BASED METRICS (for delays):

1. **Delay Time Accuracy**
   - Set delay time: 100ms, 250ms, 500ms, 1000ms
   - Measure actual delay (impulse response)
   - Tolerance: ±1ms
   - Test modulation depth/rate if applicable

2. **Feedback Quality**
   - Test feedback: 0%, 25%, 50%, 75%, 90%
   - Measure decay time
   - Detect instability/runaway at high feedback
   - Measure THD in feedback path

3. **Modulation Character** (for BBD, Tape)
   - Measure LFO rate and depth
   - Detect wow/flutter artifacts
   - Test modulation waveform (sine, triangle, random)
   - Measure pitch variation amount

CRITICAL TESTS:

1. **PitchShifter (32) - 8.673% THD Investigation**
   - Measure THD vs shift amount (-12 to +12 semitones)
   - Test with pure sine wave: 100Hz, 440Hz, 1kHz, 4kHz
   - Identify if distortion is:
     - Algorithm artifact (granular/PSOLA issues)
     - Buffer underflow/overflow
     - Coefficient quantization
   - Compare to professional standard (<0.5% for pitch shifters)
   - Test different FFT sizes or grain sizes if adjustable

2. **IntelligentHarmonizer (33) - Crash Debug**
   - Test with timeout protection
   - Try different input signals (sine, noise, speech, chord)
   - Test different harmony intervals
   - Identify crash trigger (buffer overflow? division by zero?)
   - Add safety checks if needed

3. **Detune Doubler (31) - Detuning Accuracy**
   - Measure detune amount in cents
   - Verify stereo spread
   - Test detune stability over time
   - Compare to classic chorus/doubler effects

4. **Tape Echo (34) - Tape Character**
   - Measure wow/flutter amount
   - Test saturation in feedback loop
   - Verify vintage tape characteristics
   - Compare to Space Echo, Echoplex

YOUR DELIVERABLES:

1. Create `pitch_test.cpp` with:
   - Pitch accuracy measurement (frequency detection)
   - Formant tracking
   - Spectral artifact analysis
   - Latency measurement
   - Delay time verification functions

2. Generate CSV files:
   - `pitch_engine_XX_accuracy.csv` (shift amount vs measured pitch)
   - `pitch_engine_XX_formants.csv`
   - `pitch_engine_XX_spectrum.csv` (artifact analysis)
   - `delay_engine_XX_timing.csv`

3. Create `PITCH_TIME_QUALITY_REPORT.md` with:
   - Pitch accuracy results (target ±1 cent)
   - Algorithm identification
   - Artifact characterization
   - Comparison to professional shifters (SoundToys, Antares, etc.)
   - Latency compensation values
   - Critical bugs and severity

BUILD PATTERN:
- Use FFT for frequency detection
- Implement autocorrelation for pitch tracking
- Follow reverb_test.cpp structure

SUCCESS CRITERIA:
- Pitch accuracy measured for all shifters
- PitchShifter 8.673% THD root cause found
- IntelligentHarmonizer crash reproduced and fixed
- Delay timing verified (±1ms accuracy)
- Algorithm types identified
- Quality comparison to professional tools
```

---

## PROMPT 5: Modulation Effects Testing

```
You are testing Modulation engines (23-30) in ChimeraPhoenix.

CONTEXT:
- Standalone test framework at: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
- Previous tests showed 100% pass rate for modulation category
- Need to verify QUALITY, not just functionality

ENGINES TO TEST:
23. Stereo Chorus
24. Resonant Chorus Platinum
25. Analog Phaser
26. Platinum Ring Modulator
27. Frequency Shifter
28. Harmonic Tremolo
29. Classic Tremolo
30. Rotary Speaker Platinum

MODULATION-SPECIFIC METRICS:

1. **LFO Characteristics**
   - Measure LFO rate accuracy (Hz)
   - Test rate range (min/max)
   - Verify waveform shapes: sine, triangle, square, random
   - Measure LFO phase (stereo offset)
   - Test tempo sync if applicable

2. **Modulation Depth**
   - Measure depth in cents (for pitch mod)
   - Measure depth in milliseconds (for delay mod)
   - Measure depth in dB (for amplitude mod)
   - Verify depth range (0-100%)

3. **Chorus Quality**
   - Measure chorus voices (how many delay lines)
   - Test detune amount per voice (cents)
   - Measure stereo width
   - Detect metallic/digital artifacts
   - Compare to: Dimension D, CE-1, Juno chorus

4. **Phaser Quality**
   - Count phaser stages (2, 4, 6, 8, 12?)
   - Measure notch frequencies
   - Test feedback amount
   - Measure resonance peaks
   - Verify sweep range
   - Compare to: Phase 90, Small Stone, Univibe

5. **Ring Modulator**
   - Verify frequency multiplication (fin * fcarrier)
   - Test sum and difference frequencies
   - Measure carrier bleed
   - Test with different carriers (sine, triangle, square)
   - Detect aliasing

6. **Tremolo Quality**
   - Measure tremolo depth (0-100%)
   - Verify waveform smoothness
   - Test hard vs soft tremolo
   - Measure harmonic tremolo filter separation
   - Compare to: Fender Tremolo, Vox tremolo

7. **Rotary Speaker Simulation**
   - Measure horn vs drum speed ratio (should be ~6:1)
   - Test slow/fast switch behavior
   - Measure Doppler shift amount
   - Verify crossover frequency
   - Test acceleration/deceleration time
   - Compare to Leslie 122/147

CRITICAL TESTS:

1. **All Chorus Engines (23, 24) - Character Comparison**
   - Measure differences between Stereo vs Resonant
   - Test which is more "vintage" vs "modern"
   - Verify stereo width differences
   - Compare voicing count

2. **Frequency Shifter (27) - Linear Shift Verification**
   - Input: 440Hz
   - Shift by: +10Hz, +50Hz, +100Hz
   - Measure output (should be 450Hz, 490Hz, 540Hz - LINEAR, not exponential)
   - Verify all harmonics shift by same amount (not pitch shift!)
   - Test for classic Bode/Moog frequency shifter sound

3. **Rotary Speaker (30) - Leslie Accuracy**
   - Measure fast speed: ~6.7 Hz (horn), ~1.1 Hz (drum)
   - Measure slow speed: ~0.7 Hz (horn), ~0.1 Hz (drum)
   - Verify acceleration ramp (1-2 seconds)
   - Test Doppler accuracy
   - Stereo image verification

4. **Harmonic Tremolo (28) - Filter Separation**
   - Verify high-pass/low-pass crossover
   - Measure phase relationship between bands
   - Test depth balance between bands
   - Compare to Fender Vibrolux harmonic tremolo

YOUR DELIVERABLES:

1. Create `modulation_test.cpp` with:
   - LFO rate/depth measurement
   - Spectral analysis for modulation artifacts
   - Stereo width measurement
   - Notch frequency detection (phaser)
   - Speed/acceleration measurement (rotary)

2. Generate CSV files:
   - `mod_engine_XX_lfo.csv` (LFO characteristics)
   - `mod_engine_XX_spectrum.csv` (frequency content)
   - `mod_engine_XX_stereo.csv` (stereo field)

3. Create `MODULATION_QUALITY_REPORT.md` with:
   - LFO accuracy and range
   - Character descriptions (vintage, digital, analog-style)
   - Stereo imaging quality
   - Comparison to classic hardware (Boss, MXR, Electro-Harmonix)
   - Musical usability assessment
   - Parameter range recommendations

SUCCESS CRITERIA:
- LFO rates verified accurate
- Modulation depth measured
- Chorus voice count identified
- Phaser stage count verified
- Rotary speaker speeds match Leslie
- Frequency shifter verified as LINEAR (not pitch)
- All engines characterized and compared to hardware
```

---

## PROMPT 6: Spatial & Special Effects Testing

```
You are testing Spatial, Spectral, and Special engines (44-54) in ChimeraPhoenix.

CONTEXT:
- Standalone test framework at: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
- Build system operational
- These are advanced/experimental effects

ENGINES TO TEST:
44. Stereo Widener
45. Stereo Imager
46. Dimension Expander
47. Phase Align Platinum
48. Feedback Network
49. Pitch Shifter (may be duplicate of 32)
50. Phased Vocoder
51. Spectral Freeze
52. Spectral Gate Platinum (KNOWN: Crashes on startup)
53. Granular Cloud
54. Chaos Generator

SPATIAL METRICS:

1. **Stereo Width Measurement**
   - Input: Mono signal
   - Measure output stereo correlation (-1 to +1)
   - Test width parameter: 0%, 50%, 100%, 150%
   - Verify no phase cancellation issues in mono
   - Measure frequency-dependent width

2. **Phase Alignment**
   - Test with stereo input (intentionally out of phase)
   - Measure phase correction accuracy
   - Test at multiple frequencies
   - Verify mono compatibility improvement

3. **Haas Effect / Precedence**
   - Measure inter-channel delay
   - Test Haas effect (< 30ms delay = width, >30ms = echo)
   - Verify psychoacoustic width vs actual width

SPECTRAL METRICS:

1. **FFT Analysis Quality**
   - Measure FFT window size
   - Test overlap amount
   - Detect windowing artifacts
   - Measure frequency resolution
   - Verify time/frequency trade-off

2. **Phase Vocoder Performance**
   - Measure time-stretch accuracy
   - Test pitch-shift quality (vs time-domain methods)
   - Detect phasiness artifacts
   - Measure latency/lookahead

3. **Spectral Gate/Freeze**
   - Measure gate threshold accuracy per bin
   - Test freeze: verify bins are literally frozen (not decaying)
   - Measure spectral resolution
   - Test release behavior

GRANULAR METRICS:

1. **Grain Characteristics**
   - Measure grain size (ms)
   - Test grain density (grains/sec)
   - Measure grain envelope (shape/smoothness)
   - Test pitch variation per grain
   - Verify overlap-add quality

2. **Cloud Texture**
   - Measure randomization amount
   - Test grain positioning (random vs regular)
   - Verify cloud density control
   - Test stereo spreading

CHAOS/SPECIAL METRICS:

1. **Chaos Generator**
   - Identify chaos algorithm (Lorenz, Rossler, etc.)
   - Measure output spectrum
   - Test controllability (amount parameter)
   - Verify true randomness vs pseudo-random

2. **Feedback Network**
   - Measure network topology (how many feedback paths)
   - Test stability (should NOT explode)
   - Measure feedback gain limits
   - Detect oscillation threshold

CRITICAL TESTS:

1. **Spectral Gate (52) - Startup Crash Debug**
   - Test with timeout
   - Try minimal initialization
   - Test FFT library compatibility
   - Check buffer allocation
   - Identify crash trigger

2. **Phase Align (47) - Accuracy Verification**
   - Create known phase-shifted test signal
   - Measure correction accuracy (±5° tolerance)
   - Test with different frequencies
   - Verify all-pass filter approach

3. **Stereo Widener (44) vs Stereo Imager (45)**
   - Compare algorithms (Mid/Side vs Haas vs other)
   - Measure mono compatibility
   - Test phase issues
   - Determine which is better for which use case

4. **Granular Cloud (53) - Grain Quality**
   - Test grain size range (1ms - 500ms)
   - Measure grain envelope smoothness
   - Detect clicks between grains
   - Verify cloud density control

YOUR DELIVERABLES:

1. Create `spatial_test.cpp` with:
   - Stereo correlation measurement
   - Phase analysis (FFT-based)
   - Spectral analysis (FFT)
   - Grain detection and analysis
   - Chaos pattern analysis

2. Generate CSV files:
   - `spatial_engine_XX_correlation.csv`
   - `spectral_engine_XX_bins.csv` (FFT bin analysis)
   - `granular_engine_XX_grains.csv`

3. Create `SPATIAL_SPECIAL_QUALITY_REPORT.md` with:
   - Stereo width effectiveness
   - Phase alignment accuracy
   - Spectral processing quality (FFT artifacts)
   - Granular texture description
   - Comparison to professional tools (iZotope, FabFilter, etc.)
   - Experimental effect usability

SUCCESS CRITERIA:
- Stereo width measured and verified
- Phase alignment accuracy tested
- Spectral Gate crash debugged and fixed
- FFT quality verified (no artifacts)
- Granular grain quality assessed
- All engines characterized
```

---

## PROMPT 7: Utility Effects Testing

```
You are testing Utility engines (55-56) in ChimeraPhoenix.

CONTEXT:
- Standalone test framework at: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
- These are simple but critical utility effects
- Previous tests: 100% pass rate, THD 0.000%

ENGINES TO TEST:
55. Gain Utility Platinum
56. Mono Maker Platinum

UTILITY-SPECIFIC METRICS:

1. **Gain Utility (55)**
   - **Gain Accuracy**: Set +6dB, measure actual gain (should be exactly 6.00dB ±0.01dB)
   - **Test range**: -40dB to +20dB in 1dB steps
   - **THD at all gain settings**: Should be <0.001% (better than 0.5%)
   - **Phase response**: Must be linear (no phase shift)
   - **Latency**: Should be 0 samples (instant)
   - **Channel independence**: Verify L/R are truly independent
   - **Automation smoothing**: Test for zipper noise when gain changes rapidly

2. **Mono Maker (56)**
   - **Mono summing accuracy**: (L+R)/2 or (L+R)/√2? Test both
   - **Phase preservation**: Verify no phase rotation
   - **Gain compensation**: Check if output is same loudness as input
   - **Frequency response**: Flat 20Hz-20kHz (±0.1dB)
   - **THD**: Should be 0.000% (literally bit-perfect sum)
   - **Latency**: Should be 0 samples

3. **Edge Cases for Both**
   - Test with DC offset input (should pass through or remove?)
   - Test with silent input (should stay silent)
   - Test with max level input (±1.0, should not clip)
   - Test with denormal numbers (very quiet signals)

CRITICAL TESTS:

1. **Gain Utility - Precision Test**
   - Input: 0dBFS sine wave (amplitude 1.0)
   - Set gain: +6.0206dB (exactly 2x in linear)
   - Measure output: Should be EXACTLY 2.000000 (not 1.999 or 2.001)
   - Test at all frequencies: 20Hz, 100Hz, 1kHz, 10kHz, 20kHz
   - Verify bit-perfect multiplication

2. **Mono Maker - Stereo Cancellation Test**
   - Input L: +1.0, Input R: -1.0 (perfect phase inversion)
   - Output should be: 0.0 (complete cancellation)
   - Test with various phase relationships
   - Verify no "residual stereo" leak

3. **Both - Real-time Safety**
   - Should be fastest engines (simple math operations)
   - No heap allocation
   - No complex calculations
   - CPU usage should be <0.1%

YOUR DELIVERABLES:

1. Create `utility_test.cpp` with:
   - Precision gain measurement
   - THD verification at multiple gains
   - Phase response test
   - Mono summing verification
   - Automation smoothing test

2. Generate CSV files:
   - `gain_utility_accuracy.csv` (set vs measured gain)
   - `mono_maker_cancellation.csv` (phase tests)

3. Create `UTILITY_QUALITY_REPORT.md` with:
   - Gain accuracy (±0.01dB tolerance)
   - THD measurements (should be <0.001%)
   - Latency verification (0 samples)
   - CPU usage (<0.1%)
   - Phase linearity
   - Comparison to professional tools (should match bit-perfect)

SUCCESS CRITERIA:
- Gain accuracy: ±0.01dB
- THD: <0.001% (better than current 0.000%)
- Phase: Linear (0° shift at all frequencies)
- Latency: 0 samples
- Mono summing: Bit-perfect (L+R)/2
- No automation zipper noise
```

---

## GENERAL TESTING GUIDELINES (For All Prompts)

### Build System Pattern:
```bash
# Follow this pattern from reverb testing:
1. Create test file: category_test.cpp
2. Create build script: build_category_test.sh (based on build_reverb_test.sh)
3. Compile against existing object files in build/obj/
4. Link with JUCE framework and HarfBuzz
5. Generate standalone executable
```

### Test Output Requirements:
1. **CSV files**: For plotting/analysis
2. **Markdown report**: Human-readable quality assessment
3. **Console output**: Real-time progress with clear pass/fail indicators

### Quality Rating System (1-5 stars):
- ⭐⭐⭐⭐⭐ Professional grade (matches high-end commercial)
- ⭐⭐⭐⭐ Production ready (matches mid-tier commercial)
- ⭐⭐⭐ Usable (works but has minor issues)
- ⭐⭐ Needs work (functional but flawed)
- ⭐ Broken (not functional or severe quality issues)

### Comparison Benchmarks:
- Always compare to known hardware/software (e.g., "Similar to Neve 1073")
- Reference professional standards (e.g., THD <0.01% for clean signal path)
- Identify unique character vs defects

### Issue Severity:
- 🔴 CRITICAL: Crashes, hangs, produces no sound
- ⚠️ HIGH: Quality issues (high THD, wrong behavior)
- 🟡 MEDIUM: Minor issues (slight inaccuracy, edge cases)
- 🟢 LOW: Cosmetic or nice-to-have improvements

### Success Criteria:
Each prompt should result in:
1. ✅ All engines tested (or crash reason documented)
2. ✅ Quality metrics measured
3. ✅ Comparison to professional standards
4. ✅ Issues prioritized by severity
5. ✅ Specific recommendations for fixes

---

## EXECUTION STRATEGY

**Option 1: Sequential (One category per session)**
- Thorough and detailed
- Time: ~2-4 hours per category
- Total: 1-2 days

**Option 2: Parallel (Launch all agents simultaneously)**
- Fastest results
- Time: ~4-6 hours total
- May hit token limits

**Option 3: Hybrid (2-3 agents at once)**
- Balanced approach
- Time: ~8-12 hours
- Recommended strategy

---

## FINAL DELIVERABLE STRUCTURE

After all categories tested, create:

```
MASTER_QUALITY_REPORT.md
├── Executive Summary
├── Category Breakdowns
│   ├── Distortion (8 engines)
│   ├── Dynamics (6 engines)
│   ├── Filters (8 engines)
│   ├── Pitch/Time (9 engines)
│   ├── Modulation (8 engines)
│   ├── Spatial (11 engines)
│   └── Utility (2 engines)
├── Critical Issues (Priority 1)
├── Quality Issues (Priority 2-3)
├── Overall Statistics
│   ├── Total engines: 56
│   ├── Production ready: XX
│   ├── Needs work: XX
│   ├── Broken: XX
├── Comparison Matrix (vs commercial plugins)
└── Roadmap to Professional Quality
```
