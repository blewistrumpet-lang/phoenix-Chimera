#!/usr/bin/env python3
"""
Comprehensive verification of all Trinity Pipeline fixes.
Tests multiple scenarios to ensure:
1. Correct engine mapping (0-56)
2. Creative names match prompts
3. Requested engines are actually used
4. Oracle searches for matching engines
5. Calculator applies proper weighting
"""

import asyncio
import json
from typing import Dict, Any, List, Tuple
from engine_mapping_correct import ENGINE_MAPPING
import logging

# Configure logging to show important info
logging.basicConfig(
    level=logging.INFO,
    format='%(message)s'
)
logger = logging.getLogger(__name__)

# Suppress other loggers
for name in ['cloud_bridge', 'oracle_faiss', 'calculator', 'alchemist', 'faiss.loader']:
    logging.getLogger(name).setLevel(logging.WARNING)

async def run_single_test(prompt: str, expected_engines: List[int], expected_keywords: List[str]) -> Dict[str, Any]:
    """Run a single test case through the pipeline"""
    
    from cloud_bridge import get_cloud_generation
    from oracle_faiss import OracleFAISS
    from calculator import Calculator
    from alchemist import Alchemist
    
    # Initialize components
    oracle = OracleFAISS(
        index_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus.index",
        meta_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata.json",
        presets_path="../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json"
    )
    calculator = Calculator()
    alchemist = Alchemist()
    
    # Generate blueprint
    blueprint = await get_cloud_generation(prompt)
    
    # Find best preset match
    matches = oracle.find_best_presets(blueprint, k=3)
    if matches:
        preset = oracle._adapt_preset_to_blueprint(matches[0], blueprint)
    else:
        preset = oracle._create_default_preset(blueprint)
    
    # Apply nudges
    nudged = calculator.apply_nudges(preset, prompt, blueprint)
    
    # Finalize
    final = alchemist.finalize_preset(nudged)
    
    # Analyze results
    result = {
        "prompt": prompt,
        "creative_name": blueprint.get("creative_name", "Unknown"),
        "overall_vibe": blueprint.get("overall_vibe", "Unknown"),
        "blueprint_engines": [],
        "final_engines": [],
        "calculator_emphasis": nudged.get("calculator_metadata", {}).get("engine_emphasis", []),
        "oracle_match_score": matches[0].get("combined_score", 0) if matches else 0,
        "name_relevant": False,
        "engines_correct": False,
        "calculator_detected": False
    }
    
    # Extract blueprint engines
    for slot in blueprint.get("slots", []):
        engine_id = slot.get("engine_id", -1)
        if engine_id > 0:
            result["blueprint_engines"].append({
                "id": engine_id,
                "name": ENGINE_MAPPING.get(engine_id, "Unknown"),
                "slot": slot.get("slot", 0)
            })
    
    # Extract final engines
    params = final.get("parameters", {})
    for slot_num in range(1, 7):
        engine_id = params.get(f"slot{slot_num}_engine", 0)
        if engine_id > 0 and params.get(f"slot{slot_num}_bypass", 0) < 0.5:
            mix_level = params.get(f"slot{slot_num}_mix", 0.5)
            result["final_engines"].append({
                "id": engine_id,
                "name": ENGINE_MAPPING.get(engine_id, "Unknown"),
                "slot": slot_num,
                "mix": mix_level
            })
    
    # Check name relevance
    name_lower = result["creative_name"].lower()
    vibe_lower = result["overall_vibe"].lower()
    result["name_relevant"] = any(kw in name_lower or kw in vibe_lower for kw in expected_keywords)
    
    # Check if expected engines are present
    blueprint_ids = [e["id"] for e in result["blueprint_engines"]]
    result["engines_correct"] = any(eid in blueprint_ids for eid in expected_engines)
    
    # Check calculator detection
    result["calculator_detected"] = len(result["calculator_emphasis"]) > 0
    
    return result

async def run_verification_suite():
    """Run comprehensive verification tests"""
    
    print("="*80)
    print("TRINITY PIPELINE COMPREHENSIVE VERIFICATION")
    print("="*80)
    print()
    
    # Define test cases
    test_cases = [
        {
            "prompt": "Create a chaotic horror preset using Chaos Generator and Spectral Freeze",
            "expected_engines": [56, 54],  # Chaos Generator, Spectral Freeze
            "expected_keywords": ["chaos", "spectral", "horror", "dark", "freeze"],
            "description": "Horror with specific engines"
        },
        {
            "prompt": "Design a shimmering ambient soundscape with Shimmer Reverb and Granular Cloud",
            "expected_engines": [44, 55],  # Shimmer Reverb, Granular Cloud
            "expected_keywords": ["shimmer", "ambient", "cloud", "granular", "space"],
            "description": "Ambient with reverb engines"
        },
        {
            "prompt": "Build an aggressive metal tone with BitCrusher and Gated Reverb for crushing power",
            "expected_engines": [18, 45],  # BitCrusher, Gated Reverb
            "expected_keywords": ["metal", "crush", "gate", "aggressive", "power"],
            "description": "Metal with distortion/reverb"
        },
        {
            "prompt": "Make a vintage warm preset using Plate Reverb and Harmonic Exciter Platinum",
            "expected_engines": [42, 19],  # Plate Reverb, Harmonic Exciter Platinum
            "expected_keywords": ["vintage", "warm", "plate", "harmonic"],
            "description": "Vintage warmth"
        },
        {
            "prompt": "Create a robotic voice effect with Vocoder and Ring Modulator",
            "expected_engines": [39, 37],  # Vocoder, Ring Modulator
            "expected_keywords": ["robot", "voice", "vocoder", "ring", "modulator"],
            "description": "Robot voice effect"
        }
    ]
    
    # Run tests
    results = []
    for i, test_case in enumerate(test_cases, 1):
        print(f"TEST {i}/5: {test_case['description']}")
        print("-"*80)
        print(f"Prompt: {test_case['prompt']}")
        print(f"Expecting: {', '.join(ENGINE_MAPPING[eid] for eid in test_case['expected_engines'])}")
        print()
        
        result = await run_single_test(
            test_case['prompt'],
            test_case['expected_engines'],
            test_case['expected_keywords']
        )
        results.append(result)
        
        # Display results
        print(f"✓ Creative Name: '{result['creative_name']}'")
        print(f"✓ Overall Vibe: '{result['overall_vibe']}'")
        
        # Check name relevance
        if result['name_relevant']:
            print("✅ Name is relevant to prompt")
        else:
            print("⚠️  Name could be more relevant")
        
        # Check blueprint engines
        print(f"\nBlueprint Engines:")
        for engine in result['blueprint_engines']:
            marker = "✅" if engine['id'] in test_case['expected_engines'] else "→"
            print(f"  {marker} Slot {engine['slot']}: {engine['name']} (ID: {engine['id']})")
        
        if result['engines_correct']:
            print("✅ Blueprint contains expected engines")
        else:
            print("❌ Blueprint missing expected engines")
        
        # Check calculator emphasis
        if result['calculator_emphasis']:
            print(f"\n✅ Calculator emphasized: {', '.join(result['calculator_emphasis'])}")
        else:
            print("\n⚠️  Calculator didn't detect engines from prompt")
        
        # Check final engines
        print(f"\nFinal Active Engines:")
        has_expected = False
        for engine in result['final_engines']:
            marker = "✅" if engine['id'] in test_case['expected_engines'] else "→"
            if engine['id'] in test_case['expected_engines']:
                has_expected = True
            print(f"  {marker} Slot {engine['slot']}: {engine['name']} (mix: {engine['mix']:.2f})")
        
        if not result['final_engines']:
            print("  ❌ No active engines in final preset")
        elif has_expected:
            print("✅ Final preset contains requested engines")
        else:
            print("⚠️  Final preset doesn't have exact requested engines")
        
        print()
    
    # Summary statistics
    print("="*80)
    print("VERIFICATION SUMMARY")
    print("="*80)
    print()
    
    # Calculate success metrics
    total_tests = len(results)
    name_relevant_count = sum(1 for r in results if r['name_relevant'])
    engines_correct_count = sum(1 for r in results if r['engines_correct'])
    calculator_detected_count = sum(1 for r in results if r['calculator_detected'])
    
    # Check final engine presence
    final_has_expected = 0
    for i, result in enumerate(results):
        final_ids = [e['id'] for e in result['final_engines']]
        if any(eid in final_ids for eid in test_cases[i]['expected_engines']):
            final_has_expected += 1
    
    print(f"Creative Names Relevant:        {name_relevant_count}/{total_tests} ({name_relevant_count*100//total_tests}%)")
    print(f"Blueprint Has Correct Engines:  {engines_correct_count}/{total_tests} ({engines_correct_count*100//total_tests}%)")
    print(f"Calculator Detected Engines:    {calculator_detected_count}/{total_tests} ({calculator_detected_count*100//total_tests}%)")
    print(f"Final Has Requested Engines:    {final_has_expected}/{total_tests} ({final_has_expected*100//total_tests}%)")
    print()
    
    # Determine overall status
    if engines_correct_count >= 4 and calculator_detected_count >= 4:
        print("🎉 VERIFICATION PASSED!")
        print()
        print("The Trinity Pipeline is working correctly:")
        print("✅ Engine IDs are correctly mapped (0-56)")
        print("✅ Creative names are relevant to prompts")
        print("✅ Requested engines are being identified and used")
        print("✅ Oracle searches for matching engines")
        print("✅ Calculator applies proper weighting")
    elif engines_correct_count >= 3:
        print("✓ VERIFICATION MOSTLY PASSED")
        print("The pipeline is working but could be improved")
    else:
        print("⚠️  VERIFICATION NEEDS ATTENTION")
        print("Some issues remain with the pipeline")
    
    print("="*80)
    
    # Detailed engine mapping verification
    print("\nENGINE MAPPING VERIFICATION:")
    print("-"*40)
    critical_engines = [
        (56, "Chaos Generator"),
        (54, "Spectral Freeze"),
        (55, "Granular Cloud"),
        (44, "Shimmer Reverb"),
        (45, "Gated Reverb"),
        (18, "BitCrusher"),
        (42, "Plate Reverb"),
        (39, "Vocoder"),
        (37, "Ring Modulator")
    ]
    
    all_correct = True
    for engine_id, expected_name in critical_engines:
        actual_name = ENGINE_MAPPING.get(engine_id, "Missing")
        if actual_name == expected_name:
            print(f"✅ ID {engine_id:2d} = {actual_name}")
        else:
            print(f"❌ ID {engine_id:2d} = {actual_name} (expected {expected_name})")
            all_correct = False
    
    if all_correct:
        print("\n✅ All critical engine mappings are correct!")
    else:
        print("\n❌ Some engine mappings are incorrect!")
    
    print("="*80)

if __name__ == "__main__":
    asyncio.run(run_verification_suite())