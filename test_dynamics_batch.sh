#!/bin/bash

echo "Testing all Dynamics engines..."

# List of dynamics engines to test
engines=(
    "ClassicCompressor"
    "VintageOptoCompressor" 
    "NoiseGate"
    "MasteringLimiter_Platinum"
    "TransientShaper_Platinum"
    "DimensionExpander"
)

for engine in "${engines[@]}"; do
    echo "================================"
    echo "Testing $engine..."
    echo "================================"
    
    # Check if test already exists
    if [ -f "test_${engine}" ]; then
        ./test_${engine} > "Reports/${engine}_TestResults.txt" 2>&1
        echo "Results saved to Reports/${engine}_TestResults.txt"
        
        # Extract grade
        grep "Overall Grade:" "Reports/${engine}_TestResults.txt" | tail -1
        grep "Quality Score:" "Reports/${engine}_TestResults.txt" | tail -1
    else
        echo "Test not compiled for $engine"
    fi
    
    echo ""
done

echo "All tests complete!"
