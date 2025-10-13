"""
Improved Alchemist with Better Naming and Engine Distribution
"""

import logging
import random
from typing import Dict, Any, Tuple, List
from engine_defaults import ENGINE_DEFAULTS
from engine_mapping_authoritative import ENGINE_NAMES
from signal_chain_intelligence import SignalChainIntelligence
from engine_knowledge_base import ENGINE_KNOWLEDGE

logger = logging.getLogger(__name__)

class AlchemistImproved:
    """
    Improved Alchemist with creative naming and better engine distribution
    """
    
    def __init__(self):
        # Initialize signal chain intelligence
        self.signal_intelligence = SignalChainIntelligence()
        
        # Music theory knowledge
        self.music_theory = {
            "warm": {
                "engines": [1, 15, 39],  # Opto, Tube, Plate
                "params": {"drive": 0.3, "bias": 0.6}
            },
            "aggressive": {
                "engines": [4, 22, 21, 20],  # Gate, K-Style, Rodent, Muff
                "params": {"drive": 0.8, "threshold": 0.3}
            },
            "ambient": {
                "engines": [42, 39, 35],  # Shimmer, Plate, Delay
                "params": {"size": 0.8, "decay": 0.7, "mix": 0.4}
            },
            "clean": {
                "engines": [2, 7, 54],  # Compressor, EQ, Gain
                "params": {"ratio": 0.3, "threshold": 0.5}
            },
            "vintage": {
                "engines": [1, 15, 34, 40],  # Opto, Tube, Tape Echo, Spring
                "params": {"warmth": 0.6, "saturation": 0.4}
            }
        }
        
        # Parameter safety matrix
        self.parameter_safety = {
            "feedback_limits": {
                "delay": 0.85,
                "reverb": 0.95,
                "comb": 0.75
            },
            "gain_staging": {
                "max_total": 1.5,
                "per_stage": 0.8
            },
            "resonance_safety": {
                "with_low_cutoff": 0.6,
                "normal": 0.85
            }
        }
        
        # IMPROVED: Much more diverse name components
        self.name_components = {
            "genre_specific": {
                "metal": ["Infernal", "Apocalyptic", "Molten", "Vicious", "Chainsaw"],
                "jazz": ["Smooth", "Silky", "Midnight", "Blue Note", "Velvet"],
                "ambient": ["Ethereal", "Cosmic", "Nebula", "Aurora", "Starfield"],
                "rock": ["Thunder", "Lightning", "Stone", "Granite", "Magma"],
                "pop": ["Sparkling", "Radiant", "Diamond", "Shimmer", "Gleaming"],
                "trap": ["808", "Trap Lord", "Metro", "Dark Mode", "Icy"],
                "lofi": ["Dusty", "Tape", "Nostalgia", "Bedroom", "Worn"],
                "electronic": ["Neon", "Circuit", "Digital", "Synthetic", "Cyber"],
                "folk": ["Wooden", "Hearth", "Cabin", "Mountain", "River"],
                "classical": ["Symphony", "Orchestral", "Majestic", "Grand", "Royal"]
            },
            "mood_based": {
                "dark": ["Shadow", "Midnight", "Obsidian", "Void", "Abyss"],
                "bright": ["Sunrise", "Golden", "Luminous", "Brilliant", "Solar"],
                "mysterious": ["Enigma", "Mystic", "Secret", "Hidden", "Arcane"],
                "energetic": ["Electric", "Turbo", "Hyper", "Kinetic", "Dynamic"],
                "calm": ["Serene", "Tranquil", "Peaceful", "Zen", "Gentle"],
                "chaotic": ["Chaos", "Mayhem", "Pandemonium", "Frenzy", "Storm"]
            },
            "texture_based": {
                "warm": ["Honey", "Caramel", "Amber", "Sunset", "Fireplace"],
                "cold": ["Arctic", "Frost", "Ice", "Crystal", "Winter"],
                "smooth": ["Silk", "Butter", "Cream", "Satin", "Polished"],
                "rough": ["Gravel", "Sandpaper", "Concrete", "Rust", "Barbed"],
                "thick": ["Molasses", "Dense", "Heavy", "Massive", "Thick"],
                "thin": ["Wire", "Needle", "Razor", "Glass", "Paper"]
            },
            "effect_suffixes": {
                "reverb_heavy": ["Cathedral", "Canyon", "Cosmos", "Cavern", "Temple"],
                "reverb_light": ["Room", "Studio", "Chamber", "Space", "Air"],
                "distortion_heavy": ["Annihilator", "Obliterator", "Devastator", "Pulverizer"],
                "distortion_light": ["Driver", "Booster", "Warmer", "Saturator"],
                "delay_heavy": ["Infinity", "Cascade", "Repeater", "Echo Chamber"],
                "delay_light": ["Echo", "Bounce", "Reflection", "Ghost"],
                "modulation": ["Swirl", "Wave", "Pulse", "Oscillation", "Flux"],
                "filter": ["Sweep", "Resonator", "Sculptor", "Carver"],
                "compression": ["Squasher", "Tightener", "Controller", "Limiter"],
                "pitch": ["Shifter", "Harmonizer", "Bender", "Transformer"]
            }
        }
    
    def finalize_preset(self, preset: Dict[str, Any], prompt: str = "", context: Dict = None) -> Dict[str, Any]:
        """
        Perform final validation, optimization, and safety checks on the preset
        """
        try:
            logger.info(f"Finalizing preset for prompt: '{prompt}'")
            finalized = preset.copy()
            
            # Step 1: Optimize signal chain ordering
            logger.info("Step 1: Optimizing signal chain...")
            finalized = self.signal_intelligence.optimize_signal_chain(finalized)
            
            # Step 2: Validate parameters
            logger.info("Step 2: Validating parameters...")
            is_safe, warnings = self.signal_intelligence.validate_parameters(finalized)
            
            if not is_safe:
                logger.warning(f"Safety issues detected: {warnings}")
                finalized = self._apply_safety_corrections(finalized, warnings)
            
            # Step 3: Engine count optimization (target 3-5 engines, not 6)
            finalized = self._optimize_engine_count(finalized, prompt, context)
            
            # Step 4: Parameter refinement
            logger.info("Step 4: Refining parameters...")
            finalized = self._refine_parameters(finalized)
            
            # Step 5: Apply any character nudges
            finalized = self._apply_character_adjustments(finalized, prompt)
            
            # Step 6: Create signal flow description
            finalized["signal_flow"] = self.signal_intelligence.explain_chain(finalized)
            
            # Step 7: Generate IMPROVED creative name
            logger.info("Step 7: Generating creative name...")
            finalized["name"] = self._generate_creative_name(finalized, prompt, context)
            
            # Step 8: Final validation
            finalized = self._ensure_validity(finalized)
            
            logger.info(f"Preset finalized: '{finalized.get('name', 'Unknown')}'")
            return finalized
            
        except Exception as e:
            logger.error(f"Error in finalize_preset: {str(e)}")
            return self._create_safe_default(prompt)
    
    def _optimize_engine_count(self, preset: Dict[str, Any], prompt: str, context: Dict = None) -> Dict[str, Any]:
        """
        Optimize engine count to 3-5 range (not always 6)
        Remove unnecessary engines unless they're required
        """
        # Count active engines
        active_engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                active_engines.append((slot, engine_id))
        
        # If we have 6 engines, consider removing some
        if len(active_engines) >= 6:
            # Get required engines from context
            required_engines = []
            if context and "required_engines" in context:
                required_engines = context["required_engines"]
            
            # Identify candidates for removal (not required, not essential)
            removal_candidates = []
            for slot, engine_id in active_engines:
                if engine_id not in required_engines:
                    # Check if it's a commonly overused engine
                    if engine_id in [39, 15]:  # Plate Reverb, Vintage Tube
                        # Only keep if specifically requested
                        if not any(word in prompt.lower() for word in ["plate", "tube", "vintage"]):
                            removal_candidates.append((slot, engine_id))
            
            # Remove 1-2 engines to get to 4-5 range
            if removal_candidates:
                num_to_remove = min(2, len(removal_candidates))
                for slot, engine_id in removal_candidates[:num_to_remove]:
                    logger.info(f"Removing overused engine {ENGINE_NAMES.get(engine_id)} from slot {slot}")
                    preset[f"slot{slot}_engine"] = 0
                    for p in range(10):
                        key = f"slot{slot}_param{p}"
                        if key in preset:
                            del preset[key]
        
        return preset
    
    def _generate_creative_name(self, preset: Dict[str, Any], prompt: str, context: Dict = None) -> str:
        """
        IMPROVED: Generate truly creative names based on multiple factors
        """
        prompt_lower = prompt.lower()
        
        # First check if Cloud AI provided a creative name
        if context and "cloud_blueprint" in context:
            cloud_name = context["cloud_blueprint"].get("creative_name", "")
            if cloud_name and "sonic" not in cloud_name.lower() and len(cloud_name) > 5:
                # Use Cloud AI name if it's good
                return cloud_name
        
        # Analyze the preset's character
        engines_used = []
        effect_types = set()
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                engine_name = ENGINE_NAMES.get(engine_id, "")
                engines_used.append(engine_name)
                
                # Categorize effects
                if "reverb" in engine_name.lower():
                    effect_types.add("reverb")
                elif "distortion" in engine_name.lower() or "overdrive" in engine_name.lower():
                    effect_types.add("distortion")
                elif "delay" in engine_name.lower() or "echo" in engine_name.lower():
                    effect_types.add("delay")
                elif "chorus" in engine_name.lower() or "phaser" in engine_name.lower():
                    effect_types.add("modulation")
                elif "filter" in engine_name.lower():
                    effect_types.add("filter")
                elif "compressor" in engine_name.lower() or "limiter" in engine_name.lower():
                    effect_types.add("compression")
        
        # Detect genre
        genre = None
        for g, keywords in [
            ("metal", ["metal", "heavy", "brutal", "aggressive"]),
            ("jazz", ["jazz", "smooth", "saxophone", "piano"]),
            ("ambient", ["ambient", "ethereal", "space", "atmospheric"]),
            ("rock", ["rock", "guitar", "led zeppelin", "classic rock"]),
            ("pop", ["pop", "modern", "bright", "polished"]),
            ("trap", ["trap", "808", "hip hop", "beat"]),
            ("lofi", ["lo-fi", "lofi", "bedroom", "tape"]),
            ("electronic", ["synth", "electronic", "edm", "techno"]),
            ("folk", ["folk", "acoustic", "intimate", "warm"]),
            ("classical", ["orchestral", "strings", "cinematic", "symphony"])
        ]:
            if any(kw in prompt_lower for kw in keywords):
                genre = g
                break
        
        # Detect mood
        mood = None
        for m, keywords in [
            ("dark", ["dark", "horror", "scary", "evil", "sinister"]),
            ("bright", ["bright", "happy", "uplifting", "positive"]),
            ("mysterious", ["mysterious", "strange", "weird", "unusual"]),
            ("energetic", ["energetic", "powerful", "strong", "intense"]),
            ("calm", ["calm", "peaceful", "relaxed", "gentle"]),
            ("chaotic", ["chaotic", "crazy", "wild", "insane"])
        ]:
            if any(kw in prompt_lower for kw in keywords):
                mood = m
                break
        
        # Detect texture
        texture = None
        for t, keywords in [
            ("warm", ["warm", "cozy", "analog", "vintage"]),
            ("cold", ["cold", "icy", "digital", "pristine"]),
            ("smooth", ["smooth", "silky", "clean", "polished"]),
            ("rough", ["rough", "gritty", "dirty", "raw"]),
            ("thick", ["thick", "dense", "fat", "heavy"]),
            ("thin", ["thin", "bright", "crisp", "clear"])
        ]:
            if any(kw in prompt_lower for kw in keywords):
                texture = t
                break
        
        # Build name from components
        prefix_candidates = []
        
        # Add genre-specific prefixes
        if genre and genre in self.name_components["genre_specific"]:
            prefix_candidates.extend(self.name_components["genre_specific"][genre])
        
        # Add mood-based prefixes
        if mood and mood in self.name_components["mood_based"]:
            prefix_candidates.extend(self.name_components["mood_based"][mood])
        
        # Add texture-based prefixes
        if texture and texture in self.name_components["texture_based"]:
            prefix_candidates.extend(self.name_components["texture_based"][texture])
        
        # If no specific category matched, use a random creative prefix
        if not prefix_candidates:
            all_prefixes = []
            for category in self.name_components["genre_specific"].values():
                all_prefixes.extend(category)
            for category in self.name_components["mood_based"].values():
                all_prefixes.extend(category)
            prefix_candidates = random.sample(all_prefixes, min(10, len(all_prefixes)))
        
        # Select suffix based on main effect
        suffix_candidates = []
        
        if "reverb" in effect_types:
            if any("shimmer" in eng.lower() or "hall" in eng.lower() for eng in engines_used):
                suffix_candidates.extend(self.name_components["effect_suffixes"]["reverb_heavy"])
            else:
                suffix_candidates.extend(self.name_components["effect_suffixes"]["reverb_light"])
        
        if "distortion" in effect_types:
            if any("muff" in eng.lower() or "rodent" in eng.lower() for eng in engines_used):
                suffix_candidates.extend(self.name_components["effect_suffixes"]["distortion_heavy"])
            else:
                suffix_candidates.extend(self.name_components["effect_suffixes"]["distortion_light"])
        
        if "delay" in effect_types:
            if "magnetic" in prompt_lower or "tape" in prompt_lower:
                suffix_candidates.extend(self.name_components["effect_suffixes"]["delay_heavy"])
            else:
                suffix_candidates.extend(self.name_components["effect_suffixes"]["delay_light"])
        
        if "modulation" in effect_types:
            suffix_candidates.extend(self.name_components["effect_suffixes"]["modulation"])
        
        # Default suffixes if none matched
        if not suffix_candidates:
            suffix_candidates = ["Engine", "Preset", "Sound", "Tone", "Mix"]
        
        # Create name
        prefix = random.choice(prefix_candidates) if prefix_candidates else "Custom"
        suffix = random.choice(suffix_candidates) if suffix_candidates else "Sound"
        
        # Ensure we don't create duplicate names
        name = f"{prefix} {suffix}"
        
        # Add variation if needed
        if random.random() < 0.3:  # 30% chance to add a variation
            variations = ["Pro", "Plus", "X", "Ultra", "Max", "Prime", "Elite"]
            name = f"{name} {random.choice(variations)}"
        
        return name
    
    def _apply_character_adjustments(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:
        """
        Apply subtle character adjustments based on the prompt
        """
        prompt_lower = prompt.lower()
        
        # Apply character-based adjustments
        if "warm" in prompt_lower or "vintage" in prompt_lower:
            # Boost warmth-related parameters
            for slot in range(1, 7):
                if preset.get(f"slot{slot}_engine") == 15:  # Vintage Tube
                    preset[f"slot{slot}_param0"] = min(0.8, preset.get(f"slot{slot}_param0", 0.5) + 0.1)
        
        if "aggressive" in prompt_lower or "brutal" in prompt_lower:
            # Increase drive/distortion
            for slot in range(1, 7):
                engine = preset.get(f"slot{slot}_engine", 0)
                if engine in [20, 21, 22]:  # Distortion engines
                    preset[f"slot{slot}_param0"] = min(0.9, preset.get(f"slot{slot}_param0", 0.5) + 0.2)
        
        if "clean" in prompt_lower or "pristine" in prompt_lower:
            # Reduce distortion, increase clarity
            for slot in range(1, 7):
                engine = preset.get(f"slot{slot}_engine", 0)
                if engine in [20, 21, 22]:  # Distortion engines
                    preset[f"slot{slot}_param0"] = max(0.1, preset.get(f"slot{slot}_param0", 0.5) - 0.2)
        
        return preset
    
    def _refine_parameters(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Refine parameters for better sound
        """
        # Apply engine-specific refinements
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and engine_id in ENGINE_DEFAULTS:
                defaults_info = ENGINE_DEFAULTS[engine_id]
                
                # Get the params dict
                if "params" in defaults_info:
                    params = defaults_info["params"]
                    
                    # Ensure critical parameters are within safe ranges
                    for param_key, default_val in params.items():
                        if param_key.startswith("param"):
                            try:
                                param_num = int(param_key.replace("param", ""))
                                key = f"slot{slot}_param{param_num}"
                                
                                # If parameter is at extreme values, pull it back slightly
                                current = preset.get(key, default_val)
                                # Ensure we have a numeric value
                                try:
                                    current_float = float(current)
                                    if current_float > 0.95:
                                        preset[key] = 0.9
                                    elif current_float < 0.05:
                                        preset[key] = 0.1
                                except (TypeError, ValueError):
                                    # If conversion fails, use default
                                    preset[key] = default_val
                            except ValueError:
                                continue
        
        return preset
    
    def _apply_safety_corrections(self, preset: Dict[str, Any], warnings: List[str]) -> Dict[str, Any]:
        """
        Apply safety corrections based on warnings
        """
        for warning in warnings:
            if "feedback" in warning.lower():
                # Reduce feedback parameters
                for slot in range(1, 7):
                    for param in range(10):
                        key = f"slot{slot}_param{param}"
                        if key in preset and preset[key] > 0.8:
                            preset[key] = 0.75
            
            if "gain" in warning.lower():
                # Reduce overall gain
                preset["master_output"] = min(0.7, preset.get("master_output", 0.7))
        
        return preset
    
    def _ensure_validity(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Ensure all required fields are present
        """
        # Ensure master controls
        preset.setdefault("master_input", 0.7)
        preset.setdefault("master_output", 0.7)
        preset.setdefault("master_mix", 1.0)
        
        return preset
    
    def _create_safe_default(self, prompt: str) -> Dict[str, Any]:
        """
        Create a safe default preset when things go wrong
        """
        logger.warning("Creating safe default preset")
        return {
            "name": "Safe Default",
            "slot1_engine": 2,  # Classic Compressor
            "slot1_param0": 0.5,
            "slot1_param1": 0.3,
            "slot2_engine": 7,  # Parametric EQ
            "slot2_param0": 0.5,
            "slot2_param1": 0.5,
            "slot3_engine": 39,  # Plate Reverb
            "slot3_param0": 0.4,
            "slot3_param1": 0.3,
            "slot3_param5": 0.2,
            "slot4_engine": 0,
            "slot5_engine": 0,
            "slot6_engine": 0,
            "master_input": 0.7,
            "master_output": 0.7,
            "master_mix": 1.0,
            "signal_flow": "Signal flow: Input → Classic Compressor → Parametric EQ → Plate Reverb → Output"
        }