# 🎯 ChimeraPhoenix GPIO Control - Comprehensive Implementation Plan

**Date:** October 24, 2025
**Current Status:** Week 2 Phase 2 (70% complete)
**Critical Issues:** 3 bugs identified, engine selection system needs design

---

## 🚨 CRITICAL BUGS IDENTIFIED

### Bug #1: Preset Index Integer Math Error (CRITICAL - Blocks Testing)
**Location:** `PluginProcessor.cpp:1793-1814`
**Issue:** Wrong approach for integer parameter handling

#### Current Buggy Code:
```cpp
// Line 1795-1798 - WRONG!
auto* rawValue = parameters.getRawParameterValue("preset_index");
if (rawValue) {
    // This returns normalized 0-1, not the actual int!
    int currentValue = static_cast<int>(rawValue->load() * 9.0f + 0.5f);
```

#### The Fix:
```cpp
void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    auto behavior = controlState->getEncoderBehavior(encoderIndex);
    auto* param = parameters.getParameter(behavior.parameterID);

    if (!param) return;

    // Handle AudioParameterInt specially
    auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
    if (intParam) {
        // Use the proper get() method for integer parameters!
        int currentValue = intParam->get();  // Returns actual 0-9 value
        int newValue = currentValue + static_cast<int>(delta);
        newValue = juce::jlimit(0, 9, newValue);

        DBG("Preset browse: " << currentValue << " -> " << newValue);

        // Convert to normalized for setValueNotifyingHost
        float normalized = intParam->convertTo0to1(newValue);
        intParam->setValueNotifyingHost(normalized);

        // Update preset manager
        if (gpioPresetManager) {
            gpioPresetManager->setCurrentPresetIndex(newValue);
        }
        return;
    }

    // Regular float parameter handling...
}
```

### Bug #2: Missing Integer Parameter Type Check
**Location:** `PluginProcessor.cpp:1792-1816`
**Issue:** Code tries to use getRawParameterValue for int param (wrong API)

**Fix:** Use proper dynamic_cast to AudioParameterInt and call get() method

### Bug #3: Potential Thread Safety Issue
**Location:** `handleEncoderButtonEvent()` line 1598-1604`
**Issue:** Multiple non-atomic operations during preset load

**Fix:** Consider wrapping preset operations in ScopedLock or using AudioProcessorValueTreeState transactions

---

## 🎨 ENGINE SELECTION SYSTEM DESIGN

### Current Situation:
- 6 slots, each can hold any of 57 engines
- Currently controlled via UI dropdown menus
- Need hardware control with only 3 encoders + 3 switches

### Proposed Solution: **SLOT Mode**

#### New Mode Hierarchy:
```
SW1 (MODE Switch):
├── UP: PRESET Mode (browse/load presets)
├── MID: MIX Mode (control parameters)
└── DOWN: SLOT Mode (engine selection) ← NEW!
```

#### SLOT Mode Behavior:

**Entry:** SW1 → DOWN position

**Display Layout:**
```
[SLOT 1: Compressor]  [SLOT 2: Tube]      [SLOT 3: Delay]
 ↑ E1 Focus            E2: Category        E3: Engine
```

**Control Mapping in SLOT Mode:**
- **E1:** Select slot (1-6) - turns change focus
- **E2:** Browse engine category (Dynamics/Filter/Distortion/etc)
- **E3:** Browse engines within category
- **E1 Button:** Confirm and load selected engine
- **E2 Button:** Clear current slot (set to None)
- **E3 Button:** Reserved (maybe copy/paste later)

#### Implementation Steps:

1. **Add SLOT mode to ControlState:**
```cpp
enum class Mode {
    PRESET,  // Browse/load presets
    MIX,     // Control parameters
    SLOT,    // Engine selection (was AI, now repurposed)
};
```

2. **Create Engine Browser State:**
```cpp
struct EngineSelector {
    int currentSlot = 0;        // 0-5
    int currentCategory = 0;     // 0-7
    int currentEngineInCategory = 0;

    // Helper methods
    int getCurrentEngineID() const;
    String getCategoryName() const;
    String getEngineName() const;
};
```

3. **Wire to existing setSlotEngine():**
```cpp
// On E1 button press in SLOT mode:
int engineID = engineSelector.getCurrentEngineID();
processor->setSlotEngine(currentSlot, engineID);
```

---

## 📋 COMPLETE IMPLEMENTATION PLAN

### Phase 1: Fix Critical Bugs (1 hour)
1. **Fix preset index jumping**
   - [ ] Replace getRawParameterValue with proper AudioParameterInt handling
   - [ ] Test increment/decrement behavior
   - [ ] Verify clamping at 0 and 9

2. **Test button functionality**
   - [ ] Verify E1 loads preset
   - [ ] Verify E2 saves preset
   - [ ] Check JSON file creation

3. **Add debug output**
   - [ ] Log all parameter changes
   - [ ] Log preset save/load operations
   - [ ] Log mode switches

### Phase 2: Complete Preset System (1 hour)
1. **Preset persistence**
   - [ ] Test save → reboot → load workflow
   - [ ] Verify both A/B banks saved
   - [ ] Check preset index cache

2. **Edge cases**
   - [ ] Handle empty preset slots gracefully
   - [ ] Test overwriting existing presets
   - [ ] Verify all 10 slots work

3. **Visual feedback**
   - [ ] Display preset names
   - [ ] Show saved/unsaved indicator
   - [ ] Update preset number display (1-10 not 0-9)

### Phase 3: Implement SLOT Mode (2 hours)
1. **Repurpose AI mode**
   - [ ] Rename AI → SLOT in ControlState
   - [ ] Update mode labels
   - [ ] Change SW1 DOWN behavior

2. **Engine browser logic**
   - [ ] Create category list
   - [ ] Map engines to categories
   - [ ] Implement navigation

3. **Display updates**
   - [ ] Show slot focus indicator
   - [ ] Display category/engine names
   - [ ] Visual feedback for selection

### Phase 4: Integration & Testing (1 hour)
1. **Mode transitions**
   - [ ] Test PRESET → MIX → SLOT switching
   - [ ] Verify encoder pickup (no jumps)
   - [ ] Check A/B state preserved

2. **Complete workflow test**
   - [ ] Select engine in SLOT mode
   - [ ] Adjust params in MIX mode
   - [ ] Save preset in PRESET mode
   - [ ] Reboot and verify

3. **Performance validation**
   - [ ] Check CPU usage
   - [ ] Verify no audio clicks
   - [ ] Test responsiveness

---

## 🏗️ ARCHITECTURE IMPROVEMENTS

### 1. State Management Pattern
```cpp
class GPIOStateManager {
    // Centralized state for all GPIO modes
    ControlState controlState;
    EngineSelector engineSelector;
    GPIOPresetManager presetManager;
    ABStateEngine abEngine;

    // Single source of truth for mode
    Mode getCurrentMode() const;
    void handleModeChange(Mode newMode);
};
```

### 2. Event Flow Optimization
```
GPIO Hardware → EventBus → StateManager → Parameters → Audio/Display
                              ↑
                         Single dispatch point
```

### 3. Parameter Update Strategy
- Use AudioProcessorValueTreeState transactions for atomic updates
- Implement parameter smoothing for zipper-noise prevention
- Add encoder acceleration for large value ranges

---

## 🎮 FINAL UX FLOW

### Typical User Journey:

1. **Power On**
   - Plugin loads last preset index
   - Initializes in MIX mode (SW1 middle)
   - Shows current parameters

2. **Browse Presets**
   - SW1 → UP (PRESET mode)
   - Turn E1 to browse 1-10
   - Press E1 to load

3. **Tweak Sound**
   - SW1 → MID (MIX mode)
   - E1/E2/E3 control params
   - SW2 toggles A/B comparison

4. **Change Engines**
   - SW1 → DOWN (SLOT mode)
   - E1 selects slot
   - E2/E3 browse engines
   - E1 button confirms

5. **Save Work**
   - SW1 → UP (PRESET mode)
   - Turn E1 to slot
   - Press E2 to quick save

---

## ✅ SUCCESS CRITERIA

### Week 2 Complete When:
- [ ] Preset index increments smoothly (no jumping)
- [ ] All 10 presets save/load correctly
- [ ] A/B banks work with presets
- [ ] Survives reboot with state intact

### Week 3 Goals:
- [ ] SLOT mode fully functional
- [ ] 57 engines selectable via hardware
- [ ] MODE transitions smooth
- [ ] No parameter jumps

### Beta Ready When:
- [ ] All modes working
- [ ] 100+ factory presets
- [ ] < 5% CPU usage
- [ ] Zero crashes in 24hr test

---

## 🚀 IMMEDIATE NEXT STEPS

1. **Fix the integer parameter bug** (15 min)
   - Edit PluginProcessor.cpp:1793-1814
   - Use proper AudioParameterInt::get()
   - Test on hardware

2. **Verify preset system** (30 min)
   - Test save/load with both banks
   - Check JSON persistence
   - Verify reboot survival

3. **Design review** (15 min)
   - Confirm SLOT mode approach
   - Review with stakeholder
   - Adjust if needed

4. **Begin SLOT implementation** (1 hour)
   - Update ControlState enum
   - Create EngineSelector class
   - Wire basic navigation

---

## 📊 RISK ASSESSMENT

### High Risk:
- Thread safety during preset operations
- Performance impact of engine switching
- Display update lag with multiple parameters

### Mitigation:
- Use JUCE's thread-safe mechanisms
- Defer engine creation to message thread
- Batch display updates with timer

### Fallback Options:
- Simplify to 4 categories instead of 7
- Use SW3 for additional control
- Limit to top 30 engines initially

---

## 💡 BONUS IDEAS (Post-Beta)

1. **Preset Morphing**
   - SW2 center position interpolates A↔B
   - Smooth parameter transitions
   - CPU-efficient crossfading

2. **Macro System**
   - Define parameter groups
   - Control multiple params with one encoder
   - User-definable mappings

3. **Performance Mode**
   - Lock certain parameters
   - Instant preset switching
   - MIDI learn for external control

---

## 📝 NOTES

- Current binary: 113MB (includes debug symbols)
- GPIO polling: 1ms (1000Hz)
- Display refresh: 30ms (33Hz)
- Audio buffer: 512 samples @ 48kHz
- Target CPU usage: < 5% on Pi 5

**Remember:** Ship working control first, polish later. The goal is a playable beta by November 15, not perfection.