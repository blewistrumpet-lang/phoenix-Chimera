#!/usr/bin/env python3
"""Test preset loading directly in the plugin via Trinity protocol"""

import asyncio
import websockets
import json
import time

async def test_preset_loading():
    """Connect to plugin and send a preset"""
    
    print("Testing direct preset loading to plugin...")
    print("=" * 60)
    
    # Connect to plugin's WebSocket endpoint (if available)
    # The plugin should be listening on a specific port
    
    # First, let's generate a preset from the Trinity server
    import requests
    
    # Generate preset from Trinity
    url = "http://localhost:8000/message"
    payload = {
        "type": "query", 
        "content": "epic thunder drums",
        "session_id": f"direct_test_{int(time.time())}"
    }
    
    print("1. Requesting preset from Trinity server...")
    response = requests.post(url, json=payload, timeout=30)
    
    if response.status_code == 200:
        data = response.json()
        if data.get("success"):
            preset = data.get("data", {}).get("preset", {})
            print(f"2. ✅ Got preset: {preset.get('name', 'Unknown')}")
            
            # Now we need to send this to the plugin
            # The plugin should be receiving via its Trinity connection
            print("\n3. Preset details:")
            for slot in preset.get("slots", []):
                if slot.get("engine_id", 0) != 0:
                    print(f"   Slot {slot['slot']}: {slot['engine_name']} (ID: {slot['engine_id']})")
            
            print("\n4. Plugin should now load this preset")
            print("   Check if:")
            print("   - Trinity light changes: GREEN → BLUE → GREEN")
            print("   - Engines appear in slots as listed above")
            print("   - Preset name shows in UI")
            
            return True
        else:
            print(f"2. ❌ Server error: {data.get('error')}")
            return False
    else:
        print(f"2. ❌ HTTP error: {response.status_code}")
        return False

if __name__ == "__main__":
    # For now, just test HTTP since plugin connects via its own WebSocket
    asyncio.run(test_preset_loading())