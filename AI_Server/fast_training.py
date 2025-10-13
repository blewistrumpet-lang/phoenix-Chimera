#!/usr/bin/env python3
"""
Fast Trinity Training - Reduced test set for quicker results
Uses 10 genomes × 5 prompts = 50 calls per generation (vs 300)
Should complete 10 generations in ~30 minutes
"""

import asyncio
import numpy as np
import json
import random
from pathlib import Path
from trinity_ground_truth_electronic import ElectronicGroundTruth
from trinity_safety_framework import SafetyMonitor, LearningDirector
from trinity_learning_configured import ConfiguredPipelineGenome
import requests

class FastElectronicLearning:
    """Faster training with reduced population"""
    
    def __init__(self):
        self.ground_truth = ElectronicGroundTruth()
        self.safety_monitor = SafetyMonitor()
        
        # Smaller population for speed
        self.population_size = 10  # Reduced from 30
        self.elite_size = 3
        self.population = [ConfiguredPipelineGenome() for _ in range(self.population_size)]
        
        # Fewer test prompts
        self.test_prompts = [
            "deep sub bass with analog warmth",
            "glitchy percussion chopped",
            "ethereal ambient pad floating",
            "trap hi-hats crispy rolled",
            "dubstep wobble bass heavy",
            "techno kick punchy compressed",
            "experimental noise texture",
            "acid bass 303 squelchy",
            "shimmer reverb celestial",
            "bit crushed rhythmic destruction"
        ]
        
        self.generation = 0
        self.best_fitness_history = []
        self.baseline_performance = 0.736  # From our testing
    
    async def evaluate_genome(self, genome: ConfiguredPipelineGenome) -> float:
        """Fast evaluation with 5 prompts"""
        scores = []
        test_sample = random.sample(self.test_prompts, 5)  # Only 5 prompts
        
        for prompt in test_sample:
            try:
                response = requests.post(
                    "http://localhost:8000/generate",
                    json={"prompt": prompt, "config_override": genome.config},
                    timeout=5
                )
                
                if response.status_code == 200 and response.json()["success"]:
                    preset = response.json()["preset"]
                    validation = self.ground_truth.validate_preset(preset, prompt)
                    scores.append(validation["final_score"])
                else:
                    scores.append(0.0)
            except:
                scores.append(0.0)
        
        genome.fitness = np.mean(scores) if scores else 0.0
        return genome.fitness
    
    async def evolve_generation(self):
        """Fast evolution"""
        print(f"Generation {self.generation}...", end="", flush=True)
        
        # Evaluate all genomes
        for genome in self.population:
            await self.evaluate_genome(genome)
        
        # Sort by fitness
        self.population.sort(key=lambda g: g.fitness, reverse=True)
        best = self.population[0]
        
        avg_fitness = np.mean([g.fitness for g in self.population])
        improvement = (best.fitness / self.baseline_performance - 1) * 100
        
        print(f" Best: {best.fitness:.1%} (Avg: {avg_fitness:.1%}) [{improvement:+.1f}% vs baseline]")
        self.best_fitness_history.append(best.fitness)
        
        # Create next generation
        next_gen = []
        
        # Keep elite
        next_gen.extend(self.population[:self.elite_size])
        
        # Generate offspring
        while len(next_gen) < self.population_size:
            parent1 = self.tournament_select()
            parent2 = self.tournament_select()
            
            # Crossover
            child_config = {}
            for component in parent1.config:
                if random.random() < 0.5:
                    child_config[component] = json.loads(json.dumps(parent1.config[component]))
                else:
                    child_config[component] = json.loads(json.dumps(parent2.config[component]))
            
            child = ConfiguredPipelineGenome(child_config)
            child = child.mutate(0.2)  # Higher mutation for faster evolution
            next_gen.append(child)
        
        self.population = next_gen
        self.generation += 1
        
        # Save checkpoint
        if self.generation == 10:
            self.save_checkpoint(f"checkpoints/fast_gen_{self.generation}.json")
        
        return True
    
    def tournament_select(self, size: int = 3) -> ConfiguredPipelineGenome:
        """Tournament selection"""
        tournament = random.sample(self.population, min(size, len(self.population)))
        return max(tournament, key=lambda g: g.fitness)
    
    async def run_training(self, generations: int = 10):
        """Run fast training"""
        print(f"\n🚀 Fast Training: {self.population_size} genomes × 5 prompts × {generations} generations")
        print(f"   Baseline: {self.baseline_performance:.1%}")
        print(f"   Estimated time: {generations * 3} minutes\n")
        
        for gen in range(generations):
            await self.evolve_generation()
        
        # Save best configuration
        best = self.population[0]
        with open("fast_optimized_config.json", "w") as f:
            json.dump(best.config, f, indent=2)
        
        print(f"\n✅ Training complete!")
        print(f"   Final fitness: {best.fitness:.1%}")
        print(f"   Improvement: {(best.fitness/self.baseline_performance - 1)*100:+.1f}%")
        
        return best
    
    def save_checkpoint(self, filepath: str):
        """Save checkpoint"""
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

async def main():
    system = FastElectronicLearning()
    await system.run_training(generations=10)

if __name__ == "__main__":
    import time
    start = time.time()
    asyncio.run(main())
    print(f"\nCompleted in {(time.time()-start)/60:.1f} minutes")