import logging
import random
from typing import Dict, Any, Tuple, List
from engine_defaults import ENGINE_DEFAULTS

logger = logging.getLogger(__name__)

class Alchemist:
    """
    The Alchemist performs final validation, safety checks, and parameter clamping
    to ensure the preset is valid and safe for the audio plugin.
    """
    
    def __init__(self):
        # Name generation components - organized by vibe/context
        self.vibe_adjectives = {
            # Warm/Vintage vibes
            "warm": ["Warm", "Cozy", "Smooth", "Mellow", "Golden", "Amber", "Honey"],
            "vintage": ["Vintage", "Retro", "Classic", "Analog", "Antique", "Old School", "Nostalgic"],
            "tube": ["Tube", "Valve", "Glowing", "Saturated", "Creamy", "Rich", "Full"],
            
            # Aggressive/Metal vibes
            "aggressive": ["Brutal", "Savage", "Fierce", "Crushing", "Raging", "Vicious", "Relentless"],
            "metal": ["Heavy", "Iron", "Steel", "Chrome", "Titanium", "Molten", "Forged"],
            "distorted": ["Distorted", "Mangled", "Twisted", "Shredded", "Torn", "Warped", "Mutated"],
            
            # Spacious/Ambient vibes
            "spacious": ["Vast", "Expansive", "Infinite", "Boundless", "Wide", "Open", "Panoramic"],
            "ambient": ["Floating", "Drifting", "Ethereal", "Atmospheric", "Weightless", "Suspended", "Hovering"],
            "ethereal": ["Celestial", "Heavenly", "Angelic", "Divine", "Transcendent", "Otherworldly", "Sacred"],
            
            # Dynamic/Punchy vibes
            "punchy": ["Punchy", "Snappy", "Tight", "Crisp", "Sharp", "Quick", "Responsive"],
            "dynamic": ["Dynamic", "Powerful", "Energetic", "Vibrant", "Alive", "Active", "Kinetic"],
            
            # Lo-fi/Character vibes
            "lofi": ["Lo-Fi", "Dusty", "Worn", "Faded", "Weathered", "Degraded", "Nostalgic"],
            "character": ["Textured", "Gritty", "Raw", "Organic", "Natural", "Imperfect", "Authentic"],
            
            # Clean/Pristine vibes
            "clean": ["Crystal", "Pure", "Clear", "Transparent", "Pristine", "Polished", "Refined"],
            "modern": ["Digital", "Future", "Cyber", "Neon", "Quantum", "Neural", "Binary"],
            
            # Default/Generic
            "default": ["Sonic", "Audio", "Sound", "Tonal", "Harmonic", "Musical", "Acoustic"]
        }
        
        self.vibe_nouns = {
            # Effect-based nouns
            "delay": ["Echo", "Repeat", "Reflection", "Bounce", "Trail", "Ghost", "Memory"],
            "reverb": ["Space", "Chamber", "Hall", "Cathedral", "Cave", "Room", "Sanctuary"],
            "distortion": ["Crusher", "Grinder", "Shredder", "Destroyer", "Annihilator", "Devastator", "Obliterator"],
            "modulation": ["Swirl", "Wave", "Pulse", "Cycle", "Motion", "Flow", "Current"],
            "filter": ["Sculptor", "Shaper", "Carver", "Former", "Molder", "Designer", "Architect"],
            "compression": ["Squeezer", "Tightener", "Controller", "Limiter", "Governor", "Restrainer", "Compactor"],
            
            # Instrument-based nouns
            "guitar": ["Axe", "Strings", "Frets", "Riff", "Lick", "Chord", "Strum"],
            "drums": ["Beat", "Groove", "Rhythm", "Pulse", "Thunder", "Boom", "Impact"],
            "vocal": ["Voice", "Whisper", "Shout", "Call", "Song", "Harmony", "Melody"],
            "bass": ["Foundation", "Bottom", "Depth", "Sub", "Low", "Rumble", "Growl"],
            
            # Abstract/Creative nouns
            "ambient": ["Dreams", "Clouds", "Mist", "Fog", "Haze", "Aura", "Atmosphere"],
            "experimental": ["Laboratory", "Experiment", "Discovery", "Innovation", "Creation", "Invention", "Breakthrough"],
            
            # Default nouns
            "default": ["Engine", "Machine", "Processor", "Generator", "Creator", "Builder", "Former"]
        }
        
        self.suffixes = [
            "X", "Pro", "Plus", "Max", "Ultra", "Prime", "Elite", 
            "2000", "3000", "MK2", "MK3", "V2", "V3", "Deluxe", 
            "Studio", "Master", "Signature", "Custom", "Special"
        ]
        
        # Parameter ranges for each engine type
        self.parameter_ranges = {
            # K-Style Overdrive
            0: {
                "param1": (0.0, 1.0),  # Drive
                "param2": (0.0, 1.0),  # Tone
                "param3": (0.0, 1.0)   # Level
            },
            # Tape Echo
            1: {
                "param1": (0.0, 1.0),  # Time
                "param2": (0.0, 0.95), # Feedback (limited to prevent runaway)
                "param3": (0.0, 1.0),  # Mix
                "param4": (0.0, 1.0),  # Wow
                "param5": (0.0, 1.0)   # Flutter
            },
            # Plate Reverb
            2: {
                "param1": (0.0, 1.0),  # Size
                "param2": (0.0, 1.0),  # Damping
                "param3": (0.0, 1.0),  # Predelay
                "param4": (0.0, 1.0)   # Mix
            }
        }
        
        # Safety limits - MORE CONSERVATIVE to prevent distortion
        self.safety_limits = {
            "max_total_gain": 1.0,      # Maximum combined gain across slots (reduced)
            "max_feedback": 0.75,       # Maximum feedback to prevent runaway (reduced)
            "min_output_level": 0.1,    # Minimum output level to prevent silence
            "max_drive": 0.5,           # Maximum drive/distortion amount
            "max_mix_per_slot": 0.5     # Maximum mix level per slot
        }
    
    def finalize_preset(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Perform final validation and safety checks on the preset
        """
        try:
            finalized = preset.copy()
            
            # Step 1: Validate and clamp all parameters
            self._validate_parameters(finalized)
            
            # Step 2: Apply safety checks
            self._apply_safety_checks(finalized)
            
            # Step 3: Ensure preset structure is complete
            self._ensure_complete_structure(finalized)
            
            # Step 4: Generate fresh name based on vibe
            # Always generate a new name to reflect the current prompt/vibe
            finalized["name"] = self.generate_preset_name(finalized)
            
            # Step 5: Add validation metadata
            finalized["alchemist_validated"] = True
            finalized["validation_warnings"] = self._check_for_warnings(finalized)
            
            logger.info("Preset finalized successfully")
            return finalized
            
        except Exception as e:
            logger.error(f"Error in finalize_preset: {str(e)}")
            # Return a safe default preset on error
            return self._create_safe_default()
    
    def generate_preset_name(self, preset: Dict[str, Any]) -> str:
        """
        Generate a creative name for the preset based on its characteristics and vibe
        """
        try:
            # First check if a creative name was provided by Visionary
            if "creative_name" in preset and preset["creative_name"]:
                return preset["creative_name"]
            
            # Otherwise, get the vibe from Visionary's analysis
            vibe = preset.get("vibe", "").lower()
            parameters = preset.get("parameters", {})
            active_engines = []
            
            # Find active engines
            for slot in range(1, 7):
                if parameters.get(f"slot{slot}_bypass", 1.0) < 0.5:
                    engine_id = int(parameters.get(f"slot{slot}_engine", 0))
                    if engine_id > 0:
                        active_engines.append(engine_id)
            
            # Analyze vibe and select appropriate word pools
            selected_adjectives = []
            selected_nouns = []
            
            # Parse vibe keywords
            vibe_words = vibe.split()
            
            # Match adjectives based on vibe
            for word in vibe_words:
                # Check for adjective matches
                for vibe_key, adj_list in self.vibe_adjectives.items():
                    if vibe_key in word or word in vibe_key:
                        selected_adjectives.extend(adj_list)
                
                # Check for noun context
                for noun_key, noun_list in self.vibe_nouns.items():
                    if noun_key in word or word in noun_key:
                        selected_nouns.extend(noun_list)
            
            # Also check for effect types based on active engines
            effect_types = []
            for engine_id in active_engines:
                if engine_id in [1, 8, 9, 10]:  # Delay engines
                    effect_types.append("delay")
                elif engine_id in [2, 3, 26, 30, 36, 51]:  # Reverb engines
                    effect_types.append("reverb")
                elif engine_id in [4, 5, 36]:  # Distortion engines
                    effect_types.append("distortion")
                elif engine_id in [6, 11, 18]:  # Modulation engines
                    effect_types.append("modulation")
                elif engine_id in [7, 16, 43, 44]:  # Compression engines
                    effect_types.append("compression")
                elif engine_id in [14, 17, 31]:  # Filter engines
                    effect_types.append("filter")
            
            # Add effect-based nouns
            for effect_type in set(effect_types):
                if effect_type in self.vibe_nouns:
                    selected_nouns.extend(self.vibe_nouns[effect_type])
            
            # Fallback to defaults if no matches
            if not selected_adjectives:
                selected_adjectives = self.vibe_adjectives["default"]
            if not selected_nouns:
                selected_nouns = self.vibe_nouns["default"]
            
            # Remove duplicates while preserving some variety
            selected_adjectives = list(set(selected_adjectives))
            selected_nouns = list(set(selected_nouns))
            
            # Generate name
            adj = random.choice(selected_adjectives)
            noun = random.choice(selected_nouns)
            
            # Add suffix based on intensity or special characteristics
            name = f"{adj} {noun}"
            
            # Add suffix occasionally (higher chance for certain vibes)
            suffix_chance = 0.2
            if any(word in vibe for word in ["extreme", "ultimate", "maximum", "professional"]):
                suffix_chance = 0.7
            
            if random.random() < suffix_chance:
                suffix = random.choice(self.suffixes)
                name = f"{name} {suffix}"
            
            return name
                
        except Exception as e:
            logger.error(f"Error generating preset name: {str(e)}")
            # Fallback name generation
            fallback_adj = random.choice(["Custom", "User", "Generated", "New"])
            fallback_noun = random.choice(["Preset", "Sound", "Patch", "Setting"])
            return f"{fallback_adj} {fallback_noun} {random.randint(1000, 9999)}"
    
    def _validate_parameters(self, preset: Dict[str, Any]):
        """Validate and clamp all parameters to valid ranges"""
        parameters = preset.get("parameters", {})
        
        for param_name, value in parameters.items():
            # Special handling for engine selectors - must be integers
            if "_engine" in param_name:
                if isinstance(value, (int, float)):
                    engine_id = int(value)
                    # Ensure engine ID is valid (0-55)
                    engine_id = self._clamp(engine_id, 0, 55)
                    parameters[param_name] = engine_id
                else:
                    parameters[param_name] = 0  # Default to bypass
                continue
            
            # Special handling for bypass parameters - boolean-like
            if "_bypass" in param_name:
                if isinstance(value, (int, float)):
                    parameters[param_name] = 1.0 if float(value) > 0.5 else 0.0
                else:
                    parameters[param_name] = 0.0
                continue
            
            # Handle slot parameters with engine-specific validation
            if "param" in param_name and "slot" in param_name:
                try:
                    slot_num = int(param_name[4])  # Extract slot number
                    param_idx = int(param_name.split("param")[1]) - 1  # Convert to 0-based index
                    
                    # Get engine ID for this slot
                    engine_param = f"slot{slot_num}_engine"
                    engine_id = int(parameters.get(engine_param, 0))
                    
                    # Apply engine-specific validation from ENGINE_DEFAULTS
                    if engine_id in ENGINE_DEFAULTS:
                        engine_info = ENGINE_DEFAULTS[engine_id]
                        param_key = f"param{param_idx + 1}"
                        
                        if param_key in engine_info.get("params", {}):
                            param_info = engine_info["params"][param_key]
                            default_val = param_info.get("default", 0.5)
                            min_val = param_info.get("min", 0.0)
                            max_val = param_info.get("max", 1.0)
                            
                            # Ensure value is valid
                            if not isinstance(value, (int, float)):
                                parameters[param_name] = default_val
                            else:
                                # Clamp to engine-specific range with CONSERVATIVE limits
                                clamped_value = self._clamp(float(value), min_val, max_val)
                                
                                # Additional safety for specific parameter types
                                param_name_lower = param_info.get("name", "").lower()
                                if "drive" in param_name_lower or "gain" in param_name_lower:
                                    clamped_value = min(clamped_value, self.safety_limits["max_drive"])
                                elif "mix" in param_name_lower:
                                    clamped_value = min(clamped_value, self.safety_limits["max_mix_per_slot"])
                                elif "feedback" in param_name_lower:
                                    clamped_value = min(clamped_value, self.safety_limits["max_feedback"])
                                    
                                parameters[param_name] = clamped_value
                        else:
                            # Parameter not used by this engine - set to safe default
                            parameters[param_name] = 0.0
                    else:
                        # Unknown engine - use safe defaults
                        parameters[param_name] = 0.3 if isinstance(value, (int, float)) else 0.3
                        
                except (ValueError, IndexError) as e:
                    logger.warning(f"Error parsing parameter {param_name}: {e}")
                    parameters[param_name] = 0.5
                continue
            
            # Default handling for other parameters
            if not isinstance(value, (int, float)):
                parameters[param_name] = 0.5
            else:
                parameters[param_name] = self._clamp(float(value), 0.0, 1.0)
    
    def _apply_safety_checks(self, preset: Dict[str, Any]):
        """Apply safety checks to prevent audio issues"""
        parameters = preset.get("parameters", {})
        
        # Check 1: Limit total gain to prevent clipping
        total_gain = 0.0
        active_slots = 0
        for slot in range(1, 7):  # All 6 slots
            if f"slot{slot}_bypass" not in parameters or parameters[f"slot{slot}_bypass"] < 0.5:
                # Slot is active
                active_slots += 1
                mix_param = f"slot{slot}_mix"
                if mix_param in parameters:
                    total_gain += parameters[mix_param]
        
        # Scale based on active slots to prevent clipping
        if active_slots > 0 and total_gain > self.safety_limits["max_total_gain"]:
            # Scale down mix levels proportionally
            scale_factor = self.safety_limits["max_total_gain"] / total_gain
            for slot in range(1, 7):
                mix_param = f"slot{slot}_mix"
                if mix_param in parameters and parameters.get(f"slot{slot}_bypass", 1.0) < 0.5:
                    parameters[mix_param] *= scale_factor
            logger.warning(f"Scaled down mix levels to prevent clipping (total gain was {total_gain:.2f})")
        
        # Check 2: Ensure at least one slot is active
        all_bypassed = True
        for slot in range(1, 7):
            if parameters.get(f"slot{slot}_bypass", 1.0) < 0.5:
                all_bypassed = False
                break
        
        if all_bypassed:
            # All slots bypassed - activate slot 1 with safe defaults
            parameters["slot1_bypass"] = 0.0
            parameters["slot1_engine"] = 0  # Vintage Tube
            logger.warning("All slots were bypassed - activated slot 1")
        
        # Check 3: Prevent feedback runaway in delay/echo effects
        delay_engines = [1, 8, 9, 53]  # Tape Echo, Magnetic Drum Echo, Bucket Brigade, Digital Delay
        for slot in range(1, 7):
            engine_param = f"slot{slot}_engine"
            if engine_param in parameters and parameters[engine_param] in delay_engines:
                # Check for feedback parameter (usually param2 or param4)
                for feedback_param_num in [2, 4]:
                    feedback_param = f"slot{slot}_param{feedback_param_num}"
                    if feedback_param in parameters:
                        parameters[feedback_param] = min(parameters[feedback_param], 
                                                       self.safety_limits["max_feedback"])
    
    def _ensure_complete_structure(self, preset: Dict[str, Any]):
        """Ensure the preset has all required fields"""
        # Ensure required top-level fields
        if "name" not in preset:
            preset["name"] = "Alchemist Preset"
        
        if "parameters" not in preset:
            preset["parameters"] = {}
        
        # Ensure all parameter slots exist
        parameters = preset["parameters"]
        
        for slot in range(1, 7):  # All 6 slots
            # Ensure all 10 parameters exist for each slot
            for param in range(1, 11):
                param_name = f"slot{slot}_param{param}"
                if param_name not in parameters:
                    parameters[param_name] = 0.5
            
            # Ensure engine selector exists
            engine_param = f"slot{slot}_engine"
            if engine_param not in parameters:
                parameters[engine_param] = 0  # Bypass
            
            # Ensure bypass exists
            bypass_param = f"slot{slot}_bypass"
            if bypass_param not in parameters:
                parameters[bypass_param] = 1.0  # Default to bypassed
            
            # Ensure mix exists
            mix_param = f"slot{slot}_mix"
            if mix_param not in parameters:
                parameters[mix_param] = 0.5
        
        # Ensure master parameters exist
        if "master_input" not in parameters:
            parameters["master_input"] = 0.7
        if "master_output" not in parameters:
            parameters["master_output"] = 0.7
        if "master_mix" not in parameters:
            parameters["master_mix"] = 1.0
    
    def _check_for_warnings(self, preset: Dict[str, Any]) -> list:
        """Check for potential issues and return warnings"""
        warnings = []
        parameters = preset.get("parameters", {})
        
        # Check for extreme parameter values
        for param_name, value in parameters.items():
            if "param" in param_name and isinstance(value, (int, float)):
                if value < 0.1:
                    warnings.append(f"{param_name} is very low ({value:.2f})")
                elif value > 0.9:
                    warnings.append(f"{param_name} is very high ({value:.2f})")
        
        # Check for potentially problematic combinations
        slot1_engine = parameters.get("slot1_engine", 0)
        slot2_engine = parameters.get("slot2_engine", 0)
        
        if slot1_engine == 2 and slot2_engine == 2:  # Both tape echo
            warnings.append("Double delay effects may cause timing issues")
        
        if slot1_engine == 3 and slot2_engine == 3:  # Both reverb
            warnings.append("Double reverb may cause excessive wash")
        
        return warnings
    
    def _clamp(self, value: float, min_val: float, max_val: float) -> float:
        """Clamp a value between min and max"""
        return max(min_val, min(value, max_val))
    
    def _create_safe_default(self) -> Dict[str, Any]:
        """Create a safe default preset"""
        return {
            "name": "Safe Default",
            "vibe": "neutral",
            "source": "alchemist_default",
            "parameters": {
                # Slot 1 - K-Style Overdrive with moderate settings
                "slot1_engine": 1,
                "slot1_bypass": 0.0,
                "slot1_param1": 0.3,  # Drive
                "slot1_param2": 0.5,  # Tone
                "slot1_param3": 0.7,  # Level
                "slot1_param4": 0.5,
                "slot1_param5": 0.5,
                "slot1_param6": 0.5,
                "slot1_param7": 0.5,
                "slot1_param8": 0.5,
                "slot1_param9": 0.5,
                "slot1_param10": 0.5,
                
                # Slot 2 - Bypassed
                "slot2_engine": 0,
                "slot2_bypass": 1.0,
                "slot2_param1": 0.5,
                "slot2_param2": 0.5,
                "slot2_param3": 0.5,
                "slot2_param4": 0.5,
                "slot2_param5": 0.5,
                "slot2_param6": 0.5,
                "slot2_param7": 0.5,
                "slot2_param8": 0.5,
                "slot2_param9": 0.5,
                "slot2_param10": 0.5
            },
            "alchemist_validated": True,
            "validation_warnings": []
        }