#!/usr/bin/env python3
"""
SYSTEMATIC TRINITY PIPELINE TEST
This is the most critical component - must be bulletproof
"""

import asyncio
import json
import sys
from pathlib import Path

# Add parent directory to path for imports
sys.path.insert(0, str(Path(__file__).parent))

from visionary_openai_direct import VisionaryOpenAIDirect
from oracle_faiss import OracleFAISS  
from calculator import Calculator
from alchemist import Alchemist
from engine_mapping import convert_preset_engine_ids, ENGINE_ID_TO_CHOICE

async def test_complete_pipeline():
    """Test the complete Trinity pipeline step by step"""
    print("\n" + "="*80)
    print("SYSTEMATIC TRINITY PIPELINE TEST")
    print("Testing each component in isolation and together")
    print("="*80)
    
    # Initialize components
    visionary = VisionaryOpenAIDirect()
    oracle = OracleFAISS()
    calculator = Calculator()
    alchemist = Alchemist()
    
    test_prompt = "aggressive metal guitar with heavy distortion and tight gate"
    
    print(f"\n🎯 TEST PROMPT: '{test_prompt}'")
    print("-"*80)
    
    # STEP 1: VISIONARY
    print("\n📊 STEP 1: VISIONARY")
    print("-"*40)
    blueprint = await visionary.get_blueprint(test_prompt)
    print(f"Blueprint vibe: {blueprint.get('overall_vibe')}")
    print("Requested engines:")
    for slot in blueprint['slots']:
        if slot['engine_id'] >= 0:
            print(f"  Slot {slot['slot']}: Engine ID {slot['engine_id']} ({slot.get('character', '')})")
    
    # STEP 2: ORACLE  
    print("\n🔮 STEP 2: ORACLE")
    print("-"*40)
    preset = oracle.find_best_preset(blueprint)
    print(f"Found preset: {preset.get('name', 'Unknown')}")
    print("Preset engines (from Oracle):")
    for i in range(1, 7):
        engine_id = preset['parameters'].get(f'slot{i}_engine', 0)
        if engine_id != 0:
            print(f"  Slot {i}: Engine ID {engine_id}")
    
    # STEP 3: CALCULATOR
    print("\n🧮 STEP 3: CALCULATOR")
    print("-"*40)
    nudged = calculator.apply_nudges(preset, test_prompt, blueprint)
    nudges = nudged.get('calculator_nudges', [])
    print(f"Applied {len(nudges)} nudges: {nudges[:5]}")
    
    # STEP 4: ALCHEMIST
    print("\n⚗️ STEP 4: ALCHEMIST")
    print("-"*40)
    final = alchemist.finalize_preset(nudged)
    print(f"Final preset name: {final.get('name', 'Unknown')}")
    print("Final engines (from Alchemist):")
    for i in range(1, 7):
        engine_id = final['parameters'].get(f'slot{i}_engine', 0)
        if engine_id != 0:
            print(f"  Slot {i}: Engine ID {engine_id}")
    
    # STEP 5: ENGINE MAPPING (CRITICAL!)
    print("\n🔄 STEP 5: ENGINE ID TO CHOICE MAPPING")
    print("-"*40)
    print("This is CRITICAL - the plugin expects choice indices, not engine IDs!")
    
    converted = convert_preset_engine_ids(final)
    
    print("\nEngine ID -> Choice Index conversions:")
    for i in range(1, 7):
        orig_id = final['parameters'].get(f'slot{i}_engine', 0)
        conv_idx = converted['parameters'].get(f'slot{i}_engine', 0)
        if orig_id != 0 and orig_id != -1:  # Not bypass
            status = "✅" if orig_id in ENGINE_ID_TO_CHOICE else "❌ MISSING"
            print(f"  Slot {i}: ID {orig_id:3d} -> Index {conv_idx:3d} {status}")
    
    # VERIFY CONVERSIONS
    print("\n🔍 VERIFICATION")
    print("-"*40)
    errors = []
    for i in range(1, 7):
        orig_id = final['parameters'].get(f'slot{i}_engine', 0)
        conv_idx = converted['parameters'].get(f'slot{i}_engine', 0)
        
        # Check if conversion is correct
        if orig_id != 0:  # Not bypass
            expected_idx = ENGINE_ID_TO_CHOICE.get(orig_id, -999)
            if expected_idx == -999:
                errors.append(f"Slot {i}: Engine ID {orig_id} not in mapping!")
            elif conv_idx != expected_idx:
                errors.append(f"Slot {i}: Expected index {expected_idx}, got {conv_idx}")
    
    if errors:
        print("❌ ERRORS FOUND:")
        for error in errors:
            print(f"  {error}")
    else:
        print("✅ All conversions correct!")
    
    # TEST PROBLEMATIC ENGINES
    print("\n⚠️ TESTING PROBLEMATIC ENGINES")
    print("-"*40)
    problematic_ids = [41, 40, 52]  # The ones that keep appearing
    for engine_id in problematic_ids:
        choice_idx = ENGINE_ID_TO_CHOICE.get(engine_id, -1)
        print(f"Engine ID {engine_id} -> Choice Index {choice_idx}")
    
    return converted

async def test_variety():
    """Test that different prompts produce different engines"""
    print("\n" + "="*80)
    print("VARIETY TEST - Different prompts should use different engines")
    print("="*80)
    
    visionary = VisionaryOpenAIDirect()
    oracle = OracleFAISS()
    
    test_cases = [
        "warm vintage guitar with tube saturation",
        "bright modern pop with compression",
        "dark ambient pad with long reverb",
        "aggressive metal with distortion",
        "clean jazz with subtle chorus"
    ]
    
    all_engines = []
    
    for prompt in test_cases:
        blueprint = await visionary.get_blueprint(prompt)
        preset = oracle.find_best_preset(blueprint)
        
        engines_used = []
        for i in range(1, 7):
            engine_id = preset['parameters'].get(f'slot{i}_engine', 0)
            if engine_id != 0:
                engines_used.append(engine_id)
        
        all_engines.append(engines_used)
        print(f"\n'{prompt[:30]}...': Engines {engines_used}")
    
    # Check variety
    unique_engines = set()
    for engines in all_engines:
        unique_engines.update(engines)
    
    print(f"\n📊 VARIETY ANALYSIS:")
    print(f"Total unique engines used: {len(unique_engines)}")
    print(f"Unique engines: {sorted(unique_engines)}")
    
    if len(unique_engines) < 3:
        print("❌ LOW VARIETY - Same engines being reused!")
    else:
        print("✅ Good variety in engine selection")

async def main():
    """Run all systematic tests"""
    print("\n" + "#"*80)
    print("# SYSTEMATIC TRINITY PIPELINE ANALYSIS")
    print("# This is the MOST CRITICAL component")
    print("#"*80)
    
    # Test 1: Complete pipeline
    converted_preset = await test_complete_pipeline()
    
    # Test 2: Variety
    await test_variety()
    
    # Final check
    print("\n" + "="*80)
    print("FINAL ANALYSIS")
    print("="*80)
    
    # Check if the converted preset would work in the plugin
    test_slot = converted_preset['parameters'].get('slot1_engine', 0)
    if 0 <= test_slot <= 53:
        print(f"✅ Slot 1 engine choice index {test_slot} is valid for plugin dropdown")
    else:
        print(f"❌ Slot 1 engine choice index {test_slot} is INVALID!")
    
    print("\n" + "#"*80)
    print("# END OF SYSTEMATIC TEST")
    print("#"*80)

if __name__ == "__main__":
    asyncio.run(main())