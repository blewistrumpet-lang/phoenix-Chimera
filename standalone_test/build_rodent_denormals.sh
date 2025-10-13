#!/bin/bash

set -e

echo "=== Building RodentDistortion Denormal Test ==="

PLUGIN_DIR="../JUCE_Plugin"
SOURCE_DIR="${PLUGIN_DIR}/Source"
JUCE_DIR="${PLUGIN_DIR}/JuceLibraryCode"

# Compiler flags
CXX_FLAGS="-std=c++17 -O2 -Wall -Wextra"
CXX_FLAGS+=" -I${SOURCE_DIR} -I${JUCE_DIR}"
CXX_FLAGS+=" -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
CXX_FLAGS+=" -DJUCE_STANDALONE_APPLICATION=1"

# macOS frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI"
FRAMEWORKS+=" -framework AudioToolbox -framework AudioUnit"
FRAMEWORKS+=" -framework CoreFoundation -framework Carbon"

echo "Compiling RodentDistortion.cpp..."
clang++ ${CXX_FLAGS} -c ${SOURCE_DIR}/RodentDistortion.cpp -o build/RodentDistortion.o

echo "Compiling EngineFactory.cpp..."
clang++ ${CXX_FLAGS} -c ${SOURCE_DIR}/EngineFactory.cpp -o build/EngineFactory.o

echo "Compiling test_rodent_denormals.cpp..."
clang++ ${CXX_FLAGS} -c test_rodent_denormals.cpp -o build/test_rodent_denormals.o

echo "Linking..."
clang++ ${CXX_FLAGS} \
    build/test_rodent_denormals.o \
    build/RodentDistortion.o \
    build/EngineFactory.o \
    ${FRAMEWORKS} \
    -o test_rodent_denormals

echo "Build complete! Executable: test_rodent_denormals"
echo "Run with: ./test_rodent_denormals"
