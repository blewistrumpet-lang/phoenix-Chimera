#!/bin/bash

# Build script for memory leak test
# Tests reverb engines 39-43 for memory leaks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "================================================================"
echo "  Building Memory Leak Test for Reverb Engines"
echo "================================================================"
echo ""

# Create build directory
mkdir -p build
cd build

# Find JUCE path
JUCE_DIR="../../../JUCE"
if [ ! -d "$JUCE_DIR" ]; then
    JUCE_DIR="../../JUCE"
fi
if [ ! -d "$JUCE_DIR" ]; then
    echo "ERROR: JUCE directory not found"
    exit 1
fi

echo "Using JUCE from: $JUCE_DIR"
echo ""

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
CXXFLAGS="$CXXFLAGS -I$JUCE_DIR/modules"
CXXFLAGS="$CXXFLAGS -I../../JUCE_Plugin/Source"
CXXFLAGS="$CXXFLAGS -I../../JUCE_Plugin/JuceLibraryCode"
CXXFLAGS="$CXXFLAGS -I.."

# macOS specific flags
if [[ "$OSTYPE" == "darwin"* ]]; then
    CXXFLAGS="$CXXFLAGS -framework Accelerate -framework CoreAudio -framework CoreMIDI"
    CXXFLAGS="$CXXFLAGS -framework AudioToolbox -framework CoreFoundation -framework Carbon"
    CXXFLAGS="$CXXFLAGS -framework Cocoa -framework IOKit -framework QuartzCore"
fi

# Suppress warnings
CXXFLAGS="$CXXFLAGS -Wno-deprecated-declarations"

# Source files
SOURCES="../test_reverb_memory_leaks.cpp"

# Engine source files
ENGINE_SOURCES=""
ENGINE_SOURCES="$ENGINE_SOURCES ../../JUCE_Plugin/Source/EngineFactory.cpp"
ENGINE_SOURCES="$ENGINE_SOURCES ../../JUCE_Plugin/Source/PlateReverb.cpp"
ENGINE_SOURCES="$ENGINE_SOURCES ../../JUCE_Plugin/Source/SpringReverb.cpp"
ENGINE_SOURCES="$ENGINE_SOURCES ../../JUCE_Plugin/Source/ShimmerReverb.cpp"
ENGINE_SOURCES="$ENGINE_SOURCES ../../JUCE_Plugin/Source/GatedReverb.cpp"
ENGINE_SOURCES="$ENGINE_SOURCES ../../JUCE_Plugin/Source/ConvolutionReverb.cpp"

# JUCE library compilation
echo "Compiling JUCE library..."
cat > juce_lib.cpp << 'EOF'
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#define JUCE_USE_CURL 0
#define JUCE_WEB_BROWSER 0
#define JUCE_USE_CAMERA 0

#include <JuceHeader.h>
EOF

$CXX $CXXFLAGS -c juce_lib.cpp -o juce_lib.o
echo "  juce_lib.o compiled"
echo ""

# Compile engine sources
echo "Compiling engine sources..."
for src in $ENGINE_SOURCES; do
    obj=$(basename "$src" .cpp).o
    echo "  Compiling $src..."
    $CXX $CXXFLAGS -c "$src" -o "$obj"
done
echo ""

# Compile test program
echo "Compiling test program..."
$CXX $CXXFLAGS -c $SOURCES -o test_reverb_memory_leaks.o
echo ""

# Link
echo "Linking test_reverb_memory_leaks..."
$CXX $CXXFLAGS -o test_reverb_memory_leaks \
    test_reverb_memory_leaks.o \
    juce_lib.o \
    EngineFactory.o \
    PlateReverb.o \
    SpringReverb.o \
    ShimmerReverb.o \
    GatedReverb.o \
    ConvolutionReverb.o

echo ""
echo "================================================================"
echo "  Build complete!"
echo "================================================================"
echo ""
echo "Run the test:"
echo "  cd build"
echo "  ./test_reverb_memory_leaks        # 5 minutes (default)"
echo "  ./test_reverb_memory_leaks 10     # 10 minutes"
echo "  ./test_reverb_memory_leaks 1      # Quick 1-minute test"
echo ""
