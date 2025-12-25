#!/bin/bash

# Automated test for multiple client connections

PORT=8787

echo "========================================="
echo "Automated Multi-Client Connection Test"
echo "========================================="
echo ""

# Kill any existing server on this port
echo "Cleaning up any existing server..."
pkill -f "./server $PORT" 2>/dev/null
sleep 1

# Start server in background
echo "Starting server on port $PORT..."
./server $PORT > server_test.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
sleep 2

# Check if server is running
if ! ps -p $SERVER_PID > /dev/null; then
    echo "Error: Server failed to start"
    cat server_test.log
    exit 1
fi

echo "Server started successfully"
echo ""

# Test with Python script
echo "Running connection test..."
python3 test_connections.py 127.0.0.1 $PORT

echo ""
echo "Checking server log..."
echo "========================================="
tail -50 server_test.log
echo "========================================="

# Cleanup
echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "Test completed. Check server_test.log for details."
