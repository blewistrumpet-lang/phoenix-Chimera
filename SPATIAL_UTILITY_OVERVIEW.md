# Spatial & Utility Engines Test Design Overview

**Project:** Chimera Phoenix v3.0
**Category:** Spatial Effects, Spectral Processing, Utility Engines
**Document Version:** 1.0
**Date:** 2025-10-10

---

## Executive Summary

This document outlines comprehensive testing protocols for 15+ spatial, spectral, and utility audio engines in the Chimera Phoenix plugin. These engines manipulate stereo field, spatial positioning, frequency-domain processing, and basic signal utilities.

---

## Engine Categories

### 1. SPATIAL EFFECTS (6 engines)
- **StereoImager** - Advanced stereo field manipulation with multi-band control
- **StereoWidener** - Haas-based stereo widening with bass mono
- **PhaseAlign_Platinum** - Multi-band phase alignment and correlation
- **MidSideProcessor_Platinum** - Precision M/S encoding/decoding
- **MonoMaker_Platinum** - Frequency-selective mono conversion
- **DimensionExpander** - 3D spatial enhancement with movement

### 2. SPECTRAL PROCESSORS (3 engines)
- **SpectralFreeze** - FFT-based spectral freezing and manipulation
- **SpectralGate** - Frequency-domain noise gating
- **PhasedVocoder** - Time/pitch manipulation with phase vocoding

### 3. SPECIAL PROCESSORS (2 engines)
- **GranularCloud** - Granular synthesis and time-stretching
- **ChaosGenerator** - Chaotic system-based modulation

### 4. UTILITY ENGINES (2 engines)
- **GainUtility_Platinum** - Precision gain control and metering
- **PhaseAlign_Platinum** - Phase correction utilities

---

## Common Testing Protocols

### A. Stereo Correlation Measurement

**Purpose:** Validate stereo field coherence and width accuracy

**Method:**
```cpp
float calculateCorrelation(const AudioBuffer<float>& buffer) {
    float sumLR = 0.0f, sumLL = 0.0f, sumRR = 0.0f;
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; i++) {
        float L = buffer.getSample(0, i);
        float R = buffer.getSample(1, i);
        sumLR += L * R;
        sumLL += L * L;
        sumRR += R * R;
    }

    float denominator = std::sqrt(sumLL * sumRR);
    return (denominator > 1e-10f) ? (sumLR / denominator) : 0.0f;
}
```

**Acceptance Criteria:**
- Mono signal: correlation >= 0.95 (highly correlated)
- Stereo signal: 0.0 < correlation < 0.9
- Anti-phase: correlation <= -0.9
- Measurement precision: ±0.01

---

### B. Phase Coherence Testing

**Purpose:** Ensure phase relationships don't cause cancellation

**Test Signals:**
1. **Mono Compatibility Test**
   - Input: Stereo with phase differences
   - Process: Sum to mono (L+R)/2
   - Measure: Energy loss vs original
   - Pass: < 3dB loss for musical content

2. **Bass Phase Test**
   - Input: 80Hz sine wave with 90° phase offset
   - Expected: Below 200Hz should be phase-aligned
   - Measure: Phase difference in degrees
   - Pass: < 15° below crossover frequency

**Implementation:**
```cpp
struct PhaseTest {
    float frequencyHz;
    float expectedPhaseDeg;
    float toleranceDeg;

    bool run(AudioProcessor& engine) {
        auto signal = generateSineWithPhase(frequencyHz, phaseOffset);
        engine.process(signal);
        float measuredPhase = calculatePhase(signal);
        return std::abs(measuredPhase - expectedPhaseDeg) < toleranceDeg;
    }
};
```

---

### C. FFT Accuracy Validation

**Purpose:** Verify spectral processing maintains frequency accuracy

**Reference Test:**
1. Generate multi-tone signal (440Hz, 880Hz, 1320Hz)
2. Process through FFT engine
3. Analyze output spectrum
4. Compare bin magnitudes and phases

**Acceptance Criteria:**
- Magnitude accuracy: ±0.5dB per bin
- Phase accuracy: ±5° per bin
- No spectral leakage beyond ±2 bins
- Flat noise floor: < -90dB

**FFT Validation Code:**
```cpp
bool validateFFTAccuracy(const std::vector<float>& input,
                         const std::vector<float>& output,
                         int fftSize) {
    juce::dsp::FFT fftRef(std::log2(fftSize));

    std::vector<float> refSpectrum(fftSize * 2);
    fftRef.performRealOnlyForwardTransform(refSpectrum.data());

    // Compare spectral magnitudes
    for (int i = 0; i < fftSize / 2; i++) {
        float refMag = std::sqrt(refSpectrum[i*2]^2 + refSpectrum[i*2+1]^2);
        float testMag = std::sqrt(output[i*2]^2 + output[i*2+1]^2);
        float errorDB = 20.0f * std::log10(std::abs(refMag - testMag) / refMag);

        if (errorDB > 0.5f) return false;
    }

    return true;
}
```

---

### D. Transparency Testing (Null Test)

**Purpose:** Validate bypass and unity settings produce no audible artifacts

**Null Test Protocol:**
1. Record dry input signal
2. Process through engine at bypass/unity settings
3. Invert and sum with original
4. Measure residual RMS level

**Pass Criteria:**
- Residual level: < -100dB
- No clicks or discontinuities
- Frequency response flat: ±0.01dB

**Implementation:**
```cpp
struct NullTest {
    static float measureTransparency(EngineBase* engine) {
        const int bufferSize = 4096;
        AudioBuffer<float> original(2, bufferSize);
        AudioBuffer<float> processed(2, bufferSize);

        // Generate reference signal
        generatePinkNoise(original);
        processed.makeCopyOf(original);

        // Process with bypass/unity settings
        std::map<int, float> bypassParams = {{7, 0.0f}}; // Mix = 0%
        engine->updateParameters(bypassParams);
        engine->process(processed);

        // Calculate null depth
        float residual = 0.0f;
        for (int ch = 0; ch < 2; ch++) {
            for (int i = 0; i < bufferSize; i++) {
                float diff = processed.getSample(ch, i) - original.getSample(ch, i);
                residual += diff * diff;
            }
        }

        float rms = std::sqrt(residual / (bufferSize * 2));
        return 20.0f * std::log10(rms); // Convert to dB
    }
};
```

---

## Test Signal Library

### 1. Standard Test Signals

| Signal Type | Purpose | Specifications |
|------------|---------|----------------|
| **Sine Wave** | Frequency response, phase | 20Hz - 20kHz, -18dBFS |
| **Multi-Tone** | Intermodulation | 440Hz, 880Hz, 1320Hz |
| **Pink Noise** | General frequency response | -20dBFS RMS |
| **White Noise** | High-frequency analysis | -20dBFS RMS |
| **Impulse** | Latency, transient response | 1 sample peak at 0dBFS |
| **DC Offset** | DC blocking validation | +0.5 DC level |
| **Silence** | Noise floor, denormals | All zeros |

### 2. Spatial Test Signals

| Signal | L Channel | R Channel | Purpose |
|--------|-----------|-----------|---------|
| **Pure Mid** | +1.0 | +1.0 | Mid channel isolation |
| **Pure Side** | +1.0 | -1.0 | Side channel isolation |
| **L-Only** | +1.0 | 0.0 | Channel independence |
| **90° Phase** | sin(ωt) | cos(ωt) | Phase relationship |

### 3. Edge Case Signals

- **Very Loud:** Peak at +6dBFS (test clipping protection)
- **Very Quiet:** -90dBFS (test noise floor)
- **Denormals:** 1e-30 (test denormal protection)
- **NaN/Inf:** (should be rejected safely)

---

## Performance Benchmarks

### CPU Usage Targets

| Engine Category | Max CPU @ 96kHz/64 samples |
|----------------|----------------------------|
| Spatial Effects | < 5% |
| Spectral (FFT) | < 15% |
| Utility | < 1% |
| Special FX | < 20% |

### Memory Requirements

- **Pre-allocation:** All buffers allocated in prepareToPlay()
- **Real-time safety:** Zero allocations during process()
- **Total memory:** < 10MB per engine instance

### Latency Requirements

| Engine Type | Maximum Latency |
|------------|-----------------|
| Spatial (non-FFT) | 0 samples |
| Spectral/FFT | 2048 samples @ 48kHz |
| Utility | 0 samples |

---

## Measurement Precision Standards

### Audio Measurement Tolerances

| Parameter | Tolerance | Measurement Method |
|-----------|-----------|-------------------|
| **Gain Accuracy** | ±0.01 dB | RMS comparison |
| **Frequency Response** | ±0.1 dB | Swept sine analysis |
| **Phase Response** | ±1° | FFT phase extraction |
| **THD+N** | < 0.001% | Spectrum analysis |
| **Stereo Width** | ±5% | Correlation coefficient |
| **Latency** | ±1 sample | Impulse response |

---

## Test Automation Framework

### Test Structure (GoogleTest)

```cpp
class SpatialEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<EngineType>();
        engine->prepareToPlay(48000.0, 512);
        testBuffer.setSize(2, 512);
    }

    void TearDown() override {
        engine->reset();
    }

    std::unique_ptr<EngineBase> engine;
    AudioBuffer<float> testBuffer;

    // Common test utilities
    float measureStereoWidth();
    float calculatePhaseCorrelation();
    bool validateMonoCompatibility();
};

TEST_F(SpatialEngineTest, StereoWidthRange) {
    // Test implementation
}
```

---

## Common Test Cases (All Engines)

### 1. Initialization Tests
- Engine creation succeeds
- Parameter count correct
- Default values loaded from UnifiedDefaultParameters
- prepareToPlay() initializes state

### 2. Parameter Validation
- All parameter names non-empty
- Parameter ranges [0.0, 1.0]
- updateParameters() thread-safe
- Smooth parameter transitions (no clicks)

### 3. Audio Processing
- Processes silence without crashes
- Handles loud signals (no overflow)
- Produces finite output (no NaN/Inf)
- Mix parameter works (0% = dry, 100% = wet)

### 4. Edge Cases
- Zero-length buffers rejected gracefully
- Non-stereo buffers handled
- Rapid parameter changes stable
- Sample rate changes supported

### 5. Performance
- Real-time capable at max CPU target
- Memory footprint within limits
- No allocations in process()
- Latency within specification

---

## Reporting Standards

### Test Report Format

```markdown
## Engine: [Name]
**Date:** [ISO Date]
**Tester:** [Name]
**Platform:** [macOS/Windows/Linux, CPU, RAM]

### Test Results
- **Tests Passed:** X/Y (Z%)
- **Critical Failures:** [List]
- **Warnings:** [List]

### Performance Metrics
- **CPU Usage:** X% @ 96kHz/64 samples
- **Memory:** XMB
- **Latency:** X samples

### Detailed Results
[Table of all test cases with pass/fail status]

### Recommendations
[Action items, improvements, known issues]
```

---

## Critical Test Priorities

### P0 - Must Pass (Blocker)
- No crashes or hangs
- No NaN/Inf output
- Bypass mode transparent (< -100dB residual)
- Memory safe (no leaks)

### P1 - High Priority
- Parameter accuracy within spec
- Phase coherence maintained
- Stereo width accurate ±5%
- CPU usage within target

### P2 - Medium Priority
- Thermal/aging modeling accuracy
- Edge case handling
- Parameter smoothing quality
- Visual metering accuracy

### P3 - Nice to Have
- Extended frequency range tests
- Multi-sample-rate validation
- Stress testing (hours of operation)
- Comparative analysis vs reference plugins

---

## Next Steps

1. Review detailed engine-specific test plans:
   - **SPATIAL_ENGINES.md** - All stereo/spatial processors
   - **SPECTRAL_ENGINES.md** - FFT-based effects
   - **UTILITY_ENGINES.md** - Gain, metering, utility functions

2. Implement test framework in `/JUCE_Plugin/Tests/`

3. Create automated CI/CD integration

4. Document findings and create bug reports

---

## Reference Documents

- **Engine Source Code:** `/JUCE_Plugin/Source/`
- **Existing Tests:** `/JUCE_Plugin/Tests/SpatialEffects/`, `/Tests/Utility/`
- **Parameter Defaults:** `UnifiedDefaultParameters.h`
- **DSP Utilities:** `DspEngineUtilities.h`

---

**Document Status:** FINAL DRAFT
**Requires Review:** DSP Engineering Team, QA Lead
**Implementation Target:** Q4 2025
