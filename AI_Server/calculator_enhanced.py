"""
Enhanced Calculator with Deep Musical Intelligence
Handles intelligent parameter adjustments and engine suggestions
"""

import json
import logging
from typing import Dict, Any, List, Optional
from pathlib import Path
from engine_mapping_authoritative import ENGINE_NAMES
from engine_knowledge_base import ENGINE_KNOWLEDGE, find_engines_for_use_case
from signal_chain_intelligence import SignalChainIntelligence
from engine_defaults import ENGINE_DEFAULTS

logger = logging.getLogger(__name__)

class CalculatorEnhanced:
    """
    The Calculator applies intelligent nudges to base presets based on:
    - User prompt intent
    - Musical knowledge
    - Genre conventions
    - Parameter relationships
    """
    
    def __init__(self, rules_path: str = "nudge_rules.json"):
        self.rules_path = rules_path
        self.signal_intelligence = SignalChainIntelligence()
        
        # Load or create nudge rules
        self.nudge_rules = self._load_nudge_rules()
        
        # Musical intelligence for parameter adjustments
        self.parameter_intelligence = {
            "warm": {
                "adjustments": {
                    15: {"param1": 0.4, "param2": 0.6},  # Tube: moderate drive, warm bias
                    1: {"param6": 0.3},  # Opto: add tube harmonics
                    39: {"param2": 0.4},  # Plate: more damping
                }
            },
            "aggressive": {
                "adjustments": {
                    22: {"param0": 0.8, "param1": 0.6},  # K-Style: high drive
                    21: {"param0": 0.9},  # Rodent: max distortion
                    20: {"param0": 0.8},  # Muff: high sustain
                    4: {"param0": 0.3, "param4": 0.8},  # Gate: tight threshold
                }
            },
            "ambient": {
                "adjustments": {
                    42: {"param0": 0.8, "param1": 0.7, "param2": 0.6},  # Shimmer: large/long/shimmery
                    39: {"param0": 0.7, "param1": 0.6},  # Plate: large/long
                    35: {"param0": 0.5, "param1": 0.4, "param2": 0.3},  # Delay: medium time/feedback
                }
            },
            "clean": {
                "adjustments": {
                    2: {"param0": 0.5, "param1": 0.3},  # Compressor: gentle
                    7: {},  # EQ: defaults are fine
                    54: {"param0": 0.5},  # Gain: unity
                }
            },
            "vintage": {
                "adjustments": {
                    1: {"param1": 0.3, "param6": 0.2},  # Opto: vintage compression
                    15: {"param1": 0.3, "param2": 0.5},  # Tube: warm saturation
                    34: {"param3": 0.3, "param4": 0.2},  # Tape Echo: wow/flutter
                }
            },
            "bright": {
                "adjustments": {
                    17: {"param0": 0.3, "param1": 0.7, "param2": 0.2},  # Harmonic Exciter
                    7: {"param6": 0.6, "param7": 0.6},  # EQ: boost highs
                }
            },
            "dark": {
                "adjustments": {
                    9: {"param0": 0.3, "param1": 0.4},  # Ladder: low cutoff
                    7: {"param6": 0.3, "param7": 0.3},  # EQ: cut highs
                }
            },
            "spacious": {
                "adjustments": {
                    42: {"param5": 0.5},  # Shimmer: more mix
                    44: {"param0": 0.7},  # Widener: wide
                    46: {"param0": 0.6, "param1": 0.5},  # Dimension: expansive
                }
            }
        }
        
        # Engine suggestion logic based on intent
        self.engine_suggestions = {
            "vocal": {
                "primary": [1, 7],  # Opto, EQ
                "secondary": [39, 42],  # Reverbs
                "character": [15, 17]  # Tube, Exciter
            },
            "guitar": {
                "primary": [22, 21, 20],  # Overdrives
                "secondary": [25, 30],  # Phaser, Rotary
                "character": [34, 35]  # Delays
            },
            "bass": {
                "primary": [2, 7],  # Compressor, EQ
                "secondary": [15],  # Tube
                "character": [55]  # Mono Maker
            },
            "drums": {
                "primary": [3, 2],  # Transient, Compressor
                "secondary": [4, 7],  # Gate, EQ
                "character": [39, 43]  # Reverbs
            },
            "synth": {
                "primary": [9, 10],  # Filters
                "secondary": [23, 24],  # Chorus
                "character": [26, 27]  # Ring Mod, Freq Shift
            },
            "piano": {
                "primary": [1, 7],  # Opto, EQ
                "secondary": [39, 41],  # Reverbs
                "character": [15]  # Tube warmth
            }
        }
    
    def apply_nudges(self, base_preset: Dict[str, Any], prompt: str, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Apply intelligent nudges based on prompt and blueprint
        """
        logger.info(f"Applying nudges for prompt: '{prompt}'")
        nudged = base_preset.copy()
        prompt_lower = prompt.lower()
        
        # Step 0: Preserve Cloud AI specified engines
        cloud_engines_preserved = 0
        for slot in range(1, 7):
            cloud_engine = blueprint.get(f"slot{slot}_engine", 0)
            if cloud_engine > 0:
                # If Cloud AI specified an engine for this slot, preserve it
                if nudged.get(f"slot{slot}_engine", 0) != cloud_engine:
                    # Find the engine in another slot or add it
                    found_in_other_slot = False
                    for other_slot in range(1, 7):
                        if nudged.get(f"slot{other_slot}_engine", 0) == cloud_engine:
                            found_in_other_slot = True
                            break
                    
                    if not found_in_other_slot:
                        # Add the Cloud AI engine to an empty slot
                        for empty_slot in range(1, 7):
                            if nudged.get(f"slot{empty_slot}_engine", 0) == 0:
                                nudged[f"slot{empty_slot}_engine"] = cloud_engine
                                logger.info(f"Preserved Cloud AI engine {cloud_engine} in slot {empty_slot}")
                                cloud_engines_preserved += 1
                                break
        
        # Step 1: Suggest and add missing engines
        nudged = self._suggest_and_add_engines(nudged, prompt_lower)
        
        # Step 2: Apply parameter adjustments based on intent
        nudged = self._apply_parameter_adjustments(nudged, prompt_lower)
        
        # Step 3: Apply genre-specific adjustments
        nudged = self._apply_genre_adjustments(nudged, prompt_lower)
        
        # Step 4: Apply relationship-based adjustments
        nudged = self._apply_parameter_relationships(nudged)
        
        # Step 5: Let signal intelligence suggest improvements
        suggestions = self.signal_intelligence.suggest_improvements(nudged, prompt)
        if suggestions:
            logger.info(f"Signal intelligence suggestions: {suggestions}")
        
        return nudged
    
    def suggest_engines_for_intent(self, prompt: str) -> List[int]:
        """
        Suggest which engines to use based on prompt intent
        """
        prompt_lower = prompt.lower()
        suggested = []
        
        # Check for instrument-specific needs
        for instrument, engines in self.engine_suggestions.items():
            if instrument in prompt_lower:
                suggested.extend(engines["primary"])
                if "space" in prompt_lower or "ambient" in prompt_lower:
                    suggested.extend(engines["secondary"])
                if "character" in prompt_lower or "color" in prompt_lower:
                    suggested.extend(engines["character"])
        
        # Check for effect-specific keywords
        effect_keywords = {
            "compress": [1, 2, 5],
            "distort": [15, 16, 18, 20, 21, 22],
            "reverb": [39, 40, 41, 42, 43],
            "delay": [34, 35, 36, 37],
            "modulate": [23, 24, 25, 26, 27],
            "filter": [9, 10, 11, 12],
            "eq": [7, 8, 6],
            "widen": [44, 45, 46],
            "gate": [4, 48],
            # Specific engine mappings for better matching
            "tube": [15],  # Vintage Tube Preamp
            "plate reverb": [39],  # Plate Reverb
            "shimmer": [42],  # Shimmer Reverb  
            "pitch shift": [48],  # Pitch Shifter
            "bit crusher": [18],  # Bit Crusher
            "chorus": [24],  # Classic Chorus
            "phaser": [25],  # Analog Phaser
            "vocoder": [51],  # Vocoder
            "harmonizer": [45],  # Intelligent Harmonizer
            "k-style": [22],  # K-Style Overdrive
            "k style": [22],  # K-Style Overdrive
            "tape echo": [34],  # Tape Echo
            "spring reverb": [40],  # Spring Reverb
            "parametric eq": [7]  # Parametric EQ
        }
        
        for keyword, engines in effect_keywords.items():
            if keyword in prompt_lower:
                suggested.extend(engines[:2])  # Take top 2 from each category
        
        # Check for character keywords
        character_keywords = {
            "warm": [1, 15, 39],
            "aggressive": [22, 21, 20, 4],
            "ambient": [42, 35, 46],
            "clean": [2, 7, 54],
            "vintage": [1, 15, 34, 40],
            "modern": [5, 6, 41],
            "bright": [17, 7],
            "dark": [9, 10],
            "spacious": [42, 44, 46],
            "tight": [4, 3, 2],
            "loose": [1, 39, 34]
        }
        
        for keyword, engines in character_keywords.items():
            if keyword in prompt_lower:
                suggested.extend(engines)
        
        # Remove duplicates while preserving order
        seen = set()
        unique = []
        for engine in suggested:
            if engine not in seen and engine != 0:
                seen.add(engine)
                unique.append(engine)
        
        # Limit to 6 slots, prioritize by order
        return unique[:6]
    
    def _suggest_and_add_engines(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:
        """
        Suggest and add missing engines based on intent
        """
        suggested = self.suggest_engines_for_intent(prompt)
        
        if not suggested:
            return preset
        
        # Get current engines
        current_engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                current_engines.append((slot, engine_id))
        
        # Find empty slots
        empty_slots = [i for i in range(1, 7) if preset.get(f"slot{i}_engine", 0) == 0]
        
        # Add suggested engines to empty slots
        for engine_id in suggested:
            if not empty_slots:
                break
            
            # Don't add duplicates
            if engine_id in [e[1] for e in current_engines]:
                continue
            
            slot = empty_slots.pop(0)
            preset[f"slot{slot}_engine"] = engine_id
            
            # Add default parameters
            if engine_id in ENGINE_DEFAULTS:
                for param_idx, value in enumerate(ENGINE_DEFAULTS[engine_id]):
                    preset[f"slot{slot}_param{param_idx}"] = value
            
            logger.info(f"Added {ENGINE_NAMES.get(engine_id, 'Unknown')} to slot {slot}")
        
        return preset
    
    def _apply_parameter_adjustments(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:
        """
        Apply intelligent parameter adjustments based on intent
        """
        # Detect intents in prompt
        active_intents = []
        for intent in self.parameter_intelligence.keys():
            if intent in prompt:
                active_intents.append(intent)
        
        # Apply adjustments for each intent
        for intent in active_intents:
            adjustments = self.parameter_intelligence[intent]["adjustments"]
            
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                
                if engine_id in adjustments:
                    for param, value in adjustments[engine_id].items():
                        param_key = f"slot{slot}_{param}"
                        old_value = preset.get(param_key, 0.5)
                        preset[param_key] = value
                        try:
                            logger.info(f"Adjusted {param_key} from {float(old_value):.2f} to {float(value):.2f} for {intent}")
                        except (ValueError, TypeError):
                            logger.info(f"Adjusted {param_key} from {old_value} to {value} for {intent}")
        
        return preset
    
    def _apply_genre_adjustments(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:
        """
        Apply genre-specific adjustments
        """
        genre_settings = {
            "pop": {
                "compression": 0.4,
                "brightness": 0.6,
                "reverb_size": 0.3
            },
            "rock": {
                "drive": 0.7,
                "mids": 0.4,
                "room_size": 0.5
            },
            "metal": {
                "gate_threshold": 0.3,
                "drive": 0.9,
                "low_mids": 0.2  # Scooped
            },
            "jazz": {
                "compression": 0.2,
                "warmth": 0.6,
                "reverb": 0.3
            },
            "electronic": {
                "filter_res": 0.6,
                "delay_sync": True,
                "sidechain": 0.7
            }
        }
        
        for genre, settings in genre_settings.items():
            if genre in prompt.lower():
                logger.info(f"Applying {genre} genre settings")
                
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    
                    # Apply genre-specific settings
                    if engine_id in [1, 2] and "compression" in settings:
                        preset[f"slot{slot}_param0"] = settings["compression"]
                    
                    if engine_id in range(15, 23) and "drive" in settings:
                        preset[f"slot{slot}_param0"] = settings["drive"]
                    
                    if engine_id in [39, 40, 41, 42, 43] and "reverb_size" in settings:
                        preset[f"slot{slot}_param0"] = settings.get("reverb_size", 0.5)
        
        return preset
    
    def _apply_parameter_relationships(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Apply parameter relationships (e.g., if drive increases, output decreases)
        """
        # Helper to safely get numeric value
        def get_param_value(p, key, default=0):
            try:
                return float(p.get(key, default))
            except (TypeError, ValueError):
                return default
        
        relationships = [
            # If high drive, reduce output
            {"condition": lambda p, s: get_param_value(p, f"slot{s}_param0") > 0.7,
             "engines": range(15, 23),
             "action": lambda p, s: p.update({f"slot{s}_param7": 0.5})},
            
            # If low cutoff, reduce resonance
            {"condition": lambda p, s: get_param_value(p, f"slot{s}_param0") < 0.3,
             "engines": [9, 10],
             "action": lambda p, s: p.update({f"slot{s}_param1": min(get_param_value(p, f"slot{s}_param1"), 0.6)})},
            
            # If high feedback, ensure mix is reasonable
            {"condition": lambda p, s: get_param_value(p, f"slot{s}_param1") > 0.7,
             "engines": range(34, 38),
             "action": lambda p, s: p.update({f"slot{s}_param2": min(get_param_value(p, f"slot{s}_param2"), 0.5)})}
        ]
        
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            
            for relationship in relationships:
                if engine_id in relationship["engines"]:
                    if relationship["condition"](preset, slot):
                        relationship["action"](preset, slot)
                        logger.info(f"Applied parameter relationship for slot {slot}")
        
        return preset
    
    def _load_nudge_rules(self) -> Dict[str, Any]:
        """
        Load nudge rules from file or create defaults
        """
        try:
            if Path(self.rules_path).exists():
                with open(self.rules_path, 'r') as f:
                    return json.load(f)
        except Exception as e:
            logger.warning(f"Could not load nudge rules: {e}")
        
        # Return default rules
        return {
            "version": "2.0",
            "rules": [],
            "learned_patterns": {}
        }
    
    def learn_from_adjustment(self, original: Dict, adjusted: Dict, prompt: str):
        """
        Learn from user adjustments to improve future suggestions
        """
        # This would be implemented to track user preferences
        # and improve the system over time
        pass