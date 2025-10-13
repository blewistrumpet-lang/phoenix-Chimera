# MODULATION ENGINES TEST OVERVIEW
## Common LFO and Stereo Testing Protocols

**Document Version:** 1.0
**Date:** 2025-10-10
**Target:** 11 Production-Ready Modulation Engines
**Focus:** Time-Varying Behavior Validation

---

## EXECUTIVE SUMMARY

This document establishes standardized testing protocols for all modulation engines in Project Chimera v3.0. The key challenge is **capturing and validating time-varying behavior** - modulation effects by definition change over time, making them harder to test than static processors.

### Engines Under Test

**MODULATION CATEGORY (Engine IDs 23-33)**
1. **StereoChorus** (ID 23) - Digital Chorus
2. **ResonantChorus_Platinum** (ID 24) - Advanced chorus with resonance
3. **AnalogPhaser** (ID 25) - All-pass phaser (2/4/6/8 stages)
4. **PlatinumRingModulator** (ID 26) - Ring modulation with freq shift
5. **FrequencyShifter** (ID 27) - Hilbert transform freq shifter
6. **HarmonicTremolo** (ID 28) - Crossover-based harmonic tremolo
7. **ClassicTremolo** (ID 29) - 7 tremolo types including optical/bias
8. **RotarySpeaker_Platinum** (ID 30) - Leslie-style rotary speaker
9. **PitchShifter** (ID 31) - (Not modulation, covered elsewhere)
10. **DetuneDoubler** (ID 32) - Pitch-based doubler
11. **IntelligentHarmonizer** (ID 33) - (Not pure modulation, covered elsewhere)

**TRUE MODULATION ENGINES: 8 engines** (IDs 23-30, excluding pitch processors)

---

## CORE TESTING CHALLENGES

### Challenge 1: Time-Varying Capture
- **Problem:** LFO position changes continuously
- **Solution:** Phase-locked capture with known starting conditions
- **Method:** Reset engine state, process known buffer size, extract LFO trajectory

### Challenge 2: LFO Rate Accuracy
- **Problem:** Frequency accuracy over wide range (0.01 Hz - 50 Hz)
- **Solution:** Long-duration capture for slow rates, cycle counting for verification
- **Method:** Zero-crossing detection, FFT-based frequency measurement

### Challenge 3: Stereo Phase Relationships
- **Problem:** L/R channels may have phase offset, requires correlation analysis
- **Solution:** Cross-correlation, phase coherence measurement
- **Method:** Hilbert transform for instantaneous phase extraction

### Challenge 4: Modulation Depth Verification
- **Problem:** Depth parameter affects amplitude or delay time modulation
- **Solution:** Peak-to-peak measurement of modulated parameter
- **Method:** Extract envelope, measure min/max excursion

### Challenge 5: Waveform Shape Validation
- **Problem:** Different LFO shapes (sine, triangle, square, saw, random)
- **Solution:** Shape-specific validators with tolerance bands
- **Method:** Template matching, harmonic analysis

---

## STANDARDIZED TEST PROTOCOLS

### Protocol 1: LFO Rate Accuracy Test

**Purpose:** Verify LFO frequency across full parameter range

**Test Stimulus:**
- Silent input (or DC offset for carrier extraction)
- Single LFO rate setting
- Capture duration: 100 cycles (minimum 2 seconds for 0.01 Hz)

**Measurement Method:**
```
1. Reset engine to known state
2. Process silent buffer (or impulse for delay-based effects)
3. Extract modulated parameter trajectory (e.g., delay time, gain)
4. Zero-crossing detection or autocorrelation for period
5. Calculate measured frequency = 1 / period
6. Compare to expected frequency: |measured - expected| / expected < 0.01 (1% tolerance)
```

**Rate Sweep Points (logarithmic):**
- 0.01 Hz (ultra-slow)
- 0.05 Hz
- 0.1 Hz
- 0.5 Hz
- 1.0 Hz
- 2.0 Hz
- 5.0 Hz (typical musical tremolo)
- 10.0 Hz
- 20.0 Hz (approaching audio rate)
- 50.0 Hz (maximum for most engines)

**Pass Criteria:**
- Rate accuracy within ±1% for 0.1-20 Hz
- Rate accuracy within ±2% for 0.01-0.1 Hz and 20-50 Hz (tolerance relaxed at extremes)

---

### Protocol 2: LFO Waveform Shape Verification

**Purpose:** Validate different LFO waveform shapes

**Test Stimulus:**
- Silent input or carrier tone
- Fixed LFO rate (1 Hz for easy visualization)
- Capture 5 complete cycles

**Waveform-Specific Validators:**

**SINE:**
- FFT: fundamental at LFO rate, THD < 5%
- Peak-to-peak amplitude symmetric
- Derivative continuous (smooth)

**TRIANGLE:**
- Linear segments (R² > 0.99 for each half)
- Sharp reversal at peaks (check derivative discontinuity)
- Symmetric rise/fall times

**SQUARE:**
- Edge sharpness: transition time < 1% of period
- Flat top/bottom: variance < 0.01 in plateau regions
- 50% duty cycle (unless pulse width modulation)

**SAWTOOTH:**
- Linear ramp (R² > 0.99)
- Sharp reset transition
- Correct polarity (up-ramp vs down-ramp)

**RANDOM:**
- Spectrum: flat power distribution across 0-Nyquist
- Autocorrelation: no periodic structure
- Statistical distribution: approximately Gaussian

---

### Protocol 3: Modulation Depth Measurement

**Purpose:** Quantify depth parameter mapping

**Test Method:**
- Set depth to known values: 0%, 25%, 50%, 75%, 100%
- Capture modulated parameter trajectory
- Measure peak-to-peak excursion
- Compare to expected range

**Depth Calculation (Delay-based effects):**
```
depth_actual = (delay_max - delay_min) / delay_range_nominal
depth_error = |depth_actual - depth_setting| < 0.05 (5% absolute)
```

**Depth Calculation (Amplitude effects):**
```
depth_actual = (gain_max - gain_min) / (gain_max + gain_min)
depth_error = |depth_actual - depth_setting| < 0.05
```

**Pass Criteria:**
- Depth mapping linear: R² > 0.95
- Depth accuracy within ±5% absolute error

---

### Protocol 4: Stereo Phase Relationship Test

**Purpose:** Verify L/R channel phase offset and stereo width

**Test Stimulus:**
- Mono input (identical L/R)
- Stereo phase parameter at known values: 0°, 45°, 90°, 135°, 180°
- LFO rate 1 Hz for easy analysis

**Measurement Method:**
```
1. Extract LFO trajectory from L and R channels
2. Cross-correlate L and R signals
3. Find time offset of maximum correlation: t_offset
4. Calculate phase offset: phase = 360° * t_offset * LFO_rate
5. Compare to expected phase setting
```

**Alternative: Hilbert Transform Method:**
```
1. Apply Hilbert transform to extract analytic signal
2. Calculate instantaneous phase: φ(t) = atan2(imag, real)
3. Phase difference: Δφ = φ_R - φ_L
4. Average over multiple cycles
```

**Pass Criteria:**
- Phase offset accuracy: ±5° for 0-180° range
- Phase stability: standard deviation < 3° over 10 cycles

---

### Protocol 5: Frequency Response Under Modulation

**Purpose:** Measure sideband generation and spectral characteristics

**Test Stimulus:**
- Carrier tone (1 kHz typical)
- LFO modulation at known rate
- Capture sufficient duration for frequency resolution

**Analysis:**
- FFT with high resolution (8192+ points, windowed)
- Identify carrier and sidebands
- Measure sideband spacing = LFO rate
- Measure sideband amplitude vs modulation depth

**Expected Behavior:**

**Chorus/Flanger (delay modulation):**
- Bessel function sideband pattern
- Sideband spacing = LFO rate
- Sideband amplitude proportional to modulation index

**Ring Modulator:**
- Sum and difference frequencies: f_carrier ± f_LFO
- Carrier suppression > 40 dB (for proper ring mod)
- Sideband amplitude = 0.5 × input amplitude (at 100% depth)

**Tremolo (amplitude modulation):**
- Sidebands at f_carrier ± f_LFO
- Sideband amplitude related to modulation depth m:
  - Upper sideband: m/2 × carrier amplitude
  - Lower sideband: m/2 × carrier amplitude

**Pass Criteria:**
- Sideband frequency accuracy: ±0.1 Hz
- Sideband amplitude matches theoretical prediction: ±2 dB

---

### Protocol 6: Stereo Width/Correlation Measurement

**Purpose:** Quantify stereo image characteristics

**Test Method:**
```
1. Process mono input with stereo effect
2. Calculate correlation: R = Σ(L[i] * R[i]) / sqrt(Σ(L[i]²) * Σ(R[i]²))
3. Calculate stereo width: W = 1 - |R|
4. Measure phase coherence across frequency
```

**Stereo Width Interpretation:**
- R = 1.0: Mono (no width)
- R = 0.0: Uncorrelated (maximum width)
- R = -1.0: Out-of-phase (unnatural, potential phase issues)

**Frequency-Dependent Width:**
- Calculate correlation in bands (low: 20-200 Hz, mid: 200-2k Hz, high: 2k-20k Hz)
- Some effects should preserve mono low end (bass focus)

**Pass Criteria:**
- Stereo width parameter correlates with measured width: R² > 0.9
- No unexpected phase reversals (R > -0.5 in any band)
- Low frequency correlation > 0.7 (prevents bass phase issues)

---

## COMMON TEST UTILITIES

### Utility 1: LFO Trajectory Extractor

**For Delay-Based Effects (Chorus, Flanger):**
```cpp
std::vector<float> extractDelayModulation(AudioBuffer& output,
                                          float inputFreq,
                                          float sampleRate) {
    // Use phase vocoder or instantaneous frequency analysis
    // Returns delay time as function of sample index
}
```

**For Amplitude Effects (Tremolo):**
```cpp
std::vector<float> extractAmplitudeEnvelope(AudioBuffer& output) {
    // Hilbert transform for envelope extraction
    // Returns instantaneous amplitude
}
```

**For Phase Effects (Phaser):**
```cpp
std::vector<float> extractAllPassFrequency(AudioBuffer& output,
                                           AudioBuffer& input) {
    // Measure group delay or phase response
    // Returns center frequency of all-pass sweep
}
```

---

### Utility 2: Zero-Crossing Frequency Estimator

```cpp
float estimateLFORate(const std::vector<float>& lfoTrajectory,
                      float sampleRate) {
    int zeroCrossings = 0;
    for (size_t i = 1; i < lfoTrajectory.size(); ++i) {
        if ((lfoTrajectory[i-1] < 0 && lfoTrajectory[i] >= 0) ||
            (lfoTrajectory[i-1] > 0 && lfoTrajectory[i] <= 0)) {
            zeroCrossings++;
        }
    }

    float duration = lfoTrajectory.size() / sampleRate;
    float frequency = zeroCrossings / (2.0f * duration); // 2 crossings per cycle
    return frequency;
}
```

---

### Utility 3: Autocorrelation Period Detector

```cpp
float autocorrelationPeriod(const std::vector<float>& signal,
                           float sampleRate) {
    // Autocorrelation R[τ] = Σ x[n] * x[n+τ]
    // Find first peak after zero lag (period)
    // Return frequency = sampleRate / period_samples
}
```

---

### Utility 4: Spectral Sideband Analyzer

```cpp
struct SidebandMeasurement {
    float carrierFreq;
    float carrierAmplitude;
    std::vector<float> sidebandFreqs;
    std::vector<float> sidebandAmplitudes;
};

SidebandMeasurement analyzeSidebands(const AudioBuffer& output,
                                    float expectedCarrier,
                                    float expectedLFO) {
    // FFT-based analysis
    // Peak detection around carrier ± n*LFO
    // Return measured frequencies and amplitudes
}
```

---

## ENGINE-SPECIFIC CONSIDERATIONS

### Chorus Engines
- **Key Parameters:** Rate, Depth, Delay, Width, Feedback
- **Specific Tests:** Comb filtering (notches), feedback stability
- **Unique Challenge:** Multi-voice chorus (6 voices max in ResonantChorus)

### Phaser Engines
- **Key Parameters:** Rate, Depth, Stages, Feedback, Center Frequency
- **Specific Tests:** All-pass stage counting, notch spacing, Q measurement
- **Unique Challenge:** Discrete stage counts (2, 4, 6, 8)

### Tremolo Engines
- **Key Parameters:** Rate, Depth, Type (sine/optical/bias/harmonic)
- **Specific Tests:** Optical envelope response, harmonic content
- **Unique Challenge:** Multiple tremolo algorithms in ClassicTremolo

### Ring Modulator
- **Key Parameters:** Carrier Frequency, Ring Amount, Frequency Shift
- **Specific Tests:** Carrier suppression, sideband accuracy, Hilbert transform quality
- **Unique Challenge:** Frequency shifter mode (SSB modulation)

### Rotary Speaker
- **Key Parameters:** Speed, Acceleration, Horn/Drum rates, Doppler depth
- **Specific Tests:** Crossover accuracy (800 Hz), Doppler shift measurement, rotor inertia
- **Unique Challenge:** Dual-rotor simulation with different speeds

---

## MUSICAL VALIDATION TESTS

Beyond objective measurements, each engine should be validated musically:

### Listening Test Protocol
1. **Dry/Wet Comparison:** A/B switch at 50% mix
2. **Parameter Sweep:** Slowly sweep key parameters during playback
3. **Bypass Glitch Check:** No clicks/pops when enabling/disabling
4. **Noise Floor:** Inaudible noise at zero input

### Musical Test Signals
- **Electric guitar:** Clean tone for chorus, distorted for tremolo
- **Synthesizer pad:** Sustained for phaser sweep observation
- **Drum loop:** Stereo width evaluation
- **Vocal:** Natural pitch for ring mod/freq shift weirdness check

---

## TEST EXECUTION FRAMEWORK

### Test Harness Requirements
```cpp
class ModulationEngineTestHarness {
public:
    // Setup
    void prepareEngine(double sampleRate, int blockSize);
    void resetEngineState();
    void setParameter(int index, float value);

    // Stimulus generation
    AudioBuffer generateSilence(int numSamples);
    AudioBuffer generateTone(float freq, int numSamples);
    AudioBuffer generateImpulse(int numSamples);
    AudioBuffer generateWhiteNoise(int numSamples);

    // Processing
    AudioBuffer processBuffer(const AudioBuffer& input);

    // Analysis
    std::vector<float> extractLFOTrajectory(const AudioBuffer& output);
    float measureLFORate(const std::vector<float>& trajectory);
    float measureModulationDepth(const std::vector<float>& trajectory);
    float measureStereoPhase(const AudioBuffer& stereoOutput);
    SidebandMeasurement analyzeSidebands(const AudioBuffer& output);

    // Validation
    bool validateRateAccuracy(float measured, float expected, float tolerance);
    bool validateDepthAccuracy(float measured, float expected, float tolerance);
    bool validateWaveformShape(const std::vector<float>& trajectory,
                              LFOShape shape);
};
```

---

## PER-ENGINE TEST COUNT TARGETS

| Engine | LFO Tests | Stereo Tests | Frequency Tests | Stability Tests | Musical Tests | **TOTAL** |
|--------|-----------|--------------|-----------------|-----------------|---------------|-----------|
| StereoChorus | 5 | 4 | 3 | 2 | 2 | **16** |
| ResonantChorus | 6 | 5 | 4 | 3 | 3 | **21** |
| AnalogPhaser | 5 | 4 | 5 | 3 | 3 | **20** |
| ClassicTremolo | 7 | 4 | 4 | 2 | 4 | **21** |
| HarmonicTremolo | 5 | 3 | 4 | 2 | 4 | **18** |
| PlatinumRingMod | 4 | 3 | 6 | 3 | 3 | **19** |
| RotarySpeaker | 6 | 5 | 5 | 4 | 4 | **24** |
| FrequencyShifter | 3 | 3 | 6 | 3 | 3 | **18** |

**TOTAL MODULATION TESTS: ~157 comprehensive tests**

---

## SUCCESS CRITERIA SUMMARY

**Individual Test Pass:** All assertions within tolerance
**Engine Pass:** ≥95% of tests passing
**Category Pass:** All engines passing

**Critical Tests (must pass 100%):**
- LFO rate accuracy at 0.1, 1.0, 10.0 Hz
- Bypass safety (no explosions, clicks, NaN)
- Stereo phase at 0° and 180°
- Modulation depth at 0% and 100%

**Non-Critical Tests (90% pass acceptable):**
- Extreme LFO rates (0.01 Hz, 50 Hz)
- Complex waveform shapes (random, sample-hold)
- Edge case parameter combinations

---

## NEXT STEPS

See detailed test specifications in:
1. **MODULATION_CHORUS_FLANGER_PHASER.md** - Delay/phase-based modulation
2. **MODULATION_TREMOLO_VIBRATO_UNIVIBE.md** - Amplitude-based modulation
3. **MODULATION_RING_ROTARY_PAN.md** - Frequency/spatial modulation

Each document contains 18-25 specific test cases per engine with exact stimulus, measurement procedures, and pass/fail criteria.
