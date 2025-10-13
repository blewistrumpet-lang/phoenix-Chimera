#!/usr/bin/env python3
"""
Test Signal Chain Intelligence Integration
"""

import json
from signal_chain_intelligence import SignalChainIntelligence
from alchemist import Alchemist
from calculator import Calculator

def test_signal_chain_integration():
    print("🧪 TESTING SIGNAL CHAIN INTEGRATION")
    print("=" * 60)
    
    # Test preset with problematic ordering
    bad_preset = {
        "name": "Test Preset",
        "slot1_engine": 42,  # Shimmer Reverb (should be last)
        "slot1_param0": 0.5,
        "slot1_param5": 0.7,  # Mix
        "slot2_engine": 15,  # Vintage Tube (should be early)
        "slot2_param1": 0.8,  # High drive
        "slot3_engine": 2,   # Classic Compressor (should be first)
        "slot3_param0": 0.4,
        "slot4_engine": 35,  # Digital Delay
        "slot4_param1": 0.9,  # Very high feedback - dangerous
        "slot5_engine": 0,
        "slot6_engine": 0
    }
    
    # Test with Alchemist
    print("\n1. Testing Alchemist Integration:")
    alchemist = Alchemist()
    finalized = alchemist.finalize_preset(bad_preset)
    
    print(f"  Original order: {[bad_preset.get(f'slot{i}_engine', 0) for i in range(1, 7)]}")
    print(f"  Optimized order: {[finalized.get(f'slot{i}_engine', 0) for i in range(1, 7)]}")
    print(f"  Signal flow: {finalized.get('signal_flow', 'Not generated')}")
    print(f"  Warnings: {finalized.get('validation_warnings', [])}")
    
    # Test with Calculator
    print("\n2. Testing Calculator Integration:")
    calculator = Calculator("nudge_rules.json")
    
    test_prompts = [
        "warm vintage vocals",
        "aggressive metal guitar",
        "ambient space pad"
    ]
    
    for prompt in test_prompts:
        suggested = calculator.suggest_engines_for_intent(prompt, {})
        print(f"  '{prompt}' suggests: {suggested}")
    
    # Test full pipeline
    print("\n3. Testing Full Pipeline:")
    prompt = "Create a warm vintage vocal sound with space"
    
    # Calculator suggests engines
    base_preset = {"name": "Base"}
    nudged = calculator.apply_nudges(base_preset, prompt, {})
    
    # Alchemist finalizes with signal chain optimization
    final = alchemist.finalize_preset(nudged)
    
    print(f"  Prompt: '{prompt}'")
    print(f"  Final engines: {[final.get(f'slot{i}_engine', 0) for i in range(1, 7)]}")
    print(f"  Signal flow: {final.get('signal_flow', 'Not generated')}")
    
    print("\n✅ Integration test complete!")

if __name__ == "__main__":
    test_signal_chain_integration()
