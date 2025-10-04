#!/usr/bin/env python3
"""
Complete Calculator Component with Optimization Intelligence
Optimizes presets for musical coherence and technical correctness
"""

import json
import re
import logging
from typing import Dict, List, Any, Optional
import numpy as np

logger = logging.getLogger(__name__)

class CompleteCalculator:
    def __init__(self):
        """Initialize with complete engine knowledge"""
        # Load the COMPLETE knowledge base - SINGLE SOURCE OF TRUTH
        try:
            with open("trinity_engine_knowledge_COMPLETE.json", "r") as f:
                self.knowledge = json.load(f)
            logger.info(f"✅ Calculator loaded COMPLETE knowledge for {len(self.knowledge['engines'])} engines")
        except FileNotFoundError:
            try:
                with open("trinity_engine_knowledge.json", "r") as f:
                    self.knowledge = json.load(f)
                logger.warning("⚠️ Using regular knowledge base - complete version preferred")
            except FileNotFoundError:
                logger.error("No knowledge base found!")
                raise
        
        self.engines = self.knowledge["engines"]
        self.avoid_combinations = self.knowledge["avoid_combinations"]
        
        # Musical time subdivisions for intelligent parsing
        self.time_subdivisions = {
            "whole": 1.0, "1": 1.0,
            "half": 0.5, "1/2": 0.5,
            "quarter": 0.25, "1/4": 0.25,
            "eighth": 0.125, "1/8": 0.125, "8th": 0.125,
            "sixteenth": 0.0625, "1/16": 0.0625, "16th": 0.0625,
            "dotted quarter": 0.375, "1/4 dotted": 0.375,
            "dotted eighth": 0.1875, "1/8 dotted": 0.1875, "dotted 8th": 0.1875,
            "triplet eighth": 0.0833, "1/8 triplet": 0.0833
        }
        
        # Parameter patterns for parsing
        self.param_patterns = {
            "percentage": re.compile(r'(\d+(?:\.\d+)?)\s*%\s*(\w+)?'),
            "ratio": re.compile(r'(\d+(?:\.\d+)?):(\d+(?:\.\d+)?)\s*(?:ratio)?'),
            "milliseconds": re.compile(r'(\d+(?:\.\d+)?)\s*ms'),
            "decibels": re.compile(r'([+-]?\d+(?:\.\d+)?)\s*dB'),
        }
        
    def parse_prompt_values(self, prompt: str) -> Dict[str, Any]:
        """Extract specific parameter values from user prompt"""
        extracted = {}
        prompt_lower = prompt.lower()
        
        # Check for time subdivisions
        for subdivision, value in self.time_subdivisions.items():
            if subdivision in prompt_lower:
                extracted["time_subdivision"] = {
                    "value": value,
                    "original": subdivision,
                    "type": "time"
                }
                logger.info(f"📐 Found time: {subdivision} = {value}")
        
        # Check for percentages
        for match in self.param_patterns["percentage"].finditer(prompt_lower):
            percent_val = float(match.group(1)) / 100.0
            param_hint = match.group(2) if match.group(2) else "mix"
            extracted[f"percentage_{param_hint}"] = {
                "value": percent_val,
                "original": f"{match.group(1)}%",
                "type": "percentage",
                "hint": param_hint
            }
            logger.info(f"📊 Found percentage: {match.group(1)}% {param_hint} = {percent_val}")
        
        # Check for ratios
        for match in self.param_patterns["ratio"].finditer(prompt_lower):
            numerator = float(match.group(1))
            denominator = float(match.group(2)) if match.group(2) else 1
            ratio_val = numerator / denominator if denominator > 0 else numerator
            # Map to 0-1 range
            normalized = min(1.0, ratio_val / 20.0)  # Assume max ratio of 20:1
            
            extracted["ratio"] = {
                "value": normalized,
                "original": f"{numerator}:{denominator}",
                "actual_ratio": ratio_val,
                "type": "ratio"
            }
            logger.info(f"🔢 Found ratio: {numerator}:{denominator} = {normalized}")
        
        return extracted
    
    def apply_intelligent_parameters(self, preset: Dict[str, Any], extracted_values: Dict[str, Any]) -> Dict[str, Any]:
        """Apply extracted values to appropriate parameters"""
        
        for slot in preset.get("slots", []):
            engine_id = slot.get("engine_id", 0)
            if engine_id == 0:
                continue
            
            engine_data = self.engines.get(str(engine_id), {})
            
            # Tape Echo special handling
            if engine_id == 34:
                if "time_subdivision" in extracted_values:
                    slot["parameters"][0]["value"] = extracted_values["time_subdivision"]["value"]
                    logger.info(f"  Set Tape Echo Time: {extracted_values['time_subdivision']['value']:.4f}")
                
                for key, val in extracted_values.items():
                    if "feedback" in key:
                        slot["parameters"][1]["value"] = val["value"]
                        logger.info(f"  Set Tape Echo Feedback: {val['value']:.4f}")
                    elif "mix" in key.lower() and "delay" in preset.get("name", "").lower():
                        slot["parameters"][4]["value"] = val["value"]
                        logger.info(f"  Set Tape Echo Mix: {val['value']:.4f}")
            
            # Compressor handling
            elif engine_id in [1, 2, 3, 4, 5]:
                if "ratio" in extracted_values:
                    slot["parameters"][1]["value"] = extracted_values["ratio"]["value"]
                    logger.info(f"  Set Compressor Ratio: {extracted_values['ratio']['value']:.4f}")
            
            # Shimmer Reverb
            elif engine_id == 42:
                for key, val in extracted_values.items():
                    if "mix" in key.lower() and "shimmer" in preset.get("name", "").lower():
                        slot["parameters"][0]["value"] = val["value"]
                        logger.info(f"  Set Shimmer Mix: {val['value']:.4f}")
            
            # Tube Preamp
            elif engine_id == 15:
                for key, val in extracted_values.items():
                    if "drive" in key:
                        slot["parameters"][0]["value"] = val["value"]
                        logger.info(f"  Set Tube Drive: {val['value']:.4f}")
            
            # Generic mix handling for any engine
            if "percentage_mix" in extracted_values:
                mix_idx = engine_data.get("mix_param_index", -1)
                if mix_idx >= 0 and mix_idx < len(slot.get("parameters", [])):
                    slot["parameters"][mix_idx]["value"] = extracted_values["percentage_mix"]["value"]
                    logger.info(f"  Set {engine_data.get('name', 'Engine')} Mix: {extracted_values['percentage_mix']['value']:.4f}")
        
        return preset
    
    def optimize_preset(self, preset: Dict[str, Any], user_prompt: str = "") -> Dict[str, Any]:
        """
        Optimize preset for musical coherence and technical correctness
        NOW WITH INTELLIGENT PARAMETER PARSING!
        """
        logger.info(f"Optimizing preset: {preset.get('name', 'Unnamed')}")
        
        # Parse the user prompt for specific values
        if user_prompt:
            logger.info(f"🧠 Parsing prompt for intelligent parameters: '{user_prompt}'")
            extracted_values = self.parse_prompt_values(user_prompt)
            
            if extracted_values:
                logger.info(f"✨ Applying {len(extracted_values)} intelligent parameter values")
                preset = self.apply_intelligent_parameters(preset, extracted_values)
        
        # Deep copy to avoid modifying original
        optimized = json.loads(json.dumps(preset))
        
        # 1. Optimize signal chain order
        optimized = self.optimize_signal_chain(optimized)
        
        # 2. Optimize mix levels for coherence
        optimized = self.optimize_mix_levels(optimized)
        
        # 3. Optimize gain staging
        optimized = self.optimize_gain_staging(optimized)
        
        # 4. Optimize frequency balance
        optimized = self.optimize_frequency_balance(optimized)
        
        # 5. Check and fix problematic combinations
        optimized = self.fix_problematic_combinations(optimized)
        
        # 6. Optimize dynamics relationships
        optimized = self.optimize_dynamics(optimized)
        
        logger.info(f"Optimization complete for {preset.get('name', 'Unnamed')}")
        return optimized
    
    def optimize_signal_chain(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Reorder slots for optimal signal flow"""
        slots = preset.get("slots", [])
        if not slots:
            return preset
        
        # Sort slots by recommended signal chain position
        def get_position(slot):
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            return engine.get("signal_chain_position", 5)
        
        # Sort and reassign slot indices
        sorted_slots = sorted(slots, key=get_position)
        for i, slot in enumerate(sorted_slots):
            slot["slot"] = i
        
        preset["slots"] = sorted_slots
        logger.debug(f"Reordered signal chain")
        
        # Ensure exactly 6 slots for plugin compatibility
        while len(preset["slots"]) < 6:
            preset["slots"].append({
                "slot": len(preset["slots"]),
                "engine_id": 0,
                "engine_name": "None",
                "parameters": []
            })
        
        return preset
    
    def optimize_mix_levels(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Optimize mix levels for coherent sound"""
        slots = preset.get("slots", [])
        
        # Track cumulative wetness
        cumulative_wet = 1.0
        
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            mix_index = engine.get("mix_param_index", -1)
            
            if mix_index >= 0 and mix_index < len(slot.get("parameters", [])):
                current_mix = slot["parameters"][mix_index]["value"]
                category = engine.get("category", "")
                
                # Adjust mix based on position and type
                if category == "Reverb":
                    # Reverbs should be more subtle in a chain
                    if cumulative_wet < 0.5:
                        # Already pretty wet, reduce reverb
                        optimal_mix = min(current_mix, 0.3)
                    else:
                        optimal_mix = min(current_mix, 0.5)
                
                elif category in ["Distortion", "Saturation"]:
                    # Distortion can be more present
                    optimal_mix = current_mix
                    
                elif category == "Dynamics":
                    # Compressors usually full wet
                    optimal_mix = 1.0
                    
                elif category in ["Delay", "Time"]:
                    # Delays need balance
                    optimal_mix = min(current_mix, 0.5)
                    
                else:
                    # Default: slightly reduce if chain is getting too wet
                    optimal_mix = current_mix * (0.8 if cumulative_wet < 0.7 else 1.0)
                
                # Apply optimized mix
                slot["parameters"][mix_index]["value"] = round(optimal_mix, 3)
                
                # Update cumulative wetness
                cumulative_wet *= (1.0 - optimal_mix * 0.2)  # Each effect reduces "dryness"
        
        return preset
    
    def optimize_gain_staging(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Ensure proper gain structure through the chain"""
        slots = preset.get("slots", [])
        
        # Track estimated gain through chain
        cumulative_gain = 1.0
        
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            category = engine.get("category", "")
            params = slot.get("parameters", [])
            
            # Estimate gain impact
            if category in ["Distortion", "Saturation"]:
                # These add gain - check drive/gain parameters
                if len(params) > 0:
                    drive_value = params[0].get("value", 0.5)  # Usually first param
                    cumulative_gain *= (1.0 + drive_value * 0.5)
                    
                    # If we're getting too hot, reduce drive
                    if cumulative_gain > 2.0:
                        params[0]["value"] = round(drive_value * 0.7, 3)
                        cumulative_gain *= 0.7
            
            elif category == "Dynamics":
                # Compressors reduce dynamic range but can add makeup gain
                if engine_id == 5:  # Mastering Limiter
                    # Limiter should be last and control final level
                    if len(params) > 0:
                        # Adjust threshold based on input level
                        threshold = 0.9 / cumulative_gain
                        params[0]["value"] = round(min(0.9, max(0.3, threshold)), 3)
                
                elif engine_id in [1, 2]:  # Compressors
                    # Moderate compression
                    cumulative_gain *= 0.9
            
            elif engine_id == 54:  # Gain Utility
                # Direct gain control - adjust based on current level
                if cumulative_gain > 1.5:
                    # Reduce gain
                    if len(params) > 0:
                        params[0]["value"] = round(0.7, 3)
                    cumulative_gain *= 0.7
        
        return preset
    
    def optimize_frequency_balance(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Balance frequency content across the chain"""
        slots = preset.get("slots", [])
        
        # Track frequency emphasis
        freq_emphasis = {
            "low": 1.0,
            "mid": 1.0, 
            "high": 1.0
        }
        
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            category = engine.get("category", "")
            params = slot.get("parameters", [])
            
            # Adjust EQ-type effects
            if engine_id == 7:  # Parametric EQ
                # Balance bands to avoid over-emphasis
                for i in range(0, min(9, len(params)), 3):
                    if i+1 < len(params):
                        # Every 3 params is a band (freq, gain, Q)
                        gain_param = params[i+1]
                        current_gain = gain_param.get("value", 0.5)
                        
                        # Reduce if frequencies are already emphasized
                        if current_gain > 0.5:  # Boost
                            if i < 3:  # Low band
                                gain_param["value"] = round(current_gain * freq_emphasis["low"], 3)
                                freq_emphasis["low"] *= 0.8
                            elif i < 6:  # Mid band
                                gain_param["value"] = round(current_gain * freq_emphasis["mid"], 3)
                                freq_emphasis["mid"] *= 0.8
                            else:  # High band
                                gain_param["value"] = round(current_gain * freq_emphasis["high"], 3)
                                freq_emphasis["high"] *= 0.8
            
            elif category == "Filter":
                # Filters remove frequencies
                if engine_id in [9, 10]:  # Ladder, State Variable
                    if len(params) > 0:
                        cutoff = params[0].get("value", 0.5)
                        if cutoff < 0.3:  # Low pass
                            freq_emphasis["high"] *= 0.5
                        elif cutoff > 0.7:  # High pass
                            freq_emphasis["low"] *= 0.5
            
            elif engine_id == 18:  # Bit Crusher
                # Reduces high frequencies
                freq_emphasis["high"] *= 0.7
        
        return preset
    
    def fix_problematic_combinations(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Fix or adjust problematic engine combinations"""
        slots = preset.get("slots", [])
        engine_ids = [slot.get("engine_id", 0) for slot in slots]
        
        # Check against known problematic combinations
        for avoid_combo in self.avoid_combinations:
            if all(eid in engine_ids for eid in avoid_combo):
                logger.warning(f"Found problematic combination: {avoid_combo}")
                
                # Reduce aggressiveness of second engine in combo
                for slot in slots:
                    if slot.get("engine_id") == avoid_combo[1]:
                        # Reduce mix or primary parameter
                        engine = self.engines.get(str(slot["engine_id"]), {})
                        mix_index = engine.get("mix_param_index", -1)
                        
                        if mix_index >= 0 and mix_index < len(slot.get("parameters", [])):
                            current_mix = slot["parameters"][mix_index]["value"]
                            slot["parameters"][mix_index]["value"] = round(current_mix * 0.5, 3)
                            logger.info(f"Reduced mix for engine {slot['engine_id']} due to combination")
        
        # Check for too many heavy effects
        heavy_count = sum(1 for eid in engine_ids if self.engines.get(str(eid), {}).get("cpu_load") == "heavy")
        if heavy_count > 2:
            logger.warning(f"Too many CPU-heavy effects ({heavy_count})")
            # Consider removing or bypassing some
        
        return preset
    
    def optimize_dynamics(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Optimize dynamics processing relationships"""
        slots = preset.get("slots", [])
        
        # Find dynamics processors
        compressors = []
        gates = []
        limiters = []
        
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            if engine_id in [1, 2]:  # Compressors
                compressors.append(slot)
            elif engine_id == 4:  # Gate
                gates.append(slot)
            elif engine_id == 5:  # Limiter
                limiters.append(slot)
        
        # Optimize compressor settings if multiple
        if len(compressors) > 1:
            # First compressor: gentle ratio, slower attack
            if len(compressors[0].get("parameters", [])) > 2:
                compressors[0]["parameters"][1]["value"] = 0.3  # Lower ratio
                compressors[0]["parameters"][2]["value"] = 0.6  # Slower attack
            
            # Second compressor: higher ratio, faster attack
            if len(compressors[1].get("parameters", [])) > 2:
                compressors[1]["parameters"][1]["value"] = 0.6  # Higher ratio
                compressors[1]["parameters"][2]["value"] = 0.3  # Faster attack
        
        # Ensure gate threshold is appropriate
        for gate in gates:
            if len(gate.get("parameters", [])) > 0:
                # Set reasonable threshold
                gate["parameters"][0]["value"] = 0.2  # Low threshold to avoid cutting
        
        # Ensure limiter is last and set appropriately
        for limiter in limiters:
            if len(limiter.get("parameters", [])) > 0:
                # Set threshold for safety
                limiter["parameters"][0]["value"] = 0.85  # -1dB ceiling
        
        return preset
    
    def calculate_quality_score(self, preset: Dict[str, Any]) -> float:
        """Calculate a quality score for the preset"""
        score = 100.0
        slots = preset.get("slots", [])
        
        # Check parameter counts
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            expected = engine.get("param_count", 0)
            actual = len(slot.get("parameters", []))
            
            if actual != expected:
                score -= 10
        
        # Check signal chain order
        positions = []
        for slot in slots:
            engine_id = slot.get("engine_id", 0)
            engine = self.engines.get(str(engine_id), {})
            positions.append(engine.get("signal_chain_position", 5))
        
        # Penalize if not in ascending order
        for i in range(1, len(positions)):
            if positions[i] < positions[i-1]:
                score -= 5
        
        # Check for problematic combinations
        engine_ids = [slot.get("engine_id", 0) for slot in slots]
        for avoid_combo in self.avoid_combinations:
            if all(eid in engine_ids for eid in avoid_combo):
                score -= 15
        
        # Bonus for reasonable slot count (not too many)
        if 2 <= len(slots) <= 4:
            score += 5
        
        return max(0, min(100, score))

def test_calculator():
    """Test the calculator component"""
    calculator = CompleteCalculator()
    
    # Create a test preset that needs optimization
    test_preset = {
        "name": "Test Preset",
        "description": "Needs optimization",
        "slots": [
            {
                "slot": 0,
                "engine_id": 39,  # Plate Reverb (wrong position)
                "engine_name": "Plate Reverb",
                "parameters": [{"name": f"param{i+1}", "value": 0.8} for i in range(10)]
            },
            {
                "slot": 1, 
                "engine_id": 22,  # K-Style Overdrive
                "engine_name": "K-Style Overdrive",
                "parameters": [{"name": f"param{i+1}", "value": 0.9} for i in range(4)]
            },
            {
                "slot": 2,
                "engine_id": 7,  # Parametric EQ (should be first)
                "engine_name": "Parametric EQ",
                "parameters": [{"name": f"param{i+1}", "value": 0.7} for i in range(9)]
            }
        ]
    }
    
    print("Original preset:")
    print(f"  Order: {[s['engine_name'] for s in test_preset['slots']]}")
    print(f"  Quality score: {calculator.calculate_quality_score(test_preset):.1f}")
    
    # Optimize
    optimized = calculator.optimize_preset(test_preset)
    
    print("\nOptimized preset:")
    print(f"  Order: {[s['engine_name'] for s in optimized['slots']]}")
    print(f"  Quality score: {calculator.calculate_quality_score(optimized):.1f}")
    
    # Show mix values
    for slot in optimized["slots"]:
        engine_id = slot["engine_id"]
        engine = calculator.engines.get(str(engine_id), {})
        mix_index = engine.get("mix_param_index", -1)
        if mix_index >= 0 and mix_index < len(slot.get("parameters", [])):
            mix_value = slot["parameters"][mix_index]["value"]
            print(f"  {slot['engine_name']}: Mix = {mix_value:.3f}")

if __name__ == "__main__":
    test_calculator()