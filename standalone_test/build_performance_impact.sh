#!/bin/bash

# ============================================================================
# PERFORMANCE IMPACT ANALYSIS - BUILD SCRIPT
# ============================================================================
# Compiles the performance impact test suite with optimization flags
# Date: October 11, 2025
# ============================================================================

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build_performance_impact"
SOURCE_FILE="$SCRIPT_DIR/test_performance_impact.cpp"
OUTPUT_BINARY="$BUILD_DIR/test_performance_impact"

echo "=========================================="
echo "PERFORMANCE IMPACT ANALYSIS - BUILD"
echo "=========================================="
echo ""

# Create build directory
echo "[1/4] Creating build directory..."
mkdir -p "$BUILD_DIR"

# Compile with optimizations
echo "[2/4] Compiling performance test suite..."
echo "  Optimization level: -O3 (maximum performance)"
echo "  Compiler: clang++"
echo "  Standards: C++17"
echo ""

clang++ -std=c++17 \
    -O3 \
    -march=native \
    -ffast-math \
    -Wall -Wextra \
    -I. \
    "$SOURCE_FILE" \
    -o "$OUTPUT_BINARY"

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
else
    echo "❌ Compilation failed!"
    exit 1
fi

# Check binary
echo ""
echo "[3/4] Verifying binary..."
if [ -f "$OUTPUT_BINARY" ]; then
    SIZE=$(du -h "$OUTPUT_BINARY" | cut -f1)
    echo "  Binary size: $SIZE"
    echo "  Location: $OUTPUT_BINARY"
    echo "✅ Binary verified!"
else
    echo "❌ Binary not found!"
    exit 1
fi

# Make executable
echo ""
echo "[4/4] Setting permissions..."
chmod +x "$OUTPUT_BINARY"
echo "✅ Permissions set!"

# Summary
echo ""
echo "=========================================="
echo "BUILD COMPLETE"
echo "=========================================="
echo ""
echo "To run performance analysis:"
echo "  cd $SCRIPT_DIR"
echo "  ./build_performance_impact/test_performance_impact"
echo ""
echo "Expected output:"
echo "  - Console: Real-time benchmark results"
echo "  - File: PERFORMANCE_IMPACT_ANALYSIS.md"
echo ""
echo "Performance testing will take ~10-15 minutes"
echo "=========================================="
