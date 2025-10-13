# Preset Validation - Quick Start Guide

## What is this?

A comprehensive preset validation system that verifies all factory presets for:
- Valid engine IDs
- Parameter values in range [0.0, 1.0]
- Proper JSON structure
- Trinity AI compatibility
- No crashes or errors

## Quick Run

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./build_preset_validation_simple.sh
```

That's it! The script will:
1. Build the validation program
2. Run all tests automatically
3. Generate a detailed report

## Expected Output

```
============================================
PRESET VALIDATION SUITE
============================================
Total presets to validate: 30

[1/30] Validating: Velvet Thunder (GC_001)
  [PASS] All checks passed

[2/30] Validating: Crystal Palace (GC_002)
  [PASS] All checks passed

...

============================================
VALIDATION COMPLETE
============================================
Total: 30 presets
Passed: 30
Failed: 0
Success Rate: 100%
============================================
```

## Results

### View Detailed Report

```bash
cat preset_validation_report.txt
```

### View Summary

```bash
cat PRESET_VALIDATION_SUMMARY.md
```

## What Gets Validated?

### 1. Engine IDs
- Must be in range 0-56
- Engine 0 = None/Empty (valid)
- Engines 1-56 = DSP engines

### 2. Parameters
- All values must be in range [0.0, 1.0]
- No NaN or Inf values
- Correct parameter count for each engine

### 3. Structure
- Valid JSON format
- Required fields: id, name, engines
- Valid slot assignments (0-5)
- No duplicate slots

### 4. Mix Parameter
- Must be in range [0.0, 1.0]
- Controls wet/dry blend

## Validation Results Summary

**Current Status**: ✅ ALL TESTS PASSING

- Total Presets: 30
- Passed: 30 (100%)
- Failed: 0
- Errors: 0
- Warnings: 0

## Preset Categories Tested

- Spatial Design (6 presets)
- Character & Color (5 presets)
- Studio Essentials (4 presets)
- Experimental (4 presets)
- Dynamic Processing (3 presets)
- Creative Sound Design (3 presets)
- Experimental Laboratory (2 presets)
- And more...

## Most Used Engines

1. DynamicEQ - 4 times
2. HarmonicExciter - 3 times
3. PhasedVocoder - 3 times
4. VCACompressor - 3 times
5. ShimmerReverb - 3 times

## Trinity Compatibility

All 30 presets are fully compatible with Trinity AI system:
- ✅ Valid structure
- ✅ Valid engine IDs
- ✅ Valid parameters
- ✅ Proper slot assignments

## Exit Codes

- **0**: All tests passed
- **1**: One or more tests failed

## Troubleshooting

### Build Fails

Make sure JUCE is available:
```bash
ls /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE/modules
```

### Missing Preset File

Check that the preset file exists:
```bash
ls /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/pi_deployment/JUCE_Plugin/GoldenCorpus/all_presets.json
```

### Custom Preset File

To test a different preset file:
```bash
./preset_validation_test /path/to/your/presets.json
```

## Files Created

1. **test_preset_validation_simple.cpp** - Main test program
2. **build_preset_validation_simple.sh** - Build & run script
3. **preset_validation_report.txt** - Detailed results
4. **PRESET_VALIDATION_SUMMARY.md** - Full documentation

## Integration with CI/CD

The validation can be integrated into automated testing:

```bash
#!/bin/bash
cd standalone_test
./build_preset_validation_simple.sh
if [ $? -eq 0 ]; then
    echo "Preset validation passed"
    exit 0
else
    echo "Preset validation failed"
    exit 1
fi
```

## Next Steps

1. Run the validation: `./build_preset_validation_simple.sh`
2. Review the report: `cat preset_validation_report.txt`
3. Check summary: `cat PRESET_VALIDATION_SUMMARY.md`

## Questions?

- Check `PRESET_VALIDATION_SUMMARY.md` for detailed information
- Review `preset_validation_report.txt` for per-preset details
- Examine `test_preset_validation_simple.cpp` for validation logic

---

**Quick Start Version**: 1.0
**Last Updated**: 2025-10-11
**Status**: Ready to use ✅
