#!/usr/bin/env python3
"""Test Calculator and Alchemist components"""

import json
import sys
sys.path.append('.')

from calculator import Calculator
from alchemist import Alchemist

def test_calculator():
    """Test Calculator nudging functionality"""
    print("=" * 60)
    print("TESTING CALCULATOR COMPONENT")
    print("=" * 60)
    
    calc = Calculator()
    
    # Test preset with parameters
    test_preset = {
        "name": "Test Preset",
        "parameters": {
            "slot1_engine": 38,  # K-Style Overdrive
            "slot1_param1": 0.5,  # Drive
            "slot1_param2": 0.5,  # Tone
            "slot1_param3": 0.5,  # Level
            "slot2_engine": 1,   # Tape Echo
            "slot2_param1": 0.5,  # Time
            "slot2_param2": 0.5,  # Feedback
            "slot2_param3": 0.5,  # Mix
            "slot2_param4": 0.5,  # Wow
            "slot2_param5": 0.5   # Flutter
        }
    }
    
    # Test 1: Dark keyword
    print("\nTest 1: 'dark' keyword nudging")
    prompt = "Make a dark atmospheric preset"
    blueprint = {"slots": [{"slot": 1, "engine_id": 38}]}
    
    nudged = calc.apply_nudges(test_preset.copy(), prompt, blueprint)
    
    print(f"Original slot1_param2 (tone): {test_preset['parameters']['slot1_param2']}")
    print(f"Nudged slot1_param2 (tone): {nudged['parameters']['slot1_param2']}")
    print(f"Change: {nudged['parameters']['slot1_param2'] - test_preset['parameters']['slot1_param2']}")
    print(f"Nudged parameters: {nudged.get('calculator_nudges', [])}")
    
    # Test 2: Aggressive keyword
    print("\n\nTest 2: 'aggressive' keyword nudging")
    prompt = "Create an aggressive metal tone"
    
    nudged2 = calc.apply_nudges(test_preset.copy(), prompt, blueprint)
    
    print(f"Original slot1_param1 (drive): {test_preset['parameters']['slot1_param1']}")
    print(f"Nudged slot1_param1 (drive): {nudged2['parameters']['slot1_param1']}")
    print(f"Change: {nudged2['parameters']['slot1_param1'] - test_preset['parameters']['slot1_param1']}")
    print(f"Nudged parameters: {nudged2.get('calculator_nudges', [])}")
    
    # Test 3: Multiple keywords
    print("\n\nTest 3: Multiple keywords ('vintage warm')")
    prompt = "Give me a vintage warm tape echo sound"
    
    nudged3 = calc.apply_nudges(test_preset.copy(), prompt, blueprint)
    
    print("Nudged parameters:", nudged3.get('calculator_nudges', []))
    for param in nudged3.get('calculator_nudges', []):
        original = test_preset['parameters'].get(param, 0.5)
        nudged_val = nudged3['parameters'].get(param, 0.5)
        print(f"  {param}: {original} -> {nudged_val} (change: {nudged_val - original})")
    
    # Test 4: Clamping
    print("\n\nTest 4: Parameter clamping test")
    extreme_preset = test_preset.copy()
    extreme_preset['parameters']['slot1_param1'] = 0.95  # Near max
    
    nudged4 = calc.apply_nudges(extreme_preset, "aggressive", blueprint)
    print(f"Original slot1_param1: {extreme_preset['parameters']['slot1_param1']}")
    print(f"Nudged slot1_param1: {nudged4['parameters']['slot1_param1']}")
    print("Should be clamped to 1.0")
    
    return calc

def test_alchemist():
    """Test Alchemist validation functionality"""
    print("\n\n" + "=" * 60)
    print("TESTING ALCHEMIST COMPONENT")
    print("=" * 60)
    
    alch = Alchemist()
    
    # Test 1: Basic validation
    print("\nTest 1: Basic parameter validation")
    test_preset = {
        "name": "Test",
        "vibe": "aggressive",
        "parameters": {
            "slot1_engine": 41,  # Chaos Generator
            "slot1_param1": 1.5,  # Out of range!
            "slot1_param2": -0.2,  # Negative!
            "slot1_param3": 0.7,   # Valid
            "slot2_engine": 1,     # Tape Echo
            "slot2_param2": 1.2,   # Feedback too high!
            "slot1_bypass": 0.0,
            "slot2_bypass": 0.0,
            "slot1_mix": 0.9,
            "slot2_mix": 0.9
        }
    }
    
    finalized = alch.finalize_preset(test_preset)
    
    print("Parameter validation results:")
    print(f"  slot1_param1 (was 1.5): {finalized['parameters']['slot1_param1']} (should be 1.0)")
    print(f"  slot1_param2 (was -0.2): {finalized['parameters']['slot1_param2']} (should be 0.0)")
    print(f"  slot2_param2 (was 1.2): {finalized['parameters']['slot2_param2']} (should be clamped)")
    
    # Test 2: Safety checks - total gain
    print("\n\nTest 2: Safety check - total gain limiting")
    high_gain_preset = {
        "parameters": {
            "slot1_mix": 0.9,
            "slot2_mix": 0.9,
            "slot3_mix": 0.8,
            "slot1_bypass": 0.0,
            "slot2_bypass": 0.0,
            "slot3_bypass": 0.0
        }
    }
    
    finalized2 = alch.finalize_preset(high_gain_preset)
    
    total_gain = 0
    for i in range(1, 7):
        if finalized2['parameters'].get(f'slot{i}_bypass', 1.0) < 0.5:
            total_gain += finalized2['parameters'].get(f'slot{i}_mix', 0)
    
    print(f"Original total gain: 2.6 (0.9 + 0.9 + 0.8)")
    print(f"Finalized total gain: {total_gain:.2f}")
    print(f"Should be limited to: {alch.safety_limits['max_total_gain']}")
    
    # Test 3: Complete structure
    print("\n\nTest 3: Ensuring complete structure")
    incomplete_preset = {
        "parameters": {
            "slot1_engine": 41,
            "slot1_param1": 0.8
            # Missing many parameters!
        }
    }
    
    finalized3 = alch.finalize_preset(incomplete_preset)
    
    # Count parameters
    param_count = len([k for k in finalized3['parameters'].keys() if k.startswith('slot')])
    print(f"Total slot parameters: {param_count}")
    print("Should have 78 parameters (6 slots × 13 params each)")
    
    # Check master parameters
    print(f"Master input: {finalized3['parameters'].get('master_input', 'MISSING')}")
    print(f"Master output: {finalized3['parameters'].get('master_output', 'MISSING')}")
    print(f"Master mix: {finalized3['parameters'].get('master_mix', 'MISSING')}")
    
    # Test 4: Name generation
    print("\n\nTest 4: Creative name generation")
    vibes = ["aggressive", "warm", "spacious", "experimental", "vintage"]
    
    for vibe in vibes:
        preset = {"vibe": vibe, "parameters": {}}
        name = alch.generate_preset_name(preset)
        print(f"  Vibe '{vibe}' -> Name: '{name}'")
    
    # Test 5: Validation warnings
    print("\n\nTest 5: Validation warnings")
    warning_preset = {
        "parameters": {
            "slot1_param1": 0.95,  # Very high
            "slot2_param3": 0.02,  # Very low
            "slot3_param2": 0.98   # Very high
        }
    }
    
    finalized5 = alch.finalize_preset(warning_preset)
    warnings = finalized5.get('validation_warnings', [])
    
    print(f"Validation warnings ({len(warnings)} found):")
    for warning in warnings:
        print(f"  - {warning}")
    
    # Test 6: All slots bypassed
    print("\n\nTest 6: All slots bypassed safety check")
    all_bypassed = {
        "parameters": {
            "slot1_bypass": 1.0,
            "slot2_bypass": 1.0,
            "slot3_bypass": 1.0,
            "slot4_bypass": 1.0,
            "slot5_bypass": 1.0,
            "slot6_bypass": 1.0
        }
    }
    
    finalized6 = alch.finalize_preset(all_bypassed)
    print(f"Slot 1 bypass after safety check: {finalized6['parameters']['slot1_bypass']}")
    print("Should be 0.0 (activated)")
    
    return alch

def test_integration():
    """Test Calculator and Alchemist working together"""
    print("\n\n" + "=" * 60)
    print("TESTING CALCULATOR + ALCHEMIST INTEGRATION")
    print("=" * 60)
    
    calc = Calculator()
    alch = Alchemist()
    
    # Start with a base preset
    base_preset = {
        "name": "Base",
        "vibe": "aggressive",
        "parameters": {
            "slot1_engine": 38,  # K-Style
            "slot1_param1": 0.5,
            "slot1_param2": 0.5,
            "slot1_param3": 0.5,
            "slot1_bypass": 0.0,
            "slot1_mix": 0.7,
            "slot2_engine": 41,  # Chaos Generator
            "slot2_param1": 0.5,
            "slot2_bypass": 0.0,
            "slot2_mix": 0.6
        }
    }
    
    # Apply Calculator nudges
    prompt = "Make it dark and aggressive with vintage character"
    blueprint = {
        "slots": [
            {"slot": 1, "engine_id": 38, "character": "aggressive"},
            {"slot": 2, "engine_id": 41, "character": "warm"}
        ]
    }
    
    print("\n1. Original preset:")
    print(f"   Name: {base_preset['name']}")
    print(f"   slot1_param1: {base_preset['parameters']['slot1_param1']}")
    print(f"   slot1_param2: {base_preset['parameters']['slot1_param2']}")
    
    nudged = calc.apply_nudges(base_preset, prompt, blueprint)
    
    print("\n2. After Calculator nudging:")
    print(f"   Nudged parameters: {nudged.get('calculator_nudges', [])}")
    for param in nudged.get('calculator_nudges', []):
        if param in base_preset['parameters']:
            print(f"   {param}: {base_preset['parameters'][param]} -> {nudged['parameters'][param]}")
    
    # Apply Alchemist finalization
    finalized = alch.finalize_preset(nudged)
    
    print("\n3. After Alchemist finalization:")
    print(f"   Name: {finalized['name']}")
    print(f"   Validated: {finalized.get('alchemist_validated', False)}")
    print(f"   Warnings: {finalized.get('validation_warnings', [])}")
    print(f"   Total parameters: {len(finalized['parameters'])}")

if __name__ == "__main__":
    print("CALCULATOR AND ALCHEMIST COMPONENT TESTS")
    print("=" * 60)
    
    # Run individual tests
    calc = test_calculator()
    alch = test_alchemist()
    
    # Run integration test
    test_integration()
    
    print("\n\n" + "=" * 60)
    print("ALL TESTS COMPLETED")
    print("=" * 60)