#!/bin/bash

# Build script for Comprehensive Endurance & Stress Test Suite
# Project Chimera v3.0 Phoenix

set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  Building Comprehensive Endurance Suite"
echo "═══════════════════════════════════════════════════════════════"

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Check if main build exists
if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first (run build_v2.sh)"
    exit 1
fi

# Compiler flags (matching existing build system)
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo ""
echo "Compiling test_endurance_suite.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_endurance_suite.cpp \
    -o "$OBJ_DIR/test_endurance_suite.o"

echo "Linking endurance suite..."
# Get all object files except other test files
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "standalone_test.o" \
    ! -name "reverb_test.o" \
    ! -name "filter_test.o" \
    ! -name "distortion_test.o" \
    ! -name "dynamics_test.o" \
    ! -name "pitch_test.o" \
    ! -name "modulation_test.o" \
    ! -name "spatial_test.o" \
    ! -name "utility_test.o" \
    ! -name "*_test.o" \
    ! -name "test_*.o" \
    ! -name "quick_*.o" \
    ! -name "simple_*.o" \
    ! -name "endurance_test.o" \
    ! -name "stress_test_*.o" \
    ! -name "coverage_test*.o")

clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_endurance_suite.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -o "$BUILD_DIR/endurance_suite"

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  ✅ Build Complete!"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Executable: $BUILD_DIR/endurance_suite"
echo ""
echo "Usage:"
echo "  Run all tests on all engines:"
echo "    cd build && ./endurance_suite"
echo ""
echo "  Run specific test (1-5) on all engines:"
echo "    cd build && ./endurance_suite 1      # Memory stability"
echo "    cd build && ./endurance_suite 2      # CPU stability"
echo "    cd build && ./endurance_suite 3      # Parameter stability"
echo "    cd build && ./endurance_suite 4      # Buffer overflow"
echo "    cd build && ./endurance_suite 5      # Sample rate"
echo ""
echo "  Run all tests on specific engine:"
echo "    cd build && ./endurance_suite 0 41   # All tests on Engine 41"
echo ""
echo "  Run specific test on specific engine:"
echo "    cd build && ./endurance_suite 1 41   # Memory test on Engine 41"
echo ""
echo "  Run specific test on multiple critical engines:"
echo "    cd build && ./endurance_suite 1 40   # Shimmer Reverb"
echo "    cd build && ./endurance_suite 1 41   # Plate Reverb"
echo "    cd build && ./endurance_suite 1 36   # Magnetic Drum Echo"
echo ""
echo "WARNING: Full test suite (all engines, all tests) will take ~40 hours"
echo "         Consider running specific tests or engines instead"
echo ""
