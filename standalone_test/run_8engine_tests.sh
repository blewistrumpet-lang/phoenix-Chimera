#!/bin/bash
################################################################################
# run_8engine_tests.sh - Run regression tests on 8 modified engines
#
# Tests engines: 39, 40, 52, 32, 49, 20, 33, 41
# For each engine:
#   1. Generate impulse response
#   2. Measure peak/RMS
#   3. Check for NaN/Inf
#   4. Record results
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
RESULTS_FILE="$BUILD_DIR/manual_test_results.txt"

# Define engines to test
ENGINES=(39 40 52 32 49 20 33 41)
ENGINE_NAMES=("Spring Reverb" "Shimmer Reverb" "Pitch Shifter" "Harmonizer" "Detune Doubler" "Muff Fuzz" "Octave Up" "Convolution Reverb")

echo "═══════════════════════════════════════════════════════════════"
echo "  Manual Regression Test - 8 Modified Engines"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Initialize results file
echo "Manual Regression Test Results" > "$RESULTS_FILE"
echo "Generated: $(date)" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Check if we have impulse files
echo "Checking for existing impulse response files..."
echo ""

PASS_COUNT=0
FAIL_COUNT=0

for i in "${!ENGINES[@]}"; do
    ENGINE_ID=${ENGINES[$i]}
    ENGINE_NAME=${ENGINE_NAMES[$i]}
    IMPULSE_FILE="$BUILD_DIR/impulse_engine_${ENGINE_ID}.csv"

    printf "Engine %2d (%s): " "$ENGINE_ID" "$ENGINE_NAME"

    if [ ! -f "$IMPULSE_FILE" ]; then
        echo "✗ FAIL - No impulse response file"
        echo "Engine $ENGINE_ID ($ENGINE_NAME): FAIL - No impulse response" >> "$RESULTS_FILE"
        ((FAIL_COUNT++))
        continue
    fi

    # Check file is not empty
    FILE_SIZE=$(wc -c < "$IMPULSE_FILE")
    if [ "$FILE_SIZE" -lt 100 ]; then
        echo "✗ FAIL - Empty or corrupt file"
        echo "Engine $ENGINE_ID ($ENGINE_NAME): FAIL - Empty file" >> "$RESULTS_FILE"
        ((FAIL_COUNT++))
        continue
    fi

    # Use Python to analyze the file
    ANALYSIS=$(python3 << 'EOF'
import csv
import sys
import numpy as np

try:
    left_channel = []
    right_channel = []

    with open(sys.argv[1], 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                left_channel.append(float(row['L']))
                right_channel.append(float(row['R']))
            except:
                continue

    if len(left_channel) == 0:
        print("FAIL:Empty")
        sys.exit(1)

    left = np.array(left_channel)
    right = np.array(right_channel)

    # Check for NaN
    if np.any(np.isnan(left)) or np.any(np.isnan(right)):
        print("FAIL:NaN")
        sys.exit(1)

    # Check for Inf
    if np.any(np.isinf(left)) or np.any(np.isinf(right)):
        print("FAIL:Inf")
        sys.exit(1)

    # Calculate metrics
    peak = max(np.max(np.abs(left)), np.max(np.abs(right)))
    rms_left = np.sqrt(np.mean(left**2))
    rms_right = np.sqrt(np.mean(right**2))
    rms = (rms_left + rms_right) / 2.0

    # Check for silence
    if peak < 1e-6:
        print("FAIL:Silence")
        sys.exit(1)

    # Check for excessive level
    if peak > 10.0:
        print(f"FAIL:ExcessivePeak:{peak:.2f}")
        sys.exit(1)

    # Check for minimal output
    if rms < 1e-6:
        print("FAIL:NoSignal")
        sys.exit(1)

    print(f"PASS:{peak:.4f}:{rms:.6f}")
    sys.exit(0)

except Exception as e:
    print(f"FAIL:Error:{str(e)}")
    sys.exit(1)
EOF
"$IMPULSE_FILE"
)

    # Parse analysis result
    RESULT_CODE=$(echo "$ANALYSIS" | cut -d: -f1)

    if [ "$RESULT_CODE" = "PASS" ]; then
        PEAK=$(echo "$ANALYSIS" | cut -d: -f2)
        RMS=$(echo "$ANALYSIS" | cut -d: -f3)
        echo "✓ PASS (Peak: $PEAK, RMS: $RMS)"
        echo "Engine $ENGINE_ID ($ENGINE_NAME): PASS - Peak: $PEAK, RMS: $RMS" >> "$RESULTS_FILE"
        ((PASS_COUNT++))
    else
        REASON=$(echo "$ANALYSIS" | cut -d: -f2-)
        echo "✗ FAIL ($REASON)"
        echo "Engine $ENGINE_ID ($ENGINE_NAME): FAIL - $REASON" >> "$RESULTS_FILE"
        ((FAIL_COUNT++))
    fi
done

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "                         SUMMARY"
echo "═══════════════════════════════════════════════════════════════"
echo "  Total Engines:  8"
echo "  Passed:         $PASS_COUNT"
echo "  Failed:         $FAIL_COUNT"
echo "  Pass Rate:      $(awk "BEGIN {printf \"%.1f%%\", ($PASS_COUNT/8.0)*100}")"
echo ""

if [ $PASS_COUNT -eq 8 ]; then
    echo "  ✓ ALL TESTS PASSED - NO REGRESSIONS"
else
    echo "  ✗ REGRESSIONS DETECTED"
    echo ""
    echo "  Failed engines:"
    grep "FAIL" "$RESULTS_FILE" | grep -v "Generated:" || true
fi
echo ""

echo "Results saved to: $RESULTS_FILE"
echo ""

# Exit with proper code
if [ $PASS_COUNT -eq 8 ]; then
    exit 0
else
    exit 1
fi
