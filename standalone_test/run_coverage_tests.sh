#!/bin/bash
# Run Code Coverage Tests and Generate Reports
# Executes instrumented tests and generates coverage data

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  ChimeraPhoenix Code Coverage Test Runner                 ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

BUILD_DIR="./build_coverage"
COVERAGE_DIR="$BUILD_DIR/coverage"
TEST_EXEC="$BUILD_DIR/coverage_test"
PROFDATA_FILE="$BUILD_DIR/coverage_test.profdata"
PROFRAW_FILE="default.profraw"

# Check if build exists
if [ ! -f "$TEST_EXEC" ]; then
    echo "ERROR: Coverage test executable not found!"
    echo "Please run: ./build_with_coverage.sh first"
    exit 1
fi

# Clean previous coverage data
echo "Cleaning previous coverage data..."
rm -f "$PROFRAW_FILE"
rm -f "$PROFDATA_FILE"
rm -rf "$COVERAGE_DIR"
mkdir -p "$COVERAGE_DIR"

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Running Coverage Tests"
echo "════════════════════════════════════════════════════════════"
echo ""

# Set environment variable for raw profile data output
export LLVM_PROFILE_FILE="$PROFRAW_FILE"

# Run the test
if "$TEST_EXEC"; then
    echo ""
    echo "✓ Tests completed successfully"
else
    echo ""
    echo "⚠ Tests completed with errors (coverage data still collected)"
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Processing Coverage Data"
echo "════════════════════════════════════════════════════════════"
echo ""

# Check if profraw file was generated
if [ ! -f "$PROFRAW_FILE" ]; then
    echo "ERROR: Coverage data file not generated!"
    echo "Expected: $PROFRAW_FILE"
    exit 1
fi

echo "Converting raw coverage data to indexed format..."
if xcrun llvm-profdata merge -sparse "$PROFRAW_FILE" -o "$PROFDATA_FILE"; then
    echo "✓ Coverage data indexed: $PROFDATA_FILE"
else
    echo "✗ Failed to index coverage data"
    exit 1
fi

echo ""
echo "Generating coverage reports..."
echo ""

# Get source files for coverage report
PLUGIN_SRC="../JUCE_Plugin/Source"

# Generate HTML report
echo "1. Generating HTML report..."
xcrun llvm-cov show "$TEST_EXEC" \
    -instr-profile="$PROFDATA_FILE" \
    -format=html \
    -output-dir="$COVERAGE_DIR/html" \
    -show-line-counts-or-regions \
    -show-instantiations=false \
    "$PLUGIN_SRC"/*.cpp \
    > /dev/null 2>&1 || echo "  ⚠ HTML generation had warnings"

if [ -f "$COVERAGE_DIR/html/index.html" ]; then
    echo "  ✓ HTML report: $COVERAGE_DIR/html/index.html"
else
    echo "  ⚠ HTML report generation incomplete"
fi

# Generate text summary
echo "2. Generating text summary..."
xcrun llvm-cov report "$TEST_EXEC" \
    -instr-profile="$PROFDATA_FILE" \
    "$PLUGIN_SRC"/*.cpp \
    > "$COVERAGE_DIR/coverage_summary.txt"

if [ -f "$COVERAGE_DIR/coverage_summary.txt" ]; then
    echo "  ✓ Text summary: $COVERAGE_DIR/coverage_summary.txt"
fi

# Generate detailed text report
echo "3. Generating detailed text report..."
xcrun llvm-cov show "$TEST_EXEC" \
    -instr-profile="$PROFDATA_FILE" \
    -show-line-counts-or-regions \
    "$PLUGIN_SRC"/*.cpp \
    > "$COVERAGE_DIR/coverage_detailed.txt"

if [ -f "$COVERAGE_DIR/coverage_detailed.txt" ]; then
    echo "  ✓ Detailed report: $COVERAGE_DIR/coverage_detailed.txt"
fi

# Generate JSON export for programmatic analysis
echo "4. Generating JSON export..."
xcrun llvm-cov export "$TEST_EXEC" \
    -instr-profile="$PROFDATA_FILE" \
    "$PLUGIN_SRC"/*.cpp \
    > "$COVERAGE_DIR/coverage_export.json"

if [ -f "$COVERAGE_DIR/coverage_export.json" ]; then
    echo "  ✓ JSON export: $COVERAGE_DIR/coverage_export.json"
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Coverage Summary"
echo "════════════════════════════════════════════════════════════"
echo ""

# Display summary
if [ -f "$COVERAGE_DIR/coverage_summary.txt" ]; then
    cat "$COVERAGE_DIR/coverage_summary.txt"
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Reports Generated"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "View coverage reports:"
echo "  • HTML: open $COVERAGE_DIR/html/index.html"
echo "  • Text Summary: cat $COVERAGE_DIR/coverage_summary.txt"
echo "  • Detailed: less $COVERAGE_DIR/coverage_detailed.txt"
echo "  • JSON: cat $COVERAGE_DIR/coverage_export.json | python -m json.tool"
echo ""
echo "Test results:"
echo "  • Coverage test results: cat coverage_results.txt"
echo ""
