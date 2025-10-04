"""
Preset Modifier - Intelligent preset alteration based on user requests
Interprets both technical commands and poetic descriptions
"""

import logging
from typing import Dict, Any, List, Tuple
import re

logger = logging.getLogger(__name__)

class PresetModifier:
    """
    Modifies existing presets based on natural language requests
    """
    
    def __init__(self):
        # Technical term mappings to parameter adjustments
        self.technical_mappings = {
            # Frequency adjustments
            "brighter": {"tone": 0.15, "highs": 0.2, "brightness": 0.15, "treble": 0.2},
            "darker": {"tone": -0.15, "highs": -0.2, "brightness": -0.15, "treble": -0.2},
            "more bass": {"bass": 0.2, "lows": 0.2, "low_freq": 0.15},
            "less bass": {"bass": -0.2, "lows": -0.2, "low_freq": -0.15},
            "more highs": {"highs": 0.2, "treble": 0.2, "high_freq": 0.15, "brightness": 0.1},
            "less highs": {"highs": -0.2, "treble": -0.2, "high_freq": -0.15, "brightness": -0.1},
            "warmer": {"warmth": 0.2, "saturation": 0.1, "tube": 0.15, "drive": 0.05},
            "colder": {"warmth": -0.2, "saturation": -0.1, "digital": 0.15},
            
            # Dynamics adjustments
            "more compression": {"threshold": -0.15, "ratio": 0.2, "compression": 0.2},
            "less compression": {"threshold": 0.15, "ratio": -0.2, "compression": -0.2},
            "punchier": {"attack": -0.1, "punch": 0.2, "transient": 0.15},
            "smoother": {"attack": 0.1, "release": 0.15, "smooth": 0.2},
            "louder": {"gain": 0.15, "volume": 0.15, "level": 0.15, "output": 0.1},
            "quieter": {"gain": -0.15, "volume": -0.15, "level": -0.15, "output": -0.1},
            "more aggressive": {"drive": 0.25, "distortion": 0.2, "aggression": 0.3},
            "gentler": {"drive": -0.2, "distortion": -0.15, "mix": -0.1},
            
            # Spatial adjustments  
            "more reverb": {"reverb_mix": 0.2, "room_size": 0.15, "decay": 0.1, "reverb": 0.2},
            "less reverb": {"reverb_mix": -0.2, "room_size": -0.15, "decay": -0.1, "reverb": -0.2},
            "bigger": {"size": 0.2, "room": 0.2, "space": 0.25, "width": 0.15},
            "smaller": {"size": -0.2, "room": -0.2, "space": -0.25, "width": -0.15},
            "wider": {"width": 0.25, "stereo": 0.2, "spread": 0.2, "pan_width": 0.2},
            "narrower": {"width": -0.25, "stereo": -0.2, "spread": -0.2, "pan_width": -0.2},
            "more delay": {"delay_mix": 0.2, "feedback": 0.15, "delay_time": 0.1, "echo": 0.2},
            "less delay": {"delay_mix": -0.2, "feedback": -0.15, "echo": -0.2},
            
            # Modulation adjustments
            "more movement": {"rate": 0.15, "speed": 0.15, "lfo_rate": 0.2, "modulation": 0.2},
            "less movement": {"rate": -0.15, "speed": -0.15, "lfo_rate": -0.2, "modulation": -0.2},
            "deeper": {"depth": 0.2, "intensity": 0.15, "amount": 0.15},
            "shallower": {"depth": -0.2, "intensity": -0.15, "amount": -0.15},
            "faster": {"rate": 0.25, "speed": 0.25, "tempo": 0.2},
            "slower": {"rate": -0.25, "speed": -0.25, "tempo": -0.2},
            
            # Character adjustments
            "more vintage": {"vintage": 0.3, "analog": 0.2, "warmth": 0.15, "saturation": 0.1},
            "more modern": {"digital": 0.2, "precision": 0.2, "clean": 0.15},
            "dirtier": {"dirt": 0.25, "grit": 0.2, "saturation": 0.2, "distortion": 0.15},
            "cleaner": {"dirt": -0.25, "grit": -0.2, "saturation": -0.2, "distortion": -0.15},
            "more lofi": {"bitcrush": 0.2, "quality": -0.2, "resolution": -0.15, "artifacts": 0.2},
            "more hifi": {"bitcrush": -0.2, "quality": 0.2, "resolution": 0.15, "artifacts": -0.2},
        }
        
        # Poetic term mappings to multiple adjustments
        self.poetic_mappings = {
            # Emotional/atmospheric descriptions
            "more sunshine": {
                "adjustments": {"brightness": 0.2, "warmth": 0.15, "reverb": 0.1, "highs": 0.15},
                "description": "Adding brightness and warmth for a sunny feel"
            },
            "falling": {
                "adjustments": {"pitch": -0.2, "feedback": 0.2, "reverb": 0.25, "filter_freq": -0.15},
                "description": "Creating a sense of descent and vertigo"
            },
            "floating": {
                "adjustments": {"reverb": 0.3, "delay": 0.2, "attack": 0.15, "release": 0.2},
                "description": "Adding space and smoothness for weightlessness"
            },
            "dreamy": {
                "adjustments": {"reverb": 0.25, "chorus": 0.2, "mix": 0.15, "feedback": 0.1},
                "description": "Creating an ethereal, dreamlike atmosphere"
            },
            "haunting": {
                "adjustments": {"reverb": 0.2, "delay": 0.15, "pitch": -0.05, "resonance": 0.2},
                "description": "Adding mysterious and eerie qualities"
            },
            "energetic": {
                "adjustments": {"drive": 0.2, "compression": 0.15, "treble": 0.1, "transient": 0.2},
                "description": "Boosting energy and excitement"
            },
            "melancholic": {
                "adjustments": {"tone": -0.15, "reverb": 0.2, "chorus": 0.1, "warmth": -0.1},
                "description": "Creating a sad, reflective mood"
            },
            "cosmic": {
                "adjustments": {"reverb": 0.35, "delay": 0.25, "modulation": 0.2, "filter": 0.15},
                "description": "Expanding into space with vast effects"
            },
            "underwater": {
                "adjustments": {"filter_freq": -0.3, "resonance": 0.2, "chorus": 0.25, "delay": 0.15},
                "description": "Creating submerged, filtered textures"
            },
            "crystalline": {
                "adjustments": {"brightness": 0.25, "resonance": 0.15, "reverb": 0.15, "clean": 0.2},
                "description": "Adding clarity and shimmer"
            },
            "thunderous": {
                "adjustments": {"bass": 0.3, "drive": 0.2, "compression": 0.2, "gain": 0.15},
                "description": "Creating powerful, rumbling intensity"
            },
            "silky": {
                "adjustments": {"smoothness": 0.25, "warmth": 0.15, "attack": 0.1, "saturation": 0.1},
                "description": "Adding smooth, luxurious texture"
            },
            "razor sharp": {
                "adjustments": {"treble": 0.3, "resonance": 0.2, "attack": -0.2, "precision": 0.25},
                "description": "Creating cutting, precise edges"
            },
            "organic": {
                "adjustments": {"analog": 0.2, "warmth": 0.15, "saturation": 0.1, "variation": 0.15},
                "description": "Adding natural, analog character"
            },
            "mechanical": {
                "adjustments": {"digital": 0.25, "precision": 0.2, "gate": 0.15, "quantize": 0.2},
                "description": "Creating rigid, machine-like precision"
            },
            "ethereal": {
                "adjustments": {"reverb": 0.3, "shimmer": 0.25, "pitch": 0.1, "mix": 0.2},
                "description": "Creating otherworldly, angelic textures"
            },
            "gritty": {
                "adjustments": {"distortion": 0.25, "bitcrush": 0.15, "saturation": 0.2, "dirt": 0.3},
                "description": "Adding rough, textured character"
            },
            "sparkling": {
                "adjustments": {"brightness": 0.2, "shimmer": 0.3, "treble": 0.15, "chorus": 0.1},
                "description": "Adding bright, shimmering qualities"
            },
            "murky": {
                "adjustments": {"brightness": -0.3, "clarity": -0.2, "reverb": 0.15, "filter": -0.2},
                "description": "Creating dark, unclear textures"
            },
            "explosive": {
                "adjustments": {"attack": -0.3, "drive": 0.3, "compression": 0.25, "gain": 0.2},
                "description": "Creating sudden, powerful impact"
            }
        }
        
        # Parameter name variations (maps common names to standard parameter names)
        self.parameter_aliases = {
            # Gain/Level
            "gain": ["gain", "input", "drive", "level", "volume", "output"],
            "mix": ["mix", "blend", "wet_dry", "balance"],
            
            # EQ/Tone
            "bass": ["bass", "low", "lows", "low_freq", "low_shelf"],
            "mids": ["mid", "mids", "midrange", "mid_freq"],
            "treble": ["treble", "high", "highs", "high_freq", "high_shelf", "brightness"],
            "tone": ["tone", "color", "colour", "character"],
            
            # Dynamics
            "threshold": ["threshold", "thresh", "gate_threshold"],
            "ratio": ["ratio", "compression_ratio", "comp_ratio"],
            "attack": ["attack", "attack_time", "onset"],
            "release": ["release", "release_time", "decay"],
            "compression": ["compression", "comp", "dynamics"],
            
            # Time-based
            "delay": ["delay", "echo", "delay_time", "time"],
            "feedback": ["feedback", "regen", "regeneration", "repeats"],
            "reverb": ["reverb", "room", "hall", "space", "ambience"],
            "size": ["size", "room_size", "hall_size", "space_size"],
            
            # Modulation
            "rate": ["rate", "speed", "frequency", "lfo_rate", "mod_rate"],
            "depth": ["depth", "amount", "intensity", "mod_depth", "mod_amount"],
            
            # Filter
            "frequency": ["frequency", "freq", "cutoff", "filter_freq", "filter_cutoff"],
            "resonance": ["resonance", "res", "q", "emphasis", "peak"],
            
            # Character
            "saturation": ["saturation", "sat", "warmth", "harmonics", "color"],
            "distortion": ["distortion", "dist", "overdrive", "fuzz", "crush"],
        }
    
    def modify_preset(self, current_preset: Dict[str, Any], modification_request: str) -> Dict[str, Any]:
        """
        Modify a preset based on user request
        
        Args:
            current_preset: Current preset parameters
            modification_request: User's modification request (technical or poetic)
            
        Returns:
            Modified preset with adjustments applied
        """
        logger.info(f"Processing modification request: {modification_request}")
        
        # Clone the preset
        import copy
        modified_preset = copy.deepcopy(current_preset)
        
        # Handle both parameter formats (flat dict vs slots array)
        if "slots" in modified_preset:
            # Plugin format with slots array
            slots = modified_preset["slots"]
        else:
            # Legacy format - convert to slots
            parameters = modified_preset.get("parameters", {})
            slots = self._convert_parameters_to_slots(parameters)
        
        # Parse the request
        request_lower = modification_request.lower()
        adjustments = {}
        descriptions = []
        
        # Check for technical terms
        for term, param_adjustments in self.technical_mappings.items():
            if term in request_lower:
                for param, adjustment in param_adjustments.items():
                    if param not in adjustments:
                        adjustments[param] = 0
                    adjustments[param] += adjustment
                descriptions.append(f"Applied '{term}' adjustment")
                
        # Check for poetic terms
        for term, mapping in self.poetic_mappings.items():
            if term in request_lower:
                for param, adjustment in mapping["adjustments"].items():
                    if param not in adjustments:
                        adjustments[param] = 0
                    adjustments[param] += adjustment
                descriptions.append(mapping["description"])
        
        # Parse specific parameter requests (e.g., "increase reverb by 20%")
        param_pattern = r'(increase|decrease|add|remove|more|less)\s+(\w+)(?:\s+by\s+(\d+))?'
        matches = re.finditer(param_pattern, request_lower)
        
        for match in matches:
            action, param_name, amount = match.groups()
            
            # Determine adjustment direction and amount
            if action in ["increase", "add", "more"]:
                adjustment = float(amount) / 100 if amount else 0.2
            else:  # decrease, remove, less
                adjustment = -float(amount) / 100 if amount else -0.2
            
            # Find matching parameter
            param_key = self._find_parameter_key(param_name)
            if param_key:
                if param_key not in adjustments:
                    adjustments[param_key] = 0
                adjustments[param_key] += adjustment
                descriptions.append(f"{action.capitalize()} {param_name}")
        
        # Apply adjustments to actual preset parameters
        applied_changes = []
        
        if "slots" in modified_preset:
            # Apply to slots array format
            for slot_idx, slot in enumerate(slots):
                if slot and "engine" in slot and "params" in slot:
                    engine_id = slot["engine"]
                    params = slot["params"]
                    
                    # Apply adjustments based on engine type
                    engine_type = self._get_engine_type(engine_id)
                    
                    for adj_param, adj_value in adjustments.items():
                        param_indices = self._get_param_indices_for_adjustment(engine_type, adj_param)
                        for param_idx in param_indices:
                            if param_idx < len(params):
                                old_value = params[param_idx]
                                new_value = max(0.0, min(1.0, old_value + adj_value))
                                params[param_idx] = new_value
                                if abs(new_value - old_value) > 0.01:
                                    applied_changes.append(f"Slot {slot_idx+1}, Param {param_idx+1}: {old_value:.2f} → {new_value:.2f}")
        else:
            # Apply to flat parameter format
            parameters = modified_preset.get("parameters", {})
            for slot in range(1, 7):
                for param_num in range(1, 16):
                    param_key = f"slot{slot}_param{param_num}"
                    if param_key in parameters:
                        for adj_param, adj_value in adjustments.items():
                            if self._parameter_matches(param_key, adj_param, modified_preset):
                                old_value = parameters[param_key]
                                new_value = max(0.0, min(1.0, old_value + adj_value))
                                parameters[param_key] = new_value
                                if abs(new_value - old_value) > 0.01:
                                    applied_changes.append(f"{param_key}: {old_value:.2f} → {new_value:.2f}")
        
        # Update mix levels for engines we're modifying
        if "reverb" in adjustments or "delay" in adjustments:
            if "slots" in modified_preset:
                self._adjust_effect_mix_for_slots(slots, adjustments)
            else:
                self._adjust_effect_mix(parameters, adjustments)
        
        # Add modification metadata
        modified_preset["modification_applied"] = {
            "request": modification_request,
            "adjustments": adjustments,
            "descriptions": descriptions,
            "changes": applied_changes
        }
        
        logger.info(f"Applied {len(applied_changes)} parameter changes")
        
        return modified_preset
    
    def _convert_parameters_to_slots(self, parameters: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Convert flat parameter format to slots array format"""
        slots = []
        
        for slot_num in range(1, 7):
            engine_key = f"slot{slot_num}_engine"
            if engine_key in parameters:
                engine_id = parameters.get(engine_key, 0)
                if engine_id > 0:
                    # Gather parameters for this slot
                    params = []
                    for param_num in range(1, 16):
                        param_key = f"slot{slot_num}_param{param_num}"
                        params.append(parameters.get(param_key, 0.5))
                    
                    # Create slot entry
                    slot = {
                        "slot": slot_num - 1,  # 0-based for slots array
                        "engine": engine_id,
                        "params": params,
                        "bypass": parameters.get(f"slot{slot_num}_bypass", 0.0),
                        "mix": parameters.get(f"slot{slot_num}_mix", 0.5),
                        "solo": parameters.get(f"slot{slot_num}_solo", 0.0)
                    }
                    slots.append(slot)
        
        return slots
    
    def _find_parameter_key(self, param_name: str) -> str:
        """Find the standard parameter key for a given name"""
        param_lower = param_name.lower()
        
        # Check direct parameter aliases
        for key, aliases in self.parameter_aliases.items():
            if param_lower in aliases:
                return key
                
        # Return as-is if no alias found
        return param_lower
    
    def _parameter_matches(self, param_key: str, target_param: str, preset: Dict[str, Any]) -> bool:
        """
        Check if a parameter key matches the target parameter type
        """
        # Extract slot and parameter number
        match = re.match(r'slot(\d+)_param(\d+)', param_key)
        if not match:
            return False
            
        slot_num = int(match.group(1))
        param_num = int(match.group(2))
        
        # Get the engine type for this slot
        engine_key = f"slot{slot_num}_engine"
        parameters = preset.get("parameters", {})
        engine_id = parameters.get(engine_key, 0)
        
        # Import engine defaults to understand parameter mappings
        try:
            from engine_defaults import ENGINE_DEFAULTS
            if engine_id in ENGINE_DEFAULTS:
                engine_info = ENGINE_DEFAULTS[engine_id]
                param_info_key = f"param{param_num}"
                if param_info_key in engine_info.get("params", {}):
                    param_info = engine_info["params"][param_info_key]
                    param_name = param_info.get("name", "").lower()
                    
                    # Check if this parameter matches our target
                    target_lower = target_param.lower()
                    
                    # Direct match
                    if target_lower in param_name:
                        return True
                    
                    # Check aliases
                    for key, aliases in self.parameter_aliases.items():
                        if target_lower == key:
                            return any(alias in param_name for alias in aliases)
                            
        except ImportError:
            pass
            
        return False
    
    def _get_engine_type(self, engine_id: int) -> str:
        """Get the engine type category"""
        # Based on engine mapping
        if engine_id in range(0, 14):
            return "dynamics"
        elif engine_id in range(14, 23):
            return "distortion"
        elif engine_id in range(23, 28):
            return "modulation"
        elif engine_id in range(28, 34):
            return "filter"
        elif engine_id in range(34, 40):
            return "pitch"
        elif engine_id in range(40, 47):
            return "time_based"
        elif engine_id in range(47, 52):
            return "utility"
        elif engine_id in range(52, 57):
            return "spatial"
        else:
            return "unknown"
    
    def _get_param_indices_for_adjustment(self, engine_type: str, adj_param: str) -> List[int]:
        """Get parameter indices that should be adjusted for a given adjustment type"""
        # Map adjustment types to typical parameter indices (0-based)
        # These are common patterns across engine types
        
        mappings = {
            "dynamics": {
                "threshold": [0],  # Usually first param
                "ratio": [1],      # Usually second param
                "attack": [2],     # Usually third
                "release": [3],    # Usually fourth
                "gain": [4, 9],    # Gain/makeup gain
                "mix": [14],       # Mix is usually last
            },
            "distortion": {
                "drive": [0, 1],   # Drive/gain
                "tone": [2, 3],    # Tone controls
                "brightness": [3, 4], # High frequency
                "warmth": [5, 6],  # Character/warmth
                "saturation": [0, 1, 7], # Multiple saturation controls
                "mix": [14],
            },
            "time_based": {
                "reverb": [0, 1, 14],   # Size, decay, mix
                "delay": [0, 1, 2, 14], # Time, feedback, filter, mix
                "size": [0],       # Room size
                "decay": [1],      # Decay time
                "feedback": [2],   # Feedback amount
                "mix": [14],
            },
            "modulation": {
                "rate": [0],       # Rate/speed
                "depth": [1],      # Depth/amount
                "feedback": [2],   # Feedback
                "mix": [14],
            },
            "filter": {
                "frequency": [0],  # Cutoff frequency
                "resonance": [1],  # Resonance/Q
                "drive": [2],      # Drive
                "mix": [14],
            },
            "pitch": {
                "pitch": [0],      # Pitch shift amount
                "formant": [1],    # Formant shift
                "mix": [14],
            },
            "spatial": {
                "width": [0, 1],   # Width/spread
                "depth": [2, 3],   # Depth
                "mix": [14],
            },
            "utility": {
                "gain": [0],       # Gain
                "mix": [14],
            }
        }
        
        # Get the mapping for this engine type
        engine_mappings = mappings.get(engine_type, {})
        
        # Find matching parameter indices
        indices = []
        adj_lower = adj_param.lower()
        
        for param_name, param_indices in engine_mappings.items():
            if adj_lower in param_name or param_name in adj_lower:
                indices.extend(param_indices)
        
        # If no specific mapping found, try common indices
        if not indices:
            if "mix" in adj_lower:
                indices = [14]  # Mix is usually last
            elif "gain" in adj_lower or "volume" in adj_lower:
                indices = [0, 4, 9]  # Common gain positions
            elif adj_lower in ["more", "less", "increase", "decrease"]:
                indices = [0, 1, 14]  # Affect main params and mix
        
        return list(set(indices))  # Remove duplicates
    
    def _adjust_effect_mix_for_slots(self, slots: List[Dict[str, Any]], adjustments: Dict[str, Any]):
        """Adjust mix levels for slots format"""
        for slot in slots:
            if slot and "engine" in slot and "params" in slot:
                engine_id = slot["engine"]
                params = slot["params"]
                
                # Reverb engines: 42-46
                if engine_id in range(42, 47) and "reverb" in adjustments:
                    if len(params) > 14:
                        params[14] = max(0.0, min(1.0, params[14] + adjustments["reverb"] * 0.5))
                        
                # Delay engines: 40-41
                elif engine_id in [40, 41] and "delay" in adjustments:
                    if len(params) > 14:
                        params[14] = max(0.0, min(1.0, params[14] + adjustments["delay"] * 0.5))
    
    def _adjust_effect_mix(self, parameters: Dict[str, Any], adjustments: Dict[str, Any]):
        """
        Adjust mix levels for effects that were modified
        """
        # Find reverb/delay engines and adjust their mix
        for slot in range(1, 7):
            engine_key = f"slot{slot}_engine"
            if engine_key in parameters:
                engine_id = parameters[engine_key]
                
                # Check if this is a reverb or delay engine
                # Reverb engines: 42-46
                # Delay engines: 40-41, 53
                mix_key = f"slot{slot}_mix"
                
                if engine_id in range(42, 47) and "reverb" in adjustments:
                    # Adjust reverb mix
                    if mix_key in parameters:
                        current_mix = parameters[mix_key]
                        new_mix = max(0.0, min(1.0, current_mix + adjustments["reverb"] * 0.5))
                        parameters[mix_key] = new_mix
                        
                elif engine_id in [40, 41, 53] and "delay" in adjustments:
                    # Adjust delay mix
                    if mix_key in parameters:
                        current_mix = parameters[mix_key]
                        new_mix = max(0.0, min(1.0, current_mix + adjustments["delay"] * 0.5))
                        parameters[mix_key] = new_mix
    
    def suggest_modifications(self, current_preset: Dict[str, Any]) -> List[str]:
        """
        Suggest possible modifications based on current preset
        """
        suggestions = []
        parameters = current_preset.get("parameters", {})
        
        # Analyze current state
        has_reverb = False
        has_delay = False
        has_distortion = False
        brightness_level = 0.5
        
        for slot in range(1, 7):
            engine_key = f"slot{slot}_engine"
            if engine_key in parameters:
                engine_id = parameters[engine_key]
                if engine_id in range(42, 47):
                    has_reverb = True
                elif engine_id in [40, 41, 53]:
                    has_delay = True
                elif engine_id in [15, 16, 17, 18, 19, 20]:
                    has_distortion = True
        
        # Generate contextual suggestions
        if not has_reverb:
            suggestions.append("Add some reverb for space")
        elif has_reverb:
            suggestions.append("Make it more spacious")
            
        if not has_delay:
            suggestions.append("Add delay for depth")
        elif has_delay:
            suggestions.append("More echo and feedback")
            
        if has_distortion:
            suggestions.append("Make it cleaner")
            suggestions.append("More aggressive distortion")
        else:
            suggestions.append("Add some grit and character")
            
        # Always available suggestions
        suggestions.extend([
            "Make it brighter",
            "Make it warmer",
            "Add more punch",
            "Make it feel like sunshine",
            "Create a falling sensation",
            "Make it more ethereal"
        ])
        
        return suggestions[:6]  # Return top 6 suggestions


# Global instance
preset_modifier = PresetModifier()

def modify_preset(current_preset: Dict[str, Any], request: str) -> Dict[str, Any]:
    """Main entry point for preset modification"""
    return preset_modifier.modify_preset(current_preset, request)

def get_suggestions(current_preset: Dict[str, Any]) -> List[str]:
    """Get modification suggestions for current preset"""
    return preset_modifier.suggest_modifications(current_preset)