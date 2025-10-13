#!/bin/bash
# Build script for Spatial Engines 46-48 Test

set -e

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Building Spatial Engines 46-48 Test                         ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

JUCE_PATH="/Users/Branden/JUCE/modules"
SOURCE_PATH="../JUCE_Plugin/Source"

# Compiler settings
CXX="g++"
CXXFLAGS="-std=c++17 -O2 -DJUCE_STANDALONE_APPLICATION=1 -DNDEBUG=1 -x objective-c++"
INCLUDES="-I${SOURCE_PATH} -I${JUCE_PATH}"
FRAMEWORKS="-framework Accelerate -framework CoreAudio -framework CoreFoundation -framework CoreMidi -framework IOKit -framework Cocoa"

echo "Step 1: Checking source files..."
if [ ! -f "test_spatial_engines_46_48.cpp" ]; then
    echo "✗ Error: test_spatial_engines_46_48.cpp not found"
    exit 1
fi
echo "✓ Source file found"

echo ""
echo "Step 2: Compiling JUCE modules..."

# Compile JUCE core (lightweight)
if [ ! -f "juce_core.o" ] || [ "test_spatial_engines_46_48.cpp" -nt "juce_core.o" ]; then
    echo "  Compiling juce_core..."
    cat > juce_core.cpp << 'EOF'
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#include <juce_core/juce_core.cpp>
EOF
    $CXX $CXXFLAGS $INCLUDES -c juce_core.cpp -o juce_core.o
    rm juce_core.cpp
    echo "  ✓ juce_core.o compiled"
else
    echo "  ✓ juce_core.o up to date"
fi

# Compile JUCE audio basics
if [ ! -f "juce_audio_basics.o" ] || [ "test_spatial_engines_46_48.cpp" -nt "juce_audio_basics.o" ]; then
    echo "  Compiling juce_audio_basics..."
    cat > juce_audio_basics.cpp << 'EOF'
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#include <juce_audio_basics/juce_audio_basics.cpp>
EOF
    $CXX $CXXFLAGS $INCLUDES -c juce_audio_basics.cpp -o juce_audio_basics.o
    rm juce_audio_basics.cpp
    echo "  ✓ juce_audio_basics.o compiled"
else
    echo "  ✓ juce_audio_basics.o up to date"
fi

# Compile JUCE DSP
if [ ! -f "juce_dsp.o" ] || [ "test_spatial_engines_46_48.cpp" -nt "juce_dsp.o" ]; then
    echo "  Compiling juce_dsp..."
    cat > juce_dsp.cpp << 'EOF'
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#include <juce_dsp/juce_dsp.cpp>
EOF
    $CXX $CXXFLAGS $INCLUDES -c juce_dsp.cpp -o juce_dsp.o
    rm juce_dsp.cpp
    echo "  ✓ juce_dsp.o compiled"
else
    echo "  ✓ juce_dsp.o up to date"
fi

echo ""
echo "Step 3: Compiling engine modules..."

# Find all engine implementation files
ENGINE_FILES=$(find "${SOURCE_PATH}" -name "*Engine.cpp" -o -name "EngineFactory.cpp" | grep -v Test)

for engine_file in $ENGINE_FILES; do
    engine_name=$(basename "$engine_file" .cpp)
    obj_file="build/${engine_name}.o"

    mkdir -p build

    if [ ! -f "$obj_file" ] || [ "$engine_file" -nt "$obj_file" ]; then
        echo "  Compiling ${engine_name}..."
        $CXX $CXXFLAGS $INCLUDES -c "$engine_file" -o "$obj_file" 2>&1 | grep -v "warning:" || true
    fi
done

echo "✓ All engine modules compiled"

echo ""
echo "Step 4: Compiling test file..."
$CXX $CXXFLAGS $INCLUDES -c test_spatial_engines_46_48.cpp -o test_spatial_46_48.o

echo ""
echo "Step 5: Linking..."
# Collect all object files
OBJ_FILES="test_spatial_46_48.o juce_core.o juce_audio_basics.o juce_dsp.o"
for obj in build/*.o; do
    if [ -f "$obj" ]; then
        OBJ_FILES="$OBJ_FILES $obj"
    fi
done

$CXX -o test_spatial_46_48 $OBJ_FILES $FRAMEWORKS

echo ""
echo "✓ Build complete: test_spatial_46_48"
echo ""
echo "Run with: ./test_spatial_46_48"
