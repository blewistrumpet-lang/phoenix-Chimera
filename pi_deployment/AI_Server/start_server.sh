#!/bin/bash
# ALWAYS kill ALL Python processes before starting a new server
# This prevents testing against old/cached servers

echo "🔴 KILLING ALL PYTHON PROCESSES..."
pkill -9 -f python
sleep 2

# Double check nothing is on port 8000
lsof -ti:8000 | xargs kill -9 2>/dev/null

echo "✅ All processes killed"
echo "🚀 Starting server: $1"

if [ -z "$1" ]; then
    echo "❌ Error: Please specify which server to run"
    echo "Usage: ./start_server.sh main_fixed.py"
    exit 1
fi

# Start the specified server
python3 $1 &
SERVER_PID=$!

echo "⏳ Waiting for server to start (PID: $SERVER_PID)..."
sleep 3

# Verify it's running
if ps -p $SERVER_PID > /dev/null; then
    echo "✅ Server started successfully (PID: $SERVER_PID)"
    
    # Test the health endpoint
    if curl -s http://localhost:8000/health > /dev/null 2>&1; then
        echo "✅ Server is responding to requests"
    else
        echo "⚠️ Server started but not responding yet"
    fi
else
    echo "❌ Server failed to start!"
    exit 1
fi

echo "📝 To stop: kill $SERVER_PID or run pkill -f python"
