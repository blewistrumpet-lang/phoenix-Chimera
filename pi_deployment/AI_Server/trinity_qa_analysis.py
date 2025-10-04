#!/usr/bin/env python3
"""
Trinity QA Analysis - Detailed examination of quality issues
"""

import json
import sys
from pathlib import Path
from collections import Counter

# Add AI_Server to path
sys.path.insert(0, str(Path(__file__).parent))

from engine_mapping_authoritative import ENGINE_NAMES, get_engine_name, get_engine_category

def analyze_qa_results():
    """Analyze the QA test results in detail"""
    
    # Load the QA report
    report_path = Path("trinity_qa_report.json")
    if not report_path.exists():
        print("ERROR: trinity_qa_report.json not found. Run trinity_qa_comprehensive.py first.")
        return
    
    with open(report_path, 'r') as f:
        report = json.load(f)
    
    print("=" * 80)
    print("TRINITY QUALITY ASSURANCE - DETAILED ANALYSIS")
    print("=" * 80)
    
    # Analyze engine selection issues
    print("\n🔍 ENGINE SELECTION ANALYSIS:")
    print("-" * 50)
    
    engine_usage = Counter()
    category_usage = Counter()
    
    for result in report["detailed_results"]:
        if result["success"]:
            for engine_name in result["active_engines"]:
                engine_usage[engine_name] += 1
                # Find engine ID to get category
                for eid, ename in ENGINE_NAMES.items():
                    if ename == engine_name:
                        category = get_engine_category(eid)
                        category_usage[category] += 1
                        break
    
    print("Most used engines:")
    for engine, count in engine_usage.most_common(10):
        print(f"  {engine}: {count} times")
    
    print("\nCategory distribution:")
    for category, count in category_usage.most_common():
        print(f"  {category}: {count} engines")
    
    # Analyze problematic cases
    print("\n⚠️  PROBLEMATIC TEST CASES:")
    print("-" * 50)
    
    low_engine_scores = []
    for result in report["detailed_results"]:
        if result["success"] and result["quality_scores"]["engine_selection"] < 0.3:
            low_engine_scores.append(result)
    
    print(f"Tests with poor engine selection ({len(low_engine_scores)}/{len(report['detailed_results'])}):")
    for result in low_engine_scores[:5]:  # Show first 5
        print(f"  '{result['prompt']}' -> {result['active_engines']}")
        print(f"    Score: {result['quality_scores']['engine_selection']:.3f}")
    
    # Analyze name relevance issues
    print("\n📝 NAME RELEVANCE ANALYSIS:")
    print("-" * 50)
    
    low_name_scores = []
    for result in report["detailed_results"]:
        if result["success"] and result["quality_scores"]["name_relevance"] < 0.3:
            low_name_scores.append(result)
    
    print(f"Tests with poor name relevance ({len(low_name_scores)}/{len(report['detailed_results'])}):")
    for result in low_name_scores[:5]:  # Show first 5
        print(f"  '{result['prompt']}' -> '{result['preset_name']}'")
        print(f"    Score: {result['quality_scores']['name_relevance']:.3f}")
    
    # Golden corpus comparison issues
    print("\n🏆 GOLDEN CORPUS COMPARISON:")
    print("-" * 50)
    
    corpus_comp = report["corpus_comparison"]
    print(f"Corpus engines: {corpus_comp['corpus_stats']['unique_engines']}")
    print(f"Generated engines: {corpus_comp['generated_stats']['unique_engines']}")
    print(f"Engine diversity ratio: {corpus_comp['comparison']['engine_diversity_ratio']:.3f}")
    
    if corpus_comp['corpus_stats']['unique_engines'] == 0:
        print("WARNING: Golden corpus appears to have no engine data!")
    
    # Best performing tests
    print("\n🌟 BEST PERFORMING TESTS:")
    print("-" * 50)
    
    best_tests = sorted(
        [r for r in report["detailed_results"] if r["success"]],
        key=lambda x: x["quality_scores"]["overall"],
        reverse=True
    )[:5]
    
    for result in best_tests:
        print(f"  '{result['prompt']}' -> '{result['preset_name']}'")
        print(f"    Overall: {result['quality_scores']['overall']:.3f}")
        print(f"    Engines: {result['active_engines']}")
    
    # Performance metrics
    print("\n⚡ PERFORMANCE ANALYSIS:")
    print("-" * 50)
    
    times = [r["generation_time"] for r in report["detailed_results"] if r["success"]]
    print(f"Fastest generation: {min(times):.2f}s")
    print(f"Slowest generation: {max(times):.2f}s")
    print(f"Average generation: {sum(times)/len(times):.2f}s")
    
    # Recommendations summary
    print("\n💡 SPECIFIC RECOMMENDATIONS:")
    print("-" * 50)
    
    print("1. ENGINE SELECTION ISSUES:")
    print("   - Trinity is selecting utility engines (Phase Align, Gain Utility) instead of creative ones")
    print("   - Visionary/Oracle not mapping prompts to appropriate audio effects")
    print("   - Need better keyword-to-engine mapping in blueprint generation")
    
    print("\n2. NAME RELEVANCE ISSUES:")
    print("   - Generated names don't match prompt keywords well")
    print("   - Alchemist name generation needs improvement")
    print("   - Consider using prompt words directly in naming")
    
    print("\n3. GOLDEN CORPUS ISSUES:")
    print("   - Corpus comparison showing zero engines suggests data loading issue")
    print("   - Oracle may not be properly analyzing corpus engine usage")
    print("   - Need to verify corpus data integrity")
    
    print("\n4. PRIORITY FIXES:")
    print("   - Fix Visionary to select more musical engines")
    print("   - Improve Oracle corpus analysis")
    print("   - Enhance Alchemist name generation")
    print("   - Add better prompt-to-engine keyword mapping")

if __name__ == "__main__":
    analyze_qa_results()