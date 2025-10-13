# PITCH ACCURACY SCIENTIFIC TEST - QUICK START GUIDE

## TL;DR

**What**: Scientific pitch accuracy validation using 6 detection algorithms
**Why**: Prove pitch engines meet professional standards (Melodyne/Auto-Tune level)
**How**: Build, run, analyze results

---

## Quick Build & Run

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Build (5-10 minutes)
./build_pitch_scientific.sh

# Run (5-10 minutes, 324 tests)
./build/test_pitch_accuracy_scientific

# View results
open PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md
open build/pitch_scientific_results.csv
```

---

## What Gets Tested

### Engines (6)
- Engine 31: Pitch Shifter
- Engine 32: Detune Doubler
- Engine 33: Intelligent Harmonizer
- Engine 42: Shimmer Reverb
- Engine 49: Phased Vocoder
- Engine 50: Granular Cloud

### Frequencies (6)
- 55 Hz (A1) - Low bass
- 110 Hz (A2) - Bass guitar
- 220 Hz (A3) - Male voice
- 440 Hz (A4) - Concert pitch
- 880 Hz (A5) - Soprano
- 1760 Hz (A6) - High soprano

### Pitch Shifts (9)
- -12, -7, -5, -2, 0, +2, +5, +7, +12 semitones

**Total: 324 tests**

---

## Detection Algorithms (6)

1. **YIN Autocorrelation** - Industry standard, highest accuracy
2. **Cepstrum Analysis** - Optimal for harmonics
3. **FFT Peak Detection** - Fast, accurate for pure tones
4. **Zero-Crossing Rate** - Simple validation
5. **Harmonic Product Spectrum** - Robust for complex tones
6. **AMDF** - Alternative autocorrelation

**Method**: Median consensus (all must agree within ±50 cents)

---

## Professional Standards

| Quality | Accuracy | Examples |
|---------|----------|----------|
| EXCELLENT | ±1 cent | Melodyne, Celemony |
| PROFESSIONAL | ±3 cents | Auto-Tune (industry standard) |
| ACCEPTABLE | ±5 cents | Consumer DAW plugins |
| POOR | ±10 cents | Barely usable |
| FAIL | >10 cents | Broken |

**What is a cent?** 1/100th of a semitone
- 0-3 cents: Imperceptible
- 5 cents: Trained ear may notice
- 10 cents: Clearly audible error
- 100 cents: Full semitone (wrong note!)

---

## Interpreting Results

### Statistical Summary Table

```markdown
| Engine | Quality | Mean±SD | 95% CI | Min | Max | Pass Rate |
|--------|---------|---------|--------|-----|-----|-----------|
| 31 - Pitch Shifter | PROFESSIONAL | 2.3±1.1 | [2.0, 2.6] | 0.8 | 4.7 | 92% |
```

**Key Metrics:**
- **Mean±SD**: Average error ± consistency
- **95% CI**: Statistical confidence range
- **Min/Max**: Best and worst case
- **Pass Rate**: % tests within ±5 cents
- **Quality**: Overall rating

### Example Interpretation

**Engine 31: Pitch Shifter**
- Mean: 2.3 cents → PROFESSIONAL level
- 95% CI: [2.0, 2.6] → Consistently accurate
- Max: 4.7 cents → All tests pass (< 5¢)
- **Verdict**: Auto-Tune quality, production ready

---

## Pass/Fail Criteria

### Per-Test
- **PASS**: |error| < 5 cents
- **FAIL**: |error| ≥ 5 cents

### Per-Engine
- Mean < 1¢ AND max < 3¢ → **EXCELLENT**
- Mean < 3¢ AND max < 5¢ → **PROFESSIONAL**
- Mean < 5¢ AND max < 10¢ → **ACCEPTABLE**
- Mean < 10¢ → **POOR**
- Mean ≥ 10¢ → **FAIL**

### Overall System
- Pass rate ≥ 70% → **PASS**
- Pass rate < 70% → **FAIL**

---

## Output Files

### 1. PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md
- **Format**: Markdown report
- **Content**:
  - Executive summary
  - Methodology
  - Statistical tables
  - Detailed results per engine
  - Rankings
  - Recommendations

### 2. build/pitch_scientific_results.csv
- **Format**: CSV spreadsheet
- **Columns**: EngineID, EngineName, InputFreq, SemitoneShift, ExpectedFreq, MeasuredFreq, CentError, YIN, Cepstrum, FFT, ZeroCrossing, HPS, AMDF, Valid, Pass, Notes
- **Rows**: 324 test results
- **Use**: Import to Excel/Python for analysis

---

## Expected Runtime

- **Build**: 5-10 minutes (compile JUCE + engines)
- **Execution**: 5-10 minutes (324 tests × 6 algorithms)
- **Total**: ~15-20 minutes

**Progress Display**:
```
Testing Engine 31 - Pitch Shifter...
  Progress: 54/324 (16%)
```

---

## Troubleshooting

### Build Fails
```bash
# Clean and rebuild
rm -rf build_pitch_scientific
./build_pitch_scientific.sh
```

### Missing Dependencies
```bash
# Ensure JUCE is installed
ls /Users/Branden/JUCE  # Should exist

# Ensure source files exist
ls ../JUCE_Plugin/Source/PitchShifter.cpp  # Should exist
```

### Test Fails
- Check console output for error messages
- Review CSV file for specific failures
- Low frequencies (55-110 Hz) may have higher errors
- Extreme shifts (±12 st) may be challenging

---

## Example Console Output

```
╔═══════════════════════════════════════════════════════════════════════════╗
║          SCIENTIFIC PITCH ACCURACY ANALYSIS - MULTI-ALGORITHM             ║
╚═══════════════════════════════════════════════════════════════════════════╝

Methodology: 6-Algorithm Consensus
  1. YIN Autocorrelation
  2. Cepstrum Analysis
  3. FFT Peak Detection
  4. Zero-Crossing Rate
  5. Harmonic Product Spectrum
  6. AMDF

Test Matrix:
  Engines: 6
  Frequencies: 6
  Shifts: 9
  Total tests: 324

═══════════════════════════════════════════════════════════════════════════
RUNNING TESTS
═══════════════════════════════════════════════════════════════════════════

Testing Engine 31 - Pitch Shifter...
  Progress: 54/324 (16%)

Testing Engine 32 - Detune Doubler...
  Progress: 108/324 (33%)

[... continues ...]

═══════════════════════════════════════════════════════════════════════════
CALCULATING STATISTICS
═══════════════════════════════════════════════════════════════════════════

Engine 31 - Pitch Shifter:
  Quality: PROFESSIONAL (Auto-Tune level)
  Mean: 2.30 ± 1.10 cents
  95% CI: [2.00, 2.60] cents
  Range: [0.80, 4.70] cents
  Pass rate: 92%

[... all engines ...]

═══════════════════════════════════════════════════════════════════════════
GENERATING REPORT
═══════════════════════════════════════════════════════════════════════════

Scientific report saved to: PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md

╔═══════════════════════════════════════════════════════════════════════════╗
║                           ANALYSIS COMPLETE                               ║
╚═══════════════════════════════════════════════════════════════════════════╝

Reports generated:
  - PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md (Full scientific report)
  - build/pitch_scientific_results.csv (Raw data)

Overall pass rate: 85.2%
```

---

## Using Results

### For QA/Development
- Identify problematic frequencies/shifts
- Compare algorithm implementations
- Validate after code changes
- Regression testing

### For Marketing
- "±2.3 cent accuracy (Auto-Tune level)"
- "Professional-grade pitch shifting"
- "Matches Melodyne accuracy"

### For Documentation
- Include in user manual
- Technical specifications
- Competitive comparison sheets

### For Academic/Scientific Use
- Publication-quality validation
- Regulatory compliance
- Patent applications
- Conference presentations

---

## File Locations

```
standalone_test/
├── test_pitch_accuracy_scientific.cpp     # Main test (1,086 lines)
├── build_pitch_scientific.sh              # Build script
├── PITCH_ACCURACY_SCIENTIFIC_FRAMEWORK.md # Full documentation
├── PITCH_SCIENTIFIC_QUICK_START.md        # This file
└── build/
    ├── test_pitch_accuracy_scientific     # Executable
    └── pitch_scientific_results.csv       # Raw data
PITCH_ACCURACY_SCIENTIFIC_ANALYSIS.md      # Generated report
```

---

## Summary

**Goal**: Prove pitch engines meet professional standards
**Method**: 6 independent algorithms with statistical consensus
**Standard**: ±3 cents (Auto-Tune level) or better
**Output**: Publication-quality validation report

**Bottom Line**: Scientific proof that your pitch engines work!

---

**Need Help?** See full documentation: `PITCH_ACCURACY_SCIENTIFIC_FRAMEWORK.md`
