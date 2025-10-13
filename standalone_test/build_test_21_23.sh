#!/bin/bash
# Build test for engines 21-23

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "Building test for engines 21-23..."

# Common paths
JUCE_DIR="../JUCE"
SOURCE_DIR="../JUCE_Plugin/Source"
BUILD_DIR="./build"
JUCE_MODULES="$JUCE_DIR/modules"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -I$JUCE_MODULES -I.. -I$SOURCE_DIR"
CXXFLAGS="$CXXFLAGS -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
CXXFLAGS="$CXXFLAGS -framework Accelerate -framework CoreAudio -framework CoreMIDI"
CXXFLAGS="$CXXFLAGS -framework AudioToolbox -framework CoreFoundation -framework IOKit"

# Link with pre-built objects
LINK_OBJS="$BUILD_DIR/EngineFactory.o"
LINK_OBJS="$LINK_OBJS $BUILD_DIR/juce_core.o"
LINK_OBJS="$LINK_OBJS $BUILD_DIR/juce_audio_basics.o"
LINK_OBJS="$LINK_OBJS $BUILD_DIR/juce_dsp.o"

# Check if required objects exist
if [ ! -f "$BUILD_DIR/EngineFactory.o" ]; then
    echo "ERROR: EngineFactory.o not found. Run build_all.sh first."
    exit 1
fi

# Compile test
echo "Compiling test_engines_21_23.cpp..."
$CXX $CXXFLAGS test_engines_21_23.cpp $LINK_OBJS -o test_engines_21_23

echo "Build complete: ./test_engines_21_23"
echo "Run with: ./test_engines_21_23"
