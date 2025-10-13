# How to Run Pitch & Time Effects Tests

## Quick Start (Once Build Resolved)

```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test

# Option 1: Build pitch_test
./build_pitch_test.sh

# Option 2: If build_pitch_test.sh fails, try manual build
cd build
clang++ -std=c++17 -O2 \
    ./obj/pitch_test.o \
    ./obj/EngineFactory.o \
    ./obj/juce_*.o \
    ./obj/*Engine*.o \
    -framework Accelerate -framework CoreAudio -framework CoreFoundation \
    -framework AudioToolbox -framework Cocoa -framework IOKit \
    -framework Security -framework QuartzCore -framework CoreImage \
    -framework CoreGraphics -framework CoreText -framework WebKit \
    -framework DiscRecording \
    -L/opt/homebrew/lib -lharfbuzz \
    -o pitch_test

# Run the tests
./pitch_test > pitch_test_results.txt 2>&1

# Check results
cat pitch_test_results.txt
ls -lh *.csv
```

---

## Expected Output Files

After running `./pitch_test`, you should see:

### CSV Files Generated

```
pitch_engine_31_accuracy.csv    (Detune Doubler pitch accuracy)
pitch_engine_32_accuracy.csv    (Pitch Shifter - CRITICAL)
pitch_engine_33_accuracy.csv    (Intelligent Harmonizer - may fail)
pitch_engine_49_accuracy.csv    (Pitch Shifter duplicate?)

delay_engine_34_timing.csv      (Tape Echo timing)
delay_engine_35_timing.csv      (Digital Delay timing)
delay_engine_36_timing.csv      (Magnetic Drum Echo timing)
delay_engine_37_timing.csv      (BBD timing)
delay_engine_38_timing.csv      (Buffer Repeat timing)
```

### Console Output

```
═══════════════════════════════════════════════════════════
  ChimeraPhoenix Pitch & Time Effects Analysis
═══════════════════════════════════════════════════════════

═══════════════════════════════════════════════════════════
  PITCH SHIFTING ENGINES
═══════════════════════════════════════════════════════════

Testing Engine 31: Detune Doubler...
  Testing pitch shifter engine 31: Detune Doubler
    Latency: 256 samples (5.33 ms)
    Target: 100.0 ms, Measured: 100.2 ms, Error: 0.2 ms
    ...

╔════════════════════════════════════════════════════════════╗
║  Engine 31: Detune Doubler                                 ║
╚════════════════════════════════════════════════════════════╝

PITCH ACCURACY:
  Average Error:   2.15 cents ⚠ FAIR
  Maximum Error:   5.32 cents
  Target:          ±1.0 cents (professional standard)

FORMANT PRESERVATION:
  Preserved:       ✓ YES
  Input Formants:  700 Hz  1200 Hz  2500 Hz
  Output Formants: 705 Hz  1195 Hz  2510 Hz

ARTIFACTS:
  THD:             0.42% ✓ EXCELLENT
  Aliasing:        ✓ None

LATENCY:
  Samples:         256
  Milliseconds:    5.33 ms

ALGORITHM:
  Type:            Time-domain (PSOLA/Granular)

OVERALL QUALITY:
  Score:           82.5/100
  Rating:          Excellent

═══════════════════════════════════════════════════════════
  TESTING COMPLETE
═══════════════════════════════════════════════════════════

Results saved to CSV files in current directory.
```

---

## Analyzing Results

### 1. Check Console Output

```bash
# View full output
less pitch_test_results.txt

# Check for crashes
grep -i "crash\|error\|exception" pitch_test_results.txt

# Find CRITICAL issues
grep "CRITICAL\|FAIL\|⚠" pitch_test_results.txt

# Summary statistics
grep "Score:\|Rating:" pitch_test_results.txt
```

### 2. Analyze CSV Files

#### Pitch Accuracy Analysis

```bash
# View pitch accuracy data
cat pitch_engine_32_accuracy.csv

# Expected format:
# InputFreq,OutputFreq,ExpectedFreq,ErrorCents
# 440.00,220.15,220.00,1.18
# 440.00,329.98,329.63,-1.83
# ...

# Calculate average error (requires Python/spreadsheet)
python3 << EOF
import csv
with open('pitch_engine_32_accuracy.csv') as f:
    reader = csv.DictReader(f)
    errors = [abs(float(row['ErrorCents'])) for row in reader]
    print(f"Average error: {sum(errors)/len(errors):.2f} cents")
    print(f"Maximum error: {max(errors):.2f} cents")
    print(f"Professional? {all(e < 1.0 for e in errors)}")
EOF
```

#### Delay Timing Analysis

```bash
# View delay timing data
cat delay_engine_34_timing.csv

# Expected format:
# TargetMs,MeasuredMs,ErrorMs
# 50.0,50.2,0.2
# 100.0,100.1,0.1
# ...

# Check accuracy
awk -F',' 'NR>1 {if ($3 > 1.0) print "❌ INACCURATE: " $0; else print "✓ ACCURATE: " $0}' delay_engine_34_timing.csv
```

### 3. Import to Spreadsheet

Open CSV files in Excel/Google Sheets/Numbers for:
- Graphing pitch error vs shift amount
- Timing accuracy visualization
- Statistical analysis
- Comparison between engines

---

## Interpreting Results

### Pitch Shifter Quality Grades

```
THD < 0.5%:     Professional ★★★★★
THD 0.5-1.0%:   Consumer     ★★★★☆
THD 1.0-3.0%:   Acceptable   ★★★☆☆
THD 3.0-5.0%:   Poor         ★★☆☆☆
THD > 5.0%:     Unacceptable ★☆☆☆☆

ChimeraPhoenix Engine 32: 8.673% THD = ★☆☆☆☆ (CRITICAL)
```

### Pitch Accuracy Grades

```
Error < 1 cent:    Professional ★★★★★
Error 1-5 cents:   Consumer     ★★★★☆
Error 5-10 cents:  Acceptable   ★★★☆☆
Error 10-20 cents: Poor         ★★☆☆☆
Error > 20 cents:  Broken       ★☆☆☆☆
```

### Delay Timing Grades

```
Error < 1ms:    Excellent ★★★★★
Error 1-5ms:    Good      ★★★★☆
Error 5-10ms:   Fair      ★★★☆☆
Error 10-20ms:  Poor      ★★☆☆☆
Error > 20ms:   Broken    ★☆☆☆☆
```

---

## Troubleshooting

### Test Fails to Build

**Issue:** Linker errors, undefined symbols

**Solution 1:** Rebuild all objects
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
rm -rf build/obj/*.o
./build_v2.sh
./build_pitch_test.sh
```

**Solution 2:** Check for missing engine files
```bash
# Verify all engine objects exist
ls build/obj/*Engine*.o
ls build/obj/EngineFactory.o

# If missing, check required_engines.txt
cat required_engines.txt
```

**Solution 3:** Manual linking
```bash
cd build
# Link with explicit object list
clang++ -std=c++17 -O2 \
    obj/pitch_test.o \
    obj/EngineFactory.o \
    obj/juce_core.o \
    obj/juce_audio_basics.o \
    obj/juce_dsp.o \
    obj/juce_events.o \
    obj/juce_audio_formats.o \
    obj/juce_audio_processors.o \
    obj/SheenBidi.o \
    $(find obj -name "*Engine*.o" -o -name "*Delay*.o" -o -name "*Reverb*.o") \
    -framework Accelerate -framework CoreAudio -framework CoreFoundation \
    -framework AudioToolbox -framework Cocoa -framework IOKit \
    -framework Security -framework QuartzCore -framework CoreImage \
    -framework CoreGraphics -framework CoreText -framework WebKit \
    -framework DiscRecording \
    -L/opt/homebrew/lib -lharfbuzz \
    -o pitch_test
```

### Test Crashes

**Issue:** Segmentation fault or crash during execution

**Common Causes:**
1. **IntelligentHarmonizer (Engine 33)** - Known to crash
2. **Null pointer in engine** - Check console for which engine crashed
3. **Buffer overflow** - Try with smaller test signals

**Debug Steps:**
```bash
# Run with debugger
lldb ./pitch_test
(lldb) run
# ... crash will show stack trace

# Or use crash logs
~/Library/Logs/DiagnosticReports/pitch_test_*.crash
```

**Workaround:** Comment out crashing engine
```cpp
// In pitch_test.cpp main():
// Skip Engine 33 if it crashes
if (id == 33) {
    std::cout << "  Skipping Engine 33 (known crash)\n";
    continue;
}
```

### No Output Files

**Issue:** pitch_test runs but no CSV files created

**Check:**
```bash
# Verify current directory
pwd  # Should be in build/

# Check file permissions
ls -la *.csv  # Should show CSV files
```

**Solution:**
```bash
# Run from correct directory
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/build
./pitch_test
```

### Incorrect Results

**Issue:** Results don't match expectations

**Verify Test Conditions:**
1. Sample rate: Should be 48000 Hz
2. Block size: Should be 512 samples
3. Test signal: Pure sine waves
4. Engine parameters: Check parameter mapping

**Debug Output:**
```cpp
// Add to pitch_test.cpp for debugging
std::cout << "Sample rate: " << sampleRate << std::endl;
std::cout << "Block size: " << blockSize << std::endl;
std::cout << "Input signal level: " << inputRMS << " dBFS" << std::endl;
```

---

## Advanced Usage

### Test Specific Engine Only

Modify `pitch_test.cpp main()`:

```cpp
// Test only Engine 32 (PitchShifter with known THD issue)
std::vector<std::pair<int, std::string>> pitchEngines = {
    {32, "Pitch Shifter (CRITICAL: THD 8.673%)"}
};
```

Recompile and run:
```bash
./build_pitch_test.sh
cd build
./pitch_test
```

### Custom Test Frequencies

Modify test frequency list in `testPitchShifter()`:

```cpp
// In pitch_test.cpp
std::vector<float> testFreqs = {
    55.0f,   // A1
    110.0f,  // A2
    220.0f,  // A3
    440.0f,  // A4
    880.0f,  // A5
    1760.0f  // A6
};
```

### Extended Delay Times

Modify test delays in `testDelayEngine()`:

```cpp
// Test longer delays
std::vector<float> testDelays = {
    100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f
};
```

### Higher Sample Rates

Test at different sample rates:

```cpp
const float sampleRate = 96000.0f;  // Instead of 48000.0f
```

Note: Requires engine support for higher sample rates.

---

## Automated Analysis Script

Save as `analyze_pitch_results.sh`:

```bash
#!/bin/bash

echo "═══════════════════════════════════════"
echo "  Pitch & Time Test Results Analysis"
echo "═══════════════════════════════════════"
echo

# Check for crashes
echo "Checking for crashes..."
if grep -qi "crash\|segfault\|exception" pitch_test_results.txt; then
    echo "❌ CRASHES DETECTED:"
    grep -i "crash\|segfault\|exception" pitch_test_results.txt
else
    echo "✓ No crashes detected"
fi
echo

# Count engines tested
echo "Engines tested:"
PITCH_COUNT=$(ls pitch_engine_*_accuracy.csv 2>/dev/null | wc -l)
DELAY_COUNT=$(ls delay_engine_*_timing.csv 2>/dev/null | wc -l)
echo "  Pitch shifters: $PITCH_COUNT"
echo "  Delays: $DELAY_COUNT"
echo "  Total: $((PITCH_COUNT + DELAY_COUNT))"
echo

# Check for CRITICAL issues
echo "Critical issues:"
if grep -q "CRITICAL\|✗ FAIL" pitch_test_results.txt; then
    grep "CRITICAL\|✗ FAIL" pitch_test_results.txt | head -5
else
    echo "  ✓ None found"
fi
echo

# Quality summary
echo "Quality Ratings:"
grep "Rating:" pitch_test_results.txt | sed 's/^/  /'
echo

echo "Detailed results in: pitch_test_results.txt"
echo "CSV files: $(ls *.csv 2>/dev/null | wc -l) files"
```

Run with:
```bash
chmod +x analyze_pitch_results.sh
./analyze_pitch_results.sh
```

---

## Python Analysis Script

Save as `analyze_csv.py`:

```python
#!/usr/bin/env python3
import csv
import glob
import sys
from statistics import mean, stdev

def analyze_pitch_csv(filename):
    """Analyze pitch accuracy CSV file"""
    print(f"\nAnalyzing {filename}...")

    with open(filename) as f:
        reader = csv.DictReader(f)
        errors = [abs(float(row['ErrorCents'])) for row in reader]

    if not errors:
        print("  No data found")
        return

    avg_error = mean(errors)
    max_error = max(errors)
    std_error = stdev(errors) if len(errors) > 1 else 0

    print(f"  Average error: {avg_error:.2f} cents")
    print(f"  Maximum error: {max_error:.2f} cents")
    print(f"  Std deviation: {std_error:.2f} cents")

    if avg_error < 1.0:
        print("  ★★★★★ Professional quality")
    elif avg_error < 5.0:
        print("  ★★★★☆ Consumer quality")
    elif avg_error < 10.0:
        print("  ★★★☆☆ Acceptable")
    else:
        print("  ★☆☆☆☆ Poor quality")

def analyze_delay_csv(filename):
    """Analyze delay timing CSV file"""
    print(f"\nAnalyzing {filename}...")

    with open(filename) as f:
        reader = csv.DictReader(f)
        errors = [abs(float(row['ErrorMs'])) for row in reader]

    if not errors:
        print("  No data found")
        return

    avg_error = mean(errors)
    max_error = max(errors)

    print(f"  Average error: {avg_error:.2f} ms")
    print(f"  Maximum error: {max_error:.2f} ms")

    if avg_error < 1.0:
        print("  ★★★★★ Excellent timing")
    elif avg_error < 5.0:
        print("  ★★★★☆ Good timing")
    elif avg_error < 10.0:
        print("  ★★★☆☆ Fair timing")
    else:
        print("  ★☆☆☆☆ Poor timing")

if __name__ == "__main__":
    print("═" * 50)
    print("  ChimeraPhoenix CSV Analysis")
    print("═" * 50)

    # Analyze pitch files
    pitch_files = glob.glob("pitch_engine_*_accuracy.csv")
    for f in sorted(pitch_files):
        analyze_pitch_csv(f)

    # Analyze delay files
    delay_files = glob.glob("delay_engine_*_timing.csv")
    for f in sorted(delay_files):
        analyze_delay_csv(f)

    print("\n" + "═" * 50)
```

Run with:
```bash
chmod +x analyze_csv.py
./analyze_csv.py
```

---

## Final Checklist

Before running tests:

- [ ] All engines built successfully
- [ ] pitch_test executable created
- [ ] No linker errors
- [ ] Correct working directory (build/)
- [ ] Write permissions in output directory

After running tests:

- [ ] pitch_test_results.txt created
- [ ] CSV files generated (9 files expected)
- [ ] No crashes in output
- [ ] Review critical issues
- [ ] Analyze results with scripts
- [ ] Compare to professional standards

---

## Expected Test Duration

- **Engine 31 (Detune Doubler):** ~5 seconds
- **Engine 32 (Pitch Shifter):** ~10 seconds
- **Engine 33 (Harmonizer):** ~10 seconds (or crash)
- **Engine 49 (Pitch Shifter 2):** ~10 seconds
- **Engines 34-38 (Delays):** ~3 seconds each

**Total runtime:** ~60 seconds for all engines

---

## Success Criteria

Tests are successful if:

1. ✓ All tests complete without crashing
2. ✓ CSV files generated for all engines
3. ✓ Pitch accuracy measured (all test frequencies)
4. ✓ Delay timing verified (all test times)
5. ✓ THD values calculated
6. ✓ Results match expected behavior

Tests reveal issues if:

- ✗ PitchShifter THD confirmed > 5%
- ✗ Harmonizer crashes (Engine 33)
- ✗ Pitch errors > 10 cents
- ✗ Delay timing errors > 10ms
- ✗ Any engine produces NaN/Inf

---

## Next Steps After Testing

1. **Review PITCH_TIME_QUALITY_REPORT.md**
   - Detailed root cause analysis
   - Professional standards comparison
   - Recommended fixes

2. **Implement Fixes**
   - Start with CRITICAL issues (PitchShifter THD)
   - Move to HIGH issues (Harmonizer crash)
   - Address MEDIUM/LOW issues

3. **Retest**
   - Run pitch_test again after fixes
   - Compare before/after CSV results
   - Verify improvements

4. **Document**
   - Update engine documentation
   - Note parameter ranges
   - Document known limitations

---

**Created:** October 10, 2025
**Framework Version:** 3.0
**Status:** Ready to run (pending build resolution)
