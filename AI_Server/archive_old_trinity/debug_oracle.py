#!/usr/bin/env python3
"""
Debug why Oracle returns the same preset for everything
"""

import os
from oracle_enhanced import OracleEnhanced

# Initialize Oracle
base_dir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3"
index_path = os.path.join(base_dir, "faiss_index", "corpus_clean.index")
meta_path = os.path.join(base_dir, "faiss_index", "metadata_clean.json") 
presets_path = os.path.join(base_dir, "faiss_index", "presets_clean.json")

oracle = OracleEnhanced(index_path, meta_path, presets_path)

print("Testing Oracle with different blueprints...")
print("=" * 60)

test_blueprints = [
    {"name": "vintage tube", "character": ["warm", "vintage"], "vibe": "warm vintage tube"},
    {"name": "shimmer reverb", "character": ["ethereal", "spacious"], "vibe": "shimmer reverb"},
    {"name": "bit crusher", "character": ["digital", "harsh"], "vibe": "bit crusher distortion"},
    {"name": "classic comp", "character": ["clean", "controlled"], "vibe": "classic compressor"},
]

for blueprint in test_blueprints:
    print(f"\n Blueprint: {blueprint['name']}")
    print(f"Searching for: {blueprint['vibe']}")
    result = oracle.find_best_preset(blueprint)
    print(f"Found: {result.get('creative_name', 'Unknown')}")
    
    # Show engines
    engines = []
    for slot in range(1, 7):
        engine_id = result.get(f"slot{slot}_engine", 0)
        if engine_id > 0:
            from engine_mapping_authoritative import ENGINE_NAMES
            engines.append(ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})"))
    print(f"Engines: {', '.join(engines[:4])}")

print("\n" + "=" * 60)
print("ANALYSIS:")
print("If all results are the same, the Oracle's search is broken.")
print("If results are different, the problem is earlier in the pipeline.")