# Parameter Mapping Issues Found

## Critical Discovery
The parameter mapping test reveals **significant inconsistencies** between:
1. The `getMixParameterIndex()` function in PluginProcessor.cpp
2. The actual parameter layouts in each engine
3. The expected "Mix/Wet/Dry" parameter names

## Summary of Issues

### 🔴 Critical Issues (12 engines with out-of-range indices)
These engines have mix parameter indices that exceed their actual parameter count:

| Engine ID | Engine Name | Mix Index | Param Count | Issue |
|-----------|------------|-----------|-------------|-------|
| 0 | NoneEngine | 6 | 0 | Index 6 but has 0 params |
| 9 | DynamicEQ | 6 | 4 | Index 6 but has 4 params |
| 11 | EnvelopeFilter | 7 | 6 | Index 7 but has 6 params |
| 14 | LadderFilter | 8 | 5 | Index 8 but has 5 params |
| 23 | StereoChorus | 6 | 5 | Index 6 but has 5 params |
| 24 | VintageFlanger | 7 | 7 | Index 7 but has 7 params |
| 28 | RingModulator | -1 | 4 | Should have mix but returns -1 |
| 30 | PitchShifter | 2 | 2 | Index 2 but has 2 params |
| 35 | TapeEcho | 4 | 4 | Index 4 but has 4 params |
| 39 | PlateReverb | 6 | 6 | Index 6 but has 6 params |
| 40 | SpringReverb_Platinum | 9 | 8 | Index 9 but has 8 params |
| 43 | GatedReverb | 8 | 6 | Index 8 but has 6 params |

### 🟡 Wrong Parameter Names (16 engines)
These engines have valid indices but the parameter at that index is NOT a mix parameter:

| Engine ID | Engine Name | Mix Index | Actual Param Name | Expected |
|-----------|------------|-----------|-------------------|----------|
| 1 | ClassicCompressor | 5 | Knee | Mix/Wet/Dry |
| 2 | VintageOptoCompressor_Platinum | 4 | Knee | Mix/Wet/Dry |
| 4 | NoiseGate_Platinum | 6 | SC Filter | Mix/Wet/Dry |
| 5 | TransientShaper_Platinum | 5 | Makeup | Mix/Wet/Dry |
| 7 | ParametricEQ | 8 | Band 1 Gain | Mix/Wet/Dry |
| 8 | VintageConsoleEQ | 10 | Q Character | Mix/Wet/Dry |
| 10 | AnalogPhaser | 7 | Env Release | Mix/Wet/Dry |
| 13 | FormantFilter | 7 | Stereo Width | Mix/Wet/Dry |
| 15 | VintageTubePreamp | 9 | Presence | Mix/Wet/Dry |
| 18 | BitCrusher | 6 | Dither | Mix/Wet/Dry |
| 29 | FrequencyShifter | 2 | Volume | Mix/Wet/Dry |
| 36 | BucketBrigadeDelay | 6 | Sync | Mix/Wet/Dry |
| 37 | MagneticDrumEcho | 8 | Sync | Mix/Wet/Dry |
| 41 | ConvolutionReverb | 4 | Width | Mix/Wet/Dry |

### ✅ Correctly Mapped (29 engines)
Only 29 out of 57 engines have correct mix parameter mappings

## Root Cause Analysis

### 1. getMixParameterIndex() is Wrong
The function in PluginProcessor.cpp returns hardcoded indices that don't match the actual engine implementations:

```cpp
case ENGINE_PLATE_REVERB: return 6;  // But PlateReverb only has 6 params (0-5)
case ENGINE_SPRING_REVERB: return 9; // But SpringReverb only has 8 params (0-7)
```

### 2. Parameter Layouts Changed
It appears the engines were modified over time but getMixParameterIndex() wasn't updated to match.

### 3. Mix Parameter Not Always Named "Mix"
Some engines use different names for their wet/dry control:
- "Knee" (compressors)
- "Width" (spatial effects)
- "Makeup" (dynamics)
- "Sync" (delays)

## Impact Assessment

### 🔴 High Impact
- **12 engines** will crash or behave unpredictably when trying to set mix parameter
- These engines may not process audio correctly with default parameters
- Parameter automation will fail for these engines

### 🟡 Medium Impact  
- **16 engines** have their wrong parameter modified when mix is adjusted
- This affects the user experience significantly
- Presets will sound wrong

### ✅ Low Impact
- 29 engines work correctly
- These can be used safely

## Fix Strategy

### Option 1: Fix getMixParameterIndex() (Recommended)
Update the function to return the correct indices based on actual engine parameters

### Option 2: Standardize Engines
Ensure all engines have their Mix parameter at a consistent index

### Option 3: Dynamic Discovery
Instead of hardcoding, dynamically find the parameter named "Mix" or similar

## Immediate Action Required
This is a **CRITICAL BUG** that affects 49% of all engines. The parameter mapping must be fixed before the plugin can be considered production-ready.