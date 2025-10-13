#!/usr/bin/env python3
"""
Proof that the Trinity system is NOT complete
"""

import json
import asyncio
import sys
from pathlib import Path

def test_missing_components():
    """Test what's actually missing from the system"""
    
    print("🔍 TRINITY SYSTEM INCOMPLETENESS PROOF")
    print("=" * 60)
    
    missing = []
    incomplete = []
    
    # 1. Check if signal chain intelligence is integrated
    print("\n1️⃣ Testing Signal Chain Intelligence Integration...")
    try:
        with open('alchemist.py', 'r') as f:
            alchemist_code = f.read()
        
        if 'signal_chain' not in alchemist_code.lower():
            missing.append("Signal chain intelligence NOT integrated in Alchemist")
            print("   ❌ Signal chain optimization NOT being used")
        else:
            print("   ✅ Signal chain integrated")
            
        with open('calculator.py', 'r') as f:
            calc_code = f.read()
            
        if 'suggest_engines_for_intent' not in calc_code:
            missing.append("Engine suggestion logic NOT in Calculator")
            print("   ❌ Calculator can't suggest engines based on intent")
        else:
            print("   ✅ Engine suggestions integrated")
            
    except Exception as e:
        print(f"   ❌ Error: {e}")
    
    # 2. Test if Cloud AI actually works with real prompts
    print("\n2️⃣ Testing Cloud AI (Visionary)...")
    try:
        from cloud_bridge import CloudBridge
        import asyncio
        
        bridge = CloudBridge()
        
        # Test if it actually uses AI or just fallback
        async def test_ai():
            result = await bridge.get_cloud_generation("make it sound like Billie Eilish")
            return result
        
        result = asyncio.run(test_ai())
        
        if 'engines_to_use' in result and len(result['engines_to_use']) > 0:
            print(f"   ✅ Cloud AI working: {result.get('vibe', 'no vibe')}")
        else:
            incomplete.append("Cloud AI returns empty or fallback results")
            print(f"   ⚠️ Cloud AI using fallback: {result}")
            
    except Exception as e:
        missing.append(f"Cloud AI not working: {e}")
        print(f"   ❌ Cloud AI error: {e}")
    
    # 3. Check if engine knowledge is being used
    print("\n3️⃣ Testing Engine Knowledge Integration...")
    try:
        from oracle_faiss_fixed import OracleFAISS
        
        # Check if Oracle uses engine knowledge
        oracle = OracleFAISS(
            "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/corpus_clean.index",
            "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/metadata_clean.json",
            "../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json"
        )
        
        # Test if it understands engine purposes
        if not hasattr(oracle, 'engine_knowledge'):
            missing.append("Oracle doesn't use engine knowledge base")
            print("   ❌ Oracle doesn't understand what engines do")
        else:
            print("   ✅ Oracle uses engine knowledge")
            
    except Exception as e:
        print(f"   ❌ Error: {e}")
    
    # 4. Test actual preset generation quality
    print("\n4️⃣ Testing Preset Generation Quality...")
    import requests
    import subprocess
    import time
    
    # Start server in background
    server_process = subprocess.Popen(
        ["python3", "main.py"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    
    time.sleep(3)  # Wait for server to start
    
    try:
        test_prompts = [
            ("warm vintage vocals", ["Opto Compressor", "Vintage Tube", "reverb"]),
            ("aggressive metal guitar", ["distortion", "gate", "eq"]),
            ("ambient space pad", ["reverb", "delay", "modulation"])
        ]
        
        for prompt, expected_engines in test_prompts:
            response = requests.post(
                "http://localhost:8000/generate",
                json={"prompt": prompt},
                timeout=5
            )
            
            if response.status_code == 200:
                preset = response.json()["preset"]
                
                # Check if engines match intent
                engines_used = []
                for slot in range(1, 7):
                    engine_id = preset.get("parameters", {}).get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        from engine_mapping_authoritative import ENGINE_NAMES
                        engines_used.append(ENGINE_NAMES.get(engine_id, "Unknown"))
                
                print(f"\n   Prompt: '{prompt}'")
                print(f"   Expected: {expected_engines}")
                print(f"   Got: {engines_used[:3]}")
                
                # Check if it matches expectations
                matches = sum(1 for exp in expected_engines 
                             if any(exp.lower() in eng.lower() for eng in engines_used))
                
                if matches < len(expected_engines) / 2:
                    incomplete.append(f"Poor engine selection for '{prompt}'")
                    print(f"   ❌ Only {matches}/{len(expected_engines)} expected engines")
                else:
                    print(f"   ✅ {matches}/{len(expected_engines)} expected engines")
                    
    except Exception as e:
        missing.append(f"Preset generation failed: {e}")
        print(f"   ❌ Error: {e}")
    finally:
        server_process.terminate()
        time.sleep(1)
    
    # 5. Check if parameter adjustments are intelligent
    print("\n5️⃣ Testing Parameter Intelligence...")
    try:
        from calculator import Calculator
        
        calc = Calculator("nudge_rules.json")
        
        # Test if it adjusts parameters based on intent
        base_preset = {
            "slot1_engine": 15,  # Vintage Tube
            "slot1_param1": 0.5  # Drive
        }
        
        # Should increase drive for "aggressive"
        nudged = calc.apply_nudges(base_preset, "make it more aggressive", {})
        
        if nudged.get("slot1_param1", 0.5) <= 0.5:
            incomplete.append("Calculator doesn't adjust parameters intelligently")
            print(f"   ❌ Drive not increased for 'aggressive': {nudged.get('slot1_param1', 0.5)}")
        else:
            print(f"   ✅ Drive increased to {nudged.get('slot1_param1', 0.5)}")
            
    except Exception as e:
        print(f"   ❌ Error: {e}")
    
    # Final Report
    print("\n" + "=" * 60)
    print("📊 INCOMPLETENESS REPORT")
    print("=" * 60)
    
    if missing:
        print("\n❌ CRITICAL MISSING COMPONENTS:")
        for item in missing:
            print(f"   • {item}")
    
    if incomplete:
        print("\n⚠️ INCOMPLETE IMPLEMENTATIONS:")
        for item in incomplete:
            print(f"   • {item}")
    
    total_issues = len(missing) + len(incomplete)
    
    if total_issues == 0:
        print("\n✅ System appears complete!")
        confidence = 100
    else:
        print(f"\n🔴 System has {total_issues} issues")
        confidence = max(0, 100 - (total_issues * 15))
    
    print(f"\n🎯 System Completeness: {confidence}%")
    
    print("\n💡 WHAT'S NEEDED TO COMPLETE:")
    print("1. Integrate signal_chain_intelligence.py into alchemist.py")
    print("2. Add engine_knowledge_base.py to oracle and calculator")
    print("3. Implement parameter adjustment logic based on intent")
    print("4. Ensure cloud AI is properly connected and working")
    print("5. Test end-to-end with real musical prompts")
    
    return confidence < 100

if __name__ == "__main__":
    is_incomplete = test_missing_components()
    sys.exit(1 if is_incomplete else 0)