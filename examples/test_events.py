#!/usr/bin/env python3
"""
Test script for Janus ZeroMQ Event Handler Plugin

This script demonstrates how to receive events from Janus via ZeroMQ.
"""

import zmq
import json
import signal
import sys

# Global flag for graceful shutdown
running = True

def signal_handler(sig, frame):
    """Handle SIGINT for graceful shutdown"""
    global running
    print("\nShutting down...")
    running = False

def test_events():
    """Test the ZeroMQ event handler plugin"""
    print("Testing Janus ZeroMQ Event Handler...")
    
    # Set up signal handler
    signal.signal(signal.SIGINT, signal_handler)
    
    # Create ZeroMQ context and socket
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    try:
        # Connect to Janus event publisher
        socket.connect("tcp://127.0.0.1:5546")
        print("✓ Connected to tcp://127.0.0.1:5546")
        
        # Subscribe to all events
        socket.setsockopt_string(zmq.SUBSCRIBE, "")
        print("✓ Subscribed to all events")
        
        # Set timeout for non-blocking receive
        socket.setsockopt(zmq.RCVTIMEO, 1000)  # 1 second
        
        print("\nListening for events (Press Ctrl+C to stop)...\n")
        
        event_count = 0
        while running:
            try:
                # Receive event
                event = socket.recv_string()
                event_count += 1
                
                # Parse and display event
                event_data = json.loads(event)
                event_type = event_data.get("type", "unknown")
                
                print(f"Event #{event_count} [{event_type}]:")
                print(json.dumps(event_data, indent=2))
                print("-" * 80)
                
            except zmq.error.Again:
                # Timeout, continue
                continue
            except Exception as e:
                print(f"Error processing event: {e}")
                continue
        
        print(f"\n✓ Received {event_count} events total")
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        return False
    finally:
        socket.close()
        context.term()

if __name__ == "__main__":
    print("Note: This script requires Janus to be running with the ZeroMQ event handler enabled.")
    print("Events will only be received if Janus is actively processing requests.\n")
    
    success = test_events()
    sys.exit(0 if success else 1)
