# Week 3 Phase 1: MODE Switch Macros & BYPASS Control

**Date**: October 27, 2025
**Status**: Ready for Testing
**Branch**: `fix/phase2-preset-ab-coalesce`

---

## ✅ Completed Features

### 1. MIX Mode Macro Controls

**Three musical macros (E1/E2/E3 in MIX mode):**
- **WARMTH** (E1): Dark ← → Bright frequency shaping
- **SIZE** (E2): Tight ← → Spacious spatial effects
- **PUNCH** (E3): Soft ← → Aggressive dynamics

**Implementation:**
- `MacroParameterSystem.h` - Self-contained macro system
- Values stored at 0.5 = neutral
- Macros stored per A/B bank
- Debug output shows descriptive labels (Dark/Warm/Neutral/Bright/Brilliant)

### 2. SW3 BYPASS Control

**Three bypass modes:**
- **UP**: TRUE_BYPASS - Dry signal only (no processing)
- **MID**: PROCESS - Normal processing
- **DOWN**: KILL_DRY - 100% wet signal (no dry)

**Implementation:**
- Added to ControlState as `Bypass` enum
- Applied in processBlock() for TRUE_BYPASS
- KILL_DRY mode placeholder (needs dry signal storage)

### 3. Architecture Updates

**ControlState.h:**
- Added Bypass enum and state
- Updated MIX mode encoder mappings to macros
- Added encoder pickup flags for mode switching
- Updated labels: Warmth/Size/Punch

**PluginProcessor:**
- Integrated MacroParameterSystem
- Handle macro_ parameters in updateParameterFromEncoder()
- SW3 handling in handleSwitchEvent()
- Bypass logic in processBlock()

---

## 🧪 Testing on Pi .65

### Build & Deploy
```bash
ssh branden@192.168.68.65
cd ~/phoenix-Chimera
git fetch && git reset --hard origin/fix/phase2-preset-ab-coalesce

# Apply build fixes
cd pi_deployment/JUCE_Plugin/Source
sed -i '27d' VocalFormantFilter.cpp  # Remove SSE2
echo '// Stub' > TrinityAIClient.cpp  # Stub Trinity

# Build
cd ../Builds/LinuxMakefile
killall -9 ChimeraPhoenix
rm -rf build && make -j4

# Run
./build/ChimeraPhoenix > /tmp/week3_test.log 2>&1 &
```

### Test Checklist

#### T1: MODE Switch (SW1)
- [ ] PRESET mode: E1 browses presets
- [ ] MIX mode: E1/E2/E3 show Warmth/Size/Punch
- [ ] AI mode: Placeholders work

#### T2: Macro Controls (in MIX mode)
- [ ] E1 (WARMTH): Logs show Dark → Neutral → Bright
- [ ] E2 (SIZE): Logs show Tight → Medium → Huge
- [ ] E3 (PUNCH): Logs show Soft → Neutral → Aggressive
- [ ] Values stay at 0.5 neutral on mode switch

#### T3: BYPASS Switch (SW3)
- [ ] UP: Audio passes through unprocessed
- [ ] MID: Normal processing
- [ ] DOWN: Wet only (placeholder for now)

#### T4: A/B Banks + Macros
- [ ] Bank A: Set Warmth=0.2, Size=0.7
- [ ] Bank B: Set Warmth=0.8, Size=0.3
- [ ] Switch banks: Values restore correctly

### Monitor Logs
```bash
tail -f /tmp/week3_test.log | grep -E "\[MACRO\]|\[BYPASS\]|\[ENCODER-MACRO\]"
```

### Expected Log Patterns
```
[MACRO] WARMTH set to 0.3 (Warm)
[MACRO] SIZE set to 0.7 (Large)
[MACRO] PUNCH set to 0.6 (Punchy)
[ENCODER-MACRO] macro_warmth: 0.5 -> 0.55
[BYPASS] Bypass changed to: TRUE_BYPASS
```

---

## 📝 Next Steps

### Immediate (if tests pass):
1. **Apply macros to engines** - Actually modify engine parameters
2. **Encoder pickup** - Prevent jumps on mode switch
3. **True KILL_DRY** - Store dry signal for wet-only mode

### Week 3 Phase 2:
- Engine parameter registration
- Macro-to-parameter mapping
- Per-engine macro interpretation

### Week 4:
- Polish & stability
- Performance validation
- Final testing

---

## 🔑 Key Design Decisions

1. **Fixed global macros** instead of per-engine discovery
2. **Musical descriptors** (Warmth/Size/Punch) vs technical (Tone/Space/Energy)
3. **Simple linear offsets** for v1, curves/ranges later
4. **Macros per-bank** for A/B comparison of macro settings
5. **Bypass on SW3** for immediate utility

---

## 📚 Files Modified

- `MacroParameterSystem.h` (NEW)
- `ControlState.h`
- `PluginProcessor.h`
- `PluginProcessor.cpp`

---

**Ready for testing on Pi .65!**