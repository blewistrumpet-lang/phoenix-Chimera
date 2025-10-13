#!/bin/bash

# Build script for Bug Hunting Test Suite
# Comprehensive edge case and boundary condition testing

echo "========================================="
echo "Building Bug Hunting Test Suite"
echo "========================================="

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
JUCE_MODULES="$PROJECT_ROOT/JUCE_Plugin/JuceLibraryCode"
SOURCE_DIR="$PROJECT_ROOT/JUCE_Plugin/Source"
BUILD_DIR="$SCRIPT_DIR/build"

# Clean previous build
echo "Cleaning previous build..."
rm -f "$SCRIPT_DIR/test_bug_hunting"
mkdir -p "$BUILD_DIR"

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O2 -DNDEBUG=1 -DJUCE_STANDALONE_APPLICATION=1"
CXXFLAGS="$CXXFLAGS -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1"
CXXFLAGS="$CXXFLAGS -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1"
CXXFLAGS="$CXXFLAGS -DJUCE_MODULE_AVAILABLE_juce_core=1"
CXXFLAGS="$CXXFLAGS -DJUCE_MODULE_AVAILABLE_juce_dsp=1"
CXXFLAGS="$CXXFLAGS -I$PROJECT_ROOT"
CXXFLAGS="$CXXFLAGS -I$PROJECT_ROOT/JUCE/modules"
CXXFLAGS="$CXXFLAGS -I$JUCE_MODULES"
CXXFLAGS="$CXXFLAGS -I$SOURCE_DIR"
CXXFLAGS="$CXXFLAGS -I$SCRIPT_DIR"
CXXFLAGS="$CXXFLAGS -Wno-deprecated-declarations"

# macOS frameworks
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreMIDI -framework AudioToolbox"
FRAMEWORKS="$FRAMEWORKS -framework CoreFoundation -framework Foundation -framework AppKit"

# Link with standard library
LDFLAGS="-lc++"

echo ""
echo "Step 1: Compiling JUCE stubs..."
$CXX $CXXFLAGS -c "$SCRIPT_DIR/juce_compilation_stub.cpp" -o "$BUILD_DIR/juce_stubs.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile JUCE stubs"
    exit 1
fi

echo ""
echo "Step 2: Compiling test file..."
$CXX $CXXFLAGS -c "$SCRIPT_DIR/test_bug_hunting.cpp" -o "$BUILD_DIR/test_bug_hunting.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile test file"
    exit 1
fi

echo ""
echo "Step 3: Compiling EngineFactory..."
$CXX $CXXFLAGS -c "$SOURCE_DIR/EngineFactory.cpp" -o "$BUILD_DIR/EngineFactory.o"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to compile EngineFactory"
    exit 1
fi

echo ""
echo "Step 4: Finding and compiling all engine implementations..."
ENGINE_OBJECTS=""

# Find all engine .cpp files
for ENGINE_FILE in "$SOURCE_DIR"/*Engine*.cpp; do
    if [ -f "$ENGINE_FILE" ]; then
        BASENAME=$(basename "$ENGINE_FILE" .cpp)

        # Skip EngineFactory (already compiled)
        if [ "$BASENAME" = "EngineFactory" ]; then
            continue
        fi

        echo "  Compiling $BASENAME..."
        $CXX $CXXFLAGS -c "$ENGINE_FILE" -o "$BUILD_DIR/${BASENAME}.o" 2>&1 | grep -v "warning:"

        if [ $? -eq 0 ]; then
            ENGINE_OBJECTS="$ENGINE_OBJECTS $BUILD_DIR/${BASENAME}.o"
        else
            echo "    Warning: Failed to compile $BASENAME, continuing..."
        fi
    fi
done

echo ""
echo "Step 5: Compiling additional DSP components..."
for CPP_FILE in "$SOURCE_DIR"/*.cpp; do
    if [ -f "$CPP_FILE" ]; then
        BASENAME=$(basename "$CPP_FILE" .cpp)

        # Skip if already compiled
        if [[ "$BASENAME" == *"Engine"* ]] || [ "$BASENAME" = "PluginProcessor" ] || [ "$BASENAME" = "PluginEditor" ]; then
            continue
        fi

        if [ ! -f "$BUILD_DIR/${BASENAME}.o" ]; then
            echo "  Compiling $BASENAME..."
            $CXX $CXXFLAGS -c "$CPP_FILE" -o "$BUILD_DIR/${BASENAME}.o" 2>&1 | grep -v "warning:"

            if [ $? -eq 0 ]; then
                ENGINE_OBJECTS="$ENGINE_OBJECTS $BUILD_DIR/${BASENAME}.o"
            fi
        fi
    fi
done

echo ""
echo "Step 6: Linking executable..."
$CXX $BUILD_DIR/test_bug_hunting.o \
     $BUILD_DIR/EngineFactory.o \
     $BUILD_DIR/juce_stubs.o \
     $ENGINE_OBJECTS \
     $FRAMEWORKS $LDFLAGS \
     -o "$SCRIPT_DIR/test_bug_hunting"

if [ $? -ne 0 ]; then
    echo "ERROR: Linking failed"
    exit 1
fi

echo ""
echo "========================================="
echo "Build successful!"
echo "========================================="
echo "Executable: $SCRIPT_DIR/test_bug_hunting"
echo ""
echo "To run: ./test_bug_hunting"
echo ""
