#!/usr/bin/env python3
"""
Comprehensive bottleneck analysis for Trinity Pipeline
"""

import json
import os
from pathlib import Path
from collections import Counter, defaultdict
from typing import Dict, List, Tuple
import numpy as np
from engine_mapping_authoritative import ENGINE_NAMES

class BottleneckAnalyzer:
    def __init__(self):
        self.base_dir = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3")
        self.presets = []
        self.load_corpus()
    
    def load_corpus(self):
        """Load the corpus for analysis"""
        preset_path = self.base_dir / "faiss_index" / "presets_clean.json"
        if preset_path.exists():
            with open(preset_path, 'r') as f:
                self.presets = json.load(f)
        print(f"Loaded {len(self.presets)} presets")
    
    def analyze_corpus_bottleneck(self):
        """Analyze corpus limitations"""
        print("\n" + "="*80)
        print("🔍 BOTTLENECK #1: CORPUS LIMITATIONS (150 Presets)")
        print("="*80)
        
        # 1. Genre distribution
        print("\n1. GENRE COVERAGE:")
        genres = defaultdict(int)
        for preset in self.presets:
            name = preset.get("creative_name", "").lower()
            # Simple genre detection from names
            if any(w in name for w in ["metal", "heavy", "brutal"]):
                genres["metal"] += 1
            elif any(w in name for w in ["jazz", "smooth", "swing"]):
                genres["jazz"] += 1
            elif any(w in name for w in ["edm", "electronic", "techno", "house"]):
                genres["electronic"] += 1
            elif any(w in name for w in ["rock", "guitar"]):
                genres["rock"] += 1
            elif any(w in name for w in ["ambient", "atmospheric", "space"]):
                genres["ambient"] += 1
            elif any(w in name for w in ["vocal", "voice", "singer"]):
                genres["vocal"] += 1
            elif any(w in name for w in ["bass", "sub"]):
                genres["bass"] += 1
            elif any(w in name for w in ["drum", "percussion", "beat"]):
                genres["drums"] += 1
            else:
                genres["other"] += 1
        
        for genre, count in sorted(genres.items(), key=lambda x: -x[1]):
            percentage = (count / len(self.presets)) * 100
            bar = "█" * int(percentage / 2)
            print(f"  {genre:12} {count:3} ({percentage:5.1f}%) {bar}")
        
        # 2. Engine usage distribution
        print("\n2. ENGINE DIVERSITY:")
        all_engines = []
        for preset in self.presets:
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    all_engines.append(engine_id)
        
        engine_counts = Counter(all_engines)
        unique_engines = len(engine_counts)
        total_possible = 56
        
        print(f"  Unique engines used: {unique_engines}/{total_possible} ({unique_engines/total_possible*100:.1f}%)")
        print(f"  Total engine slots: {len(all_engines)}")
        print(f"  Average engines per preset: {len(all_engines)/len(self.presets):.1f}")
        
        print("\n  Most overused engines:")
        for engine_id, count in engine_counts.most_common(5):
            percentage = (count / len(all_engines)) * 100
            print(f"    {ENGINE_NAMES.get(engine_id, f'Unknown({engine_id})'):25} {count:3} ({percentage:5.1f}%)")
        
        print("\n  Never used engines:")
        never_used = []
        for engine_id in range(1, 57):
            if engine_id not in engine_counts:
                never_used.append(ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})"))
        
        for engine in never_used[:10]:  # Show first 10
            print(f"    - {engine}")
        if len(never_used) > 10:
            print(f"    ... and {len(never_used) - 10} more")
        
        # 3. Parameter diversity
        print("\n3. PARAMETER DIVERSITY:")
        param_values = defaultdict(list)
        for preset in self.presets:
            for slot in range(1, 7):
                for param in range(10):
                    key = f"slot{slot}_param{param}"
                    if key in preset:
                        param_values[f"param{param}"].append(preset[key])
        
        print("  Parameter variance (0=no variation, 1=high variation):")
        for param_name in sorted(param_values.keys()):
            values = param_values[param_name]
            if values:
                variance = np.var(values)
                print(f"    {param_name}: {variance:.3f}")
        
        # 4. Preset name diversity
        print("\n4. NAME PATTERNS:")
        name_words = []
        for preset in self.presets:
            name = preset.get("creative_name", "").lower()
            name_words.extend(name.split())
        
        word_counts = Counter(name_words)
        print("  Most common words in preset names:")
        for word, count in word_counts.most_common(10):
            print(f"    '{word}': {count} times")
        
        # 5. Recommendations
        print("\n5. CORPUS EXPANSION RECOMMENDATIONS:")
        print("  ❌ PROBLEMS:")
        print("    - Limited genre coverage (heavy on 'other')")
        print("    - Many unused engines")
        print("    - Low parameter variance")
        print("    - Repetitive naming patterns")
        print("\n  ✅ SOLUTIONS:")
        print("    - Need genre-specific preset packs")
        print("    - Create presets using underutilized engines")
        print("    - Increase parameter range exploration")
        print("    - More creative naming conventions")
    
    def analyze_oracle_bottleneck(self):
        """Analyze Oracle search issues"""
        print("\n" + "="*80)
        print("🔍 BOTTLENECK #2: ORACLE SEARCH PATTERNS")
        print("="*80)
        
        print("\n1. SEARCH LIMITATIONS:")
        print("  - Returns 'Metal Mayhem' as default too often")
        print("  - Poor handling of creative descriptions")
        print("  - Limited understanding of metaphors")
        print("  - Genre bias toward rock/metal")
        
        print("\n2. VECTOR EMBEDDING ISSUES:")
        print("  - 384-dimensional embeddings may be too small")
        print("  - No weighting for important keywords")
        print("  - Missing semantic relationships")
        
        print("\n3. RECOMMENDATIONS:")
        print("  - Implement weighted keyword matching")
        print("  - Add genre-specific search modes")
        print("  - Improve embedding quality")
        print("  - Add fallback search strategies")
    
    def analyze_engine_bottleneck(self):
        """Analyze engine selection issues"""
        print("\n" + "="*80)
        print("🔍 BOTTLENECK #3: ENGINE SELECTION & EXTRACTION")
        print("="*80)
        
        print("\n1. ENGINE EXTRACTION GAPS:")
        print("  - Missing metaphorical mappings:")
        print("    • 'underwater' → chorus/phaser/filtered")
        print("    • 'space' → reverb/delay/dimension")
        print("    • 'crispy' → exciter/eq/brightness")
        print("    • 'muddy' → eq/filter")
        print("    • 'punchy' → compressor/transient")
        
        print("\n2. ENGINE COUNT ISSUES:")
        print("  - Average 5.3-5.8 engines (target: 3-5)")
        print("  - Always filling all 6 slots")
        print("  - Not considering 'less is more'")
        
        print("\n3. RECOMMENDATIONS:")
        print("  - Expand metaphor dictionary")
        print("  - Implement quality over quantity logic")
        print("  - Add engine combination rules")
        print("  - Consider signal chain dependencies")
    
    def analyze_calculator_bottleneck(self):
        """Analyze Calculator limitations"""
        print("\n" + "="*80)
        print("🔍 BOTTLENECK #4: CALCULATOR NUDGING")
        print("="*80)
        
        print("\n1. NUDGE LIMITATIONS:")
        print("  - Too subtle (±0.1 typical)")
        print("  - Not enough variation between similar prompts")
        print("  - Limited understanding of intensity words")
        print("  - No learning from user feedback")
        
        print("\n2. CHARACTER MAPPING GAPS:")
        print("  - 'Slightly' vs 'Very' not differentiated")
        print("  - 'Subtle' vs 'Extreme' treated similarly")
        print("  - Missing compound descriptors")
        
        print("\n3. RECOMMENDATIONS:")
        print("  - Implement intensity scaling")
        print("  - Add modifier detection (very, slightly, extremely)")
        print("  - Create preset-specific nudge rules")
        print("  - Add feedback learning mechanism")
    
    def analyze_performance_bottleneck(self):
        """Analyze performance issues"""
        print("\n" + "="*80)
        print("🔍 BOTTLENECK #5: PERFORMANCE & EFFICIENCY")
        print("="*80)
        
        print("\n1. RESPONSE TIME BREAKDOWN (estimated):")
        print("  - Visionary (Cloud AI): 0.8-1.2s")
        print("  - Oracle (FAISS search): 0.1-0.2s")
        print("  - Calculator (nudging): 0.05s")
        print("  - Alchemist (validation): 0.1s")
        print("  - Network overhead: 0.3-0.5s")
        print("  - Total: ~2.0s average")
        
        print("\n2. SCALING ISSUES:")
        print("  - FAISS index rebuild needed for new presets")
        print("  - No caching of common requests")
        print("  - Sequential processing (not parallel)")
        
        print("\n3. RECOMMENDATIONS:")
        print("  - Implement request caching")
        print("  - Parallel processing where possible")
        print("  - Incremental FAISS updates")
        print("  - Optimize Cloud AI calls")
    
    def analyze_quality_bottleneck(self):
        """Analyze quality control issues"""
        print("\n" + "="*80)
        print("🔍 BOTTLENECK #6: QUALITY CONTROL")
        print("="*80)
        
        print("\n1. VALIDATION GAPS:")
        print("  - No preset quality scoring")
        print("  - No A/B testing framework")
        print("  - No user feedback loop")
        print("  - No automatic bad preset detection")
        
        print("\n2. TESTING LIMITATIONS:")
        print("  - Manual testing only")
        print("  - No automated quality metrics")
        print("  - No regression testing")
        
        print("\n3. RECOMMENDATIONS:")
        print("  - Implement preset quality scorer")
        print("  - Add automated testing suite")
        print("  - Create feedback collection system")
        print("  - Build preset validation framework")

    def create_expansion_strategy(self):
        """Create safe corpus expansion strategy"""
        print("\n" + "="*80)
        print("🚀 SAFE CORPUS EXPANSION STRATEGY")
        print("="*80)
        
        print("\n📋 PHASE 1: QUALITY CONTROL FRAMEWORK")
        print("""
1. Create Preset Validator:
   - Parameter range checker (0.0-1.0)
   - Engine compatibility validator
   - Signal chain logic checker
   - Duplicate detector
   
2. Quality Scoring System:
   - Musical coherence (do engines work together?)
   - Parameter balance (not all 0 or all 1)
   - Genre appropriateness
   - Naming quality
   
3. Testing Harness:
   - Automated audio rendering test
   - Null safety check (no crashes)
   - Performance impact measurement
        """)
        
        print("\n📋 PHASE 2: CORPUS EXPANSION SOURCES")
        print("""
1. Community Contributions:
   - User-submitted presets with validation
   - Voting/rating system
   - Require 5+ positive reviews
   
2. Algorithmic Generation:
   - Use successful presets as seeds
   - Genetic algorithm variations
   - Grid search parameter space
   
3. Professional Curation:
   - Partner with sound designers
   - Genre-specific preset packs
   - Signature artist presets
   
4. AI-Assisted Creation:
   - GPT-4 guided preset design
   - Style transfer from reference tracks
   - Automated A/B testing
        """)
        
        print("\n📋 PHASE 3: IMPLEMENTATION PLAN")
        print("""
1. Build Validation Pipeline:
   def validate_preset(preset):
       checks = [
           validate_parameters(),
           validate_engines(),
           validate_signal_chain(),
           validate_naming(),
           check_duplicates()
       ]
       return all(checks) and quality_score > 0.7
   
2. Incremental Addition:
   - Add 10 presets at a time
   - Test impact on search quality
   - Monitor performance metrics
   - Rollback if issues detected
   
3. Diversity Requirements:
   - Max 3 presets per genre per batch
   - Must use at least 2 unique engines
   - Parameter variance > 0.2
   - No duplicate names
        """)
        
        print("\n📋 PHASE 4: MONITORING & MAINTENANCE")
        print("""
1. Quality Metrics Dashboard:
   - Search accuracy trends
   - Response time monitoring
   - Preset usage statistics
   - User satisfaction scores
   
2. Automated Cleanup:
   - Remove unused presets (< 1% usage)
   - Merge similar presets
   - Update outdated parameters
   
3. Continuous Improvement:
   - A/B test new presets
   - Collect user feedback
   - Iterate based on data
        """)

    def create_improvement_priorities(self):
        """Priority-ranked improvements"""
        print("\n" + "="*80)
        print("🎯 IMPROVEMENT PRIORITIES (Accuracy > Speed)")
        print("="*80)
        
        priorities = [
            {
                "priority": 1,
                "area": "Corpus Expansion",
                "effort": "High",
                "impact": "Very High",
                "timeline": "2-3 weeks",
                "description": "Expand from 150 to 500+ quality presets with validation"
            },
            {
                "priority": 2,
                "area": "Engine Extraction",
                "effort": "Medium",
                "impact": "High",
                "timeline": "1 week",
                "description": "Add metaphor mappings and context understanding"
            },
            {
                "priority": 3,
                "area": "Oracle Search",
                "effort": "Medium",
                "impact": "High",
                "timeline": "1 week",
                "description": "Improve search with weighted keywords and genre modes"
            },
            {
                "priority": 4,
                "area": "Quality Validation",
                "effort": "High",
                "impact": "High",
                "timeline": "2 weeks",
                "description": "Build comprehensive validation and testing framework"
            },
            {
                "priority": 5,
                "area": "Calculator Intelligence",
                "effort": "Low",
                "impact": "Medium",
                "timeline": "3 days",
                "description": "Add intensity scaling and modifier detection"
            },
            {
                "priority": 6,
                "area": "Performance",
                "effort": "Medium",
                "impact": "Low",
                "timeline": "1 week",
                "description": "Add caching and parallel processing"
            }
        ]
        
        for p in priorities:
            print(f"\n{p['priority']}. {p['area'].upper()}")
            print(f"   Impact: {p['impact']} | Effort: {p['effort']} | Timeline: {p['timeline']}")
            print(f"   {p['description']}")

def main():
    analyzer = BottleneckAnalyzer()
    
    # Run all analyses
    analyzer.analyze_corpus_bottleneck()
    analyzer.analyze_oracle_bottleneck()
    analyzer.analyze_engine_bottleneck()
    analyzer.analyze_calculator_bottleneck()
    analyzer.analyze_performance_bottleneck()
    analyzer.analyze_quality_bottleneck()
    
    # Provide solutions
    analyzer.create_expansion_strategy()
    analyzer.create_improvement_priorities()
    
    print("\n" + "="*80)
    print("✅ ANALYSIS COMPLETE")
    print("="*80)

if __name__ == "__main__":
    main()