#!/usr/bin/env python3
"""
Trinity Learning System - FORCED CLOUD AI VERSION
Ensures AI stays on during training - no fallback allowed
"""

import asyncio
import numpy as np
import json
import random
from pathlib import Path
from trinity_ground_truth_electronic import ElectronicGroundTruth
from trinity_safety_framework import SafetyMonitor
from engine_mapping_authoritative import *
import requests

class ForcedCloudLearning:
    """Learning system that REQUIRES cloud AI - no fallback"""
    
    def __init__(self):
        self.ground_truth = ElectronicGroundTruth()
        self.safety_monitor = SafetyMonitor()
        
        # Population settings for thorough learning
        self.population_size = 20  # Balance between thorough and speed
        self.elite_size = 5
        self.generation = 0
        self.best_fitness_history = []
        
        # FORCE CLOUD AI ON - This is the key!
        self.base_config = {
            "visionary": {
                "temperature": 0.8,
                "use_cloud": True,  # LOCKED ON
                "keyword_weight": 1.2,
                "creativity_bias": 0.7,
                "experimental_bonus": 0.3
            },
            "oracle": {
                "engine_weight": 15.0,
                "vibe_weight": 0.5,
                "search_k": 10,
                "similarity_threshold": 0.2,
                "prefer_experimental": True
            },
            "calculator": {
                "nudge_intensity": 0.6,
                "keyword_sensitivity": 1.5,
                "harmonic_balance": 0.4,
                "parameter_range": 0.4,
                "preserve_extremes": True,
                "genre_bias": "electronic"
            },
            "alchemist": {
                "validation_strictness": 0.3,
                "name_creativity": 0.9,
                "safety_threshold": 0.6,
                "max_effects": 5
            }
        }
        
        # Create initial population with variations
        self.population = []
        for _ in range(self.population_size):
            genome = self.create_genome_variant()
            self.population.append(genome)
        
        # Test prompts focused on engine selection
        self.test_prompts = [
            # Bass-focused (should select bass engines)
            "deep sub bass with analog warmth",
            "808 trap bass with saturation",
            "reese bass detuned wide",
            
            # Reverb-focused (should select reverb)
            "huge cathedral reverb space",
            "shimmer reverb ethereal",
            "spring reverb vintage",
            
            # Distortion-focused (should select distortion)
            "heavy metal distortion aggressive",
            "tube overdrive warm",
            "bit crusher digital destruction",
            
            # Clear multi-effect needs
            "compressed punchy drums with reverb",
            "filtered acid bass with delay",
            "distorted guitar with chorus"
        ]
    
    def create_genome_variant(self):
        """Create a variant but KEEP use_cloud=True"""
        config = json.loads(json.dumps(self.base_config))
        
        # Mutate some parameters but NEVER touch use_cloud
        if random.random() < 0.5:
            config["oracle"]["engine_weight"] *= random.uniform(0.8, 1.2)
        if random.random() < 0.5:
            config["calculator"]["keyword_sensitivity"] *= random.uniform(0.8, 1.2)
        if random.random() < 0.5:
            config["visionary"]["creativity_bias"] = random.uniform(0.5, 0.9)
            
        # FORCE use_cloud to True no matter what
        config["visionary"]["use_cloud"] = True
        
        return {"config": config, "fitness": 0.0}
    
    async def evaluate_genome(self, genome):
        """Evaluate with focus on engine selection accuracy"""
        scores = []
        
        # Test 5 random prompts per genome
        test_sample = random.sample(self.test_prompts, 5)
        
        for prompt in test_sample:
            try:
                response = requests.post(
                    "http://localhost:8000/generate",
                    json={"prompt": prompt, "config_override": genome["config"]},
                    timeout=10
                )
                
                if response.status_code == 200 and response.json()["success"]:
                    preset = response.json()["preset"]
                    
                    # Score based on engine selection relevance
                    score = self.score_engine_selection(prompt, preset)
                    scores.append(score)
                else:
                    scores.append(0.0)
            except:
                scores.append(0.0)
        
        genome["fitness"] = np.mean(scores) if scores else 0.0
        return genome["fitness"]
    
    def score_engine_selection(self, prompt, preset):
        """Score how well engines match the prompt"""
        prompt_lower = prompt.lower()
        params = preset.get("parameters", {})
        
        # Extract selected engines
        selected_engines = []
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                selected_engines.append(engine_id)
        
        if not selected_engines:
            return 0.0
        
        score = 0.0
        
        # Check for keyword matches
        if "bass" in prompt_lower:
            if any(e in [ENGINE_SUB_BASS, ENGINE_VINTAGE_TUBE, ENGINE_RODENT_DISTORTION] for e in selected_engines):
                score += 0.3
        
        if "reverb" in prompt_lower:
            if any(e in [ENGINE_PLATE_REVERB, ENGINE_SPRING_REVERB, ENGINE_SHIMMER_REVERB, 
                         ENGINE_GATED_REVERB, ENGINE_CONVOLUTION_REVERB] for e in selected_engines):
                score += 0.3
        
        if "distortion" in prompt_lower or "overdrive" in prompt_lower:
            if any(e in [ENGINE_RODENT_DISTORTION, ENGINE_VINTAGE_TUBE, ENGINE_MUFF_FUZZ,
                         ENGINE_KSTYLE_OVERDRIVE, ENGINE_BIT_CRUSHER] for e in selected_engines):
                score += 0.3
        
        if "delay" in prompt_lower or "echo" in prompt_lower:
            if any(e in [ENGINE_TAPE_ECHO, ENGINE_DIGITAL_DELAY, ENGINE_BUCKET_BRIGADE] for e in selected_engines):
                score += 0.3
        
        if "filter" in prompt_lower:
            if any(e in [ENGINE_LADDER_FILTER, ENGINE_COMB_FILTER, ENGINE_FORMANT_FILTER] for e in selected_engines):
                score += 0.3
        
        # Penalize too many engines
        if len(selected_engines) > 5:
            score *= 0.8
        elif len(selected_engines) <= 4:
            score += 0.1
        
        return min(score, 1.0)
    
    async def evolve_generation(self):
        """Evolve one generation"""
        print(f"Generation {self.generation}...", end="", flush=True)
        
        # Evaluate all genomes
        for genome in self.population:
            await self.evaluate_genome(genome)
        
        # Sort by fitness
        self.population.sort(key=lambda g: g["fitness"], reverse=True)
        best = self.population[0]
        
        avg_fitness = np.mean([g["fitness"] for g in self.population])
        print(f" Best: {best['fitness']:.1%}, Avg: {avg_fitness:.1%}")
        
        # Create next generation
        next_gen = []
        
        # Keep elite (but verify use_cloud=True)
        for elite in self.population[:self.elite_size]:
            elite["config"]["visionary"]["use_cloud"] = True  # ENFORCE
            next_gen.append(elite)
        
        # Generate offspring
        while len(next_gen) < self.population_size:
            parent1 = random.choice(self.population[:10])  # Select from top half
            parent2 = random.choice(self.population[:10])
            
            # Crossover
            child_config = {}
            for component in parent1["config"]:
                if random.random() < 0.5:
                    child_config[component] = json.loads(json.dumps(parent1["config"][component]))
                else:
                    child_config[component] = json.loads(json.dumps(parent2["config"][component]))
            
            # Mutation
            if random.random() < 0.3:
                if "oracle" in child_config:
                    child_config["oracle"]["engine_weight"] *= random.uniform(0.9, 1.1)
                if "calculator" in child_config:
                    child_config["calculator"]["keyword_sensitivity"] *= random.uniform(0.9, 1.1)
            
            # FORCE CLOUD AI ON
            child_config["visionary"]["use_cloud"] = True
            
            next_gen.append({"config": child_config, "fitness": 0.0})
        
        self.population = next_gen
        self.generation += 1
        
        # Save checkpoint every 10 generations
        if self.generation % 10 == 0:
            self.save_checkpoint()
        
        return best["fitness"]
    
    def save_checkpoint(self):
        """Save current best configuration"""
        best = self.population[0]
        
        # VERIFY cloud is on before saving
        best["config"]["visionary"]["use_cloud"] = True
        
        checkpoint = {
            "generation": self.generation,
            "best_fitness": best["fitness"],
            "best_config": best["config"],
            "population_size": len(self.population)
        }
        
        Path("checkpoints").mkdir(exist_ok=True)
        with open(f"checkpoints/forced_cloud_gen_{self.generation}.json", "w") as f:
            json.dump(checkpoint, f, indent=2)
        
        print(f"  💾 Saved checkpoint at generation {self.generation}")
    
    async def run(self, generations=50):
        """Run training for specified generations"""
        print("\n" + "="*70)
        print("FORCED CLOUD AI TRAINING - NO FALLBACK")
        print("="*70)
        print(f"Population: {self.population_size} genomes")
        print(f"Generations: {generations}")
        print(f"Cloud AI: LOCKED ON ✓")
        print("-"*70)
        
        for _ in range(generations):
            await self.evolve_generation()
        
        # Save final config
        best = self.population[0]
        best["config"]["visionary"]["use_cloud"] = True  # One more time to be sure!
        
        with open("best_forced_cloud_config.json", "w") as f:
            json.dump(best["config"], f, indent=2)
        
        print("\n" + "="*70)
        print(f"✅ Training Complete!")
        print(f"   Final fitness: {best['fitness']:.1%}")
        print(f"   Config saved to: best_forced_cloud_config.json")
        print(f"   Cloud AI: Still ON ✓")
        print("="*70)
        
        return best

async def main():
    system = ForcedCloudLearning()
    await system.run(generations=50)

if __name__ == "__main__":
    import time
    start = time.time()
    asyncio.run(main())
    print(f"\nCompleted in {(time.time()-start)/60:.1f} minutes")