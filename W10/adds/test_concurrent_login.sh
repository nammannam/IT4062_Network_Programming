#!/bin/bash

# Test script để verify bug fix: multiple clients login với username khác nhau

echo "========================================="
echo "Test Concurrent Login - Bug Fix Verification"
echo "========================================="
echo ""

PORT=8787

# Kill existing server
pkill -f "./server $PORT" 2>/dev/null
sleep 1

# Start server
echo "Starting server on port $PORT..."
./server $PORT > server_concurrent_test.log 2>&1 &
SERVER_PID=$!
sleep 2

echo "Server PID: $SERVER_PID"
echo ""

# Test với Python
python3 << 'EOF'
import socket
import time
import struct
import threading

def create_message(msg_type, data):
    """Create protocol message"""
    data_bytes = data.encode('utf-8')
    length = len(data_bytes)
    msg = struct.pack('BB', msg_type, length) + data_bytes
    return msg

def client_login_test(client_id, username, password):
    """Test client login"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', 8787))
        print(f"[Client {client_id}] Connected")
        
        # Send username
        time.sleep(0.2)
        msg = create_message(0x01, username)  # MSG_LOGIN
        sock.send(msg)
        print(f"[Client {client_id}] Sent username: {username}")
        
        # Receive password request
        response = sock.recv(1024)
        resp_type = response[0]
        print(f"[Client {client_id}] Response: 0x{resp_type:02X}")
        
        if resp_type == 0x03:  # MSG_PASSWORD
            # Send password
            time.sleep(0.2)
            msg = create_message(0x02, password)  # MSG_PASSWORD  
            sock.send(msg)
            print(f"[Client {client_id}] Sent password")
            
            # Receive login response
            response = sock.recv(1024)
            resp_type = response[0]
            resp_len = response[1]
            resp_value = response[2:2+resp_len].decode('utf-8')
            
            print(f"[Client {client_id}] Login response: 0x{resp_type:02X} - '{resp_value}'")
            
            # Verify correct username returned
            if resp_type == 0x11:  # MSG_CF
                if username in resp_value:
                    print(f"[Client {client_id}] ✅ SUCCESS: Got correct username '{username}'")
                else:
                    print(f"[Client {client_id}] ❌ BUG: Expected '{username}' but got '{resp_value}'")
            
            # Send a test message
            time.sleep(0.2)
            msg = create_message(0x10, f"Hello from {username}")  # MSG_TEXT
            sock.send(msg)
            
            # Receive echo
            response = sock.recv(1024)
            resp_type = response[0]
            resp_len = response[1]
            resp_value = response[2:2+resp_len].decode('utf-8')
            print(f"[Client {client_id}] Echo: '{resp_value}'")
            
            # Verify echo has correct username
            if username in resp_value:
                print(f"[Client {client_id}] ✅ Echo correct")
            else:
                print(f"[Client {client_id}] ❌ Echo wrong: expected '{username}' in '{resp_value}'")
        
        time.sleep(1)
        sock.close()
        print(f"[Client {client_id}] Disconnected")
        
    except Exception as e:
        print(f"[Client {client_id}] Error: {e}")

# Test scenario: 2 clients login gần như đồng thời
print("="*60)
print("Test: Client 1 (quangvn) đăng nhập trước")
print("      Client 2 (namnk) đăng nhập sau")
print("      Verify rằng Client 2 không nhận username của Client 1")
print("="*60)
print()

# Start Client 1
thread1 = threading.Thread(target=client_login_test, args=(1, "quangvn", "vnquang12"))
thread1.start()

time.sleep(0.5)  # Client 1 login first

# Start Client 2  
thread2 = threading.Thread(target=client_login_test, args=(2, "namnk", "namnk123"))
thread2.start()

# Wait for both
thread1.join()
thread2.join()

print()
print("="*60)
print("Test completed!")
print("="*60)
EOF

echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo ""
echo "Check server_concurrent_test.log for details"
