#!/bin/bash
set -e

echo "Building DetuneDoubler THD Test (Minimal)..."

JUCE_PATH="/Users/Branden/JUCE/modules"
SOURCE_PATH="../JUCE_Plugin/Source"

CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 -DJUCE_MODULE_AVAILABLE_juce_core=1 -DJUCE_MODULE_AVAILABLE_juce_dsp=1 -DJUCE_MODULE_AVAILABLE_juce_audio_formats=1"

INCLUDES="-I${JUCE_PATH} -I${SOURCE_PATH} -I."
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework AppKit -framework WebKit -framework Security -framework QuartzCore"

mkdir -p build/obj

# Use cached JUCE modules if available
if [ ! -f build/obj/juce_core.o ]; then
    echo "Compiling JUCE modules..."
    $CXX $CXXFLAGS $INCLUDES -c ${JUCE_PATH}/juce_core/juce_core.mm -o build/obj/juce_core.o 2>&1 | grep -v "warning:" || true
    $CXX $CXXFLAGS $INCLUDES -c ${JUCE_PATH}/juce_audio_basics/juce_audio_basics.mm -o build/obj/juce_audio_basics.o 2>&1 | grep -v "warning:" || true
    $CXX $CXXFLAGS $INCLUDES -c ${JUCE_PATH}/juce_dsp/juce_dsp.mm -o build/obj/juce_dsp.o 2>&1 | grep -v "warning:" || true
    $CXX $CXXFLAGS $INCLUDES -c ${JUCE_PATH}/juce_audio_formats/juce_audio_formats.mm -o build/obj/juce_audio_formats.o 2>&1 | grep -v "warning:" || true
fi

# Recompile affected files
echo "Compiling DetuneDoubler..."
$CXX $CXXFLAGS $INCLUDES -c ${SOURCE_PATH}/DetuneDoubler.cpp -o build/obj/DetuneDoubler.o 2>&1 | grep -v "warning:" || true

# Compile test
echo "Compiling test..."
$CXX $CXXFLAGS $INCLUDES -c test_detunedoubler_thd.cpp -o build/obj/test_detunedoubler_thd.o 2>&1 | grep -v "warning:" || true

# Find all required object files
echo "Gathering object files..."
REQUIRED_OBJS="build/obj/test_detunedoubler_thd.o build/obj/DetuneDoubler.o build/obj/juce_core.o build/obj/juce_audio_basics.o build/obj/juce_dsp.o build/obj/juce_audio_formats.o build/obj/EngineFactory.o build/obj/PitchShifter.o build/obj/SMBPitchShiftFixed.o build/obj/PitchShiftFactory.o"

# Link
echo "Linking..."
$CXX $CXXFLAGS \
    $REQUIRED_OBJS \
    $FRAMEWORKS \
    -o test_detunedoubler_thd_minimal

if [ $? -eq 0 ]; then
    echo ""
    echo "═══════════════════════════════════════════"
    echo " BUILD SUCCESSFUL!"
    echo "═══════════════════════════════════════════"
    echo ""
    echo "Run with: ./test_detunedoubler_thd_minimal"
else
    echo "Link failed!"
    exit 1
fi
