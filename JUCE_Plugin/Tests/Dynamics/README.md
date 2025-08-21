# DYNAMICS Audio Engine Test Suite

This directory contains comprehensive test suites for all six DYNAMICS audio engines in the Chimera Phoenix plugin.

## Engine Coverage

### 1. ENGINE_OPTO_COMPRESSOR (ID: 1)
**File:** `VintageOptoCompressor_Test.cpp`
**Features Tested:**
- Opto cell simulation timing (attack ~10ms, program-dependent release)
- Thermal modeling and component aging
- LA-2A style compression characteristics
- Parameter sweeps for all 8 parameters
- Gain reduction curve linearity
- Emphasis filter pre/de-emphasis behavior
- Analog noise and warmth simulation

**Test Cases:**
- Parameter responsiveness validation
- Compression ratio accuracy across input levels
- Attack time measurement (5-50ms expected range)
- Frequency response consistency
- Bypass state null testing
- Stability with extreme parameter values
- Thermal drift over extended processing

### 2. ENGINE_VCA_COMPRESSOR (ID: 2)
**File:** `ClassicCompressor_Test.cpp`
**Features Tested:**
- Professional VCA compression characteristics
- 10-parameter comprehensive control
- SIMD optimization validation
- Lookahead processing (up to 512 samples)
- Sidechain filtering with TPT SVF design
- Program-dependent auto-release
- Professional metering accuracy

**Test Cases:**
- Parameter sweep validation (all 10 parameters)
- Threshold detection precision (-60dB to 0dB range)
- Attack/Release timing measurement (0.1ms to 5000ms)
- Compression ratio accuracy (1:1 to ∞:1)
- Knee characteristics (hard vs soft)
- Lookahead impulse response
- Frequency response and sidechain filtering
- Distortion and artifact analysis

### 3. ENGINE_TRANSIENT_SHAPER (ID: 3)
**File:** `TransientShaper_Test.cpp`
**Features Tested:**
- Multi-algorithm transient detection (Peak, RMS, Hilbert, Hybrid)
- Attack/Sustain separation accuracy
- Zero-latency and lookahead modes
- Professional oversampling (2x/4x)
- Soft-knee compression for natural dynamics
- Complete denormal protection

**Test Cases:**
- Parameter validation for all 10 parameters
- Attack enhancement measurement
- Sustain processing accuracy
- Attack/Sustain separation quality
- Detection algorithm comparison
- Timing accuracy verification
- Lookahead vs zero-latency performance
- Oversampling quality assessment
- Soft-knee behavior analysis

### 4. ENGINE_NOISE_GATE (ID: 4)
**File:** `NoiseGate_Test.cpp`
**Features Tested:**
- Gate state machine (CLOSED, OPENING, OPEN, HOLDING, CLOSING)
- Hysteresis behavior to prevent chatter
- Advanced envelope detection (Peak/RMS/Spectral)
- Thermal modeling and component aging
- Lookahead processing for smooth operation
- ZDF sidechain filtering

**Test Cases:**
- Parameter sweep validation (all 8 parameters)
- Threshold detection accuracy across levels
- Gate timing measurement (attack, hold, release)
- Hysteresis behavior with chattering signals
- Range attenuation testing (-40dB to 0dB)
- Sidechain filter frequency response
- Lookahead processing verification
- Gate state transition analysis
- Analog modeling stability

### 5. ENGINE_MASTERING_LIMITER (ID: 5)
**File:** `MasteringLimiter_Test.cpp`
**Features Tested:**
- Brick-wall limiting with configurable ceiling
- True-peak detection and limiting
- Professional lookahead processing
- 0dBFS compliance verification
- Inter-sample peak detection
- Professional metering (GR, Input, Output, True Peak)
- Stereo linking behavior

**Test Cases:**
- Parameter validation for all 10 parameters
- Brick-wall limiting accuracy (±0.1dB tolerance)
- True-peak vs sample-peak limiting
- 0dBFS compliance with various signals
- Lookahead processing effectiveness
- Release time precision measurement
- Threshold behavior analysis
- Knee characteristics (hard vs soft)
- Stereo linking verification
- Professional metering validation

### 6. ENGINE_DYNAMIC_EQ (ID: 6)
**File:** `DynamicEQ_Test.cpp`
**Features Tested:**
- Frequency-dependent dynamic processing
- TPT filter implementation with thermal modeling
- Multiple operation modes (Compressor/Expander/Gate)
- 2x oversampling with anti-aliasing
- Dynamic processor with lookahead
- Static EQ vs dynamic EQ behavior

**Test Cases:**
- Parameter sweep validation (all 8 parameters)
- Frequency-dependent processing accuracy
- Dynamic threshold behavior
- Operation mode comparison (Compressor/Expander/Gate)
- Filter response and stability
- Attack/Release timing measurement
- Static vs dynamic EQ comparison
- Mix parameter dry/wet blending
- Broadband vs narrowband processing
- Analog modeling and thermal effects

## Test Signal Types

### Standard Test Signals
- **Sine Waves:** Pure tones at specified frequencies and levels
- **Frequency Sweeps:** Logarithmic sweeps from 20Hz to 20kHz
- **Pink Noise:** Broadband test signal with 1/f spectrum
- **White Noise:** Flat spectrum broadband signal
- **Impulses:** Delta functions for transient testing

### Specialized Test Signals
- **Calibrated Bursts:** Precise level signals for threshold testing
- **Inter-Sample Peak Signals:** Test true-peak detection
- **Chattering Signals:** Alternating levels for hysteresis testing
- **Dynamic Content:** Varying level signals for dynamic response
- **Complex Multi-Tone:** Harmonic content for distortion testing

## Measurement Techniques

### Level Measurements
- **RMS Level:** Time-averaged power measurement in dB
- **Peak Level:** Maximum absolute sample value in dB
- **True Peak:** Inter-sample peak estimation via oversampling

### Timing Measurements
- **Attack Time:** 10% to 90% of final gain reduction
- **Release Time:** 90% to 10% decay from peak
- **Hold Time:** Duration gate remains open after input drops

### Response Analysis
- **Frequency Response:** DFT-based magnitude response at specific frequencies
- **Dynamic Response:** Level-dependent processing measurement
- **Compression Ratio:** Input vs output level relationship
- **Gain Reduction:** Amount of level reduction applied

### Quality Metrics
- **Limiting Accuracy:** Deviation from target ceiling
- **Threshold Accuracy:** Precision of threshold detection
- **Separation Quality:** Attack/sustain processing effectiveness
- **Spectral Centroid:** Frequency content brightness measure

## Test Validation Criteria

### Safety Tests
- **No NaN/Inf Values:** All output samples must be finite
- **No Clipping:** Output must not exceed 0dBFS
- **Stability:** No oscillation or runaway behavior
- **Memory Safety:** No buffer overruns or access violations

### Performance Tests
- **Parameter Responsiveness:** Audible effect when parameters change
- **Timing Accuracy:** Attack/release times within expected ranges
- **Frequency Accuracy:** Processing occurs at correct frequencies
- **Level Accuracy:** Thresholds and ceilings respected

### Quality Tests
- **Low Distortion:** THD+N below acceptable limits
- **Smooth Operation:** No artifacts or discontinuities
- **Consistent Behavior:** Repeatable results across runs
- **Professional Standards:** Broadcast and mastering compliance

## Running the Tests

### Individual Engine Tests
```bash
# Compile and run individual test
g++ -std=c++17 -I../../Source VintageOptoCompressor_Test.cpp -o opto_test
./opto_test

# Results will be saved to:
# VintageOptoCompressor_TestResults.txt
```

### All Dynamics Tests
```bash
# Run complete dynamics test suite
./run_dynamics_tests.sh

# Generates combined report:
# DynamicsTestSuite_Report.txt
```

## Test Output Format

Each test generates a detailed log file with:
- Test execution summary
- Parameter sweep results
- Measurement data with units
- Pass/fail status for each test case
- Performance benchmarks
- Error analysis and recommendations

### Example Output
```
=== Vintage Opto Compressor Test Suite ===
Sample Rate: 44100.0 Hz
Block Size: 512 samples
Engine ID: 1

--- Parameter Sweep Tests ---
Testing parameter 0: Input Gain
  [PASS] Input Gain at 0.0 produces valid output
  [PASS] Input Gain at 0.1 produces valid output
  ...
  Response range: 12.3dB

--- Compression Ratio Tests ---
Input: -30.0dB -> Output: -28.5dB
Input: -20.0dB -> Output: -16.2dB
  [PASS] Compression ratio test 1 (ratio: 3.2:1)

=== Test Summary ===
Tests Passed: 45
Tests Failed: 0
Success Rate: 100%
```

## Professional Standards Compliance

All tests verify compliance with:
- **AES17:** Digital audio measurement standards
- **ITU-R BS.1770:** Loudness measurement guidelines  
- **EBU R128:** European broadcasting standards
- **ATSC A/85:** Advanced television audio standards

## Test Maintenance

### Adding New Tests
1. Follow existing test structure and naming conventions
2. Include comprehensive parameter validation
3. Add appropriate signal generators and analyzers
4. Document expected behavior and tolerances
5. Update this README with new test descriptions

### Modifying Existing Tests
1. Maintain backward compatibility with test results
2. Update tolerance values based on engine improvements
3. Add new measurement techniques as needed
4. Keep test execution time reasonable (<30 seconds per engine)

## Dependencies

- **JUCE Framework:** Audio processing and buffer management
- **C++ Standard Library:** Mathematical functions and containers
- **Engine Headers:** Individual engine implementations
- **Test Signal Generators:** Synthetic audio signal creation
- **Audio Analyzers:** Measurement and validation utilities

## Notes

- All file paths in test results use absolute paths
- Test signals are generated procedurally for consistency
- Measurement tolerances account for floating-point precision
- Tests are designed to run on standard development hardware
- Results may vary slightly between different CPU architectures