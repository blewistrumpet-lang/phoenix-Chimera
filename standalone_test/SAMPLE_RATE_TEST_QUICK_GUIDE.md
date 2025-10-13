# Sample Rate Independence Test - Quick Start Guide

## What This Test Does

Tests all Chimera Phoenix engines at multiple sample rates (44.1k, 48k, 88.2k, 96k) to verify:
- No crashes
- Correct frequency scaling
- Similar sonic character
- Reasonable CPU usage

## Quick Start

### Build the Test

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Option 1: Use the dedicated build script
./build_sample_rate_test.sh

# Option 2: Build all engines first, then test
./build_all.sh
# Then manually link the sample rate test
```

### Run the Test

```bash
cd build
./test_sample_rate_independence
```

### View Results

```bash
cat sample_rate_compatibility_report.txt
```

## What Gets Tested

24 key engines across all major categories:

### Dynamics (3)
- Opto Compressor
- VCA Compressor
- Mastering Limiter

### Filters (3)
- Parametric EQ
- Ladder Filter
- State Variable Filter

### Distortion (3)
- Tube Preamp
- Muff Fuzz
- K-Style Overdrive

### Modulation (4)
- Chorus
- Phaser
- Tremolo
- Rotary Speaker

### Delay (3)
- Tape Echo
- Digital Delay
- Bucket Brigade

### Reverb (3)
- Plate Reverb
- Spring Reverb
- Shimmer Reverb

### Spatial (2)
- Stereo Widener
- Dimension Expander

### Special (3)
- Spectral Freeze
- Phased Vocoder
- Granular Cloud

## Reading The Report

### Status Indicators

**PASSED**:
- Engine works at all sample rates
- Output is consistent
- Frequency scaling is correct

**PASSED (with warnings)**:
- Engine works but shows minor inconsistencies
- Output levels vary 10-15% across rates
- Performance scaling slightly suboptimal

**FAILED**:
- Engine crashes at one or more rates
- Produces invalid output (NaN/Inf)
- Incorrect frequency scaling

### Key Metrics

- **Peak**: Maximum absolute sample value
- **RMS**: Root mean square level
- **THD**: Total harmonic distortion (%)
- **Time**: Processing time in milliseconds

### Example Output

```
Engine 01: Vintage Opto Compressor
  Status: PASSED

  Sample Rate | Init | Process | Peak    | RMS     | THD    | Time (ms)
  ------------|------|---------|---------|---------|--------|----------
     44.1k    |  OK  |   OK    | 0.4521  | 0.3201  |  0.8%  |    8.42
     48.0k    |  OK  |   OK    | 0.4518  | 0.3199  |  0.8%  |    8.89
     88.2k    |  OK  |   OK    | 0.4523  | 0.3203  |  0.8%  |   16.78
     96.0k    |  OK  |   OK    | 0.4520  | 0.3200  |  0.8%  |   17.93
```

## Troubleshooting

### Build Fails

**Problem**: Missing object files
**Solution**: Run `./build_all.sh` first to compile all engines

**Problem**: Link errors about missing symbols
**Solution**: Make sure harfbuzz is installed: `brew install harfbuzz`

**Problem**: Can't find JUCE modules
**Solution**: Check JUCE_DIR path in build script

### Test Crashes

**Problem**: Segfault during test
**Solution**: This indicates a real bug - note which engine/sample rate and investigate

**Problem**: NaN/Inf in output
**Solution**: This indicates a division by zero or uninitialized variable

### Inconsistent Results

**Problem**: Different output levels at different rates
**Solution**: Check if time constants or filters are properly scaled

## Technical Notes

### Sample Rate Parameter Flow

```
Plugin Host
    ↓
  prepareToPlay(sampleRate, blockSize)
    ↓
  Engine stores sampleRate
    ↓
  Engine recalculates all coefficients
    ↓
  process() uses stored sampleRate
```

### Common Scaling Patterns

**Frequency**:
```cpp
float normalizedFreq = hz / sampleRate;
```

**Time**:
```cpp
int samples = seconds * sampleRate;
```

**Coefficients**:
```cpp
float coeff = exp(-1.0 / (timeConstant * sampleRate));
```

## Files in This Test Suite

- `test_sample_rate_independence.cpp` - Main test program
- `build_sample_rate_test.sh` - Build script
- `SAMPLE_RATE_INDEPENDENCE_REPORT.md` - Full documentation
- `SAMPLE_RATE_TEST_QUICK_GUIDE.md` - This file

## Expected Results

### All Tests Pass

Congratulations! The engines are properly sample-rate independent.

### Some Tests Fail

1. Note which engines failed
2. Note which sample rates caused failures
3. Check the error messages in the report
4. Review those engines for hardcoded sample rate assumptions

### Common Issues Found

1. **Hardcoded 48000**: Change to use actual sample rate
2. **Fixed buffer sizes**: Allocate based on `maxTime * maxSampleRate`
3. **Frame counting**: Use phase accumulation instead
4. **Unscaled coefficients**: Multiply by sample rate factor

## Next Steps

After running this test:

1. **All Pass**: Great! Consider testing at extreme rates (8k, 192k)
2. **Some Warnings**: Investigate minor inconsistencies
3. **Some Failures**: Fix the failing engines and retest
4. **Need More Info**: Check the full report for detailed analysis

## Support

For more details, see:
- Full report: `SAMPLE_RATE_INDEPENDENCE_REPORT.md`
- Source code: `test_sample_rate_independence.cpp`
- Build script: `build_sample_rate_test.sh`

---

Quick Reference | v1.0 | Chimera Phoenix v3.0
