#!/bin/bash
# Master build script for ChimeraPhoenix Standalone Test Suite
# Compiles all test executables with proper dependency management

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  ChimeraPhoenix Standalone Test Suite - Master Build      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Configuration
JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

# Check JUCE exists
if [ ! -d "$JUCE_DIR/modules" ]; then
    echo "ERROR: JUCE not found at $JUCE_DIR"
    exit 1
fi

# Create build directories
mkdir -p "$BUILD_DIR"
mkdir -p "$OBJ_DIR"

# Common flags
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework AudioToolbox -framework Cocoa -framework IOKit -framework Security -framework QuartzCore -framework CoreImage -framework CoreGraphics -framework CoreText -framework WebKit -framework DiscRecording"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1 -DJUCE_BUILD_INFO_SUFFIX=\"\""

# Compiler flags for C++
CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"

# Compiler flags for Objective-C++ (JUCE modules)
OBJCPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable -x objective-c++"

# Track success/failure
BUILD_SUCCESS=()
BUILD_FAILED=()

# ============================================================================
# STEP 1: Compile JUCE Modules
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 1: Compiling JUCE modules (Objective-C++)"
echo "════════════════════════════════════════════════════════════"
JUCE_OBJS=""

JUCE_MODULES="juce_core juce_audio_basics juce_audio_formats juce_audio_processors juce_dsp juce_events juce_data_structures juce_graphics juce_gui_basics juce_gui_extra"

for module in $JUCE_MODULES; do
    if [ -f "$OBJ_DIR/$module.o" ]; then
        echo "  ✓ $module (cached)"
    else
        echo "  → $module..."
        if clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
            -c "$JUCE_DIR/modules/$module/$module.cpp" \
            -o "$OBJ_DIR/$module.o" 2>&1; then
            echo "    ✓ compiled"
        else
            echo "    ✗ FAILED"
            BUILD_FAILED+=("JUCE:$module")
        fi
    fi
    JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/$module.o"
done

# Compile juce_core_CompilationTime (build metadata)
# NOTE: This replaces the old juce_build_info.cpp which caused duplicate symbols
if [ -f "$OBJ_DIR/juce_core_CompilationTime.o" ]; then
    echo "  ✓ juce_core_CompilationTime (cached)"
else
    echo "  → juce_core_CompilationTime (build metadata)..."
    if clang++ $OBJCPP_FLAGS $INCLUDES $DEFINES \
        -c "$JUCE_DIR/modules/juce_core/juce_core_CompilationTime.cpp" \
        -o "$OBJ_DIR/juce_core_CompilationTime.o" 2>&1; then
        echo "    ✓ compiled"
    else
        echo "    ✗ FAILED"
        BUILD_FAILED+=("JUCE:juce_core_CompilationTime")
    fi
fi
JUCE_OBJS="$JUCE_OBJS $OBJ_DIR/juce_core_CompilationTime.o"

echo ""

# ============================================================================
# STEP 2: Compile SheenBidi Library
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 2: Compiling SheenBidi library (C)"
echo "════════════════════════════════════════════════════════════"

if [ -f "$OBJ_DIR/SheenBidi.o" ]; then
    echo "  ✓ SheenBidi (cached)"
else
    echo "  → SheenBidi..."
    if clang -std=c11 -O2 $INCLUDES -DSB_CONFIG_UNITY \
        -c "$JUCE_DIR/modules/juce_graphics/unicode/sheenbidi/Source/SheenBidi.c" \
        -o "$OBJ_DIR/SheenBidi.o" 2>&1; then
        echo "    ✓ compiled"
    else
        echo "    ✗ FAILED"
        BUILD_FAILED+=("SheenBidi")
    fi
fi
SHEENBIDI_OBJ="$OBJ_DIR/SheenBidi.o"

echo ""

# ============================================================================
# STEP 3: Compile Engine Sources
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 3: Compiling engine sources (C++)"
echo "════════════════════════════════════════════════════════════"

ENGINE_OBJS=""
ENGINE_COUNT=0
while IFS= read -r filename; do
    filepath="$PLUGIN_SRC/$filename"
    if [ -f "$filepath" ]; then
        basename=$(basename "$filename" .cpp)
        if [ -f "$OBJ_DIR/$basename.o" ]; then
            echo "  ✓ $basename (cached)"
        else
            echo "  → $basename..."
            if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
                -c "$filepath" \
                -o "$OBJ_DIR/$basename.o" 2>&1; then
                echo "    ✓ compiled"
            else
                echo "    ✗ FAILED"
                BUILD_FAILED+=("Engine:$basename")
            fi
        fi
        ENGINE_OBJS="$ENGINE_OBJS $OBJ_DIR/$basename.o"
        ((ENGINE_COUNT++))
    else
        echo "  ⚠ Warning: $filepath not found"
    fi
done < required_engines.txt

echo ""
echo "  Total engines: $ENGINE_COUNT"
echo ""

# ============================================================================
# STEP 4: Compile and Link Test Executables
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "Step 4: Building test executables"
echo "════════════════════════════════════════════════════════════"
echo ""

# Get all non-test object files for linking
# Exclude all test mains and debug test files
ALL_LIB_OBJS=$(find "$OBJ_DIR" -name "*.o" \
    ! -name "*_test.o" \
    ! -name "*_test_simple.o" \
    ! -name "standalone_test.o" \
    ! -name "test_*.o" \
    ! -name "reverb_test.o" \
    ! -name "validate_reverb_test.o" \
    | sort | uniq)

# Define test executables
# Format: "test_name:source_file:display_name"
TESTS=(
    "standalone_test:standalone_test.cpp:Main Standalone Test"
    "reverb_test:reverb_test.cpp:Reverb Deep Analysis"
    "validate_reverb_test:validate_reverb_test.cpp:Reverb Validation"
    "filter_test:filter_test.cpp:Filter & EQ Analysis"
    "distortion_test:distortion_test.cpp:Distortion Analysis"
    "dynamics_test:dynamics_test.cpp:Dynamics Analysis"
    "modulation_test:modulation_test.cpp:Modulation Analysis"
    "pitch_test:pitch_test.cpp:Pitch Shift Analysis"
    "spatial_test:spatial_test.cpp:Spatial FX Analysis"
    "utility_test:utility_test.cpp:Utility Analysis"
)

# Build each test
for test_def in "${TESTS[@]}"; do
    IFS=':' read -r test_name test_source test_display <<< "$test_def"

    if [ ! -f "$test_source" ]; then
        echo "[$test_name] ⊘ Source file not found: $test_source"
        continue
    fi

    echo "[$test_name] $test_display"
    echo "  → Compiling $test_source..."

    # Compile test file
    if clang++ $CPP_FLAGS $INCLUDES $DEFINES \
        -c "$test_source" \
        -o "$OBJ_DIR/${test_name}.o" 2>&1; then
        echo "    ✓ compiled"
    else
        echo "    ✗ Compilation failed"
        BUILD_FAILED+=("Test:$test_name")
        continue
    fi

    # Link executable
    echo "  → Linking..."
    if clang++ $CPP_FLAGS \
        "$OBJ_DIR/${test_name}.o" \
        $ALL_LIB_OBJS \
        $FRAMEWORKS \
        -L/opt/homebrew/lib -lharfbuzz \
        -o "$BUILD_DIR/$test_name" 2>&1; then
        echo "    ✓ $BUILD_DIR/$test_name"
        BUILD_SUCCESS+=("$test_name")
    else
        echo "    ✗ Linking failed"
        BUILD_FAILED+=("Test:$test_name")
    fi

    echo ""
done

# ============================================================================
# Build Summary
# ============================================================================

echo "════════════════════════════════════════════════════════════"
echo "BUILD SUMMARY"
echo "════════════════════════════════════════════════════════════"
echo ""

if [ ${#BUILD_SUCCESS[@]} -gt 0 ]; then
    echo "✓ Successfully built ${#BUILD_SUCCESS[@]} test executables:"
    for test in "${BUILD_SUCCESS[@]}"; do
        echo "    • $test"
    done
    echo ""
fi

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

echo "════════════════════════════════════════════════════════════"
echo "BUILD SUCCESSFUL"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "Run tests with:"
echo "  cd $BUILD_DIR && ./<test_name>"
echo ""
echo "Available tests:"
for test in "${BUILD_SUCCESS[@]}"; do
    echo "  • ./$test"
done
echo ""
