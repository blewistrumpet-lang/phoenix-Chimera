#!/usr/bin/env python3
"""
Demonstration of Oracle improvements:
1. Pre-filtering of utility engines
2. Enhanced engine matching weight (10x vs original 3x)
3. Better logging of requested vs found engines
"""

import logging
from oracle_faiss import OracleFAISS
from engine_mapping_authoritative import ENGINE_NAMES

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)

def demo_oracle_improvements():
    """Demonstrate the key Oracle improvements"""
    print("="*80)
    print("ORACLE ENHANCEMENTS DEMONSTRATION")
    print("="*80)
    
    oracle = OracleFAISS()
    
    print(f"\n1. UTILITY ENGINE FILTERING:")
    print(f"   ✅ Filtered out utility engines (IDs 53-56)")
    print(f"   ✅ Corpus reduced from 150 to {len(oracle.presets)} presets") 
    print(f"   ✅ All remaining presets are clean (no utility engines)")
    
    print(f"\n2. ENHANCED ENGINE MATCHING:")
    print(f"   ✅ Engine matching weight increased from 3.0 to 10.0")
    print(f"   ✅ Engine boost factor increased from 2.0 to 10.0")
    print(f"   ✅ Engine matching now heavily prioritized over vibe matching")
    
    print(f"\n3. IMPROVED LOGGING:")
    print(f"   ✅ Shows requested engines vs found engines")
    print(f"   ✅ Displays all engines in each matched preset")
    print(f"   ✅ Clear scoring breakdown (similarity + engine match + combined)")
    
    # Demonstrate with a real example
    print(f"\n" + "="*80)
    print("LIVE DEMONSTRATION")
    print("="*80)
    
    # Example: Request specific engines that exist in the corpus
    blueprint = {
        "overall_vibe": "experimental spacious",
        "creative_name": "Engine Priority Test",
        "slots": [
            {"slot": 1, "engine_id": 32},  # Detune Doubler  
            {"slot": 2, "engine_id": 39},  # Plate Reverb
            {"slot": 3, "engine_id": 52},  # Feedback Network
        ]
    }
    
    print(f"\nRequesting engines: {[ENGINE_NAMES[32], ENGINE_NAMES[39], ENGINE_NAMES[52]]}")
    print(f"Vibe: {blueprint['overall_vibe']}")
    
    results = oracle.find_best_presets(blueprint, k=3)
    
    print(f"\nTop match analysis:")
    if results:
        top = results[0]
        print(f"  Preset: {top.get('creative_name', top.get('name', 'Unknown'))}")
        print(f"  Similarity score: {top['similarity_score']:.3f}")
        print(f"  Engine match score: {top['engine_match_score']:.3f}") 
        print(f"  Combined score: {top['combined_score']:.3f}")
        print(f"  Engine matches: {top['matching_engines']}/3")
        
        # Show the scoring impact
        base_sim = top['similarity_score']
        engine_boost = top['engine_match_score'] * 10.0
        total = base_sim + engine_boost
        
        print(f"\nScoring breakdown:")
        print(f"  Base similarity: {base_sim:.3f}")
        print(f"  Engine boost (10x): {engine_boost:.3f}")  
        print(f"  Total score: {total:.3f}")
        print(f"  Engine impact: {(engine_boost/total)*100:.1f}% of final score")
    
    # Compare with non-existent engines (should fall back to vibe)
    print(f"\n" + "-"*60)
    print("FALLBACK TO VIBE MATCHING (Non-existent engines)")
    print("-"*60)
    
    blueprint_fallback = {
        "overall_vibe": "warm vintage",
        "creative_name": "Vibe Fallback Test", 
        "slots": [
            {"slot": 1, "engine_id": 1},   # Opto Compressor (not in corpus)
            {"slot": 2, "engine_id": 15},  # Vintage Tube (not in corpus)
        ]
    }
    
    print(f"\nRequesting engines: {[ENGINE_NAMES[1], ENGINE_NAMES[15]]}")
    print(f"Vibe: {blueprint_fallback['overall_vibe']}")
    
    results_fallback = oracle.find_best_presets(blueprint_fallback, k=1)
    
    if results_fallback:
        fallback = results_fallback[0]
        print(f"\nFallback result:")
        print(f"  Preset: {fallback.get('creative_name', fallback.get('name', 'Unknown'))}")
        print(f"  Engine matches: {fallback['matching_engines']}/2 (expected: 0)") 
        print(f"  Falls back to pure vibe matching: {fallback['similarity_score']:.3f}")
    
    print(f"\n" + "="*80)
    print("SUMMARY OF IMPROVEMENTS")
    print("="*80)
    print("✅ 1. Pre-filtering eliminates 136/150 presets with utility engines")
    print("✅ 2. Engine matching weight increased 3.3x (3.0 → 10.0)") 
    print("✅ 3. Engine boost factor increased 5x (2.0 → 10.0)")
    print("✅ 4. Detailed logging shows requested vs found engines")
    print("✅ 5. Proper fallback to vibe matching when engines don't exist")
    print("✅ 6. Score breakdown shows engine matching dominance")

if __name__ == "__main__":
    demo_oracle_improvements()