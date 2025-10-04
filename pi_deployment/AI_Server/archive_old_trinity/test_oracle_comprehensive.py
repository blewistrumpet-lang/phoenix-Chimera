#!/usr/bin/env python3
"""
Comprehensive Oracle Testing Script
Tests all Oracle components with different scenarios to ensure proper preset matching.
"""

import json
import logging
import sys
import traceback
from pathlib import Path
from typing import Dict, Any, List

# Set up logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# Import Oracle components
try:
    from oracle import Oracle
    from oracle_string_ids import OracleStringIDs
    from oracle_faiss import OracleFAISS
    from engine_mapping_authoritative import ENGINE_VINTAGE_TUBE, ENGINE_TAPE_ECHO, ENGINE_PLATE_REVERB, ENGINE_NONE
except ImportError as e:
    logger.error(f"Failed to import Oracle components: {e}")
    sys.exit(1)

class OracleTestSuite:
    """Comprehensive test suite for all Oracle components"""
    
    def __init__(self):
        """Initialize test suite"""
        self.test_results = {
            "oracle_basic": {"status": "NOT_TESTED", "errors": []},
            "oracle_string_ids": {"status": "NOT_TESTED", "errors": []},
            "oracle_faiss": {"status": "NOT_TESTED", "errors": []},
            "engine_matching": {"status": "NOT_TESTED", "errors": []},
            "similarity_search": {"status": "NOT_TESTED", "errors": []},
            "fallback_handling": {"status": "NOT_TESTED", "errors": []}
        }
        
        # Test blueprints for different scenarios
        self.test_blueprints = {
            "vintage_warmth": {
                "slots": [
                    {"slot": 1, "engine_id": ENGINE_VINTAGE_TUBE, "character": "warm"},
                    {"slot": 2, "engine_id": ENGINE_TAPE_ECHO, "character": "vintage"},
                    {"slot": 3, "engine_id": ENGINE_PLATE_REVERB, "character": "spacious"}
                ],
                "overall_vibe": "warm vintage with spacious reverb"
            },
            
            "aggressive_modern": {
                "slots": [
                    {"slot": 1, "engine_id": 35, "character": "aggressive"},  # Muff Fuzz
                    {"slot": 2, "engine_id": 20, "character": "punchy"},      # Transient Shaper
                    {"slot": 3, "engine_id": 33, "character": "digital"}     # Bit Crusher
                ],
                "overall_vibe": "aggressive modern distortion"
            },
            
            "ambient_experimental": {
                "slots": [
                    {"slot": 1, "engine_id": 16, "character": "textural"},    # Granular Cloud
                    {"slot": 2, "engine_id": 39, "character": "frozen"},      # Spectral Freeze
                    {"slot": 3, "engine_id": 49, "character": "feedback"}    # Feedback Network
                ],
                "overall_vibe": "ambient experimental texture"
            },
            
            "simple_single": {
                "slots": [
                    {"slot": 1, "engine_id": ENGINE_VINTAGE_TUBE, "character": "warm"}
                ],
                "overall_vibe": "simple warm tone"
            },
            
            "empty_blueprint": {
                "slots": [],
                "overall_vibe": "minimal"
            }
        }
    
    def run_all_tests(self) -> Dict[str, Any]:
        """Run all Oracle tests"""
        logger.info("Starting comprehensive Oracle test suite...")
        
        # Test each Oracle component
        self.test_oracle_basic()
        self.test_oracle_string_ids()
        self.test_oracle_faiss()
        
        # Test specific functionality
        self.test_engine_matching()
        self.test_similarity_search()
        self.test_fallback_handling()
        
        # Generate summary
        self._generate_summary()
        
        return self.test_results
    
    def test_oracle_basic(self):
        """Test basic Oracle component"""
        logger.info("Testing Oracle (basic)...")
        
        try:
            oracle = Oracle()
            
            if len(oracle.corpus) == 0:
                raise Exception("No corpus loaded")
            
            # Test with vintage blueprint
            blueprint = self.test_blueprints["vintage_warmth"]
            preset = oracle.find_best_preset(blueprint)
            
            if not preset or "parameters" not in preset:
                raise Exception("Invalid preset returned")
            
            self.test_results["oracle_basic"]["status"] = "PASSED"
            self.test_results["oracle_basic"]["corpus_size"] = len(oracle.corpus)
            logger.info(f"Oracle basic test PASSED (corpus size: {len(oracle.corpus)})")
            
        except Exception as e:
            self.test_results["oracle_basic"]["status"] = "FAILED"
            self.test_results["oracle_basic"]["errors"].append(str(e))
            logger.error(f"Oracle basic test FAILED: {e}")
    
    def test_oracle_string_ids(self):
        """Test Oracle with string IDs"""
        logger.info("Testing Oracle (string IDs)...")
        
        try:
            oracle = OracleStringIDs()
            
            if len(oracle.presets) == 0:
                raise Exception("No presets loaded")
            
            # Test with modern blueprint
            blueprint = self.test_blueprints["aggressive_modern"]
            preset = oracle.find_best_preset(blueprint)
            
            if not preset or "parameters" not in preset:
                raise Exception("Invalid preset returned")
            
            self.test_results["oracle_string_ids"]["status"] = "PASSED"
            self.test_results["oracle_string_ids"]["presets_count"] = len(oracle.presets)
            self.test_results["oracle_string_ids"]["faiss_vectors"] = oracle.index.ntotal if oracle.index else 0
            logger.info(f"Oracle string IDs test PASSED (presets: {len(oracle.presets)}, vectors: {oracle.index.ntotal})")
            
        except Exception as e:
            self.test_results["oracle_string_ids"]["status"] = "FAILED"
            self.test_results["oracle_string_ids"]["errors"].append(str(e))
            logger.error(f"Oracle string IDs test FAILED: {e}")
    
    def test_oracle_faiss(self):
        """Test Oracle with FAISS"""
        logger.info("Testing Oracle (FAISS)...")
        
        try:
            oracle = OracleFAISS()
            
            if oracle.index.ntotal == 0:
                raise Exception("FAISS index is empty")
            
            if len(oracle.presets) == 0:
                raise Exception("No presets loaded")
            
            # Test with experimental blueprint
            blueprint = self.test_blueprints["ambient_experimental"]
            preset = oracle.find_best_preset(blueprint)
            
            if not preset or "parameters" not in preset:
                raise Exception("Invalid preset returned")
            
            # Test multiple results
            results = oracle.find_best_presets(blueprint, k=3)
            if len(results) == 0:
                raise Exception("No similarity search results")
            
            self.test_results["oracle_faiss"]["status"] = "PASSED"
            self.test_results["oracle_faiss"]["presets_count"] = len(oracle.presets)
            self.test_results["oracle_faiss"]["faiss_vectors"] = oracle.index.ntotal
            self.test_results["oracle_faiss"]["vector_dimension"] = oracle.index.d
            logger.info(f"Oracle FAISS test PASSED (presets: {len(oracle.presets)}, vectors: {oracle.index.ntotal})")
            
        except Exception as e:
            self.test_results["oracle_faiss"]["status"] = "FAILED"
            self.test_results["oracle_faiss"]["errors"].append(str(e))
            logger.error(f"Oracle FAISS test FAILED: {e}")
    
    def test_engine_matching(self):
        """Test engine-based preset matching"""
        logger.info("Testing engine matching functionality...")
        
        try:
            oracle = OracleFAISS()  # Use FAISS for best matching
            
            test_cases = [
                (self.test_blueprints["vintage_warmth"], "vintage"),
                (self.test_blueprints["aggressive_modern"], "aggressive"),
                (self.test_blueprints["simple_single"], "simple")
            ]
            
            matching_results = []
            
            for blueprint, test_name in test_cases:
                results = oracle.find_best_presets(blueprint, k=3)
                
                if not results:
                    raise Exception(f"No matches found for {test_name} blueprint")
                
                # Check that results have similarity scores
                for result in results:
                    if "similarity_score" not in result:
                        raise Exception("Missing similarity score in result")
                
                matching_results.append({
                    "test": test_name,
                    "matches": len(results),
                    "best_score": results[0]["similarity_score"],
                    "preset_name": results[0].get("creative_name", results[0].get("name", "Unknown"))
                })
                
                logger.info(f"  {test_name}: {len(results)} matches, best = {results[0].get('creative_name', 'Unknown')} (score: {results[0]['similarity_score']:.3f})")
            
            self.test_results["engine_matching"]["status"] = "PASSED"
            self.test_results["engine_matching"]["test_results"] = matching_results
            logger.info("Engine matching test PASSED")
            
        except Exception as e:
            self.test_results["engine_matching"]["status"] = "FAILED"
            self.test_results["engine_matching"]["errors"].append(str(e))
            logger.error(f"Engine matching test FAILED: {e}")
    
    def test_similarity_search(self):
        """Test FAISS similarity search with different queries"""
        logger.info("Testing similarity search performance...")
        
        try:
            oracle = OracleFAISS()
            
            # Test different vibe searches
            vibe_tests = [
                {"overall_vibe": "warm vintage analog", "expected_themes": ["warm", "vintage", "analog"]},
                {"overall_vibe": "bright aggressive digital", "expected_themes": ["bright", "aggressive"]},
                {"overall_vibe": "spacious ambient ethereal", "expected_themes": ["spacious", "ambient", "ethereal"]},
                {"overall_vibe": "experimental chaos glitch", "expected_themes": ["experimental", "chaos", "glitch"]}
            ]
            
            search_results = []
            
            for vibe_test in vibe_tests:
                blueprint = {
                    "slots": [],  # No specific engines, just vibe
                    "overall_vibe": vibe_test["overall_vibe"]
                }
                
                results = oracle.find_best_presets(blueprint, k=5)
                
                if not results:
                    raise Exception(f"No results for vibe: {vibe_test['overall_vibe']}")
                
                search_results.append({
                    "vibe": vibe_test["overall_vibe"],
                    "matches": len(results),
                    "top_preset": results[0].get("creative_name", "Unknown")
                })
                
                logger.info(f"  Vibe '{vibe_test['overall_vibe']}': {len(results)} matches, top = {results[0].get('creative_name', 'Unknown')}")
            
            self.test_results["similarity_search"]["status"] = "PASSED"
            self.test_results["similarity_search"]["vibe_tests"] = search_results
            logger.info("Similarity search test PASSED")
            
        except Exception as e:
            self.test_results["similarity_search"]["status"] = "FAILED"
            self.test_results["similarity_search"]["errors"].append(str(e))
            logger.error(f"Similarity search test FAILED: {e}")
    
    def test_fallback_handling(self):
        """Test fallback behavior with edge cases"""
        logger.info("Testing fallback handling...")
        
        try:
            oracle = OracleFAISS()
            
            # Test empty blueprint
            empty_result = oracle.find_best_preset(self.test_blueprints["empty_blueprint"])
            if not empty_result or "parameters" not in empty_result:
                raise Exception("Failed to handle empty blueprint")
            
            # Test invalid engine IDs
            invalid_blueprint = {
                "slots": [
                    {"slot": 1, "engine_id": 9999, "character": "invalid"}  # Invalid engine
                ],
                "overall_vibe": "invalid test"
            }
            
            invalid_result = oracle.find_best_preset(invalid_blueprint)
            if not invalid_result or "parameters" not in invalid_result:
                raise Exception("Failed to handle invalid engine ID")
            
            # Test with no matches (should return default)
            extreme_blueprint = {
                "slots": [
                    {"slot": 1, "engine_id": ENGINE_NONE}  # Bypass only
                ],
                "overall_vibe": "completely empty should use defaults"
            }
            
            default_result = oracle.find_best_preset(extreme_blueprint)
            if not default_result or "parameters" not in default_result:
                raise Exception("Failed to provide default preset")
            
            self.test_results["fallback_handling"]["status"] = "PASSED"
            logger.info("Fallback handling test PASSED")
            
        except Exception as e:
            self.test_results["fallback_handling"]["status"] = "FAILED"
            self.test_results["fallback_handling"]["errors"].append(str(e))
            logger.error(f"Fallback handling test FAILED: {e}")
    
    def _generate_summary(self):
        """Generate test summary"""
        total_tests = len(self.test_results)
        passed_tests = sum(1 for result in self.test_results.values() if result["status"] == "PASSED")
        failed_tests = sum(1 for result in self.test_results.values() if result["status"] == "FAILED")
        
        self.test_results["summary"] = {
            "total_tests": total_tests,
            "passed": passed_tests,
            "failed": failed_tests,
            "success_rate": passed_tests / total_tests if total_tests > 0 else 0
        }
    
    def print_results(self):
        """Print comprehensive test results"""
        print("\n" + "="*80)
        print("ORACLE COMPREHENSIVE TEST RESULTS")
        print("="*80)
        
        for test_name, result in self.test_results.items():
            if test_name == "summary":
                continue
                
            status_icon = "✅" if result["status"] == "PASSED" else "❌" if result["status"] == "FAILED" else "⏸️"
            print(f"\n{status_icon} {test_name.upper().replace('_', ' ')}: {result['status']}")
            
            # Show additional info
            if result["status"] == "PASSED":
                if "corpus_size" in result:
                    print(f"   • Corpus size: {result['corpus_size']} presets")
                if "presets_count" in result:
                    print(f"   • Presets loaded: {result['presets_count']}")
                if "faiss_vectors" in result:
                    print(f"   • FAISS vectors: {result['faiss_vectors']}")
                if "vector_dimension" in result:
                    print(f"   • Vector dimension: {result['vector_dimension']}")
                if "test_results" in result:
                    for test_result in result["test_results"]:
                        print(f"   • {test_result['test']}: {test_result['matches']} matches, best = {test_result['preset_name']}")
            
            elif result["status"] == "FAILED" and result["errors"]:
                for error in result["errors"]:
                    print(f"   ❌ Error: {error}")
        
        # Summary
        if "summary" in self.test_results:
            summary = self.test_results["summary"]
            print(f"\n📊 SUMMARY:")
            print(f"   • Total tests: {summary['total_tests']}")
            print(f"   • Passed: {summary['passed']}")
            print(f"   • Failed: {summary['failed']}")
            print(f"   • Success rate: {summary['success_rate']:.1%}")
        
        print("\n" + "="*80)
    
    def save_results(self, output_path: str = None):
        """Save results to JSON file"""
        if output_path is None:
            output_path = "oracle_test_results.json"
        
        with open(output_path, 'w') as f:
            json.dump(self.test_results, f, indent=2)
        
        logger.info(f"Test results saved to {output_path}")


def main():
    """Run Oracle test suite"""
    test_suite = OracleTestSuite()
    
    try:
        results = test_suite.run_all_tests()
        test_suite.print_results()
        test_suite.save_results()
        
        # Exit with appropriate code
        summary = results.get("summary", {})
        if summary.get("failed", 0) == 0:
            logger.info("All Oracle tests PASSED!")
            return 0
        else:
            logger.error(f"{summary.get('failed', 0)} Oracle tests FAILED")
            return 1
            
    except Exception as e:
        logger.error(f"Test suite execution failed: {e}")
        logger.error(traceback.format_exc())
        return 1


if __name__ == "__main__":
    exit(main())