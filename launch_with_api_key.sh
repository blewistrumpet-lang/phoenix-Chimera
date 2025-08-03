#!/bin/bash

# ChimeraPhoenix Launcher with API Key
# This script sets your OpenAI API key and launches Logic Pro

echo "ChimeraPhoenix API Key Setup"
echo "============================"
echo ""

# Check if API key is already set in .env file
if [ -f "AI_Server/.env" ]; then
    source AI_Server/.env
    if [ ! -z "$OPENAI_API_KEY" ] && [ "$OPENAI_API_KEY" != "your-api-key-here" ]; then
        echo "✓ API key found in .env file"
    else
        echo "⚠️  Please edit AI_Server/.env and add your OpenAI API key"
        echo "   Replace 'your-api-key-here' with your actual key"
        exit 1
    fi
else
    echo "Enter your OpenAI API key (or press Enter to skip):"
    read -s OPENAI_API_KEY
    
    if [ ! -z "$OPENAI_API_KEY" ]; then
        export OPENAI_API_KEY
        echo "✓ API key set for this session"
        
        echo ""
        echo "Would you like to save this key for future use? (y/n)"
        read -n 1 save_key
        echo ""
        
        if [ "$save_key" = "y" ] || [ "$save_key" = "Y" ]; then
            echo "OPENAI_API_KEY=$OPENAI_API_KEY" > AI_Server/.env
            echo "✓ API key saved to AI_Server/.env"
        fi
    else
        echo "⚠️  No API key provided - AI preset generation will use fallback presets"
    fi
fi

echo ""
echo "Launching Logic Pro..."
export OPENAI_API_KEY
open -a "Logic Pro"

echo ""
echo "Logic Pro launched with ChimeraPhoenix ready!"
echo ""
echo "Notes:"
echo "- The AI server will start automatically when you load the plugin"
echo "- If you have an API key set, it will use OpenAI for preset generation"
echo "- Without an API key, it will use high-quality fallback presets"