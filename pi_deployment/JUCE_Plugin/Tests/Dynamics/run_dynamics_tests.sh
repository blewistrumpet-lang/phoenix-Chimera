#!/bin/bash

# =============================================================================
# DYNAMICS Test Suite Runner
# Comprehensive testing for all 6 dynamics engines
# =============================================================================

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TEST_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Dynamics"
SOURCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
JUCE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/JuceLibraryCode"

# Compiler settings
CXX="g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra"
INCLUDES="-I${SOURCE_DIR} -I${JUCE_DIR}"
LIBS="-framework CoreAudio -framework CoreMIDI -framework Cocoa -framework IOKit"

# Test files and their corresponding engine names
declare -a TEST_FILES=(
    "VintageOptoCompressor_Test.cpp"
    "ClassicCompressor_Test.cpp" 
    "TransientShaper_Test.cpp"
    "NoiseGate_Test.cpp"
    "MasteringLimiter_Test.cpp"
    "DynamicEQ_Test.cpp"
)

declare -a ENGINE_NAMES=(
    "Vintage Opto Compressor"
    "Classic Compressor"
    "Transient Shaper"
    "Noise Gate" 
    "Mastering Limiter"
    "Dynamic EQ"
)

declare -a ENGINE_IDS=(
    "ENGINE_OPTO_COMPRESSOR"
    "ENGINE_VCA_COMPRESSOR"
    "ENGINE_TRANSIENT_SHAPER"
    "ENGINE_NOISE_GATE"
    "ENGINE_MASTERING_LIMITER"
    "ENGINE_DYNAMIC_EQ"
)

# Results tracking
TOTAL_TESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0
FAILED_ENGINES=()

# Report file
REPORT_FILE="${TEST_DIR}/DynamicsTestSuite_Report.txt"
START_TIME=$(date +%s)

# Functions
print_header() {
    echo -e "${BLUE}=================================================================${NC}"
    echo -e "${BLUE}                DYNAMICS TEST SUITE RUNNER                      ${NC}"
    echo -e "${BLUE}=================================================================${NC}"
    echo ""
    echo -e "${YELLOW}Testing 6 dynamics engines with comprehensive validation${NC}"
    echo -e "${YELLOW}Start Time: $(date)${NC}"
    echo ""
}

print_separator() {
    echo -e "${BLUE}-----------------------------------------------------------------${NC}"
}

compile_test() {
    local test_file=$1
    local engine_name=$2
    local executable_name="${test_file%.cpp}"
    
    echo -e "${YELLOW}Compiling ${engine_name}...${NC}"
    
    if ${CXX} ${CXXFLAGS} ${INCLUDES} "${TEST_DIR}/${test_file}" -o "${TEST_DIR}/${executable_name}" ${LIBS} 2>/dev/null; then
        echo -e "${GREEN}✓ Compilation successful${NC}"
        return 0
    else
        echo -e "${RED}✗ Compilation failed${NC}"
        return 1
    fi
}

run_test() {
    local executable_name=$1
    local engine_name=$2
    
    echo -e "${YELLOW}Running ${engine_name} tests...${NC}"
    
    cd "${TEST_DIR}"
    
    if timeout 120 "./${executable_name}" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Tests completed successfully${NC}"
        return 0
    else
        echo -e "${RED}✗ Tests failed or timed out${NC}"
        return 1
    fi
}

analyze_results() {
    local test_file=$1
    local engine_name=$2
    local results_file="${test_file%.cpp}_TestResults.txt"
    
    if [[ -f "${TEST_DIR}/${results_file}" ]]; then
        # Extract test statistics
        local passed=$(grep -o "\[PASS\]" "${TEST_DIR}/${results_file}" | wc -l | tr -d ' ')
        local failed=$(grep -o "\[FAIL\]" "${TEST_DIR}/${results_file}" | wc -l | tr -d ' ')
        local total=$((passed + failed))
        
        TOTAL_TESTS=$((TOTAL_TESTS + total))
        TOTAL_PASSED=$((TOTAL_PASSED + passed))
        TOTAL_FAILED=$((TOTAL_FAILED + failed))
        
        if [[ $failed -eq 0 ]]; then
            echo -e "${GREEN}✓ All ${total} tests passed${NC}"
        else
            echo -e "${RED}✗ ${failed}/${total} tests failed${NC}"
            FAILED_ENGINES+=("${engine_name}")
        fi
        
        echo "  Tests passed: ${passed}"
        echo "  Tests failed: ${failed}"
        echo "  Success rate: $(( passed * 100 / total ))%"
    else
        echo -e "${RED}✗ Results file not found${NC}"
        FAILED_ENGINES+=("${engine_name}")
    fi
}

generate_report() {
    echo "Generating comprehensive test report..."
    
    {
        echo "================================================================="
        echo "                DYNAMICS TEST SUITE REPORT"
        echo "================================================================="
        echo ""
        echo "Test Suite: DYNAMICS Audio Engines"
        echo "Date: $(date)"
        echo "Duration: $(($(date +%s) - START_TIME)) seconds"
        echo ""
        echo "SUMMARY"
        echo "-------"
        echo "Total Tests: ${TOTAL_TESTS}"
        echo "Tests Passed: ${TOTAL_PASSED}"
        echo "Tests Failed: ${TOTAL_FAILED}"
        echo "Success Rate: $(( TOTAL_PASSED * 100 / TOTAL_TESTS ))%"
        echo ""
        
        if [[ ${#FAILED_ENGINES[@]} -eq 0 ]]; then
            echo "RESULT: ALL ENGINES PASSED ✓"
        else
            echo "RESULT: ${#FAILED_ENGINES[@]} ENGINE(S) FAILED ✗"
            echo ""
            echo "FAILED ENGINES:"
            for engine in "${FAILED_ENGINES[@]}"; do
                echo "  - ${engine}"
            done
        fi
        
        echo ""
        echo "DETAILED RESULTS"
        echo "=================="
        echo ""
        
        for i in "${!TEST_FILES[@]}"; do
            echo "Engine $((i + 1)): ${ENGINE_NAMES[i]} (${ENGINE_IDS[i]})"
            echo "Test File: ${TEST_FILES[i]}"
            
            local results_file="${TEST_FILES[i]%.cpp}_TestResults.txt"
            if [[ -f "${TEST_DIR}/${results_file}" ]]; then
                echo "Results File: ${results_file}"
                
                # Extract key metrics
                local passed=$(grep -o "\[PASS\]" "${TEST_DIR}/${results_file}" | wc -l | tr -d ' ')
                local failed=$(grep -o "\[FAIL\]" "${TEST_DIR}/${results_file}" | wc -l | tr -d ' ')
                
                echo "Tests Passed: ${passed}"
                echo "Tests Failed: ${failed}"
                
                if [[ $failed -gt 0 ]]; then
                    echo "Failed Tests:"
                    grep "\[FAIL\]" "${TEST_DIR}/${results_file}" | head -10
                    if [[ $(grep -c "\[FAIL\]" "${TEST_DIR}/${results_file}") -gt 10 ]]; then
                        echo "  ... and $(($(grep -c "\[FAIL\]" "${TEST_DIR}/${results_file}") - 10)) more"
                    fi
                fi
            else
                echo "Results File: NOT FOUND"
            fi
            
            echo ""
        done
        
        echo "ENGINE SPECIFICATIONS"
        echo "===================="
        echo ""
        echo "1. ENGINE_OPTO_COMPRESSOR (ID: 1)"
        echo "   - Vintage opto-electronic compressor simulation"
        echo "   - LA-2A style program-dependent release"
        echo "   - 8 parameters with thermal modeling"
        echo ""
        echo "2. ENGINE_VCA_COMPRESSOR (ID: 2)"  
        echo "   - Professional VCA compressor"
        echo "   - 10 parameters with lookahead and sidechain"
        echo "   - SIMD optimized processing"
        echo ""
        echo "3. ENGINE_TRANSIENT_SHAPER (ID: 3)"
        echo "   - Multi-algorithm transient detection"
        echo "   - Attack/sustain separation"
        echo "   - 10 parameters with oversampling"
        echo ""
        echo "4. ENGINE_NOISE_GATE (ID: 4)"
        echo "   - Advanced gate with hysteresis"
        echo "   - 8 parameters with thermal modeling"
        echo "   - Spectral detection algorithms"
        echo ""
        echo "5. ENGINE_MASTERING_LIMITER (ID: 5)"
        echo "   - Brick-wall limiter with true-peak detection"
        echo "   - 10 parameters with professional metering"
        echo "   - 0dBFS compliance verification"
        echo ""
        echo "6. ENGINE_DYNAMIC_EQ (ID: 6)"
        echo "   - Frequency-dependent dynamic processing"
        echo "   - 8 parameters with TPT filtering"
        echo "   - Multiple operation modes"
        echo ""
        
    } > "${REPORT_FILE}"
}

cleanup() {
    # Remove compiled test executables
    for test_file in "${TEST_FILES[@]}"; do
        local executable_name="${test_file%.cpp}"
        if [[ -f "${TEST_DIR}/${executable_name}" ]]; then
            rm "${TEST_DIR}/${executable_name}"
        fi
    done
}

# Main execution
main() {
    print_header
    
    # Change to test directory
    cd "${TEST_DIR}"
    
    # Run tests for each engine
    for i in "${!TEST_FILES[@]}"; do
        print_separator
        echo -e "${BLUE}Testing Engine $((i + 1))/6: ${ENGINE_NAMES[i]}${NC}"
        echo -e "${BLUE}Engine ID: ${ENGINE_IDS[i]}${NC}"
        echo ""
        
        # Compile test
        if compile_test "${TEST_FILES[i]}" "${ENGINE_NAMES[i]}"; then
            # Run test
            local executable_name="${TEST_FILES[i]%.cpp}"
            if run_test "${executable_name}" "${ENGINE_NAMES[i]}"; then
                # Analyze results
                analyze_results "${TEST_FILES[i]}" "${ENGINE_NAMES[i]}"
            else
                FAILED_ENGINES+=("${ENGINE_NAMES[i]}")
            fi
        else
            FAILED_ENGINES+=("${ENGINE_NAMES[i]}")
        fi
        
        echo ""
    done
    
    # Generate final report
    print_separator
    generate_report
    
    # Print summary
    echo -e "${BLUE}=================================================================${NC}"
    echo -e "${BLUE}                      TEST SUMMARY                              ${NC}"
    echo -e "${BLUE}=================================================================${NC}"
    echo ""
    echo "Total Tests Run: ${TOTAL_TESTS}"
    echo "Tests Passed: ${TOTAL_PASSED}"
    echo "Tests Failed: ${TOTAL_FAILED}"
    
    if [[ ${#FAILED_ENGINES[@]} -eq 0 ]]; then
        echo -e "${GREEN}Result: ALL DYNAMICS ENGINES PASSED! ✓${NC}"
        echo ""
        echo -e "${GREEN}All 6 dynamics engines are functioning correctly.${NC}"
        echo -e "${GREEN}Professional audio processing standards verified.${NC}"
    else
        echo -e "${RED}Result: ${#FAILED_ENGINES[@]} ENGINE(S) FAILED ✗${NC}"
        echo ""
        echo -e "${RED}Failed engines:${NC}"
        for engine in "${FAILED_ENGINES[@]}"; do
            echo -e "${RED}  - ${engine}${NC}"
        done
    fi
    
    echo ""
    echo -e "${YELLOW}Detailed report saved to: ${REPORT_FILE}${NC}"
    echo -e "${YELLOW}Individual test results available in *_TestResults.txt files${NC}"
    echo ""
    echo -e "${BLUE}Test suite completed in $(($(date +%s) - START_TIME)) seconds${NC}"
    
    # Cleanup
    cleanup
    
    # Return appropriate exit code
    if [[ ${#FAILED_ENGINES[@]} -eq 0 ]]; then
        exit 0
    else
        exit 1
    fi
}

# Handle interruption
trap cleanup EXIT

# Run the test suite
main "$@"