#!/bin/bash

# Simple ChimeraPhoenix Launcher
# Just double-click this or run it to start Logic with everything configured

echo "Starting ChimeraPhoenix..."

# Load the API key from .env file
cd "$(dirname "$0")"
export $(grep -v '^#' AI_Server/.env | xargs) 2>/dev/null

# Kill any old Logic Pro instances
pkill -x "Logic Pro" 2>/dev/null
sleep 1

# Clear AU cache for fresh start (optional, comment out if you want faster startup)
rm -rf ~/Library/Caches/AudioUnitCache/* 2>/dev/null

# Launch Logic Pro with environment
open -a "Logic Pro"

echo "✓ Logic Pro launched with ChimeraPhoenix ready!"
echo "  Your API key is already configured."
echo "  The AI server will start automatically when you load the plugin."