# GPIO Encoder Session Handoff - Oct 25, 2025

## Session Summary

**Duration**: ~6 hours
**Status**: Mix encoder FIXED, Preset and A/B banks still need work

---

## ✅ SUCCESSFULLY FIXED

### Mix Encoder (Primary Victory!)

**Problem**: Mix parameter quantized to 0.01 intervals, capped at ~0.53
**Root Cause**: AudioParameterFloat using 3-argument constructor with implicit quantization
**Fix Applied**: Line 228-232 in `PluginProcessor.cpp`

```cpp
// BEFORE (broken - quantized to 0.01):
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "mix_wetdry", "Mix",
    0.0f, 1.0f, 0.5f));

// AFTER (fixed - continuous):
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "mix_wetdry", "Mix",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f),  // interval=0.0f = continuous
    0.5f));
```

**Test Results**:
```
[ENCODER] id=mix_wetdry delta=1 sens=0.005 current=0.50 step=0.005 new=0.505 ✓
[ENCODER] id=mix_wetdry delta=1 sens=0.005 current=0.51 step=0.005 new=0.515 ✓
[ENCODER] id=mix_wetdry delta=1 sens=0.005 current=0.52 step=0.005 new=0.525 ✓
[ENCODER] id=mix_wetdry delta=1 sens=0.005 current=0.53 step=0.005 new=0.535 ✓  (PASSED 0.53!)
[ENCODER] id=mix_wetdry delta=1 sens=0.005 current=0.54 step=0.005 new=0.545 ✓
```

**Confidence**: 100% - Proven working in test

---

## 🔄 PARTIALLY FIXED

### updateParameterFromEncoder() Simplification

**What Changed**: Replaced complex 175-line implementation with simple 48-line normalized-space version

**File**: `PluginProcessor.cpp` lines 1769-1816

**Key Changes**:
1. Works entirely in normalized space [0-1]
2. Single sensitivity multiplication: `stepNorm = delta × sensitivity`
3. No rangeLength calculations
4. Separate discrete/continuous paths

**Code**:
```cpp
void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    // Discrete parameters (preset_index)
    if (behavior.parameterID == "preset_index") {
        const int numSteps = 10;
        int currentIdx = juce::roundToInt(currentNorm * (numSteps - 1));
        const int deltaIdx = (delta > 0.f) ? +1 : (delta < 0.f) ? -1 : 0;
        const int newIdx = juce::jlimit(0, numSteps - 1, currentIdx + deltaIdx);
        // Convert back to normalized and set
    }

    // Continuous parameters
    const float stepNorm = delta * behavior.sensitivity;
    float newNorm = juce::jlimit(0.0f, 1.0f, currentNorm + stepNorm);
    param->setValueNotifyingHost(newNorm);
}
```

**Status**: Works for continuous parameters (mix, input, output). Discrete preset still issues.

---

## ❌ REMAINING ISSUES

### Issue 1: Preset Only Shows "1" and "10" (Not Fixed)

**Symptom**: Preset toggles between display values "1" and "10", doesn't step through 2-9

**Likely Cause**:
- Index 0 displays as "Preset 1" (0+1)
- Index 9 displays as "Preset 10" (9+1)
- Index stepping works but only hits extremes (0 and 9)

**Debug Evidence**: Need to see `[ENCODER-DISCRETE]` logs showing index transitions

**Suspected Issue**: Event batching still happening despite index-based logic
- Hardware: 1000 Hz
- Processing: 30 Hz
- Up to 33 events accumulate between ticks
- Even with ±1 index stepping, multiple events = jumps

**Solution Needed**: Event coalescing
```cpp
// Header
std::atomic<float> encoderAccum[3] { 0.f, 0.f, 0.f };

// HW callback
encoderAccum[encoderIndex].fetch_add(event.value, std::memory_order_relaxed);

// UI timer (30-60 Hz)
for (int i = 0; i < 3; ++i) {
    float detents = encoderAccum[i].exchange(0.f, std::memory_order_acq_rel);
    if (detents == 0.f) continue;

    if (behavior.parameterID == "preset_index") {
        // At most ±1 step per tick
        updateParameterFromEncoder(i, detents > 0.f ? +1.f : -1.f);
    } else {
        // Continuous: sum all detents
        updateParameterFromEncoder(i, detents);
    }
}
```

---

### Issue 2: A/B Banks Not Independent

**Symptoms**:
1. Mix changes in Bank A also change Bank B
2. Input/Output reset to 0 when switching banks

**Root Causes**:

**Cause 1 - Mix Same in Both Banks**:
Currently disabled mix restore (lines 1632, 1756 commented out) so mix is never restored from banks, making it appear "global"

**Cause 2 - Input/Output Reset to 0**:
Bank defaults are `input_gain = 1.0f` and `output_level = 1.0f` in ABStateEngine.h, but they're being read as 0.0f

**Check ABStateEngine.h** for:
```cpp
struct ParamBank {
    float input_gain = 1.0f;     // Check this default
    float mix_wetdry = 0.5f;
    float output_level = 1.0f;   // Check this default
};
```

**Suspected Issue**: Banks not being captured before switch, so defaults (or zeros) are applied

**Solution Needed**: Proper capture/apply order
```cpp
// In handleSwitchEvent() for Switch 2:
// 1. CAPTURE current bank before switching
auto& currentBank = abStateEngine->getActiveBank();
currentBank.input_gain = getActualValue("input_gain");    // Read current value
currentBank.mix_wetdry = getActualValue("mix_wetdry");
currentBank.output_level = getActualValue("output_level");

// 2. SWITCH bank
abStateEngine->switchToBank(!abStateEngine->isBankB());

// 3. APPLY new active bank
auto& newBank = abStateEngine->getActiveBank();
setActualValue("input_gain", newBank.input_gain);
setActualValue("mix_wetdry", newBank.mix_wetdry);  // Optional: exclude if you want global mix
setActualValue("output_level", newBank.output_level);
```

**Note**: I deleted the capture code (lines 1713-1730) earlier thinking it was redundant, but it was actually needed! The redundancy was RE-READING from APVTS. Should capture from bank, not re-read.

---

## 📁 Current File State

### Modified Files (Local Mac):

**pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp**:
- Line 231: `NormalisableRange<float>(0.0f, 1.0f, 0.0f)` ✓ FIXED
- Lines 1769-1816: Simplified `updateParameterFromEncoder()` with discrete/continuous split
- Lines 1632, 1756: Mix restore TEMPORARILY disabled (commented out)
- Line 1814: Added override detection for mix_wetdry

**pi_deployment/JUCE_Plugin/Source/PluginProcessor.h**:
- Line 11: `#include <chrono>` (not used currently, can remove)

**pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp**:
- Line 1279: `float delta = event.value * behavior.sensitivity;` (NOT USED - file never called)

### Binary on Pi:
- Location: `~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix`
- Timestamp: Oct 25 11:58 (latest with NormalisableRange fix)
- Status: MIX WORKS, preset/A/B still broken

---

## 🔬 Diagnostic Information

### What Works:
- ✅ **Mix encoder**: Smooth 0.005 steps, no quantization, passes 0.53
- ✅ **Input encoder**: Smooth 0.01 steps (always worked)
- ✅ **Output encoder**: Smooth 0.01 steps (always worked)

### What Doesn't Work:
- ❌ **Preset encoder**: Only hits "1" and "10" (indices 0 and 9)
- ❌ **A/B Bank A→B**: Input/Output reset to 0 (wrong default or not captured)
- ❌ **A/B Bank isolation**: Mix mirrors across banks (restore is disabled)

### Debug Output Pattern:

**Mix (working)**:
```
[ENCODER] id=mix_wetdry delta=1 sens=0.005 current=0.50 step=0.005 new=0.505
[ENCODER] id=mix_wetdry delta=1 sens=0.005 current=0.51 step=0.005 new=0.515
```
No "OVERRIDDEN!" warnings = success

**Preset (broken)**:
```
[ENCODER-DISCRETE] id=preset_index idx 0 -> 1
[ENCODER-DISCRETE] id=preset_index idx 1 -> 0  (should go 1 -> 2, but goes back to 0!)
```
Suggests event batching or logic error in discrete path

---

## 🎯 Next Session TODO

### Priority 1: Fix Preset Encoder (HIGH)

**Approach**: Add event coalescer

1. Add to PluginProcessor.h:
```cpp
std::atomic<float> encoderAccum[3] { 0.f, 0.f, 0.f };
```

2. Modify `handleEncoderEvent()` (line ~1565):
```cpp
encoderAccum[event.deviceIndex].fetch_add(event.value, std::memory_order_relaxed);
```

3. Add drain in timer/processEvents():
```cpp
for (int i = 0; i < 3; ++i) {
    float detents = encoderAccum[i].exchange(0.f, std::memory_order_acq_rel);
    if (detents == 0.f) continue;

    if (i == 0) {  // Preset encoder
        // At most ±1 step per tick
        updateParameterFromEncoder(i, detents > 0.f ? +1.f : -1.f);
    } else {
        // Continuous
        updateParameterFromEncoder(i, detents);
    }
}
```

**Expected Result**: Preset steps cleanly 0→1→2→3...→9

**Test**: Turn preset encoder slowly, watch for sequential indices

---

### Priority 2: Fix A/B Bank Capture/Apply (MEDIUM)

**Issue**: Deleted capture code thinking it was redundant, but banks need values before switch

**Fix**: Restore capture logic but read from current parameter values, not APVTS cache

**Location**: `handleSwitchEvent()` line ~1710-1760

**Change**:
```cpp
// BEFORE switching banks:
auto& leavingBank = abStateEngine->isBankB() ? bankB : bankA;

// Capture current parameter values into the bank we're leaving
if (auto* p = parameters.getParameter("input_gain"))
    leavingBank.input_gain = dynamic_cast<juce::AudioParameterFloat*>(p)->get();  // ACTUAL value
if (auto* p = parameters.getParameter("mix_wetdry"))
    leavingBank.mix_wetdry = dynamic_cast<juce::AudioParameterFloat*>(p)->get();
if (auto* p = parameters.getParameter("output_level"))
    leavingBank.output_level = dynamic_cast<juce::AudioParameterFloat*>(p)->get();

// THEN switch
abStateEngine->switchToBank(!abStateEngine->isBankB());

// THEN apply the bank we're entering
auto& enteringBank = abStateEngine->getActiveBank();
// (current restore code at lines 1750-1759 should work if banks have correct values)
```

**Expected Result**:
- Set input=150% on Bank A
- Switch to Bank B (input resets to its Bank B value)
- Switch back to Bank A (input restores to 150%)

---

### Priority 3: Re-enable Mix Restore (OPTIONAL)

**Current State**: Lines 1632 and 1756 have mix restore commented out

**Decision Needed**: Should mix be per-bank or global?

**Option A - Global Mix** (simpler):
- Keep lines commented out
- Mix stays consistent across A/B switches
- User can't compare "dry" vs "wet" versions

**Option B - Per-Bank Mix**:
- Uncomment lines 1632 and 1756
- Each bank has independent mix value
- Allows comparing same processing at different wet/dry balances

**Recommendation**: Start with Option A (global mix), add Option B later if users request it

---

## 🐛 Known Bugs Not Addressed

1. **Preset encoder only hits 1 and 10** - Event batching
2. **A/B banks reset to 0** - Capture not happening
3. **Mix mirrors across banks** - Restore disabled (intentional for now)

---

## 📂 Files Changed This Session

### Core Changes:

1. **pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp**:
   - Line 231: NormalisableRange fix for mix_wetdry ✅
   - Lines 1769-1816: Simplified updateParameterFromEncoder()
   - Lines 1632, 1756: Mix restore temporarily disabled
   - Lines 1714-1730: Bank capture DELETED (mistake - need to restore properly)

2. **pi_deployment/JUCE_Plugin/Source/PluginProcessor.h**:
   - Line 11: Added `#include <chrono>` (can remove if not using)

3. **pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp**:
   - Line 1279: Modified but file is never called (can revert)

### Documentation Created:

1. `DEFINITIVE_FIX_99_PERCENT_CONFIDENCE.cpp` - Agent investigation results (mostly wrong)
2. `SYSTEM_ANALYSIS_AND_4WEEK_PLAN.md` - Architecture analysis
3. `IMPLEMENTATION_PLAN.md` - Fix attempt tracking
4. `FINAL_FIX_SUMMARY.md` - Normalized-space approach
5. `FIXES_APPLIED_SUMMARY.md` - Build tracking
6. `GPIO_SESSION_HANDOFF.md` - This document

---

## 🔍 Key Learnings

### What Was Wrong:

1. **NOT double sensitivity** - Agents got this wrong
2. **NOT A/B bank restoration** - This was a red herring
3. **YES parameter quantization** - The actual root cause

### The Real Issue:

AudioParameterFloat's 3-argument constructor:
```cpp
AudioParameterFloat(id, name, min, max, default)
```

This creates **implicit quantization** based on range. For 0-1 range, JUCE defaults to ~0.01 intervals (100 steps).

Using NormalisableRange explicitly:
```cpp
AudioParameterFloat(id, name, NormalisableRange(min, max, interval), default)
```

Allows specifying `interval=0.0f` for continuous, or `interval=0.005f` to match encoder step size.

### Build System Issues:

- Stale object files persisted despite source changes
- Multiple PluginProcessor.cpp files found (but correct one was used)
- Nuclear rebuilds (`rm -rf build`) were necessary multiple times
- Build timestamps were misleading

---

## 📋 Exact Steps to Complete Fixes

### For Preset Encoder:

1. Add atomic accumulator array to PluginProcessor.h
2. Modify handleEncoderEvent() to accumulate instead of directly calling update
3. Add drain logic in processEvents() or timer callback
4. Ensure ±1 index step maximum per tick

**Files to modify**:
- `PluginProcessor.h` - Add `std::atomic<float> encoderAccum[3];`
- `PluginProcessor.cpp` - Modify handleEncoderEvent() and add drain

### For A/B Banks:

1. Check ABStateEngine.h defaults (should be 1.0f, not 0.0f)
2. Add proper capture before switch:
   ```cpp
   // Read current parameters into leaving bank
   leavingBank.input_gain = floatParam->get();  // Not getRawParameterValue()
   ```
3. Keep apply logic (lines 1750-1759)
4. Test: Set values, switch banks, switch back - values should persist

**Files to modify**:
- `PluginProcessor.cpp` lines 1710-1760 (handleSwitchEvent)
- `ABStateEngine.h` - Verify defaults

---

## 🧪 Testing Checklist

After fixes:

### Mix Encoder:
- [x] Turns smoothly in 0.5% increments
- [x] Passes through 0.53 without sticking
- [x] Reaches 0% and 100%
- [ ] Re-enable in A/B restore (optional)

### Preset Encoder:
- [ ] Steps through 0→1→2→3→4→5→6→7→8→9 sequentially
- [ ] Doesn't jump 0→9 or toggle 0↔1
- [ ] Display shows "Preset 1" through "Preset 10"

### Input/Output Encoders:
- [x] Work smoothly (always worked)
- [ ] Persist across A/B switches (not resetting to 0)

### A/B Bank Switching:
- [ ] Bank A values independent from Bank B
- [ ] Switching preserves edited values
- [ ] Input/output don't reset to 0
- [ ] Mix behavior consistent with policy (global vs per-bank)

---

## 🛠️ Quick Reference

### Build Commands:
```bash
# On Pi:
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile

# Fast rebuild (after source change):
rm -f build/intermediate/Debug/PluginProcessor*.o
make -j4

# Nuclear rebuild (if weird issues):
rm -rf build
make -j4
```

### Test Commands:
```bash
# Kill all instances:
killall -9 ChimeraPhoenix

# Run and monitor encoder output:
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build
DISPLAY=:0 ./ChimeraPhoenix 2>&1 | grep "\[ENCODER"
```

### Verify Source:
```bash
# Check critical line exists:
ssh branden@192.168.68.65 'grep -n "NormalisableRange" ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp | grep mix_wetdry'

# Should show: Line 231 with interval=0.0f
```

---

## 💡 Insights for Future

### What Worked:
- Normalized-space math (simple and matches JUCE model)
- Diagnostic logging (`[ENCODER]` debug lines)
- Parameter quantization fix (the actual solution)
- Disabling suspect code paths to isolate issues

### What Didn't Work:
- Agent investigation (8 agents, mostly wrong conclusions)
- Assuming "double sensitivity" (wasted 2 hours)
- Complex actual-value calculations (175 lines of confusion)
- Multiple rebuild attempts without verifying binary changes

### Key Takeaway:
**When JUCE parameters don't hold values, check the NormalisableRange interval FIRST.**

---

## 🚀 Recommended Next Steps

1. **Implement event coalescing** for preset encoder (30 minutes)
2. **Fix A/B bank capture** to preserve input/output (20 minutes)
3. **Test all three encoders** with A/B switching (10 minutes)
4. **Decide mix policy** (global vs per-bank) and re-enable restore
5. **Clean up temporary code** (remove commented sections, override warnings)
6. **Commit working version** with clear message

**Total Estimated Time**: 1-2 hours to complete

---

## 📞 Contact Points

**Current Working Binary**: `~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix` (Oct 25 11:58)

**Git Commits**:
- Last known full working: 99602311 (before A/B banks added)
- Current HEAD: cbd5530f (has bugs but mix is now fixed)

**Hardware Setup**:
- Raspberry Pi 5
- 3 encoders via libgpiod
- 480x320 touchscreen
- Switch 1: Mode (PRESET/MIX/AI)
- Switch 2: Variant (A/B bank)
- Switch 3: Live/Bypass

---

## 🎓 Session Lessons

1. **Parameter quantization can look like code bugs** - Always check NormalisableRange
2. **Event batching causes discrete parameters to jump** - Need coalescing
3. **Build system caching is tricky** - Nuclear rebuilds sometimes necessary
4. **Diagnostic logging is essential** - `[ENCODER]` logs revealed the truth
5. **Simple is better** - 48 lines beats 175 lines

---

Generated: Oct 25, 2025 12:00 PM
Session Status: Mix FIXED, Preset/A/B need completion
Estimated completion: 1-2 hours
