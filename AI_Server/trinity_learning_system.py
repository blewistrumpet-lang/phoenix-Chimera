"""
Trinity Learning System - Hybrid RLHF + Evolutionary Algorithm
Learns to generate better presets through iterative testing and feedback
"""

import numpy as np
import json
import asyncio
import random
from typing import Dict, Any, List, Tuple
from dataclasses import dataclass, asdict
from pathlib import Path
import logging
from datetime import datetime
import hashlib

logger = logging.getLogger(__name__)

@dataclass
class PresetEvaluation:
    """Stores evaluation data for a generated preset"""
    prompt: str
    preset: Dict[str, Any]
    
    # Automatic metrics (no human needed)
    prompt_engine_match: float  # 0-1: How well engines match prompt
    parameter_diversity: float  # 0-1: Parameter variation
    engine_utilization: float   # 0-1: How many slots used effectively
    sonic_coherence: float     # 0-1: Do engines work well together
    
    # Human metrics (when available)
    human_rating: float = None  # 1-10 scale
    human_feedback: str = None
    
    # Meta
    timestamp: str = None
    generation_method: str = "trinity"
    pipeline_config: Dict = None
    
    def __post_init__(self):
        if self.timestamp is None:
            self.timestamp = datetime.now().isoformat()
    
    @property
    def automatic_score(self) -> float:
        """Combined automatic score (no human needed)"""
        return (
            self.prompt_engine_match * 0.4 +
            self.parameter_diversity * 0.2 +
            self.engine_utilization * 0.2 +
            self.sonic_coherence * 0.2
        )
    
    @property
    def combined_score(self) -> float:
        """Combined score including human feedback if available"""
        if self.human_rating is not None:
            # Weight human rating heavily when available
            return (self.human_rating / 10.0) * 0.7 + self.automatic_score * 0.3
        return self.automatic_score


class PipelineGenome:
    """Represents a Trinity pipeline configuration that can evolve"""
    
    def __init__(self, config: Dict[str, Any] = None):
        if config is None:
            config = self.default_config()
        
        self.config = config
        self.fitness = 0.0
        self.evaluations = []
        self.generation = 0
        self.id = hashlib.md5(json.dumps(config, sort_keys=True).encode()).hexdigest()[:8]
    
    @staticmethod
    def default_config() -> Dict[str, Any]:
        """Default pipeline configuration"""
        return {
            # Visionary parameters
            "visionary": {
                "temperature": 0.7,
                "use_cloud": True,
                "keyword_weight": 1.0,
                "creativity_bias": 0.5
            },
            
            # Oracle parameters
            "oracle": {
                "engine_weight": 10.0,
                "vibe_weight": 1.0,
                "search_k": 5,
                "similarity_threshold": 0.3
            },
            
            # Calculator parameters
            "calculator": {
                "nudge_intensity": 0.5,
                "keyword_sensitivity": 1.0,
                "harmonic_balance": 0.5,
                "parameter_range": 0.3
            },
            
            # Alchemist parameters
            "alchemist": {
                "validation_strictness": 0.5,
                "name_creativity": 0.7,
                "safety_threshold": 0.8
            }
        }
    
    def mutate(self, mutation_rate: float = 0.1) -> 'PipelineGenome':
        """Create mutated version of this genome"""
        mutated_config = json.loads(json.dumps(self.config))  # Deep copy
        
        def mutate_value(value, rate):
            if random.random() < rate:
                if isinstance(value, float):
                    # Gaussian mutation for floats
                    return np.clip(value + np.random.normal(0, 0.1), 0.0, 1.0)
                elif isinstance(value, bool):
                    # Flip boolean
                    return not value
            return value
        
        # Recursively mutate configuration
        for component, params in mutated_config.items():
            for param, value in params.items():
                params[param] = mutate_value(value, mutation_rate)
        
        child = PipelineGenome(mutated_config)
        child.generation = self.generation + 1
        return child
    
    def crossover(self, other: 'PipelineGenome') -> 'PipelineGenome':
        """Create offspring from two parent genomes"""
        child_config = {}
        
        # Component-level crossover
        for component in self.config:
            if random.random() < 0.5:
                child_config[component] = json.loads(json.dumps(self.config[component]))
            else:
                child_config[component] = json.loads(json.dumps(other.config[component]))
        
        child = PipelineGenome(child_config)
        child.generation = max(self.generation, other.generation) + 1
        return child


class TrinityLearningSystem:
    """
    Main learning system that combines:
    - Evolutionary optimization of pipeline parameters
    - Reinforcement learning from human feedback
    - Automatic evaluation metrics
    - Active learning for efficient human labeling
    """
    
    def __init__(self, 
                 population_size: int = 50,
                 elite_size: int = 10,
                 mutation_rate: float = 0.1,
                 crossover_rate: float = 0.7):
        
        self.population_size = population_size
        self.elite_size = elite_size
        self.mutation_rate = mutation_rate
        self.crossover_rate = crossover_rate
        
        # Initialize population
        self.population = [PipelineGenome() for _ in range(population_size)]
        self.generation = 0
        
        # Evaluation history
        self.all_evaluations = []
        self.best_genome_history = []
        
        # Prompt corpus for testing
        self.test_prompts = self.load_test_prompts()
        
        # Metrics
        self.metrics = {
            "avg_fitness": [],
            "best_fitness": [],
            "diversity": [],
            "human_agreement": []
        }
    
    def load_test_prompts(self) -> List[str]:
        """Load or generate test prompts for evaluation"""
        prompts = [
            # Basic instrument processing
            "warm vintage guitar tone",
            "punchy drum bus compression",
            "silky smooth vocal chain",
            "deep analog bass",
            "bright piano sparkle",
            
            # Genre-specific
            "aggressive metal distortion",
            "lo-fi hip hop vibe",
            "ethereal ambient pad",
            "funky disco bass",
            "modern pop vocal",
            
            # Creative/experimental
            "psychedelic space echo",
            "glitchy electronic textures",
            "vintage tape saturation",
            "cinematic tension builder",
            "experimental noise generator",
            
            # Technical requests
            "mastering chain with warmth",
            "transparent mix bus glue",
            "surgical EQ for vocals",
            "parallel compression for drums",
            "stereo widening for guitars",
        ]
        
        # Generate variations
        variations = []
        modifiers = ["subtle", "extreme", "vintage", "modern", "warm", "bright", "dark", "aggressive"]
        for prompt in prompts:
            for modifier in modifiers[:3]:  # Don't explode the size
                variations.append(f"{modifier} {prompt}")
        
        return prompts + variations
    
    async def evaluate_genome(self, genome: PipelineGenome, num_tests: int = 20) -> float:
        """Evaluate a genome's fitness using automatic metrics"""
        evaluations = []
        
        # Sample random prompts for testing
        test_prompts = random.sample(self.test_prompts, min(num_tests, len(self.test_prompts)))
        
        for prompt in test_prompts:
            # Generate preset using this genome's configuration
            preset = await self.generate_with_config(prompt, genome.config)
            
            # Calculate automatic metrics
            evaluation = PresetEvaluation(
                prompt=prompt,
                preset=preset,
                prompt_engine_match=self.calculate_engine_match(prompt, preset),
                parameter_diversity=self.calculate_parameter_diversity(preset),
                engine_utilization=self.calculate_engine_utilization(preset),
                sonic_coherence=self.calculate_sonic_coherence(preset),
                pipeline_config=genome.config
            )
            
            evaluations.append(evaluation)
            genome.evaluations.append(evaluation)
        
        # Calculate fitness as average score
        genome.fitness = np.mean([e.automatic_score for e in evaluations])
        return genome.fitness
    
    async def generate_with_config(self, prompt: str, config: Dict[str, Any]) -> Dict[str, Any]:
        """Generate a preset using specific pipeline configuration"""
        # This would call the actual Trinity pipeline with modified parameters
        # For now, we'll simulate it
        import requests
        
        try:
            # Inject configuration into the generation request
            response = requests.post(
                "http://localhost:8000/generate",
                json={
                    "prompt": prompt,
                    "config_override": config  # Pass configuration to pipeline
                },
                timeout=5
            )
            
            if response.status_code == 200:
                data = response.json()
                if data["success"]:
                    return data["preset"]
        except:
            pass
        
        # Return empty preset on failure
        return {"parameters": {}, "name": "Failed Generation"}
    
    def calculate_engine_match(self, prompt: str, preset: Dict[str, Any]) -> float:
        """Calculate how well selected engines match the prompt"""
        from engine_mapping_authoritative import *
        
        prompt_lower = prompt.lower()
        params = preset.get("parameters", {})
        
        # Extract active engines
        active_engines = []
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                active_engines.append(engine_id)
        
        if not active_engines:
            return 0.0
        
        # Check for expected engines based on prompt keywords
        expected_engines = set()
        
        # Map keywords to engine IDs
        keyword_map = {
            "distortion": DISTORTION_ENGINES,
            "reverb": [ENGINE_PLATE_REVERB, ENGINE_SPRING_REVERB, ENGINE_CONVOLUTION_REVERB],
            "delay": [ENGINE_TAPE_ECHO, ENGINE_DIGITAL_DELAY],
            "compression": [ENGINE_OPTO_COMPRESSOR, ENGINE_VCA_COMPRESSOR],
            "eq": [ENGINE_PARAMETRIC_EQ, ENGINE_VINTAGE_CONSOLE_EQ],
            "filter": FILTER_ENGINES,
            "chorus": [ENGINE_DIGITAL_CHORUS, ENGINE_RESONANT_CHORUS],
            "phaser": [ENGINE_ANALOG_PHASER],
        }
        
        for keyword, engines in keyword_map.items():
            if keyword in prompt_lower:
                expected_engines.update(engines)
        
        if not expected_engines:
            # No specific engines expected, check if we have reasonable variety
            return min(len(active_engines) / 3.0, 1.0)
        
        # Calculate match ratio
        matches = len(set(active_engines).intersection(expected_engines))
        return matches / max(len(expected_engines), 1)
    
    def calculate_parameter_diversity(self, preset: Dict[str, Any]) -> float:
        """Calculate parameter diversity (avoid all 0.5 defaults)"""
        params = preset.get("parameters", {})
        
        param_values = []
        for key, value in params.items():
            if "param" in key and isinstance(value, (int, float)):
                param_values.append(value)
        
        if not param_values:
            return 0.0
        
        # Calculate standard deviation as measure of diversity
        std_dev = np.std(param_values)
        # Normalize to 0-1 range (0.5 std dev is considered very diverse)
        return min(std_dev * 2, 1.0)
    
    def calculate_engine_utilization(self, preset: Dict[str, Any]) -> float:
        """Calculate how effectively slots are utilized"""
        params = preset.get("parameters", {})
        
        active_slots = 0
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                active_slots += 1
        
        # Optimal is 3-4 active slots
        if active_slots == 0:
            return 0.0
        elif active_slots <= 2:
            return active_slots * 0.3
        elif active_slots <= 4:
            return 1.0
        else:
            return 0.8  # Slight penalty for too many engines
    
    def calculate_sonic_coherence(self, preset: Dict[str, Any]) -> float:
        """Calculate if engines work well together"""
        from engine_mapping_authoritative import *
        
        params = preset.get("parameters", {})
        
        # Extract active engines with categories
        active_categories = []
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                category = get_engine_category(engine_id)
                active_categories.append(category)
        
        if not active_categories:
            return 0.0
        
        # Good combinations
        coherence_score = 1.0
        
        # Penalize multiple engines from same category (except desired combinations)
        category_counts = {}
        for cat in active_categories:
            category_counts[cat] = category_counts.get(cat, 0) + 1
        
        for category, count in category_counts.items():
            if count > 2 and category != "Delay & Reverb":
                coherence_score *= 0.7  # Penalty for redundancy
        
        # Bonus for good combinations
        categories_set = set(active_categories)
        if "Dynamics" in categories_set and "Distortion" in categories_set:
            coherence_score *= 1.1  # Good combo
        if "Filters & EQ" in categories_set and "Delay & Reverb" in categories_set:
            coherence_score *= 1.1  # Good combo
        
        return min(coherence_score, 1.0)
    
    async def evolve_generation(self):
        """Run one generation of evolution"""
        logger.info(f"Generation {self.generation} starting...")
        
        # Evaluate all genomes
        for genome in self.population:
            await self.evaluate_genome(genome)
        
        # Sort by fitness
        self.population.sort(key=lambda g: g.fitness, reverse=True)
        
        # Record best genome
        best = self.population[0]
        self.best_genome_history.append(best)
        logger.info(f"Best fitness: {best.fitness:.3f} (Genome {best.id})")
        
        # Create next generation
        next_generation = []
        
        # Keep elite
        next_generation.extend(self.population[:self.elite_size])
        
        # Generate offspring
        while len(next_generation) < self.population_size:
            # Tournament selection
            parent1 = self.tournament_select()
            parent2 = self.tournament_select()
            
            if random.random() < self.crossover_rate:
                child = parent1.crossover(parent2)
            else:
                child = parent1
            
            # Mutation
            child = child.mutate(self.mutation_rate)
            next_generation.append(child)
        
        self.population = next_generation
        self.generation += 1
        
        # Update metrics
        fitnesses = [g.fitness for g in self.population]
        self.metrics["avg_fitness"].append(np.mean(fitnesses))
        self.metrics["best_fitness"].append(max(fitnesses))
        self.metrics["diversity"].append(np.std(fitnesses))
    
    def tournament_select(self, tournament_size: int = 5) -> PipelineGenome:
        """Select genome using tournament selection"""
        tournament = random.sample(self.population, min(tournament_size, len(self.population)))
        return max(tournament, key=lambda g: g.fitness)
    
    async def active_learning_select(self, n_samples: int = 10) -> List[PresetEvaluation]:
        """Select most informative presets for human evaluation"""
        # Select presets where automatic metrics disagree most
        # Or where we have highest uncertainty
        
        all_recent = []
        for genome in self.population[:20]:  # Top 20 genomes
            all_recent.extend(genome.evaluations[-10:])  # Recent evaluations
        
        if not all_recent:
            return []
        
        # Sort by uncertainty (variance in automatic metrics)
        def uncertainty(eval):
            scores = [
                eval.prompt_engine_match,
                eval.parameter_diversity,
                eval.engine_utilization,
                eval.sonic_coherence
            ]
            return np.std(scores)
        
        all_recent.sort(key=uncertainty, reverse=True)
        return all_recent[:n_samples]
    
    def update_with_human_feedback(self, evaluation: PresetEvaluation, rating: float, feedback: str = ""):
        """Update genome fitness based on human feedback"""
        evaluation.human_rating = rating
        evaluation.human_feedback = feedback
        
        # Find genome that created this preset
        for genome in self.population:
            if evaluation in genome.evaluations:
                # Recalculate fitness with human feedback
                scores = [e.combined_score for e in genome.evaluations]
                genome.fitness = np.mean(scores)
                break
    
    async def run_training(self, num_generations: int = 100):
        """Run full training loop"""
        logger.info(f"Starting training for {num_generations} generations...")
        
        for gen in range(num_generations):
            await self.evolve_generation()
            
            # Every 10 generations, request human feedback on uncertain cases
            if gen % 10 == 0 and gen > 0:
                uncertain_samples = await self.active_learning_select()
                logger.info(f"Generation {gen}: Requesting human feedback on {len(uncertain_samples)} samples")
                # In real implementation, this would save samples for human review
            
            # Log progress
            if gen % 5 == 0:
                best = self.population[0]
                logger.info(f"Gen {gen}: Best fitness={best.fitness:.3f}, Avg={self.metrics['avg_fitness'][-1]:.3f}")
                
                # Save checkpoint
                self.save_checkpoint(f"checkpoints/gen_{gen}.json")
        
        logger.info("Training complete!")
        return self.population[0]  # Return best genome
    
    def save_checkpoint(self, filepath: str):
        """Save current state to file"""
        Path(filepath).parent.mkdir(parents=True, exist_ok=True)
        
        checkpoint = {
            "generation": self.generation,
            "population": [{"config": g.config, "fitness": g.fitness} for g in self.population],
            "metrics": self.metrics,
            "best_genome": {
                "config": self.population[0].config,
                "fitness": self.population[0].fitness
            }
        }
        
        with open(filepath, 'w') as f:
            json.dump(checkpoint, f, indent=2)
    
    def load_checkpoint(self, filepath: str):
        """Load state from file"""
        with open(filepath, 'r') as f:
            checkpoint = json.load(f)
        
        self.generation = checkpoint["generation"]
        self.metrics = checkpoint["metrics"]
        
        # Reconstruct population
        self.population = []
        for genome_data in checkpoint["population"]:
            genome = PipelineGenome(genome_data["config"])
            genome.fitness = genome_data["fitness"]
            self.population.append(genome)


async def main():
    """Run the Trinity Learning System"""
    
    # Initialize learning system
    system = TrinityLearningSystem(
        population_size=50,
        elite_size=10,
        mutation_rate=0.15,
        crossover_rate=0.7
    )
    
    # Run training
    best_genome = await system.run_training(num_generations=100)
    
    # Save best configuration
    with open("best_pipeline_config.json", "w") as f:
        json.dump(best_genome.config, f, indent=2)
    
    print(f"\nTraining complete! Best fitness: {best_genome.fitness:.3f}")
    print(f"Best configuration saved to best_pipeline_config.json")
    print(f"\nBest genome configuration:")
    print(json.dumps(best_genome.config, indent=2))

if __name__ == "__main__":
    asyncio.run(main())