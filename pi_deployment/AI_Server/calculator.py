"""
Calculator - Sophisticated multi-layered nudge system for the Trinity Pipeline
Applies intelligent parameter adjustments based on creative analysis
"""

import json
import logging
import math
from typing import Dict, Any, List, Tuple, Optional
from pathlib import Path
from engine_mapping_authoritative import *

logger = logging.getLogger(__name__)

class Calculator:
    """
    The Calculator applies sophisticated, multi-layered parameter nudges
    based on the user's prompt, Visionary's blueprint, and creative analysis.

    Implements a three-tier nudging system:
    1. Parameter Roles - Understanding what each parameter does
    2. Context Modifiers - Adjustments based on prompt analysis
    3. Engine-Specific Rules - Fine-tuned per-engine adjustments
    """

    def __init__(self, rules_path: str = "nudge_rules.json"):
        self.rules_path = Path(rules_path)
        self.rules = self._load_rules()

        # Load COMPLETE engine knowledge - ALL 57 engines with full parameter specs
        self.engine_knowledge = self._load_engine_knowledge()
        logger.info(f"✅ Calculator loaded complete knowledge for {len(self.engine_knowledge)} engines")

        # Parameter role definitions for intelligent nudging
        self.parameter_roles = self._initialize_parameter_roles()
    
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

    def _load_engine_knowledge(self) -> Dict[str, Any]:
        """Load COMPLETE engine knowledge - all 57 engines with full parameter specifications"""
        try:
            # Try complete knowledge first
            knowledge_path = Path("trinity_engine_knowledge_COMPLETE.json")
            if knowledge_path.exists():
                with open(knowledge_path, 'r') as f:
                    full_knowledge = json.load(f)
                    # Return engines dict keyed by string IDs
                    return full_knowledge.get("engines", {})

            # Fallback to regular knowledge
            knowledge_path = Path("trinity_engine_knowledge.json")
            if knowledge_path.exists():
                with open(knowledge_path, 'r') as f:
                    full_knowledge = json.load(f)
                    return full_knowledge.get("engines", {})

            logger.warning("No engine knowledge file found - calculator will have limited understanding")
            return {}

        except Exception as e:
            logger.error(f"Error loading engine knowledge: {e}")
            return {}
    
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
    
    def _initialize_parameter_roles(self) -> Dict[str, Dict[str, str]]:
        """
        Initialize parameter role definitions for intelligent nudging.
        Maps parameter types to their functional roles.
        """
        return {
            "intensity": ["drive", "gain", "amount", "depth", "strength"],
            "tone": ["frequency", "tone", "brightness", "color", "eq"],
            "time": ["delay", "time", "rate", "speed", "tempo"],
            "space": ["size", "room", "decay", "reverb", "width"],
            "dynamics": ["threshold", "ratio", "attack", "release", "compression"],
            "modulation": ["depth", "rate", "phase", "feedback", "mix"],
            "character": ["warmth", "saturation", "harmonics", "texture", "quality"]
        }
    
    def apply_nudges(self, preset: Dict[str, Any], prompt: str, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Apply sophisticated multi-layered parameter nudges.
        Compatible with the existing engine/parameter mapping system.
        """
        try:
            # Import engine defaults for compatibility
            from engine_defaults import ENGINE_DEFAULTS
            
            nudged_preset = preset.copy()
            prompt_lower = prompt.lower()
            creative_analysis = blueprint.get("creative_analysis", {})
            
            # Track nudge operations for transparency
            nudge_log = {
                "applied_nudges": [],
                "total_adjustments": 0,
                "affected_parameters": set(),
                "engine_emphasis": []
            }
            
            # Layer 0: ENGINE-SPECIFIC PROMPT NUDGING (NEW)
            # Apply strong nudges when specific engines are mentioned in the prompt
            self._apply_engine_prompt_nudges(
                nudged_preset, prompt_lower, blueprint, nudge_log
            )
            
            # Layer 1: Creative Analysis Nudges
            self._apply_creative_analysis_nudges(
                nudged_preset, creative_analysis, nudge_log
            )
            
            # Layer 2: Keyword-Based Contextual Nudges
            self._apply_contextual_nudges(
                nudged_preset, prompt_lower, nudge_log
            )
            
            # Layer 3: Engine-Specific Intelligent Nudges
            for slot_info in blueprint.get("slots", []):
                slot_num = slot_info.get("slot", 1)
                engine_id = slot_info.get("engine_id", ENGINE_NONE)
                character = slot_info.get("character", "").lower()
                intensity = slot_info.get("intensity", 0.5)
                
                if engine_id != ENGINE_NONE and validate_engine_id(engine_id):
                    # Apply engine-aware nudges
                    self._apply_engine_specific_nudges(
                        nudged_preset, slot_num, engine_id, 
                        character, intensity, nudge_log
                    )
            
            # Layer 4: Harmonic Balancing
            self._apply_harmonic_balancing(nudged_preset, nudge_log)
            
            # Layer 5: Intelligent Utility Engine Addition (NEW)
            logger.info("Starting utility engine analysis...")
            self._analyze_and_add_utility_engines(
                nudged_preset, prompt_lower, blueprint, nudge_log
            )
            
            # Add detailed metadata
            nudged_preset["calculator_metadata"] = {
                "nudge_log": nudge_log,
                "total_adjustments": nudge_log["total_adjustments"],
                "affected_parameters": list(nudge_log["affected_parameters"]),
                "creative_influence": creative_analysis,
                "engine_emphasis": nudge_log.get("engine_emphasis", [])
            }
            
            logger.info(f"Applied {nudge_log['total_adjustments']} nudges to {len(nudge_log['affected_parameters'])} parameters")
            if nudge_log.get("engine_emphasis"):
                logger.info(f"Emphasized engines from prompt: {', '.join(nudge_log['engine_emphasis'])}")
            
            return nudged_preset
            
        except Exception as e:
            logger.error(f"Error in apply_nudges: {str(e)}")
            return preset
    
    def _apply_creative_analysis_nudges(self, preset: Dict[str, Any], 
                                       creative_analysis: Dict[str, Any], 
                                       nudge_log: Dict[str, Any]):
        """
        Apply nudges based on Visionary's creative analysis.
        Uses mood, intensity, space, and character metrics.
        """
        mood = creative_analysis.get("mood", "neutral")
        intensity = creative_analysis.get("intensity", 0.5)
        space = creative_analysis.get("space", 0.5)
        character = creative_analysis.get("character", "balanced")
        
        # Mood-based adjustments
        mood_adjustments = {
            "warm": {"tone_shift": -0.1, "harmonic_boost": 0.1},
            "aggressive": {"drive_boost": 0.2, "compression": 0.15},
            "dark": {"high_cut": -0.2, "low_boost": 0.1},
            "bright": {"high_boost": 0.15, "presence": 0.1},
            "ethereal": {"reverb_increase": 0.2, "delay_feedback": 0.1}
        }
        
        if mood in mood_adjustments:
            for param_type, adjustment in mood_adjustments[mood].items():
                self._apply_typed_nudge(preset, param_type, adjustment, nudge_log)
        
        # Intensity-based scaling
        if intensity > 0.7:
            self._scale_intensity_parameters(preset, intensity, nudge_log)
        elif intensity < 0.3:
            self._reduce_intensity_parameters(preset, intensity, nudge_log)
        
        # Space-based adjustments
        if space > 0.7:
            self._expand_spatial_parameters(preset, space, nudge_log)
        elif space < 0.3:
            self._tighten_spatial_parameters(preset, space, nudge_log)
    
    def _apply_contextual_nudges(self, preset: Dict[str, Any], 
                                prompt_lower: str, 
                                nudge_log: Dict[str, Any]):
        """
        Apply context-based nudges from prompt keywords.
        Uses sophisticated keyword analysis and weighting.
        """
        # Extended keyword rules with weights
        contextual_rules = {
            "vintage": {
                "weight": 1.0,
                "nudges": {"warmth": 0.15, "saturation": 0.1, "high_rolloff": -0.1}
            },
            "modern": {
                "weight": 1.0,
                "nudges": {"clarity": 0.15, "precision": 0.1, "digital_character": 0.1}
            },
            "aggressive": {
                "weight": 1.2,
                "nudges": {"drive": 0.25, "attack": 0.15, "presence": 0.2}
            },
            "subtle": {
                "weight": 0.8,
                "nudges": {"mix": -0.2, "intensity": -0.15, "transparency": 0.1}
            },
            "spacious": {
                "weight": 1.0,
                "nudges": {"reverb": 0.2, "width": 0.15, "depth": 0.1}
            },
            "tight": {
                "weight": 1.0,
                "nudges": {"gate": 0.15, "damping": 0.1, "focus": 0.2}
            },
            "warm": {
                "weight": 0.9,
                "nudges": {"low_mids": 0.1, "harmonics": 0.15, "smoothness": 0.1}
            },
            "bright": {
                "weight": 0.9,
                "nudges": {"highs": 0.15, "air": 0.1, "sparkle": 0.1}
            },
            "punchy": {
                "weight": 1.1,
                "nudges": {"transient": 0.2, "attack": 0.15, "impact": 0.15}
            },
            "smooth": {
                "weight": 0.9,
                "nudges": {"compression": 0.1, "saturation": 0.05, "glue": 0.1}
            }
        }
        
        # Apply weighted nudges based on found keywords
        for keyword, rule_data in contextual_rules.items():
            if keyword in prompt_lower:
                weight = rule_data["weight"]
                for nudge_type, base_amount in rule_data["nudges"].items():
                    adjusted_amount = base_amount * weight
                    self._apply_typed_nudge(preset, nudge_type, adjusted_amount, nudge_log)
                    
                nudge_log["applied_nudges"].append({
                    "type": "contextual",
                    "keyword": keyword,
                    "weight": weight
                })
    
    def _apply_engine_specific_nudges(self, preset: Dict[str, Any], 
                                     slot_num: int, engine_id: int,
                                     character: str, intensity: float,
                                     nudge_log: Dict[str, Any]):
        """
        Apply engine-specific intelligent nudges.
        Uses ENGINE_DEFAULTS for parameter understanding.
        """
        from engine_defaults import ENGINE_DEFAULTS
        
        if engine_id not in ENGINE_DEFAULTS:
            return
        
        engine_info = ENGINE_DEFAULTS[engine_id]
        engine_name = engine_info.get("name", "Unknown")
        
        # Apply character-based nudges for this specific engine
        for param_num in range(1, 11):
            param_key = f"param{param_num}"
            if param_key in engine_info.get("params", {}):
                param_info = engine_info["params"][param_key]
                param_name = param_info.get("name", "").lower()
                param_slot_key = f"slot{slot_num}_param{param_num}"
                
                if param_slot_key not in preset["parameters"]:
                    continue
                
                current_value = preset["parameters"][param_slot_key]
                nudge_amount = 0.0
                
                # Intelligent nudging based on parameter role and character
                if "drive" in param_name or "gain" in param_name:
                    if character == "aggressive":
                        nudge_amount = 0.15 * intensity
                    elif character == "warm":
                        nudge_amount = 0.08 * intensity
                    elif character == "clean":
                        nudge_amount = -0.1 * intensity
                
                elif "tone" in param_name or "freq" in param_name:
                    if character == "bright":
                        nudge_amount = 0.12 * intensity
                    elif character == "warm":
                        nudge_amount = -0.08 * intensity
                    elif character == "vintage":
                        nudge_amount = -0.05 * intensity
                
                elif "mix" in param_name:
                    if character == "subtle":
                        nudge_amount = -0.15 * intensity
                    elif character == "prominent":
                        nudge_amount = 0.1 * intensity
                
                elif "feedback" in param_name:
                    if character == "spacious":
                        nudge_amount = 0.1 * intensity
                    elif character == "tight":
                        nudge_amount = -0.15 * intensity
                
                elif "time" in param_name or "delay" in param_name:
                    if character == "rhythmic":
                        nudge_amount = 0.0  # Keep as-is for rhythmic
                    elif character == "ambient":
                        nudge_amount = 0.2 * intensity
                
                # Apply the nudge
                if abs(nudge_amount) > 0.001:
                    new_value = self._clamp(current_value + nudge_amount, 0.0, 1.0)
                    preset["parameters"][param_slot_key] = new_value
                    
                    nudge_log["affected_parameters"].add(param_slot_key)
                    nudge_log["total_adjustments"] += 1
                    nudge_log["applied_nudges"].append({
                        "parameter": param_slot_key,
                        "engine": engine_name,
                        "character": character,
                        "adjustment": nudge_amount,
                        "old_value": current_value,
                        "new_value": new_value
                    })
    
    def _apply_engine_prompt_nudges(self, preset: Dict[str, Any], prompt_lower: str, 
                                   blueprint: Dict[str, Any], nudge_log: Dict[str, Any]):
        """
        Apply strong nudges when specific engines are mentioned in the prompt.
        This ensures requested engines are prominently featured.
        """
        from engine_defaults import ENGINE_DEFAULTS
        
        # Detect engine mentions in the prompt using authoritative mapping
        mentioned_engines = set()
        engine_keywords = []
        
        # Check for engine names using authoritative constants
        for engine_id, engine_name in ENGINE_NAMES.items():
            if engine_id == ENGINE_NONE:
                continue
            engine_name_lower = engine_name.lower()
            if engine_name_lower in prompt_lower:
                mentioned_engines.add(engine_id)
                engine_keywords.append(engine_name_lower)
                logger.info(f"Detected engine '{engine_name}' in prompt -> Engine {engine_id}")
        
        # Check for specific effect keywords that map to engines using authoritative constants
        effect_keywords = {
            "chaos": ENGINE_CHAOS_GENERATOR, "chaotic": ENGINE_CHAOS_GENERATOR, "random": ENGINE_CHAOS_GENERATOR,
            "spectral": ENGINE_SPECTRAL_FREEZE, "freeze": ENGINE_SPECTRAL_FREEZE, "frozen": ENGINE_SPECTRAL_FREEZE,
            "granular": ENGINE_GRANULAR_CLOUD, "grains": ENGINE_GRANULAR_CLOUD, "clouds": ENGINE_GRANULAR_CLOUD,
            "gated": ENGINE_GATED_REVERB, "gate": ENGINE_GATED_REVERB,
            "shimmer": ENGINE_SHIMMER_REVERB, "shimmering": ENGINE_SHIMMER_REVERB,
            "plate": ENGINE_PLATE_REVERB, "room": ENGINE_PLATE_REVERB,
            "spring": ENGINE_SPRING_REVERB, "metallic reverb": ENGINE_SPRING_REVERB,
            "bitcrush": ENGINE_BIT_CRUSHER, "8-bit": ENGINE_BIT_CRUSHER, "lofi": ENGINE_BIT_CRUSHER, "crushed": ENGINE_BIT_CRUSHER,
            "harmonizer": ENGINE_INTELLIGENT_HARMONIZER, "harmonize": ENGINE_INTELLIGENT_HARMONIZER, "harmony": ENGINE_INTELLIGENT_HARMONIZER,
            "vocoder": ENGINE_PHASED_VOCODER, "robot": ENGINE_PHASED_VOCODER, "vocoded": ENGINE_PHASED_VOCODER,
            "formant": ENGINE_FORMANT_FILTER, "vocal filter": ENGINE_VOCAL_FORMANT,
            "ring mod": ENGINE_RING_MODULATOR, "ring modulator": ENGINE_RING_MODULATOR,
            "pitch shift": ENGINE_PITCH_SHIFTER, "pitch": ENGINE_PITCH_SHIFTER,
            "convolution": ENGINE_CONVOLUTION_REVERB, "impulse": ENGINE_CONVOLUTION_REVERB
        }
        
        for keyword, engine_id in effect_keywords.items():
            if keyword in prompt_lower and engine_id not in mentioned_engines:
                mentioned_engines.add(engine_id)
                engine_keywords.append(keyword)
                logger.info(f"Detected effect keyword '{keyword}' in prompt -> Engine {engine_id}: {get_engine_name(engine_id)}")
        
        if not mentioned_engines:
            return  # No specific engines mentioned
        
        # Apply strong nudges for mentioned engines
        nudge_log["engine_emphasis"] = [f"{get_engine_name(eid)} ({eid})" for eid in mentioned_engines]
        
        # Find slots with mentioned engines and boost their presence
        for slot_num in range(1, 7):
            engine_param = f"slot{slot_num}_engine"
            if engine_param in preset["parameters"]:
                current_engine = preset["parameters"][engine_param]
                
                if current_engine in mentioned_engines:
                    # This slot has a mentioned engine - make it prominent!
                    logger.info(f"Slot {slot_num} has requested engine {current_engine} - boosting presence")
                    
                    # Ensure it's not bypassed
                    bypass_param = f"slot{slot_num}_bypass"
                    if bypass_param in preset["parameters"]:
                        preset["parameters"][bypass_param] = 0.0
                        nudge_log["affected_parameters"].add(bypass_param)
                    
                    # Boost mix level significantly
                    mix_param = f"slot{slot_num}_mix"
                    if mix_param in preset["parameters"]:
                        current_mix = preset["parameters"][mix_param]
                        # Ensure at least 70% mix for requested engines
                        boosted_mix = max(current_mix, 0.7)
                        if boosted_mix != current_mix:
                            preset["parameters"][mix_param] = boosted_mix
                            nudge_log["affected_parameters"].add(mix_param)
                            nudge_log["applied_nudges"].append({
                                "parameter": mix_param,
                                "reason": f"Engine {get_engine_name(current_engine)} mentioned in prompt",
                                "adjustment": boosted_mix - current_mix,
                                "old_value": current_mix,
                                "new_value": boosted_mix
                            })
                            nudge_log["total_adjustments"] += 1
                    
                    # Apply characteristic parameter boosts based on the engine type
                    if current_engine in ENGINE_DEFAULTS:
                        engine_info = ENGINE_DEFAULTS[current_engine]
                        engine_name = engine_info.get("name", "Unknown")
                        
                        # Boost key parameters for this engine type using authoritative constants
                        if current_engine == ENGINE_CHAOS_GENERATOR:
                            # Chaos Generator - boost randomness parameters
                            for param_num in [1, 2, 3]:  # Typically chaos, rate, depth
                                param_key = f"slot{slot_num}_param{param_num}"
                                if param_key in preset["parameters"]:
                                    current = preset["parameters"][param_key]
                                    boosted = min(current + 0.2, 1.0)
                                    if boosted != current:
                                        preset["parameters"][param_key] = boosted
                                        nudge_log["affected_parameters"].add(param_key)
                                        nudge_log["total_adjustments"] += 1
                        
                        elif current_engine == ENGINE_SPECTRAL_FREEZE:
                            # Spectral Freeze - boost freeze amount
                            param_key = f"slot{slot_num}_param1"  # Freeze amount
                            if param_key in preset["parameters"]:
                                current = preset["parameters"][param_key]
                                boosted = min(current + 0.3, 1.0)
                                preset["parameters"][param_key] = boosted
                                nudge_log["affected_parameters"].add(param_key)
                                nudge_log["total_adjustments"] += 1
                        
                        elif current_engine == ENGINE_GATED_REVERB:
                            # Gated Reverb - ensure strong gating
                            param_key = f"slot{slot_num}_param2"  # Gate threshold
                            if param_key in preset["parameters"]:
                                preset["parameters"][param_key] = max(preset["parameters"][param_key], 0.6)
                                nudge_log["affected_parameters"].add(param_key)
                                nudge_log["total_adjustments"] += 1
    
    def _apply_harmonic_balancing(self, preset: Dict[str, Any], nudge_log: Dict[str, Any]):
        """
        Final pass to ensure harmonic balance across all slots.
        Prevents frequency masking and ensures coherent mix.
        """
        # Analyze frequency distribution across active slots
        frequency_distribution = self._analyze_frequency_distribution(preset)
        
        # Apply corrective nudges if needed
        if frequency_distribution.get("low_heavy", False):
            # Reduce low frequencies in later slots
            for slot in range(4, 7):
                for param in range(1, 11):
                    param_key = f"slot{slot}_param{param}"
                    if param_key in preset["parameters"]:
                        # Subtle high-pass effect
                        if "2" in param_key or "3" in param_key:  # Typically tone/frequency params
                            preset["parameters"][param_key] = min(
                                preset["parameters"][param_key] + 0.05, 1.0
                            )
        
        elif frequency_distribution.get("high_heavy", False):
            # Add warmth to balance brightness
            for slot in range(1, 4):
                for param in range(1, 11):
                    param_key = f"slot{slot}_param{param}"
                    if param_key in preset["parameters"]:
                        if "2" in param_key:  # Typically tone params
                            preset["parameters"][param_key] = max(
                                preset["parameters"][param_key] - 0.05, 0.0
                            )
    
    def _apply_typed_nudge(self, preset: Dict[str, Any], nudge_type: str, 
                          amount: float, nudge_log: Dict[str, Any]):
        """
        Apply a nudge based on parameter type rather than specific parameter.
        This allows for more flexible and intelligent nudging.
        """
        # Map nudge types to likely parameter patterns
        type_patterns = {
            "drive": ["param1", "param3"],  # Common drive/gain positions
            "tone": ["param2"],  # Common tone position
            "mix": ["param3", "param10"],  # Common mix positions
            "feedback": ["param2", "param4"],  # Common feedback positions
            "reverb": ["param1", "param3"],  # Size and mix for reverbs
            "compression": ["param2", "param3"],  # Ratio and threshold
            "warmth": ["param2"],  # Tone/frequency
            "clarity": ["param2", "param8"],  # EQ and presence
            "width": ["param7", "param8"],  # Stereo width params
        }
        
        # Find and adjust matching parameters
        patterns = type_patterns.get(nudge_type, [])
        for slot in range(1, 7):
            for pattern in patterns:
                param_key = f"slot{slot}_{pattern}"
                if param_key in preset["parameters"]:
                    current = preset["parameters"][param_key]
                    new_value = self._clamp(current + amount, 0.0, 1.0)
                    if abs(new_value - current) > 0.001:
                        preset["parameters"][param_key] = new_value
                        nudge_log["affected_parameters"].add(param_key)
                        nudge_log["total_adjustments"] += 1
    
    def _scale_intensity_parameters(self, preset: Dict[str, Any], 
                                   intensity: float, nudge_log: Dict[str, Any]):
        """Scale intensity-related parameters based on creative analysis"""
        scale_factor = 0.5 + (intensity * 0.5)  # Scale between 0.5 and 1.0
        
        for param_key in preset["parameters"]:
            if "param1" in param_key or "param3" in param_key:  # Common intensity params
                current = preset["parameters"][param_key]
                scaled = current * scale_factor
                preset["parameters"][param_key] = self._clamp(scaled, 0.0, 1.0)
                nudge_log["affected_parameters"].add(param_key)
    
    def _reduce_intensity_parameters(self, preset: Dict[str, Any], 
                                    intensity: float, nudge_log: Dict[str, Any]):
        """Reduce intensity for subtle effects"""
        reduction_factor = 0.3 + (intensity * 0.7)  # Scale between 0.3 and 1.0
        
        for param_key in preset["parameters"]:
            if "param1" in param_key:  # Drive/intensity params
                current = preset["parameters"][param_key]
                reduced = current * reduction_factor
                preset["parameters"][param_key] = self._clamp(reduced, 0.0, 1.0)
                nudge_log["affected_parameters"].add(param_key)
    
    def _expand_spatial_parameters(self, preset: Dict[str, Any], 
                                  space: float, nudge_log: Dict[str, Any]):
        """Expand spatial parameters for wider sound"""
        expansion = (space - 0.5) * 0.3  # Max 0.15 expansion
        
        for param_key in preset["parameters"]:
            # Target reverb size, delay time, width params
            if any(x in param_key for x in ["param1", "param7", "param8"]):
                if "slot" in param_key:
                    slot_num = int(param_key[4])
                    if slot_num >= 5:  # Later slots for spatial effects
                        current = preset["parameters"][param_key]
                        expanded = current + expansion
                        preset["parameters"][param_key] = self._clamp(expanded, 0.0, 1.0)
                        nudge_log["affected_parameters"].add(param_key)
    
    def _tighten_spatial_parameters(self, preset: Dict[str, Any], 
                                   space: float, nudge_log: Dict[str, Any]):
        """Tighten spatial parameters for focused sound"""
        tightening = (0.5 - space) * 0.2  # Max 0.1 tightening
        
        for param_key in preset["parameters"]:
            if "param1" in param_key or "param3" in param_key:  # Size/mix params
                if "slot" in param_key:
                    slot_num = int(param_key[4])
                    if slot_num >= 5:  # Spatial effect slots
                        current = preset["parameters"][param_key]
                        tightened = current - tightening
                        preset["parameters"][param_key] = self._clamp(tightened, 0.0, 1.0)
                        nudge_log["affected_parameters"].add(param_key)
    
    def _analyze_frequency_distribution(self, preset: Dict[str, Any]) -> Dict[str, bool]:
        """Analyze the frequency distribution across all active slots"""
        low_accumulation = 0.0
        high_accumulation = 0.0
        
        for slot in range(1, 7):
            # Check if slot is active
            bypass_key = f"slot{slot}_bypass"
            if preset["parameters"].get(bypass_key, 1.0) > 0.5:
                continue  # Slot is bypassed
            
            # Analyze tone parameters
            tone_param = f"slot{slot}_param2"
            if tone_param in preset["parameters"]:
                tone_value = preset["parameters"][tone_param]
                if tone_value < 0.4:
                    low_accumulation += (0.4 - tone_value)
                elif tone_value > 0.6:
                    high_accumulation += (tone_value - 0.6)
        
        return {
            "low_heavy": low_accumulation > 0.5,
            "high_heavy": high_accumulation > 0.5,
            "balanced": abs(high_accumulation - low_accumulation) < 0.2
        }
    
    def _clamp(self, value: float, min_val: float, max_val: float) -> float:
        """Clamp a value between min and max"""
        return max(min_val, min(value, max_val))
    
    def apply_modification_nudges(self, preset: Dict[str, Any], modification_blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """
        Apply modification nudges based on Visionary's analysis.
        This is specifically for the /modify endpoint.
        
        Args:
            preset: Current preset to modify
            modification_blueprint: Analysis from Visionary with parameter targets
            
        Returns:
            Modified preset with changes applied
        """
        modified_preset = preset.copy()
        parameters = modified_preset.get("parameters", {})
        
        # Log the blueprint for debugging if needed
        # print(f"DEBUG Calculator: Blueprint received: {modification_blueprint}")
        
        # Track changes
        change_log = {
            "applied_changes": [],
            "total_adjustments": 0,
            "affected_parameters": set(),
            "intent": modification_blueprint.get("intent", "modification")
        }
        
        # Apply mood shift
        mood_shift = modification_blueprint.get("mood_shift", "")
        if mood_shift:
            self._apply_mood_modification(parameters, mood_shift, change_log)
        
        # Apply parameter targets from Visionary
        targets = modification_blueprint.get("parameter_targets", {})
        for target_type, adjustment in targets.items():
            self._apply_targeted_modification(parameters, target_type, adjustment, change_log)
        
        # Apply intensity change
        intensity_change = modification_blueprint.get("intensity_change", 0.0)
        if abs(intensity_change) > 0.01:
            self._apply_intensity_modification(parameters, intensity_change, change_log)
        
        # Handle engine suggestions
        engine_suggestions = modification_blueprint.get("engine_suggestions", {})
        # print(f"DEBUG Calculator: Engine suggestions: {engine_suggestions}")
        if engine_suggestions.get("add"):
            # print(f"DEBUG Calculator: Adding engines: {engine_suggestions['add']}")
            self._suggest_engine_additions(parameters, engine_suggestions["add"], change_log)
        if engine_suggestions.get("remove"):
            # print(f"DEBUG Calculator: Removing engines: {engine_suggestions['remove']}")
            self._suggest_engine_removals(parameters, engine_suggestions["remove"], change_log)
        
        # IMPORTANT: Update the parameters back to the preset
        modified_preset["parameters"] = parameters
        
        # Add modification metadata
        modified_preset["modification_metadata"] = {
            "blueprint": modification_blueprint,
            "change_log": change_log,
            "total_changes": change_log["total_adjustments"],
            "affected_parameters": list(change_log["affected_parameters"])
        }
        
        logger.info(f"Applied modification: {modification_blueprint.get('intent', 'unknown')}")
        logger.info(f"Total changes: {change_log['total_adjustments']} parameters affected")
        
        return modified_preset
    
    def _apply_mood_modification(self, parameters: Dict[str, Any], mood: str, change_log: Dict):
        """Apply parameter changes for mood shifts"""
        mood_mappings = {
            "darker": {
                "tone_params": -0.2,  # params 2, 3
                "brightness_params": -0.25,  # params 4, 5
                "filter_params": -0.15  # params 6, 7
            },
            "brighter": {
                "tone_params": 0.2,
                "brightness_params": 0.25,
                "filter_params": 0.15
            },
            "warmer": {
                "saturation_params": 0.15,  # params 1, 7
                "tone_params": -0.1,
                "harmonic_params": 0.1  # params 8, 9
            },
            "colder": {
                "saturation_params": -0.15,
                "tone_params": 0.1,
                "digital_params": 0.15
            },
            "ethereal": {
                "reverb_params": 0.2,
                "delay_params": 0.15,
                "modulation_params": 0.1
            },
            "grounded": {
                "reverb_params": -0.2,
                "compression_params": 0.15,
                "attack_params": -0.1
            }
        }
        
        if mood in mood_mappings:
            adjustments = mood_mappings[mood]
            
            for slot in range(1, 7):
                engine_id = parameters.get(f"slot{slot}_engine", 0)
                if engine_id > 0 and parameters.get(f"slot{slot}_bypass", 0) < 0.5:
                    # Apply mood-based adjustments to relevant parameters
                    for adjustment_type, amount in adjustments.items():
                        param_indices = self._get_mood_param_indices(adjustment_type)
                        for param_idx in param_indices:
                            param_key = f"slot{slot}_param{param_idx}"
                            if param_key in parameters:
                                old_value = parameters[param_key]
                                new_value = self._clamp(old_value + amount, 0.0, 1.0)
                                if abs(new_value - old_value) > 0.01:
                                    parameters[param_key] = new_value
                                    change_log["affected_parameters"].add(param_key)
                                    change_log["total_adjustments"] += 1
                                    change_log["applied_changes"].append({
                                        "parameter": param_key,
                                        "reason": f"mood shift: {mood}",
                                        "adjustment": amount,
                                        "old_value": old_value,
                                        "new_value": new_value
                                    })
    
    def _apply_targeted_modification(self, parameters: Dict[str, Any], target_type: str, adjustment: float, change_log: Dict):
        """Apply targeted parameter modifications"""
        # Map target types to parameter patterns
        target_mappings = {
            "brightness": [2, 3, 4],  # Tone and brightness params
            "highs": [3, 4, 5],  # High frequency params
            "lows": [1, 6, 7],  # Low frequency params
            "warmth": [1, 7, 8],  # Warmth and saturation
            "saturation": [1, 7, 9],  # Saturation and harmonics
            "drive": [1, 2],  # Drive and gain
            "compression": [2, 3, 4],  # Compression params
            "reverb": [1, 2, 15],  # Reverb size, decay, mix
            "delay": [1, 2, 15],  # Delay time, feedback, mix
            "space": [1, 2, 8],  # Spatial parameters
            "attack": [3, 4],  # Attack times
            "release": [4, 5],  # Release times
            "filter": [1, 2],  # Filter frequency and resonance
            "feedback": [2, 3],  # Feedback parameters
            "mix": [15],  # Mix level (usually last param)
        }
        
        if target_type in target_mappings:
            param_indices = target_mappings[target_type]
            
            for slot in range(1, 7):
                engine_id = parameters.get(f"slot{slot}_engine", 0)
                if engine_id > 0 and parameters.get(f"slot{slot}_bypass", 0) < 0.5:
                    # Check if this engine type is relevant for the target
                    if self._engine_has_target_capability(engine_id, target_type):
                        for param_idx in param_indices:
                            param_key = f"slot{slot}_param{param_idx}"
                            if param_key in parameters:
                                old_value = parameters[param_key]
                                new_value = self._clamp(old_value + adjustment, 0.0, 1.0)
                                if abs(new_value - old_value) > 0.01:
                                    parameters[param_key] = new_value
                                    change_log["affected_parameters"].add(param_key)
                                    change_log["total_adjustments"] += 1
                                    change_log["applied_changes"].append({
                                        "parameter": param_key,
                                        "reason": f"target: {target_type}",
                                        "adjustment": adjustment,
                                        "old_value": old_value,
                                        "new_value": new_value
                                    })
    
    def _apply_intensity_modification(self, parameters: Dict[str, Any], intensity_change: float, change_log: Dict):
        """Apply intensity modifications across relevant parameters"""
        # Intensity affects drive, compression, mix levels
        intensity_params = [1, 2, 15]  # Drive, amount, mix
        
        for slot in range(1, 7):
            engine_id = parameters.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and parameters.get(f"slot{slot}_bypass", 0) < 0.5:
                for param_idx in intensity_params:
                    param_key = f"slot{slot}_param{param_idx}"
                    if param_key in parameters:
                        old_value = parameters[param_key]
                        # Scale adjustment based on current value
                        scaled_adjustment = intensity_change * (0.5 if param_idx == 15 else 1.0)
                        new_value = self._clamp(old_value + scaled_adjustment, 0.0, 1.0)
                        if abs(new_value - old_value) > 0.01:
                            parameters[param_key] = new_value
                            change_log["affected_parameters"].add(param_key)
                            change_log["total_adjustments"] += 1
    
    def _suggest_engine_additions(self, parameters: Dict[str, Any], engines_to_add: List[str], change_log: Dict):
        """Actually add engines to available slots"""
        from engine_defaults import ENGINE_DEFAULTS
        
        # print(f"DEBUG _suggest_engine_additions: Called with engines: {engines_to_add}")
        
        for engine_name in engines_to_add:
            # Get engine ID from name using authoritative mapping
            engine_name_lower = engine_name.lower()
            engine_id = get_engine_id(engine_name_lower)
            
            if engine_id == ENGINE_NONE:
                # Try to find partial match in ENGINE_NAMES
                for eid, name in ENGINE_NAMES.items():
                    if engine_name_lower in name.lower() or name.lower() in engine_name_lower:
                        engine_id = eid
                        break
            
            if engine_id != ENGINE_NONE:
                # print(f"DEBUG: Found engine ID {engine_id} for {engine_name}")
                # Find an available slot (preferably empty, otherwise least important)
                best_slot = self._find_best_slot_for_engine(parameters, engine_id)
                # print(f"DEBUG: Best slot for engine: {best_slot}")
                
                if best_slot:
                    # Add the engine
                    # print(f"DEBUG: Adding {engine_name} (ID {engine_id}) to slot {best_slot}")
                    parameters[f"slot{best_slot}_engine"] = engine_id
                    parameters[f"slot{best_slot}_bypass"] = 0.0  # Active
                    parameters[f"slot{best_slot}_mix"] = 0.3  # Reasonable starting mix
                    
                    # Set default parameters for this engine
                    if engine_id in ENGINE_DEFAULTS:
                        defaults = ENGINE_DEFAULTS[engine_id].get("defaults", {})
                        for param_num in range(1, 16):
                            param_key = f"slot{best_slot}_param{param_num}"
                            default_key = f"param{param_num}"
                            if default_key in defaults:
                                parameters[param_key] = defaults[default_key]
                            else:
                                parameters[param_key] = 0.5  # Default middle value
                    
                    change_log["affected_parameters"].add(f"slot{best_slot}_engine")
                    change_log["total_adjustments"] += 1
                    change_log["applied_changes"].append({
                        "type": "engine_addition",
                        "action": "added",
                        "engine": get_engine_name(engine_id),
                        "slot": best_slot,
                        "reason": "requested in modification"
                    })
                    
                    logger.info(f"Added {get_engine_name(engine_id)} to slot {best_slot}")
                else:
                    # No available slot, just log as suggestion
                    change_log["applied_changes"].append({
                        "type": "engine_suggestion",
                        "action": "add",
                        "engine": engine_name,
                        "reason": "no available slot"
                    })
    
    def _suggest_engine_removals(self, parameters: Dict[str, Any], engines_to_remove: List[str], change_log: Dict):
        """Actually remove or bypass engines"""
        
        for engine_name in engines_to_remove:
            # Find and bypass matching engines
            engine_name_lower = engine_name.lower()
            
            for slot in range(1, 7):
                engine_id = parameters.get(f"slot{slot}_engine", ENGINE_NONE)
                if engine_id != ENGINE_NONE:
                    current_engine_name = get_engine_name(engine_id).lower()
                    
                    if engine_name_lower in current_engine_name or current_engine_name in engine_name_lower:
                        # Bypass this engine
                        parameters[f"slot{slot}_bypass"] = 1.0
                        parameters[f"slot{slot}_mix"] = 0.0
                        
                        change_log["affected_parameters"].add(f"slot{slot}_bypass")
                        change_log["total_adjustments"] += 1
                        change_log["applied_changes"].append({
                            "type": "engine_removal",
                            "action": "bypassed",
                            "engine": get_engine_name(engine_id),
                            "slot": slot,
                            "reason": "requested in modification"
                        })
                        
                        logger.info(f"Bypassed {get_engine_name(engine_id)} in slot {slot}")
                        break
    
    def _find_best_slot_for_engine(self, parameters: Dict[str, Any], engine_id: int) -> int:
        """Find the best slot to place a new engine"""
        # First, look for empty slots
        for slot in range(1, 7):
            current_engine = parameters.get(f"slot{slot}_engine", ENGINE_NONE)
            if current_engine == ENGINE_NONE:
                return slot
        
        # Next, look for bypassed slots
        for slot in range(1, 7):
            if parameters.get(f"slot{slot}_bypass", 0) > 0.5:
                return slot
        
        # Finally, look for slots with low mix that we can replace
        # Sort slots by mix level and replace the quietest one
        slot_mixes = []
        for slot in range(1, 7):
            mix = parameters.get(f"slot{slot}_mix", 0.5)
            slot_mixes.append((slot, mix))
        
        slot_mixes.sort(key=lambda x: x[1])  # Sort by mix level
        
        # Return the slot with lowest mix (least important)
        if slot_mixes:
            return slot_mixes[0][0]
        
        return None  # No suitable slot found
    
    def _get_mood_param_indices(self, adjustment_type: str) -> List[int]:
        """Get parameter indices for mood-based adjustments"""
        mappings = {
            "tone_params": [2, 3],
            "brightness_params": [4, 5],
            "filter_params": [6, 7],
            "saturation_params": [1, 7],
            "harmonic_params": [8, 9],
            "digital_params": [10, 11],
            "reverb_params": [1, 2, 15],
            "delay_params": [1, 2, 15],
            "modulation_params": [1, 2, 3],
            "compression_params": [2, 3, 4],
            "attack_params": [3, 4]
        }
        return mappings.get(adjustment_type, [])
    
    def _engine_has_target_capability(self, engine_id: int, target_type: str) -> bool:
        """Check if an engine can handle a specific target type"""
        # Import engine defaults to understand capabilities
        try:
            from engine_defaults import ENGINE_DEFAULTS
            if engine_id not in ENGINE_DEFAULTS:
                return True  # Default to true if unknown
            
            engine_info = ENGINE_DEFAULTS[engine_id]
            engine_name = engine_info.get("name", "").lower()
            
            # Map target types to engine capabilities
            if target_type in ["reverb", "space"] and "reverb" in engine_name:
                return True
            elif target_type in ["delay", "echo", "feedback"] and ("delay" in engine_name or "echo" in engine_name):
                return True
            elif target_type in ["drive", "saturation", "warmth"] and any(x in engine_name for x in ["distortion", "overdrive", "saturation", "tube"]):
                return True
            elif target_type in ["compression", "attack", "release"] and any(x in engine_name for x in ["compressor", "limiter", "dynamics"]):
                return True
            elif target_type in ["filter", "highs", "lows", "brightness"] and any(x in engine_name for x in ["filter", "eq", "equalizer"]):
                return True
            
            # Default: allow modification for generic targets
            return target_type in ["mix", "intensity"]
            
        except ImportError:
            return True  # Default to true if can't import
    
    def _analyze_and_add_utility_engines(self, preset: Dict[str, Any], prompt_lower: str, 
                                        blueprint: Dict[str, Any], nudge_log: Dict[str, Any]):
        """
        Analyze the preset and prompt to intelligently add utility engines when needed.
        
        Adds:
        - Mid-Side Processor (53) for stereo/width issues
        - Gain Utility (54) for level management
        - Mono Maker (55) for mono compatibility (bass/sub)
        - Phase Align (56) for phase correction
        """
        parameters = preset.get("parameters", {})
        
        # Analyze current engine mix and identify potential issues
        analysis = self._analyze_preset_for_utility_needs(preset, prompt_lower, blueprint)
        
        # Add utility engines based on analysis
        utility_engines_to_add = []
        
        if analysis["needs_stereo_processing"]:
            if self._add_utility_engine(parameters, ENGINE_MID_SIDE_PROCESSOR, "stereo width enhancement", nudge_log):
                utility_engines_to_add.append("Mid-Side Processor")
        
        if analysis["needs_level_management"]:
            if self._add_utility_engine(parameters, ENGINE_GAIN_UTILITY, "level management", nudge_log):
                utility_engines_to_add.append("Gain Utility")
        
        if analysis["needs_mono_compatibility"]:
            if self._add_utility_engine(parameters, ENGINE_MONO_MAKER, "mono compatibility for low frequencies", nudge_log):
                utility_engines_to_add.append("Mono Maker")
        
        if analysis["needs_phase_correction"]:
            if self._add_utility_engine(parameters, ENGINE_PHASE_ALIGN, "phase alignment", nudge_log):
                utility_engines_to_add.append("Phase Align")
        
        nudge_log["utility_engines_added"] = utility_engines_to_add
        
        if utility_engines_to_add:
            logger.info(f"Calculator added utility engines: {', '.join(utility_engines_to_add)}")
            # Also store at the preset level for easier access
            if "calculator_utility_engines" not in preset:
                preset["calculator_utility_engines"] = []
            preset["calculator_utility_engines"].extend(utility_engines_to_add)
    
    def _analyze_preset_for_utility_needs(self, preset: Dict[str, Any], prompt_lower: str, 
                                         blueprint: Dict[str, Any]) -> Dict[str, bool]:
        """
        Analyze preset and prompt to determine what utility engines are needed.
        """
        parameters = preset.get("parameters", {})
        creative_analysis = blueprint.get("creative_analysis", {})
        
        # Initialize analysis results
        analysis = {
            "needs_stereo_processing": False,
            "needs_level_management": False,
            "needs_mono_compatibility": False,
            "needs_phase_correction": False
        }
        
        # Analyze prompt for utility keywords
        stereo_keywords = ["stereo", "width", "wide", "spacious", "panoramic", "spread", "imaging"]
        level_keywords = ["loud", "quiet", "gain", "level", "volume", "boost", "cut", "balance"]
        mono_keywords = ["bass", "sub", "low", "bottom", "foundation", "mono", "center", "focused"]
        phase_keywords = ["phase", "alignment", "timing", "sync", "coherent", "polarity"]
        
        # Check for stereo processing needs
        if any(keyword in prompt_lower for keyword in stereo_keywords):
            analysis["needs_stereo_processing"] = True
        
        # Check spatial characteristics from creative analysis
        space_level = creative_analysis.get("space", 0.5)
        if space_level > 0.7:  # High spatial requirements
            analysis["needs_stereo_processing"] = True
        
        # Check for level management needs
        if any(keyword in prompt_lower for keyword in level_keywords):
            analysis["needs_level_management"] = True
        
        # Check intensity from creative analysis
        intensity = creative_analysis.get("intensity", 0.5)
        if intensity > 0.8 or intensity < 0.2:  # Extreme intensity levels
            analysis["needs_level_management"] = True
        
        # Check for mono compatibility needs (especially for bass-heavy content)
        if any(keyword in prompt_lower for keyword in mono_keywords):
            analysis["needs_mono_compatibility"] = True
        
        # Check for low-frequency engines that might benefit from mono processing
        has_bass_engines = self._has_low_frequency_engines(parameters)
        if has_bass_engines:
            analysis["needs_mono_compatibility"] = True
        
        # Check for phase correction needs
        if any(keyword in prompt_lower for keyword in phase_keywords):
            analysis["needs_phase_correction"] = True
        
        # Check for multiple delay/reverb engines that might cause phase issues
        delay_reverb_count = self._count_delay_reverb_engines(parameters)
        if delay_reverb_count >= 2:
            analysis["needs_phase_correction"] = True
        
        return analysis
    
    def _has_low_frequency_engines(self, parameters: Dict[str, Any]) -> bool:
        """
        Check if the preset has engines that emphasize low frequencies.
        """
        low_freq_engines = [
            ENGINE_VINTAGE_TUBE,  # Often used for bass warmth
            ENGINE_MULTIBAND_SATURATOR,
            ENGINE_OPTO_COMPRESSOR,
            ENGINE_VCA_COMPRESSOR,
            # Add other engines known for low-frequency emphasis
        ]
        
        for slot in range(1, 7):
            engine_id = parameters.get(f"slot{slot}_engine", ENGINE_NONE)
            if engine_id in low_freq_engines and parameters.get(f"slot{slot}_bypass", 1.0) < 0.5:
                return True
        
        return False
    
    def _count_delay_reverb_engines(self, parameters: Dict[str, Any]) -> int:
        """
        Count active delay and reverb engines that might cause phase issues.
        """
        count = 0
        
        for slot in range(1, 7):
            engine_id = parameters.get(f"slot{slot}_engine", ENGINE_NONE)
            if parameters.get(f"slot{slot}_bypass", 1.0) < 0.5:  # Active slot
                if engine_id in DELAY_REVERB_ENGINES:
                    count += 1
        
        return count
    
    def _add_utility_engine(self, parameters: Dict[str, Any], utility_engine_id: int, 
                           reason: str, nudge_log: Dict[str, Any]) -> bool:
        """
        Add a utility engine to an available slot.
        Returns True if successfully added, False if no slot available.
        """
        from engine_defaults import ENGINE_DEFAULTS
        
        # Find an empty slot (prefer later slots for utilities)
        available_slot = None
        for slot in range(6, 0, -1):  # Check slots 6 to 1
            engine_param = f"slot{slot}_engine"
            if parameters.get(engine_param, ENGINE_NONE) == ENGINE_NONE:
                available_slot = slot
                break
        
        if not available_slot:
            # No empty slots available
            return False
        
        # Add the utility engine
        parameters[f"slot{available_slot}_engine"] = utility_engine_id
        parameters[f"slot{available_slot}_bypass"] = 0.0  # Active
        parameters[f"slot{available_slot}_mix"] = 0.3  # Conservative mix level
        
        # Set appropriate default parameters for this utility engine
        if utility_engine_id in ENGINE_DEFAULTS:
            defaults = ENGINE_DEFAULTS[utility_engine_id].get("params", {})
            for param_num in range(1, 11):
                param_key = f"slot{available_slot}_param{param_num}"
                default_key = f"param{param_num}"
                if default_key in defaults:
                    parameters[param_key] = defaults[default_key].get("default", 0.5)
                else:
                    parameters[param_key] = 0.5  # Default middle value
        
        # Log the addition
        engine_name = get_engine_name(utility_engine_id)
        nudge_log["affected_parameters"].add(f"slot{available_slot}_engine")
        nudge_log["total_adjustments"] += 1
        nudge_log["applied_nudges"].append({
            "type": "utility_engine_addition",
            "engine": engine_name,
            "slot": available_slot,
            "reason": reason
        })
        
        logger.info(f"Added {engine_name} to slot {available_slot} for {reason}")
        return True
    
    def save_rules(self):
        """Save current rules to file"""
        try:
            with open(self.rules_path, 'w') as f:
                json.dump(self.rules, f, indent=2)
        except Exception as e:
            logger.error(f"Error saving rules: {str(e)}")