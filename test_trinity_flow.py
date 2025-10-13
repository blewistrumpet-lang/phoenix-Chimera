#!/usr/bin/env python3
"""Test the complete Trinity flow"""

import requests
import json
import time
import sys

def test_preset_generation(prompt):
    """Test preset generation and monitoring"""
    
    print(f"\n{'='*60}")
    print(f"Testing Trinity Pipeline")
    print(f"Prompt: '{prompt}'")
    print(f"{'='*60}\n")
    
    # Send request to server
    url = "http://localhost:8000/message"
    payload = {
        "type": "query",
        "content": prompt,
        "session_id": f"test_{int(time.time())}"
    }
    
    print("1. Sending request to server...")
    try:
        response = requests.post(url, json=payload)
        response.raise_for_status()
        data = response.json()
        
        if data.get("success"):
            preset = data.get("data", {}).get("preset", {})
            preset_name = preset.get("name", "Unknown")
            
            print(f"2. ✅ Server generated preset: '{preset_name}'")
            print(f"3. Preset details:")
            
            # Show active engines
            active_engines = []
            for slot in preset.get("slots", []):
                if slot.get("engine_id", 0) != 0:
                    active_engines.append(f"   Slot {slot['slot']}: {slot['engine_name']} (ID: {slot['engine_id']})")
            
            if active_engines:
                print("   Active engines:")
                for engine in active_engines:
                    print(engine)
            else:
                print("   No active engines (all slots None)")
            
            print(f"\n4. Check plugin UI:")
            print(f"   - Trinity light should change: GREEN → BLUE → GREEN")
            print(f"   - Preset name should appear: '{preset_name}'")
            print(f"   - Engine slots should update")
            
            return True
        else:
            print(f"2. ❌ Server error: {data.get('error', 'Unknown error')}")
            return False
            
    except Exception as e:
        print(f"2. ❌ Request failed: {e}")
        return False

def main():
    """Run tests"""
    
    # Test various prompts
    test_prompts = [
        "mana from heaven",           # Poetic
        "thunderstorm of chaos",       # Poetic/aggressive
        "warm vintage guitar",         # Technical
        "epic cinematic drums",        # Mixed
        "test preset",                 # Generic
        "golden sunset warmth",        # Color/emotion
        "digital ice crystals"         # Abstract/technical
    ]
    
    print("\nTrinity Pipeline Test Suite")
    print("============================")
    print("Make sure:")
    print("1. ChimeraPhoenix plugin is running")
    print("2. Trinity server is running on port 8000")
    print("3. Plugin UI is visible to see updates")
    
    input("\nPress Enter to start tests...")
    
    for i, prompt in enumerate(test_prompts, 1):
        print(f"\n[Test {i}/{len(test_prompts)}]")
        success = test_preset_generation(prompt)
        
        if i < len(test_prompts):
            input("\nPress Enter for next test...")
    
    print(f"\n{'='*60}")
    print("Test suite complete!")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()