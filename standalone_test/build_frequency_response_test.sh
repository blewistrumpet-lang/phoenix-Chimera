#!/bin/bash
#
# Build script for Frequency Response Test Suite
# Engines 8-14: Filters and EQs
#

set -e  # Exit on error

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Building Frequency Response Test Suite                     ║"
echo "║  Engines 8-14: Filters & EQs                                 ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Paths
JUCE_DIR="/Users/Branden/branden/JUCE"
PLUGIN_DIR="../JUCE_Plugin/Source"
BUILD_DIR="build"

# Create build directory
mkdir -p "$BUILD_DIR"

# Compiler settings
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra"
INCLUDES="-I${JUCE_DIR}/modules -I${PLUGIN_DIR} -I."
DEFINES="-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1"

# Required JUCE modules
JUCE_MODULES=(
    "juce_core"
    "juce_audio_basics"
    "juce_dsp"
)

echo "[1/6] Compiling JUCE modules..."
for module in "${JUCE_MODULES[@]}"; do
    obj_file="${BUILD_DIR}/${module}.o"
    if [ ! -f "$obj_file" ]; then
        echo "  Compiling $module..."
        ${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
            -c "${JUCE_DIR}/modules/${module}/${module}.cpp" \
            -o "$obj_file"
    else
        echo "  $module already compiled (using cached)"
    fi
done

echo ""
echo "[2/6] Compiling Engine source files..."

# List of engine files needed for filters/EQs (8-14)
ENGINE_SOURCES=(
    "EngineBase.cpp"
    "EngineFactory.cpp"
    "VintageConsoleEQ_Studio.cpp"
    "LadderFilter.cpp"
    "StateVariableFilter.cpp"
    "FormantFilter.cpp"
    "EnvelopeFilter.cpp"
    "CombResonator.cpp"
    "VocalFormantFilter.cpp"
)

for source in "${ENGINE_SOURCES[@]}"; do
    if [ -f "${PLUGIN_DIR}/${source}" ]; then
        obj_file="${BUILD_DIR}/$(basename ${source} .cpp).o"
        echo "  Compiling ${source}..."
        ${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
            -c "${PLUGIN_DIR}/${source}" \
            -o "$obj_file" || {
                echo "    Warning: Failed to compile ${source}, continuing..."
            }
    else
        echo "  Warning: ${source} not found, skipping..."
    fi
done

echo ""
echo "[3/6] Compiling test program..."
${CXX} ${CXXFLAGS} ${INCLUDES} ${DEFINES} \
    -c test_frequency_response_8_14.cpp \
    -o "${BUILD_DIR}/test_frequency_response_8_14.o"

echo ""
echo "[4/6] Linking executable..."

# Collect all object files
OBJ_FILES="${BUILD_DIR}/*.o"

# Link with required frameworks
${CXX} ${CXXFLAGS} ${OBJ_FILES} \
    -framework CoreFoundation \
    -framework CoreAudio \
    -framework CoreMIDI \
    -framework AudioToolbox \
    -framework Accelerate \
    -o test_frequency_response_8_14

echo ""
echo "[5/6] Checking executable..."
if [ -f "test_frequency_response_8_14" ]; then
    file test_frequency_response_8_14
    echo "✓ Build successful!"
else
    echo "✗ Build failed - executable not created"
    exit 1
fi

echo ""
echo "[6/6] Making plot script executable..."
chmod +x plot_frequency_response.py

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Build Complete!                                             ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""
echo "To run the frequency response test:"
echo "  ./test_frequency_response_8_14"
echo ""
echo "To generate plots (after test completes):"
echo "  python3 plot_frequency_response.py"
echo ""
echo "Output files:"
echo "  - frequency_response_engine_N.csv (per engine)"
echo "  - FREQUENCY_RESPONSE_REPORT.txt (summary)"
echo "  - frequency_response_plots/*.png (graphs)"
echo ""
