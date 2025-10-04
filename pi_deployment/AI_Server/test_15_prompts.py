#!/usr/bin/env python3
"""
Test 15 diverse prompts to evaluate 4-engine minimum enforcement
"""

import requests
import json
import time
from datetime import datetime

TEST_PROMPTS = [
    # Poetic/Abstract (5)
    ("POETIC", "ethereal whispers in the void"),
    ("POETIC", "crystalline dreams of tomorrow"),
    ("POETIC", "velvet thunder across midnight skies"),
    ("POETIC", "liquid starlight cascades"),
    ("POETIC", "obsidian mirror reflections"),
    
    # Technical/Specific (5)
    ("TECHNICAL", "parallel compression with 4:1 ratio"),
    ("TECHNICAL", "tape delay at 1/8 dotted with 35% feedback"),
    ("TECHNICAL", "tube saturation at 3rd harmonic with soft knee"),
    ("TECHNICAL", "multiband compression with crossover at 200Hz and 2kHz"),
    ("TECHNICAL", "brickwall limiter with -0.3dB ceiling"),
    
    # Artistic/Genre (5)
    ("ARTISTIC", "vintage Beatles Abbey Road drums"),
    ("ARTISTIC", "80s synthwave lead with neon glow"),
    ("ARTISTIC", "Nashville country guitar twang"),
    ("ARTISTIC", "Detroit techno warehouse bass"),
    ("ARTISTIC", "Gothic cathedral organ grandeur")
]

def test_prompt(prompt):
    """Test a single prompt"""
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
                "description": preset.get("description", ""),
                "engines": engines,
                "engine_count": len(engines),
                "debug": data.get("debug", {})
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

def analyze_engine_selection(prompt, engines):
    """Analyze if engine selection matches prompt intent"""
    prompt_lower = prompt.lower()
    engine_names_lower = [e["name"].lower() for e in engines]
    
    matches = []
    
    # Check for specific matches
    if "compression" in prompt_lower or "compressor" in prompt_lower:
        if any("compress" in n for n in engine_names_lower):
            matches.append("✓ Compression")
    
    if "delay" in prompt_lower:
        if any("delay" in n or "echo" in n for n in engine_names_lower):
            matches.append("✓ Delay/Echo")
    
    if "tape" in prompt_lower:
        if any("tape" in n for n in engine_names_lower):
            matches.append("✓ Tape")
    
    if "reverb" in prompt_lower or "cathedral" in prompt_lower:
        if any("reverb" in n for n in engine_names_lower):
            matches.append("✓ Reverb")
    
    if "vintage" in prompt_lower or "beatles" in prompt_lower or "60s" in prompt_lower:
        if any("vintage" in n or "tube" in n or "opto" in n for n in engine_names_lower):
            matches.append("✓ Vintage")
    
    if "80s" in prompt_lower or "synthwave" in prompt_lower:
        if any("chorus" in n or "phaser" in n for n in engine_names_lower):
            matches.append("✓ 80s Character")
    
    if "ethereal" in prompt_lower or "shimmer" in prompt_lower:
        if any("shimmer" in n or "freeze" in n or "frequency shift" in n for n in engine_names_lower):
            matches.append("✓ Ethereal")
    
    return matches

print("="*80)
print("15-PROMPT COMPREHENSIVE TEST")
print(f"Started: {datetime.now().strftime('%H:%M:%S')}")
print("="*80)

results = []
passing = 0
failing = 0
category_stats = {"POETIC": [], "TECHNICAL": [], "ARTISTIC": []}

for i, (category, prompt) in enumerate(TEST_PROMPTS, 1):
    print(f"\n[{i}/15] {category}: \"{prompt}\"")
    print("-"*60)
    
    result = test_prompt(prompt)
    
    if result["success"]:
        engine_count = result["engine_count"]
        
        # Check 4-engine minimum
        if engine_count >= 4:
            status = f"✅ PASS ({engine_count} engines)"
            passing += 1
            category_stats[category].append(1)
        else:
            status = f"❌ FAIL ({engine_count} engines - MIN 4 REQUIRED)"
            failing += 1
            category_stats[category].append(0)
        
        print(f"Status: {status}")
        print(f"Preset: \"{result['preset_name']}\"")
        
        # Show engines
        print(f"Engines:")
        for engine in result['engines']:
            print(f"  • {engine['name']} (ID: {engine['id']})")
        
        # Analyze matches
        matches = analyze_engine_selection(prompt, result['engines'])
        if matches:
            print(f"Intent Matches: {', '.join(matches)}")
        
    else:
        print(f"❌ ERROR: {result['error']}")
        failing += 1
        category_stats[category].append(0)
    
    results.append({
        "category": category,
        "prompt": prompt,
        "result": result
    })
    
    time.sleep(1)  # Avoid overwhelming the server

# Summary
print("\n" + "="*80)
print("SUMMARY")
print("="*80)

print(f"\n📊 OVERALL RESULTS:")
print(f"  ✅ Passing (4+ engines): {passing}/15 ({passing/15*100:.0f}%)")
print(f"  ❌ Failing (<4 engines): {failing}/15 ({failing/15*100:.0f}%)")

print(f"\n📈 BY CATEGORY:")
for category in ["POETIC", "TECHNICAL", "ARTISTIC"]:
    scores = category_stats[category]
    if scores:
        success_rate = sum(scores) / len(scores) * 100
        print(f"  {category}: {sum(scores)}/{len(scores)} passed ({success_rate:.0f}%)")

# Engine frequency analysis
print(f"\n🔧 ENGINE USAGE FREQUENCY:")
engine_freq = {}
for result in results:
    if result["result"].get("success"):
        for engine in result["result"]["engines"]:
            engine_freq[engine["name"]] = engine_freq.get(engine["name"], 0) + 1

top_engines = sorted(engine_freq.items(), key=lambda x: x[1], reverse=True)[:10]
for engine, count in top_engines:
    print(f"  {engine}: {count}x")

# Final assessment
print(f"\n🎯 ASSESSMENT:")
success_rate = passing / 15 * 100
if success_rate >= 90:
    print("✅ EXCELLENT - 4-engine minimum consistently enforced")
elif success_rate >= 75:
    print("👍 GOOD - Mostly meeting requirements")
elif success_rate >= 60:
    print("⚠️ FAIR - Needs improvement")
else:
    print("❌ POOR - Significant issues with enforcement")

print(f"\nCompleted: {datetime.now().strftime('%H:%M:%S')}")