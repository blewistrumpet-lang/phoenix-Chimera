"""
Golden Corpus Generator - Experimental Signal Chain Focus
Generates presets using the correct 57-engine architecture with 6 slots and 15 parameters
"""

import json
import random
import numpy as np
from typing import Dict, List, Any, Tuple
import os
from datetime import datetime

class ExperimentalCorpusGenerator:
    """
    Generates experimental audio presets for Chimera Phoenix
    Focuses on unconventional signal chains and creative routing
    """
    
    def __init__(self):
        # Load engine definitions
        self.engines = self._load_engine_definitions()
        
        # Experimental chain templates based on research
        self.experimental_chains = self._define_experimental_chains()
        
        # Parameter patterns for different effects
        self.parameter_patterns = self._define_parameter_patterns()
        
        # Categories with experimental focus
        self.categories = {
            "Experimental": 0.35,  # 35% of corpus
            "Ambient": 0.15,
            "Glitch": 0.15,
            "Lo-Fi": 0.10,
            "Spatial": 0.10,
            "Harmonic": 0.05,
            "Rhythmic": 0.05,
            "Textural": 0.05
        }
    
    def _load_engine_definitions(self) -> Dict[int, Dict]:
        """Load engine definitions with correct IDs"""
        return {
            # Dynamics (0-13)
            0: {"name": "None", "type": "bypass"},
            1: {"name": "VintageOptoCompressor", "type": "dynamics"},
            2: {"name": "ClassicCompressor", "type": "dynamics"},
            3: {"name": "MasteringLimiter", "type": "dynamics"},
            4: {"name": "NoiseGate", "type": "dynamics"},
            5: {"name": "TransientShaper", "type": "dynamics"},
            6: {"name": "MultibandCompressor", "type": "dynamics"},
            7: {"name": "VintageVCA", "type": "dynamics"},
            8: {"name": "ParallelCompressor", "type": "dynamics"},
            9: {"name": "TubeLimiter", "type": "dynamics"},
            10: {"name": "Expander", "type": "dynamics"},
            11: {"name": "EnvelopeFollower", "type": "dynamics"},
            12: {"name": "DynamicEQ", "type": "dynamics"},
            13: {"name": "GlueCompressor", "type": "dynamics"},
            
            # Distortion (14-22)
            14: {"name": "TubeSaturation", "type": "distortion"},
            15: {"name": "VintageTubePreamp", "type": "distortion"},
            16: {"name": "AnalogTapeEmulation", "type": "distortion"},
            17: {"name": "KStyleOverdrive", "type": "distortion"},
            18: {"name": "BitCrusher", "type": "distortion"},
            19: {"name": "HarmonicExciter", "type": "distortion"},
            20: {"name": "WaveFolder", "type": "distortion"},
            21: {"name": "FuzzPedal", "type": "distortion"},
            22: {"name": "VintageDistortion", "type": "distortion"},
            
            # Modulation (23-27)
            23: {"name": "ClassicChorus", "type": "modulation"},
            24: {"name": "AnalogPhaser", "type": "modulation"},
            25: {"name": "VintageFlanger", "type": "modulation"},
            26: {"name": "ClassicTremolo", "type": "modulation"},
            27: {"name": "FrequencyShifter", "type": "modulation"},
            
            # Filter (28-33)
            28: {"name": "ParametricEQ", "type": "filter"},
            29: {"name": "GraphicEQ", "type": "filter"},
            30: {"name": "VintageEQ", "type": "filter"},
            31: {"name": "LadderFilter", "type": "filter"},
            32: {"name": "VocalFormantFilter", "type": "filter"},
            33: {"name": "EnvelopeFilter", "type": "filter"},
            
            # Pitch (34-39)
            34: {"name": "SimplePitchShift", "type": "pitch"},
            35: {"name": "IntelligentHarmonizer", "type": "pitch"},
            36: {"name": "FormantShifter", "type": "pitch"},
            37: {"name": "RingModulator", "type": "pitch"},
            38: {"name": "PitchCorrection", "type": "pitch"},
            39: {"name": "Vocoder", "type": "pitch"},
            
            # Time-Based (40-46)
            40: {"name": "DigitalDelay", "type": "delay"},
            41: {"name": "TapeDelay", "type": "delay"},
            42: {"name": "PlateReverb", "type": "reverb"},
            43: {"name": "SpringReverb", "type": "reverb"},
            44: {"name": "ShimmerReverb", "type": "reverb"},
            45: {"name": "GatedReverb", "type": "reverb"},
            46: {"name": "ConvolutionReverb", "type": "reverb"},
            
            # Utility (47-51)
            47: {"name": "StereoImager", "type": "utility"},
            48: {"name": "AutoPanner", "type": "utility"},
            49: {"name": "Gain", "type": "utility"},
            50: {"name": "PhaseRotator", "type": "utility"},
            51: {"name": "MidSideProcessor", "type": "utility"},
            
            # Special/Experimental (52-56)
            52: {"name": "DimensionExpander", "type": "spatial"},
            53: {"name": "BucketBrigadeDelay", "type": "special"},
            54: {"name": "SpectralFreeze", "type": "special"},
            55: {"name": "GranularCloud", "type": "special"},
            56: {"name": "ChaosGenerator", "type": "special"}
        }
    
    def _define_experimental_chains(self) -> List[Dict]:
        """Define experimental signal chain templates"""
        return [
            # Chaotic Spectral Morphing
            {
                "name": "Chaotic Spectral Morphing",
                "slots": [56, 54, 44, 0, 0, 0],  # Chaos → Spectral Freeze → Shimmer
                "description": "Constantly evolving spectral content with chaotic modulation"
            },
            
            # Granular Feedback Madness
            {
                "name": "Granular Feedback Madness",
                "slots": [55, 53, 27, 45, 0, 0],  # Granular → Bucket → FreqShift → Gated
                "description": "Dense granular texture with metallic feedback"
            },
            
            # Spectral Freeze Stutter
            {
                "name": "Spectral Freeze Stutter",
                "slots": [54, 56, 52, 0, 0, 0],  # Spectral → Chaos → Dimension
                "description": "Frozen spectral content with chaotic amplitude"
            },
            
            # Harmonic Chaos Spiral
            {
                "name": "Harmonic Chaos Spiral",
                "slots": [35, 56, 27, 45, 0, 0],  # Harmonizer → Chaos → FreqShift → Gated
                "description": "Harmonized chaos with unstable pitch"
            },
            
            # Vocal Formant Destruction
            {
                "name": "Vocal Formant Destruction",
                "slots": [32, 37, 54, 53, 0, 0],  # VocalFormant → Ring → Spectral → Bucket
                "description": "Vocal formants destroyed and reconstructed"
            },
            
            # Lo-Fi Degradation Chain
            {
                "name": "Lo-Fi Degradation",
                "slots": [18, 56, 41, 20, 43, 0],  # BitCrush → Chaos → Tape → WaveFold → Spring
                "description": "Heavy degradation with vintage character"
            },
            
            # Ambient Drift
            {
                "name": "Ambient Drift",
                "slots": [54, 44, 56, 51, 52, 0],  # Spectral → Shimmer → Chaos → M/S → Dimension
                "description": "Slowly evolving ambient textures"
            },
            
            # Glitch Matrix
            {
                "name": "Glitch Matrix",
                "slots": [55, 18, 27, 53, 0, 0],  # Granular → BitCrush → FreqShift → Bucket
                "description": "Micro-stutters with digital artifacts"
            },
            
            # Resonant Modulation
            {
                "name": "Resonant Modulation",
                "slots": [31, 56, 37, 43, 0, 0],  # Ladder → Chaos → Ring → Spring
                "description": "Resonant filtering with chaotic modulation"
            },
            
            # Pitch Spiral
            {
                "name": "Pitch Spiral",
                "slots": [34, 55, 53, 19, 0, 0],  # PitchShift → Granular → Bucket → Exciter
                "description": "Ascending pitch spirals with granular texture"
            },
            
            # Formant Feedback Web
            {
                "name": "Formant Web",
                "slots": [32, 39, 45, 0, 0, 0],  # VocalFormant → Vocoder → Gated
                "description": "Formant-shaped feedback network"
            },
            
            # Chaos Field
            {
                "name": "Chaos Field",
                "slots": [56, 52, 37, 47, 0, 0],  # Chaos → Dimension → Ring → Stereo
                "description": "Chaotic stereo movement"
            },
            
            # Spectral Resonance
            {
                "name": "Spectral Resonance",
                "slots": [54, 31, 27, 52, 0, 0],  # Spectral → Ladder → FreqShift → Dimension
                "description": "Resonant spectral sweeps"
            },
            
            # Rhythmic Chaos
            {
                "name": "Rhythmic Chaos",
                "slots": [56, 5, 45, 0, 0, 0],  # Chaos → Transient → Gated
                "description": "Chaotic rhythmic patterns"
            },
            
            # Harmonic Web
            {
                "name": "Harmonic Web",
                "slots": [55, 35, 53, 52, 0, 0],  # Granular → Harmonizer → Bucket → Dimension
                "description": "Complex harmonic textures"
            }
        ]
    
    def _define_parameter_patterns(self) -> Dict[str, List[float]]:
        """Define parameter patterns for different effect types"""
        return {
            # Chaos patterns
            "chaos_slow": [0.1, 0.8, 0.6, 0.4, 0.5, 0.3, 0.7, 0.2, 0.9, 0.4, 0.6, 0.5, 0.3, 0.8, 0.6],
            "chaos_fast": [0.8, 0.4, 0.9, 0.3, 0.7, 0.5, 0.2, 0.8, 0.4, 0.6, 0.9, 0.3, 0.7, 0.5, 0.8],
            
            # Spectral patterns
            "spectral_freeze": [0.9, 0.4, 0.6, 0.8, 0.3, 0.7, 0.5, 0.2, 0.8, 0.6, 0.4, 0.7, 0.3, 0.9, 0.7],
            "spectral_morph": [0.5, 0.8, 0.3, 0.6, 0.9, 0.2, 0.7, 0.4, 0.5, 0.8, 0.3, 0.6, 0.9, 0.4, 0.6],
            
            # Granular patterns  
            "granular_dense": [0.05, 0.9, 0.8, 0.6, 0.7, 0.4, 0.8, 0.3, 0.9, 0.5, 0.7, 0.6, 0.4, 0.8, 0.8],
            "granular_sparse": [0.3, 0.4, 0.2, 0.8, 0.3, 0.6, 0.4, 0.7, 0.2, 0.5, 0.3, 0.8, 0.4, 0.6, 0.5],
            
            # Reverb patterns
            "reverb_huge": [0.95, 0.9, 0.2, 0.8, 0.6, 0.7, 0.4, 0.9, 0.3, 0.8, 0.5, 0.7, 0.6, 0.9, 0.8],
            "reverb_gated": [0.4, 0.8, 0.9, 0.1, 0.3, 0.8, 0.6, 0.2, 0.9, 0.4, 0.7, 0.3, 0.8, 0.5, 0.6],
            
            # Distortion patterns
            "distortion_warm": [0.6, 0.7, 0.4, 0.8, 0.5, 0.6, 0.3, 0.7, 0.4, 0.6, 0.5, 0.8, 0.3, 0.6, 0.7],
            "distortion_harsh": [0.9, 0.3, 0.8, 0.2, 0.9, 0.4, 0.7, 0.1, 0.9, 0.3, 0.8, 0.2, 0.9, 0.4, 0.8],
            
            # Filter patterns
            "filter_sweep": [0.2, 0.9, 0.8, 0.3, 0.6, 0.4, 0.8, 0.2, 0.7, 0.5, 0.9, 0.3, 0.6, 0.4, 0.7],
            "filter_resonant": [0.5, 0.95, 0.3, 0.8, 0.4, 0.7, 0.2, 0.9, 0.5, 0.6, 0.3, 0.8, 0.4, 0.7, 0.6]
        }
    
    def generate_experimental_preset(self, chain_template: Dict, variation: float = 0.3) -> Dict:
        """Generate a preset based on experimental chain template"""
        preset = {
            "creative_name": self._generate_creative_name(chain_template["name"]),
            "category": self._select_category(),
            "description": chain_template["description"],
            "tags": ["experimental", "creative", "unconventional"],
            "complexity": random.uniform(0.6, 1.0),
            "brightness": random.uniform(0.3, 0.8),
            "warmth": random.uniform(0.2, 0.7)
        }
        
        # Generate parameters for each slot
        for slot_idx in range(6):
            slot_num = slot_idx + 1
            engine_id = chain_template["slots"][slot_idx]
            
            # Set engine
            preset[f"slot{slot_num}_engine"] = engine_id
            preset[f"slot{slot_num}_bypass"] = 0.0 if engine_id > 0 else 1.0
            preset[f"slot{slot_num}_solo"] = 0.0
            
            if engine_id > 0:
                # Get appropriate parameter pattern
                pattern = self._get_pattern_for_engine(engine_id)
                params = self._apply_variation(pattern, variation)
                
                # Set parameters
                for param_idx in range(15):
                    param_num = param_idx + 1
                    if param_num == 15:
                        # Mix parameter - experimental presets use creative mixing
                        preset[f"slot{slot_num}_param15"] = random.uniform(0.4, 0.9)
                    else:
                        preset[f"slot{slot_num}_param{param_num}"] = params[param_idx]
            else:
                # Bypassed slot - all zeros
                for param_idx in range(15):
                    preset[f"slot{slot_num}_param{param_idx + 1}"] = 0.0
        
        return preset
    
    def _get_pattern_for_engine(self, engine_id: int) -> List[float]:
        """Get parameter pattern based on engine type"""
        engine_info = self.engines.get(engine_id, {"type": "unknown"})
        engine_type = engine_info["type"]
        
        patterns_map = {
            "special": ["chaos_slow", "chaos_fast"],
            "reverb": ["reverb_huge", "reverb_gated"],
            "distortion": ["distortion_warm", "distortion_harsh"],
            "filter": ["filter_sweep", "filter_resonant"],
            "pitch": ["spectral_freeze", "spectral_morph"],
            "delay": ["granular_dense", "granular_sparse"]
        }
        
        pattern_names = patterns_map.get(engine_type, ["chaos_slow"])
        pattern_name = random.choice(pattern_names)
        return self.parameter_patterns.get(pattern_name, [0.5] * 14)
    
    def _apply_variation(self, pattern: List[float], variation: float) -> List[float]:
        """Apply random variation to parameter pattern"""
        varied = []
        for value in pattern:
            # Apply variation within bounds
            delta = random.uniform(-variation, variation)
            new_value = max(0.0, min(1.0, value + delta))
            varied.append(new_value)
        return varied
    
    def _generate_creative_name(self, base_name: str) -> str:
        """Generate creative preset name"""
        prefixes = ["Quantum", "Fractal", "Ethereal", "Morphing", "Crystalline", 
                   "Nebula", "Astral", "Chromatic", "Prismatic", "Kaleidoscope"]
        suffixes = ["Dreams", "Cascade", "Vortex", "Horizon", "Particles",
                   "Waves", "Echo", "Flux", "Storm", "Meditation"]
        
        if random.random() > 0.5:
            return f"{random.choice(prefixes)} {base_name}"
        else:
            return f"{base_name} {random.choice(suffixes)}"
    
    def _select_category(self) -> str:
        """Select category with experimental bias"""
        rand = random.random()
        cumulative = 0.0
        for category, probability in self.categories.items():
            cumulative += probability
            if rand < cumulative:
                return category
        return "Experimental"
    
    def generate_category_based_presets(self, count: int = 50) -> List[Dict]:
        """Method 1: Category-based generation with experimental focus"""
        presets = []
        
        for i in range(count):
            # Select experimental chain
            chain = random.choice(self.experimental_chains)
            
            # Generate with variation
            variation = random.uniform(0.2, 0.4)
            preset = self.generate_experimental_preset(chain, variation)
            
            # Add unique ID
            preset["id"] = f"GC_{i+1:03d}_EXP"
            
            presets.append(preset)
        
        return presets
    
    def generate_engineering_focused_presets(self, count: int = 30) -> List[Dict]:
        """Method 2: Engineering-focused systematic generation"""
        presets = []
        
        # Systematically explore each experimental engine
        experimental_engines = [54, 55, 56, 52, 53]  # Special engines
        
        for i in range(count):
            # Build chain around experimental engine
            main_engine = experimental_engines[i % len(experimental_engines)]
            
            # Create supporting chain
            chain = {
                "name": f"Engineering Test {i+1}",
                "slots": [main_engine, 0, 0, 0, 0, 0],
                "description": f"Systematic exploration of {self.engines[main_engine]['name']}"
            }
            
            # Add complementary engines
            if main_engine == 56:  # Chaos Generator
                chain["slots"][1] = random.choice([54, 37, 27])  # Spectral/Ring/FreqShift
            elif main_engine == 55:  # Granular Cloud
                chain["slots"][1] = random.choice([53, 35, 19])  # Bucket/Harmonizer/Exciter
            elif main_engine == 54:  # Spectral Freeze
                chain["slots"][1] = random.choice([56, 55, 44])  # Chaos/Granular/Shimmer
            
            # Add processing
            chain["slots"][2] = random.choice([42, 43, 44, 45, 46])  # Reverb
            chain["slots"][3] = random.choice([47, 51, 52])  # Spatial
            
            preset = self.generate_experimental_preset(chain, variation=0.25)
            preset["id"] = f"GC_{i+1:03d}_ENG"
            preset["tags"].append("systematic")
            
            presets.append(preset)
        
        return presets
    
    def generate_musical_context_presets(self, count: int = 40) -> List[Dict]:
        """Method 5: Musical context-driven generation"""
        contexts = {
            "Film Score": ["ambient", "tension", "atmospheric"],
            "Electronic": ["glitch", "idm", "experimental"],
            "Ambient": ["drone", "meditation", "soundscape"],
            "Industrial": ["harsh", "metallic", "aggressive"],
            "Psychedelic": ["trippy", "morphing", "colorful"]
        }
        
        presets = []
        
        for i in range(count):
            context_name = list(contexts.keys())[i % len(contexts)]
            context_tags = contexts[context_name]
            
            # Select appropriate chain for context
            if "ambient" in context_tags or "drone" in context_tags:
                chain = random.choice([c for c in self.experimental_chains 
                                     if "Ambient" in c["name"] or "Drift" in c["name"]])
            elif "glitch" in context_tags:
                chain = random.choice([c for c in self.experimental_chains 
                                     if "Glitch" in c["name"] or "Stutter" in c["name"]])
            else:
                chain = random.choice(self.experimental_chains)
            
            preset = self.generate_experimental_preset(chain, variation=0.3)
            preset["id"] = f"GC_{i+1:03d}_MUS"
            preset["context"] = context_name
            preset["tags"].extend(context_tags)
            
            presets.append(preset)
        
        return presets
    
    def generate_cross_pollination_presets(self, count: int = 30) -> List[Dict]:
        """Method 6: Cross-pollination of different techniques"""
        presets = []
        
        for i in range(count):
            # Combine two different experimental chains
            chain1 = random.choice(self.experimental_chains)
            chain2 = random.choice(self.experimental_chains)
            
            # Create hybrid chain
            hybrid_chain = {
                "name": f"{chain1['name']} + {chain2['name']}",
                "slots": [
                    chain1["slots"][0],
                    chain2["slots"][0] if chain2["slots"][0] != chain1["slots"][0] else chain2["slots"][1],
                    chain1["slots"][1] if chain1["slots"][1] > 0 else chain2["slots"][1],
                    chain2["slots"][2] if chain2["slots"][2] > 0 else chain1["slots"][2],
                    random.choice([47, 51, 52]),  # Add spatial processing
                    0
                ],
                "description": f"Hybrid: {chain1['description']} meets {chain2['description']}"
            }
            
            preset = self.generate_experimental_preset(hybrid_chain, variation=0.35)
            preset["id"] = f"GC_{i+1:03d}_HYB"
            preset["tags"].append("hybrid")
            
            presets.append(preset)
        
        return presets
    
    def generate_complete_corpus(self) -> Dict[str, Any]:
        """Generate complete corpus using all methods"""
        print("Generating Experimental Golden Corpus...")
        
        all_presets = []
        
        # Method 1: Category-based (50 presets)
        print("Generating category-based experimental presets...")
        all_presets.extend(self.generate_category_based_presets(50))
        
        # Method 2: Engineering-focused (30 presets)
        print("Generating engineering-focused systematic presets...")
        all_presets.extend(self.generate_engineering_focused_presets(30))
        
        # Method 5: Musical context (40 presets)
        print("Generating musical context-driven presets...")
        all_presets.extend(self.generate_musical_context_presets(40))
        
        # Method 6: Cross-pollination (30 presets)
        print("Generating cross-pollination hybrid presets...")
        all_presets.extend(self.generate_cross_pollination_presets(30))
        
        # Renumber all presets
        for idx, preset in enumerate(all_presets):
            preset["id"] = f"GC_{idx+1:04d}"
            preset["index"] = idx
        
        # Create corpus metadata
        metadata = {
            "version": "3.0",
            "engine_count": 57,
            "total_presets": len(all_presets),
            "generation_date": datetime.now().isoformat(),
            "focus": "experimental",
            "methods_used": [
                "Category-based generation",
                "Engineering-focused systematic",
                "Musical context-driven",
                "Cross-pollination hybrid"
            ],
            "categories": dict(sorted(
                {preset["category"]: sum(1 for p in all_presets if p["category"] == preset["category"]) 
                 for preset in all_presets}.items()
            ))
        }
        
        return {
            "metadata": metadata,
            "presets": all_presets
        }
    
    def save_corpus(self, corpus: Dict[str, Any], output_dir: str):
        """Save corpus to files"""
        os.makedirs(output_dir, exist_ok=True)
        os.makedirs(os.path.join(output_dir, "presets"), exist_ok=True)
        
        # Save metadata
        with open(os.path.join(output_dir, "corpus_metadata.json"), "w") as f:
            json.dump(corpus["metadata"], f, indent=2)
        
        # Save each preset
        for preset in corpus["presets"]:
            preset_file = os.path.join(output_dir, "presets", f"{preset['id']}.json")
            with open(preset_file, "w") as f:
                json.dump(preset, f, indent=2)
        
        print(f"Saved {len(corpus['presets'])} presets to {output_dir}")
    
    def validate_preset(self, preset: Dict) -> bool:
        """Validate preset structure"""
        required_fields = ["creative_name", "category"]
        
        # Check required fields
        for field in required_fields:
            if field not in preset:
                return False
        
        # Check all slots
        for slot in range(1, 7):
            # Check engine
            engine_key = f"slot{slot}_engine"
            if engine_key not in preset:
                return False
            if not 0 <= preset[engine_key] <= 56:
                return False
            
            # Check all 15 parameters
            for param in range(1, 16):
                param_key = f"slot{slot}_param{param}"
                if param_key not in preset:
                    return False
                if not 0.0 <= preset[param_key] <= 1.0:
                    return False
        
        return True


# Main execution
if __name__ == "__main__":
    generator = ExperimentalCorpusGenerator()
    
    # Generate complete corpus
    corpus = generator.generate_complete_corpus()
    
    # Validate all presets
    valid_count = sum(1 for preset in corpus["presets"] if generator.validate_preset(preset))
    print(f"Validated {valid_count}/{len(corpus['presets'])} presets")
    
    # Save corpus
    output_directory = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3"
    generator.save_corpus(corpus, output_directory)
    
    print("\nCorpus generation complete!")
    print(f"Total presets: {corpus['metadata']['total_presets']}")
    print(f"Categories: {corpus['metadata']['categories']}")