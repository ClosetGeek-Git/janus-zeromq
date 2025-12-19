#!/usr/bin/env python3
"""
Test script for Janus ZeroMQ Transport Plugin

This script demonstrates how to communicate with Janus via ZeroMQ transport.
"""

import zmq
import json
import sys

def test_transport():
    """Test the ZeroMQ transport plugin"""
    print("Testing Janus ZeroMQ Transport...")
    
    # Create ZeroMQ context and socket
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    
    try:
        # Connect to Janus
        socket.connect("tcp://127.0.0.1:5545")
        print("✓ Connected to tcp://127.0.0.1:5545")
        
        # Set timeout
        socket.setsockopt(zmq.RCVTIMEO, 5000)  # 5 seconds
        
        # Test 1: Get server info
        print("\nTest 1: Get server info")
        request = {
            "janus": "info",
            "transaction": "test-info"
        }
        print(f"Sending: {json.dumps(request, indent=2)}")
        socket.send_string(json.dumps(request))
        
        response = socket.recv_string()
        response_data = json.loads(response)
        print(f"Response: {json.dumps(response_data, indent=2)}")
        
        if response_data.get("janus") == "server_info":
            print("✓ Server info received successfully")
        else:
            print("✗ Unexpected response")
            return False
        
        # Test 2: Create session
        print("\nTest 2: Create session")
        request = {
            "janus": "create",
            "transaction": "test-create"
        }
        print(f"Sending: {json.dumps(request, indent=2)}")
        socket.send_string(json.dumps(request))
        
        response = socket.recv_string()
        response_data = json.loads(response)
        print(f"Response: {json.dumps(response_data, indent=2)}")
        
        if response_data.get("janus") == "success":
            session_id = response_data.get("data", {}).get("id")
            print(f"✓ Session created: {session_id}")
        else:
            print("✗ Failed to create session")
            return False
        
        print("\n✓ All tests passed!")
        return True
        
    except zmq.error.Again:
        print("✗ Timeout waiting for response. Is Janus running with ZeroMQ transport enabled?")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False
    finally:
        socket.close()
        context.term()

if __name__ == "__main__":
    success = test_transport()
    sys.exit(0 if success else 1)
