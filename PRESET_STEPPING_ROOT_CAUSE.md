# Preset Stepping - Root Cause & Final Fix

**Date**: October 26, 2025 01:31
**Status**: Fix identified and committed, building on Pi .65

---

## 🐛 The Actual Bug

### What the Logs Revealed
```
[ENCODER-DISCRETE] rawNorm=9 currentIdx=81 -> newIdx=9
```

**The smoking gun**: `rawNorm=9` and `currentIdx=81`

### Root Cause
```cpp
// BROKEN CODE:
const float rawNorm = rawParam->load();  // Returns 9 (actual int!)
const int currentIdx = juce::roundToInt(rawNorm * 9.0f);  // 9 * 9 = 81 ❌
```

**AudioParameterInt's `getRawParameterValue()` returns the ACTUAL INTEGER VALUE (0-9), NOT normalized (0-1)!**

This is different from AudioParameterFloat, where raw values ARE normalized.

### Why It Only Showed 1 and 10
```
Turn CW from index 0:
  currentIdx = round(0 * 9) = 0  ✓
  newIdx = 0 + 1 = 1
  newNorm = 1/9 = 0.111
  ✓ Should go to preset 2

BUT after setting to 0.111:
  Next read: rawParam->load() returns... WAIT

The issue: When we SET newNorm=0.111 via setValueNotifyingHost(),
JUCE converts it BACK to the integer (round(0.111 * 9) = 1),
so rawParam->load() should return 1, not 9!

Unless...
```

**Wait - the logs show `rawNorm=9` when reading.** If I just set it to `newNorm=0.111` (index 1), why does the next read show 9?

### The ACTUAL Issue
Looking at the pattern again:
```
Set newNorm=0.111 (index 1)
Next cycle reads: rawNorm=9
```

**This means something ELSE is changing the parameter to 9 between my write and the next read!**

Possible causes:
1. GPIOPresetManager->setCurrentPresetIndex(newIdx) triggers something
2. parameterChanged callback does something
3. There's a feedback loop

---

## ✅ The Fix (Commit ce27f47f)

```cpp
// Correct approach: Raw value for AudioParameterInt IS the integer
const int currentIdx = static_cast<int>(rawParam->load());  // No multiplication!

const int deltaIdx = (delta > 0.f) ? +1 : -1;
const int newIdx = juce::jlimit(0, 9, currentIdx + deltaIdx);
const float newNorm = static_cast<float>(newIdx) / 9.0f;

param->setValueNotifyingHost(newNorm);
```

---

## 🔍 Still Need to Test

**Current Build**: Oct 26 01:31 (building now on Pi .65)
**Expected Log Pattern**:
```
[ENCODER-DISCRETE] currentIdx=0 -> newIdx=1 newNorm=0.111 (display 2/10)
[ENCODER-DISCRETE] currentIdx=1 -> newIdx=2 newNorm=0.222 (display 3/10)
[ENCODER-DISCRETE] currentIdx=2 -> newIdx=3 newNorm=0.333 (display 4/10)
...
```

**If it STILL shows `currentIdx=81`**, then the issue is NOT the multiplication - it's that **something is modifying the parameter between writes**.

---

## 🎯 Next Steps

### When Build Finishes:
1. Run the binary
2. Turn E1 slowly
3. Check logs for `currentIdx` values

**If currentIdx is correct (0, 1, 2, 3...)**: ✅ FIXED!

**If currentIdx is still wrong (81, 0, etc.)**: Need to find what's modifying the parameter

---

**Build is running on Pi .65 now. Wait for it to complete (~5 more min), then test.**

Log command:
```bash
ssh branden@192.168.68.65 "tail -f /tmp/FINAL.log | grep ENCODER-DISCRETE"
```
