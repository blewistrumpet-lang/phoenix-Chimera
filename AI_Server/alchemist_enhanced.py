"""
Enhanced Alchemist with Signal Chain Intelligence and Deep Safety Validation
This is the PRODUCTION version with all features integrated
"""

import logging
import random
from typing import Dict, Any, Tuple, List
from engine_defaults import ENGINE_DEFAULTS
from engine_mapping_authoritative import ENGINE_NAMES
from signal_chain_intelligence import SignalChainIntelligence
from engine_knowledge_base import ENGINE_KNOWLEDGE

logger = logging.getLogger(__name__)

class AlchemistEnhanced:
    """
    The Alchemist performs final validation, safety checks, signal chain optimization,
    and parameter clamping to ensure the preset is valid and sounds good.
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
        
        # Creative name components
        self.name_components = {
            "prefixes": {
                "warm": ["Warm", "Cozy", "Vintage", "Analog"],
                "aggressive": ["Brutal", "Savage", "Crushing", "Raging"],
                "ambient": ["Ethereal", "Floating", "Celestial", "Dreamy"],
                "clean": ["Crystal", "Pure", "Pristine", "Clear"],
                "experimental": ["Quantum", "Neural", "Fractal", "Chaos"]
            },
            "suffixes": {
                "reverb": ["Space", "Chamber", "Hall", "Cathedral"],
                "distortion": ["Crusher", "Shredder", "Destroyer", "Grinder"],
                "modulation": ["Swirl", "Wave", "Pulse", "Vortex"],
                "delay": ["Echo", "Repeat", "Cascade", "Infinity"]
            }
        }
    
    def finalize_preset(self, preset: Dict[str, Any], prompt: str = "") -> Dict[str, Any]:
        """
        Perform final validation, optimization, and safety checks on the preset
        """
        try:
            logger.info(f"Finalizing preset for prompt: '{prompt}'")
            finalized = preset.copy()
            
            # Step 1: Optimize signal chain ordering FIRST
            logger.info("Step 1: Optimizing signal chain...")
            finalized = self.signal_intelligence.optimize_signal_chain(finalized)
            
            # Step 2: Validate parameters with signal chain intelligence
            logger.info("Step 2: Validating parameters...")
            is_safe, warnings = self.signal_intelligence.validate_parameters(finalized)
            
            if not is_safe:
                logger.warning(f"Safety issues detected: {warnings}")
                finalized = self._apply_safety_corrections(finalized, warnings)
            
            # Step 3: Apply musical intelligence based on prompt
            logger.info("Step 3: Applying musical intelligence...")
            finalized = self._apply_musical_intelligence(finalized, prompt)
            
            # Step 4: Ensure complete structure
            logger.info("Step 4: Ensuring complete structure...")
            finalized = self._ensure_complete_structure(finalized)
            
            # Step 5: Final safety validation
            logger.info("Step 5: Final safety validation...")
            finalized = self._final_safety_check(finalized)
            
            # Step 6: Generate intelligent name
            logger.info("Step 6: Generating intelligent name...")
            finalized["name"] = self._generate_intelligent_name(finalized, prompt)
            
            # Step 7: Add signal flow explanation
            logger.info("Step 7: Adding signal flow explanation...")
            finalized["signal_flow"] = self.signal_intelligence.explain_chain(finalized)
            
            # Step 8: Add metadata
            finalized["alchemist_validated"] = True
            finalized["signal_chain_optimized"] = True
            finalized["safety_validated"] = is_safe
            finalized["warnings"] = warnings if warnings else []
            finalized["musical_intelligence_applied"] = True
            
            logger.info(f"✅ Preset finalized: {finalized['name']}")
            logger.info(f"   Signal flow: {finalized['signal_flow']}")
            
            return finalized
            
        except Exception as e:
            logger.error(f"Error in finalize_preset: {str(e)}")
            return self._create_safe_default(prompt)
    
    def _apply_musical_intelligence(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:
        """
        Apply music theory knowledge to enhance the preset
        """
        prompt_lower = prompt.lower()
        
        # Detect musical intent
        for intent, knowledge in self.music_theory.items():
            if intent in prompt_lower:
                logger.info(f"Applying {intent} musical intelligence")
                
                # Adjust parameters based on intent
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id == 0:
                        continue
                    
                    # Apply intelligent parameter adjustments
                    if engine_id == 15 and "warm" in prompt_lower:  # Vintage Tube
                        preset[f"slot{slot}_param1"] = 0.4  # Moderate drive
                        preset[f"slot{slot}_param2"] = 0.6  # Warm bias
                    
                    elif engine_id == 39 and "ambient" in prompt_lower:  # Plate Reverb
                        preset[f"slot{slot}_param0"] = 0.8  # Large size
                        preset[f"slot{slot}_param1"] = 0.7  # Long decay
                        preset[f"slot{slot}_param5"] = 0.4  # Moderate mix
                    
                    elif engine_id in [20, 21, 22] and "aggressive" in prompt_lower:  # Distortions
                        preset[f"slot{slot}_param0"] = 0.8  # High drive
                        preset[f"slot{slot}_param2"] = 0.7  # Aggressive tone
                    
                    elif engine_id == 42 and "ethereal" in prompt_lower:  # Shimmer
                        preset[f"slot{slot}_param2"] = 0.7  # High shimmer
                        preset[f"slot{slot}_param3"] = 0.6  # Pitch up
        
        return preset
    
    def _apply_safety_corrections(self, preset: Dict[str, Any], warnings: List[str]) -> Dict[str, Any]:
        """
        Correct safety issues identified by signal chain intelligence
        """
        for warning in warnings:
            if "feedback" in warning.lower():
                # Reduce all feedback parameters
                for slot in range(1, 7):
                    for param in range(16):
                        key = f"slot{slot}_param{param}"
                        if key in preset and "feedback" in str(param):
                            preset[key] = min(preset[key], 0.75)
            
            elif "resonance" in warning.lower():
                # Reduce resonance parameters
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id in [9, 10]:  # Filters
                        preset[f"slot{slot}_param1"] = min(preset.get(f"slot{slot}_param1", 0), 0.6)
            
            elif "clipping" in warning.lower() or "drive" in warning.lower():
                # Reduce drive/gain parameters
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id in range(15, 23):  # Distortion engines
                        preset[f"slot{slot}_param0"] = min(preset.get(f"slot{slot}_param0", 0), 0.6)
                        preset[f"slot{slot}_param1"] = min(preset.get(f"slot{slot}_param1", 0), 0.6)
        
        return preset
    
    def _final_safety_check(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Final pass to ensure all parameters are safe
        """
        # Check total gain staging
        total_gain = 0
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id in [15, 16, 17, 18, 19, 20, 21, 22]:  # Distortion engines
                try:
                    param0 = float(preset.get(f"slot{slot}_param0", 0))
                    param1 = float(preset.get(f"slot{slot}_param1", 0))
                    drive = param0 + param1
                    total_gain += drive
                except (TypeError, ValueError):
                    # Skip if parameters can't be converted to float
                    pass
        
        # If too much gain, scale back
        if total_gain > self.parameter_safety["gain_staging"]["max_total"]:
            scale = self.parameter_safety["gain_staging"]["max_total"] / total_gain
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id in range(15, 23):
                    try:
                        if f"slot{slot}_param0" in preset:
                            preset[f"slot{slot}_param0"] = float(preset[f"slot{slot}_param0"]) * scale
                        if f"slot{slot}_param1" in preset:
                            preset[f"slot{slot}_param1"] = float(preset[f"slot{slot}_param1"]) * scale
                    except (TypeError, ValueError):
                        pass
        
        # Ensure mix levels are reasonable
        for slot in range(1, 7):
            mix_key = f"slot{slot}_mix"
            if mix_key in preset and preset[mix_key] > 0.8:
                preset[mix_key] = 0.8  # Cap at 80% mix
        
        # Ensure master levels are safe
        preset["master_input"] = min(preset.get("master_input", 0.7), 0.9)
        preset["master_output"] = min(preset.get("master_output", 0.7), 0.9)
        
        return preset
    
    def _ensure_complete_structure(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Ensure all required preset fields exist
        """
        # Ensure all slots have engine definitions
        for slot in range(1, 7):
            if f"slot{slot}_engine" not in preset:
                preset[f"slot{slot}_engine"] = 0
            
            # If engine exists, ensure it has default parameters
            engine_id = preset[f"slot{slot}_engine"]
            if engine_id > 0 and engine_id in ENGINE_DEFAULTS:
                defaults = ENGINE_DEFAULTS[engine_id]
                for param_idx, default_value in enumerate(defaults):
                    param_key = f"slot{slot}_param{param_idx}"
                    if param_key not in preset:
                        preset[param_key] = default_value
        
        # Ensure master parameters
        preset.setdefault("master_input", 0.7)
        preset.setdefault("master_output", 0.7)
        preset.setdefault("master_mix", 1.0)
        
        return preset
    
    def _generate_intelligent_name(self, preset: Dict[str, Any], prompt: str) -> str:
        """
        Generate a creative, relevant name based on the preset and prompt
        """
        prompt_lower = prompt.lower()
        
        # Analyze the preset's character
        engines_used = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and engine_id in ENGINE_KNOWLEDGE:
                engines_used.append(ENGINE_KNOWLEDGE[engine_id]["name"])
        
        # Determine prefix based on vibe
        prefix = "Sonic"
        if "warm" in prompt_lower or "vintage" in prompt_lower:
            prefix = random.choice(self.name_components["prefixes"]["warm"])
        elif "aggressive" in prompt_lower or "metal" in prompt_lower:
            prefix = random.choice(self.name_components["prefixes"]["aggressive"])
        elif "ambient" in prompt_lower or "space" in prompt_lower:
            prefix = random.choice(self.name_components["prefixes"]["ambient"])
        elif "clean" in prompt_lower:
            prefix = random.choice(self.name_components["prefixes"]["clean"])
        
        # Determine suffix based on main effect
        suffix = "Engine"
        if any("Reverb" in eng for eng in engines_used):
            suffix = random.choice(self.name_components["suffixes"]["reverb"])
        elif any("Distortion" in eng or "Overdrive" in eng for eng in engines_used):
            suffix = random.choice(self.name_components["suffixes"]["distortion"])
        elif any("Chorus" in eng or "Phaser" in eng for eng in engines_used):
            suffix = random.choice(self.name_components["suffixes"]["modulation"])
        elif any("Delay" in eng or "Echo" in eng for eng in engines_used):
            suffix = random.choice(self.name_components["suffixes"]["delay"])
        
        return f"{prefix} {suffix}"
    
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
            "signal_flow": "Compression → EQ → Reverb",
            "alchemist_validated": True,
            "warnings": ["Fallback preset used"]
        }