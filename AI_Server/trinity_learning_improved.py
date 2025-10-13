#!/usr/bin/env python3
"""
Improved Trinity Learning with Semantic Scoring
Rewards creative but valid engine choices
"""

import asyncio
import numpy as np
import json
import random
from pathlib import Path
from trinity_ground_truth_electronic import ElectronicGroundTruth
from engine_mapping_authoritative import *
import requests

class ImprovedTrinityLearning:
    """Learning system with better semantic understanding"""
    
    def __init__(self):
        self.population_size = 20
        self.elite_size = 5
        self.generation = 0
        
        # Base config with cloud FORCED ON
        self.base_config = {
            "visionary": {
                "temperature": 0.8,
                "use_cloud": True,  # ALWAYS ON
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
        
        self.population = [self.create_variant() for _ in range(self.population_size)]
        
        # Semantic engine groups (not just literal matches)
        self.semantic_groups = {
            "bass_processing": [
                ENGINE_PARAMETRIC_EQ, ENGINE_DYNAMIC_EQ,  # EQ for bass shaping
                ENGINE_LADDER_FILTER, ENGINE_STATE_VARIABLE_FILTER,  # Classic bass filters
                ENGINE_VINTAGE_TUBE, ENGINE_HARMONIC_EXCITER,  # Bass enhancement
                ENGINE_MONO_MAKER,  # Centers sub bass
                ENGINE_MULTIBAND_COMPRESSOR, ENGINE_MULTIBAND_SATURATOR  # Multiband for bass
            ],
            "wobble_modulation": [
                ENGINE_LADDER_FILTER, ENGINE_FORMANT_FILTER, ENGINE_ENVELOPE_FILTER,
                ENGINE_AUTO_WAH, ENGINE_RING_MODULATOR, ENGINE_TREMOLO,
                ENGINE_VIBRATO, ENGINE_CHORUS, ENGINE_PHASER
            ],
            "distortion_character": [
                ENGINE_BIT_CRUSHER, ENGINE_RODENT_DISTORTION, ENGINE_VINTAGE_TUBE,
                ENGINE_MUFF_FUZZ, ENGINE_KSTYLE_OVERDRIVE, ENGINE_WAVE_FOLDER,
                ENGINE_TAPE_ECHO  # Tape saturation counts
            ],
            "space_reverb": [
                ENGINE_PLATE_REVERB, ENGINE_SPRING_REVERB, ENGINE_SHIMMER_REVERB,
                ENGINE_GATED_REVERB, ENGINE_CONVOLUTION_REVERB, ENGINE_TAPE_ECHO,
                ENGINE_DIGITAL_DELAY, ENGINE_BUCKET_BRIGADE
            ],
            "dynamics_punch": [
                ENGINE_VCA_COMPRESSOR, ENGINE_OPTO_COMPRESSOR, ENGINE_MULTIBAND_COMPRESSOR,
                ENGINE_TRANSIENT_SHAPER, ENGINE_MASTERING_LIMITER, ENGINE_NOISE_GATE
            ]
        }
        
        # Test prompts with semantic expectations
        self.test_cases = [
            {
                "prompt": "deep dubstep bass wobble",
                "needs": ["bass_processing", "wobble_modulation"],
                "optional": ["distortion_character", "space_reverb"]
            },
            {
                "prompt": "ethereal ambient pad shimmer",
                "needs": ["space_reverb"],
                "optional": ["wobble_modulation"]
            },
            {
                "prompt": "aggressive metal distortion",
                "needs": ["distortion_character"],
                "optional": ["dynamics_punch"]
            },
            {
                "prompt": "punchy compressed drums",
                "needs": ["dynamics_punch"],
                "optional": ["distortion_character"]
            },
            {
                "prompt": "acid bass 303 filter sweep",
                "needs": ["bass_processing", "wobble_modulation"],
                "optional": ["distortion_character"]
            }
        ]
    
    def create_variant(self):
        """Create config variant but KEEP cloud ON"""
        config = json.loads(json.dumps(self.base_config))
        
        # Mutate some values
        if random.random() < 0.5:
            config["oracle"]["engine_weight"] *= random.uniform(0.8, 1.2)
        if random.random() < 0.5:
            config["calculator"]["keyword_sensitivity"] *= random.uniform(0.8, 1.2)
            
        # FORCE cloud ON
        config["visionary"]["use_cloud"] = True
        
        return {"config": config, "fitness": 0.0}
    
    def semantic_score(self, prompt, preset):
        """Score based on semantic understanding, not literal matches"""
        params = preset.get("parameters", {})
        
        # Get selected engines
        selected = []
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                selected.append(engine_id)
        
        if not selected:
            return 0.0
        
        # Find matching test case
        test_case = None
        for tc in self.test_cases:
            if tc["prompt"] in prompt or prompt in tc["prompt"]:
                test_case = tc
                break
        
        if not test_case:
            # Generic scoring for unknown prompts
            return min(len(selected) / 4.0, 1.0) if len(selected) <= 5 else 0.5
        
        score = 0.0
        
        # Check required semantic groups
        for need in test_case["needs"]:
            group_engines = self.semantic_groups.get(need, [])
            if any(e in group_engines for e in selected):
                score += 0.4  # Major points for meeting needs
        
        # Check optional semantic groups
        for opt in test_case.get("optional", []):
            group_engines = self.semantic_groups.get(opt, [])
            if any(e in group_engines for e in selected):
                score += 0.2  # Bonus for optional enhancements
        
        # Penalties
        if len(selected) > 5:
            score *= 0.7  # Too many engines
        elif len(selected) < 2:
            score *= 0.5  # Too few engines
        
        # Bonus for good combinations
        if len(selected) >= 3 and len(selected) <= 4:
            score += 0.1  # Sweet spot
        
        return min(score, 1.0)
    
    async def evaluate_genome(self, genome):
        """Evaluate using semantic scoring"""
        scores = []
        
        # Test subset of prompts
        for test_case in random.sample(self.test_cases, 3):
            prompt = test_case["prompt"]
            
            try:
                response = requests.post(
                    "http://localhost:8000/generate",
                    json={"prompt": prompt, "config_override": genome["config"]},
                    timeout=10
                )
                
                if response.status_code == 200 and response.json()["success"]:
                    preset = response.json()["preset"]
                    score = self.semantic_score(prompt, preset)
                    scores.append(score)
                else:
                    scores.append(0.0)
            except:
                scores.append(0.0)
        
        genome["fitness"] = np.mean(scores) if scores else 0.0
        return genome["fitness"]
    
    async def evolve_generation(self):
        """Evolve with semantic understanding"""
        print(f"Gen {self.generation}...", end="", flush=True)
        
        # Evaluate
        for genome in self.population:
            await self.evaluate_genome(genome)
        
        # Sort and report
        self.population.sort(key=lambda g: g["fitness"], reverse=True)
        best = self.population[0]
        avg_fitness = np.mean([g["fitness"] for g in self.population])
        
        print(f" Best: {best['fitness']:.1%}, Avg: {avg_fitness:.1%}")
        
        # Save checkpoint every 10
        if self.generation % 10 == 0 and self.generation > 0:
            self.save_checkpoint()
        
        # Next generation
        next_gen = []
        
        # Elite (verified cloud ON)
        for elite in self.population[:self.elite_size]:
            elite["config"]["visionary"]["use_cloud"] = True
            next_gen.append(elite)
        
        # Breed rest
        while len(next_gen) < self.population_size:
            p1 = random.choice(self.population[:10])
            p2 = random.choice(self.population[:10])
            
            # Crossover
            child_config = {}
            for component in p1["config"]:
                if random.random() < 0.5:
                    child_config[component] = json.loads(json.dumps(p1["config"][component]))
                else:
                    child_config[component] = json.loads(json.dumps(p2["config"][component]))
            
            # Mutate
            if random.random() < 0.3:
                if "oracle" in child_config:
                    child_config["oracle"]["engine_weight"] = np.clip(
                        child_config["oracle"]["engine_weight"] + random.uniform(-2, 2),
                        5.0, 20.0
                    )
            
            # FORCE cloud ON
            child_config["visionary"]["use_cloud"] = True
            
            next_gen.append({"config": child_config, "fitness": 0.0})
        
        self.population = next_gen
        self.generation += 1
    
    def save_checkpoint(self):
        """Save progress"""
        best = self.population[0]
        best["config"]["visionary"]["use_cloud"] = True  # Verify
        
        Path("checkpoints").mkdir(exist_ok=True)
        with open(f"checkpoints/improved_gen_{self.generation}.json", "w") as f:
            json.dump({
                "generation": self.generation,
                "best_fitness": best["fitness"],
                "best_config": best["config"]
            }, f, indent=2)
        
        print(f"\n💾 Checkpoint saved (Gen {self.generation}, Fitness: {best['fitness']:.1%})")
    
    async def run(self, generations=50):
        """Run improved training"""
        print("\n" + "="*70)
        print("IMPROVED TRINITY LEARNING - SEMANTIC SCORING")
        print("="*70)
        print("Changes from previous:")
        print("  • Semantic scoring (creative choices rewarded)")
        print("  • Smaller test set (3 prompts vs 5)")
        print("  • Better mutation strategy")
        print("  • Cloud AI: LOCKED ON")
        print("-"*70)
        
        for _ in range(generations):
            await self.evolve_generation()
        
        # Save final
        best = self.population[0]
        best["config"]["visionary"]["use_cloud"] = True
        
        with open("best_semantic_config.json", "w") as f:
            json.dump(best["config"], f, indent=2)
        
        print("\n" + "="*70)
        print(f"✅ Complete! Final fitness: {best['fitness']:.1%}")
        print(f"   Saved to: best_semantic_config.json")
        print("="*70)

async def main():
    system = ImprovedTrinityLearning()
    await system.run(50)

if __name__ == "__main__":
    import time
    start = time.time()
    asyncio.run(main())
    print(f"Time: {(time.time()-start)/60:.1f} minutes")