#!/usr/bin/env python3
"""
Comprehensive test of the Trinity AI pipeline
Tests all 4 components: Visionary, Oracle, Calculator, Alchemist
"""

import asyncio
import json
import logging
from typing import Dict, Any, List
from main import generate_preset, GenerateRequest

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)

class TrinityPipelineTest:
    def __init__(self):
        self.test_prompts = [
            # Basic prompts
            {
                "prompt": "Create a warm vintage tube sound",
                "expected_traits": ["warm", "vintage", "tube"],
                "category": "vintage"
            },
            # Complex multi-effect prompts
            {
                "prompt": "Build an aggressive modern metal tone with tight compression and gated reverb",
                "expected_traits": ["aggressive", "tight", "metal"],
                "category": "distortion"
            },
            # Ambient/experimental
            {
                "prompt": "Design a spacious ambient soundscape with shimmer and granular textures",
                "expected_traits": ["spacious", "ambient", "shimmer"],
                "category": "ambient"
            },
            # Specific instrument
            {
                "prompt": "Create a lo-fi hip hop vocal chain with vintage compression and tape saturation",
                "expected_traits": ["lofi", "vintage", "vocal"],
                "category": "vocal"
            },
            # Psychedelic
            {
                "prompt": "Craft a psychedelic guitar effect with phaser, ring modulation and chaos",
                "expected_traits": ["psychedelic", "modulation", "experimental"],
                "category": "experimental"
            },
            # Clean/pristine
            {
                "prompt": "Make a clean pristine vocal with subtle EQ and transparent limiting",
                "expected_traits": ["clean", "transparent", "vocal"],
                "category": "clean"
            },
            # Bass specific
            {
                "prompt": "Design a punchy bass tone with multiband saturation and tight compression",
                "expected_traits": ["punchy", "bass", "tight"],
                "category": "bass"
            },
            # Drum processing
            {
                "prompt": "Create an 80s drum sound with gated reverb and transient shaping",
                "expected_traits": ["drums", "gated", "80s"],
                "category": "drums"
            }
        ]
        
        self.test_results = []
    
    async def run_all_tests(self):
        """Run all test prompts through the Trinity pipeline"""
        print("=" * 80)
        print("TRINITY AI PIPELINE COMPREHENSIVE TEST")
        print("=" * 80)
        
        for i, test in enumerate(self.test_prompts):
            print(f"\nTest {i+1}/{len(self.test_prompts)}: {test['category'].upper()}")
            print(f"Prompt: {test['prompt']}")
            print("-" * 80)
            
            result = await self.test_prompt(test)
            self.test_results.append(result)
            
            # Brief pause between tests
            await asyncio.sleep(0.5)
        
        # Generate summary report
        self.generate_report()
    
    async def test_prompt(self, test_case: Dict[str, Any]) -> Dict[str, Any]:
        """Test a single prompt through the pipeline"""
        try:
            # Generate preset
            request = GenerateRequest(prompt=test_case["prompt"])
            response = await generate_preset(request)
            
            if response.success:
                preset = response.preset
                
                # Analyze results
                active_engines = self.get_active_engines(preset)
                parameter_stats = self.analyze_parameters(preset)
                
                # Check if expected traits are reflected
                traits_found = self.check_traits(preset, test_case["expected_traits"])
                
                result = {
                    "prompt": test_case["prompt"],
                    "category": test_case["category"],
                    "success": True,
                    "preset_name": preset.get("name", "Unknown"),
                    "vibe": preset.get("vibe", "Unknown"),
                    "active_engines": active_engines,
                    "parameter_stats": parameter_stats,
                    "traits_found": traits_found,
                    "warnings": preset.get("validation_warnings", [])
                }
                
                # Print summary
                print(f"✓ Success: {preset.get('name', 'Unknown')}")
                print(f"  Vibe: {preset.get('vibe', 'Unknown')}")
                print(f"  Active Engines: {len(active_engines)}")
                for engine in active_engines[:3]:  # Show first 3
                    print(f"    - Slot {engine['slot']}: Engine {engine['id']}")
                print(f"  Traits Found: {traits_found['found']}/{traits_found['total']}")
                if preset.get("validation_warnings"):
                    print(f"  ⚠ Warnings: {len(preset.get('validation_warnings', []))}")
                
            else:
                result = {
                    "prompt": test_case["prompt"],
                    "category": test_case["category"],
                    "success": False,
                    "error": response.message
                }
                print(f"✗ Failed: {response.message}")
            
            return result
            
        except Exception as e:
            result = {
                "prompt": test_case["prompt"],
                "category": test_case["category"],
                "success": False,
                "error": str(e)
            }
            print(f"✗ Exception: {str(e)}")
            return result
    
    def get_active_engines(self, preset: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Extract active engines from preset"""
        active = []
        params = preset.get("parameters", {})
        
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            bypass = params.get(f"slot{slot}_bypass", 1.0)
            
            if bypass < 0.5 and engine_id > 0:
                active.append({
                    "slot": slot,
                    "id": engine_id,
                    "mix": params.get(f"slot{slot}_mix", 0.5)
                })
        
        return active
    
    def analyze_parameters(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Analyze parameter distribution"""
        params = preset.get("parameters", {})
        values = []
        
        for key, value in params.items():
            if "param" in key and isinstance(value, (int, float)):
                values.append(float(value))
        
        if values:
            return {
                "count": len(values),
                "min": min(values),
                "max": max(values),
                "avg": sum(values) / len(values)
            }
        else:
            return {"count": 0, "min": 0, "max": 0, "avg": 0}
    
    def check_traits(self, preset: Dict[str, Any], expected_traits: List[str]) -> Dict[str, Any]:
        """Check if expected traits are reflected in the preset"""
        # Check in vibe, calculator nudges, and other metadata
        preset_text = json.dumps(preset).lower()
        found = 0
        
        for trait in expected_traits:
            if trait.lower() in preset_text:
                found += 1
        
        return {
            "found": found,
            "total": len(expected_traits),
            "percentage": (found / len(expected_traits) * 100) if expected_traits else 0
        }
    
    def generate_report(self):
        """Generate final test report"""
        print("\n" + "=" * 80)
        print("TEST SUMMARY REPORT")
        print("=" * 80)
        
        # Overall stats
        total_tests = len(self.test_results)
        successful = sum(1 for r in self.test_results if r.get("success", False))
        
        print(f"\nTotal Tests: {total_tests}")
        print(f"Successful: {successful}")
        print(f"Failed: {total_tests - successful}")
        print(f"Success Rate: {successful/total_tests*100:.1f}%")
        
        # Category breakdown
        print("\nBy Category:")
        categories = {}
        for result in self.test_results:
            cat = result["category"]
            if cat not in categories:
                categories[cat] = {"total": 0, "success": 0}
            categories[cat]["total"] += 1
            if result.get("success", False):
                categories[cat]["success"] += 1
        
        for cat, stats in categories.items():
            rate = stats["success"] / stats["total"] * 100
            print(f"  {cat.capitalize()}: {stats['success']}/{stats['total']} ({rate:.0f}%)")
        
        # Trait matching
        print("\nTrait Matching Performance:")
        trait_scores = []
        for result in self.test_results:
            if result.get("success") and "traits_found" in result:
                trait_scores.append(result["traits_found"]["percentage"])
        
        if trait_scores:
            avg_trait_match = sum(trait_scores) / len(trait_scores)
            print(f"  Average Trait Match: {avg_trait_match:.1f}%")
        
        # Common warnings
        print("\nCommon Warnings:")
        all_warnings = []
        for result in self.test_results:
            if result.get("success") and result.get("warnings"):
                all_warnings.extend(result["warnings"])
        
        if all_warnings:
            from collections import Counter
            warning_counts = Counter(all_warnings)
            for warning, count in warning_counts.most_common(3):
                print(f"  - {warning} ({count} occurrences)")
        else:
            print("  No warnings generated")
        
        # Engine usage
        print("\nMost Used Engines:")
        engine_usage = {}
        for result in self.test_results:
            if result.get("success") and result.get("active_engines"):
                for engine in result["active_engines"]:
                    engine_id = engine["id"]
                    engine_usage[engine_id] = engine_usage.get(engine_id, 0) + 1
        
        if engine_usage:
            sorted_engines = sorted(engine_usage.items(), key=lambda x: x[1], reverse=True)
            for engine_id, count in sorted_engines[:5]:
                print(f"  - Engine {engine_id}: {count} times")
        
        print("\n" + "=" * 80)
        print("TEST COMPLETED")
        print("=" * 80)


async def main():
    """Run the Trinity pipeline test"""
    tester = TrinityPipelineTest()
    await tester.run_all_tests()


if __name__ == "__main__":
    asyncio.run(main())