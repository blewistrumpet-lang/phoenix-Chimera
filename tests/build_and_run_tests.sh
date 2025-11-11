#!/bin/bash

# build_and_run_tests.sh - Build and run the safety test suite

echo "========================================="
echo "Building Phoenix-Chimera Safety Tests"
echo "========================================="

# Set build directory
BUILD_DIR="build"
JUCE_DIR="/Users/Branden/JUCE"
PROJECT_DIR=".."
PLUGIN_SOURCE="${PROJECT_DIR}/JUCE_Plugin/Source"

# Create build directory
mkdir -p $BUILD_DIR

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -Wall -Wextra -g -O2"

# JUCE flags
JUCE_FLAGS="-I${JUCE_DIR}/modules"
JUCE_FLAGS="${JUCE_FLAGS} -DJUCE_STANDALONE_APPLICATION=1"
JUCE_FLAGS="${JUCE_FLAGS} -DJUCE_USE_CURL=0"
JUCE_FLAGS="${JUCE_FLAGS} -DJUCE_WEB_BROWSER=0"

# Include paths
INCLUDES="-I${PLUGIN_SOURCE} -I${PLUGIN_SOURCE}/../JuceLibraryCode"

# macOS frameworks
FRAMEWORKS="-framework Accelerate"
FRAMEWORKS="${FRAMEWORKS} -framework AudioToolbox"
FRAMEWORKS="${FRAMEWORKS} -framework CoreAudio"
FRAMEWORKS="${FRAMEWORKS} -framework CoreAudioKit"
FRAMEWORKS="${FRAMEWORKS} -framework CoreMIDI"
FRAMEWORKS="${FRAMEWORKS} -framework Carbon"
FRAMEWORKS="${FRAMEWORKS} -framework Cocoa"
FRAMEWORKS="${FRAMEWORKS} -framework IOKit"
FRAMEWORKS="${FRAMEWORKS} -framework QuartzCore"

# Source files needed for tests
SOURCES="test_safety_complete.cpp"
SOURCES="${SOURCES} ${PLUGIN_SOURCE}/SafetyCore.h"  # Header only
SOURCES="${SOURCES} ${PLUGIN_SOURCE}/ThreadSafeEngineManager.h"  # Header only
SOURCES="${SOURCES} ${PLUGIN_SOURCE}/SafeEngineBase.h"  # Header only

# We need to compile some actual engine sources for testing
# Add a minimal set of engines
ENGINE_SOURCES="${PLUGIN_SOURCE}/EngineBase.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_SOURCE}/EngineFactory.cpp"
ENGINE_SOURCES="${ENGINE_SOURCES} ${PLUGIN_SOURCE}/NoneEngine.cpp"

# Check if source files exist
echo "Checking for required source files..."
for src in $ENGINE_SOURCES; do
    if [ ! -f "$src" ]; then
        echo "Warning: $src not found, will use stub"
        ENGINE_SOURCES=""
        break
    fi
done

# Compile with AddressSanitizer for memory safety
echo ""
echo "Building with AddressSanitizer..."
$CXX $CXXFLAGS $JUCE_FLAGS $INCLUDES \
    -fsanitize=address \
    -fno-omit-frame-pointer \
    $SOURCES $ENGINE_SOURCES \
    $FRAMEWORKS \
    -o $BUILD_DIR/test_safety_asan

if [ $? -ne 0 ]; then
    echo "❌ Build with AddressSanitizer failed"
    exit 1
fi

echo "✅ Build with AddressSanitizer complete"

# Compile with ThreadSanitizer for thread safety
echo ""
echo "Building with ThreadSanitizer..."
$CXX $CXXFLAGS $JUCE_FLAGS $INCLUDES \
    -fsanitize=thread \
    $SOURCES $ENGINE_SOURCES \
    $FRAMEWORKS \
    -o $BUILD_DIR/test_safety_tsan

if [ $? -ne 0 ]; then
    echo "❌ Build with ThreadSanitizer failed"
    exit 1
fi

echo "✅ Build with ThreadSanitizer complete"

# Regular build for performance testing
echo ""
echo "Building release version..."
$CXX $CXXFLAGS $JUCE_FLAGS $INCLUDES \
    -O3 \
    $SOURCES $ENGINE_SOURCES \
    $FRAMEWORKS \
    -o $BUILD_DIR/test_safety_release

if [ $? -ne 0 ]; then
    echo "❌ Release build failed"
    exit 1
fi

echo "✅ Release build complete"

echo ""
echo "========================================="
echo "Running Safety Tests"
echo "========================================="

# Run with AddressSanitizer
echo ""
echo "--- Running with AddressSanitizer ---"
ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1 \
    $BUILD_DIR/test_safety_asan 2>&1 | tee $BUILD_DIR/asan_report.txt

# Check for AddressSanitizer errors
if grep -q "ERROR: AddressSanitizer" $BUILD_DIR/asan_report.txt; then
    echo ""
    echo "❌ AddressSanitizer detected memory safety issues!"
    echo "See $BUILD_DIR/asan_report.txt for details"
    exit 1
fi

# Run with ThreadSanitizer
echo ""
echo "--- Running with ThreadSanitizer ---"
TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1 \
    $BUILD_DIR/test_safety_tsan 2>&1 | tee $BUILD_DIR/tsan_report.txt

# Check for ThreadSanitizer warnings
if grep -q "WARNING: ThreadSanitizer" $BUILD_DIR/tsan_report.txt; then
    echo ""
    echo "❌ ThreadSanitizer detected race conditions!"
    echo "See $BUILD_DIR/tsan_report.txt for details"
    exit 1
fi

# Run release version for performance
echo ""
echo "--- Running release version ---"
$BUILD_DIR/test_safety_release

echo ""
echo "========================================="
echo "Test Results Summary"
echo "========================================="

echo ""
echo "✅ Memory safety: PASSED (AddressSanitizer clean)"
echo "✅ Thread safety: PASSED (ThreadSanitizer clean)"
echo ""

# Check if Valgrind is available for additional memory checking
if command -v valgrind &> /dev/null; then
    echo "Running Valgrind memory check..."
    valgrind --leak-check=full --show-leak-kinds=all \
             --track-origins=yes --verbose \
             $BUILD_DIR/test_safety_release 2>&1 | tee $BUILD_DIR/valgrind_report.txt

    if grep -q "ERROR SUMMARY: 0 errors" $BUILD_DIR/valgrind_report.txt; then
        echo "✅ Valgrind: No memory errors detected"
    else
        echo "⚠️  Valgrind detected issues - see $BUILD_DIR/valgrind_report.txt"
    fi
else
    echo "ℹ️  Valgrind not found - skipping additional memory checks"
fi

echo ""
echo "========================================="
echo "All safety tests complete!"
echo "Reports saved in $BUILD_DIR/"
echo "========================================="