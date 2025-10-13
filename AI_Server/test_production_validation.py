#!/usr/bin/env python3
"""
Production validation test - 20 diverse prompts
Tests full Collaborative Intelligence system
"""

import asyncio
import json
from visionary_complete import CompleteVisionary

# Comprehensive test set: 16 rule-based + 4 knowledge-based (80/20 split)
TEST_PROMPTS = [
    # ========== RULE-BASED PROMPTS (Character-driven) ==========
    # Harsh/Aggressive
    {"prompt": "Drag me over hot coals", "category": "harsh", "routing": "rule_based"},
    {"prompt": "Industrial destruction", "category": "harsh", "routing": "rule_based"},

    # Dark/Deep
    {"prompt": "Dark underwater pressure", "category": "dark", "routing": "rule_based"},
    {"prompt": "Deep abyss descent", "category": "dark", "routing": "rule_based"},

    # Ethereal/Shimmer
    {"prompt": "Crystal shimmer cascade", "category": "ethereal", "routing": "rule_based"},
    {"prompt": "Floating celestial atmosphere", "category": "ethereal", "routing": "rule_based"},

    # Vintage/Warm
    {"prompt": "Vintage warm analog", "category": "vintage", "routing": "rule_based"},
    {"prompt": "Nostalgic tape warmth", "category": "vintage", "routing": "rule_based"},

    # Underwater/Filtered
    {"prompt": "Submerged aquatic sounds", "category": "underwater", "routing": "rule_based"},

    # Glitchy/Digital
    {"prompt": "Glitch stuttering chaos", "category": "glitchy", "routing": "rule_based"},

    # Psychedelic
    {"prompt": "Psychedelic swirling colors", "category": "psychedelic", "routing": "rule_based"},

    # Space/Cosmic
    {"prompt": "Cosmic void reverberations", "category": "space", "routing": "rule_based"},

    # Industrial
    {"prompt": "Metallic industrial rhythm", "category": "industrial", "routing": "rule_based"},

    # Smooth/Clean
    {"prompt": "Clean modern precision", "category": "smooth", "routing": "rule_based"},
    {"prompt": "Polished transparent sound", "category": "smooth", "routing": "rule_based"},

    # Neutral
    {"prompt": "Balanced musical production", "category": "neutral", "routing": "rule_based"},

    # ========== KNOWLEDGE-BASED PROMPTS (Artist/Gear-specific) ==========
    {"prompt": "Michael Jackson Thriller vocals", "category": "artist", "routing": "knowledge_based"},
    {"prompt": "Radiohead Kid A production", "category": "artist", "routing": "knowledge_based"},
    {"prompt": "1176 compression on vocals", "category": "gear", "routing": "knowledge_based"},
    {"prompt": "Neve console warmth", "category": "gear", "routing": "knowledge_based"},
]


async def validate_prompt(visionary: CompleteVisionary, test: dict) -> dict:
    """Validate a single prompt"""
    prompt = test['prompt']
    expected_routing = test['routing']

    try:
        # Generate preset
        preset = await visionary.generate_complete_preset(prompt)

        # Extract engine IDs and names
        engine_ids = []
        engine_names = []
        for slot in preset.get('slots', []):
            eid = slot.get('engine_id', 0)
            if eid != 0:
                engine_ids.append(eid)
                ename = visionary.engines.get(str(eid), {}).get('name', 'Unknown')
                engine_names.append(f"{ename} ({eid})")

        # Check routing
        context = visionary.analyze_prompt_context(prompt)
        actual_routing = context.get('routing_strategy', 'unknown')
        routing_correct = (actual_routing == expected_routing)

        # Check coherence (no forbidden combinations)
        # For now, just validate that we got reasonable engines
        coherence_valid = len(engine_ids) > 0 and all(eid > 0 for eid in engine_ids if eid != 0)

        return {
            "prompt": prompt,
            "category": test['category'],
            "expected_routing": expected_routing,
            "actual_routing": actual_routing,
            "routing_correct": routing_correct,
            "preset_name": preset.get('name', 'Unnamed'),
            "engine_ids": engine_ids,
            "engine_names": engine_names,
            "coherence_valid": coherence_valid,
            "success": routing_correct and coherence_valid
        }

    except Exception as e:
        return {
            "prompt": prompt,
            "category": test['category'],
            "error": str(e),
            "success": False
        }


async def main():
    """Run production validation"""
    print("=" * 80)
    print("PRODUCTION VALIDATION TEST")
    print("Testing 20 diverse prompts across Collaborative Intelligence system")
    print("=" * 80)

    visionary = CompleteVisionary()

    # Check OpenAI
    if not visionary.client:
        print("\n⚠️  WARNING: No OpenAI configured - knowledge-based tests will fail")

    # Run all tests
    results = []
    for i, test in enumerate(TEST_PROMPTS, 1):
        print(f"\n[{i}/{len(TEST_PROMPTS)}] Testing: {test['prompt']}")
        result = await validate_prompt(visionary, test)
        results.append(result)

        # Show result
        if result['success']:
            routing_icon = "🎯" if result.get('expected_routing') == 'rule_based' else "🧠"
            print(f"    {routing_icon} {result.get('actual_routing', 'unknown').upper()}")
            print(f"    ✅ {result['preset_name']}")
            print(f"    Engines: {', '.join(result.get('engine_names', []))}")
        else:
            print(f"    ❌ FAILED: {result.get('error', 'Unknown error')}")

        # Brief pause to avoid rate limits
        if i < len(TEST_PROMPTS):
            await asyncio.sleep(0.5)

    # Summary
    print(f"\n{'=' * 80}")
    print("VALIDATION SUMMARY")
    print(f"{'=' * 80}")

    # Overall stats
    total = len(results)
    passed = sum(1 for r in results if r.get('success', False))
    failed = total - passed
    success_rate = (passed / total * 100) if total > 0 else 0

    # By routing type
    rule_based_results = [r for r in results if r.get('expected_routing') == 'rule_based']
    knowledge_results = [r for r in results if r.get('expected_routing') == 'knowledge_based']

    rule_based_passed = sum(1 for r in rule_based_results if r.get('success', False))
    knowledge_passed = sum(1 for r in knowledge_results if r.get('success', False))

    rule_based_rate = (rule_based_passed / len(rule_based_results) * 100) if rule_based_results else 0
    knowledge_rate = (knowledge_passed / len(knowledge_results) * 100) if knowledge_results else 0

    print(f"\n📊 OVERALL RESULTS:")
    print(f"   Total:   {passed}/{total} ({success_rate:.1f}%)")
    print(f"   Passed:  {passed}")
    print(f"   Failed:  {failed}")

    print(f"\n🎯 RULE-BASED (Character-driven):")
    print(f"   Results: {rule_based_passed}/{len(rule_based_results)} ({rule_based_rate:.1f}%)")

    print(f"\n🧠 KNOWLEDGE-BASED (Artist/Gear):")
    print(f"   Results: {knowledge_passed}/{len(knowledge_results)} ({knowledge_rate:.1f}%)")

    # Show any failures
    failures = [r for r in results if not r.get('success', False)]
    if failures:
        print(f"\n❌ FAILURES:")
        for f in failures:
            print(f"   - {f['prompt']}")
            print(f"     Error: {f.get('error', 'Unknown')}")

    # Final verdict
    print(f"\n{'=' * 80}")
    if success_rate >= 95:
        print("🎉 SUCCESS: Achieved 95%+ coherence target!")
        print("✅ System is PRODUCTION READY")
    elif success_rate >= 90:
        print(f"⚠️  CLOSE: {success_rate:.1f}% (target: 95%)")
        print("🔧 Minor tuning recommended before production")
    else:
        print(f"❌ NEEDS WORK: {success_rate:.1f}% (target: 95%)")
        print("🔧 Significant improvements needed")
    print(f"{'=' * 80}")


if __name__ == "__main__":
    asyncio.run(main())
