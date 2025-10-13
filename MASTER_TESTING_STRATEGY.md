# ChimeraPhoenix DSP Engine Testing Suite
## Master Testing Strategy & Implementation Guide

**Version**: 2.0
**Date**: October 10, 2025
**Author**: Master Architect - Claude Code
**Target**: 57 Production-Grade Audio DSP Engines
**Platform**: macOS (Intel/Apple Silicon)
**Scope**: Comprehensive Validation, Regression Testing, Performance Benchmarking

---

## Executive Summary

This document provides the complete architectural foundation for testing all 57 audio DSP engines in Project Chimera Phoenix. The testing suite is designed to validate production-quality audio processing, detect regressions, measure performance, and ensure studio-grade reliability.

**Key Objectives**:
- Validate correct audio processing for all 57 engines
- Ensure stability under real-world conditions
- Measure and benchmark performance (CPU, latency, memory)
- Generate comprehensive reports with audio artifacts and visual analysis
- Create regression test baseline for ongoing development
- Enable automated CI/CD quality gates

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Engine Taxonomy](#2-engine-taxonomy)
3. [Test Signal Library](#3-test-signal-library)
4. [Measurement Methodologies](#4-measurement-methodologies)
5. [Category-Specific Test Strategies](#5-category-specific-test-strategies)
6. [Test Harness Architecture](#6-test-harness-architecture)
7. [Output Formats & Reporting](#7-output-formats--reporting)
8. [Performance Benchmarking](#8-performance-benchmarking)
9. [Success Criteria](#9-success-criteria)
10. [Implementation Roadmap](#10-implementation-roadmap)
11. [Tools & Dependencies](#11-tools--dependencies)

---

## 1. Architecture Overview

### 1.1 Testing Philosophy

**NO MOCKS. REAL DATA. REAL PROCESSING.**

Every test will:
- Use actual DSP engines (not mocks or stubs)
- Process real audio signals
- Measure real outputs
- Generate real artifacts (WAV files, spectrograms, metrics)
- Run on real hardware (Mac, not simulated)

### 1.2 Three-Tier Testing Strategy

```
┌─────────────────────────────────────────────────┐
│  TIER 1: UNIT TESTS (Per Engine)               │
│  - Parameter validation                         │
│  - Signal processing correctness                │
│  - Stability & safety                           │
│  - Basic functionality                          │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│  TIER 2: INTEGRATION TESTS (Categories)        │
│  - Cross-engine compatibility                   │
│  - Parameter automation                         │
│  - Block size variations                        │
│  - Sample rate changes                          │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│  TIER 3: STRESS & PERFORMANCE TESTS             │
│  - 1000-iteration stability                     │
│  - CPU profiling                                │
│  - Memory leak detection                        │
│  - Extreme parameter sweeps                     │
└─────────────────────────────────────────────────┘
```

### 1.3 Test Harness Components

```cpp
// Core Test Infrastructure
namespace ChimeraTest {
    // Test execution engine
    class TestRunner {
        void runAllTests();
        void runCategory(EngineCategory cat);
        void runEngine(int engineID);
    };

    // Signal generation
    class SignalGenerator {
        AudioBuffer generateSine(freq, amplitude, duration);
        AudioBuffer generateNoise(type, amplitude, duration);
        AudioBuffer generateImpulse(amplitude, position);
        AudioBuffer generateSweep(startFreq, endFreq, duration);
        AudioBuffer generateMusicContent(type);  // Drums, vocals, mix
    };

    // Analysis & measurement
    class AudioAnalyzer {
        FFTResult performFFT(buffer);
        float measureTHD(buffer);
        float measureRMS(buffer);
        float measurePeak(buffer);
        float detectPitch(buffer);
        EnvelopeData extractEnvelope(buffer);
        SpectrogramData generateSpectrogram(buffer);
    };

    // Reporting
    class ReportGenerator {
        void generateJSON(results);
        void generateHTML(results);
        void saveWAVFile(buffer, filename);
        void saveSpectrogram(data, filename);
        void generateSummaryDashboard(allResults);
    };
}
```

---

## 2. Engine Taxonomy

### 2.1 Complete Engine Classification

Based on `/JUCE_Plugin/Source/EngineTypes.h`, all 57 engines categorized by processing type:

#### **Category 1: DYNAMICS & COMPRESSION (6 engines)**
```
ID  Name                        Primary Test Focus
──────────────────────────────────────────────────────────
1   VintageOptoCompressor       Optical compression behavior, slow attack
2   ClassicCompressor (VCA)     Ratio accuracy, attack/release timing
3   TransientShaper             Attack/sustain separation, envelope detection
4   NoiseGate                   Threshold precision, hold/release timing
5   MasteringLimiter            True peak limiting, lookahead accuracy
6   DynamicEQ                   Frequency-selective compression
```

#### **Category 2: FILTERS & EQ (8 engines)**
```
ID  Name                        Primary Test Focus
──────────────────────────────────────────────────────────
7   ParametricEQ                Multi-band frequency response, Q accuracy
8   VintageConsoleEQ            Analog modeling, saturation behavior
9   LadderFilter                Self-oscillation, resonance stability
10  StateVariableFilter         Multi-mode operation (LP/HP/BP/Notch)
11  FormantFilter               Vowel accuracy, formant transitions
12  EnvelopeFilter              Envelope tracking, filter modulation
13  CombResonator               Harmonic resonances, feedback stability
14  VocalFormantFilter          Speech formant accuracy
```

#### **Category 3: DISTORTION & SATURATION (8 engines)**
```
ID  Name                        Primary Test Focus
──────────────────────────────────────────────────────────
15  VintageTubePreamp           Tube modeling, harmonic generation
16  WaveFolder                  Wavefolding topology, spectral expansion
17  HarmonicExciter             Even/odd harmonic enhancement
18  BitCrusher                  Bit reduction, sample rate reduction
19  MultibandSaturator          Frequency-split saturation
20  MuffFuzz                    Transistor modeling, sustain character
21  RodentDistortion            Clipping characteristics, tone shaping
22  KStyleOverdrive             Diode clipping, dynamic response
```

#### **Category 4: MODULATION EFFECTS (11 engines)**
```
ID  Name                        Primary Test Focus
──────────────────────────────────────────────────────────
23  DigitalChorus (StereoChorus) LFO modulation, stereo spread
24  ResonantChorus              Resonant filtering + modulation
25  AnalogPhaser                All-pass stages, feedback, sweeping
26  RingModulator (Analog)      Sideband generation, carrier frequency
27  FrequencyShifter            Linear frequency shifting (not pitch)
28  HarmonicTremolo             Multi-band amplitude modulation
29  ClassicTremolo              Simple LFO amplitude modulation
30  RotarySpeaker               Horn/rotor simulation, Doppler effect
31  PitchShifter                Pitch shifting accuracy (PSOLA/FFT)
32  DetuneDoubler               Micro-detuning, stereo thickening
33  IntelligentHarmonizer       Chord detection, harmonic generation
```

#### **Category 5: REVERB & DELAY (10 engines)**
```
ID  Name                        Primary Test Focus
──────────────────────────────────────────────────────────
34  TapeEcho                    Tape saturation, wow/flutter, feedback
35  DigitalDelay                Clean digital delay, modulation
36  MagneticDrumEcho            Drum echo character, degradation
37  BucketBrigadeDelay          Analog BBD character, dark tone
38  BufferRepeat                Glitch repeat, buffer manipulation
39  PlateReverb                 EMT-140 modeling, diffusion
40  SpringReverb                Spring characteristics, drip
41  ConvolutionReverb           IR accuracy, zero-latency mode
42  ShimmerReverb               Pitch shifting in feedback
43  GatedReverb                 Gate envelope, 80s character
```

#### **Category 6: SPATIAL & SPECIAL EFFECTS (9 engines)**
```
ID  Name                        Primary Test Focus
──────────────────────────────────────────────────────────
44  StereoWidener               Stereo field expansion
45  StereoImager                M/S processing, width control
46  DimensionExpander           Haas effect, spatial placement
47  SpectralFreeze              FFT freeze, time stretching
48  SpectralGate                Frequency-selective gating
49  PhasedVocoder               Phase vocoder implementation
50  GranularCloud               Grain generation, density, pitch
51  ChaosGenerator              Chaos algorithms, unpredictability
52  FeedbackNetwork             Complex feedback routing
```

#### **Category 7: UTILITY (4 engines)**
```
ID  Name                        Primary Test Focus
──────────────────────────────────────────────────────────
53  MidSideProcessor            M/S encoding/decoding accuracy
54  GainUtility                 Precision gain control
55  MonoMaker                   Stereo-to-mono folding
56  PhaseAlign                  Phase correction, delay compensation
```

### 2.2 Special Test Requirements by Engine

**Engines Requiring Musical Content**:
- IntelligentHarmonizer (needs chords)
- TransientShaper (needs drums)
- VocalFormantFilter (needs vocals)
- GranularCloud (needs complex material)

**Engines Requiring Tempo Sync**:
- TapeEcho, DigitalDelay (tempo-synced modes)
- ClassicTremolo, HarmonicTremolo
- BufferRepeat

**Engines with Latency**:
- MasteringLimiter (lookahead)
- ConvolutionReverb (IR length)
- PhasedVocoder (FFT block size)
- SpectralFreeze, SpectralGate

**Engines Requiring Extended Analysis**:
- All reverbs (RT60 measurement)
- All delays (echo detection)
- All pitch shifters (pitch detection)
- All distortions (THD+N measurement)

---

## 3. Test Signal Library

### 3.1 Standard Test Signals

```cpp
namespace TestSignals {
    // TONES
    const float TONE_440HZ    = 440.0f;   // A4 - pitch detection reference
    const float TONE_1KHZ     = 1000.0f;  // Standard calibration
    const float TONE_100HZ    = 100.0f;   // Low frequency test
    const float TONE_10KHZ    = 10000.0f; // High frequency test

    // CALIBRATION LEVELS
    const float LEVEL_0DB     = 1.0f;     // Full scale
    const float LEVEL_MINUS6DB  = 0.501f;   // -6dBFS
    const float LEVEL_MINUS12DB = 0.251f;   // -12dBFS
    const float LEVEL_MINUS18DB = 0.126f;   // -18dBFS

    // DURATIONS
    const double SHORT_BURST  = 0.1;      // 100ms
    const double MEDIUM_TONE  = 1.0;      // 1 second
    const double LONG_TAIL    = 5.0;      // 5 seconds (reverb decay)
    const double STRESS_TEST  = 30.0;     // 30 seconds (stability)

    // SIGNAL TYPES
    enum SignalType {
        SINE_WAVE,
        SQUARE_WAVE,
        TRIANGLE_WAVE,
        SAWTOOTH_WAVE,
        WHITE_NOISE,
        PINK_NOISE,
        IMPULSE,
        SWEEP_LINEAR,
        SWEEP_LOG,
        MUSICAL_DRUMS,
        MUSICAL_VOCALS,
        MUSICAL_GUITAR,
        MUSICAL_PIANO,
        MUSICAL_MIX
    };
}
```

### 3.2 Musical Content Library

For engines that need realistic material, create a library of:

1. **Drum Loops** (for TransientShaper, gates, compressors)
   - Kick, snare, hi-hat patterns
   - Various tempos (60-180 BPM)
   - Dynamic range: soft to aggressive

2. **Vocal Phrases** (for VocalFormant, harmonizers)
   - Male/female voices
   - Sustained vowels (A, E, I, O, U)
   - Melodic phrases

3. **Guitar Riffs** (for distortion, saturation)
   - Clean DI guitar
   - Chord progressions
   - Single note runs

4. **Piano** (for reverbs, spatial effects)
   - Sustained chords
   - Percussive attacks
   - Wide dynamic range

5. **Full Mixes** (for mastering tools, limiters)
   - Balanced stereo mixes
   - Various genres
   - Peak levels at -6dBFS

### 3.3 Signal Generation Implementation

```cpp
class SignalLibrary {
public:
    // Synthesized signals
    static AudioBuffer<float> generateSine(
        double frequency,
        double amplitude,
        double duration,
        double sampleRate = 48000.0,
        int numChannels = 2
    );

    static AudioBuffer<float> generateNoise(
        NoiseType type,        // WHITE, PINK, BROWN
        double amplitude,
        double duration,
        double sampleRate = 48000.0
    );

    static AudioBuffer<float> generateImpulse(
        double amplitude,
        int position,
        int totalSamples,
        int numChannels = 2
    );

    static AudioBuffer<float> generateSweep(
        double startFreq,
        double endFreq,
        SweepType type,        // LINEAR, LOGARITHMIC
        double amplitude,
        double duration,
        double sampleRate = 48000.0
    );

    // Load pre-recorded musical content
    static AudioBuffer<float> loadMusicalContent(
        const String& contentType  // "drums_60bpm", "vocals_male_A", etc.
    );

    // Generate standardized test suite
    static TestSignalSet generateStandardTestSet() {
        TestSignalSet set;
        set.impulse = generateImpulse(1.0, 0, 48000);
        set.sine440 = generateSine(440.0, 0.5, 1.0);
        set.sine1k = generateSine(1000.0, 0.5, 1.0);
        set.whiteNoise = generateNoise(WHITE, 0.1, 1.0);
        set.pinkNoise = generateNoise(PINK, 0.1, 1.0);
        set.sweep20_20k = generateSweep(20, 20000, LOGARITHMIC, 0.5, 5.0);
        return set;
    }
};
```

---

## 4. Measurement Methodologies

### 4.1 Frequency Domain Analysis

```cpp
class FrequencyAnalyzer {
public:
    struct FFTResult {
        std::vector<float> magnitudes;    // dB
        std::vector<float> phases;        // radians
        std::vector<float> frequencies;   // Hz
        float fundamentalFreq;            // Detected fundamental
        float spectralCentroid;           // Weighted frequency average
        float spectralSpread;             // Frequency distribution
    };

    FFTResult performFFT(
        const AudioBuffer<float>& buffer,
        int fftSize = 8192,
        WindowType window = HANN
    );

    // Measure frequency response
    struct FrequencyResponse {
        std::vector<float> frequencies;
        std::vector<float> gains_dB;
        std::vector<float> phases;
    };

    FrequencyResponse measureFrequencyResponse(
        EngineBase* engine,
        double sampleRate,
        FreqRange range = {20.0, 20000.0},
        int numPoints = 100
    );

    // Specialized measurements
    float measureTHD(const AudioBuffer<float>& signal, float fundamental);
    float measureTHDN(const AudioBuffer<float>& signal, float fundamental);
    float measureSNR(const AudioBuffer<float>& signal, const AudioBuffer<float>& noise);
    float measureCrestFactor(const AudioBuffer<float>& signal);
};
```

### 4.2 Time Domain Analysis

```cpp
class TimeAnalyzer {
public:
    // Envelope detection
    struct EnvelopeData {
        std::vector<float> envelope;
        float attackTime;      // 10% to 90%
        float releaseTime;     // 90% to 10%
        float peakValue;
        float sustainLevel;
    };

    EnvelopeData extractEnvelope(
        const AudioBuffer<float>& buffer,
        EnvelopeType type = RMS  // PEAK, RMS, TRUE_PEAK
    );

    // Transient detection
    struct TransientInfo {
        std::vector<int> transientPositions;
        std::vector<float> transientStrengths;
        float averageSpacing;
    };

    TransientInfo detectTransients(
        const AudioBuffer<float>& buffer,
        float threshold_dB = -20.0f
    );

    // Delay/Echo detection
    struct EchoData {
        std::vector<float> echoTimes;     // Seconds
        std::vector<float> echoLevels;    // dB relative to direct
        float rt60;                        // Reverb decay time
    };

    EchoData detectEchoes(
        const AudioBuffer<float>& impulseResponse,
        double sampleRate
    );

    // Correlation analysis
    float measureStereoCorrelation(const AudioBuffer<float>& stereoSignal);
    float measurePhaseCoherence(const AudioBuffer<float>& left,
                               const AudioBuffer<float>& right);
};
```

### 4.3 Pitch & Harmony Analysis

```cpp
class PitchAnalyzer {
public:
    // Pitch detection using multiple algorithms
    struct PitchResult {
        float frequency;           // Hz
        float confidence;          // 0.0 to 1.0
        float clarity;             // Harmonic clarity
        std::string noteName;      // "A4", "C#3", etc.
    };

    PitchResult detectPitch(
        const AudioBuffer<float>& buffer,
        double sampleRate,
        PitchAlgorithm algo = YIN  // YIN, AUTOCORRELATION, FFT
    );

    // Harmonic analysis
    struct HarmonicSpectrum {
        float fundamental;
        std::vector<float> harmonics;      // Levels in dB
        float evenToOdd;                   // Ratio
        float thd;                         // Total harmonic distortion
    };

    HarmonicSpectrum analyzeHarmonics(
        const AudioBuffer<float>& buffer,
        float fundamental,
        int numHarmonics = 10
    );

    // Chord detection (for Intelligent Harmonizer)
    struct ChordInfo {
        std::string chordName;     // "Cmaj7", "Gm", etc.
        std::vector<float> notes;  // Detected pitches
        float confidence;
    };

    ChordInfo detectChord(
        const AudioBuffer<float>& buffer,
        double sampleRate
    );
};
```

### 4.4 Modulation Analysis

```cpp
class ModulationAnalyzer {
public:
    // LFO rate detection
    struct ModulationInfo {
        float rate_Hz;             // LFO frequency
        float depth;               // Modulation depth (0-1)
        ModulationType type;       // AMPLITUDE, FREQUENCY, PHASE
        float phase;               // LFO phase offset
    };

    ModulationInfo detectModulation(
        const AudioBuffer<float>& modulatedSignal,
        double sampleRate
    );

    // Vibrato/Tremolo depth
    float measureModulationDepth(
        const AudioBuffer<float>& signal,
        ModulationType type
    );

    // Chorus/Flanger analysis
    struct ChorusCharacteristics {
        float delayTime;           // Average delay
        float delayVariation;      // Peak deviation
        float lfRate;              // Modulation rate
        int numVoices;             // Detected voices
    };

    ChorusCharacteristics analyzeChorus(
        const AudioBuffer<float>& output,
        const AudioBuffer<float>& input
    );
};
```

### 4.5 Dynamic Range & Compression

```cpp
class DynamicsAnalyzer {
public:
    // Compression curve measurement
    struct CompressionCurve {
        std::vector<float> inputLevels_dB;
        std::vector<float> outputLevels_dB;
        float ratio;               // Measured ratio
        float threshold_dB;        // Detected threshold
        float kneeWidth_dB;        // Soft knee width
    };

    CompressionCurve measureCompression(
        EngineBase* compressor,
        double sampleRate
    );

    // Attack/Release timing
    struct TimingMeasurement {
        float attackTime_ms;       // 10% to 90%
        float releaseTime_ms;      // 90% to 10%
        float accuracy;            // vs. expected
    };

    TimingMeasurement measureAttackRelease(
        EngineBase* dynamics,
        float expectedAttack_ms,
        float expectedRelease_ms,
        double sampleRate
    );

    // Gain reduction metering
    struct GainReductionData {
        std::vector<float> grCurve;    // Gain reduction over time
        float peakGR_dB;               // Maximum GR
        float averageGR_dB;            // RMS GR
    };

    GainReductionData measureGainReduction(
        const AudioBuffer<float>& input,
        const AudioBuffer<float>& output
    );
};
```

---

## 5. Category-Specific Test Strategies

### 5.1 DYNAMICS & COMPRESSION

**Test Protocol**:
```cpp
class DynamicsTestSuite {
    struct DynamicsTest {
        String engineName;

        // Test 1: Threshold Accuracy
        bool testThreshold() {
            // Send signal at threshold + 6dB
            // Verify compression engages
            // Measure actual threshold point
        }

        // Test 2: Ratio Verification
        bool testRatio() {
            // Sweep input from -40dB to 0dB
            // Measure output levels
            // Calculate actual ratio
            // Compare to setting
        }

        // Test 3: Attack Time
        bool testAttack() {
            // Send impulse
            // Measure time to 90% GR
            // Compare to parameter setting
        }

        // Test 4: Release Time
        bool testRelease() {
            // Stop signal
            // Measure time for GR to return
        }

        // Test 5: Knee Characteristics
        bool testKnee() {
            // Measure curve around threshold
            // Verify hard vs. soft knee
        }

        // Test 6: Sidechain (if supported)
        bool testSidechain() {
            // Send different sidechain signal
            // Verify compression follows SC
        }

        // Test 7: Lookahead (if applicable)
        bool testLookahead() {
            // Send impulse
            // Verify anticipatory gain reduction
            // Measure latency
        }
    };
};
```

**Pass Criteria**:
- Threshold detection: ±1 dB
- Ratio accuracy: ±10% (e.g., 4:1 should be 3.6:1 to 4.4:1)
- Attack/Release timing: ±20% of set value
- No pumping or instability
- Gain reduction visible and measurable

### 5.2 FILTERS & EQ

**Test Protocol**:
```cpp
class FilterTestSuite {
    struct FilterTest {
        // Test 1: Frequency Response
        bool testFrequencyResponse() {
            // Send white noise
            // Sweep cutoff frequency
            // Measure attenuation slope
            // Verify filter type (LP/HP/BP/Notch)
        }

        // Test 2: Q/Resonance
        bool testResonance() {
            // Set resonance to various levels
            // Measure peak magnitude at cutoff
            // Verify no self-oscillation (unless intended)
        }

        // Test 3: Filter Slope
        bool testSlope() {
            // Measure rolloff
            // Verify poles (12dB/oct, 24dB/oct, etc.)
        }

        // Test 4: Stability
        bool testStability() {
            // Extreme settings
            // Sweep cutoff rapidly
            // Verify no explosion or NaN
        }

        // Test 5: Phase Response
        bool testPhase() {
            // Measure phase shift
            // Verify linear phase (if claimed)
        }
    };
};
```

**Pass Criteria**:
- Cutoff frequency: ±5% of setting
- Q/resonance: measurable peak at cutoff
- Filter slope: within ±3dB/octave of spec
- No self-oscillation (unless designed for it)
- Stable across full parameter range

### 5.3 DISTORTION & SATURATION

**Test Protocol**:
```cpp
class DistortionTestSuite {
    struct DistortionTest {
        // Test 1: Harmonic Generation
        bool testHarmonics() {
            // Send pure 100Hz sine
            // Measure harmonics at 200, 300, 400Hz, etc.
            // Verify THD increases with drive
        }

        // Test 2: Even/Odd Harmonic Balance
        bool testHarmonicBalance() {
            // Tube should favor even (2nd, 4th)
            // Fuzz should have odd (3rd, 5th)
        }

        // Test 3: Clipping Behavior
        bool testClipping() {
            // Send increasing amplitude
            // Verify soft/hard clipping characteristic
            // Measure output ceiling
        }

        // Test 4: Dynamic Response
        bool testDynamics() {
            // Verify saturation responds to signal level
            // Check compression behavior
        }

        // Test 5: Frequency Dependency
        bool testFrequencyResponse() {
            // Some distortions are freq-dependent
            // Measure effect at different frequencies
        }
    };
};
```

**Pass Criteria**:
- THD increases with drive parameter
- Harmonic content matches design (tube = even, transistor = odd)
- Output stays bounded (no overflow)
- Frequency response appropriate for type
- DC offset removed

### 5.4 MODULATION EFFECTS

**Test Protocol**:
```cpp
class ModulationTestSuite {
    struct ModulationTest {
        // Test 1: LFO Rate
        bool testLFORate() {
            // Set rate to known value (e.g., 2 Hz)
            // Send steady tone
            // Detect modulation frequency
            // Verify rate accuracy
        }

        // Test 2: Modulation Depth
        bool testDepth() {
            // Set depth to various levels
            // Measure actual depth in output
            // Verify parameter scaling
        }

        // Test 3: Stereo Spread
        bool testStereoSpread() {
            // Send mono signal
            // Verify L/R phase difference
            // Measure stereo width
        }

        // Test 4: Waveform Shape
        bool testWaveform() {
            // Verify sine/triangle/square LFO
            // Measure harmonic content of modulation
        }

        // Test 5: Tempo Sync (if supported)
        bool testTempoSync() {
            // Set BPM
            // Verify modulation matches tempo
        }
    };
};
```

**Pass Criteria**:
- LFO rate: ±5% of setting
- Modulation depth measurable and controllable
- Stereo effects create width (correlation < 0.9)
- No aliasing or digital artifacts
- Smooth parameter changes (no zipper noise)

### 5.5 REVERB & DELAY

**Test Protocol**:
```cpp
class ReverbDelayTestSuite {
    struct ReverbTest {
        // Test 1: Impulse Response
        bool testImpulseResponse() {
            // Send impulse
            // Measure decay time (RT60)
            // Verify reverb tail extends beyond input
        }

        // Test 2: Decay Time
        bool testDecayTime() {
            // Measure RT60 at different Size settings
            // Verify parameter control
        }

        // Test 3: Frequency Diffusion
        bool testDiffusion() {
            // Send white noise
            // Verify spectral smearing
            // Measure diffusion parameter effect
        }

        // Test 4: Stereo Width
        bool testStereoWidth() {
            // Send mono impulse
            // Verify stereo output
            // Measure decorrelation
        }

        // Test 5: Stability
        bool testStability() {
            // Long-term test (30 seconds)
            // Verify no runaway feedback
            // Check for NaN/Inf
        }
    };

    struct DelayTest {
        // Test 1: Delay Time
        bool testDelayTime() {
            // Send click
            // Detect echo position
            // Measure actual delay time
        }

        // Test 2: Feedback
        bool testFeedback() {
            // Send impulse
            // Count echoes
            // Measure decay rate
        }

        // Test 3: Modulation (analog delays)
        bool testModulation() {
            // Detect wow/flutter
            // Measure modulation depth
        }

        // Test 4: Tempo Sync
        bool testTempoSync() {
            // Verify delay matches BPM
        }
    };
};
```

**Pass Criteria**:
- Reverb RT60 > 100ms (at minimum settings)
- Delay time: ±1ms or ±5% (whichever is larger)
- Feedback stable (no runaway)
- Stereo decorrelation measurable
- No clicks or pops during parameter changes

### 5.6 PITCH EFFECTS

**Test Protocol**:
```cpp
class PitchTestSuite {
    struct PitchTest {
        // Test 1: Pitch Accuracy
        bool testPitchAccuracy() {
            // Send 440Hz (A4)
            // Shift by +12 semitones
            // Measure output frequency
            // Should be 880Hz (A5) ±5Hz
        }

        // Test 2: Formant Preservation
        bool testFormants() {
            // Send vocal content
            // Verify formants don't shift (if claimed)
        }

        // Test 3: Latency
        bool testLatency() {
            // Measure processing delay
            // Verify reported latency matches actual
        }

        // Test 4: Artifacts
        bool testArtifacts() {
            // Listen for chirping, metallic sound
            // Measure intermodulation distortion
        }

        // Test 5: Polyphony (Harmonizer)
        bool testPolyphony() {
            // Send chord
            // Verify all notes detected and shifted
        }
    };
};
```

**Pass Criteria**:
- Pitch accuracy: ±10 cents (±1.7%)
- Latency reported and < 50ms (for real-time use)
- Minimal artifacts (THD+N < 0.5%)
- Formants preserved (if claimed)
- Stable across audio spectrum

### 5.7 SPATIAL EFFECTS

**Test Protocol**:
```cpp
class SpatialTestSuite {
    struct SpatialTest {
        // Test 1: Stereo Width
        bool testWidth() {
            // Send correlated stereo
            // Measure output correlation
            // Verify width parameter effect
        }

        // Test 2: Phase Coherence
        bool testPhaseCoherence() {
            // Verify no phase cancellation in mono
        }

        // Test 3: M/S Processing
        bool testMidSide() {
            // Send known M/S signal
            // Verify encoding/decoding accuracy
        }

        // Test 4: Haas Effect (Dimension Expander)
        bool testHaas() {
            // Measure inter-channel delay
            // Verify spatial positioning
        }
    };
};
```

**Pass Criteria**:
- Stereo width controllable and measurable
- Mono compatibility (no phase issues)
- M/S conversion accurate (if applicable)
- No artificial coloration

### 5.8 UTILITY

**Test Protocol**:
```cpp
class UtilityTestSuite {
    struct UtilityTest {
        // Test 1: Gain Accuracy (GainUtility)
        bool testGainAccuracy() {
            // Set gain to known value
            // Measure actual gain
            // Verify ±0.1dB accuracy
        }

        // Test 2: Phase Inversion
        bool testPhaseInversion() {
            // Invert phase
            // Verify 180° shift
        }

        // Test 3: M/S Encoding
        bool testMSEncoding() {
            // Encode to M/S
            // Decode back
            // Verify bit-perfect (or ±epsilon)
        }

        // Test 4: Mono Summing
        bool testMonoSum() {
            // Sum stereo to mono
            // Verify no level change
        }
    };
};
```

**Pass Criteria**:
- Gain accuracy: ±0.1dB
- Phase operations exact
- No unintended processing
- Bit-transparent at default settings

---

## 6. Test Harness Architecture

### 6.1 Directory Structure

```
Project_Chimera_v3.0_Phoenix/
├── JUCE_Plugin/
│   ├── Source/                    # Engine implementations
│   └── Tests/                     # Existing test structure
│
├── ChimeraTestSuite/              # NEW: Master test harness
│   ├── Source/
│   │   ├── Core/
│   │   │   ├── TestRunner.h/cpp
│   │   │   ├── TestCase.h/cpp
│   │   │   └── TestRegistry.h/cpp
│   │   │
│   │   ├── SignalGeneration/
│   │   │   ├── SignalGenerator.h/cpp
│   │   │   ├── SynthesizedSignals.h/cpp
│   │   │   └── MusicalContentLoader.h/cpp
│   │   │
│   │   ├── Analysis/
│   │   │   ├── FrequencyAnalyzer.h/cpp
│   │   │   ├── TimeAnalyzer.h/cpp
│   │   │   ├── PitchAnalyzer.h/cpp
│   │   │   ├── ModulationAnalyzer.h/cpp
│   │   │   └── DynamicsAnalyzer.h/cpp
│   │   │
│   │   ├── CategoryTests/
│   │   │   ├── DynamicsTests.h/cpp
│   │   │   ├── FilterTests.h/cpp
│   │   │   ├── DistortionTests.h/cpp
│   │   │   ├── ModulationTests.h/cpp
│   │   │   ├── ReverbDelayTests.h/cpp
│   │   │   ├── PitchTests.h/cpp
│   │   │   ├── SpatialTests.h/cpp
│   │   │   └── UtilityTests.h/cpp
│   │   │
│   │   ├── PerformanceBenchmark/
│   │   │   ├── CPUProfiler.h/cpp
│   │   │   ├── MemoryTracker.h/cpp
│   │   │   └── LatencyMeasurement.h/cpp
│   │   │
│   │   ├── Reporting/
│   │   │   ├── ReportGenerator.h/cpp
│   │   │   ├── JSONExporter.h/cpp
│   │   │   ├── HTMLDashboard.h/cpp
│   │   │   └── SpectrogramRenderer.h/cpp
│   │   │
│   │   └── Main.cpp               # Test harness entry point
│   │
│   ├── Resources/
│   │   ├── MusicalContent/        # Pre-recorded test audio
│   │   │   ├── drums_60bpm.wav
│   │   │   ├── vocals_male_A.wav
│   │   │   ├── guitar_clean.wav
│   │   │   └── piano_chords.wav
│   │   │
│   │   └── Templates/
│   │       ├── report_template.html
│   │       └── dashboard.css
│   │
│   ├── Output/                    # Generated test results
│   │   ├── TestResults/
│   │   │   └── YYYY-MM-DD_HH-MM-SS/
│   │   │       ├── summary.json
│   │   │       ├── dashboard.html
│   │   │       ├── audio/         # Output WAV files
│   │   │       ├── spectrograms/  # PNG spectrograms
│   │   │       └── logs/          # Detailed logs
│   │   │
│   │   └── Baseline/              # Regression baseline
│   │       ├── engine_01_baseline.json
│   │       ├── engine_02_baseline.json
│   │       └── ...
│   │
│   ├── CMakeLists.txt             # Build configuration
│   └── README.md                  # Test suite documentation
│
└── TestResults/                   # Legacy test results (preserve)
```

### 6.2 Test Framework Class Hierarchy

```cpp
namespace ChimeraTest {

// Base test case
class TestCase {
public:
    virtual ~TestCase() = default;

    virtual String getName() const = 0;
    virtual String getDescription() const = 0;
    virtual bool run() = 0;

    struct Result {
        bool passed;
        String message;
        float confidence;          // 0.0 to 1.0
        std::map<String, float> metrics;
        std::vector<String> warnings;
    };

    Result getResult() const { return result; }

protected:
    Result result;
    void logInfo(const String& msg);
    void logWarning(const String& msg);
    void logError(const String& msg);
};

// Engine test base
class EngineTestCase : public TestCase {
public:
    EngineTestCase(int engineID, const String& engineName)
        : engineID(engineID), engineName(engineName) {}

    virtual void setupEngine() = 0;
    virtual void teardownEngine() = 0;

protected:
    int engineID;
    String engineName;
    std::unique_ptr<EngineBase> engine;
    double sampleRate = 48000.0;
    int blockSize = 512;

    // Helper methods
    AudioBuffer<float> processSignal(const AudioBuffer<float>& input);
    void setParameter(int paramIndex, float value);
    void resetEngine();
};

// Category-specific test suites
class DynamicsTestCase : public EngineTestCase {
public:
    bool testThreshold();
    bool testRatio();
    bool testAttackRelease();
    bool testKnee();
};

class FilterTestCase : public EngineTestCase {
public:
    bool testFrequencyResponse();
    bool testResonance();
    bool testStability();
};

// ... similar for other categories

// Test registry
class TestRegistry {
public:
    static TestRegistry& getInstance();

    void registerTest(std::unique_ptr<TestCase> test);
    std::vector<TestCase*> getTestsForEngine(int engineID);
    std::vector<TestCase*> getTestsForCategory(EngineCategory cat);
    std::vector<TestCase*> getAllTests();

private:
    std::vector<std::unique_ptr<TestCase>> tests;
};

// Test runner
class TestRunner {
public:
    struct Configuration {
        bool runAll = true;
        std::vector<int> specificEngines;
        std::vector<EngineCategory> specificCategories;
        bool verbose = false;
        bool generateAudio = true;
        bool generateSpectrograms = true;
        bool generateHTML = true;
        bool runPerformanceTests = true;
        String outputDirectory = "Output/TestResults";
    };

    TestRunner(const Configuration& config);

    void runTests();
    void generateReports();

    struct Summary {
        int totalTests;
        int passed;
        int failed;
        std::vector<TestCase::Result> results;
        std::chrono::milliseconds totalDuration;
    };

    Summary getSummary() const { return summary; }

private:
    Configuration config;
    Summary summary;
    std::unique_ptr<ReportGenerator> reporter;

    void runTestCase(TestCase* test);
    void logProgress(const String& message);
};

} // namespace ChimeraTest
```

### 6.3 Build System Integration

```cmake
# ChimeraTestSuite/CMakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(ChimeraTestSuite VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JUCE
find_package(JUCE CONFIG REQUIRED)

# Test harness executable
add_executable(ChimeraTestSuite
    Source/Main.cpp
    Source/Core/TestRunner.cpp
    Source/Core/TestCase.cpp
    Source/Core/TestRegistry.cpp

    Source/SignalGeneration/SignalGenerator.cpp
    Source/SignalGeneration/SynthesizedSignals.cpp
    Source/SignalGeneration/MusicalContentLoader.cpp

    Source/Analysis/FrequencyAnalyzer.cpp
    Source/Analysis/TimeAnalyzer.cpp
    Source/Analysis/PitchAnalyzer.cpp
    Source/Analysis/ModulationAnalyzer.cpp
    Source/Analysis/DynamicsAnalyzer.cpp

    Source/CategoryTests/DynamicsTests.cpp
    Source/CategoryTests/FilterTests.cpp
    Source/CategoryTests/DistortionTests.cpp
    Source/CategoryTests/ModulationTests.cpp
    Source/CategoryTests/ReverbDelayTests.cpp
    Source/CategoryTests/PitchTests.cpp
    Source/CategoryTests/SpatialTests.cpp
    Source/CategoryTests/UtilityTests.cpp

    Source/PerformanceBenchmark/CPUProfiler.cpp
    Source/PerformanceBenchmark/MemoryTracker.cpp
    Source/PerformanceBenchmark/LatencyMeasurement.cpp

    Source/Reporting/ReportGenerator.cpp
    Source/Reporting/JSONExporter.cpp
    Source/Reporting/HTMLDashboard.cpp
    Source/Reporting/SpectrogramRenderer.cpp
)

# Link JUCE modules
target_link_libraries(ChimeraTestSuite
    PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_dsp
)

# Link engine library
target_link_libraries(ChimeraTestSuite
    PRIVATE
        ChimeraEngines  # All 57 engines
)

# Include directories
target_include_directories(ChimeraTestSuite
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/../JUCE_Plugin/Source
)

# Compiler options
if(APPLE)
    target_compile_options(ChimeraTestSuite PRIVATE -Wall -Wextra -O3)
endif()

# Copy resources
add_custom_command(TARGET ChimeraTestSuite POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources
        $<TARGET_FILE_DIR:ChimeraTestSuite>/Resources
)
```

---

## 7. Output Formats & Reporting

### 7.1 JSON Test Results

```json
{
  "testRun": {
    "timestamp": "2025-10-10T14:30:00Z",
    "platform": "macOS 14.5.0 (Darwin 24.5.0)",
    "sampleRate": 48000,
    "blockSize": 512,
    "totalTests": 342,
    "passed": 335,
    "failed": 7,
    "duration_ms": 45230
  },
  "engines": [
    {
      "id": 1,
      "name": "VintageOptoCompressor",
      "category": "DYNAMICS",
      "tests": [
        {
          "name": "Threshold Detection",
          "passed": true,
          "confidence": 0.95,
          "metrics": {
            "expectedThreshold_dB": -12.0,
            "measuredThreshold_dB": -12.3,
            "error_dB": 0.3
          },
          "message": "Threshold accurate within ±1dB tolerance"
        },
        {
          "name": "Ratio Accuracy",
          "passed": true,
          "confidence": 0.92,
          "metrics": {
            "expectedRatio": 4.0,
            "measuredRatio": 3.85,
            "error_percent": 3.75
          }
        },
        {
          "name": "Attack Timing",
          "passed": true,
          "confidence": 0.88,
          "metrics": {
            "expectedAttack_ms": 10.0,
            "measuredAttack_ms": 11.2,
            "error_percent": 12.0
          }
        }
      ],
      "overallPassed": true,
      "overallConfidence": 0.92,
      "artifacts": {
        "audioFiles": [
          "audio/engine_01_input.wav",
          "audio/engine_01_output.wav"
        ],
        "spectrograms": [
          "spectrograms/engine_01_frequency_response.png",
          "spectrograms/engine_01_compression_curve.png"
        ]
      }
    }
  ],
  "categories": {
    "DYNAMICS": {
      "totalEngines": 6,
      "passed": 6,
      "failed": 0,
      "averageConfidence": 0.89
    },
    "FILTERS_EQ": {
      "totalEngines": 8,
      "passed": 8,
      "failed": 0,
      "averageConfidence": 0.93
    }
  },
  "failedEngines": [
    {
      "id": 47,
      "name": "SpectralFreeze",
      "failureReason": "No spectral freezing detected in output",
      "confidence": 0.15
    }
  ]
}
```

### 7.2 HTML Dashboard

Interactive HTML dashboard with:

```html
<!DOCTYPE html>
<html>
<head>
    <title>Chimera Phoenix Test Results</title>
    <link rel="stylesheet" href="dashboard.css">
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
</head>
<body>
    <header>
        <h1>ChimeraPhoenix DSP Engine Test Suite</h1>
        <div class="summary">
            <span class="passed">335 Passed</span>
            <span class="failed">7 Failed</span>
            <span class="confidence">Average Confidence: 91%</span>
        </div>
    </header>

    <section class="overview">
        <!-- Category breakdown pie chart -->
        <div id="categoryChart"></div>

        <!-- Confidence distribution histogram -->
        <div id="confidenceChart"></div>
    </section>

    <section class="engines">
        <!-- Expandable engine cards -->
        <div class="engine-card passed">
            <h3>01: VintageOptoCompressor</h3>
            <span class="status">✓ PASSED</span>
            <span class="confidence">92%</span>

            <div class="details">
                <h4>Tests</h4>
                <ul>
                    <li>✓ Threshold Detection (95%)</li>
                    <li>✓ Ratio Accuracy (92%)</li>
                    <li>✓ Attack Timing (88%)</li>
                </ul>

                <h4>Metrics</h4>
                <table>
                    <tr><td>Expected Threshold</td><td>-12.0 dB</td></tr>
                    <tr><td>Measured Threshold</td><td>-12.3 dB</td></tr>
                    <tr><td>Error</td><td>0.3 dB</td></tr>
                </table>

                <h4>Artifacts</h4>
                <audio controls src="audio/engine_01_output.wav"></audio>
                <img src="spectrograms/engine_01_frequency_response.png">
                <img src="spectrograms/engine_01_compression_curve.png">
            </div>
        </div>

        <div class="engine-card failed">
            <h3>47: SpectralFreeze</h3>
            <span class="status">✗ FAILED</span>
            <span class="confidence">15%</span>

            <div class="details">
                <p class="error">No spectral freezing detected in output</p>
                <!-- Details... -->
            </div>
        </div>
    </section>

    <section class="performance">
        <h2>Performance Benchmarks</h2>
        <!-- CPU usage charts, latency graphs, memory footprint -->
    </section>
</body>
</html>
```

### 7.3 Audio Artifacts

For each engine, generate:

```
Output/TestResults/2025-10-10_14-30-00/
├── audio/
│   ├── engine_01_input.wav           # Input signal
│   ├── engine_01_output.wav          # Processed output
│   ├── engine_01_dry_wet_50.wav      # Mix at 50%
│   ├── engine_01_sweep.wav           # Frequency sweep
│   └── ...
│
├── spectrograms/
│   ├── engine_01_input_spectrogram.png
│   ├── engine_01_output_spectrogram.png
│   ├── engine_01_frequency_response.png
│   ├── engine_01_compression_curve.png  (for dynamics)
│   ├── engine_01_harmonic_spectrum.png  (for distortion)
│   └── ...
```

### 7.4 Spectrogram Generation

```cpp
class SpectrogramRenderer {
public:
    struct SpectrogramConfig {
        int fftSize = 2048;
        int hopSize = 512;
        WindowType window = HANN;
        ColormapType colormap = VIRIDIS;
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;
        float minDB = -80.0f;
        float maxDB = 0.0f;
        int width = 1200;
        int height = 600;
    };

    void renderSpectrogram(
        const AudioBuffer<float>& audio,
        const String& outputPath,
        const SpectrogramConfig& config = {}
    );

    void renderComparison(
        const AudioBuffer<float>& input,
        const AudioBuffer<float>& output,
        const String& outputPath
    );
};
```

### 7.5 Console Output

```
╔═══════════════════════════════════════════════════════════════╗
║        ChimeraPhoenix DSP Engine Test Suite v2.0             ║
║                    Master Test Run                            ║
╚═══════════════════════════════════════════════════════════════╝

Configuration:
  Platform:     macOS 14.5.0 (Darwin 24.5.0)
  Sample Rate:  48000 Hz
  Block Size:   512 samples
  Output Dir:   Output/TestResults/2025-10-10_14-30-00

Starting test run for 57 engines...

═══════════════════════════════════════════════════════════════
 CATEGORY: DYNAMICS & COMPRESSION (6 engines)
═══════════════════════════════════════════════════════════════

[1/57] VintageOptoCompressor
  ✓ Threshold Detection          [92%] ─────────────────── 0.8s
  ✓ Ratio Accuracy                [89%] ─────────────────── 1.2s
  ✓ Attack/Release Timing         [86%] ─────────────────── 2.1s
  ✓ Knee Characteristics          [91%] ─────────────────── 0.9s
  ✓ Stability Test                [95%] ─────────────────── 3.5s
  ✓ PASSED (Overall: 91%)                  Total: 8.5s

[2/57] ClassicCompressor
  ✓ Threshold Detection          [95%] ─────────────────── 0.7s
  ✓ Ratio Accuracy                [93%] ─────────────────── 1.1s
  ✓ Attack/Release Timing         [90%] ─────────────────── 2.0s
  ✓ Lookahead Processing          [88%] ─────────────────── 1.5s
  ✓ Sidechain Filtering           [92%] ─────────────────── 1.8s
  ✓ PASSED (Overall: 92%)                  Total: 7.1s

[3/57] TransientShaper
  ✓ Attack Enhancement           [94%] ─────────────────── 1.2s
  ✓ Sustain Reduction            [89%] ─────────────────── 1.4s
  ✓ Envelope Detection           [91%] ─────────────────── 1.0s
  ✓ PASSED (Overall: 91%)                  Total: 3.6s

...

═══════════════════════════════════════════════════════════════
 CATEGORY: DISTORTION & SATURATION (8 engines)
═══════════════════════════════════════════════════════════════

[15/57] VintageTubePreamp
  ✓ Harmonic Generation          [93%] ─────────────────── 1.5s
  ✓ Even Harmonic Dominance      [90%] ─────────────────── 1.2s
  ✓ Tube Saturation Curve        [88%] ─────────────────── 1.8s
  ✓ PASSED (Overall: 90%)                  Total: 4.5s

...

═══════════════════════════════════════════════════════════════
 TEST SUMMARY
═══════════════════════════════════════════════════════════════

Total Engines:        57
Tests Executed:       342
Passed:               335 (98.0%)
Failed:               7 (2.0%)
Average Confidence:   91%
Total Duration:       12m 45s

PASSED Engines: 50/57
  ✓ All Dynamics engines (6/6)
  ✓ All Filter engines (8/8)
  ✓ All Distortion engines (8/8)
  ✓ All Modulation engines (11/11)
  ✓ 8/10 Reverb/Delay engines
  ✓ 5/9 Spatial/Special engines
  ✓ All Utility engines (4/4)

FAILED Engines: 7/57
  ✗ PlateReverb          [15%] - No reverb tail detected
  ✗ SpringReverb         [22%] - Insufficient decay time
  ✗ SpectralFreeze       [18%] - No spectral holding
  ✗ SpectralGate         [25%] - Frequency gating not working
  ✗ GranularCloud        [12%] - No grain generation
  ✗ ChaosGenerator       [20%] - Output too predictable
  ✗ FeedbackNetwork      [16%] - No feedback detected

Reports generated:
  📄 summary.json
  📊 dashboard.html
  🎵 342 audio artifacts (WAV)
  📈 684 spectrograms (PNG)

Full results: Output/TestResults/2025-10-10_14-30-00/

═══════════════════════════════════════════════════════════════
```

---

## 8. Performance Benchmarking

### 8.1 CPU Profiling

```cpp
class CPUProfiler {
public:
    struct CPUMetrics {
        float averageCPU;          // Percentage (0-100)
        float peakCPU;             // Peak usage
        float minCPU;              // Minimum usage
        std::chrono::nanoseconds processingTime;
        float efficiency;          // samples processed per microsecond
    };

    CPUMetrics profileEngine(
        EngineBase* engine,
        const AudioBuffer<float>& testSignal,
        int numIterations = 1000
    );

    // Real-time performance test
    bool testRealTimePerformance(
        EngineBase* engine,
        double sampleRate,
        int blockSize
    ) {
        // Process must complete in less than block duration
        auto maxDuration = std::chrono::microseconds(
            (int64_t)(blockSize * 1000000 / sampleRate)
        );

        auto start = std::chrono::high_resolution_clock::now();
        // ... process ...
        auto end = std::chrono::high_resolution_clock::now();

        return (end - start) < maxDuration;
    }
};
```

### 8.2 Memory Tracking

```cpp
class MemoryTracker {
public:
    struct MemoryMetrics {
        size_t initialization;      // Bytes allocated in constructor
        size_t preparation;         // Bytes allocated in prepareToPlay
        size_t processing;          // Bytes allocated during process (should be 0!)
        size_t total;               // Total footprint
        bool realTimeSafe;          // No allocations in process()
    };

    MemoryMetrics profileEngine(EngineBase* engine, double sampleRate, int blockSize);

    // Detect memory leaks
    bool detectLeaks(EngineBase* engine, int numCycles = 1000);
};
```

### 8.3 Latency Measurement

```cpp
class LatencyMeasurement {
public:
    struct LatencyMetrics {
        int reportedLatency_samples;
        int measuredLatency_samples;
        int error_samples;
        bool accurate;              // Error within ±1 sample
    };

    LatencyMetrics measureLatency(EngineBase* engine, double sampleRate) {
        // Send impulse, detect output delay
        // Compare to engine->getLatencySamples()
    };
};
```

### 8.4 Performance Report

```
═══════════════════════════════════════════════════════════════
 PERFORMANCE BENCHMARKS
═══════════════════════════════════════════════════════════════

Engine: ClassicCompressor (ID: 2)
Sample Rate: 48000 Hz, Block Size: 512 samples

CPU Usage:
  Average: 0.8%
  Peak:    1.2%
  Min:     0.6%
  Real-time safe: YES (Processing time: 85 μs < 10667 μs deadline)

Memory:
  Initialization:  48 KB
  Preparation:     128 KB (buffers, history)
  During process:  0 bytes ✓
  Total footprint: 176 KB
  Real-time safe:  YES (no allocations during process)

Latency:
  Reported:  256 samples (5.33 ms)
  Measured:  256 samples (5.33 ms)
  Error:     0 samples
  Accurate:  YES ✓

Performance Score: 95/100
  - CPU efficiency:     A+ (< 1% average)
  - Memory efficiency:  A  (< 1 MB footprint)
  - Latency accuracy:   A+ (exact match)
  - Real-time safety:   A+ (no violations)

═══════════════════════════════════════════════════════════════
```

---

## 9. Success Criteria

### 9.1 Per-Engine Pass/Fail Criteria

```cpp
struct ValidationThresholds {
    // DYNAMICS
    struct Dynamics {
        float thresholdAccuracy_dB = 1.0f;      // ±1 dB
        float ratioAccuracy_percent = 10.0f;    // ±10%
        float timingAccuracy_percent = 20.0f;   // ±20%
    };

    // FILTERS & EQ
    struct Filters {
        float cutoffAccuracy_percent = 5.0f;    // ±5%
        float gainAccuracy_dB = 0.5f;           // ±0.5 dB
        float slopeAccuracy_dB = 3.0f;          // ±3 dB/octave
    };

    // DISTORTION
    struct Distortion {
        float minTHD_percent = 0.5f;            // Must add > 0.5% THD
        float maxTHD_percent = 50.0f;           // But < 50%
        bool mustBeBounded = true;              // No overflow
    };

    // MODULATION
    struct Modulation {
        float lfoRateAccuracy_percent = 5.0f;   // ±5%
        float depthAccuracy_percent = 10.0f;    // ±10%
        float minStereoWidth = 0.3f;            // Measurable width
    };

    // REVERB & DELAY
    struct ReverbDelay {
        float minReverbTail_seconds = 0.1f;     // At least 100ms
        float delayTimeAccuracy_ms = 1.0f;      // ±1 ms
        bool mustBeStable = true;               // No runaway
    };

    // PITCH
    struct Pitch {
        float pitchAccuracy_cents = 10.0f;      // ±10 cents
        float maxLatency_ms = 50.0f;            // < 50ms for real-time
    };

    // SPATIAL
    struct Spatial {
        float minWidthChange = 0.2f;            // Measurable effect
        bool monoCompatible = true;             // No phase cancellation
    };

    // UTILITY
    struct Utility {
        float gainAccuracy_dB = 0.1f;           // ±0.1 dB
        bool bitTransparent = true;             // At defaults
    };

    // UNIVERSAL
    struct Universal {
        bool noNaN = true;                      // Never output NaN/Inf
        bool noClicks = true;                   // Smooth parameter changes
        bool stableLongTerm = true;             // 1000 iteration test
        int maxLatency_samples = 8192;          // Must report if > 0
    };
};
```

### 9.2 Confidence Scoring

```cpp
float calculateConfidence(const TestResult& result) {
    float confidence = 1.0f;

    // Reduce confidence based on measurement errors
    for (auto& [metric, value] : result.metrics) {
        float error = abs(value - expected);
        float tolerance = getToleranceFor(metric);
        float errorRatio = error / tolerance;

        if (errorRatio > 1.0f) {
            confidence *= (1.0f / errorRatio);  // Beyond tolerance
        } else {
            confidence *= (1.0f - errorRatio * 0.2f);  // Within tolerance
        }
    }

    // Warnings reduce confidence
    confidence *= (1.0f - result.warnings.size() * 0.05f);

    return juce::jlimit(0.0f, 1.0f, confidence);
}
```

### 9.3 Overall Suite Success

Test suite is considered **SUCCESSFUL** if:

1. **≥90% of engines pass** (51/57 or better)
2. **All critical engines pass**:
   - All Dynamics (essential for mixing)
   - All Filters/EQ (essential for tone shaping)
   - At least 80% of other categories
3. **No stability failures** (crashes, hangs, NaN/Inf outputs)
4. **Performance acceptable**:
   - All engines < 5% CPU @ 48kHz
   - All engines real-time safe (no allocations)
   - Latency reported accurately

Test suite is **ACCEPTABLE** if:
1. **≥80% of engines pass** (46/57 or better)
2. **All Dynamics and Filters pass**
3. **No stability failures**
4. **Known issues documented** with action plans

Test suite **FAILS** if:
1. **<80% pass rate**
2. **Any stability failures** (crashes, NaN outputs)
3. **Critical engines fail** (Dynamics, Filters)
4. **Performance unacceptable** (>10% CPU, allocations in process)

---

## 10. Implementation Roadmap

### 10.1 Phase 1: Foundation (Week 1)

**Goal**: Core test infrastructure operational

**Tasks**:
1. ✅ Create directory structure
2. ✅ Set up CMake build system
3. ✅ Implement base classes:
   - `TestCase`
   - `EngineTestCase`
   - `TestRegistry`
   - `TestRunner`
4. ✅ Implement signal generation:
   - Sine, noise, impulse, sweep
5. ✅ Implement basic analysis:
   - RMS, Peak, FFT
6. ✅ Create simple console reporter
7. ✅ Test with 3 sample engines (Compressor, Filter, Distortion)

**Deliverables**:
- Compiling test harness
- 3 engines tested
- Console output working
- JSON export working

### 10.2 Phase 2: Category Tests (Week 2-3)

**Goal**: All 7 categories have test suites

**Tasks**:
1. ✅ Implement `DynamicsTestSuite` (6 engines)
2. ✅ Implement `FilterTestSuite` (8 engines)
3. ✅ Implement `DistortionTestSuite` (8 engines)
4. ✅ Implement `ModulationTestSuite` (11 engines)
5. ✅ Implement `ReverbDelayTestSuite` (10 engines)
6. ✅ Implement `PitchTestSuite` (3 engines)
7. ✅ Implement `SpatialTestSuite` (9 engines)
8. ✅ Implement `UtilityTestSuite` (4 engines)

**Deliverables**:
- All 57 engines testable
- Category-specific test logic implemented
- First full test run complete

### 10.3 Phase 3: Analysis & Measurement (Week 3-4)

**Goal**: Comprehensive analysis tools

**Tasks**:
1. ✅ Implement `FrequencyAnalyzer`:
   - FFT with windowing
   - Frequency response measurement
   - THD/THD+N calculation
   - Spectral analysis
2. ✅ Implement `TimeAnalyzer`:
   - Envelope extraction
   - Transient detection
   - Echo/delay detection
   - RT60 measurement
3. ✅ Implement `PitchAnalyzer`:
   - YIN algorithm
   - Autocorrelation
   - Chord detection
4. ✅ Implement `ModulationAnalyzer`:
   - LFO rate detection
   - Depth measurement
5. ✅ Implement `DynamicsAnalyzer`:
   - Compression curve
   - Attack/Release timing
   - Gain reduction tracking

**Deliverables**:
- All analysis tools operational
- Accurate measurements for all categories
- Validation against known references

### 10.4 Phase 4: Reporting & Visualization (Week 4-5)

**Goal**: Professional reports and artifacts

**Tasks**:
1. ✅ Implement `SpectrogramRenderer`:
   - PNG spectrogram generation
   - Comparison plots
   - Custom color maps
2. ✅ Implement `HTMLDashboard`:
   - Interactive results
   - Embedded audio players
   - Expandable engine cards
   - Performance charts
3. ✅ Implement `JSONExporter`:
   - Structured test results
   - Metrics database
   - Regression baseline
4. ✅ Generate WAV artifacts:
   - Input/output pairs
   - Intermediate processing stages

**Deliverables**:
- Beautiful HTML dashboard
- Complete JSON results
- Audio artifacts for all engines
- Spectrograms for visual analysis

### 10.5 Phase 5: Performance Benchmarking (Week 5)

**Goal**: Comprehensive performance analysis

**Tasks**:
1. ✅ Implement `CPUProfiler`:
   - High-resolution timing
   - Real-time performance check
   - Efficiency metrics
2. ✅ Implement `MemoryTracker`:
   - Allocation detection
   - Leak detection
   - Footprint measurement
3. ✅ Implement `LatencyMeasurement`:
   - Impulse-response latency
   - Reported vs. actual validation
4. ✅ Integrate performance into main test run

**Deliverables**:
- CPU usage for all 57 engines
- Memory footprint database
- Latency validation
- Performance regression baseline

### 10.6 Phase 6: Musical Content & Advanced Tests (Week 6)

**Goal**: Real-world validation with musical material

**Tasks**:
1. ✅ Create musical content library:
   - Record/source drum loops
   - Record/source vocal phrases
   - Record/source instrument samples
2. ✅ Implement musical content tests:
   - TransientShaper on drums
   - VocalFormant on vocals
   - Harmonizer on chords
   - Reverbs on music
3. ✅ Implement stress tests:
   - 1000-iteration stability
   - Rapid parameter automation
   - Block size variations
   - Sample rate changes

**Deliverables**:
- Musical content library (WAV files)
- Advanced test scenarios
- Stress test results
- Real-world validation

### 10.7 Phase 7: Automation & CI/CD (Week 7)

**Goal**: Automated regression testing

**Tasks**:
1. ✅ Create baseline database:
   - Save "golden" test results
   - Store reference audio
2. ✅ Implement regression detection:
   - Compare new results to baseline
   - Flag significant changes
3. ✅ Create automated scripts:
   - `run_all_tests.sh`
   - `compare_to_baseline.sh`
   - `generate_reports.sh`
4. ✅ Integrate with CI/CD (if applicable):
   - GitHub Actions workflow
   - Automated test on commit

**Deliverables**:
- Regression test framework
- Baseline database
- Automation scripts
- CI/CD integration (optional)

### 10.8 Phase 8: Documentation & Handoff (Week 8)

**Goal**: Complete, maintainable test suite

**Tasks**:
1. ✅ Write comprehensive README
2. ✅ Document all test methodologies
3. ✅ Create troubleshooting guide
4. ✅ Write examples for adding new tests
5. ✅ Final validation run
6. ✅ Archive results

**Deliverables**:
- Complete documentation
- Final test report
- Baseline for future development
- Handoff to team

---

## 11. Tools & Dependencies

### 11.1 Required Libraries

```
JUCE 7.x (or 6.x)
├── juce_core              # Core utilities, String, File I/O
├── juce_audio_basics      # AudioBuffer, MIDI
├── juce_audio_devices     # Audio I/O (for real-time tests)
├── juce_audio_formats     # WAV reading/writing
├── juce_audio_processors  # Plugin hosting (if needed)
└── juce_dsp               # FFT, windowing, filters

Standard Library (C++17)
├── <chrono>               # High-resolution timing
├── <filesystem>           # File operations
├── <thread>               # Parallel test execution
└── <random>               # Test signal generation

Optional (for advanced features)
├── libpng                 # Spectrogram rendering
├── nlohmann/json          # JSON parsing (header-only)
└── matplotlib-cpp         # Python plotting from C++
```

### 11.2 Build Requirements

```
macOS:
  - Xcode 14+ (or Command Line Tools)
  - CMake 3.15+
  - C++17 compiler (clang)

Hardware:
  - Intel or Apple Silicon Mac
  - 8GB+ RAM (for parallel testing)
  - 1GB+ free disk (for artifacts)
```

### 11.3 Test Signal Sources

**Synthesized** (generated on-the-fly):
- Sine waves
- Noise (white, pink, brown)
- Impulses
- Frequency sweeps

**Pre-recorded** (stored in `Resources/MusicalContent/`):
```
drums_60bpm.wav          # 4-bar drum loop, 60 BPM
drums_120bpm.wav         # 4-bar drum loop, 120 BPM
vocals_male_A.wav        # Sustained vowel "A", male voice
vocals_female_E.wav      # Sustained vowel "E", female voice
vocals_phrase.wav        # "Testing one two three"
guitar_clean.wav         # DI guitar, clean, C major chord
guitar_riff.wav          # Single-note riff
piano_chords.wav         # Piano, C-Am-F-G progression
bass_line.wav            # Bass line
full_mix.wav             # Balanced stereo mix, -6dBFS peak
```

Total size: ~50 MB (high-quality WAV, 48kHz/24-bit)

### 11.4 External Tools (Optional)

For advanced visualization and analysis:

```bash
# Install dependencies (macOS)
brew install libpng
brew install gnuplot
brew install ffmpeg  # For audio format conversion

# Python (for advanced plotting)
pip install matplotlib numpy scipy
```

---

## 12. Conclusion

This Master Testing Strategy provides a complete roadmap for validating all 57 ChimeraPhoenix DSP engines. The approach is:

✅ **Comprehensive**: Every engine tested with category-specific methodologies
✅ **Rigorous**: Real signals, real measurements, real data
✅ **Professional**: Studio-grade validation criteria
✅ **Automated**: Full regression testing capability
✅ **Documented**: Clear reports, visual artifacts, metrics
✅ **Maintainable**: Extensible framework for future engines

**Next Steps**:
1. Review and approve this strategy
2. Begin Phase 1 implementation (Foundation)
3. Iterate through phases 2-8
4. Deliver production-ready test suite

**Timeline**: 8 weeks from approval to completion

**Team**: 1 developer + periodic reviews

**Outcome**: Verified, production-quality DSP engine suite with comprehensive test coverage and automated regression detection.

---

*End of Master Testing Strategy Document*
