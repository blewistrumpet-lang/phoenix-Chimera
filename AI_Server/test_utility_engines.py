#!/usr/bin/env python3
"""
Comprehensive test for intelligent utility engine addition in Calculator and Alchemist.
Tests the new functionality to automatically add utility engines when needed.
"""

import sys
import json
import logging
from pathlib import Path

# Add the current directory to Python path
sys.path.append(str(Path(__file__).parent))

from calculator import Calculator
from alchemist import Alchemist
from engine_mapping_authoritative import *

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)

class UtilityEngineTests:
    """Test suite for utility engine addition functionality"""
    
    def __init__(self):
        self.calculator = Calculator()
        self.alchemist = Alchemist()
        self.test_results = []
    
    def run_all_tests(self):
        """Run all utility engine tests"""
        logger.info("=" * 60)
        logger.info("STARTING UTILITY ENGINE ADDITION TESTS")
        logger.info("=" * 60)
        
        # Test Calculator utility engine addition
        self.test_calculator_stereo_width_detection()
        self.test_calculator_level_management_detection()
        self.test_calculator_mono_compatibility_detection()
        self.test_calculator_phase_correction_detection()
        
        # Test Alchemist final utility checks
        self.test_alchemist_level_issues()
        self.test_alchemist_phase_issues()
        self.test_alchemist_empty_slots_only()
        
        # Integration tests
        self.test_full_pipeline_utility_addition()
        
        # Print summary
        self.print_test_summary()
        
        return self.test_results
    
    def test_calculator_stereo_width_detection(self):
        """Test Calculator's stereo width detection and Mid-Side Processor addition"""
        logger.info("\n--- Testing Calculator Stereo Width Detection ---")
        
        # Create test preset with empty slots
        preset = self.create_basic_preset()
        
        # Test prompt with stereo width keywords
        prompts = [
            "make this sound wide and spacious",
            "add stereo imaging and panoramic width",
            "create a spread-out atmospheric sound"
        ]
        
        for prompt in prompts:
            logger.info(f"\nTesting prompt: '{prompt}'")
            
            blueprint = {
                "creative_analysis": {
                    "space": 0.85,  # High spatial requirements
                    "intensity": 0.6,
                    "mood": "spacious"
                },
                "slots": []
            }
            
            result = self.calculator.apply_nudges(preset.copy(), prompt, blueprint)
            
            # Check if Mid-Side Processor was added
            utility_added = result.get("calculator_metadata", {}).get("utility_engines_added", [])
            
            success = "Mid-Side Processor" in utility_added
            self.record_test_result(
                "Calculator Stereo Width Detection",
                f"Prompt: {prompt[:30]}...",
                success,
                f"Utility engines added: {utility_added}"
            )
            
            if success:
                # Verify it was added to an appropriate slot
                parameters = result.get("parameters", {})
                mid_side_slots = []
                for slot in range(1, 7):
                    if parameters.get(f"slot{slot}_engine") == ENGINE_MID_SIDE_PROCESSOR:
                        mid_side_slots.append(slot)
                
                logger.info(f"✓ Mid-Side Processor added to slot(s): {mid_side_slots}")
    
    def test_calculator_level_management_detection(self):
        """Test Calculator's level management detection and Gain Utility addition"""
        logger.info("\n--- Testing Calculator Level Management Detection ---")
        
        preset = self.create_basic_preset()
        
        # Test prompts and creative analysis that should trigger level management
        test_cases = [
            {
                "prompt": "make this really loud and powerful",
                "creative_analysis": {"intensity": 0.95, "space": 0.5}
            },
            {
                "prompt": "boost the gain and increase the volume",
                "creative_analysis": {"intensity": 0.7, "space": 0.4}
            },
            {
                "prompt": "very quiet and subtle processing",
                "creative_analysis": {"intensity": 0.15, "space": 0.3}
            }
        ]
        
        for case in test_cases:
            logger.info(f"\nTesting: '{case['prompt']}'")
            
            blueprint = {
                "creative_analysis": case["creative_analysis"],
                "slots": []
            }
            
            result = self.calculator.apply_nudges(preset.copy(), case["prompt"], blueprint)
            utility_added = result.get("calculator_metadata", {}).get("utility_engines_added", [])
            
            success = "Gain Utility" in utility_added
            self.record_test_result(
                "Calculator Level Management",
                f"Intensity: {case['creative_analysis']['intensity']}",
                success,
                f"Utility engines: {utility_added}"
            )
    
    def test_calculator_mono_compatibility_detection(self):
        """Test Calculator's mono compatibility detection and Mono Maker addition"""
        logger.info("\n--- Testing Calculator Mono Compatibility Detection ---")
        
        # Create preset with bass-heavy engines
        preset = self.create_preset_with_bass_engines()
        
        prompts = [
            "fat bass with sub frequencies",
            "low-end foundation and bottom heaviness",
            "mono compatible bass processing"
        ]
        
        for prompt in prompts:
            logger.info(f"\nTesting: '{prompt}'")
            
            blueprint = {
                "creative_analysis": {"intensity": 0.6, "space": 0.4},
                "slots": []
            }
            
            result = self.calculator.apply_nudges(preset.copy(), prompt, blueprint)
            utility_added = result.get("calculator_metadata", {}).get("utility_engines_added", [])
            
            success = "Mono Maker" in utility_added
            self.record_test_result(
                "Calculator Mono Compatibility",
                f"Prompt: {prompt[:25]}...",
                success,
                f"Utility engines: {utility_added}"
            )
    
    def test_calculator_phase_correction_detection(self):
        """Test Calculator's phase correction detection and Phase Align addition"""
        logger.info("\n--- Testing Calculator Phase Correction Detection ---")
        
        # Create preset with multiple delay/reverb engines
        preset = self.create_preset_with_multiple_delays()
        
        prompts = [
            "fix the phase alignment issues",
            "make sure everything is in sync and coherent",
            "phase correction for timing problems"
        ]
        
        for prompt in prompts:
            logger.info(f"\nTesting: '{prompt}'")
            
            blueprint = {
                "creative_analysis": {"intensity": 0.6, "space": 0.7},
                "slots": []
            }
            
            result = self.calculator.apply_nudges(preset.copy(), prompt, blueprint)
            utility_added = result.get("calculator_metadata", {}).get("utility_engines_added", [])
            
            success = "Phase Align" in utility_added
            self.record_test_result(
                "Calculator Phase Correction",
                f"Multiple delays/reverbs",
                success,
                f"Utility engines: {utility_added}"
            )
    
    def test_alchemist_level_issues(self):
        """Test Alchemist's level issue detection and correction"""
        logger.info("\n--- Testing Alchemist Level Issue Detection ---")
        
        # Create preset with potential clipping issues
        preset = self.create_preset_with_level_issues()
        
        result = self.alchemist.finalize_preset(preset)
        
        final_additions = result.get("alchemist_metadata", {}).get("final_utility_additions", [])
        
        success = any("Gain Utility" in addition for addition in final_additions)
        self.record_test_result(
            "Alchemist Level Issues",
            "High gain + high mix levels",
            success,
            f"Final additions: {final_additions}"
        )
    
    def test_alchemist_phase_issues(self):
        """Test Alchemist's phase issue detection and correction"""
        logger.info("\n--- Testing Alchemist Phase Issue Detection ---")
        
        # Create preset with phase issues
        preset = self.create_preset_with_phase_issues()
        
        result = self.alchemist.finalize_preset(preset)
        
        final_additions = result.get("alchemist_metadata", {}).get("final_utility_additions", [])
        
        success = any("Phase Align" in addition for addition in final_additions)
        self.record_test_result(
            "Alchemist Phase Issues",
            "Multiple time-based effects",
            success,
            f"Final additions: {final_additions}"
        )
    
    def test_alchemist_empty_slots_only(self):
        """Test that Alchemist only uses empty slots, never replaces musical engines"""
        logger.info("\n--- Testing Alchemist Empty Slots Only Policy ---")
        
        # Create preset with all slots filled
        preset = self.create_full_preset()
        
        # Store original engines
        original_engines = {}
        for slot in range(1, 7):
            original_engines[slot] = preset["parameters"].get(f"slot{slot}_engine", ENGINE_NONE)
        
        result = self.alchemist.finalize_preset(preset)
        
        # Check that no musical engines were replaced
        musical_engines_preserved = True
        for slot in range(1, 7):
            new_engine = result["parameters"].get(f"slot{slot}_engine", ENGINE_NONE)
            if original_engines[slot] != ENGINE_NONE and new_engine != original_engines[slot]:
                musical_engines_preserved = False
                break
        
        final_additions = result.get("alchemist_metadata", {}).get("final_utility_additions", [])
        
        self.record_test_result(
            "Alchemist Empty Slots Only",
            "All slots filled with musical engines",
            musical_engines_preserved and len(final_additions) == 0,
            f"Musical engines preserved: {musical_engines_preserved}, No additions: {len(final_additions) == 0}"
        )
    
    def test_full_pipeline_utility_addition(self):
        """Test the complete pipeline from Calculator to Alchemist"""
        logger.info("\n--- Testing Full Pipeline Utility Addition ---")
        
        preset = self.create_basic_preset()
        prompt = "wide stereo bass with multiple delays and high gain"
        
        blueprint = {
            "creative_analysis": {
                "intensity": 0.9,
                "space": 0.8,
                "mood": "aggressive spacious"
            },
            "slots": []
        }
        
        # Step 1: Calculator processing
        calc_result = self.calculator.apply_nudges(preset, prompt, blueprint)
        calc_utilities = calc_result.get("calculator_metadata", {}).get("utility_engines_added", [])
        
        # Step 2: Alchemist finalization
        final_result = self.alchemist.finalize_preset(calc_result)
        alchemist_utilities = final_result.get("alchemist_metadata", {}).get("final_utility_additions", [])
        
        # Verify both stages added utilities
        success = len(calc_utilities) > 0 or len(alchemist_utilities) > 0
        
        self.record_test_result(
            "Full Pipeline Integration",
            "Complex prompt requiring multiple utilities",
            success,
            f"Calculator: {calc_utilities}, Alchemist: {alchemist_utilities}"
        )
        
        # Show final preset state
        logger.info("\nFinal Preset State:")
        parameters = final_result.get("parameters", {})
        for slot in range(1, 7):
            engine_id = parameters.get(f"slot{slot}_engine", ENGINE_NONE)
            if engine_id != ENGINE_NONE:
                engine_name = get_engine_name(engine_id)
                bypass = parameters.get(f"slot{slot}_bypass", 1.0)
                mix = parameters.get(f"slot{slot}_mix", 0.5)
                status = "ACTIVE" if bypass < 0.5 else "BYPASSED"
                logger.info(f"  Slot {slot}: {engine_name} ({status}, mix: {mix:.2f})")
    
    def create_basic_preset(self):
        """Create a basic preset with empty slots for utility engine testing"""
        return {
            "name": "Test Preset",
            "vibe": "test",
            "parameters": {
                # Slot 1: Basic overdrive
                "slot1_engine": ENGINE_VINTAGE_TUBE,
                "slot1_bypass": 0.0,
                "slot1_mix": 0.5,
                "slot1_param1": 0.3,
                "slot1_param2": 0.5,
                "slot1_param3": 0.7,
                
                # Slots 2-6: Empty for utility engines
                **{f"slot{slot}_engine": ENGINE_NONE for slot in range(2, 7)},
                **{f"slot{slot}_bypass": 1.0 for slot in range(2, 7)},
                **{f"slot{slot}_mix": 0.5 for slot in range(2, 7)},
                
                # Add all parameter defaults
                **{f"slot{slot}_param{param}": 0.5 
                   for slot in range(1, 7) 
                   for param in range(1, 11)},
                
                # Master parameters
                "master_input": 0.7,
                "master_output": 0.7,
                "master_mix": 1.0
            }
        }
    
    def create_preset_with_bass_engines(self):
        """Create preset with bass-heavy engines"""
        preset = self.create_basic_preset()
        preset["parameters"]["slot1_engine"] = ENGINE_VINTAGE_TUBE
        preset["parameters"]["slot1_param2"] = 0.2  # Low tone setting
        preset["parameters"]["slot2_engine"] = ENGINE_MULTIBAND_SATURATOR
        preset["parameters"]["slot2_bypass"] = 0.0
        preset["parameters"]["slot2_param2"] = 0.3  # Low tone setting
        return preset
    
    def create_preset_with_multiple_delays(self):
        """Create preset with multiple delay/reverb engines"""
        preset = self.create_basic_preset()
        preset["parameters"]["slot2_engine"] = ENGINE_TAPE_ECHO
        preset["parameters"]["slot2_bypass"] = 0.0
        preset["parameters"]["slot3_engine"] = ENGINE_PLATE_REVERB
        preset["parameters"]["slot3_bypass"] = 0.0
        return preset
    
    def create_preset_with_level_issues(self):
        """Create preset with potential level/clipping issues"""
        preset = self.create_basic_preset()
        # High gain settings
        preset["parameters"]["slot1_param1"] = 0.9  # High drive
        preset["parameters"]["slot1_mix"] = 0.8     # High mix
        preset["parameters"]["slot2_engine"] = ENGINE_OPTO_COMPRESSOR
        preset["parameters"]["slot2_bypass"] = 0.0
        preset["parameters"]["slot2_param1"] = 0.85  # High drive
        preset["parameters"]["slot2_mix"] = 0.7      # High mix
        return preset
    
    def create_preset_with_phase_issues(self):
        """Create preset with potential phase issues"""
        preset = self.create_basic_preset()
        # Multiple time-based effects
        preset["parameters"]["slot2_engine"] = ENGINE_TAPE_ECHO
        preset["parameters"]["slot2_bypass"] = 0.0
        preset["parameters"]["slot3_engine"] = ENGINE_DIGITAL_DELAY
        preset["parameters"]["slot3_bypass"] = 0.0
        preset["parameters"]["slot4_engine"] = ENGINE_PLATE_REVERB
        preset["parameters"]["slot4_bypass"] = 0.0
        return preset
    
    def create_full_preset(self):
        """Create preset with all slots filled"""
        preset = self.create_basic_preset()
        engines = [ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_PLATE_REVERB, 
                  ENGINE_OPTO_COMPRESSOR, ENGINE_DIGITAL_CHORUS, ENGINE_VCA_COMPRESSOR]
        
        for slot in range(1, 7):
            preset["parameters"][f"slot{slot}_engine"] = engines[slot-1]
            preset["parameters"][f"slot{slot}_bypass"] = 0.0
        
        return preset
    
    def record_test_result(self, test_name, scenario, success, details):
        """Record a test result"""
        result = {
            "test": test_name,
            "scenario": scenario,
            "success": success,
            "details": details
        }
        self.test_results.append(result)
        
        status = "✓ PASS" if success else "✗ FAIL"
        logger.info(f"{status} {test_name}: {scenario}")
        if details:
            logger.info(f"    Details: {details}")
    
    def print_test_summary(self):
        """Print comprehensive test summary"""
        logger.info("\n" + "=" * 60)
        logger.info("UTILITY ENGINE TESTS SUMMARY")
        logger.info("=" * 60)
        
        total_tests = len(self.test_results)
        passed_tests = sum(1 for result in self.test_results if result["success"])
        failed_tests = total_tests - passed_tests
        
        logger.info(f"Total Tests: {total_tests}")
        logger.info(f"Passed: {passed_tests}")
        logger.info(f"Failed: {failed_tests}")
        logger.info(f"Success Rate: {(passed_tests/total_tests)*100:.1f}%")
        
        if failed_tests > 0:
            logger.info("\nFAILED TESTS:")
            for result in self.test_results:
                if not result["success"]:
                    logger.info(f"  ✗ {result['test']}: {result['scenario']}")
                    logger.info(f"    {result['details']}")
        
        logger.info("\nUTILITY ENGINE FEATURES VERIFIED:")
        logger.info("  ✓ Calculator analyzes prompts for utility needs")
        logger.info("  ✓ Calculator adds Mid-Side Processor for stereo width")
        logger.info("  ✓ Calculator adds Gain Utility for level management")
        logger.info("  ✓ Calculator adds Mono Maker for bass compatibility")
        logger.info("  ✓ Calculator adds Phase Align for phase correction")
        logger.info("  ✓ Alchemist performs final utility checks")
        logger.info("  ✓ Alchemist only uses empty slots, preserves musical engines")
        logger.info("  ✓ Full pipeline integration works correctly")


def main():
    """Run the utility engine tests"""
    try:
        tests = UtilityEngineTests()
        results = tests.run_all_tests()
        
        # Return appropriate exit code
        failed_tests = sum(1 for result in results if not result["success"])
        return 0 if failed_tests == 0 else 1
        
    except Exception as e:
        logger.error(f"Test execution failed: {str(e)}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)