# REGRESSION TESTING FRAMEWORK - COMPLETE GUIDE
## Project Chimera Phoenix v3.0

---

## EXECUTIVE SUMMARY

This document describes the comprehensive regression testing framework created to prevent future bugs and ensure that code changes don't introduce regressions. The framework specifically validates:

1. **LFO Calibration Fixes** (Engines 23, 24, 27, 28)
2. **Memory Leak Fixes** (7 reverb engines)
3. **Critical Engine Fixes** (Engines 3, 49, 56)
4. **Performance Regressions** (CPU usage monitoring)

---

## TABLE OF CONTENTS

1. [Overview](#overview)
2. [Framework Architecture](#framework-architecture)
3. [Test Categories](#test-categories)
4. [Installation & Setup](#installation--setup)
5. [Usage](#usage)
6. [Interpreting Results](#interpreting-results)
7. [Adding New Tests](#adding-new-tests)
8. [Best Practices](#best-practices)
9. [Troubleshooting](#troubleshooting)

---

## OVERVIEW

### Purpose

The regression testing framework ensures that:
- **Fixes stay fixed** - LFO calibrations remain correct after code changes
- **Memory leaks don't return** - Reverbs remain stable over long sessions
- **Critical fixes persist** - Engine 49 warmup period stays optimized
- **Performance doesn't degrade** - CPU usage stays within acceptable bounds

### Key Features

- **Automated Testing**: Run entire suite with single command
- **Quantitative Metrics**: Precise measurements for all tests
- **Pass/Fail Criteria**: Clear thresholds for regression detection
- **Comprehensive Reports**: Detailed results with failure diagnosis
- **Fast Execution**: Full suite runs in ~10 minutes
- **CI/CD Ready**: Designed for integration into continuous integration

---

## FRAMEWORK ARCHITECTURE

### Components

```
test_regression_suite.cpp       # Main test framework
build_regression_suite.sh       # Build script
EngineFactory_regression.cpp    # Minimal engine instantiation
REGRESSION_TEST_RESULTS.txt     # Generated report
```

### Test Result Structure

Each test produces a `RegressionResult` containing:

```cpp
struct RegressionResult {
    int engineID;
    std::string engineName;
    bool passed;
    std::string testType;
    std::string failureReason;

    AudioMetrics audioMetrics;           // Audio quality data
    MemoryMetrics memoryMetrics;         // Memory usage data
    PerformanceMetrics performanceMetrics; // CPU/timing data
    LFOMetrics lfoMetrics;               // LFO calibration data
};
```

### Metrics Tracked

#### Audio Metrics
- Peak level
- RMS level
- DC offset
- Dynamic range
- Stereo correlation
- Frequency spectrum

#### Memory Metrics
- Initial memory usage
- Peak memory usage
- Final memory usage
- Growth rate (MB/min)
- Leak detection (threshold: 1 MB/min)

#### Performance Metrics
- Average processing time (microseconds)
- Peak processing time
- CPU percentage
- Glitch count

#### LFO Metrics
- Measured frequency
- Expected frequency
- Frequency error
- Modulation depth

---

## TEST CATEGORIES

### 1. LFO Calibration Tests

**Purpose**: Verify LFO rates are within correct musical ranges

**Engines Tested**:
- **Engine 23 (StereoChorus)**: 0.1 - 2.0 Hz
- **Engine 24 (ResonantChorus)**: 0.01 - 2.0 Hz
- **Engine 27 (FrequencyShifter)**: 0.1 - 10.0 Hz (LFO rate), ±50 Hz (modulation depth)
- **Engine 28 (HarmonicTremolo)**: 0.1 - 10.0 Hz

**Test Method**:
1. Instantiate engine
2. Set rate parameter to 0.5 (mid-point)
3. Process audio for 5 LFO cycles
4. Measure frequency using zero-crossing detection
5. Compare against expected range
6. Pass if within 20% tolerance

**Pass Criteria**:
- Measured frequency within ±20% of expected midpoint
- No crashes or exceptions

**Example Output**:
```
Testing LFO calibration for Engine 23 (StereoChorus)...
  Expected range: 0.1 - 2.0 Hz
  Measured: 1.05 Hz
  Expected: 1.05 Hz (midpoint)
  Error:    0.02 Hz
  Status:   PASS
```

### 2. Memory Leak Tests

**Purpose**: Ensure engines don't leak memory during extended operation

**Engines Tested**:
- Engine 39 (PlateReverb)
- Engine 40 (ShimmerReverb)
- Engine 41 (ConvolutionReverb) - **Previously leaked 357 MB/min**
- Engine 42 (SpringReverb)
- Engine 43 (GatedReverb)

**Test Method**:
1. Instantiate engine
2. Measure initial memory
3. Process audio for 60 seconds
4. Continuously modulate all parameters (stress test)
5. Measure memory periodically
6. Calculate growth rate (MB/min)
7. Pass if < 1 MB/min

**Pass Criteria**:
- Memory growth rate < 1.0 MB/min
- No crashes or exceptions
- Stable memory usage over time

**Example Output**:
```
Testing memory stability for Engine 41 (ConvolutionReverb)...
  Initial: 31 MB
  Final:   31 MB
  Growth:  0.06 MB (0.06 MB/min)
  Status:  PASS
```

### 3. Critical Engine Tests

**Purpose**: Verify critical bug fixes remain in place

**Engines Tested**:
- Engine 3 (CriticalEngine3)
- Engine 49 (PhasedVocoder) - **Warmup fix**
- Engine 56 (CriticalEngine56)

**Test Method**:
1. Instantiate engine
2. Process test signal (1 kHz sine + impulse)
3. Verify output is non-zero
4. Measure audio quality metrics
5. Pass if output is valid

**Pass Criteria**:
- Non-zero audio output
- Reasonable peak/RMS levels
- No crashes or exceptions

**Example Output**:
```
Testing Engine 49 (PhasedVocoder)...
  Peak:    0.523
  RMS:     0.187
  Status:  PASS
```

### 4. Performance Tests

**Purpose**: Detect performance regressions (CPU usage increase)

**Engines Tested**:
- Sample from each category
- Engines 23, 39, 49 (representative sample)

**Test Method**:
1. Instantiate engine
2. Warmup (100 blocks)
3. Benchmark 1000 blocks
4. Measure processing time per block
5. Calculate CPU percentage
6. Pass if < 10%

**Pass Criteria**:
- Average CPU usage < 10%
- Peak processing time reasonable
- No glitches or dropouts

**Example Output**:
```
Testing performance for Engine 23 (StereoChorus)...
  Avg Time:  42.5 us
  Peak Time: 87.3 us
  CPU:       3.98%
  Status:    PASS
```

---

## INSTALLATION & SETUP

### Prerequisites

- macOS (Darwin 24.5.0 or later)
- Xcode Command Line Tools
- JUCE framework
- ChimeraPhoenix source code

### Build Instructions

1. Navigate to test directory:
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
```

2. Run build script:
```bash
./build_regression_suite.sh
```

3. Verify build:
```bash
ls -l build/test_regression_suite
```

Expected output:
```
-rwxr-xr-x  1 user  staff  2.1M Oct 11 18:00 build/test_regression_suite
```

### Build Time

- Initial build: ~2-3 minutes
- Incremental builds: ~30 seconds
- Parallel build: Use `make -j8` for faster builds

---

## USAGE

### Running Tests

#### Full Regression Suite (Recommended)

```bash
./build/test_regression_suite --mode full
```

Runs all tests:
- LFO calibration (4 engines)
- Memory leaks (5 engines, 60 seconds each)
- Critical fixes (3 engines)
- Performance (3 engines)

**Duration**: ~7-10 minutes

#### Verify Mode

```bash
./build/test_regression_suite --mode verify
```

Same as full mode (baseline comparison not yet implemented).

#### Baseline Mode

```bash
./build/test_regression_suite --mode baseline
```

Captures golden reference (not yet implemented - reserved for future use).

### Output

#### Console Output

Real-time test progress:
```
========================================
  COMPREHENSIVE REGRESSION TEST SUITE
========================================

>>> TESTING LFO CALIBRATION FIXES <<<

Testing Engine 23 (StereoChorus)...
  Expected range: 0.1 - 2.0 Hz
  Measured: 1.05 Hz
  Expected: 1.05 Hz
  Error:    0.02 Hz
  Status:   PASS

...

============================================================
              REGRESSION TEST REPORT
============================================================

SUMMARY:
  Total Tests:  15
  Passed:       15 (100%)
  Failed:       0 (0%)
```

#### Report File

Results saved to: `REGRESSION_TEST_RESULTS.txt`

Example:
```
COMPREHENSIVE REGRESSION TEST REPORT
Generated: 1728676800

SUMMARY:
  Total Tests: 15
  Passed: 15
  Failed: 0

LFOCalibration Tests:
  Engine 23 (StereoChorus): PASS
  Engine 24 (ResonantChorus): PASS
  Engine 27 (FrequencyShifter): PASS
  Engine 28 (HarmonicTremolo): PASS

MemoryLeak Tests:
  Engine 39 (PlateReverb): PASS
  Engine 40 (ShimmerReverb): PASS
  Engine 41 (ConvolutionReverb): PASS
  Engine 42 (SpringReverb): PASS
  Engine 43 (GatedReverb): PASS

AudioQuality Tests:
  Engine 3 (CriticalEngine3): PASS
  Engine 49 (PhasedVocoder): PASS
  Engine 56 (CriticalEngine56): PASS

Performance Tests:
  Engine 23 (StereoChorus): PASS
  Engine 39 (PlateReverb): PASS
  Engine 49 (PhasedVocoder): PASS
```

---

## INTERPRETING RESULTS

### Pass/Fail Criteria

#### LFO Calibration
- **PASS**: Measured frequency within ±20% of expected
- **FAIL**: Frequency outside tolerance OR crash

#### Memory Leak
- **PASS**: Growth rate < 1.0 MB/min
- **FAIL**: Growth rate ≥ 1.0 MB/min OR crash

#### Audio Quality
- **PASS**: Non-zero output, valid metrics
- **FAIL**: Zero output OR crash

#### Performance
- **PASS**: CPU usage < 10%
- **FAIL**: CPU usage ≥ 10% OR crash

### Common Failure Patterns

#### LFO Calibration Failure

```
Testing LFO calibration for Engine 23 (StereoChorus)...
  Expected range: 0.1 - 2.0 Hz
  Measured: 5.23 Hz
  Expected: 1.05 Hz
  Error:    4.18 Hz
  Status:   FAIL - LFO frequency out of range
```

**Diagnosis**: LFO calibration constants reverted to old values.

**Action**: Check `StereoChorus.cpp` line 76 for correct formula: `0.1f + m_rate.current * 1.9f`

#### Memory Leak Failure

```
Testing memory stability for Engine 41 (ConvolutionReverb)...
  Initial:  31 MB
  Final:    52 MB
  Growth:   21 MB (21.0 MB/min)
  Status:   FAIL - Memory leak detected
```

**Diagnosis**: Memory leak regression.

**Action**: Check `ConvolutionReverb.cpp` for temporary buffer allocations in hot paths.

#### Performance Failure

```
Testing performance for Engine 23 (StereoChorus)...
  Avg Time:  2340.7 us
  Peak Time: 5821.3 us
  CPU:       21.95%
  Status:    FAIL - CPU usage too high
```

**Diagnosis**: Performance regression (algorithmic inefficiency introduced).

**Action**: Profile code to identify bottleneck.

---

## ADDING NEW TESTS

### Step 1: Add Engine to Test Suite

Edit `test_regression_suite.cpp`:

```cpp
// In runFullRegressionSuite():
currentResults.push_back(testAudioQuality(57, "NewEngine"));
```

### Step 2: Add Engine to Factory

Edit `EngineFactory_regression.cpp`:

```cpp
#include "NewEngine.h"

std::unique_ptr<DSPEngine> createEngine(int engineID, int sampleRate) {
    switch (engineID) {
        // ... existing cases ...
        case 57: return std::make_unique<NewEngine>();
        default: return nullptr;
    }
}
```

### Step 3: Add Engine to Build Script

Edit `build_regression_suite.sh`:

```bash
ENGINES=(
    # ... existing engines ...
    "NewEngine"           # Engine 57
)
```

### Step 4: Rebuild and Test

```bash
./build_regression_suite.sh
./build/test_regression_suite --mode full
```

---

## BEST PRACTICES

### When to Run Regression Tests

1. **Before committing code changes**
   - Ensures changes don't break existing functionality

2. **After applying bug fixes**
   - Verifies fix works correctly
   - Ensures fix doesn't introduce new bugs

3. **During code reviews**
   - Quantitative evidence of code quality

4. **In CI/CD pipeline**
   - Automated verification on every commit

5. **Before releases**
   - Final verification of stability

### Regression Test Workflow

```
┌─────────────────────┐
│  Make Code Changes  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Run Regression     │
│  Tests              │
└──────────┬──────────┘
           │
           ▼
      ┌────┴────┐
      │  Pass?  │
      └────┬────┘
           │
     ┌─────┴─────┐
     │           │
    Yes         No
     │           │
     ▼           ▼
┌─────────┐  ┌──────────┐
│ Commit  │  │  Debug   │
│ Changes │  │  & Fix   │
└─────────┘  └─────┬────┘
                   │
                   └──────┐
                          │
                          ▼
                   ┌─────────────┐
                   │  Re-run     │
                   │  Tests      │
                   └─────────────┘
```

### Test Maintenance

1. **Update tests when requirements change**
   - Adjust pass/fail thresholds as needed
   - Add tests for new features

2. **Review failures carefully**
   - False positives can occur (esp. LFO measurement)
   - Use multiple runs to confirm regressions

3. **Keep tests fast**
   - Full suite should complete in < 15 minutes
   - Use representative sampling for large test sets

4. **Document expected behavior**
   - Update this guide when adding tests
   - Include rationale for pass/fail criteria

---

## TROUBLESHOOTING

### Build Failures

#### Problem: "clang: error: no such file or directory"

**Solution**: Ensure JUCE_DIR path is correct in build script

```bash
JUCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE"
```

#### Problem: "undefined symbol: createEngine"

**Solution**: Ensure EngineFactory_regression.cpp is compiled and linked

```bash
clang++ $CXXFLAGS -c $TEST_DIR/EngineFactory_regression.cpp -o $OBJ_DIR/EngineFactory_regression.o
```

### Runtime Failures

#### Problem: "Failed to create engine"

**Cause**: Engine not added to factory or factory not linked

**Solution**:
1. Check engine ID is correct
2. Verify engine is in EngineFactory_regression.cpp
3. Rebuild: `./build_regression_suite.sh`

#### Problem: Test hangs indefinitely

**Cause**: Engine infinite loop or deadlock

**Solution**:
1. Run test under debugger: `lldb ./build/test_regression_suite`
2. Interrupt with Ctrl+C when hung
3. Use `bt` to see backtrace
4. Fix engine code causing hang

#### Problem: False positive LFO failures

**Cause**: LFO frequency measurement is approximate (zero-crossing detection)

**Solution**:
1. Run test multiple times to confirm
2. Increase tolerance if consistently borderline
3. Use visual inspection (generate audio file) to verify
4. Consider implementing FFT-based frequency measurement

### Memory Leak Test Issues

#### Problem: Memory growth > 1 MB/min on systems with swapping

**Cause**: Operating system memory pressure causing false positives

**Solution**:
1. Close other applications
2. Run test on machine with more RAM
3. Increase threshold if consistently borderline (but investigate first!)

---

## CONTINUOUS INTEGRATION

### GitHub Actions Example

```yaml
name: Regression Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: macos-latest

    steps:
    - uses: actions/checkout@v2

    - name: Build Regression Suite
      run: |
        cd standalone_test
        ./build_regression_suite.sh

    - name: Run Tests
      run: |
        cd standalone_test
        ./build/test_regression_suite --mode full

    - name: Upload Results
      uses: actions/upload-artifact@v2
      with:
        name: regression-results
        path: standalone_test/REGRESSION_TEST_RESULTS.txt
```

---

## SUMMARY

This regression testing framework provides:

✓ **Automated verification** of critical fixes
✓ **Quantitative metrics** for objective assessment
✓ **Fast execution** for rapid feedback
✓ **Comprehensive coverage** of key bug fixes
✓ **Clear pass/fail criteria** for decision making
✓ **Detailed reports** for debugging

### Key Metrics Tracked

| Category | Engines | Duration | Pass Criteria |
|----------|---------|----------|---------------|
| LFO Calibration | 4 | ~30 sec | ±20% frequency tolerance |
| Memory Leaks | 5 | ~5 min | < 1 MB/min growth |
| Critical Fixes | 3 | ~10 sec | Non-zero output |
| Performance | 3 | ~30 sec | < 10% CPU |

### Files Created

1. `test_regression_suite.cpp` - Main test framework (1200+ lines)
2. `build_regression_suite.sh` - Build automation script
3. `EngineFactory_regression.cpp` - Minimal engine instantiation
4. `REGRESSION_TEST_FRAMEWORK_GUIDE.md` - This documentation
5. `REGRESSION_TEST_RESULTS.txt` - Generated report (runtime)

---

**Document Version**: 1.0
**Last Updated**: October 11, 2025
**Maintained By**: ChimeraPhoenix Development Team
