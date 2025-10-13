#!/bin/bash

echo "════════════════════════════════════════════════════════════════"
echo "  Building Spatial/Stereo Real-World Test"
echo "  Engines: 46 (StereoImager), 53 (MidSideProcessor), 56 (PhaseAlign)"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Set paths
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_DIR="$PROJECT_ROOT/JUCE"
STANDALONE_DIR="$PROJECT_ROOT/standalone_test"

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O3 -I$PROJECT_ROOT -I$JUCE_DIR/modules -I$STANDALONE_DIR -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"

# JUCE frameworks (macOS - minimal for audio only)
FRAMEWORKS="-framework Accelerate -framework AudioToolbox -framework CoreAudio -framework CoreMIDI -framework Foundation"

# Output
OUTPUT="test_spatial_realworld"

echo "Step 1: Compiling spatial test..."
$CXX $CXXFLAGS \
    -c "$STANDALONE_DIR/test_spatial_realworld.cpp" \
    -o "$STANDALONE_DIR/test_spatial_realworld.o" \
    || { echo "ERROR: Test compilation failed"; exit 1; }

echo "Step 2: Compiling spatial engine sources..."
PLUGIN_DIR="$PROJECT_ROOT/JUCE_Plugin/Source"

# Spatial engines
ENGINE_SOURCES=(
    "$PLUGIN_DIR/StereoImager.cpp"
    "$PLUGIN_DIR/MidSideProcessor_Platinum.cpp"
    "$PLUGIN_DIR/PhaseAlign_Platinum.cpp"
)

ENGINE_OBJS=""
for src in "${ENGINE_SOURCES[@]}"; do
    basename=$(basename "$src" .cpp)
    objfile="$STANDALONE_DIR/${basename}.o"
    ENGINE_OBJS="$ENGINE_OBJS $objfile"

    echo "  Compiling $basename..."
    $CXX $CXXFLAGS \
        -c "$src" \
        -o "$objfile" \
        || { echo "ERROR: Failed to compile $basename"; exit 1; }
done

echo "Step 3: Locating precompiled JUCE modules..."
JUCE_OBJS=""
# Use existing JUCE modules from previous builds
JUCE_MODULES=("juce_core" "juce_audio_basics")

for module in "${JUCE_MODULES[@]}"; do
    objfile="$STANDALONE_DIR/${module}.o"
    if [ -f "$objfile" ]; then
        JUCE_OBJS="$JUCE_OBJS $objfile"
        echo "  Found $module.o"
    else
        echo "  WARNING: Missing $objfile - may cause link errors"
    fi
done

echo "Step 4: Compiling JUCE stubs..."
if [ ! -f "$STANDALONE_DIR/juce_stubs.o" ]; then
    $CXX $CXXFLAGS \
        -c "$STANDALONE_DIR/juce_stubs.cpp" \
        -o "$STANDALONE_DIR/juce_stubs.o" \
        || { echo "ERROR: JUCE stubs compilation failed"; exit 1; }
fi

echo "Step 5: Linking..."
$CXX \
    "$STANDALONE_DIR/test_spatial_realworld.o" \
    $ENGINE_OBJS \
    $JUCE_OBJS \
    "$STANDALONE_DIR/juce_stubs.o" \
    $FRAMEWORKS \
    -o "$STANDALONE_DIR/$OUTPUT" \
    || { echo "ERROR: Linking failed"; exit 1; }

echo ""
echo "✓ Build successful!"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo "  Running Spatial/Stereo Tests"
echo "════════════════════════════════════════════════════════════════"
echo ""

cd "$STANDALONE_DIR"
./$OUTPUT
EXIT_CODE=$?

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "  Test completed with exit code: $EXIT_CODE"
echo "════════════════════════════════════════════════════════════════"

exit $EXIT_CODE
