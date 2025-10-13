#!/bin/bash

# Spatial Engines Validation Build Script

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║    Building Spatial Engines Validation Test                   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SOURCE_DIR="$SCRIPT_DIR/../JUCE_Plugin/Source"
BUILD_DIR="$SCRIPT_DIR/build"
JUCE_DIR="$SCRIPT_DIR/../JUCE"

# Create build directory
mkdir -p "$BUILD_DIR/obj"

echo ""
echo "→ Compiling spatial engine sources..."

# DimensionExpander
echo "  - DimensionExpander..."
g++ -std=c++17 -c "$SOURCE_DIR/DimensionExpander.cpp" \
    -o "$BUILD_DIR/obj/DimensionExpander.o" \
    -I"$JUCE_DIR/modules" \
    -I"$SOURCE_DIR" \
    -I"$SOURCE_DIR/../JuceLibraryCode" \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_USE_COREIMAGE_LOADER=1 \
    -DJUCE_USE_COREAUDIO=1 \
    -DJUCE_USE_COREMIDI=1 \
    -DNDEBUG=1 \
    -O2 -Wall

# Common compile flags
COMMON_FLAGS="-std=c++17 -I$JUCE_DIR/modules -I$SOURCE_DIR -I$SOURCE_DIR/../JuceLibraryCode -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -O2 -Wall"

# SpectralFreeze
echo "  - SpectralFreeze..."
g++ $COMMON_FLAGS -c "$SOURCE_DIR/SpectralFreeze.cpp" \
    -o "$BUILD_DIR/obj/SpectralFreeze.o"

# SpectralGate_Platinum
echo "  - SpectralGate_Platinum..."
g++ $COMMON_FLAGS -c "$SOURCE_DIR/SpectralGate_Platinum.cpp" \
    -o "$BUILD_DIR/obj/SpectralGate_Platinum.o"

# MidSideProcessor_Platinum
echo "  - MidSideProcessor_Platinum..."
g++ $COMMON_FLAGS -c "$SOURCE_DIR/MidSideProcessor_Platinum.cpp" \
    -o "$BUILD_DIR/obj/MidSideProcessor_Platinum.o"

# PhaseAlign_Platinum
echo "  - PhaseAlign_Platinum..."
g++ $COMMON_FLAGS -c "$SOURCE_DIR/PhaseAlign_Platinum.cpp" \
    -o "$BUILD_DIR/obj/PhaseAlign_Platinum.o"

# Test executable
echo ""
echo "→ Compiling test executable..."
g++ $COMMON_FLAGS -c "$SCRIPT_DIR/test_spatial_engines_validation.cpp" \
    -o "$BUILD_DIR/obj/test_spatial_engines_validation.o"

echo ""
echo "→ Linking..."

g++ -std=c++17 \
    "$BUILD_DIR/obj/test_spatial_engines_validation.o" \
    "$BUILD_DIR/obj/DimensionExpander.o" \
    "$BUILD_DIR/obj/SpectralFreeze.o" \
    "$BUILD_DIR/obj/SpectralGate_Platinum.o" \
    "$BUILD_DIR/obj/MidSideProcessor_Platinum.o" \
    "$BUILD_DIR/obj/PhaseAlign_Platinum.o" \
    -o "$BUILD_DIR/test_spatial_engines_validation" \
    -framework Accelerate \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Foundation \
    -framework CoreFoundation \
    -L"$JUCE_DIR/modules" \
    "$JUCE_DIR/modules/juce_audio_basics/juce_audio_basics.mm" \
    "$JUCE_DIR/modules/juce_core/juce_core.mm" \
    "$JUCE_DIR/modules/juce_events/juce_events.mm" \
    "$JUCE_DIR/modules/juce_audio_formats/juce_audio_formats.mm" \
    "$JUCE_DIR/modules/juce_audio_processors/juce_audio_processors.mm" \
    "$JUCE_DIR/modules/juce_audio_devices/juce_audio_devices.mm" \
    "$JUCE_DIR/modules/juce_dsp/juce_dsp.mm" \
    -O2

echo ""
echo "✓ Build complete!"
echo ""
echo "Run with: $BUILD_DIR/test_spatial_engines_validation"
