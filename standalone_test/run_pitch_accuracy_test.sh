#!/bin/bash
#
# run_pitch_accuracy_test.sh - Build and run pitch accuracy test suite
#
# This script:
# 1. Compiles the pitch accuracy test program
# 2. Runs the test for engines 32-38, 49-50
# 3. Generates comprehensive analysis report with Python
# 4. Creates visualizations and detailed metrics
#

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="${BUILD_DIR}/obj"
TEST_NAME="test_pitch_accuracy"
TEST_BIN="${BUILD_DIR}/${TEST_NAME}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print header
echo ""
echo "════════════════════════════════════════════════════════════════════"
echo "  ChimeraPhoenix Pitch Accuracy Test Suite"
echo "════════════════════════════════════════════════════════════════════"
echo ""

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Compiler settings
CXX="clang++"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"

# Include paths
INCLUDES="-I. -I${PLUGIN_SRC} -I${PLUGIN_SRC}/../JuceLibraryCode -I${JUCE_DIR}/modules"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation \
            -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
            -framework QuartzCore -framework CoreImage -framework CoreGraphics \
            -framework CoreText -framework WebKit -framework DiscRecording"

# Defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
         -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Check if we need to rebuild
NEEDS_BUILD=false
if [ ! -f "$TEST_BIN" ]; then
    NEEDS_BUILD=true
    echo -e "${YELLOW}→ Test binary not found, building...${NC}"
elif [ "test_pitch_accuracy.cpp" -nt "$TEST_BIN" ]; then
    NEEDS_BUILD=true
    echo -e "${YELLOW}→ Source file updated, rebuilding...${NC}"
else
    echo -e "${GREEN}✓ Test binary is up to date${NC}"
fi

# Build test if needed
if [ "$NEEDS_BUILD" = true ]; then
    echo ""
    echo "Building pitch accuracy test..."
    echo "────────────────────────────────────────────────────────────────────"

    # Check if we have required dependencies (use existing objects from Makefile)
    echo "→ Checking for JUCE and engine objects..."

    # List of required JUCE objects
    JUCE_OBJS="${OBJ_DIR}/juce_core.o \
               ${OBJ_DIR}/juce_audio_basics.o \
               ${OBJ_DIR}/juce_audio_formats.o \
               ${OBJ_DIR}/juce_audio_processors.o \
               ${OBJ_DIR}/juce_dsp.o \
               ${OBJ_DIR}/juce_events.o \
               ${OBJ_DIR}/juce_data_structures.o \
               ${OBJ_DIR}/juce_graphics.o \
               ${OBJ_DIR}/juce_gui_basics.o \
               ${OBJ_DIR}/juce_gui_extra.o \
               ${OBJ_DIR}/juce_core_CompilationTime.o \
               ${OBJ_DIR}/SheenBidi.o"

    # Check if JUCE objects exist
    MISSING_JUCE=false
    for obj in $JUCE_OBJS; do
        if [ ! -f "$obj" ]; then
            MISSING_JUCE=true
            break
        fi
    done

    if [ "$MISSING_JUCE" = true ]; then
        echo -e "${RED}✗ JUCE objects not found. Please run 'make' first to build JUCE libraries.${NC}"
        echo ""
        echo "Run: cd $SCRIPT_DIR && make"
        exit 1
    fi

    echo -e "${GREEN}✓ JUCE objects found${NC}"

    # Get engine objects from required_engines.txt
    if [ -f "required_engines.txt" ]; then
        ENGINE_OBJS=""
        while IFS= read -r engine_src; do
            engine_name=$(basename "$engine_src" .cpp)
            ENGINE_OBJS="$ENGINE_OBJS ${OBJ_DIR}/${engine_name}.o"
        done < required_engines.txt

        # Check if engine objects exist
        MISSING_ENGINES=false
        for obj in $ENGINE_OBJS; do
            if [ ! -f "$obj" ]; then
                MISSING_ENGINES=true
                break
            fi
        done

        if [ "$MISSING_ENGINES" = true ]; then
            echo -e "${RED}✗ Engine objects not found. Please run 'make' first to build engines.${NC}"
            echo ""
            echo "Run: cd $SCRIPT_DIR && make"
            exit 1
        fi

        echo -e "${GREEN}✓ Engine objects found${NC}"
    else
        echo -e "${RED}✗ required_engines.txt not found${NC}"
        exit 1
    fi

    # Compile test source
    echo ""
    echo "→ Compiling test_pitch_accuracy.cpp..."
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c test_pitch_accuracy.cpp -o "${OBJ_DIR}/test_pitch_accuracy.o"

    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Compilation failed${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓ Compilation successful${NC}"

    # Link test executable
    echo ""
    echo "→ Linking test executable..."
    $CXX $CPP_FLAGS "${OBJ_DIR}/test_pitch_accuracy.o" $JUCE_OBJS $ENGINE_OBJS $FRAMEWORKS -L/opt/homebrew/lib -lharfbuzz -o "$TEST_BIN"

    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Linking failed${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓ Build complete: ${TEST_BIN}${NC}"
    echo ""
fi

# Run the test
echo ""
echo "════════════════════════════════════════════════════════════════════"
echo "  Running Pitch Accuracy Tests"
echo "════════════════════════════════════════════════════════════════════"
echo ""

"$TEST_BIN"

TEST_EXIT_CODE=$?

if [ $TEST_EXIT_CODE -ne 0 ]; then
    echo ""
    echo -e "${YELLOW}⚠ Test completed with exit code: ${TEST_EXIT_CODE}${NC}"
fi

# Check if results were generated
if [ ! -f "${BUILD_DIR}/pitch_accuracy_results.csv" ]; then
    echo ""
    echo -e "${RED}✗ No results file generated. Test may have failed.${NC}"
    exit 1
fi

echo ""
echo "════════════════════════════════════════════════════════════════════"
echo "  Analyzing Results"
echo "════════════════════════════════════════════════════════════════════"
echo ""

# Check for Python and required packages
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}✗ Python 3 not found. Cannot generate analysis report.${NC}"
    exit 1
fi

# Check for numpy
if ! python3 -c "import numpy" 2>/dev/null; then
    echo -e "${YELLOW}⚠ NumPy not installed. Installing...${NC}"
    python3 -m pip install numpy matplotlib --user
fi

# Check for matplotlib
if ! python3 -c "import matplotlib" 2>/dev/null; then
    echo -e "${YELLOW}⚠ Matplotlib not installed. Installing...${NC}"
    python3 -m pip install matplotlib --user
fi

# Run analysis script
echo "→ Running analysis script..."
echo ""

python3 analyze_pitch_accuracy.py

ANALYSIS_EXIT_CODE=$?

echo ""
echo "════════════════════════════════════════════════════════════════════"
echo "  Summary"
echo "════════════════════════════════════════════════════════════════════"
echo ""

if [ -f "${BUILD_DIR}/pitch_accuracy_report.txt" ]; then
    echo -e "${GREEN}✓ Detailed report:  ${BUILD_DIR}/pitch_accuracy_report.txt${NC}"
fi

if [ -f "${BUILD_DIR}/pitch_accuracy_results.csv" ]; then
    echo -e "${GREEN}✓ CSV results:      ${BUILD_DIR}/pitch_accuracy_results.csv${NC}"
fi

if [ -d "${BUILD_DIR}/pitch_accuracy_plots" ]; then
    echo -e "${GREEN}✓ Plots:            ${BUILD_DIR}/pitch_accuracy_plots/${NC}"

    # List plot files
    for plot in "${BUILD_DIR}"/pitch_accuracy_plots/*.png; do
        if [ -f "$plot" ]; then
            echo "    • $(basename "$plot")"
        fi
    done
fi

echo ""

# Final exit code
if [ $TEST_EXIT_CODE -eq 0 ] && [ $ANALYSIS_EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ All tests and analysis completed successfully!${NC}"
    echo ""
    exit 0
elif [ $ANALYSIS_EXIT_CODE -eq 0 ]; then
    echo -e "${YELLOW}⚠ Tests completed with warnings${NC}"
    echo ""
    exit 0
else
    echo -e "${YELLOW}⚠ Analysis completed with warnings${NC}"
    echo ""
    exit 1
fi
