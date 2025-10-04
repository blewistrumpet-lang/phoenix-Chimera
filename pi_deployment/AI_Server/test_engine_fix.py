#!/usr/bin/env python3
"""
Test script to verify that the Trinity Pipeline correctly handles
specific engine requests in prompts.
"""

import asyncio
import json
import logging
from typing import Dict, Any

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

async def test_pipeline():
    """Test the complete pipeline with specific engine requests"""
    
    # Import after setting up logging
    from cloud_bridge import get_cloud_generation
    from oracle_faiss import OracleFAISS
    from calculator import Calculator
    from alchemist import Alchemist
    from engine_mapping_correct import ENGINE_MAPPING
    
    # Initialize components
    oracle = OracleFAISS(
        index_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus.index",
        meta_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata.json",
        presets_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json"
    )
    calculator = Calculator(rules_path="nudge_rules.json")
    alchemist = Alchemist()
    
    # Test prompts with specific engine requests
    test_prompts = [
        "Create a dark horror preset with chaos generator and spectral freeze for terrifying atmospheres",
        "Make a shimmering ambient preset using shimmer reverb and granular clouds",
        "Build an aggressive metal tone with bitcrusher and gated reverb",
        "Design a vintage warmth preset with plate reverb and harmonic exciter"
    ]
    
    for prompt in test_prompts:
        print("\n" + "="*80)
        print(f"TESTING: {prompt}")
        print("="*80)
        
        try:
            # Step 1: Cloud/Visionary
            print("\n1. CLOUD/VISIONARY - Generating blueprint...")
            blueprint = await get_cloud_generation(prompt)
            
            print(f"   Creative Name: {blueprint.get('creative_name', 'Unknown')}")
            print(f"   Overall Vibe: {blueprint.get('overall_vibe', 'Unknown')}")
            print("\n   Requested Engines:")
            for slot in blueprint.get('slots', []):
                engine_id = slot.get('engine_id', -1)
                if engine_id > 0:
                    engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
                    print(f"   - Slot {slot.get('slot', '?')}: {engine_name} (ID: {engine_id})")
            
            # Step 2: Oracle
            print("\n2. ORACLE - Finding best match...")
            matches = oracle.find_best_presets(blueprint, k=3)
            if matches:
                best_match = matches[0]
                print(f"   Best match: {best_match.get('creative_name', 'Unknown')}")
                print(f"   - Similarity: {best_match.get('similarity_score', 0):.3f}")
                print(f"   - Engine matches: {best_match.get('matching_engines', 0)}")
                print(f"   - Combined score: {best_match.get('combined_score', 0):.3f}")
                
                # Adapt to blueprint
                preset = oracle._adapt_preset_to_blueprint(best_match, blueprint)
            else:
                print("   No matches found, using default")
                preset = oracle._create_default_preset(blueprint)
            
            # Step 3: Calculator
            print("\n3. CALCULATOR - Applying nudges...")
            nudged_preset = calculator.apply_nudges(preset, prompt, blueprint)
            
            calc_meta = nudged_preset.get('calculator_metadata', {})
            print(f"   Total adjustments: {calc_meta.get('total_adjustments', 0)}")
            print(f"   Affected parameters: {len(calc_meta.get('affected_parameters', []))}")
            
            if calc_meta.get('engine_emphasis'):
                print(f"   Engine emphasis applied to: {', '.join(calc_meta['engine_emphasis'])}")
            
            # Show some nudge details
            nudge_log = calc_meta.get('nudge_log', {})
            if nudge_log.get('applied_nudges'):
                print("\n   Sample nudges applied:")
                for nudge in nudge_log['applied_nudges'][:3]:
                    if isinstance(nudge, dict) and 'parameter' in nudge:
                        reason = nudge.get('reason', nudge.get('character', 'adjustment'))
                        adj = nudge.get('adjustment', 0)
                        print(f"   - {nudge['parameter']}: {adj:+.2f} ({reason})")
            
            # Step 4: Alchemist
            print("\n4. ALCHEMIST - Finalizing...")
            final_preset = alchemist.finalize_preset(nudged_preset)
            print(f"   Final name: {final_preset.get('name', 'Unknown')}")
            
            # Verify engines in final preset
            print("\n5. VERIFICATION - Final engine configuration:")
            for slot_num in range(1, 7):
                engine_key = f"slot{slot_num}_engine"
                bypass_key = f"slot{slot_num}_bypass"
                mix_key = f"slot{slot_num}_mix"
                
                if engine_key in final_preset.get('parameters', {}):
                    engine_id = final_preset['parameters'][engine_key]
                    if engine_id > 0:
                        engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
                        bypassed = final_preset['parameters'].get(bypass_key, 0) > 0.5
                        mix_level = final_preset['parameters'].get(mix_key, 0.5)
                        
                        status = "BYPASSED" if bypassed else f"Active (mix: {mix_level:.2f})"
                        print(f"   Slot {slot_num}: {engine_name} - {status}")
            
            print("\n✅ Test completed successfully")
            
        except Exception as e:
            print(f"\n❌ Error: {str(e)}")
            import traceback
            traceback.print_exc()

if __name__ == "__main__":
    asyncio.run(test_pipeline())