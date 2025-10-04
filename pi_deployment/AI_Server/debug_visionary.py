#!/usr/bin/env python3
"""
Debug what the Cloud AI (Visionary) is generating
"""

import asyncio
from cloud_bridge import CloudBridge

async def test_visionary():
    """Test what blueprints the Visionary generates"""
    
    cloud = CloudBridge()
    
    test_prompts = [
        "vintage tube warmth",
        "shimmer reverb ethereal",
        "bit crusher distortion",
        "classic compressor clean",
        "tape echo with spring reverb"
    ]
    
    print("Testing Cloud AI (Visionary) Blueprint Generation")
    print("=" * 60)
    
    for prompt in test_prompts:
        print(f"\nPrompt: '{prompt}'")
        try:
            blueprint = await cloud.get_cloud_generation(prompt)
            print(f"Blueprint name: {blueprint.get('name', 'MISSING')}")
            print(f"Blueprint vibe: {blueprint.get('vibe', 'MISSING')}")
            print(f"Character: {blueprint.get('character', [])}")
            
            # Check if engines are specified
            for slot in range(1, 7):
                engine_key = f"slot{slot}_engine"
                if engine_key in blueprint:
                    print(f"  Slot {slot}: Engine {blueprint[engine_key]}")
            
        except Exception as e:
            print(f"ERROR: {e}")
    
    print("\n" + "=" * 60)
    print("ANALYSIS:")
    print("If all blueprints have the same name/vibe, Cloud AI is the problem.")
    print("If blueprints are different but generic, we need better prompting.")

if __name__ == "__main__":
    asyncio.run(test_visionary())