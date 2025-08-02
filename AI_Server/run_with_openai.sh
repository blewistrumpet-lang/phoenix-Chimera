#!/bin/bash
# Script to run both the OpenAI bridge server and main server

echo "Starting Chimera Phoenix AI Server with OpenAI support..."
echo "=============================================="

# Kill any existing processes on ports 9999 and 8000
echo "Cleaning up existing processes..."
lsof -ti:9999 | xargs kill -9 2>/dev/null
lsof -ti:8000 | xargs kill -9 2>/dev/null

# Start OpenAI bridge server in background
echo "Starting OpenAI bridge server on port 9999..."
python3 openai_bridge_server.py &
BRIDGE_PID=$!
echo "Bridge server PID: $BRIDGE_PID"

# Wait for bridge server to start
sleep 3

# Start main FastAPI server
echo "Starting main AI server on port 8000..."
python3 main.py &
MAIN_PID=$!
echo "Main server PID: $MAIN_PID"

echo ""
echo "=============================================="
echo "Servers are running!"
echo "OpenAI Bridge: http://localhost:9999"
echo "Main API: http://localhost:8000"
echo "API Docs: http://localhost:8000/docs"
echo ""
echo "Press Ctrl+C to stop all servers"
echo "=============================================="

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "Shutting down servers..."
    kill $BRIDGE_PID 2>/dev/null
    kill $MAIN_PID 2>/dev/null
    echo "Servers stopped."
    exit 0
}

# Trap Ctrl+C
trap cleanup INT

# Keep script running
wait