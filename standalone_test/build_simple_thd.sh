#!/bin/bash

set -e

echo "Building Simple THD Test..."

JUCE_PATH="/Users/Branden/JUCE/modules"

CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1"

SOURCE_PATH="../JUCE_Plugin/Source"
INCLUDES="-I${JUCE_PATH} -I${SOURCE_PATH} -I."

# Use cached JUCE modules
JUCE_OBJS="build/obj/juce_core.o build/obj/juce_audio_basics.o build/obj/juce_dsp.o"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit -framework Security -framework QuartzCore"

echo "Compiling test_clean_engines_thd_simple.cpp..."
$CXX $CXXFLAGS $INCLUDES -c test_clean_engines_thd_simple.cpp -o build/obj/test_clean_engines_thd_simple.o

echo "Linking..."
$CXX $CXXFLAGS build/obj/test_clean_engines_thd_simple.o $JUCE_OBJS $FRAMEWORKS -o test_clean_engines_thd_simple

if [ $? -eq 0 ]; then
    echo ""
    echo "═══════════════════════════════════════════"
    echo " BUILD SUCCESSFUL!"
    echo "═══════════════════════════════════════════"
    echo ""
    echo "To run: ./test_clean_engines_thd_simple"
    echo ""
else
    echo "Build failed"
    exit 1
fi
