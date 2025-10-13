#!/bin/bash
# Build script for Modulation Test Suite
# Compiles and runs modulation engine quality tests

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║    Building ChimeraPhoenix Modulation Test Suite          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"

# Use existing object files from main build
OBJ_DIR="$BUILD_DIR/obj"

if [ ! -d "$OBJ_DIR" ]; then
    echo "ERROR: Main build must be completed first (run build_v2.sh)"
    exit 1
fi

# Compiler flags
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

echo "[1/3] Compiling modulation_test.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c modulation_test.cpp \
    -o "$OBJ_DIR/modulation_test.o"

if [ $? -ne 0 ]; then
    echo "✗ Compilation failed"
    exit 1
fi

echo "[2/3] Linking modulation test..."
ALL_OBJS=$(find "$OBJ_DIR" -name "*.o" ! -name "standalone_test.o" ! -name "reverb_test.o" ! -name "modulation_test.o" ! -name "validate_reverb_test.o" ! -name "*_test.o" ! -name "*_test_simple.o" ! -name "juce_core_CompilationTime.o" | sort | uniq)

clang++ $CPP_FLAGS \
    "$OBJ_DIR/modulation_test.o" \
    $ALL_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/modulation_test"

if [ $? -ne 0 ]; then
    echo "✗ Linking failed"
    exit 1
fi

echo "✓ Build successful"
echo ""
echo "[3/3] Running modulation tests..."
echo ""

cd "$BUILD_DIR"
./modulation_test

echo ""
echo "Generated files:"
ls -lh mod_engine_*.csv 2>/dev/null | awk '{print "  - " $9 " (" $5 ")"}'

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    TESTING COMPLETE                        ║"
echo "╚════════════════════════════════════════════════════════════╝"
