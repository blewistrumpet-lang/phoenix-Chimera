#!/bin/bash

echo "================================================================"
echo "BUILDING INTEGRATION TEST SUITE"
echo "Project Chimera Phoenix v3.0"
echo "================================================================"
echo ""

# Configuration
JUCE_PATH="/Users/Branden/JUCE"
SOURCE_PATH="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
PLUGIN_PATH="${SOURCE_PATH}/pi_deployment/JUCE_Plugin"
TEST_PATH="${SOURCE_PATH}/standalone_test"
BUILD_DIR="${TEST_PATH}/build_integration"

# Create build directory
mkdir -p "${BUILD_DIR}"

# Compiler flags
CXX_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable"
INCLUDES="-I${JUCE_PATH}/modules -I${PLUGIN_PATH}/Source -I${PLUGIN_PATH}/JuceLibraryCode"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCER_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE=0"

# Framework flags for macOS
FRAMEWORKS="-framework Accelerate -framework AudioToolbox -framework CoreAudio -framework CoreFoundation -framework CoreMIDI -framework IOKit -framework Cocoa"

echo "Step 1: Compiling integration test..."
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    "${TEST_PATH}/test_integration_suite.cpp" \
    -o "${BUILD_DIR}/test_integration_suite.o"

if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed!"
    exit 1
fi
echo "  ✓ Integration test compiled"

echo ""
echo "Step 2: Compiling JUCE modules..."

# Compile JUCE core
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    -x objective-c++ \
    "${JUCE_PATH}/modules/juce_core/juce_core.mm" \
    -o "${BUILD_DIR}/juce_core.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE core compilation failed!"
    exit 1
fi
echo "  ✓ juce_core compiled"

# Compile JUCE audio basics
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    -x objective-c++ \
    "${JUCE_PATH}/modules/juce_audio_basics/juce_audio_basics.mm" \
    -o "${BUILD_DIR}/juce_audio_basics.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE audio_basics compilation failed!"
    exit 1
fi
echo "  ✓ juce_audio_basics compiled"

# Compile JUCE audio processors
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    -x objective-c++ \
    "${JUCE_PATH}/modules/juce_audio_processors/juce_audio_processors.mm" \
    -o "${BUILD_DIR}/juce_audio_processors.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE audio_processors compilation failed!"
    exit 1
fi
echo "  ✓ juce_audio_processors compiled"

# Compile JUCE events
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    -x objective-c++ \
    "${JUCE_PATH}/modules/juce_events/juce_events.mm" \
    -o "${BUILD_DIR}/juce_events.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE events compilation failed!"
    exit 1
fi
echo "  ✓ juce_events compiled"

# Compile JUCE data structures
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    -x objective-c++ \
    "${JUCE_PATH}/modules/juce_data_structures/juce_data_structures.mm" \
    -o "${BUILD_DIR}/juce_data_structures.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE data_structures compilation failed!"
    exit 1
fi
echo "  ✓ juce_data_structures compiled"

# Compile JUCE graphics
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    -x objective-c++ \
    "${JUCE_PATH}/modules/juce_graphics/juce_graphics.mm" \
    -o "${BUILD_DIR}/juce_graphics.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE graphics compilation failed!"
    exit 1
fi
echo "  ✓ juce_graphics compiled"

# Compile JUCE gui_basics
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    -x objective-c++ \
    "${JUCE_PATH}/modules/juce_gui_basics/juce_gui_basics.mm" \
    -o "${BUILD_DIR}/juce_gui_basics.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: JUCE gui_basics compilation failed!"
    exit 1
fi
echo "  ✓ juce_gui_basics compiled"

echo ""
echo "Step 3: Compiling Engine Factory..."
g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
    "${PLUGIN_PATH}/Source/EngineFactory.cpp" \
    -o "${BUILD_DIR}/EngineFactory.o" 2>&1 | head -20

if [ $? -ne 0 ]; then
    echo "ERROR: EngineFactory compilation failed!"
    exit 1
fi
echo "  ✓ EngineFactory compiled"

echo ""
echo "Step 4: Compiling additional sources..."

# Compile MidSideProcessor
if [ -f "${PLUGIN_PATH}/Source/MidSideProcessor_Platinum.cpp" ]; then
    g++ -c ${CXX_FLAGS} ${INCLUDES} ${DEFINES} \
        "${PLUGIN_PATH}/Source/MidSideProcessor_Platinum.cpp" \
        -o "${BUILD_DIR}/MidSideProcessor_Platinum.o" 2>&1 | head -20

    if [ $? -eq 0 ]; then
        echo "  ✓ MidSideProcessor compiled"
        EXTRA_OBJS="${BUILD_DIR}/MidSideProcessor_Platinum.o"
    fi
fi

echo ""
echo "Step 5: Linking integration test executable..."
g++ ${CXX_FLAGS} \
    "${BUILD_DIR}/test_integration_suite.o" \
    "${BUILD_DIR}/juce_core.o" \
    "${BUILD_DIR}/juce_audio_basics.o" \
    "${BUILD_DIR}/juce_audio_processors.o" \
    "${BUILD_DIR}/juce_events.o" \
    "${BUILD_DIR}/juce_data_structures.o" \
    "${BUILD_DIR}/juce_graphics.o" \
    "${BUILD_DIR}/juce_gui_basics.o" \
    "${BUILD_DIR}/EngineFactory.o" \
    ${EXTRA_OBJS} \
    ${FRAMEWORKS} \
    -ldl -lpthread \
    -o "${TEST_PATH}/test_integration_suite"

if [ $? -ne 0 ]; then
    echo "ERROR: Linking failed!"
    exit 1
fi

echo ""
echo "================================================================"
echo "BUILD SUCCESSFUL"
echo "================================================================"
echo "Executable: ${TEST_PATH}/test_integration_suite"
echo ""
echo "To run the test:"
echo "  cd ${TEST_PATH}"
echo "  ./test_integration_suite"
echo ""
