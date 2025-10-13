# REAL-WORLD AUDIO TESTING - SYSTEM INDEX

## Quick Links

**Start Here** → [QUICK START GUIDE](REAL_WORLD_TESTING_QUICK_START.md)

**Complete Documentation** → [README](REAL_WORLD_AUDIO_TESTING_README.md)

**Full Deliverable** → [DELIVERABLE](REAL_WORLD_TESTING_DELIVERABLE.md)

**Summary** → [SUMMARY](REAL_WORLD_TESTING_SUMMARY.txt)

---

## One-Line Execution

```bash
./run_real_world_tests.sh
```

---

## System Components

### 1. Executable Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| `run_real_world_tests.sh` | **Complete workflow** | `./run_real_world_tests.sh` |
| `generate_musical_materials.py` | Generate test audio | `python3 generate_musical_materials.py` |
| `build_real_world_test.sh` | Build test framework | `./build_real_world_test.sh` |
| `analyze_real_world_results.py` | Analyze results | `python3 analyze_real_world_results.py` |

**Recommended**: Use `run_real_world_tests.sh` for complete automation

---

### 2. Source Code

| File | Description |
|------|-------------|
| `test_real_world_audio.cpp` | C++ test framework (processes all engines) |
| `ComprehensiveTHDEngineFactory.h/.cpp` | Engine factory (creates engines) |

---

### 3. Documentation

| Document | Purpose | When to Use |
|----------|---------|-------------|
| `REAL_WORLD_TESTING_QUICK_START.md` | Quick reference | **First time use** |
| `REAL_WORLD_AUDIO_TESTING_README.md` | Complete guide | Deep dive, troubleshooting |
| `REAL_WORLD_TESTING_DELIVERABLE.md` | Full deliverable | Technical details, integration |
| `REAL_WORLD_TESTING_SUMMARY.txt` | Overview | High-level understanding |
| `INDEX_REAL_WORLD_TESTING.md` | This file | Navigation |

---

## Test Materials (Generated)

| Material | Description | Tests |
|----------|-------------|-------|
| `drum_loop_120bpm.wav` | 120 BPM drums | Dynamics, transients |
| `bass_line_e1_e2.wav` | 40-80Hz bass | Filters, sub-bass |
| `vocal_sample_formants.wav` | Vocal with formants | Pitch, de-esser |
| `guitar_chord_emajor.wav` | Acoustic guitar | Reverb, delay |
| `piano_notes_c1_c4_c7.wav` | Piano across range | Full-range processing |
| `white_noise_burst.wav` | 0.5s noise burst | Spectral, gates |
| `pink_noise_sustained.wav` | 3s pink noise | Frequency response |

**Location**: `real_world_test_materials/` (created on first run)

---

## Output Files (Generated)

| File | Description |
|------|-------------|
| `REAL_WORLD_AUDIO_TESTING_REPORT.md` | **Main report** with all results |
| `real_world_test_results.json` | JSON export for automation |
| `output_engine_*.wav` | Processed audio for failed tests |

---

## Workflow Diagram

```
┌─────────────────────────────────────────────────────────┐
│                  run_real_world_tests.sh                │
│                   (One Command Entry)                    │
└────────────┬────────────────────────────────────────────┘
             │
             ├─► Step 1: generate_musical_materials.py
             │           └─► real_world_test_materials/*.wav
             │
             ├─► Step 2: build_real_world_test.sh
             │           └─► test_real_world_audio (executable)
             │
             ├─► Step 3: test_real_world_audio
             │           └─► REAL_WORLD_AUDIO_TESTING_REPORT.md
             │
             └─► Step 4: Display summary
                         └─► Pass rate, failures, recommendations
```

---

## Quick Reference Commands

### Complete Workflow
```bash
./run_real_world_tests.sh
```

### Individual Steps
```bash
# 1. Generate materials
python3 generate_musical_materials.py

# 2. Build
./build_real_world_test.sh

# 3. Test
./test_real_world_audio

# 4. Analyze
python3 analyze_real_world_results.py
```

### View Results
```bash
# Main report
open REAL_WORLD_AUDIO_TESTING_REPORT.md

# Summary only
head -50 REAL_WORLD_AUDIO_TESTING_REPORT.md

# Find failures
grep "Grade: F" REAL_WORLD_AUDIO_TESTING_REPORT.md
```

---

## Grading Quick Reference

| Grade | Score | Status | Action |
|-------|-------|--------|--------|
| A | 90-100 | ✅ Excellent | None needed |
| B | 80-89 | ✅ Good | Monitor |
| C | 70-79 | ⚠️ Acceptable | Review if time permits |
| D | 60-69 | 🚨 Poor | Needs improvement |
| F | <60 | 🚨 Failed | **Fix immediately** |

**Target**: 95%+ pass rate (Grade C or better)

---

## Test Coverage

- **Engines**: 57 (ID 0-56)
- **Materials**: 7 realistic audio samples
- **Total Tests**: 399 (57 × 7)
- **Duration**: ~2-3 minutes
- **Metrics**: 8 quality measurements per test

---

## Common Tasks

### First Time Setup
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./run_real_world_tests.sh
```

### Daily Development
```bash
# After making changes to an engine
./run_real_world_tests.sh

# Check your engine's results
grep "Engine YOUR_ID:" REAL_WORLD_AUDIO_TESTING_REPORT.md -A 15
```

### Pre-Release Validation
```bash
# Full test suite
./run_real_world_tests.sh

# Verify pass rate
grep "Pass Rate:" REAL_WORLD_AUDIO_TESTING_REPORT.md

# Ensure ≥95%
```

### Troubleshooting
```bash
# Regenerate materials
python3 generate_musical_materials.py

# Clean build
rm -rf build/*.o
./build_real_world_test.sh

# Detailed analysis
python3 analyze_real_world_results.py
```

---

## Integration Points

### With Existing Tests
- Complements impulse, THD, stereo, and CPU tests
- Run alongside technical measurements
- Provides subjective quality validation

### With CI/CD
```bash
# Add to pipeline
./run_real_world_tests.sh
PASS_RATE=$(grep "Pass Rate:" REAL_WORLD_AUDIO_TESTING_REPORT.md | awk '{print $3}' | sed 's/%//')
[ $PASS_RATE -ge 95 ]
```

---

## Performance

- **Material Generation**: ~10 seconds
- **Build Time**: ~30 seconds (first time), ~5 seconds (incremental)
- **Test Execution**: ~2-3 minutes
- **Analysis**: <1 second
- **Total End-to-End**: ~4-5 minutes

---

## Dependencies

### Required
- **C++ Compiler**: clang++ (macOS) or g++ (Linux)
- **Python 3**: For material generation and analysis
- **NumPy**: Python library for audio synthesis
- **JUCE**: Audio framework (already in project)

### Install NumPy (if needed)
```bash
pip3 install numpy
```

---

## File Locations

All files in:
```
/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/
```

Key directories:
- `.` - Scripts and documentation
- `real_world_test_materials/` - Generated test audio (created on first run)
- `build/` - Compiled object files

---

## Documentation Navigator

### I'm New - Where Do I Start?
→ **[QUICK START GUIDE](REAL_WORLD_TESTING_QUICK_START.md)**

### I Want Complete Details
→ **[README](REAL_WORLD_AUDIO_TESTING_README.md)**

### I Need Technical Specs
→ **[DELIVERABLE](REAL_WORLD_TESTING_DELIVERABLE.md)**

### I Want a High-Level Overview
→ **[SUMMARY](REAL_WORLD_TESTING_SUMMARY.txt)**

### I'm Looking for Something Specific
→ This index file

---

## Support & Troubleshooting

### Issue: Materials not generating
**Solution**: Check NumPy installation
```bash
pip3 install numpy
python3 generate_musical_materials.py
```

### Issue: Build fails
**Solution**: Check JUCE path and create build directory
```bash
mkdir -p build
./build_real_world_test.sh
```

### Issue: Tests failing
**Solution**: Check engine implementation
1. Review report for specific issues
2. Listen to processed audio (`output_engine_*.wav`)
3. Run standalone engine tests
4. Check processBlock() implementation

### Issue: Need more info
**Solution**: Check documentation
1. [QUICK START](REAL_WORLD_TESTING_QUICK_START.md) for basic usage
2. [README](REAL_WORLD_AUDIO_TESTING_README.md) for troubleshooting
3. [DELIVERABLE](REAL_WORLD_TESTING_DELIVERABLE.md) for technical details

---

## Success Indicators

✅ **System Working**: Materials generate, tests run, report created
✅ **High Quality**: Pass rate ≥95%
✅ **No Criticals**: No Grade F failures
✅ **Expected Behavior**: Bypass engine scores all A's

🚨 **Needs Attention**: Pass rate <95%, multiple F grades, silent outputs

---

## Next Steps After Installation

1. **Run First Test**
   ```bash
   ./run_real_world_tests.sh
   ```

2. **Review Results**
   ```bash
   open REAL_WORLD_AUDIO_TESTING_REPORT.md
   ```

3. **Understand Grades**
   - Read [QUICK START](REAL_WORLD_TESTING_QUICK_START.md) grade section

4. **Address Issues**
   - Fix any Grade F failures
   - Review Grade D engines

5. **Integrate into Workflow**
   - Add to pre-release checklist
   - Consider CI/CD integration

---

## Additional Resources

### Example Reports
After running tests, you'll find:
- Grade distribution
- Engine-by-engine results
- Material-specific analysis
- Problem patterns
- Recommendations

### JSON Export
Machine-readable results in `real_world_test_results.json`:
- Complete test data
- Aggregated statistics
- Ready for custom analysis

### Processed Audio
Failed tests save processed audio as `output_engine_*.wav`:
- Listen to understand issues
- Compare with original materials
- Identify specific problems

---

## System Status

**Version**: 1.0
**Status**: ✅ COMPLETE AND OPERATIONAL
**Date**: October 11, 2025
**Tested**: Ready for immediate use

**All components delivered and documented.**

---

## Quick Action Matrix

| I Want To... | Run This |
|-------------|----------|
| Test everything now | `./run_real_world_tests.sh` |
| Just generate materials | `python3 generate_musical_materials.py` |
| Just build framework | `./build_real_world_test.sh` |
| Just run tests | `./test_real_world_audio` |
| Analyze existing results | `python3 analyze_real_world_results.py` |
| View main report | `open REAL_WORLD_AUDIO_TESTING_REPORT.md` |
| Find failures | `grep "Grade: F" REAL_WORLD_AUDIO_TESTING_REPORT.md` |
| Check specific engine | `grep "Engine N:" REAL_WORLD_AUDIO_TESTING_REPORT.md -A 15` |
| Listen to materials | `open real_world_test_materials/*.wav` |
| Read quick start | `open REAL_WORLD_TESTING_QUICK_START.md` |
| Read full guide | `open REAL_WORLD_AUDIO_TESTING_README.md` |

---

**SYSTEM READY - START TESTING NOW**

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./run_real_world_tests.sh
```

---

**Questions?** Check [QUICK START](REAL_WORLD_TESTING_QUICK_START.md) or [README](REAL_WORLD_AUDIO_TESTING_README.md)
