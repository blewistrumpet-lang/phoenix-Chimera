#!/usr/bin/env python3
"""
Test to demonstrate Visionary's engine selection reasoning
"""

import requests
import json
import time

def test_engine_reasoning(prompt):
    """Send a prompt and display engine selection reasoning"""
    
    print(f"\n{'='*80}")
    print(f"🎵 TESTING: {prompt}")
    print('='*80)
    
    # Send request to Trinity server
    url = "http://localhost:8000/message"
    
    payload = {
        "type": "query",
        "content": f"{prompt} {int(time.time())}",
        "message": f"{prompt} {int(time.time())}"
    }
    
    print(f"⏳ Processing through Trinity pipeline...")
    start_time = time.time()
    
    try:
        response = requests.post(url, json=payload, timeout=30)
        elapsed = time.time() - start_time
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Response received in {elapsed:.1f} seconds")
            
            # Display the preset name
            preset_name = result.get("data", {}).get("name", "Unknown")
            print(f"\n🎼 PRESET GENERATED: {preset_name}")
            
            # Display engines selected
            slots = result.get("data", {}).get("slots", [])
            active_slots = [s for s in slots if s.get("engine_id", 0) != 0]
            
            print(f"\n🔧 ENGINES SELECTED:")
            for slot in active_slots:
                print(f"  Slot {slot['slot']}: {slot['engine_name']} (ID: {slot['engine_id']})")
            
            print(f"\n📊 Stats:")
            print(f"  - {len(active_slots)} engines used")
            print(f"  - {elapsed:.1f}s total processing time")
            
        else:
            print(f"❌ Error: Status {response.status_code}")
            print(response.text)
            
    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    """Run tests that should trigger clear engine selection reasoning"""
    
    print("\n" + "🧠"*40)
    print("ENGINE SELECTION REASONING DEMONSTRATION")
    print("Check trinity_engine_reasoning.log for detailed explanations!")
    print("🧠"*40)
    
    # Test prompts designed to trigger specific engine selections
    test_prompts = [
        "warm vintage vocal with tape saturation and analog compression",
        "heavy metal guitar with aggressive distortion and tight noise gate",
        "ambient pad with long shimmer reverb and subtle chorus",
        "punchy drum bus with transient shaping and parallel compression",
        "clean acoustic guitar with spring reverb and gentle EQ"
    ]
    
    print(f"\n📋 Testing {len(test_prompts)} prompts")
    print("After each test, check the log file for:")
    print("  🧠 VISIONARY ENGINE SELECTION REASONING")
    print("  🧮 CALCULATOR AI REASONING")
    print()
    
    for prompt in test_prompts:
        test_engine_reasoning(prompt)
        print("\n⏰ Waiting 3 seconds before next test...")
        time.sleep(3)
    
    print("\n" + "✨"*40)
    print("TESTING COMPLETE!")
    print("\nTo see the engine selection reasoning, run:")
    print('  grep "VISIONARY ENGINE SELECTION" trinity_engine_reasoning.log -A 10')
    print('  grep "CALCULATOR AI REASONING" trinity_engine_reasoning.log -A 10')
    print("\nThe reasoning explains:")
    print("  - Why each engine was selected for its slot")
    print("  - The overall approach to achieve the requested sound")
    print("  - Signal flow decisions")
    print("  - Key parameter choices")
    print("✨"*40)

if __name__ == "__main__":
    main()