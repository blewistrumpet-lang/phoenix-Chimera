# Distortion & Saturation Engine Test Suite

This directory contains comprehensive test suites for all DISTORTION & SATURATION audio engines in Project Chimera.

## Engines Tested

1. **ENGINE_VINTAGE_TUBE** (ID: 15) - Vintage Tube Preamp
2. **ENGINE_WAVE_FOLDER** (ID: 16) - Wave Folder
3. **ENGINE_HARMONIC_EXCITER** (ID: 17) - Harmonic Exciter
4. **ENGINE_BIT_CRUSHER** (ID: 18) - Bit Crusher
5. **ENGINE_MULTIBAND_SATURATOR** (ID: 19) - Multiband Saturator
6. **ENGINE_MUFF_FUZZ** (ID: 20) - Muff Fuzz
7. **ENGINE_RODENT_DISTORTION** (ID: 21) - Rodent Distortion
8. **ENGINE_K_STYLE** (ID: 22) - K-Style Overdrive

## Test Categories

Each engine test includes:

### 1. Harmonic Content Analysis
- THD (Total Harmonic Distortion) measurements
- Harmonic spectrum analysis using FFT
- Even/odd harmonic balance verification
- Harmonic generation vs drive level

### 2. Clipping/Saturation Behavior
- Input/output transfer function analysis
- Soft vs hard clipping characteristics
- Saturation curve accuracy
- Dynamic range testing

### 3. Output Level Compensation
- Gain compensation accuracy
- RMS level consistency
- Peak limiting behavior
- Headroom management

### 4. Tone Shaping Controls
- EQ response verification
- Filter characteristic analysis
- Tone stack modeling accuracy
- Frequency response measurements

### 5. Aliasing and Oversampling
- Aliasing detection tests
- Oversampling effectiveness
- Anti-aliasing filter performance
- Nyquist frequency behavior

### 6. Engine-Specific Tests

#### Vintage Tube Preamp
- Tube type modeling accuracy
- Thermal noise characteristics
- Power supply ripple effects
- Plate voltage saturation

#### Wave Folder
- Folding threshold precision
- Wave symmetry analysis
- Folding harmonics content
- Anti-aliasing effectiveness

#### Harmonic Exciter
- Harmonic enhancement accuracy
- Frequency-selective processing
- Musical vs non-musical harmonics
- Phase coherence

#### Bit Crusher
- Bit depth reduction accuracy
- Sample rate downsampling
- Quantization noise characteristics
- Dithering effectiveness

#### Multiband Saturator
- Crossover frequency accuracy
- Band isolation testing
- Independent saturation per band
- Phase alignment between bands

#### Muff Fuzz
- Sustain behavior analysis
- Gate threshold accuracy
- Compression characteristics
- Feedback control

#### Rodent Distortion
- Clipping characteristics
- Filter response analysis
- Gain structure verification
- Tonal accuracy

#### K-Style Overdrive
- Overdrive curve modeling
- Tone stack accuracy
- Drive response characteristics
- Clean/overdrive blend

## Test Utilities

### Signal Generators
- Pure sine waves for THD analysis
- Frequency sweeps for response testing
- Pink noise for statistical analysis
- Impulse responses for transient testing

### Analysis Tools
- FFT-based harmonic analysis
- Transfer function plotting
- Statistical measurements
- Aliasing detection algorithms

### Quality Metrics
- THD+N measurements
- Signal-to-noise ratio
- Dynamic range calculations
- Frequency response deviation

## Usage

### Running Individual Tests
```bash
# Compile and run specific engine test
g++ -std=c++17 -I../../Source VintageTubePreamp_Test.cpp -o tube_test
./tube_test
```

### Running All Tests
```bash
# Run complete test suite
./run_distortion_tests.sh
```

### Test Output
Each test generates:
- Console output with pass/fail results
- Detailed log files with measurements
- CSV data files for analysis
- Error reports for failed tests

## Test Standards

### Pass Criteria
- THD < 5% for musical distortion engines
- Aliasing < -60dB above Nyquist/2
- Parameter response within ±3dB
- No NaN/Inf values in output
- Consistent performance across sample rates

### Performance Requirements
- Real-time processing capability
- CPU usage < 10% per engine
- Memory allocation in prepare phase only
- Thread-safe parameter updates

## Continuous Integration

These tests are designed to be run automatically in CI/CD pipelines to ensure:
- Engine regression detection
- Performance monitoring
- Quality assurance
- Cross-platform compatibility

## Contributing

When adding new distortion engines:
1. Create comprehensive test file following existing patterns
2. Include all standard test categories
3. Add engine-specific tests for unique features
4. Update this README with new engine details
5. Add to the test runner script