#!/bin/bash

echo "=== Chimera Phoenix Engine Validation Script ==="
echo "This script checks each engine systematically"
echo ""

# Create a test report file
REPORT="engine_validation_report.txt"
echo "=== Engine Validation Report ===" > $REPORT
echo "Date: $(date)" >> $REPORT
echo "" >> $REPORT

# Function to check if a source file exists and has proper structure
check_engine_source() {
    local engine_name=$1
    local engine_file="${engine_name}.cpp"
    local header_file="${engine_name}.h"
    
    echo -n "Checking $engine_name... "
    
    if [ ! -f "$engine_file" ]; then
        echo "✗ Missing .cpp file"
        echo "  $engine_name: Missing .cpp file" >> $REPORT
        return 1
    fi
    
    if [ ! -f "$header_file" ]; then
        echo "✗ Missing .h file"
        echo "  $engine_name: Missing .h file" >> $REPORT
        return 1
    fi
    
    # Check for common issues
    local issues=""
    
    # Check for updateParameters function
    if ! grep -q "updateParameters.*const.*map.*int.*float" "$engine_file"; then
        issues="$issues\n    - Missing or incorrect updateParameters signature"
    fi
    
    # Check for process function
    if ! grep -q "process.*AudioBuffer" "$engine_file"; then
        issues="$issues\n    - Missing or incorrect process signature"
    fi
    
    # Check for parameter index usage
    local max_param=$(grep -o "getParam([0-9]*" "$engine_file" 2>/dev/null | sed 's/getParam(//' | sort -n | tail -1)
    if [ ! -z "$max_param" ] && [ "$max_param" -gt 14 ]; then
        issues="$issues\n    - Uses parameter index $max_param (>14)"
    fi
    
    # Check for static variables in process (thread safety issue)
    if grep -q "process.*{" "$engine_file"; then
        # Look for static variables within process function
        awk '/process.*AudioBuffer.*{/,/^}/' "$engine_file" | grep -q "static " && \
            issues="$issues\n    - Static variable in process() - thread safety risk"
    fi
    
    # Check for buffer overflow risks
    if grep -q "\[2048\]" "$engine_file" || grep -q "\[4096\]" "$engine_file"; then
        if ! grep -q "std::min.*numSamples" "$engine_file"; then
            issues="$issues\n    - Fixed buffer size without bounds checking"
        fi
    fi
    
    if [ -z "$issues" ]; then
        echo "✓"
        echo "  $engine_name: OK" >> $REPORT
        return 0
    else
        echo "⚠ Issues found"
        echo "  $engine_name: Issues found" >> $REPORT
        echo -e "$issues" >> $REPORT
        return 1
    fi
}

# List of all engines to check
engines=(
    "NoneEngine"
    "VintageOptoCompressor_Platinum"
    "ClassicCompressor"
    "TransientShaper_Platinum"
    "NoiseGate_Platinum"
    "MasteringLimiter_Platinum"
    "DynamicEQ"
    "ParametricEQ_Studio"
    "VintageConsoleEQ_Studio"
    "LadderFilter"
    "StateVariableFilter"
    "FormantFilter"
    "EnvelopeFilter"
    "CombResonator"
    "VocalFormantFilter"
    "VintageTubePreamp_Studio"
    "WaveFolder"
    "HarmonicExciter_Platinum"
    "BitCrusher"
    "MultibandSaturator"
    "MuffFuzz"
    "RodentDistortion"
    "KStyleOverdrive"
    "StereoChorus"
    "ResonantChorus_Platinum"
    "AnalogPhaser"
    "PlatinumRingModulator"
    "FrequencyShifter"
    "HarmonicTremolo"
    "ClassicTremolo"
    "RotarySpeaker_Platinum"
    "PitchShifter"
    "DetuneDoubler"
    "IntelligentHarmonizer"
    "TapeEcho"
    "DigitalDelay"
    "MagneticDrumEcho"
    "BucketBrigadeDelay"
    "BufferRepeat_Platinum"
    "PlateReverb"
    "SpringReverb_Platinum"
    "ConvolutionReverb"
    "ShimmerReverb"
    "GatedReverb"
    "StereoWidener"
    "StereoImager"
    "DimensionExpander"
    "SpectralFreeze"
    "SpectralGate_Platinum"
    "PhasedVocoder"
    "GranularCloud"
    "ChaosGenerator_Platinum"
    "FeedbackNetwork"
    "MidSideProcessor_Platinum"
    "GainUtility_Platinum"
    "MonoMaker_Platinum"
    "PhaseAlign_Platinum"
)

# Check each engine
passed=0
failed=0
failed_engines=""

for engine in "${engines[@]}"; do
    if check_engine_source "$engine"; then
        ((passed++))
    else
        ((failed++))
        failed_engines="$failed_engines $engine"
    fi
done

echo ""
echo "=== SUMMARY ==="
echo "Total engines: ${#engines[@]}"
echo "Passed: $passed"
echo "Failed: $failed"

if [ ! -z "$failed_engines" ]; then
    echo "Failed engines:$failed_engines"
fi

echo "" >> $REPORT
echo "=== SUMMARY ===" >> $REPORT
echo "Total: ${#engines[@]}, Passed: $passed, Failed: $failed" >> $REPORT

# Now check parameter mapping in the three Studio engines
echo ""
echo "=== Checking Studio Engine Parameter Mappings ==="

echo "" >> $REPORT
echo "=== Studio Engine Parameter Analysis ===" >> $REPORT

for studio_engine in "ParametricEQ_Studio" "VintageConsoleEQ_Studio" "VintageTubePreamp_Studio"; do
    echo ""
    echo "$studio_engine parameter usage:"
    echo "$studio_engine:" >> $REPORT
    
    if [ -f "${studio_engine}.cpp" ]; then
        # Extract all getParam calls and show what indices are used
        grep -o "getParam([0-9]*" "${studio_engine}.cpp" | sed 's/getParam(//' | sort -n -u | while read idx; do
            echo "  - Uses parameter $idx"
            echo "  - Parameter $idx" >> $REPORT
        done
    fi
done

echo ""
echo "Report written to: $REPORT"
echo ""
echo "Next step: Review the report and fix any identified issues"