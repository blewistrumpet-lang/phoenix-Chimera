#!/bin/bash
# Simple build for dynamics engines 0-5 test
# This compiles and links only the necessary engines

set -e

echo "Building Dynamics Engines 0-5 Test (Simple)..."
echo "==============================================="

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj_simple"

mkdir -p "$OBJ_DIR"

# Compiler flags (release mode to match existing objects)
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Just compile our factory and test
echo "Compiling MinimalEngineFactory.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c MinimalEngineFactory.cpp \
    -o "$OBJ_DIR/MinimalEngineFactory.o"

# Compile test file
echo "Compiling test_dynamics_engines.cpp..."
clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c test_dynamics_engines.cpp \
    -o "$OBJ_DIR/test_dynamics_engines.o"

# Use precompiled dynamics engines from main build
echo "Using precompiled object files from main build..."
JUCE_OBJS=$(find "./build/obj" -name "juce_*.o" 2>/dev/null | sort | uniq)

# Get the dynamics engine object files
DYNAMICS_OBJS=$(find "./build/obj" -name "VintageOptoCompressor_Platinum.o" -o -name "ClassicCompressor.o" -o -name "TransientShaper_Platinum.o" -o -name "NoiseGate_Platinum.o" -o -name "MasteringLimiter_Platinum.o" 2>/dev/null | sort | uniq)

if [ -z "$JUCE_OBJS" ]; then
    echo "ERROR: JUCE object files not found. Run main build first."
    exit 1
fi

if [ -z "$DYNAMICS_OBJS" ]; then
    echo "ERROR: Dynamics engine object files not found. Run main build first."
    exit 1
fi

# Link everything
echo "Linking executable..."
clang++ $CPP_FLAGS \
    "$OBJ_DIR"/*.o \
    $DYNAMICS_OBJS \
    $JUCE_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/test_dynamics_simple"

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Run test with:"
    echo "  cd $BUILD_DIR && ./test_dynamics_simple"
    echo ""
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi
