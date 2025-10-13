#!/bin/bash
# Clean only test executables (keep object files)

echo "Cleaning test executables..."
echo "════════════════════════════════════════════════════════════"

BUILD_DIR="./build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Nothing to clean - build directory doesn't exist"
    exit 0
fi

# Count executables
TEST_COUNT=$(find "$BUILD_DIR" -maxdepth 1 -type f -executable 2>/dev/null | wc -l)

echo "Found $TEST_COUNT test executables"
echo ""

# Remove executables
for exe in "$BUILD_DIR"/*; do
    if [ -f "$exe" ] && [ -x "$exe" ]; then
        rm -f "$exe"
        echo "  ✓ Removed $(basename $exe)"
    fi
done

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Test executables removed (object files preserved)"
echo "════════════════════════════════════════════════════════════"
