#!/bin/bash

# Chimera Phoenix Comprehensive Test Harness Build and Run Script
# This script builds and runs the comprehensive test harness for all engines

set -e  # Exit on any error

echo "======================================"
echo "Chimera Phoenix Test Harness Builder"
echo "======================================"

# Configuration
BUILD_DIR="Build"
OUTPUT_DIR="TestResults"
CMAKE_BUILD_TYPE="Release"

# Parse command line arguments
VERBOSE=false
CLEAN_BUILD=false
RUN_TESTS=true
ENGINE_ID=""
PARALLEL=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose)
            VERBOSE=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --no-test)
            RUN_TESTS=false
            shift
            ;;
        --engine)
            ENGINE_ID="$2"
            shift 2
            ;;
        --sequential)
            PARALLEL="--sequential"
            shift
            ;;
        --debug)
            CMAKE_BUILD_TYPE="Debug"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --verbose      Enable verbose build output"
            echo "  --clean        Clean build directory before building"
            echo "  --no-test      Build only, don't run tests"
            echo "  --engine ID    Test only specific engine ID"
            echo "  --sequential   Run tests sequentially instead of parallel"
            echo "  --debug        Build in debug mode"
            echo "  --help         Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                    # Build and test all engines"
            echo "  $0 --engine 15        # Build and test only engine 15"
            echo "  $0 --clean --verbose  # Clean build with verbose output"
            echo "  $0 --no-test          # Just build, don't run tests"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Create necessary directories
echo "Setting up directories..."
mkdir -p "$BUILD_DIR"
mkdir -p "$OUTPUT_DIR"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"/*
fi

# Check for required dependencies
echo "Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is required but not installed"
    exit 1
fi

if ! command -v make &> /dev/null && ! command -v ninja &> /dev/null; then
    echo "Error: Make or Ninja is required but not installed"
    exit 1
fi

# Configure build
echo "Configuring build..."
cd "$BUILD_DIR"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"

if [ "$VERBOSE" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_VERBOSE_MAKEFILE=ON"
fi

cmake .. $CMAKE_ARGS

# Build
echo "Building Comprehensive Test Harness..."
if [ "$VERBOSE" = true ]; then
    cmake --build . --target ComprehensiveTestHarness --config $CMAKE_BUILD_TYPE -- -v
else
    cmake --build . --target ComprehensiveTestHarness --config $CMAKE_BUILD_TYPE
fi

# Check if build was successful
if [ ! -f "./ComprehensiveTestHarness" ] && [ ! -f "./Release/ComprehensiveTestHarness" ] && [ ! -f "./Debug/ComprehensiveTestHarness" ]; then
    echo "Error: Build failed - ComprehensiveTestHarness executable not found"
    exit 1
fi

echo "Build completed successfully!"

# Find the executable
TEST_EXECUTABLE=""
if [ -f "./ComprehensiveTestHarness" ]; then
    TEST_EXECUTABLE="./ComprehensiveTestHarness"
elif [ -f "./Release/ComprehensiveTestHarness" ]; then
    TEST_EXECUTABLE="./Release/ComprehensiveTestHarness"
elif [ -f "./Debug/ComprehensiveTestHarness" ]; then
    TEST_EXECUTABLE="./Debug/ComprehensiveTestHarness"
fi

# Go back to main directory
cd ..

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo ""
    echo "======================================"
    echo "Running Comprehensive Tests"
    echo "======================================"
    
    # Build test command
    TEST_CMD="$BUILD_DIR/$TEST_EXECUTABLE"
    TEST_ARGS="--verbose --output-dir $OUTPUT_DIR"
    
    if [ ! -z "$ENGINE_ID" ]; then
        TEST_ARGS="$TEST_ARGS --engine $ENGINE_ID"
    fi
    
    if [ ! -z "$PARALLEL" ]; then
        TEST_ARGS="$TEST_ARGS $PARALLEL"
    fi
    
    echo "Running: $TEST_CMD $TEST_ARGS"
    echo ""
    
    # Run the tests
    if $TEST_CMD $TEST_ARGS; then
        echo ""
        echo "======================================"
        echo "Tests completed successfully!"
        echo "======================================"
        echo "Results saved to: $OUTPUT_DIR/"
        echo "  - Summary: $OUTPUT_DIR/test_summary.txt"
        echo "  - Detailed: $OUTPUT_DIR/test_detailed.txt"  
        echo "  - HTML Report: $OUTPUT_DIR/test_report.html"
        echo "  - JSON Data: $OUTPUT_DIR/test_report.json"
        echo "  - CSV Data: $OUTPUT_DIR/test_report.csv"
        echo ""
        echo "Open $OUTPUT_DIR/test_report.html in your browser for a visual report"
        
        # Show quick summary of results
        if [ -f "$OUTPUT_DIR/test_summary.txt" ]; then
            echo ""
            echo "Quick Summary:"
            head -20 "$OUTPUT_DIR/test_summary.txt" | grep -E "(Total Engines|Working Engines|Critical Issues|Errors|Warnings|Average Score)" || true
        fi
        
    else
        TEST_EXIT_CODE=$?
        echo ""
        echo "======================================"
        echo "Tests completed with issues"
        echo "======================================"
        echo "Exit code: $TEST_EXIT_CODE"
        
        case $TEST_EXIT_CODE in
            1)
                echo "Status: Errors found (check reports for details)"
                ;;
            2)
                echo "Status: Critical issues found (immediate attention required)"
                ;;
            3)
                echo "Status: Some engines failed to create"
                ;;
            4)
                echo "Status: Fatal error during testing"
                ;;
            *)
                echo "Status: Unknown error occurred"
                ;;
        esac
        
        echo ""
        echo "Check the reports in $OUTPUT_DIR/ for detailed information"
        exit $TEST_EXIT_CODE
    fi
    
else
    echo ""
    echo "Build completed. Test executable: $BUILD_DIR/$TEST_EXECUTABLE"
    echo "To run tests manually:"
    echo "  $BUILD_DIR/$TEST_EXECUTABLE --help"
    echo "  $BUILD_DIR/$TEST_EXECUTABLE --verbose --output-dir $OUTPUT_DIR"
fi

echo ""
echo "Script completed successfully!"