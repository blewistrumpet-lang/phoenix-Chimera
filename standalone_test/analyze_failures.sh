#!/bin/bash
# Analyze failed engines

failed_engines=(6 9 15 20 32 33 39 40 49 52)
engine_names=(
  "6:Dynamic EQ"
  "9:Ladder Filter Pro"
  "15:Vintage Tube Preamp Studio"
  "20:Muff Fuzz"
  "32:Pitch Shifter"
  "33:Intelligent Harmonizer"
  "39:Convolution Reverb"
  "40:Shimmer Reverb"
  "49:Pitch Shifter"
  "52:Spectral Gate Platinum"
)

echo "Analyzing Failed Engines"
echo "========================"
echo ""

for engine_info in "${engine_names[@]}"; do
    id=$(echo $engine_info | cut -d: -f1)
    name=$(echo $engine_info | cut -d: -f2-)
    
    echo "Engine $id: $name"
    echo "---"
    
    if [ -f "/tmp/test_$id.log" ]; then
        # Check for specific error types
        if grep -q "TIMEOUT" /tmp/test_$id.log || grep -q "Killed" /tmp/test_$id.log; then
            echo "  Issue: TIMEOUT/HANG - Infinite loop or deadlock"
        elif grep -q "NaN" /tmp/test_$id.log; then
            echo "  Issue: NaN detected in output"
        elif grep -q "Inf" /tmp/test_$id.log; then
            echo "  Issue: Infinity detected in output"  
        elif grep -q "Safety.*FAIL" /tmp/test_$id.log; then
            echo "  Issue: Failed safety test"
        elif grep -q "Quality.*FAIL" /tmp/test_$id.log; then
            echo "  Issue: Failed quality test (THD too high)"
        elif grep -q "Performance.*FAIL" /tmp/test_$id.log; then
            echo "  Issue: Failed performance test (CPU too high)"
        else
            echo "  Issue: Unknown failure"
        fi
        
        # Show last few relevant lines
        echo "  Details:"
        grep -E "(FAIL|ERROR|NaN|Inf|THD|CPU)" /tmp/test_$id.log | head -3 | sed 's/^/    /'
    fi
    echo ""
done
