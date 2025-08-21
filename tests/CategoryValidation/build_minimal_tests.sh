#!/bin/bash

echo "Building minimal validation tests..."

# Compile each test
g++ -std=c++17 dynamics_minimal_test.cpp -o dynamics_test
g++ -std=c++17 eq_filter_minimal_test.cpp -o eq_filter_test
g++ -std=c++17 distortion_minimal_test.cpp -o distortion_test
g++ -std=c++17 modulation_minimal_test.cpp -o modulation_test
g++ -std=c++17 delay_minimal_test.cpp -o delay_test
g++ -std=c++17 reverb_minimal_test.cpp -o reverb_test
g++ -std=c++17 spatial_minimal_test.cpp -o spatial_test
g++ -std=c++17 utility_minimal_test.cpp -o utility_test

echo "Build complete. Running all tests..."
echo ""

./dynamics_test
echo ""
./eq_filter_test
echo ""
./distortion_test
echo ""
./modulation_test
echo ""
./delay_test
echo ""
./reverb_test
echo ""
./spatial_test
echo ""
./utility_test

echo ""
echo "===================================="
echo "ALL CATEGORY VALIDATIONS COMPLETE"
echo "===================================="
