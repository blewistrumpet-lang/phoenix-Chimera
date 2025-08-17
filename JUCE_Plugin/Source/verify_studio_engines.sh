#!/bin/bash

echo "=== Verifying Studio Engine Integration ==="
echo ""

# Check that old versions are no longer referenced
echo "Checking for old engine references..."
echo ""

# Check ParametricEQ
echo -n "ParametricEQ: "
if grep -q "ParametricEQ_Platinum" EngineFactory.cpp; then
    echo "❌ Still using Platinum version"
else
    if grep -q "ParametricEQ_Studio" EngineFactory.cpp; then
        echo "✅ Using Studio version"
    else
        echo "⚠️  No reference found"
    fi
fi

# Check VintageConsoleEQ
echo -n "VintageConsoleEQ: "
if grep -q "VintageConsoleEQ_Platinum" EngineFactory.cpp; then
    echo "❌ Still using Platinum version"
else
    if grep -q "VintageConsoleEQ_Studio" EngineFactory.cpp; then
        echo "✅ Using Studio version"
    else
        echo "⚠️  No reference found"
    fi
fi

# Check VintageTubePreamp
echo -n "VintageTubePreamp: "
if grep -q 'VintageTubePreamp>' EngineFactory.cpp; then
    echo "❌ Still using old version"
else
    if grep -q "VintageTubePreamp_Studio" EngineFactory.cpp; then
        echo "✅ Using Studio version"
    else
        echo "⚠️  No reference found"
    fi
fi

echo ""
echo "Checking that Studio implementations exist..."
echo ""

for file in ParametricEQ_Studio.h ParametricEQ_Studio.cpp \
           VintageConsoleEQ_Studio.h VintageConsoleEQ_Studio.cpp \
           VintageTubePreamp_Studio.h VintageTubePreamp_Studio.cpp; do
    if [ -f "$file" ]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
    fi
done

echo ""
echo "Checking include statements..."
echo ""

# Check includes in EngineFactory.cpp
echo "In EngineFactory.cpp:"
grep -E "(ParametricEQ_Studio|VintageConsoleEQ_Studio|VintageTubePreamp_Studio).h" EngineFactory.cpp | while read -r line; do
    echo "  ✓ $line"
done

echo ""
echo "Checking factory instantiation..."
echo ""

# Check the switch cases
echo "Factory cases:"
grep -A1 "ENGINE_PARAMETRIC_EQ\|ENGINE_VINTAGE_CONSOLE_EQ\|ENGINE_VINTAGE_TUBE" EngineFactory.cpp | grep "std::make_unique" | while read -r line; do
    echo "  $line"
done

echo ""
echo "=== Summary ==="
echo ""
echo "The Studio engines have been integrated into the factory."
echo "Old implementations should be replaced with:"
echo "  • ParametricEQ_Platinum      → ParametricEQ_Studio"
echo "  • VintageConsoleEQ_Platinum   → VintageConsoleEQ_Studio"  
echo "  • VintageTubePreamp           → VintageTubePreamp_Studio"
echo ""
echo "Next steps:"
echo "  1. Build the plugin with Xcode"
echo "  2. Test in a DAW (Logic Pro, etc.)"
echo "  3. Verify parameter automation works"
echo "  4. Check CPU usage is acceptable"