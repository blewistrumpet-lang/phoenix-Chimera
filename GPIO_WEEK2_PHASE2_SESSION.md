# GPIO Control Implementation - Week 2 Phase 2 Session

**Date:** October 24, 2025 (Continuation)
**Goal:** Implement preset system (Week 2 Phase 2 of Trinity GPIO Plan)
**Status:** 🟡 PARTIAL - Core infrastructure complete, bugs remain

---

## 🎯 Session Objectives

From the 4-Week Trinity GPIO Plan, Week 2 Phase 2 goals:
- ✅ RAM presets (10 slots)
- ✅ E1 browse presets in PRESET mode
- 🟡 E1 button to load preset (implemented, not tested)
- 🟡 E2 button to quick save (implemented, not tested)
- ✅ JSON persistence with atomic saves
- ✅ Preset index cache

---

## ✅ What Was Completed

### 1. GPIOPresetManager Class Created
**File:** `pi_deployment/JUCE_Plugin/Source/GPIOPresetManager.h`

```cpp
class GPIOPresetManager {
    struct Preset {
        String name;
        ABStateEngine::ParamBank bankA;
        ABStateEngine::ParamBank bankB;
        bool valid;
    };

    Preset presets[10];  // 10 RAM slots

    void savePreset(int index, ...);
    bool loadPreset(int index, ...);
    bool saveToJSON(File file);      // Atomic temp→rename
    bool loadFromJSON(File file);
    bool savePresetIndexCache(File file);
    bool loadPresetIndexCache(File file);
};
```

**Features:**
- 10 preset slots in RAM (0-9)
- Each preset stores both A and B banks
- JSON persistence with atomic writes
- Preset index cache survives reboots
- Graceful handling of empty slots

### 2. PluginProcessor Integration
**Files Modified:**
- `pi_deployment/JUCE_Plugin/Source/PluginProcessor.h`
- `pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp`

**Changes:**
1. Added `preset_index` parameter (AudioParameterInt, 0-9)
2. Initialized `gpioPresetManager` in constructor
3. Wired encoder button callback for E1/E2 presses
4. Implemented `handleEncoderButtonEvent()`:
   - E1 button → Load preset from current index
   - E2 button → Quick save to current index
5. Added file path helpers for JSON storage
6. Enhanced `updateParameterFromEncoder()` to handle integer parameters

### 3. ControlState Encoder Mapping
**File:** `pi_deployment/JUCE_Plugin/Source/ControlState.h`

**PRESET Mode mapping:**
```cpp
case Mode::PRESET:
    case 0:  // E1 → preset_index (Browse presets 0-9)
        behavior.parameterID = "preset_index";
        behavior.sensitivity = 1.0f;  // Integer steps
        break;
    case 1:  // E2 → mix_wetdry
        behavior.parameterID = "mix_wetdry";
        break;
    case 2:  // E3 → output_level
        behavior.parameterID = "output_level";
        break;
```

**Labels updated:**
- PRESET mode: "Browse", "Mix", "Output"
- MIX mode: "Input", "Mix", "Output"
- AI mode: "Complexity", "Refine", "Evolve"

### 4. Display Formatting
**File:** `pi_deployment/JUCE_Plugin/Source/HardwareDisplayComponents.h`

Added `preset_index` formatting:
```cpp
else if (paramID == "preset_index") {
    int presetNum = static_cast<int>(normalizedValue * 9.0f + 0.5f);
    return juce::String(presetNum + 1);  // Display as 1-10
}
```

---

## 🐛 Known Issues

### 1. **Preset Index Jumping Erratically**
**Symptom:** E1 encoder causes preset_index to jump: 0→1→9→1→9
**Evidence from logs:**
```
Encoder 1 event: delta=1
Preset index changed to 1
...
Encoder 1 event: delta=1
Preset index changed to 9   ← Should be 2!
```

**Likely Cause:** Integer parameter handling in `updateParameterFromEncoder()` has math error. The delta of +1 is jumping too far.

**Location:** `pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp:1790-1813`

### 2. **Encoder Buttons Not Tested**
E1/E2 button press functionality implemented but not hardware tested yet:
- E1 press → load preset
- E2 press → quick save

### 3. **Display Update Issues**
Labels updated correctly after rebuild, but preset number display needs verification.

---

## 📝 Files Changed Today

### New Files Created:
1. `pi_deployment/JUCE_Plugin/Source/GPIOPresetManager.h` (new, 340 lines)

### Files Modified:
1. `pi_deployment/JUCE_Plugin/Source/PluginProcessor.h`
   - Added GPIOPresetManager include and member
   - Added handleEncoderButtonEvent() declaration
   - Added preset file path helpers

2. `pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp`
   - Line 238-242: Added preset_index parameter
   - Line 322-331: Initialize GPIOPresetManager
   - Line 363-365: Subscribe to ENCODER_PRESS events
   - Line 433-455: Load presets/cache from JSON
   - Line 1584-1682: Implemented handleEncoderButtonEvent()
   - Line 1790-1813: Enhanced integer parameter handling (BUGGY)
   - Line 1821-1845: File path helpers

3. `pi_deployment/JUCE_Plugin/Source/ControlState.h`
   - Line 104-106: E1 → preset_index in PRESET mode
   - Line 163-165: Updated labels ("Browse", "Mix", "Output")

4. `pi_deployment/JUCE_Plugin/Source/HardwareDisplayComponents.h`
   - Line 143-147: Added preset_index number formatting

---

## 🧪 Testing Results

### What Works:
✅ **Mode switching:** SW1 changes mode (PRESET/MIX/AI)
✅ **Label updates:** Encoders show correct labels per mode
✅ **E1 parameter routing:** E1 controls preset_index in PRESET mode
✅ **GPIOPresetManager init:** Initializes with 10 empty slots
✅ **E2/E3 still work:** Mix and Output controls functional

### What's Broken:
❌ **Preset browsing:** Values jump erratically (0→1→9→1)
❌ **Button presses:** Not tested yet
❌ **Preset save/load:** Can't test until browsing works
❌ **Display numbers:** Need to verify preset numbers show correctly

---

## 🔍 Root Cause Analysis

The integer parameter handling code in `updateParameterFromEncoder()` has a bug. Current implementation:

```cpp
// AudioParameterInt handling
auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
if (intParam) {
    int currentValue = static_cast<int>(intParam->get());
    int intDelta = static_cast<int>(delta);
    int newValue = juce::jlimit(intParam->getRange().getStart(),
                                 intParam->getRange().getEnd(),
                                 currentValue + intDelta);

    float normalized = intParam->convertTo0to1(newValue);
    intParam->setValueNotifyingHost(normalized);

    if (behavior.parameterID == "preset_index" && gpioPresetManager) {
        gpioPresetManager->setCurrentPresetIndex(newValue);
    }
    return;
}
```

**Problem:** The math is converting delta incorrectly or the parameter range is wrong.

---

## 📋 Next Steps - Immediate

### Priority 1: Fix Preset Index Jumping
1. Add debug logging to `updateParameterFromEncoder()` for integer params
2. Verify `preset_index` parameter range (should be 0-9)
3. Check if `intParam->get()` returns correct current value
4. Verify delta calculation doesn't overflow

### Priority 2: Test Button Functionality
1. Verify E1 button press triggers `handleEncoderButtonEvent(0)`
2. Verify E2 button press triggers `handleEncoderButtonEvent(1)`
3. Test preset save workflow
4. Test preset load workflow

### Priority 3: Display Verification
1. Verify preset numbers display correctly (1-10)
2. Verify display updates when browsing
3. Add preset name display (currently shows "Preset 1", "Preset 2", etc.)

---

## 📅 Remaining Week 2 Work

From the Trinity GPIO Plan, Week 2 acceptance criteria:
> **Done When:** Load → tweak A → flip to B → tweak → save → reboot → everything's there

**Status:**
- ✅ A/B banks toggle (completed Week 2 Phase 1)
- 🟡 Preset browsing (infrastructure done, bugs remain)
- ⏳ Preset save (implemented, not tested)
- ⏳ Preset load (implemented, not tested)
- ⏳ JSON persistence (implemented, not tested)
- ⏳ Reboot persistence (implemented, not tested)

**Estimated time to complete Week 2:** 2-4 hours
- 1 hour: Fix preset index jumping bug
- 1 hour: Test and debug button save/load
- 30 min: Verify display formatting
- 30 min: End-to-end testing (save → reboot → load)

---

## 📅 Week 3 Preview: Engines + MODE

From the 4-week plan:

**Goal:** Engines register parameters, MODE switch changes behavior

**Ship:**
- [ ] Engines register parameters with registry
- [ ] Preset graph stores serial chain + engine IDs
- [ ] MODE switch mappings:
  - **PRESET:** E1 browse/load, E2 wet/dry, E3 output ✅ (partially done)
  - **MIX:** E1 tone macro, E2 space macro, E3 output
  - **AI:** E1/E2/E3 placeholders (labels only) ✅ (done)
- [ ] 300ms mode-latch + encoder pickup (no jumps)

**Timeline:** Nov 6-12 (2 weeks from now)

---

## 📅 Week 4: Polish + QA

**Goal:** Beta-ready stability and UX

**Ship:**
- [ ] Unsaved-dot indicator for A/B edits
- [ ] Single-level undo in RAM
- [ ] Graceful error handling on preset load
- [ ] Performance validation (CPU, no clicks on A↔B)
- [ ] Developer toggles/logging (can disable for live)

**Timeline:** Nov 13-15

---

## 🏆 Overall Progress Against 4-Week Plan

| Week | Phase | Goal | Status |
|------|-------|------|--------|
| Week 1 | - | Foundation (encoders control params) | ✅ COMPLETE |
| Week 2 | Phase 1 | A/B Banks | ✅ COMPLETE |
| Week 2 | Phase 2 | Preset System | 🟡 IN PROGRESS (70% done) |
| Week 3 | - | Engines + MODE macros | ⏳ TODO |
| Week 4 | - | Polish + QA | ⏳ TODO |

**Overall Timeline:** Slightly behind - Week 2 Phase 2 taking longer than planned

---

## 💡 Technical Learnings

### 1. File Synchronization Critical
**Issue:** Agent edited files locally, but they weren't on the Pi
**Solution:** Always rsync after local edits before building
**Command:** `rsync -av local/Source/ pi:~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/`

### 2. Make Doesn't Auto-Detect Header Changes
**Issue:** Changed ControlState.h but make said "Nothing to be done"
**Solution:** Touch a .cpp file to force rebuild
**Command:** `touch ../../Source/PluginProcessor.cpp && make`

### 3. Integer Parameter Handling Tricky
AudioParameterInt vs AudioParameterFloat have different APIs:
- `AudioParameterFloat::getValue()` → returns 0-1 normalized
- `AudioParameterInt::get()` → returns actual int value (not normalized)
- Need `convertTo0to1()` when setting int params

---

## 🔧 Debugging Recommendations

### For Preset Index Jumping:
Add this diagnostic to `updateParameterFromEncoder()`:

```cpp
if (intParam) {
    int currentValue = static_cast<int>(intParam->get());
    DBG("INT PARAM DEBUG:");
    DBG("  paramID: " << behavior.parameterID);
    DBG("  delta: " << delta);
    DBG("  current: " << currentValue);
    DBG("  range: " << intParam->getRange().getStart()
        << " to " << intParam->getRange().getEnd());
    // ... rest of code
```

### For Button Testing:
Monitor logs when pressing E1/E2 buttons:
```bash
ssh branden@192.168.68.65 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build && ./ChimeraPhoenix 2>&1 | grep -i "button\|load\|save\|preset"'
```

---

## 📁 Code Reference

### Key Implementations:

**Preset Manager:**
- `GPIOPresetManager.h:84-120` - savePreset() method
- `GPIOPresetManager.h:122-145` - loadPreset() method
- `GPIOPresetManager.h:175-223` - JSON save with atomic write
- `GPIOPresetManager.h:225-264` - JSON load with error handling

**Processor Integration:**
- `PluginProcessor.cpp:238-242` - preset_index parameter definition
- `PluginProcessor.cpp:326` - GPIOPresetManager initialization
- `PluginProcessor.cpp:1584-1682` - Button event handler (E1=load, E2=save)
- `PluginProcessor.cpp:1790-1813` - Integer parameter update (BUGGY)

**Display:**
- `HardwareDisplayComponents.h:143-147` - Preset number formatting
- `ControlState.h:163-165` - PRESET mode labels

---

## 🚨 Critical Bugs to Fix Before Week 2 Complete

### Bug #1: Preset Index Erratic Jumping (CRITICAL)
**Priority:** P0 - Blocks all preset functionality
**File:** `PluginProcessor.cpp:1790-1813`
**Fix needed:** Debug integer parameter math

### Bug #2: Button Press Event Routing (UNKNOWN)
**Priority:** P1 - Prevents save/load testing
**Status:** Implemented but not hardware tested
**File:** `PluginProcessor.cpp:1584-1682`

### Bug #3: Display Number Formatting (LOW)
**Priority:** P2 - UX issue only
**Status:** Needs visual verification
**File:** `HardwareDisplayComponents.h:143-147`

---

## 🎯 Definition of Done for Week 2

Per the 4-week plan:
> **Done When:** Load → tweak A → flip to B → tweak → save → reboot → everything's there

**Test Workflow:**
1. Boot plugin
2. Turn E1 to preset 3
3. Set parameters in Bank A
4. Switch to Bank B (SW2=UP)
5. Set different parameters in Bank B
6. Press E2 to save preset 3
7. Turn E1 to preset 5
8. Press E1 to load preset 3
9. Verify: Both A and B banks restored
10. Reboot Pi
11. Verify: Preset 3 still available
12. **PASS:** All values match pre-reboot state

**Current Capability:** Steps 1-6 blocked by preset index bug

---

## 📊 Code Statistics

**New Code Today:**
- Lines added: ~450
- Lines modified: ~100
- New files: 1 (GPIOPresetManager.h)
- Modified files: 4

**Total GPIO System (Weeks 1-2):**
- Lines of code: ~2,200
- Classes: 5 (HardwareController, EventBus, ControlState, ABStateEngine, GPIOPresetManager)
- Parameters: 4 (input_gain, mix_wetdry, output_level, preset_index)
- Modes: 3 (PRESET, MIX, AI)

---

## 🔮 Next Session Plan

### Session Goal: Complete Week 2 Phase 2

**Task 1: Fix Preset Index Bug** (60 min)
- Add debug logging to integer parameter handling
- Identify why values jump 0→1→9
- Fix the math/range issue
- Verify smooth 0→1→2→3→...→9 progression

**Task 2: Test Button Functionality** (45 min)
- Verify E1 button press detected
- Test preset load from button
- Verify E2 button press detected
- Test quick save from button
- Check JSON file creation

**Task 3: End-to-End Testing** (30 min)
- Full workflow: browse → adjust → save → load → reboot
- Verify preset persistence
- Verify preset index cache
- Screenshot verification

**Task 4: Documentation** (15 min)
- Update GPIO_OCT24_SESSION_SUMMARY.md
- Document preset system completion
- Add usage guide for presets

**Total Estimated Time:** 2.5 hours

---

## 📖 Usage Guide (Once Working)

### Browsing Presets:
1. Switch SW1 to UP (PRESET mode)
2. Turn E1 clockwise/counter-clockwise
3. Display shows "Browse" with preset number 1-10

### Saving a Preset:
1. Adjust parameters (E2=mix, E3=output)
2. Tweak both A and B banks if desired
3. Turn E1 to desired preset slot (1-10)
4. **Press E2 button** → Quick save

### Loading a Preset:
1. Turn E1 to desired preset slot
2. **Press E1 button** → Load preset
3. Both A and B banks restored
4. Active bank applied to parameters

### Preset Persistence:
- Presets saved to: `/home/branden/.config/ChimeraPhoenix/gpio_presets/presets.json`
- Last preset index cached to: `.../preset_cache.json`
- Survives reboot automatically

---

## 🏁 Conclusion

**Week 2 Phase 2 Status:** 70% complete

**What's Working:**
- ✅ 10-slot preset manager infrastructure
- ✅ E1 encoder routed to preset_index
- ✅ Mode-based label switching
- ✅ JSON persistence code ready
- ✅ Button event handlers implemented

**What Needs Fixing:**
- ❌ Preset index jumping bug (CRITICAL)
- ⏳ Button press testing
- ⏳ End-to-end save/load workflow

**Recommendation:** Fix the integer parameter bug first (likely a 15-minute fix once debugged properly), then proceed with testing. The architecture is solid - just needs the math correction.

---

**Next Session:** Fix preset index bug, test buttons, complete Week 2 Phase 2
**Estimated Completion:** 2-3 hours from current state
