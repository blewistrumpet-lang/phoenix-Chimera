import json
import logging
from typing import Dict, Any, List
from pathlib import Path

logger = logging.getLogger(__name__)

class Calculator:
    """
    The Calculator applies intelligent parameter nudges based on the user's prompt
    and the Visionary's blueprint.
    """
    
    def __init__(self, rules_path: str = "nudge_rules.json"):
        self.rules_path = Path(rules_path)
        self.rules = self._load_rules()
    
    def _load_rules(self) -> Dict[str, Any]:
        """Load nudge rules from JSON file"""
        try:
            if self.rules_path.exists():
                with open(self.rules_path, 'r') as f:
                    return json.load(f)
            else:
                # Return default rules if file doesn't exist
                return self._get_default_rules()
        except Exception as e:
            logger.error(f"Error loading rules: {str(e)}")
            return self._get_default_rules()
    
    def _get_default_rules(self) -> Dict[str, Any]:
        """Default nudge rules for MVP"""
        return {
            "keyword_rules": {
                # Mood/vibe keywords
                "dark": {
                    "slot1_param2": -0.2,  # Reduce tone/brightness
                    "slot2_param1": 0.1    # Increase reverb size
                },
                "bright": {
                    "slot1_param2": 0.2,   # Increase tone/brightness
                    "slot2_param2": -0.1   # Reduce damping
                },
                "aggressive": {
                    "slot1_param1": 0.3,   # Increase drive
                    "slot1_param3": 0.1    # Increase level
                },
                "subtle": {
                    "slot1_param1": -0.2,  # Reduce drive
                    "slot2_param3": -0.1   # Reduce mix
                },
                "warm": {
                    "slot1_param1": 0.1,   # Slight drive increase
                    "slot1_param2": -0.1   # Slight tone reduction
                },
                "vintage": {
                    "slot2_param4": 0.2,   # Increase wow
                    "slot2_param5": 0.1    # Increase flutter
                },
                "modern": {
                    "slot2_param4": -0.1,  # Reduce wow
                    "slot2_param5": -0.1   # Reduce flutter
                },
                "spacious": {
                    "slot2_param1": 0.2,   # Increase size/time
                    "slot2_param3": 0.1    # Increase mix
                },
                "tight": {
                    "slot2_param1": -0.2,  # Reduce size/time
                    "slot2_param2": 0.1    # Increase damping
                }
            },
            "engine_specific_rules": {
                # K-Style Overdrive (engine_id: 0)
                "0": {
                    "default_adjustments": {
                        "slot1_param1": 0.0,  # Drive baseline
                        "slot1_param2": 0.0,  # Tone baseline
                        "slot1_param3": 0.0   # Level baseline
                    }
                },
                # Tape Echo (engine_id: 1)
                "1": {
                    "default_adjustments": {
                        "slot2_param1": 0.0,  # Time baseline
                        "slot2_param2": 0.0,  # Feedback baseline
                        "slot2_param3": 0.0   # Mix baseline
                    }
                },
                # Plate Reverb (engine_id: 2)
                "2": {
                    "default_adjustments": {
                        "slot2_param1": 0.0,  # Size baseline
                        "slot2_param2": 0.0,  # Damping baseline
                        "slot2_param3": 0.0   # Mix baseline
                    }
                }
            }
        }
    
    def apply_nudges(self, preset: Dict[str, Any], prompt: str, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Apply parameter nudges to the preset based on prompt analysis
        """
        try:
            nudged_preset = preset.copy()
            prompt_lower = prompt.lower()
            
            # Track which parameters have been nudged
            nudged_params = set()
            
            # Apply keyword-based nudges
            for keyword, adjustments in self.rules.get("keyword_rules", {}).items():
                if keyword in prompt_lower:
                    for param_name, adjustment in adjustments.items():
                        if param_name in nudged_preset["parameters"]:
                            current_value = nudged_preset["parameters"][param_name]
                            new_value = self._clamp(current_value + adjustment, 0.0, 1.0)
                            nudged_preset["parameters"][param_name] = new_value
                            nudged_params.add(param_name)
                            logger.info(f"Applied nudge for '{keyword}': {param_name} {current_value:.2f} -> {new_value:.2f}")
            
            # Apply engine-specific adjustments based on blueprint
            for slot_info in blueprint.get("slots", []):
                slot_num = slot_info.get("slot", 1)
                engine_id = slot_info.get("engine_id", -1)
                character = slot_info.get("character", "").lower()
                
                if engine_id >= 0:
                    # Apply character-based nudges
                    if character == "warm":
                        self._apply_character_nudge(nudged_preset, slot_num, "warm", nudged_params)
                    elif character == "aggressive":
                        self._apply_character_nudge(nudged_preset, slot_num, "aggressive", nudged_params)
                    elif character == "spacious":
                        self._apply_character_nudge(nudged_preset, slot_num, "spacious", nudged_params)
            
            # Add metadata
            nudged_preset["calculator_nudges"] = list(nudged_params)
            
            return nudged_preset
            
        except Exception as e:
            logger.error(f"Error in apply_nudges: {str(e)}")
            return preset
    
    def _apply_character_nudge(self, preset: Dict[str, Any], slot: int, character: str, nudged_params: set):
        """Apply character-specific nudges to a slot"""
        character_nudges = {
            "warm": {
                f"slot{slot}_param1": 0.1,   # Slight parameter increases
                f"slot{slot}_param2": -0.05
            },
            "aggressive": {
                f"slot{slot}_param1": 0.2,
                f"slot{slot}_param3": 0.1
            },
            "spacious": {
                f"slot{slot}_param1": 0.15,
                f"slot{slot}_param3": 0.1
            }
        }
        
        if character in character_nudges:
            for param_name, adjustment in character_nudges[character].items():
                if param_name in preset["parameters"] and param_name not in nudged_params:
                    current_value = preset["parameters"][param_name]
                    new_value = self._clamp(current_value + adjustment, 0.0, 1.0)
                    preset["parameters"][param_name] = new_value
                    nudged_params.add(param_name)
    
    def _clamp(self, value: float, min_val: float, max_val: float) -> float:
        """Clamp a value between min and max"""
        return max(min_val, min(value, max_val))
    
    def save_rules(self):
        """Save current rules to file"""
        try:
            with open(self.rules_path, 'w') as f:
                json.dump(self.rules, f, indent=2)
        except Exception as e:
            logger.error(f"Error saving rules: {str(e)}")