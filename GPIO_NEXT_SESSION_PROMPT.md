# GPIO Control System - Next Session Briefing

**Date Created:** October 24, 2025
**For:** Next development session (continuing Week 2 Phase 2)
**Project:** ChimeraPhoenix v3.0 - Raspberry Pi GPIO Hardware Control

---

## 🎯 Mission Statement

You are implementing a **hardware GPIO control system** for the ChimeraPhoenix audio plugin running on a **Raspberry Pi 5**. The system uses **3 rotary encoders** and **3 three-way switches** to control audio parameters and browse/load/save presets.

This is **Week 2 Phase 2** of a **4-week pragmatic GPIO implementation plan**. You are 70% through this phase.

---

## 🖥️ Development Environment

### **Repository Structure:**
- **Local Mac:** `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/`
- **Remote Pi:** `branden@192.168.68.65:~/phoenix-Chimera/`
- **SSH Access:** `ssh branden@192.168.68.65`
- **Working Directory:** `pi_deployment/` (contains Pi-specific code)

### **Key Directories:**

```
Project_Chimera_v3.0_Phoenix/
├── pi_deployment/                    # WORK HERE - Pi-specific code
│   └── JUCE_Plugin/
│       ├── Source/                   # Edit these files
│       │   ├── PluginProcessor.cpp   # Main audio processor
│       │   ├── PluginProcessor.h     # Processor header
│       │   ├── PluginEditor_Pi.cpp   # Pi-specific editor
│       │   ├── HardwareController.cpp # GPIO hardware driver
│       │   ├── HardwareController.h
│       │   ├── ControlState.h        # Mode & encoder mapping
│       │   ├── ABStateEngine.h       # A/B bank system
│       │   ├── GPIOPresetManager.h   # 10-slot preset manager (NEW)
│       │   └── HardwareDisplayComponents.h # Encoder/switch displays
│       └── Builds/LinuxMakefile/
│           └── build/
│               └── ChimeraPhoenix    # Final executable
└── JUCE_Plugin/                      # Main desktop version - DON'T EDIT
```

### **Build Commands:**

```bash
# 1. Edit files locally on Mac in pi_deployment/JUCE_Plugin/Source/

# 2. Sync to Pi:
rsync -av /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/pi_deployment/JUCE_Plugin/Source/ \
  branden@192.168.68.65:~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/

# 3. Build on Pi:
ssh branden@192.168.68.65 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile && make -j4'

# 4. Run on Pi:
ssh branden@192.168.68.65 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build && DISPLAY=:0 ./ChimeraPhoenix 2>&1'
```

---

## 📖 Context Documents - READ THESE FIRST

### **Essential Reading (in order):**

1. **TRINITY_GPIO_4WEEK_PLAN.md** - The master plan
   - Path: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/TRINITY_GPIO_4WEEK_PLAN.md`
   - Defines the 4-week roadmap and architecture
   - Explains MODE system (PRESET/MIX/AI)
   - Defines A/B banks, preset system, macros

2. **GPIO_OCT24_SESSION_SUMMARY.md** - Week 1 & Week 2 Phase 1 completion
   - Path: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/GPIO_OCT24_SESSION_SUMMARY.md`
   - Documents completed work (encoders, A/B banks, displays)
   - Critical bugs fixed (value compounding, switch init)
   - Testing results and lessons learned

3. **GPIO_WEEK2_PHASE2_SESSION.md** - Today's work (Week 2 Phase 2)
   - Path: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/GPIO_WEEK2_PHASE2_SESSION.md`
   - Documents preset system implementation
   - Identifies critical bugs (preset index jumping)
   - Lists what's working vs what needs fixing

---

## ✅ What's Already Working

### **Week 1: Foundation** (100% Complete)
- ✅ 3 encoders controlling 3 global parameters
  - E1 → input_gain (0-2.0, displayed as dB)
  - E2 → mix_wetdry (0-1.0, displayed as %)
  - E3 → output_level (0-2.0, displayed as dB)
- ✅ Hardware GPIO driver using libgpiod
- ✅ Real-time parameter updates (1ms polling)
- ✅ Formatted displays (dB/% conversion)

### **Week 2 Phase 1: A/B Banks** (100% Complete)
- ✅ ABStateEngine class with dual banks
- ✅ SW2 toggles between Bank A (cyan) and Bank B (orange)
- ✅ Each bank stores independent parameter values
- ✅ Switch position initialization (boots to actual position)
- ✅ Color-coded visual feedback

### **Week 2 Phase 2: Preset System** (70% Complete)
- ✅ GPIOPresetManager class (10 RAM slots)
- ✅ preset_index parameter (0-9)
- ✅ E1 routed to preset browsing in PRESET mode
- ✅ E1/E2 button handlers implemented
- ✅ JSON persistence with atomic saves
- ✅ Preset index caching
- ✅ Mode-based label switching ("Browse"/"Input"/etc.)

---

## 🚨 CRITICAL BUG - Your #1 Priority

### **Bug: Preset Index Jumping Erratically**

**Location:** `pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp` lines 1790-1813

**Symptom:** When turning E1 in PRESET mode, preset_index jumps: 0→1→9→1→9 instead of 0→1→2→3...→9

**Evidence from logs:**
```
Encoder 1 event: delta=1
Preset index changed to 1      ← Correct
Encoder 1 event: delta=1
Preset index changed to 9      ← WRONG! Should be 2
Encoder 1 event: delta=1
Preset index changed to 9      ← Stuck at 9
```

**The Buggy Code:**
```cpp
void ChimeraAudioProcessor::updateParameterFromEncoder(int encoderIndex, float delta) {
    auto behavior = controlState->getEncoderBehavior(encoderIndex);
    // ... get param ...

    // Try int parameter first
    auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param);
    if (intParam) {
        int currentValue = static_cast<int>(intParam->get());
        int intDelta = static_cast<int>(delta);
        int newValue = juce::jlimit(intParam->getRange().getStart(),
                                     intParam->getRange().getEnd(),
                                     currentValue + intDelta);

        float normalized = intParam->convertTo0to1(newValue);
        intParam->setValueNotifyingHost(normalized);

        // Special handling for preset index
        if (behavior.parameterID == "preset_index" && gpioPresetManager) {
            gpioPresetManager->setCurrentPresetIndex(newValue);
        }
        return;
    }

    // ... float parameter handling ...
}
```

**Your Task:**
1. Add debug logging to see what values are being calculated
2. Verify preset_index parameter range is correct (should be 0-9)
3. Check if `intParam->get()` returns the right value
4. Fix the math so each encoder click = +1 or -1 preset number
5. Test: Turn E1 and verify smooth 1→2→3→4→5...→10→1 progression

---

## 🛠️ How to Debug This Bug

### Step 1: Add Diagnostic Logging

Insert this at the top of the `if (intParam)` block in `updateParameterFromEncoder()`:

```cpp
if (intParam) {
    int currentValue = static_cast<int>(intParam->get());
    int intDelta = static_cast<int>(delta);

    DBG("=== INT PARAM DEBUG ===");
    DBG("  paramID: " << behavior.parameterID);
    DBG("  encoder delta: " << delta);
    DBG("  int delta: " << intDelta);
    DBG("  current value: " << currentValue);
    DBG("  range: " << intParam->getRange().getStart()
        << " to " << intParam->getRange().getEnd());
    DBG("  calculated newValue: " << (currentValue + intDelta));

    // ... rest of existing code ...
```

### Step 2: Rebuild and Test

```bash
# Sync updated file to Pi
rsync -av /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp \
  branden@192.168.68.65:~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/

# Rebuild
ssh branden@192.168.68.65 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile && \
  touch ../../Source/PluginProcessor.cpp && make -j4'

# Run and watch logs
ssh branden@192.168.68.65 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build && \
  DISPLAY=:0 ./ChimeraPhoenix 2>&1 | grep -E "INT PARAM|Preset index"'
```

### Step 3: Analyze Output

Watch for patterns when turning E1:
- Is `current value` reading correctly?
- Is `delta` always +1 or -1?
- Is `newValue` correct math?
- Is the range actually 0-9?

**Expected output:**
```
=== INT PARAM DEBUG ===
  paramID: preset_index
  encoder delta: 1
  int delta: 1
  current value: 0
  range: 0 to 9
  calculated newValue: 1
Preset index changed to 1
```

---

## 📋 After Fixing the Bug - Testing Checklist

### Test 1: Preset Browsing
- [ ] Turn E1 clockwise: preset goes 1→2→3...→10
- [ ] Turn E1 counter-clockwise: preset goes 10→9→8...→1
- [ ] At preset 1, turning CCW stays at 1 (clamped)
- [ ] At preset 10, turning CW stays at 10 (clamped)

### Test 2: Preset Saving
- [ ] Set input gain to -6dB, mix to 75%, output to +3dB in Bank A
- [ ] Switch to Bank B (SW2=UP)
- [ ] Set input gain to +6dB, mix to 25%, output to 0dB in Bank B
- [ ] Turn E1 to preset 3
- [ ] **Press E2 button**
- [ ] Check logs: Should see "GPIOPresetManager: Saved preset 3"
- [ ] Check file created: `/home/branden/.config/ChimeraPhoenix/gpio_presets/presets.json`

### Test 3: Preset Loading
- [ ] Turn E1 to preset 5 (empty slot)
- [ ] **Press E1 button**
- [ ] Check logs: Should see "Preset slot 5 is empty"
- [ ] Turn E1 back to preset 3 (saved earlier)
- [ ] **Press E1 button**
- [ ] Check logs: Should see "Loaded preset 3"
- [ ] Verify Bank A values restored (-6dB, 75%, +3dB)
- [ ] Switch to Bank B
- [ ] Verify Bank B values restored (+6dB, 25%, 0dB)

### Test 4: Reboot Persistence
- [ ] Save preset 7 with distinct values
- [ ] Kill plugin: `pkill -9 ChimeraPhoenix`
- [ ] Restart plugin
- [ ] Turn E1 to preset 7
- [ ] Press E1 to load
- [ ] **Verify all values match pre-reboot state**

---

## 🔧 Hardware Configuration

### **GPIO Pin Mapping:**

```
Encoder 1:  A=GPIO5,  B=GPIO6,  Button=GPIO26
Encoder 2:  A=GPIO23, B=GPIO24, Button=GPIO25
Encoder 3:  A=GPIO17, B=GPIO27, Button=GPIO22

Switch 1:   Pin1=GPIO19, Pin2=GPIO21  (MODE: UP=PRESET, MID=MIX, DOWN=AI)
Switch 2:   Pin1=GPIO16, Pin2=GPIO20  (BANK: UP=B, MID=A, DOWN=reserved)
Switch 3:   Pin1=GPIO12, Pin2=GPIO13  (Future use)
```

### **Mode Behavior Matrix:**

| Mode | SW1 Pos | E1 Function | E1 Label | E2 Function | E2 Label | E3 Function | E3 Label |
|------|---------|-------------|----------|-------------|----------|-------------|----------|
| **PRESET** | UP | Browse presets 0-9 | "Browse" | Mix wet/dry | "Mix" | Output level | "Output" |
| **MIX** | MIDDLE | Input gain | "Input" | Mix wet/dry | "Mix" | Output level | "Output" |
| **AI** | DOWN | (Placeholder) | "Complexity" | (Placeholder) | "Refine" | (Placeholder) | "Evolve" |

### **Button Functions:**
- **E1 button** (in PRESET mode): Load selected preset
- **E2 button** (in PRESET mode): Quick save to current preset slot
- **E3 button**: Reserved for future use

---

## 📚 Architecture Overview

### **Component Hierarchy:**

```
ChimeraAudioProcessor (PluginProcessor.cpp)
├── HardwareController          # GPIO polling (1ms rate)
├── EventBus                    # Thread-safe event routing
├── ControlState                # Mode tracking & encoder mapping
├── ABStateEngine               # Dual A/B parameter banks
└── GPIOPresetManager           # 10-slot preset storage (NEW)

ChimeraAudioProcessorEditor_Pi (PluginEditor_Pi.cpp)
├── EncoderDisplay[3]           # Shows param name + formatted value
└── SwitchDisplay[3]            # Shows UP/MIDDLE/DOWN position
```

### **Event Flow:**

```
1. GPIO Pin Change → HardwareController::pollHardware()
2. Encoder/Switch State → EventBus::post(Event)
3. Event → PluginProcessor::handleEncoderEvent() / handleSwitchEvent()
4. Query ControlState::getEncoderBehavior() → Get parameterID for current mode
5. Update JUCE parameter → Audio thread picks up changes
6. Timer callback → Update EncoderDisplay with formatted value
```

---

## 📊 Current Status Snapshot

### **Completed Features:**

✅ **GPIO Hardware Driver**
- Encoder rotation detection (quadrature decoding)
- Button press detection (active-low, pull-up)
- Switch position decoding (3-way: UP/MID/DOWN)
- 1ms polling rate, thread-safe

✅ **Parameter System**
- `input_gain`: 0.0-2.0, displays as ±dB
- `mix_wetdry`: 0.0-1.0, displays as %
- `output_level`: 0.0-2.0, displays as ±dB
- `preset_index`: 0-9, displays as 1-10 (NEW)

✅ **A/B Bank System**
- Dual independent banks (A & B)
- SW2 toggles instantly
- Each bank remembers separate values
- Color-coded (cyan=A, orange=B)

✅ **Preset Infrastructure**
- GPIOPresetManager with 10 slots
- JSON persistence (atomic temp→rename)
- Preset index cache for reboots
- Button handlers wired (E1=load, E2=save)

### **Broken/Incomplete:**

❌ **Preset Index Jumping** (CRITICAL - P0)
- E1 encoder causes values to jump erratically
- Blocks all preset testing
- Bug location: `PluginProcessor.cpp:1790-1813`

⏳ **Button Functionality** (P1)
- Implemented but not tested
- Need to verify E1/E2 button presses work

⏳ **Display Numbers** (P2)
- Need visual verification that presets show 1-10

---

## 🎯 Your Immediate Tasks

### **Task 1: Fix Preset Index Bug** (60 min estimate)

**File:** `pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp`
**Method:** `updateParameterFromEncoder()` around line 1790

**Steps:**
1. Read the current implementation
2. Add diagnostic DBG() statements (see template above)
3. Sync to Pi and rebuild
4. Run plugin and turn E1 in PRESET mode
5. Analyze logs to identify the math error
6. Fix the calculation
7. Verify smooth 1→2→3...→10 progression

**Success Criteria:**
- E1 CW: preset increments by 1 each click
- E1 CCW: preset decrements by 1 each click
- Clamped at min (1) and max (10)
- No erratic jumps

### **Task 2: Test Button Save/Load** (45 min estimate)

**Test E2 Button (Quick Save):**
1. Set unique parameters in Banks A & B
2. Turn E1 to preset 5
3. Press E2 button
4. Watch logs for "GPIOPresetManager: Saved preset 5"
5. Check file: `cat ~/.config/ChimeraPhoenix/gpio_presets/presets.json`

**Test E1 Button (Load Preset):**
1. Turn E1 to preset 5
2. Press E1 button
3. Watch logs for "Loaded preset 5"
4. Verify parameters restored in both banks

**Success Criteria:**
- E2 button creates/updates JSON file
- E1 button restores exact parameter values
- Both A and B banks loaded correctly

### **Task 3: Verify Display** (15 min estimate)

**Take Screenshot:**
```bash
ssh branden@192.168.68.65 'grim /tmp/preset_test.png'
scp branden@192.168.68.65:/tmp/preset_test.png /tmp/
```

**Verify in PRESET mode:**
- E1 display shows "Browse" + preset number (1-10, not %)
- E2 display shows "Mix" + percentage
- E3 display shows "Output" + dB

---

## 📝 Code Patterns to Follow

### **Parameter Access Pattern:**
```cpp
// Get actual value (use for saving to banks/presets)
float actualValue = parameters.getRawParameterValue("input_gain")->load();

// Get normalized 0-1 value (use for JUCE host notifications)
float normalized = parameters.getParameter("input_gain")->getValue();

// Set parameter (use convertTo0to1 for actual→normalized)
auto* param = parameters.getParameter("input_gain");
param->setValueNotifyingHost(param->convertTo0to1(1.5f));
```

### **Diagnostic Logging Pattern:**
```cpp
DBG("Description: value=" << value);  // Use DBG() for all logging
```

### **File Sync Pattern:**
```bash
# Always sync files before building
rsync -av local/Source/FileToSync.cpp pi:~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/

# Force rebuild if headers changed
ssh pi 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile && \
  touch ../../Source/PluginProcessor.cpp && make -j4'
```

---

## 🗺️ Week-by-Week Roadmap

### **Week 1: Foundation** ✅ DONE
- Goal: 3 encoders controlling parameters
- Status: Complete, tested, committed

### **Week 2: Presets** 🟡 IN PROGRESS (70%)
- **Phase 1:** A/B Banks ✅ DONE
- **Phase 2:** 10-slot presets 🟡 CURRENT
  - Infrastructure: ✅ Complete
  - Bug fixing: ❌ In progress (preset index jumping)
  - Testing: ⏳ Blocked by bug

**Acceptance Criteria:**
> Load → tweak A → flip to B → tweak → save → reboot → everything's there

**ETA to Complete:** 2-3 hours (fix bug + testing)

### **Week 3: Engines + Macros** ⏳ TODO (Starts ~Nov 6)
- Engines register parameters with registry
- Preset graph stores engine chain
- MIX mode macros (tone/space)
- 300ms mode-latch + encoder pickup

### **Week 4: Polish + QA** ⏳ TODO (Nov 13-15)
- Unsaved-dot indicator
- Single-level undo
- Performance validation
- Error handling

---

## 🎓 Important Lessons from Previous Sessions

### **1. JUCE Parameter System Gotchas**
- `getRawParameterValue()` returns ACTUAL values in param's range
- `getValue()` returns 0-1 normalized
- `AudioParameterInt::get()` returns int, not normalized float
- Always use `convertTo0to1()` when setting parameters

### **2. Build System Quirks**
- Make doesn't auto-detect header-only changes
- Solution: `touch *.cpp` to force rebuild
- Always check binary timestamp after building

### **3. File Sync Critical**
- Agent edits happen locally on Mac
- Must `rsync` to Pi before building
- Pi never edits local files directly

### **4. GPIO Timing**
- Can't read pins immediately after hardware thread starts
- Must wait for `hardwareInitialized` flag
- Use `readImmediateSwitchPositions()` for init

---

## 📞 How to Get Help

### **Reference Files:**
- **4-week plan:** `TRINITY_GPIO_4WEEK_PLAN.md`
- **Week 1+2.1 summary:** `GPIO_OCT24_SESSION_SUMMARY.md`
- **Week 2.2 status:** `GPIO_WEEK2_PHASE2_SESSION.md`

### **Useful Grep Commands:**
```bash
# Find encoder handling code
ssh pi 'grep -n "updateParameterFromEncoder" ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp'

# Find parameter definitions
ssh pi 'grep -n "preset_index\|input_gain\|mix_wetdry" ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/PluginProcessor.cpp | head -20'

# Watch GPIO events live
ssh pi 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build && \
  ./ChimeraPhoenix 2>&1 | grep -E "ENC|SW|Button|Preset"'
```

### **Take Screenshots:**
```bash
ssh branden@192.168.68.65 'grim /tmp/screenshot.png'
scp branden@192.168.68.65:/tmp/screenshot.png /tmp/
```

---

## 🚀 Quick Start Checklist

**Before you begin:**
- [ ] Read TRINITY_GPIO_4WEEK_PLAN.md (understand the big picture)
- [ ] Read GPIO_OCT24_SESSION_SUMMARY.md (know what's already done)
- [ ] Read GPIO_WEEK2_PHASE2_SESSION.md (know current status)
- [ ] Verify SSH access: `ssh branden@192.168.68.65 'pwd'`

**Your first actions:**
1. Read PluginProcessor.cpp:1790-1813 (the buggy code)
2. Add debug logging as shown above
3. Sync, rebuild, test
4. Fix the bug
5. Test preset save/load workflow
6. Update GPIO_WEEK2_PHASE2_SESSION.md with results

---

## 💾 File Persistence Locations

**On Raspberry Pi:**
- Presets: `/home/branden/.config/ChimeraPhoenix/gpio_presets/presets.json`
- Preset cache: `/home/branden/.config/ChimeraPhoenix/gpio_presets/preset_cache.json`

**JSON Format:**
```json
{
  "presets": [
    {
      "name": "Preset 1",
      "valid": true,
      "bankA": {
        "input_gain": 1.2,
        "mix_wetdry": 0.75,
        "output_level": 1.5
      },
      "bankB": {
        "input_gain": 1.8,
        "mix_wetdry": 0.25,
        "output_level": 1.0
      }
    },
    // ... 9 more presets
  ]
}
```

---

## 🎯 Definition of Success

**Week 2 Phase 2 is COMPLETE when:**

1. ✅ Turn E1 in PRESET mode → preset number increments smoothly 1-10
2. ✅ Adjust parameters in Bank A
3. ✅ Switch to Bank B, adjust different parameters
4. ✅ Press E2 button → preset saves
5. ✅ Turn E1 to different preset
6. ✅ Press E1 button → preset loads (both banks restored)
7. ✅ Reboot Pi
8. ✅ Plugin remembers last preset index
9. ✅ Load saved preset → all values match

**Then:** Week 2 is DONE. Move to Week 3 (Engines + Macros).

---

## 🏆 Motivation

You're implementing a **professional hardware control system** for a **boutique audio processor**. When complete, users will have:

- **Instant A/B comparison** for tweaking sounds
- **10 preset slots** accessible via hardware encoders
- **Mode-based workflows** (preset browsing, sound mixing, AI interaction)
- **Tactile, no-menu control** - everything at your fingertips

This is **Week 2 of 4** - you're halfway there. The foundation is solid, the architecture is clean. Just need to fix one integer math bug and test the button functionality.

**You've got this.**

---

## 📞 Emergency Commands

### **Something's broken:**
```bash
# Kill everything
ssh branden@192.168.68.65 'pkill -9 ChimeraPhoenix'

# Check if GPIO is stuck
ssh branden@192.168.68.65 'lsof | grep gpiochip'

# Verify files synced
ssh branden@192.168.68.65 'ls -lh ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/GPIO*.h'
```

### **Build failed:**
```bash
# Clean build
ssh branden@192.168.68.65 'cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile && make clean && make -j4 2>&1 | tee /tmp/build.log'

# Check for errors
ssh branden@192.168.68.65 'grep -i error /tmp/build.log'
```

### **Can't access Pi:**
```bash
# Check SSH connection
ping 192.168.68.65

# Restart SSH if needed
ssh branden@192.168.68.65 'sudo systemctl restart ssh'
```

---

## 🎬 Final Words

**Start Here:**
1. Read this document completely
2. Read the three reference .md files
3. Fix the preset index bug (add logging first!)
4. Test button functionality
5. Document your results

**Core Philosophy:**
- Working beats perfect
- Ship the core, iterate on the magic
- Test on real hardware frequently

**Expected Time to Complete Week 2:** 2-3 hours from current state

Good luck! The preset system is almost there. 🚀

---

**Last Updated:** October 24, 2025
**Next Session Goal:** Fix preset index bug, complete Week 2 Phase 2
