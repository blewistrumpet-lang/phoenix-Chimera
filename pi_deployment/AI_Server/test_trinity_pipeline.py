#!/usr/bin/env python3
"""
Complete Trinity Pipeline Test
Tests the entire flow from prompt to preset delivery
"""

import asyncio
import aiohttp
import json
import time

async def test_complete_pipeline():
    """Test the complete Trinity pipeline end-to-end"""
    
    # Use the session from the running plugin
    session_id = "http_session_1996544f505_70ba7ec2f3059816"
    base_url = "http://localhost:8000"
    
    print("="*60)
    print("TRINITY PIPELINE END-TO-END TEST")
    print("="*60)
    
    async with aiohttp.ClientSession() as session:
        # Step 1: Send a test query
        print("\n1. Sending query: 'ethereal space ambient'")
        query_data = {
            "type": "query",
            "content": "ethereal space ambient",
            "message": "ethereal space ambient",
            "session_id": session_id,
            "timestamp": int(time.time() * 1000)
        }
        
        async with session.post(f"{base_url}/message", json=query_data) as resp:
            result = await resp.json()
            print(f"   Response: {result}")
        
        # Step 2: Wait for processing
        print("\n2. Waiting for pipeline to complete...")
        await asyncio.sleep(3)
        
        # Step 3: Poll for messages
        print("\n3. Polling for preset response...")
        poll_url = f"{base_url}/poll?session={session_id}"
        
        for attempt in range(5):
            async with session.get(poll_url) as resp:
                poll_result = await resp.json()
                
                if poll_result.get("messages") and len(poll_result["messages"]) > 0:
                    print(f"   ✅ Received {len(poll_result['messages'])} message(s)")
                    
                    for msg in poll_result["messages"]:
                        if msg.get("type") == "preset":
                            preset = msg.get("data", {}).get("preset", {})
                            print(f"\n   🎼 PRESET RECEIVED: {preset.get('name', 'Unknown')}")
                            print(f"   Engines:")
                            for slot in preset.get("slots", []):
                                if slot.get("engine_id", 0) != 0:
                                    print(f"      Slot {slot['slot']}: {slot.get('engine_name', 'Unknown')}")
                            return True
                else:
                    print(f"   Attempt {attempt+1}: No messages yet...")
                    await asyncio.sleep(1)
        
        print("   ❌ No preset received after 5 attempts")
        return False

async def main():
    success = await test_complete_pipeline()
    
    print("\n" + "="*60)
    if success:
        print("✅ PIPELINE TEST SUCCESSFUL!")
        print("The Trinity Pipeline is working end-to-end.")
    else:
        print("❌ PIPELINE TEST FAILED")
        print("Check server logs for errors.")
    print("="*60)

if __name__ == "__main__":
    asyncio.run(main())
