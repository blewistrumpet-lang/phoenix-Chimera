#!/usr/bin/env python3
"""
Comprehensive pipeline analysis - identify areas for improvement
Focus on:
1. Naming creativity (too many "Sonic X" names)
2. Engine selection accuracy beyond required engines
3. Signal chain ordering
4. Overall preset quality
"""

import requests
import json
from collections import Counter, defaultdict
from engine_mapping_authoritative import ENGINE_NAMES
import time

def analyze_pipeline_performance():
    """Run comprehensive tests to identify improvement areas"""
    
    print("🔬 PIPELINE IMPROVEMENT ANALYSIS")
    print("=" * 80)
    
    # Diverse test prompts covering different genres and use cases
    test_prompts = [
        # Specific genre requests
        "Create a dark ambient horror soundtrack preset",
        "Modern trap 808 bass with heavy processing",
        "Classic 70s rock guitar tone like Led Zeppelin",
        "Ethereal dream pop vocals with lots of atmosphere",
        "Aggressive dubstep bass with heavy distortion",
        "Smooth jazz saxophone with natural reverb",
        "Cinematic orchestral strings with huge space",
        "Lo-fi bedroom pop vocals with tape warmth",
        "Synthwave lead with retro processing",
        "Folk acoustic guitar intimate and warm",
        
        # Technical requests
        "I need compression, EQ, and hall reverb",
        "Give me analog warmth with tube saturation",
        "Clean signal with just dynamics processing",
        "Maximum distortion and aggression",
        "Subtle modulation effects only",
        
        # Creative/vague requests
        "Make it sound like outer space",
        "Warm and fuzzy vintage vibes",
        "Crystal clear modern production",
        "Dirty and gritty underground sound",
        "Heavenly angelic atmosphere",
        
        # Specific engine requests
        "Use shimmer reverb and bit crusher together",
        "Classic chorus with tape echo please",
        "Vintage tube preamp into plate reverb",
        "Envelope filter and analog phaser combination",
        "Spring reverb with magnetic drum delay"
    ]
    
    results = []
    preset_names = []
    engine_usage = Counter()
    signal_chains = []
    name_patterns = defaultdict(int)
    
    for i, prompt in enumerate(test_prompts, 1):
        print(f"\nTest {i}/{len(test_prompts)}: {prompt[:50]}...")
        
        try:
            response = requests.post(
                "http://localhost:8000/generate",
                json={"prompt": prompt},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                metadata = data.get("metadata", {})
                
                # Collect preset name
                name = preset.get("name", "Unknown")
                preset_names.append(name)
                
                # Analyze name patterns
                if "Sonic" in name:
                    name_patterns["Sonic X"] += 1
                elif "Vintage" in name:
                    name_patterns["Vintage X"] += 1
                elif "Modern" in name:
                    name_patterns["Modern X"] += 1
                elif "Classic" in name:
                    name_patterns["Classic X"] += 1
                else:
                    name_patterns["Unique"] += 1
                
                # Collect engines used
                engines_in_preset = []
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        engines_in_preset.append(engine_id)
                        engine_usage[engine_id] += 1
                
                # Analyze signal chain
                signal_flow = preset.get("signal_flow", "")
                signal_chains.append(signal_flow)
                
                results.append({
                    "prompt": prompt,
                    "name": name,
                    "engines": engines_in_preset,
                    "num_engines": len(engines_in_preset),
                    "required_match": metadata.get("match_rate", 0),
                    "signal_flow": signal_flow
                })
                
        except Exception as e:
            print(f"   Error: {str(e)}")
            results.append({
                "prompt": prompt,
                "name": "ERROR",
                "engines": [],
                "num_engines": 0,
                "required_match": 0,
                "signal_flow": ""
            })
        
        time.sleep(0.3)  # Small delay between requests
    
    # ANALYSIS REPORT
    print("\n" + "=" * 80)
    print("📊 ANALYSIS REPORT")
    print("=" * 80)
    
    # 1. NAMING CREATIVITY ISSUE
    print("\n🏷️ PRESET NAMING ANALYSIS:")
    print("-" * 40)
    
    print("\nName Pattern Distribution:")
    total_names = len(preset_names)
    for pattern, count in sorted(name_patterns.items(), key=lambda x: -x[1]):
        percentage = (count / total_names) * 100
        print(f"  {pattern:15} {count:3} ({percentage:5.1f}%)")
    
    print("\nAll Generated Names:")
    name_counter = Counter(preset_names)
    for name, count in name_counter.most_common():
        if count > 1:
            print(f"  ⚠️ '{name}' appeared {count} times")
        else:
            print(f"  ✓ '{name}'")
    
    # Calculate naming diversity score
    unique_names = len(set(preset_names))
    naming_diversity = (unique_names / total_names) * 100
    print(f"\nNaming Diversity: {naming_diversity:.1f}% ({unique_names}/{total_names} unique)")
    
    # 2. ENGINE SELECTION PATTERNS
    print("\n🎛️ ENGINE USAGE ANALYSIS:")
    print("-" * 40)
    
    print("\nMost Used Engines:")
    for engine_id, count in engine_usage.most_common(10):
        engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
        percentage = (count / len(test_prompts)) * 100
        print(f"  {engine_name:30} {count:3} ({percentage:5.1f}%)")
    
    # Check for overuse of certain engines
    overused = [e for e, c in engine_usage.items() if c > len(test_prompts) * 0.5]
    if overused:
        print("\n⚠️ Potentially Overused Engines (>50% of presets):")
        for engine_id in overused:
            print(f"  - {ENGINE_NAMES.get(engine_id)}")
    
    # 3. SIGNAL CHAIN ANALYSIS
    print("\n📊 SIGNAL CHAIN PATTERNS:")
    print("-" * 40)
    
    # Check signal chain consistency
    chain_patterns = Counter(signal_chains)
    if len(chain_patterns) < len(test_prompts) * 0.5:
        print("⚠️ Low signal chain diversity detected")
    else:
        print("✓ Good signal chain diversity")
    
    # 4. ENGINE COUNT DISTRIBUTION
    print("\n📈 ENGINE COUNT PER PRESET:")
    print("-" * 40)
    
    engine_counts = Counter([r["num_engines"] for r in results])
    for count, freq in sorted(engine_counts.items()):
        print(f"  {count} engines: {freq} presets")
    
    avg_engines = sum(r["num_engines"] for r in results) / len(results)
    print(f"\nAverage engines per preset: {avg_engines:.1f}")
    
    # 5. ACCURACY METRICS
    print("\n🎯 ACCURACY METRICS:")
    print("-" * 40)
    
    required_matches = [r["required_match"] for r in results if r["required_match"] > 0]
    if required_matches:
        avg_match = sum(required_matches) / len(required_matches)
        print(f"Average required engine match rate: {avg_match:.1f}%")
    
    # 6. IDENTIFIED ISSUES
    print("\n⚠️ IDENTIFIED ISSUES:")
    print("-" * 40)
    
    issues = []
    
    if naming_diversity < 80:
        issues.append(f"Low naming diversity ({naming_diversity:.1f}%) - too many duplicate names")
    
    if name_patterns.get("Sonic X", 0) > total_names * 0.2:
        issues.append(f"Overuse of 'Sonic X' pattern ({name_patterns['Sonic X']}/{total_names})")
    
    if overused:
        issues.append(f"{len(overused)} engines appear in >50% of presets")
    
    if avg_engines < 3 or avg_engines > 5:
        issues.append(f"Unusual average engine count: {avg_engines:.1f}")
    
    for issue in issues:
        print(f"  • {issue}")
    
    # 7. RECOMMENDATIONS
    print("\n💡 RECOMMENDATIONS:")
    print("-" * 40)
    
    recommendations = []
    
    if naming_diversity < 80:
        recommendations.append("Improve Alchemist's name generation - it's defaulting to templates")
        recommendations.append("Check if Cloud AI creative names are being properly used")
    
    if name_patterns.get("Sonic X", 0) > total_names * 0.2:
        recommendations.append("Fix the fallback naming in Alchemist - remove 'Sonic' default")
        recommendations.append("Ensure genre-specific naming is working")
    
    if overused:
        recommendations.append("Oracle may be biased toward certain presets")
        recommendations.append("Check if Calculator is adding unnecessary engines")
    
    for rec in recommendations:
        print(f"  → {rec}")
    
    # 8. SUCCESS CRITERIA
    print("\n" + "=" * 80)
    print("🏆 OVERALL ASSESSMENT:")
    
    success_count = 0
    total_criteria = 4
    
    if naming_diversity >= 80:
        print("✅ Naming diversity: PASS")
        success_count += 1
    else:
        print(f"❌ Naming diversity: FAIL ({naming_diversity:.1f}% < 80%)")
    
    if name_patterns.get("Sonic X", 0) <= total_names * 0.2:
        print("✅ Name pattern variety: PASS")
        success_count += 1
    else:
        print(f"❌ Name pattern variety: FAIL (too many Sonic names)")
    
    if not overused:
        print("✅ Engine distribution: PASS")
        success_count += 1
    else:
        print(f"❌ Engine distribution: FAIL ({len(overused)} overused engines)")
    
    if 3 <= avg_engines <= 5:
        print("✅ Engine count balance: PASS")
        success_count += 1
    else:
        print(f"❌ Engine count balance: FAIL (avg {avg_engines:.1f})")
    
    print(f"\nOVERALL SCORE: {success_count}/{total_criteria} criteria met")
    
    if success_count == total_criteria:
        print("🎉 Pipeline performing excellently!")
    elif success_count >= 3:
        print("👍 Pipeline performing well, minor improvements needed")
    else:
        print("⚠️ Pipeline needs significant improvements")
    
    return results, issues, recommendations

if __name__ == "__main__":
    results, issues, recommendations = analyze_pipeline_performance()