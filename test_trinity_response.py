#!/usr/bin/env python3
"""
Test script to verify Trinity response format and help debug plugin integration
"""

import requests
import json

def test_trinity_response():
    """Test Trinity endpoint response format"""
    url = "http://localhost:8000/generate"
    payload = {
        "prompt": "warm vintage compression test",
        "max_generation_time": 30
    }
    
    try:
        print("Testing Trinity endpoint...")
        response = requests.post(url, json=payload, timeout=30)
        
        if response.status_code == 200:
            data = response.json()
            print("\n✅ Request successful!")
            print(f"Status code: {response.status_code}")
            
            # Check top-level structure
            print("\n🔍 Top-level structure:")
            for key in data.keys():
                print(f"  - {key}: {type(data[key])}")
            
            # Check preset structure
            if "preset" in data:
                preset = data["preset"]
                print(f"\n🎵 Preset structure:")
                for key in preset.keys():
                    print(f"  - {key}: {type(preset[key])}")
                
                # Check slots structure
                if "slots" in preset:
                    slots = preset["slots"]
                    print(f"\n🎰 Slots structure:")
                    print(f"  - Number of slots: {len(slots)}")
                    
                    if len(slots) > 0:
                        print(f"  - First slot structure:")
                        for key in slots[0].keys():
                            print(f"    - {key}: {type(slots[0][key])}")
                        
                        # Show actual engine IDs
                        print(f"\n🔧 Engine IDs in slots:")
                        for i, slot in enumerate(slots):
                            engine_id = slot.get("engine_id", "missing")
                            engine_name = slot.get("engine_name", "missing")
                            param_count = len(slot.get("parameters", []))
                            print(f"    Slot {i}: Engine {engine_id} ({engine_name}) - {param_count} params")
                
                print(f"\n📋 Full response structure (formatted):")
                print(json.dumps(data, indent=2))
                
            else:
                print("❌ No 'preset' key found in response!")
                
        else:
            print(f"❌ Request failed with status code: {response.status_code}")
            print(f"Response: {response.text}")
            
    except requests.exceptions.RequestException as e:
        print(f"❌ Request error: {e}")
    except json.JSONDecodeError as e:
        print(f"❌ JSON decode error: {e}")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")

if __name__ == "__main__":
    test_trinity_response()