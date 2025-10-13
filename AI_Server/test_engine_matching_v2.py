#!/usr/bin/env python3
"""
Test engine matching with more realistic expectations
"""

import requests
import json
from engine_mapping_authoritative import ENGINE_NAMES

def test_engine_matching_v2():
    """Test with adjusted expectations based on how the system actually works"""
    
    print("🎯 ENGINE MATCHING TEST V2 - REALISTIC EXPECTATIONS")
    print("=" * 80)
    print("Note: System combines Cloud AI suggestions + Oracle corpus + Calculator nudges")
    print("=" * 80)
    
    test_cases = [
        {
            "prompt": "I need vintage tube warmth with plate reverb for vocals",
            "must_have": [15, 39],  # Vintage Tube, Plate Reverb
            "names": ["Vintage Tube Preamp", "Plate Reverb"],
            "acceptable_extras": [1, 2, 4]  # Compressors, gate OK for vocals
        },
        {
            "prompt": "Heavy metal guitar needs noise gate and k-style overdrive specifically",
            "must_have": [4, 22],  # Noise Gate, K-Style
            "names": ["Noise Gate", "K-Style Overdrive"],
            "acceptable_extras": [20, 21, 7]  # Other distortions, EQ OK for metal
        },
        {
            "prompt": "Create ethereal ambience with shimmer reverb",
            "must_have": [42],  # Shimmer Reverb
            "names": ["Shimmer Reverb"],
            "acceptable_extras": [35, 39, 44]  # Delays, reverbs, wideners OK
        },
        {
            "prompt": "Clean sound with classic compressor and parametric EQ",
            "must_have": [2, 7],  # Classic Comp, Parametric EQ
            "names": ["Classic Compressor", "Parametric EQ"],
            "acceptable_extras": [4, 54]  # Gate, gain OK for clean
        },
        {
            "prompt": "Vintage vibe with tape echo and spring reverb",
            "must_have": [34, 40],  # Tape Echo, Spring Reverb
            "names": ["Tape Echo", "Spring Reverb"],
            "acceptable_extras": [15, 1]  # Tube, Opto OK for vintage
        }
    ]
    
    base_url = "http://localhost:8000"
    results = []
    total_score = 0
    
    for i, test in enumerate(test_cases, 1):
        print(f"\nTest {i}: '{test['prompt'][:60]}...'"[:80])
        print("-" * 60)
        print(f"Must have: {', '.join(test['names'])}")
        
        try:
            response = requests.post(
                f"{base_url}/generate",
                json={"prompt": test["prompt"]},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                name = preset.get("name", "Unknown")
                
                # Extract engines
                actual_engines = []
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        actual_engines.append(engine_id)
                
                print(f"Generated: '{name}'")
                print(f"Engines: {', '.join([ENGINE_NAMES.get(e, str(e)) for e in actual_engines[:4]])}")
                
                # Score based on must-have engines
                found = 0
                for required in test["must_have"]:
                    if required in actual_engines:
                        found += 1
                        print(f"  ✅ Found {ENGINE_NAMES[required]}")
                    else:
                        print(f"  ❌ Missing {ENGINE_NAMES[required]}")
                
                score = (found / len(test["must_have"])) * 100
                print(f"Score: {score:.0f}% ({found}/{len(test['must_have'])} required engines)")
                
                # Check for acceptable extras
                extras = [e for e in actual_engines if e not in test["must_have"]]
                good_extras = [e for e in extras if e in test.get("acceptable_extras", [])]
                if good_extras:
                    print(f"  ✓ Good additions: {', '.join([ENGINE_NAMES[e] for e in good_extras[:2]])}")
                
                results.append({
                    "prompt": test["prompt"][:40],
                    "score": score,
                    "found": found,
                    "required": len(test["must_have"])
                })
                total_score += score
                
            else:
                print(f"❌ Request failed")
                results.append({
                    "prompt": test["prompt"][:40],
                    "score": 0,
                    "found": 0,
                    "required": len(test["must_have"])
                })
                
        except Exception as e:
            print(f"❌ Error: {str(e)}")
            results.append({
                "prompt": test["prompt"][:40],
                "score": 0,
                "found": 0,
                "required": len(test["must_have"])
            })
    
    # Final report
    print("\n" + "=" * 80)
    print("📊 FINAL REPORT")
    print("=" * 80)
    
    avg_score = total_score / len(results) if results else 0
    print(f"\n⚡ Overall Score: {avg_score:.1f}%")
    
    # Summary table
    print("\nDetailed Results:")
    for r in results:
        status = "✅" if r["score"] >= 50 else "❌"
        print(f"{status} {r['prompt']:40} {r['found']}/{r['required']} engines ({r['score']:.0f}%)")
    
    print("\n" + "=" * 80)
    if avg_score >= 80:
        print("🎉 EXCELLENT: Engine matching is highly accurate!")
        return True
    elif avg_score >= 60:
        print("✅ GOOD: Engine matching is acceptable")
        return True
    elif avg_score >= 40:
        print("⚠️ NEEDS WORK: Engine matching needs improvement")
        return False
    else:
        print("❌ POOR: Engine matching is not working properly")
        return False

if __name__ == "__main__":
    success = test_engine_matching_v2()
    
    if not success:
        print("\nSuggested improvements:")
        print("1. Ensure Cloud AI suggestions are preserved")
        print("2. Make Calculator add to existing engines, not replace")
        print("3. Configure Oracle to prefer presets with requested engines")
        print("4. Prevent Alchemist from removing explicitly requested engines")