# Phase 2 Quickstart Guide

**Goal**: Deploy and verify GPIO fixes on Raspberry Pi

---

## Option 1: Deploy from Pi (Recommended)

```bash
# SSH into Pi
ssh branden@192.168.68.65  # or .68

# Navigate to project
cd ~/phoenix-Chimera

# Run automated deployment
./deploy_phase2.sh

# If successful, you'll see:
# ✅ BUILD SUCCESSFUL
# To run: ./build/ChimeraPhoenix
```

---

## Option 2: Deploy from Mac

```bash
# From Mac, in project root
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix

# Deploy to Pi .65
./deploy_phase2.sh 192.168.68.65

# OR deploy to Pi .68
./deploy_phase2.sh 192.168.68.68
```

---

## Run the App

### Terminal 1: Run ChimeraPhoenix
```bash
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
./build/ChimeraPhoenix
```

### Terminal 2: Run Tests
```bash
cd ~/phoenix-Chimera
./test_phase2.sh
```

The test script will:
- Guide you through 6 verification tests (A1-A6)
- Monitor logs in real-time
- Generate a pass/fail report
- Save results to `/tmp/phase2_test_results.txt`

---

## Manual Testing (if you prefer)

### A1: Preset Stepping (E1)
1. Turn E1 slowly clockwise 10 detents
2. UI should show: 1 → 2 → 3 → ... → 10
3. **PASS**: Clean stepping, no jumps
4. **FAIL**: Jumps (especially 1 ↔ 10)

### A2: Fast Encoder Spin (E1)
1. Spin E1 as fast as possible
2. Should move ~1 slot per 33ms frame
3. **PASS**: Smooth increment, no multi-jumps
4. **FAIL**: Skips slots or jumps wildly

### A3: Continuous Encoder Smoothness (E2, E3)
1. Spin E2 (Mix) and E3 (Output) fast
2. **PASS**: Smooth, responsive
3. **FAIL**: Stuttery or laggy

### A4: A/B Bank Independence
1. Bank A: Set Mix=40%, Output=+3dB
2. Bank B: Set Mix=80%, Output=-6dB
3. Toggle SW2: A → B → A
4. **PASS**: Bank A values restored exactly
5. **FAIL**: Values reset or overwritten

### A5: Mix Per-Bank
1. Bank A: Mix=30%
2. Bank B: Mix=70%
3. Toggle SW2 multiple times
4. **PASS**: Mix differs (30% vs 70%)
5. **FAIL**: Mix same in both banks

### A6: Debounce
1. Flip SW2 rapidly 5 times
2. **PASS**: No double-triggers
3. **FAIL**: Applies same bank twice

---

## Debug Logs

### Real-time monitoring:
```bash
tail -f /tmp/chimera_debug.txt | grep -E "\[ENCODER-|\[DRAIN-|\[AB_SWITCH\]"
```

### Key log patterns:
```
[ENCODER-DISCRETE] preset_index: 0.0 -> 0.111 (slot 2/10)
[DRAIN-DISCRETE] E1 accumulated 5.0 detents, applying +1.0 step
[DRAIN-CONTINUOUS] E2 applying 12.0 detents
[AB_SWITCH] CAPTURED Bank A: input_gain=1.0 mix_wetdry=0.4 output_level=1.5
[AB_SWITCH] APPLYING Bank B: input_gain=1.0 mix_wetdry=0.8 output_level=0.5
[AB_SWITCH] Debounce: ignoring (only 5ms since last switch)
```

---

## If Tests Fail

### Capture diagnostics:
```bash
# Last 100 relevant log lines
grep -E "\[ENCODER-|\[DRAIN-|\[AB_SWITCH\]" /tmp/chimera_debug.txt | tail -100 > ~/phase2_fail_logs.txt

# Test results
cat /tmp/phase2_test_results.txt >> ~/phase2_fail_logs.txt
```

### Report to Claude:
1. Which test failed (A1, A2, etc.)
2. Exact behavior ("E1 jumped from 3 to 8 on fast spin")
3. Paste `~/phase2_fail_logs.txt` content

---

## Success Criteria

**All 6 tests pass** → Proceed to Week 3 Phase 1 (ControlState mode mapping)

**1-2 tests fail** → Minor tweaks needed

**3+ tests fail** → Re-examine implementation

---

## Next Session Prep

If verification succeeds, prepare for Week 3 by reading:
- `TRINITY_GPIO_4WEEK_PLAN.md` (Week 3 section)
- `GPIO_INTEGRATION_SUMMARY.md` (ControlState mode details)

Create a new handoff doc with:
- Which tests passed/failed
- Any unexpected behaviors
- Audio quality notes (clicks, pops, latency)
