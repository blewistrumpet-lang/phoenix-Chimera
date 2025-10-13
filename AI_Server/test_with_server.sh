#!/bin/bash
# Test script that ensures correct server is running

echo "🧪 TEST WITH CORRECT SERVER"
echo "=========================="

# Kill everything first
echo "1️⃣ Killing all Python processes..."
pkill -9 -f python
sleep 2

# Check nothing is running
if pgrep -f python > /dev/null; then
    echo "❌ ERROR: Python processes still running!"
    pgrep -f python
    exit 1
fi

echo "✅ All processes cleared"

# Start the correct server
SERVER_FILE=$1
TEST_FILE=$2

if [ -z "$SERVER_FILE" ] || [ -z "$TEST_FILE" ]; then
    echo "Usage: ./test_with_server.sh <server.py> <test.py>"
    echo "Example: ./test_with_server.sh main_fixed.py test_fixed_pipeline.py"
    exit 1
fi

echo "2️⃣ Starting server: $SERVER_FILE"
python3 $SERVER_FILE &
SERVER_PID=$!

# Wait for server
echo "3️⃣ Waiting for server to initialize..."
sleep 4

# Verify server is running
if ! ps -p $SERVER_PID > /dev/null; then
    echo "❌ Server failed to start!"
    exit 1
fi

# Check which server is actually responding
echo "4️⃣ Verifying correct server is running..."
HEALTH_CHECK=$(curl -s http://localhost:8000/health 2>/dev/null)
echo "Health check: $HEALTH_CHECK"

# Run the test
echo "5️⃣ Running test: $TEST_FILE"
python3 $TEST_FILE

# Kill server after test
echo "6️⃣ Cleaning up..."
kill $SERVER_PID 2>/dev/null
pkill -f python

echo "✅ Test complete and cleaned up"
