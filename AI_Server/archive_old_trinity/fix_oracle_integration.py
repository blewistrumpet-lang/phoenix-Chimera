#!/usr/bin/env python3
"""
Fix the Oracle integration to properly use Cloud AI blueprints
"""

import asyncio
from cloud_bridge import CloudBridge
import json

async def test_cloud_blueprint():
    """Test what Cloud AI actually returns"""
    
    cloud = CloudBridge()
    prompt = "vintage tube warmth with plate reverb"
    
    print(f"Testing Cloud AI with: '{prompt}'")
    print("=" * 60)
    
    blueprint = await cloud.get_cloud_generation(prompt)
    
    print("\nRaw Blueprint Structure:")
    print(json.dumps(blueprint, indent=2, default=str)[:1000])
    
    print("\n" + "=" * 60)
    print("ANALYSIS:")
    print(f"Has 'vibe'? {('vibe' in blueprint)}")
    print(f"Has 'name'? {('name' in blueprint)}")
    print(f"Has 'creative_name'? {('creative_name' in blueprint)}")
    print(f"Has 'slots'? {('slots' in blueprint)}")
    print(f"Has 'slot1_engine'? {('slot1_engine' in blueprint)}")
    
    # Check if we need to convert slots array to flat format
    if 'slots' in blueprint and not 'slot1_engine' in blueprint:
        print("\n⚠️ PROBLEM: Blueprint has 'slots' array but Oracle expects flat format!")
        print("Need to convert slots array to slot1_engine, slot2_engine, etc.")
        
        # Show conversion
        print("\nConverted format:")
        for slot in blueprint.get('slots', []):
            slot_num = slot.get('slot', 0)
            engine_id = slot.get('engine_id', 0)
            print(f"  slot{slot_num}_engine = {engine_id}")
    
    return blueprint

if __name__ == "__main__":
    asyncio.run(test_cloud_blueprint())