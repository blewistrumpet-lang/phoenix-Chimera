#!/bin/bash

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Building Modulation Engines 24-27 Test                   ║"
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
OUTPUT="$BUILD_DIR/test_modulation_24_27"

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

# Engine 24: ResonantChorus_Platinum
if [ ! -f "$OBJ_DIR/ResonantChorus_Platinum.o" ]; then
    echo "  → ResonantChorus_Platinum"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/ResonantChorus_Platinum.cpp" \
        -o "$OBJ_DIR/ResonantChorus_Platinum.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ ResonantChorus_Platinum.o exists"
fi

# Engine 25: AnalogPhaser
if [ ! -f "$OBJ_DIR/AnalogPhaser.o" ]; then
    echo "  → AnalogPhaser"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/AnalogPhaser.cpp" \
        -o "$OBJ_DIR/AnalogPhaser.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ AnalogPhaser.o exists"
fi

# Engine 26: PlatinumRingModulator
if [ ! -f "$OBJ_DIR/PlatinumRingModulator.o" ]; then
    echo "  → PlatinumRingModulator"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/PlatinumRingModulator.cpp" \
        -o "$OBJ_DIR/PlatinumRingModulator.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ PlatinumRingModulator.o exists"
fi

# Engine 27: FrequencyShifter
if [ ! -f "$OBJ_DIR/FrequencyShifter.o" ]; then
    echo "  → FrequencyShifter"
    $CXX $CPP_FLAGS $INCLUDES $DEFINES -c "$PLUGIN_SRC/FrequencyShifter.cpp" \
        -o "$OBJ_DIR/FrequencyShifter.o" 2>&1 | grep -v "warning:" | head -10
else
    echo "  ✓ FrequencyShifter.o exists"
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
$CXX $CPP_FLAGS $INCLUDES $DEFINES -c "test_modulation_24_27.cpp" \
    -o "$OBJ_DIR/test_modulation_24_27.o" 2>&1 | grep -v "warning:" | head -20

if [ ! -f "$OBJ_DIR/test_modulation_24_27.o" ]; then
    echo "✗ Test compilation failed"
    exit 1
fi

echo ""
echo "Linking executable..."

# Collect all engine and JUCE object files (exclude test object files)
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" ! -name "standalone_test.o" ! -name "reverb_test.o" ! -name "modulation_test.o" ! -name "validate_reverb_test.o" ! -name "*_test.o" ! -name "*_test_simple.o" ! -name "test_*.o" | sort | uniq)

$CXX $CPP_FLAGS \
    "$OBJ_DIR/test_modulation_24_27.o" \
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
