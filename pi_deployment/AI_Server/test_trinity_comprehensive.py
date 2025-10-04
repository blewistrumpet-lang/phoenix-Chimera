"""
Comprehensive Trinity Pipeline Validation
Tests all fixes to ensure the pipeline works correctly end-to-end
"""

import asyncio
import requests
import json
from typing import Dict, Any, List
from engine_mapping_authoritative import *
import time

class TrinityValidator:
    """Comprehensive validation of the Trinity pipeline"""
    
    def __init__(self):
        self.base_url = "http://localhost:8000"
        self.test_results = []
        self.passed = 0
        self.failed = 0
        
    def test_preset_generation(self, prompt: str) -> Dict[str, Any]:
        """Test a single preset generation"""
        try:
            response = requests.post(
                f"{self.base_url}/generate",
                json={"prompt": prompt},
                timeout=10
            )
            return response.json()
        except Exception as e:
            return {"success": False, "error": str(e)}
    
    def analyze_result(self, prompt: str, result: Dict[str, Any]) -> Dict[str, Any]:
        """Analyze a generation result"""
        analysis = {
            "prompt": prompt,
            "success": result.get("success", False),
            "preset_name": None,
            "musical_engines": [],
            "utility_engines": [],
            "expected_engines": [],
            "found_expected": [],
            "issues": []
        }
        
        if not result.get("success"):
            analysis["issues"].append(f"Generation failed: {result.get('message', 'Unknown error')}")
            return analysis
        
        preset = result.get("preset", {})
        params = preset.get("parameters", {})
        
        analysis["preset_name"] = preset.get("name", "Unknown")
        
        # Extract engines
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                if engine_id in UTILITY_ENGINES:
                    analysis["utility_engines"].append((slot, engine_id, get_engine_name(engine_id)))
                else:
                    analysis["musical_engines"].append((slot, engine_id, get_engine_name(engine_id)))
        
        # Check expected engines based on prompt
        prompt_lower = prompt.lower()
        
        if "tube" in prompt_lower or "vintage" in prompt_lower:
            analysis["expected_engines"].append(ENGINE_VINTAGE_TUBE)
        if "phaser" in prompt_lower:
            analysis["expected_engines"].append(ENGINE_ANALOG_PHASER)
        if "delay" in prompt_lower or "echo" in prompt_lower:
            analysis["expected_engines"].extend([ENGINE_TAPE_ECHO, ENGINE_DIGITAL_DELAY])
        if "shimmer" in prompt_lower:
            analysis["expected_engines"].append(ENGINE_SHIMMER_REVERB)
        if "envelope filter" in prompt_lower:
            analysis["expected_engines"].append(ENGINE_ENVELOPE_FILTER)
        if "bit crusher" in prompt_lower or "bitcrusher" in prompt_lower:
            analysis["expected_engines"].append(ENGINE_BIT_CRUSHER)
        if "chorus" in prompt_lower:
            analysis["expected_engines"].extend([ENGINE_DIGITAL_CHORUS, ENGINE_RESONANT_CHORUS])
        if "distortion" in prompt_lower or "fuzz" in prompt_lower:
            analysis["expected_engines"].extend(DISTORTION_ENGINES)
        if "compression" in prompt_lower or "compressor" in prompt_lower:
            analysis["expected_engines"].extend([ENGINE_OPTO_COMPRESSOR, ENGINE_VCA_COMPRESSOR])
        if "gate" in prompt_lower and "noise" in prompt_lower:
            analysis["expected_engines"].append(ENGINE_NOISE_GATE)
        
        # Check which expected engines were found
        musical_engine_ids = [e[1] for e in analysis["musical_engines"]]
        for expected_id in analysis["expected_engines"]:
            if expected_id in musical_engine_ids:
                analysis["found_expected"].append(expected_id)
        
        # Determine issues
        if analysis["expected_engines"]:
            match_rate = len(analysis["found_expected"]) / len(set(analysis["expected_engines"]))
            if match_rate < 0.5:
                analysis["issues"].append(f"Low engine match rate: {match_rate:.1%}")
        
        # Check for inappropriate utility engines in main slots
        if analysis["utility_engines"]:
            main_slot_utilities = [u for u in analysis["utility_engines"] if u[0] <= 4]
            if main_slot_utilities:
                analysis["issues"].append(f"Utility engines in main slots: {main_slot_utilities}")
        
        return analysis
    
    def run_comprehensive_tests(self):
        """Run comprehensive test suite"""
        print("\n" + "="*100)
        print("TRINITY PIPELINE COMPREHENSIVE VALIDATION")
        print("="*100)
        
        test_prompts = [
            # Specific engine requests
            "warm vintage guitar with tube saturation and spring reverb",
            "aggressive metal with noise gate and heavy distortion",
            "spacious ambient pad with shimmer reverb",
            "funky bass with envelope filter",
            "crispy drums with compression and transient shaping",
            "psychedelic guitar with phaser and tape delay",
            
            # Creative descriptions
            "ethereal vocal chain with lush reverb",
            "punchy EDM lead with filter sweeps",
            "lo-fi hip hop beat with bit crusher",
            "smooth jazz guitar with chorus",
            "industrial noise with ring modulation",
            "cinematic soundscape with granular effects",
            
            # Technical requests
            "mastering chain with EQ and limiting",
            "vocal de-essing with dynamic EQ",
            "stereo widening for mix bus",
            "mono-compatible bass processing",
            
            # Edge cases
            "simple clean boost",
            "everything!",
            "nothing",
            "chaos generator and feedback network"
        ]
        
        for i, prompt in enumerate(test_prompts, 1):
            print(f"\n[{i}/{len(test_prompts)}] Testing: \"{prompt}\"")
            print("-" * 80)
            
            # Generate preset
            result = self.test_preset_generation(prompt)
            analysis = self.analyze_result(prompt, result)
            
            # Display results
            if analysis["success"]:
                print(f"✅ Success: Generated \"{analysis['preset_name']}\"")
                
                # Show engines
                if analysis["musical_engines"]:
                    print(f"\n   Musical Engines ({len(analysis['musical_engines'])}):")
                    for slot, engine_id, name in analysis["musical_engines"]:
                        print(f"      Slot {slot}: {name} (ID {engine_id})")
                
                if analysis["utility_engines"]:
                    print(f"\n   Utility Engines ({len(analysis['utility_engines'])}):")
                    for slot, engine_id, name in analysis["utility_engines"]:
                        print(f"      Slot {slot}: {name} (ID {engine_id})")
                
                # Check expectations
                if analysis["expected_engines"]:
                    match_rate = len(analysis["found_expected"]) / len(set(analysis["expected_engines"]))
                    print(f"\n   Engine Match Rate: {match_rate:.1%}")
                    if match_rate >= 0.5:
                        print(f"   ✅ Good engine matching")
                    else:
                        print(f"   ⚠️  Poor engine matching")
                
                # Check utility placement
                if analysis["utility_engines"]:
                    main_slots = [u[0] for u in analysis["utility_engines"] if u[0] <= 4]
                    if not main_slots:
                        print(f"   ✅ Utility engines in auxiliary slots (5-6)")
                    else:
                        print(f"   ⚠️  Utility engines in main slots: {main_slots}")
                
                if not analysis["issues"]:
                    self.passed += 1
                else:
                    print(f"\n   Issues: {', '.join(analysis['issues'])}")
                    self.failed += 1
            else:
                print(f"❌ Failed: {analysis['issues']}")
                self.failed += 1
            
            self.test_results.append(analysis)
            
            # Small delay between tests
            time.sleep(0.5)
        
        # Final summary
        print("\n" + "="*100)
        print("VALIDATION SUMMARY")
        print("="*100)
        
        total = self.passed + self.failed
        success_rate = (self.passed / total * 100) if total > 0 else 0
        
        print(f"\nTotal Tests: {total}")
        print(f"Passed: {self.passed} ({self.passed/total*100:.1f}%)")
        print(f"Failed: {self.failed} ({self.failed/total*100:.1f}%)")
        
        # Aggregate statistics
        total_musical = sum(len(r["musical_engines"]) for r in self.test_results)
        total_utility = sum(len(r["utility_engines"]) for r in self.test_results)
        total_engines = total_musical + total_utility
        
        print(f"\nEngine Statistics:")
        print(f"  Total Engines Selected: {total_engines}")
        print(f"  Musical Engines: {total_musical} ({total_musical/total_engines*100:.1f}%)")
        print(f"  Utility Engines: {total_utility} ({total_utility/total_engines*100:.1f}%)")
        
        # Check utility placement
        utility_in_main = sum(1 for r in self.test_results 
                             for u in r["utility_engines"] if u[0] <= 4)
        utility_in_aux = sum(1 for r in self.test_results 
                            for u in r["utility_engines"] if u[0] > 4)
        
        if total_utility > 0:
            print(f"\nUtility Placement:")
            print(f"  In Main Slots (1-4): {utility_in_main} ({utility_in_main/total_utility*100:.1f}%)")
            print(f"  In Auxiliary Slots (5-6): {utility_in_aux} ({utility_in_aux/total_utility*100:.1f}%)")
        
        # Overall assessment
        print("\n" + "="*100)
        if success_rate >= 90 and utility_in_main == 0:
            print("🎉 EXCELLENT: Trinity Pipeline is working perfectly!")
        elif success_rate >= 75:
            print("✅ GOOD: Trinity Pipeline is working well with minor issues")
        elif success_rate >= 50:
            print("⚠️  FAIR: Trinity Pipeline has some issues that need attention")
        else:
            print("❌ POOR: Trinity Pipeline has significant problems")
        
        return self.test_results

def main():
    """Run validation"""
    validator = TrinityValidator()
    
    # Check if server is running
    try:
        response = requests.get("http://localhost:8000/health", timeout=2)
        if response.status_code == 200:
            print("✅ Trinity server is running")
        else:
            print("⚠️ Trinity server responded but may have issues")
    except:
        print("❌ Trinity server is not running. Please start it first.")
        return
    
    # Run tests
    results = validator.run_comprehensive_tests()
    
    # Save results
    with open("trinity_validation_results.json", "w") as f:
        json.dump(results, f, indent=2)
    print(f"\nDetailed results saved to trinity_validation_results.json")

if __name__ == "__main__":
    main()