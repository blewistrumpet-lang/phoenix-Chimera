# FREQUENCY RESPONSE TEST SUITE
## Comprehensive Testing for Filter & EQ Engines 8-14

**Version:** 1.0
**Author:** Claude Code Automated Test Suite
**Date:** October 11, 2025
**Status:** Production Ready

---

## OVERVIEW

This test suite provides comprehensive frequency response analysis for ChimeraPhoenix audio plugin engines 8-14 (Filters & EQs). It generates logarithmic sine sweeps from 20Hz to 20kHz, measures output amplitude per frequency, plots response curves, and verifies that filters actually filter.

### Engines Tested

| Engine ID | Name | Type | Key Features |
|-----------|------|------|--------------|
| 8 | VintageConsoleEQ_Studio | Vintage EQ | Neve/API/Pultec emulation |
| 9 | LadderFilter | Lowpass | Moog-style 4-pole ladder |
| 10 | StateVariableFilter | Multi-mode | LP/BP/HP/Notch outputs |
| 11 | FormantFilter | Vowel | Vocal formant synthesis |
| 12 | EnvelopeFilter | AutoWah | Dynamic envelope-controlled filter |
| 13 | CombResonator | Resonator | Harmonic comb filtering |
| 14 | VocalFormantFilter | Advanced Vocal | 5-formant voice synthesis |

---

## QUICK START

### Run Complete Test Suite
```bash
./run_frequency_response_suite.sh
```

This single command will:
1. ✓ Compile the test executable
2. ✓ Run frequency response measurements
3. ✓ Generate CSV data files
4. ✓ Create visualization plots
5. ✓ Generate comprehensive reports

### Expected Runtime
- **Compilation:** ~30 seconds
- **Testing:** ~3-5 minutes (7 engines × 100 frequencies)
- **Plot Generation:** ~10 seconds
- **Total:** ~5 minutes

---

## TEST METHODOLOGY

### 1. Signal Generation
- **Frequency Range:** 20 Hz to 20,000 Hz
- **Frequency Points:** 100 (logarithmically spaced)
- **Test Signal:** Pure sine waves
- **Input Level:** -6 dB (0.5 linear amplitude)
- **Test Duration:** 0.5 seconds per frequency

### 2. Measurement Process
For each test frequency:
1. Reset engine to known state
2. Generate pure sine wave at test frequency
3. Process through engine in 512-sample blocks
4. Allow 100ms settling time
5. Measure RMS output level (excluding settling period)
6. Calculate gain in dB: `Gain = 20 × log10(output/input)`
7. Check for stability (no NaN/Inf values)

### 3. Analysis Metrics
- **Maximum Gain:** Peak boost in dB
- **Minimum Gain:** Maximum attenuation in dB
- **Gain Range:** Difference between max and min (filtering effectiveness)
- **Cutoff Frequency:** -3dB point relative to maximum
- **Resonance Peak:** Maximum gain above average
- **Stability:** No numerical instabilities across all frequencies

### 4. Verification Criteria
| Criterion | Threshold | Purpose |
|-----------|-----------|---------|
| Gain Range | > 6 dB | Verify filter actually filters |
| Stability | No NaN/Inf | Ensure numerical stability |
| Output Level | < 10.0 | Prevent runaway feedback |
| Data Points | 100 | Sufficient frequency resolution |

---

## OUTPUT FILES

### 1. CSV Data Files
**Location:** `frequency_response_engine_N.csv`
**Format:** Comma-separated values

```csv
Frequency_Hz,Input_Level,Output_Level,Gain_dB,Phase_Deg
20.000000,0.500000,0.245000,-6.15,0.0
25.118864,0.500000,0.256000,-5.82,0.0
...
```

**Columns:**
- `Frequency_Hz`: Test frequency in Hertz
- `Input_Level`: Input signal amplitude (linear, 0.5 = -6dB)
- `Output_Level`: Measured RMS output amplitude
- `Gain_dB`: Calculated gain (20 × log10(output/input))
- `Phase_Deg`: Phase shift in degrees (currently 0, reserved for future)

### 2. Visualization Plots
**Location:** `frequency_response_plots/`

#### Individual Engine Plots
`frequency_response_engine_N.png` - Detailed plot per engine showing:
- Frequency response (gain vs. frequency)
- Output amplitude (linear)
- Annotated peak and minimum points
- -3dB reference line
- Key frequency markers (100Hz, 1kHz, 10kHz)

#### Combined Comparison Plot
`frequency_response_combined.png` - All engines overlaid:
- **Top panel:** Absolute frequency response comparison
- **Bottom panel:** Normalized response (0dB @ 1kHz)
- Color-coded per engine
- Octave frequency markers
- -3dB and -6dB reference lines

#### Comparison Grid
`frequency_response_grid.png` - 2×4 grid of all engine responses for quick visual comparison

### 3. Text Reports

#### Detailed Analysis Report
**File:** `FREQUENCY_RESPONSE_REPORT.txt`

Contains:
- Test configuration details
- Per-engine analysis:
  - Creation status
  - Stability verification
  - Filtering effectiveness
  - Max/min gain measurements
  - Cutoff frequency identification
  - Error messages (if any)
- Summary statistics
- Pass/fail results

#### Summary Document
**File:** `FREQUENCY_RESPONSE_TEST_SUMMARY.md`

Markdown-formatted summary including:
- Test objectives and methodology
- List of engines tested
- Output file inventory
- Key findings highlights
- Usage examples
- Verification criteria

---

## MANUAL EXECUTION

If you prefer step-by-step execution:

### Step 1: Build
```bash
./build_frequency_response_test.sh
```

### Step 2: Run Test
```bash
./test_frequency_response_8_14
```

Expected output:
```
╔══════════════════════════════════════════════════════════════╗
║  COMPREHENSIVE FREQUENCY RESPONSE TEST SUITE                 ║
║  Filter & EQ Engines 8-14                                    ║
╚══════════════════════════════════════════════════════════════╝

[Testing each engine...]
```

### Step 3: Generate Plots
```bash
python3 plot_frequency_response.py
```

Required Python packages:
- matplotlib
- pandas
- numpy

Install with:
```bash
pip3 install matplotlib pandas numpy
```

---

## INTERPRETING RESULTS

### Frequency Response Curves

#### Ideal Lowpass Filter (e.g., Engine 9: LadderFilter)
```
Gain (dB)
  +3 ─┐
   0 ─┤████████████╗
  -3 ─┤            ╚╗
  -6 ─┤             ╚╗
 -12 ─┤              ╚╗
 -24 ─┤               ╚════════
      └─────────────────────────► Frequency
     20Hz    1kHz    10kHz   20kHz
            ↑ Cutoff
```

**Characteristics:**
- Flat passband (low frequencies)
- Sharp transition (cutoff region)
- Steep rolloff (high frequencies)
- Possible resonance peak at cutoff

#### Ideal Highpass Filter
```
Gain (dB)
  +3 ─┐              ┌████████
   0 ─┤           ┌──┘
  -3 ─┤         ┌─┘
  -6 ─┤       ┌─┘
 -12 ─┤     ┌─┘
 -24 ─┤════─┘
      └─────────────────────────► Frequency
```

#### Ideal Bandpass Filter (e.g., Engine 11: FormantFilter)
```
Gain (dB)
  +6 ─┐       ┌─╗
  +3 ─┤     ┌─┘ ╚─┐
   0 ─┤   ┌─┘     ╚─┐
  -3 ─┤ ┌─┘         ╚─┐
  -6 ─┤─┘             ╚─┐
     └─────────────────────────► Frequency
        ↑ Center frequency
```

#### Comb Filter (e.g., Engine 13: CombResonator)
```
Gain (dB)
  +3 ─┤  ┌┐  ┌┐  ┌┐  ┌┐
   0 ─┤  ││  ││  ││  ││
  -3 ─┤──┘╚──┘╚──┘╚──┘╚──
      └─────────────────────────► Frequency
        f  2f 3f 4f  (harmonics)
```

### Key Metrics Interpretation

#### Gain Range
| Range | Interpretation |
|-------|----------------|
| > 30 dB | Very strong filtering (steep slopes) |
| 20-30 dB | Strong filtering (typical for resonant filters) |
| 10-20 dB | Moderate filtering |
| 6-10 dB | Gentle filtering (EQ-style) |
| < 6 dB | Minimal filtering (bypass-like) |

#### Cutoff Frequency
- **Definition:** Frequency where gain drops 3dB below maximum
- **Typical Values:**
  - Lowpass: 100Hz - 10kHz
  - Highpass: 20Hz - 5kHz
  - Bandpass: Center frequency location
- **Accuracy:** Should be within ±5% of expected value

#### Resonance Peak
- **Definition:** Maximum gain above average response
- **Interpretation:**
  - 0-3 dB: Low Q (gentle resonance)
  - 3-6 dB: Moderate Q (musical resonance)
  - 6-12 dB: High Q (sharp resonance)
  - > 12 dB: Very high Q (self-oscillation capable)

---

## TROUBLESHOOTING

### Build Errors

#### Error: "JUCE modules not found"
**Solution:**
```bash
# Check JUCE path in build script
JUCE_DIR="/Users/Branden/branden/JUCE"  # Update this path
```

#### Error: "Engine source files not found"
**Solution:**
```bash
# Verify plugin source directory
ls ../JUCE_Plugin/Source/
```

#### Error: "Framework not found"
**Solution:**
```bash
# Ensure you're on macOS with required frameworks
xcode-select --install
```

### Runtime Errors

#### Error: "EngineFactory returned nullptr"
**Cause:** Engine not properly registered in factory
**Solution:** Check EngineFactory.cpp for engine ID registration

#### Error: "Unstable output at XXX Hz"
**Cause:** Numerical instability (NaN/Inf values)
**Solution:** Check filter implementation for:
- Divide-by-zero errors
- Coefficient calculations
- Resonance limiting

#### Error: "Filters correctly: NO"
**Cause:** Gain range < 6dB (insufficient filtering)
**Possible Reasons:**
- Mix parameter not at 100%
- Filter parameters not set correctly
- Bypass mode activated
- Implementation issue

### Plot Generation Errors

#### Error: "matplotlib not found"
**Solution:**
```bash
pip3 install matplotlib pandas numpy
# or
conda install matplotlib pandas numpy
```

#### Error: "No frequency response data found"
**Solution:**
```bash
# Ensure CSV files exist
ls frequency_response_engine_*.csv
# If not, re-run test
./test_frequency_response_8_14
```

---

## ADVANCED USAGE

### Custom Frequency Range

Edit `test_frequency_response_8_14.cpp`:
```cpp
// Change frequency range (Hz)
std::vector<float> testFrequencies =
    generateLogFrequencies(10.0f,   // Start: 10 Hz
                          30000.0f, // End: 30 kHz
                          200);     // Points: 200
```

### Custom Test Duration

Edit settling and measurement times:
```cpp
const int testLength = SAMPLE_RATE;        // 1.0 second (was 0.5)
const int settleSamples = SAMPLE_RATE / 5; // 200ms settle (was 100ms)
```

### Export to Other Formats

#### MATLAB/Octave
```bash
# CSV files are directly compatible
octave> data = csvread('frequency_response_engine_9.csv', 1, 0);
octave> semilogx(data(:,1), data(:,4));
```

#### Excel/Google Sheets
- Open CSV files directly
- Create scatter plot with logarithmic X-axis

#### Custom Python Analysis
```python
import pandas as pd
import numpy as np

# Load data
df = pd.read_csv('frequency_response_engine_9.csv')

# Calculate filter order from slope
# (slope in dB/octave = 20 × order)
high_freqs = df[df['Frequency_Hz'] > 5000]
slope = np.polyfit(np.log10(high_freqs['Frequency_Hz']),
                   high_freqs['Gain_dB'], 1)[0]
filter_order = abs(slope) / 20

print(f"Estimated filter order: {filter_order}")
```

---

## VALIDATION CRITERIA

### Pass/Fail Criteria

Each engine must meet these criteria to pass:

| Test | Criterion | Threshold |
|------|-----------|-----------|
| **Creation** | Engine instantiates | Non-null pointer |
| **Stability** | No numerical errors | No NaN/Inf values |
| **Filtering** | Gain variation | > 6 dB range |
| **Output** | Signal level | < 10.0 linear |
| **Data Quality** | Measurements complete | 100 data points |

### Expected Results Per Engine

#### Engine 8: VintageConsoleEQ_Studio
- **Type:** EQ (boost/cut)
- **Expected Gain Range:** 6-15 dB
- **Character:** Gentle slopes, musical response
- **Pass Criteria:** Visible boost/cut at EQ frequencies

#### Engine 9: LadderFilter
- **Type:** Lowpass
- **Expected Gain Range:** 20-40 dB
- **Character:** 24dB/oct slope, possible resonance peak
- **Pass Criteria:** Clear lowpass behavior, cutoff identifiable

#### Engine 10: StateVariableFilter
- **Type:** Multi-mode (LP/BP/HP/Notch)
- **Expected Gain Range:** 15-30 dB
- **Character:** 12dB/oct slope, clean response
- **Pass Criteria:** Appropriate filter shape for selected mode

#### Engine 11: FormantFilter
- **Type:** Bandpass (multiple peaks)
- **Expected Gain Range:** 10-25 dB
- **Character:** Multiple resonant peaks (formants)
- **Pass Criteria:** Visible formant peaks at vowel frequencies

#### Engine 12: EnvelopeFilter
- **Type:** Dynamic filter (AutoWah)
- **Expected Gain Range:** Variable (10-30 dB)
- **Character:** Envelope-dependent response
- **Pass Criteria:** Response varies with input level (may show average)

#### Engine 13: CombResonator
- **Type:** Comb (harmonic peaks/notches)
- **Expected Gain Range:** 6-20 dB
- **Character:** Equally-spaced harmonics
- **Pass Criteria:** Multiple peaks at harmonic intervals

#### Engine 14: VocalFormantFilter
- **Type:** Advanced formant (5 peaks)
- **Expected Gain Range:** 10-25 dB
- **Character:** Complex multi-peak response
- **Pass Criteria:** 5 formant peaks visible (F1-F5)

---

## TECHNICAL DETAILS

### Sample Rate and Resolution
- **Sample Rate:** 48,000 Hz (48 kHz)
- **Block Size:** 512 samples
- **Frequency Resolution:** Logarithmic spacing
  - More points in low frequencies (20-200 Hz)
  - Fewer points in high frequencies (10-20 kHz)
  - Matches human frequency perception

### Logarithmic Frequency Spacing
```
f[n] = f_min × (f_max / f_min)^(n / N)

Where:
  f_min = 20 Hz (lowest test frequency)
  f_max = 20,000 Hz (highest test frequency)
  N = 100 (total test points)
  n = 0 to N-1 (point index)
```

**Example frequencies:**
- Point 0: 20.0 Hz
- Point 10: 35.6 Hz
- Point 20: 63.3 Hz
- Point 30: 112.7 Hz
- Point 50: 357.0 Hz
- Point 70: 1,268 Hz
- Point 90: 11,274 Hz
- Point 99: 20,000 Hz

### RMS Measurement
```cpp
RMS = sqrt(Σ(x[i]²) / N)

Where:
  x[i] = sample value
  N = number of samples (excluding settling time)
```

### Gain Calculation
```cpp
Gain_dB = 20 × log10(RMS_output / RMS_input)

For input amplitude of 0.5 (-6 dB):
  0 dB gain = unity (0.5 in, 0.5 out)
  +6 dB gain = 2× amplitude (0.5 in, 1.0 out)
  -6 dB gain = 0.5× amplitude (0.5 in, 0.25 out)
```

---

## PERFORMANCE BENCHMARKS

### Expected Execution Times
| Stage | Duration | Notes |
|-------|----------|-------|
| Compilation | 20-40s | Depends on cached object files |
| Engine 8 test | 25-35s | 100 frequencies |
| Engine 9 test | 25-35s | 100 frequencies |
| Engine 10 test | 25-35s | 100 frequencies |
| Engine 11 test | 30-40s | Complex formant filter |
| Engine 12 test | 30-40s | Envelope detection overhead |
| Engine 13 test | 20-30s | Simple comb filter |
| Engine 14 test | 35-45s | Most complex (5 formants) |
| Plot generation | 5-15s | Depends on system |
| **Total** | **4-6 min** | Complete suite |

### Resource Usage
- **CPU:** 100% of single core during testing
- **Memory:** ~50-100 MB
- **Disk:** ~2-5 MB (CSV + plots)

---

## CONTINUOUS INTEGRATION

### Automated Testing
To integrate into CI/CD pipeline:

```bash
# Run test suite with exit code checking
./run_frequency_response_suite.sh
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "Frequency response tests PASSED"
    exit 0
else
    echo "Frequency response tests FAILED"
    # Archive logs and plots for debugging
    tar -czf test_artifacts.tar.gz \
        frequency_response_*.csv \
        frequency_response_plots/ \
        FREQUENCY_RESPONSE_REPORT.txt \
        frequency_response_test_output.log
    exit 1
fi
```

### Regression Detection
Compare current results with baseline:

```bash
# After running tests
python3 compare_frequency_responses.py \
    baseline_responses/ \
    frequency_response_*.csv
```

(Script `compare_frequency_responses.py` to be implemented)

---

## CONTRIBUTING

### Adding New Engines
To test additional engines:

1. Edit `test_frequency_response_8_14.cpp`:
   - Add engine ID to test list
   - Add engine-specific parameters

2. Edit `plot_frequency_response.py`:
   - Add engine to `ENGINES` dict
   - Assign color and name

3. Update `build_frequency_response_test.sh`:
   - Add engine source files to `ENGINE_SOURCES`

4. Rebuild and test:
   ```bash
   ./run_frequency_response_suite.sh
   ```

### Reporting Issues
If a test fails unexpectedly:

1. Save all output files
2. Check `frequency_response_test_output.log`
3. Review `FREQUENCY_RESPONSE_REPORT.txt`
4. Report with:
   - Engine ID and name
   - Error message
   - CSV data (if generated)
   - Expected vs. actual behavior

---

## REFERENCES

### Filter Theory
- Smith, Julius O. "Introduction to Digital Filters with Audio Applications"
- Orfanidis, Sophocles J. "Introduction to Signal Processing"
- Zölzer, Udo. "DAFX: Digital Audio Effects"

### Test Methodology
- Audio Engineering Society (AES) Standard AES17-2015
- ITU-R BS.1116-3: Methods for subjective assessment of audio systems

### Related Documents
- `FILTER_ENGINES_8_14_TEST_REPORT.md` - Previous test results
- `TESTING_METHODOLOGY.md` - Overall testing approach
- `FILTER_QUALITY_REPORT.md` - Quality assessment

---

## LICENSE

This test suite is part of the ChimeraPhoenix audio plugin project.

---

## CHANGELOG

### Version 1.0 (2025-10-11)
- Initial release
- 7 engines tested (8-14)
- 100 frequency points (20Hz-20kHz)
- CSV export
- Plot generation (individual, combined, grid)
- Comprehensive reporting
- Automated test runner

---

## CONTACT

**Project:** ChimeraPhoenix Audio Plugin
**Test Suite:** Frequency Response Analysis
**Generated:** Claude Code Automated Test Suite
**Date:** October 11, 2025

---

**END OF DOCUMENTATION**
