# Pitch Accuracy Test Suite

Comprehensive test suite for measuring pitch accuracy of pitch shifter engines in ChimeraPhoenix.

## Overview

This test suite evaluates the pitch accuracy of pitch shifting engines (32-38, 49-50) by:
- Testing multiple semitone shifts: -12, -7, -5, 0, +5, +7, +12
- Testing across multiple frequencies: 110Hz, 220Hz, 440Hz, 880Hz, 1760Hz
- Measuring output frequencies using FFT analysis with parabolic interpolation
- Calculating cent error (1 cent = 1/100 semitone)
- Generating comprehensive reports and visualizations

## Files

### Core Test Files
- **`test_pitch_accuracy.cpp`** - C++ test program that runs pitch accuracy tests
- **`analyze_pitch_accuracy.py`** - Python script for analyzing results and generating reports
- **`run_pitch_accuracy_test.sh`** - Shell script that builds, runs tests, and generates reports

### Generated Output
- **`build/pitch_accuracy_results.csv`** - Raw test results in CSV format
- **`build/pitch_accuracy_report.txt`** - Detailed text report with statistics
- **`build/pitch_accuracy_plots/`** - Directory containing visualization plots

## Tested Engines

The test suite evaluates the following engines:

| Engine ID | Engine Name |
|-----------|-------------|
| 32 | Pitch Shifter |
| 33 | Intelligent Harmonizer |
| 34 | Tape Echo |
| 35 | Digital Delay |
| 36 | Magnetic Drum Echo |
| 37 | Bucket Brigade Delay |
| 38 | Buffer Repeat Platinum |
| 49 | Pitch Shifter (Alt) |
| 50 | GranularCloud |

## Test Configuration

### Sample Rate
- 48000 Hz

### Block Size
- 512 samples

### Test Frequencies
- 110 Hz (A2)
- 220 Hz (A3)
- 440 Hz (A4 - Concert pitch)
- 880 Hz (A5)
- 1760 Hz (A6)

### Semitone Shifts
- -12 semitones (down one octave)
- -7 semitones (down perfect fifth)
- -5 semitones (down perfect fourth)
- 0 semitones (unity/bypass)
- +5 semitones (up perfect fourth)
- +7 semitones (up perfect fifth)
- +12 semitones (up one octave)

## Quality Thresholds

The test suite uses the following quality thresholds for pitch accuracy:

| Rating | Cent Error | Description |
|--------|------------|-------------|
| **PROFESSIONAL** | < 1 cent | Studio-quality pitch accuracy |
| **EXCELLENT** | < 5 cents | Very good accuracy, suitable for most applications |
| **GOOD** | < 10 cents | Acceptable accuracy for most use cases |
| **FAIR** | < 20 cents | Noticeable but usable |
| **POOR** | ≥ 20 cents | Significant pitch errors |

### Reference
- 1 cent = 1/100 of a semitone
- 100 cents = 1 semitone
- Human pitch perception threshold: ~5-10 cents depending on context
- Professional pitch correction tools typically aim for < 1 cent accuracy

## Usage

### Quick Start
```bash
# Run the complete test suite (build, test, analyze)
./run_pitch_accuracy_test.sh
```

### Manual Steps

#### 1. Build the test
```bash
# First, ensure JUCE and engine objects are built
make

# Or compile manually
clang++ -std=c++17 -O2 test_pitch_accuracy.cpp -o build/test_pitch_accuracy [includes and libraries]
```

#### 2. Run the test
```bash
./build/test_pitch_accuracy
```

This generates `build/pitch_accuracy_results.csv`

#### 3. Analyze results
```bash
python3 analyze_pitch_accuracy.py
```

This generates:
- `build/pitch_accuracy_report.txt` - Detailed report
- `build/pitch_accuracy_plots/` - Visualization plots

## Output Reports

### CSV Results (`pitch_accuracy_results.csv`)

Contains raw data for each test:
```csv
EngineID,EngineName,InputFreq,SemitoneShift,ExpectedFreq,MeasuredFreq,CentError,Valid,ErrorMsg
32,"Pitch Shifter",440.00,-12,220.00,220.15,1.18,YES,""
32,"Pitch Shifter",440.00,-7,293.66,293.80,0.82,YES,""
...
```

### Text Report (`pitch_accuracy_report.txt`)

Comprehensive report including:
- Overall summary statistics
- Engine summary table
- Detailed per-engine reports with:
  - Overall statistics (avg error, max error, std dev)
  - Quality rating
  - Error by semitone shift
  - Error by input frequency
  - Error distribution

### Visualization Plots

The analysis generates several plots in `build/pitch_accuracy_plots/`:

1. **`pitch_accuracy_by_engine.png`**
   - Bar chart showing average error for each engine
   - Color-coded by quality rating
   - Threshold lines for quality levels

2. **`pitch_error_distribution.png`**
   - Histogram of pitch errors across all tests
   - Shows distribution of accuracy
   - Threshold lines for quality levels

3. **`pitch_error_by_shift.png`**
   - Line plot of average error vs semitone shift
   - Shows how accuracy varies with shift amount
   - Averaged across all engines

4. **`pitch_error_by_frequency.png`**
   - Line plot of average error vs input frequency
   - Shows frequency-dependent accuracy
   - Logarithmic frequency scale

## Technical Details

### Pitch Detection Algorithm

The test suite uses FFT-based frequency detection with:
- FFT size: 8192 samples
- Hann windowing
- Parabolic interpolation for sub-bin accuracy
- Analysis window: 32768 samples (~680ms at 48kHz)
- Transient skip: First 15% of output

### Cent Error Calculation

```
cents = 1200 × log₂(measured_freq / expected_freq)
```

Where:
- `measured_freq` = detected output frequency
- `expected_freq` = input_freq × 2^(semitones/12)

### Validation

Results are considered valid if:
- Output frequency is detected (> 1e-6 amplitude)
- Frequency is within ±2 semitones of expected
- No NaN or Inf values

## Requirements

### Build Requirements
- clang++ with C++17 support
- JUCE framework (tested with JUCE 7.x)
- macOS frameworks (Accelerate, CoreAudio, etc.)
- HarfBuzz library

### Analysis Requirements
- Python 3.6+
- NumPy
- Matplotlib

Install Python dependencies:
```bash
pip3 install numpy matplotlib
```

## Interpreting Results

### Good Results
- Average error < 5 cents
- Valid measurements for all tests
- Consistent accuracy across frequencies
- Consistent accuracy across shift amounts

### Warning Signs
- Average error > 10 cents
- High error at specific frequencies
- High error at extreme shifts (-12, +12)
- Failed measurements (silence or wrong frequency)

### Common Issues

1. **Silence or No Output**
   - Engine may not be properly configured
   - Mix parameter may be at 0%
   - Engine may require specific parameter settings

2. **Wrong Frequency Range**
   - Parameter mapping may be incorrect
   - Engine may have limited shift range
   - Engine may not be a pitch shifter

3. **High Error at Extremes**
   - Normal for some algorithms
   - Phase vocoder artifacts
   - Time-stretching limits

4. **Frequency-Dependent Errors**
   - Window size effects
   - Algorithm-specific behavior
   - Formant preservation artifacts

## Example Output

```
═══════════════════════════════════════════════════════════════════════
                        OVERALL SUMMARY
═══════════════════════════════════════════════════════════════════════

  Engines Tested:      9
  Total Tests:         315
  Valid Measurements:  285 (90.5%)
  Failed Measurements: 30 (9.5%)

  Overall Statistics:
    Average Error:     3.42 cents
    Median Error:      2.15 cents
    Std Deviation:     4.28 cents
    Max Error:         15.67 cents
    Min Error:         0.03 cents

═══════════════════════════════════════════════════════════════════════
                    ENGINE SUMMARY TABLE
═══════════════════════════════════════════════════════════════════════

ID   Engine Name                    Valid    Avg Error    Max Error    Rating
--------------------------------------------------------------------------------
32   Pitch Shifter                  35/35     2.45 cents   8.23 cents  ✓ EXCELLENT
33   Intelligent Harmonizer         35/35     1.87 cents   5.12 cents  ✓ EXCELLENT
49   Pitch Shifter (Alt)            35/35     3.21 cents   9.45 cents  ✓ EXCELLENT
...
```

## Troubleshooting

### Build Errors
```bash
# Ensure JUCE and engines are built first
make clean && make

# Check for required files
ls -la build/obj/*.o
```

### Test Crashes
- Check engine IDs are valid
- Ensure proper initialization
- Check for memory issues

### Analysis Errors
```bash
# Install missing dependencies
pip3 install numpy matplotlib

# Check results file exists
ls -la build/pitch_accuracy_results.csv
```

## Future Enhancements

Potential improvements to the test suite:
- Add latency measurement
- Add formant preservation testing
- Add artifact detection (aliasing, pre-echo, etc.)
- Add THD measurement
- Add transient response testing
- Add real-world audio testing (not just sine waves)
- Add comparison to reference implementations

## References

- [Cent (music)](https://en.wikipedia.org/wiki/Cent_(music))
- [Pitch detection algorithm](https://en.wikipedia.org/wiki/Pitch_detection_algorithm)
- [Phase vocoder](https://en.wikipedia.org/wiki/Phase_vocoder)
- [PSOLA](https://en.wikipedia.org/wiki/PSOLA)

## License

Part of ChimeraPhoenix project.

## Author

Generated for Project Chimera v3.0 Phoenix testing infrastructure.
