#!/bin/bash

echo "Building Deep Modulation Validation Test..."

# Find JUCE path
JUCE_PATH="/Users/Branden/JUCE"
if [ ! -d "$JUCE_PATH" ]; then
    echo "Error: JUCE not found at $JUCE_PATH"
    exit 1
fi

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -I$JUCE_PATH/modules -I../JUCE_Plugin/Source"
CXXFLAGS="$CXXFLAGS -DJUCE_STANDALONE_APPLICATION=1"
CXXFLAGS="$CXXFLAGS -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"

# macOS-specific frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI"
FRAMEWORKS="$FRAMEWORKS -framework IOKit -framework AppKit -framework Foundation"

# Source files
SOURCES=(
    "deep_modulation_validation.cpp"
    "../JUCE_Plugin/Source/EngineFactory.cpp"
    "../JUCE_Plugin/Source/StereoChorus.cpp"
    "../JUCE_Plugin/Source/ResonantChorus_Platinum.cpp"
    "../JUCE_Plugin/Source/AnalogPhaser.cpp"
    "../JUCE_Plugin/Source/PlatinumRingModulator.cpp"
    "../JUCE_Plugin/Source/FrequencyShifter.cpp"
    "../JUCE_Plugin/Source/HarmonicTremolo.cpp"
    "../JUCE_Plugin/Source/ClassicTremolo.cpp"
    "../JUCE_Plugin/Source/DimensionExpander.cpp"
    "../JUCE_Plugin/Source/VocalFormantFilter.cpp"
    "../JUCE_Plugin/Source/EnvelopeFilter.cpp"
)

# JUCE modules
JUCE_SOURCES=(
    "$JUCE_PATH/modules/juce_core/juce_core.cpp"
    "$JUCE_PATH/modules/juce_audio_basics/juce_audio_basics.cpp"
    "$JUCE_PATH/modules/juce_audio_devices/juce_audio_devices.cpp"
    "$JUCE_PATH/modules/juce_audio_formats/juce_audio_formats.cpp"
    "$JUCE_PATH/modules/juce_audio_processors/juce_audio_processors.cpp"
    "$JUCE_PATH/modules/juce_events/juce_events.cpp"
    "$JUCE_PATH/modules/juce_graphics/juce_graphics.cpp"
    "$JUCE_PATH/modules/juce_data_structures/juce_data_structures.cpp"
    "$JUCE_PATH/modules/juce_dsp/juce_dsp.cpp"
)

# Build
mkdir -p build

echo "Compiling..."
$CXX $CXXFLAGS "${SOURCES[@]}" "${JUCE_SOURCES[@]}" $FRAMEWORKS -o build/deep_modulation_validation

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Run with: ./build/deep_modulation_validation"
else
    echo "Build failed!"
    exit 1
fi
