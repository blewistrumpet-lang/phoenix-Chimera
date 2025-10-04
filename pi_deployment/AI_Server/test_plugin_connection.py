#!/usr/bin/env python3
"""
Test script to verify the Phoenix plugin can connect to the TRUE Trinity server
"""

import requests
import json
import time

def test_server_health():
    """Test if server is healthy"""
    print("Testing server health...")
    try:
        response = requests.get("http://localhost:8000/health", timeout=5)
        if response.status_code == 200:
            health = response.json()
            print(f"✅ Server is {health.get('status', 'unknown')}")
            print(f"   Version: {health.get('version', 'unknown')}")
            
            # Check components
            components = health.get('components', {})
            print(f"   Visionary: {components.get('visionary', 'error')}")
            print(f"   Calculator: {components.get('calculator', 'error')}")
            print(f"   Alchemist: {components.get('alchemist', 'error')}")
            print(f"   Oracle: {components.get('oracle', 'should be removed!')}")
            print(f"   Corpus: {components.get('corpus', 'should not be needed!')}")
            return True
        else:
            print(f"❌ Server returned status code: {response.status_code}")
            return False
    except requests.exceptions.ConnectionError:
        print("❌ Cannot connect to server at http://localhost:8000")
        print("   Run ./start_trinity.sh first!")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

def test_generate_endpoint():
    """Test the /generate endpoint"""
    print("\nTesting preset generation...")
    
    test_prompts = [
        "warm vintage guitar tone",
        "aggressive metal distortion",
        "ethereal ambient reverb"
    ]
    
    for prompt in test_prompts:
        print(f"\n📝 Testing: '{prompt}'")
        
        request_data = {
            "prompt": prompt,
            "intensity": 0.5,
            "complexity": 3
        }
        
        try:
            start_time = time.time()
            response = requests.post(
                "http://localhost:8000/generate",
                json=request_data,
                timeout=30
            )
            elapsed = time.time() - start_time
            
            if response.status_code == 200:
                result = response.json()
                if result.get('success'):
                    preset = result.get('preset', {})
                    metadata = result.get('metadata', {})
                    
                    print(f"✅ Generated: {preset.get('name', 'Unnamed')}")
                    print(f"   Time: {elapsed:.2f}s")
                    print(f"   Pipeline: {metadata.get('pipeline_version', 'unknown')}")
                    
                    # Verify it's using TRUE Trinity (no Oracle/corpus)
                    if metadata.get('no_oracle') and metadata.get('no_corpus'):
                        print(f"   ✅ TRUE Trinity confirmed (no Oracle/corpus)")
                    else:
                        print(f"   ⚠️  Warning: May be using old Trinity")
                    
                    # Check for slots (plugin format)
                    slot_count = 0
                    for i in range(1, 7):
                        if f"slot{i}_engine" in preset:
                            slot_count += 1
                    print(f"   Slots configured: {slot_count}/6")
                    
                else:
                    print(f"❌ Generation failed: {result.get('message', 'Unknown error')}")
            else:
                print(f"❌ Server returned status code: {response.status_code}")
                print(f"   Response: {response.text[:200]}")
                
        except requests.exceptions.Timeout:
            print("❌ Request timed out after 30 seconds")
        except Exception as e:
            print(f"❌ Error: {e}")

def main():
    print("="*60)
    print("🔌 PHOENIX PLUGIN → TRUE TRINITY CONNECTION TEST")
    print("="*60)
    
    # Test health endpoint
    if not test_server_health():
        print("\n⚠️  Server is not running!")
        print("Start it with: ./start_trinity.sh")
        return
    
    # Test generation
    test_generate_endpoint()
    
    print("\n" + "="*60)
    print("📌 Connection Summary:")
    print("  • Server URL: http://localhost:8000")
    print("  • Endpoints: /generate, /health")
    print("  • Pipeline: TRUE Trinity (no Oracle/corpus)")
    print("  • Plugin expects: slot1_engine, slot1_param0, etc.")
    print("="*60)

if __name__ == "__main__":
    main()