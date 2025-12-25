#!/usr/bin/env python3
"""
Test script to simulate multiple client connections to the server
"""
import socket
import time
import sys

def test_connection(server_ip, server_port, client_id):
    """Test a single client connection"""
    try:
        print(f"[Client {client_id}] Attempting to connect to {server_ip}:{server_port}")
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((server_ip, server_port))
        print(f"[Client {client_id}] ✓ Connected successfully!")
        
        # Keep connection alive for a while
        time.sleep(2)
        
        sock.close()
        print(f"[Client {client_id}] Connection closed")
        return True
    except Exception as e:
        print(f"[Client {client_id}] ✗ Failed: {e}")
        return False

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 test_connections.py <server_ip> <server_port>")
        sys.exit(1)
    
    server_ip = sys.argv[1]
    server_port = int(sys.argv[2])
    
    print("=" * 60)
    print("Testing Multiple Client Connections")
    print("=" * 60)
    print()
    
    # Test sequential connections
    print("Test 1: Sequential connections (one after another)")
    print("-" * 60)
    for i in range(1, 4):
        result = test_connection(server_ip, server_port, i)
        time.sleep(0.5)
    
    print()
    print("Test 2: Concurrent connections (all at once)")
    print("-" * 60)
    
    # Create multiple sockets simultaneously
    sockets = []
    for i in range(1, 4):
        try:
            print(f"[Client {i}] Creating connection...")
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((server_ip, server_port))
            sockets.append((i, sock))
            print(f"[Client {i}] ✓ Connected!")
            time.sleep(0.2)  # Small delay between connections
        except Exception as e:
            print(f"[Client {i}] ✗ Failed: {e}")
    
    print()
    print(f"Total concurrent connections: {len(sockets)}")
    print()
    print("Keeping connections alive for 5 seconds...")
    time.sleep(5)
    
    print("Closing all connections...")
    for client_id, sock in sockets:
        sock.close()
        print(f"[Client {client_id}] Closed")
    
    print()
    print("=" * 60)
    print(f"Test completed. Successfully created {len(sockets)} concurrent connections.")
    print("=" * 60)

if __name__ == "__main__":
    main()
