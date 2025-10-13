#!/usr/bin/env python3
"""
Trace a single request through the enhanced pipeline
"""

import asyncio
import json
from cloud_bridge import CloudBridge
from oracle_enhanced import OracleEnhanced
from calculator_enhanced import CalculatorEnhanced
from alchemist_enhanced import AlchemistEnhanced
import os

async def trace_request():
    """Trace a request step by step"""
    
    prompt = "Add vintage tube warmth and plate reverb"
    print(f"TRACING: '{prompt}'")
    print("=" * 80)
    
    # Step 1: Cloud AI
    print("\n1. CLOUD AI (VISIONARY):")
    cloud = CloudBridge()
    blueprint = await cloud.get_cloud_generation(prompt)
    
    print(f"   Raw vibe: {blueprint.get('vibe', 'NONE')}")
    print(f"   Overall vibe: {blueprint.get('overall_vibe', 'NONE')}")
    print(f"   Creative name: {blueprint.get('creative_name', 'NONE')}")
    
    # Show slots
    if "slots" in blueprint:
        for slot in blueprint.get("slots", []):
            if slot.get("engine_id", 0) > 0:
                print(f"   Slot {slot['slot']}: Engine {slot['engine_id']} ({slot.get('character', '')})")
    
    # Convert format
    print("\n   Converting to Oracle format...")
    if "slots" in blueprint:
        for slot in blueprint.get("slots", []):
            slot_num = slot.get("slot", 0)
            engine_id = slot.get("engine_id", 0)
            blueprint[f"slot{slot_num}_engine"] = engine_id
            print(f"   Added slot{slot_num}_engine = {engine_id}")
    
    if "overall_vibe" in blueprint and "vibe" not in blueprint:
        blueprint["vibe"] = blueprint["overall_vibe"]
        print(f"   Set vibe = {blueprint['vibe']}")
    
    # Step 2: Oracle
    print("\n2. ORACLE:")
    base_dir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3"
    oracle = OracleEnhanced(
        os.path.join(base_dir, "faiss_index", "corpus_clean.index"),
        os.path.join(base_dir, "faiss_index", "metadata_clean.json"),
        os.path.join(base_dir, "faiss_index", "presets_clean.json")
    )
    
    print(f"   Blueprint vibe for search: {blueprint.get('vibe', 'NONE')}")
    preset = oracle.find_best_preset(blueprint)
    
    print(f"   Found preset: {preset.get('creative_name', 'Unknown')}")
    
    # Show engines
    for slot in range(1, 7):
        engine_id = preset.get(f"slot{slot}_engine", 0)
        if engine_id > 0:
            from engine_mapping_authoritative import ENGINE_NAMES
            print(f"   Slot {slot}: {ENGINE_NAMES.get(engine_id, f'Unknown({engine_id})')}")
    
    # Step 3: Calculator
    print("\n3. CALCULATOR:")
    calc = CalculatorEnhanced()
    nudged = calc.apply_nudges(preset, prompt, blueprint)
    
    # Show changes
    for slot in range(1, 7):
        old_engine = preset.get(f"slot{slot}_engine", 0)
        new_engine = nudged.get(f"slot{slot}_engine", 0)
        if old_engine != new_engine:
            from engine_mapping_authoritative import ENGINE_NAMES
            print(f"   Changed slot {slot}: {ENGINE_NAMES.get(old_engine, 'Empty')} → {ENGINE_NAMES.get(new_engine, 'Empty')}")
    
    # Step 4: Alchemist
    print("\n4. ALCHEMIST:")
    alch = AlchemistEnhanced()
    final = alch.finalize_preset(nudged, prompt)
    
    print(f"   Final name: {final.get('name', 'Unknown')}")
    
    # Show final engines
    print("\n   FINAL ENGINES:")
    for slot in range(1, 7):
        engine_id = final.get(f"slot{slot}_engine", 0)
        if engine_id > 0:
            from engine_mapping_authoritative import ENGINE_NAMES
            print(f"   Slot {slot}: {ENGINE_NAMES.get(engine_id, f'Unknown({engine_id})')}")
    
    # Analysis
    print("\n" + "=" * 80)
    print("ANALYSIS:")
    print(f"Did we get Vintage Tube (15)? {15 in [final.get(f'slot{s}_engine', 0) for s in range(1, 7)]}")
    print(f"Did we get Plate Reverb (39)? {39 in [final.get(f'slot{s}_engine', 0) for s in range(1, 7)]}")

if __name__ == "__main__":
    asyncio.run(trace_request())