#!/bin/bash

echo "======================================================================"
echo "  BUILDING PITCH ENGINE AUDIO QUALITY ANALYSIS SUITE"
echo "======================================================================"
echo ""

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

BUILD_DIR="build_pitch_quality"

echo "Step 1: Preparing build directory..."
echo "-------------------------------------------------------------------"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "Step 2: Running CMake..."
echo "-------------------------------------------------------------------"

# Copy CMakeLists.txt for this build
cp ../CMakeLists_pitch_quality.txt ../CMakeLists.txt

cmake -DCMAKE_BUILD_TYPE=Release ..

if [ $? -ne 0 ]; then
    echo ""
    echo "======================================================================"
    echo "  CMAKE CONFIGURATION FAILED"
    echo "======================================================================"
    exit 1
fi

echo ""
echo "Step 3: Building with make..."
echo "-------------------------------------------------------------------"
make -j$(sysctl -n hw.ncpu) test_pitch_audio_quality

if [ $? -eq 0 ]; then
    echo ""
    echo "======================================================================"
    echo "  BUILD SUCCESSFUL!"
    echo "======================================================================"
    echo ""

    # Copy executable to parent directory
    if [ -f "test_pitch_audio_quality" ]; then
        cp test_pitch_audio_quality ../
        echo "Executable: test_pitch_audio_quality"
        echo ""
        echo "Run with:"
        echo "  ./test_pitch_audio_quality"
        echo ""
        echo "This will generate:"
        echo "  - PITCH_ENGINE_AUDIO_QUALITY_ANALYSIS.md (comprehensive report)"
        echo ""
    else
        echo "Warning: Executable not found at expected location"
        ls -la
    fi
else
    echo ""
    echo "======================================================================"
    echo "  BUILD FAILED"
    echo "======================================================================"
    exit 1
fi
