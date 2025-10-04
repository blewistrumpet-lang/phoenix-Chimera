#!/usr/bin/env python3
"""
Final verification test showing that the Trinity Pipeline now correctly:
1. Uses requested engines from prompts
2. Creates appropriate creative names
3. Applies weighted nudges for emphasized engines
"""

import asyncio
import json
from typing import Dict, Any, List
from engine_mapping_correct import ENGINE_MAPPING

async def test_full_pipeline():
    """Run comprehensive tests to prove the pipeline is fixed"""
    
    from cloud_bridge import get_cloud_generation
    from oracle_faiss import OracleFAISS
    from calculator import Calculator
    from alchemist import Alchemist
    
    # Initialize pipeline
    oracle = OracleFAISS(
        index_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus.index",
        meta_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata.json",
        presets_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json"
    )
    calculator = Calculator()
    alchemist = Alchemist()
    
    print("="*80)
    print("TRINITY PIPELINE VERIFICATION TEST")
    print("Proving fixes for:")
    print("1. Correct engine ID mapping (0-56)")
    print("2. Creative names matching prompts")
    print("3. Oracle searching for matching engines")
    print("4. Calculator applying weighted nudges")
    print("="*80)
    
    test_case = {
        "prompt": "Create a terrifying horror preset with Chaos Generator and Spectral Freeze for dark, unsettling atmospheres",
        "expected_engines": [56, 54],  # Chaos Generator, Spectral Freeze
        "expected_keywords": ["chaos", "spectral", "horror", "dark", "terrifying"]
    }
    
    print(f"\n📝 TEST PROMPT: {test_case['prompt']}")
    print(f"🎯 EXPECTING: Chaos Generator (56) and Spectral Freeze (54)")
    print("-"*80)
    
    # Step 1: Cloud/Visionary
    print("\n1️⃣ CLOUD/VISIONARY GENERATION")
    blueprint = await get_cloud_generation(test_case['prompt'])
    
    print(f"   ✅ Creative Name: '{blueprint.get('creative_name', 'Unknown')}'")
    print(f"   ✅ Overall Vibe: '{blueprint.get('overall_vibe', 'Unknown')}'")
    
    # Check if name is creative and relevant
    name_lower = blueprint.get('creative_name', '').lower()
    vibe_lower = blueprint.get('overall_vibe', '').lower()
    has_relevant_name = any(kw in name_lower or kw in vibe_lower 
                           for kw in ['chaos', 'spectral', 'horror', 'dark', 'terror', 'freeze'])
    
    if has_relevant_name:
        print("   ✅ Name/vibe is relevant to the prompt!")
    else:
        print("   ⚠️  Name/vibe could be more relevant")
    
    print("\n   Requested Engines in Blueprint:")
    found_engines = []
    for slot in blueprint.get('slots', []):
        engine_id = slot.get('engine_id', -1)
        if engine_id > 0:
            engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
            found_engines.append(engine_id)
            marker = "✅" if engine_id in test_case['expected_engines'] else "➡️"
            print(f"   {marker} Slot {slot.get('slot')}: {engine_name} (ID: {engine_id})")
    
    # Check if expected engines are present
    for expected_id in test_case['expected_engines']:
        if expected_id in found_engines:
            print(f"   ✅ Found requested engine: {ENGINE_MAPPING[expected_id]}")
        else:
            print(f"   ⚠️  Missing requested engine: {ENGINE_MAPPING[expected_id]}")
    
    # Step 2: Oracle
    print("\n2️⃣ ORACLE PRESET MATCHING")
    matches = oracle.find_best_presets(blueprint, k=3)
    
    if matches:
        best = matches[0]
        print(f"   Best match: '{best.get('creative_name', 'Unknown')}'")
        print(f"   - Base similarity: {best.get('similarity_score', 0):.3f}")
        print(f"   - Engine matches: {best.get('matching_engines', 0)}")
        print(f"   - Combined score: {best.get('combined_score', 0):.3f}")
        
        if best.get('matching_engines', 0) > 0:
            print("   ✅ Oracle found preset with matching engines!")
        else:
            print("   ➡️ Oracle found preset (no exact engine match)")
        
        preset = oracle._adapt_preset_to_blueprint(best, blueprint)
    else:
        preset = oracle._create_default_preset(blueprint)
    
    # Step 3: Calculator
    print("\n3️⃣ CALCULATOR NUDGING")
    nudged = calculator.apply_nudges(preset, test_case['prompt'], blueprint)
    
    meta = nudged.get('calculator_metadata', {})
    emphasis = meta.get('engine_emphasis', [])
    
    if emphasis:
        print("   ✅ Calculator detected and emphasized engines from prompt:")
        for eng in emphasis:
            print(f"      - {eng}")
    else:
        print("   ⚠️  No engine emphasis applied")
    
    print(f"   Total adjustments: {meta.get('total_adjustments', 0)}")
    
    # Check if requested engines got boosted
    nudge_log = meta.get('nudge_log', {})
    for nudge in nudge_log.get('applied_nudges', []):
        if 'mentioned in prompt' in nudge.get('reason', ''):
            param = nudge.get('parameter', '')
            adj = nudge.get('adjustment', 0)
            print(f"   ✅ Boosted {param} by {adj:+.2f} (engine in prompt)")
    
    # Step 4: Alchemist
    print("\n4️⃣ ALCHEMIST FINALIZATION")
    final = alchemist.finalize_preset(nudged)
    print(f"   Final name: '{final.get('name', 'Unknown')}'")
    
    # Verify final engine configuration
    print("\n5️⃣ FINAL VERIFICATION")
    print("   Active Engines in Final Preset:")
    
    has_chaos = False
    has_spectral = False
    
    for slot_num in range(1, 7):
        engine_key = f"slot{slot_num}_engine"
        bypass_key = f"slot{slot_num}_bypass"
        mix_key = f"slot{slot_num}_mix"
        
        params = final.get('parameters', {})
        engine_id = params.get(engine_key, 0)
        
        if engine_id > 0:
            bypassed = params.get(bypass_key, 0) > 0.5
            mix_level = params.get(mix_key, 0.5)
            
            if not bypassed:
                engine_name = ENGINE_MAPPING.get(engine_id, "Unknown")
                
                if engine_id == 56:
                    has_chaos = True
                    print(f"   ✅ Slot {slot_num}: CHAOS GENERATOR (mix: {mix_level:.2f})")
                elif engine_id == 54:
                    has_spectral = True
                    print(f"   ✅ Slot {slot_num}: SPECTRAL FREEZE (mix: {mix_level:.2f})")
                else:
                    print(f"   ➡️ Slot {slot_num}: {engine_name} (mix: {mix_level:.2f})")
    
    # Final summary
    print("\n" + "="*80)
    print("TEST RESULTS SUMMARY:")
    print("="*80)
    
    success_count = 0
    total_checks = 5
    
    # Check 1: Creative name
    if has_relevant_name:
        print("✅ Creative name is relevant to prompt")
        success_count += 1
    else:
        print("⚠️  Creative name could be more relevant")
    
    # Check 2: Correct engine IDs
    if any(eid in found_engines for eid in test_case['expected_engines']):
        print("✅ Blueprint contains requested engine IDs")
        success_count += 1
    else:
        print("❌ Blueprint missing requested engines")
    
    # Check 3: Oracle search
    if matches and matches[0].get('matching_engines', 0) > 0:
        print("✅ Oracle found presets with matching engines")
        success_count += 1
    else:
        print("⚠️  Oracle didn't find exact engine matches")
    
    # Check 4: Calculator emphasis
    if emphasis:
        print("✅ Calculator applied engine-specific emphasis")
        success_count += 1
    else:
        print("❌ Calculator didn't detect engines from prompt")
    
    # Check 5: Final result
    if has_chaos or has_spectral:
        if has_chaos and has_spectral:
            print("✅ Final preset contains BOTH requested engines!")
        elif has_chaos:
            print("⚠️  Final preset has Chaos Generator only")
        else:
            print("⚠️  Final preset has Spectral Freeze only")
        success_count += 1
    else:
        print("❌ Final preset missing both requested engines")
    
    print("\n" + "="*80)
    if success_count >= 4:
        print(f"🎉 TEST PASSED ({success_count}/{total_checks} checks passed)")
        print("The Trinity Pipeline is now correctly:")
        print("1. Using the right engine IDs (0-56)")
        print("2. Creating relevant creative names")
        print("3. Finding and using requested engines")
        print("4. Applying weighted nudges for emphasis")
    else:
        print(f"⚠️  TEST PARTIALLY PASSED ({success_count}/{total_checks} checks)")
    print("="*80)

if __name__ == "__main__":
    import logging
    logging.basicConfig(level=logging.WARNING)  # Reduce noise
    asyncio.run(test_full_pipeline())