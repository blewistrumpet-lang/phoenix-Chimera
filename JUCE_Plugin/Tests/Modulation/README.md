# Modulation Effects Test Suite

## Overview

This comprehensive test suite validates the functionality, accuracy, and performance of all modulation engines in Project Chimera. Each test file provides thorough coverage of engine-specific characteristics and common modulation parameters.

## Engines Tested

### 1. ENGINE_DIGITAL_CHORUS (StereoChorus)
**File:** `DigitalChorus_Test.cpp`

**Key Tests:**
- LFO rate accuracy and waveform shape analysis
- Depth/intensity modulation precision
- Delay time modulation accuracy
- Stereo imaging and width control
- Feedback loop stability
- Mix parameter behavior
- Performance and real-time capability

**Specific Measurements:**
- Chorus delay time modulation accuracy (±20% tolerance)
- LFO rate tracking (0.5Hz - 10Hz range)
- Stereo correlation analysis
- Modulation depth measurement

### 2. ENGINE_RESONANT_CHORUS (ResonantChorus)
**File:** `ResonantChorus_Test.cpp`

**Key Tests:**
- Resonance frequency tracking accuracy
- Filter Q factor measurement
- Comb filter characteristics analysis
- LFO modulation interaction with resonance
- Stereo width and correlation
- Feedback stability with resonance

**Specific Measurements:**
- Resonant peak detection and tracking
- Q factor measurement at -3dB points
- Comb filter spacing analysis
- Spectral centroid measurement for coloration

### 3. ENGINE_ANALOG_PHASER (AnalogPhaser)
**File:** `AnalogPhaser_Test.cpp`

**Key Tests:**
- Notch frequency tracking accuracy
- All-pass stage configuration (2/4/6/8 stages)
- Feedback stability and coloration
- Stereo spread and phase relationships
- LFO rate accuracy
- Mix parameter behavior

**Specific Measurements:**
- Notch depth and frequency detection
- Phase shift measurement at specific frequencies
- Stage count estimation from phase response
- Stereo phase difference analysis

### 4. ENGINE_RING_MODULATOR (AnalogRingModulator)
**File:** `RingModulator_Test.cpp`

**Key Tests:**
- Carrier frequency precision and stability
- Sideband generation and analysis
- Harmonic distortion measurement
- DC offset handling
- Carrier suppression effectiveness
- Stereo imaging and width control

**Specific Measurements:**
- Sideband frequency accuracy (±5% tolerance)
- Total Harmonic Distortion (THD) analysis
- Carrier suppression ratio (dB)
- Ring vs. amplitude modulation modes

### 5. ENGINE_FREQUENCY_SHIFTER (FrequencyShifter)
**File:** `FrequencyShifter_Test.cpp`

**Key Tests:**
- Frequency shift accuracy and linearity
- Harmonic preservation vs. pitch shifting
- Aliasing control and artifact analysis
- Complex signal handling
- Performance optimization

**Specific Measurements:**
- Frequency shift precision (±30Hz tolerance)
- Aliasing suppression (-20dB minimum)
- Spectral integrity preservation

### 6. ENGINE_HARMONIC_TREMOLO (HarmonicTremolo)
**File:** `HarmonicTremolo_Test.cpp`

**Key Tests:**
- Harmonic emphasis and filtering
- Crossover frequency behavior
- LFO rate accuracy
- Depth/intensity modulation
- Stereo phase relationships

**Specific Measurements:**
- Crossover frequency tracking
- Harmonic content analysis
- Modulation depth calculation
- Stereo correlation measurement

### 7. ENGINE_CLASSIC_TREMOLO (ClassicTremolo)
**File:** `ClassicTremolo_Test.cpp`

**Key Tests:**
- LFO rate accuracy and tempo sync
- Waveform shape accuracy (sine, triangle, square)
- Depth/intensity modulation precision
- Stereo phase relationships
- Mix parameter behavior

**Specific Measurements:**
- Modulation depth measurement
- LFO waveform analysis
- Amplitude modulation precision

### 8. ENGINE_ROTARY_SPEAKER (RotarySpeaker)
**File:** `RotarySpeaker_Test.cpp`

**Key Tests:**
- Horn and rotor speed accuracy
- Doppler effect simulation
- Amplitude modulation precision
- Crossover frequency behavior
- Stereo imaging and spatial effects

**Specific Measurements:**
- Doppler effect quantification
- Stereo width measurement
- Crossover frequency response
- Spatial movement simulation

### 9. ENGINE_PITCH_SHIFTER (PitchShifter)
**File:** `PitchShifter_Test.cpp`

**Key Tests:**
- Pitch tracking accuracy across frequency range
- Formant preservation quality
- Time-stretch artifact analysis
- Harmonic content preservation
- Transient handling

**Specific Measurements:**
- Pitch shift ratio accuracy (±20% tolerance)
- Formant preservation correlation
- Harmonic content analysis
- Transient preservation measurement

### 10. ENGINE_DETUNE_DOUBLER (DetuneDoubler)
**File:** `DetuneDoubler_Test.cpp`

**Key Tests:**
- Detune amount accuracy and precision
- Voice spread and stereo imaging
- Chorus effect generation
- Complex signal handling
- Performance optimization

**Specific Measurements:**
- Detune effect quantification
- Stereo width analysis
- Chorus modulation depth
- Multi-voice signal processing

### 11. ENGINE_INTELLIGENT_HARMONIZER (IntelligentHarmonizer)
**File:** `IntelligentHarmonizer_Test.cpp`

**Key Tests:**
- Pitch tracking accuracy and stability
- Harmony generation and voice leading
- Scale/key tracking and adherence
- Voice count and arrangement
- Formant preservation quality

**Specific Measurements:**
- Pitch stability analysis
- Voice count estimation
- Harmonic content measurement
- Scale adherence validation

## Test Architecture

### Common Test Framework

Each test file follows a consistent architecture:

```cpp
class [Engine]Analyzer {
    // Specialized analysis functions for the engine type
    static float calculateRMS_dB(const std::vector<float>& signal);
    static bool hasInvalidValues(const std::vector<float>& signal);
    // Engine-specific measurement functions
};

class TestSignalGenerator {
    // Signal generation for testing specific characteristics
    static std::vector<std::vector<float>> generateStereoSineWave(...);
    // Specialized test signals for the engine type
};

class [Engine]Test {
    // Test execution framework with logging and validation
    void runAllTests();
    // Individual test methods
};
```

### Test Signal Types

- **Sine Waves:** Precise frequency testing and tracking
- **Complex Waveforms:** Harmonic content analysis
- **Noise Signals:** Broadband response testing
- **Impulse Responses:** Transient analysis
- **Swept Frequencies:** Frequency response measurement
- **Multi-tone Signals:** Intermodulation testing

### Measurement Techniques

- **FFT Analysis:** Spectral content and frequency response
- **Correlation Analysis:** Stereo relationships and modulation
- **Envelope Detection:** Amplitude modulation measurement
- **Pitch Detection:** Autocorrelation-based fundamental tracking
- **Harmonic Analysis:** Spectral peak detection and THD
- **Phase Analysis:** Stereo phase relationships

## Usage

### Compiling Individual Tests

```bash
# Example for Digital Chorus
g++ -std=c++17 -I../../Source DigitalChorus_Test.cpp -o DigitalChorus_Test

# Link with JUCE libraries as needed
g++ -std=c++17 -I../../Source -I../../JuceLibraryCode DigitalChorus_Test.cpp -ljuce_audio_basics -ljuce_audio_processors -o DigitalChorus_Test
```

### Running Tests

```bash
# Run individual test
./DigitalChorus_Test

# Results are saved to:
# - [Engine]_TestResults.txt (detailed log)
# - [Engine]_Data.csv (measurement data)
```

### Batch Testing

Create a script to run all modulation tests:

```bash
#!/bin/bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/

for test in DigitalChorus ResonantChorus AnalogPhaser RingModulator FrequencyShifter HarmonicTremolo ClassicTremolo RotarySpeaker PitchShifter DetuneDoubler IntelligentHarmonizer; do
    echo "Running ${test} test..."
    ./${test}_Test
    echo "Completed ${test} test."
done
```

## Test Standards and Tolerances

### Performance Requirements
- **Real-time Ratio:** < 0.5 (50% CPU usage maximum)
- **Processing Latency:** < 10ms at 44.1kHz
- **Memory Usage:** No dynamic allocation in process loop

### Accuracy Tolerances
- **Frequency Tracking:** ±5% for fundamental frequencies
- **LFO Rates:** ±10% for modulation frequencies
- **Amplitude Modulation:** ±3dB for depth settings
- **Phase Relationships:** ±15° for stereo effects
- **Harmonic Distortion:** THD < 1% at moderate settings

### Quality Metrics
- **Signal Validity:** No NaN, infinite, or out-of-range values
- **Output Levels:** -60dB to +6dB range for test signals
- **Frequency Response:** Flat within ±3dB unless intentionally shaped
- **Noise Floor:** Below -80dB for digital processing

## Integration Notes

### JUCE Dependencies
Tests require JUCE AudioBuffer and related audio processing classes. Ensure proper linking with:
- juce_audio_basics
- juce_audio_processors
- juce_core

### Engine Compatibility
Each test assumes the engine implements the EngineBase interface with:
- `prepareToPlay(double sampleRate, int samplesPerBlock)`
- `process(juce::AudioBuffer<float>& buffer)`
- `updateParameters(const std::map<int, float>& params)`
- `getNumParameters()`
- `getParameterName(int index)`

### Output Analysis
Test results include both pass/fail status and quantitative measurements for:
- Performance profiling
- Quality assurance validation
- Regression testing
- Algorithm optimization verification

This comprehensive test suite ensures all modulation effects meet the quality and performance standards required for professional audio production.