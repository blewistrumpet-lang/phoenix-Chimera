#!/bin/bash

# Build MuffFuzz CPU Benchmark Test

set -e

echo "=== Building MuffFuzz CPU Benchmark Test ==="

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin"
BUILD_DIR="build"
OBJ_DIR="$BUILD_DIR/obj"

# Create build directories
mkdir -p "$OBJ_DIR"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O3 -DNDEBUG"
CXXFLAGS="$CXXFLAGS -I$JUCE_DIR/modules"
CXXFLAGS="$CXXFLAGS -I$PLUGIN_DIR/Source"
CXXFLAGS="$CXXFLAGS -I."

# JUCE framework flags for macOS
FRAMEWORKS="-framework Cocoa -framework CoreAudio -framework CoreMIDI -framework IOKit -framework Accelerate -framework QuartzCore -framework AudioToolbox -framework CoreFoundation"

# JUCE preprocessor definitions
JUCE_DEFINES="-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1"
JUCE_DEFINES="$JUCE_DEFINES -DJUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0"
JUCE_DEFINES="$JUCE_DEFINES -DJUCER_XCODE_MAC_F6D2F4CF=1"
JUCE_DEFINES="$JUCE_DEFINES -DJUCE_APP_VERSION=1.0.0 -DJUCE_APP_VERSION_HEX=0x10000"

echo "Compiling MuffFuzz.cpp..."
$CXX $CXXFLAGS $JUCE_DEFINES -c "$PLUGIN_DIR/Source/MuffFuzz.cpp" -o "$OBJ_DIR/MuffFuzz.o"

echo "Compiling EngineBase.cpp..."
$CXX $CXXFLAGS $JUCE_DEFINES -c "$PLUGIN_DIR/Source/EngineBase.cpp" -o "$OBJ_DIR/EngineBase.o" 2>/dev/null || true

echo "Compiling DspEngineUtilities.cpp..."
$CXX $CXXFLAGS $JUCE_DEFINES -c "$PLUGIN_DIR/Source/DspEngineUtilities.cpp" -o "$OBJ_DIR/DspEngineUtilities.o" 2>/dev/null || true

echo "Compiling JUCE modules..."

# Compile minimal JUCE modules needed
$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ -DJUCE_CORE_INCLUDE_OBJC_HELPERS=1 -DJUCE_CORE_INCLUDE_JNI_HELPERS=0 - <<'EOF' -o "$OBJ_DIR/juce_core.o"
#include <juce_core/juce_core.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_events.o"
#include <juce_events/juce_events.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_audio_basics.o"
#include <juce_audio_basics/juce_audio_basics.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_audio_devices.o"
#include <juce_audio_devices/juce_audio_devices.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_audio_formats.o"
#include <juce_audio_formats/juce_audio_formats.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_audio_processors.o"
#include <juce_audio_processors/juce_audio_processors.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_data_structures.o"
#include <juce_data_structures/juce_data_structures.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_graphics.o"
#include <juce_graphics/juce_graphics.cpp>
EOF

$CXX $CXXFLAGS $JUCE_DEFINES -c -x c++ - <<'EOF' -o "$OBJ_DIR/juce_gui_basics.o"
#include <juce_gui_basics/juce_gui_basics.cpp>
EOF

echo "Compiling test program..."
$CXX $CXXFLAGS $JUCE_DEFINES -c test_muff_fuzz_cpu.cpp -o "$OBJ_DIR/test_muff_fuzz_cpu.o"

echo "Linking..."
$CXX $CXXFLAGS -o "$BUILD_DIR/test_muff_fuzz_cpu" \
    "$OBJ_DIR/test_muff_fuzz_cpu.o" \
    "$OBJ_DIR/MuffFuzz.o" \
    "$OBJ_DIR/EngineBase.o" \
    "$OBJ_DIR/DspEngineUtilities.o" \
    "$OBJ_DIR/juce_core.o" \
    "$OBJ_DIR/juce_events.o" \
    "$OBJ_DIR/juce_audio_basics.o" \
    "$OBJ_DIR/juce_audio_devices.o" \
    "$OBJ_DIR/juce_audio_formats.o" \
    "$OBJ_DIR/juce_audio_processors.o" \
    "$OBJ_DIR/juce_data_structures.o" \
    "$OBJ_DIR/juce_graphics.o" \
    "$OBJ_DIR/juce_gui_basics.o" \
    $FRAMEWORKS

echo ""
echo "Build complete: $BUILD_DIR/test_muff_fuzz_cpu"
echo "Run with: $BUILD_DIR/test_muff_fuzz_cpu"
