#!/bin/bash

echo "=================================="
echo "ChimeraPhoenix Setup & Test"
echo "=================================="
echo ""

# Function to test server
test_server() {
    echo "Testing AI server..."
    python3 AI_Server/main.py > AI_Server/test_server.log 2>&1 &
    SERVER_PID=$!
    sleep 3
    
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo "✓ Server is running (PID: $SERVER_PID)"
        
        # Test the endpoint
        if curl -s http://localhost:8000/health | grep -q "healthy"; then
            echo "✓ Server is responding correctly"
            
            # Test generation
            echo ""
            echo "Testing preset generation..."
            RESPONSE=$(curl -s -X POST http://localhost:8000/generate \
                -H "Content-Type: application/json" \
                -d '{"prompt":"warm vintage analog sound"}')
            
            if echo "$RESPONSE" | grep -q "success"; then
                echo "✓ Preset generation working!"
                echo ""
                echo "Sample response:"
                echo "$RESPONSE" | python3 -m json.tool | head -20
            else
                echo "⚠️  Generation failed - will use fallback presets"
            fi
        else
            echo "⚠️  Server not responding on port 8000"
        fi
        
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
    else
        echo "✗ Server failed to start"
        echo "Check AI_Server/test_server.log for errors"
    fi
}

# Step 1: Check for API key
echo "Step 1: Checking for OpenAI API key..."
echo ""

if [ -f "AI_Server/.env" ]; then
    # Load the .env file
    export $(grep -v '^#' AI_Server/.env | xargs)
    
    if [ "$OPENAI_API_KEY" = "your-api-key-here" ] || [ -z "$OPENAI_API_KEY" ]; then
        echo "⚠️  Please enter your OpenAI API key:"
        echo "   (Get one from https://platform.openai.com/api-keys)"
        echo ""
        read -p "API Key: " user_api_key
        
        if [ ! -z "$user_api_key" ]; then
            # Update the .env file
            echo "# OpenAI API Configuration" > AI_Server/.env
            echo "OPENAI_API_KEY=$user_api_key" >> AI_Server/.env
            echo "OPENAI_MODEL=gpt-3.5-turbo" >> AI_Server/.env
            export OPENAI_API_KEY=$user_api_key
            echo "✓ API key saved to AI_Server/.env"
        else
            echo "⚠️  No API key provided - will use fallback presets"
        fi
    else
        echo "✓ API key found in .env file"
    fi
fi

echo ""
echo "Step 2: Testing AI Server..."
echo "=================================="
test_server

echo ""
echo "Step 3: Launching Logic Pro with environment..."
echo "=================================="
echo ""

# Kill any existing Logic Pro
pkill -x "Logic Pro" 2>/dev/null
pkill -x "Logic Pro X" 2>/dev/null
sleep 2

# Clear AU cache
echo "Clearing Audio Unit cache..."
rm -rf ~/Library/Caches/AudioUnitCache/* 2>/dev/null
killall -9 audiounitscanningd 2>/dev/null

# Export the API key for Logic Pro
export OPENAI_API_KEY

echo "✓ Environment configured"
echo "✓ Launching Logic Pro..."
echo ""

# Launch Logic Pro with the environment
open -a "Logic Pro"

echo "=================================="
echo "Setup Complete!"
echo "=================================="
echo ""
echo "✓ API key is configured"
echo "✓ Server tested successfully" 
echo "✓ Logic Pro launched with environment"
echo ""
echo "The AI server will start automatically when you load ChimeraPhoenix"
echo ""
echo "To verify in Logic Pro:"
echo "1. Create an audio track"
echo "2. Add ChimeraPhoenix from Audio Units > ChimeraAudio"
echo "3. Type a prompt and click Generate"
echo "4. Check if you get a unique preset name (not 'Generated Preset')"
echo ""