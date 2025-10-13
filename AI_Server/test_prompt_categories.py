#!/usr/bin/env python3
"""
Test the Trinity Pipeline with various prompt categories
"""

import asyncio
import json
import aiohttp
import time
from typing import Dict, List, Any

# Define test prompts in three categories
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

async def test_prompt(session: aiohttp.ClientSession, prompt: str) -> Dict[str, Any]:
    """Test a single prompt"""
    url = "http://localhost:8000/generate"
    
    try:
        async with session.post(url, json={"prompt": prompt}, timeout=aiohttp.ClientTimeout(total=30)) as response:
            if response.status == 200:
                data = await response.json()
                return {
                    "prompt": prompt,
                    "success": data.get("success", False),
                    "preset_name": data["preset"]["name"] if "preset" in data else "ERROR",
                    "engines": [
                        {"id": s["engine_id"], "name": s["engine_name"]} 
                        for s in data.get("preset", {}).get("slots", [])
                        if s.get("engine_id", 0) != 0
                    ],
                    "engine_count": len([s for s in data.get("preset", {}).get("slots", []) if s.get("engine_id", 0) != 0]),
                    "error": None
                }
            else:
                return {
                    "prompt": prompt,
                    "success": False,
                    "error": f"HTTP {response.status}"
                }
    except asyncio.TimeoutError:
        return {
            "prompt": prompt,
            "success": False,
            "error": "Timeout"
        }
    except Exception as e:
        return {
            "prompt": prompt,
            "success": False,
            "error": str(e)
        }

def judge_result(result: Dict[str, Any], category: str) -> Dict[str, Any]:
    """Judge the quality of a result"""
    score = 0
    feedback = []
    
    if not result["success"]:
        return {
            "score": 0,
            "grade": "F",
            "feedback": [f"Failed: {result.get('error', 'Unknown error')}"]
        }
    
    # Check engine count (should be 4+)
    engine_count = result["engine_count"]
    if engine_count >= 4:
        score += 25
        feedback.append(f"✅ Good engine count ({engine_count})")
    elif engine_count >= 2:
        score += 15
        feedback.append(f"⚠️ Low engine count ({engine_count})")
    else:
        feedback.append(f"❌ Too few engines ({engine_count})")
    
    # Check preset name creativity
    preset_name = result.get("preset_name", "")
    prompt_words = set(result["prompt"].lower().split())
    preset_words = set(preset_name.lower().split())
    
    if preset_name and preset_name != "Generated Preset":
        if len(preset_words & prompt_words) < len(preset_words) / 2:
            score += 15
            feedback.append(f"✅ Creative name: '{preset_name}'")
        else:
            score += 5
            feedback.append(f"⚠️ Generic name: '{preset_name}'")
    else:
        feedback.append(f"❌ No creative name")
    
    # Category-specific judging
    engines = result.get("engines", [])
    engine_names_lower = [e["name"].lower() for e in engines]
    
    if category == "POETIC":
        # Should interpret creatively and select atmospheric engines
        reverb_count = sum(1 for name in engine_names_lower if "reverb" in name)
        mod_count = sum(1 for name in engine_names_lower if any(m in name for m in ["chorus", "phaser", "flanger"]))
        
        if reverb_count > 0:
            score += 20
            feedback.append(f"✅ Added reverb for atmosphere")
        if mod_count > 0:
            score += 20
            feedback.append(f"✅ Added modulation for movement")
        
        # Check for creative interpretation
        if any("shimmer" in n for n in engine_names_lower) and "ethereal" in result["prompt"]:
            score += 20
            feedback.append("✅ Excellent creative interpretation")
            
    elif category == "TECHNICAL":
        # Should select specifically mentioned technical effects
        prompt_lower = result["prompt"].lower()
        
        # Check for technical accuracy
        if "compression" in prompt_lower and any("compress" in n for n in engine_names_lower):
            score += 20
            feedback.append("✅ Correctly identified compression need")
        if "eq" in prompt_lower and any("eq" in n or "parametric" in n for n in engine_names_lower):
            score += 20
            feedback.append("✅ Correctly identified EQ need")
        if "delay" in prompt_lower and any("delay" in n or "echo" in n for n in engine_names_lower):
            score += 20
            feedback.append("✅ Correctly identified delay need")
        if "filter" in prompt_lower and any("filter" in n or "ladder" in n for n in engine_names_lower):
            score += 20
            feedback.append("✅ Correctly identified filter need")
            
    elif category == "ARTISTIC":
        # Should match the artistic style with appropriate engines
        prompt_lower = result["prompt"].lower()
        
        if "vintage" in prompt_lower and any("vintage" in n or "tube" in n or "tape" in n for n in engine_names_lower):
            score += 20
            feedback.append("✅ Matched vintage character")
        if "distort" in prompt_lower and any("distort" in n or "overdrive" in n for n in engine_names_lower):
            score += 20
            feedback.append("✅ Added appropriate distortion")
        if any(inst in prompt_lower for inst in ["drum", "bass", "guitar", "vocal"]):
            if any("compress" in n for n in engine_names_lower):
                score += 10
                feedback.append("✅ Added compression for instrument")
            if any("eq" in n for n in engine_names_lower):
                score += 10
                feedback.append("✅ Added EQ for instrument")
    
    # Determine grade
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

async def test_all_prompts():
    """Test all prompts and judge results"""
    
    async with aiohttp.ClientSession() as session:
        print("="*80)
        print("TESTING TRINITY PIPELINE WITH 30 PROMPTS")
        print("="*80)
        
        all_results = {}
        
        # Test each category
        for category, prompts in [
            ("VAGUE POETIC", VAGUE_POETIC_PROMPTS),
            ("TECHNICAL", TECHNICAL_PROMPTS),
            ("ARTISTIC", ARTISTIC_PROMPTS)
        ]:
            print(f"\n{'='*40}")
            print(f"{category} PROMPTS")
            print(f"{'='*40}")
            
            category_results = []
            category_scores = []
            
            for i, prompt in enumerate(prompts, 1):
                print(f"\n{i}. \"{prompt}\"")
                
                # Test the prompt
                result = await test_prompt(session, prompt)
                
                # Judge the result
                judgment = judge_result(result, category.split()[0])
                category_scores.append(judgment["score"])
                
                # Display results
                if result["success"]:
                    print(f"   → {result['preset_name']}")
                    engine_list = ", ".join([f"{e['name']} ({e['id']})" for e in result['engines']])
                    print(f"   Engines: {engine_list}")
                    print(f"   Grade: {judgment['grade']} ({judgment['score']}/100)")
                    for feedback in judgment["feedback"]:
                        print(f"     {feedback}")
                else:
                    print(f"   ❌ FAILED: {result.get('error', 'Unknown')}")
                
                category_results.append({
                    "prompt": prompt,
                    "result": result,
                    "judgment": judgment
                })
                
                # Small delay to avoid overwhelming the server
                await asyncio.sleep(0.5)
            
            # Category summary
            avg_score = sum(category_scores) / len(category_scores) if category_scores else 0
            print(f"\n{category} SUMMARY:")
            print(f"  Average Score: {avg_score:.1f}/100")
            print(f"  Grade Distribution:")
            grades = [r["judgment"]["grade"] for r in category_results]
            for grade in ["A", "B", "C", "D", "F"]:
                count = grades.count(grade)
                print(f"    {grade}: {count} ({count*100/len(grades):.0f}%)")
            
            all_results[category] = {
                "results": category_results,
                "average_score": avg_score
            }
        
        # Overall summary
        print("\n" + "="*80)
        print("OVERALL SUMMARY")
        print("="*80)
        
        total_score = 0
        total_count = 0
        
        for category, data in all_results.items():
            print(f"\n{category}:")
            print(f"  Average: {data['average_score']:.1f}/100")
            total_score += data['average_score'] * len(data['results'])
            total_count += len(data['results'])
            
            # Show best and worst
            sorted_results = sorted(data['results'], key=lambda x: x['judgment']['score'], reverse=True)
            if sorted_results:
                best = sorted_results[0]
                worst = sorted_results[-1]
                print(f"  Best: \"{best['prompt'][:40]}...\" → {best['result']['preset_name']} ({best['judgment']['grade']})")
                print(f"  Worst: \"{worst['prompt'][:40]}...\" → {worst['result'].get('preset_name', 'FAILED')} ({worst['judgment']['grade']})")
        
        overall_avg = total_score / total_count if total_count > 0 else 0
        print(f"\n🎯 OVERALL AVERAGE: {overall_avg:.1f}/100")
        
        if overall_avg >= 80:
            print("✅ EXCELLENT - System is working very well!")
        elif overall_avg >= 70:
            print("👍 GOOD - System is working reasonably well")
        elif overall_avg >= 60:
            print("⚠️ FAIR - System needs improvement")
        else:
            print("❌ POOR - System has significant issues")

if __name__ == "__main__":
    asyncio.run(test_all_prompts())