#!/bin/bash

# Test all engine parameters in batches
echo "=== COMPREHENSIVE PARAMETER TESTING FOR ALL ENGINES ==="
echo "This will test every parameter of every engine through its full range"
echo ""

# Test Dynamics engines (0-5)
echo "Testing DYNAMICS engines..."
for i in {0..5}; do
    echo "Engine $i:"
    ./test_parameters_comprehensive $i $i | grep -E "(Testing:|Param [0-9]+:|✅|❌|⚠️|SUMMARY)"
    echo ""
done

# Test Filter engines (6-14)
echo "Testing FILTER engines..."
for i in {6..14}; do
    echo "Engine $i:"
    ./test_parameters_comprehensive $i $i | grep -E "(Testing:|Param [0-9]+:|✅|❌|⚠️|SUMMARY)"
    echo ""
done

# Test Distortion engines (15-22)
echo "Testing DISTORTION engines..."
for i in {15..22}; do
    echo "Engine $i:"
    ./test_parameters_comprehensive $i $i | grep -E "(Testing:|Param [0-9]+:|✅|❌|⚠️|SUMMARY)"
    echo ""
done

# Test Modulation engines (23-30)
echo "Testing MODULATION engines..."
for i in {23..30}; do
    echo "Engine $i:"
    ./test_parameters_comprehensive $i $i | grep -E "(Testing:|Param [0-9]+:|✅|❌|⚠️|SUMMARY)"
    echo ""
done

# Test Time-Based engines (31-40)
echo "Testing TIME-BASED engines..."
for i in {31..40}; do
    echo "Engine $i:"
    ./test_parameters_comprehensive $i $i | grep -E "(Testing:|Param [0-9]+:|✅|❌|⚠️|SUMMARY)"
    echo ""
done

# Test Pitch engines (41-46)
echo "Testing PITCH engines..."
for i in {41..46}; do
    echo "Engine $i:"
    ./test_parameters_comprehensive $i $i | grep -E "(Testing:|Param [0-9]+:|✅|❌|⚠️|SUMMARY)"
    echo ""
done

# Test Utility & Spatial engines (47-56)
echo "Testing UTILITY & SPATIAL engines..."
for i in {47..56}; do
    echo "Engine $i:"
    ./test_parameters_comprehensive $i $i | grep -E "(Testing:|Param [0-9]+:|✅|❌|⚠️|SUMMARY)"
    echo ""
done

echo "=== PARAMETER TESTING COMPLETE ==="