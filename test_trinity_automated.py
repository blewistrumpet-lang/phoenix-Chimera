#!/usr/bin/env python3
"""Automated Trinity flow test"""

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
        response = requests.post(url, json=payload, timeout=30)
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
            
            print(f"\n4. ✅ Preset ready for plugin")
            print(f"   Plugin should show:")
            print(f"   - Trinity light: GREEN → BLUE → GREEN")
            print(f"   - Preset name: '{preset_name}'")
            print(f"   - Engine slots updated as above")
            
            return True
        else:
            print(f"2. ❌ Server error: {data.get('error', 'Unknown error')}")
            return False
            
    except requests.exceptions.Timeout:
        print(f"2. ❌ Request timed out after 30 seconds")
        return False
    except Exception as e:
        print(f"2. ❌ Request failed: {e}")
        return False

def main():
    """Run automated tests"""
    
    # Test various prompts
    test_prompts = [
        "mana from heaven",           # Poetic
        "thunderstorm of chaos",       # Poetic/aggressive  
        "warm vintage guitar",         # Technical
        "epic cinematic drums",        # Mixed
        "golden sunset warmth",        # Color/emotion
        "digital ice crystals"         # Abstract/technical
    ]
    
    print("\nTrinity Pipeline Automated Test")
    print("================================")
    print("Testing preset generation from various prompts...")
    
    results = []
    for i, prompt in enumerate(test_prompts, 1):
        print(f"\n[Test {i}/{len(test_prompts)}]")
        success = test_preset_generation(prompt)
        results.append((prompt, success))
        
        # Brief pause between tests
        if i < len(test_prompts):
            time.sleep(2)
    
    # Summary
    print(f"\n{'='*60}")
    print("TEST RESULTS SUMMARY")
    print(f"{'='*60}")
    
    passed = sum(1 for _, success in results if success)
    failed = len(results) - passed
    
    for prompt, success in results:
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status}: '{prompt}'")
    
    print(f"\nTotal: {passed}/{len(results)} passed, {failed} failed")
    
    if passed == len(results):
        print("\n🎉 All tests passed! Trinity Pipeline is working correctly!")
    else:
        print(f"\n⚠️  {failed} test(s) failed. Check server logs for details.")
    
    print(f"{'='*60}\n")
    
    return 0 if passed == len(results) else 1

if __name__ == "__main__":
    sys.exit(main())