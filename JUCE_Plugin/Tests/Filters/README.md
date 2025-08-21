# Filter & EQ Engine Test Suite

This directory contains comprehensive C++ test suites for all the filter and EQ engines in the Chimera Phoenix audio plugin. Each test suite is designed to validate the accuracy, stability, and performance of its respective audio engine.

## Test Files Overview

### 1. ParametricEQ_Test.cpp
**Tests for ENGINE_PARAMETRIC_EQ**
- Frequency response accuracy across all bands (low shelf, mid bell, high shelf)
- Q/bandwidth behavior validation  
- Gain control precision (±15dB range)
- Band interaction and phase coherence
- Multi-band processing with simultaneous EQ curves
- Smooth parameter transitions and automation
- THD+N measurements at various signal levels
- Impulse and step response analysis

### 2. VintageConsoleEQ_Test.cpp  
**Tests for ENGINE_VINTAGE_CONSOLE_EQ**
- Analog modeling accuracy and vintage character
- Console-style frequency curves and saturation
- Drive stages and harmonic enhancement
- Component modeling with tolerances and drift
- Thermal variation simulation
- Intermodulation distortion measurements
- Vintage vs modern mode comparisons
- Program material processing tests

### 3. LadderFilter_Test.cpp
**Tests for ENGINE_LADDER_FILTER**
- Self-oscillation threshold detection and stability
- 4-pole lowpass response accuracy (-24dB/octave)
- Zero-delay feedback topology verification
- Moog-style saturation characteristics
- Component tolerance modeling
- Professional oversampling effectiveness
- Resonance stability at extreme settings
- Vintage mode authenticity

### 4. StateVariableFilter_Test.cpp
**Tests for ENGINE_STATE_VARIABLE_FILTER**
- Multi-mode operation (LP/HP/BP/Notch/All-pass)
- Mode switching continuity without artifacts
- Zero-delay feedback SVF topology
- Cascaded multi-pole configurations (1, 2, 4-pole)
- Envelope following functionality
- Drive and analog modeling features
- Parameter smoothing effectiveness
- Real-time performance optimization

### 5. FormantFilter_Test.cpp
**Tests for ENGINE_FORMANT_FILTER**
- Vowel formant frequency accuracy (A, E, I, O, U)
- Formant bandwidth and Q factor precision
- Vowel morphing smoothness and realism
- Professional oversampling and anti-aliasing
- Speech signal processing and enhancement
- Voice modeling across different vocal tract sizes
- Harmonic content analysis and preservation

### 6. EnvelopeFilter_Test.cpp
**Tests for ENGINE_ENVELOPE_FILTER**
- Envelope follower response timing (attack/release)
- Signal tracking accuracy and dynamics
- Sensitivity and range parameter interaction
- Filter mode operation with envelope control
- Up/down direction control
- Real-time envelope extraction
- Musical instrument processing tests
- Dynamic response across signal levels

### 7. CombResonator_Test.cpp
**Tests for ENGINE_COMB_RESONATOR**
- Comb filter frequency response accuracy
- Resonant peak positioning and harmonic spacing
- Delay time precision and modulation
- Feedback stability limits
- Multi-comb configuration and interaction
- Pitch resonance and string simulation
- Interpolation quality for smooth modulation
- Musical applications (guitar, vocals)

### 8. VocalFormantFilter_Test.cpp
**Tests for ENGINE_VOCAL_FORMANT**
- Professional vowel formant modeling
- Voice characteristics (male, female, child)
- Brightness control and spectral shaping
- Thread-safe parameter updates
- Advanced modulation effects
- Speech processing optimization
- Musical instrument "vocalization"
- Performance stability under load

## Test Framework Features

### Signal Generators
Each test suite includes specialized signal generators:
- **Harmonic Series**: For testing frequency-dependent processing
- **Swept Sines**: For frequency response measurements  
- **White/Pink Noise**: For stability and resonance testing
- **Impulses**: For transient response analysis
- **Musical Signals**: For real-world application testing
- **Speech-like Signals**: For vocal processing validation

### Analysis Tools
Comprehensive analysis capabilities:
- **Frequency Response**: Magnitude and phase measurements
- **Harmonic Analysis**: THD, IMD, and spectral content
- **Envelope Following**: Attack/release timing verification
- **Formant Detection**: Advanced peak detection algorithms
- **Stability Monitoring**: NaN/Inf detection and bounds checking
- **Performance Profiling**: Real-time processing capability

### Test Categories

#### 1. Basic Functionality
- Parameter count and naming verification
- Engine initialization and reset behavior
- Basic audio processing pipeline

#### 2. Frequency Response
- Swept sine measurements across audio spectrum
- Filter characteristic verification (LP/HP/BP/Notch)
- Cutoff frequency tracking accuracy
- Rolloff rate measurements

#### 3. Stability and Limits  
- Extreme parameter combinations
- High-level signal handling
- Feedback stability thresholds
- NaN/Inf protection verification

#### 4. Musical Applications
- Real instrument processing tests
- Vocal enhancement validation
- Creative effect demonstrations
- Mix-ready audio quality

#### 5. Performance Optimization
- Real-time processing capability
- Memory usage efficiency
- Thread safety verification
- Parameter automation smoothness

## Building and Running Tests

### Prerequisites
- JUCE framework
- C++17 compatible compiler
- CMake 3.15 or higher

### Compilation
```bash
# Navigate to the Tests/Filters directory
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Filters

# Compile individual test (example for ParametricEQ)
g++ -std=c++17 -I../../Source -I../../JuceLibraryCode \
    ParametricEQ_Test.cpp ../../Source/ParametricEQ.cpp \
    -o ParametricEQ_Test

# Run the test
./ParametricEQ_Test
```

### Batch Testing
```bash
# Run all filter tests
for test in *_Test.cpp; do
    echo "Running ${test%.*}..."
    ./"${test%.*}"
done
```

## Test Results

Each test generates a detailed log file:
- `ParametricEQ_TestResults.txt`
- `VintageConsoleEQ_TestResults.txt`  
- `LadderFilter_TestResults.txt`
- `StateVariableFilter_TestResults.txt`
- `FormantFilter_TestResults.txt`
- `EnvelopeFilter_TestResults.txt`
- `CombResonator_TestResults.txt`
- `VocalFormantFilter_TestResults.txt`

### Log File Contents
- Test execution timestamp
- Parameter verification results
- Frequency response measurements
- Error tolerance analysis
- Performance benchmarks
- Pass/fail status for each test category

## Quality Assurance Standards

### Frequency Accuracy
- **Cutoff Frequency**: ±5% tolerance
- **Formant Positioning**: ±10% tolerance for F1/F2, ±15% for F3+
- **Harmonic Spacing**: ±2% tolerance for comb filters

### Amplitude Accuracy  
- **Gain Control**: ±0.1dB precision
- **THD+N**: <1% for clean modes, <5% for vintage/driven modes
- **Dynamic Range**: >90dB for modern modes, >60dB for vintage

### Timing Accuracy
- **Attack/Release**: ±20% tolerance
- **Delay Time**: ±2% precision
- **Phase Response**: Verified for linear-phase modes

### Stability Requirements
- **No NaN/Inf**: Strict zero tolerance
- **Bounded Output**: All outputs <100x input level
- **Parameter Automation**: Smooth with <0.1% discontinuity rate

## Integration with CI/CD

These tests are designed for:
- **Automated Testing**: Command-line execution with return codes
- **Regression Detection**: Consistent baseline measurements  
- **Performance Monitoring**: Benchmark tracking over time
- **Quality Gates**: Pass/fail criteria for release validation

## Extending the Test Suite

To add new test categories:

1. **Create Test Function**: Add new test method to the test suite class
2. **Add to Runner**: Include in the `runAllTests()` method
3. **Update Documentation**: Document new test coverage
4. **Set Tolerances**: Define appropriate pass/fail criteria

Example:
```cpp
void testNewFeature() {
    logFile << "\n--- New Feature Tests ---" << std::endl;
    
    // Test implementation
    std::map<int, float> params = { /* ... */ };
    filter->updateParameters(params);
    
    // Process test signal
    // Analyze results
    // Assert pass/fail criteria
    
    logFile << "✓ New feature tests passed" << std::endl;
}
```

## Support and Maintenance

- **Test Updates**: Maintain alongside engine development
- **Baseline Refresh**: Update reference values when engines improve
- **Platform Testing**: Verify across different OS/compiler combinations
- **Performance Tracking**: Monitor test execution time trends

For questions or issues with the test suite, refer to the main project documentation or contact the development team.