#!/usr/bin/env python3
"""Test the training results with real prompts"""

import requests
import json
from pathlib import Path

# Load the optimized config
with open("best_electronic_config.json", "r") as f:
    optimized_config = json.load(f)

print("\n" + "="*80)
print("TESTING TRAINING RESULTS - 10 GENERATIONS")
print("="*80)

# Test prompts covering different electronic genres
test_prompts = [
    "deep dubstep bass wobble with sub frequencies",
    "glitchy IDM percussion chopped and stuttered", 
    "ethereal ambient pad with shimmer reverb",
    "trap 808 bass with distortion and punch",
    "acid techno 303 squelchy filter sweeps",
    "lo-fi hip hop warm vintage character",
    "neurofunk bass growl metallic aggressive",
    "future garage atmospheric spacey texture"
]

print("\n📊 BASELINE (Before Training):")
print("-" * 80)

baseline_results = []
for prompt in test_prompts[:4]:
    try:
        response = requests.post(
            "http://localhost:8000/generate",
            json={"prompt": prompt},
            timeout=10
        )
        if response.status_code == 200:
            data = response.json()
            if data["success"]:
                preset = data["preset"]
                params = preset.get("parameters", {})
                
                # Count engines
                engines = []
                from engine_mapping_authoritative import get_engine_name
                for slot in range(1, 7):
                    engine_id = params.get(f"slot{slot}_engine", 0)
                    if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                        engines.append(get_engine_name(engine_id))
                
                print(f"\nPrompt: '{prompt[:40]}...'")
                print(f"  Name: {preset.get('name', 'Unknown')}")
                print(f"  Engines ({len(engines)}): {', '.join(engines[:3])}...")
                
                baseline_results.append({
                    "prompt": prompt,
                    "name": preset.get("name"),
                    "engine_count": len(engines),
                    "engines": engines
                })
    except Exception as e:
        print(f"  Error: {e}")

print("\n📊 OPTIMIZED (After 10 Generations):")
print("-" * 80)

optimized_results = []
for prompt in test_prompts[:4]:
    try:
        response = requests.post(
            "http://localhost:8000/generate",
            json={"prompt": prompt, "config_override": optimized_config},
            timeout=10
        )
        if response.status_code == 200:
            data = response.json()
            if data["success"]:
                preset = data["preset"]
                params = preset.get("parameters", {})
                
                # Count engines
                engines = []
                for slot in range(1, 7):
                    engine_id = params.get(f"slot{slot}_engine", 0)
                    if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                        engines.append(get_engine_name(engine_id))
                
                print(f"\nPrompt: '{prompt[:40]}...'")
                print(f"  Name: {preset.get('name', 'Unknown')}")
                print(f"  Engines ({len(engines)}): {', '.join(engines[:3])}...")
                
                optimized_results.append({
                    "prompt": prompt,
                    "name": preset.get("name"),
                    "engine_count": len(engines),
                    "engines": engines
                })
    except Exception as e:
        print(f"  Error: {e}")

# Analysis
print("\n" + "="*80)
print("ANALYSIS:")
print("="*80)

if baseline_results and optimized_results:
    baseline_avg_engines = sum(r["engine_count"] for r in baseline_results) / len(baseline_results)
    optimized_avg_engines = sum(r["engine_count"] for r in optimized_results) / len(optimized_results)
    
    print(f"\n📈 Engine Count:")
    print(f"  Baseline: {baseline_avg_engines:.1f} engines/preset")
    print(f"  Optimized: {optimized_avg_engines:.1f} engines/preset")
    print(f"  Target: ≤5 engines (you wanted room for manual addition)")
    
    # Check for improvements
    improvements = []
    issues = []
    
    if optimized_avg_engines <= 5:
        improvements.append("✅ Respecting 5-engine limit")
    else:
        issues.append("❌ Still exceeding 5-engine limit")
    
    # Check engine selection relevance
    print(f"\n🎯 Engine Selection Quality:")
    for i, (opt, base) in enumerate(zip(optimized_results, baseline_results)):
        prompt_keywords = opt["prompt"].lower().split()
        
        # Check for relevant engines
        relevant = False
        if "bass" in prompt_keywords and any("bass" in e.lower() or "sub" in e.lower() for e in opt["engines"]):
            relevant = True
        if "reverb" in prompt_keywords and any("reverb" in e.lower() for e in opt["engines"]):
            relevant = True
        if "distortion" in prompt_keywords and any("distort" in e.lower() or "overdrive" in e.lower() for e in opt["engines"]):
            relevant = True
        
        print(f"  '{opt['prompt'][:30]}...': {'✓' if relevant else '✗'} relevant")
    
    print(f"\n📊 Configuration Changes:")
    print(f"  Oracle engine_weight: 10.0 → {optimized_config['oracle']['engine_weight']:.1f}")
    print(f"  Visionary creativity: 0.7 → {optimized_config['visionary']['creativity_bias']:.2f}")
    print(f"  Calculator sensitivity: 1.5 → {optimized_config['calculator']['keyword_sensitivity']:.2f}")
    
    print(f"\n🎯 Verdict:")
    if optimized_avg_engines < baseline_avg_engines and optimized_avg_engines <= 5:
        print("  ✅ SUCCESS - Training improved engine selection!")
    elif abs(optimized_avg_engines - baseline_avg_engines) < 0.5:
        print("  ⚠️ MARGINAL - Minimal improvement from training")
    else:
        print("  ❌ UNSUCCESSFUL - Training didn't improve performance")
else:
    print("  ❌ Could not complete tests")