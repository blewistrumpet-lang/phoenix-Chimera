#!/usr/bin/env python3
"""
Analyze the corpus to see what engines are available after filtering
"""

import logging
import json
from oracle_faiss import OracleFAISS
from engine_mapping_authoritative import ENGINE_NAMES, get_engine_category

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def analyze_corpus():
    """Analyze what engines are available in the filtered corpus"""
    oracle = OracleFAISS()
    
    print(f"\nAnalyzing {len(oracle.presets)} presets after utility engine filtering:")
    print("="*80)
    
    # Collect all engines used
    all_engines = set()
    engine_counts = {}
    
    for i, preset in enumerate(oracle.presets):
        name = preset.get('creative_name', preset.get('name', f'Preset {i}'))
        engines = oracle._get_preset_engines(preset)
        
        print(f"\nPreset {i+1}: {name}")
        print(f"  Engines: {sorted(engines)}")
        
        # Show engine names and categories
        for engine_id in sorted(engines):
            engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
            category = get_engine_category(engine_id)
            print(f"    {engine_id}: {engine_name} ({category})")
            
            all_engines.add(engine_id)
            engine_counts[engine_id] = engine_counts.get(engine_id, 0) + 1
    
    print(f"\n{'='*80}")
    print("ENGINE USAGE SUMMARY")
    print("="*80)
    
    # Group by category
    categories = {}
    for engine_id in all_engines:
        category = get_engine_category(engine_id)
        if category not in categories:
            categories[category] = []
        categories[category].append(engine_id)
    
    for category, engines in sorted(categories.items()):
        print(f"\n{category}:")
        for engine_id in sorted(engines):
            name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
            count = engine_counts[engine_id]
            print(f"  {engine_id}: {name} (used in {count} presets)")
    
    print(f"\nTotal unique engines: {len(all_engines)}")
    print(f"Most used engines:")
    sorted_engines = sorted(engine_counts.items(), key=lambda x: x[1], reverse=True)
    for engine_id, count in sorted_engines[:10]:
        name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
        print(f"  {engine_id}: {name} ({count} presets)")

if __name__ == "__main__":
    analyze_corpus()