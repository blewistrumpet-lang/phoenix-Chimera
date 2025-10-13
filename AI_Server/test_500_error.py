#!/usr/bin/env python3
"""
Test to reproduce and diagnose the 500 Internal Server Error
"""

import requests
import json
import traceback

def test_500_error():
    """Try to reproduce the 500 error"""
    
    print("🔍 INVESTIGATING 500 ERROR")
    print("=" * 80)
    
    # Test prompts that might trigger the error
    test_prompts = [
        # Complex multi-engine requests
        "Use all 5 reverbs at once: plate, spring, hall, shimmer, and gated",
        "I need 10 engines: compression, EQ, gate, limiter, tube, distortion, chorus, phaser, delay, reverb",
        
        # Conflicting requirements
        "No effects at all but add lots of reverb",
        "Super clean but extremely distorted",
        
        # Edge cases
        "",  # Empty
        " ",  # Whitespace only
        "\n\n\n",  # Newlines
        "🎸" * 1000,  # Lots of emojis
        
        # Specific engine requests that might not exist
        "Use engine 999",
        "Add the quantum flux capacitor engine",
        
        # Requests that might cause assertion errors
        "Set all parameters to -1",
        "Set all parameters to 2.0",
        "Use negative feedback values",
    ]
    
    base_url = "http://localhost:8000"
    
    for i, prompt in enumerate(test_prompts, 1):
        print(f"\nTest {i}: {prompt[:50]}...")
        print("-" * 40)
        
        try:
            response = requests.post(
                f"{base_url}/generate",
                json={"prompt": prompt},
                timeout=10
            )
            
            if response.status_code == 500:
                print(f"❌ FOUND 500 ERROR!")
                print(f"   Prompt: {prompt}")
                print(f"   Response: {response.text[:500]}")
                
                # Try to get more details
                try:
                    error_data = response.json()
                    print(f"   Error details: {json.dumps(error_data, indent=2)[:500]}")
                except:
                    pass
                
                return prompt
                
            elif response.status_code != 200:
                print(f"⚠️ Status {response.status_code}")
            else:
                print(f"✅ Success")
                
        except Exception as e:
            print(f"❌ Exception: {str(e)}")
            traceback.print_exc()
    
    print("\n" + "=" * 80)
    print("Could not reproduce 500 error with test cases")
    
    # Try the exact sequence from the logs
    print("\nTrying sequence that might have caused the error...")
    
    # Based on logs, it seems to happen with many missing required engines
    complex_prompt = "I need state variable filter, gated reverb, muff fuzz, stereo chorus, plate reverb all at once"
    
    print(f"Testing: {complex_prompt}")
    try:
        response = requests.post(
            f"{base_url}/generate",
            json={"prompt": complex_prompt},
            timeout=10
        )
        
        if response.status_code == 500:
            print(f"❌ REPRODUCED 500 ERROR!")
            print(f"   Response: {response.text}")
        else:
            print(f"✅ Status {response.status_code}")
            
    except Exception as e:
        print(f"❌ Exception: {str(e)}")

if __name__ == "__main__":
    test_500_error()