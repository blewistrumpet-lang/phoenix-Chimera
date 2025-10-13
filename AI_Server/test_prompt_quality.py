#!/usr/bin/env python3
"""
Comprehensive prompt testing with quality ratings
"""

import requests
import json
import time
from typing import Dict, List, Tuple
from engine_mapping_authoritative import ENGINE_NAMES

class PresetQualityAnalyzer:
    def __init__(self):
        self.base_url = "http://localhost:8000"
        
        # Expected mappings for quality assessment
        self.expected_engines = {
            "vintage": ["Vintage Tube Preamp", "Vintage Opto Compressor", "Tape Echo", "Spring Reverb"],
            "modern": ["Classic Compressor", "Parametric EQ", "Digital Delay", "Hall Reverb"],
            "aggressive": ["Noise Gate", "K-Style Overdrive", "Rodent Distortion", "Muff Fuzz"],
            "ambient": ["Shimmer Reverb", "Plate Reverb", "Dimension Expander", "Stereo Widener"],
            "lofi": ["Bit Crusher", "Tape Echo", "Spring Reverb", "Wow & Flutter"],
            "clean": ["Classic Compressor", "Parametric EQ", "Dynamic EQ"],
            "warm": ["Vintage Tube Preamp", "Analog Warmth", "Tape Echo"],
            "bright": ["Parametric EQ", "Exciter", "Presence Booster"],
            "dark": ["Low Pass Filter", "Muff Fuzz", "Gated Reverb"],
            "spacey": ["Shimmer Reverb", "Digital Delay", "Dimension Expander", "Chorus"]
        }
        
        # Signal chain order scoring
        self.ideal_order = {
            "dynamics": 1,  # Compressor, Gate, Limiter
            "eq": 2,        # EQ types
            "filter": 3,    # Filters
            "distortion": 4, # Distortion, Overdrive
            "modulation": 5, # Chorus, Phaser, Flanger
            "delay": 6,      # Delays
            "reverb": 7,     # Reverbs
            "spatial": 8     # Widener, Expander
        }
    
    def categorize_engine(self, engine_name: str) -> str:
        """Categorize an engine by type"""
        name_lower = engine_name.lower()
        
        if any(x in name_lower for x in ["compressor", "limiter", "gate", "transient"]):
            return "dynamics"
        elif "eq" in name_lower or "equalizer" in name_lower:
            return "eq"
        elif "filter" in name_lower:
            return "filter"
        elif any(x in name_lower for x in ["distortion", "overdrive", "fuzz", "tube", "saturation"]):
            return "distortion"
        elif any(x in name_lower for x in ["chorus", "phaser", "flanger", "tremolo", "vibrato"]):
            return "modulation"
        elif "delay" in name_lower or "echo" in name_lower:
            return "delay"
        elif "reverb" in name_lower:
            return "reverb"
        elif any(x in name_lower for x in ["widener", "expander", "imager"]):
            return "spatial"
        else:
            return "other"
    
    def rate_signal_chain(self, engines: List[Tuple[int, str]]) -> Tuple[float, str]:
        """Rate signal chain ordering (0-100)"""
        if not engines:
            return 0, "No engines"
        
        # Get categories in order
        categories = [self.categorize_engine(name) for _, name in engines]
        
        # Check ordering
        score = 100
        issues = []
        
        for i in range(len(categories) - 1):
            cat1 = categories[i]
            cat2 = categories[i + 1]
            
            expected_pos1 = self.ideal_order.get(cat1, 9)
            expected_pos2 = self.ideal_order.get(cat2, 9)
            
            if expected_pos1 > expected_pos2:
                score -= 15
                issues.append(f"{cat1} should come before {cat2}")
        
        # Check for logical issues
        if "reverb" in categories and categories.index("reverb") < len(categories) - 2:
            score -= 10
            issues.append("Reverb should typically be at the end")
        
        if "dynamics" in categories and categories.index("dynamics") > 3:
            score -= 10
            issues.append("Dynamics should typically be early in chain")
        
        explanation = "Good signal flow" if score > 80 else ", ".join(issues[:2])
        return max(0, score), explanation
    
    def rate_name_relevance(self, name: str, prompt: str) -> Tuple[float, str]:
        """Rate how well the name matches the prompt (0-100)"""
        name_lower = name.lower()
        prompt_lower = prompt.lower()
        
        score = 50  # Base score
        
        # Check for generic names (bad)
        if any(x in name_lower for x in ["safe default", "preset", "unknown", "custom sound"]):
            return 0, "Generic/default name"
        
        # Check for keyword matches (good)
        keywords = {
            "vintage": ["vintage", "retro", "classic", "old", "analog"],
            "modern": ["modern", "digital", "contemporary", "new"],
            "warm": ["warm", "cozy", "soft", "mellow"],
            "aggressive": ["aggressive", "brutal", "heavy", "extreme"],
            "ambient": ["ambient", "ethereal", "space", "atmospheric"],
            "clean": ["clean", "pristine", "pure", "clear"]
        }
        
        matches = []
        for category, words in keywords.items():
            if any(w in prompt_lower for w in words):
                if any(w in name_lower for w in words):
                    score += 25
                    matches.append(category)
        
        # Check for specific instrument mentions
        instruments = ["guitar", "bass", "vocal", "drum", "piano", "synth", "strings"]
        for inst in instruments:
            if inst in prompt_lower and inst in name_lower:
                score += 15
                matches.append(inst)
        
        # Creativity bonus
        if len(name.split()) >= 2 and not any(x in name_lower for x in ["sonic", "preset", "effect"]):
            score += 10
        
        explanation = f"Matches: {', '.join(matches)}" if matches else "No keyword matches"
        return min(100, score), explanation
    
    def rate_engine_selection(self, engines: List[str], prompt: str) -> Tuple[float, str]:
        """Rate how well engines match the prompt intent (0-100)"""
        prompt_lower = prompt.lower()
        score = 50  # Base score
        matches = []
        misses = []
        
        # Check for expected engines based on keywords
        for keyword, expected in self.expected_engines.items():
            if keyword in prompt_lower:
                for engine in expected:
                    if any(engine.lower() in e.lower() for e in engines):
                        score += 10
                        matches.append(engine)
                    else:
                        score -= 5
                        misses.append(engine)
        
        # Check for explicitly requested engines
        all_engine_names = list(ENGINE_NAMES.values())
        for engine_name in all_engine_names:
            if engine_name.lower() in prompt_lower:
                if any(engine_name.lower() in e.lower() for e in engines):
                    score += 20
                    matches.append(f"✓{engine_name}")
                else:
                    score -= 20
                    misses.append(f"✗{engine_name}")
        
        # Penalize too many engines (overly complex)
        if len(engines) > 5:
            score -= 10
            misses.append("Too many engines")
        
        # Penalize too few engines (too simple)
        if len(engines) < 3:
            score -= 10
            misses.append("Too few engines")
        
        explanation = ""
        if matches:
            explanation += f"Good: {', '.join(matches[:3])}"
        if misses:
            explanation += f" Missing: {', '.join(misses[:3])}"
        
        return max(0, min(100, score)), explanation.strip()
    
    def test_prompt(self, prompt: str) -> Dict:
        """Test a single prompt and return detailed results"""
        try:
            response = requests.post(
                f"{self.base_url}/generate",
                json={"prompt": prompt},
                timeout=10
            )
            
            if response.status_code != 200:
                return {
                    "prompt": prompt,
                    "error": f"Status {response.status_code}",
                    "overall_score": 0
                }
            
            data = response.json()
            preset = data.get("preset", {})
            metadata = data.get("metadata", {})
            
            # Extract preset details
            name = preset.get("name", "Unknown")
            signal_flow = preset.get("signal_flow", "")
            
            # Extract engines in order
            engines = []
            engine_names = []
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
                    engines.append((slot, engine_name))
                    engine_names.append(engine_name)
            
            # Rate different aspects
            name_score, name_explanation = self.rate_name_relevance(name, prompt)
            chain_score, chain_explanation = self.rate_signal_chain(engines)
            engine_score, engine_explanation = self.rate_engine_selection(engine_names, prompt)
            
            # Calculate overall score
            overall_score = (name_score * 0.3 + chain_score * 0.3 + engine_score * 0.4)
            
            return {
                "prompt": prompt,
                "preset_name": name,
                "engines": engines,
                "signal_flow": signal_flow,
                "scores": {
                    "name_relevance": (name_score, name_explanation),
                    "signal_chain": (chain_score, chain_explanation),
                    "engine_selection": (engine_score, engine_explanation),
                    "overall": overall_score
                },
                "metadata": metadata
            }
            
        except Exception as e:
            return {
                "prompt": prompt,
                "error": str(e),
                "overall_score": 0
            }
    
    def run_comprehensive_test(self):
        """Run comprehensive prompt tests"""
        print("🎯 COMPREHENSIVE PROMPT QUALITY ANALYSIS")
        print("=" * 80)
        
        # Diverse test prompts
        test_prompts = [
            # Specific genre/style requests
            "Create a warm vintage 1960s Motown bass sound",
            "Modern EDM supersaw lead with heavy sidechain compression",
            "Nashville country guitar with subtle chorus and spring reverb",
            "Dark techno kick drum with heavy distortion and compression",
            "Ethereal new age pad with shimmer reverb and slow attack",
            
            # Technical requests
            "I need vintage tube preamp into plate reverb with subtle compression",
            "Give me aggressive noise gate, K-style overdrive, and tight EQ",
            "Add bit crusher and tape echo for lo-fi hip hop vocals",
            "Use dimension expander and stereo widener for huge soundscape",
            "Classic compressor with 4:1 ratio and slow attack for drums",
            
            # Creative/descriptive requests
            "Make it sound like a guitar played in a cathedral",
            "Underwater dreamy vocals with lots of modulation",
            "Crispy modern pop vocals with de-esser and brightness",
            "Gritty garage rock bass with fuzz and spring reverb",
            "Silky smooth jazz piano with warm compression",
            
            # Multiple requirement requests
            "Vintage warmth with modern clarity and controlled dynamics",
            "Aggressive but clean metal tone with gate and EQ",
            "Ambient space with subtle distortion and long reverb",
            "Punchy drums with transient shaping and parallel compression",
            "Wide stereo synth with chorus, delay, and hall reverb",
            
            # Edge cases
            "Just make it sound good",
            "Professional mastering chain",
            "Experimental glitch effects",
            "Natural acoustic sound",
            "Maximum aggression and distortion"
        ]
        
        results = []
        total_scores = {
            "name_relevance": [],
            "signal_chain": [],
            "engine_selection": [],
            "overall": []
        }
        
        for i, prompt in enumerate(test_prompts, 1):
            print(f"\n{'='*60}")
            print(f"Test {i}/{len(test_prompts)}")
            print(f"PROMPT: {prompt}")
            print("-" * 60)
            
            result = self.test_prompt(prompt)
            results.append(result)
            
            if "error" in result:
                print(f"❌ ERROR: {result['error']}")
                continue
            
            # Display results
            print(f"📝 PRESET NAME: '{result['preset_name']}'")
            
            print(f"\n🎛️ ENGINES (in signal chain order):")
            for slot, engine in result['engines']:
                print(f"   Slot {slot}: {engine}")
            
            print(f"\n📊 SIGNAL FLOW:")
            if "→" in result['signal_flow']:
                parts = result['signal_flow'].split("→")
                for j, part in enumerate(parts):
                    print(f"   {j+1}. {part.strip()}")
            else:
                print(f"   {result['signal_flow']}")
            
            print(f"\n📈 QUALITY RATINGS:")
            scores = result['scores']
            
            name_score, name_exp = scores['name_relevance']
            print(f"   Name Relevance:    {name_score:3.0f}/100 - {name_exp}")
            total_scores["name_relevance"].append(name_score)
            
            chain_score, chain_exp = scores['signal_chain']
            print(f"   Signal Chain:      {chain_score:3.0f}/100 - {chain_exp}")
            total_scores["signal_chain"].append(chain_score)
            
            engine_score, engine_exp = scores['engine_selection']
            print(f"   Engine Selection:  {engine_score:3.0f}/100 - {engine_exp}")
            total_scores["engine_selection"].append(engine_score)
            
            overall = scores['overall']
            print(f"   OVERALL:           {overall:3.0f}/100")
            total_scores["overall"].append(overall)
            
            # Grade
            if overall >= 85:
                grade = "A"
                emoji = "🌟"
            elif overall >= 75:
                grade = "B"
                emoji = "✅"
            elif overall >= 65:
                grade = "C"
                emoji = "👍"
            elif overall >= 55:
                grade = "D"
                emoji = "⚠️"
            else:
                grade = "F"
                emoji = "❌"
            
            print(f"\n   Grade: {emoji} {grade}")
            
            time.sleep(0.5)  # Small delay between requests
        
        # Final analysis
        print("\n" + "=" * 80)
        print("📊 OVERALL ANALYSIS")
        print("=" * 80)
        
        # Calculate averages
        for category, scores in total_scores.items():
            if scores:
                avg = sum(scores) / len(scores)
                min_score = min(scores)
                max_score = max(scores)
                print(f"\n{category.upper().replace('_', ' ')}:")
                print(f"   Average: {avg:.1f}/100")
                print(f"   Range: {min_score:.0f}-{max_score:.0f}")
                
                # Distribution
                excellent = sum(1 for s in scores if s >= 85)
                good = sum(1 for s in scores if 75 <= s < 85)
                fair = sum(1 for s in scores if 65 <= s < 75)
                poor = sum(1 for s in scores if s < 65)
                
                print(f"   Distribution:")
                print(f"      Excellent (85+): {excellent}")
                print(f"      Good (75-84):    {good}")
                print(f"      Fair (65-74):    {fair}")
                print(f"      Poor (<65):      {poor}")
        
        # Identify patterns
        print("\n" + "=" * 80)
        print("🔍 PATTERN ANALYSIS")
        print("=" * 80)
        
        # Find worst performing prompts
        worst_results = sorted(results, key=lambda x: x.get('scores', {}).get('overall', 0))[:5]
        print("\n❌ Worst Performing Prompts:")
        for r in worst_results:
            if 'scores' in r:
                print(f"   {r['scores']['overall']:.0f}/100: {r['prompt'][:50]}...")
        
        # Find best performing prompts
        best_results = sorted(results, key=lambda x: x.get('scores', {}).get('overall', 100), reverse=True)[:5]
        print("\n✅ Best Performing Prompts:")
        for r in best_results:
            if 'scores' in r:
                print(f"   {r['scores']['overall']:.0f}/100: {r['prompt'][:50]}...")
        
        # Common issues
        print("\n⚠️ Common Issues Identified:")
        issues = []
        
        avg_name = sum(total_scores["name_relevance"]) / len(total_scores["name_relevance"]) if total_scores["name_relevance"] else 0
        if avg_name < 70:
            issues.append(f"Name relevance low ({avg_name:.0f}/100) - names don't match prompts well")
        
        avg_chain = sum(total_scores["signal_chain"]) / len(total_scores["signal_chain"]) if total_scores["signal_chain"] else 0
        if avg_chain < 70:
            issues.append(f"Signal chain ordering issues ({avg_chain:.0f}/100)")
        
        avg_engine = sum(total_scores["engine_selection"]) / len(total_scores["engine_selection"]) if total_scores["engine_selection"] else 0
        if avg_engine < 70:
            issues.append(f"Engine selection accuracy low ({avg_engine:.0f}/100)")
        
        for issue in issues:
            print(f"   • {issue}")
        
        # Enhancement recommendations
        print("\n" + "=" * 80)
        print("💡 ENHANCEMENT PLAN")
        print("=" * 80)
        
        recommendations = []
        
        if avg_name < 75:
            recommendations.append({
                "area": "Preset Naming",
                "issue": "Names don't reflect prompt content well",
                "solution": "Improve keyword extraction and genre detection in name generation",
                "priority": "HIGH" if avg_name < 65 else "MEDIUM"
            })
        
        if avg_chain < 75:
            recommendations.append({
                "area": "Signal Chain Ordering",
                "issue": "Effects not in optimal order",
                "solution": "Strengthen signal chain intelligence rules",
                "priority": "HIGH" if avg_chain < 65 else "MEDIUM"
            })
        
        if avg_engine < 75:
            recommendations.append({
                "area": "Engine Selection",
                "issue": "Not selecting most appropriate engines",
                "solution": "Improve engine_extraction.py keyword mappings and Oracle scoring",
                "priority": "HIGH" if avg_engine < 65 else "MEDIUM"
            })
        
        # Check for specific patterns
        vintage_results = [r for r in results if 'vintage' in r['prompt'].lower()]
        if vintage_results:
            vintage_avg = sum(r['scores']['overall'] for r in vintage_results if 'scores' in r) / len(vintage_results)
            if vintage_avg < 70:
                recommendations.append({
                    "area": "Vintage Sound Recognition",
                    "issue": "Poor performance on vintage requests",
                    "solution": "Add more vintage-specific engine mappings",
                    "priority": "MEDIUM"
                })
        
        for rec in recommendations:
            print(f"\n📌 {rec['area']} ({rec['priority']} Priority)")
            print(f"   Issue: {rec['issue']}")
            print(f"   Solution: {rec['solution']}")
        
        if not recommendations:
            print("\n✅ System performing well! Only minor optimizations needed.")
        
        # Final score
        overall_system_score = sum(total_scores["overall"]) / len(total_scores["overall"]) if total_scores["overall"] else 0
        print("\n" + "=" * 80)
        print(f"🎯 OVERALL SYSTEM SCORE: {overall_system_score:.1f}/100")
        
        if overall_system_score >= 80:
            print("🌟 EXCELLENT - System is performing very well!")
        elif overall_system_score >= 70:
            print("✅ GOOD - System is working well with room for improvement")
        elif overall_system_score >= 60:
            print("⚠️ FAIR - System needs some improvements")
        else:
            print("❌ POOR - System needs significant improvements")
        
        return results, recommendations

if __name__ == "__main__":
    analyzer = PresetQualityAnalyzer()
    results, recommendations = analyzer.run_comprehensive_test()