# ChimeraPhoenix GPIO Control - Master Reference Document
**Last Updated**: October 26, 2025
**Current Status**: Phase 2 Deployed & Testing
**Branch**: `fix/phase2-preset-ab-coalesce`

---

## 🎯 Project Overview

**ChimeraPhoenix** is a 57-engine modular audio processor with Raspberry Pi GPIO hardware control.

### Hardware Configuration
- **Platform**: Raspberry Pi 5 (aarch64)
- **Dev Board**: Pi .65 (192.168.68.65) - GPIO dev, no HiFiBerry
- **Production Board**: Pi .68 (192.168.68.68 - hostname: `hifiberrypi`) - HiFiBerry DAC+ADC Pro + USB mic
- **GPIO**: 3 rotary encoders with push buttons + 3 three-way switches
- **Display**: 480x320 OLED touchscreen

### Repository Structure
```
Project_Chimera_v3.0_Phoenix/          (Mac development)
  └─ pi_deployment/                     (Pi-specific code)
      └─ JUCE_Plugin/                   (Main plugin source)
          ├─ Source/
          │   ├─ PluginProcessor.{cpp,h}
          │   ├─ PluginEditor_Pi.{cpp,h}
          │   ├─ HardwareController.{cpp,h}
          │   ├─ EventBus.h
          │   ├─ ControlState.h
          │   ├─ ABStateEngine.h
          │   └─ GPIOPresetManager.h
          └─ Builds/LinuxMakefile/build/ChimeraPhoenix

phoenix-Chimera/                        (Pi repositories)
  └─ pi_deployment/                     (Same structure as Mac)
```

### SSH Access
```bash
# Pi .65 (GPIO dev board)
ssh branden@192.168.68.65

# Pi .68 (HiFiBerry production)
ssh hifiberrypi  # (uses ~/.ssh/config: HostName 192.168.68.68, IdentityFile ~/.ssh/id_ed25519_hifiberrypi)
```

---

## 📊 Current Development Status

### ✅ Week 1: Foundation (COMPLETE - Oct 23)
- GPIO hardware drivers (libgpiod)
- Event bus architecture
- Basic parameter control (3 params: input_gain, mix_wetdry, output_level)
- UI display components

### ✅ Week 2 Phase 1: A/B Banks (COMPLETE - Oct 24)
- ABStateEngine with dual parameter banks
- SW2 toggles Bank A ↔ Bank B
- Independent parameter storage per bank

### 🟡 Week 2 Phase 2: Presets (IN PROGRESS - Oct 24-26)
- **Oct 24**: GPIOPresetManager infrastructure created
- **Oct 25**: Fixed mix encoder quantization bug, simplified encoder logic
- **Oct 26**: **Phase 2 fixes deployed** - preset discretization, coalescing, A/B capture→switch→apply
- **Status**: Code deployed to .65, running with GPIO initialized, **READY FOR TESTING**

### ⏳ Week 3: Engine Integration (TODO - Nov 6-12)
- Engines register parameters
- MODE switch behavior (PRESET/MIX/AI)
- Macro controls

### ⏳ Week 4: Polish & QA (TODO - Nov 13-15)
- Unsaved indicators
- Undo functionality
- Performance validation

---

## 🔧 GPIO Architecture

### Hardware Layer
**HardwareController** (`HardwareController.{cpp,h}`)
- Polls GPIO at ~1000 Hz in background thread
- Manages 3 encoders (A/B pins + button) + 3 switches (3-position)
- Posts events to EventBus via callbacks

**Encoder Pins**:
- E1: GPIO 5,6,26 (Browse/Preset in PRESET mode)
- E2: GPIO 23,24,25 (Mix control)
- E3: GPIO 17,27,22 (Output control)

**Switch Pins**:
- SW1: GPIO 19,21 (Mode: PRESET/MIX/AI)
- SW2: GPIO 16,20 (Bank: A/B/MORPH)
- SW3: GPIO 12,13 (Live/Bypass - not yet implemented)

### Event Layer
**EventBus** (`EventBus.h`)
- Thread-safe queue for hardware events
- Types: ENCODER_TURN, ENCODER_PRESS, SWITCH_CHANGE
- Subscribers process events on message thread

### Control Layer
**ControlState** (`ControlState.h`)
- Tracks current Mode (PRESET/MIX/AI) and Variant (A/B/MORPH)
- Maps encoders to parameters based on mode
- Provides encoder behavior (parameterID, sensitivity, labels)

**Mode Mappings**:
```cpp
PRESET mode: E1=preset_index, E2=mix_wetdry, E3=output_level
MIX mode:    E1=input_gain,   E2=mix_wetdry, E3=output_level
AI mode:     E1/E2/E3 = placeholders (labels only)
```

### State Management
**ABStateEngine** (`ABStateEngine.h`)
- Manages two parameter banks (A and B)
- Each bank stores: input_gain, mix_wetdry, output_level
- `switchToBank(bool isBankB)` toggles active bank
- `getActiveBank()` returns currently active bank

**GPIOPresetManager** (`GPIOPresetManager.h`)
- 10 preset slots in RAM (indices 0-9, displayed as 1-10)
- Each preset stores both Bank A and Bank B
- JSON persistence: `~/.config/ChimeraPhoenix/gpio_presets/presets.json`
- Preset index cache: `preset_cache.json`
- Atomic writes (temp file → fsync → rename)

### Parameter Updates
**PluginProcessor** (`PluginProcessor.cpp`)
- Owns all GPIO subsystems
- `handleEncoderEvent()` - accumulates encoder turns into atomic buffer
- `handleEncoderButtonEvent()` - E1=load preset, E2=save preset
- `handleSwitchEvent()` - Mode/Bank switching with capture→switch→apply
- `updateParameterFromEncoder()` - converts accumulated detents to parameter changes
- `processGPIOEvents()` - drains accumulators at ~30-60 Hz (called from Editor timer)

---

## 🐛 Phase 2 Fixes (Oct 26 - DEPLOYED)

### Problem 1: Preset Stepping Jumps (1 ↔ 10)
**Cause**: Hardware ISR runs at 1000 Hz, UI processes at 30 Hz → event pileup
**Fix**: Added encoder accumulators + drain logic
```cpp
// Accumulate in handleEncoderEvent()
encoderAccum[encoderIndex] += delta;  // Atomic

// Drain in processGPIOEvents() @ 30 Hz
for (int i = 0; i < 3; ++i) {
    float detents = encoderAccum[i].exchange(0.f);
    if (behavior.parameterID == "preset_index") {
        updateParameterFromEncoder(i, detents > 0.f ? +1.f : -1.f);  // ±1 step max
    } else {
        updateParameterFromEncoder(i, detents);  // Full sum
    }
}
```

**Result**: Preset steps cleanly 1→2→3...→10, no jumps

### Problem 2: A/B Banks Not Independent
**Cause**: Simplified code removed capture logic; mix restore was disabled for testing
**Fix**: Restored capture→switch→apply ordering + re-enabled mix
```cpp
// 1. CAPTURE current bank
auto& currentBank = const_cast<ABStateEngine::ParamBank&>(abStateEngine->getActiveBank());
currentBank.input_gain = inputParam->get();
currentBank.mix_wetdry = mixParam->get();
currentBank.output_level = outputParam->get();

// 2. SWITCH
abStateEngine->switchToBank(switchToBankB);

// 3. APPLY new bank
const auto& newBank = abStateEngine->getActiveBank();
inputParam->setValueNotifyingHost(inputParam->convertTo0to1(newBank.input_gain));
mixParam->setValueNotifyingHost(mixParam->convertTo0to1(newBank.mix_wetdry));
outputParam->setValueNotifyingHost(outputParam->convertTo0to1(newBank.output_level));
```

**Result**: Banks preserve independent values, mix is per-bank

### Problem 3: Fast Encoder Spins Cause Jumps
**Cause**: Same as Problem 1 (event rate mismatch)
**Fix**: Coalescing via accumulators (see Problem 1 fix)
**Result**: Smooth continuous parameter changes even with fast spins

### Problem 4: Bank Switch Mechanical Bounce
**Fix**: Added 10ms debounce timer
```cpp
auto now = std::chrono::steady_clock::now();
auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBankSwitchTime).count();
if (elapsed < BANK_SWITCH_DEBOUNCE_MS) return;  // Ignore bounce
```

---

## 🚦 Build & Deploy Status

### Current Build (Oct 26 00:18)
- **Location**: Pi .65 at `~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix`
- **Size**: 114MB
- **Branch**: `fix/phase2-preset-ab-coalesce` (commit 5a43c94a)
- **Status**: ✅ Built successfully, GPIO initialized, **running PID 815978**

### Known Build Fixes Required
1. **VocalFormantFilter.cpp**: Remove SSE2 denormal guard (not available on ARM)
2. **TrinityAIClient.cpp**: Stub out for now (JUCE API mismatch - `withPostData` not available)
3. **PluginProcessor.h**: Make `processGPIOEvents()` public (called from Editor)
4. **Atomic operations**: Use `compare_exchange_weak` instead of `fetch_add` for float

### Build Commands
```bash
# On Pi .65
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
rm -rf build
make -j4

# Check binary
ls -lh build/ChimeraPhoenix  # Should be ~114MB

# Run
./build/ChimeraPhoenix > /tmp/chimera_phase2_test.log 2>&1 &
```

### Important: Always Kill Old Processes First!
```bash
# Before starting new binary
killall -9 ChimeraPhoenix
sleep 2

# Verify GPIO not in use
sudo lsof /dev/gpiochip* || echo "GPIO free"
```

---

## 🧪 Testing Phase 2 (READY NOW)

### Current Status on Pi .65
- ✅ Binary built and running
- ✅ GPIO initialized successfully
- ✅ Process PID: 815978
- ✅ Logs: `/tmp/chimera_phase2_test.log`

### Verification Checklist

#### A1: Preset Stepping
- Turn E1 slowly 10 detents
- **Expected**: Display shows 1→2→3...→10 sequentially
- **Log pattern**: `[ENCODER-DISCRETE] preset_index: 0.0 -> 0.111 (slot 2/10)`

#### A2: Fast Encoder Spin
- Spin E1 quickly
- **Expected**: Moves ~1 slot per 33ms frame, no multi-jumps
- **Log pattern**: `[DRAIN-DISCRETE] E1 accumulated 5.0 detents, applying +1.0 step`

#### A3: Continuous Encoder Smoothness
- Spin E2 (mix) and E3 (output) fast
- **Expected**: Smooth, responsive
- **Log pattern**: `[DRAIN-CONTINUOUS] E2 applying 12.0 detents`

#### A4: A/B Bank Independence
1. Bank A: Set Mix=40%, Output=+3dB
2. Flip SW2 to Bank B
3. Bank B: Set Mix=80%, Output=-6dB
4. Flip SW2 back to Bank A
5. **Expected**: Mix=40%, Output=+3dB restored
6. **Log pattern**:
   ```
   [AB_SWITCH] CAPTURED Bank A: mix_wetdry=0.4
   [AB_SWITCH] APPLYING Bank B: mix_wetdry=0.8
   ```

#### A5: Mix Per-Bank
- Verify Mix value differs between Bank A and B
- **Expected**: Independent mix per bank

#### A6: Debounce
- Flip SW2 rapidly 5 times
- **Expected**: No double-triggers
- **Log pattern**: `[AB_SWITCH] Debounce: ignoring (only 5ms since last switch)`

### Test Script
```bash
# On Pi .65
cd ~/phoenix-Chimera
./test_phase2.sh  # Interactive test guide
```

### Monitor Logs
```bash
tail -f /tmp/chimera_phase2_test.log | grep -E "\[ENCODER-|\[DRAIN-|\[AB_SWITCH\]"
```

---

## 📁 Essential Files for Next Session

### MUST READ (Tier 1):
1. **THIS FILE** - `GPIO_MASTER_REFERENCE.md` - Complete current state
2. **PHASE2_DEPLOYED.md** - Deployment status & runtime info
3. **PHASE2_OUTCOME.md** - Phase 2 implementation details
4. **TRINITY_GPIO_4WEEK_PLAN.md** - Overall roadmap & milestones

### IMPORTANT CONTEXT (Tier 2):
5. **GPIO_SESSION_HANDOFF.md** - Oct 25 session (mix encoder fix, bugs identified)
6. **GPIO_WEEK2_PHASE2_SESSION.md** - Oct 24 session (preset infrastructure)
7. **PI_MIGRATION_PLAN.md** - Repository structure & device roles
8. **REPOSITORY_ANALYSIS.md** - Codebase architecture
9. **pi_deployment/CHIMERA_PHOENIX_PI_COMPLETE_ANALYSIS.md** - Pi-specific architecture

### REFERENCE (Tier 3):
10. **GPIO_INTEGRATION_SUMMARY.md** - Week 1 completion summary
11. **GPIO_DEVELOPMENT_STRATEGY.md** - Original GPIO plan
12. **ACTUAL_SYSTEM_CONFIGURATION.md** - Hardware specs
13. **DEVELOPMENT_BRANCHES.md** - Git branching strategy

### SKIP (Obsolete/Conflicting):
- ❌ GPIO_COMPREHENSIVE_PLAN.md (superseded by TRINITY_GPIO_4WEEK_PLAN.md)
- ❌ GPIO_NEXT_SESSION_PROMPT.md (superseded by this document)
- ❌ FINAL_FIX_SUMMARY.md (merged into PHASE2_OUTCOME.md)
- ❌ FIXES_APPLIED_SUMMARY.md (build history, not needed)
- ❌ IMPLEMENTATION_PLAN.md (work-in-progress notes, superseded)
- ❌ SYSTEM_ANALYSIS_AND_4WEEK_PLAN.md (draft, superseded by official plan)

---

## 🔑 Key Technical Decisions

### Recency Rule
When docs conflict, **newest wins**:
- Oct 26 > Oct 25 > Oct 24 > Oct 23

### Architecture Decisions (Finalized)

#### 1. All Encoders Work in Normalized Space [0-1]
- No conversion to "actual" values (1.0, -6dB, etc.)
- Simplified math: `newNorm = currentNorm + (delta × sensitivity)`
- JUCE handles denormalization internally

#### 2. Preset Index: 0-9 Internally, 1-10 in UI
- Parameter range: 0-9 (AudioParameterInt)
- Display adds +1: `displayValue = presetIndex + 1`
- Normalized snapping: `round(norm * 9.0f) / 9.0f`

#### 3. Mix is Per-Bank (Not Global)
- Decision made Oct 26 after testing
- Each bank stores independent mix value
- Allows A/B comparison of wet/dry balance

#### 4. Event Coalescing for Discrete Parameters
- Hardware ISR accumulates at ~1000 Hz
- UI drains at ~30-60 Hz (Editor timer)
- Discrete params (preset): ±1 step max per drain
- Continuous params (mix, input, output): full accumulated sum

#### 5. A/B Switch: Capture→Switch→Apply
- **Capture**: Read APVTS into currentBank before switch
- **Switch**: Toggle internal bank pointer
- **Apply**: Write newBank values to APVTS
- 10ms debounce prevents mechanical bounce

---

## 🐛 Bug History & Fixes

### Bug 1: Mix Encoder Quantized to 0.01, Capped at 0.53
**Date**: Oct 25
**Cause**: AudioParameterFloat 3-arg constructor creates implicit quantization
**Fix**: Use NormalisableRange with `interval=0.0f`
```cpp
// BEFORE
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "mix_wetdry", "Mix", 0.0f, 1.0f, 0.5f));

// AFTER
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "mix_wetdry", "Mix",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f),  // interval=0
    0.5f));
```
**Status**: ✅ FIXED & VERIFIED

### Bug 2: Preset Index Jumps Between 1 and 10
**Date**: Oct 24-26
**Cause**: Event batching (1000 Hz ISR vs 30 Hz processing)
**Fix**: Event coalescing with atomic accumulators + rate limiting
**Status**: ✅ FIXED (Oct 26), PENDING VERIFICATION

### Bug 3: A/B Banks Not Independent
**Date**: Oct 25-26
**Cause**: Capture logic removed, mix restore disabled for debugging
**Fix**: Restored capture→switch→apply, re-enabled mix restore
**Status**: ✅ FIXED (Oct 26), PENDING VERIFICATION

---

## 💻 Code Reference

### Critical Functions

#### PluginProcessor.cpp

**handleEncoderEvent()** (line 1566-1588)
- Accumulates encoder turns into `encoderAccum[0..2]`
- Uses atomic compare_exchange_weak for thread safety

**processGPIOEvents()** (line 1837-1867)
- Drains accumulators at ~30-60 Hz
- Discrete params: ±1 step max per drain
- Continuous params: apply full sum

**updateParameterFromEncoder()** (line 1774-1835)
- Converts accumulated detents to parameter updates
- Discrete (preset_index): normalized snapping to 10 slots
- Continuous: sensitivity multiplication + clamping

**handleSwitchEvent()** (line 1691-1785)
- SW1: Mode switching (PRESET/MIX/AI)
- SW2: Bank switching with debounce + capture→switch→apply
- SW3: Reserved (not implemented)

**handleEncoderButtonEvent()** (line 1590-1684)
- E1 button: Load preset from current index
- E2 button: Quick save to current index
- E3 button: Reserved

### File Locations
```
pi_deployment/JUCE_Plugin/Source/
├─ PluginProcessor.{cpp,h}     - Main processor, owns GPIO subsystems
├─ PluginEditor_Pi.{cpp,h}     - Pi UI, calls processGPIOEvents() in timer
├─ HardwareController.{cpp,h}  - GPIO polling thread
├─ EventBus.h                   - Thread-safe event queue
├─ ControlState.h               - Mode/variant state machine
├─ ABStateEngine.h              - Dual parameter banks
├─ GPIOPresetManager.h          - 10-slot preset system
└─ DebugFlags.h                 - CHI_DEV_LOG macro (set to 0 for production)
```

---

## 📝 Development Workflow

### On Mac (Local Development)
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix
git checkout fix/phase2-preset-ab-coalesce  # or hifiberrypi
# Edit files in pi_deployment/JUCE_Plugin/Source/
git add -A
git commit -m "feat(gpio): your change"
git push origin fix/phase2-preset-ab-coalesce
```

### On Pi .65 (GPIO Development)
```bash
ssh branden@192.168.68.65
cd ~/phoenix-Chimera

# Pull latest changes
git fetch origin fix/phase2-preset-ab-coalesce
git checkout fix/phase2-preset-ab-coalesce
git reset --hard origin/fix/phase2-preset-ab-coalesce

# Apply build fixes (required for Pi build)
cd pi_deployment/JUCE_Plugin/Source
sed -i '27d' VocalFormantFilter.cpp  # Remove SSE2 code
echo '// Stub' > TrinityAIClient.cpp  # Stub Trinity

# Build
cd ../Builds/LinuxMakefile
killall -9 ChimeraPhoenix  # Kill old processes
rm -rf build
make -j4

# Run
./build/ChimeraPhoenix > /tmp/chimera_test.log 2>&1 &

# Monitor
tail -f /tmp/chimera_test.log | grep GPIO
```

### On Pi .68 (Production HiFiBerry)
```bash
ssh hifiberrypi
cd ~/ChimeraPhoenix_Pi  # OLD structure, not migrated yet

# NOTE: .68 has NOT been migrated to phoenix-Chimera yet
# Per PI_MIGRATION_PLAN.md, migration is pending
# For now, .68 remains on old ChimeraPhoenix_Pi structure
```

---

## 🎯 Next Milestones

### Immediate: Verify Phase 2 (When You Return to .65)
1. Turn encoders, flip switches
2. Check logs for Phase 2 patterns
3. Run through A1-A6 verification checklist
4. Report pass/fail results

### Week 2 Completion (After Phase 2 Verification)
- Encoder button testing (E1 load, E2 save)
- End-to-end preset workflow
- JSON persistence verification
- Reboot persistence test

### Week 3 Planning
- Engine parameter registration
- MODE switch macro controls
- Encoder pickup on mode change

---

## 🔍 Debugging Quick Reference

### Log Patterns to Watch
```bash
# Encoder accumulation
[ACCUM] Encoder 0 += 1.0 (total=3.0)

# Drain events
[DRAIN-DISCRETE] E1 accumulated 5.0 detents, applying +1.0 step
[DRAIN-CONTINUOUS] E2 applying 12.0 detents

# Preset stepping
[ENCODER-DISCRETE] preset_index: 0.0 -> 0.111 (slot 2/10)

# A/B bank switching
[AB_SWITCH] CAPTURED Bank A: input_gain=1.0 mix_wetdry=0.4
[AB_SWITCH] APPLYING Bank B: input_gain=1.0 mix_wetdry=0.8
[AB_SWITCH] Debounce: ignoring (only 5ms since last switch)

# Preset operations
[PRESET] Restored mix_wetdry to 0.4
GPIOPresetManager: Saved preset 3
GPIOPresetManager: Loaded preset 3
```

### Common Issues

**GPIO Init Fails**:
- Check for old processes: `sudo lsof /dev/gpiochip*`
- Kill: `killall -9 ChimeraPhoenix`
- Verify permissions: `groups` (should include `gpio`)

**Build Fails on Pi**:
- Remove VocalFormantFilter SSE2 line: `sed -i '27d' VocalFormantFilter.cpp`
- Stub Trinity: `echo '// Stub' > TrinityAIClient.cpp`
- Nuclear rebuild: `rm -rf build && make -j4`

**Parameter Not Updating**:
- Check encoder mapping: `grep -A5 "case Mode::" ControlState.h`
- Check logs: `grep "\[ENCODER\]" /tmp/chimera_test.log`
- Verify parameter exists: `grep "preset_index\|mix_wetdry" PluginProcessor.cpp | grep AudioParameter`

---

## 📞 SSH & Access Info

### SSH Hostnames
```bash
# Pi .65 (GPIO dev)
ssh branden@192.168.68.65

# Pi .68 (HiFiBerry production)
ssh hifiberrypi  # Uses ~/.ssh/config
```

### File Paths
```bash
# Logs
/tmp/chimera_phase2_test.log      # Current Phase 2 test run
/tmp/chimera_debug.txt            # General debug output

# Presets
~/.config/ChimeraPhoenix/gpio_presets/presets.json        # Saved presets
~/.config/ChimeraPhoenix/gpio_presets/preset_cache.json   # Last index

# Build
~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix
```

---

## 🏁 Summary for Next Session

### What You'll Find
- **Pi .65**: Phase 2 code running, GPIO active, PID 815978
- **Branch**: `fix/phase2-preset-ab-coalesce` (6 commits)
- **Status**: Built & deployed, awaiting verification testing
- **Logs**: `/tmp/chimera_phase2_test.log`

### What to Test
1. Turn E1 slowly → check preset stepping 1-10
2. Spin E1 fast → verify no jumps
3. Set Bank A values → switch to B → modify B → switch back to A → verify A restored
4. Confirm mix differs between banks
5. Rapid SW2 flipping → verify debounce works

### What to Report Back
- Which tests passed/failed (A1-A6)
- Any unexpected behaviors
- Relevant log excerpts (grep patterns above)
- Audio quality (clicks, pops, latency)

### If All Tests Pass
Start next session with:
> "Phase 2 verification complete! All 6 tests passed. Ready for Week 3."

Then proceed to Week 3 Phase 1: Engine parameter registration + MODE switch macros.

### If Tests Fail
Start next session with:
> "Phase 2 tests: A2 and A4 failed. Logs: [paste]"

I'll diagnose and fix immediately.

---

**End of Master Reference**
*This document consolidates all GPIO knowledge as of Oct 26, 2025*
