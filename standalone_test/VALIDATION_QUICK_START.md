# Modulation Engine Validation - Quick Start Guide

## Running the Deep Validation Tests

### Build the Test Suite

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./build_deep_modulation_validation.sh
```

### Run Tests

```bash
./build/deep_modulation_validation
```

### Expected Output

The test suite will:
1. Test 10 modulation engines
2. Perform 4 tests per engine (LFO, Depth, Stereo, Feedback)
3. Generate 2 CSV reports with detailed measurements
4. Display pass/fail summary

**Total Runtime:** ~2-3 minutes

---

## Test Coverage

### Engines Validated

| ID | Engine Name | Primary Test Focus |
|----|-------------|-------------------|
| 23 | Stereo Chorus | LFO rate, stereo width |
| 24 | Resonant Chorus Platinum | Voice count, detune, stereo |
| 25 | Analog Phaser | Notch spacing, feedback stability |
| 26 | Platinum Ring Modulator | Harmonic content, spectrum |
| 27 | Frequency Shifter | Linearity, Hz accuracy |
| 28 | Harmonic Tremolo | Split-band modulation, depth |
| 29 | Classic Tremolo | Rate accuracy, waveform quality |
| 46 | Dimension Expander | Stereo width, bass retention |
| 14 | Vocal Formant Filter | Formant tracking, vowel morph |
| 12 | Envelope Filter | Envelope response, tracking |

### Tests Performed

1. **LFO Rate Accuracy**
   - Measures actual Hz of modulation
   - Validates against musical ranges
   - Detects parameter scaling issues

2. **Depth Parameter Linearity**
   - Tests 7 depth settings (0-100%)
   - Calculates correlation coefficient
   - Validates linear response

3. **Stereo Width Analysis**
   - Cross-correlation measurement
   - Phase offset calculation
   - L/R balance verification

4. **Feedback Stability**
   - Tests increasing feedback levels
   - Detects oscillation threshold
   - Validates stability margins

---

## Generated Reports

### CSV Files

After running tests, you'll find:

**modulation_lfo_rates.csv**
```csv
Engine ID,Engine Name,LFO Rate (Hz),Amplitude Modulation (%)
23,Stereo Chorus,27.07,42.3
24,Resonant Chorus Platinum,47.75,38.5
...
```

**modulation_stereo_analysis.csv**
```csv
Engine ID,Engine Name,Correlation,Width (%),Phase (deg),L RMS,R RMS
23,Stereo Chorus,0.571,42.9,58.2,0.245,0.248
24,Resonant Chorus Platinum,0.037,96.3,21.0,0.312,0.308
...
```

### Main Report

**MODULATION_PARAMETER_VALIDATION_REPORT.md**
- Comprehensive parameter documentation
- LFO analysis for each engine
- Stereo width measurements
- Feedback stability tests
- Hardware comparison matrix
- Critical issues and fixes
- Recommended parameter ranges

---

## Interpreting Results

### LFO Rate Test

✓ **PASS:** LFO rate within musical range (0.01-20 Hz)
❌ **FAIL:** LFO too fast/slow for musical use

**Common Issues:**
- Parameter scaling incorrect
- Need exponential/logarithmic mapping
- Missing Hz conversion

### Depth Linearity Test

✓ **PASS:** Correlation > 0.8 (linear response)
❌ **FAIL:** Correlation < 0.8 (non-linear)

**Good Linearity:** User expectations match actual modulation

### Stereo Width Test

✓ **PASS:** Stereo width > 1% (some decorrelation)
⚠️ **WARNING:** Width > 95% (may cause phase issues)

**Optimal Range:** 40-80% for most applications

### Feedback Stability Test

✓ **PASS:** Stable feedback >= 70%
❌ **FAIL:** Oscillates below 70%

**Excellent:** >85% stable feedback

---

## Known Issues (as of Oct 11, 2025)

### Critical

**Engine 27: Frequency Shifter**
- Status: ❌ BROKEN
- Issue: Non-linear frequency shifting (up to 259 Hz error)
- Fix: Requires algorithm rework

### High Priority

**Engines 23 & 24: Chorus LFO Rates**
- Status: ⚠️ NEEDS CALIBRATION
- Issue: LFO 13-48× too fast
- Fix: Rescale parameter mapping

**Engine 28: Harmonic Tremolo**
- Status: ⚠️ NEEDS TUNING
- Issue: LFO 2× too fast
- Fix: Simple scaling adjustment

### Production Ready (7 engines)

✓ Engine 25: Analog Phaser
✓ Engine 26: Platinum Ring Modulator
✓ Engine 29: Classic Tremolo (98% hardware match!)
✓ Engine 46: Dimension Expander
✓ Engine 14: Vocal Formant Filter
✓ Engine 12: Envelope Filter

---

## Quick Reference: Pass Criteria

| Test | Pass Threshold | Measurement |
|------|---------------|-------------|
| LFO Rate | 0.01-20 Hz | Autocorrelation |
| Depth Linearity | r > 0.80 | Correlation coefficient |
| Stereo Width | > 1% | Cross-correlation |
| Feedback Stability | > 70% | Oscillation threshold |

---

## Troubleshooting

### Build Fails

**Issue:** JUCE not found
```bash
# Check JUCE path in build script
export JUCE_PATH="/path/to/JUCE"
```

**Issue:** Missing source files
```bash
# Ensure you're in the correct directory
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
```

### Test Hangs

**Issue:** Long test duration
- Normal runtime: 2-3 minutes for all engines
- Each engine: ~15-20 seconds

**Issue:** Infinite loop
- Check console output
- Ctrl+C to abort
- Report which engine caused hang

### Unexpected Results

**Issue:** All tests fail
- Verify sample rate (48 kHz expected)
- Check audio buffer creation
- Ensure engine initialization succeeds

**Issue:** LFO rates show 0 Hz
- Signal may not have modulation at default settings
- Try increasing depth parameter
- Check if engine requires specific parameter setup

---

## Next Steps

1. **Review Report:** Read MODULATION_PARAMETER_VALIDATION_REPORT.md
2. **Fix Critical Issues:** Start with Engine 27 (Frequency Shifter)
3. **Tune LFO Rates:** Calibrate Engines 23, 24, 28
4. **Validate Fixes:** Re-run tests after implementing fixes
5. **User Testing:** Get feedback on musical usability

---

## Contact & Support

For questions about the validation suite:
- Review the full report (MODULATION_PARAMETER_VALIDATION_REPORT.md)
- Check existing test reports (MODULATION_TECHNICAL_SUMMARY.md)
- Examine source code (deep_modulation_validation.cpp)

---

**Last Updated:** October 11, 2025
**Validation Suite Version:** 1.0
**Compatibility:** macOS (Apple Silicon), Linux, Windows (via WSL)
