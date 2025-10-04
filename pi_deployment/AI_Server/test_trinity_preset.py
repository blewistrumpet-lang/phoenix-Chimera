#!/usr/bin/env python3
"""Test Trinity Pipeline with preset generation"""

import requests
import json
import time

def test_preset_generation():
    """Test generating a preset with specific engine (CHAOS_GENERATOR = 41)"""
    
    # Test prompt that should trigger CHAOS_GENERATOR
    prompt = "Create a chaotic glitchy preset with lots of randomness and buffer repeats"
    
    print(f"Testing Trinity Pipeline with prompt: '{prompt}'")
    print("=" * 60)
    
    # Send request to the AI server
    url = "http://localhost:8000/generate"
    payload = {"prompt": prompt}
    
    print("Sending request to Trinity AI Server...")
    start_time = time.time()
    
    try:
        response = requests.post(url, json=payload, timeout=30)
        response_time = time.time() - start_time
        
        if response.status_code == 200:
            preset = response.json()
            print(f"\nSuccess! Response received in {response_time:.2f} seconds")
            print("\nPreset Name:", preset.get("name", "Unknown"))
            print("\nPreset Structure:")
            print(json.dumps(preset, indent=2))
            
            # Check for CHAOS_GENERATOR (ID 41) in the preset
            print("\n" + "=" * 60)
            print("Checking Engine IDs in preset:")
            
            # Handle nested response structure
            actual_preset = preset.get("preset", preset)
            
            if "parameters" in actual_preset:
                params = actual_preset["parameters"]
                engine_found = False
                
                for slot in range(1, 7):
                    engine_key = f"slot{slot}_engine"
                    if engine_key in params:
                        engine_id = int(params[engine_key])
                        print(f"  Slot {slot}: Engine ID {engine_id}", end="")
                        
                        # Check if it's CHAOS_GENERATOR
                        if engine_id == 41:
                            print(" ✓ CHAOS_GENERATOR FOUND!")
                            engine_found = True
                        elif engine_id == 40:
                            print(" ✓ BUFFER_REPEAT FOUND!")
                            engine_found = True
                        else:
                            print()
                
                if engine_found:
                    print("\n✅ Engine mapping test PASSED - Chaos/Buffer engines found!")
                else:
                    print("\n⚠️  No chaos/buffer engines found, but preset was generated")
            
            # Check macros if present
            if "macros" in preset:
                print("\nMacro suggestions:")
                for key, value in preset["macros"].items():
                    print(f"  {key}: {value}")
            
            return preset
            
        else:
            print(f"\n❌ Error: Status code {response.status_code}")
            print("Response:", response.text)
            return None
            
    except requests.exceptions.Timeout:
        print("\n❌ Error: Request timed out after 30 seconds")
        return None
    except Exception as e:
        print(f"\n❌ Error: {str(e)}")
        return None

def test_specific_engine():
    """Test requesting a specific engine by name"""
    
    prompt = "Make a preset using the chaos generator engine for experimental sounds"
    
    print(f"\n\nTest 2: Specific engine request")
    print(f"Prompt: '{prompt}'")
    print("=" * 60)
    
    url = "http://localhost:8000/generate"
    payload = {"prompt": prompt}
    
    try:
        response = requests.post(url, json=payload, timeout=30)
        
        if response.status_code == 200:
            preset = response.json()
            print(f"Success! Preset: {preset.get('name', 'Unknown')}")
            
            # Check engines
            if "parameters" in preset:
                for slot in range(1, 7):
                    engine_key = f"slot{slot}_engine"
                    if engine_key in preset["parameters"]:
                        engine_id = int(preset["parameters"][engine_key])
                        if engine_id == 41:
                            print(f"✅ CHAOS_GENERATOR found in slot {slot}!")
                            return True
            
            print("⚠️  CHAOS_GENERATOR not found in preset")
            return False
            
    except Exception as e:
        print(f"❌ Error: {str(e)}")
        return False

if __name__ == "__main__":
    # Run the tests
    print("TRINITY PIPELINE TEST - ENGINE ID MAPPING")
    print("=" * 60)
    
    # Test 1: General chaos preset
    preset1 = test_preset_generation()
    
    # Test 2: Specific engine request
    test_specific_engine()
    
    print("\n" + "=" * 60)
    print("Test complete!")