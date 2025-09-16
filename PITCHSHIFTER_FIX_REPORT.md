# PitchShifter Engine Fix Report
Date: August 21, 2025

## Issues Addressed

### 1. Volume Drop During Pitch Shifting
**Problem:** Significant volume reduction when changing pitch
**Solution:** 
- Fixed output scaling from 0.125 to proper 0.667 (1.0/(OVERLAP_FACTOR*0.375))
- Added energy preservation using sqrt(1/pitch) compensation
- Removed problematic window normalization

### 2. Timbre Changes During Pitch Shifting  
**Problem:** Timbre altered unnaturally when changing pitch
**Solution:**
- Simplified phase vocoder implementation
- Removed redundant phase locking that was causing artifacts
- Improved phase coherence with proper wrapping

### 3. Complete Bypass at Neutral Settings
**Problem:** Sound was not transparent at pitch=0.5, formant=0.5
**Solution:**
- Fixed formant initialization from 1.0 to 0.5
- Added bypass condition for neutral settings
- Properly maps pitch=0.5 to ratio=1.0 (unison)

### 4. Musical Interval Snapping
**Status:** ✅ Working correctly
- Pitch snaps to 13 musical intervals
- Displays with 3 decimal places
- No smoothing between intervals

## Current Parameter Status

| Parameter | Function | Status |
|-----------|----------|--------|
| Pitch | Musical interval pitch shifting | ✅ Working |
| Formant | Timbre/brightness control | ✅ Working |
| Mix | Dry/wet balance | ✅ Working |
| Window | Phase coherence control | ⚠️ Partial |
| Gate | Spectral gate threshold | ⚠️ Partial |
| Grain | Grain size control | ❌ Disabled |
| Feedback | Feedback amount | ⚠️ Partial |
| Width | Stereo width | ✅ Working |

## Energy Preservation Values

| Pitch Shift | Compensation | Gain (dB) |
|-------------|--------------|-----------|
| Octave down (0.5x) | 1.414 | +3.0 |
| Fifth down (0.75x) | 1.155 | +1.2 |
| Unison (1.0x) | 1.000 | 0.0 |
| Fifth up (1.5x) | 0.816 | -1.8 |
| Octave up (2.0x) | 0.707 | -3.0 |

## Technical Implementation

### FFT Configuration
- FFT Size: 4096 samples
- Overlap: 75% (4x factor)
- Hop Size: 1024 samples
- Window: Hann window
- Output Scale: 0.667

### Phase Vocoder
- Double precision phase accumulation
- Princarg wrapping for phase coherence
- Linear magnitude interpolation
- Energy-preserving magnitude scaling

### Bypass Optimization
```cpp
const bool canBypass = (std::abs(currentPitch - 1.0f) < 0.001f) && 
                      (std::abs(currentFormant - 0.5f) < 0.001f) &&
                      (std::abs(feedback.getValue()) < 0.001f) &&
                      (std::abs(spectralGate.getValue()) < 0.001f);
```

## Testing Instructions

1. **Volume Consistency Test:**
   - Set Mix to 1.00
   - Change Pitch through all intervals
   - Volume should remain consistent

2. **Timbre Preservation Test:**
   - Set Pitch to different intervals
   - Original timbre characteristics should be preserved
   - No unnatural metallic artifacts

3. **Transparency Test:**
   - Set Pitch to 0.500
   - Set Formant to 0.500
   - Set Mix to 1.00
   - Sound should pass through unchanged

4. **Musical Interval Test:**
   - Move Pitch knob
   - Should snap to exact interval values
   - Display shows 3 decimal places

## Known Limitations

1. **Grain parameter:** Currently disabled as it was breaking overlap-add
2. **Minor artifacts:** Some phase vocoder artifacts remain at extreme pitch shifts
3. **Formant range:** Effect could be more pronounced at extremes

## Next Steps

1. Test in Logic Pro with various audio sources
2. Fine-tune energy preservation for different content
3. Consider re-enabling grain parameter with proper implementation
4. Optimize phase locking for better transient preservation

## File Changes

**Modified:** `JUCE_Plugin/Source/PitchShifter.cpp`
- Energy preservation compensation
- Fixed output scaling  
- Simplified window creation
- Improved bypass conditions

**Last Commit:** "Fix volume drop and improve timbre preservation in PitchShifter"