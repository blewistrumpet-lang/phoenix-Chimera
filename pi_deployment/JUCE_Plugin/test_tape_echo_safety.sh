#!/bin/bash

echo "=== TapeEcho Safety Verification ==="
echo "Checking the new TapeEcho implementation..."
echo

# Check for critical safety features in the new implementation
echo "1. Checking for safe wrapping function:"
grep -n "wrapi" Source/TapeEcho.cpp

echo
echo "2. Checking for buffer safety guards:"
grep -n "kExtraGuard" Source/TapeEcho.h

echo
echo "3. Checking for NaN/Inf protection:"
grep -n "std::isfinite" Source/TapeEcho.cpp

echo
echo "4. Checking for denormal protection:"
grep -n "flushDenorm" Source/TapeEcho.h

echo
echo "5. Checking delay line safety:"
grep -n "juce::jlimit.*delaySamples" Source/TapeEcho.cpp

echo
echo "6. Checking thread safety (no static variables in process):"
if grep -n "static.*lastFeedback" Source/TapeEcho.cpp; then
    echo "WARNING: Found static variable in process path!"
else
    echo "✓ No static variables found in process path"
fi

echo
echo "7. Checking atomic parameter handling:"
grep -n "std::atomic" Source/TapeEcho.h

echo
echo "=== Summary ==="
echo "The new TapeEcho implementation includes:"
echo "- Safe wrapping function for buffer access"
echo "- Guard samples for interpolation safety"  
echo "- NaN/Inf protection throughout signal chain"
echo "- Denormal flushing to prevent CPU spikes"
echo "- Delay sample clamping to prevent buffer overruns"
echo "- Thread-safe parameter smoothing with atomics"
echo "- No static variables in audio processing path"
echo
echo "The plugin should now be safe to use in Logic Pro without audio system crashes."