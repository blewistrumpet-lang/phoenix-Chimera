#!/usr/bin/env python3
"""
Trinity Quality Assurance Agent - Comprehensive Test Suite
Tests the complete Trinity pipeline against golden corpus standards
Generates quality metrics and validation reports
"""

import asyncio
import json
import sys
import time
from pathlib import Path
from typing import Dict, List, Any, Tuple
from dataclasses import dataclass
from statistics import mean, median
import logging

# Add AI_Server to path
sys.path.insert(0, str(Path(__file__).parent))

from main import generate_preset, GenerateRequest
from engine_mapping_authoritative import (
    ENGINE_NAMES, get_engine_name, get_engine_category, 
    DYNAMICS_ENGINES, FILTER_ENGINES, DISTORTION_ENGINES,
    MODULATION_ENGINES, DELAY_REVERB_ENGINES, SPATIAL_ENGINES
)

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class TestResult:
    """Results from a single test prompt"""
    prompt: str
    success: bool
    preset: Dict[str, Any]
    generation_time: float
    error_message: str = ""
    quality_scores: Dict[str, float] = None
    
    def __post_init__(self):
        if self.quality_scores is None:
            self.quality_scores = {}

@dataclass
class QualityMetrics:
    """Quality assessment metrics"""
    engine_selection_accuracy: float = 0.0
    parameter_validity: float = 0.0
    preset_coherence: float = 0.0
    name_relevance: float = 0.0
    overall_score: float = 0.0

class TrinityQualityAssurance:
    """Comprehensive quality assurance for Trinity pipeline"""
    
    def __init__(self):
        self.test_prompts = self._create_test_prompts()
        self.golden_corpus = self._load_golden_corpus()
        self.results: List[TestResult] = []
        
    def _create_test_prompts(self) -> List[Dict[str, Any]]:
        """Create comprehensive test prompts covering all musical styles"""
        return [
            # SIMPLE DESCRIPTIVE PROMPTS
            {"prompt": "warm", "category": "simple", "expected_engines": ["tube", "analog"], "expected_category": "Distortion"},
            {"prompt": "bright", "category": "simple", "expected_engines": ["eq", "exciter"], "expected_category": "Filters & EQ"},
            {"prompt": "aggressive", "category": "simple", "expected_engines": ["distortion", "compressor"], "expected_category": "Distortion"},
            {"prompt": "smooth", "category": "simple", "expected_engines": ["compressor", "eq"], "expected_category": "Dynamics"},
            {"prompt": "crisp", "category": "simple", "expected_engines": ["eq", "enhancer"], "expected_category": "Filters & EQ"},
            
            # COMPLEX DESCRIPTIVE PROMPTS
            {"prompt": "vintage analog warmth with subtle tape saturation", "category": "complex", "expected_engines": ["tube", "tape"], "expected_category": "Distortion"},
            {"prompt": "pristine digital clarity with surgical precision", "category": "complex", "expected_engines": ["eq", "limiter"], "expected_category": "Filters & EQ"},
            {"prompt": "lush analog chorus with vintage character", "category": "complex", "expected_engines": ["chorus", "analog"], "expected_category": "Modulation"},
            {"prompt": "ethereal shimmer with celestial reverb tails", "category": "complex", "expected_engines": ["shimmer", "reverb"], "expected_category": "Delay & Reverb"},
            {"prompt": "punchy compression with musical transparency", "category": "complex", "expected_engines": ["compressor", "opto"], "expected_category": "Dynamics"},
            
            # GENRE-SPECIFIC PROMPTS
            {"prompt": "80s synthwave nostalgic glow", "category": "genre", "expected_engines": ["chorus", "analog"], "expected_category": "Modulation"},
            {"prompt": "modern trap aggressive punch", "category": "genre", "expected_engines": ["compressor", "distortion"], "expected_category": "Dynamics"},
            {"prompt": "ambient drone expansive space", "category": "genre", "expected_engines": ["reverb", "delay"], "expected_category": "Delay & Reverb"},
            {"prompt": "jazz fusion smooth sophistication", "category": "genre", "expected_engines": ["chorus", "compressor"], "expected_category": "Modulation"},
            {"prompt": "metal brutal distortion", "category": "genre", "expected_engines": ["distortion", "gate"], "expected_category": "Distortion"},
            {"prompt": "lo-fi hip hop vinyl character", "category": "genre", "expected_engines": ["bit_crusher", "tape"], "expected_category": "Distortion"},
            
            # TECHNICAL EQUIPMENT PROMPTS
            {"prompt": "LA-2A opto compressor vintage", "category": "technical", "expected_engines": ["opto_compressor"], "expected_category": "Dynamics"},
            {"prompt": "Moog ladder filter resonance", "category": "technical", "expected_engines": ["ladder_filter"], "expected_category": "Filters & EQ"},
            {"prompt": "shimmer reverb ethereal", "category": "technical", "expected_engines": ["shimmer_reverb"], "expected_category": "Delay & Reverb"},
            {"prompt": "1176 FET compressor attack", "category": "technical", "expected_engines": ["vca_compressor"], "expected_category": "Dynamics"},
            {"prompt": "Lexicon plate reverb hall", "category": "technical", "expected_engines": ["plate_reverb"], "expected_category": "Delay & Reverb"},
            {"prompt": "Echoplex tape delay vintage", "category": "technical", "expected_engines": ["tape_echo"], "expected_category": "Delay & Reverb"},
            {"prompt": "API console EQ character", "category": "technical", "expected_engines": ["vintage_console_eq"], "expected_category": "Filters & EQ"},
            
            # CREATIVE ARTISTIC PROMPTS
            {"prompt": "underwater cathedral acoustics", "category": "creative", "expected_engines": ["reverb", "filter"], "expected_category": "Delay & Reverb"},
            {"prompt": "broken radio transmission", "category": "creative", "expected_engines": ["bit_crusher", "gate"], "expected_category": "Distortion"},
            {"prompt": "cosmic wind through crystal caves", "category": "creative", "expected_engines": ["shimmer", "delay"], "expected_category": "Delay & Reverb"},
            {"prompt": "industrial machinery rhythmic", "category": "creative", "expected_engines": ["distortion", "tremolo"], "expected_category": "Distortion"},
            {"prompt": "silk curtains in moonlight", "category": "creative", "expected_engines": ["chorus", "reverb"], "expected_category": "Modulation"},
        ]
    
    def _load_golden_corpus(self) -> List[Dict[str, Any]]:
        """Load golden corpus presets for comparison"""
        try:
            corpus_path = Path("../JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets.json")
            if corpus_path.exists():
                with open(corpus_path, 'r') as f:
                    return json.load(f)
            else:
                logger.warning(f"Golden corpus not found at {corpus_path}")
                return []
        except Exception as e:
            logger.error(f"Error loading golden corpus: {e}")
            return []
    
    async def run_comprehensive_tests(self) -> Dict[str, Any]:
        """Run all quality assurance tests"""
        print("\n" + "=" * 80)
        print("TRINITY QUALITY ASSURANCE - COMPREHENSIVE TEST SUITE")
        print("=" * 80)
        print(f"Testing {len(self.test_prompts)} prompts against Trinity pipeline")
        print(f"Golden corpus: {len(self.golden_corpus)} reference presets")
        print("=" * 80)
        
        start_time = time.time()
        
        # Run all test prompts
        for i, test_data in enumerate(self.test_prompts):
            print(f"\n[{i+1:2d}/{len(self.test_prompts)}] Testing: '{test_data['prompt']}'")
            print(f"         Category: {test_data['category']}")
            
            result = await self._test_single_prompt(test_data)
            self.results.append(result)
            
            if result.success:
                print(f"         ✓ Generated: '{result.preset.get('name', 'Unnamed')}'")
                print(f"         ⏱ Time: {result.generation_time:.2f}s")
            else:
                print(f"         ✗ Failed: {result.error_message}")
            
            # Small delay between tests
            await asyncio.sleep(0.5)
        
        total_time = time.time() - start_time
        
        # Analyze results and generate report
        print(f"\n" + "=" * 80)
        print("ANALYZING RESULTS AND GENERATING QUALITY REPORT")
        print("=" * 80)
        
        quality_report = self._analyze_quality_metrics()
        corpus_comparison = self._compare_to_golden_corpus()
        
        final_report = {
            "test_execution": {
                "total_tests": len(self.test_prompts),
                "successful_tests": sum(1 for r in self.results if r.success),
                "failed_tests": sum(1 for r in self.results if not r.success),
                "total_time_seconds": round(total_time, 2),
                "average_generation_time": round(mean([r.generation_time for r in self.results if r.success]), 2) if any(r.success for r in self.results) else 0
            },
            "quality_metrics": quality_report,
            "corpus_comparison": corpus_comparison,
            "detailed_results": [self._result_to_dict(r) for r in self.results],
            "recommendations": self._generate_recommendations()
        }
        
        # Save detailed report
        report_path = Path("trinity_qa_report.json")
        with open(report_path, 'w') as f:
            json.dump(final_report, f, indent=2)
        
        # Print summary
        self._print_quality_summary(final_report)
        
        return final_report
    
    async def _test_single_prompt(self, test_data: Dict[str, Any]) -> TestResult:
        """Test a single prompt and measure quality"""
        prompt = test_data["prompt"]
        start_time = time.time()
        
        try:
            request = GenerateRequest(prompt=prompt)
            response = await generate_preset(request)
            generation_time = time.time() - start_time
            
            if response.success:
                quality_scores = self._assess_preset_quality(response.preset, test_data)
                return TestResult(
                    prompt=prompt,
                    success=True,
                    preset=response.preset,
                    generation_time=generation_time,
                    quality_scores=quality_scores
                )
            else:
                return TestResult(
                    prompt=prompt,
                    success=False,
                    preset={},
                    generation_time=generation_time,
                    error_message=response.message
                )
                
        except Exception as e:
            generation_time = time.time() - start_time
            return TestResult(
                prompt=prompt,
                success=False,
                preset={},
                generation_time=generation_time,
                error_message=str(e)
            )
    
    def _assess_preset_quality(self, preset: Dict[str, Any], test_data: Dict[str, Any]) -> Dict[str, float]:
        """Assess quality of a generated preset"""
        scores = {}
        
        # 1. Engine Selection Accuracy
        scores["engine_selection"] = self._score_engine_selection(preset, test_data)
        
        # 2. Parameter Musical Validity
        scores["parameter_validity"] = self._score_parameter_validity(preset)
        
        # 3. Preset Coherence
        scores["preset_coherence"] = self._score_preset_coherence(preset)
        
        # 4. Name Relevance
        scores["name_relevance"] = self._score_name_relevance(preset, test_data["prompt"])
        
        # 5. Overall Score (weighted average)
        scores["overall"] = (
            scores["engine_selection"] * 0.3 +
            scores["parameter_validity"] * 0.25 +
            scores["preset_coherence"] * 0.25 +
            scores["name_relevance"] * 0.2
        )
        
        return scores
    
    def _score_engine_selection(self, preset: Dict[str, Any], test_data: Dict[str, Any]) -> float:
        """Score how well selected engines match the prompt intent"""
        params = preset.get("parameters", {})
        active_engines = []
        
        # Find active engines
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            bypass = params.get(f"slot{slot}_bypass", 1.0)
            if engine_id > 0 and bypass < 0.5:  # Active engine
                engine_name = get_engine_name(engine_id).lower()
                active_engines.append(engine_name)
        
        if not active_engines:
            return 0.0
        
        # Check if expected engines are present
        expected = test_data.get("expected_engines", [])
        expected_category = test_data.get("expected_category", "")
        
        score = 0.0
        
        # Direct engine name matches (60% weight)
        if expected:
            matches = 0
            for exp in expected:
                for active in active_engines:
                    if exp.lower() in active or active in exp.lower():
                        matches += 1
                        break
            score += 0.6 * (matches / len(expected))
        
        # Category matches (40% weight)
        if expected_category:
            category_score = 0.0
            for slot in range(1, 7):
                engine_id = params.get(f"slot{slot}_engine", 0)
                bypass = params.get(f"slot{slot}_bypass", 1.0)
                if engine_id > 0 and bypass < 0.5:
                    if get_engine_category(engine_id) == expected_category:
                        category_score += 1.0
            if category_score > 0:
                score += 0.4 * min(1.0, category_score / len(active_engines))
        
        return min(1.0, score)
    
    def _score_parameter_validity(self, preset: Dict[str, Any]) -> float:
        """Score if parameters are in musical/useful ranges"""
        params = preset.get("parameters", {})
        valid_params = 0
        total_params = 0
        
        for param_name, value in params.items():
            if "param" in param_name and isinstance(value, (int, float)):
                total_params += 1
                
                # Parameters should generally be in 0.0-1.0 range
                if 0.0 <= value <= 1.0:
                    valid_params += 1
                    
                    # Bonus for non-extreme values (more musical)
                    if 0.1 <= value <= 0.9:
                        valid_params += 0.5
        
        return (valid_params / total_params) if total_params > 0 else 0.0
    
    def _score_preset_coherence(self, preset: Dict[str, Any]) -> float:
        """Score if engines work well together (no conflicts)"""
        params = preset.get("parameters", {})
        active_engines = []
        active_categories = []
        
        # Get active engines and categories
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            bypass = params.get(f"slot{slot}_bypass", 1.0)
            if engine_id > 0 and bypass < 0.5:
                active_engines.append(engine_id)
                active_categories.append(get_engine_category(engine_id))
        
        if len(active_engines) == 0:
            return 0.0
        
        score = 1.0
        
        # Penalty for too many distortion effects
        distortion_count = sum(1 for cat in active_categories if cat == "Distortion")
        if distortion_count > 2:
            score -= 0.3
        
        # Penalty for conflicting reverbs
        reverb_count = sum(1 for eid in active_engines if eid in DELAY_REVERB_ENGINES)
        if reverb_count > 2:
            score -= 0.2
        
        # Bonus for logical signal flow (dynamics first, reverb last)
        if active_engines:
            first_engine = active_engines[0]
            last_engine = active_engines[-1]
            
            if first_engine in DYNAMICS_ENGINES:
                score += 0.1
            if last_engine in DELAY_REVERB_ENGINES:
                score += 0.1
        
        return max(0.0, min(1.0, score))
    
    def _score_name_relevance(self, preset: Dict[str, Any], prompt: str) -> float:
        """Score how well the preset name matches the prompt"""
        name = preset.get("name", "").lower()
        prompt_words = set(prompt.lower().split())
        name_words = set(name.lower().split())
        
        if not name or not prompt_words:
            return 0.0
        
        # Calculate word overlap
        overlap = len(prompt_words.intersection(name_words))
        relevance = overlap / len(prompt_words)
        
        # Bonus for creative interpretation
        creative_bonus = 0.0
        if len(name_words) > len(prompt_words):  # Name is more descriptive
            creative_bonus = 0.2
        
        return min(1.0, relevance + creative_bonus)
    
    def _analyze_quality_metrics(self) -> Dict[str, Any]:
        """Analyze overall quality metrics across all tests"""
        successful_results = [r for r in self.results if r.success]
        
        if not successful_results:
            return {"error": "No successful tests to analyze"}
        
        # Calculate averages for each metric
        engine_scores = [r.quality_scores.get("engine_selection", 0) for r in successful_results]
        param_scores = [r.quality_scores.get("parameter_validity", 0) for r in successful_results]
        coherence_scores = [r.quality_scores.get("preset_coherence", 0) for r in successful_results]
        name_scores = [r.quality_scores.get("name_relevance", 0) for r in successful_results]
        overall_scores = [r.quality_scores.get("overall", 0) for r in successful_results]
        
        return {
            "engine_selection_accuracy": {
                "average": round(mean(engine_scores), 3),
                "median": round(median(engine_scores), 3),
                "min": round(min(engine_scores), 3),
                "max": round(max(engine_scores), 3)
            },
            "parameter_validity": {
                "average": round(mean(param_scores), 3),
                "median": round(median(param_scores), 3),
                "min": round(min(param_scores), 3),
                "max": round(max(param_scores), 3)
            },
            "preset_coherence": {
                "average": round(mean(coherence_scores), 3),
                "median": round(median(coherence_scores), 3),
                "min": round(min(coherence_scores), 3),
                "max": round(max(coherence_scores), 3)
            },
            "name_relevance": {
                "average": round(mean(name_scores), 3),
                "median": round(median(name_scores), 3),
                "min": round(min(name_scores), 3),
                "max": round(max(name_scores), 3)
            },
            "overall_quality": {
                "average": round(mean(overall_scores), 3),
                "median": round(median(overall_scores), 3),
                "min": round(min(overall_scores), 3),
                "max": round(max(overall_scores), 3)
            }
        }
    
    def _compare_to_golden_corpus(self) -> Dict[str, Any]:
        """Compare generated presets to golden corpus standards"""
        if not self.golden_corpus:
            return {"error": "No golden corpus available for comparison"}
        
        successful_results = [r for r in self.results if r.success]
        
        # Analyze corpus characteristics
        corpus_engines = []
        corpus_params = []
        
        for corpus_preset in self.golden_corpus:
            params = corpus_preset.get("parameters", {})
            for slot in range(1, 7):
                engine_id = params.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    corpus_engines.append(engine_id)
            
            # Sample parameter values
            for param_name, value in params.items():
                if "param" in param_name and isinstance(value, (int, float)):
                    corpus_params.append(value)
        
        # Compare generated presets
        generated_engines = []
        generated_params = []
        
        for result in successful_results:
            params = result.preset.get("parameters", {})
            for slot in range(1, 7):
                engine_id = params.get(f"slot{slot}_engine", 0)
                bypass = params.get(f"slot{slot}_bypass", 1.0)
                if engine_id > 0 and bypass < 0.5:
                    generated_engines.append(engine_id)
            
            for param_name, value in params.items():
                if "param" in param_name and isinstance(value, (int, float)):
                    generated_params.append(value)
        
        return {
            "corpus_stats": {
                "total_presets": len(self.golden_corpus),
                "unique_engines": len(set(corpus_engines)),
                "avg_engines_per_preset": round(len(corpus_engines) / len(self.golden_corpus), 2),
                "avg_param_value": round(mean(corpus_params), 3) if corpus_params else 0
            },
            "generated_stats": {
                "total_presets": len(successful_results),
                "unique_engines": len(set(generated_engines)),
                "avg_engines_per_preset": round(len(generated_engines) / len(successful_results), 2) if successful_results else 0,
                "avg_param_value": round(mean(generated_params), 3) if generated_params else 0
            },
            "comparison": {
                "engine_diversity_ratio": round(len(set(generated_engines)) / len(set(corpus_engines)), 3) if corpus_engines else 0,
                "param_similarity": self._calculate_param_similarity(corpus_params, generated_params)
            }
        }
    
    def _calculate_param_similarity(self, corpus_params: List[float], generated_params: List[float]) -> float:
        """Calculate similarity between parameter distributions"""
        if not corpus_params or not generated_params:
            return 0.0
        
        corpus_avg = mean(corpus_params)
        generated_avg = mean(generated_params)
        
        # Simple similarity based on average difference
        diff = abs(corpus_avg - generated_avg)
        similarity = max(0.0, 1.0 - diff)
        
        return round(similarity, 3)
    
    def _generate_recommendations(self) -> List[str]:
        """Generate specific recommendations for improvement"""
        recommendations = []
        successful_results = [r for r in self.results if r.success]
        
        if not successful_results:
            recommendations.append("CRITICAL: No successful preset generations - check pipeline configuration")
            return recommendations
        
        # Calculate averages
        avg_engine_score = mean([r.quality_scores.get("engine_selection", 0) for r in successful_results])
        avg_param_score = mean([r.quality_scores.get("parameter_validity", 0) for r in successful_results])
        avg_coherence_score = mean([r.quality_scores.get("preset_coherence", 0) for r in successful_results])
        avg_name_score = mean([r.quality_scores.get("name_relevance", 0) for r in successful_results])
        
        # Engine selection recommendations
        if avg_engine_score < 0.7:
            recommendations.append(f"Improve engine selection accuracy (current: {avg_engine_score:.2f}) - review Visionary blueprint generation")
        
        # Parameter validity recommendations
        if avg_param_score < 0.8:
            recommendations.append(f"Improve parameter validity (current: {avg_param_score:.2f}) - review Calculator nudge rules")
        
        # Coherence recommendations
        if avg_coherence_score < 0.8:
            recommendations.append(f"Improve preset coherence (current: {avg_coherence_score:.2f}) - add conflict detection to Alchemist")
        
        # Name relevance recommendations
        if avg_name_score < 0.6:
            recommendations.append(f"Improve name relevance (current: {avg_name_score:.2f}) - enhance Alchemist name generation")
        
        # Success rate recommendations
        success_rate = len(successful_results) / len(self.results)
        if success_rate < 0.9:
            recommendations.append(f"Improve success rate (current: {success_rate:.2f}) - investigate pipeline failures")
        
        # Performance recommendations
        avg_time = mean([r.generation_time for r in successful_results])
        if avg_time > 10.0:
            recommendations.append(f"Optimize generation speed (current: {avg_time:.2f}s) - consider caching or optimization")
        
        if not recommendations:
            recommendations.append("Excellent! Trinity pipeline is performing at high quality standards.")
        
        return recommendations
    
    def _result_to_dict(self, result: TestResult) -> Dict[str, Any]:
        """Convert TestResult to dictionary for JSON serialization"""
        return {
            "prompt": result.prompt,
            "success": result.success,
            "generation_time": round(result.generation_time, 3),
            "error_message": result.error_message,
            "quality_scores": result.quality_scores,
            "preset_name": result.preset.get("name", "") if result.success else "",
            "preset_vibe": result.preset.get("vibe", "") if result.success else "",
            "active_engines": self._get_active_engines(result.preset) if result.success else []
        }
    
    def _get_active_engines(self, preset: Dict[str, Any]) -> List[str]:
        """Get list of active engine names from preset"""
        params = preset.get("parameters", {})
        active = []
        
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            bypass = params.get(f"slot{slot}_bypass", 1.0)
            if engine_id > 0 and bypass < 0.5:
                active.append(get_engine_name(engine_id))
        
        return active
    
    def _print_quality_summary(self, report: Dict[str, Any]):
        """Print a comprehensive quality summary"""
        print("\n" + "=" * 80)
        print("TRINITY QUALITY ASSURANCE - FINAL REPORT")
        print("=" * 80)
        
        exec_data = report["test_execution"]
        quality_data = report["quality_metrics"]
        
        print(f"\n📊 TEST EXECUTION SUMMARY:")
        print(f"   Total Tests: {exec_data['total_tests']}")
        print(f"   ✓ Successful: {exec_data['successful_tests']}")
        print(f"   ✗ Failed: {exec_data['failed_tests']}")
        print(f"   Success Rate: {exec_data['successful_tests']/exec_data['total_tests']:.1%}")
        print(f"   Total Time: {exec_data['total_time_seconds']}s")
        print(f"   Avg Generation: {exec_data['average_generation_time']}s")
        
        if "error" not in quality_data:
            print(f"\n🎯 QUALITY METRICS (0.0-1.0 scale):")
            print(f"   Engine Selection: {quality_data['engine_selection_accuracy']['average']:.3f}")
            print(f"   Parameter Validity: {quality_data['parameter_validity']['average']:.3f}")
            print(f"   Preset Coherence: {quality_data['preset_coherence']['average']:.3f}")
            print(f"   Name Relevance: {quality_data['name_relevance']['average']:.3f}")
            print(f"   📈 OVERALL SCORE: {quality_data['overall_quality']['average']:.3f}")
            
            # Quality verdict
            overall_score = quality_data['overall_quality']['average']
            if overall_score >= 0.9:
                verdict = "🏆 EXCELLENT - Production Ready"
            elif overall_score >= 0.8:
                verdict = "🥈 GOOD - Minor improvements needed"
            elif overall_score >= 0.7:
                verdict = "🥉 ACCEPTABLE - Significant improvements needed"
            else:
                verdict = "⚠️  NEEDS WORK - Major issues to address"
            
            print(f"\n🏁 FINAL VERDICT: {verdict}")
        
        print(f"\n💡 RECOMMENDATIONS:")
        for i, rec in enumerate(report["recommendations"], 1):
            print(f"   {i}. {rec}")
        
        print(f"\n📁 Detailed report saved to: trinity_qa_report.json")
        print("=" * 80)

async def main():
    """Run the comprehensive Trinity QA test suite"""
    qa = TrinityQualityAssurance()
    await qa.run_comprehensive_tests()

if __name__ == "__main__":
    asyncio.run(main())