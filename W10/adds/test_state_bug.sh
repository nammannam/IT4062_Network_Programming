#!/bin/bash

# Test script to reproduce the state confusion bug with multiple clients

echo "Starting server..."
./server 8787 > server_debug.log 2>&1 &
SERVER_PID=$!
sleep 2

echo "Server PID: $SERVER_PID"
echo ""
echo "Testing with Python script to simulate 2 clients logging in simultaneously..."
echo ""

python3 << 'EOF'
import socket
import time
import struct
import threading

def create_message(msg_type, data):
    """Create a protocol message"""
    data_bytes = data.encode('utf-8')
    length = len(data_bytes)
    # Pack: type (1 byte), length (1 byte), value (variable)
    msg = struct.pack('BB', msg_type, length) + data_bytes
    # Pad to message size if needed
    return msg

def client_session(client_id, username, password):
    """Simulate one client session"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', 8787))
        print(f"[Client {client_id}] Connected")
        
        # Send username
        time.sleep(0.1)
        msg = create_message(0x01, username)  # MSG_LOGIN
        sock.send(msg)
        print(f"[Client {client_id}] Sent username: {username}")
        
        # Receive response
        response = sock.recv(1024)
        resp_type = response[0]
        print(f"[Client {client_id}] Response type: 0x{resp_type:02X}")
        
        if resp_type == 0x02:  # MSG_PASSWORD
            # Send password
            time.sleep(0.1)
            msg = create_message(0x02, password)  # MSG_PASSWORD
            sock.send(msg)
            print(f"[Client {client_id}] Sent password")
            
            # Receive response
            response = sock.recv(1024)
            resp_type = response[0]
            resp_len = response[1]
            resp_value = response[2:2+resp_len].decode('utf-8')
            print(f"[Client {client_id}] Login response: 0x{resp_type:02X} - {resp_value}")
            
            # Send a text message
            time.sleep(0.1)
            msg = create_message(0x03, "Hello from " + username)  # MSG_TEXT
            sock.send(msg)
            
            # Receive echo
            response = sock.recv(1024)
            resp_type = response[0]
            resp_len = response[1]
            resp_value = response[2:2+resp_len].decode('utf-8')
            print(f"[Client {client_id}] Echo response: {resp_value}")
            
            # Check if response contains wrong username
            if username not in resp_value and resp_type == 0x11:
                print(f"[Client {client_id}] ❌ BUG DETECTED! Expected '{username}' but got '{resp_value}'")
            else:
                print(f"[Client {client_id}] ✅ Correct response")
        
        time.sleep(1)
        sock.close()
        print(f"[Client {client_id}] Disconnected")
        
    except Exception as e:
        print(f"[Client {client_id}] Error: {e}")

# Start two clients at nearly the same time
print("Starting Client 1 (quangvn)...")
thread1 = threading.Thread(target=client_session, args=(1, "quangvn", "vnquang12"))
thread1.start()

time.sleep(0.05)  # Slight delay

print("Starting Client 2 (namnk)...")
thread2 = threading.Thread(target=client_session, args=(2, "namnk", "namnk123"))
thread2.start()

# Wait for both to finish
thread1.join()
thread2.join()

print("\n✅ Test completed!")
EOF

echo ""
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo ""
echo "Server log:"
cat server_debug.log
