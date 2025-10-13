#!/bin/bash
# Build all modulation engine source files

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║    Compiling Modulation Engines                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

JUCE_DIR="/Users/Branden/JUCE"
PLUGIN_SRC="../JUCE_Plugin/Source"
BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

mkdir -p "$OBJ_DIR"

CPP_FLAGS="-std=c++17 -O2 -Wall -Wno-unused-parameter -Wno-unused-variable"
INCLUDES="-I. -I$PLUGIN_SRC -I$PLUGIN_SRC/../JuceLibraryCode -I$JUCE_DIR/modules"
DEFINES="-DJUCE_STANDALONE_APPLICATION=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DNDEBUG=1 -DJUCE_USE_FREETYPE=0 -DJUCE_USE_CORETEXT_LAYOUT=1"

# List of modulation engines to compile
ENGINES=(
    "StereoChorus"
    "ResonantChorus_Platinum"
    "AnalogPhaser"
    "PlatinumRingModulator"
    "AnalogRingModulator"
    "FrequencyShifter"
    "HarmonicTremolo"
    "ClassicTremolo"
    "RotarySpeaker_Platinum"
    "RotarySpeaker"
)

echo "Compiling engine source files..."
for engine in "${ENGINES[@]}"; do
    SRC_FILE="$PLUGIN_SRC/${engine}.cpp"
    OBJ_FILE="$OBJ_DIR/${engine}.o"

    if [ -f "$SRC_FILE" ]; then
        if [ ! -f "$OBJ_FILE" ] || [ "$SRC_FILE" -nt "$OBJ_FILE" ]; then
            echo "  - Compiling $engine..."
            clang++ $CPP_FLAGS $INCLUDES $DEFINES \
                -c "$SRC_FILE" \
                -o "$OBJ_FILE"

            if [ $? -ne 0 ]; then
                echo "    ✗ Failed to compile $engine"
                exit 1
            fi
        else
            echo "  - $engine (up to date)"
        fi
    else
        echo "  - Warning: $SRC_FILE not found"
    fi
done

echo ""
echo "✓ All modulation engines compiled successfully"
