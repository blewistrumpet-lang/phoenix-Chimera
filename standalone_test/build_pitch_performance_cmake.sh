#!/bin/bash

echo "======================================================"
echo "Building Pitch Engine Performance Profiler with CMake"
echo "======================================================"

STANDALONE_DIR="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test"
BUILD_DIR="${STANDALONE_DIR}/build_perf"

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Copy CMakeLists.txt
cp "${STANDALONE_DIR}/CMakeLists_pitch_performance.txt" "${BUILD_DIR}/CMakeLists.txt"

echo "Configuring with CMake..."
cmake -G "Unix Makefiles" . -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

echo "Building..."
make -j4

if [ $? -eq 0 ]; then
    echo ""
    echo "======================================================"
    echo "BUILD SUCCESSFUL!"
    echo "======================================================"
    echo "Executable: ${BUILD_DIR}/test_pitch_performance"
    echo ""
    echo "Running performance profiler..."
    echo ""
    "${BUILD_DIR}/test_pitch_performance_artefacts/Release/test_pitch_performance" || \
    "${BUILD_DIR}/test_pitch_performance" || \
    find "${BUILD_DIR}" -name "test_pitch_performance" -type f -executable -exec {} \;
else
    echo ""
    echo "ERROR: Build failed!"
    exit 1
fi
