#!/bin/bash

echo "============================================"
echo "Building Preset Validation System"
echo "============================================"

# Paths
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
PLUGIN_SOURCE="$PROJECT_ROOT/pi_deployment/JUCE_Plugin/Source"
JUCE_PATH="$PROJECT_ROOT/JUCE"
TEST_DIR="$PROJECT_ROOT/standalone_test"

# Output
OUTPUT="$TEST_DIR/preset_validation_test"

echo ""
echo "[1/4] Cleaning previous build..."
rm -f "$OUTPUT"
rm -f "$TEST_DIR"/*.o

echo ""
echo "[2/4] Compiling JUCE modules..."

# Compile JUCE modules if not already compiled
if [ ! -f "$TEST_DIR/juce_core.o" ]; then
    clang++ -std=c++17 -c \
        -I"$JUCE_PATH/modules" \
        -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -DJUCE_USE_CURL=0 \
        "$JUCE_PATH/modules/juce_core/juce_core.cpp" \
        -o "$TEST_DIR/juce_core.o"
fi

if [ ! -f "$TEST_DIR/juce_audio_basics.o" ]; then
    clang++ -std=c++17 -c \
        -I"$JUCE_PATH/modules" \
        -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
        -DJUCE_STANDALONE_APPLICATION=1 \
        "$JUCE_PATH/modules/juce_audio_basics/juce_audio_basics.cpp" \
        -o "$TEST_DIR/juce_audio_basics.o"
fi

if [ ! -f "$TEST_DIR/juce_dsp.o" ]; then
    clang++ -std=c++17 -c \
        -I"$JUCE_PATH/modules" \
        -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
        -DJUCE_STANDALONE_APPLICATION=1 \
        "$JUCE_PATH/modules/juce_dsp/juce_dsp.cpp" \
        -o "$TEST_DIR/juce_dsp.o"
fi

echo ""
echo "[3/4] Compiling preset validation test..."

clang++ -std=c++17 -c \
    -I"$JUCE_PATH/modules" \
    -I"$PLUGIN_SOURCE" \
    -I"$PROJECT_ROOT" \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    test_preset_validation.cpp \
    -o test_preset_validation.o

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Compilation failed"
    exit 1
fi

echo ""
echo "[4/4] Linking..."

# Find all necessary engine and processor object files
ENGINE_OBJECTS=$(find "$PROJECT_ROOT/pi_deployment/JUCE_Plugin" -name "*.o" -type f | grep -v "CMake" | tr '\n' ' ')

clang++ -std=c++17 \
    test_preset_validation.o \
    juce_core.o \
    juce_audio_basics.o \
    juce_dsp.o \
    $ENGINE_OBJECTS \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework Accelerate \
    -framework QuartzCore \
    -framework AudioToolbox \
    -framework Metal \
    -framework MetalKit \
    -o "$OUTPUT"

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Linking failed"
    echo ""
    echo "Attempting to compile plugin objects..."

    # Try to compile essential plugin objects
    cd "$PLUGIN_SOURCE"

    if [ ! -f PluginProcessor.o ]; then
        echo "Compiling PluginProcessor..."
        clang++ -std=c++17 -c \
            -I"$JUCE_PATH/modules" \
            -I"$PLUGIN_SOURCE" \
            -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
            -DJUCE_STANDALONE_APPLICATION=1 \
            PluginProcessor.cpp \
            -o PluginProcessor.o
    fi

    if [ ! -f EngineFactory.o ]; then
        echo "Compiling EngineFactory..."
        clang++ -std=c++17 -c \
            -I"$JUCE_PATH/modules" \
            -I"$PLUGIN_SOURCE" \
            -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
            -DJUCE_STANDALONE_APPLICATION=1 \
            EngineFactory.cpp \
            -o EngineFactory.o
    fi

    cd "$TEST_DIR"

    echo ""
    echo "Retrying link with essential objects..."

    clang++ -std=c++17 \
        test_preset_validation.o \
        juce_core.o \
        juce_audio_basics.o \
        juce_dsp.o \
        "$PLUGIN_SOURCE"/*.o \
        -framework Cocoa \
        -framework IOKit \
        -framework CoreAudio \
        -framework CoreMIDI \
        -framework Accelerate \
        -framework QuartzCore \
        -framework AudioToolbox \
        -framework Metal \
        -framework MetalKit \
        -o "$OUTPUT" 2>&1 | head -20

    if [ $? -ne 0 ]; then
        echo ""
        echo "[ERROR] Linking still failed. Please ensure plugin is built first."
        echo "Run: cd $PROJECT_ROOT/pi_deployment/JUCE_Plugin && cmake --build build"
        exit 1
    fi
fi

echo ""
echo "============================================"
echo "Build successful!"
echo "Executable: $OUTPUT"
echo "============================================"
echo ""

# Run the test
echo "Running preset validation..."
echo ""

"$OUTPUT"

exit $?
