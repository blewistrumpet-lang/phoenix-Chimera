#!/usr/bin/env python3
"""
Signal Chain Intelligence - Ensures proper effect ordering and parameter safety
"""

from typing import List, Dict, Any, Tuple
from engine_mapping_authoritative import ENGINE_NAMES

# Signal chain categories with proper ordering (lower number = earlier in chain)
SIGNAL_CHAIN_PRIORITY = {
    # Input/Utility (can go anywhere but typically first)
    54: 0,   # Gain Utility - input gain staging
    
    # Dynamics - Should come early to control dynamics before processing
    4: 10,   # Noise Gate - remove noise before processing
    2: 11,   # Classic Compressor - control dynamics
    1: 12,   # Vintage Opto Compressor
    3: 13,   # Transient Shaper - shape transients
    5: 15,   # Mastering Limiter - typically later but before final stages
    
    # EQ & Filters - Shape frequency before distortion
    7: 20,   # Parametric EQ
    8: 21,   # Vintage Console EQ
    6: 22,   # Dynamic EQ
    9: 25,   # Ladder Filter
    10: 26,  # State Variable Filter
    11: 27,  # Formant Filter
    12: 28,  # Envelope Filter
    13: 29,  # Comb Resonator
    14: 30,  # Vocal Formant Filter
    
    # Distortion & Saturation - Add harmonics after EQ
    15: 40,  # Vintage Tube Preamp
    22: 41,  # K-Style Overdrive
    21: 42,  # Rodent Distortion
    20: 43,  # Muff Fuzz
    16: 44,  # Wave Folder
    17: 45,  # Harmonic Exciter
    19: 46,  # Multiband Saturator
    18: 47,  # Bit Crusher - extreme distortion last
    
    # Modulation - Add movement after tone shaping
    29: 50,  # Classic Tremolo
    28: 51,  # Harmonic Tremolo
    23: 52,  # Digital Chorus
    24: 53,  # Resonant Chorus
    25: 54,  # Analog Phaser
    30: 55,  # Rotary Speaker
    26: 56,  # Ring Modulator
    27: 57,  # Frequency Shifter
    
    # Pitch Effects - Can go various places
    32: 60,  # Detune Doubler
    31: 61,  # Pitch Shifter
    33: 62,  # Intelligent Harmonizer
    
    # Time-based Effects - Delays before reverbs
    35: 70,  # Digital Delay
    34: 71,  # Tape Echo
    36: 72,  # Magnetic Drum Echo
    37: 73,  # Bucket Brigade Delay
    38: 74,  # Buffer Repeat
    
    # Reverbs - Typically at the end
    39: 80,  # Plate Reverb
    40: 81,  # Spring Reverb
    41: 82,  # Convolution Reverb
    43: 83,  # Gated Reverb
    42: 84,  # Shimmer Reverb - often last for ethereal effect
    
    # Special Effects - Position varies by effect
    47: 85,  # Spectral Freeze
    48: 86,  # Spectral Gate
    49: 87,  # Phased Vocoder
    50: 88,  # Granular Cloud
    51: 89,  # Chaos Generator
    52: 90,  # Feedback Network
    
    # Spatial & Width - Usually near the end
    44: 91,  # Stereo Widener
    45: 92,  # Stereo Imager
    46: 93,  # Dimension Expander
    
    # Final Utility - Last in chain
    53: 95,  # Mid-Side Processor
    55: 96,  # Mono Maker - bass mono for vinyl
    56: 97,  # Phase Align - fix phase issues
}

# Parameter safety limits to prevent audio issues
PARAMETER_SAFETY = {
    # Feedback parameters - prevent runaway feedback
    "feedback": {
        "max_safe": 0.95,
        "typical_max": 0.8,
        "warning": "High feedback can cause runaway oscillation"
    },
    
    # Resonance with filter cutoff interaction
    "resonance": {
        "max_safe": 0.9,
        "max_with_low_cutoff": 0.7,  # Lower when cutoff < 200Hz
        "warning": "High resonance with low cutoff causes self-oscillation"
    },
    
    # Drive/distortion limits
    "drive": {
        "max_safe": 0.95,
        "cumulative_max": 1.5,  # Total across all distortions
        "warning": "Excessive drive causes unpleasant digital clipping"
    },
    
    # Mix levels for extreme effects
    "bit_crusher_mix": {
        "max_safe": 0.7,
        "typical_max": 0.5,
        "warning": "Full bit crushing destroys intelligibility"
    },
    
    # Delay feedback with short times
    "delay_feedback_short": {
        "max_safe": 0.6,  # When delay < 100ms
        "warning": "High feedback with short delays creates harsh comb filtering"
    }
}

class SignalChainIntelligence:
    """Ensures intelligent signal chain ordering and parameter safety"""
    
    def __init__(self):
        self.chain_priority = SIGNAL_CHAIN_PRIORITY
        self.safety_limits = PARAMETER_SAFETY
    
    def optimize_signal_chain(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """
        Reorder engines in slots for optimal signal flow
        Returns optimized preset with proper effect ordering
        """
        # Extract engines from slots
        engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                # Store engine with its parameters
                engine_data = {
                    "id": engine_id,
                    "priority": self.chain_priority.get(engine_id, 50),
                    "slot": slot,
                    "params": {}
                }
                # Collect parameters for this slot
                for param in range(16):
                    param_key = f"slot{slot}_param{param}"
                    if param_key in preset:
                        engine_data["params"][f"param{param}"] = preset[param_key]
                engines.append(engine_data)
        
        # Sort by priority (lower number = earlier in chain)
        engines.sort(key=lambda x: x["priority"])
        
        # Rebuild preset with optimized ordering
        optimized = {}
        
        # Copy non-slot data
        for key, value in preset.items():
            if not key.startswith("slot"):
                optimized[key] = value
        
        # Place engines in new order
        for new_slot, engine_data in enumerate(engines, 1):
            optimized[f"slot{new_slot}_engine"] = engine_data["id"]
            for param_name, param_value in engine_data["params"].items():
                param_num = int(param_name.replace("param", ""))
                optimized[f"slot{new_slot}_param{param_num}"] = param_value
        
        # Fill empty slots
        for slot in range(len(engines) + 1, 7):
            optimized[f"slot{slot}_engine"] = 0
        
        return optimized
    
    def validate_parameters(self, preset: Dict[str, Any]) -> Tuple[bool, List[str]]:
        """
        Validate parameters for safety and musicality
        Returns (is_safe, list_of_warnings)
        """
        warnings = []
        is_safe = True
        
        # Helper to safely convert values to float
        def safe_float(value, default=0):
            try:
                return float(value)
            except (TypeError, ValueError):
                return default
        
        # Check cumulative drive/distortion
        total_drive = 0
        distortion_engines = [15, 16, 17, 18, 19, 20, 21, 22]
        
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            
            # Check distortion accumulation
            if engine_id in distortion_engines:
                # Assume param1 is typically drive
                drive = safe_float(preset.get(f"slot{slot}_param1", 0))
                total_drive += drive
            
            # Check bit crusher mix
            if engine_id == 18:  # Bit Crusher
                mix = safe_float(preset.get(f"slot{slot}_param2", 0))
                if mix > 0.7:
                    warnings.append(f"Bit Crusher mix very high ({mix*100:.1f}%) - may destroy audio")
            
            # Check delay feedback
            if engine_id in [34, 35, 36, 37]:  # Delay engines
                time = safe_float(preset.get(f"slot{slot}_param0", 0.5))
                feedback = safe_float(preset.get(f"slot{slot}_param1", 0))
                
                if time < 0.1 and feedback > 0.6:  # Short delay, high feedback
                    warnings.append(f"High feedback ({feedback*100:.1f}%) with short delay - harsh comb filtering")
                elif feedback > 0.95:
                    warnings.append(f"Extremely high delay feedback ({feedback*100:.1f}%) - potential runaway")
                    is_safe = False
            
            # Check filter resonance
            if engine_id in [9, 10]:  # Ladder, State Variable filters
                cutoff = safe_float(preset.get(f"slot{slot}_param0", 0.5))
                resonance = safe_float(preset.get(f"slot{slot}_param1", 0))
                
                if cutoff < 0.2 and resonance > 0.7:
                    warnings.append(f"High resonance ({resonance*100:.1f}%) with low cutoff - self-oscillation")
        
        # Check total distortion
        if total_drive > 1.5:
            warnings.append(f"Very high total distortion ({total_drive:.1f}) - severe clipping likely")
            if total_drive > 2.0:
                is_safe = False
        
        return is_safe, warnings
    
    def suggest_improvements(self, preset: Dict[str, Any], prompt: str) -> Dict[str, str]:
        """
        Suggest improvements based on prompt intent and current preset
        Returns dict of suggestions
        """
        suggestions = {}
        prompt_lower = prompt.lower()
        
        # Check for missing essential effects based on prompt
        engines_present = [preset.get(f"slot{slot}_engine", 0) for slot in range(1, 7)]
        
        if "vocal" in prompt_lower and 1 not in engines_present:
            suggestions["compression"] = "Consider adding Opto Compressor for smooth vocal control"
        
        if "warm" in prompt_lower and 15 not in engines_present:
            suggestions["warmth"] = "Add Vintage Tube Preamp for analog warmth"
        
        if "space" in prompt_lower or "ambient" in prompt_lower:
            has_reverb = any(e in engines_present for e in [39, 40, 41, 42, 43])
            if not has_reverb:
                suggestions["space"] = "Add reverb (Plate or Shimmer) for spatial depth"
        
        if "punch" in prompt_lower and 3 not in engines_present:
            suggestions["punch"] = "Add Transient Shaper to enhance punch and attack"
        
        # Check signal chain order
        optimized = self.optimize_signal_chain(preset)
        if optimized != preset:
            suggestions["ordering"] = "Signal chain can be optimized for better sound"
        
        # Check safety
        is_safe, warnings = self.validate_parameters(preset)
        if warnings:
            suggestions["safety"] = f"Parameter adjustments needed: {warnings[0]}"
        
        return suggestions
    
    def explain_chain(self, preset: Dict[str, Any]) -> str:
        """
        Generate human-readable explanation of the signal chain
        """
        engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                engine_name = ENGINE_NAMES.get(engine_id, "Unknown")
                engines.append(engine_name)
        
        if not engines:
            return "Empty preset - no processing"
        
        # Build explanation
        explanation = "Signal flow: "
        
        # Group by category
        dynamics = [e for e in engines if any(d in e.lower() for d in ["compress", "limit", "gate", "transient"])]
        eq = [e for e in engines if any(q in e.lower() for q in ["eq", "filter", "parametric"])]
        distortion = [e for e in engines if any(d in e.lower() for d in ["tube", "distort", "overdrive", "fuzz", "crush", "excite", "saturat"])]
        modulation = [e for e in engines if any(m in e.lower() for m in ["chorus", "phase", "tremolo", "rotar"])]
        delays = [e for e in engines if any(d in e.lower() for d in ["delay", "echo"])]
        reverbs = [e for e in engines if "reverb" in e.lower()]
        
        parts = []
        if dynamics:
            parts.append(f"Dynamics control ({', '.join(dynamics)})")
        if eq:
            parts.append(f"Frequency shaping ({', '.join(eq)})")
        if distortion:
            parts.append(f"Harmonic enhancement ({', '.join(distortion)})")
        if modulation:
            parts.append(f"Modulation ({', '.join(modulation)})")
        if delays:
            parts.append(f"Time effects ({', '.join(delays)})")
        if reverbs:
            parts.append(f"Spatial processing ({', '.join(reverbs)})")
        
        explanation += " → ".join(parts)
        
        return explanation


# Convenience functions
def optimize_preset(preset: Dict[str, Any]) -> Dict[str, Any]:
    """Quick function to optimize a preset's signal chain"""
    intelligence = SignalChainIntelligence()
    return intelligence.optimize_signal_chain(preset)

def validate_preset(preset: Dict[str, Any]) -> Tuple[bool, List[str]]:
    """Quick function to validate a preset's parameters"""
    intelligence = SignalChainIntelligence()
    return intelligence.validate_parameters(preset)

def explain_preset(preset: Dict[str, Any]) -> str:
    """Quick function to explain a preset's signal flow"""
    intelligence = SignalChainIntelligence()
    return intelligence.explain_chain(preset)


if __name__ == "__main__":
    # Test the signal chain intelligence
    print("🧠 SIGNAL CHAIN INTELLIGENCE TEST")
    print("=" * 60)
    
    # Test preset with bad ordering
    test_preset = {
        "slot1_engine": 42,  # Shimmer Reverb (should be last)
        "slot1_param0": 0.7,
        "slot2_engine": 15,  # Vintage Tube (should be earlier)
        "slot2_param1": 0.5,
        "slot3_engine": 1,   # Opto Compressor (should be first)
        "slot3_param1": 0.3,
        "slot4_engine": 18,  # Bit Crusher
        "slot4_param2": 0.9,  # Very high mix - dangerous
        "slot5_engine": 35,  # Digital Delay
        "slot5_param0": 0.05, # Short delay
        "slot5_param1": 0.8,  # High feedback - dangerous with short delay
        "slot6_engine": 0
    }
    
    intelligence = SignalChainIntelligence()
    
    # Test ordering
    print("\nORIGINAL ORDER:")
    for slot in range(1, 7):
        engine_id = test_preset.get(f"slot{slot}_engine", 0)
        if engine_id > 0:
            print(f"  Slot {slot}: {ENGINE_NAMES[engine_id]}")
    
    optimized = intelligence.optimize_signal_chain(test_preset)
    print("\nOPTIMIZED ORDER:")
    for slot in range(1, 7):
        engine_id = optimized.get(f"slot{slot}_engine", 0)
        if engine_id > 0:
            print(f"  Slot {slot}: {ENGINE_NAMES[engine_id]}")
    
    # Test validation
    print("\nVALIDATION:")
    is_safe, warnings = intelligence.validate_parameters(test_preset)
    print(f"  Safe: {is_safe}")
    if warnings:
        print("  Warnings:")
        for warning in warnings:
            print(f"    • {warning}")
    
    # Test explanation
    print("\nSIGNAL CHAIN EXPLANATION:")
    explanation = intelligence.explain_chain(optimized)
    print(f"  {explanation}")
    
    # Test suggestions
    print("\nSUGGESTIONS for 'warm ambient vocals':")
    suggestions = intelligence.suggest_improvements(optimized, "warm ambient vocals")
    for key, suggestion in suggestions.items():
        print(f"  • {suggestion}")
    
    print("\n✅ Signal Chain Intelligence Ready!")