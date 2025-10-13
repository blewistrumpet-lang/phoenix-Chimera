#!/bin/bash
# Build utility test - compile engines and test

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Building Utility Engines Test Suite                      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

mkdir -p "$OBJ_DIR"

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Step 1: Compile utility engines if needed
ENGINE_FILES=(
    "$PLUGIN_SRC/GainUtility_Platinum.cpp"
    "$PLUGIN_SRC/MonoMaker_Platinum.cpp"
)

echo "[1/3] Compiling utility engines..."
for engine_file in "${ENGINE_FILES[@]}"; do
    engine_name=$(basename "$engine_file" .cpp)
    obj_file="$OBJ_DIR/${engine_name}.o"

    if [ ! -f "$obj_file" ]; then
        echo "  Compiling $engine_name..."
        clang++ $CPP_FLAGS $INCLUDES $DEFINES \
            -c "$engine_file" \
            -o "$obj_file"

        if [ $? -ne 0 ]; then
            echo "✗ Failed to compile $engine_name"
            exit 1
        fi
    else
        echo "  $engine_name already compiled"
    fi
done

# Step 2: Compile test file
echo ""
echo "[2/3] Compiling utility_test.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c utility_test.cpp \
    -o "$OBJ_DIR/utility_test.o"

if [ $? -ne 0 ]; then
    echo "✗ Failed to compile utility_test.cpp"
    exit 1
fi

# Step 3: Link
echo ""
echo "[3/3] Linking utility test..."

# Get all JUCE and necessary object files, excluding other test binaries
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" ! -name "standalone_test.o" ! -name "reverb_test.o" ! -name "validate_reverb_test.o" ! -name "filter_test.o" ! -name "distortion_test.o" ! -name "modulation_test.o" ! -name "pitch_test.o" ! -name "spatial_test.o" ! -name "utility_test.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/utility_test.o" \
    "$OBJ_DIR/GainUtility_Platinum.o" \
    "$OBJ_DIR/MonoMaker_Platinum.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/utility_test"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo "  Executable: $BUILD_DIR/utility_test"
    echo ""
    echo "Run with:"
    echo "  cd $BUILD_DIR && ./utility_test"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi
