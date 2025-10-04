#!/usr/bin/env python3
"""
Test the FIXED pipeline to verify engine matching is working
"""

import requests
import json
from typing import Dict, List
from engine_mapping_authoritative import ENGINE_NAMES
import time

def test_fixed_pipeline():
    """
    Comprehensive test of the fixed pipeline
    """
    
    print("🧪 TESTING FIXED TRINITY PIPELINE")
    print("=" * 80)
    
    # Test cases with CLEAR engine requirements
    test_cases = [
        {
            "name": "Explicit Tube & Plate",
            "prompt": "I need vintage tube warmth and plate reverb",
            "required": [15, 39],  # Vintage Tube, Plate Reverb
            "engine_names": ["Vintage Tube Preamp", "Plate Reverb"]
        },
        {
            "name": "Shimmer Request",
            "prompt": "Give me shimmer reverb for ethereal ambience",
            "required": [42],  # Shimmer Reverb
            "engine_names": ["Shimmer Reverb"]
        },
        {
            "name": "Bit Crusher",
            "prompt": "I want bit crusher distortion for lo-fi sound",
            "required": [18],  # Bit Crusher
            "engine_names": ["Bit Crusher"]
        },
        {
            "name": "Specific Combo",
            "prompt": "Classic compressor with parametric EQ please",
            "required": [2, 7],  # Classic Compressor, Parametric EQ
            "engine_names": ["Classic Compressor", "Parametric EQ"]
        },
        {
            "name": "Gate & K-Style",
            "prompt": "Metal guitar needs noise gate and k-style overdrive",
            "required": [4, 22],  # Noise Gate, K-Style
            "engine_names": ["Noise Gate", "K-Style Overdrive"]
        },
        {
            "name": "Tape & Spring",
            "prompt": "Vintage vibe with tape echo and spring reverb",
            "required": [34, 40],  # Tape Echo, Spring Reverb
            "engine_names": ["Tape Echo", "Spring Reverb"]
        },
        {
            "name": "Chorus & Phaser",
            "prompt": "Add classic chorus and analog phaser for modulation",
            "required": [24, 25],  # Chorus, Phaser
            "engine_names": ["Classic Chorus", "Analog Phaser"]
        },
        {
            "name": "Harmonizer & Vocoder",
            "prompt": "I need intelligent harmonizer and vocoder effects",
            "required": [45, 51],  # Harmonizer, Vocoder
            "engine_names": ["Intelligent Harmonizer", "Vocoder"]
        },
        {
            "name": "Engine Numbers",
            "prompt": "Please use engine 15 and engine 39 specifically",
            "required": [15, 39],
            "engine_names": ["Vintage Tube Preamp", "Plate Reverb"]
        },
        {
            "name": "Multiple Reverbs",
            "prompt": "I want shimmer reverb with plate reverb and spring reverb",
            "required": [42, 39, 40],  # All three reverbs
            "engine_names": ["Shimmer Reverb", "Plate Reverb", "Spring Reverb"]
        }
    ]
    
    results = []
    total_engines_required = 0
    total_engines_found = 0
    
    base_url = "http://localhost:8000"
    
    for test in test_cases:
        print(f"\n{'='*60}")
        print(f"Test: {test['name']}")
        print(f"Prompt: '{test['prompt']}'")
        print(f"Required: {', '.join(test['engine_names'])}")
        print("-" * 60)
        
        try:
            # Make request
            response = requests.post(
                f"{base_url}/generate",
                json={"prompt": test["prompt"]},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                metadata = data.get("metadata", {})
                
                # Extract engines from result
                result_engines = []
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        result_engines.append(engine_id)
                
                # Check each required engine
                found_count = 0
                missing = []
                
                print("Results:")
                for required_id in test["required"]:
                    if required_id in result_engines:
                        print(f"  ✅ Found {ENGINE_NAMES[required_id]}")
                        found_count += 1
                    else:
                        print(f"  ❌ Missing {ENGINE_NAMES[required_id]}")
                        missing.append(ENGINE_NAMES[required_id])
                
                # Calculate score
                score = (found_count / len(test["required"])) * 100
                
                # Show what we got
                print(f"\nActual engines:")
                for i, engine_id in enumerate(result_engines[:6], 1):
                    print(f"  Slot {i}: {ENGINE_NAMES.get(engine_id, f'Unknown({engine_id})')}")
                
                print(f"\nScore: {score:.0f}% ({found_count}/{len(test['required'])})")
                
                # Check metadata
                if "required_engines" in metadata:
                    print(f"Metadata: System identified {len(metadata['required_engines'])} required engines")
                if "warnings" in metadata and metadata["warnings"]:
                    print(f"Warnings: {metadata['warnings']}")
                
                results.append({
                    "name": test["name"],
                    "score": score,
                    "found": found_count,
                    "required": len(test["required"]),
                    "missing": missing
                })
                
                total_engines_required += len(test["required"])
                total_engines_found += found_count
                
            else:
                print(f"❌ Request failed: {response.status_code}")
                print(f"   Error: {response.text[:200]}")
                results.append({
                    "name": test["name"],
                    "score": 0,
                    "found": 0,
                    "required": len(test["required"]),
                    "missing": test["engine_names"]
                })
                total_engines_required += len(test["required"])
                
        except Exception as e:
            print(f"❌ Error: {str(e)}")
            results.append({
                "name": test["name"],
                "score": 0,
                "found": 0,
                "required": len(test["required"]),
                "missing": test["engine_names"]
            })
            total_engines_required += len(test["required"])
        
        # Small delay between requests
        time.sleep(0.5)
    
    # Final Report
    print("\n" + "=" * 80)
    print("📊 FINAL REPORT - FIXED PIPELINE")
    print("=" * 80)
    
    # Overall accuracy
    overall_accuracy = (total_engines_found / total_engines_required) * 100 if total_engines_required > 0 else 0
    print(f"\n🎯 Overall Accuracy: {overall_accuracy:.1f}%")
    print(f"   Total Required: {total_engines_required} engines")
    print(f"   Total Found: {total_engines_found} engines")
    
    # Per-test summary
    print("\n📋 Test Summary:")
    perfect_tests = 0
    for r in results:
        status = "✅" if r["score"] == 100 else "⚠️" if r["score"] >= 50 else "❌"
        print(f"{status} {r['name']:25} {r['found']}/{r['required']} engines ({r['score']:.0f}%)")
        if r["score"] == 100:
            perfect_tests += 1
    
    # Missing engines analysis
    print("\n❌ Commonly Missing Engines:")
    all_missing = []
    for r in results:
        all_missing.extend(r["missing"])
    
    if all_missing:
        from collections import Counter
        missing_counts = Counter(all_missing)
        for engine, count in missing_counts.most_common(5):
            print(f"   - {engine}: Missing in {count} tests")
    
    # Success criteria
    print("\n" + "=" * 80)
    print("🏆 SUCCESS CRITERIA:")
    print(f"   Target: 80% overall accuracy")
    print(f"   Actual: {overall_accuracy:.1f}%")
    print(f"   Perfect tests: {perfect_tests}/{len(results)}")
    
    if overall_accuracy >= 80:
        print("\n✅ SUCCESS! The fixed pipeline achieves target accuracy!")
    elif overall_accuracy >= 60:
        print("\n⚠️ BETTER but still needs improvement")
    else:
        print("\n❌ STILL BROKEN - Major issues remain")
    
    return overall_accuracy >= 80

if __name__ == "__main__":
    success = test_fixed_pipeline()
    exit(0 if success else 1)