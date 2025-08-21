#!/bin/bash

# run_distortion_tests.sh
# Comprehensive test runner for all DISTORTION & SATURATION engines
# 
# This script compiles and runs all distortion engine tests,
# generates reports, and provides a summary of results.

set -e  # Exit on any error

# Configuration
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin"
TESTS_DIR="${PROJECT_ROOT}/Tests/Distortion"
RESULTS_DIR="${TESTS_DIR}/Results"
SOURCE_DIR="${PROJECT_ROOT}/Source"
JUCE_DIR="${PROJECT_ROOT}/JuceLibraryCode"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create results directory
mkdir -p "${RESULTS_DIR}"

echo -e "${BLUE}=== Distortion & Saturation Engine Test Suite ===${NC}"
echo "Project Root: ${PROJECT_ROOT}"
echo "Tests Directory: ${TESTS_DIR}"
echo "Results Directory: ${RESULTS_DIR}"
echo ""

# Function to print colored status
print_status() {
    local status=$1
    local message=$2
    case $status in
        "PASS")
            echo -e "${GREEN}[PASS]${NC} $message"
            ;;
        "FAIL")
            echo -e "${RED}[FAIL]${NC} $message"
            ;;
        "INFO")
            echo -e "${BLUE}[INFO]${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}[WARN]${NC} $message"
            ;;
    esac
}

# Function to compile and run a test
run_test() {
    local test_name=$1
    local test_file="${TESTS_DIR}/${test_name}_Test.cpp"
    local test_binary="${RESULTS_DIR}/${test_name}_test"
    
    print_status "INFO" "Starting ${test_name} test..."
    
    # Check if test file exists
    if [ ! -f "$test_file" ]; then
        print_status "FAIL" "Test file not found: $test_file"
        return 1
    fi
    
    # Compile the test
    print_status "INFO" "Compiling ${test_name}..."
    
    # Compilation flags
    COMPILE_FLAGS="-std=c++17 -O2 -DJUCE_STANDALONE_APPLICATION=1"
    INCLUDE_FLAGS="-I${SOURCE_DIR} -I${JUCE_DIR}"
    LINK_FLAGS="-framework Accelerate -framework AudioToolbox -framework CoreAudio -framework CoreMIDI"
    
    # Compile
    if g++ $COMPILE_FLAGS $INCLUDE_FLAGS "$test_file" $LINK_FLAGS -o "$test_binary" 2>/dev/null; then
        print_status "PASS" "Compilation successful for ${test_name}"
    else
        print_status "FAIL" "Compilation failed for ${test_name}"
        print_status "INFO" "Attempting compilation with verbose output..."
        
        # Try with more verbose output for debugging
        if g++ $COMPILE_FLAGS $INCLUDE_FLAGS "$test_file" $LINK_FLAGS -o "$test_binary"; then
            print_status "PASS" "Compilation successful for ${test_name} (second attempt)"
        else
            print_status "FAIL" "Compilation failed for ${test_name} - skipping test"
            return 1
        fi
    fi
    
    # Run the test
    print_status "INFO" "Running ${test_name} test..."
    
    # Change to results directory for output files
    pushd "$RESULTS_DIR" > /dev/null
    
    if timeout 300 "$test_binary"; then  # 5 minute timeout
        print_status "PASS" "${test_name} test completed successfully"
        
        # Move result files to organized location
        if [ -f "${test_name}_TestResults.txt" ]; then
            mv "${test_name}_TestResults.txt" "${RESULTS_DIR}/"
        fi
        if [ -f "${test_name}_Data.csv" ]; then
            mv "${test_name}_Data.csv" "${RESULTS_DIR}/"
        fi
        
        popd > /dev/null
        return 0
    else
        print_status "FAIL" "${test_name} test failed or timed out"
        popd > /dev/null
        return 1
    fi
}

# Function to generate summary report
generate_summary() {
    local summary_file="${RESULTS_DIR}/TestSummary.txt"
    local csv_summary="${RESULTS_DIR}/TestSummary.csv"
    
    print_status "INFO" "Generating test summary..."
    
    {
        echo "=== DISTORTION & SATURATION ENGINE TEST SUMMARY ==="
        echo "Generated: $(date)"
        echo "Test Suite: Project Chimera v3.0 Phoenix"
        echo ""
        echo "ENGINES TESTED:"
        echo "1. ENGINE_VINTAGE_TUBE (ID: 15) - Vintage Tube Preamp"
        echo "2. ENGINE_WAVE_FOLDER (ID: 16) - Wave Folder"
        echo "3. ENGINE_HARMONIC_EXCITER (ID: 17) - Harmonic Exciter"
        echo "4. ENGINE_BIT_CRUSHER (ID: 18) - Bit Crusher"
        echo "5. ENGINE_MULTIBAND_SATURATOR (ID: 19) - Multiband Saturator"
        echo "6. ENGINE_MUFF_FUZZ (ID: 20) - Muff Fuzz"
        echo "7. ENGINE_RODENT_DISTORTION (ID: 21) - Rodent Distortion"
        echo "8. ENGINE_K_STYLE (ID: 22) - K-Style Overdrive"
        echo ""
        echo "TEST RESULTS:"
        echo "============="
    } > "$summary_file"
    
    # CSV header
    echo "Engine,TestFile,Status,PassedTests,FailedTests,SuccessRate" > "$csv_summary"
    
    local total_tests=0
    local passed_engines=0
    local total_passed=0
    local total_failed=0
    
    # Analyze each test result
    for engine in "VintageTubePreamp" "WaveFolder" "HarmonicExciter" "BitCrusher" "MultibandSaturator" "MuffFuzz" "RodentDistortion" "KStyleOverdrive"; do
        local result_file="${RESULTS_DIR}/${engine}_TestResults.txt"
        
        if [ -f "$result_file" ]; then
            # Extract test statistics from result file
            local passed=$(grep -c "\\[PASS\\]" "$result_file" 2>/dev/null || echo "0")
            local failed=$(grep -c "\\[FAIL\\]" "$result_file" 2>/dev/null || echo "0")
            local total=$((passed + failed))
            
            if [ $total -gt 0 ]; then
                local success_rate=$(echo "scale=1; $passed * 100 / $total" | bc 2>/dev/null || echo "0")
                
                {
                    echo "$engine:"
                    echo "  Tests Passed: $passed"
                    echo "  Tests Failed: $failed"
                    echo "  Success Rate: ${success_rate}%"
                    echo ""
                } >> "$summary_file"
                
                echo "$engine,$result_file,COMPLETED,$passed,$failed,${success_rate}%" >> "$csv_summary"
                
                total_tests=$((total_tests + 1))
                total_passed=$((total_passed + passed))
                total_failed=$((total_failed + failed))
                
                if [ "$failed" -eq 0 ] && [ "$passed" -gt 0 ]; then
                    passed_engines=$((passed_engines + 1))
                fi
            else
                echo "$engine,$result_file,NO_RESULTS,0,0,0%" >> "$csv_summary"
                {
                    echo "$engine:"
                    echo "  Status: No test results found"
                    echo ""
                } >> "$summary_file"
            fi
        else
            echo "$engine,$result_file,NOT_RUN,0,0,0%" >> "$csv_summary"
            {
                echo "$engine:"
                echo "  Status: Test not run or failed"
                echo ""
            } >> "$summary_file"
        fi
    done
    
    # Overall summary
    local overall_success=0
    if [ $((total_passed + total_failed)) -gt 0 ]; then
        overall_success=$(echo "scale=1; $total_passed * 100 / ($total_passed + $total_failed)" | bc 2>/dev/null || echo "0")
    fi
    
    {
        echo "OVERALL SUMMARY:"
        echo "================"
        echo "Engines Tested: $total_tests"
        echo "Engines Passed: $passed_engines"
        echo "Total Tests Passed: $total_passed"
        echo "Total Tests Failed: $total_failed"
        echo "Overall Success Rate: ${overall_success}%"
        echo ""
        echo "DETAILED RESULTS:"
        echo "=================="
        echo "Individual test results and data files are available in:"
        echo "$RESULTS_DIR"
        echo ""
        echo "CSV data files contain detailed measurements for analysis."
    } >> "$summary_file"
    
    print_status "PASS" "Summary generated: $summary_file"
    print_status "PASS" "CSV summary generated: $csv_summary"
}

# Main execution
main() {
    print_status "INFO" "Starting distortion engine test suite..."
    
    # Check dependencies
    if ! command -v g++ &> /dev/null; then
        print_status "FAIL" "g++ compiler not found"
        exit 1
    fi
    
    if ! command -v bc &> /dev/null; then
        print_status "WARN" "bc calculator not found - some calculations may fail"
    fi
    
    # Array of engines to test
    declare -a engines=(
        "VintageTubePreamp"
        "WaveFolder" 
        "HarmonicExciter"
        "BitCrusher"
        "MultibandSaturator"
        "MuffFuzz"
        "RodentDistortion"
        "KStyleOverdrive"
    )
    
    local successful_tests=0
    local total_engines=${#engines[@]}
    
    # Run each test
    for engine in "${engines[@]}"; do
        echo ""
        echo -e "${YELLOW}--- Testing $engine ---${NC}"
        
        if run_test "$engine"; then
            successful_tests=$((successful_tests + 1))
        fi
        
        echo ""
    done
    
    # Generate summary
    echo ""
    echo -e "${BLUE}=== Generating Summary ===${NC}"
    generate_summary
    
    # Final report
    echo ""
    echo -e "${BLUE}=== FINAL REPORT ===${NC}"
    print_status "INFO" "Tested $total_engines engines"
    print_status "INFO" "Successfully completed: $successful_tests/$total_engines tests"
    
    if [ $successful_tests -eq $total_engines ]; then
        print_status "PASS" "All distortion engine tests completed successfully!"
    elif [ $successful_tests -gt 0 ]; then
        print_status "WARN" "Some tests completed successfully ($successful_tests/$total_engines)"
    else
        print_status "FAIL" "No tests completed successfully"
        exit 1
    fi
    
    echo ""
    print_status "INFO" "Results available in: $RESULTS_DIR"
    print_status "INFO" "Summary report: ${RESULTS_DIR}/TestSummary.txt"
    print_status "INFO" "CSV summary: ${RESULTS_DIR}/TestSummary.csv"
    
    # List generated files
    echo ""
    echo -e "${BLUE}Generated Files:${NC}"
    ls -la "$RESULTS_DIR"
}

# Cleanup function
cleanup() {
    print_status "INFO" "Cleaning up temporary files..."
    # Remove compiled test binaries
    rm -f "${RESULTS_DIR}"/*_test
}

# Set up cleanup trap
trap cleanup EXIT

# Run main function
main "$@"