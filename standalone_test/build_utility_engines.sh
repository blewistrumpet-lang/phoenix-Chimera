#!/bin/bash

# Build script for utility engine validation tests

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
JUCE_DIR="$PROJECT_ROOT/JUCE"
SOURCE_DIR="$PROJECT_ROOT/JUCE_Plugin/Source"

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O2 -DNDEBUG"
CXXFLAGS="$CXXFLAGS -I$JUCE_DIR/modules"
CXXFLAGS="$CXXFLAGS -I$SOURCE_DIR"
CXXFLAGS="$CXXFLAGS -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"

# macOS frameworks
LDFLAGS="-framework Accelerate -framework CoreAudio -framework CoreMIDI"
LDFLAGS="$LDFLAGS -framework AudioToolbox -framework CoreFoundation"
LDFLAGS="$LDFLAGS -framework Carbon -framework Cocoa -framework IOKit"
LDFLAGS="$LDFLAGS -framework Security -framework AudioUnit -framework QuartzCore"

# Output
BUILD_DIR="$SCRIPT_DIR/build"
OUTPUT="$BUILD_DIR/test_utility_engines"

mkdir -p "$BUILD_DIR/obj"

echo "========================================="
echo "Building Utility Engine Validation Tests"
echo "========================================="
echo ""

# Build JUCE modules
echo "[1/8] Building juce_core..."
$CXX $CXXFLAGS -c "$JUCE_DIR/modules/juce_core/juce_core.mm" \
    -o "$BUILD_DIR/obj/juce_core.o" || exit 1

echo "[2/8] Building juce_audio_basics..."
$CXX $CXXFLAGS -c "$JUCE_DIR/modules/juce_audio_basics/juce_audio_basics.mm" \
    -o "$BUILD_DIR/obj/juce_audio_basics.o" || exit 1

echo "[3/8] Building juce_dsp..."
$CXX $CXXFLAGS -c "$JUCE_DIR/modules/juce_dsp/juce_dsp.mm" \
    -o "$BUILD_DIR/obj/juce_dsp.o" || exit 1

echo "[3.5/8] Building juce_audio_formats..."
$CXX $CXXFLAGS -c "$JUCE_DIR/modules/juce_audio_formats/juce_audio_formats.mm" \
    -o "$BUILD_DIR/obj/juce_audio_formats.o" || exit 1

# Build engine sources
echo "[4/8] Building GranularCloud..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/GranularCloud.cpp" \
    -o "$BUILD_DIR/obj/GranularCloud.o" || exit 1

echo "[5/8] Building PhasedVocoder..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/PhasedVocoder.cpp" \
    -o "$BUILD_DIR/obj/PhasedVocoder.o" || exit 1

echo "[6/8] Building GainUtility_Platinum..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/GainUtility_Platinum.cpp" \
    -o "$BUILD_DIR/obj/GainUtility_Platinum.o" || exit 1

echo "[7/8] Building MonoMaker_Platinum..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/MonoMaker_Platinum.cpp" \
    -o "$BUILD_DIR/obj/MonoMaker_Platinum.o" || exit 1

# Build test executable
echo "[8/8] Building test executable..."
$CXX $CXXFLAGS "$SCRIPT_DIR/test_utility_engines.cpp" \
    "$BUILD_DIR/obj/juce_core.o" \
    "$BUILD_DIR/obj/juce_audio_basics.o" \
    "$BUILD_DIR/obj/juce_audio_formats.o" \
    "$BUILD_DIR/obj/juce_dsp.o" \
    "$BUILD_DIR/obj/GranularCloud.o" \
    "$BUILD_DIR/obj/PhasedVocoder.o" \
    "$BUILD_DIR/obj/GainUtility_Platinum.o" \
    "$BUILD_DIR/obj/MonoMaker_Platinum.o" \
    $LDFLAGS -o "$OUTPUT" || exit 1

echo ""
echo "========================================="
echo "Build complete!"
echo "========================================="
echo "Executable: $OUTPUT"
echo ""
echo "Running tests..."
echo ""

# Run tests
"$OUTPUT"

exit $?
