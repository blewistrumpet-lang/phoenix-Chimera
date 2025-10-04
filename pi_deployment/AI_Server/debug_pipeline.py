#!/usr/bin/env python3
"""
Debug the full enhanced pipeline to find the f-string error
"""

import sys
import logging

# Set up detailed logging
logging.basicConfig(level=logging.DEBUG)

from smart_oracle import SmartOracle
from oracle_enhanced import OracleEnhanced
from calculator_enhanced import CalculatorEnhanced
from alchemist_enhanced import AlchemistEnhanced
from cloud_bridge import CloudBridge

def test_pipeline():
    print("Testing Enhanced Pipeline Step by Step...")
    print("="*60)
    
    # Initialize components
    cloud_bridge = CloudBridge()
    
    # Paths for Oracle
    import os
    base_dir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3"
    index_path = os.path.join(base_dir, "faiss_index", "corpus_clean.index")
    meta_path = os.path.join(base_dir, "faiss_index", "metadata_clean.json") 
    presets_path = os.path.join(base_dir, "faiss_index", "presets_clean.json")
    
    oracle = OracleEnhanced(index_path, meta_path, presets_path)
    calculator = CalculatorEnhanced()
    alchemist = AlchemistEnhanced()
    
    prompt = "Create warm vintage vocals"
    
    # Step 1: Cloud AI (Visionary)
    print("\n1. CLOUD AI (VISIONARY):")
    try:
        import asyncio
        blueprint = asyncio.run(cloud_bridge.get_cloud_generation(prompt))
        if blueprint:
            print(f"   ✅ Blueprint: {blueprint.get('name', 'Unknown')}")
            print(f"   Type: {type(blueprint)}")
        else:
            # Fallback
            blueprint = {"name": "unknown", "character": ["warm", "vintage"]}
            print(f"   ⚠️ Using fallback blueprint")
    except Exception as e:
        print(f"   ❌ Error: {e}")
        import traceback
        traceback.print_exc()
        # Use fallback
        blueprint = {"name": "unknown", "character": ["warm", "vintage"]}
    
    # Step 2: Oracle
    print("\n2. ORACLE:")
    try:
        preset = oracle.find_best_preset(blueprint)
        print(f"   ✅ Preset: {preset.get('name', 'Unknown')}")
        print(f"   Type: {type(preset)}")
        # Check parameter types
        for slot in range(1, 7):
            for param in range(16):
                key = f"slot{slot}_param{param}"
                if key in preset:
                    val = preset[key]
                    print(f"   {key}: {val} (type: {type(val).__name__})")
                    if not isinstance(val, (int, float)):
                        print(f"   ⚠️ WARNING: {key} is not numeric!")
    except Exception as e:
        print(f"   ❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return
    
    # Step 3: Calculator
    print("\n3. CALCULATOR:")
    try:
        nudged = calculator.apply_nudges(preset, prompt, blueprint)
        print(f"   ✅ Nudged: {nudged.get('name', 'Unknown')}")
        print(f"   Type: {type(nudged)}")
    except Exception as e:
        print(f"   ❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return
    
    # Step 4: Alchemist
    print("\n4. ALCHEMIST:")
    try:
        final = alchemist.finalize_preset(nudged, prompt)
        print(f"   ✅ Final: {final.get('name', 'Unknown')}")
    except Exception as e:
        print(f"   ❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return
    
    print("\n" + "="*60)
    print("✅ Pipeline Complete!")

if __name__ == "__main__":
    test_pipeline()