# PlateReverb Fix Report

**Date**: October 11, 2025
**Engine**: PlateReverb (Engine 39)
**Bug**: Pre-delay buffer read-before-write causing zero output
**Status**: ✅ FIXED

---

## Problem

PlateReverb (Engine 39) was producing zero output after the initial impulse. The reverb tail was completely silent, making it unusable.

**Symptoms**:
- Peak at sample 0: 0.767 (input impulse passes through)
- All subsequent samples: 0.000 (complete silence)
- No reverb tail or decay

---

## Root Cause

**File**: `JUCE_Plugin/Source/PlateReverb.cpp`
**Lines**: 305-323 (process method)

The pre-delay buffer was implemented incorrectly:

```cpp
// BUGGY CODE (Original):
if (predelaySize > 0) {
    delayedL = predelayBufferL[predelayIndex];  // READ FIRST (reads zeros!)
    delayedR = predelayBufferR[predelayIndex];

    predelayBufferL[predelayIndex] = inputL;    // WRITE SECOND
    predelayBufferR[predelayIndex] = inputR;

    if (++predelayIndex >= predelaySize) {
        predelayIndex = 0;
    }
}
```

**Problem**:
1. During buffer fill-up period, reads happen from uninitialized buffer (zeros)
2. Zeros are fed to the Freeverb algorithm
3. Freeverb processes zeros → outputs zeros
4. No reverb tail is generated

---

## Solution

Reorder operations: **write before read**, and calculate proper delayed read index.

```cpp
// FIXED CODE:
if (predelaySize > 0) {
    // Write current input to buffer first
    predelayBufferL[predelayIndex] = inputL;
    predelayBufferR[predelayIndex] = inputR;

    // Calculate read index (predelaySize samples ago, wrapped)
    int readIndex = predelayIndex - predelaySize;
    if (readIndex < 0) {
        readIndex += static_cast<int>(predelayBufferL.size());
    }

    // Read delayed signal
    delayedL = predelayBufferL[readIndex];
    delayedR = predelayBufferR[readIndex];

    if (++predelayIndex >= static_cast<int>(predelayBufferL.size())) {
        predelayIndex = 0;
    }
}
```

**Key Changes**:
1. Write input to buffer BEFORE reading
2. Calculate proper read index with wraparound: `readIndex = (writeIndex - delaySize + bufferSize) % bufferSize`
3. Use total buffer size (not predelaySize) for wraparound check

---

## Test Results

### Before Fix
```
Engine 39 (PlateReverb):
  Peak Left:   0.767 at sample 0
  Peak Right:  0.767 at sample 0

  0ms:    L=0.767 R=0.767  (input impulse)
  10ms:   L=0     R=0      (SILENCE)
  100ms:  L=0     R=0      (SILENCE)
  500ms:  L=0     R=0      (SILENCE)

  Status: BROKEN - No reverb tail
```

### After Fix
```
Engine 39 (PlateReverb):
  Peak Left:   0.026 at sample 3394 (71ms)
  Peak Right:  0.024 at sample 2795 (58ms)

  0ms:    L=0           R=0           (dry suppressed at 100% wet)
  10ms:   L=0           R=0           (delay building up)
  100ms:  L=-0.000525   R=0.0155      (REVERB TAIL PRESENT!)
  500ms:  L=0.000499    R=-0.00036    (smooth decay)
  1s:     L=-1.04e-06   R=2.93e-05    (tail still audible)

  Status: WORKING - Reverb tail present, smooth decay
```

---

## Regression Testing

Tested all other reverbs to ensure no regressions:

| Engine | Name | Status | Notes |
|--------|------|--------|-------|
| 39 | PlateReverb | ✅ FIXED | Reverb tail now present |
| 40 | ShimmerReverb | ✅ Working | No regression |
| 41 | ConvolutionReverb | ⚠️ Broken | Pre-existing issue (not caused by this fix) |
| 42 | SpringReverb | ✅ Working | No regression |
| 43 | GatedReverb | ✅ Working | No regression |

---

## Impact

- **Bug Severity**: CRITICAL (engine completely non-functional)
- **Fix Complexity**: Low (reorder buffer operations)
- **Risk**: Low (isolated to PlateReverb pre-delay)
- **Testing**: Comprehensive (impulse response analysis)

---

## Files Modified

1. `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/PlateReverb.cpp`
   - Lines 305-323: Fixed pre-delay buffer logic
   - Lines 417-420: Removed debug printf statements

---

## Next Steps

1. ✅ Fix applied and tested
2. ✅ Regression tests passed
3. ⏭️ Document in BUGS_BY_SEVERITY.md (Bug #2 resolved)
4. ⏭️ Move to next critical bug (Engine 33 - Harmonizer crash)

---

## Technical Notes

**Pre-delay Buffer Pattern**:
- Common issue in circular buffers: read-before-write vs write-before-read
- Always write first, then calculate delayed read index
- Use modulo arithmetic for wraparound: `(current - delay + bufferSize) % bufferSize`

**Freeverb Architecture**:
- 8 parallel comb filters per channel
- 4 serial allpass filters per channel
- Pre-delay BEFORE comb filters
- If pre-delay feeds zeros → entire algorithm outputs zeros
