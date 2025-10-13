#!/usr/bin/env python3
"""
Test and analyze each Trinity Pipeline component individually
"""

import asyncio
import json
from pathlib import Path
import sys

# Import all Trinity components
from cloud_bridge_enhanced import CloudBridgeEnhanced
from oracle_enhanced import OracleEnhanced
from calculator_enhanced import CalculatorEnhanced
from alchemist_improved import AlchemistImproved
from engine_extraction import extract_required_engines
from music_theory_intelligence import MusicTheoryIntelligence
from engine_mapping_authoritative import ENGINE_NAMES

def print_section(title):
    """Print a formatted section header"""
    print(f"\n{'='*80}")
    print(f"🔍 {title}")
    print('='*80)

async def test_visionary():
    """Test the Visionary (Cloud Bridge) component"""
    print_section("VISIONARY / CLOUD BRIDGE ANALYSIS")
    
    visionary = CloudBridgeEnhanced()
    
    test_prompts = [
        "Create a warm vintage guitar tone",
        "Make it sound like underwater vocals",
        "Professional mastering chain"
    ]
    
    print("\n📊 WHAT IT DOES:")
    print("1. Creative Interpretation - Translates ANY prompt (technical or poetic)")
    print("2. Naming - Generates creative, relevant preset names")
    print("3. Technical Translation - Converts poetic descriptions to technical terms")
    print("4. Character Analysis - Extracts mood, genre, instrument, intensity")
    
    print("\n🧪 TESTING VISIONARY:")
    
    for prompt in test_prompts:
        print(f"\nInput: '{prompt}'")
        result = await visionary.get_cloud_generation(prompt)
        
        if result:
            print(f"  Creative Name: '{result.get('creative_name', 'None')}'")
            print(f"  Technical Translation: '{result.get('technical_translation', 'None')}'")
            print(f"  Character Tags: {result.get('character_tags', [])}")
            
            analysis = result.get('creative_analysis', {})
            print(f"  Analysis:")
            print(f"    - Mood: {analysis.get('mood', 'None')}")
            print(f"    - Genre: {analysis.get('genre', 'None')}")
            print(f"    - Instrument: {analysis.get('instrument', 'None')}")
            print(f"    - Intensity: {analysis.get('intensity', 0)}")
            print(f"    - Space: {analysis.get('space', 0)}")
            print(f"    - Warmth: {analysis.get('warmth', 0)}")
    
    print("\n💡 WHY IT EXISTS:")
    print("- Bridges the gap between human creativity and technical requirements")
    print("- Provides consistent naming that matches user intent")
    print("- Translates metaphors ('underwater') to technical terms")
    
    return result

def test_oracle():
    """Test the Oracle component"""
    print_section("ORACLE ANALYSIS")
    
    # Initialize Oracle
    base_dir = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3")
    oracle = OracleEnhanced(
        str(base_dir / "faiss_index" / "corpus_clean.index"),
        str(base_dir / "faiss_index" / "metadata_clean.json"),
        str(base_dir / "faiss_index" / "presets_clean.json")
    )
    
    print("\n📊 WHAT IT DOES:")
    print("1. Vector Search - Uses FAISS to find similar presets from corpus")
    print("2. Similarity Matching - Compares input to 150 professional presets")
    print("3. Musical Intelligence - Understands genre/instrument relationships")
    print("4. Engine Scoring - Prioritizes presets with required engines")
    
    print(f"\n📚 CORPUS SIZE: {len(oracle.presets)} presets")
    
    print("\n🧪 TESTING ORACLE:")
    
    test_queries = [
        {
            "prompt": "warm vintage guitar",
            "required_engines": [15],  # Vintage Tube
            "genre": "rock",
            "instrument": "guitar"
        },
        {
            "prompt": "modern EDM bass",
            "required_engines": [2],  # Compressor
            "genre": "edm",
            "instrument": "bass"
        }
    ]
    
    for query in test_queries:
        print(f"\nQuery: {query['prompt']}")
        preset = oracle.find_best_preset(query)
        
        print(f"  Found Preset: {preset.get('creative_name', 'Unknown')}")
        print(f"  Corpus ID: {preset.get('corpus_id', 'None')}")
        print(f"  Similarity Score: {preset.get('similarity_score', 0):.2f}")
        
        # Show engines
        engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                engines.append(ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})"))
        print(f"  Engines: {', '.join(engines[:3])}...")
    
    print("\n💡 WHY IT EXISTS:")
    print("- Leverages professional presets as starting points")
    print("- Provides musically-appropriate combinations")
    print("- Ensures valid parameter ranges from tested presets")
    
    return preset

def test_calculator():
    """Test the Calculator component"""
    print_section("CALCULATOR ANALYSIS")
    
    calculator = CalculatorEnhanced()
    
    print("\n📊 WHAT IT DOES:")
    print("1. Parameter Nudging - Fine-tunes parameters based on prompt")
    print("2. Character Adjustment - Modifies for warm/bright/aggressive/etc")
    print("3. Engine Preservation - Maintains required engines")
    print("4. Intelligent Tweaking - Makes subtle improvements")
    
    print("\n🧪 TESTING CALCULATOR:")
    
    # Create a test preset
    test_preset = {
        "name": "Test Preset",
        "slot1_engine": 15,  # Vintage Tube
        "slot1_param0": 0.5,
        "slot1_param1": 0.5,
        "slot2_engine": 39,  # Plate Reverb
        "slot2_param0": 0.5,
        "slot2_param1": 0.5,
        "slot2_param5": 0.3,  # Mix
    }
    
    test_prompts = [
        ("Make it warmer", {"warmth": 0.8}),
        ("More aggressive", {"intensity": 0.9}),
        ("Subtle and clean", {"intensity": 0.2})
    ]
    
    for prompt, expected in test_prompts:
        print(f"\nPrompt: '{prompt}'")
        nudged = calculator.apply_nudges(test_preset.copy(), prompt, expected)
        
        # Check what changed
        changes = []
        for key in nudged:
            if key.startswith("slot") and "param" in key:
                if test_preset.get(key, 0) != nudged.get(key, 0):
                    changes.append(f"{key}: {test_preset.get(key, 0):.2f} → {nudged.get(key, 0):.2f}")
        
        if changes:
            print("  Parameter Changes:")
            for change in changes[:3]:
                print(f"    - {change}")
        else:
            print("  No parameter changes")
    
    print("\n💡 WHY IT EXISTS:")
    print("- Fine-tunes Oracle's preset to match specific request")
    print("- Applies musical intelligence to parameter adjustments")
    print("- Ensures smooth transitions between effects")
    
    return nudged

def test_alchemist():
    """Test the Alchemist component"""
    print_section("ALCHEMIST ANALYSIS")
    
    alchemist = AlchemistImproved()
    
    print("\n📊 WHAT IT DOES:")
    print("1. Signal Chain Optimization - Reorders effects for best flow")
    print("2. Parameter Validation - Ensures safe ranges, prevents feedback")
    print("3. Safety Corrections - Fixes problematic parameter combinations")
    print("4. Technical Polish - Final engineering pass")
    
    print("\n🧪 TESTING ALCHEMIST:")
    
    # Test preset with bad signal chain order
    test_preset = {
        "name": "Test",
        "slot1_engine": 39,  # Reverb (should be later)
        "slot1_param0": 0.99,  # Dangerous feedback
        "slot2_engine": 2,   # Compressor (should be earlier)
        "slot2_param0": 0.5,
        "slot3_engine": 7,   # EQ (should be early)
        "slot3_param0": 0.5,
        "slot4_engine": 20,  # Distortion
        "slot4_param0": 0.95,  # Very high
    }
    
    print("\nBefore Alchemist:")
    for slot in range(1, 5):
        engine_id = test_preset.get(f"slot{slot}_engine", 0)
        if engine_id > 0:
            print(f"  Slot {slot}: {ENGINE_NAMES.get(engine_id, 'Unknown')}")
    
    # Process through Alchemist
    finalized = alchemist.finalize_preset(test_preset.copy(), "Test prompt")
    
    print("\nAfter Alchemist:")
    for slot in range(1, 7):
        engine_id = finalized.get(f"slot{slot}_engine", 0)
        if engine_id > 0:
            param0 = finalized.get(f"slot{slot}_param0", 0)
            print(f"  Slot {slot}: {ENGINE_NAMES.get(engine_id, 'Unknown')} (param0={param0:.2f})")
    
    print(f"\nSignal Flow: {finalized.get('signal_flow', 'None')}")
    
    # Check safety
    is_safe, warnings = alchemist.signal_intelligence.validate_parameters(finalized)
    print(f"\nSafety Check: {'✅ SAFE' if is_safe else '⚠️ WARNINGS'}")
    if warnings:
        for w in warnings[:3]:
            print(f"  - {w}")
    
    print("\n💡 WHY IT EXISTS:")
    print("- Ensures professional audio engineering standards")
    print("- Prevents dangerous parameter combinations")
    print("- Optimizes signal flow for best sound quality")
    
    return finalized

def test_engine_extraction():
    """Test engine extraction"""
    print_section("ENGINE EXTRACTION ANALYSIS")
    
    print("\n📊 WHAT IT DOES:")
    print("1. Keyword Mapping - Maps natural language to engine IDs")
    print("2. Direct Detection - Finds explicitly mentioned engines")
    print("3. Contextual Understanding - Infers engines from descriptions")
    
    print("\n🧪 TESTING EXTRACTION:")
    
    test_prompts = [
        "I need vintage tube preamp and plate reverb",
        "Add shimmer reverb with bit crusher",
        "Classic compressor with noise gate",
        "Warm vintage sound",  # Indirect
        "Aggressive metal tone"  # Indirect
    ]
    
    for prompt in test_prompts:
        engines = extract_required_engines(prompt)
        print(f"\nPrompt: '{prompt}'")
        if engines:
            engine_names = [ENGINE_NAMES.get(e, f"ID{e}") for e in engines]
            print(f"  Extracted: {', '.join(engine_names)}")
        else:
            print(f"  Extracted: None")
    
    print("\n💡 WHY IT EXISTS:")
    print("- Ensures user-requested engines are included")
    print("- Provides hard requirements for the pipeline")
    print("- Translates user language to technical IDs")

def analyze_data_flow():
    """Analyze data flow through the pipeline"""
    print_section("DATA FLOW ANALYSIS")
    
    print("\n📊 TRINITY PIPELINE FLOW:")
    print("""
    USER PROMPT
         ↓
    ┌─────────────┐
    │  VISIONARY  │ → Creative Name, Technical Translation, Character Tags
    └─────────────┘
         ↓
    ┌─────────────┐
    │   ORACLE    │ → Best matching preset from corpus (150 presets)
    └─────────────┘
         ↓
    ┌─────────────┐
    │ CALCULATOR  │ → Fine-tuned parameters based on prompt character
    └─────────────┘
         ↓
    ┌─────────────┐
    │  ALCHEMIST  │ → Signal chain optimization, safety validation
    └─────────────┘
         ↓
    FINAL PRESET
    """)
    
    print("\n📦 DATA STRUCTURE EVOLUTION:")
    
    print("\n1. VISIONARY OUTPUT:")
    print("""   {
      "creative_name": "Warm Vintage Guitar",
      "technical_translation": "tube saturation, warmth, vintage character",
      "character_tags": ["warm", "vintage", "smooth"],
      "creative_analysis": {
        "mood": "warm",
        "genre": "rock",
        "instrument": "guitar",
        "intensity": 0.4,
        "space": 0.3,
        "warmth": 0.8
      }
   }""")
    
    print("\n2. ORACLE OUTPUT:")
    print("""   {
      "slot1_engine": 15,  # Vintage Tube Preamp
      "slot1_param0": 0.6,
      "slot2_engine": 39,  # Plate Reverb  
      "slot2_param0": 0.4,
      "corpus_id": "GC_0045",
      "similarity_score": 0.85
   }""")
    
    print("\n3. CALCULATOR OUTPUT:")
    print("""   {
      ...oracle_preset,
      "slot1_param0": 0.7,  # Increased warmth
      "slot2_param5": 0.25, # Adjusted mix
      # Parameters nudged based on character
   }""")
    
    print("\n4. ALCHEMIST OUTPUT:")
    print("""   {
      ...calculator_preset,
      "name": "Warm Vintage Guitar",  # From Visionary
      "signal_flow": "Input → Tube → Reverb → Output",
      "master_input": 0.7,
      "master_output": 0.7,
      "master_mix": 1.0
      # Optimized and validated
   }""")

async def main():
    """Run all component tests"""
    print("🎼 TRINITY PIPELINE TECHNICAL ANALYSIS")
    print("=" * 80)
    
    # Test each component
    visionary_result = await test_visionary()
    oracle_result = test_oracle()
    calculator_result = test_calculator()
    alchemist_result = test_alchemist()
    test_engine_extraction()
    analyze_data_flow()
    
    # Summary
    print_section("SUMMARY: WHY TRINITY?")
    
    print("""
The Trinity Pipeline separates concerns into specialized components:

1. VISIONARY (Creative AI)
   - Human-like understanding of creative descriptions
   - Consistent, relevant naming
   - Bridges creative and technical worlds

2. ORACLE (Knowledge Base)
   - 150 professional presets as foundation
   - Vector similarity search (FAISS)
   - Musical intelligence and genre awareness

3. CALCULATOR (Fine Tuning)
   - Prompt-specific adjustments
   - Character-based parameter nudging
   - Preserves user requirements

4. ALCHEMIST (Engineering)
   - Professional audio standards
   - Signal chain optimization
   - Safety validation

BENEFITS:
✅ Modular - Each component can be improved independently
✅ Reliable - Oracle provides known-good starting points
✅ Creative - Visionary understands poetic descriptions
✅ Safe - Alchemist prevents audio problems
✅ Flexible - Can handle technical or creative prompts

FLOW: Creativity → Knowledge → Customization → Engineering
    """)

if __name__ == "__main__":
    asyncio.run(main())