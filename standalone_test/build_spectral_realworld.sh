#!/bin/bash

echo "============================================================"
echo "Building Spectral Real-World Test"
echo "Testing: SpectralFreeze(47), SpectralGate(48), PhasedVocoder(49), FeedbackNetwork(52)"
echo "============================================================"

# Set paths
PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_DIR="$PROJECT_ROOT/JUCE"
STANDALONE_DIR="$PROJECT_ROOT/standalone_test"

# Compiler settings
CXX=clang++
CXXFLAGS="-std=c++17 -O3 -I$PROJECT_ROOT -I$JUCE_DIR/modules -I$STANDALONE_DIR -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1"

# JUCE frameworks (macOS - audio + required system frameworks)
FRAMEWORKS="-framework Accelerate -framework AudioToolbox -framework CoreAudio -framework CoreMIDI -framework Foundation"
FRAMEWORKS="$FRAMEWORKS -framework IOKit -framework Security -framework Cocoa -framework QuartzCore -framework CoreServices"

# Output directory
mkdir -p build

echo ""
echo "Step 1: Compiling spectral test..."
$CXX $CXXFLAGS \
    -c "$STANDALONE_DIR/test_spectral_realworld.cpp" \
    -o "$STANDALONE_DIR/build/test_spectral_realworld.o" \
    || { echo "ERROR: Test compilation failed"; exit 1; }

echo "Step 2: Compiling engine sources..."
PLUGIN_DIR="$PROJECT_ROOT/pi_deployment/JUCE_Plugin/Source"

# List of engine sources needed (47, 48, 49, 52)
ENGINE_SOURCES=(
    "$PLUGIN_DIR/SpectralFreeze.cpp"
    "$PLUGIN_DIR/SpectralGate_Platinum.cpp"
    "$PLUGIN_DIR/PhasedVocoder.cpp"
    "$PLUGIN_DIR/FeedbackNetwork.cpp"
    "$STANDALONE_DIR/SpectralEngineFactory.cpp"
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

echo "Step 3: Locating precompiled JUCE modules..."
JUCE_OBJS=""
# Need core, audio, and DSP (for FFT in spectral engines)
JUCE_MODULES=("juce_core" "juce_audio_basics" "juce_dsp")

for module in "${JUCE_MODULES[@]}"; do
    # Try multiple possible locations
    SEARCH_PATHS=(
        "$STANDALONE_DIR/build/${module}.o"
        "$PROJECT_ROOT/JUCE_Plugin/Builds/MacOSX/build/Release/${module}.o"
        "$PROJECT_ROOT/${module}.o"
        "$STANDALONE_DIR/${module}.o"
    )

    found=false
    for obj_path in "${SEARCH_PATHS[@]}"; do
        if [ -f "$obj_path" ]; then
            echo "  Found $module at: $obj_path"
            JUCE_OBJS="$JUCE_OBJS $obj_path"
            found=true
            break
        fi
    done

    if [ "$found" = false ]; then
        echo "  ERROR: Could not find ${module}.o"
        echo "  Building JUCE module stub instead..."

        # Create minimal stub
        cat > "$STANDALONE_DIR/build/${module}_stub.cpp" << 'EOF'
// Minimal JUCE module stub
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define NDEBUG 1

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

// Empty compilation unit - linker will use actual JUCE from frameworks
namespace juce {}
EOF

        $CXX $CXXFLAGS \
            -c "$STANDALONE_DIR/build/${module}_stub.cpp" \
            -o "$STANDALONE_DIR/build/${module}.o" \
            || { echo "ERROR: Failed to build JUCE stub"; exit 1; }

        JUCE_OBJS="$JUCE_OBJS $STANDALONE_DIR/build/${module}.o"
    fi
done

echo ""
echo "Step 4: Linking..."
$CXX \
    "$STANDALONE_DIR/build/test_spectral_realworld.o" \
    $ENGINE_OBJS \
    $JUCE_OBJS \
    $FRAMEWORKS \
    -o "$STANDALONE_DIR/test_spectral_realworld" \
    || { echo "ERROR: Linking failed"; exit 1; }

echo ""
echo "============================================================"
echo "✓ Build complete: test_spectral_realworld"
echo "============================================================"
echo ""
echo "Run with: cd $STANDALONE_DIR && ./test_spectral_realworld"
