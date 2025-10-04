#!/usr/bin/env python3
"""
FINAL Trinity Learning System - Properly Designed
- Uses only real engines that exist
- Semantic understanding of musical intent
- Gradual scoring with multiple factors
- Proper genetic algorithm parameters
"""

import asyncio
import numpy as np
import json
import random
from pathlib import Path
from engine_mapping_authoritative import *
import requests
from typing import List, Dict, Any, Tuple

class ProperTrinityTraining:
    """Correctly implemented training system"""
    
    def __init__(self):
        # Genetic algorithm parameters (tuned properly)
        self.population_size = 30  # Larger for diversity
        self.elite_size = 6  # 20% elite preservation
        self.mutation_rate = 0.15  # Moderate mutation
        self.crossover_rate = 0.7
        self.tournament_size = 5
        
        # Training tracking
        self.generation = 0
        self.best_fitness_history = []
        self.diversity_history = []
        
        # VERIFIED REAL ENGINES - grouped by actual musical function
        self.engine_groups = {
            # Bass shaping (EQ/Filters that can boost/shape low frequencies)
            "bass_shaping": [
                ENGINE_PARAMETRIC_EQ,  # 7 - Can boost bass frequencies
                ENGINE_DYNAMIC_EQ,  # 6 - Dynamic bass enhancement
                ENGINE_VINTAGE_CONSOLE_EQ,  # 8 - Vintage bass character
                ENGINE_LADDER_FILTER,  # 9 - Classic bass filter
                ENGINE_STATE_VARIABLE_FILTER,  # 10 - Versatile filtering
            ],
            
            # Bass enhancement (Adds harmonics/warmth)
            "bass_enhancement": [
                ENGINE_VINTAGE_TUBE,  # 15 - Tube warmth
                ENGINE_HARMONIC_EXCITER,  # 17 - Harmonic enhancement
                ENGINE_MULTIBAND_SATURATOR,  # 19 - Frequency-specific saturation
                ENGINE_TAPE_ECHO,  # 34 - Tape saturation
            ],
            
            # Bass utility (Mono/phase coherence)
            "bass_utility": [
                ENGINE_MONO_MAKER,  # 55 - Centers sub frequencies
                ENGINE_PHASE_ALIGN,  # 56 - Phase coherence
            ],
            
            # Modulation (For wobbles/movement)
            "modulation": [
                ENGINE_LADDER_FILTER,  # 9 - Filter sweeps
                ENGINE_FORMANT_FILTER,  # 11 - Vowel-like modulation
                ENGINE_ENVELOPE_FILTER,  # 12 - Envelope following
                ENGINE_COMB_RESONATOR,  # 13 - Resonant effects
                ENGINE_RING_MODULATOR,  # 26 - Ring mod
                ENGINE_FREQUENCY_SHIFTER,  # 27 - Frequency shifting
                ENGINE_HARMONIC_TREMOLO,  # 28 - Harmonic tremolo
                ENGINE_CLASSIC_TREMOLO,  # 29 - Classic tremolo
                ENGINE_ANALOG_PHASER,  # 25 - Phasing
                ENGINE_DIGITAL_CHORUS,  # 23 - Chorus
                ENGINE_RESONANT_CHORUS,  # 24 - Resonant chorus
            ],
            
            # Distortion/Character
            "distortion": [
                ENGINE_VINTAGE_TUBE,  # 15
                ENGINE_WAVE_FOLDER,  # 16
                ENGINE_BIT_CRUSHER,  # 18
                ENGINE_MUFF_FUZZ,  # 20
                ENGINE_RODENT_DISTORTION,  # 21
                ENGINE_K_STYLE,  # 22 - Note: K_STYLE not KSTYLE
            ],
            
            # Dynamics
            "dynamics": [
                ENGINE_OPTO_COMPRESSOR,  # 1
                ENGINE_VCA_COMPRESSOR,  # 2
                ENGINE_TRANSIENT_SHAPER,  # 3
                ENGINE_NOISE_GATE,  # 4
                ENGINE_MASTERING_LIMITER,  # 5
                ENGINE_DYNAMIC_EQ,  # 6
            ],
            
            # Space/Reverb
            "reverb": [
                ENGINE_PLATE_REVERB,  # 39
                ENGINE_SPRING_REVERB,  # 40
                ENGINE_CONVOLUTION_REVERB,  # 41
                ENGINE_SHIMMER_REVERB,  # 42
                ENGINE_GATED_REVERB,  # 43
            ],
            
            # Delay/Echo
            "delay": [
                ENGINE_TAPE_ECHO,  # 34
                ENGINE_DIGITAL_DELAY,  # 35
                ENGINE_MAGNETIC_DRUM_ECHO,  # 36
                ENGINE_BUCKET_BRIGADE_DELAY,  # 37 - Note: BUCKET_BRIGADE_DELAY
                ENGINE_BUFFER_REPEAT,  # 38
            ],
            
            # Pitch
            "pitch": [
                ENGINE_PITCH_SHIFTER,  # 31
                ENGINE_DETUNE_DOUBLER,  # 32
                ENGINE_INTELLIGENT_HARMONIZER,  # 33
            ],
            
            # Spatial
            "spatial": [
                ENGINE_STEREO_WIDENER,  # 44
                ENGINE_STEREO_IMAGER,  # 45
                ENGINE_DIMENSION_EXPANDER,  # 46
                ENGINE_MID_SIDE_PROCESSOR,  # 53
            ],
            
            # Experimental
            "experimental": [
                ENGINE_SPECTRAL_FREEZE,  # 47
                ENGINE_SPECTRAL_GATE,  # 48
                ENGINE_PHASED_VOCODER,  # 49
                ENGINE_GRANULAR_CLOUD,  # 50
                ENGINE_CHAOS_GENERATOR,  # 51
                ENGINE_FEEDBACK_NETWORK,  # 52
            ]
        }
        
        # Test suite with semantic understanding
        self.test_suite = [
            # Bass-focused
            {
                "prompt": "deep sub bass with warmth",
                "required_groups": ["bass_shaping"],
                "beneficial_groups": ["bass_enhancement", "bass_utility"],
                "avoid_groups": ["reverb"],  # Too much reverb muddies bass
                "max_engines": 4
            },
            {
                "prompt": "808 trap bass with distortion",
                "required_groups": ["bass_shaping", "distortion"],
                "beneficial_groups": ["dynamics", "bass_utility"],
                "avoid_groups": [],
                "max_engines": 4
            },
            {
                "prompt": "dubstep bass wobble",
                "required_groups": ["bass_shaping", "modulation"],
                "beneficial_groups": ["distortion", "bass_utility"],
                "avoid_groups": [],
                "max_engines": 5
            },
            
            # Reverb/Space
            {
                "prompt": "huge cathedral reverb",
                "required_groups": ["reverb"],
                "beneficial_groups": ["delay", "spatial"],
                "avoid_groups": ["distortion"],
                "max_engines": 3
            },
            {
                "prompt": "ethereal shimmer pad",
                "required_groups": ["reverb"],
                "beneficial_groups": ["modulation", "spatial"],
                "avoid_groups": ["distortion", "dynamics"],
                "max_engines": 4
            },
            
            # Distortion
            {
                "prompt": "aggressive metal guitar",
                "required_groups": ["distortion"],
                "beneficial_groups": ["dynamics", "bass_shaping"],
                "avoid_groups": [],
                "max_engines": 5
            },
            {
                "prompt": "warm tube saturation",
                "required_groups": ["distortion"],
                "beneficial_groups": ["bass_enhancement"],
                "avoid_groups": ["experimental"],
                "max_engines": 3
            },
            
            # Modulation
            {
                "prompt": "psychedelic phaser sweep",
                "required_groups": ["modulation"],
                "beneficial_groups": ["delay", "reverb"],
                "avoid_groups": [],
                "max_engines": 4
            },
            
            # Dynamics
            {
                "prompt": "punchy drum compression",
                "required_groups": ["dynamics"],
                "beneficial_groups": ["distortion"],
                "avoid_groups": ["reverb", "delay"],
                "max_engines": 3
            },
            
            # Experimental
            {
                "prompt": "glitchy granular textures",
                "required_groups": ["experimental"],
                "beneficial_groups": ["modulation", "delay"],
                "avoid_groups": [],
                "max_engines": 5
            },
            
            # Multi-effect chains
            {
                "prompt": "vintage vocal chain",
                "required_groups": ["dynamics"],
                "beneficial_groups": ["bass_shaping", "reverb", "delay"],
                "avoid_groups": ["experimental"],
                "max_engines": 5
            },
            {
                "prompt": "lofi hip hop character",
                "required_groups": ["distortion"],
                "beneficial_groups": ["modulation", "bass_shaping"],
                "avoid_groups": [],
                "max_engines": 4
            }
        ]
        
        # Initialize population with base config
        self.base_config = {
            "visionary": {
                "temperature": 0.8,
                "use_cloud": True,  # ALWAYS TRUE
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
        
        self.population = [self.create_individual() for _ in range(self.population_size)]
    
    def create_individual(self) -> Dict[str, Any]:
        """Create an individual with variation"""
        config = json.loads(json.dumps(self.base_config))
        
        # Apply random variations (but keep use_cloud=True)
        if random.random() < 0.3:
            config["oracle"]["engine_weight"] *= random.uniform(0.8, 1.3)
        if random.random() < 0.3:
            config["calculator"]["keyword_sensitivity"] *= random.uniform(0.8, 1.3)
        if random.random() < 0.3:
            config["visionary"]["creativity_bias"] = random.uniform(0.5, 0.9)
        
        # ENFORCE cloud AI
        config["visionary"]["use_cloud"] = True
        
        return {
            "config": config,
            "fitness": 0.0,
            "age": 0,
            "id": random.randint(1000, 9999)
        }
    
    def score_engine_selection(self, test_case: Dict, preset: Dict) -> float:
        """Score engine selection with musical understanding"""
        params = preset.get("parameters", {})
        
        # Extract active engines
        active_engines = []
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                active_engines.append(engine_id)
        
        if not active_engines:
            return 0.0
        
        score = 0.0
        max_score = 0.0
        
        # Check required groups (40% weight)
        for req_group in test_case["required_groups"]:
            max_score += 0.4
            group_engines = self.engine_groups.get(req_group, [])
            if any(e in group_engines for e in active_engines):
                score += 0.4
        
        # Check beneficial groups (20% weight each)
        for ben_group in test_case.get("beneficial_groups", []):
            max_score += 0.2
            group_engines = self.engine_groups.get(ben_group, [])
            if any(e in group_engines for e in active_engines):
                score += 0.2
        
        # Penalize avoided groups
        for avoid_group in test_case.get("avoid_groups", []):
            group_engines = self.engine_groups.get(avoid_group, [])
            if any(e in group_engines for e in active_engines):
                score *= 0.8  # 20% penalty
        
        # Engine count scoring (prefer 3-4 engines)
        engine_count = len(active_engines)
        max_allowed = test_case.get("max_engines", 5)
        
        if engine_count > max_allowed:
            score *= 0.7  # Too many
        elif engine_count == 0:
            score = 0.0
        elif 3 <= engine_count <= 4:
            score *= 1.1  # Sweet spot bonus
        elif engine_count < 2:
            score *= 0.8  # Too few
        
        # Normalize score
        if max_score > 0:
            normalized = score / max_score
        else:
            normalized = score
        
        return min(normalized, 1.0)
    
    async def evaluate_individual(self, individual: Dict) -> float:
        """Evaluate an individual's fitness"""
        scores = []
        
        # Test on random subset (6 prompts for balance)
        test_sample = random.sample(self.test_suite, min(6, len(self.test_suite)))
        
        for test_case in test_sample:
            try:
                response = requests.post(
                    "http://localhost:8000/generate",
                    json={
                        "prompt": test_case["prompt"],
                        "config_override": individual["config"]
                    },
                    timeout=10
                )
                
                if response.status_code == 200:
                    data = response.json()
                    if data["success"]:
                        preset = data["preset"]
                        score = self.score_engine_selection(test_case, preset)
                        scores.append(score)
                    else:
                        scores.append(0.0)
                else:
                    scores.append(0.0)
            except Exception as e:
                scores.append(0.0)
        
        individual["fitness"] = np.mean(scores) if scores else 0.0
        individual["age"] += 1
        
        return individual["fitness"]
    
    def mutate(self, individual: Dict, rate: float = None) -> Dict:
        """Mutate an individual"""
        if rate is None:
            rate = self.mutation_rate
        
        mutated = json.loads(json.dumps(individual))
        config = mutated["config"]
        
        # Mutate oracle parameters
        if random.random() < rate:
            param = random.choice(["engine_weight", "vibe_weight", "similarity_threshold"])
            if param == "engine_weight":
                config["oracle"][param] = np.clip(
                    config["oracle"][param] + random.uniform(-3, 3),
                    5.0, 25.0
                )
            elif param == "vibe_weight":
                config["oracle"][param] = np.clip(
                    config["oracle"][param] + random.uniform(-0.3, 0.3),
                    0.1, 2.0
                )
            else:
                config["oracle"][param] = np.clip(
                    config["oracle"][param] + random.uniform(-0.1, 0.1),
                    0.1, 0.5
                )
        
        # Mutate calculator parameters
        if random.random() < rate:
            param = random.choice(["nudge_intensity", "keyword_sensitivity"])
            config["calculator"][param] = np.clip(
                config["calculator"][param] + random.uniform(-0.2, 0.2),
                0.1, 2.0
            )
        
        # Mutate visionary (but NOT use_cloud)
        if random.random() < rate:
            config["visionary"]["creativity_bias"] = np.clip(
                config["visionary"]["creativity_bias"] + random.uniform(-0.1, 0.1),
                0.3, 0.9
            )
        
        # ALWAYS ENFORCE CLOUD
        config["visionary"]["use_cloud"] = True
        
        mutated["id"] = random.randint(1000, 9999)  # New ID
        mutated["age"] = 0  # Reset age
        
        return mutated
    
    def crossover(self, parent1: Dict, parent2: Dict) -> Tuple[Dict, Dict]:
        """Create offspring via crossover"""
        child1_config = {}
        child2_config = {}
        
        # Component-level crossover
        for component in parent1["config"]:
            if random.random() < 0.5:
                child1_config[component] = json.loads(json.dumps(parent1["config"][component]))
                child2_config[component] = json.loads(json.dumps(parent2["config"][component]))
            else:
                child1_config[component] = json.loads(json.dumps(parent2["config"][component]))
                child2_config[component] = json.loads(json.dumps(parent1["config"][component]))
        
        # ENFORCE CLOUD
        child1_config["visionary"]["use_cloud"] = True
        child2_config["visionary"]["use_cloud"] = True
        
        child1 = {
            "config": child1_config,
            "fitness": 0.0,
            "age": 0,
            "id": random.randint(1000, 9999)
        }
        
        child2 = {
            "config": child2_config,
            "fitness": 0.0,
            "age": 0,
            "id": random.randint(1000, 9999)
        }
        
        return child1, child2
    
    def tournament_selection(self) -> Dict:
        """Select individual via tournament"""
        tournament = random.sample(self.population, min(self.tournament_size, len(self.population)))
        return max(tournament, key=lambda x: x["fitness"])
    
    async def evolve_generation(self):
        """Evolve one generation"""
        print(f"\nGeneration {self.generation}:")
        print("-" * 50)
        
        # Evaluate population
        for i, individual in enumerate(self.population):
            await self.evaluate_individual(individual)
            if (i + 1) % 10 == 0:
                print(f"  Evaluated {i + 1}/{len(self.population)}...")
        
        # Sort by fitness
        self.population.sort(key=lambda x: x["fitness"], reverse=True)
        
        # Statistics
        best = self.population[0]
        avg_fitness = np.mean([ind["fitness"] for ind in self.population])
        diversity = np.std([ind["fitness"] for ind in self.population])
        
        self.best_fitness_history.append(best["fitness"])
        self.diversity_history.append(diversity)
        
        print(f"  Best: {best['fitness']:.3f} (ID: {best['id']})")
        print(f"  Average: {avg_fitness:.3f}")
        print(f"  Diversity: {diversity:.3f}")
        
        # Create next generation
        next_gen = []
        
        # Elite preservation
        for elite in self.population[:self.elite_size]:
            next_gen.append(json.loads(json.dumps(elite)))
        
        # Fill rest with offspring
        while len(next_gen) < self.population_size:
            if random.random() < self.crossover_rate:
                # Crossover
                parent1 = self.tournament_selection()
                parent2 = self.tournament_selection()
                child1, child2 = self.crossover(parent1, parent2)
                
                # Mutate offspring
                if random.random() < self.mutation_rate:
                    child1 = self.mutate(child1)
                if random.random() < self.mutation_rate:
                    child2 = self.mutate(child2)
                
                next_gen.append(child1)
                if len(next_gen) < self.population_size:
                    next_gen.append(child2)
            else:
                # Direct mutation of tournament winner
                parent = self.tournament_selection()
                child = self.mutate(parent, rate=0.3)  # Higher mutation
                next_gen.append(child)
        
        self.population = next_gen[:self.population_size]
        self.generation += 1
        
        # Save checkpoint every 10 generations
        if self.generation % 10 == 0:
            self.save_checkpoint()
    
    def save_checkpoint(self):
        """Save training checkpoint"""
        Path("checkpoints").mkdir(exist_ok=True)
        
        best = self.population[0]
        best["config"]["visionary"]["use_cloud"] = True  # Verify
        
        checkpoint = {
            "generation": self.generation,
            "best_fitness": best["fitness"],
            "best_config": best["config"],
            "fitness_history": self.best_fitness_history,
            "diversity_history": self.diversity_history,
            "population_size": self.population_size
        }
        
        filename = f"checkpoints/trinity_final_gen_{self.generation}.json"
        with open(filename, 'w') as f:
            json.dump(checkpoint, f, indent=2)
        
        print(f"\n💾 Checkpoint saved: {filename}")
    
    async def run(self, generations: int = 50):
        """Run training"""
        print("\n" + "="*80)
        print("TRINITY LEARNING SYSTEM - FINAL VERSION")
        print("="*80)
        print(f"Population: {self.population_size} individuals")
        print(f"Generations: {generations}")
        print(f"Test cases: {len(self.test_suite)} scenarios")
        print(f"Engine groups: {len(self.engine_groups)} categories")
        print("Cloud AI: LOCKED ON ✓")
        print("="*80)
        
        for gen in range(generations):
            await self.evolve_generation()
            
            # Early stopping if converged
            if len(self.best_fitness_history) > 10:
                recent = self.best_fitness_history[-10:]
                if max(recent) - min(recent) < 0.01:
                    print("\n⚠️ Converged - stopping early")
                    break
        
        # Save final configuration
        best = self.population[0]
        best["config"]["visionary"]["use_cloud"] = True
        
        with open("trinity_final_config.json", 'w') as f:
            json.dump(best["config"], f, indent=2)
        
        print("\n" + "="*80)
        print("TRAINING COMPLETE")
        print("="*80)
        print(f"Final best fitness: {best['fitness']:.3f}")
        print(f"Generations run: {self.generation}")
        print(f"Config saved to: trinity_final_config.json")
        print("="*80)

async def main():
    system = ProperTrinityTraining()
    await system.run(50)

if __name__ == "__main__":
    import time
    start = time.time()
    asyncio.run(main())
    elapsed = time.time() - start
    print(f"\nTotal time: {elapsed/60:.1f} minutes")