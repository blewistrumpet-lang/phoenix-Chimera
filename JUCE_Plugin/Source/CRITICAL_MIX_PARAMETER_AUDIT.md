# CRITICAL: Mix Parameter Index Audit

## Problem Found
Many engines are not processing audio because their Mix parameter is incorrectly mapped. The plugin's `getMixParameterIndex()` returns wrong indices that don't match the actual engine definitions.

## Engines Verified and Fixed

### ✅ TransientShaper_Platinum
- **Actual Mix Index**: 9 (from enum in .h file)
- **getMixParameterIndex returned**: 6 (WRONG)
- **Status**: FIXED - Now returns 9
- **Default set**: YES - Set to 1.0 in applyDefaultParameters

## Engines To Check

### Engine 17: HarmonicExciter_Platinum
- Has early return: `if (cache.mixAmt < 0.001f) return;`
- Need to verify actual Mix parameter index
- Need to ensure getMixParameterIndex returns correct value

### All Other Engines
Need to systematically check each engine:
1. Find actual Mix/Wet/Dry parameter index from .h file
2. Verify getMixParameterIndex returns correct value
3. Ensure applyDefaultParameters sets Mix to appropriate value (usually 1.0)
4. Check for early returns based on mix value

## How to Fix Each Engine

1. Check the engine's .h file for parameter enum:
```cpp
enum ParamID : int {
    // ... other params
    Mix = X  // Find actual index
};
```

2. Update getMixParameterIndex() in PluginProcessor.cpp:
```cpp
case ENGINE_NAME:
    return X; // Use actual index from .h file
```

3. Add to applyDefaultParameters() if missing:
```cpp
case ENGINE_NAME:
    // Set other params...
    parameters.getParameter(slotPrefix + juce::String(X+1))->setValueNotifyingHost(1.0f); // Mix
    break;
```

## Testing Protocol
1. Load engine in Logic
2. Check if it processes audio with default parameters
3. Manually set Mix to 100% and verify it works
4. Document any other parameter issues found