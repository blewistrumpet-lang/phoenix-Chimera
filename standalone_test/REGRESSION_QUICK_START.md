# REGRESSION TESTING - QUICK START GUIDE
## Project Chimera Phoenix v3.0

---

## QUICK REFERENCE CARD

### Build & Run (One-Time Setup)

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./build_regression_suite.sh
./build/test_regression_suite --mode full
```

**Duration**: ~10 minutes for full suite

### View Results

```bash
cat REGRESSION_TEST_RESULTS.txt
```

---

## WHAT GETS TESTED

| Category | Engines | Test Type | Pass Criteria |
|----------|---------|-----------|---------------|
| **LFO Calibration** | 23, 24, 27, 28 | Frequency measurement | ±20% tolerance |
| **Memory Leaks** | 39, 40, 41, 42, 43 | 60-second stress test | < 1 MB/min growth |
| **Critical Fixes** | 3, 49, 56 | Audio validation | Non-zero output |
| **Performance** | 23, 39, 49 | CPU benchmark | < 10% CPU |

---

## INTERPRETING RESULTS

### PASS - All Good!
```
Engine 23 (StereoChorus): PASS
```
No action needed. Fix is stable.

### FAIL - Regression Detected!
```
Engine 23 (StereoChorus): FAIL - LFO frequency out of range: 5.23 Hz (expected 1.05 Hz)
```

**Action**: Check source code for reverted changes.

**File to Check**: `StereoChorus.cpp` line 76
**Expected Code**: `0.1f + m_rate.current * 1.9f`

---

## COMMON FAILURES & FIXES

### LFO Calibration Failure

**Symptom**: Measured frequency doesn't match expected

**Cause**: LFO rate constants reverted to old values

**Fix**: Restore correct calibration constants

| Engine | File | Line | Correct Formula |
|--------|------|------|-----------------|
| 23 | StereoChorus.cpp | 76 | `0.1f + m_rate.current * 1.9f` |
| 24 | ResonantChorus.cpp | 80 | `0.01f + rate * 1.99f` |
| 27 | FrequencyShifter.cpp | 265 | `m_modDepth.current * 50.0f` |
| 28 | HarmonicTremolo.cpp | 165 | `0.1f + rate * 9.9f` |

### Memory Leak Failure

**Symptom**: Growth rate > 1 MB/min

**Cause**: Temporary buffer allocations in hot paths

**Fix**: Use in-place processing, check for temporary `std::vector` allocations

**Critical File**: `ConvolutionReverb.cpp`
- Lines 161-171 (brightness filter)
- Lines 188-200 (decorrelation)
- Lines 264-279 (damping)
- Lines 517-559 (parameter change detection)

### Performance Regression

**Symptom**: CPU usage > 10%

**Cause**: Algorithmic inefficiency introduced

**Fix**: Profile code, identify bottleneck

---

## WHEN TO RUN TESTS

- ✓ Before committing code
- ✓ After applying bug fixes
- ✓ During code review
- ✓ Before release
- ✓ In CI/CD pipeline (automated)

---

## WORKFLOW

```
1. Make code changes
   ↓
2. Run: ./build_regression_suite.sh
   ↓
3. Run: ./build/test_regression_suite --mode full
   ↓
4. Check results
   ↓
   ├─ PASS → Commit changes ✓
   └─ FAIL → Debug & fix → Re-test
```

---

## FILES TO KNOW

| File | Purpose |
|------|---------|
| `test_regression_suite.cpp` | Main test framework (1200+ lines) |
| `build_regression_suite.sh` | Build script |
| `REGRESSION_TEST_FRAMEWORK_GUIDE.md` | Complete documentation |
| `REGRESSION_FRAMEWORK_DELIVERABLE.md` | Summary report |
| `REGRESSION_TEST_RESULTS.txt` | Generated test results |

---

## KEY METRICS

### LFO Calibration

| Engine | Name | Range | Status |
|--------|------|-------|--------|
| 23 | StereoChorus | 0.1 - 2.0 Hz | ✓ Fixed |
| 24 | ResonantChorus | 0.01 - 2.0 Hz | ✓ Fixed |
| 27 | FrequencyShifter | ±50 Hz mod | ✓ Fixed |
| 28 | HarmonicTremolo | 0.1 - 10.0 Hz | ✓ Fixed |

### Memory Leaks

| Engine | Name | Before | After | Status |
|--------|------|--------|-------|--------|
| 41 | ConvolutionReverb | 357 MB/min | 0.06 MB/min | ✓ Fixed |
| 39 | PlateReverb | 0.04 MB/min | Stable | ✓ Stable |
| 40 | ShimmerReverb | 0.00 MB/min | Stable | ✓ Stable |
| 42 | SpringReverb | 0.00 MB/min | Stable | ✓ Stable |
| 43 | GatedReverb | 0.00 MB/min | Stable | ✓ Stable |

---

## TROUBLESHOOTING

### Build Fails
```bash
# Clean rebuild
rm -rf build/obj
./build_regression_suite.sh
```

### Test Hangs
```bash
# Run under debugger
lldb ./build/test_regression_suite
(lldb) run --mode full
# When hung: Ctrl+C
(lldb) bt  # View backtrace
```

### False Positives
- Run test 3 times to confirm
- Check for system memory pressure
- Review test tolerance settings

---

## NEED MORE INFO?

**Complete Guide**: `REGRESSION_TEST_FRAMEWORK_GUIDE.md`

**Topics Covered**:
- Architecture details
- Adding new tests
- CI/CD integration
- Best practices
- Detailed troubleshooting

---

## SUPPORT

**Documentation Location**:
```
/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
```

**Key Documents**:
1. `REGRESSION_QUICK_START.md` - This file (quick reference)
2. `REGRESSION_TEST_FRAMEWORK_GUIDE.md` - Complete guide
3. `REGRESSION_FRAMEWORK_DELIVERABLE.md` - Mission report
4. `BUG_TRACKING.md` - Known issues database
5. `LFO_CALIBRATION_VERIFICATION_REPORT.txt` - LFO fix verification
6. `REVERB_MEMORY_LEAK_FIX_REPORT.md` - Memory leak fix details

---

## SUMMARY

**What**: Automated regression testing framework
**Why**: Prevent fixed bugs from returning
**How**: Run `./build/test_regression_suite --mode full`
**When**: Before commits, after fixes, before releases
**Duration**: ~10 minutes
**Coverage**: LFO calibration, memory leaks, critical fixes, performance

**Status**: Ready for Production Use ✓

---

**Quick Start Version**: 1.0
**Last Updated**: October 11, 2025
