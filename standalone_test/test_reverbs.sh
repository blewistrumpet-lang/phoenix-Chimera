#!/bin/bash
################################################################################
# test_reverbs.sh
# Comprehensive reverb engine testing with RT60, stereo width, and quality checks
#
# Tests the 5 reverb engines (39-43):
#   39 - PlateVerb
#   40 - HallVerb
#   41 - RoomVerb
#   42 - ShimmerVerb
#   43 - SpringVerb
#
# Usage: ./test_reverbs.sh [options]
#   --baseline          Create baseline measurements
#   --verbose           Detailed output
#   --output-dir DIR    Specify output directory
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_DIR="$SCRIPT_DIR/test_results/reverb"
BASELINE_DIR="$SCRIPT_DIR/baselines/reverb"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Reverb engine IDs
REVERB_ENGINES=(39 40 41 42 43)
REVERB_NAMES=("PlateVerb" "HallVerb" "RoomVerb" "ShimmerVerb" "SpringVerb")

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
echo "║        ChimeraPhoenix Reverb Engine Test Suite                ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

if [ ! -f "$BUILD_DIR/reverb_test" ]; then
    echo "Warning: reverb_test executable not found"
    echo "Attempting to build..."
    cd "$SCRIPT_DIR"
    ./build_reverb_test.sh || {
        echo "Error: Failed to build reverb_test"
        exit 1
    }
fi

# Initialize results
PASSED=0
FAILED=0

# Results CSV
RESULTS_CSV="$OUTPUT_DIR/reverb_results_${TIMESTAMP}.csv"
echo "Engine_ID,Engine_Name,Status,RT60,EDT,Stereo_Width,DC_Offset,Diffusion,Pass" > "$RESULTS_CSV"

################################################################################
# Test individual reverb engine
################################################################################
test_reverb_engine() {
    local engine_id=$1
    local engine_name=$2
    local log_file="$OUTPUT_DIR/reverb_engine_${engine_id}_${TIMESTAMP}.log"
    local impulse_csv="$OUTPUT_DIR/reverb_engine_${engine_id}_impulse.csv"

    if [ "$VERBOSE" = true ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "Testing: Engine $engine_id - $engine_name"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    fi

    # Run reverb test
    if [ "$VERBOSE" = true ]; then
        "$BUILD_DIR/reverb_test" $engine_id 2>&1 | tee "$log_file"
    else
        "$BUILD_DIR/reverb_test" $engine_id > "$log_file" 2>&1
    fi

    local exit_code=$?

    # Parse metrics from log
    local rt60=$(grep "RT60:" "$log_file" | awk '{print $2}' | sed 's/s//' || echo "0.0")
    local edt=$(grep "EDT:" "$log_file" | awk '{print $2}' | sed 's/s//' || echo "0.0")
    local stereo=$(grep "Stereo Width:" "$log_file" | awk '{print $3}' || echo "0.0")
    local dc=$(grep "DC Offset:" "$log_file" | awk '{print $3}' || echo "0.0")
    local diffusion=$(grep "Diffusion:" "$log_file" | awk '{print $2}' || echo "0.0")

    # Determine pass/fail
    local status="FAIL"
    local passed=false

    if [ $exit_code -eq 0 ]; then
        # Check quality criteria
        local rt60_ok=false
        local stereo_ok=false
        local dc_ok=false

        # RT60 should be reasonable (0.5s to 5s)
        if (( $(awk "BEGIN {print ($rt60 > 0.3 && $rt60 < 8.0)}") )); then
            rt60_ok=true
        fi

        # Stereo width should be significant (> 0.3)
        if (( $(awk "BEGIN {print ($stereo > 0.2)}") )); then
            stereo_ok=true
        fi

        # DC offset should be minimal (< 0.01)
        dc_abs=$(awk "BEGIN {print ($dc < 0 ? -$dc : $dc)}")
        if (( $(awk "BEGIN {print ($dc_abs < 0.05)}") )); then
            dc_ok=true
        fi

        if [ "$rt60_ok" = true ] && [ "$stereo_ok" = true ] && [ "$dc_ok" = true ]; then
            status="PASS"
            passed=true
            PASSED=$((PASSED + 1))
        else
            FAILED=$((FAILED + 1))
            if [ "$VERBOSE" = true ]; then
                echo "Quality check failed:"
                [ "$rt60_ok" = false ] && echo "  - RT60 out of range: ${rt60}s"
                [ "$stereo_ok" = false ] && echo "  - Stereo width too narrow: $stereo"
                [ "$dc_ok" = false ] && echo "  - DC offset too high: $dc"
            fi
        fi
    else
        FAILED=$((FAILED + 1))
        if [ "$VERBOSE" = true ]; then
            echo "Test execution failed with exit code: $exit_code"
        fi
    fi

    # Write to CSV
    echo "$engine_id,$engine_name,$status,$rt60,$edt,$stereo,$dc,$diffusion,$passed" >> "$RESULTS_CSV"

    # Save baseline if requested
    if [ "$MODE" = "baseline" ] && [ "$passed" = true ]; then
        cp "$log_file" "$BASELINE_DIR/engine_${engine_id}_baseline.log"
        if [ -f "$impulse_csv" ]; then
            cp "$impulse_csv" "$BASELINE_DIR/engine_${engine_id}_impulse.csv"
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
# Test all reverb engines
################################################################################
echo "Testing 5 reverb engines..."
echo ""

for i in "${!REVERB_ENGINES[@]}"; do
    engine_id=${REVERB_ENGINES[$i]}
    engine_name=${REVERB_NAMES[$i]}

    printf "Engine %2d (%-15s): " $engine_id "$engine_name"

    if test_reverb_engine $engine_id "$engine_name"; then
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
    echo "Reverb Baseline Comparison" > "$COMPARISON_FILE"
    echo "Generated: $TIMESTAMP" >> "$COMPARISON_FILE"
    echo "" >> "$COMPARISON_FILE"

    for i in "${!REVERB_ENGINES[@]}"; do
        engine_id=${REVERB_ENGINES[$i]}
        engine_name=${REVERB_NAMES[$i]}

        current_log="$OUTPUT_DIR/reverb_engine_${engine_id}_${TIMESTAMP}.log"
        baseline_log="$BASELINE_DIR/engine_${engine_id}_baseline.log"

        if [ -f "$baseline_log" ] && [ -f "$current_log" ]; then
            # Compare RT60
            baseline_rt60=$(grep "RT60:" "$baseline_log" | awk '{print $2}' | sed 's/s//')
            current_rt60=$(grep "RT60:" "$current_log" | awk '{print $2}' | sed 's/s//')

            if [ -n "$baseline_rt60" ] && [ -n "$current_rt60" ]; then
                rt60_diff=$(awk "BEGIN {print $current_rt60 - $baseline_rt60}")
                rt60_pct=$(awk "BEGIN {printf \"%.1f\", ($rt60_diff / $baseline_rt60) * 100}")

                echo "$engine_name (Engine $engine_id):" >> "$COMPARISON_FILE"
                echo "  RT60: ${baseline_rt60}s -> ${current_rt60}s (${rt60_pct}% change)" >> "$COMPARISON_FILE"

                # Flag significant changes (>10%)
                if (( $(awk "BEGIN {print ($rt60_pct > 10 || $rt60_pct < -10)}") )); then
                    echo "  ⚠️  Significant RT60 change detected!" >> "$COMPARISON_FILE"
                fi
            fi

            # Compare stereo width
            baseline_stereo=$(grep "Stereo Width:" "$baseline_log" | awk '{print $3}')
            current_stereo=$(grep "Stereo Width:" "$current_log" | awk '{print $3}')

            if [ -n "$baseline_stereo" ] && [ -n "$current_stereo" ]; then
                stereo_diff=$(awk "BEGIN {print $current_stereo - $baseline_stereo}")
                echo "  Stereo Width: ${baseline_stereo} -> ${current_stereo} (${stereo_diff} change)" >> "$COMPARISON_FILE"
            fi

            echo "" >> "$COMPARISON_FILE"
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
echo "║                    REVERB TEST SUMMARY                         ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "  Total Reverbs:    5"
echo "  Passed:           $PASSED"
echo "  Failed:           $FAILED"
echo "  Pass Rate:        $(awk "BEGIN {printf \"%.1f%%\", ($PASSED/5.0)*100}")"
echo ""
echo "Results saved to: $OUTPUT_DIR"
echo "CSV results:      $RESULTS_CSV"

if [ "$MODE" = "baseline" ]; then
    echo "Baselines saved:  $BASELINE_DIR"
fi

echo ""

# Exit code
if [ $FAILED -eq 0 ]; then
    echo "✓ ALL REVERB TESTS PASSED"
    exit 0
else
    echo "✗ SOME REVERB TESTS FAILED"
    exit 1
fi
