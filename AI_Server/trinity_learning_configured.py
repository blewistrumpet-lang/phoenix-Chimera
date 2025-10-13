"""
Trinity Learning System - Configured for Electronic/Experimental Music
Priorities: Engine Selection (40%) > Signal Flow (25%) > Mix (15%) > Parameters (10%) + Creativity (10%)
Target: Sound designers & musicians, max 5 effects, experimental encouraged
"""

import numpy as np
import json
import asyncio
import requests
import random
from typing import Dict, Any, List, Tuple
from dataclasses import dataclass
from pathlib import Path
import logging
from datetime import datetime
from trinity_ground_truth_electronic import ElectronicGroundTruth
from trinity_safety_framework import SafetyMonitor, LearningDirector
from engine_mapping_authoritative import *

logger = logging.getLogger(__name__)

class ConfiguredPipelineGenome:
    """Pipeline configuration optimized for electronic music production"""
    
    def __init__(self, config: Dict[str, Any] = None):
        if config is None:
            config = self.electronic_optimized_config()
        
        self.config = config
        self.fitness = 0.0
        self.evaluations = []
        self.generation = 0
    
    @staticmethod
    def electronic_optimized_config() -> Dict[str, Any]:
        """Starting configuration optimized for electronic/experimental music"""
        return {
            "visionary": {
                "temperature": 0.8,  # Higher for creativity
                "use_cloud": True,
                "keyword_weight": 1.2,  # Prioritize engine keywords
                "creativity_bias": 0.7,  # Lean creative
                "experimental_bonus": 0.3  # Encourage rare combinations
            },
            
            "oracle": {
                "engine_weight": 15.0,  # Very high - engine selection is top priority
                "vibe_weight": 0.5,     # Lower - vibe less important
                "search_k": 10,         # Search more candidates
                "similarity_threshold": 0.2,  # More lenient
                "prefer_experimental": True
            },
            
            "calculator": {
                "nudge_intensity": 0.6,  # Moderate nudging
                "keyword_sensitivity": 1.5,  # High sensitivity to prompt
                "harmonic_balance": 0.4,  # Less important for electronic
                "parameter_range": 0.4,  # Wider parameter exploration
                "preserve_extremes": True,  # Keep extreme settings
                "genre_bias": "electronic"
            },
            
            "alchemist": {
                "validation_strictness": 0.3,  # Lenient - allow experimental
                "name_creativity": 0.9,  # Very creative names
                "safety_threshold": 0.6,  # Lower threshold for experimental
                "max_effects": 5  # Hard limit
            }
        }
    
    def mutate(self, mutation_rate: float = 0.15) -> 'ConfiguredPipelineGenome':
        """Mutate with electronic music considerations"""
        mutated = json.loads(json.dumps(self.config))
        
        # Target specific parameters for electronic optimization
        electronic_critical_params = [
            ("oracle", "engine_weight"),  # Most critical
            ("visionary", "experimental_bonus"),
            ("calculator", "keyword_sensitivity"),
            ("alchemist", "max_effects")
        ]
        
        for component, param in electronic_critical_params:
            if random.random() < mutation_rate * 1.5:  # Higher chance for critical params
                current = mutated[component][param]
                if isinstance(current, float):
                    # Larger mutations for exploration
                    mutated[component][param] = np.clip(
                        current + np.random.normal(0, 0.15), 0.0, 20.0
                    )
        
        # Standard mutation for other params
        for component, params in mutated.items():
            for param, value in params.items():
                if random.random() < mutation_rate:
                    if isinstance(value, float):
                        params[param] = np.clip(value + np.random.normal(0, 0.1), 0.0, 1.0)
                    elif isinstance(value, bool):
                        params[param] = not value if random.random() < 0.3 else value
        
        child = ConfiguredPipelineGenome(mutated)
        child.generation = self.generation + 1
        return child


class ElectronicLearningSystem:
    """Learning system configured for electronic/experimental music"""
    
    def __init__(self):
        self.ground_truth = ElectronicGroundTruth()
        self.safety_monitor = SafetyMonitor()
        self.learning_director = LearningDirector()
        
        # Population
        self.population_size = 30  # Smaller for faster iteration
        self.elite_size = 5
        self.population = [ConfiguredPipelineGenome() for _ in range(self.population_size)]
        
        # Test prompts focused on electronic genres
        self.test_prompts = self._load_electronic_prompts()
        
        # Tracking
        self.generation = 0
        self.best_fitness_history = []
        self.baseline_performance = None
    
    def _load_electronic_prompts(self) -> List[str]:
        """Load test prompts for electronic music"""
        return [
            # Bass design
            "deep sub bass with analog warmth",
            "reese bass detuned and wide",
            "808 bass with saturation",
            "neurofunk bass metallic growl",
            "acid bass 303 squelchy",
            
            # Glitch/IDM
            "glitchy percussion chopped",
            "idm textures complex evolving",
            "bit crushed rhythmic destruction",
            "granular glitch atmosphere",
            
            # Ambient/Atmospheric
            "ethereal ambient pad floating",
            "dark ambient drone ominous",
            "shimmer reverb celestial",
            "evolving soundscape organic",
            
            # Trap/Hip-Hop
            "trap hi-hats crispy rolled",
            "phonk bass distorted heavy",
            "chopped vocal formant shifted",
            "boom bap drums vintage",
            
            # Dubstep/DnB
            "dubstep wobble bass heavy",
            "drum and bass breaks chopped",
            "jungle amen break processed",
            "future garage atmosphere",
            
            # Techno/House
            "techno kick punchy compressed",
            "acid house 303 filter sweep",
            "deep house chord stab",
            "minimal techno percussion",
            
            # Experimental
            "experimental noise texture",
            "feedback network chaos",
            "spectral freeze glitch",
            "ring modulated metallic"
        ]
    
    async def evaluate_genome(self, genome: ConfiguredPipelineGenome) -> float:
        """Evaluate genome using electronic music criteria"""
        
        scores = []
        test_sample = random.sample(self.test_prompts, min(10, len(self.test_prompts)))
        
        for prompt in test_sample:
            try:
                # Generate preset with this configuration
                response = requests.post(
                    "http://localhost:8000/generate",
                    json={"prompt": prompt, "config_override": genome.config},
                    timeout=5
                )
                
                if response.status_code == 200 and response.json()["success"]:
                    preset = response.json()["preset"]
                    
                    # Validate against electronic ground truth
                    validation = self.ground_truth.validate_preset(preset, prompt)
                    scores.append(validation["final_score"])
                else:
                    scores.append(0.0)
            except:
                scores.append(0.0)
        
        genome.fitness = np.mean(scores) if scores else 0.0
        return genome.fitness
    
    async def establish_baseline(self) -> float:
        """Establish baseline with current configuration"""
        baseline_scores = []
        
        for prompt in random.sample(self.test_prompts, 5):
            try:
                response = requests.post(
                    "http://localhost:8000/generate",
                    json={"prompt": prompt},
                    timeout=5
                )
                
                if response.status_code == 200 and response.json()["success"]:
                    preset = response.json()["preset"]
                    validation = self.ground_truth.validate_preset(preset, prompt)
                    baseline_scores.append(validation["final_score"])
            except:
                baseline_scores.append(0.0)
        
        self.baseline_performance = np.mean(baseline_scores) if baseline_scores else 0.5
        self.safety_monitor.set_baseline(self.baseline_performance)
        
        logger.info(f"Baseline established: {self.baseline_performance:.3f}")
        return self.baseline_performance
    
    async def evolve_generation(self):
        """Evolve one generation with safety checks"""
        
        # Evaluate all genomes
        for genome in self.population:
            await self.evaluate_genome(genome)
        
        # Sort by fitness
        self.population.sort(key=lambda g: g.fitness, reverse=True)
        best = self.population[0]
        
        # Safety check
        if self.baseline_performance:
            safe, message = self.learning_director.can_proceed_safely(best.fitness)
            if not safe:
                logger.warning(f"Safety check failed: {message}")
                # Rollback to baseline
                return False
        
        # Log progress
        avg_fitness = np.mean([g.fitness for g in self.population])
        logger.info(f"Gen {self.generation}: Best={best.fitness:.3f}, Avg={avg_fitness:.3f}")
        self.best_fitness_history.append(best.fitness)
        
        # Create next generation
        next_gen = []
        
        # Keep elite
        next_gen.extend(self.population[:self.elite_size])
        
        # Generate offspring
        while len(next_gen) < self.population_size:
            parent1 = self.tournament_select()
            parent2 = self.tournament_select()
            
            # Crossover at component level
            child_config = {}
            for component in parent1.config:
                if random.random() < 0.5:
                    child_config[component] = json.loads(json.dumps(parent1.config[component]))
                else:
                    child_config[component] = json.loads(json.dumps(parent2.config[component]))
            
            child = ConfiguredPipelineGenome(child_config)
            child = child.mutate(0.15)
            next_gen.append(child)
        
        self.population = next_gen
        self.generation += 1
        
        return True
    
    def tournament_select(self, size: int = 3) -> ConfiguredPipelineGenome:
        """Tournament selection"""
        tournament = random.sample(self.population, min(size, len(self.population)))
        return max(tournament, key=lambda g: g.fitness)
    
    async def run_training(self, generations: int = 50):
        """Run training with checkpoints"""
        
        # Establish baseline
        await self.establish_baseline()
        print(f"Starting baseline: {self.baseline_performance:.3f}")
        
        # Training loop
        for gen in range(generations):
            success = await self.evolve_generation()
            
            if not success:
                print("Training stopped due to safety check")
                break
            
            # Checkpoint every 10 generations
            if gen % 10 == 0:
                self.save_checkpoint(f"checkpoints/electronic_gen_{gen}.json")
                
                # Report improvement
                if self.best_fitness_history:
                    improvement = (self.best_fitness_history[-1] / self.baseline_performance - 1) * 100
                    print(f"Generation {gen}: {improvement:+.1f}% improvement over baseline")
        
        # Save final configuration
        best = self.population[0]
        with open("best_electronic_config.json", "w") as f:
            json.dump(best.config, f, indent=2)
        
        print(f"\nTraining complete!")
        print(f"Final fitness: {best.fitness:.3f}")
        print(f"Improvement: {(best.fitness/self.baseline_performance - 1)*100:+.1f}%")
        
        return best
    
    def save_checkpoint(self, filepath: str):
        """Save training state"""
        Path(filepath).parent.mkdir(parents=True, exist_ok=True)
        
        checkpoint = {
            "generation": self.generation,
            "baseline": self.baseline_performance,
            "best_config": self.population[0].config,
            "best_fitness": self.population[0].fitness,
            "fitness_history": self.best_fitness_history
        }
        
        with open(filepath, 'w') as f:
            json.dump(checkpoint, f, indent=2)


def demonstrate_system():
    """Demonstrate the configured learning system"""
    
    print("\n" + "="*80)
    print("TRINITY LEARNING SYSTEM - ELECTRONIC/EXPERIMENTAL CONFIGURATION")
    print("="*80)
    
    print("\n📊 CONFIGURATION:")
    print("  • Target Genres: Electronic, Hip-Hop, Ambient, Experimental")
    print("  • Priority: Engine Selection (40%) > Signal Flow (25%) > Mix (15%)")
    print("  • Max Effects: 5 (leave room for manual addition)")
    print("  • Approach: Creative/experimental encouraged")
    
    print("\n🎯 GROUND TRUTH:")
    ground_truth = ElectronicGroundTruth()
    print(f"  • {len(ground_truth.ground_truth_presets)} validated presets")
    print(f"  • Genres covered: Bass Design, Glitch, Ambient, Trap, Dubstep, Techno, IDM")
    
    print("\n🧬 EVOLUTION SETTINGS:")
    print("  • Population: 30 genomes")
    print("  • Elite preservation: 5 best")
    print("  • Mutation rate: 15%")
    print("  • Evaluation: 10 prompts per genome")
    
    print("\n🛡️ SAFETY MECHANISMS:")
    print("  • Baseline performance tracking")
    print("  • Regression detection (80% threshold)")
    print("  • Emergency stop (50% threshold)")
    print("  • Checkpoint saves every 10 generations")
    
    print("\n✅ READY TO START LEARNING")
    print("="*80)
    
    return True


# Run demonstration
if __name__ == "__main__":
    import random
    
    if demonstrate_system():
        print("\n🚀 To start training, run:")
        print("   system = ElectronicLearningSystem()")
        print("   await system.run_training(generations=50)")
        
        # Quick validation test
        print("\n📝 Quick Validation Test:")
        gt = ElectronicGroundTruth()
        
        test_preset = {
            "parameters": {
                "slot1_engine": ENGINE_LADDER_FILTER,
                "slot1_bypass": 0.0,
                "slot1_mix": 0.9,
                "slot2_engine": ENGINE_BIT_CRUSHER,
                "slot2_bypass": 0.0,
                "slot2_mix": 0.6,
                "slot3_engine": ENGINE_TAPE_ECHO,
                "slot3_bypass": 0.0,
                "slot3_mix": 0.4
            }
        }
        
        result = gt.validate_preset(test_preset, "lofi glitch with filter sweeps")
        print(f"   Test Score: {result['final_score']:.2%}")
        print(f"   Engine Selection: {result['scores']['engine_selection']:.2%}")
        print(f"   Signal Flow: {result['scores']['signal_flow']:.2%}")