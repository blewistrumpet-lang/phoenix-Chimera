# Logic Pro Testing Guide for Chimera Phoenix 3.0

## How to Load the Plugin

1. **Create an Audio Track** (⌘+Option+N)
2. **Open Plugin Menu**: Click on Audio FX slot
3. **Navigate to**: Audio Units → Chimera Audio → ChimeraPhoenix
4. If not visible, try: **Logic Pro → Settings → Plug-in Manager** and rescan

## Priority Tests

### 1. Test the Fixed Engines
Load these engines from the dropdown and verify the fixes:

| Engine | What to Test | Expected Behavior |
|--------|--------------|-------------------|
| **Classic Compressor** | Threshold parameter | Should show -40dB to 0dB range |
| **Noise Gate** | Range parameter | 0 = no gating, 1 = max gating |
| **Bit Crusher** | Bits at 0 | Should bypass (no crushing) |
| **Ring Modulator** | Check for Mix control | Should have 5 parameters total |
| **Feedback Network** | Max feedback | Should not runaway/explode |

### 2. Test Musical Pitch Engines
These should now use musical intervals:

| Engine | Test | Expected |
|--------|------|----------|
| **Pitch Shifter** | Move pitch knob | Should snap to semitones |
| **Intelligent Harmonizer** | Interval parameter | Should show musical intervals (3rd, 5th, octave) |

### 3. Check Each Category
Quick test one engine from each category:

- **Dynamics**: Vintage Opto Compressor
- **EQ**: Parametric EQ  
- **Distortion**: Bit Crusher
- **Modulation**: Stereo Chorus
- **Delay**: Digital Delay
- **Reverb**: Room Reverb
- **Spatial**: Stereo Widener
- **Utility**: Gain Utility

## What to Look For

### ✅ Good Signs:
- Plugin loads without crashing
- All 57 engines appear in dropdown
- Audio passes through
- Parameters respond to automation
- No CPU spikes
- Presets recall correctly

### ⚠️ Issues to Note:
- Any engine that doesn't process audio
- Parameters showing 0-1 instead of real values
- Excessive CPU usage (check Logic's CPU meter)
- Clicking/popping sounds
- Parameters that don't work as expected

## Quick Audio Test

1. Load a simple loop or audio file
2. Insert ChimeraPhoenix
3. Try these engines in order:
   - **Gain Utility** (should just change volume)
   - **Classic Compressor** (should compress dynamics)
   - **Parametric EQ** (should change frequency balance)
   - **Stereo Chorus** (should add movement)
   - **Room Reverb** (should add space)

## Automation Test

1. Show automation (press 'A')
2. Select any parameter
3. Draw automation curve
4. Play back - parameter should follow curve smoothly

## CPU Performance Check

1. View → Show Control Bar → CPU
2. Load multiple instances
3. Expected: ~5% CPU per instance (except LinearPhaseEQ)

## Save/Recall Test

1. Set up a sound you like
2. Save as User Preset
3. Change all parameters
4. Reload preset - should restore exactly

---

## If Plugin Doesn't Appear

Try these steps:
```bash
# In Terminal:
killall -9 Logic Pro
rm -rf ~/Library/Caches/AudioUnitCache/*
killall -9 AudioComponentRegistrar

# Then restart Logic and check:
Logic Pro → Settings → Plug-in Manager → Reset & Rescan
```

## Expected Engine List (57 total)

The dropdown should show:
1. None
2. Vintage Opto Compressor
3. Classic Compressor
4. Transient Shaper
5. Noise Gate
6. Mastering Limiter
7. Dynamic EQ
8. Parametric EQ
9. Vintage Console EQ
10. Ladder Filter
... (up to 57 engines)

---

## Report Results

After testing, note:
- [ ] Plugin loads successfully
- [ ] All engines appear in list
- [ ] Audio processes correctly
- [ ] Parameters work as expected
- [ ] No crashes or hangs
- [ ] CPU usage acceptable
- [ ] Automation works
- [ ] Presets save/recall

**Test completed:** _____________________