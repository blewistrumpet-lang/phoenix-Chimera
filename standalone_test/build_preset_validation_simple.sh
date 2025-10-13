#!/bin/bash

echo "============================================"
echo "Building Simple Preset Validation System"
echo "============================================"

PROJECT_ROOT="/Users/Branden/branden/Project_Chimera_v3.0_Phoenix"
JUCE_PATH="$PROJECT_ROOT/JUCE"
TEST_DIR="$PROJECT_ROOT/standalone_test"
OUTPUT="$TEST_DIR/preset_validation_test"

echo ""
echo "[1/3] Cleaning previous build..."
rm -f "$OUTPUT"

echo ""
echo "[2/3] Compiling..."

clang++ -std=c++17 \
    -I"$JUCE_PATH/modules" \
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
    -DJUCE_STANDALONE_APPLICATION=1 \
    -DJUCE_USE_CURL=0 \
    -DJUCE_WEB_BROWSER=0 \
    -DNDEBUG=1 \
    test_preset_validation_simple.cpp \
    juce_compilation_stub.cpp \
    "$JUCE_PATH/modules/juce_core/juce_core.mm" \
    "$JUCE_PATH/modules/juce_data_structures/juce_data_structures.mm" \
    "$JUCE_PATH/modules/juce_events/juce_events.mm" \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreFoundation \
    -framework Security \
    -o "$OUTPUT"

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Compilation failed"
    exit 1
fi

echo ""
echo "[3/3] Build successful!"
echo "Executable: $OUTPUT"
echo ""

echo "============================================"
echo "Running Preset Validation"
echo "============================================"
echo ""

"$OUTPUT"

EXIT_CODE=$?

echo ""
echo "============================================"
echo "Validation complete!"
echo "Exit code: $EXIT_CODE"
echo "============================================"

exit $EXIT_CODE
