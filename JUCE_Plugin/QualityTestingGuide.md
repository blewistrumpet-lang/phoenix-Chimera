# Chimera Phoenix Quality Testing Framework

## Overview

The Quality Testing Framework provides comprehensive automated testing for all 50 engines in the Chimera Phoenix plugin. It ensures that every engine meets boutique quality standards for audio quality, functionality, DSP performance, and analog modeling features.

## Test Categories

### 1. Audio Quality Tests
- **DC Offset**: Verifies output has minimal DC offset (<-60dB)
- **Peak Level**: Ensures no clipping with default parameters
- **THD**: Measures Total Harmonic Distortion (<1%)
- **Noise Floor**: Checks noise levels are acceptable (<-90dB)
- **Zipper Noise**: Detects parameter stepping artifacts
- **Gain Staging**: Verifies unity gain with neutral settings
- **Stereo Imaging**: Analyzes stereo field correlation

### 2. Functional Tests
- **Parameter Response**: All parameters affect the output
- **Parameter Ranges**: Values properly clamped to 0.0-1.0
- **Extreme Parameters**: Stability with min/max values
- **Stereo Handling**: Independent channel processing
- **Bypass Behavior**: Clean bypass operation
- **Memory Leaks**: No memory allocation issues
- **Thread Safety**: Safe for real-time processing
- **State Recall**: Parameters save/load correctly

### 3. DSP Quality Tests
- **Frequency Response**: Proper frequency characteristics
- **Impulse Response**: Clean transient behavior
- **Aliasing Detection**: No aliasing artifacts
- **Latency Measurement**: Processing delay <10ms
- **Filter Stability**: No self-oscillation
- **Phase Coherence**: Minimal phase distortion
- **Oversampling Quality**: Proper anti-aliasing
- **Interpolation Quality**: Smooth parameter changes

### 4. Boutique Quality Tests
- **Thermal Modeling**: Analog temperature drift active
- **Component Aging**: Gradual parameter changes over time
- **Parameter Smoothing**: No zipper noise on changes
- **DC Blocking**: Input/output DC removal
- **Analog Noise**: Realistic noise floor (-120dB to -80dB)
- **Component Tolerance**: Parameter variations
- **Vintage Character**: Analog-style behavior
- **Warmth and Color**: Harmonic enhancement

### 5. Engine-Specific Tests
- **Delays**: Timing accuracy, no clicks
- **Reverbs**: Natural decay, no metallic artifacts
- **Filters**: Resonance stability
- **Dynamics**: Smooth compression curves
- **Distortion**: Musical harmonic content

### 6. Performance Metrics
- **CPU Usage**: <10% per instance
- **Memory Usage**: Efficient allocation
- **Processing Latency**: Minimal delay
- **Efficiency Score**: Overall performance rating

## Running Tests

### Command Line Usage

```bash
# Run all tests
./QualityTestRunner

# Test specific engine
./QualityTestRunner --engine 0

# Run test suite
./QualityTestRunner --suite audio_quality
./QualityTestRunner --suite performance
./QualityTestRunner --suite boutique
```

### Integration with Build System

```bash
# Build the test runner
mkdir build && cd build
cmake ..
make QualityTestRunner

# Run tests
./QualityTestRunner
```

## Test Results

### Console Output
- Real-time test progress
- Pass/fail status for each engine
- Performance metrics
- Overall summary

### HTML Report
- Detailed test results in `test_results.html`
- Color-coded pass/fail indicators
- Failed test details
- Recommendations for improvements

### JSON Report
- Machine-readable results in `test_results.json`
- Integration with CI/CD pipelines
- Automated quality tracking

## Quality Thresholds

```cpp
struct QualityThresholds {
    float maxDCOffset = 0.001f;        // -60dB
    float maxTHD = 0.01f;              // 1% THD
    float minSNR = 60.0f;              // 60dB SNR
    float maxLatencySamples = 512.0f;  // ~10ms at 48kHz
    float maxCPUUsage = 10.0f;         // 10% CPU
    float parameterSmoothingMs = 50.0f; // 50ms smoothing
};
```

## Interpreting Results

### Pass Criteria
- Overall score ≥90%
- No critical failures
- All boutique features present
- Performance within limits

### Common Issues
- **DC Offset**: Missing or ineffective DC blocking
- **Zipper Noise**: Parameters need smoothing
- **CPU Usage**: DSP algorithms need optimization
- **No Thermal Modeling**: Boutique features not implemented

## Continuous Integration

### GitHub Actions Example
```yaml
name: Quality Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make QualityTestRunner
      - name: Run Tests
        run: ./build/QualityTestRunner
      - name: Upload Results
        uses: actions/upload-artifact@v2
        with:
          name: test-results
          path: |
            test_results.html
            test_results.json
```

## Adding New Tests

### 1. Add Test Method to EngineQualityTest
```cpp
bool testNewFeature(EngineBase* engine) {
    // Test implementation
    return passed;
}
```

### 2. Add to Test Category
```cpp
struct NewTestResults {
    TestResult newFeatureTest;
    // ...
};
```

### 3. Include in runAllTests()
```cpp
results.newCategory = testNewFeature(engine.get());
```

## Best Practices

1. **Run tests after any engine modifications**
2. **Monitor performance metrics over time**
3. **Use HTML reports for detailed analysis**
4. **Integrate with CI/CD for automatic testing**
5. **Update thresholds as quality improves**

## Troubleshooting

### Test Runner Crashes
- Check JUCE framework is properly linked
- Verify all engine files are included in build
- Check for null pointer exceptions

### False Failures
- Adjust quality thresholds if too strict
- Check test signal generation
- Verify sample rate settings

### Performance Issues
- Run tests in Release mode
- Test on appropriate hardware
- Check for debug assertions

## Future Enhancements

1. **A/B Testing**: Compare against reference implementations
2. **Listening Tests**: Automated perceptual quality metrics
3. **Regression Testing**: Track quality over versions
4. **Benchmark Suite**: Industry-standard test signals
5. **Cloud Testing**: Distributed testing across platforms