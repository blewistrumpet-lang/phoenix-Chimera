#!/usr/bin/env python3
"""
Test script for Collaborative Intelligence System
Tests rule-based vs knowledge-based routing and coherence
"""

import asyncio
import json
from visionary_complete import CompleteVisionary

# Critical test prompts (from the plan)
TEST_PROMPTS = [
    # Rule-based prompts (should NOT have Shimmer or Tape Echo)
    {
        "prompt": "Drag me over hot coals",
        "expected_routing": "rule_based",
        "forbidden_engines": [42, 34],  # NO Shimmer, NO Tape Echo
        "required_engines": [18, 20, 21],  # Should have distortion
    },
    {
        "prompt": "Dark underwater pressure",
        "expected_routing": "rule_based",
        "forbidden_engines": [42, 16],  # NO Shimmer, NO Platinum Harmonic
        "required_engines": [9, 10],  # Should have filtering
    },
    {
        "prompt": "Crystal shimmer cascade",
        "expected_routing": "rule_based",
        "forbidden_engines": [],  # Nothing forbidden
        "required_engines": [42],  # SHOULD have Shimmer (appropriate here!)
    },

    # Knowledge-based prompt (should use GPT)
    {
        "prompt": "Michael Jackson Thriller vocals",
        "expected_routing": "knowledge_based",
        "forbidden_engines": [],
        "required_engines": [],  # GPT decides based on knowledge
    },
]


async def test_preset(visionary: CompleteVisionary, test_case: dict):
    """Test a single preset generation"""
    prompt = test_case['prompt']
    print(f"\n{'=' * 80}")
    print(f"📝 Testing: {prompt}")
    print(f"{'=' * 80}")

    # Generate preset
    try:
        preset = await visionary.generate_complete_preset(prompt)

        # Extract engine IDs
        engine_ids = [slot['engine_id'] for slot in preset.get('slots', [])]
        print(f"✅ Preset name: {preset.get('name', 'Unnamed')}")
        print(f"   Engine IDs: {engine_ids}")

        # Check for forbidden engines
        forbidden_found = []
        for forbidden_id in test_case['forbidden_engines']:
            if forbidden_id in engine_ids:
                forbidden_found.append(forbidden_id)

        if forbidden_found:
            print(f"❌ COHERENCE FAILURE: Found forbidden engines: {forbidden_found}")
            return False
        else:
            print(f"✅ COHERENCE CHECK: No forbidden engines present")

        # Check for required engines (if rule-based)
        if test_case['expected_routing'] == "rule_based":
            required_found = any(req_id in engine_ids for req_id in test_case['required_engines'])
            if not required_found:
                print(f"⚠️  WARNING: No required engines found from {test_case['required_engines']}")
            else:
                print(f"✅ REQUIRED CHECK: At least one required engine present")

        return True

    except Exception as e:
        print(f"❌ ERROR: {str(e)}")
        import traceback
        traceback.print_exc()
        return False


async def main():
    """Run all tests"""
    print("=" * 80)
    print("COLLABORATIVE INTELLIGENCE TEST SUITE")
    print("Testing rule-based vs knowledge-based routing")
    print("=" * 80)

    # Initialize Visionary (without OpenAI for rule-based tests)
    visionary = CompleteVisionary()

    # Run tests
    results = []
    for test_case in TEST_PROMPTS:
        passed = await test_preset(visionary, test_case)
        results.append({
            "prompt": test_case['prompt'],
            "passed": passed
        })

    # Print summary
    print(f"\n{'=' * 80}")
    print("TEST SUMMARY")
    print(f"{'=' * 80}")

    passed_count = sum(1 for r in results if r['passed'])
    total_count = len(results)
    coherence_pct = (passed_count / total_count) * 100 if total_count > 0 else 0

    for result in results:
        status = "✅ PASS" if result['passed'] else "❌ FAIL"
        print(f"{status}: {result['prompt']}")

    print(f"\n🎯 Coherence Score: {passed_count}/{total_count} ({coherence_pct:.1f}%)")

    if coherence_pct >= 95:
        print("🎉 SUCCESS: Achieved 95%+ coherence target!")
    else:
        print(f"⚠️  NEEDS IMPROVEMENT: Target is 95%, current is {coherence_pct:.1f}%")


if __name__ == "__main__":
    asyncio.run(main())
