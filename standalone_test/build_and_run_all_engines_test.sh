#!/bin/bash

# Build and run comprehensive test for ALL 57 engines
# NO HALLUCINATIONS - Real compilation, real execution, real results

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "========================================"
echo "Building ALL 57 Engines Test"
echo "========================================"

# Clean previous build
rm -f test_all_57_engines_comprehensive
rm -f all_engines_test_results.csv

# Compile
echo "Compiling test program..."
clang++ -std=c++17 -O2 \
    -I. \
    -I../JUCE_Plugin/Source \
    -I../JUCE_Plugin/JuceLibraryCode \
    -I/Users/Branden/JUCE/modules \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DNDEBUG=1 \
    test_all_57_engines_comprehensive.cpp \
    ../JUCE_Plugin/Builds/MacOSX/build/Release/libChimeraPhoenix.a \
    -framework CoreAudio \
    -framework CoreFoundation \
    -framework Accelerate \
    -framework AudioToolbox \
    -framework CoreMIDI \
    -framework Cocoa \
    -framework IOKit \
    -o test_all_57_engines_comprehensive

if [ $? -ne 0 ]; then
    echo "❌ Compilation FAILED"
    exit 1
fi

echo "✓ Compilation successful"
echo ""
echo "========================================"
echo "Running Test (this will take ~10 minutes)"
echo "========================================"
echo ""

# Run the test
./test_all_57_engines_comprehensive | tee all_engines_test_output.txt

EXIT_CODE=$?

echo ""
echo "========================================"
echo "Test Complete"
echo "========================================"
echo "Exit code: $EXIT_CODE"
echo "Results saved to:"
echo "  - all_engines_test_results.csv (detailed)"
echo "  - all_engines_test_output.txt (full log)"

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ ALL ENGINES PASSED"
else
    echo "❌ SOME ENGINES FAILED - See output above"
fi

exit $EXIT_CODE
