#!/bin/bash
# Master Coverage Report Generator
# Orchestrates building, testing, and reporting for code coverage

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  ChimeraPhoenix Master Coverage Report Generator          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check dependencies
echo "Checking dependencies..."

# Check for clang
if ! command -v clang++ &> /dev/null; then
    echo "ERROR: clang++ not found"
    exit 1
fi

# Check for llvm-cov
if ! command -v xcrun &> /dev/null; then
    echo "ERROR: xcrun not found (required for llvm-cov)"
    exit 1
fi

# Check for Python 3
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 not found"
    exit 1
fi

echo "✓ All dependencies found"
echo ""

# Step 1: Build with coverage instrumentation
echo "════════════════════════════════════════════════════════════"
echo "STEP 1: Building with Coverage Instrumentation"
echo "════════════════════════════════════════════════════════════"
echo ""

if [ ! -x "./build_with_coverage.sh" ]; then
    chmod +x ./build_with_coverage.sh
fi

if ./build_with_coverage.sh; then
    echo ""
    echo "✓ Build completed successfully"
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi

echo ""

# Step 2: Run tests and collect coverage
echo "════════════════════════════════════════════════════════════"
echo "STEP 2: Running Tests and Collecting Coverage"
echo "════════════════════════════════════════════════════════════"
echo ""

if [ ! -x "./run_coverage_tests.sh" ]; then
    chmod +x ./run_coverage_tests.sh
fi

if ./run_coverage_tests.sh; then
    echo ""
    echo "✓ Coverage collection completed"
else
    echo ""
    echo "⚠ Coverage collection completed with warnings"
fi

echo ""

# Step 3: Analyze coverage and generate reports
echo "════════════════════════════════════════════════════════════"
echo "STEP 3: Analyzing Coverage and Generating Reports"
echo "════════════════════════════════════════════════════════════"
echo ""

if [ ! -x "./analyze_coverage.py" ]; then
    chmod +x ./analyze_coverage.py
fi

if python3 ./analyze_coverage.py; then
    echo ""
    echo "✓ Analysis completed"
else
    echo ""
    echo "⚠ Analysis completed with warnings"
fi

echo ""

# Generate final summary
echo "════════════════════════════════════════════════════════════"
echo "COVERAGE REPORT COMPLETE"
echo "════════════════════════════════════════════════════════════"
echo ""

COVERAGE_DIR="./build_coverage/coverage"

echo "Generated Reports:"
echo ""
echo "1. HTML Coverage (LLVM):"
echo "   open $COVERAGE_DIR/html/index.html"
echo ""
echo "2. Interactive Dashboard:"
echo "   open $COVERAGE_DIR/dashboard.html"
echo ""
echo "3. Text Reports:"
echo "   • Summary:       cat $COVERAGE_DIR/coverage_summary.txt"
echo "   • Detailed:      less $COVERAGE_DIR/coverage_detailed.txt"
echo "   • Analysis:      cat $COVERAGE_DIR/analysis_report.txt"
echo "   • Test Results:  cat coverage_results.txt"
echo ""
echo "4. JSON Export (for CI/CD):"
echo "   cat $COVERAGE_DIR/coverage_export.json"
echo ""

# Display key metrics
if [ -f "$COVERAGE_DIR/analysis_report.txt" ]; then
    echo "════════════════════════════════════════════════════════════"
    echo "KEY METRICS"
    echo "════════════════════════════════════════════════════════════"
    grep -A 3 "OVERALL COVERAGE SUMMARY" "$COVERAGE_DIR/analysis_report.txt" | tail -3
    echo ""
fi

echo "════════════════════════════════════════════════════════════"
echo ""
