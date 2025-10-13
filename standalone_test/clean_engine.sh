#!/bin/bash
# Clean specific engine object files to force recompilation

if [ $# -eq 0 ]; then
    echo "Usage: ./clean_engine.sh <engine_name> [engine_name2...]"
    echo ""
    echo "Examples:"
    echo "  ./clean_engine.sh ConvolutionReverb"
    echo "  ./clean_engine.sh PlateReverb SpringReverb"
    echo ""
    exit 1
fi

BUILD_DIR="./build"
OBJ_DIR="$BUILD_DIR/obj"

if [ ! -d "$OBJ_DIR" ]; then
    echo "No object directory found"
    exit 0
fi

echo "Cleaning engine object files..."
echo "════════════════════════════════════════════════════════════"

REMOVED_COUNT=0

for engine in "$@"; do
    OBJ_FILE="$OBJ_DIR/${engine}.o"
    if [ -f "$OBJ_FILE" ]; then
        rm -f "$OBJ_FILE"
        echo "  ✓ Removed ${engine}.o"
        ((REMOVED_COUNT++))
    else
        echo "  ⊘ ${engine}.o not found"
    fi
done

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Removed $REMOVED_COUNT object files"
echo "Run ./build_all.sh to recompile"
echo "════════════════════════════════════════════════════════════"
