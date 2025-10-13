#!/bin/bash

# Build script for buffer size independence test
# Tests all engines with buffer sizes: 32, 64, 128, 256, 512, 1024, 2048

set -e

echo "=========================================================================="
echo "  Building Buffer Size Independence Test"
echo "=========================================================================="
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="./build/obj"

# Compiler settings
CXX="clang++"
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Include paths
INCLUDES="-I. -I${PLUGIN_SRC} -I${PLUGIN_SRC}/../JuceLibraryCode -I${JUCE_DIR}/modules"

# Frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation \
            -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security \
            -framework QuartzCore -framework CoreImage -framework CoreGraphics \
            -framework CoreText -framework WebKit -framework DiscRecording"

# Defines
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
         -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# Create directories
mkdir -p ${BUILD_DIR}
mkdir -p ${OBJ_DIR}

echo "Step 1: Compiling JUCE modules..."
echo "-----------------------------------"

# Compile JUCE modules if not already compiled
JUCE_MODULES="juce_core juce_audio_basics juce_audio_formats juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra"

for MODULE in ${JUCE_MODULES}; do
    if [ ! -f "${OBJ_DIR}/${MODULE}.o" ]; then
        echo "  Compiling ${MODULE}..."
        ${CXX} ${OBJCPP_FLAGS} ${INCLUDES} ${DEFINES} -c \
            ${JUCE_DIR}/modules/${MODULE}/${MODULE}.cpp \
            -o ${OBJ_DIR}/${MODULE}.o
    fi
done

# Compile SheenBidi (C library used by JUCE)
if [ ! -f "${OBJ_DIR}/SheenBidi.o" ]; then
    echo "  Compiling SheenBidi..."
    clang -std=c11 -O2 ${INCLUDES} -DSB_CONFIG_UNITY -c \
        ${JUCE_DIR}/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c \
        -o ${OBJ_DIR}/SheenBidi.o
fi

# Compile juce_core_CompilationTime
if [ ! -f "${OBJ_DIR}/juce_core_CompilationTime.o" ]; then
    echo "  Compiling juce_core_CompilationTime..."
    ${CXX} ${OBJCPP_FLAGS} ${INCLUDES} ${DEFINES} -c \
        ${JUCE_DIR}/modules/juce_core/juce_core_CompilationTime.cpp \
        -o ${OBJ_DIR}/juce_core_CompilationTime.o
fi

echo ""
echo "Step 2: Compiling engine sources..."
echo "-----------------------------------"

# Read required engines from file
ENGINES=$(cat required_engines.txt)

for ENGINE_PATH in ${ENGINES}; do
    ENGINE_NAME=$(basename ${ENGINE_PATH} .cpp)
    OBJ_FILE="${OBJ_DIR}/${ENGINE_NAME}.o"

    if [ ! -f "${OBJ_FILE}" ]; then
        echo "  Compiling ${ENGINE_NAME}..."
        ${CXX} ${CPP_FLAGS} ${INCLUDES} ${DEFINES} -c \
            ${PLUGIN_SRC}/${ENGINE_NAME}.cpp \
            -o ${OBJ_FILE}
    fi
done

echo ""
echo "Step 3: Compiling test source..."
echo "-----------------------------------"

echo "  Compiling test_buffer_size_independence.cpp..."
${CXX} ${CPP_FLAGS} ${INCLUDES} ${DEFINES} -c \
    test_buffer_size_independence.cpp \
    -o ${OBJ_DIR}/test_buffer_size_independence.o

echo ""
echo "Step 4: Linking executable..."
echo "-----------------------------------"

# Collect all object files
JUCE_OBJS=""
for MODULE in ${JUCE_MODULES}; do
    JUCE_OBJS="${JUCE_OBJS} ${OBJ_DIR}/${MODULE}.o"
done
JUCE_OBJS="${JUCE_OBJS} ${OBJ_DIR}/SheenBidi.o ${OBJ_DIR}/juce_core_CompilationTime.o"

ENGINE_OBJS=""
for ENGINE_PATH in ${ENGINES}; do
    ENGINE_NAME=$(basename ${ENGINE_PATH} .cpp)
    ENGINE_OBJS="${ENGINE_OBJS} ${OBJ_DIR}/${ENGINE_NAME}.o"
done

echo "  Linking test_buffer_size_independence..."
${CXX} ${CPP_FLAGS} \
    ${OBJ_DIR}/test_buffer_size_independence.o \
    ${JUCE_OBJS} \
    ${ENGINE_OBJS} \
    ${FRAMEWORKS} \
    -L/opt/homebrew/lib -lharfbuzz \
    -o ${BUILD_DIR}/test_buffer_size_independence

echo ""
echo "=========================================================================="
echo "  Build complete!"
echo "=========================================================================="
echo ""
echo "Executable: ${BUILD_DIR}/test_buffer_size_independence"
echo ""
echo "To run the test:"
echo "  cd ${BUILD_DIR}"
echo "  ./test_buffer_size_independence"
echo ""
echo "This will generate:"
echo "  - buffer_size_independence_report.txt"
echo "  - buffer_size_independence_results.csv"
echo ""
