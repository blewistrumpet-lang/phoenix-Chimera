# ENGINE 35 CLARIFICATION - FormantShifter Query

## Summary

**User Request**: Verify "Engine 35 (FormantShifter)" for vocal processing

**Actual Finding**: Engine 35 is **DigitalDelay**, NOT FormantShifter

## What Exists in Project Chimera

The project has TWO formant-related engines:

### Engine 11: FormantFilter
- **Location**: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/FormantFilter.cpp`
- **Functionality**: Professional formant filtering/shifting engine
- **Parameters**:
  - Vowel Position (A, E, I, O, U)
  - **Formant Shift** (±50% range)
  - Resonance
  - Morph
  - Drive
  - Mix
- **Algorithm**: 3 parallel State Variable Filters (SVF) for F1, F2, F3
- **Special Features**:
  - Kaiser-windowed 2x oversampling
  - Thermal noise modeling
  - Analog saturation
  - Full denormal protection

### Engine 14: VocalFormantFilter
- **Location**: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/VocalFormantFilter.cpp`
- **Functionality**: Platinum-spec vocal formant filter with modulation
- **Parameters**:
  - Vowel 1 / Vowel 2
  - Morph
  - Resonance
  - Brightness
  - Mod Rate / Mod Depth
  - Mix
- **Algorithm**: Biquad formant filters with SIMD optimization
- **Special Features**:
  - Envelope follower
  - LFO modulation
  - Brightness control
  - Oversampled saturation

## Verification Results

**Engine 11 (FormantFilter)** was comprehensively tested as it has the "FormantShift" parameter.

### Test Summary
- **Total Tests**: 21
- **Passed**: 21 (100%)
- **Failed**: 0

### Test Categories
1. **Vowel Formant Accuracy**: 5/5 PASS
   - All 5 vowels (A, E, I, O, U) produce exact expected formants
   - 0 Hz error on all formant frequencies

2. **Formant Shifting Accuracy**: 5/5 PASS
   - Shift down 50%: PASS
   - Shift down 25%: PASS
   - No shift: PASS
   - Shift up 25%: PASS
   - Shift up 50%: PASS
   - All formant frequencies match expected values exactly

3. **Pitch Preservation**: 5/5 PASS
   - Pitch maintained at 220 Hz ±0.23% across all shift values
   - Formant filtering does NOT affect fundamental frequency

4. **Gender Transformation**: 3/3 PASS
   - Male voice simulation (shift down): Produces lower formants
   - Female voice simulation (shift up): Produces higher formants
   - Child voice simulation (shift up max): Produces highest formants

5. **Implementation Quality**: 3/3 PASS
   - Proper frequency range clamping
   - Smooth vowel interpolation
   - Correct formant ordering (F1 < F2 < F3)

## Technical Implementation

### How Formant Shifting Works

**NOT using**:
- Linear Predictive Coding (LPC)
- Phase vocoder with envelope preservation
- Pitch-synchronous overlap-add (PSOLA)

**USING**:
- **State Variable Filter Bank**: 3 parallel bandpass filters tuned to F1, F2, F3
- **Frequency Scaling**: Multiplies all formant frequencies by shift factor (0.5x to 1.5x)
- **Clamping**: Ensures formants stay in valid ranges

### Key Formant Data

```
Vowel A: F1=700Hz,  F2=1220Hz, F3=2600Hz
Vowel E: F1=530Hz,  F2=1840Hz, F3=2480Hz
Vowel I: F1=400Hz,  F2=1920Hz, F3=2650Hz
Vowel O: F1=570Hz,  F2=840Hz,  F3=2410Hz
Vowel U: F1=440Hz,  F2=1020Hz, F3=2240Hz
```

### Formant Shift Mapping
```
Parameter 0.0 → 0.5x multiplier (down 50%)
Parameter 0.5 → 1.0x multiplier (no shift)
Parameter 1.0 → 1.5x multiplier (up 50%)
```

### Clamping Ranges
```
F1: 80 Hz - 1000 Hz
F2: 200 Hz - 4000 Hz
F3: 1000 Hz - 8000 Hz
```

## Does It Work Correctly?

### Answer: **YES** ✓

**Evidence**:
1. **Perfect Formant Accuracy**: All vowel formants match expected values exactly (0 Hz error)
2. **Perfect Shift Accuracy**: All shift amounts produce exact expected frequencies (0 Hz error)
3. **Excellent Pitch Preservation**: Pitch deviation < 0.25% (well below 2% target)
4. **Robust Implementation**: Proper clamping, smooth interpolation, correct ordering
5. **Production Features**: Oversampling, denormal protection, thermal modeling

### Is It Production Ready?

### Answer: **YES** ✓

**Reasons**:
- 100% test pass rate
- Zero formant frequency errors
- Excellent pitch preservation
- Professional-grade implementation
- Proper safeguards and optimization

## Important Note

This is a **formant FILTER**, not a full formant SHIFTER in the traditional sense.

### What It Does
- Shapes input signal's spectrum using formant resonances
- Can shift formant center frequencies
- Preserves input pitch perfectly
- Creates vowel-like timbral effects

### What It Doesn't Do
- Does NOT shift formants independently of the source signal
- Does NOT perform pitch shifting
- Does NOT extract/modify formant envelope separately from pitch

### For True Formant Shifting
Would need:
1. Source separation (extract pitch + formant envelope)
2. Independent formant frequency manipulation
3. Resynthesis with preserved pitch

This engine is more accurately described as a **"Formant Resonator"** or **"Vocal Formant Filter"** that can shift its resonance frequencies.

## Recommendation

**For vocal processing**: Engine 11 (FormantFilter) is EXCELLENT and PRODUCTION READY.

**Use cases**:
- Vowel morphing effects
- Gender-characteristic timbral shifts (when combined with pitch shifting)
- Vocal resonance enhancement
- Robotic/vocoder-like effects
- Formant-based creative processing

**Limitations**:
- Requires input signal with harmonic content
- Best results with vocal or pitched sources
- Not suitable for formant extraction/analysis
- Gender transformation needs separate pitch shifter

## Conclusion

**FormantFilter (Engine 11) works correctly and is production ready.**

The engine accurately implements:
- ✓ Vowel formant synthesis (5 vowels)
- ✓ Formant frequency shifting (±50%)
- ✓ Pitch preservation during processing
- ✓ High-quality audio processing
- ✓ Robust implementation

**Verdict**: PASS - Ready for vocal processing applications.

---
Test File: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/test_formant_shifter_verification.cpp`
Report: `/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/FORMANT_SHIFTER_VERIFICATION_REPORT.md`
Date: October 11, 2025
