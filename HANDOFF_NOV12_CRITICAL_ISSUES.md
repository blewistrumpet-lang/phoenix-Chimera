# Chimera Phoenix - Critical Issues Handoff - November 12, 2025

## 📍 Current System State

**Location:** Raspberry Pi 5 at 192.168.68.107
**SSH:** `ssh -i ~/.ssh/id_ed25519 branden@192.168.68.107`
**Branch:** `hifiberrypi`
**Last Commits:** c3e35e90 (Trinity v5), 67843cb4 (GPIO build fix)

### Running Services:
```
✅ JACK Audio (PID 2402)
   jackd -R -dalsa -dhw:sndrpihifiberry -r48000 -p512 -n3 -i2 -o2

✅ Trinity v5.0 Server (PID 130637)
   ~/phoenix-Chimera/pi_deployment/AI_Server/trinity_server_v5_20251111.py
   Port: 8000

✅ ChimeraPhoenix_Pi Plugin (PID 136884)
   ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi
   DISPLAY=:0
   145MB RAM
```

### Hardware:
- ✅ HiFiBerry DAC+ADC Pro (card 0) - working with new breakout board
- ✅ USB Microphone (card 1: USB PnP Sound Device) - recording audio
- ✅ GPIO: 3 encoders + 3 switches (Nov 3 pin remapping applied)
- ✅ Display: 800x480 touchscreen (plugin window: 800x450)

---

## 🚨 CRITICAL ISSUES

### Issue 1: Alchemist Breaking Preset Format ⚠️⚠️⚠️
**Priority:** HIGHEST - Blocking engine loading

**Error:**
```
alchemist_trinity - ERROR: float() argument must be a string or a real number, not 'dict'
```

**Location:** `AI_Server/alchemist_trinity.py`

**Problem:**
- Calculator outputs: `{"name": "param1", "value": 0.5}` (dict)
- Alchemist expects: `0.5` (float)
- Mismatch causes validation to fail
- Malformed preset returned to plugin
- Plugin can't parse it
- **Engines never load**

**Evidence:**
- Trinity generates "Echoes of You" preset ✅
- Progress shows 100% complete ✅
- But no engines appear in plugin ❌

**Fix Required:**
Update `alchemist_trinity.py` to handle dict format:
```python
# Line ~235 or similar:
# OLD:
param_value = params[i]  # Expects float

# NEW:
param_value = params[i]["value"] if isinstance(params[i], dict) else params[i]
```

---

### Issue 2: Voice Recording Only Captures Partial Speech ⚠️
**Priority:** HIGH

**Symptom:**
- User says: "warm analog tape delay"
- Whisper transcribes: "you"
- Only catching first syllable or last word

**Possible Causes:**
1. Recording duration too short (check maxRecordingSeconds in VoiceRecordButton.cpp)
2. Button release stops recording too early
3. Mic input level too low (clipping/silence)
4. Recording starts late (misses beginning)

**Location:** `pi_deployment/JUCE_Plugin/Source/VoiceRecordButton.cpp`

**Diagnostic Test:**
```bash
# Manually record 10 seconds
arecord -D hw:1,0 -f S16_LE -r 16000 -c 1 -d 10 /tmp/manual_test.wav

# Speak full phrase during recording

# Check transcription
curl -X POST http://localhost:8000/transcribe -F "audio=@/tmp/manual_test.wav"
```

**Check:**
- Line ~10: `maxRecordingSeconds = ?` (should be 10, not 3)
- Recording start/stop logic in button press/release
- Audio callback actually capturing samples

---

### Issue 3: UI Layout Broken at 800x450 ⚠️
**Priority:** HIGH

**Problems Reported:**
- Text covered by encoder/switch overlay boxes
- Switches not visible on screen at all
- Elements overlapping
- Window resolution: 800x450 (reduced from 800x480 to avoid title bar overflow)

**Location:** `pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp`

**Root Cause:**
- UI designed for 800x480
- Changed to 800x450 (line 6: `setSize(800, 450)`)
- Layout calculations still assume 480 height
- Lost 30 pixels causes overflow

**Areas to Fix:**
1. Header area positioning
2. Encoder display positioning
3. Switch display positioning
4. Preset name label
5. Status/progress labels
6. Voice button placement
7. Meter displays

**Quick Fix Option:**
```cpp
// Line 6:
setSize(800, 480);  // Restore original
setResizable(false, false);
// Then hide window title bar or go fullscreen
```

**Better Fix:**
Redesign layout proportionally for 800x450

---

### Issue 4: GUI Window Not Visible ⚠️⚠️
**Priority:** CRITICAL

**Symptoms:**
- Plugin process running (PID 136884)
- Log shows GPIO initialized, voice transcription working
- Screenshot is completely black
- `xwininfo` finds no Chimera window
- Hundreds of "200" outputs in log (suspicious)

**Possible Causes:**
1. Plugin running without editor (hasEditor() = false?)
2. Window created but not painted (stuck in initialization?)
3. Window created off-screen
4. DISPLAY=:0 incorrect (should be :1?)
5. X11 connection failing (xcb errors earlier)
6. The "200" spam suggests timer/callback going haywire

**Diagnostics Needed:**
```bash
# Check if editor was created
strings build/ChimeraPhoenix_Pi | grep "createEditor"

# Check what DISPLAY is actually showing
DISPLAY=:0 xlsclients 2>&1

# Try both displays
pkill ChimeraPhoenix_Pi
DISPLAY=:1 ./build/ChimeraPhoenix_Pi &  # Try :1 instead
```

**The "200" Pattern:**
```
200200200200200200...
```
This repeats endlessly in the log. Likely:
- Timer callback printing status codes
- Loop gone wild
- Check timerCallback() in PluginEditor_Pi.cpp

---

### Issue 5: Progress Bar Not Updating
**Priority:** MEDIUM (fix after Alchemist)

**Setup:**
- Trinity v5 writes: `/tmp/trinity_progress/gen_*.json` ✅
- Plugin polls: `/tmp/trinity_progress/gen_*.json` ✅
- IDs match correctly ✅

**But:** UI shows "Trinity generating preset" without percentage

**Possible Causes:**
1. Progress bar component not visible (overlapped?)
2. updateUIFromProgress() not being called (check onProgressUpdate callback)
3. Repaint() not working
4. Progress bar off-screen
5. The "200" spam blocking message thread

**Check:**
```cpp
// In updateUIFromProgress():
DBG("Progress update: " << percent << "%");  // Add logging
```

---

## 📂 Key File Locations

### Plugin Binary:
```
~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi
```

### Source Files:
```
~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Source/
├── PluginEditor_Pi.cpp      # UI layout, voice button, progress
├── PluginEditor_Pi.h         # FileProgressMonitor class
├── PluginProcessor.cpp       # GPIO handling, engine loading
├── HardwareController.cpp    # GPIO hardware driver
└── VoiceRecordButton.cpp     # Voice recording
```

### Trinity v5 Files:
```
~/phoenix-Chimera/pi_deployment/AI_Server/
├── trinity_server_v5_20251111.py    # Main server
├── visionary_hybrid.py              # Three-tier classification
├── calculator_gpt4o_v5.py          # GPT-4o-mini parameters
└── alchemist_trinity.py             # ⚠️ HAS BUG - breaks presets
```

### Build Configuration:
```
.jucer: JUCE_Plugin/ChimeraPhoenix_Pi.jucer
Defines: CHIMERA_PI=1, ENABLE_GPIO_HARDWARE=1
Linker: LDFLAGS="-lgpiod"
Build: make CONFIG=Release LDFLAGS='-lgpiod' -j4
```

---

## 🔬 Evidence & Logs

### Trinity Is Generating Presets:
```bash
# From /tmp/trinity_v5_fixed.log:
✅ [gen_1762961279194] Generated: Echoes of You (Tier 2)

# Progress file exists:
/tmp/trinity_progress/gen_1762961279194.json
{
  "stage": "complete",
  "percent": 100,
  "preset_name": "Echoes of You"
}
```

### Voice Transcription Working (Partially):
```bash
# From plugin log:
[PluginEditor] ✅ Transcription: you

# From Trinity log:
🎤 Transcription request: chimera_voice_1762961202773.wav
✅ Transcribed: 'you'
```

### GPIO Hardware Working:
```bash
# From plugin log:
🔧 GPIO: Initializing hardware...
🔧 GPIO: Opening gpiochip4...
📍 Switch Positions: SW1=UP SW2=MIDDLE SW3=MIDDLE
```

### Alchemist Error:
```bash
# Repeated in every preset generation:
alchemist_trinity - ERROR: float() argument must be a string or a real number, not 'dict'
```

### Mysterious "200" Spam:
```bash
# Plugin log shows hundreds of:
200200200200200200200200200200200...
```
This is abnormal and suggests runaway timer or callback.

---

## 🎯 Quick Start Commands for Next Session

### Start Everything:
```bash
ssh -i ~/.ssh/id_ed25519 branden@192.168.68.107

# 1. Stop everything
sudo systemctl stop chimera-phoenix.service
pkill -9 -f ChimeraPhoenix_Pi
pkill -9 -f trinity_server
killall -9 jackd

# 2. Start JACK
jackd -R -dalsa -dhw:sndrpihifiberry -r48000 -p512 -n3 -i2 -o2 &
sleep 3

# 3. Start Trinity v5
cd ~/phoenix-Chimera/pi_deployment/AI_Server
python3 trinity_server_v5_20251111.py > /tmp/trinity.log 2>&1 &
sleep 3

# 4. Start Plugin
cd ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile
DISPLAY=:0 ./build/ChimeraPhoenix_Pi > /tmp/chimera.log 2>&1 &
```

### Monitor Logs:
```bash
# Terminal 1: Trinity
tail -f /tmp/trinity.log | grep -E "Generate|Transcri|ERROR"

# Terminal 2: Plugin
tail -f /tmp/chimera.log | grep -v "^200$"

# Terminal 3: Progress
watch -n 0.5 'ls -lht /tmp/trinity_progress/*.json | head -3'
```

---

## 🔧 Priority Fix Order

**1. Fix Alchemist (30 min)**
- Update alchemist_trinity.py to handle dict parameters
- Test preset generation
- Verify engines load

**2. Debug GUI Rendering (30 min)**
- Find why window not visible
- Fix the "200" spam
- Get GUI displaying

**3. Fix Voice Recording (30 min)**
- Increase recording duration
- Test full phrase capture
- Verify Whisper gets complete audio

**4. UI Redesign (2-3 hours)**
- Layout for 800x450
- Make all elements visible
- Proper spacing

---

## 📋 What You Built Today (Don't Lose!)

✅ GPIO build system working
✅ Trinity v5.0 with three-tier intelligence
✅ GPT-4o-mini calculator
✅ Progress tracking with matching IDs
✅ HiFiBerry + GPIO coexisting (new breakout board)
✅ Voice → Whisper → Trinity pipeline functional
✅ 57 DSP engines compiled
✅ A/B banks, Macros, Preset system in code

**The foundation is solid.** Just need to fix:
1. Alchemist parameter format
2. GUI rendering
3. Voice recording duration
4. UI layout

---

## 📞 Quick Reference

### File Paths:
```
Mac Local: /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/
Pi Remote: ~/phoenix-Chimera/pi_deployment/
Binary: ~/phoenix-Chimera/pi_deployment/JUCE_Plugin/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi
```

### Recent Commits:
```
c3e35e90 - fix(trinity): Use plugin's request_id for progress tracking
07eb1153 - feat(trinity): Create Trinity v5.0 Ultimate Edition
67843cb4 - fix(gpio): Complete GPIO build configuration
bf12baa6 - fix(gpio): Apply Nov 3 GPIO pin remapping
60c865e2 - cleanup: Remove deprecated desktop editor files
```

### Key Documentation:
```
SESSION_SUMMARY_NOV11_2025.md - What was accomplished
ISSUES_TO_FIX_NOV12.md - Detailed issue analysis
GPIO_HARDWARE_TEST_RESULTS_NOV3_2025.md - GPIO pin mappings
PI_VOICE_SETUP.md - Voice integration guide
```

---

## 🎯 First Thing to Do Next Session

**Test if GUI is actually rendering:**
```bash
ssh -i ~/.ssh/id_ed25519 branden@192.168.68.107

# Check what you see on the physical Pi screen
# Is it:
# A) Completely black/blank
# B) Desktop with no window
# C) Chimera window but garbled/overlapped
# D) Working UI with some layout issues

# Then check plugin status:
ps aux | grep ChimeraPhoenix_Pi

# And window list:
DISPLAY=:0 wmctrl -l 2>&1
```

**If GUI not visible at all:**
→ Focus on fixing window creation/rendering

**If GUI visible but messy:**
→ Focus on fixing Alchemist, then UI layout

---

## 💾 Backup Current State

Before making changes:
```bash
# On Pi:
cd ~/phoenix-Chimera/pi_deployment
tar czf ~/chimera_backup_nov12.tar.gz JUCE_Plugin/Source/ AI_Server/

# On Mac:
cd ~/branden/Project_Chimera_v3.0_Phoenix
git add -A
git commit -m "wip: Session end Nov 12 - Trinity v5 deployed, issues documented"
git push origin hifiberrypi
```

---

## 🧪 Test Commands

### Test Voice Recording Manually:
```bash
# 10 second recording from USB mic
arecord -D hw:1,0 -f S16_LE -r 16000 -c 1 -d 10 /tmp/voice_manual.wav

# During recording, say clearly:
# "warm analog tape delay with moderate feedback"

# Check file size (should be ~300KB for 10 seconds)
ls -lh /tmp/voice_manual.wav

# Test transcription
curl -X POST http://localhost:8000/transcribe -F "audio=@/tmp/voice_manual.wav"

# Expected: Full phrase transcribed
```

### Test Trinity Preset Generation:
```bash
# Test with clear prompt
curl -s -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt":"compressor with 4 to 1 ratio", "request_id":"test_123"}' \
  > /tmp/trinity_test.json

# Check response
cat /tmp/trinity_test.json | python3 -m json.tool

# Check for Alchemist error
grep "ERROR" /tmp/trinity.log | tail -5

# Check progress
cat /tmp/trinity_progress/test_123.json
```

### Test Engine Loading:
```bash
# In plugin, manually test setSlotEngine
# Check if engines can load at all outside of Trinity

# Watch plugin log for engine loading
tail -f /tmp/chimera.log | grep -E "setSlotEngine|EngineFactory|engine.*loaded"
```

---

## 🗺️ Code Architecture Reference

### Trinity v5.0 Pipeline:
```
Voice Input (USB Mic)
    ↓
Whisper API (transcribe)
    ↓
HybridVisionary (three-tier classification)
├─ Tier 1: Literal ("4:1 compressor") → Fast, precise
├─ Tier 2: Guided ("warm delay") → Balanced
└─ Tier 3: Poetic ("ethereal shimmer") → Creative
    ↓
GPT4oCalculator (parameter optimization)
├─ Parse literals: "1/8 dotted" → 0.1875
├─ Parse percentages: "35%" → 0.35
└─ GPT-4o-mini: Musical style intelligence
    ↓
AlchemistTrinity (local validation) ⚠️ CURRENTLY BROKEN
├─ Safety rules
├─ Dangerous combinations
└─ Professional polish
    ↓
Plugin (apply preset)
├─ Load engines into slots
├─ Set 15 parameters per engine
└─ Update UI
```

### GPIO Control Flow:
```
Hardware (encoders/switches)
    ↓
HardwareController (libgpiod, 1000Hz polling)
    ↓
EventBus (thread-safe queue)
    ↓
ControlState (mode mapping)
    ↓
PluginProcessor (parameter updates)
    ↓
DSP Engines
```

---

## 🐛 Known Working vs Broken

### ✅ Confirmed Working:
- GPIO hardware reading all 6 components
- JACK audio (HiFiBerry with new breakout)
- USB mic recording audio
- Whisper API transcription (partial)
- Trinity v5 generating presets
- Progress file creation with correct IDs
- Build system (can compile and deploy)

### ❌ Confirmed Broken:
- Alchemist parameter format (causes preset corruption)
- Voice recording duration/quality (only catches partial words)
- UI layout (text overlapped, switches invisible)
- Engine loading (likely due to Alchemist error)
- GUI rendering (window not visible? or black screenshot)
- Something spamming "200" to stdout

### ❓ Unknown Status:
- Progress bar updates (can't see UI)
- A/B bank switching (can't test without GUI)
- Macro controls (can't test without GUI)
- GPIO encoder responses (can't see UI feedback)

---

## 💡 Recommendations for Next Session

**Option A: Fix Functionality First (Recommended)**
1. Fix Alchemist parameter format (critical blocker)
2. Verify engines load
3. Fix voice recording duration
4. Then worry about UI

**Option B: Fix UI First**
1. Debug why GUI not rendering
2. Fix layout for 800x450
3. Then test functionality

**Option C: Start Fresh with Known-Good UI**
- Use older PluginEditor that definitely renders
- Add GPIO features incrementally
- Avoid the current "200" spam issue

---

## 🎯 Session Goals for Tomorrow

**Minimum Success Criteria:**
- [ ] GUI visible on Pi screen
- [ ] Voice button captures full phrases
- [ ] Trinity preset loads engines into slots
- [ ] Can see parameter changes from encoders

**Stretch Goals:**
- [ ] Progress bar shows real-time updates
- [ ] UI layout clean and readable
- [ ] All switches visible
- [ ] Voice → preset → audio processing complete flow working

---

## 📊 Context for AI Assistant

**What we accomplished Nov 11:**
- Fixed GPIO build system (was broken, couldn't compile)
- Applied Nov 3 GPIO pin remapping (I2S conflicts, hardware shorts)
- Fixed HiFiBerry audio (ribbon cable seating issue)
- Created Trinity v5.0 with three-tier intelligence
- Got voice transcription working (partially)

**Current blockers:**
- Alchemist breaking preset format → engines don't load
- GUI possibly not rendering (need to verify with user what they see)
- Voice only catching partial phrases
- UI layout needs redesign for 450px height

**User is a solo developer** building commercial hardware product for investors. Priority is getting GPIO + voice working for demos.

**Development approach:**
- Edit locally on Mac in `pi_deployment/JUCE_Plugin/Source/`
- Sync with `./sync_and_build_pi.sh`
- Test on actual Pi hardware (GPIO can only work there)

---

**Last Updated:** November 12, 2025 at 10:00 AM CST
**Next Session:** Fix Alchemist first, then debug GUI rendering
**Status:** System running but functionality impaired by parameter format bug
