#!/bin/bash

echo "============================================================"
echo "Building Distortion Real-World Test"
echo "============================================================"

# Set paths
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_DIR="$PROJECT_ROOT/JUCE"
STANDALONE_DIR="$PROJECT_ROOT/standalone_test"

echo ""
echo "Step 0: Generating test materials..."
cd "$STANDALONE_DIR"
python3 generate_distortion_test_materials.py
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to generate test materials"
    exit 1
fi
echo ""

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O3 -I$PROJECT_ROOT -I$JUCE_DIR/modules -I$STANDALONE_DIR -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"

# JUCE frameworks (macOS - minimal for audio only)
FRAMEWORKS="-framework Accelerate -framework AudioToolbox -framework CoreAudio -framework CoreMIDI -framework Foundation"

# Output directory
mkdir -p build

echo ""
echo "Step 1: Compiling distortion test..."
$CXX $CXXFLAGS \
    -c "$STANDALONE_DIR/test_distortion_realworld.cpp" \
    -o "$STANDALONE_DIR/build/test_distortion_realworld.o" \
    || { echo "ERROR: Test compilation failed"; exit 1; }

echo "Step 2: Compiling distortion engine factory..."
$CXX $CXXFLAGS \
    -c "$STANDALONE_DIR/DistortionEngineFactory.cpp" \
    -o "$STANDALONE_DIR/build/DistortionEngineFactory.o" \
    || { echo "ERROR: Factory compilation failed"; exit 1; }

echo "Step 3: Compiling engine sources..."
PLUGIN_DIR="$PROJECT_ROOT/JUCE_Plugin/Source"

# List of engine sources needed (15-22)
ENGINE_SOURCES=(
    "$PLUGIN_DIR/VintageTubePreamp_Studio.cpp"
    "$PLUGIN_DIR/WaveFolder.cpp"
    "$PLUGIN_DIR/HarmonicExciter_Platinum.cpp"
    "$PLUGIN_DIR/BitCrusher.cpp"
    "$PLUGIN_DIR/MultibandSaturator.cpp"
    "$PLUGIN_DIR/MuffFuzz.cpp"
    "$PLUGIN_DIR/RodentDistortion.cpp"
    "$PLUGIN_DIR/KStyleOverdrive.cpp"
)

ENGINE_OBJS=""
for src in "${ENGINE_SOURCES[@]}"; do
    basename=$(basename "$src" .cpp)
    objfile="$STANDALONE_DIR/build/${basename}.o"
    ENGINE_OBJS="$ENGINE_OBJS $objfile"

    if [ ! -f "$objfile" ] || [ "$src" -nt "$objfile" ]; then
        echo "  Compiling $basename..."
        $CXX $CXXFLAGS \
            -c "$src" \
            -o "$objfile" \
            || { echo "ERROR: Failed to compile $basename"; exit 1; }
    fi
done

echo "Step 4: Locating precompiled JUCE modules (audio only)..."
JUCE_OBJS=""
# Only use core audio modules - no GUI/graphics to avoid extra framework dependencies
JUCE_MODULES=("juce_core" "juce_audio_basics")

for module in "${JUCE_MODULES[@]}"; do
    objfile="$STANDALONE_DIR/${module}.o"
    if [ -f "$objfile" ]; then
        JUCE_OBJS="$JUCE_OBJS $objfile"
        echo "  Found $module.o"
    else
        echo "  Warning: $module.o not found, may cause linking issues"
    fi
done

echo "Step 5: Compiling JUCE stubs..."
if [ ! -f "$STANDALONE_DIR/build/juce_stubs.o" ]; then
    $CXX $CXXFLAGS \
        -c "$STANDALONE_DIR/juce_stubs.cpp" \
        -o "$STANDALONE_DIR/build/juce_stubs.o" \
        || { echo "ERROR: JUCE stubs compilation failed"; exit 1; }
fi

echo "Step 6: Linking executable..."
$CXX \
    "$STANDALONE_DIR/build/test_distortion_realworld.o" \
    "$STANDALONE_DIR/build/DistortionEngineFactory.o" \
    $ENGINE_OBJS \
    $JUCE_OBJS \
    "$STANDALONE_DIR/build/juce_stubs.o" \
    $FRAMEWORKS \
    -o "$STANDALONE_DIR/test_distortion_realworld" \
    || { echo "ERROR: Linking failed"; exit 1; }

echo ""
echo "============================================================"
echo "✅ BUILD SUCCESSFUL"
echo "============================================================"
echo ""
echo "Run the test:"
echo "  cd $STANDALONE_DIR"
echo "  ./test_distortion_realworld"
echo ""
echo "To run test now:"
cd "$STANDALONE_DIR"
./test_distortion_realworld
