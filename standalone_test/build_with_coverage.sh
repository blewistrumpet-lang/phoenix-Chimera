#!/bin/bash
# Code Coverage Build Script for ChimeraPhoenix
# Builds all engines and tests with coverage instrumentation
# Supports both gcov and llvm-cov

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  ChimeraPhoenix Code Coverage Build                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build_coverage"
OBJ_DIR="$BUILD_DIR/obj"
COVERAGE_DIR="$BUILD_DIR/coverage"

# Check JUCE exists
if [ ! -d "$JUCE_DIR/modules" ]; then
    echo "ERROR: JUCE not found at $JUCE_DIR"
    exit 1
fi

# Clean and create build directories
echo "Cleaning previous coverage data..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"
mkdir -p "$COVERAGE_DIR"

# Common includes and frameworks
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1 -DJUCE_BUILD_INFO_SUFFIX=\"\""

# Coverage flags for LLVM (clang)
# Using -fprofile-instr-generate and -fcoverage-mapping for llvm-cov
COVERAGE_FLAGS="-fprofile-instr-generate -fcoverage-mapping -O0 -g"

# Compiler flags with coverage instrumentation
CPP_FLAGS="-std=c++17 -Wall -Wno-unused-parameter -Wno-unused-variable $COVERAGE_FLAGS"
OBJCPP_FLAGS="-std=c++17 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++ $COVERAGE_FLAGS"

# Track build status
BUILD_SUCCESS=()
BUILD_FAILED=()

# ============================================================================
# STEP 1: Compile JUCE Modules with Coverage
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 1: Compiling JUCE modules (with coverage)"
echo "════════════════════════════════════════════════════════════"
JUCE_OBJS=""

JUCE_MODULES="juce_core juce_audio_basics juce_audio_formats juce_audio_processors juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra"

for module in $JUCE_MODULES; do
    echo "  → $module..."
    if clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
        -c "$JUCE_DIR/modules/$module/$module.cpp" \
        -o "$OBJ_DIR/$module.o" 2>&1; then
        echo "    ✓ compiled"
    else
        echo "    ✗ FAILED"
        BUILD_FAILED+=("JUCE:$module")
    fi
    JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/$module.o"
done

# Compile juce_core_CompilationTime
echo "  → juce_core_CompilationTime..."
if clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
    -c "$JUCE_DIR/modules/juce_core/juce_core_CompilationTime.cpp" \
    -o "$OBJ_DIR/juce_core_CompilationTime.o" 2>&1; then
    echo "    ✓ compiled"
else
    echo "    ✗ FAILED"
    BUILD_FAILED+=("JUCE:juce_core_CompilationTime")
fi
JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/juce_core_CompilationTime.o"

echo ""

# ============================================================================
# STEP 2: Compile SheenBidi Library
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 2: Compiling SheenBidi library"
echo "════════════════════════════════════════════════════════════"

echo "  → SheenBidi..."
if clang -std=c11 -O0 -g -fprofile-instr-generate -fcoverage-mapping $INCLUDES -DSB_CONFIG_UNITY \
    -c "$JUCE_DIR/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" \
    -o "$OBJ_DIR/SheenBidi.o" 2>&1; then
    echo "    ✓ compiled"
else
    echo "    ✗ FAILED"
    BUILD_FAILED+=("SheenBidi")
fi
SHEENBIDI_OBJ="$OBJ_DIR/SheenBidi.o"

echo ""

# ============================================================================
# STEP 3: Compile Engine Sources with Coverage
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 3: Compiling engine sources (with coverage)"
echo "════════════════════════════════════════════════════════════"

ENGINE_OBJS=""
ENGINE_COUNT=0
while IFS= read -r filename; do
    filepath="$PLUGIN_SRC/$filename"
    if [ -f "$filepath" ]; then
        basename=$(basename "$filename" .cpp)
        echo "  → $basename..."
        if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
            -c "$filepath" \
            -o "$OBJ_DIR/$basename.o" 2>&1; then
            echo "    ✓ compiled"
        else
            echo "    ✗ FAILED"
            BUILD_FAILED+=("Engine:$basename")
        fi
        ENGINE_OBJS="$ENGINE_OBJS $OBJ_DIR/$basename.o"
        ((ENGINE_COUNT++))
    else
        echo "  ⚠ Warning: $filepath not found"
    fi
done < required_engines.txt

echo ""
echo "  Total engines compiled: $ENGINE_COUNT"
echo ""

# ============================================================================
# STEP 4: Build Coverage Test Executable
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 4: Building coverage test executable"
echo "════════════════════════════════════════════════════════════"
echo ""

# Get all object files (excluding test mains we'll compile separately)
ALL_LIB_OBJS=$(find "$OBJ_DIR" -name "*.o" | sort | uniq)

echo "  → Compiling coverage_test.cpp..."
if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
    -c "coverage_test.cpp" \
    -o "$OBJ_DIR/coverage_test.o" 2>&1; then
    echo "    ✓ compiled"
else
    echo "    ✗ Compilation failed"
    BUILD_FAILED+=("Test:coverage_test")
    exit 1
fi

echo "  → Linking coverage_test..."
if clang++ $CPP_FLAGS \
    "$OBJ_DIR/coverage_test.o" \
    $ALL_LIB_OBJS \
    $FRAMEWORKS \
    -L/opt/homebrew/lib -lharfbuzz \
    -o "$BUILD_DIR/coverage_test" 2>&1; then
    echo "    ✓ $BUILD_DIR/coverage_test"
    BUILD_SUCCESS+=("coverage_test")
else
    echo "    ✗ Linking failed"
    BUILD_FAILED+=("Test:coverage_test")
    exit 1
fi

echo ""

# ============================================================================
# Build Summary
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "COVERAGE BUILD SUMMARY"
echo "════════════════════════════════════════════════════════════"
echo ""

if [ ${#BUILD_FAILED[@]} -gt 0 ]; then
    echo "✗ Failed builds (${#BUILD_FAILED[@]}):"
    for failed in "${BUILD_FAILED[@]}"; do
        echo "    • $failed"
    done
    echo ""
    echo "════════════════════════════════════════════════════════════"
    echo "BUILD COMPLETED WITH ERRORS"
    echo "════════════════════════════════════════════════════════════"
    exit 1
fi

echo "✓ Coverage-instrumented build successful!"
echo ""
echo "Next steps:"
echo "  1. Run: ./run_coverage_tests.sh"
echo "  2. View: open $COVERAGE_DIR/index.html"
echo ""
echo "Build artifacts:"
echo "  • Executable: $BUILD_DIR/coverage_test"
echo "  • Object files: $OBJ_DIR/"
echo "  • Coverage data: $COVERAGE_DIR/ (generated after test run)"
echo ""
