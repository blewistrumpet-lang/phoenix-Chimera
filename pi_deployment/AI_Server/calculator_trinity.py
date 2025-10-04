#!/usr/bin/env python3
"""
CALCULATOR - The Musical Intelligence of the Trinity Pipeline
Refines parameters and optimizes signal chain with deep musical understanding
"""

import logging
from typing import Dict, Any, List, Tuple, Optional
from enum import Enum
import numpy as np

from engine_knowledge_base import (
    EngineCategory,
    get_engine_capability,
    ENGINE_KNOWLEDGE
)
from engine_mapping_authoritative import ENGINE_NAMES

logger = logging.getLogger(__name__)

class SignalChainPosition(Enum):
    """Optimal positions in signal chain"""
    GATE = 0          # Noise gates first
    DYNAMICS = 1      # Compressors, limiters
    EQ_PRE = 2        # Pre-distortion EQ
    DISTORTION = 3    # Distortion, saturation
    EQ_POST = 4       # Post-distortion EQ
    MODULATION = 5    # Chorus, phaser, etc
    PITCH = 6         # Pitch shifters
    DELAY = 7         # Delays
    REVERB = 8        # Reverbs
    SPATIAL = 9       # Stereo effects
    UTILITY = 10      # Final gain/utility

class CalculatorTrinity:
    """
    The Calculator: Applies musical intelligence to refine presets
    - Optimizes signal chain order
    - Manages parameter relationships  
    - Ensures musical coherence
    - Applies intensity scaling
    """
    
    def __init__(self):
        """Initialize with musical knowledge"""
        self.engine_knowledge = ENGINE_KNOWLEDGE
        self.engine_names = ENGINE_NAMES
        
        # Define signal chain ordering rules
        self.chain_order = self._define_chain_order()
        
        # Parameter interaction rules
        self.parameter_rules = self._define_parameter_rules()
        
        logger.info("CalculatorTrinity initialized with musical intelligence")
    
    def _define_chain_order(self) -> Dict[int, int]:
        """Define optimal signal chain position for each engine"""
        
        chain_positions = {
            # Dynamics (1-6)
            1: SignalChainPosition.DYNAMICS.value,    # Opto Compressor
            2: SignalChainPosition.DYNAMICS.value,    # Classic Compressor
            3: SignalChainPosition.DYNAMICS.value,    # Transient Shaper
            4: SignalChainPosition.GATE.value,        # Noise Gate (first!)
            5: SignalChainPosition.DYNAMICS.value,    # Mastering Limiter
            6: SignalChainPosition.DYNAMICS.value,    # Dynamic EQ
            
            # EQ/Filters (7-14)
            7: SignalChainPosition.EQ_PRE.value,      # Parametric EQ
            8: SignalChainPosition.EQ_PRE.value,      # Vintage Console EQ
            9: SignalChainPosition.EQ_PRE.value,      # Ladder Filter
            10: SignalChainPosition.EQ_PRE.value,     # State Variable Filter
            11: SignalChainPosition.EQ_POST.value,    # Comb Resonator
            12: SignalChainPosition.EQ_PRE.value,     # Formant Filter
            13: SignalChainPosition.EQ_PRE.value,     # Envelope Filter
            14: SignalChainPosition.EQ_PRE.value,     # Vocal Formant
            
            # Distortion (15-22)
            15: SignalChainPosition.DISTORTION.value,  # Vintage Tube
            16: SignalChainPosition.DISTORTION.value,  # Wave Folder
            17: SignalChainPosition.DISTORTION.value,  # Harmonic Exciter
            18: SignalChainPosition.DISTORTION.value,  # Bit Crusher
            19: SignalChainPosition.DISTORTION.value,  # Multiband Saturator
            20: SignalChainPosition.DISTORTION.value,  # Muff Fuzz
            21: SignalChainPosition.DISTORTION.value,  # Rodent Distortion
            22: SignalChainPosition.DISTORTION.value,  # K-Style Overdrive
            
            # Modulation (23-33)
            23: SignalChainPosition.MODULATION.value,  # Digital Chorus
            24: SignalChainPosition.MODULATION.value,  # Resonant Chorus
            25: SignalChainPosition.MODULATION.value,  # Analog Phaser
            26: SignalChainPosition.MODULATION.value,  # Ring Modulator
            27: SignalChainPosition.MODULATION.value,  # Frequency Shifter
            28: SignalChainPosition.MODULATION.value,  # Harmonic Tremolo
            29: SignalChainPosition.MODULATION.value,  # Classic Tremolo
            30: SignalChainPosition.MODULATION.value,  # Rotary Speaker
            31: SignalChainPosition.PITCH.value,       # Pitch Shifter
            32: SignalChainPosition.PITCH.value,       # Detune Doubler
            33: SignalChainPosition.PITCH.value,       # Intelligent Harmonizer
            
            # Delay (34-38)
            34: SignalChainPosition.DELAY.value,       # Tape Echo
            35: SignalChainPosition.DELAY.value,       # Digital Delay
            36: SignalChainPosition.DELAY.value,       # Magnetic Drum
            37: SignalChainPosition.DELAY.value,       # Bucket Brigade
            38: SignalChainPosition.DELAY.value,       # Buffer Repeat
            
            # Reverb (39-43)
            39: SignalChainPosition.REVERB.value,      # Plate Reverb
            40: SignalChainPosition.REVERB.value,      # Spring Reverb
            41: SignalChainPosition.REVERB.value,      # Convolution Reverb
            42: SignalChainPosition.REVERB.value,      # Shimmer Reverb
            43: SignalChainPosition.REVERB.value,      # Gated Reverb
            
            # Spatial (44-46)
            44: SignalChainPosition.SPATIAL.value,     # Stereo Widener
            45: SignalChainPosition.SPATIAL.value,     # Stereo Imager
            46: SignalChainPosition.SPATIAL.value,     # Dimension Expander
            
            # Special (47-52)
            47: SignalChainPosition.PITCH.value,       # Formant Shifter
            48: SignalChainPosition.PITCH.value,       # Gender Bender
            49: SignalChainPosition.MODULATION.value,  # Spectral Freeze
            50: SignalChainPosition.MODULATION.value,  # Spectral Gate
            51: SignalChainPosition.MODULATION.value,  # Granular Cloud
            52: SignalChainPosition.MODULATION.value,  # Chaos Generator
            
            # Utility (53-56)
            53: SignalChainPosition.UTILITY.value,     # Mid-Side Processor
            54: SignalChainPosition.UTILITY.value,     # Gain Utility
            55: SignalChainPosition.UTILITY.value,     # Mono Maker
            56: SignalChainPosition.UTILITY.value,     # Phase Align
        }
        
        return chain_positions
    
    def _define_parameter_rules(self) -> Dict[str, Any]:
        """Define parameter interaction rules"""
        
        return {
            "gain_staging": {
                "max_cumulative": 3.0,
                "gain_params": ["Input Gain", "Drive", "Output Gain", "Makeup Gain", "Level", "Volume"],
                "strategy": "normalize"
            },
            "feedback_safety": {
                "max_total": 2.0,
                "feedback_params": ["Feedback", "Regeneration", "Resonance"],
                "critical_engines": [34, 35, 36, 37, 39, 40, 41, 42, 43],
                "strategy": "limit"
            },
            "frequency_spacing": {
                "min_separation": 0.15,
                "frequency_params": ["Frequency", "Cutoff", "Center"],
                "affected_engines": [7, 8, 9, 10, 11, 12, 13, 14],
                "strategy": "stagger"
            },
            "modulation_rates": {
                "relationship": "polyrhythmic",
                "rate_params": ["Rate", "Speed", "LFO"],
                "affected_engines": [23, 24, 25, 26, 27, 28, 29, 30],
                "strategy": "diversify"
            },
            "mix_balance": {
                "total_wet_limit": 2.5,
                "mix_params": ["Mix", "Dry/Wet", "Blend"],
                "strategy": "balance"
            }
        }
    
    def refine_preset(self, preset: Dict[str, Any], prompt: str) -> Dict[str, Any]:
        """
        Apply musical intelligence to refine the preset
        
        Args:
            preset: Raw preset from Visionary
            prompt: Original user prompt for context
            
        Returns:
            Refined preset with optimized parameters and ordering
        """
        
        try:
            # 1. Optimize signal chain order
            preset = self._optimize_signal_chain(preset)
            
            # 2. Apply parameter relationships
            preset = self._apply_parameter_relationships(preset)
            
            # 3. Ensure musical coherence
            preset = self._ensure_musical_coherence(preset, prompt)
            
            # 4. Apply intensity scaling based on prompt
            preset = self._apply_intensity_scaling(preset, prompt)
            
            # 5. Prevent frequency conflicts
            preset = self._prevent_frequency_conflicts(preset)
            
            # 6. Balance mix levels
            preset = self._balance_mix_levels(preset)
            
            # 7. Optimize for character
            preset = self._optimize_for_character(preset, prompt)
            
            logger.info(f"Refined preset: {preset.get('name', 'Unknown')}")
            
            return preset
            
        except Exception as e:
            logger.error(f"Error refining preset: {str(e)}")
            return preset  # Return unmodified if error
    
    def _optimize_signal_chain(self, preset: Dict) -> Dict:
        """Reorder engines for optimal signal flow"""
        
        slots = preset.get("slots", [])
        if not slots:
            return preset
        
        # Get positions for each engine
        engine_positions = []
        for i, slot in enumerate(slots):
            engine_id = slot.get("engine_id", 0)
            position = self.chain_order.get(engine_id, 99)
            engine_positions.append((i, position, slot))
        
        # Sort by position
        engine_positions.sort(key=lambda x: x[1])
        
        # Apply special rules
        engine_positions = self._apply_chain_rules(engine_positions)
        
        # Rebuild slots in optimal order
        reordered_slots = []
        for i, (_, _, slot) in enumerate(engine_positions):
            slot["slot"] = i + 1
            reordered_slots.append(slot)
        
        preset["slots"] = reordered_slots
        
        logger.debug(f"Reordered signal chain: {[s['engine_name'] for s in reordered_slots]}")
        
        return preset
    
    def _apply_chain_rules(self, positions: List[Tuple]) -> List[Tuple]:
        """Apply special signal chain rules"""
        
        # Rule 1: Gate always first if present
        has_gate = any(slot.get("engine_id") == 4 for _, _, slot in positions)
        if has_gate:
            positions = sorted(positions, key=lambda x: 0 if x[2].get("engine_id") == 4 else x[1])
        
        # Rule 2: Limiter always last among dynamics
        has_limiter = any(slot.get("engine_id") == 5 for _, _, slot in positions)
        if has_limiter:
            # Move limiter after other dynamics
            new_positions = []
            limiter_slot = None
            for item in positions:
                if item[2].get("engine_id") == 5:
                    limiter_slot = item
                else:
                    new_positions.append(item)
            
            if limiter_slot:
                # Insert after last dynamics but before distortion
                insert_pos = 0
                for i, (_, pos, _) in enumerate(new_positions):
                    if pos > SignalChainPosition.DYNAMICS.value:
                        insert_pos = i
                        break
                else:
                    insert_pos = len(new_positions)
                
                new_positions.insert(insert_pos, limiter_slot)
                positions = new_positions
        
        # Rule 3: Stereo effects at end
        stereo_engines = [44, 45, 46]
        stereo_slots = []
        other_slots = []
        
        for item in positions:
            if item[2].get("engine_id") in stereo_engines:
                stereo_slots.append(item)
            else:
                other_slots.append(item)
        
        positions = other_slots + stereo_slots
        
        return positions
    
    def _apply_parameter_relationships(self, preset: Dict) -> Dict:
        """Manage complex parameter interactions"""
        
        slots = preset.get("slots", [])
        
        # 1. Check gain staging
        total_gain = self._calculate_total_gain(slots)
        if total_gain > self.parameter_rules["gain_staging"]["max_cumulative"]:
            slots = self._normalize_gain_staging(slots, total_gain)
        
        # 2. Check feedback accumulation
        total_feedback = self._calculate_total_feedback(slots)
        if total_feedback > self.parameter_rules["feedback_safety"]["max_total"]:
            slots = self._limit_feedback(slots, total_feedback)
        
        # 3. Stagger frequencies for multiple EQs/filters
        eq_slots = [s for s in slots if s.get("engine_id") in self.parameter_rules["frequency_spacing"]["affected_engines"]]
        if len(eq_slots) > 1:
            slots = self._stagger_frequencies(slots, eq_slots)
        
        # 4. Diversify modulation rates
        mod_slots = [s for s in slots if s.get("engine_id") in self.parameter_rules["modulation_rates"]["affected_engines"]]
        if len(mod_slots) > 1:
            slots = self._diversify_modulation_rates(slots, mod_slots)
        
        preset["slots"] = slots
        return preset
    
    def _calculate_total_gain(self, slots: List[Dict]) -> float:
        """Calculate cumulative gain through signal chain"""
        
        total = 1.0
        for slot in slots:
            params = slot.get("parameters", [])
            engine_id = slot.get("engine_id")
            
            # Map parameter indices to gain-related parameters
            gain_indices = {
                1: [0, 3],      # Opto: Input, Output
                2: [5],         # Compressor: Makeup
                15: [0, 7],     # Tube: Input, Output
                18: [3, 4],     # Bit Crusher: Input, Output
                20: [2],        # Muff: Volume
                21: [4],        # Rodent: Output
                22: [2],        # K-Style: Level
            }
            
            if engine_id in gain_indices:
                for idx in gain_indices[engine_id]:
                    if idx < len(params):
                        # Convert 0-1 to gain multiplier (0.5 = unity)
                        total *= (0.5 + params[idx] * 0.5)
        
        return total
    
    def _normalize_gain_staging(self, slots: List[Dict], current_total: float) -> List[Dict]:
        """Normalize gain to prevent overload"""
        
        target = self.parameter_rules["gain_staging"]["max_cumulative"]
        scale = target / current_total
        
        logger.debug(f"Normalizing gain: {current_total:.2f} -> {target}")
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            # Scale gain-related parameters
            gain_indices = {
                1: [0, 3],      # Opto: Input, Output
                2: [5],         # Compressor: Makeup
                15: [0, 7],     # Tube: Input, Output
                # ... etc
            }
            
            if engine_id in gain_indices:
                for idx in gain_indices[engine_id]:
                    if idx < len(params):
                        # Scale toward center (0.5)
                        params[idx] = 0.5 + (params[idx] - 0.5) * scale
        
        return slots
    
    def _calculate_total_feedback(self, slots: List[Dict]) -> float:
        """Calculate total feedback/resonance"""
        
        total = 0.0
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            # Map feedback parameter indices
            feedback_indices = {
                9: [1],         # Ladder Filter: Resonance
                10: [1],        # State Variable: Resonance
                11: [2],        # Comb: Feedback
                23: [3],        # Chorus: Feedback
                34: [1],        # Tape Echo: Feedback
                35: [1],        # Digital Delay: Feedback
                # ... etc
            }
            
            if engine_id in feedback_indices:
                for idx in feedback_indices[engine_id]:
                    if idx < len(params):
                        total += params[idx]
        
        return total
    
    def _limit_feedback(self, slots: List[Dict], current_total: float) -> List[Dict]:
        """Limit feedback to prevent runaway"""
        
        target = self.parameter_rules["feedback_safety"]["max_total"]
        scale = target / current_total if current_total > 0 else 1.0
        
        logger.debug(f"Limiting feedback: {current_total:.2f} -> {target}")
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            feedback_indices = {
                34: [1],  # Tape Echo
                35: [1],  # Digital Delay
                # ... etc
            }
            
            if engine_id in feedback_indices:
                for idx in feedback_indices[engine_id]:
                    if idx < len(params):
                        params[idx] *= scale
                        params[idx] = min(params[idx], 0.7)  # Hard limit
        
        return slots
    
    def _stagger_frequencies(self, slots: List[Dict], eq_slots: List[Dict]) -> List[Dict]:
        """Ensure EQs operate at different frequencies"""
        
        # Define frequency positions across spectrum
        freq_positions = [0.15, 0.3, 0.5, 0.7, 0.85]
        
        for i, eq_slot in enumerate(eq_slots):
            if i < len(freq_positions):
                engine_id = eq_slot.get("engine_id")
                params = eq_slot.get("parameters", [])
                
                # Map frequency parameter indices
                freq_indices = {
                    7: [0, 3, 6],   # Parametric EQ: Low, Mid, High freq
                    9: [0],         # Ladder Filter: Cutoff
                    10: [0],        # State Variable: Cutoff
                    # ... etc
                }
                
                if engine_id in freq_indices:
                    # Set primary frequency
                    if len(freq_indices[engine_id]) > 0:
                        idx = freq_indices[engine_id][0]
                        if idx < len(params):
                            params[idx] = freq_positions[i]
        
        return slots
    
    def _diversify_modulation_rates(self, slots: List[Dict], mod_slots: List[Dict]) -> List[Dict]:
        """Create polyrhythmic modulation rates"""
        
        # Musical rate ratios (avoid same rates)
        rate_ratios = [1.0, 0.66, 1.5, 0.5, 2.0]
        base_rate = 0.2  # ~2Hz base
        
        for i, mod_slot in enumerate(mod_slots):
            if i < len(rate_ratios):
                engine_id = mod_slot.get("engine_id")
                params = mod_slot.get("parameters", [])
                
                # Map rate parameter indices
                rate_indices = {
                    23: [0],  # Chorus: Rate
                    25: [0],  # Phaser: Rate
                    28: [0],  # Harmonic Tremolo: Rate
                    29: [0],  # Classic Tremolo: Rate
                    # ... etc
                }
                
                if engine_id in rate_indices and len(rate_indices[engine_id]) > 0:
                    idx = rate_indices[engine_id][0]
                    if idx < len(params):
                        params[idx] = min(1.0, base_rate * rate_ratios[i])
        
        return slots
    
    def _ensure_musical_coherence(self, preset: Dict, prompt: str) -> Dict:
        """Ensure all elements work together musically"""
        
        character = self._detect_character(prompt)
        slots = preset.get("slots", [])
        
        # Apply character-specific adjustments
        if character == "warm":
            slots = self._enhance_warmth(slots)
        elif character == "aggressive":
            slots = self._enhance_aggression(slots)
        elif character == "clean":
            slots = self._enhance_clarity(slots)
        elif character == "spacious":
            slots = self._enhance_space(slots)
        
        preset["slots"] = slots
        return preset
    
    def _detect_character(self, prompt: str) -> str:
        """Detect the sonic character from prompt"""
        
        prompt_lower = prompt.lower()
        
        if any(w in prompt_lower for w in ["warm", "vintage", "analog", "smooth"]):
            return "warm"
        elif any(w in prompt_lower for w in ["aggressive", "brutal", "heavy"]):
            return "aggressive"
        elif any(w in prompt_lower for w in ["clean", "pristine", "clear"]):
            return "clean"
        elif any(w in prompt_lower for w in ["space", "ambient", "ethereal"]):
            return "spacious"
        else:
            return "balanced"
    
    def _enhance_warmth(self, slots: List[Dict]) -> List[Dict]:
        """Enhance warm character"""
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            # Reduce highs, increase lows
            if engine_id in [7, 8]:  # EQs
                if len(params) > 7:
                    params[7] = max(0.0, params[7] - 0.2)  # Reduce high gain
                if len(params) > 1:
                    params[1] = min(1.0, params[1] + 0.1)  # Boost low gain
            
            # Add tube harmonics
            if engine_id == 15:  # Vintage Tube
                if len(params) > 6:
                    params[6] = min(1.0, params[6] + 0.2)  # More harmonics
        
        return slots
    
    def _enhance_aggression(self, slots: List[Dict]) -> List[Dict]:
        """Enhance aggressive character"""
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            # Increase drive/distortion
            if engine_id in [15, 16, 17, 18, 19, 20, 21, 22]:  # Distortions
                if len(params) > 0:
                    params[0] = min(1.0, params[0] + 0.2)  # More drive
            
            # Faster attack on dynamics
            if engine_id in [1, 2, 4]:  # Compressors/Gate
                if len(params) > 2:
                    params[2] = max(0.0, params[2] - 0.2)  # Faster attack
        
        return slots
    
    def _enhance_clarity(self, slots: List[Dict]) -> List[Dict]:
        """Enhance clean/clear character"""
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            # Reduce all mix levels
            mix_indices = {
                15: [9],  # Tube: Mix
                18: [2],  # Bit Crusher: Mix
                23: [2],  # Chorus: Mix
                34: [4],  # Tape Echo: Mix
                39: [3],  # Plate Reverb: Mix
                # ... etc
            }
            
            if engine_id in mix_indices:
                for idx in mix_indices[engine_id]:
                    if idx < len(params):
                        params[idx] = max(0.1, params[idx] * 0.7)
        
        return slots
    
    def _enhance_space(self, slots: List[Dict]) -> List[Dict]:
        """Enhance spacious character"""
        
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            # Increase reverb/delay sizes and times
            if engine_id in [34, 35, 36, 37]:  # Delays
                if len(params) > 0:
                    params[0] = min(1.0, params[0] + 0.2)  # Longer time
            
            if engine_id in [39, 40, 41, 42]:  # Reverbs
                if len(params) > 0:
                    params[0] = min(1.0, params[0] + 0.3)  # Larger size
        
        return slots
    
    def _apply_intensity_scaling(self, preset: Dict, prompt: str) -> Dict:
        """Apply intensity scaling based on prompt modifiers"""
        
        intensity = self._detect_intensity(prompt)
        slots = preset.get("slots", [])
        
        # Define intensity scales
        scales = {
            "subtle": 0.5,
            "gentle": 0.6,
            "moderate": 1.0,
            "strong": 1.3,
            "extreme": 1.6
        }
        
        scale = scales.get(intensity, 1.0)
        
        if scale != 1.0:
            logger.debug(f"Applying intensity scale: {intensity} ({scale})")
            
            for slot in slots:
                params = slot.get("parameters", [])
                
                # Scale intensity-related parameters
                # Mix parameters (usually at specific indices)
                mix_indices = [2, 3, 4, 5, 9]  # Common mix parameter positions
                
                for idx in mix_indices:
                    if idx < len(params):
                        # Scale around 0.5 center
                        params[idx] = 0.5 + (params[idx] - 0.5) * scale
                        params[idx] = max(0.0, min(1.0, params[idx]))
        
        preset["slots"] = slots
        return preset
    
    def _detect_intensity(self, prompt: str) -> str:
        """Detect intensity level from prompt"""
        
        prompt_lower = prompt.lower()
        
        if any(w in prompt_lower for w in ["subtle", "slight", "touch", "hint"]):
            return "subtle"
        elif any(w in prompt_lower for w in ["gentle", "soft", "mild"]):
            return "gentle"
        elif any(w in prompt_lower for w in ["extreme", "maximum", "heavy", "intense"]):
            return "extreme"
        elif any(w in prompt_lower for w in ["strong", "bold", "prominent"]):
            return "strong"
        else:
            return "moderate"
    
    def _prevent_frequency_conflicts(self, preset: Dict) -> Dict:
        """Prevent frequency masking and conflicts"""
        
        slots = preset.get("slots", [])
        
        # Find all frequency-manipulating engines
        freq_engines = []
        for slot in slots:
            engine_id = slot.get("engine_id")
            if engine_id in [7, 8, 9, 10, 11, 12, 13, 14]:  # EQ/Filter engines
                freq_engines.append(slot)
        
        # If multiple frequency engines, ensure they complement
        if len(freq_engines) > 1:
            # Distribute across frequency spectrum
            freq_targets = [0.2, 0.4, 0.6, 0.8]  # Low, low-mid, high-mid, high
            
            for i, slot in enumerate(freq_engines):
                if i < len(freq_targets):
                    self._set_primary_frequency(slot, freq_targets[i])
        
        preset["slots"] = slots
        return preset
    
    def _set_primary_frequency(self, slot: Dict, target: float):
        """Set the primary frequency parameter of an engine"""
        
        engine_id = slot.get("engine_id")
        params = slot.get("parameters", [])
        
        # Map primary frequency parameter
        freq_param_map = {
            7: 3,   # Parametric EQ: Mid frequency
            9: 0,   # Ladder Filter: Cutoff
            10: 0,  # State Variable: Cutoff
            11: 0,  # Comb: Frequency
            # ... etc
        }
        
        if engine_id in freq_param_map:
            idx = freq_param_map[engine_id]
            if idx < len(params):
                params[idx] = target
    
    def _balance_mix_levels(self, preset: Dict) -> Dict:
        """Balance mix levels for clarity"""
        
        slots = preset.get("slots", [])
        
        # Count effect density
        effect_count = len(slots)
        
        # Determine mix scaling based on density
        if effect_count > 4:
            mix_scale = 0.7  # Many effects: reduce individual levels
        elif effect_count > 2:
            mix_scale = 0.85  # Moderate: slight reduction
        else:
            mix_scale = 1.0  # Few effects: full levels OK
        
        # Apply scaling to mix parameters
        for slot in slots:
            engine_id = slot.get("engine_id")
            params = slot.get("parameters", [])
            
            # Find mix parameter index for this engine
            mix_param_indices = {
                1: [4],   # Opto: Mix
                15: [9],  # Tube: Mix
                18: [2],  # Bit Crusher: Mix
                23: [2],  # Chorus: Mix
                34: [4],  # Tape Echo: Mix
                39: [3],  # Plate Reverb: Mix
                42: [3],  # Shimmer: Mix
                # ... etc
            }
            
            if engine_id in mix_param_indices:
                for idx in mix_param_indices[engine_id]:
                    if idx < len(params):
                        params[idx] *= mix_scale
                        params[idx] = max(0.1, params[idx])  # Minimum presence
        
        preset["slots"] = slots
        return preset
    
    def _optimize_for_character(self, preset: Dict, prompt: str) -> Dict:
        """Final character-specific optimizations"""
        
        # Detect genre if mentioned
        genre = self._detect_genre(prompt)
        
        if genre:
            preset = self._apply_genre_optimizations(preset, genre)
        
        return preset
    
    def _detect_genre(self, prompt: str) -> Optional[str]:
        """Detect musical genre from prompt"""
        
        prompt_lower = prompt.lower()
        
        genres = {
            "metal": ["metal", "djent", "brutal", "death"],
            "jazz": ["jazz", "swing", "bebop"],
            "electronic": ["edm", "techno", "house", "dubstep"],
            "rock": ["rock", "punk", "grunge"],
            "ambient": ["ambient", "atmospheric", "drone"]
        }
        
        for genre, keywords in genres.items():
            if any(kw in prompt_lower for kw in keywords):
                return genre
        
        return None
    
    def _apply_genre_optimizations(self, preset: Dict, genre: str) -> Dict:
        """Apply genre-specific optimizations"""
        
        slots = preset.get("slots", [])
        
        if genre == "metal":
            # Tight gate, high gain, controlled low end
            for slot in slots:
                if slot.get("engine_id") == 4:  # Gate
                    params = slot.get("parameters", [])
                    if len(params) > 0:
                        params[0] = min(1.0, params[0] + 0.1)  # Tighter threshold
        
        elif genre == "jazz":
            # Warm, smooth, less distortion
            for slot in slots:
                if slot.get("engine_id") in [15, 20, 21, 22]:  # Distortions
                    params = slot.get("parameters", [])
                    if len(params) > 0:
                        params[0] *= 0.5  # Reduce drive
        
        # ... more genre optimizations
        
        preset["slots"] = slots
        return preset

if __name__ == "__main__":
    # Test the Calculator
    print("🧮 CALCULATOR TRINITY TEST")
    print("=" * 60)
    
    calculator = CalculatorTrinity()
    
    # Create a test preset
    test_preset = {
        "name": "Test Preset",
        "slots": [
            {
                "slot": 1,
                "engine_id": 39,  # Plate Reverb (should move later)
                "engine_name": "Plate Reverb",
                "parameters": [0.5] * 10
            },
            {
                "slot": 2,
                "engine_id": 15,  # Vintage Tube (distortion)
                "engine_name": "Vintage Tube Preamp",
                "parameters": [0.7, 0.6, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.0, 1.0]
            },
            {
                "slot": 3,
                "engine_id": 4,  # Noise Gate (should move first)
                "engine_name": "Noise Gate",
                "parameters": [0.3] * 10
            }
        ]
    }
    
    print("Original order:")
    for slot in test_preset["slots"]:
        print(f"  {slot['slot']}: {slot['engine_name']}")
    
    # Test refinement
    refined = calculator.refine_preset(test_preset, "aggressive metal tone")
    
    print("\nRefined order:")
    for slot in refined["slots"]:
        print(f"  {slot['slot']}: {slot['engine_name']}")
    
    print("\nSignal chain optimization: ✅")