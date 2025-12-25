#!/bin/bash

# Test Multiple Client Connections
# This script helps test if the server can handle multiple concurrent clients

PORT=8787

echo "========================================="
echo "Testing Multiple Client Connections"
echo "========================================="
echo ""

# Check if server is compiled
if [ ! -f "./server" ]; then
    echo "Error: Server executable not found. Run 'make' first."
    exit 1
fi

if [ ! -f "./client_nonblocking" ]; then
    echo "Error: Client executable not found. Run 'make' first."
    exit 1
fi

echo "Instructions:"
echo "1. Start the server in one terminal: ./server $PORT"
echo "2. Then run this script in another terminal"
echo ""
echo "Press Enter to start client connections..."
read

echo ""
echo "Starting 3 client connections..."
echo ""

# Start 3 clients
for i in 1 2 3; do
    echo "Starting client #$i..."
    xterm -e "./client_nonblocking 127.0.0.1 $PORT" &
    sleep 0.5
done

echo ""
echo "3 clients started in separate xterm windows"
echo "Try logging in with different accounts:"
echo "  - admin / admin123"
echo "  - namnk / namnk123"
echo "  - quangvn / vnquang12"
echo ""
echo "Check server terminal for connection logs"
echo ""
echo "Press Ctrl+C to exit"
wait
