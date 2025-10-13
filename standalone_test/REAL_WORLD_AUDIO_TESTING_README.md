# REAL-WORLD AUDIO TESTING SUITE

## Overview

This suite tests all 57 Chimera Phoenix engines with realistic musical materials instead of synthetic test signals. It provides subjective quality ratings (A/B/C/D/F) based on audio fidelity, artifact detection, and dynamic range preservation.

## Why Real-World Testing?

Synthetic signals (sine waves, impulses) are excellent for measuring technical specs, but they don't reveal how engines handle:
- Complex harmonic content
- Transients and attacks
- Natural envelopes and dynamics
- Musical frequency distributions
- Real-world signal characteristics

This suite bridges that gap.

---

## Test Materials

### 1. Drum Loop (120 BPM, 4 bars)
- **Content**: Kick, snare, hi-hats with realistic transients
- **Tests**: Dynamics engines, transient shapers, compressors
- **Key Challenge**: Preserving punch and attack

### 2. Bass Line (E1-E2 range, 40-80Hz)
- **Content**: Bass notes with harmonics (40-80Hz fundamental)
- **Tests**: Filters, EQs, distortion, sub-frequency handling
- **Key Challenge**: Sub-bass clarity without mud

### 3. Vocal Sample
- **Content**: Formants, vibrato, sibilance
- **Tests**: Pitch shifters, formant processing, de-essers, compressors
- **Key Challenge**: Natural vocal character preservation

### 4. Guitar Chord (E major)
- **Content**: Acoustic guitar with full frequency range
- **Tests**: Reverbs, delays, modulation, stereo processing
- **Key Challenge**: Natural decay and harmonic richness

### 5. Piano Notes (C1, C4, C7)
- **Content**: Three notes across full frequency spectrum
- **Tests**: Full-range processing, reverb density, delay feedback
- **Key Challenge**: Inharmonicity and natural decay

### 6. White Noise Burst
- **Content**: 0.5s burst of full-spectrum noise
- **Tests**: Spectral processing, gates, expanders
- **Key Challenge**: Uniform frequency response

### 7. Pink Noise (Sustained)
- **Content**: 3s of balanced-spectrum noise
- **Tests**: Frequency response accuracy, artifact detection
- **Key Challenge**: Flat response without coloration

---

## Quick Start

### Step 1: Generate Test Materials

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
python3 generate_musical_materials.py
```

**Output**: 7 WAV files in `real_world_test_materials/`
- drum_loop_120bpm.wav
- bass_line_e1_e2.wav
- vocal_sample_formants.wav
- guitar_chord_emajor.wav
- piano_notes_c1_c4_c7.wav
- white_noise_burst.wav
- pink_noise_sustained.wav

**Duration**: ~10 seconds (generates all materials)

---

### Step 2: Build Test Suite

```bash
chmod +x build_real_world_test.sh
./build_real_world_test.sh
```

**Output**: `test_real_world_audio` executable

---

### Step 3: Run Tests

```bash
./test_real_world_audio
```

**What It Does**:
1. Loads all 7 test materials
2. Processes each material through all 57 engines
3. Analyzes input vs. output quality
4. Assigns subjective grades (A/B/C/D/F)
5. Identifies issues and artifacts
6. Generates comprehensive report

**Duration**: ~2-3 minutes for 399 tests (57 engines × 7 materials)

**Progress Display**:
```
[Engine 0] Bypass
------------------------------------------------------------
  [1.2%] Testing with: 120 BPM drum loop... Grade: A
  [2.5%] Testing with: Bass line... Grade: A
  ...

[Engine 1] Vintage Opto Compressor
------------------------------------------------------------
  [3.8%] Testing with: 120 BPM drum loop... Grade: B
  ...
```

---

### Step 4: Review Report

```bash
open REAL_WORLD_AUDIO_TESTING_REPORT.md
```

or

```bash
cat REAL_WORLD_AUDIO_TESTING_REPORT.md
```

---

## Grading System

### Grade Criteria

**A (Excellent)** - Score 90-100
- No introduced clipping
- No artifacts
- Minimal dynamic range loss (<5 dB)
- Noise floor increase <2x
- DC offset <1%
- **Status**: Production ready

**B (Good)** - Score 80-89
- Minor dynamic range loss (5-10 dB)
- Slight noise floor increase (2-5x)
- No critical artifacts
- **Status**: Production ready with monitoring

**C (Acceptable)** - Score 70-79
- Moderate dynamic range loss (10-15 dB)
- Noticeable noise floor increase
- Minor artifacts present
- **Status**: Acceptable for most uses

**D (Poor)** - Score 60-69
- Significant dynamic range loss (>15 dB)
- High noise floor increase
- Artifacts affecting sound quality
- **Status**: Needs improvement

**F (Failed)** - Score <60
- Output silence
- Severe clipping
- Critical artifacts
- Unusable audio quality
- **Status**: Requires immediate attention

---

## Metrics Analyzed

### 1. Peak Level
- Maximum sample amplitude
- Detects clipping (>0.99)

### 2. RMS Level
- Average signal energy
- Measures loudness

### 3. Dynamic Range
- Ratio of peak to RMS in dB
- Indicates punch and transients

### 4. DC Offset
- Average signal value
- Should be near zero

### 5. Stereo Correlation
- Relationship between L/R channels
- Detects phase issues

### 6. Noise Floor
- RMS of quietest 10% of signal
- Measures added noise

### 7. Artifact Detection
- Counts discontinuities
- Identifies glitches/clicks

### 8. Clipping Detection
- Samples hitting digital ceiling
- Critical quality issue

---

## Report Structure

### 1. Summary Statistics
- Grade distribution (A/B/C/D/F counts)
- Pass rate percentage
- Overall quality metrics

### 2. Detailed Results by Engine
For each of 57 engines:
- Overall grade
- Results table (material × grade × issues)
- Critical issues highlighted

### 3. Recommendations
- Engines requiring attention (D/F grades)
- Top performers (all A grades)
- Specific fix priorities

---

## Example Report Snippet

```markdown
### Engine 1: Vintage Opto Compressor

**Overall Grade**: B

| Material | Grade | Issues |
|----------|-------|--------|
| drum_loop_120bpm.wav | B | Dynamic range loss: 6.2 dB |
| bass_line_e1_e2.wav | A | None |
| vocal_sample_formants.wav | B | Noise floor increased 2.3x |
| guitar_chord_emajor.wav | A | None |
| piano_notes_c1_c4_c7.wav | A | None |
| white_noise_burst.wav | B | None |
| pink_noise_sustained.wav | A | None |
```

---

## Interpreting Results

### Dynamic Range Loss
- **Acceptable**: <5 dB (compressors, limiters)
- **Warning**: 5-10 dB (aggressive processing)
- **Problem**: >10 dB (over-processing)

### Noise Floor Increase
- **Acceptable**: <2x (minimal degradation)
- **Warning**: 2-5x (noticeable but usable)
- **Problem**: >5x (significant quality loss)

### DC Offset
- **Acceptable**: <0.01 (1%)
- **Warning**: 0.01-0.05 (1-5%)
- **Problem**: >0.05 (>5%)

### Artifacts
- **Acceptable**: 0-10 discontinuities
- **Warning**: 10-50 discontinuities
- **Problem**: >50 discontinuities or audible clicks

---

## Troubleshooting

### Issue: "Failed to load test materials"

**Solution**:
```bash
python3 generate_musical_materials.py
```

### Issue: Build fails

**Solution**:
```bash
# Create build directory
mkdir -p build

# Check JUCE path
ls /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE

# Rebuild JUCE stubs
rm build/juce_stubs.o
./build_real_world_test.sh
```

### Issue: Engine creates silent output

**Analysis**: Check report for:
- "Output is silent" in issues column
- Grade F
- Indicates engine crash or zero output

**Fix**: Investigate engine's processBlock() implementation

### Issue: Engine introduces clipping

**Analysis**: Check report for:
- "Introduced clipping" in issues column
- Indicates insufficient headroom

**Fix**: Add makeup gain control or adjust internal scaling

---

## Advanced Usage

### Test Single Engine

Modify `test_real_world_audio.cpp`:
```cpp
// Change this loop
for (int engineID = 0; engineID <= 56; ++engineID) {

// To test only engine 15
for (int engineID = 15; engineID <= 15; ++engineID) {
```

### Test Single Material

Modify material loading:
```cpp
std::vector<std::string> filenames = {
    "drum_loop_120bpm.wav"  // Only test drums
};
```

### Save All Processed Audio

Change this:
```cpp
// Save processed audio for critical failures
if (result.subjectiveGrade == 'F') {

// To save everything
if (true) {
```

### Adjust Grading Strictness

Modify penalty scores in `assignGrade()`:
```cpp
// More strict
if (output.hasClipping && !input.hasClipping) {
    score -= 50;  // Was 30
}

// More lenient
if (output.hasClipping && !input.hasClipping) {
    score -= 15;  // Was 30
}
```

---

## Integration with CI/CD

### Automated Testing

```bash
#!/bin/bash
# run_real_world_tests.sh

# Generate materials if missing
if [ ! -d "real_world_test_materials" ]; then
    python3 generate_musical_materials.py
fi

# Build test suite
./build_real_world_test.sh || exit 1

# Run tests
./test_real_world_audio || exit 1

# Check for failures
FAILURES=$(grep -c "Grade: F" REAL_WORLD_AUDIO_TESTING_REPORT.md)

if [ $FAILURES -gt 0 ]; then
    echo "ERROR: $FAILURES engine tests failed!"
    exit 1
fi

echo "All tests passed!"
```

### Quality Gates

Set minimum pass rates:
```bash
PASS_RATE=$(grep "Pass Rate:" REAL_WORLD_AUDIO_TESTING_REPORT.md | awk '{print $3}' | sed 's/%//')

if [ $PASS_RATE -lt 95 ]; then
    echo "ERROR: Pass rate ($PASS_RATE%) below 95% threshold"
    exit 1
fi
```

---

## Comparison with Other Test Types

| Test Type | Purpose | Real-World Testing |
|-----------|---------|-------------------|
| **Impulse Tests** | Frequency response | Confirms natural material response |
| **THD Tests** | Harmonic distortion | Reveals distortion on complex signals |
| **Stereo Tests** | Phase/correlation | Tests with natural stereo content |
| **Latency Tests** | Processing delay | Measures with real-time simulation |
| **CPU Tests** | Performance | Realistic processing load |

**Real-world testing is complementary, not replacement.**

---

## Expected Results

### Perfect Engines (Grade A on all materials)
- Engine 0: Bypass
- Engine 7: Parametric EQ (transparent settings)
- Engine 56: Utility (unity gain)

### Engines Expected to Alter Audio (Grade B-C)
- Dynamics processors (compressors, limiters)
- Distortion/saturation engines
- Extreme modulation effects
- Creative effects (granular, etc.)

### Engines Requiring Investigation (Grade D-F)
- Any engine producing silence
- Engines with severe artifacts
- Engines introducing unexpected clipping

---

## Performance Notes

### Test Duration
- Material generation: ~10 seconds
- Build time: ~30 seconds
- Test execution: ~2-3 minutes
- **Total**: ~4 minutes end-to-end

### Disk Space
- Test materials: ~5 MB
- Processed audio (failures only): ~1-2 MB
- Report: ~100 KB

### Memory Usage
- Peak RAM: ~200 MB
- Streaming processing (minimal memory footprint)

---

## Future Enhancements

### Potential Additions
1. **More Materials**: Orchestra, electronic music, speech
2. **Frequency Analysis**: FFT-based spectral comparison
3. **Perceptual Metrics**: LUFS, ITU-R BS.1770
4. **A/B Comparison**: Side-by-side audio players
5. **Visual Reports**: Waveform/spectrum plots
6. **Batch Testing**: Multiple parameter sets per engine
7. **Reference Comparisons**: Against commercial plugins

---

## Conclusion

This real-world audio testing suite provides essential subjective quality assessment that complements technical measurements. By testing with realistic musical materials, you can:

1. Identify issues not caught by synthetic tests
2. Validate musical transparency
3. Detect artifacts in real-world contexts
4. Ensure production-ready quality
5. Build confidence in your DSP implementations

**Run this suite before any major release or when investigating quality issues.**

---

## Quick Reference Commands

```bash
# Full workflow
python3 generate_musical_materials.py
./build_real_world_test.sh
./test_real_world_audio
open REAL_WORLD_AUDIO_TESTING_REPORT.md

# Clean and rebuild
rm -rf real_world_test_materials build/*.o test_real_world_audio
python3 generate_musical_materials.py
./build_real_world_test.sh

# View just summary
head -50 REAL_WORLD_AUDIO_TESTING_REPORT.md

# Count failures
grep "Grade: F" REAL_WORLD_AUDIO_TESTING_REPORT.md | wc -l

# List problem engines
grep -A 3 "Grade: F" REAL_WORLD_AUDIO_TESTING_REPORT.md

# Find specific engine results
grep -A 10 "Engine 15:" REAL_WORLD_AUDIO_TESTING_REPORT.md
```

---

**Author**: Chimera Phoenix Development Team
**Version**: 1.0
**Date**: October 11, 2025
