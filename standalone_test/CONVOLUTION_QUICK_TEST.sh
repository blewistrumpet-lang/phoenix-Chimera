#!/bin/bash
# Quick test script for ConvolutionReverb (Engine 41) after fix

set -e

echo "=========================================="
echo "  ConvolutionReverb Quick Verification"
echo "=========================================="
echo ""

# Recompile ConvolutionReverb with fixes
echo "[Step 1] Recompiling ConvolutionReverb.cpp..."
clang++ -std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable \
    -I. -I../JUCE_Plugin/Source -I../JUCE_Plugin/Source/../JuceLibraryCode \
    -I/Users/Branden/JUCE/modules \
    -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1 \
    -c ../JUCE_Plugin/Source/ConvolutionReverb.cpp \
    -o build/obj/ConvolutionReverb.o

if [ $? -ne 0 ]; then
    echo "✗ Compilation failed!"
    exit 1
fi

echo "✓ Compilation successful"
echo ""

# Check if standalone test exists
if [ ! -f "build/standalone_test" ]; then
    echo "ERROR: standalone_test executable not found!"
    echo "Please run main build first: ./build_v2.sh"
    exit 1
fi

echo "[Step 2] Testing ConvolutionReverb with impulse..."
cd build

# Test 1: Basic impulse response (100% wet, no damping)
echo "  Test 1: Basic impulse (100% wet, no filtering)"
./standalone_test --engine 41 \
    --parameter 0:1.0 \
    --parameter 4:0.0 \
    --duration 5.0 \
    --output ../convolution_test_basic.wav > ../convolution_test_basic.log 2>&1

# Test 2: With damping
echo "  Test 2: With damping=0.5"
./standalone_test --engine 41 \
    --parameter 0:1.0 \
    --parameter 4:0.5 \
    --duration 5.0 \
    --output ../convolution_test_damped.wav > ../convolution_test_damped.log 2>&1

# Test 3: Different IR type
echo "  Test 3: EMT Plate IR (type 1)"
./standalone_test --engine 41 \
    --parameter 0:1.0 \
    --parameter 1:0.25 \
    --duration 5.0 \
    --output ../convolution_test_plate.wav > ../convolution_test_plate.log 2>&1

cd ..

echo ""
echo "[Step 3] Analyzing results..."
echo ""

# Check if Python is available for analysis
if command -v python3 &> /dev/null; then
    # Analyze test outputs
    for testfile in convolution_test_basic.wav convolution_test_damped.wav convolution_test_plate.wav; do
        if [ -f "$testfile" ]; then
            echo "Analyzing $testfile..."
            python3 << EOF
import wave
import numpy as np
try:
    w = wave.open('$testfile', 'r')
    frames = w.readframes(w.getnframes())
    audio = np.frombuffer(frames, dtype=np.int16).astype(np.float32) / 32768.0
    w.close()

    peak = np.max(np.abs(audio))
    rms = np.sqrt(np.mean(audio**2))
    nonzero = np.sum(np.abs(audio) > 0.01)
    total = len(audio)

    print(f"  Peak: {peak:.4f}")
    print(f"  RMS:  {rms:.4f}")
    print(f"  NonZero samples: {nonzero} ({100.0*nonzero/total:.1f}%)")

    # Success criteria
    if peak > 0.1 and rms > 0.01 and nonzero > 1000:
        print("  ✓ PASS")
    else:
        print("  ✗ FAIL - Output too weak or sparse")
    print()
except Exception as e:
    print(f"  Error analyzing: {e}")
    print()
EOF
        else
            echo "  ✗ File not found: $testfile"
        fi
    done
else
    echo "Python3 not found - skipping automated analysis"
    echo "Please manually check output files:"
    ls -lh convolution_test_*.wav
fi

echo ""
echo "=========================================="
echo "  Test Complete"
echo "=========================================="
echo ""
echo "Check logs for diagnostic output:"
echo "  convolution_test_basic.log"
echo "  convolution_test_damped.log"
echo "  convolution_test_plate.log"
echo ""
echo "Look for these diagnostic messages:"
echo "  - 'ConvolutionReverb: Final IR - Length=..., Peak=..., NonZero=...%'"
echo "  - 'ConvolutionReverb: Input=..., Output=...'"
echo ""
echo "Success criteria:"
echo "  - Peak > 0.1"
echo "  - RMS > 0.01"
echo "  - NonZero samples > 1000"
echo "  - No 'IR destroyed' error messages"
echo ""
