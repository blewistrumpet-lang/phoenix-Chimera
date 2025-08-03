#!/usr/bin/env python3
"""
Test the engine mapping system thoroughly
This is CRITICAL for the plugin to work correctly
"""

import sys
from engine_mapping import (
    ENGINE_ID_TO_CHOICE, 
    CHOICE_TO_ENGINE_ID,
    engine_id_to_choice_index,
    choice_index_to_engine_id,
    convert_preset_engine_ids
)

def test_mapping_completeness():
    """Test that all valid engine IDs have mappings"""
    print("\n" + "="*80)
    print("TESTING ENGINE MAPPING COMPLETENESS")
    print("="*80)
    
    # All valid engine IDs (excluding commented out 10, 13, 37)
    valid_engine_ids = [
        -1,  # Bypass
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        11, 12, 14, 15, 16, 17, 18, 19, 20,
        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        51, 52, 53, 54, 55
    ]
    
    missing = []
    for engine_id in valid_engine_ids:
        if engine_id not in ENGINE_ID_TO_CHOICE:
            missing.append(engine_id)
    
    if missing:
        print(f"❌ MISSING ENGINE IDS: {missing}")
        return False
    else:
        print(f"✅ All {len(valid_engine_ids)} valid engine IDs have mappings")
        return True

def test_bidirectional_mapping():
    """Test that mappings are bidirectional"""
    print("\n" + "="*80)
    print("TESTING BIDIRECTIONAL MAPPING")
    print("="*80)
    
    errors = []
    for engine_id, choice_idx in ENGINE_ID_TO_CHOICE.items():
        reverse = CHOICE_TO_ENGINE_ID.get(choice_idx)
        if reverse != engine_id:
            errors.append(f"Engine {engine_id} -> Choice {choice_idx} -> Engine {reverse}")
    
    if errors:
        print("❌ BIDIRECTIONAL ERRORS:")
        for error in errors:
            print(f"  {error}")
        return False
    else:
        print(f"✅ All mappings are correctly bidirectional")
        return True

def test_critical_engines():
    """Test specific critical engine mappings"""
    print("\n" + "="*80)
    print("TESTING CRITICAL ENGINE MAPPINGS")
    print("="*80)
    
    critical_tests = [
        # (engine_id, expected_choice_index, engine_name)
        (-1, 0, "Bypass"),
        (0, 48, "Vintage Tube"),
        (1, 2, "Tape Echo"),
        (38, 1, "K-Style Overdrive"),
        (41, 44, "Chaos Generator"),
        (47, 41, "Noise Gate"),
        (54, 52, "Dynamic EQ"),
        (27, 39, "Parametric EQ"),
        (11, 17, "Stereo Chorus"),
    ]
    
    passed = True
    for engine_id, expected_choice, name in critical_tests:
        actual_choice = engine_id_to_choice_index(engine_id)
        if actual_choice != expected_choice:
            print(f"❌ {name} (ID {engine_id}): Expected choice {expected_choice}, got {actual_choice}")
            passed = False
        else:
            print(f"✅ {name} (ID {engine_id}) -> Choice {expected_choice}")
    
    return passed

def test_preset_conversion():
    """Test converting a full preset"""
    print("\n" + "="*80)
    print("TESTING PRESET CONVERSION")
    print("="*80)
    
    # Sample preset with engine IDs as returned by Oracle/Alchemist
    test_preset = {
        "name": "Test Preset",
        "parameters": {
            "slot1_engine": 41,  # Chaos Generator
            "slot2_engine": 40,  # Buffer Repeat
            "slot3_engine": 52,  # Resonant Chorus
            "slot4_engine": 0,   # Vintage Tube
            "slot5_engine": -1,  # Bypass
            "slot6_engine": 54,  # Dynamic EQ
            "slot1_bypass": 0.0,
            "slot2_bypass": 0.0,
            "slot3_bypass": 0.0,
            "slot4_bypass": 0.0,
            "slot5_bypass": 1.0,
            "slot6_bypass": 0.0,
        }
    }
    
    converted = convert_preset_engine_ids(test_preset)
    
    expected_conversions = [
        (1, 41, 44, "Chaos Generator"),
        (2, 40, 45, "Buffer Repeat"),
        (3, 52, 50, "Resonant Chorus"),
        (4, 0, 48, "Vintage Tube"),
        (5, -1, 0, "Bypass"),
        (6, 54, 52, "Dynamic EQ"),
    ]
    
    print("Original -> Converted:")
    passed = True
    for slot, orig_id, expected_choice, name in expected_conversions:
        actual_choice = converted["parameters"][f"slot{slot}_engine"]
        status = "✅" if actual_choice == expected_choice else "❌"
        print(f"  Slot {slot}: {orig_id:3d} -> {actual_choice:3d} {status} ({name})")
        if actual_choice != expected_choice:
            print(f"    ERROR: Expected {expected_choice}")
            passed = False
    
    return passed

def test_problematic_mappings():
    """Test the mappings that were causing issues"""
    print("\n" + "="*80)
    print("TESTING PROBLEMATIC MAPPINGS FROM BUG REPORTS")
    print("="*80)
    
    # These were the engines being returned by the AI server
    # and causing wrong engines to load
    problematic = [
        (41, "Should be Chaos Generator, not Noise Gate"),
        (40, "Should be Buffer Repeat"),
        (52, "Should be Resonant Chorus"),
    ]
    
    for engine_id, description in problematic:
        choice_idx = engine_id_to_choice_index(engine_id)
        reverse_id = choice_index_to_engine_id(choice_idx)
        
        print(f"Engine ID {engine_id} ({description}):")
        print(f"  -> Choice Index: {choice_idx}")
        print(f"  -> Reverse to ID: {reverse_id}")
        
        if reverse_id != engine_id:
            print(f"  ❌ ERROR: Mapping is not bidirectional!")
            return False
        else:
            print(f"  ✅ Mapping is correct")
    
    return True

def main():
    """Run all tests"""
    print("\n" + "#"*80)
    print("# ENGINE MAPPING SYSTEM TEST SUITE")
    print("# This is CRITICAL for plugin functionality")
    print("#"*80)
    
    tests = [
        ("Completeness", test_mapping_completeness),
        ("Bidirectional", test_bidirectional_mapping),
        ("Critical Engines", test_critical_engines),
        ("Preset Conversion", test_preset_conversion),
        ("Problematic Mappings", test_problematic_mappings),
    ]
    
    results = []
    for name, test_func in tests:
        try:
            passed = test_func()
            results.append((name, passed))
        except Exception as e:
            print(f"\n❌ Test '{name}' CRASHED: {e}")
            results.append((name, False))
    
    # Summary
    print("\n" + "="*80)
    print("TEST SUMMARY")
    print("="*80)
    
    all_passed = True
    for name, passed in results:
        status = "✅ PASSED" if passed else "❌ FAILED"
        print(f"{name:20s}: {status}")
        if not passed:
            all_passed = False
    
    print("\n" + "="*80)
    if all_passed:
        print("✅ ALL TESTS PASSED - Engine mapping system is correct!")
    else:
        print("❌ TESTS FAILED - Engine mapping system has errors!")
        print("This MUST be fixed before the plugin will work correctly!")
    print("="*80)
    
    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main())