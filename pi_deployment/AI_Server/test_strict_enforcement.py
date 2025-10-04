#!/usr/bin/env python3
"""
Test strict enforcement of engine selection
"""

import json

# Load the complete knowledge base
with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
    knowledge = json.load(f)

print("="*60)
print("STRICT ENFORCEMENT TEST")
print("="*60)

# Check we have the AI instructions
if "ai_instructions" in knowledge:
    print("\n✅ AI Instructions present:")
    for rule in knowledge["ai_instructions"]["critical"]:
        print(f"  • {rule}")
else:
    print("❌ Missing AI instructions!")

# Check selection rules
if "engine_selection_rules" in knowledge:
    print(f"\n✅ Selection rules present: {len(knowledge['engine_selection_rules'])} rules")
    for rule_name, rule_data in knowledge["engine_selection_rules"].items():
        print(f"  {rule_name}: Engine {rule_data['engine_id']} ({rule_data['engine_name']})")
        print(f"    Keywords: {', '.join(rule_data['keywords'])}")
else:
    print("❌ Missing selection rules!")

# Check engine completeness
print("\n🔍 Checking engine completeness:")
complete_engines = 0
incomplete_engines = []

for eid, engine in knowledge["engines"].items():
    has_function = "function" in engine
    has_character = "character" in engine
    has_use_cases = "use_cases" in engine
    
    if has_function and has_character:
        complete_engines += 1
    else:
        incomplete_engines.append(f"{eid}: {engine.get('name', 'Unknown')}")

print(f"  Complete engines: {complete_engines}/{len(knowledge['engines'])}")
if incomplete_engines:
    print(f"  Incomplete: {', '.join(incomplete_engines[:5])}")

# Test enforcement logic
print("\n🧪 Testing enforcement logic:")

def test_enforcement(prompt, expected_engines):
    """Test if prompt would enforce correct engines"""
    prompt_lower = prompt.lower()
    enforced = []
    
    for rule_name, rule_data in knowledge.get("engine_selection_rules", {}).items():
        for keyword in rule_data.get("keywords", []):
            if keyword in prompt_lower:
                enforced.append({
                    "id": rule_data["engine_id"],
                    "name": rule_data["engine_name"],
                    "keyword": keyword
                })
                break
    
    print(f"\nPrompt: '{prompt}'")
    print(f"  Expected: {expected_engines}")
    enforced_str = [f"{e['name']} (ID {e['id']})" for e in enforced]
    print(f"  Enforced: {enforced_str}")
    
    # Check if expectations met
    expected_ids = [e[0] for e in expected_engines]
    enforced_ids = [e["id"] for e in enforced]
    
    for exp_id, exp_name in expected_engines:
        if exp_id in enforced_ids:
            print(f"    ✅ {exp_name} enforced")
        else:
            print(f"    ❌ {exp_name} NOT enforced!")
    
    return all(eid in enforced_ids for eid in expected_ids)

# Test cases
tests = [
    ("ambient pad with shimmer reverb", [(42, "Shimmer Reverb")]),
    ("guitar with spring reverb", [(40, "Spring Reverb")]),
    ("metal with noise gate", [(4, "Noise Gate")]),
    ("ethereal chorus effect", [(42, "Shimmer Reverb"), (23, "Digital Chorus")])
]

all_passed = True
for prompt, expected in tests:
    passed = test_enforcement(prompt, expected)
    all_passed = all_passed and passed

print("\n" + "="*60)
if all_passed:
    print("✅ ALL ENFORCEMENT TESTS PASSED!")
else:
    print("❌ Some enforcement tests failed - rules need adjustment")
print("="*60)