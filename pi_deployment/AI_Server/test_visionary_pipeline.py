#!/usr/bin/env python3
"""
Test the Visionary-led pipeline with focus on naming and creative interpretation
"""

import requests
import json
import time
from engine_mapping_authoritative import ENGINE_NAMES

def test_visionary_pipeline():
    """
    Test creative interpretation and naming quality
    """
    print("🎨 TESTING VISIONARY-LED PIPELINE")
    print("=" * 80)
    
    # Test cases focusing on creative/poetic prompts that were failing before
    test_prompts = [
        # Poetic/Creative descriptions
        {
            "prompt": "Make it sound like underwater dreamy vocals with lots of modulation",
            "expected": "underwater, dreamy, modulation",
            "category": "creative"
        },
        {
            "prompt": "I want it to sound like outer space",
            "expected": "space, cosmic, ethereal", 
            "category": "creative"
        },
        {
            "prompt": "Create the sound of a guitar played in a cathedral",
            "expected": "guitar, cathedral, reverb",
            "category": "creative"
        },
        
        # Technical requests
        {
            "prompt": "Modern EDM supersaw lead with heavy sidechain compression",
            "expected": "EDM, supersaw, compression",
            "category": "technical"
        },
        {
            "prompt": "Vintage 1960s Motown bass sound",
            "expected": "vintage, Motown, bass",
            "category": "technical"
        },
        
        # Genre-specific
        {
            "prompt": "Nashville country guitar with subtle chorus and spring reverb",
            "expected": "country, guitar, chorus, spring",
            "category": "genre"
        },
        {
            "prompt": "Dark techno kick drum with heavy distortion",
            "expected": "techno, kick, distortion",
            "category": "genre"
        },
        
        # Vague requests
        {
            "prompt": "Just make it sound good",
            "expected": "balanced, polished",
            "category": "vague"
        },
        {
            "prompt": "Professional mastering chain",
            "expected": "mastering, professional",
            "category": "vague"
        },
        
        # Complex descriptions
        {
            "prompt": "Warm vintage warmth with modern clarity and controlled dynamics",
            "expected": "vintage, modern, dynamics",
            "category": "complex"
        }
    ]
    
    results = []
    
    for test in test_prompts:
        print(f"\n{'='*60}")
        print(f"PROMPT: {test['prompt']}")
        print(f"Category: {test['category']}")
        print(f"Expected: {test['expected']}")
        print("-" * 60)
        
        try:
            response = requests.post(
                "http://localhost:8000/generate",
                json={"prompt": test["prompt"]},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                metadata = data.get("metadata", {})
                
                # Get the name
                name = preset.get("name", "Unknown")
                
                # Get engines
                engines = []
                engine_count = 0
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        engine_count += 1
                        engines.append(ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})"))
                
                # Get metadata
                creative_name = metadata.get("creative_name", "")
                technical_translation = metadata.get("technical_translation", "")
                character_tags = metadata.get("character_tags", [])
                
                print(f"\n📝 PRESET NAME: '{name}'")
                print(f"   Creative Name: '{creative_name}'")
                print(f"   Technical Translation: '{technical_translation}'")
                print(f"   Character Tags: {character_tags}")
                
                print(f"\n🎛️ ENGINES ({engine_count} total):")
                for i, engine in enumerate(engines, 1):
                    print(f"   {i}. {engine}")
                
                # Rate the result
                name_score = rate_name(name, test["prompt"], test["expected"])
                engine_score = rate_engine_count(engine_count)
                
                print(f"\n📊 QUALITY RATINGS:")
                print(f"   Name Relevance: {name_score}/10")
                print(f"   Engine Count: {engine_score}/10")
                
                overall = (name_score + engine_score) / 2
                
                # Grade
                if overall >= 8:
                    grade = "🌟 EXCELLENT"
                elif overall >= 6:
                    grade = "✅ GOOD"
                elif overall >= 4:
                    grade = "⚠️ FAIR"
                else:
                    grade = "❌ POOR"
                
                print(f"   OVERALL: {overall:.1f}/10 - {grade}")
                
                results.append({
                    "prompt": test["prompt"],
                    "category": test["category"],
                    "name": name,
                    "name_score": name_score,
                    "engine_count": engine_count,
                    "engine_score": engine_score,
                    "overall": overall
                })
                
            else:
                print(f"❌ Request failed: {response.status_code}")
                results.append({
                    "prompt": test["prompt"],
                    "category": test["category"],
                    "name": "ERROR",
                    "name_score": 0,
                    "engine_count": 0,
                    "engine_score": 0,
                    "overall": 0
                })
                
        except Exception as e:
            print(f"❌ Error: {str(e)}")
            results.append({
                "prompt": test["prompt"],
                "category": test["category"],
                "name": "ERROR",
                "name_score": 0,
                "engine_count": 0,
                "engine_score": 0,
                "overall": 0
            })
        
        time.sleep(0.5)
    
    # Summary
    print("\n" + "=" * 80)
    print("📊 SUMMARY REPORT")
    print("=" * 80)
    
    # By category
    categories = {}
    for r in results:
        cat = r["category"]
        if cat not in categories:
            categories[cat] = []
        categories[cat].append(r["overall"])
    
    print("\nBy Category:")
    for cat, scores in categories.items():
        avg = sum(scores) / len(scores) if scores else 0
        print(f"   {cat:10} Average: {avg:.1f}/10")
    
    # Overall
    all_scores = [r["overall"] for r in results]
    overall_avg = sum(all_scores) / len(all_scores) if all_scores else 0
    
    print(f"\nOVERALL AVERAGE: {overall_avg:.1f}/10")
    
    # Name quality
    name_scores = [r["name_score"] for r in results]
    name_avg = sum(name_scores) / len(name_scores) if name_scores else 0
    print(f"Name Quality Average: {name_avg:.1f}/10")
    
    # Engine count
    engine_counts = [r["engine_count"] for r in results]
    engine_avg = sum(engine_counts) / len(engine_counts) if engine_counts else 0
    print(f"Average Engine Count: {engine_avg:.1f}")
    
    # Best and worst
    print("\n✅ Best Results:")
    best = sorted(results, key=lambda x: x["overall"], reverse=True)[:3]
    for r in best:
        print(f"   {r['overall']:.1f}/10: '{r['name']}' - {r['prompt'][:40]}...")
    
    print("\n❌ Worst Results:")
    worst = sorted(results, key=lambda x: x["overall"])[:3]
    for r in worst:
        if r["overall"] > 0:
            print(f"   {r['overall']:.1f}/10: '{r['name']}' - {r['prompt'][:40]}...")
    
    # Success criteria
    print("\n" + "=" * 80)
    if overall_avg >= 7:
        print("🎉 SUCCESS! Visionary pipeline is working well!")
    elif overall_avg >= 5:
        print("👍 BETTER but still needs improvement")
    else:
        print("❌ NEEDS WORK - Significant issues remain")
    
    return results

def rate_name(name: str, prompt: str, expected: str) -> int:
    """Rate name quality 0-10"""
    score = 5  # Base score
    
    name_lower = name.lower()
    prompt_lower = prompt.lower()
    
    # Check for bad names
    if any(x in name_lower for x in ["safe default", "unknown", "custom preset", "sonic"]):
        return 2
    
    # Check for expected keywords
    expected_words = expected.split(", ")
    for word in expected_words:
        if word in name_lower or word in prompt_lower and any(w in name_lower for w in prompt_lower.split()):
            score += 1
    
    # Bonus for creative names
    if len(name.split()) >= 2 and len(name) > 10:
        score += 1
    
    # Check for relevance
    prompt_words = prompt_lower.split()
    relevant_words = ["guitar", "bass", "vocal", "drum", "piano", "synth", 
                      "edm", "country", "jazz", "metal", "techno",
                      "vintage", "modern", "warm", "bright", "dark"]
    
    for word in relevant_words:
        if word in prompt_lower and word in name_lower:
            score += 1
    
    return min(10, score)

def rate_engine_count(count: int) -> int:
    """Rate engine count 0-10"""
    if count == 0:
        return 0
    elif count <= 2:
        return 5
    elif count <= 4:
        return 10
    elif count <= 5:
        return 8
    else:
        return 6  # Too many

if __name__ == "__main__":
    results = test_visionary_pipeline()