# Testing PitchShifter and IntelligentHarmonizer in Logic Pro

## Plugin Status
✅ Plugin built successfully
✅ Installed to ~/Library/Audio/Plug-Ins/Components/ChimeraPhoenix.component

## Test Procedure

### 1. Setup in Logic Pro
1. Open Logic Pro
2. Create new project with audio track
3. Add ChimeraPhoenix as Audio Effect
4. Use Test Oscillator (440Hz sine wave) as input

### 2. Test PitchShifter (Engine ID 31)

**Expected Results After Fixes:**

| Parameter | Value | Expected Result |
|-----------|-------|----------------|
| **Pitch** | 0.0 | 2 octaves down (110 Hz) |
| **Pitch** | 0.5 | No change (440 Hz) |
| **Pitch** | 1.0 | 2 octaves up (1760 Hz) |
| **Formant** | 0.5 | No formant shift |
| **Mix** | 1.0 | 100% wet signal |
| **Window** | 0→1 | Tonal quality change |
| **Gate** | 0→1 | Spectral gating effect |
| **Grain** | 0→1 | Texture change (hop size) |
| **Feedback** | 0→1 | Echo/delay effect |
| **Width** | 0→1 | Stereo width (if stereo) |

**What Was Fixed:**
- ✅ Phase vocoder bug (all frequency bins now update)
- ✅ Formant mapping (was 1.25 at default, now 1.0)
- ✅ Feedback (separate read/write positions)
- ✅ Grain (controls hop size dynamically)
- ✅ Window (shapes analysis window)

### 3. Test IntelligentHarmonizer (Engine ID 19)

**Expected Results:**

| Parameter | Value | Expected Result |
|-----------|-------|----------------|
| **Interval** | 0.0 | Octave down |
| **Interval** | 0.09 | 5th down |
| **Interval** | 0.45 | Unison (no change) |
| **Interval** | 0.82 | 5th up |
| **Interval** | 1.0 | Octave + 5th up |
| **Key** | 0-1 | Root note selection |
| **Scale** | 0-1 | Scale type (Major, Minor, etc) |
| **Voices** | 0-1 | 1-4 harmony voices |
| **Spread** | 0-1 | Stereo spread of voices |
| **Humanize** | 0-1 | Pitch variation/vibrato |
| **Formant** | 0-1 | Formant preservation |
| **Mix** | 0-1 | Dry/wet blend |

**Implementation Notes:**
- Uses PSOLA, not phase vocoder (no FFT bug)
- Quantizes to 12 discrete musical intervals
- Generates intelligent harmonies based on scale
- Should be working (no bugs found in analysis)

### 4. Testing Checklist

**PitchShifter:**
- [ ] Pitch parameter shifts frequency correctly
- [ ] All 8 parameters have audible effect
- [ ] No clicks/pops when changing parameters
- [ ] Smooth parameter automation
- [ ] CPU usage acceptable

**IntelligentHarmonizer:**
- [ ] Interval parameter creates correct harmony
- [ ] Multiple voices work
- [ ] Scale quantization works
- [ ] Stereo spread works
- [ ] No artifacts or glitches

### 5. Known Issues to Watch For

1. **Parameter Display**: Still shows 0-1 values instead of meaningful units
2. **Other Pitch Engines**: Not yet analyzed/fixed
   - PitchCorrection (30)
   - FrequencyShifter (13)
   - RingModulator (33)
   - GranularDelay (15)

### 6. If Issues Found

Document:
- Which parameter doesn't work
- What the expected behavior should be
- Any error messages or crashes
- CPU usage if excessive

## Next Steps After Testing

1. If PitchShifter works → Apply similar fixes to other pitch engines
2. If IntelligentHarmonizer works → Move to Time/Delay group
3. If issues found → Debug specific parameter implementation
4. Continue systematic audit of all 57 engines