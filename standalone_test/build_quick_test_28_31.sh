#!/bin/bash

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Building Quick Test: Engines 28-31                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Compiler settings
CXX="clang++"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Include paths
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation \
            -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
            -framework QuartzCore -framework CoreImage -framework CoreGraphics \
            -framework CoreText -framework WebKit -framework DiscRecording"

# Defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
         -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Output executable
OUTPUT="$BUILD_DIR/quick_test_28_31"

echo "Checking for required JUCE objects..."
JUCE_OBJS=(
    "$OBJ_DIR/juce_core.o"
    "$OBJ_DIR/juce_audio_basics.o"
    "$OBJ_DIR/juce_dsp.o"
)

NEED_JUCE=false
for obj in "${JUCE_OBJS[@]}"; do
    if [ ! -f "$obj" ]; then
        echo "  Missing: $(basename $obj)"
        NEED_JUCE=true
    fi
done

if [ "$NEED_JUCE" = true ]; then
    echo "Compiling required JUCE modules..."

    # juce_core
    if [ ! -f "$OBJ_DIR/juce_core.o" ]; then
        echo "  → juce_core"
        $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "$JUCE_DIR/modules/juce_core/juce_core.cpp" \
            -o "$OBJ_DIR/juce_core.o" 2>&1 | grep -v "warning:" | head -20
    fi

    # juce_audio_basics
    if [ ! -f "$OBJ_DIR/juce_audio_basics.o" ]; then
        echo "  → juce_audio_basics"
        $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "$JUCE_DIR/modules/juce_audio_basics/juce_audio_basics.cpp" \
            -o "$OBJ_DIR/juce_audio_basics.o" 2>&1 | grep -v "warning:" | head -20
    fi

    # juce_dsp
    if [ ! -f "$OBJ_DIR/juce_dsp.o" ]; then
        echo "  → juce_dsp"
        $CXX $OBJCPP_FLAGS $INCLUDES $DEFINES -c "$JUCE_DIR/modules/juce_dsp/juce_dsp.cpp" \
            -o "$OBJ_DIR/juce_dsp.o" 2>&1 | grep -v "warning:" | head -20
    fi
else
    echo "  ✓ All JUCE objects present"
fi

echo ""
echo "Compiling engine sources..."

# Engine 28: HarmonicTremolo
if [ ! -f "$OBJ_DIR/HarmonicTremolo.o" ]; then
    echo "  → HarmonicTremolo"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/HarmonicTremolo.cpp" \
        -o "$OBJ_DIR/HarmonicTremolo.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ HarmonicTremolo.o exists"
fi

# Engine 29: ClassicTremolo
if [ ! -f "$OBJ_DIR/ClassicTremolo.o" ]; then
    echo "  → ClassicTremolo"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/ClassicTremolo.cpp" \
        -o "$OBJ_DIR/ClassicTremolo.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ ClassicTremolo.o exists"
fi

# Engine 30: RotarySpeaker_Platinum
if [ ! -f "$OBJ_DIR/RotarySpeaker_Platinum.o" ]; then
    echo "  → RotarySpeaker_Platinum"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/RotarySpeaker_Platinum.cpp" \
        -o "$OBJ_DIR/RotarySpeaker_Platinum.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ RotarySpeaker_Platinum.o exists"
fi

# Engine 31: PitchShifter
if [ ! -f "$OBJ_DIR/PitchShifter.o" ]; then
    echo "  → PitchShifter"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/PitchShifter.cpp" \
        -o "$OBJ_DIR/PitchShifter.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ PitchShifter.o exists"
fi

# EngineFactory
if [ ! -f "$OBJ_DIR/EngineFactory.o" ]; then
    echo "  → EngineFactory"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/EngineFactory.cpp" \
        -o "$OBJ_DIR/EngineFactory.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ EngineFactory.o exists"
fi

# EngineBase is header-only, no compilation needed
echo "  ✓ EngineBase (header-only)"

# UnifiedDefaultParameters
if [ ! -f "$OBJ_DIR/UnifiedDefaultParameters.o" ]; then
    echo "  → UnifiedDefaultParameters"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/UnifiedDefaultParameters.cpp" \
        -o "$OBJ_DIR/UnifiedDefaultParameters.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ UnifiedDefaultParameters.o exists"
fi

echo ""
echo "Compiling test file..."
$CXX $CPP_FLAGS $INCLUDES $DEFINES -c "quick_test_28_31.cpp" \
    -o "$OBJ_DIR/quick_test_28_31.o" 2>&1 | grep -v "warning:" | head -20

if [ ! -f "$OBJ_DIR/quick_test_28_31.o" ]; then
    echo "✗ Test compilation failed"
    exit 1
fi

echo ""
echo "Linking executable..."

# Collect all engine and JUCE object files (exclude test object files and duplicates)
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "standalone_test.o" \
    ! -name "reverb_test.o" \
    ! -name "modulation_test.o" \
    ! -name "validate_reverb_test.o" \
    ! -name "quick_test_*.o" \
    ! -name "*_test*.o" \
    ! -name "test_*.o" \
    | grep -v "_test[0-9]" \
    | sort | uniq)

$CXX $CPP_FLAGS \
    "$OBJ_DIR/quick_test_28_31.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$OUTPUT" 2>&1 | head -50

if [ -f "$OUTPUT" ]; then
    echo ""
    echo "✓ Build successful: $OUTPUT"
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║  Running Tests...                                          ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""

    "$OUTPUT"
    EXIT_CODE=$?

    echo ""
    if [ $EXIT_CODE -eq 0 ]; then
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║  ✓ ALL TESTS PASSED                                       ║"
        echo "╚════════════════════════════════════════════════════════════╝"
    else
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║  ✗ SOME TESTS FAILED                                      ║"
        echo "╚════════════════════════════════════════════════════════════╝"
    fi

    exit $EXIT_CODE
else
    echo ""
    echo "✗ Build failed - linking error"
    exit 1
fi
