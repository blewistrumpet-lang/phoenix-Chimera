#!/bin/bash
# Build script for PlateReverb (Engine 39) regression test

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build"
JUCE_DIR="${SCRIPT_DIR}/../JUCE_Plugin"
SOURCE_DIR="${JUCE_DIR}/Source"

echo "═══════════════════════════════════════════════════════════════"
echo "  Building PlateReverb Regression Test (Engine 39)"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Create build directory
mkdir -p "${BUILD_DIR}"

# Compile test
echo "Compiling test_plateverb_regression.cpp..."
g++ -std=c++17 \
    -I"${SOURCE_DIR}" \
    -I"${JUCE_DIR}/JuceLibraryCode" \
    -I"${SCRIPT_DIR}/../JUCE/modules" \
    -o "${BUILD_DIR}/test_plateverb_regression.o" \
    -c "${SCRIPT_DIR}/test_plateverb_regression.cpp" \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DNDEBUG=1

# Compile PlateReverb engine
echo "Compiling PlateReverb.cpp..."
g++ -std=c++17 \
    -I"${SOURCE_DIR}" \
    -I"${JUCE_DIR}/JuceLibraryCode" \
    -I"${SCRIPT_DIR}/../JUCE/modules" \
    -o "${BUILD_DIR}/PlateReverb_test.o" \
    -c "${SOURCE_DIR}/PlateReverb.cpp" \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DNDEBUG=1

# Compile JUCE stubs if not already compiled
if [ ! -f "${BUILD_DIR}/juce_core.o" ]; then
    echo "Compiling JUCE core..."
    g++ -std=c++17 \
        -I"${SCRIPT_DIR}/../JUCE/modules" \
        -o "${BUILD_DIR}/juce_core.o" \
        -c "${SCRIPT_DIR}/../JUCE/modules/juce_core/juce_core.cpp" \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
        -DNDEBUG=1
fi

if [ ! -f "${BUILD_DIR}/juce_audio_basics.o" ]; then
    echo "Compiling JUCE audio basics..."
    g++ -std=c++17 \
        -I"${SCRIPT_DIR}/../JUCE/modules" \
        -o "${BUILD_DIR}/juce_audio_basics.o" \
        -c "${SCRIPT_DIR}/../JUCE/modules/juce_audio_basics/juce_audio_basics.cpp" \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
        -DNDEBUG=1
fi

if [ ! -f "${BUILD_DIR}/juce_dsp.o" ]; then
    echo "Compiling JUCE DSP..."
    g++ -std=c++17 \
        -I"${SCRIPT_DIR}/../JUCE/modules" \
        -o "${BUILD_DIR}/juce_dsp.o" \
        -c "${SCRIPT_DIR}/../JUCE/modules/juce_dsp/juce_dsp.cpp" \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
        -DNDEBUG=1
fi

# Compile compilation time constants if needed
if [ ! -f "${BUILD_DIR}/juce_core_CompilationTime.o" ]; then
    echo "Creating compilation time constants..."
    cat > /tmp/juce_compilation_time.cpp << 'EOF'
namespace juce {
    const char* juce_compilationDate = __DATE__;
    const char* juce_compilationTime = __TIME__;
}
EOF
    g++ -std=c++17 \
        -I"${SCRIPT_DIR}/../JUCE/modules" \
        -o "${BUILD_DIR}/juce_core_CompilationTime.o" \
        -c /tmp/juce_compilation_time.cpp \
        -DJUCE_STANDALONE_APPLICATION=1 \
        -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
        -DNDEBUG=1
fi

# Link
echo "Linking..."
g++ -o "${BUILD_DIR}/test_plateverb_regression" \
    "${BUILD_DIR}/test_plateverb_regression.o" \
    "${BUILD_DIR}/PlateReverb_test.o" \
    "${BUILD_DIR}/juce_core.o" \
    "${BUILD_DIR}/juce_audio_basics.o" \
    "${BUILD_DIR}/juce_dsp.o" \
    "${BUILD_DIR}/obj/juce_audio_formats.o" \
    "${BUILD_DIR}/obj/juce_graphics.o" \
    "${BUILD_DIR}/juce_core_CompilationTime.o" \
    -framework Accelerate \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework IOKit \
    -framework Foundation \
    -framework CoreFoundation \
    -framework Cocoa \
    -framework Security \
    -framework QuartzCore \
    -framework CoreImage \
    -framework CoreGraphics \
    -framework CoreText

echo ""
echo "✓ Build complete: ${BUILD_DIR}/test_plateverb_regression"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  Running Regression Test..."
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Run the test
"${BUILD_DIR}/test_plateverb_regression"
exit_code=$?

echo ""
if [ $exit_code -eq 0 ]; then
    echo "✓ REGRESSION TEST PASSED"
else
    echo "✗ REGRESSION TEST FAILED"
fi

exit $exit_code
