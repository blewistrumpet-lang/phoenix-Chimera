#!/bin/bash
# ChimeraPhoenix Pi Build Verification Script
# Run this on the Raspberry Pi to verify the build

set -e

echo "=========================================="
echo "ChimeraPhoenix Pi Build Verification"
echo "=========================================="
echo ""

PI_PROJECT_DIR="$HOME/phoenix-Chimera/pi_deployment/JUCE_Plugin"
BINARY_PATH="$PI_PROJECT_DIR/Builds/LinuxMakefile/build/ChimeraPhoenix_Pi"
SOURCE_DIR="$PI_PROJECT_DIR/Source"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓${NC} $2"
    else
        echo -e "${RED}✗${NC} $2"
    fi
}

print_info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

echo "1. COMPILATION VERIFICATION"
echo "----------------------------"

# Check if binary exists
if [ -f "$BINARY_PATH" ]; then
    print_status 0 "Binary exists: $BINARY_PATH"

    # Get file info
    FILE_SIZE=$(stat -f%z "$BINARY_PATH" 2>/dev/null || stat -c%s "$BINARY_PATH" 2>/dev/null)
    FILE_SIZE_MB=$((FILE_SIZE / 1024 / 1024))
    print_info "File size: ${FILE_SIZE_MB}MB (${FILE_SIZE} bytes)"

    # Check if modified today
    if [ "$(uname)" = "Darwin" ]; then
        MOD_TIME=$(stat -f "%Sm" -t "%Y-%m-%d" "$BINARY_PATH")
    else
        MOD_TIME=$(stat -c %y "$BINARY_PATH" | cut -d' ' -f1)
    fi
    print_info "Last modified: $MOD_TIME"

    # Check if it's an ELF executable
    FILE_TYPE=$(file "$BINARY_PATH")
    if echo "$FILE_TYPE" | grep -q "ELF.*executable"; then
        print_status 0 "File is ELF executable"
        echo "   $FILE_TYPE"
    else
        print_status 1 "File is NOT an ELF executable"
        echo "   $FILE_TYPE"
    fi

    # Check file size is reasonable (>1MB)
    if [ $FILE_SIZE -gt 1048576 ]; then
        print_status 0 "File size is reasonable (>1MB)"
    else
        print_status 1 "File size is suspiciously small (<1MB)"
    fi
else
    print_status 1 "Binary NOT FOUND: $BINARY_PATH"
    echo ""
    echo "Expected location: $BINARY_PATH"
    exit 1
fi

echo ""
echo "2. SYMBOL VERIFICATION"
echo "----------------------"

# Check for required symbols
SYMBOLS_TO_CHECK=(
    "updateUIFromProgress"
    "stopProgressMonitoring"
    "FileProgressMonitor"
    "sendTrinityRequest"
)

for symbol in "${SYMBOLS_TO_CHECK[@]}"; do
    if nm "$BINARY_PATH" 2>/dev/null | grep -q "$symbol"; then
        print_status 0 "Symbol found: $symbol"
    elif strings "$BINARY_PATH" | grep -q "$symbol"; then
        print_status 0 "Symbol found (via strings): $symbol"
    else
        print_status 1 "Symbol NOT found: $symbol"
    fi
done

echo ""
echo "3. SOURCE CODE CONSISTENCY"
echo "--------------------------"

# Check for PluginEditor_Pi files
if [ -f "$SOURCE_DIR/PluginEditor_Pi.cpp" ]; then
    print_status 0 "PluginEditor_Pi.cpp exists"
    LINE_COUNT=$(wc -l < "$SOURCE_DIR/PluginEditor_Pi.cpp")
    print_info "Line count: $LINE_COUNT lines"

    if [ $LINE_COUNT -eq 936 ]; then
        print_status 0 "Line count matches expected (936 lines)"
    else
        print_status 1 "Line count mismatch (expected 936, got $LINE_COUNT)"
    fi
else
    print_status 1 "PluginEditor_Pi.cpp NOT FOUND"
fi

if [ -f "$SOURCE_DIR/PluginEditor_Pi.h" ]; then
    print_status 0 "PluginEditor_Pi.h exists"

    # Check for currentRequestId in header
    if grep -q "currentRequestId" "$SOURCE_DIR/PluginEditor_Pi.h"; then
        print_status 0 "currentRequestId variable declared in header"
    else
        print_status 1 "currentRequestId variable NOT found in header"
    fi

    # Check for progressMonitor in header
    if grep -q "progressMonitor" "$SOURCE_DIR/PluginEditor_Pi.h"; then
        print_status 0 "progressMonitor variable declared in header"
    else
        print_status 1 "progressMonitor variable NOT found in header"
    fi
else
    print_status 1 "PluginEditor_Pi.h NOT FOUND"
fi

echo ""
echo "4. CRITICAL: PROGRESS FIELD NAMES"
echo "----------------------------------"

if [ -f "$SOURCE_DIR/PluginEditor_Pi.cpp" ]; then
    print_info "Searching for JSON field access in updateUIFromProgress()..."

    # Extract the updateUIFromProgress function
    sed -n '/void.*updateUIFromProgress/,/^}/p' "$SOURCE_DIR/PluginEditor_Pi.cpp" > /tmp/updateUIFromProgress_func.txt

    # Look for progress field accesses
    echo ""
    print_info "Field names found:"

    grep -o 'progress\[\"[^\"]*\"\]' /tmp/updateUIFromProgress_func.txt | sort -u | while read -r field; do
        echo "   - $field"
    done

    # Check for specific common fields
    echo ""
    print_info "Checking specific field names:"

    if grep -q 'progress\[\"stage\"\]' /tmp/updateUIFromProgress_func.txt; then
        print_status 0 "Uses: progress[\"stage\"]"
    else
        print_status 1 "Does NOT use: progress[\"stage\"]"
    fi

    if grep -q 'progress\[\"overall_progress\"\]' /tmp/updateUIFromProgress_func.txt; then
        print_status 0 "Uses: progress[\"overall_progress\"]"
    elif grep -q 'progress\[\"percent\"\]' /tmp/updateUIFromProgress_func.txt; then
        print_status 0 "Uses: progress[\"percent\"]"
    else
        print_status 1 "Does NOT use: progress[\"overall_progress\"] or progress[\"percent\"]"
    fi

    if grep -q 'progress\[\"message\"\]' /tmp/updateUIFromProgress_func.txt; then
        print_status 0 "Uses: progress[\"message\"]"
    else
        print_status 1 "Does NOT use: progress[\"message\"]"
    fi

    if grep -q 'progress\[\"preset_name\"\]' /tmp/updateUIFromProgress_func.txt; then
        print_status 0 "Uses: progress[\"preset_name\"]"
    else
        print_status 1 "Does NOT use: progress[\"preset_name\"]"
    fi

    echo ""
    print_info "Full updateUIFromProgress() function:"
    echo "--------------------------------------"
    cat /tmp/updateUIFromProgress_func.txt

    rm -f /tmp/updateUIFromProgress_func.txt
else
    print_status 1 "Cannot verify field names - PluginEditor_Pi.cpp not found"
fi

echo ""
echo "5. IMPLEMENTATION COUNT VERIFICATION"
echo "-------------------------------------"

if [ -f "$SOURCE_DIR/PluginEditor_Pi.cpp" ] && [ -f "$SOURCE_DIR/PluginEditor_Pi.h" ]; then
    # Count method declarations in header
    HEADER_METHODS=$(grep -c "^[[:space:]]*void\|^[[:space:]]*bool\|^[[:space:]]*int\|^[[:space:]]*String" "$SOURCE_DIR/PluginEditor_Pi.h" || echo "0")

    # Count method implementations in cpp
    CPP_METHODS=$(grep -c "::" "$SOURCE_DIR/PluginEditor_Pi.cpp" || echo "0")

    print_info "Method declarations in header: $HEADER_METHODS"
    print_info "Method implementations in cpp: $CPP_METHODS"

    # List key methods
    echo ""
    print_info "Key methods found in header:"
    grep "void\|bool\|int\|String" "$SOURCE_DIR/PluginEditor_Pi.h" | grep -v "^//" | grep -v "^\s*\*" | head -20
fi

echo ""
echo "=========================================="
echo "OVERALL ASSESSMENT"
echo "=========================================="

# Calculate success criteria
if [ -f "$BINARY_PATH" ] && [ $FILE_SIZE -gt 1048576 ] && echo "$FILE_TYPE" | grep -q "ELF.*executable"; then
    echo -e "${GREEN}BUILD STATUS: SUCCESS${NC}"
    echo ""
    echo "The binary exists, is the correct file type, and has a reasonable size."
    echo ""
    echo "Next steps:"
    echo "1. Test the binary execution"
    echo "2. Verify Trinity AI server connectivity"
    echo "3. Test progress monitoring functionality"
else
    echo -e "${RED}BUILD STATUS: FAIL${NC}"
    echo ""
    echo "Issues detected with the build. Please review the errors above."
fi

echo ""
echo "Verification complete."
echo "=========================================="
