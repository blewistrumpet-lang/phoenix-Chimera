#!/bin/bash

# Comprehensive Verification Test Suite for All 10 Fixed Engines
# Runs individual comprehensive tests and generates summary matrix

echo "========================================================================"
echo "         COMPREHENSIVE VERIFICATION - ALL 10 FIXED ENGINES"
echo "========================================================================"
echo ""
echo "Date: $(date)"
echo "System: $(uname -s) $(uname -r)"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Results tracking
declare -A results
declare -A details

# Test counter
total_tests=0
passed_tests=0

# Function to run a test
run_test() {
    local engine_num=$1
    local engine_name=$2
    local test_binary=$3

    echo "========================================================================"
    echo "Testing Engine $engine_num: $engine_name"
    echo "========================================================================"

    total_tests=$((total_tests + 1))

    # Compile test
    echo "Compiling $test_binary..."
    g++ -std=c++17 -O2 -I.. "${test_binary}.cpp" -o "$test_binary" 2>&1 | grep -v "warning"

    if [ $? -ne 0 ]; then
        echo -e "${RED}COMPILATION FAILED${NC}"
        results[$engine_num]="COMPILE FAIL"
        details[$engine_num]="Failed to compile test"
        return 1
    fi

    # Run test
    echo ""
    echo "Running test..."
    ./"$test_binary" > "${test_binary}_output.log" 2>&1
    test_result=$?

    # Display output
    cat "${test_binary}_output.log"
    echo ""

    if [ $test_result -eq 0 ]; then
        echo -e "${GREEN}✓ ENGINE $engine_num PASSED${NC}"
        results[$engine_num]="PASS"
        passed_tests=$((passed_tests + 1))

        # Extract key metrics from log
        metrics=$(grep -E "(THD|CPU|Latency|Width|RMS|Max Output)" "${test_binary}_output.log" | head -5)
        details[$engine_num]="$metrics"
    else
        echo -e "${RED}✗ ENGINE $engine_num FAILED${NC}"
        results[$engine_num]="FAIL"

        # Extract failure info
        failure=$(grep -E "(FAIL|Error|error)" "${test_binary}_output.log" | head -3)
        details[$engine_num]="$failure"
    fi

    echo ""
    sleep 1
}

# Run all comprehensive tests
echo "Starting comprehensive verification tests..."
echo ""

run_test 32 "DetuneDoubler" "test_engine32_comprehensive"
run_test 52 "SpectralGate" "test_engine52_comprehensive"
run_test 49 "PhasedVocoder" "test_engine49_comprehensive"
run_test 33 "IntelligentHarmonizer" "test_engine33_comprehensive"
run_test 41 "ConvolutionReverb" "test_engine41_comprehensive"
run_test 40 "ShimmerReverb" "test_engine40_comprehensive"
run_test 6 "DynamicEQ" "test_engine6_comprehensive"
run_test 20 "MuffFuzz" "test_engine20_comprehensive"
run_test 21 "RodentDistortion" "test_engine21_comprehensive"

# Debug cleanup verification
echo "========================================================================"
echo "Verifying Debug Cleanup - All Engines"
echo "========================================================================"
echo ""
echo "Checking for debug output in engine source files..."

total_tests=$((total_tests + 1))

debug_found=0
debug_files=""

# Check for common debug patterns in source files
for engine_file in ../src/engines/*.h; do
    if [ -f "$engine_file" ]; then
        # Check for cout, cerr, printf patterns
        if grep -n "std::cout\|std::cerr\|printf\|fprintf" "$engine_file" | grep -v "^[[:space:]]*//" > /dev/null; then
            debug_found=$((debug_found + 1))
            filename=$(basename "$engine_file")
            debug_files="$debug_files\n  - $filename"
        fi
    fi
done

if [ $debug_found -eq 0 ]; then
    echo -e "${GREEN}✓ NO DEBUG OUTPUT FOUND${NC}"
    results["DEBUG"]="PASS"
    passed_tests=$((passed_tests + 1))
else
    echo -e "${YELLOW}⚠ DEBUG OUTPUT FOUND in $debug_found files:${NC}"
    echo -e "$debug_files"
    results["DEBUG"]="WARNING"
fi

echo ""
sleep 1

# Generate Summary Matrix
echo ""
echo "========================================================================"
echo "                        VERIFICATION SUMMARY"
echo "========================================================================"
echo ""
printf "%-8s %-25s %-12s\n" "ENGINE" "NAME" "STATUS"
echo "------------------------------------------------------------------------"

# Individual engine results
printf "%-8s %-25s " "32" "DetuneDoubler"
if [ "${results[32]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[32]}${NC}"
fi

printf "%-8s %-25s " "52" "SpectralGate"
if [ "${results[52]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[52]}${NC}"
fi

printf "%-8s %-25s " "49" "PhasedVocoder"
if [ "${results[49]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[49]}${NC}"
fi

printf "%-8s %-25s " "33" "IntelligentHarmonizer"
if [ "${results[33]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[33]}${NC}"
fi

printf "%-8s %-25s " "41" "ConvolutionReverb"
if [ "${results[41]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[41]}${NC}"
fi

printf "%-8s %-25s " "40" "ShimmerReverb"
if [ "${results[40]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[40]}${NC}"
fi

printf "%-8s %-25s " "6" "DynamicEQ"
if [ "${results[6]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[6]}${NC}"
fi

printf "%-8s %-25s " "20" "MuffFuzz"
if [ "${results[20]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[20]}${NC}"
fi

printf "%-8s %-25s " "21" "RodentDistortion"
if [ "${results[21]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
else
    echo -e "${RED}✗ ${results[21]}${NC}"
fi

printf "%-8s %-25s " "DEBUG" "Debug Cleanup"
if [ "${results[DEBUG]}" = "PASS" ]; then
    echo -e "${GREEN}✓ PASS${NC}"
elif [ "${results[DEBUG]}" = "WARNING" ]; then
    echo -e "${YELLOW}⚠ WARNING${NC}"
else
    echo -e "${RED}✗ FAIL${NC}"
fi

echo "------------------------------------------------------------------------"
echo ""

# Overall statistics
pass_rate=$(awk "BEGIN {printf \"%.1f\", ($passed_tests/$total_tests)*100}")

echo "OVERALL RESULTS:"
echo "  Total Tests:  $total_tests"
echo "  Passed:       $passed_tests"
echo "  Failed:       $((total_tests - passed_tests))"
echo "  Pass Rate:    $pass_rate%"
echo ""

if [ $passed_tests -eq $total_tests ]; then
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}   ALL TESTS PASSED - VERIFICATION OK${NC}"
    echo -e "${GREEN}========================================${NC}"
    exit 0
else
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}   SOME TESTS FAILED - REVIEW NEEDED${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
