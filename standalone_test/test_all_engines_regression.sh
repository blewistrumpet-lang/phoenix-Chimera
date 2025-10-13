#!/bin/bash
################################################################################
# test_all_engines_regression.sh
# Comprehensive regression test suite for all 56 engines
#
# Usage: ./test_all_engines_regression.sh [options]
#   --baseline          Create new baseline files
#   --compare           Compare against baseline (default)
#   --parallel          Run tests in parallel
#   --output-dir DIR    Specify output directory (default: test_results)
#   --timeout SECONDS   Timeout per engine (default: 30)
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT_DIR="$SCRIPT_DIR/test_results"
BASELINE_DIR="$SCRIPT_DIR/baselines"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$OUTPUT_DIR/run_$TIMESTAMP"

# Options
MODE="compare"
PARALLEL=false
TIMEOUT=30

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --baseline)
            MODE="baseline"
            shift
            ;;
        --compare)
            MODE="compare"
            shift
            ;;
        --parallel)
            PARALLEL=true
            shift
            ;;
        --output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --timeout)
            TIMEOUT="$2"
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
mkdir -p "$RUN_DIR"
mkdir -p "$BASELINE_DIR"

# Check if executable exists
if [ ! -f "$BUILD_DIR/standalone_test" ]; then
    echo "Error: standalone_test executable not found in $BUILD_DIR"
    echo "Run ./build_all.sh first"
    exit 1
fi

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     ChimeraPhoenix Comprehensive Regression Test Suite         ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "Mode:        $MODE"
echo "Output:      $RUN_DIR"
echo "Timeout:     ${TIMEOUT}s per engine"
echo "Parallel:    $PARALLEL"
echo ""

# Initialize results
PASSED=0
FAILED=0
TIMEOUT_ENGINES=""
REGRESSED_ENGINES=""
IMPROVED_ENGINES=""

# JSON results file
RESULTS_JSON="$RUN_DIR/results.json"
echo "{" > "$RESULTS_JSON"
echo "  \"timestamp\": \"$TIMESTAMP\"," >> "$RESULTS_JSON"
echo "  \"mode\": \"$MODE\"," >> "$RESULTS_JSON"
echo "  \"results\": [" >> "$RESULTS_JSON"

################################################################################
# Test individual engine
################################################################################
test_engine() {
    local engine_id=$1
    local result_file="$RUN_DIR/engine_${engine_id}.log"
    local json_file="$RUN_DIR/engine_${engine_id}.json"
    local csv_file="$RUN_DIR/engine_${engine_id}_output.csv"

    echo "Testing Engine $engine_id..." >> "$result_file"

    # Run test with timeout
    timeout ${TIMEOUT}s "$BUILD_DIR/standalone_test" --engine $engine_id > "$result_file" 2>&1
    local exit_code=$?

    # Parse results
    local engine_name=$(grep "Testing Engine" "$result_file" | head -1 | cut -d: -f2 | xargs || echo "Unknown")
    local passed=false
    local timeout=false
    local peak=0.0
    local rms=0.0
    local thd=0.0
    local cpu=0.0

    if [ $exit_code -eq 124 ]; then
        # Timeout
        timeout=true
        echo "TIMEOUT" >> "$result_file"
    elif grep -q "✓ PASSED" "$result_file"; then
        passed=true
        # Extract metrics
        peak=$(grep "Peak:" "$result_file" | awk '{print $2}' || echo "0.0")
        rms=$(grep "RMS:" "$result_file" | awk '{print $2}' || echo "0.0")
        thd=$(grep "THD:" "$result_file" | awk '{print $2}' | sed 's/%//' || echo "0.0")
        cpu=$(grep "CPU:" "$result_file" | awk '{print $2}' | sed 's/%//' || echo "0.0")
    fi

    # Create JSON result
    cat > "$json_file" <<EOF
{
  "engine_id": $engine_id,
  "engine_name": "$engine_name",
  "timestamp": "$TIMESTAMP",
  "passed": $passed,
  "timeout": $timeout,
  "metrics": {
    "peak_level": $peak,
    "rms_level": $rms,
    "thd_percent": $thd,
    "cpu_percent": $cpu
  }
}
EOF

    # Return status
    if [ "$timeout" = "true" ]; then
        return 2
    elif [ "$passed" = "true" ]; then
        return 0
    else
        return 1
    fi
}

################################################################################
# Run all engine tests
################################################################################
echo "Running tests on all 56 engines..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

first_result=true

for engine_id in {1..56}; do
    printf "Engine %2d: " $engine_id

    # Run test
    if test_engine $engine_id; then
        echo "✓ PASS"
        PASSED=$((PASSED + 1))

        # Add to JSON (comma separator for all but first)
        if [ "$first_result" = "false" ]; then
            echo "    ," >> "$RESULTS_JSON"
        fi
        first_result=false
        cat "$RUN_DIR/engine_${engine_id}.json" | sed 's/^/    /' >> "$RESULTS_JSON"

    elif [ $? -eq 2 ]; then
        echo "⏱ TIMEOUT"
        FAILED=$((FAILED + 1))
        TIMEOUT_ENGINES="$TIMEOUT_ENGINES $engine_id"

        if [ "$first_result" = "false" ]; then
            echo "    ," >> "$RESULTS_JSON"
        fi
        first_result=false
        cat "$RUN_DIR/engine_${engine_id}.json" | sed 's/^/    /' >> "$RESULTS_JSON"

    else
        echo "✗ FAIL"
        FAILED=$((FAILED + 1))

        if [ "$first_result" = "false" ]; then
            echo "    ," >> "$RESULTS_JSON"
        fi
        first_result=false
        cat "$RUN_DIR/engine_${engine_id}.json" | sed 's/^/    /' >> "$RESULTS_JSON"
    fi
done

echo "" >> "$RESULTS_JSON"
echo "  ]," >> "$RESULTS_JSON"
echo "  \"summary\": {" >> "$RESULTS_JSON"
echo "    \"total\": 56," >> "$RESULTS_JSON"
echo "    \"passed\": $PASSED," >> "$RESULTS_JSON"
echo "    \"failed\": $FAILED" >> "$RESULTS_JSON"
echo "  }" >> "$RESULTS_JSON"
echo "}" >> "$RESULTS_JSON"

################################################################################
# Handle baseline mode
################################################################################
if [ "$MODE" = "baseline" ]; then
    echo ""
    echo "Creating baseline files..."

    for engine_id in {1..56}; do
        if [ -f "$RUN_DIR/engine_${engine_id}.json" ]; then
            cp "$RUN_DIR/engine_${engine_id}.json" "$BASELINE_DIR/engine_${engine_id}_baseline.json"
            if [ -f "$RUN_DIR/engine_${engine_id}_output.csv" ]; then
                cp "$RUN_DIR/engine_${engine_id}_output.csv" "$BASELINE_DIR/engine_${engine_id}_output.csv"
            fi
        fi
    done

    echo "Baseline files created in: $BASELINE_DIR"
fi

################################################################################
# Compare against baseline
################################################################################
if [ "$MODE" = "compare" ] && [ -d "$BASELINE_DIR" ]; then
    echo ""
    echo "Comparing against baseline..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    COMPARISON_FILE="$RUN_DIR/comparison.txt"
    echo "Regression Comparison Report" > "$COMPARISON_FILE"
    echo "Generated: $TIMESTAMP" >> "$COMPARISON_FILE"
    echo "" >> "$COMPARISON_FILE"

    for engine_id in {1..56}; do
        current="$RUN_DIR/engine_${engine_id}.json"
        baseline="$BASELINE_DIR/engine_${engine_id}_baseline.json"

        if [ -f "$baseline" ] && [ -f "$current" ]; then
            # Compare pass/fail status
            baseline_passed=$(grep '"passed"' "$baseline" | grep -o 'true\|false')
            current_passed=$(grep '"passed"' "$current" | grep -o 'true\|false')

            if [ "$baseline_passed" = "true" ] && [ "$current_passed" = "false" ]; then
                echo "Engine $engine_id: REGRESSION (was passing, now failing)" >> "$COMPARISON_FILE"
                REGRESSED_ENGINES="$REGRESSED_ENGINES $engine_id"
            elif [ "$baseline_passed" = "false" ] && [ "$current_passed" = "true" ]; then
                echo "Engine $engine_id: IMPROVEMENT (was failing, now passing)" >> "$COMPARISON_FILE"
                IMPROVED_ENGINES="$IMPROVED_ENGINES $engine_id"
            fi

            # Compare THD
            baseline_thd=$(grep '"thd_percent"' "$baseline" | grep -o '[0-9.]*' | head -1)
            current_thd=$(grep '"thd_percent"' "$current" | grep -o '[0-9.]*' | head -1)

            if [ -n "$baseline_thd" ] && [ -n "$current_thd" ]; then
                thd_diff=$(awk "BEGIN {print $current_thd - $baseline_thd}")
                if (( $(awk "BEGIN {print ($thd_diff > 0.1)}") )); then
                    echo "Engine $engine_id: THD increased by ${thd_diff}%" >> "$COMPARISON_FILE"
                fi
            fi

            # Compare CPU
            baseline_cpu=$(grep '"cpu_percent"' "$baseline" | grep -o '[0-9.]*' | head -1)
            current_cpu=$(grep '"cpu_percent"' "$current" | grep -o '[0-9.]*' | head -1)

            if [ -n "$baseline_cpu" ] && [ -n "$current_cpu" ]; then
                cpu_diff=$(awk "BEGIN {print $current_cpu - $baseline_cpu}")
                if (( $(awk "BEGIN {print ($cpu_diff > 1.0)}") )); then
                    echo "Engine $engine_id: CPU usage increased by ${cpu_diff}%" >> "$COMPARISON_FILE"
                fi
            fi
        fi
    done

    if [ -n "$REGRESSED_ENGINES" ]; then
        echo ""
        echo "⚠️  REGRESSIONS DETECTED: $REGRESSED_ENGINES"
    fi

    if [ -n "$IMPROVED_ENGINES" ]; then
        echo "✓  IMPROVEMENTS: $IMPROVED_ENGINES"
    fi
fi

################################################################################
# Print Summary
################################################################################
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                      TEST SUMMARY                              ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "  Total Engines:    56"
echo "  Passed:           $PASSED"
echo "  Failed:           $FAILED"
echo "  Pass Rate:        $(awk "BEGIN {printf \"%.1f%%\", ($PASSED/56.0)*100}")"
echo ""

if [ -n "$TIMEOUT_ENGINES" ]; then
    echo "  Timeout Engines: $TIMEOUT_ENGINES"
    echo ""
fi

if [ -n "$REGRESSED_ENGINES" ]; then
    echo "  ⚠️  Regressions:  $REGRESSED_ENGINES"
    echo ""
fi

if [ -n "$IMPROVED_ENGINES" ]; then
    echo "  ✓  Improvements:  $IMPROVED_ENGINES"
    echo ""
fi

echo "Results saved to: $RUN_DIR"
echo "JSON results:     $RESULTS_JSON"

# Exit with appropriate code
if [ $FAILED -eq 0 ] && [ -z "$REGRESSED_ENGINES" ]; then
    echo ""
    echo "✓ ALL TESTS PASSED"
    exit 0
elif [ -n "$REGRESSED_ENGINES" ]; then
    echo ""
    echo "✗ REGRESSIONS DETECTED"
    exit 1
else
    echo ""
    echo "✗ SOME TESTS FAILED"
    exit 1
fi
