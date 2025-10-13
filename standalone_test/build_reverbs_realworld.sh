#!/bin/bash
################################################################################
# build_reverbs_realworld.sh - Build real-world reverb test
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
OBJ_DIR="$BUILD_DIR/obj"
SOURCE_DIR="$SCRIPT_DIR/../JUCE_Plugin/Source"
JUCE_DIR="/Users/Branden/JUCE"

echo "════════════════════════════════════════════════════════════════"
echo "Building Real-World Reverb Test (Engines 39-43)"
echo "════════════════════════════════════════════════════════════════"

# Create build directory
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Common flags
INCLUDES="-I. -I$SOURCE_DIR -I$SOURCE_DIR/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"

# Get all pre-compiled object files (exclude test mains)
echo "Gathering pre-compiled object files..."
ALL_LIB_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "*_test.o" \
    ! -name "*_test_simple.o" \
    ! -name "standalone_test.o" \
    ! -name "test_*.o" \
    ! -name "reverb_test.o" \
    ! -name "validate_reverb_test.o" \
    | sort | uniq)

# Count objects
OBJ_COUNT=$(echo "$ALL_LIB_OBJS" | wc -l | tr -d ' ')
echo "Found $OBJ_COUNT pre-compiled object files"

# Compile JUCE stubs
echo "Compiling JUCE stubs..."
if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SCRIPT_DIR/juce_stubs_reverb_test.cpp" \
    -o "$OBJ_DIR/juce_stubs_reverb_test.o" 2>&1; then
    echo "  ✓ Compiled stubs"
else
    echo "  ✗ Stub compilation failed"
    exit 1
fi

# Compile test source
echo "Compiling test_reverbs_realworld.cpp..."
if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "$SCRIPT_DIR/test_reverbs_realworld.cpp" \
    -o "$OBJ_DIR/test_reverbs_realworld.o" 2>&1; then
    echo "  ✓ Compiled"
else
    echo "  ✗ Compilation failed"
    exit 1
fi

# Link executable
echo "Linking executable..."
if clang++ $CPP_FLAGS \
    "$OBJ_DIR/test_reverbs_realworld.o" \
    "$OBJ_DIR/juce_stubs_reverb_test.o" \
    $ALL_LIB_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_reverbs_realworld" 2>&1; then
    echo "  ✓ Build successful!"
    echo ""
    echo "Executable: $BUILD_DIR/test_reverbs_realworld"
    echo ""
else
    echo "  ✗ Linking failed"
    exit 1
fi
