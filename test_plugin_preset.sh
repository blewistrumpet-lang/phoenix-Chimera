#!/bin/bash

echo "========================================="
echo "TESTING TRINITY PRESET LOADING"
echo "========================================="

# Kill any existing instance
pkill -f ChimeraPhoenix

# Launch plugin in background
echo "Launching ChimeraPhoenix plugin..."
/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Builds/MacOSX/build/Debug/ChimeraPhoenix.app/Contents/MacOS/ChimeraPhoenix &
PLUGIN_PID=$!

# Wait for plugin to start
sleep 3

# Send preset request to Trinity server
echo "Sending preset request to Trinity..."
RESPONSE=$(curl -s -X POST http://localhost:8000/generate \
  -H "Content-Type: application/json" \
  -d '{"prompt": "aggressive metal distortion with heavy compression"}')

# Check if successful
if echo "$RESPONSE" | grep -q '"success":true'; then
    echo "✅ Preset generated successfully!"
    PRESET_NAME=$(echo "$RESPONSE" | grep -o '"name":"[^"]*' | cut -d'"' -f4)
    echo "Preset name: $PRESET_NAME"
    
    # Extract engine IDs
    echo "Engines in preset:"
    echo "$RESPONSE" | grep -o '"engine_id":[0-9]*' | cut -d':' -f2 | while read id; do
        echo "  - Engine ID: $id"
    done
else
    echo "❌ Failed to generate preset"
    echo "$RESPONSE"
fi

echo ""
echo "Plugin should now have engines loaded in its slots."
echo "Check the plugin UI to verify engines are visible."
echo ""
echo "Press Enter to close the plugin..."
read

# Clean up
kill $PLUGIN_PID 2>/dev/null
echo "Test complete."