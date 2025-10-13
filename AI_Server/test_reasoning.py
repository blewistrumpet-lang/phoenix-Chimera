#!/usr/bin/env python3
"""
Test script to demonstrate AI reasoning in the Trinity pipeline
Shows detailed explanations from each AI component
"""

import requests
import json
import time
import sys

def test_reasoning(prompt, wait_time=20):
    """Send a prompt and display detailed reasoning from AI components"""
    
    print(f"\n{'='*80}")
    print(f"🎵 PROMPT: {prompt}")
    print('='*80)
    
    # Send request to Trinity server
    url = "http://localhost:8000/message"
    
    payload = {
        "type": "query",
        "content": f"{prompt} {int(time.time())}",
        "message": f"{prompt} {int(time.time())}"
    }
    
    print(f"⏳ Sending to Trinity pipeline...")
    start_time = time.time()
    
    try:
        response = requests.post(url, json=payload, timeout=30)
        elapsed = time.time() - start_time
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Response received in {elapsed:.1f} seconds")
            
            # Display the preset name
            preset_name = result.get("data", {}).get("name", "Unknown")
            print(f"\n🎼 PRESET NAME: {preset_name}")
            print(f"📝 Message: {result.get('message', '')}")
            
            # Display engines used
            slots = result.get("data", {}).get("slots", [])
            active_slots = [s for s in slots if s.get("engine_id", 0) != 0]
            
            print(f"\n🔧 ENGINES SELECTED ({len(active_slots)} active):")
            for slot in active_slots:
                print(f"  Slot {slot['slot']}: {slot['engine_name']} (ID: {slot['engine_id']})")
                
                # Show parameter variations
                params = slot.get("parameters", [])[:5]  # First 5 params
                values = [p.get("value", 0.5) for p in params]
                unique_values = len(set(values))
                print(f"    Parameter variation: {unique_values}/5 unique values")
                print(f"    Sample values: {[f'{v:.2f}' for v in values]}")
            
            print(f"\n⚡ Performance: {elapsed:.1f}s total pipeline time")
            
        else:
            print(f"❌ Error: Status {response.status_code}")
            print(response.text)
            
    except requests.exceptions.Timeout:
        print("❌ Request timed out after 30 seconds")
    except Exception as e:
        print(f"❌ Error: {e}")
    
    # Wait a bit between tests
    print(f"\n⏰ Waiting {wait_time} seconds to check logs for reasoning...")
    time.sleep(wait_time)

def main():
    """Run multiple test prompts to show AI reasoning"""
    
    print("\n" + "🎭"*40)
    print("TRINITY AI REASONING DEMONSTRATION")
    print("This test shows how AI components explain their choices")
    print("🎭"*40)
    
    # Test prompts that should trigger interesting reasoning
    test_prompts = [
        "warm vintage analog tape with subtle compression and saturation",
        "aggressive djent metal guitar with tight gating and scooped mids",
        "ethereal ambient soundscape with long reverb tails and shimmer",
        "punchy drum bus with parallel compression and transient enhancement",
        "smooth jazz guitar with warm tube character and spring reverb"
    ]
    
    print(f"\n📋 Testing {len(test_prompts)} prompts to demonstrate AI reasoning")
    print("Check trinity_reasoning.log for detailed AI explanations!\n")
    
    for i, prompt in enumerate(test_prompts, 1):
        print(f"\n{'='*80}")
        print(f"TEST {i}/{len(test_prompts)}")
        test_reasoning(prompt, wait_time=5 if i < len(test_prompts) else 2)
    
    print("\n" + "✨"*40)
    print("TESTING COMPLETE!")
    print("Check trinity_reasoning.log for detailed reasoning:")
    print("  - 🧠 VISIONARY AI REASONING: Why engines were selected")
    print("  - 🧮 CALCULATOR AI REASONING: Why parameters were optimized")
    print("✨"*40)
    print("\nExample log entries to look for:")
    print('  grep "VISIONARY AI REASONING" trinity_reasoning.log -A 10')
    print('  grep "CALCULATOR AI REASONING" trinity_reasoning.log -A 10')
    print()

if __name__ == "__main__":
    main()