#!/bin/bash

#==============================================================================
# Modulation Effects Test Suite Runner
# 
# This script compiles and runs all modulation engine tests, collecting
# results and generating a comprehensive test report.
#
# Usage: ./run_modulation_tests.sh [options]
# Options:
#   -c, --compile-only    Only compile tests, don't run them
#   -r, --run-only        Only run tests (assume already compiled)
#   -v, --verbose         Show detailed compilation output
#   -h, --help           Show this help message
#==============================================================================

set -e  # Exit on any error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR/../../Source"
JUCE_DIR="$SCRIPT_DIR/../../JuceLibraryCode"
RESULTS_DIR="$SCRIPT_DIR/Results"
TIMESTAMP=$(date '+%Y-%m-%d_%H-%M-%S')

# Test engines array
declare -a ENGINES=(
    "DigitalChorus"
    "ResonantChorus" 
    "AnalogPhaser"
    "RingModulator"
    "FrequencyShifter"
    "HarmonicTremolo"
    "ClassicTremolo"
    "RotarySpeaker"
    "PitchShifter"
    "DetuneDoubler"
    "IntelligentHarmonizer"
)

# Compiler settings
CXX_COMPILER="g++"
CXX_FLAGS="-std=c++17 -O2 -Wall -Wextra"
INCLUDE_FLAGS="-I$SOURCE_DIR -I$JUCE_DIR"
LINK_FLAGS="-ljuce_audio_basics -ljuce_audio_processors -ljuce_core -lpthread"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Flags
COMPILE_ONLY=false
RUN_ONLY=false
VERBOSE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--compile-only)
            COMPILE_ONLY=true
            shift
            ;;
        -r|--run-only)
            RUN_ONLY=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Modulation Effects Test Suite Runner"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  -c, --compile-only    Only compile tests, don't run them"
            echo "  -r, --run-only        Only run tests (assume already compiled)"
            echo "  -v, --verbose         Show detailed compilation output"
            echo "  -h, --help           Show this help message"
            echo ""
            echo "Tests included:"
            for engine in "${ENGINES[@]}"; do
                echo "  - $engine"
            done
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Create results directory
mkdir -p "$RESULTS_DIR"

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "INFO")
            echo -e "${BLUE}[INFO]${NC} $message"
            ;;
        "SUCCESS")
            echo -e "${GREEN}[SUCCESS]${NC} $message"
            ;;
        "WARNING")
            echo -e "${YELLOW}[WARNING]${NC} $message"
            ;;
        "ERROR")
            echo -e "${RED}[ERROR]${NC} $message"
            ;;
    esac
}

# Function to compile a test
compile_test() {
    local engine=$1
    local test_file="${engine}_Test.cpp"
    local executable="${engine}_Test"
    
    print_status "INFO" "Compiling $test_file..."
    
    if [[ ! -f "$test_file" ]]; then
        print_status "ERROR" "Test file $test_file not found!"
        return 1
    fi
    
    local compile_cmd="$CXX_COMPILER $CXX_FLAGS $INCLUDE_FLAGS $test_file -o $executable $LINK_FLAGS"
    
    if [[ "$VERBOSE" == true ]]; then
        print_status "INFO" "Compile command: $compile_cmd"
    fi
    
    if [[ "$VERBOSE" == true ]]; then
        eval $compile_cmd
    else
        eval "$compile_cmd" 2>&1 | grep -E "(error|Error|ERROR)" || true
    fi
    
    if [[ $? -eq 0 && -f "$executable" ]]; then
        print_status "SUCCESS" "Successfully compiled $executable"
        return 0
    else
        print_status "ERROR" "Failed to compile $test_file"
        return 1
    fi
}

# Function to run a test
run_test() {
    local engine=$1
    local executable="${engine}_Test"
    
    if [[ ! -f "$executable" ]]; then
        print_status "ERROR" "Executable $executable not found! Did compilation succeed?"
        return 1
    fi
    
    print_status "INFO" "Running $executable..."
    
    # Run the test and capture output
    local start_time=$(date +%s)
    if ./"$executable" > "$RESULTS_DIR/${engine}_output_${TIMESTAMP}.txt" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_status "SUCCESS" "$executable completed successfully (${duration}s)"
        
        # Move result files to results directory
        if [[ -f "${engine}_TestResults.txt" ]]; then
            mv "${engine}_TestResults.txt" "$RESULTS_DIR/${engine}_TestResults_${TIMESTAMP}.txt"
        fi
        if [[ -f "${engine}_Data.csv" ]]; then
            mv "${engine}_Data.csv" "$RESULTS_DIR/${engine}_Data_${TIMESTAMP}.csv"
        fi
        
        return 0
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        print_status "ERROR" "$executable failed after ${duration}s"
        return 1
    fi
}

# Function to generate summary report
generate_summary() {
    local compile_success=$1
    local compile_total=$2
    local run_success=$3
    local run_total=$4
    
    local summary_file="$RESULTS_DIR/TestSummary_${TIMESTAMP}.txt"
    
    cat > "$summary_file" << EOF
=================================================================
MODULATION EFFECTS TEST SUITE SUMMARY
=================================================================
Test Run: $TIMESTAMP
Total Engines Tested: ${#ENGINES[@]}

COMPILATION RESULTS:
- Successful: $compile_success/$compile_total
- Failed: $((compile_total - compile_success))/$compile_total

EXECUTION RESULTS:
- Successful: $run_success/$run_total  
- Failed: $((run_total - run_success))/$run_total

ENGINES TESTED:
EOF

    for engine in "${ENGINES[@]}"; do
        echo "- $engine" >> "$summary_file"
    done
    
    cat >> "$summary_file" << EOF

RESULT FILES:
- Summary: TestSummary_${TIMESTAMP}.txt
- Individual Results: *_TestResults_${TIMESTAMP}.txt
- Measurement Data: *_Data_${TIMESTAMP}.csv
- Test Output Logs: *_output_${TIMESTAMP}.txt

=================================================================
EOF

    print_status "INFO" "Summary report generated: $summary_file"
}

# Main execution
print_status "INFO" "Starting Modulation Effects Test Suite"
print_status "INFO" "Timestamp: $TIMESTAMP"
print_status "INFO" "Results directory: $RESULTS_DIR"

# Change to script directory
cd "$SCRIPT_DIR"

# Compilation phase
compile_success=0
compile_total=0

if [[ "$RUN_ONLY" == false ]]; then
    print_status "INFO" "=== COMPILATION PHASE ==="
    
    for engine in "${ENGINES[@]}"; do
        compile_total=$((compile_total + 1))
        if compile_test "$engine"; then
            compile_success=$((compile_success + 1))
        fi
    done
    
    print_status "INFO" "Compilation phase complete: $compile_success/$compile_total successful"
fi

# Execution phase  
run_success=0
run_total=0

if [[ "$COMPILE_ONLY" == false ]]; then
    print_status "INFO" "=== EXECUTION PHASE ==="
    
    for engine in "${ENGINES[@]}"; do
        run_total=$((run_total + 1))
        if run_test "$engine"; then
            run_success=$((run_success + 1))
        fi
    done
    
    print_status "INFO" "Execution phase complete: $run_success/$run_total successful"
fi

# Generate summary report
if [[ "$COMPILE_ONLY" == false ]]; then
    generate_summary $compile_success $compile_total $run_success $run_total
else
    generate_summary $compile_success $compile_total 0 0
fi

# Final status
if [[ "$COMPILE_ONLY" == false ]] && [[ $run_success -eq $run_total ]] && [[ $run_total -gt 0 ]]; then
    print_status "SUCCESS" "All modulation tests completed successfully!"
    exit 0
elif [[ "$COMPILE_ONLY" == true ]] && [[ $compile_success -eq $compile_total ]] && [[ $compile_total -gt 0 ]]; then
    print_status "SUCCESS" "All modulation tests compiled successfully!"
    exit 0
else
    print_status "WARNING" "Some tests failed. Check individual results for details."
    exit 1
fi