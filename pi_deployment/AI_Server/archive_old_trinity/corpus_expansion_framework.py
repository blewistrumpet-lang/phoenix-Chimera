#!/usr/bin/env python3
"""
Safe Corpus Expansion Framework - Quality over Quantity
"""

import json
import random
import hashlib
from typing import Dict, Any, List, Tuple, Optional
from pathlib import Path
from engine_mapping_authoritative import ENGINE_NAMES

class PresetValidator:
    """Validates presets before adding to corpus"""
    
    def __init__(self):
        self.min_quality_score = 0.7
        self.existing_presets = []
        self.load_existing_corpus()
    
    def load_existing_corpus(self):
        """Load existing corpus for duplicate checking"""
        corpus_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json")
        if corpus_path.exists():
            with open(corpus_path, 'r') as f:
                self.existing_presets = json.load(f)
    
    def validate_preset(self, preset: Dict[str, Any]) -> Tuple[bool, List[str], float]:
        """
        Comprehensive preset validation
        Returns: (is_valid, errors, quality_score)
        """
        errors = []
        scores = []
        
        # 1. Structure validation
        structure_valid, structure_errors = self._validate_structure(preset)
        if not structure_valid:
            errors.extend(structure_errors)
            return False, errors, 0.0
        
        # 2. Parameter validation
        param_valid, param_errors, param_score = self._validate_parameters(preset)
        errors.extend(param_errors)
        scores.append(param_score)
        
        # 3. Engine validation
        engine_valid, engine_errors, engine_score = self._validate_engines(preset)
        errors.extend(engine_errors)
        scores.append(engine_score)
        
        # 4. Signal chain validation
        chain_valid, chain_errors, chain_score = self._validate_signal_chain(preset)
        errors.extend(chain_errors)
        scores.append(chain_score)
        
        # 5. Naming validation
        name_valid, name_errors, name_score = self._validate_naming(preset)
        errors.extend(name_errors)
        scores.append(name_score)
        
        # 6. Uniqueness validation
        unique, unique_errors, unique_score = self._validate_uniqueness(preset)
        errors.extend(unique_errors)
        scores.append(unique_score)
        
        # 7. Musical coherence
        coherent, coherence_errors, coherence_score = self._validate_musical_coherence(preset)
        errors.extend(coherence_errors)
        scores.append(coherence_score)
        
        # Calculate overall quality score
        quality_score = sum(scores) / len(scores) if scores else 0.0
        
        # Determine if valid
        is_valid = (
            param_valid and 
            engine_valid and 
            chain_valid and 
            name_valid and 
            unique and 
            coherent and
            quality_score >= self.min_quality_score
        )
        
        return is_valid, errors, quality_score
    
    def _validate_structure(self, preset: Dict) -> Tuple[bool, List[str]]:
        """Validate basic structure"""
        errors = []
        required_fields = ["name", "creative_name"]
        
        for field in required_fields:
            if field not in preset:
                errors.append(f"Missing required field: {field}")
        
        # Check for at least one engine
        has_engine = False
        for slot in range(1, 7):
            if preset.get(f"slot{slot}_engine", 0) > 0:
                has_engine = True
                break
        
        if not has_engine:
            errors.append("Preset must have at least one engine")
        
        return len(errors) == 0, errors
    
    def _validate_parameters(self, preset: Dict) -> Tuple[bool, List[str], float]:
        """Validate parameter ranges and diversity"""
        errors = []
        param_values = []
        
        for slot in range(1, 7):
            for param in range(10):
                key = f"slot{slot}_param{param}"
                if key in preset:
                    value = preset[key]
                    
                    # Check range
                    if not isinstance(value, (int, float)):
                        errors.append(f"{key} must be numeric")
                    elif value < 0 or value > 1:
                        errors.append(f"{key} out of range: {value}")
                    else:
                        param_values.append(value)
        
        # Calculate diversity score
        if param_values:
            # Check if all parameters are the same (bad)
            if len(set(param_values)) == 1:
                errors.append("All parameters have the same value")
                diversity_score = 0.0
            else:
                # Calculate variance
                mean = sum(param_values) / len(param_values)
                variance = sum((x - mean) ** 2 for x in param_values) / len(param_values)
                # Score based on variance (0.08 is good variance)
                diversity_score = min(1.0, variance / 0.08)
        else:
            diversity_score = 0.5
        
        return len(errors) == 0, errors, diversity_score
    
    def _validate_engines(self, preset: Dict) -> Tuple[bool, List[str], float]:
        """Validate engine selection"""
        errors = []
        engines = []
        
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                if engine_id < 1 or engine_id > 56:
                    errors.append(f"Invalid engine ID in slot {slot}: {engine_id}")
                else:
                    engines.append(engine_id)
        
        # Check engine count (prefer 3-5)
        if len(engines) == 0:
            errors.append("No engines defined")
            score = 0.0
        elif len(engines) <= 2:
            score = 0.7
        elif len(engines) <= 5:
            score = 1.0
        else:
            score = 0.8  # Slightly penalize too many engines
        
        # Check for duplicate engines (usually bad)
        if len(engines) != len(set(engines)):
            errors.append("Duplicate engines detected")
            score *= 0.5
        
        # Bonus for using underutilized engines
        underused_engines = [53, 54, 55, 56]  # Utility engines
        rare_engines = [47, 48, 49, 50, 51, 52]  # Special effects
        
        for engine in engines:
            if engine in rare_engines:
                score = min(1.0, score + 0.1)
        
        return len(errors) == 0, errors, score
    
    def _validate_signal_chain(self, preset: Dict) -> Tuple[bool, List[str], float]:
        """Validate signal chain ordering"""
        errors = []
        
        # Get engines in order
        chain = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                chain.append((slot, engine_id))
        
        if not chain:
            return True, [], 1.0
        
        # Check for logical ordering issues
        score = 1.0
        
        # Reverb should typically be at the end
        reverb_ids = list(range(39, 44))  # All reverb engines
        for i, (slot, engine_id) in enumerate(chain[:-1]):
            if engine_id in reverb_ids:
                # Check if non-reverb comes after
                for j in range(i + 1, len(chain)):
                    if chain[j][1] not in reverb_ids:
                        errors.append("Reverb should typically be at the end of chain")
                        score *= 0.8
                        break
        
        # Dynamics should be early
        dynamics_ids = list(range(1, 6))  # Compressors, gates, limiters
        for i, (slot, engine_id) in enumerate(chain):
            if engine_id in dynamics_ids and i > 3:
                errors.append("Dynamics processors should typically be early in chain")
                score *= 0.9
        
        return len(errors) == 0, errors, score
    
    def _validate_naming(self, preset: Dict) -> Tuple[bool, List[str], float]:
        """Validate preset naming"""
        errors = []
        
        name = preset.get("name", "")
        creative_name = preset.get("creative_name", "")
        
        # Check for bad naming patterns
        bad_patterns = [
            "Creative Mix",
            "Sonic",
            "Unknown",
            "Default",
            "Test",
            "Preset"
        ]
        
        score = 1.0
        for pattern in bad_patterns:
            if pattern.lower() in name.lower():
                errors.append(f"Name contains generic pattern: {pattern}")
                score *= 0.5
            if pattern.lower() in creative_name.lower():
                errors.append(f"Creative name contains generic pattern: {pattern}")
                score *= 0.5
        
        # Check name length
        if len(name) < 3:
            errors.append("Name too short")
            score *= 0.7
        elif len(name) > 50:
            errors.append("Name too long")
            score *= 0.9
        
        # Bonus for descriptive names
        good_words = ["warm", "bright", "dark", "vintage", "modern", "aggressive", 
                     "smooth", "crispy", "thick", "wide", "narrow"]
        for word in good_words:
            if word in name.lower() or word in creative_name.lower():
                score = min(1.0, score + 0.05)
        
        return len(errors) == 0, errors, score
    
    def _validate_uniqueness(self, preset: Dict) -> Tuple[bool, List[str], float]:
        """Check for duplicates"""
        errors = []
        
        # Create preset fingerprint
        fingerprint = self._create_fingerprint(preset)
        
        # Check against existing
        for existing in self.existing_presets:
            existing_fingerprint = self._create_fingerprint(existing)
            if fingerprint == existing_fingerprint:
                errors.append(f"Duplicate of existing preset: {existing.get('creative_name', 'Unknown')}")
                return False, errors, 0.0
        
        # Check name uniqueness
        preset_name = preset.get("creative_name", "").lower()
        for existing in self.existing_presets:
            if existing.get("creative_name", "").lower() == preset_name:
                errors.append(f"Duplicate name: {preset_name}")
                return False, errors, 0.0
        
        return True, errors, 1.0
    
    def _validate_musical_coherence(self, preset: Dict) -> Tuple[bool, List[str], float]:
        """Check if engines make musical sense together"""
        errors = []
        engines = []
        
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                engines.append(engine_id)
        
        score = 1.0
        
        # Check for conflicting combinations
        # Multiple reverbs usually not needed
        reverb_ids = list(range(39, 44))
        reverb_count = sum(1 for e in engines if e in reverb_ids)
        if reverb_count > 2:
            errors.append(f"Too many reverbs ({reverb_count})")
            score *= 0.7
        
        # Multiple compressors can be okay but flag if excessive
        comp_ids = list(range(1, 5))
        comp_count = sum(1 for e in engines if e in comp_ids)
        if comp_count > 3:
            errors.append(f"Too many compressors ({comp_count})")
            score *= 0.8
        
        # Check for good combinations
        good_combos = [
            ([15, 39]),  # Tube + Plate Reverb
            ([2, 7]),    # Compressor + EQ
            ([18, 34]),  # Bit Crusher + Tape Echo
            ([24, 39]),  # Chorus + Reverb
        ]
        
        for combo in good_combos:
            if all(e in engines for e in combo):
                score = min(1.0, score + 0.1)
        
        return len(errors) == 0, errors, score
    
    def _create_fingerprint(self, preset: Dict) -> str:
        """Create unique fingerprint for duplicate detection"""
        # Use engines and key parameters
        data = []
        for slot in range(1, 7):
            engine = preset.get(f"slot{slot}_engine", 0)
            if engine > 0:
                data.append(f"e{slot}:{engine}")
                for param in range(3):  # Just first 3 params
                    value = preset.get(f"slot{slot}_param{param}", 0)
                    data.append(f"p{slot}{param}:{value:.2f}")
        
        return hashlib.md5("|".join(data).encode()).hexdigest()


class PresetGenerator:
    """Generate high-quality presets for corpus expansion"""
    
    def __init__(self):
        self.validator = PresetValidator()
    
    def generate_genre_preset(self, genre: str, style: str = "default") -> Dict[str, Any]:
        """Generate a genre-specific preset"""
        
        genre_templates = {
            "metal": {
                "engines": [4, 22, 21, 7],  # Gate, K-Style, Rodent, EQ
                "character": "aggressive",
                "params": {"drive": 0.8, "gate": 0.3}
            },
            "jazz": {
                "engines": [1, 7, 39],  # Opto, EQ, Plate
                "character": "smooth",
                "params": {"ratio": 0.3, "warmth": 0.6}
            },
            "edm": {
                "engines": [2, 9, 36, 42],  # Compressor, Filter, Delay, Shimmer
                "character": "modern",
                "params": {"ratio": 0.7, "cutoff": 0.6}
            },
            "ambient": {
                "engines": [42, 46, 35],  # Shimmer, Dimension, Digital Delay
                "character": "spacious",
                "params": {"size": 0.9, "mix": 0.5}
            },
            "hiphop": {
                "engines": [2, 18, 34, 40],  # Compressor, Bit Crusher, Tape, Spring
                "character": "lofi",
                "params": {"crush": 0.4, "warmth": 0.5}
            },
            "country": {
                "engines": [1, 24, 40],  # Opto, Chorus, Spring
                "character": "warm",
                "params": {"depth": 0.3, "mix": 0.2}
            },
            "classical": {
                "engines": [7, 38],  # EQ, Hall Reverb
                "character": "natural",
                "params": {"size": 0.7, "decay": 0.6}
            },
            "rock": {
                "engines": [15, 22, 34, 39],  # Tube, K-Style, Tape, Plate
                "character": "vintage",
                "params": {"drive": 0.6, "warmth": 0.5}
            }
        }
        
        template = genre_templates.get(genre, genre_templates["rock"])
        
        # Create base preset
        preset = {
            "name": f"{genre.title()} {style.title()}",
            "creative_name": self._generate_creative_name(genre, style),
        }
        
        # Add engines
        engines = template["engines"].copy()
        if style == "heavy":
            # Add more distortion
            if 20 not in engines:  # Muff Fuzz
                engines.append(20)
        elif style == "clean":
            # Remove distortion
            engines = [e for e in engines if e not in range(15, 23)]
            if 7 not in engines:  # Add EQ
                engines.append(7)
        
        # Place engines in slots
        for i, engine_id in enumerate(engines[:6], 1):
            preset[f"slot{i}_engine"] = engine_id
            
            # Generate varied parameters
            for param in range(10):
                if param == 0:  # Main parameter
                    base = template["params"].get("drive", 0.5)
                elif param == 5:  # Mix parameter
                    base = template["params"].get("mix", 0.3)
                else:
                    base = 0.5
                
                # Add variation
                variation = random.uniform(-0.2, 0.2)
                value = max(0.0, min(1.0, base + variation))
                preset[f"slot{i}_param{param}"] = round(value, 3)
        
        # Fill remaining slots with 0
        for slot in range(len(engines) + 1, 7):
            preset[f"slot{slot}_engine"] = 0
        
        return preset
    
    def _generate_creative_name(self, genre: str, style: str) -> str:
        """Generate creative preset name"""
        
        prefixes = {
            "metal": ["Brutal", "Crushing", "Infernal", "Savage", "Molten"],
            "jazz": ["Smooth", "Velvet", "Midnight", "Silky", "Blue"],
            "edm": ["Neon", "Electric", "Pulse", "Quantum", "Hyper"],
            "ambient": ["Ethereal", "Cosmic", "Floating", "Nebula", "Aurora"],
            "hiphop": ["Street", "Urban", "Boom", "Trap", "Wave"],
            "country": ["Nashville", "Prairie", "Dusty", "Wooden", "Whiskey"],
            "classical": ["Symphony", "Chamber", "Grand", "Majestic", "Royal"],
            "rock": ["Thunder", "Stone", "Rebel", "Highway", "Electric"]
        }
        
        suffixes = {
            "default": ["Preset", "Sound", "Tone", "Mix", "Setting"],
            "heavy": ["Crusher", "Destroyer", "Annihilator", "Devastator", "Obliterator"],
            "clean": ["Polish", "Shine", "Crystal", "Pure", "Clear"],
            "vintage": ["Classic", "Retro", "Nostalgia", "Heritage", "Legacy"],
            "modern": ["Future", "Digital", "Ultra", "Neo", "Cyber"]
        }
        
        prefix_list = prefixes.get(genre, ["Custom"])
        suffix_list = suffixes.get(style, suffixes["default"])
        
        prefix = random.choice(prefix_list)
        suffix = random.choice(suffix_list)
        
        # Sometimes add a middle word
        if random.random() < 0.3:
            middle_words = ["Power", "Dream", "Magic", "Fire", "Ice", "Storm"]
            middle = random.choice(middle_words)
            return f"{prefix} {middle} {suffix}"
        else:
            return f"{prefix} {suffix}"
    
    def generate_batch(self, size: int = 10) -> List[Dict[str, Any]]:
        """Generate a batch of diverse presets"""
        presets = []
        genres = ["metal", "jazz", "edm", "ambient", "hiphop", "country", "classical", "rock"]
        styles = ["default", "heavy", "clean", "vintage", "modern"]
        
        for _ in range(size):
            genre = random.choice(genres)
            style = random.choice(styles)
            
            preset = self.generate_genre_preset(genre, style)
            
            # Validate
            is_valid, errors, score = self.validator.validate_preset(preset)
            
            if is_valid:
                preset["quality_score"] = score
                preset["genre"] = genre
                preset["style"] = style
                presets.append(preset)
            else:
                print(f"Generated invalid preset: {errors[:2]}")
        
        return presets


def main():
    """Demo the expansion framework"""
    print("🚀 CORPUS EXPANSION FRAMEWORK")
    print("="*80)
    
    # Generate some presets
    generator = PresetGenerator()
    validator = PresetValidator()
    
    print("\n📦 GENERATING TEST BATCH...")
    batch = generator.generate_batch(5)
    
    print(f"\nGenerated {len(batch)} valid presets:\n")
    
    for preset in batch:
        print(f"• {preset['creative_name']}")
        print(f"  Genre: {preset['genre']}, Style: {preset['style']}")
        print(f"  Quality Score: {preset['quality_score']:.2f}")
        
        # Show engines
        engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                engines.append(ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})"))
        print(f"  Engines: {', '.join(engines)}")
        
        # Show parameter diversity
        params = []
        for slot in range(1, 7):
            for param in range(3):  # Just show first 3
                key = f"slot{slot}_param{param}"
                if key in preset:
                    params.append(preset[key])
        if params:
            variance = sum((x - 0.5) ** 2 for x in params) / len(params)
            print(f"  Parameter variance: {variance:.3f}")
        print()
    
    print("\n✅ SAFE EXPANSION PROCESS:")
    print("1. Generate presets with PresetGenerator")
    print("2. Validate with PresetValidator") 
    print("3. Only add if quality_score > 0.7")
    print("4. Monitor impact on search quality")
    print("5. Rollback if performance degrades")

if __name__ == "__main__":
    main()