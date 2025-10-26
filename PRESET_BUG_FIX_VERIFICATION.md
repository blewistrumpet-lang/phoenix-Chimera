# 🔧 Preset Index Bug Fix - Verification Guide

**Date Fixed:** October 24, 2025
**File Modified:** `pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp`
**Method:** `updateParameterFromEncoder()` (lines 1785-1861)

---

## ✅ What Was Fixed

### The Bug:
- **Symptom:** Preset index jumped erratically (0→1→9→1→9) instead of incrementing smoothly
- **Root Cause:** Using wrong API for integer parameters
  - `getRawParameterValue()` returns normalized 0-1, not actual integer
  - Math conversion was incorrect: `rawValue->load() * 9.0f + 0.5f`

### The Fix:
- **Correct API:** Use `dynamic_cast<AudioParameterInt*>` and call `get()` method
- **Proper Flow:**
  1. Cast parameter to AudioParameterInt
  2. Use `intParam->get()` to get actual value (0-9)
  3. Add delta directly as integer
  4. Use `intParam->convertTo0to1()` when setting

### Key Changes:
```cpp
// OLD (BUGGY):
auto* rawValue = parameters.getRawParameterValue("preset_index");
int currentValue = static_cast<int>(rawValue->load() * 9.0f + 0.5f);

// NEW (FIXED):
auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
int currentValue = intParam->get();  // Returns actual 0-9
```

---

## 🧪 How to Verify the Fix

### Quick Test (5 minutes):

1. **Sync and Build:**
   ```bash
   chmod +x test_preset_fix.sh
   ./test_preset_fix.sh
   ```

2. **Hardware Test:**
   - Set SW1 to UP (PRESET mode)
   - Turn E1 clockwise one click at a time
   - **Expected:** Preset increments 0→1→2→3→4→5→6→7→8→9
   - **Not:** Random jumps or stuck values

3. **Debug Output:**
   You should see clean increments:
   ```
   === INT PARAM UPDATE ===
     Parameter: preset_index
     Current: 0
     Delta: 1
     New Value: 1
     Range: 0 to 9
   ```

### Complete Test (15 minutes):

1. **Increment Test:**
   - [ ] Turn E1 CW: Verify 0→1→2→3...→9
   - [ ] At 9, turn CW: Should stay at 9 (clamped)

2. **Decrement Test:**
   - [ ] Turn E1 CCW: Verify 9→8→7→6...→0
   - [ ] At 0, turn CCW: Should stay at 0 (clamped)

3. **Save/Load Test:**
   - [ ] Set to preset 3
   - [ ] Adjust parameters
   - [ ] Press E2 (save)
   - [ ] Turn to preset 5
   - [ ] Turn back to preset 3
   - [ ] Press E1 (load)
   - [ ] Verify parameters restored

4. **A/B Bank Test:**
   - [ ] In preset 3, set Bank A params
   - [ ] Switch to Bank B (SW2)
   - [ ] Set different params
   - [ ] Save preset (E2)
   - [ ] Load preset (E1)
   - [ ] Verify both banks restored

---

## 📊 Expected Behavior

### Correct Preset Browsing:
```
Turn E1 CW once  → Preset 0 to 1
Turn E1 CW once  → Preset 1 to 2
Turn E1 CW once  → Preset 2 to 3
...
Turn E1 CW once  → Preset 8 to 9
Turn E1 CW once  → Preset stays at 9 (max)

Turn E1 CCW once → Preset 9 to 8
Turn E1 CCW once → Preset 8 to 7
...
Turn E1 CCW once → Preset 1 to 0
Turn E1 CCW once → Preset stays at 0 (min)
```

### Debug Log Pattern:
```
=== INT PARAM UPDATE ===
  Parameter: preset_index
  Current: 3
  Delta: 1
  New Value: 4
  Range: 0 to 9
Preset manager index updated to: 4
```

---

## 🚨 If Still Broken

### Check These:

1. **File Actually Updated:**
   ```bash
   ssh branden@192.168.68.65 \
     'grep -n "FIXED: Use proper get()" ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp'
   ```
   Should show line 1799

2. **Binary Rebuilt:**
   ```bash
   ssh branden@192.168.68.65 \
     'ls -l ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix'
   ```
   Check timestamp is recent

3. **Parameter Range:**
   ```bash
   ssh branden@192.168.68.65 \
     'grep -n "preset_index" ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp | grep AudioParameterInt'
   ```
   Should show: `0, 9, 0` (min, max, default)

---

## 📝 What's Next

Once preset browsing works:

1. **Test Preset Save/Load:**
   - E1 button = Load preset
   - E2 button = Quick save

2. **Verify JSON Persistence:**
   ```bash
   ssh branden@192.168.68.65 \
     'cat ~/.config/ChimeraPhoenix/gpio_presets/presets.json'
   ```

3. **Test Reboot Survival:**
   - Save preset
   - Kill plugin
   - Restart
   - Load preset
   - Verify values match

---

## ✅ Success Criteria

The bug is FIXED when:
- [ ] E1 increments preset by exactly 1 per click
- [ ] No erratic jumping (0→9 or similar)
- [ ] Clamping works at 0 and 9
- [ ] Preset save/load functions work
- [ ] Display shows correct preset number (1-10)

---

## 📞 Rollback if Needed

If something went wrong, restore original:
```bash
cp BACKUP_updateParameterFromEncoder_BEFORE_FIX.cpp \
   pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp

# Then rebuild...
```

---

**Remember:** This fix enables the entire preset system. Once working, you can save/load presets, implement engine selection, and complete the GPIO control system!