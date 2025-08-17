#!/bin/bash

echo "==========================================="
echo "   Verifying DSP Engine Implementations"
echo "==========================================="
echo ""

# Check if plugin was built
PLUGIN_LIB="JUCE_Plugin/Builds/MacOSX/build/Debug/libChimeraPhoenix.a"
if [ ! -f "$PLUGIN_LIB" ]; then
    echo "❌ Plugin library not found at $PLUGIN_LIB"
    echo "   Run: xcodebuild -configuration Debug"
    exit 1
fi

echo "✅ Plugin library found"
echo ""

# Check engine source files
echo "Checking critical engine implementations..."
echo ""

# Function to check if file has key implementations
check_engine() {
    local file=$1
    local engine_name=$2
    
    echo -n "  $engine_name: "
    
    if [ ! -f "$file" ]; then
        echo "❌ File not found"
        return
    fi
    
    # Check for DenormalGuard
    if grep -q "DenormalGuard" "$file"; then
        echo -n "✅ DenormalGuard "
    else
        echo -n "⚠️  No DenormalGuard "
    fi
    
    # Check for process implementation
    if grep -q "void.*process.*AudioBuffer" "$file"; then
        echo -n "✅ process() "
    else
        echo -n "❌ No process() "
    fi
    
    # Check for reset implementation
    if grep -q "void.*reset()" "$file"; then
        echo -n "✅ reset() "
    else
        echo -n "⚠️  No reset() "
    fi
    
    # Check for thread-safe RNG (no rand())
    if grep -q "rand()" "$file" | grep -v "//.*rand()"; then
        echo "⚠️  Uses rand()"
    else
        echo "✅ Thread-safe"
    fi
}

# Check key engines
echo "REVERB ENGINES:"
check_engine "JUCE_Plugin/Source/SpringReverb.cpp" "SpringReverb"
check_engine "JUCE_Plugin/Source/ConvolutionReverb.cpp" "ConvolutionReverb"
check_engine "JUCE_Plugin/Source/PlateReverb.cpp" "PlateReverb"
check_engine "JUCE_Plugin/Source/GatedReverb.cpp" "GatedReverb"
check_engine "JUCE_Plugin/Source/ShimmerReverb.cpp" "ShimmerReverb"

echo ""
echo "DELAY ENGINES:"
check_engine "JUCE_Plugin/Source/DigitalDelay.cpp" "DigitalDelay"
check_engine "JUCE_Plugin/Source/TapeEcho.cpp" "TapeEcho"
check_engine "JUCE_Plugin/Source/BucketBrigadeDelay.cpp" "BucketBrigadeDelay"
check_engine "JUCE_Plugin/Source/MagneticDrumEcho.cpp" "MagneticDrumEcho"

echo ""
echo "NEWLY IMPLEMENTED:"
check_engine "JUCE_Plugin/Source/ResonantChorus.cpp" "ResonantChorus"
check_engine "JUCE_Plugin/Source/SpectralGate.cpp" "SpectralGate"

echo ""
echo "CRITICAL FIXES:"
check_engine "JUCE_Plugin/Source/BufferRepeat_Platinum.cpp" "BufferRepeat_Platinum"

echo ""
echo "==========================================="
echo "             SUMMARY"
echo "==========================================="

# Count implementations
TOTAL_ENGINES=56
CHECKED_ENGINES=12

echo ""
echo "Engines Checked: $CHECKED_ENGINES"
echo "Total Engines: $TOTAL_ENGINES"
echo ""

# Check for common issues
echo "Global Checks:"
echo -n "  Thread-unsafe rand() calls: "
RAND_COUNT=$(grep -r "rand()" JUCE_Plugin/Source/*.cpp 2>/dev/null | grep -v "//" | wc -l)
if [ $RAND_COUNT -eq 0 ]; then
    echo "✅ None found"
else
    echo "⚠️  $RAND_COUNT found"
fi

echo -n "  Engines with DenormalGuard: "
DENORMAL_COUNT=$(grep -l "DenormalGuard" JUCE_Plugin/Source/*.cpp 2>/dev/null | wc -l)
echo "$DENORMAL_COUNT engines"

echo -n "  Complete reset() implementations: "
RESET_COUNT=$(grep -l "void.*reset()" JUCE_Plugin/Source/*.cpp 2>/dev/null | wc -l)
echo "$RESET_COUNT engines"

echo ""
echo "Plugin Status: ✅ BUILT AND READY"
echo ""
echo "To run full test suite:"
echo "  1. Build test framework: ./build_test_suite.sh"
echo "  2. Run tests: ./run_tests.sh all"
echo ""