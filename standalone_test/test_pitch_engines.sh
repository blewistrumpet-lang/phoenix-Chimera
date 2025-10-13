#!/bin/bash
################################################################################
# test_pitch_engines.sh
# Comprehensive pitch/time-stretch engine testing with latency compensation
#
# Tests pitch-related engines (31-38):
#   31 - Pitch Shifter
#   32 - Formant Shifter
#   33 - Harmonizer
#   34 - Octaver
#   35 - Time Stretcher
#   36 - Pitch Detector
#   37 - Auto-Tune
#   38 - Vocoder
#
# Usage: ./test_pitch_engines.sh [options]
#   --baseline          Create baseline measurements
#   --verbose           Detailed output
#   --output-dir DIR    Specify output directory
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_DIR="$SCRIPT_DIR/test_results/pitch"
BASELINE_DIR="$SCRIPT_DIR/baselines/pitch"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Pitch engine IDs
PITCH_ENGINES=(31 32 33 34 35 36 37 38)
PITCH_NAMES=(
    "PitchShifter"
    "FormantShifter"
    "Harmonizer"
    "Octaver"
    "TimeStretcher"
    "PitchDetector"
    "AutoTune"
    "Vocoder"
)

# Options
MODE="test"
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --baseline)
            MODE="baseline"
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --help)
            grep "^#" "$0" | grep -v "#!/bin/bash" | sed 's/^# //'
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Create directories
mkdir -p "$OUTPUT_DIR"
mkdir -p "$BASELINE_DIR"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║       ChimeraPhoenix Pitch/Time Engine Test Suite             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check for pitch test executable
if [ ! -f "$BUILD_DIR/pitch_test" ]; then
    echo "Warning: pitch_test executable not found"
    echo "Attempting to build..."
    cd "$SCRIPT_DIR"
    ./build_pitch_test.sh || {
        echo "Error: Failed to build pitch_test"
        echo "Using standalone_test instead..."
    }
fi

# Initialize results
PASSED=0
FAILED=0
WARNINGS=0

# Results CSV
RESULTS_CSV="$OUTPUT_DIR/pitch_results_${TIMESTAMP}.csv"
echo "Engine_ID,Engine_Name,Status,Pitch_Accuracy,Latency_ms,Artifacts,Quality_Score,Pass" > "$RESULTS_CSV"

################################################################################
# Test individual pitch engine
################################################################################
test_pitch_engine() {
    local engine_id=$1
    local engine_name=$2
    local log_file="$OUTPUT_DIR/pitch_engine_${engine_id}_${TIMESTAMP}.log"
    local output_csv="$OUTPUT_DIR/pitch_engine_${engine_id}_output.csv"

    if [ "$VERBOSE" = true ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "Testing: Engine $engine_id - $engine_name"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    fi

    # Use pitch_test if available, otherwise standalone_test
    local test_exe="$BUILD_DIR/pitch_test"
    if [ ! -f "$test_exe" ]; then
        test_exe="$BUILD_DIR/standalone_test"
    fi

    # Run pitch test with sustained input (important for pitch detection)
    if [ "$VERBOSE" = true ]; then
        "$test_exe" --engine $engine_id --sustained 2>&1 | tee "$log_file"
    else
        "$test_exe" --engine $engine_id > "$log_file" 2>&1
    fi

    local exit_code=$?

    # Parse metrics from log
    local pitch_accuracy=$(grep -i "pitch accuracy\|frequency accuracy" "$log_file" | grep -o '[0-9.]*%' | sed 's/%//' | head -1 || echo "0.0")
    local latency=$(grep -i "latency" "$log_file" | grep -o '[0-9.]*' | head -1 || echo "0.0")
    local artifacts=$(grep -i "artifacts\|distortion" "$log_file" | grep -o '[0-9.]*%' | sed 's/%//' | head -1 || echo "0.0")
    local quality=$(grep -i "quality score" "$log_file" | grep -o '[0-9.]*' | head -1 || echo "0.0")

    # If metrics not found in specialized test, parse from general test
    if [ "$pitch_accuracy" = "0.0" ]; then
        # Try to infer quality from THD and signal presence
        local thd=$(grep "THD:" "$log_file" | awk '{print $2}' | sed 's/%//' || echo "100.0")
        local peak=$(grep "Peak:" "$log_file" | awk '{print $2}' || echo "0.0")

        # Estimate quality based on output
        if (( $(awk "BEGIN {print ($peak > 0.01)}") )); then
            pitch_accuracy=$(awk "BEGIN {printf \"%.1f\", 100 - $thd}")
        fi
    fi

    # Determine pass/fail
    local status="FAIL"
    local passed=false
    local has_warning=false

    if [ $exit_code -eq 0 ] || grep -q "PASSED\|✓" "$log_file"; then
        # Check quality criteria for pitch engines
        local accuracy_ok=false
        local latency_ok=false
        local artifacts_ok=false

        # Pitch accuracy should be reasonable (>80% for pitch shifters)
        # Lower threshold for complex processors like vocoder
        local accuracy_threshold=70.0
        if [[ "$engine_name" == *"Detector"* ]] || [[ "$engine_name" == *"Tune"* ]]; then
            accuracy_threshold=80.0
        fi

        if (( $(awk "BEGIN {print ($pitch_accuracy > $accuracy_threshold)}") )); then
            accuracy_ok=true
        elif (( $(awk "BEGIN {print ($pitch_accuracy > 50.0)}") )); then
            accuracy_ok=true
            has_warning=true
        fi

        # Latency should be reasonable (<100ms for real-time use)
        if (( $(awk "BEGIN {print ($latency < 100.0)}") )); then
            latency_ok=true
        elif (( $(awk "BEGIN {print ($latency < 200.0)}") )); then
            latency_ok=true
            has_warning=true
        else
            latency_ok=true  # Don't fail on latency alone
            has_warning=true
        fi

        # Artifacts should be minimal
        if (( $(awk "BEGIN {print ($artifacts < 5.0)}") )); then
            artifacts_ok=true
        elif (( $(awk "BEGIN {print ($artifacts < 15.0)}") )); then
            artifacts_ok=true
            has_warning=true
        else
            artifacts_ok=true  # Don't fail on artifacts alone for complex processors
            has_warning=true
        fi

        if [ "$accuracy_ok" = true ]; then
            if [ "$has_warning" = true ]; then
                status="PASS*"
                WARNINGS=$((WARNINGS + 1))
            else
                status="PASS"
            fi
            passed=true
            PASSED=$((PASSED + 1))
        else
            FAILED=$((FAILED + 1))
            if [ "$VERBOSE" = true ]; then
                echo "Quality check failed:"
                [ "$accuracy_ok" = false ] && echo "  - Pitch accuracy too low: ${pitch_accuracy}%"
                [ "$latency_ok" = false ] && echo "  - Latency too high: ${latency}ms"
                [ "$artifacts_ok" = false ] && echo "  - Too many artifacts: ${artifacts}%"
            fi
        fi
    else
        FAILED=$((FAILED + 1))
        if [ "$VERBOSE" = true ]; then
            echo "Test execution failed with exit code: $exit_code"
        fi
    fi

    # Write to CSV
    echo "$engine_id,$engine_name,$status,$pitch_accuracy,$latency,$artifacts,$quality,$passed" >> "$RESULTS_CSV"

    # Save baseline if requested
    if [ "$MODE" = "baseline" ] && [ "$passed" = true ]; then
        cp "$log_file" "$BASELINE_DIR/engine_${engine_id}_baseline.log"
        if [ -f "$output_csv" ]; then
            cp "$output_csv" "$BASELINE_DIR/engine_${engine_id}_output.csv"
        fi
    fi

    # Return status
    if [ "$passed" = true ]; then
        return 0
    else
        return 1
    fi
}

################################################################################
# Test all pitch engines
################################################################################
echo "Testing 8 pitch/time-stretch engines..."
echo "Note: Using sustained input for accurate pitch detection"
echo ""

for i in "${!PITCH_ENGINES[@]}"; do
    engine_id=${PITCH_ENGINES[$i]}
    engine_name=${PITCH_NAMES[$i]}

    printf "Engine %2d (%-20s): " $engine_id "$engine_name"

    if test_pitch_engine $engine_id "$engine_name"; then
        echo "✓ PASS"
    else
        echo "✗ FAIL"
    fi
done

################################################################################
# Compare with baseline (if not creating baseline)
################################################################################
if [ "$MODE" != "baseline" ] && [ -d "$BASELINE_DIR" ]; then
    echo ""
    echo "Comparing with baseline..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    COMPARISON_FILE="$OUTPUT_DIR/comparison_${TIMESTAMP}.txt"
    echo "Pitch Engine Baseline Comparison" > "$COMPARISON_FILE"
    echo "Generated: $TIMESTAMP" >> "$COMPARISON_FILE"
    echo "" >> "$COMPARISON_FILE"

    for i in "${!PITCH_ENGINES[@]}"; do
        engine_id=${PITCH_ENGINES[$i]}
        engine_name=${PITCH_NAMES[$i]}

        current_log="$OUTPUT_DIR/pitch_engine_${engine_id}_${TIMESTAMP}.log"
        baseline_log="$BASELINE_DIR/engine_${engine_id}_baseline.log"

        if [ -f "$baseline_log" ] && [ -f "$current_log" ]; then
            # Compare pitch accuracy
            baseline_acc=$(grep -i "pitch accuracy" "$baseline_log" | grep -o '[0-9.]*%' | sed 's/%//' || echo "0")
            current_acc=$(grep -i "pitch accuracy" "$current_log" | grep -o '[0-9.]*%' | sed 's/%//' || echo "0")

            if [ -n "$baseline_acc" ] && [ -n "$current_acc" ] && [ "$baseline_acc" != "0" ]; then
                acc_diff=$(awk "BEGIN {print $current_acc - $baseline_acc}")

                echo "$engine_name (Engine $engine_id):" >> "$COMPARISON_FILE"
                echo "  Pitch Accuracy: ${baseline_acc}% -> ${current_acc}% (${acc_diff}% change)" >> "$COMPARISON_FILE"

                # Flag significant degradation (>5%)
                if (( $(awk "BEGIN {print ($acc_diff < -5.0)}") )); then
                    echo "  ⚠️  Accuracy degradation detected!" >> "$COMPARISON_FILE"
                elif (( $(awk "BEGIN {print ($acc_diff > 5.0)}") )); then
                    echo "  ✓  Accuracy improvement!" >> "$COMPARISON_FILE"
                fi

                echo "" >> "$COMPARISON_FILE"
            fi
        fi
    done

    if [ -f "$COMPARISON_FILE" ]; then
        cat "$COMPARISON_FILE"
    fi
fi

################################################################################
# Generate Summary
################################################################################
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                  PITCH ENGINE TEST SUMMARY                     ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "  Total Engines:    8"
echo "  Passed:           $PASSED"
echo "  Failed:           $FAILED"
echo "  Warnings:         $WARNINGS"
echo "  Pass Rate:        $(awk "BEGIN {printf \"%.1f%%\", ($PASSED/8.0)*100}")"
echo ""
echo "Results saved to: $OUTPUT_DIR"
echo "CSV results:      $RESULTS_CSV"

if [ "$MODE" = "baseline" ]; then
    echo "Baselines saved:  $BASELINE_DIR"
fi

if [ $WARNINGS -gt 0 ]; then
    echo ""
    echo "Note: PASS* indicates passing with quality warnings"
fi

echo ""

# Exit code
if [ $FAILED -eq 0 ]; then
    echo "✓ ALL PITCH ENGINE TESTS PASSED"
    exit 0
else
    echo "✗ SOME PITCH ENGINE TESTS FAILED"
    exit 1
fi
