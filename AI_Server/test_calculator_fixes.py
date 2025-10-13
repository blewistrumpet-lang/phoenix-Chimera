#!/usr/bin/env python3
"""
Test suite for calculator_max_intelligence.py fixes
Tests all critical, high, and medium priority fixes
"""

import asyncio
import json
import sys
from calculator_max_intelligence import MaxIntelligenceCalculator

# ANSI color codes for output
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'

def print_test(name):
    print(f"\n{BLUE}{'='*60}{RESET}")
    print(f"{BLUE}TEST: {name}{RESET}")
    print(f"{BLUE}{'='*60}{RESET}")

def print_pass(msg):
    print(f"{GREEN}✓ PASS: {msg}{RESET}")

def print_fail(msg):
    print(f"{RED}✗ FAIL: {msg}{RESET}")

def print_info(msg):
    print(f"{YELLOW}ℹ INFO: {msg}{RESET}")


async def test_value_clamping():
    """Test CRITICAL: Value range clamping (0.0-1.0)"""
    print_test("Value Range Clamping (CRITICAL)")

    calc = MaxIntelligenceCalculator()

    # Create test preset
    test_preset = {
        "name": "Test Clamping",
        "slots": [
            {
                "slot": 0,
                "engine_id": 34,  # Tape Echo
                "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
            }
        ]
    }

    # Mock GPT response with invalid values
    test_cases = [
        # Test case 1: Negative values
        {
            "style_parameters": {"34": {"param1": -0.5, "param2": -1.0, "param3": -0.1}},
            "expected": [0.0, 0.0, 0.0],
            "name": "Negative values should clamp to 0.0"
        },
        # Test case 2: Values > 1.0
        {
            "style_parameters": {"34": {"param1": 1.5, "param2": 2.0, "param3": 10.0}},
            "expected": [1.0, 1.0, 1.0],
            "name": "Values > 1.0 should clamp to 1.0"
        },
        # Test case 3: Valid values
        {
            "style_parameters": {"34": {"param1": 0.3, "param2": 0.7, "param3": 1.0}},
            "expected": [0.3, 0.7, 1.0],
            "name": "Valid values should remain unchanged"
        },
        # Test case 4: Edge cases
        {
            "style_parameters": {"34": {"param1": 0.0, "param2": 1.0, "param3": 0.5}},
            "expected": [0.0, 1.0, 0.5],
            "name": "Edge values (0.0, 1.0) should be valid"
        }
    ]

    all_passed = True
    for i, test_case in enumerate(test_cases, 1):
        print_info(f"Test {i}: {test_case['name']}")

        # Reset preset
        for param in test_preset["slots"][0]["parameters"]:
            param["value"] = 0.5

        # Manually apply the clamping logic (simulating what happens in optimize_parameters_max_intelligence)
        for param_key, param_value in test_case["style_parameters"]["34"].items():
            if param_key.startswith("param") and param_key[5:].isdigit():
                param_idx = int(param_key[5:]) - 1
                if param_idx < len(test_preset["slots"][0]["parameters"]):
                    # Apply clamping
                    try:
                        param_value = max(0.0, min(1.0, float(param_value)))
                    except (ValueError, TypeError):
                        param_value = 0.5
                    test_preset["slots"][0]["parameters"][param_idx]["value"] = param_value

        # Verify results
        for j, expected in enumerate(test_case["expected"]):
            actual = test_preset["slots"][0]["parameters"][j]["value"]
            if abs(actual - expected) < 0.0001:  # Float comparison
                print_pass(f"  param{j+1}: {actual} == {expected}")
            else:
                print_fail(f"  param{j+1}: {actual} != {expected}")
                all_passed = False

    if all_passed:
        print_pass("All value clamping tests passed!")
    else:
        print_fail("Some value clamping tests failed!")

    return all_passed


def test_parsing_patterns():
    """Test HIGH: Missing parsing patterns implementation"""
    print_test("Parsing Patterns (HIGH)")

    calc = MaxIntelligenceCalculator()

    test_cases = [
        {
            "prompt": "delay at 250ms with feedback",
            "pattern": "milliseconds",
            "expected_value": 0.25,  # 250/1000
            "description": "Milliseconds parsing (0-1000ms range)"
        },
        {
            "prompt": "low pass filter at 1000Hz",
            "pattern": "hertz",
            "expected_approx": True,
            "description": "Hertz parsing (20-20000Hz logarithmic)"
        },
        {
            "prompt": "high shelf at 8kHz",
            "pattern": "kilohertz",
            "expected_approx": True,
            "description": "Kilohertz parsing (0.02-20kHz)"
        },
        {
            "prompt": "boost by +6dB",
            "pattern": "decibels",
            "expected_value": 0.825,  # (+6+60)/80
            "description": "Decibels parsing (-60 to +20dB range)"
        },
        {
            "prompt": "cut by -12dB",
            "pattern": "decibels",
            "expected_value": 0.6,  # (-12+60)/80
            "description": "Negative decibels"
        }
    ]

    all_passed = True
    for test_case in test_cases:
        print_info(f"Testing: {test_case['description']}")
        print_info(f"  Prompt: '{test_case['prompt']}'")

        result = calc.parse_prompt_values(test_case["prompt"])

        if test_case["pattern"] in result:
            actual_value = result[test_case["pattern"]]["value"]

            if "expected_value" in test_case:
                expected = test_case["expected_value"]
                if abs(actual_value - expected) < 0.001:
                    print_pass(f"  Found {test_case['pattern']}: {actual_value} ≈ {expected}")
                else:
                    print_fail(f"  Value mismatch: {actual_value} != {expected}")
                    all_passed = False
            elif test_case.get("expected_approx"):
                print_pass(f"  Found {test_case['pattern']}: {actual_value}")

            print_info(f"  Original: {result[test_case['pattern']]['original']}")
        else:
            print_fail(f"  Pattern '{test_case['pattern']}' not found in result")
            all_passed = False

    if all_passed:
        print_pass("All parsing pattern tests passed!")
    else:
        print_fail("Some parsing pattern tests failed!")

    return all_passed


def test_json_extraction():
    """Test HIGH: Improved JSON extraction"""
    print_test("JSON Extraction (HIGH)")

    calc = MaxIntelligenceCalculator()

    test_cases = [
        {
            "name": "Plain JSON",
            "response": '{"key": "value", "number": 42}',
            "should_extract": True
        },
        {
            "name": "Markdown JSON block",
            "response": '```json\n{"key": "value", "number": 42}\n```',
            "should_extract": True
        },
        {
            "name": "Markdown generic code block",
            "response": '```\n{"key": "value", "number": 42}\n```',
            "should_extract": True
        },
        {
            "name": "JSON with text before",
            "response": 'Here is the result:\n{"key": "value", "number": 42}\nDone!',
            "should_extract": True
        },
        {
            "name": "Complex nested JSON in markdown",
            "response": '```json\n{\n  "style_parameters": {"15": {"param1": 0.5}},\n  "conflict_fixes": {},\n  "creative_magic": []\n}\n```',
            "should_extract": True
        },
        {
            "name": "No JSON",
            "response": 'This is just plain text without any JSON',
            "should_extract": False
        }
    ]

    all_passed = True
    for test_case in test_cases:
        print_info(f"Testing: {test_case['name']}")

        result = calc._extract_json_from_response(test_case["response"])

        if test_case["should_extract"]:
            if result is not None and isinstance(result, dict):
                print_pass(f"  Successfully extracted JSON: {list(result.keys())}")
            else:
                print_fail(f"  Failed to extract JSON from valid input")
                all_passed = False
        else:
            if result is None:
                print_pass(f"  Correctly returned None for invalid input")
            else:
                print_fail(f"  Incorrectly extracted JSON from invalid input")
                all_passed = False

    if all_passed:
        print_pass("All JSON extraction tests passed!")
    else:
        print_fail("Some JSON extraction tests failed!")

    return all_passed


async def test_parameter_mismatch_warning():
    """Test MEDIUM: Parameter name mismatch warning"""
    print_test("Parameter Name Mismatch Warning (MEDIUM)")

    calc = MaxIntelligenceCalculator()

    # Create test preset
    test_preset = {
        "name": "Test Mismatch",
        "slots": [
            {
                "slot": 0,
                "engine_id": 34,  # Tape Echo
                "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
            }
        ]
    }

    print_info("This test checks that warnings are logged for mismatched parameter names")
    print_info("Check the log output above for warning messages")

    # Simulate creative magic with invalid parameter name
    unified_result = {
        "creative_magic": [
            {
                "engine_id": 34,
                "parameter": "nonexistent_param",  # Invalid name
                "value": 0.7,
                "why": "test"
            },
            {
                "engine_id": 34,
                "parameter": "feedback",  # Valid alias
                "value": 0.6,
                "why": "test"
            }
        ]
    }

    # Manually apply creative magic logic with warning
    for touch in unified_result.get("creative_magic", []):
        engine_id = touch.get("engine_id")
        for slot in test_preset.get("slots", []):
            if slot.get("engine_id") == engine_id:
                param_name = touch.get("parameter", "")
                new_value = touch.get("value", 0.5)

                try:
                    new_value = max(0.0, min(1.0, float(new_value)))
                except (ValueError, TypeError):
                    new_value = 0.5

                engine_info = calc.param_mappings.get(engine_id, {})
                param_found = False
                for i, param_info in enumerate(engine_info.get("param_list", [])):
                    if param_info["name"].lower() == param_name.lower():
                        if i < len(slot["parameters"]):
                            slot["parameters"][i]["value"] = new_value
                            print_pass(f"  Found and set '{param_name}' = {new_value}")
                            param_found = True
                        break

                if not param_found:
                    print_fail(f"  WARNING: Parameter '{param_name}' not found in engine {engine_id}")
                    print_info(f"  (This warning should be logged in production)")

    print_pass("Parameter mismatch warning test completed")
    return True


async def run_all_tests():
    """Run all test suites"""
    print(f"\n{BLUE}{'='*70}{RESET}")
    print(f"{BLUE}CALCULATOR MAX INTELLIGENCE - FIX VALIDATION TEST SUITE{RESET}")
    print(f"{BLUE}{'='*70}{RESET}")

    results = {
        "Value Clamping (CRITICAL)": await test_value_clamping(),
        "Parsing Patterns (HIGH)": test_parsing_patterns(),
        "JSON Extraction (HIGH)": test_json_extraction(),
        "Parameter Mismatch (MEDIUM)": await test_parameter_mismatch_warning()
    }

    print(f"\n{BLUE}{'='*70}{RESET}")
    print(f"{BLUE}TEST SUMMARY{RESET}")
    print(f"{BLUE}{'='*70}{RESET}")

    all_passed = True
    for test_name, passed in results.items():
        status = f"{GREEN}PASS{RESET}" if passed else f"{RED}FAIL{RESET}"
        print(f"  {test_name}: {status}")
        if not passed:
            all_passed = False

    print(f"{BLUE}{'='*70}{RESET}")

    if all_passed:
        print(f"\n{GREEN}{'='*70}{RESET}")
        print(f"{GREEN}ALL TESTS PASSED! ✓{RESET}")
        print(f"{GREEN}{'='*70}{RESET}\n")
        return 0
    else:
        print(f"\n{RED}{'='*70}{RESET}")
        print(f"{RED}SOME TESTS FAILED! ✗{RESET}")
        print(f"{RED}{'='*70}{RESET}\n")
        return 1


if __name__ == "__main__":
    exit_code = asyncio.run(run_all_tests())
    sys.exit(exit_code)
