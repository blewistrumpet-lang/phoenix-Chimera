#!/bin/bash

echo "=========================================="
echo "Compiling SMBPitchShiftFixed Verification"
echo "=========================================="

JUCE_DIR="../JUCE_Plugin/Source"
TEST_FILE="test_smb_pitchshift_verification.cpp"
SMB_IMPL="${JUCE_DIR}/SMBPitchShiftFixed.cpp"
OUTPUT="test_smb_verification"

# Compiler flags
CXX="clang++"
CXXFLAGS="-std=c++17 -O2 -I${JUCE_DIR}"

echo "Compiling ${TEST_FILE} and ${SMB_IMPL}..."

${CXX} ${CXXFLAGS} ${TEST_FILE} ${SMB_IMPL} -o ${OUTPUT}

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo ""
    echo "=========================================="
    echo "Running Verification Tests"
    echo "=========================================="
    echo ""
    ./${OUTPUT}
    EXIT_CODE=$?
    echo ""
    echo "=========================================="
    echo "Test completed with exit code: ${EXIT_CODE}"
    echo "=========================================="
    exit ${EXIT_CODE}
else
    echo "Compilation failed!"
    exit 1
fi
