#!/bin/bash
# Clean all build artifacts

echo "Cleaning all build artifacts..."
echo "════════════════════════════════════════════════════════════"

BUILD_DIR="./build"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Nothing to clean - build directory doesn't exist"
    exit 0
fi

# Count items before cleaning
OBJ_COUNT=$(find "$BUILD_DIR/obj" -name "*.o" 2>/dev/null | wc -l)
TEST_COUNT=$(find "$BUILD_DIR" -maxdepth 1 -type f -executable 2>/dev/null | wc -l)

echo "Found:"
echo "  • $OBJ_COUNT object files"
echo "  • $TEST_COUNT executables"
echo ""

# Remove object files
if [ -d "$BUILD_DIR/obj" ]; then
    echo "Removing object files..."
    rm -f "$BUILD_DIR/obj"/*.o
    echo "  ✓ Object files removed"
fi

# Remove executables
echo "Removing test executables..."
for exe in "$BUILD_DIR"/*; do
    if [ -f "$exe" ] && [ -x "$exe" ]; then
        rm -f "$exe"
        echo "  ✓ Removed $(basename $exe)"
    fi
done

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Clean complete!"
echo "════════════════════════════════════════════════════════════"
