#!/usr/bin/env python3
"""
Quick test of prompt categories - 3 from each
"""

import requests
import json
import time

# Sample prompts from each category
TEST_PROMPTS = {
    "VAGUE_POETIC": [
        "ethereal whispers in the void",
        "crystalline dreams of tomorrow",
        "velvet thunder across midnight skies"
    ],
    "TECHNICAL": [
        "parallel compression with 4:1 ratio and slow attack",
        "tape delay at 1/8 dotted with feedback at 35%",
        "high-pass filter at 80Hz with gentle Q"
    ],
    "ARTISTIC": [
        "vintage Beatles Abbey Road drum sound",
        "80s synthwave lead with neon glow",
        "Gothic cathedral organ grandeur"
    ]
}

def test_prompt(prompt):
    """Test a single prompt"""
    url = "http://localhost:8000/generate"
    
    try:
        response = requests.post(url, json={"prompt": prompt}, timeout=30)
        if response.status_code == 200:
            data = response.json()
            preset = data.get("preset", {})
            engines = [
                f"{s['engine_name']} ({s['engine_id']})" 
                for s in preset.get("slots", [])
                if s.get("engine_id", 0) != 0
            ]
            return {
                "success": True,
                "preset_name": preset.get("name", "Unknown"),
                "engines": engines,
                "engine_count": len(engines)
            }
    except Exception as e:
        return {"success": False, "error": str(e)}
    
    return {"success": False, "error": "Unknown"}

def judge_result(prompt, result, category):
    """Quick judgment of result"""
    if not result["success"]:
        return "❌ FAILED", []
    
    feedback = []
    score = 0
    
    # Check engine count
    if result["engine_count"] >= 4:
        score += 40
        feedback.append(f"✅ {result['engine_count']} engines")
    else:
        score += 20
        feedback.append(f"⚠️ Only {result['engine_count']} engines")
    
    # Check for specific engines based on prompt
    prompt_lower = prompt.lower()
    engines_str = " ".join(result["engines"]).lower()
    
    # Category-specific checks
    if category == "VAGUE_POETIC":
        if any(r in engines_str for r in ["reverb", "shimmer", "plate"]):
            score += 30
            feedback.append("✅ Added reverb")
        if any(m in engines_str for m in ["chorus", "phaser", "flanger"]):
            score += 20
            feedback.append("✅ Added modulation")
            
    elif category == "TECHNICAL":
        if "compression" in prompt_lower and "compress" in engines_str:
            score += 30
            feedback.append("✅ Found compression")
        if "delay" in prompt_lower and ("delay" in engines_str or "echo" in engines_str):
            score += 30
            feedback.append("✅ Found delay")
        if "filter" in prompt_lower and ("filter" in engines_str or "eq" in engines_str):
            score += 30
            feedback.append("✅ Found filter/EQ")
            
    elif category == "ARTISTIC":
        if "vintage" in prompt_lower and any(v in engines_str for v in ["vintage", "tube", "tape"]):
            score += 30
            feedback.append("✅ Vintage character")
        if "80s" in prompt_lower and any(s in engines_str for s in ["chorus", "reverb", "delay"]):
            score += 20
            feedback.append("✅ 80s effects")
        if "cathedral" in prompt_lower and "reverb" in engines_str:
            score += 30
            feedback.append("✅ Cathedral reverb")
    
    # Creative name check
    if result["preset_name"] and result["preset_name"] != "Generated Preset":
        score += 10
        feedback.append(f"✅ Creative name: {result['preset_name']}")
    
    # Grade
    if score >= 80:
        grade = "A"
    elif score >= 70:
        grade = "B"
    elif score >= 60:
        grade = "C"
    elif score >= 50:
        grade = "D"
    else:
        grade = "F"
    
    return f"{grade} ({score}%)", feedback

print("="*80)
print("TRINITY PIPELINE PROMPT TEST - QUICK VERSION")
print("="*80)

all_scores = []
all_grades = {"A": 0, "B": 0, "C": 0, "D": 0, "F": 0}

for category, prompts in TEST_PROMPTS.items():
    print(f"\n{category}")
    print("-"*40)
    
    category_scores = []
    
    for i, prompt in enumerate(prompts, 1):
        print(f"\n{i}. \"{prompt}\"")
        
        # Test the prompt
        result = test_prompt(prompt)
        
        # Judge it
        grade_str, feedback = judge_result(prompt, result, category)
        grade = grade_str.split()[0]
        
        if result["success"]:
            print(f"   → {result['preset_name']}")
            print(f"   Engines: {', '.join(result['engines'][:3])}{'...' if len(result['engines']) > 3 else ''}")
            print(f"   Grade: {grade_str}")
            for fb in feedback[:3]:  # Show first 3 feedback items
                print(f"     {fb}")
            
            # Extract score
            score = int(grade_str.split("(")[1].split("%")[0])
            category_scores.append(score)
            all_scores.append(score)
            all_grades[grade] = all_grades.get(grade, 0) + 1
        else:
            print(f"   ❌ FAILED: {result.get('error', 'Unknown')}")
            all_grades["F"] = all_grades.get("F", 0) + 1
        
        time.sleep(0.5)  # Small delay
    
    if category_scores:
        avg = sum(category_scores) / len(category_scores)
        print(f"\n{category} Average: {avg:.0f}%")

# Overall summary
print("\n" + "="*80)
print("OVERALL SUMMARY")
print("="*80)

if all_scores:
    overall_avg = sum(all_scores) / len(all_scores)
    print(f"\n🎯 Overall Average: {overall_avg:.0f}%")
    
    print("\nGrade Distribution:")
    for grade in ["A", "B", "C", "D", "F"]:
        count = all_grades[grade]
        total = sum(all_grades.values())
        pct = (count / total * 100) if total > 0 else 0
        bar = "█" * int(pct / 5)
        print(f"  {grade}: {count:2d} ({pct:3.0f}%) {bar}")
    
    print("\n📊 Assessment:")
    if overall_avg >= 80:
        print("✅ EXCELLENT - System is highly intelligent and responsive")
    elif overall_avg >= 70:
        print("👍 GOOD - System understands most requests well")
    elif overall_avg >= 60:
        print("⚠️ FAIR - System needs some improvement")
    else:
        print("❌ POOR - System has significant issues")
else:
    print("No successful tests completed!")