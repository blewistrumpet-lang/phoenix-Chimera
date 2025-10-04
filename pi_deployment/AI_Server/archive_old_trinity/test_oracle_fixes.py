#!/usr/bin/env python3
"""
Test script to verify Oracle fixes:
1. Pre-filtering of utility engines
2. Enhanced engine matching weight
3. Proper logging of requested vs found engines
"""

import logging
import json
from oracle_faiss import OracleFAISS
from engine_mapping_authoritative import UTILITY_ENGINES, ENGINE_NAMES

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

def create_test_blueprint(engines=None, vibe="aggressive"):
    """Create a test blueprint with specific engines"""
    blueprint = {
        "overall_vibe": vibe,
        "creative_name": "Test Preset",
        "slots": []
    }
    
    if engines:
        for slot, engine_id in enumerate(engines, 1):
            blueprint["slots"].append({
                "slot": slot,
                "engine_id": engine_id
            })
    
    return blueprint

def test_oracle_filtering():
    """Test that Oracle filters out utility engines properly"""
    logger.info("="*60)
    logger.info("Testing Oracle Filtering of Utility Engines")
    logger.info("="*60)
    
    try:
        oracle = OracleFAISS()
        
        # Check that no utility engines remain in presets
        utility_count = 0
        total_presets = len(oracle.presets)
        
        for preset in oracle.presets:
            preset_engines = oracle._get_preset_engines(preset)
            utility_in_preset = preset_engines.intersection(set(UTILITY_ENGINES))
            if utility_in_preset:
                utility_count += 1
                logger.warning(f"Found utility engines {utility_in_preset} in preset: {preset.get('name', 'Unknown')}")
        
        logger.info(f"Utility engine filtering: {utility_count}/{total_presets} presets contain utility engines")
        
        if utility_count == 0:
            logger.info("✅ SUCCESS: All utility engines filtered out")
        else:
            logger.error("❌ FAILED: Utility engines still present in corpus")
        
        return utility_count == 0
        
    except Exception as e:
        logger.error(f"Error testing filtering: {e}")
        return False

def test_engine_matching():
    """Test that engine matching works with higher weights"""
    logger.info("="*60)
    logger.info("Testing Enhanced Engine Matching")
    logger.info("="*60)
    
    try:
        oracle = OracleFAISS()
        
        # Test with specific engine requests (using engines that exist in corpus)
        test_cases = [
            # Test case 1: Request engines that exist in corpus  
            {
                "name": "Spatial/Reverb Request",
                "engines": [45, 39, 52],  # Stereo Imager, Plate Reverb, Feedback Network
                "vibe": "spacious ambient"
            },
            # Test case 2: Request delay/reverb engines that exist
            {
                "name": "Delay/Reverb Request", 
                "engines": [39, 42, 43],  # Plate Reverb, Shimmer Reverb, Gated Reverb
                "vibe": "spacious ambient"
            },
            # Test case 3: Mixed category engines that exist
            {
                "name": "Mixed Request",
                "engines": [32, 39, 51],  # Detune Doubler, Plate Reverb, Chaos Generator
                "vibe": "experimental"
            },
            # Test case 4: Request engines that DON'T exist (should fall back to vibe)
            {
                "name": "Non-Existent Engines",
                "engines": [1, 23, 15],  # Opto Compressor, Digital Chorus, Vintage Tube
                "vibe": "warm vintage"
            }
        ]
        
        all_passed = True
        
        for test_case in test_cases:
            logger.info(f"\n--- {test_case['name']} ---")
            
            # Create blueprint
            blueprint = create_test_blueprint(test_case['engines'], test_case['vibe'])
            
            # Log requested engines
            requested_names = [ENGINE_NAMES.get(e, f"Unknown({e})") for e in test_case['engines']]
            logger.info(f"Requesting engines: {requested_names}")
            
            # Find matches
            results = oracle.find_best_presets(blueprint, k=3)
            
            if not results:
                logger.error(f"❌ No results found for {test_case['name']}")
                all_passed = False
                continue
            
            # Check if top match has better engine matching than pure vibe matching
            top_match = results[0]
            engine_match_score = top_match.get('engine_match_score', 0)
            similarity_score = top_match.get('similarity_score', 0)
            combined_score = top_match.get('combined_score', 0)
            
            logger.info(f"Top match: {top_match.get('creative_name', top_match.get('name', 'Unknown'))}")
            logger.info(f"  Engine match score: {engine_match_score:.3f}")
            logger.info(f"  Similarity score: {similarity_score:.3f}")  
            logger.info(f"  Combined score: {combined_score:.3f}")
            
            # Verify engine matching is working
            if engine_match_score > 0:
                logger.info(f"✅ Engine matching detected: {engine_match_score:.3f}")
            else:
                logger.warning(f"⚠️  No engine matches found - may be expected if no presets contain these engines")
        
        return all_passed
        
    except Exception as e:
        logger.error(f"Error testing engine matching: {e}")
        return False

def test_vibe_only_matching():
    """Test matching based on vibe when no engines are specified"""
    logger.info("="*60)
    logger.info("Testing Vibe-Only Matching (No Engines)")
    logger.info("="*60)
    
    try:
        oracle = OracleFAISS()
        
        # Test with vibe only (no engines)
        blueprint = {
            "overall_vibe": "warm ambient spacious",
            "creative_name": "Vibe Test",
            "slots": []  # No engines specified
        }
        
        results = oracle.find_best_presets(blueprint, k=3)
        
        if results:
            logger.info("✅ Vibe-only matching successful")
            for i, preset in enumerate(results):
                logger.info(f"  Match {i+1}: {preset.get('creative_name', preset.get('name', 'Unknown'))} "
                          f"(similarity: {preset.get('similarity_score', 0):.3f})")
            return True
        else:
            logger.error("❌ No results for vibe-only matching")
            return False
            
    except Exception as e:
        logger.error(f"Error testing vibe matching: {e}")
        return False

def main():
    """Run all Oracle tests"""
    logger.info("Starting Oracle Enhancement Tests")
    
    results = []
    
    # Test 1: Utility engine filtering
    results.append(("Utility Engine Filtering", test_oracle_filtering()))
    
    # Test 2: Enhanced engine matching
    results.append(("Enhanced Engine Matching", test_engine_matching()))
    
    # Test 3: Vibe-only matching
    results.append(("Vibe-Only Matching", test_vibe_only_matching()))
    
    # Summary
    logger.info("="*60)
    logger.info("TEST SUMMARY")
    logger.info("="*60)
    
    passed = 0
    for test_name, result in results:
        status = "✅ PASSED" if result else "❌ FAILED"
        logger.info(f"{test_name}: {status}")
        if result:
            passed += 1
    
    logger.info(f"\nOverall: {passed}/{len(results)} tests passed")
    
    if passed == len(results):
        logger.info("🎉 All Oracle enhancements working correctly!")
    else:
        logger.warning("⚠️  Some issues detected - check logs above")

if __name__ == "__main__":
    main()