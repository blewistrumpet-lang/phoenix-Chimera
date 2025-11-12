# Quick Start for Next Session

## 🎯 Where You Are

**System Status:** Pi 2 (192.168.68.107)
- ✅ GPIO plugin built and running
- ✅ Trinity v5.0 server operational
- ✅ Hardware working (GPIO + HiFiBerry + USB mic)
- ❌ Alchemist bug breaking engine loading
- ❌ UI rendering unclear
- ❌ Voice catching partial words only

## 🔴 Critical Issues (Fix First)

### 1. Alchemist Parameter Format Bug
**File:** `AI_Server/alchemist_trinity.py`
**Error:** `float() argument must be a string or a real number, not 'dict'`
**Impact:** Engines don't load from Trinity presets

### 2. GUI Not Visible
**Symptom:** Screenshot is black, no X11 window found
**Check:** What do YOU see on the Pi screen right now?

### 3. Voice Recording Partial
**Symptom:** Says "warm analog tape delay" → Whisper hears "you"
**File:** `pi_deployment/JUCE_Plugin/Source/VoiceRecordButton.cpp`

## 📂 Read These First

1. **HANDOFF_NOV12_CRITICAL_ISSUES.md** ← Full details
2. **SESSION_SUMMARY_NOV11_2025.md** ← What was accomplished
3. **ISSUES_TO_FIX_NOV12.md** ← Technical analysis

## 🚀 Quick Commands

### SSH to Pi:
```bash
ssh -i ~/.ssh/id_ed25519 branden@192.168.68.107
```

### Check What's Running:
```bash
ps aux | grep -E "ChimeraPhoenix_Pi|trinity_server|jackd" | grep -v grep
```

### View Logs:
```bash
tail -f /tmp/chimera.log      # Plugin
tail -f /tmp/trinity.log      # Trinity v5
```

### Restart Everything:
```bash
# Use the commands in HANDOFF document, section "Quick Start Commands"
```

## 🎯 Next Steps

**Step 1:** Ask user what they see on Pi screen
**Step 2:** Fix Alchemist parameter format
**Step 3:** Test if engines load
**Step 4:** Fix voice recording duration
**Step 5:** UI redesign (if needed)

## 📊 Key Facts

- **GPIO pins:** 4,5,6,7,8,10,11,12,13,14,16,23,24,25,26 (Nov 3 mapping)
- **Binary:** ChimeraPhoenix_Pi (13MB)
- **Build:** `make CONFIG=Release LDFLAGS='-lgpiod' -j4`
- **Trinity:** v5.0.20251111 (three-tier, GPT-4o)
- **Breakout:** New board works with HiFiBerry

---

**Good luck! The system is 90% there - just needs debugging! 🚀**
