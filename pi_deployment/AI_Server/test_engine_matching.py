#!/usr/bin/env python3
"""
Test engine matching accuracy with detailed analysis
"""

import requests
import json
from engine_mapping_authoritative import ENGINE_NAMES

def test_engine_matching():
    """Test engine matching with specific expectations"""
    
    print("🎯 TESTING ENGINE MATCHING ACCURACY")
    print("=" * 80)
    
    # More specific test cases with clear engine expectations
    test_cases = [
        {
            "prompt": "Add vintage tube warmth and plate reverb",
            "expected_engines": [15, 39],  # Vintage Tube, Plate Reverb
            "expected_names": ["Vintage Tube Preamp", "Plate Reverb"]
        },
        {
            "prompt": "Heavy metal with noise gate and k-style overdrive",
            "expected_engines": [4, 22],  # Noise Gate, K-Style
            "expected_names": ["Noise Gate", "K-Style Overdrive"]
        },
        {
            "prompt": "Shimmer reverb with pitch shifting",
            "expected_engines": [42, 48],  # Shimmer, Pitch Shifter
            "expected_names": ["Shimmer Reverb", "Pitch Shifter"]
        },
        {
            "prompt": "Classic compressor with parametric EQ",
            "expected_engines": [2, 7],  # Classic Comp, Parametric EQ
            "expected_names": ["Classic Compressor", "Parametric EQ"]
        },
        {
            "prompt": "Tape echo with spring reverb",
            "expected_engines": [34, 40],  # Tape Echo, Spring Reverb
            "expected_names": ["Tape Echo", "Spring Reverb"]
        },
        {
            "prompt": "Bit crusher distortion",
            "expected_engines": [18],  # Bit Crusher (was 19 in old mapping)
            "expected_names": ["Bit Crusher"]
        },
        {
            "prompt": "Chorus and phaser modulation",
            "expected_engines": [24, 25],  # Classic Chorus, Analog Phaser
            "expected_names": ["Classic Chorus", "Analog Phaser"]
        },
        {
            "prompt": "Vocoder with harmonizer",
            "expected_engines": [51, 45],  # Vocoder, Intelligent Harmonizer
            "expected_names": ["Vocoder", "Intelligent Harmonizer"]
        }
    ]
    
    base_url = "http://localhost:8000"
    results = []
    
    for i, test in enumerate(test_cases, 1):
        print(f"\nTest {i}: '{test['prompt']}'")
        print("-" * 60)
        print(f"Expected: {', '.join(test['expected_names'])}")
        
        try:
            response = requests.post(
                f"{base_url}/generate",
                json={"prompt": test["prompt"]},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                
                # Extract actual engines
                actual_engines = []
                actual_names = []
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        actual_engines.append(engine_id)
                        actual_names.append(ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})"))
                
                print(f"Actual:   {', '.join(actual_names[:4])}")
                
                # Calculate match
                matches = 0
                for expected_id in test["expected_engines"]:
                    if expected_id in actual_engines:
                        matches += 1
                        print(f"  ✅ Found {ENGINE_NAMES[expected_id]}")
                    else:
                        print(f"  ❌ Missing {ENGINE_NAMES[expected_id]}")
                
                match_rate = (matches / len(test["expected_engines"])) * 100
                print(f"Match Rate: {match_rate:.0f}% ({matches}/{len(test['expected_engines'])})")
                
                results.append({
                    "prompt": test["prompt"],
                    "match_rate": match_rate,
                    "expected": test["expected_names"],
                    "actual": actual_names,
                    "missing": [ENGINE_NAMES[e] for e in test["expected_engines"] if e not in actual_engines]
                })
                
            else:
                print(f"❌ Request failed: {response.status_code}")
                results.append({
                    "prompt": test["prompt"],
                    "match_rate": 0,
                    "expected": test["expected_names"],
                    "actual": [],
                    "missing": test["expected_names"]
                })
                
        except Exception as e:
            print(f"❌ Error: {str(e)}")
            results.append({
                "prompt": test["prompt"],
                "match_rate": 0,
                "expected": test["expected_names"],
                "actual": [],
                "missing": test["expected_names"]
            })
    
    # Summary
    print("\n" + "=" * 80)
    print("📊 ENGINE MATCHING ANALYSIS")
    print("=" * 80)
    
    total_match = sum(r["match_rate"] for r in results) / len(results) if results else 0
    print(f"\n⚡ Overall Match Rate: {total_match:.1f}%")
    
    if total_match < 50:
        print("❌ CRITICAL: Engine matching is very poor")
    elif total_match < 70:
        print("⚠️ WARNING: Engine matching needs improvement")
    elif total_match < 90:
        print("✅ GOOD: Engine matching is decent but could be better")
    else:
        print("🎉 EXCELLENT: Engine matching is highly accurate")
    
    # Analyze patterns
    print("\n📋 Common Issues:")
    all_missing = []
    for r in results:
        all_missing.extend(r["missing"])
    
    if all_missing:
        from collections import Counter
        missing_counts = Counter(all_missing)
        for engine, count in missing_counts.most_common(5):
            print(f"  - {engine}: Missing in {count} tests")
    
    return total_match

if __name__ == "__main__":
    match_rate = test_engine_matching()
    
    print("\n" + "=" * 80)
    print("🔧 RECOMMENDATIONS TO IMPROVE ENGINE MATCHING:")
    print("=" * 80)
    print("""
1. The Oracle is returning the same preset for all queries
   - Check if FAISS search is working properly
   - Verify the corpus has diverse presets
   
2. The Calculator needs better keyword → engine mapping
   - When prompt says "tube", add Vintage Tube (15)
   - When prompt says "plate reverb", add Plate Reverb (39)
   - When prompt says "shimmer", add Shimmer Reverb (42)
   
3. The Cloud AI (Visionary) may not be suggesting the right engines
   - Check if blueprint contains engine suggestions
   - Verify engine mapping is correct
   
4. The Alchemist might be removing requested engines
   - Check if signal chain optimization is too aggressive
   - Ensure requested engines aren't being replaced
    """)