#!/bin/bash

# Test script for multiple clients connecting to server

PORT=8787

echo "Testing multiple client connections..."
echo "Make sure server is running on port $PORT"
echo ""

# Test with netcat to see if server accepts multiple connections
for i in 1 2 3; do
    echo "Attempting connection #$i..."
    (echo "test$i"; sleep 1) | nc -v 127.0.0.1 $PORT &
    sleep 0.5
done

echo ""
echo "All 3 connections attempted. Check server output."
echo "Press Ctrl+C to stop"
wait
