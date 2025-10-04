#!/usr/bin/env python3
"""
Comprehensive test of Trinity Pipeline with 30 prompts
Tests one at a time with detailed results
"""

import requests
import json
import time

# Define all 30 test prompts
VAGUE_POETIC_PROMPTS = [
    "ethereal whispers in the void",
    "crystalline dreams of tomorrow",
    "velvet thunder across midnight skies",
    "golden tears of autumn rain",
    "shadows dancing on moonbeams",
    "frozen echoes of lost time",
    "silk and steel harmonies",
    "liquid starlight cascades",
    "obsidian mirror reflections",
    "rose petals on broken glass"
]

TECHNICAL_PROMPTS = [
    "parallel compression with 4:1 ratio and slow attack",
    "high-pass filter at 80Hz with gentle Q",
    "tube saturation at 3rd harmonic with soft knee",
    "stereo widening with haas effect and phase correlation",
    "dynamic EQ with sidechain at 2-4kHz",
    "multiband compression with crossover at 200Hz and 2kHz",
    "tape delay at 1/8 dotted with feedback at 35%",
    "optical compressor with 2:1 ratio and auto-release",
    "harmonic exciter focusing on even harmonics",
    "brickwall limiter with -0.3dB ceiling and fast lookahead"
]

ARTISTIC_PROMPTS = [
    "vintage Beatles Abbey Road drum sound",
    "80s synthwave lead with neon glow",
    "Nashville country guitar twang",
    "Detroit techno warehouse bass",
    "Seattle grunge distorted vocals",
    "French house filtered disco loop",
    "Jazz club smoky saxophone",
    "Gothic cathedral organ grandeur",
    "Tropical reggaeton percussion groove",
    "Scandinavian black metal guitar wall"
]

def test_single_prompt(prompt):
    """Test a single prompt and return detailed results"""
    url = "http://localhost:8000/generate"
    
    try:
        response = requests.post(url, json={"prompt": prompt}, timeout=30)
        if response.status_code == 200:
            data = response.json()
            preset = data.get("preset", {})
            
            # Extract engines
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
                "preset_description": preset.get("description", ""),
                "engines": engines,
                "engine_count": len(engines),
                "raw_data": preset
            }
        else:
            return {
                "success": False,
                "error": f"HTTP {response.status_code}"
            }
    except Exception as e:
        return {
            "success": False,
            "error": str(e)
        }

def judge_result(prompt, result, category):
    """Comprehensive judgment of result quality"""
    
    if not result["success"]:
        return {
            "score": 0,
            "grade": "F",
            "feedback": [f"❌ Failed: {result.get('error', 'Unknown error')}"]
        }
    
    score = 0
    feedback = []
    
    # 1. Engine count (25 points)
    engine_count = result["engine_count"]
    if engine_count >= 4:
        score += 25
        feedback.append(f"✅ Good engine count: {engine_count} engines")
    elif engine_count == 3:
        score += 15
        feedback.append(f"⚠️ Borderline engine count: {engine_count} engines (min 4)")
    elif engine_count == 2:
        score += 10
        feedback.append(f"⚠️ Low engine count: {engine_count} engines")
    else:
        score += 5
        feedback.append(f"❌ Too few engines: {engine_count}")
    
    # 2. Creative naming (10 points)
    preset_name = result["preset_name"]
    prompt_words = set(prompt.lower().split())
    preset_words = set(preset_name.lower().split())
    
    if preset_name and preset_name != "Generated Preset":
        overlap = len(prompt_words & preset_words) / max(len(preset_words), 1)
        if overlap < 0.3:  # Very creative
            score += 10
            feedback.append(f"✅ Very creative name: '{preset_name}'")
        elif overlap < 0.5:  # Somewhat creative
            score += 7
            feedback.append(f"👍 Creative name: '{preset_name}'")
        else:  # Too literal
            score += 3
            feedback.append(f"⚠️ Literal name: '{preset_name}'")
    
    # 3. Category-specific evaluation (65 points)
    prompt_lower = prompt.lower()
    engine_names = [e["name"].lower() for e in result["engines"]]
    engine_ids = [e["id"] for e in result["engines"]]
    
    if category == "VAGUE_POETIC":
        # Check for atmospheric interpretation
        points = 0
        
        # Reverb presence (20 points)
        reverb_types = ["reverb", "shimmer", "plate", "spring", "hall"]
        if any(rt in " ".join(engine_names) for rt in reverb_types):
            points += 20
            feedback.append("✅ Added reverb for atmosphere")
            
        # Modulation (15 points)
        mod_types = ["chorus", "phaser", "flanger", "tremolo"]
        if any(mt in " ".join(engine_names) for mt in mod_types):
            points += 15
            feedback.append("✅ Added modulation for movement")
            
        # Creative interpretation (15 points)
        if "ethereal" in prompt_lower and 42 in engine_ids:  # Shimmer reverb
            points += 15
            feedback.append("✅ Perfect match: Shimmer for 'ethereal'")
        elif "thunder" in prompt_lower and any("distort" in n or "overdrive" in n for n in engine_names):
            points += 15
            feedback.append("✅ Good interpretation: Distortion for 'thunder'")
        elif "crystalline" in prompt_lower and any("delay" in n or "echo" in n for n in engine_names):
            points += 10
            feedback.append("👍 Good: Delay for 'crystalline'")
            
        # Delay/Echo (15 points)
        if any("delay" in n or "echo" in n for n in engine_names):
            points += 15
            feedback.append("✅ Added delay/echo for space")
            
        score += points
        
    elif category == "TECHNICAL":
        # Check for technical accuracy
        points = 0
        
        # Exact matches (40 points possible)
        if "compression" in prompt_lower:
            if any("compress" in n for n in engine_names):
                points += 20
                feedback.append("✅ Correctly identified compression")
                if "parallel" in prompt_lower and any(e["id"] in [1, 2] for e in result["engines"]):
                    points += 10
                    feedback.append("✅ Used appropriate compressor for parallel")
                    
        if "filter" in prompt_lower:
            if any("filter" in n or "eq" in n for n in engine_names):
                points += 20
                feedback.append("✅ Correctly identified filter/EQ need")
                    
        if "delay" in prompt_lower:
            if any("delay" in n or "echo" in n for n in engine_names):
                points += 20
                feedback.append("✅ Correctly identified delay")
                if "tape" in prompt_lower and 34 in engine_ids:
                    points += 5
                    feedback.append("✅ Perfect: Tape Echo for tape delay")
                    
        if "saturation" in prompt_lower or "tube" in prompt_lower:
            if any("tube" in n or "saturat" in n for n in engine_names):
                points += 15
                feedback.append("✅ Correctly identified saturation")
                
        if "limiter" in prompt_lower:
            if 5 in engine_ids:  # Mastering Limiter
                points += 10
                feedback.append("✅ Correctly selected limiter")
                
        score += min(points, 65)  # Cap at 65 points
        
    elif category == "ARTISTIC":
        # Check for style matching
        points = 0
        
        # Era-specific (20 points)
        if "vintage" in prompt_lower or "60s" in prompt_lower or "beatles" in prompt_lower:
            if any("vintage" in n or "tube" in n or "tape" in n for n in engine_names):
                points += 20
                feedback.append("✅ Matched vintage character")
                
        if "80s" in prompt_lower:
            if any("chorus" in n or "reverb" in n for n in engine_names):
                points += 20
                feedback.append("✅ Matched 80s character (chorus/reverb)")
                
        # Instrument-specific (20 points)
        if "drum" in prompt_lower:
            if any("compress" in n for n in engine_names) and any("transient" in n for n in engine_names):
                points += 20
                feedback.append("✅ Perfect drum processing")
            elif any("compress" in n for n in engine_names):
                points += 10
                feedback.append("👍 Added compression for drums")
                
        if "guitar" in prompt_lower:
            if any("overdrive" in n or "distort" in n for n in engine_names):
                points += 15
                feedback.append("✅ Added guitar distortion/overdrive")
            if "country" in prompt_lower and 40 in engine_ids:  # Spring reverb
                points += 10
                feedback.append("✅ Perfect: Spring reverb for country")
                
        if "bass" in prompt_lower:
            if any("compress" in n for n in engine_names):
                points += 10
                feedback.append("✅ Added compression for bass")
                
        # Genre-specific (25 points)
        if "metal" in prompt_lower:
            if any("distort" in n or "gate" in n for n in engine_names):
                points += 25
                feedback.append("✅ Matched metal sound (distortion/gate)")
                
        if "jazz" in prompt_lower:
            if any("reverb" in n for n in engine_names) and any("compress" in n for n in engine_names):
                points += 20
                feedback.append("✅ Jazz atmosphere (reverb + dynamics)")
                
        score += min(points, 65)  # Cap at 65 points
    
    # Determine final grade
    if score >= 90:
        grade = "A"
    elif score >= 80:
        grade = "B"
    elif score >= 70:
        grade = "C"
    elif score >= 60:
        grade = "D"
    else:
        grade = "F"
    
    return {
        "score": score,
        "grade": grade,
        "feedback": feedback
    }

# Main test execution
print("="*80)
print("COMPREHENSIVE TRINITY PIPELINE TEST")
print("Testing 30 prompts across 3 categories")
print("="*80)

all_results = []
category_scores = {"VAGUE_POETIC": [], "TECHNICAL": [], "ARTISTIC": []}
grade_distribution = {"A": 0, "B": 0, "C": 0, "D": 0, "F": 0}

# Test all prompts
test_num = 0
for category, prompts in [("VAGUE_POETIC", VAGUE_POETIC_PROMPTS), 
                          ("TECHNICAL", TECHNICAL_PROMPTS), 
                          ("ARTISTIC", ARTISTIC_PROMPTS)]:
    
    print(f"\n{'='*40}")
    print(f"{category} PROMPTS")
    print(f"{'='*40}")
    
    for prompt in prompts:
        test_num += 1
        print(f"\n[{test_num}/30] Testing: \"{prompt}\"")
        print("-"*60)
        
        # Test the prompt
        result = test_single_prompt(prompt)
        
        # Judge the result
        judgment = judge_result(prompt, result, category)
        
        # Store results
        all_results.append({
            "category": category,
            "prompt": prompt,
            "result": result,
            "judgment": judgment
        })
        
        category_scores[category].append(judgment["score"])
        grade_distribution[judgment["grade"]] += 1
        
        # Display results
        if result["success"]:
            print(f"✅ Success!")
            print(f"Preset Name: {result['preset_name']}")
            print(f"Engines ({result['engine_count']}):")
            for e in result["engines"]:
                print(f"  • {e['name']} (ID: {e['id']})")
            print(f"\nGrade: {judgment['grade']} ({judgment['score']}/100)")
            print("Feedback:")
            for fb in judgment["feedback"]:
                print(f"  {fb}")
        else:
            print(f"❌ FAILED: {result.get('error', 'Unknown')}")
        
        # Small delay between tests
        time.sleep(1)

# Final summary
print("\n" + "="*80)
print("FINAL RESULTS SUMMARY")
print("="*80)

# Category averages
for category in ["VAGUE_POETIC", "TECHNICAL", "ARTISTIC"]:
    scores = category_scores[category]
    if scores:
        avg = sum(scores) / len(scores)
        print(f"\n{category}:")
        print(f"  Average Score: {avg:.1f}/100")
        print(f"  Best: {max(scores)}/100")
        print(f"  Worst: {min(scores)}/100")

# Overall average
all_scores = [r["judgment"]["score"] for r in all_results]
if all_scores:
    overall_avg = sum(all_scores) / len(all_scores)
    print(f"\n🎯 OVERALL AVERAGE: {overall_avg:.1f}/100")
    
    # Grade distribution
    print("\nGrade Distribution:")
    total = sum(grade_distribution.values())
    for grade in ["A", "B", "C", "D", "F"]:
        count = grade_distribution[grade]
        pct = (count / total * 100) if total > 0 else 0
        bar = "█" * int(pct / 5)
        print(f"  {grade}: {count:2d} ({pct:5.1f}%) {bar}")
    
    # Final assessment
    print("\n📊 FINAL ASSESSMENT:")
    if overall_avg >= 80:
        print("✅ EXCELLENT - AI is highly intelligent and understands context well")
    elif overall_avg >= 70:
        print("👍 GOOD - AI performs well with most requests")
    elif overall_avg >= 60:
        print("⚠️ FAIR - AI needs improvement in interpretation")
    else:
        print("❌ POOR - AI has significant understanding issues")