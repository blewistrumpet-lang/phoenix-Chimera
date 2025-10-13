#!/usr/bin/env python3
"""
Quick test with 5 prompts to verify 4-engine minimum enforcement
"""

import requests
import json
import time

TEST_PROMPTS = [
    ("POETIC", "ethereal whispers in the void"),
    ("TECHNICAL", "parallel compression with 4:1 ratio"),  
    ("ARTISTIC", "vintage Beatles Abbey Road drums"),
    ("POETIC", "crystalline dreams of tomorrow"),
    ("TECHNICAL", "tape delay at 1/8 dotted with 35% feedback")
]

def test_prompt(prompt):
    """Test a single prompt"""
    url = "http://localhost:8000/generate"
    
    try:
        response = requests.post(url, json={"prompt": prompt}, timeout=30)
        if response.status_code == 200:
            data = response.json()
            preset = data.get("preset", {})
            
            # Count engines
            engines = []
            for slot in preset.get("slots", []):
                if slot.get("engine_id", 0) != 0:
                    engines.append({
                        "id": slot["engine_id"],
                        "name": slot["engine_name"]
                    })
            
            return {
                "success": True,
                "preset_name": preset.get("name", "Unknown"),
                "engines": engines,
                "engine_count": len(engines),
                "reasoning": data.get("debug", {}).get("visionary", {}).get("reasoning", {})
            }
        else:
            return {
                "success": False,
                "error": f"HTTP {response.status_code}: {response.text}"
            }
    except Exception as e:
        return {
            "success": False,
            "error": str(e)
        }

print("="*80)
print("5-PROMPT TEST WITH 4-ENGINE MINIMUM ENFORCEMENT")
print("="*80)

# Give server time to start
time.sleep(5)

results = []
passing = 0
failing = 0

for i, (category, prompt) in enumerate(TEST_PROMPTS, 1):
    print(f"\n[{i}/5] {category}: \"{prompt}\"")
    print("-"*60)
    
    result = test_prompt(prompt)
    
    if result["success"]:
        engine_count = result["engine_count"]
        
        # Check 4-engine minimum
        if engine_count >= 4:
            status = "✅ PASS"
            passing += 1
        else:
            status = f"❌ FAIL (only {engine_count} engines)"
            failing += 1
            
        print(f"{status}")
        print(f"Preset: {result['preset_name']}")
        print(f"Engines ({engine_count}):")
        
        for engine in result['engines']:
            print(f"  • {engine['name']} (ID: {engine['id']})")
            
        # Show reasoning if available
        reasoning = result.get("reasoning", {})
        if reasoning.get("engine_selection"):
            print("\nEngine Selection Reasoning:")
            for reason in reasoning["engine_selection"][:3]:  # Show first 3
                print(f"  - {reason}")
                
    else:
        print(f"❌ ERROR: {result['error']}")
        failing += 1
    
    results.append(result)
    time.sleep(1)

# Summary
print("\n" + "="*80)
print("SUMMARY")
print("="*80)
print(f"✅ Passing (4+ engines): {passing}/5")
print(f"❌ Failing (<4 engines): {failing}/5")

if passing == 5:
    print("\n🎉 SUCCESS! All prompts generated presets with 4+ engines!")
elif passing >= 3:
    print("\n👍 GOOD: Most prompts are meeting the 4-engine minimum")
else:
    print("\n⚠️ PROBLEM: 4-engine minimum is not being enforced properly")

# Check for specific issues
for i, result in enumerate(results, 1):
    if result["success"] and result["engine_count"] < 4:
        print(f"\nPrompt {i} only used {result['engine_count']} engines - needs investigation")