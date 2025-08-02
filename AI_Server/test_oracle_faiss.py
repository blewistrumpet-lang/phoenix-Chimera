#!/usr/bin/env python3
"""Test the Oracle FAISS integration"""

import json
import logging
from oracle_faiss import OracleFAISS

logging.basicConfig(level=logging.INFO)

def test_oracle():
    # Initialize Oracle
    oracle = OracleFAISS()
    
    # Test blueprint 1: Warm vintage tone
    blueprint1 = {
        "slots": [
            {"slot": 1, "engine_id": 0, "character": "warm"},
            {"slot": 2, "engine_id": 1, "character": "vintage"}
        ],
        "overall_vibe": "warm vintage tone"
    }
    
    print("\n=== Test 1: Warm Vintage Tone ===")
    results = oracle.find_best_presets(blueprint1, k=3)
    for i, preset in enumerate(results):
        print(f"{i+1}. {preset.get('name', 'Unknown')} - Score: {preset.get('similarity_score', 0):.3f}")
        print(f"   Category: {preset.get('category', 'Unknown')}")
        print(f"   Vibe: {preset.get('vibe', 'Unknown')}")
    
    # Test blueprint 2: Aggressive space
    blueprint2 = {
        "slots": [
            {"slot": 1, "engine_id": 36, "character": "aggressive"},  # Rodent Distortion
            {"slot": 2, "engine_id": 2, "character": "spacious"}      # Shimmer Reverb
        ],
        "overall_vibe": "aggressive spacious sound"
    }
    
    print("\n=== Test 2: Aggressive Space ===")
    results = oracle.find_best_presets(blueprint2, k=3)
    for i, preset in enumerate(results):
        print(f"{i+1}. {preset.get('name', 'Unknown')} - Score: {preset.get('similarity_score', 0):.3f}")
        print(f"   Category: {preset.get('category', 'Unknown')}")
    
    # Test blending
    print("\n=== Test 3: Blending Top 2 Presets ===")
    if len(results) >= 2:
        blended = oracle.blend_presets(results[:2], weights=[0.7, 0.3])
        print(f"Blended preset: {blended.get('name', 'Unknown')}")
        print(f"Number of parameters: {len(blended.get('parameters', {}))}")

if __name__ == "__main__":
    test_oracle()