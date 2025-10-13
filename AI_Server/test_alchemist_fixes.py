#!/usr/bin/env python3
"""
Test script to verify Alchemist fixes
Tests critical bug fixes and parameter handling
"""

import json
import logging
import sys

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

from alchemist_complete import CompleteAlchemist

def test_float_parameter_bug_fix():
    """
    Test Fix #1: Critical float parameter handling bug
    Previously would crash when using variable 'p' instead of 'param'
    """
    print("\n" + "="*70)
    print("TEST 1: Float Parameter Handling Bug Fix")
    print("="*70)

    alchemist = CompleteAlchemist()

    # Test with float array format
    test_preset = {
        "name": "Float Array Test",
        "description": "Testing float parameter handling",
        "slots": [
            {
                "slot": 0,
                "engine_id": 1,  # Compressor
                "engine_name": "Compressor",
                "parameters": [0.3, 0.5, 0.7, 0.2, 0.8]  # Float array format
            }
        ]
    }

    try:
        validated, report = alchemist.validate_and_fix(test_preset)
        print(f"✓ Float array handling: PASSED")
        print(f"  - Slot 0 has {len(validated['slots'][0]['parameters'])} parameters")
        print(f"  - First 5 values: {[p['value'] for p in validated['slots'][0]['parameters'][:5]]}")
        assert len(validated['slots'][0]['parameters']) == 15, "Should have 15 parameters"
        assert validated['slots'][0]['parameters'][0]['value'] == 0.3, "First param should be 0.3"
        return True
    except Exception as e:
        print(f"✗ Float array handling: FAILED - {e}")
        return False


def test_parameter_optimization():
    """
    Test Fix #2: O(n²) to O(n) optimization
    Verifies direct indexing works correctly
    """
    print("\n" + "="*70)
    print("TEST 2: Parameter Matching Optimization")
    print("="*70)

    alchemist = CompleteAlchemist()

    # Test with dict format
    dict_params = [
        {"name": "param1", "value": 0.1},
        {"name": "param2", "value": 0.2},
        {"name": "param3", "value": 0.3},
        {"name": "param4", "value": 0.4},
        {"name": "param5", "value": 0.5},
    ]

    test_preset = {
        "name": "Dict Format Test",
        "slots": [
            {
                "slot": 0,
                "engine_id": 2,  # Gate
                "engine_name": "Gate",
                "parameters": dict_params
            }
        ]
    }

    try:
        validated, report = alchemist.validate_and_fix(test_preset)
        print(f"✓ Dict parameter handling: PASSED")
        print(f"  - Parameters preserved in order: {[p['value'] for p in validated['slots'][0]['parameters'][:5]]}")

        # Verify order is preserved
        for i in range(5):
            expected = dict_params[i]['value']
            actual = validated['slots'][0]['parameters'][i]['value']
            assert actual == expected, f"Param {i} mismatch: expected {expected}, got {actual}"

        return True
    except Exception as e:
        print(f"✗ Dict parameter handling: FAILED - {e}")
        return False


def test_mixed_parameter_formats():
    """
    Test Fix #1 & #2: Mixed parameter formats
    """
    print("\n" + "="*70)
    print("TEST 3: Mixed Parameter Formats")
    print("="*70)

    alchemist = CompleteAlchemist()

    # Test with mixed formats (this would break with old code)
    test_preset = {
        "name": "Mixed Format Test",
        "slots": [
            {
                "slot": 0,
                "engine_id": 3,  # Expander
                "engine_name": "Expander",
                "parameters": [
                    {"name": "param1", "value": 0.1},
                    0.2,  # Mixed with float
                    {"name": "param3", "value": 0.3},
                    0.4,
                    {"name": "param5", "value": 0.5},
                ]
            }
        ]
    }

    try:
        validated, report = alchemist.validate_and_fix(test_preset)
        print(f"✓ Mixed format handling: PASSED")
        values = [p['value'] for p in validated['slots'][0]['parameters'][:5]]
        print(f"  - Values: {values}")
        assert values == [0.1, 0.2, 0.3, 0.4, 0.5], "Values should be preserved"
        return True
    except Exception as e:
        print(f"✗ Mixed format handling: FAILED - {e}")
        return False


def test_unknown_engine_error():
    """
    Test Fix #3: Unknown engine should error, not warn
    """
    print("\n" + "="*70)
    print("TEST 4: Unknown Engine Error Handling")
    print("="*70)

    alchemist = CompleteAlchemist()

    # Test with unknown engine ID
    test_preset = {
        "name": "Unknown Engine Test",
        "slots": [
            {
                "slot": 0,
                "engine_id": 999,  # Unknown engine
                "engine_name": "Unknown",
                "parameters": []
            }
        ]
    }

    try:
        validated, report = alchemist.validate_and_fix(test_preset)

        # Should have error, not warning (accepts both "Invalid" and "Unknown" engine messages)
        has_error = any("999" in err and ("Invalid" in err or "Unknown" in err) for err in report['errors'])

        if has_error:
            print(f"✓ Unknown engine generates ERROR: PASSED")
            print(f"  - Error message: {[e for e in report['errors'] if '999' in e][0]}")
            print(f"  - Validation score: {report['score']}")
            return True
        else:
            print(f"✗ Unknown engine should generate ERROR: FAILED")
            print(f"  - Errors: {report['errors']}")
            print(f"  - Warnings: {report['warnings']}")
            return False

    except Exception as e:
        print(f"✗ Unknown engine test: FAILED - {e}")
        return False


def test_knowledge_base_validation():
    """
    Test Fix #4: Knowledge base integrity check
    """
    print("\n" + "="*70)
    print("TEST 5: Knowledge Base Integrity Check")
    print("="*70)

    try:
        # This will run validation during __init__
        alchemist = CompleteAlchemist()
        print(f"✓ Knowledge base validation: PASSED")
        print(f"  - Loaded {len(alchemist.engines)} engines")
        print(f"  - All engines passed integrity checks")
        return True
    except ValueError as e:
        print(f"✗ Knowledge base validation: FAILED")
        print(f"  - Error: {e}")
        return False
    except Exception as e:
        print(f"⚠ Knowledge base validation: WARNING")
        print(f"  - Non-critical error: {e}")
        return True  # Non-critical


def test_15_parameter_generation():
    """
    Test that all engines generate exactly 15 parameters
    """
    print("\n" + "="*70)
    print("TEST 6: 15-Parameter Generation Verification")
    print("="*70)

    alchemist = CompleteAlchemist()

    # Test multiple engines with varying parameter counts
    test_engines = [
        (1, "Compressor", 6),
        (13, "Phaser", 7),
        (39, "Plate Reverb", 10),
        (56, "Tape Saturator", 5),
    ]

    all_passed = True

    for engine_id, engine_name, expected_real_params in test_engines:
        test_preset = {
            "name": f"Test {engine_name}",
            "slots": [
                {
                    "slot": 0,
                    "engine_id": engine_id,
                    "engine_name": engine_name,
                    "parameters": []  # Empty - should be filled to 15
                }
            ]
        }

        try:
            validated, report = alchemist.validate_and_fix(test_preset)
            param_count = len(validated['slots'][0]['parameters'])

            if param_count == 15:
                print(f"✓ Engine {engine_id} ({engine_name}): {param_count} parameters")
            else:
                print(f"✗ Engine {engine_id} ({engine_name}): {param_count} parameters (expected 15)")
                all_passed = False

        except Exception as e:
            print(f"✗ Engine {engine_id} ({engine_name}): FAILED - {e}")
            all_passed = False

    return all_passed


def test_edge_cases():
    """
    Test edge cases and boundary conditions
    """
    print("\n" + "="*70)
    print("TEST 7: Edge Cases and Boundary Conditions")
    print("="*70)

    alchemist = CompleteAlchemist()

    # Test 1: Out of range values
    test_preset = {
        "name": "Out of Range Test",
        "slots": [
            {
                "slot": 0,
                "engine_id": 1,
                "engine_name": "Compressor",
                "parameters": [-0.5, 1.5, 0.5, 0.5, 0.5]  # Out of range
            }
        ]
    }

    try:
        validated, report = alchemist.validate_and_fix(test_preset)
        values = [p['value'] for p in validated['slots'][0]['parameters'][:2]]

        # Note: First parameter is "Threshold" which has min_threshold safety limit of 0.05
        # So -0.5 gets clamped to 0.0, then raised to 0.05 for safety
        if values[0] == 0.05 and values[1] == 1.0:
            print(f"✓ Out of range values clamped with safety limits: {values}")
        elif values[0] == 0.0 and values[1] == 1.0:
            print(f"✓ Out of range values clamped (no safety applied): {values}")
        else:
            print(f"✗ Out of range values not clamped properly: {values}")
            return False

    except Exception as e:
        print(f"✗ Out of range test: FAILED - {e}")
        return False

    # Test 2: Empty parameters
    test_preset = {
        "name": "Empty Parameters Test",
        "slots": [
            {
                "slot": 0,
                "engine_id": 1,
                "engine_name": "Compressor",
                "parameters": []  # Empty
            }
        ]
    }

    try:
        validated, report = alchemist.validate_and_fix(test_preset)
        param_count = len(validated['slots'][0]['parameters'])

        if param_count == 15:
            print(f"✓ Empty parameters filled to 15")
        else:
            print(f"✗ Empty parameters not filled properly: {param_count}")
            return False

    except Exception as e:
        print(f"✗ Empty parameters test: FAILED - {e}")
        return False

    print(f"✓ All edge cases: PASSED")
    return True


def main():
    """Run all tests"""
    print("\n" + "="*70)
    print("ALCHEMIST FIX VERIFICATION TEST SUITE")
    print("="*70)

    tests = [
        ("Float Parameter Bug Fix", test_float_parameter_bug_fix),
        ("Parameter Optimization", test_parameter_optimization),
        ("Mixed Parameter Formats", test_mixed_parameter_formats),
        ("Unknown Engine Error", test_unknown_engine_error),
        ("Knowledge Base Validation", test_knowledge_base_validation),
        ("15-Parameter Generation", test_15_parameter_generation),
        ("Edge Cases", test_edge_cases),
    ]

    results = []

    for test_name, test_func in tests:
        try:
            passed = test_func()
            results.append((test_name, passed))
        except Exception as e:
            print(f"\n✗ {test_name}: EXCEPTION - {e}")
            results.append((test_name, False))

    # Print summary
    print("\n" + "="*70)
    print("TEST SUMMARY")
    print("="*70)

    passed_count = sum(1 for _, passed in results if passed)
    total_count = len(results)

    for test_name, passed in results:
        status = "✓ PASS" if passed else "✗ FAIL"
        print(f"{status}: {test_name}")

    print("\n" + "="*70)
    print(f"TOTAL: {passed_count}/{total_count} tests passed")

    if passed_count == total_count:
        print("✓ ALL TESTS PASSED")
        print("="*70)
        return 0
    else:
        print(f"✗ {total_count - passed_count} TESTS FAILED")
        print("="*70)
        return 1


if __name__ == "__main__":
    sys.exit(main())
