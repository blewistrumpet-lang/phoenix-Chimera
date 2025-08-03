#!/bin/bash

# Chimera Engine Test Runner Script
# Compiles and runs comprehensive tests on all engines

echo "========================================="
echo "Chimera Engine Test Suite"
echo "========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Parse arguments
QUICK_MODE=""
HTML_REPORT=""
JSON_REPORT=""
CATEGORY=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --quick)
            QUICK_MODE="--quick"
            shift
            ;;
        --html)
            HTML_REPORT="--html"
            shift
            ;;
        --json)
            JSON_REPORT="--json"
            shift
            ;;
        --category)
            CATEGORY="--category $2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --quick        Run quick tests only"
            echo "  --html         Generate HTML report"
            echo "  --json         Generate JSON report"
            echo "  --category <name>  Test specific category (Dynamics, Filters, Delays, Reverbs, Modulation, Distortion)"
            echo "  --help         Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Check if test executable exists, if not compile it
TEST_EXEC="$SCRIPT_DIR/build/TestAllEngines"

if [ ! -f "$TEST_EXEC" ]; then
    echo -e "${YELLOW}Test executable not found. Compiling...${NC}"
    
    mkdir -p build
    cd build
    
    # Compile the test runner
    echo "Compiling test suite..."
    g++ -std=c++17 \
        -I"$SCRIPT_DIR/JUCE_Plugin/Source" \
        -I"$HOME/JUCE/modules" \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -framework CoreAudio \
        -framework CoreMIDI \
        -framework AudioToolbox \
        -framework Accelerate \
        -framework CoreFoundation \
        -framework IOKit \
        -framework AppKit \
        -framework OpenGL \
        -framework Carbon \
        -framework QuartzCore \
        "$SCRIPT_DIR/JUCE_Plugin/Source/TestAllEngines.cpp" \
        "$SCRIPT_DIR/JUCE_Plugin/Source/EngineTestSuite.cpp" \
        "$SCRIPT_DIR/JUCE_Plugin/Source/EngineTestProtocols.cpp" \
        "$SCRIPT_DIR/JUCE_Plugin/Source/AudioMeasurements.cpp" \
        "$SCRIPT_DIR/JUCE_Plugin/Source/TestSignalGenerator.cpp" \
        "$SCRIPT_DIR/JUCE_Plugin/Source/EngineFactory.cpp" \
        "$SCRIPT_DIR/JUCE_Plugin/Source/"*.cpp \
        -o TestAllEngines
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Compilation failed!${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Compilation successful!${NC}"
    cd ..
fi

# Create reports directory
mkdir -p test_reports

# Run the tests
echo ""
echo "Running engine tests..."
echo "------------------------"

"$TEST_EXEC" $QUICK_MODE $HTML_REPORT $JSON_REPORT $CATEGORY

# Check exit code
if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
else
    echo -e "\n${RED}Some tests failed. Check the report for details.${NC}"
fi

echo ""
echo "Reports saved in: $SCRIPT_DIR/test_reports/"
echo "========================================="