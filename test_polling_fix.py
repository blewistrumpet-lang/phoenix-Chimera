#!/usr/bin/env python3
"""
Test script to verify the Trinity Pipeline polling fix
"""

import json
import time
import requests
import threading

def test_plugin_query():
    """Send a test query and immediately check the polling response"""
    
    # Test session ID
    session_id = "test_session_fix_validation"
    
    # Send a test query
    print("Sending test query...")
    query_response = requests.post("http://localhost:8000/message", json={
        "type": "query",
        "content": "test polling fix",
        "message": "test polling fix", 
        "session_id": session_id,
        "timestamp": int(time.time() * 1000)
    })
    
    print(f"Query response: {query_response.status_code}")
    if query_response.status_code == 200:
        print(f"Response body: {query_response.json()}")
    
    # Wait a bit for pipeline to complete
    print("Waiting for pipeline to complete...")
    time.sleep(15)
    
    # Test polling
    print(f"Polling for session: {session_id}")
    poll_response = requests.get(f"http://localhost:8000/poll?session={session_id}")
    
    print(f"Poll response: {poll_response.status_code}")
    if poll_response.status_code == 200:
        poll_data = poll_response.json()
        print(f"Poll response body: {json.dumps(poll_data, indent=2)}")
        
        # Check if we received messages
        if poll_data.get("messages"):
            print(f"SUCCESS: Received {len(poll_data['messages'])} messages")
            for i, msg in enumerate(poll_data["messages"]):
                print(f"Message {i+1}: {json.dumps(msg, indent=2)}")
        else:
            print("FAILURE: No messages received in polling response")
    
    # Test polling again (should be empty)
    print("Polling again (should be empty)...")
    poll_response2 = requests.get(f"http://localhost:8000/poll?session={session_id}")
    if poll_response2.status_code == 200:
        poll_data2 = poll_response2.json() 
        print(f"Second poll: {len(poll_data2.get('messages', []))} messages")

if __name__ == "__main__":
    test_plugin_query()