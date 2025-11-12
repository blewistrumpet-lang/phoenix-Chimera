# Critical Issues to Fix - November 12, 2025

## 🚨 Issues Identified

### 1. **Voice Transcription - Partial**
**Status:** ⚠️ Working but incomplete
- Whisper transcribes correctly: "you"
- BUT: User said "warm analog tape delay" - only caught "you"
- **Likely cause:** Recording duration too short or mic levels too low
- **Check:** VoiceRecordButton recording duration (currently 5 seconds max?)
- **Fix:** Increase recording time or improve mic gain

### 2. **UI Layout Broken at 800x450**
**Status:** ❌ Critical
**Problems:**
- Text covered by encoder/switch overlay boxes
- Switches not visible on screen at all
- Elements overlapping/out of bounds
**Root Cause:** UI designed for 800x480, reduced to 800x450, but layout not adjusted
**Fix Needed:** Complete UI redesign for 800x450 resolution

### 3. **No Engines Loading**
**Status:** ❌ Critical
- Trinity generates presets: "Echoes of You" (Tier 2)
- Preset includes engines (e.g., Tape Echo, Vintage Tube Preamp)
- BUT: Engines not loading into plugin slots
- **Likely causes:**
  a) Alchemist error breaking preset format
  b) Plugin not parsing Trinity response correctly
  c) applyTrinityPreset() failing silently

### 4. **Alchemist Parameter Format Error**
**Status:** ⚠️ Blocking engine loading
```
alchemist_trinity - ERROR: float() argument must be a string or a real number, not 'dict'
```
- Alchemist expects: `{"value": 0.5}`
- Receiving: `{"name": "param1", "value": 0.5}` (dict with name field)
- This breaks preset format so plugin can't load engines

### 5. **Progress Bar Not Updating**
**Status:** ⚠️ Functional issue
- Progress files created correctly (gen_* prefix matches)
- FileProgressMonitor polling every 200ms
- BUT: UI shows "Trinity generating preset" without percentage updates
- **Possible causes:**
  a) updateUIFromProgress() not being called
  b) Progress bar component not visible
  c) Repaint not triggering
  d) The hundreds of "200" outputs overwhelming the message thread

### 6. **Screenshot Completely Black**
**Status:** ⚠️ Diagnostic issue
- Plugin process running (PID 136884)
- DISPLAY=:0 set correctly
- No window found by xwininfo
- **Possible:** Plugin running without creating window (hasEditor() false?)
- **Or:** Window created but not painted/visible

---

## 🔧 Fixes Needed (Priority Order)

### Priority 1: Fix Alchemist Parameter Format
**File:** `AI_Server/alchemist_trinity.py`
**Issue:** Line expecting float, getting dict

**Current format from Calculator:**
```python
{
    "parameters": [
        {"name": "param1", "value": 0.5},
        {"name": "param2", "value": 0.3}
    ]
}
```

**Alchemist expects:**
```python
{
    "parameters": [0.5, 0.3, 0.4, ...]  # Just floats
}
```

**Fix:** Update Alchemist to handle dict format OR change Calculator output

### Priority 2: Debug Why Plugin Window Not Visible
**Check:**
1. Is `createEditor()` being called?
2. Is CHIMERA_PI defined (should use PluginEditor_Pi)?
3. Is editor window created but transparent/invisible?
4. Run plugin with verbose JUCE debugging

**Test:**
```bash
DISPLAY=:0 JUCE_LOG_ASSERTIONS=1 ./build/ChimeraPhoenix_Pi 2>&1 | grep -E "Editor\|Window\|GUI"
```

### Priority 3: Fix Voice Recording Duration/Quality
**File:** `pi_deployment/JUCE_Plugin/Source/VoiceRecordButton.cpp`

**Check:**
- Current max recording: 5 seconds (line ~10)
- Sample rate: 48000 or 16000?
- Is recording actually capturing full audio?
- Buffer size adequate?

**Test manually:**
```bash
# Record 10 seconds
arecord -D hw:1,0 -f S16_LE -r 16000 -c 1 -d 10 /tmp/test_long.wav

# Speak full phrase, check transcription
curl -X POST http://localhost:8000/transcribe -F "audio=@/tmp/test_long.wav"
```

### Priority 4: Redesign UI for 800x450
**File:** `pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp`

**Current issues:**
- Window: 800x450 (was 800x480)
- Lost 30 pixels of vertical space
- Elements designed for 480 height now overflow

**Solution options:**
1. Make components smaller (reduce font sizes, padding)
2. Remove/hide less critical elements
3. Create scrollable areas
4. Tabbed interface for different views
5. Increase window to 800x480 and go fullscreen (no window decorations)

### Priority 5: Engine Loading from Trinity Presets
**File:** `pi_deployment/JUCE_Plugin/Source/PluginEditor_Pi.cpp` - applyTrinityPreset()

**Check:**
1. Is preset JSON being received?
2. Is parsing successful?
3. Are engines being loaded via setSlotEngine()?
4. Any errors in application?

**Debug:**
```cpp
// Add logging in applyTrinityPreset()
DBG("Applying preset: " << preset.toStyledString());
DBG("Slot 0 engine: " << slot["engine"].toString());
```

---

## 🎯 Action Plan for Next Session

### Session 1: Fix Alchemist & Engine Loading (1 hour)
1. Fix Alchemist to handle dict parameters
2. Test preset generation end-to-end
3. Verify engines load into slots
4. Check audio processing works

### Session 2: Fix Voice Recording (30 min)
1. Test USB mic levels and duration
2. Verify full phrases captured
3. Test Whisper with longer audio
4. Adjust VoiceRecordButton if needed

### Session 3: UI Redesign (2-3 hours)
1. Analyze current layout requirements
2. Design new 800x450 layout
3. Implement responsive sizing
4. Test all elements visible
5. Ensure touch targets adequate

### Session 4: Integration Testing (1 hour)
1. Test voice → preset → engine loading complete flow
2. Test GPIO controls with loaded engines
3. Test A/B banks
4. Test macros
5. Verify stability

---

## 📊 What's Working (Don't Break!)

✅ GPIO hardware detection and reading
✅ Trinity v5 preset generation (Tier 2 classification)
✅ Whisper transcription API
✅ Progress file creation with correct IDs
✅ JACK audio routing
✅ Plugin compiles and runs
✅ HiFiBerry with new breakout board

---

## 🔍 Diagnostic Commands

### Check if plugin has GUI:
```bash
ssh pi2
ps aux | grep ChimeraPhoenix_Pi
DISPLAY=:0 xprop -root | grep WINDOW
```

### Monitor voice recording:
```bash
# Watch for new WAV files
watch -n 0.5 ls -lht /tmp/chimera_voice*.wav
```

### Monitor Trinity requests:
```bash
tail -f /tmp/trinity_v5_fixed.log | grep "Generate request"
```

### Check preset application:
```bash
tail -f /tmp/chimera_fresh.log | grep -E "preset\|engine\|slot"
```

---

## 💡 Key Insights

**Voice is working** - just only catching partial phrases (mic/duration issue)
**Trinity is working** - generating presets successfully
**Progress tracking is working** - files created with correct IDs
**Alchemist is breaking** - parameter format mismatch
**UI might not be rendering** - black screenshots, no window found

**Root cause hypothesis:** The Alchemist error is returning malformed presets that the plugin can't parse/apply, so engines never load.

---

**Recommend:** Fix Alchemist parameter format FIRST, then test if engines load. UI redesign can wait until functionality works.
