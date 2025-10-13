# ENDURANCE TESTING QUICK START GUIDE
## Project Chimera v3.0 Phoenix

---

## Overview

This guide shows you how to run long-term stability tests to catch issues like:
- Memory leaks (357 MB/min on Plate Reverb!)
- Performance degradation (6007% slower over time!)
- Audio quality issues (DC offset, clipping)
- Buffer overflow issues
- Sample rate compatibility

---

## Quick Start: Test Critical Engines

### 1. Using Existing Endurance Test (5 minutes per engine)

The background test already completed on 10 critical engines. To run on specific engines:

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Build the endurance test
./build_endurance_test.sh

# Run on specific engines
cd build

# Test reverbs (most problematic)
./endurance_test 5 39  # Convolution Reverb (11.2 MB/min leak)
./endurance_test 5 40  # Shimmer Reverb (CRITICAL - 98% DC offset)
./endurance_test 5 41  # Plate Reverb (CRITICAL - 357 MB/min leak!)
./endurance_test 5 42  # Spring Reverb (1029% degradation)
./endurance_test 5 43  # Gated Reverb (564% degradation)

# Test delays
./endurance_test 5 34  # Tape Echo (3.9 MB/min leak)
./endurance_test 5 35  # Digital Delay (24% DC offset)
./endurance_test 5 36  # Magnetic Drum (CRITICAL - 1121% degradation)
./endurance_test 5 37  # Bucket Brigade (584% degradation)
./endurance_test 5 38  # Buffer Repeat (264% degradation)

# Custom duration (in minutes)
./endurance_test 30 41  # 30-minute test on Plate Reverb
./endurance_test 1 40   # Quick 1-minute test on Shimmer
```

---

## Test Results Interpretation

### Memory Leak Analysis

**Thresholds:**
- < 1 MB/min: Acceptable
- 1-5 MB/min: Small leak (fix medium priority)
- 5-10 MB/min: Moderate leak (fix high priority)
- 10-100 MB/min: Large leak (fix critical priority)
- > 100 MB/min: **SEVERE LEAK** (production blocker)

**Current Results:**
```
Engine 41 (Plate Reverb):         357.8 MB/min  ❌ CRITICAL
Engine 39 (Convolution Reverb):    11.2 MB/min  ⚠️  HIGH
Engine 40 (Shimmer Reverb):         6.1 MB/min  ⚠️  MODERATE
Engine 34 (Tape Echo):              3.9 MB/min  ⚠️  LOW
Engine 35 (Digital Delay):          2.0 MB/min  ⚠️  LOW
Engine 37 (Bucket Brigade):         1.4 MB/min  ⚠️  LOW
Engine 38 (Buffer Repeat):          1.4 MB/min  ⚠️  LOW
Engine 36 (Magnetic Drum):          0.5 MB/min  ✅ OK
Engine 42 (Spring Reverb):          0.0 MB/min  ✅ OK
Engine 43 (Gated Reverb):           0.0 MB/min  ✅ OK
```

### Performance Degradation Analysis

**Thresholds:**
- < 20%: Acceptable
- 20-200%: Moderate (monitor)
- 200-1000%: High (needs optimization)
- 1000-5000%: Critical (major issue)
- > 5000%: **EXTREME** (production blocker)

**Current Results:**
```
Engine 41 (Plate Reverb):          6007%  ❌ EXTREME
Engine 40 (Shimmer Reverb):        1371%  ❌ CRITICAL
Engine 36 (Magnetic Drum):         1121%  ❌ CRITICAL
Engine 42 (Spring Reverb):         1029%  ❌ CRITICAL
Engine 39 (Convolution Reverb):     589%  ⚠️  HIGH
Engine 37 (Bucket Brigade):         584%  ⚠️  HIGH
Engine 43 (Gated Reverb):           564%  ⚠️  HIGH
Engine 34 (Tape Echo):              373%  ⚠️  MODERATE
Engine 38 (Buffer Repeat):          264%  ⚠️  MODERATE
Engine 35 (Digital Delay):          191%  ⚠️  MODERATE
```

### Audio Quality Issues

**Failure Criteria:**
- Any NaN or Inf: **CRITICAL**
- DC offset > 1% of blocks: Issue
- DC offset > 50%: **CRITICAL**
- Clipping > 1% of blocks: Issue

**Current Results:**
```
Engine 40 (Shimmer Reverb):
  - DC Offset: 98% of blocks     ❌ CRITICAL
  - Clipping: 100% of blocks     ❌ CRITICAL

Engine 35 (Digital Delay):
  - DC Offset: 24% of blocks     ⚠️  MODERATE

Engine 38 (Buffer Repeat):
  - DC Offset: 7% of blocks      ⚠️  MINOR

Engine 34 (Tape Echo):
  - DC Offset: 1% of blocks      ⚠️  MINOR

All other engines: Perfect ✅
```

---

## Understanding Test Output

### Example Output
```
═══════════════════════════════════════════════════════════════
  TEST RESULTS
═══════════════════════════════════════════════════════════════

DURATION:
  Test Time:       0.06 minutes          ← Actual wall-clock time
  Samples Processed: 14400000             ← 5 minutes of audio
  Blocks Processed:  28125                ← Number of 512-sample blocks

MEMORY ANALYSIS:
  Initial:         28.83 MB              ← Memory at start
  Final:           50.28 MB              ← Memory at end
  Peak:            50.28 MB              ← Highest memory usage
  Growth:          21.45 MB ⚠️ LEAK      ← Total growth
  Growth Rate:     357.834 MB/min        ← Leak rate (CRITICAL if >1)

PERFORMANCE ANALYSIS:
  Avg Block Time:  123.45 μs             ← Average processing time
  Min Block Time:  18.71 μs              ← Fastest block
  Max Block Time:  6031.38 μs            ← Slowest block (spikes!)
  Real-time Ratio: 0.012x (excellent)    ← CPU headroom
  Degradation:     ⚠️ 6007.0% slower    ← First vs last blocks

AUDIO QUALITY:
  NaN Detected:    0 ✓                   ← No invalid numbers
  Inf Detected:    0 ✓                   ← No infinities
  DC Offset:       107 blocks ✓          ← Low-frequency drift
  Clipping:        0 blocks ✓            ← No clipping

OVERALL RESULT:  ❌ FAILED
  - Memory leak detected
  - Performance degradation
```

### Key Metrics Explained

**Real-time Ratio:**
- The fraction of real-time required to process audio
- 0.001x = 1000x faster than real-time (excellent headroom)
- 0.5x = 2x faster than real-time (still good)
- 1.0x = exactly real-time (danger zone)
- > 1.0x = slower than real-time (will glitch)

**Degradation:**
- Compares first 10% of blocks to last 10% of blocks
- Shows how much slower processing becomes over time
- 100% = twice as slow at end vs start
- 6007% = 60x slower at end vs start (CRITICAL)

**Memory Growth Rate:**
- MB per minute of processing
- At 357 MB/min, a 10-minute song would leak 3.5 GB
- At 1 MB/min (threshold), 1 hour = 60 MB (acceptable)

---

## Recommended Test Schedule

### Phase 1: Critical Engines (Week 1)

Test the 3 production blockers after fixes:

```bash
# After fixing Engine 41 (Plate Reverb)
cd build && ./endurance_test 30 41
# Expected: < 1 MB/min, < 20% degradation

# After fixing Engine 40 (Shimmer Reverb)
cd build && ./endurance_test 30 40
# Expected: No DC offset, no clipping

# After fixing Engine 36 (Magnetic Drum)
cd build && ./endurance_test 30 36
# Expected: < 100% degradation
```

### Phase 2: High Priority (Week 2-3)

Test engines with moderate issues:

```bash
# Test each for 15 minutes after fixes
./endurance_test 15 39  # Convolution Reverb
./endurance_test 15 35  # Digital Delay
./endurance_test 15 42  # Spring Reverb
./endurance_test 15 43  # Gated Reverb
```

### Phase 3: All Engines (Week 4+)

Run comprehensive suite on all 56 engines:

```bash
# 10-minute tests on all engines
for i in {0..55}; do
  ./endurance_test 10 $i
done
```

---

## Troubleshooting

### Build Fails
```bash
# Error: "Main build must be completed first"
# Solution: Build the main project first
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./build_v2.sh
```

### Test Takes Too Long
```bash
# Use shorter duration for quick checks
./endurance_test 1 41   # 1-minute test

# Or test specific category
./endurance_test 5 34   # Just one delay
./endurance_test 5 39   # Just one reverb
```

### Memory Issues on Your System
```bash
# If test causes system issues, use shorter durations
./endurance_test 2 41   # 2-minute test
```

---

## Advanced: Comprehensive Test Suite

The new comprehensive test suite (`test_endurance_suite.cpp`) includes 5 test types:

### Test 1: Memory Stability (30 min)
```bash
cd build && ./endurance_suite 1 41
```
- Tests for memory leaks
- Samples memory every 10 seconds
- Fails if growth > 1 MB/min

### Test 2: CPU Stability (30 min)
```bash
cd build && ./endurance_suite 2 41
```
- Tracks CPU usage over time
- Measures CPU drift
- Fails if drift > 20%

### Test 3: Parameter Stability (10 min)
```bash
cd build && ./endurance_suite 3 41
```
- Continuous LFO modulation on all parameters
- Tests for parameter-induced instability
- Checks for drift/crashes

### Test 4: Buffer Overflow (5 min)
```bash
cd build && ./endurance_suite 4 41
```
- Tests buffer sizes: 64, 128, 256, 512, 1024, 2048, 4096, 8192
- Rapid buffer size changes
- Checks for crashes/artifacts

### Test 5: Sample Rate (5 min)
```bash
cd build && ./endurance_suite 5 41
```
- Tests rates: 44.1k, 48k, 88.2k, 96k, 192k
- Verifies compatibility
- Checks for rate-dependent bugs

### Run All Tests
```bash
cd build && ./endurance_suite 0 41  # All tests on Engine 41
```

**Note:** Building this requires pre-compiled object files. If build fails, use the existing `endurance_test` instead.

---

## Critical Engines Priority List

Based on test results, test these engines first:

### Priority 1: Production Blockers (Fix Immediately)
1. **Engine 41 (Plate Reverb)**: 357 MB/min leak + 6007% degradation
2. **Engine 40 (Shimmer Reverb)**: 98% DC offset + 100% clipping
3. **Engine 36 (Magnetic Drum)**: 1121% degradation

### Priority 2: High Priority (Fix Week 2)
4. **Engine 39 (Convolution Reverb)**: 11.2 MB/min leak
5. **Engine 35 (Digital Delay)**: 24% DC offset
6. **Engine 42 (Spring Reverb)**: 1029% degradation
7. **Engine 43 (Gated Reverb)**: 564% degradation

### Priority 3: Medium Priority (Fix Week 3-4)
8. **Engine 34 (Tape Echo)**: 3.9 MB/min leak
9. **Engine 37 (Bucket Brigade)**: 584% degradation
10. **Engine 38 (Buffer Repeat)**: 264% degradation

---

## Success Criteria

An engine PASSES endurance testing when:
- ✅ Memory growth < 1 MB/min
- ✅ Performance degradation < 20%
- ✅ No NaN or Inf values
- ✅ DC offset in < 1% of blocks
- ✅ Clipping in < 1% of blocks
- ✅ No crashes for 30+ minutes

---

## Files & Documentation

- **Test Program:** `endurance_test.cpp` (existing, working)
- **Build Script:** `build_endurance_test.sh`
- **Background Test Results:** See bash output (592d64)
- **Summary:** `ENDURANCE_TEST_SUMMARY.md`
- **Full Report:** `COMPREHENSIVE_ENDURANCE_STRESS_TEST_REPORT.md`
- **This Guide:** `ENDURANCE_TESTING_QUICK_START.md`

---

## Next Steps

1. **Read the full report:** `COMPREHENSIVE_ENDURANCE_STRESS_TEST_REPORT.md`
2. **Fix Engine 41 first** (Plate Reverb - worst offender)
3. **Re-test after each fix** using 30-minute tests
4. **Move to next priority** after verifying fix
5. **Run comprehensive suite** after all critical fixes

---

**Created:** October 11, 2025
**Version:** 1.0
**Contact:** See main project documentation
