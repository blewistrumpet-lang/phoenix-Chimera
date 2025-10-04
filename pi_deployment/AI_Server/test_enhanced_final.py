#!/usr/bin/env python3
"""
Final test of the fully enhanced Trinity system
"""

import requests
import json
from typing import Dict, Any

def test_enhanced_system():
    """Test the enhanced system with various musical prompts"""
    
    print("🎵 TESTING ENHANCED TRINITY SYSTEM")
    print("=" * 60)
    
    # Test cases covering different genres and use cases
    test_cases = [
        {
            "prompt": "Create warm vintage vocals like Billie Eilish - intimate and close",
            "expected_engines": ["Opto", "Tube", "Reverb"],
            "expected_character": "warm, intimate"
        },
        {
            "prompt": "Aggressive metal guitar with heavy distortion and tight gating",
            "expected_engines": ["Gate", "Overdrive", "EQ"],
            "expected_character": "aggressive, tight"
        },
        {
            "prompt": "Ethereal ambient pad with shimmer reverb and lots of space",
            "expected_engines": ["Shimmer", "Delay", "Dimension"],
            "expected_character": "spacious, ethereal"
        },
        {
            "prompt": "Jazz piano with natural dynamics and warm room sound",
            "expected_engines": ["Opto", "EQ", "Reverb"],
            "expected_character": "natural, warm"
        },
        {
            "prompt": "Modern pop vocals - bright, compressed, and polished",
            "expected_engines": ["Compressor", "Exciter", "Reverb"],
            "expected_character": "bright, polished"
        }
    ]
    
    base_url = "http://localhost:8000"
    results = []
    
    for i, test in enumerate(test_cases, 1):
        print(f"\n{i}. Testing: '{test['prompt'][:50]}...'")
        print("-" * 50)
        
        try:
            # Make request
            response = requests.post(
                f"{base_url}/generate",
                json={"prompt": test["prompt"]},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                metadata = data.get("metadata", {})
                
                # Extract information
                name = preset.get("name", "Unknown")
                signal_flow = preset.get("signal_flow", "No flow")
                warnings = preset.get("warnings", [])
                musical_analysis = metadata.get("musical_analysis", {})
                oracle_explanation = metadata.get("oracle_explanation", "")
                
                # Check engines
                engines = []
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        from engine_mapping_authoritative import ENGINE_NAMES
                        engines.append(ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})"))
                
                # Display results
                print(f"✅ SUCCESS: Generated '{name}'")
                print(f"   Engines: {', '.join(engines[:4])}")
                print(f"   Signal Flow: {signal_flow[:80]}...")
                
                # Display musical analysis
                if musical_analysis:
                    print(f"   Musical Analysis:")
                    print(f"      Genre: {musical_analysis.get('genre', 'none')}")
                    print(f"      Instrument: {musical_analysis.get('instrument', 'none')}")
                    print(f"      Character: {', '.join(musical_analysis.get('character', []))}")
                
                # Display Oracle explanation
                if oracle_explanation:
                    print(f"   Oracle: {oracle_explanation[:80]}...")
                
                # Check optimizations
                if metadata.get("signal_chain_optimized"):
                    print(f"   ✅ Signal chain optimized")
                if metadata.get("safety_validated"):
                    print(f"   ✅ Safety validated")
                
                # Display warnings if any
                if warnings:
                    print(f"   ⚠️ Warnings: {len(warnings)} parameter warnings")
                
                # Check if expectations were met
                matches = 0
                for expected in test["expected_engines"]:
                    if any(expected.lower() in engine.lower() for engine in engines):
                        matches += 1
                
                success_rate = (matches / len(test["expected_engines"])) * 100
                print(f"   Match Rate: {success_rate:.0f}% ({matches}/{len(test['expected_engines'])} expected engines)")
                
                results.append({
                    "prompt": test["prompt"][:50],
                    "success": True,
                    "match_rate": success_rate,
                    "name": name
                })
                
            else:
                print(f"❌ FAILED: HTTP {response.status_code}")
                print(f"   Error: {response.text[:200]}")
                results.append({
                    "prompt": test["prompt"][:50],
                    "success": False,
                    "match_rate": 0,
                    "name": "Failed"
                })
                
        except Exception as e:
            print(f"❌ ERROR: {str(e)}")
            results.append({
                "prompt": test["prompt"][:50],
                "success": False,
                "match_rate": 0,
                "name": "Error"
            })
    
    # Final Summary
    print("\n" + "=" * 60)
    print("📊 FINAL TEST RESULTS")
    print("=" * 60)
    
    successful = sum(1 for r in results if r["success"])
    total = len(results)
    avg_match = sum(r["match_rate"] for r in results) / total if total > 0 else 0
    
    print(f"\n✅ Successful: {successful}/{total} tests")
    print(f"📈 Average Match Rate: {avg_match:.1f}%")
    
    print("\nDetailed Results:")
    for r in results:
        status = "✅" if r["success"] else "❌"
        print(f"{status} {r['prompt']} -> {r['name']} ({r['match_rate']:.0f}% match)")
    
    # System verdict
    print("\n" + "=" * 60)
    if successful == total and avg_match >= 80:
        print("🎉 SYSTEM IS PERFECT! All tests passed with high accuracy!")
        print("✨ The enhanced Trinity system is fully operational!")
    elif successful >= total * 0.8 and avg_match >= 60:
        print("✅ SYSTEM IS GOOD! Most tests passed with decent accuracy.")
        print("📝 Minor improvements could be made but it's functional.")
    elif successful >= total * 0.5:
        print("⚠️ SYSTEM IS PARTIALLY WORKING. Some features need attention.")
    else:
        print("❌ SYSTEM HAS ISSUES. Significant problems detected.")
    
    return successful == total and avg_match >= 80

if __name__ == "__main__":
    import sys
    
    # Import required modules
    sys.path.insert(0, '/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server')
    
    success = test_enhanced_system()
    sys.exit(0 if success else 1)