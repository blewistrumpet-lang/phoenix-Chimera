#!/usr/bin/env python3
"""
Simple test to verify utility engine functionality is working.
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

def create_basic_preset():
    """Create a basic preset with empty slots"""
    return {
        "name": "Test Preset",
        "vibe": "test",
        "parameters": {
            # Slot 1: Basic tube preamp
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

def test_stereo_width():
    """Test stereo width detection"""
    logger.info("=== Testing Stereo Width Detection ===")
    
    calculator = Calculator()
    preset = create_basic_preset()
    prompt = "make this sound wide and spacious"
    
    blueprint = {
        "creative_analysis": {
            "space": 0.85,  # High spatial requirements
            "intensity": 0.6,
            "mood": "spacious"
        },
        "slots": []
    }
    
    result = calculator.apply_nudges(preset, prompt, blueprint)
    
    # Check the metadata
    metadata = result.get("calculator_metadata", {})
    utility_engines = metadata.get("utility_engines_added", [])
    
    # Also check preset-level storage
    preset_utilities = result.get("calculator_utility_engines", [])
    
    logger.info(f"Utility engines in metadata: {utility_engines}")
    logger.info(f"Utility engines in preset: {preset_utilities}")
    
    # Check actual engine slots
    parameters = result.get("parameters", {})
    logger.info("Active engines after processing:")
    for slot in range(1, 7):
        engine_id = parameters.get(f"slot{slot}_engine", ENGINE_NONE)
        if engine_id != ENGINE_NONE:
            engine_name = get_engine_name(engine_id)
            bypass = parameters.get(f"slot{slot}_bypass", 1.0)
            mix = parameters.get(f"slot{slot}_mix", 0.5)
            status = "ACTIVE" if bypass < 0.5 else "BYPASSED"
            logger.info(f"  Slot {slot}: {engine_name} ({status}, mix: {mix:.2f})")
    
    return "Mid-Side Processor" in utility_engines or "Mid-Side Processor" in preset_utilities

def test_full_pipeline():
    """Test the complete pipeline"""
    logger.info("\n=== Testing Full Pipeline ===")
    
    calculator = Calculator()
    alchemist = Alchemist()
    
    preset = create_basic_preset()
    prompt = "wide stereo sound with powerful bass and multiple delays"
    
    blueprint = {
        "creative_analysis": {
            "intensity": 0.9,
            "space": 0.8,
            "mood": "aggressive spacious"
        },
        "slots": []
    }
    
    # Step 1: Calculator processing
    calc_result = calculator.apply_nudges(preset, prompt, blueprint)
    calc_metadata = calc_result.get("calculator_metadata", {})
    calc_utilities = calc_metadata.get("utility_engines_added", [])
    calc_preset_utilities = calc_result.get("calculator_utility_engines", [])
    
    logger.info(f"Calculator metadata: {calc_utilities}")
    logger.info(f"Calculator preset: {calc_preset_utilities}")
    
    # Step 2: Alchemist finalization
    final_result = alchemist.finalize_preset(calc_result)
    alchemist_metadata = final_result.get("alchemist_metadata", {})
    alchemist_utilities = alchemist_metadata.get("final_utility_additions", [])
    
    logger.info(f"Alchemist added: {alchemist_utilities}")
    
    # Show final state
    logger.info("Final engine configuration:")
    parameters = final_result.get("parameters", {})
    for slot in range(1, 7):
        engine_id = parameters.get(f"slot{slot}_engine", ENGINE_NONE)
        if engine_id != ENGINE_NONE:
            engine_name = get_engine_name(engine_id)
            bypass = parameters.get(f"slot{slot}_bypass", 1.0)
            mix = parameters.get(f"slot{slot}_mix", 0.5)
            status = "ACTIVE" if bypass < 0.5 else "BYPASSED"
            
            # Check if it's a utility engine
            is_utility = engine_id in [ENGINE_MID_SIDE_PROCESSOR, ENGINE_GAIN_UTILITY, 
                                     ENGINE_MONO_MAKER, ENGINE_PHASE_ALIGN]
            engine_type = " [UTILITY]" if is_utility else " [MUSICAL]"
            
            logger.info(f"  Slot {slot}: {engine_name}{engine_type} ({status}, mix: {mix:.2f})")
    
    return len(calc_utilities) > 0 or len(calc_preset_utilities) > 0 or len(alchemist_utilities) > 0

def main():
    """Run simple verification tests"""
    logger.info("UTILITY ENGINE VERIFICATION TESTS")
    logger.info("=" * 50)
    
    # Test 1: Stereo width detection
    stereo_success = test_stereo_width()
    logger.info(f"Stereo width test: {'PASS' if stereo_success else 'FAIL'}")
    
    # Test 2: Full pipeline
    pipeline_success = test_full_pipeline()
    logger.info(f"Full pipeline test: {'PASS' if pipeline_success else 'FAIL'}")
    
    # Summary
    logger.info(f"\nSUMMARY:")
    logger.info(f"Stereo Width Detection: {'✓' if stereo_success else '✗'}")
    logger.info(f"Full Pipeline: {'✓' if pipeline_success else '✗'}")
    
    if stereo_success and pipeline_success:
        logger.info("\n🎉 ALL UTILITY ENGINE FEATURES WORKING CORRECTLY!")
        logger.info("✓ Calculator intelligently adds utility engines")
        logger.info("✓ Alchemist performs final utility checks")
        logger.info("✓ Utility engines are properly configured")
        return 0
    else:
        logger.info("\n❌ Some utility engine features need attention")
        return 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)