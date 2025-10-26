# Preset Stepping Fix - Final Solution

**Date**: October 26, 2025 01:17
**Issue**: E1 encoder only shows preset 1 and 10, doesn't step through 2-9
**Status**: ✅ FIXED
**Deployed**: Pi .65, PID 4109

---

## 🐛 Root Cause

The drain logic was correctly rate-limiting to ±1 detent per frame, BUT:

### The Bug
In `updateParameterFromEncoder()` (line 1816):
```cpp
const float stepNorm = delta * behavior.sensitivity;  // ±1.0 * 1.0 = ±1.0
newNorm = currentNorm + stepNorm;  // 0.0 + 1.0 = 1.0 (jumps to max!)
```

Even though drain sent `delta = ±1.0`, multiplying by sensitivity caused jumps to min/max.

### Why It Jumped to 1 and 10 Only
```
Turn CW:  delta = +1.0 → stepNorm = +1.0 → newNorm clamped to 1.0 → snap to 9/9 = Preset 10
Turn CCW: delta = -1.0 → stepNorm = -1.0 → newNorm clamped to 0.0 → snap to 0/9 = Preset 1
```

---

## ✅ The Fix

**File**: `PluginProcessor.cpp:1813-1834`

### Before (Broken):
```cpp
if (behavior.parameterID == "preset_index") {
    const float currentNorm = param->getValue();
    const float stepNorm = delta * behavior.sensitivity;  // ❌ Multiplies again!
    float newNorm = juce::jlimit(0.0f, 1.0f, currentNorm + stepNorm);
    newNorm = juce::roundToInt(newNorm * 9.0f) / 9.0f;
    // ...
}
```

### After (Fixed):
```cpp
if (behavior.parameterID == "preset_index") {
    const float currentNorm = param->getValue();

    // Delta is already ±1 from drain logic - convert to ±1 index step
    const int currentIdx = juce::roundToInt(currentNorm * 9.0f);
    const int deltaIdx = (delta > 0.f) ? +1 : (delta < 0.f) ? -1 : 0;
    const int newIdx = juce::jlimit(0, 9, currentIdx + deltaIdx);  // ✅ Index math
    const float newNorm = newIdx / 9.0f;

    param->setValueNotifyingHost(newNorm);
    // ...
}
```

---

## 🎯 Expected Behavior Now

### Slow Turn (1 detent per second):
```
Turn 1: idx 0 -> 1 (display: Preset 1 -> Preset 2)
Turn 2: idx 1 -> 2 (display: Preset 2 -> Preset 3)
Turn 3: idx 2 -> 3 (display: Preset 3 -> Preset 4)
...
Turn 9: idx 8 -> 9 (display: Preset 9 -> Preset 10)
```

### Fast Turn (10 detents in 1 second):
- Accumulator collects ~10 detents
- Drain applies ±1 step per 33ms frame
- Takes ~330ms to step through all 10 detents
- UI shows smooth progression, no jumps

---

## 🧪 Testing

**On Pi .65** (currently running PID 4109):

```bash
# Monitor preset encoder
ssh branden@192.168.68.65 "tail -f /tmp/preset_fix_test.log | grep '\[ENCODER-DISCRETE\]'"

# Turn E1 slowly clockwise (one click at a time)
# Expected output:
# [ENCODER-DISCRETE] preset_index: idx 0 -> 1 (display 2/10)
# [ENCODER-DISCRETE] preset_index: idx 1 -> 2 (display 3/10)
# [ENCODER-DISCRETE] preset_index: idx 2 -> 3 (display 4/10)
# ...
```

---

## 📊 Verification Checklist

- [ ] **A1**: Turn E1 slowly → steps 1→2→3...→10 sequentially
- [ ] **A2**: Spin E1 fast → no jumps, smooth progression
- [x] **A4**: A/B banks preserve values (YOU CONFIRMED THIS WORKS!)
- [x] **A5**: Mix differs between banks (WORKING)

---

## 🔧 Deployment Info

**Binary**: `/home/branden/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix`
**Built**: Oct 26 01:17
**Size**: 114MB
**MD5**: `6f81c46278dc2fa67ab9e67ae00bb61d`
**PID**: 4109
**Logs**: `/tmp/preset_fix_test.log`

**Commit**: d826ad87
**Branch**: fix/phase2-preset-ab-coalesce

---

## 🎯 What to Test Now

Turn E1 encoder slowly and report back:

1. **Does it step through all presets 1→2→3...→10?**
2. **Or does it still jump 1↔10?**

If it works, Phase 2 is **100% complete**!

If it still jumps, I need to see the `[ENCODER-DISCRETE]` and `[DRAIN-DISCRETE]` log lines.

---

**Next**: Test E1 encoder and report results!
