#!/usr/bin/env python3
"""
Comprehensive System Test - Verify 100% Completeness
Tests all enhanced components and musical intelligence
"""

import json
import sys
from typing import Dict, Any, List
import asyncio

# Import enhanced components
from alchemist_enhanced import AlchemistEnhanced
from calculator_enhanced import CalculatorEnhanced  
from oracle_enhanced import OracleEnhanced
from music_theory_intelligence import MusicTheoryIntelligence, analyze_musical_intent
from signal_chain_intelligence import SignalChainIntelligence
from engine_knowledge_base import ENGINE_KNOWLEDGE

def test_complete_system():
    """
    Comprehensive test of all system components
    """
    print("🧪 COMPREHENSIVE TRINITY SYSTEM TEST")
    print("=" * 60)
    
    test_results = {
        "alchemist": {"passed": 0, "failed": 0, "tests": []},
        "calculator": {"passed": 0, "failed": 0, "tests": []},
        "oracle": {"passed": 0, "failed": 0, "tests": []},
        "music_theory": {"passed": 0, "failed": 0, "tests": []},
        "signal_chain": {"passed": 0, "failed": 0, "tests": []},
        "integration": {"passed": 0, "failed": 0, "tests": []}
    }
    
    # Test 1: Alchemist Enhanced
    print("\n1️⃣ Testing Enhanced Alchemist...")
    try:
        alchemist = AlchemistEnhanced()
        
        # Test signal chain optimization
        bad_preset = {
            "slot1_engine": 42,  # Shimmer (should be last)
            "slot2_engine": 2,   # Compressor (should be first)
            "slot3_engine": 18,  # Bit Crusher
            "slot3_param2": 0.9  # Dangerous mix level
        }
        
        finalized = alchemist.finalize_preset(bad_preset, "warm vintage vocals")
        
        # Check if signal chain was optimized
        if finalized.get("slot1_engine") == 2:  # Compressor moved to slot1
            test_results["alchemist"]["passed"] += 1
            test_results["alchemist"]["tests"].append("✅ Signal chain optimized")
        else:
            test_results["alchemist"]["failed"] += 1
            test_results["alchemist"]["tests"].append("❌ Signal chain not optimized")
        
        # Check if safety was applied
        if finalized.get("slot3_param2", 1.0) < 0.9:
            test_results["alchemist"]["passed"] += 1
            test_results["alchemist"]["tests"].append("✅ Safety validation applied")
        else:
            test_results["alchemist"]["failed"] += 1
            test_results["alchemist"]["tests"].append("❌ Safety validation failed")
        
        # Check if name was generated
        if "name" in finalized and finalized["name"] != "":
            test_results["alchemist"]["passed"] += 1
            test_results["alchemist"]["tests"].append(f"✅ Name generated: {finalized['name']}")
        else:
            test_results["alchemist"]["failed"] += 1
            test_results["alchemist"]["tests"].append("❌ Name generation failed")
        
        print(f"   Alchemist: {test_results['alchemist']['passed']}/3 tests passed")
        
    except Exception as e:
        test_results["alchemist"]["failed"] += 3
        test_results["alchemist"]["tests"].append(f"❌ Alchemist error: {e}")
    
    # Test 2: Calculator Enhanced
    print("\n2️⃣ Testing Enhanced Calculator...")
    try:
        calculator = CalculatorEnhanced()
        
        # Test engine suggestions
        test_prompts = {
            "warm vintage vocals": [1, 15, 39],  # Should suggest Opto, Tube, Reverb
            "aggressive metal guitar": [4, 22, 21],  # Should suggest Gate, K-Style, Rodent
            "ambient space pad": [42, 35, 46]  # Should suggest Shimmer, Delay, Dimension
        }
        
        for prompt, expected in test_prompts.items():
            suggested = calculator.suggest_engines_for_intent(prompt)
            
            # Check if at least 2 expected engines are suggested
            matches = sum(1 for e in expected if e in suggested)
            if matches >= 2:
                test_results["calculator"]["passed"] += 1
                test_results["calculator"]["tests"].append(f"✅ '{prompt}': {matches}/3 engines")
            else:
                test_results["calculator"]["failed"] += 1
                test_results["calculator"]["tests"].append(f"❌ '{prompt}': only {matches}/3 engines")
        
        # Test parameter adjustments
        base_preset = {
            "slot1_engine": 15,  # Vintage Tube
            "slot1_param1": 0.5  # Drive at 50%
        }
        
        nudged = calculator.apply_nudges(base_preset, "make it more aggressive", {})
        
        # Check if drive was increased
        if nudged.get("slot1_param1", 0.5) > 0.5:
            test_results["calculator"]["passed"] += 1
            test_results["calculator"]["tests"].append("✅ Parameters adjusted for aggressive")
        else:
            test_results["calculator"]["failed"] += 1
            test_results["calculator"]["tests"].append("❌ Parameters not adjusted")
        
        print(f"   Calculator: {test_results['calculator']['passed']}/4 tests passed")
        
    except Exception as e:
        test_results["calculator"]["failed"] += 4
        test_results["calculator"]["tests"].append(f"❌ Calculator error: {e}")
    
    # Test 3: Oracle Enhanced
    print("\n3️⃣ Testing Enhanced Oracle...")
    try:
        oracle = OracleEnhanced(
            "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus_clean.index",
            "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata_clean.json",
            "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json"
        )
        
        # Test musical understanding
        test_blueprints = [
            {"vibe": "warm vintage vocals", "expected_engines": [1, 15, 39]},
            {"vibe": "aggressive metal guitar", "expected_engines": [4, 22]},
            {"vibe": "ambient space pad", "expected_engines": [42, 35]}
        ]
        
        for blueprint in test_blueprints:
            preset = oracle.find_best_preset(blueprint)
            
            if "oracle_explanation" in preset:
                test_results["oracle"]["passed"] += 1
                test_results["oracle"]["tests"].append(f"✅ Explanation: {preset['oracle_explanation'][:50]}...")
            else:
                test_results["oracle"]["failed"] += 1
                test_results["oracle"]["tests"].append("❌ No explanation generated")
        
        print(f"   Oracle: {test_results['oracle']['passed']}/3 tests passed")
        
    except Exception as e:
        test_results["oracle"]["failed"] += 3
        test_results["oracle"]["tests"].append(f"❌ Oracle error: {e}")
    
    # Test 4: Music Theory Intelligence
    print("\n4️⃣ Testing Music Theory Intelligence...")
    try:
        music_theory = MusicTheoryIntelligence()
        
        test_analyses = [
            ("billie eilish style vocals", {"reference": "billie_eilish", "character": ["intimate", "dark", "minimal"]}),
            ("metal guitar with scooped mids", {"genre": "metal", "instrument": "guitar"}),
            ("jazz piano with warmth", {"genre": "jazz", "instrument": "piano", "character": ["warm"]})
        ]
        
        for prompt, expected in test_analyses:
            analysis = music_theory.analyze_prompt_musically(prompt)
            
            # Check if analysis matches expectations
            matches = 0
            if expected.get("genre") == analysis.get("genre"):
                matches += 1
            if expected.get("instrument") == analysis.get("instrument"):
                matches += 1
            if expected.get("reference") == analysis.get("reference"):
                matches += 1
            
            if matches >= 2:
                test_results["music_theory"]["passed"] += 1
                test_results["music_theory"]["tests"].append(f"✅ '{prompt[:30]}...': {matches}/3 matched")
            else:
                test_results["music_theory"]["failed"] += 1
                test_results["music_theory"]["tests"].append(f"❌ '{prompt[:30]}...': only {matches}/3")
        
        print(f"   Music Theory: {test_results['music_theory']['passed']}/3 tests passed")
        
    except Exception as e:
        test_results["music_theory"]["failed"] += 3
        test_results["music_theory"]["tests"].append(f"❌ Music theory error: {e}")
    
    # Test 5: Signal Chain Intelligence
    print("\n5️⃣ Testing Signal Chain Intelligence...")
    try:
        signal_intel = SignalChainIntelligence()
        
        # Test ordering
        bad_chain = {
            "slot1_engine": 39,  # Reverb (should be last)
            "slot2_engine": 18,  # Bit Crusher (mid)
            "slot3_engine": 2,   # Compressor (should be first)
        }
        
        optimized = signal_intel.optimize_signal_chain(bad_chain)
        
        # Check if compressor is now first
        if optimized.get("slot1_engine") == 2:
            test_results["signal_chain"]["passed"] += 1
            test_results["signal_chain"]["tests"].append("✅ Signal chain correctly ordered")
        else:
            test_results["signal_chain"]["failed"] += 1
            test_results["signal_chain"]["tests"].append("❌ Signal chain ordering failed")
        
        # Test safety validation
        dangerous_preset = {
            "slot1_engine": 35,  # Delay
            "slot1_param0": 0.1,  # Short time
            "slot1_param1": 0.9,  # High feedback - dangerous!
        }
        
        is_safe, warnings = signal_intel.validate_parameters(dangerous_preset)
        
        if not is_safe or warnings:
            test_results["signal_chain"]["passed"] += 1
            test_results["signal_chain"]["tests"].append("✅ Detected dangerous parameters")
        else:
            test_results["signal_chain"]["failed"] += 1
            test_results["signal_chain"]["tests"].append("❌ Failed to detect danger")
        
        # Test explanation
        explanation = signal_intel.explain_chain(optimized)
        if "Signal flow:" in explanation:
            test_results["signal_chain"]["passed"] += 1
            test_results["signal_chain"]["tests"].append("✅ Generated signal flow explanation")
        else:
            test_results["signal_chain"]["failed"] += 1
            test_results["signal_chain"]["tests"].append("❌ No explanation generated")
        
        print(f"   Signal Chain: {test_results['signal_chain']['passed']}/3 tests passed")
        
    except Exception as e:
        test_results["signal_chain"]["failed"] += 3
        test_results["signal_chain"]["tests"].append(f"❌ Signal chain error: {e}")
    
    # Test 6: Full Integration
    print("\n6️⃣ Testing Full Integration...")
    try:
        # Test complete pipeline
        test_cases = [
            {
                "prompt": "Create warm vintage vocals like Billie Eilish",
                "expected": {
                    "engines": [1, 15, 39],  # Opto, Tube, Reverb
                    "character": "warm",
                    "safety": True
                }
            },
            {
                "prompt": "Aggressive metal guitar with heavy distortion",
                "expected": {
                    "engines": [4, 22],  # Gate, Overdrive
                    "character": "aggressive",
                    "safety": True
                }
            },
            {
                "prompt": "Ethereal ambient pad with lots of space",
                "expected": {
                    "engines": [42, 35],  # Shimmer, Delay
                    "character": "spacious",
                    "safety": True
                }
            }
        ]
        
        for test in test_cases:
            # Simulate full pipeline
            prompt = test["prompt"]
            
            # 1. Music theory analysis
            analysis = analyze_musical_intent(prompt)
            
            # 2. Oracle finds preset
            oracle = OracleEnhanced(
                "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus_clean.index",
                "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata_clean.json",
                "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json"
            )
            blueprint = {"vibe": prompt}
            preset = oracle.find_best_preset(blueprint)
            
            # 3. Calculator nudges
            calculator = CalculatorEnhanced()
            nudged = calculator.apply_nudges(preset, prompt, blueprint)
            
            # 4. Alchemist finalizes
            alchemist = AlchemistEnhanced()
            final = alchemist.finalize_preset(nudged, prompt)
            
            # Check results
            success = True
            
            # Check if expected engines are present
            final_engines = [final.get(f"slot{i}_engine", 0) for i in range(1, 7)]
            expected_engines = test["expected"]["engines"]
            
            matches = sum(1 for e in expected_engines if e in final_engines)
            if matches >= len(expected_engines) // 2:
                test_results["integration"]["passed"] += 1
                test_results["integration"]["tests"].append(f"✅ '{prompt[:30]}...': {matches} engines matched")
            else:
                test_results["integration"]["failed"] += 1
                test_results["integration"]["tests"].append(f"❌ '{prompt[:30]}...': only {matches} matched")
        
        print(f"   Integration: {test_results['integration']['passed']}/3 tests passed")
        
    except Exception as e:
        test_results["integration"]["failed"] += 3
        test_results["integration"]["tests"].append(f"❌ Integration error: {e}")
    
    # Final Report
    print("\n" + "=" * 60)
    print("📊 FINAL TEST REPORT")
    print("=" * 60)
    
    total_passed = sum(r["passed"] for r in test_results.values())
    total_failed = sum(r["failed"] for r in test_results.values())
    total_tests = total_passed + total_failed
    
    print(f"\n✅ Passed: {total_passed}/{total_tests}")
    print(f"❌ Failed: {total_failed}/{total_tests}")
    
    for component, results in test_results.items():
        print(f"\n{component.upper()}:")
        for test in results["tests"]:
            print(f"  {test}")
    
    # Calculate system completeness
    if total_tests > 0:
        completeness = (total_passed / total_tests) * 100
    else:
        completeness = 0
    
    print(f"\n🎯 SYSTEM COMPLETENESS: {completeness:.1f}%")
    
    if completeness >= 90:
        print("✨ SYSTEM IS PRODUCTION READY!")
    elif completeness >= 70:
        print("⚠️ SYSTEM MOSTLY WORKING - Some fixes needed")
    elif completeness >= 50:
        print("🔧 SYSTEM PARTIALLY WORKING - Significant work needed")
    else:
        print("❌ SYSTEM NOT READY - Major issues to fix")
    
    return completeness >= 90

if __name__ == "__main__":
    success = test_complete_system()
    sys.exit(0 if success else 1)