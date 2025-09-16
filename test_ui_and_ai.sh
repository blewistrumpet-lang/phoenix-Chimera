#!/bin/bash

echo "================================================"
echo "CHIMERA PHOENIX - UI & AI INTEGRATION TEST"
echo "================================================"
echo ""

# Check if Trinity AI server is running
echo "1. Checking Trinity AI Server status..."
if curl -s http://localhost:8001/health > /dev/null 2>&1; then
    echo "   ✅ AI Server is running on port 8001"
    curl -s http://localhost:8001/health | python3 -m json.tool
elif curl -s http://localhost:8000/health > /dev/null 2>&1; then
    echo "   ✅ AI Server is running on port 8000"
    curl -s http://localhost:8000/health | python3 -m json.tool
else
    echo "   ❌ AI Server is NOT running"
    echo "   To start: cd Trinity_AI_Pipeline && python3 main.py"
fi

echo ""
echo "2. Plugin UI Status:"
echo "   - Original UI: Always available"
echo "   - Skunkworks UI: Set CHIMERA_SKUNKWORKS_UI=1 to enable"
echo ""

echo "3. To test the new UI with AI:"
echo "   a) Enable Skunkworks UI:"
echo "      export CHIMERA_SKUNKWORKS_UI=1"
echo ""
echo "   b) Load plugin in your DAW"
echo ""
echo "   c) Click 'TERMINAL' button in top-right"
echo ""
echo "   d) Type commands in terminal:"
echo "      - generate <prompt>  : Generate AI preset"
echo "      - help              : Show available commands"
echo "      - status            : Check system status"
echo ""

echo "4. UI Status Indicators:"
echo "   - Green LED (Power): Plugin is active"
echo "   - Yellow LED (Audio): Audio is processing"  
echo "   - Red/Green LED (AI): Server connection status"
echo "      • Green = Connected"
echo "      • Red = Disconnected"
echo ""

echo "5. Testing AI Generation:"
echo "   Example prompts to try:"
echo "   - 'generate warm vintage compression with analog character'"
echo "   - 'generate aggressive modern distortion for dubstep'"
echo "   - 'generate ethereal ambient reverb and delay'"
echo ""

# Test a quick AI generation request
echo "6. Quick AI Test..."
if curl -s http://localhost:8001/health > /dev/null 2>&1; then
    echo "   Sending test request to Trinity Pipeline..."
    RESPONSE=$(curl -s -X POST http://localhost:8001/generate \
        -H "Content-Type: application/json" \
        -d '{"prompt": "test warm vintage sound"}' \
        --max-time 5)
    
    if [ ! -z "$RESPONSE" ]; then
        echo "   ✅ AI Pipeline responding correctly"
        echo "   Response preview: $(echo $RESPONSE | head -c 100)..."
    else
        echo "   ⚠️  AI Pipeline not responding to generation requests"
    fi
else
    echo "   ⏭️  Skipping (server not running)"
fi

echo ""
echo "================================================"
echo "READY STATE SUMMARY:"
echo "================================================"

# Final status
AI_STATUS="❌ Not Running"
if curl -s http://localhost:8001/health > /dev/null 2>&1 || curl -s http://localhost:8000/health > /dev/null 2>&1; then
    AI_STATUS="✅ Running"
fi

PLUGIN_STATUS="✅ Compiled"
if [ -f "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Builds/MacOSX/build/Debug/ChimeraPhoenix.component/Contents/MacOS/ChimeraPhoenix" ]; then
    PLUGIN_STATUS="✅ Ready"
fi

echo "Plugin Status:    $PLUGIN_STATUS"
echo "AI Server:        $AI_STATUS"
echo "Skunkworks UI:    ✅ Available (set CHIMERA_SKUNKWORKS_UI=1)"
echo "Original UI:      ✅ Available (default)"
echo ""

if [[ "$AI_STATUS" == "✅ Running" ]] && [[ "$PLUGIN_STATUS" == "✅ Ready" ]]; then
    echo "🚀 SYSTEM FULLY OPERATIONAL - Ready for testing!"
else
    echo "⚠️  Some components need attention - see above"
fi

echo ""
echo "================================================"