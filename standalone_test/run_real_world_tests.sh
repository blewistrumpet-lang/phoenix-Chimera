#!/bin/bash

echo "=========================================================================="
echo "REAL-WORLD AUDIO TESTING SUITE - COMPLETE WORKFLOW"
echo "=========================================================================="
echo ""
echo "This script will:"
echo "  1. Generate realistic musical test materials"
echo "  2. Build the testing framework"
echo "  3. Test all 57 engines with real audio"
echo "  4. Generate comprehensive quality report"
echo ""
echo "Estimated time: 4-5 minutes"
echo ""
read -p "Press Enter to continue..."

# Change to standalone_test directory
cd "$(dirname "$0")"

# Step 1: Generate test materials
echo ""
echo "=========================================================================="
echo "STEP 1/4: Generating Musical Test Materials"
echo "=========================================================================="
echo ""

if [ -d "real_world_test_materials" ]; then
    echo "Test materials already exist."
    read -p "Regenerate? (y/n): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf real_world_test_materials
        python3 generate_musical_materials.py || { echo "ERROR: Material generation failed"; exit 1; }
    fi
else
    python3 generate_musical_materials.py || { echo "ERROR: Material generation failed"; exit 1; }
fi

# Step 2: Build test suite
echo ""
echo "=========================================================================="
echo "STEP 2/4: Building Test Framework"
echo "=========================================================================="
echo ""

# Make build script executable
chmod +x build_real_world_test.sh

# Build
./build_real_world_test.sh || { echo "ERROR: Build failed"; exit 1; }

# Step 3: Run tests
echo ""
echo "=========================================================================="
echo "STEP 3/4: Testing All Engines with Real-World Audio"
echo "=========================================================================="
echo ""
echo "Testing 57 engines with 7 musical materials (399 total tests)"
echo "This will take 2-3 minutes..."
echo ""

./test_real_world_audio || { echo "ERROR: Testing failed"; exit 1; }

# Step 4: Display results
echo ""
echo "=========================================================================="
echo "STEP 4/4: Results Summary"
echo "=========================================================================="
echo ""

# Extract key statistics from report
if [ -f "REAL_WORLD_AUDIO_TESTING_REPORT.md" ]; then
    echo "Test Results:"
    echo "-------------"
    grep -A 6 "SUMMARY STATISTICS" REAL_WORLD_AUDIO_TESTING_REPORT.md | tail -6
    echo ""

    # Check for failures
    FAIL_COUNT=$(grep -c "| F |" REAL_WORLD_AUDIO_TESTING_REPORT.md || echo "0")
    PASS_RATE=$(grep "Pass Rate:" REAL_WORLD_AUDIO_TESTING_REPORT.md | awk '{print $3}')

    echo "Pass Rate: $PASS_RATE"
    echo "Failures: $FAIL_COUNT tests"
    echo ""

    if [ "$FAIL_COUNT" -gt "0" ]; then
        echo "⚠️  WARNING: Some engines have critical issues (Grade F)"
        echo "Review REAL_WORLD_AUDIO_TESTING_REPORT.md for details"
        echo ""
        echo "Problem engines:"
        grep -B 2 "Grade: F" REAL_WORLD_AUDIO_TESTING_REPORT.md | grep "Engine" | sort -u
    else
        echo "✅ All engines passed real-world audio testing!"
    fi
fi

echo ""
echo "=========================================================================="
echo "COMPLETE - View full report:"
echo "=========================================================================="
echo ""
echo "  open REAL_WORLD_AUDIO_TESTING_REPORT.md"
echo ""
echo "Or view in terminal:"
echo ""
echo "  cat REAL_WORLD_AUDIO_TESTING_REPORT.md"
echo ""
echo "Test materials saved in: real_world_test_materials/"
echo "Processed audio (failures): output_engine_*.wav"
echo ""
echo "=========================================================================="
