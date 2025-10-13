#!/bin/bash

echo "================================================================"
echo "Building Chimera Preset System Validation Test"
echo "================================================================"

# Paths
JUCE_PATH="/Users/Branden/JUCE"
PLUGIN_SOURCE="../pi_deployment/JUCE_Plugin/Source"
ENGINE_SOURCE="../pi_deployment/Engines"
TEST_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test"

# Output
OUTPUT="preset_system_validation"

# Compiler
CXX="clang++"

# Compiler flags
CXXFLAGS="-std=c++17 -O2 -I$JUCE_PATH/modules -I$PLUGIN_SOURCE -I$ENGINE_SOURCE"
CXXFLAGS="$CXXFLAGS -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
CXXFLAGS="$CXXFLAGS -DJUCE_STANDALONE_APPLICATION=1"
CXXFLAGS="$CXXFLAGS -DJUCE_USE_CURL=0"
CXXFLAGS="$CXXFLAGS -DJUCE_WEB_BROWSER=0"

# Linker flags
LDFLAGS="-framework Accelerate -framework CoreFoundation -framework CoreAudio"
LDFLAGS="$LDFLAGS -framework CoreMIDI -framework AudioToolbox -framework AudioUnit"
LDFLAGS="$LDFLAGS -framework Foundation -framework AppKit -framework IOKit"
LDFLAGS="$LDFLAGS -framework QuartzCore -framework Carbon -framework Cocoa"
LDFLAGS="$LDFLAGS -framework Metal -framework MetalKit"

echo ""
echo "Step 1: Compiling JUCE modules..."

# JUCE Core
$CXX $CXXFLAGS -c -x objective-c++ \
    -DJUCE_CORE_INCLUDE_OBJC_HELPERS=1 \
    -o juce_core.o \
    $JUCE_PATH/modules/juce_core/juce_core.mm

# JUCE Events
$CXX $CXXFLAGS -c -x objective-c++ \
    -o juce_events.o \
    $JUCE_PATH/modules/juce_events/juce_events.mm

# JUCE Data Structures
$CXX $CXXFLAGS -c -x objective-c++ \
    -o juce_data_structures.o \
    $JUCE_PATH/modules/juce_data_structures/juce_data_structures.mm

# JUCE Audio Basics
$CXX $CXXFLAGS -c -x objective-c++ \
    -o juce_audio_basics.o \
    $JUCE_PATH/modules/juce_audio_basics/juce_audio_basics.mm

# JUCE Audio Processors
$CXX $CXXFLAGS -c -x objective-c++ \
    -o juce_audio_processors.o \
    $JUCE_PATH/modules/juce_audio_processors/juce_audio_processors.mm

# JUCE Audio Devices
$CXX $CXXFLAGS -c -x objective-c++ \
    -o juce_audio_devices.o \
    $JUCE_PATH/modules/juce_audio_devices/juce_audio_devices.mm

# JUCE Graphics
$CXX $CXXFLAGS -c -x objective-c++ \
    -o juce_graphics.o \
    $JUCE_PATH/modules/juce_graphics/juce_graphics.mm

# JUCE GUI Basics
$CXX $CXXFLAGS -c -x objective-c++ \
    -o juce_gui_basics.o \
    $JUCE_PATH/modules/juce_gui_basics/juce_gui_basics.mm

echo ""
echo "Step 2: Compiling Plugin Source..."

# PluginProcessor
$CXX $CXXFLAGS -c -x objective-c++ \
    -o PluginProcessor.o \
    $PLUGIN_SOURCE/PluginProcessor.cpp

# EngineFactory
$CXX $CXXFLAGS -c -x objective-c++ \
    -o EngineFactory.o \
    $PLUGIN_SOURCE/EngineFactory.cpp

echo ""
echo "Step 3: Compiling Test Program..."

$CXX $CXXFLAGS -c -x objective-c++ \
    -o test_preset_system.o \
    test_preset_system_comprehensive.cpp

echo ""
echo "Step 4: Linking..."

$CXX -o $OUTPUT \
    juce_core.o \
    juce_events.o \
    juce_data_structures.o \
    juce_audio_basics.o \
    juce_audio_processors.o \
    juce_audio_devices.o \
    juce_graphics.o \
    juce_gui_basics.o \
    PluginProcessor.o \
    EngineFactory.o \
    test_preset_system.o \
    $LDFLAGS

if [ $? -eq 0 ]; then
    echo ""
    echo "================================================================"
    echo "Build Successful!"
    echo "================================================================"
    echo ""
    echo "Running Preset System Validation..."
    echo ""

    ./$OUTPUT

    TEST_RESULT=$?

    echo ""
    echo "================================================================"
    if [ $TEST_RESULT -eq 0 ]; then
        echo "PRESET SYSTEM VALIDATION: PASSED"
    else
        echo "PRESET SYSTEM VALIDATION: FAILED"
    fi
    echo "================================================================"

    exit $TEST_RESULT
else
    echo ""
    echo "================================================================"
    echo "Build Failed!"
    echo "================================================================"
    exit 1
fi
