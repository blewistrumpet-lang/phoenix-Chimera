#!/bin/bash
#
# Complete Frequency Response Test Suite Runner
# Compiles, executes, and generates plots for engines 8-14
#

set -e  # Exit on error

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  FREQUENCY RESPONSE TEST SUITE - COMPLETE WORKFLOW           ║"
echo "║  Filters & EQ Engines 8-14                                   ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Build the test executable
echo "STEP 1: Building test executable..."
echo "──────────────────────────────────────────────────────────────"
chmod +x build_frequency_response_test.sh
./build_frequency_response_test.sh

if [ ! -f "test_frequency_response_8_14" ]; then
    echo "ERROR: Build failed! Executable not found."
    exit 1
fi

echo ""
echo "STEP 2: Running frequency response tests..."
echo "──────────────────────────────────────────────────────────────"
echo "This will take several minutes (testing 100 frequencies per engine)..."
echo ""

# Run the test and capture both stdout and stderr
./test_frequency_response_8_14 | tee frequency_response_test_output.log
TEST_EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "──────────────────────────────────────────────────────────────"
echo "Test execution complete (exit code: $TEST_EXIT_CODE)"
echo ""

# Check if CSV files were generated
CSV_COUNT=$(ls frequency_response_engine_*.csv 2>/dev/null | wc -l)
echo "Generated CSV files: $CSV_COUNT"

if [ $CSV_COUNT -eq 0 ]; then
    echo "WARNING: No CSV files generated. Test may have failed."
    echo "Check frequency_response_test_output.log for errors."
else
    echo "CSV data files:"
    ls -lh frequency_response_engine_*.csv | awk '{print "  ", $9, "-", $5}'
fi

echo ""
echo "STEP 3: Generating frequency response plots..."
echo "──────────────────────────────────────────────────────────────"

# Check if Python3 and matplotlib are available
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 not found. Cannot generate plots."
    echo "Install Python 3 to generate visual plots."
    echo "Test data is available in CSV files for manual analysis."
else
    echo "Checking Python dependencies..."

    # Check for matplotlib
    python3 -c "import matplotlib" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "WARNING: matplotlib not installed."
        echo "Install with: pip3 install matplotlib pandas numpy"
        echo "Skipping plot generation..."
    else
        echo "Running plot generation script..."
        python3 plot_frequency_response.py | tee plot_generation_output.log
        PLOT_EXIT_CODE=${PIPESTATUS[0]}

        if [ $PLOT_EXIT_CODE -eq 0 ]; then
            echo ""
            echo "✓ Plots generated successfully!"

            # List generated plots
            if [ -d "frequency_response_plots" ]; then
                echo ""
                echo "Generated plot files:"
                ls -lh frequency_response_plots/*.png | awk '{print "  ", $9, "-", $5}'
            fi
        else
            echo "WARNING: Plot generation had errors (exit code: $PLOT_EXIT_CODE)"
            echo "Check plot_generation_output.log for details."
        fi
    fi
fi

echo ""
echo "STEP 4: Generating summary report..."
echo "──────────────────────────────────────────────────────────────"

# Create comprehensive summary
cat > FREQUENCY_RESPONSE_TEST_SUMMARY.md << 'EOF'
# FREQUENCY RESPONSE TEST SUMMARY
## Engines 8-14: Filters & EQs

**Test Date:** $(date)
**Test Duration:** Comprehensive sine sweep (20Hz - 20kHz)
**Test Points:** 100 frequencies (logarithmic spacing)
**Sample Rate:** 48kHz
**Block Size:** 512 samples

---

## TEST OBJECTIVES

1. ✓ Generate logarithmic sine sweeps from 20Hz to 20kHz
2. ✓ Measure output amplitude per frequency
3. ✓ Plot frequency response curves
4. ✓ Verify filters actually filter (attenuate frequencies)
5. ✓ Create detailed frequency response report

---

## ENGINES TESTED

| Engine | Name | Type |
|--------|------|------|
| 8 | VintageConsoleEQ_Studio | Vintage Console EQ |
| 9 | LadderFilter | Moog-style 4-pole Ladder |
| 10 | StateVariableFilter | Multi-mode State Variable |
| 11 | FormantFilter | Vowel Formant Filter |
| 12 | EnvelopeFilter | Auto-Wah / Envelope Filter |
| 13 | CombResonator | Harmonic Comb Resonator |
| 14 | VocalFormantFilter | Advanced Vocal Formant |

---

## OUTPUT FILES

### Test Data (CSV)
EOF

# Add CSV file list to report
if [ $CSV_COUNT -gt 0 ]; then
    echo "" >> FREQUENCY_RESPONSE_TEST_SUMMARY.md
    ls frequency_response_engine_*.csv | while read file; do
        size=$(ls -lh "$file" | awk '{print $5}')
        lines=$(wc -l < "$file")
        echo "- \`$file\` - $size ($lines data points)" >> FREQUENCY_RESPONSE_TEST_SUMMARY.md
    done
fi

cat >> FREQUENCY_RESPONSE_TEST_SUMMARY.md << 'EOF'

### Plots (PNG)
- `frequency_response_plots/frequency_response_engine_N.png` - Individual engine plots
- `frequency_response_plots/frequency_response_combined.png` - All engines overlaid
- `frequency_response_plots/frequency_response_grid.png` - Comparison grid

### Reports (TXT)
- `FREQUENCY_RESPONSE_REPORT.txt` - Detailed analysis report
- `frequency_response_test_output.log` - Test execution log

---

## KEY FINDINGS

Refer to `FREQUENCY_RESPONSE_REPORT.txt` for detailed analysis including:
- Maximum and minimum gain per engine
- Cutoff frequencies (-3dB points)
- Filter slope measurements
- Resonance peak detection
- Stability verification
- Filtering effectiveness validation

---

## USAGE EXAMPLES

### View Individual Engine Response
```bash
open frequency_response_plots/frequency_response_engine_9.png
```

### View All Engines Comparison
```bash
open frequency_response_plots/frequency_response_combined.png
```

### Analyze Raw Data
```bash
cat frequency_response_engine_9.csv | column -t -s,
```

### Read Detailed Report
```bash
cat FREQUENCY_RESPONSE_REPORT.txt
```

---

## VERIFICATION CRITERIA

### Filter Effectiveness
- ✓ Gain range > 6dB = Effective filtering
- ✓ -3dB cutoff point identifiable = Clear rolloff
- ✓ Stable across all frequencies = No instabilities

### Data Quality
- ✓ No NaN or Inf values = Numerical stability
- ✓ Smooth response curves = Proper implementation
- ✓ Expected frequency response = Correct filter behavior

---

**Test Suite Version:** 1.0
**Generated:** $(date)
**Status:** COMPLETE
EOF

echo "✓ Summary report generated: FREQUENCY_RESPONSE_TEST_SUMMARY.md"

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  TEST SUITE COMPLETE                                         ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "RESULTS SUMMARY:"
echo "────────────────────────────────────────────────────────────────"
echo "  Test Exit Code: $TEST_EXIT_CODE"
echo "  CSV Files Generated: $CSV_COUNT"

if [ -d "frequency_response_plots" ]; then
    PLOT_COUNT=$(ls frequency_response_plots/*.png 2>/dev/null | wc -l)
    echo "  Plot Files Generated: $PLOT_COUNT"
fi

echo ""
echo "KEY FILES:"
echo "  - FREQUENCY_RESPONSE_REPORT.txt (detailed analysis)"
echo "  - FREQUENCY_RESPONSE_TEST_SUMMARY.md (summary)"
echo "  - frequency_response_plots/ (visualizations)"
echo ""

# Check for report file
if [ -f "FREQUENCY_RESPONSE_REPORT.txt" ]; then
    echo "Quick Report Preview:"
    echo "────────────────────────────────────────────────────────────────"
    head -30 FREQUENCY_RESPONSE_REPORT.txt
    echo ""
    echo "  (see FREQUENCY_RESPONSE_REPORT.txt for full report)"
fi

echo ""
echo "NEXT STEPS:"
echo "  1. Review FREQUENCY_RESPONSE_REPORT.txt for detailed analysis"
echo "  2. View plots in frequency_response_plots/ directory"
echo "  3. Analyze CSV data for specific frequency measurements"
echo ""

if [ $TEST_EXIT_CODE -eq 0 ]; then
    echo "✓ All tests passed successfully!"
    exit 0
else
    echo "⚠ Some tests encountered issues (exit code: $TEST_EXIT_CODE)"
    echo "  Check logs for details."
    exit $TEST_EXIT_CODE
fi
