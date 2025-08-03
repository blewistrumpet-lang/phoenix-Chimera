#!/usr/bin/env python3
"""Test Trinity AI server connection and preset generation"""

import requests
import json
import time

def test_server_health():
    """Test if server is running"""
    try:
        response = requests.get("http://localhost:8000/health")
        if response.status_code == 200:
            print("✅ Trinity Server is healthy:", response.json())
            return True
    except:
        print("❌ Trinity Server is not responding")
        return False

def test_preset_generation():
    """Test preset generation through Trinity pipeline"""
    prompt = "Create a warm, vintage analog synthesizer sound with subtle modulation"
    
    print(f"\n📝 Testing preset generation with prompt: '{prompt}'")
    
    try:
        response = requests.post(
            "http://localhost:8000/generate",
            json={"prompt": prompt},
            timeout=30
        )
        
        if response.status_code == 200:
            result = response.json()
            if result.get("success"):
                print("✅ Preset generated successfully!")
                preset = result.get("preset", {})
                print(f"   Name: {preset.get('name', 'Unknown')}")
                print(f"   Description: {preset.get('description', 'No description')}")
                
                # Check if parameters are present
                params = preset.get("parameters", {})
                if params:
                    print(f"   Parameters: {len(params)} parameters configured")
                    # Show first few parameters
                    for i, (key, value) in enumerate(list(params.items())[:5]):
                        print(f"     - {key}: {value}")
                    if len(params) > 5:
                        print(f"     ... and {len(params) - 5} more")
                
                return True
            else:
                print("❌ Preset generation failed:", result.get("message"))
        else:
            print(f"❌ Server returned error {response.status_code}")
    except requests.exceptions.Timeout:
        print("❌ Request timed out (server may be processing)")
    except Exception as e:
        print(f"❌ Error: {e}")
    
    return False

def test_oracle_search():
    """Test Oracle's FAISS search capability"""
    prompt = "aggressive distortion"
    
    print(f"\n🔍 Testing Oracle search with prompt: '{prompt}'")
    
    try:
        response = requests.post(
            "http://localhost:8000/oracle/search",
            json={"prompt": prompt, "k": 3}
        )
        
        if response.status_code == 200:
            results = response.json()
            print(f"✅ Found {len(results)} similar presets:")
            for i, result in enumerate(results, 1):
                print(f"   {i}. {result.get('name', 'Unknown')} (similarity: {result.get('similarity', 0):.3f})")
            return True
        else:
            print(f"❌ Search failed with status {response.status_code}")
    except Exception as e:
        print(f"❌ Error: {e}")
    
    return False

def main():
    print("=" * 60)
    print("Trinity AI Server Connection Test")
    print("=" * 60)
    
    # Test server health
    if not test_server_health():
        print("\n⚠️  Server is not running. Start it with:")
        print("    cd AI_Server && python3 start_server.py")
        return
    
    # Test Oracle search
    test_oracle_search()
    
    # Test preset generation
    test_preset_generation()
    
    print("\n" + "=" * 60)
    print("Test complete!")
    print("=" * 60)
    
    print("\n📌 To test in Logic Pro:")
    print("1. Create an Audio Track")
    print("2. Add ChimeraPhoenix as an Audio Effect")
    print("3. Enter a prompt in the Command Center")
    print("4. Click GENERATE")
    print("5. The Trinity pipeline will create a preset")

if __name__ == "__main__":
    main()