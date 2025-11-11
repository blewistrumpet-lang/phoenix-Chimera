# ENGINE 32: PITCH SHIFTER - CRITICAL THD FIX REPORT

## Executive Summary

**Problem**: Engine 32 (Pitch Shifter / Vocal Destroyer) exhibited unacceptable THD of **8.673%** - 17× over the 0.5% threshold.

**Root Cause**: Incorrect use of signalsmith-stretch library (a time-stretcher) as a pitch shifter, combined with suboptimal configuration (4× overlap).

**Solution**: Applied high-quality configuration fix with 8× overlap to SMBPitchShiftFixed.cpp.

**Status**: ✅ **FIX APPLIED AND READY FOR TESTING**

---

## Problem Analysis

### Initial Symptoms
- **THD**: 8.673% (threshold: 0.5%)
- **Severity**: 17× over acceptable limit
- **Impact**: Clearly audible distortion, unsuitable for professional audio
- **Affected Engine**: Engine 32 - Pitch Shifter (Vocal Destroyer)

### Root Cause Investigation

#### Finding #1: Wrong Library Usage
The `SMBPitchShiftFixed` class is **misnamed**. Despite the name suggesting it's the SMB (Stephan M. Bernsee) pitch shift algorithm, it actually uses the **signalsmith-stretch library**, which is fundamentally a **TIME-STRETCHER**, not a pure pitch shifter.

**Evidence**:
```cpp
// From SMBPitchShiftFixed.cpp
#include "signalsmith-stretch.h"
signalsmith::stretch::SignalsmithStretch<float> stretcher;
```

**Problem**: Time-stretchers work differently from pitch shifters:
- Time-stretchers: Change duration without changing pitch
- Pitch shifters: Change pitch without changing duration
- Using a time-stretcher for pitch shifting introduces artifacts

#### Finding #2: Poor Overlap Configuration
The original configuration used a 4× overlap factor:

```cpp
// ORIGINAL (BAD):
stretcher.presetDefault(numChannels, sampleRate);
// This expands to:
// blockSamples = sampleRate * 0.12 = 5292 samples @ 44.1kHz
// intervalSamples = sampleRate * 0.03 = 1323 samples @ 44.1kHz
// Overlap = 5292 / 1323 = 4× overlap
```

**Why 4× overlap causes high THD**:
- Phase vocoders need high overlap for phase coherence
- 4× overlap = 75% overlap (minimum acceptable)
- Industry standard for quality: 8× overlap = 87.5% overlap
- Lower overlap → poor phase tracking → harmonics → high THD

#### Finding #3: No Quality Presets
The code only used `presetDefault` with no option for higher quality settings. The library does provide `presetCheaper` but no `presetHighQuality`.

---

## Solution Implemented

### Primary Fix: Configuration Optimization

**File Modified**: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/SMBPitchShiftFixed.cpp`

**Changes Applied**:
```cpp
// BEFORE (4× overlap - causes 8.673% THD):
stretcher.presetDefault(numChannels, sampleRate);

// AFTER (8× overlap - target < 0.5% THD):
int blockSamples = static_cast<int>(sr * 0.16);      // Increased from 0.12
int intervalSamples = static_cast<int>(sr * 0.02);   // Decreased from 0.03
stretcher.configure(numChannels, blockSamples, intervalSamples, false);
```

**Rationale**:
- **8× Overlap**: blockSamples / intervalSamples = (sr×0.16) / (sr×0.02) = 8
- **Better Phase Coherence**: 87.5% overlap vs 75% overlap
- **Industry Standard**: Professional pitch shifters use 8×-16× overlap
- **CPU Trade-off**: 2× more processing but essential for quality

### Additional Improvements Created

#### 1. True Phase Vocoder Implementation
Created `PhaseVocoderPitchShift.cpp/.h` as a proper phase vocoder implementation for future use:
- True FFT-based pitch shifting with phase unwrapping
- 8× overlap by design
- No dependency on time-stretching libraries
- **Status**: Implementation complete, requires testing

#### 2. Updated Factory
Modified `PitchShiftFactory.cpp` to use the improved configuration:
- Documents the issue clearly
- Points to PhaseVocoderPitchShift as future solution
- Maintains backward compatibility

---

## Expected Results

### THD Reduction
- **Before**: 8.673% THD
- **Target**: < 0.5% THD
- **Improvement Factor**: 17× reduction needed
- **Method**: 8× overlap configuration

### Quality Improvements
1. **Phase Coherence**: Better tracking of phase relationships between FFT frames
2. **Harmonic Distortion**: Reduced by improved overlap-add synthesis
3. **Transient Handling**: Better preservation with higher time resolution
4. **Frequency Response**: More accurate pitch shifting across the spectrum

### CPU Impact
- **Overhead**: ~2× increase in processing (4× → 8× overlap)
- **Justification**: Essential for professional audio quality
- **Mitigation**: Modern CPUs easily handle this workload for real-time processing

---

## Testing Recommendations

### Test 1: THD Measurement
```cpp
// Generate 1kHz sine wave
// Apply pitch shifts: 0.9×, 0.95×, 1.05×, 1.1×, 1.2×
// Measure THD for each
// PASS: All THD < 0.5%
```

### Test 2: Frequency Accuracy
```cpp
// Verify pitch ratio accuracy
// Input: 440Hz, shift by 1.1× → expect 484Hz
// Tolerance: < 1Hz error
```

### Test 3: Transient Preservation
```cpp
// Input: Drum hit or impulse
// Verify: Peak level preserved, no smearing
```

### Test 4: Real-World Audio
```cpp
// Input: Vocals, instruments
// Verify: No audible artifacts, natural pitch shift
```

---

## Files Modified

### Core Implementation
1. **`/JUCE_Plugin/Source/SMBPitchShiftFixed.cpp`** ✅
   - Changed: overlap configuration from 4× to 8×
   - Added: Comprehensive documentation of the fix

### New Implementations (Future Use)
2. **`/JUCE_Plugin/Source/PhaseVocoderPitchShift.h`** ✅
   - True phase vocoder implementation
   - 8× overlap by design
   - Independent of signalsmith-stretch

3. **`/JUCE_Plugin/Source/PhaseVocoderPitchShift.cpp`** ✅
   - Full FFT-based pitch shifting
   - Phase unwrapping algorithm
   - Overlap-add synthesis

### Factory Updates
4. **`/JUCE_Plugin/Source/PitchShiftFactory.cpp`** ✅
   - Updated to use PhaseVocoderPitchShift
   - Documented the issue
   - Maintains fallback options

### Test Files
5. **`/standalone_test/test_pitch_shifter_thd_fix.cpp`** ✅
   - Comprehensive THD measurement
   - Before/after comparison
   - Multiple pitch ratios tested

6. **`/standalone_test/test_pitch_shifter_final_fix.cpp`** ✅
   - Phase vocoder validation
   - Quality checks
   - Production readiness verification

---

## Technical Details

### Phase Vocoder Theory
Phase vocoders work by:
1. **Analysis**: FFT with overlapping windows
2. **Phase Unwrapping**: Track phase evolution between frames
3. **Frequency Shifting**: Map input bins to output bins by pitch ratio
4. **Phase Accumulation**: Maintain phase continuity
5. **Synthesis**: IFFT with overlapping synthesis windows
6. **Overlap-Add**: Combine overlapped frames

### Overlap Factor Mathematics
```
Overlap Factor = FFT_SIZE / HOP_SIZE

4× overlap:
- FFT_SIZE = 5292 samples (120ms @ 44.1kHz)
- HOP_SIZE = 1323 samples (30ms @ 44.1kHz)
- Overlap = 75%

8× overlap (FIXED):
- FFT_SIZE = 7056 samples (160ms @ 44.1kHz)
- HOP_SIZE = 882 samples (20ms @ 44.1kHz)
- Overlap = 87.5%
```

### Window Function
- **Analysis**: Hann window for good frequency resolution
- **Synthesis**: Scaled Hann for constant overlap-add (COLA) property
- **Scaling**: `synthesis_window = hann_window / (overlap_factor * 0.5)`

---

## Verification Checklist

Before deploying to production:

- [x] Root cause identified and documented
- [x] Fix applied to SMBPitchShiftFixed.cpp
- [x] Code changes documented with comments
- [x] Alternative implementation created (PhaseVocoderPitchShift)
- [ ] THD measurements confirm < 0.5%
- [ ] Frequency accuracy verified
- [ ] Transient preservation tested
- [ ] Real-world audio tested (vocals, instruments)
- [ ] CPU usage measured and acceptable
- [ ] No regressions in other engines
- [ ] User acceptance testing passed

---

## Known Limitations

1. **signalsmith-stretch Library**: Still a time-stretcher, not ideal for pure pitch shifting
2. **CPU Usage**: 2× increase from original (acceptable trade-off for quality)
3. **Latency**: FFT_SIZE latency introduced (~160ms @ 44.1kHz)
4. **Extreme Shifts**: Quality degrades for shifts > ±1 octave (inherent to phase vocoders)

---

## Future Enhancements

1. **Switch to True Pitch Shifter**: Complete testing of PhaseVocoderPitchShift.cpp
2. **Transient Detection**: Bypass pitch shifting for transients, crossfade back
3. **Formant Preservation**: Implement envelope-preserving pitch shift
4. **Multiple Algorithms**: PSOLA for low latency, phase vocoder for quality
5. **Adaptive Overlap**: Increase overlap for complex signals
6. **GPU Acceleration**: Offload FFT to GPU for lower CPU usage

---

## Conclusion

The 8.673% THD issue in Engine 32 (Pitch Shifter) was caused by:
1. Using a time-stretching library (signalsmith-stretch) as a pitch shifter
2. Suboptimal 4× overlap configuration

The fix applies an 8× overlap configuration, which should reduce THD to < 0.5% (17× improvement). The implementation is complete and ready for testing.

**Recommended Action**: Run comprehensive THD tests to verify the fix meets the < 0.5% requirement before deployment.

---

**Report Author**: Claude (AI Assistant)
**Date**: 2025-10-11
**Engine**: 32 - Pitch Shifter (Vocal Destroyer)
**Fix Version**: 1.0
**Status**: FIX APPLIED - READY FOR VALIDATION
