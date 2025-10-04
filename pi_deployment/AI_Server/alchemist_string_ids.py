"""
Alchemist component updated to use string engine identifiers
"""

import logging
import random
from typing import Dict, Any, List
from engine_definitions import ENGINES

logger = logging.getLogger(__name__)

class AlchemistStringIDs:
    """
    Alchemist that finalizes presets with creative naming using string IDs
    """
    
    def __init__(self):
        # Creative name components based on engine combinations
        self.name_adjectives = {
            "vintage_tube": ["Velvet", "Warm", "Glowing", "Amber"],
            "tape_echo": ["Echoing", "Nostalgic", "Analog", "Magnetic"],
            "shimmer_reverb": ["Crystalline", "Ethereal", "Shimmering", "Celestial"],
            "plate_reverb": ["Metallic", "Studio", "Classic", "Resonant"],
            "rodent_distortion": ["Fierce", "Aggressive", "Raw", "Brutal"],
            "muff_fuzz": ["Fuzzy", "Thick", "Wall of", "Sustained"],
            "granular_cloud": ["Granular", "Scattered", "Cloudy", "Textural"],
            "chaos_generator": ["Chaotic", "Random", "Wild", "Unpredictable"],
            "vintage_opto": ["Smooth", "Optical", "Gentle", "Musical"],
            "ladder_filter": ["Filtered", "Resonant", "Squelchy", "Synth"],
            "harmonic_exciter": ["Excited", "Bright", "Sparkling", "Enhanced"],
            "dimension_expander": ["Dimensional", "Wide", "Expansive", "3D"],
            "default": ["Custom", "Unique", "Special", "Crafted"]
        }
        
        self.name_nouns = {
            "vintage_tube": ["Thunder", "Warmth", "Glow", "Saturation"],
            "tape_echo": ["Memories", "Echo", "Delay", "Time"],
            "shimmer_reverb": ["Palace", "Heaven", "Dreams", "Space"],
            "plate_reverb": ["Chamber", "Hall", "Room", "Studio"],
            "rodent_distortion": ["Beast", "Fury", "Power", "Force"],
            "muff_fuzz": ["Wall", "Fuzz", "Sound", "Tone"],
            "granular_cloud": ["Cloud", "Texture", "Grain", "Particles"],
            "chaos_generator": ["Storm", "Chaos", "Energy", "Flux"],
            "vintage_opto": ["Compression", "Dynamics", "Control", "Touch"],
            "ladder_filter": ["Filter", "Resonance", "Sweep", "Movement"],
            "harmonic_exciter": ["Harmonics", "Presence", "Air", "Brilliance"],
            "dimension_expander": ["Dimension", "Width", "Space", "Field"],
            "default": ["Preset", "Sound", "Tone", "Effect"]
        }
        
        # Safety limits for parameters
        self.param_limits = {
            "master_input": (0.1, 0.95),
            "master_output": (0.1, 0.95),
            "master_mix": (0.0, 1.0)
        }
    
    def finalize_preset(self, preset: Dict[str, Any], prompt: str = "") -> Dict[str, Any]:
        """Finalize preset with creative naming and safety checks"""
        finalized = preset.copy()
        
        # Generate creative name based on engines
        finalized['name'] = self._generate_creative_name(finalized, prompt)
        
        # Apply safety limits
        finalized = self._apply_safety_limits(finalized)
        
        # Add metadata
        finalized['alchemist_processed'] = True
        finalized['prompt_excerpt'] = prompt[:100] if prompt else ""
        
        # Ensure all required fields exist
        if 'parameters' not in finalized:
            finalized['parameters'] = {}
        
        # Ensure master parameters exist
        if 'master_input' not in finalized['parameters']:
            finalized['parameters']['master_input'] = 0.7
        if 'master_output' not in finalized['parameters']:
            finalized['parameters']['master_output'] = 0.7
        if 'master_mix' not in finalized['parameters']:
            finalized['parameters']['master_mix'] = 1.0
        
        logger.info(f"Alchemist finalized preset: {finalized['name']}")
        return finalized
    
    def _generate_creative_name(self, preset: Dict[str, Any], prompt: str) -> str:
        """Generate a creative preset name based on engines and prompt"""
        # Extract active engines
        active_engines = []
        for slot in range(1, 7):
            engine = preset.get('parameters', {}).get(f'slot{slot}_engine', 'bypass')
            if engine != 'bypass' and engine in ENGINES:
                active_engines.append(engine)
        
        if not active_engines:
            return "Empty Canvas"
        
        # Use the most prominent engine for naming
        primary_engine = active_engines[0]
        
        # Get adjective and noun
        adjectives = self.name_adjectives.get(primary_engine, self.name_adjectives['default'])
        nouns = self.name_nouns.get(primary_engine, self.name_nouns['default'])
        
        # Add variety based on secondary engines
        if len(active_engines) > 1:
            secondary_engine = active_engines[1]
            if secondary_engine in self.name_nouns:
                nouns.extend(self.name_nouns[secondary_engine][:2])
        
        # Generate name
        adjective = random.choice(adjectives)
        noun = random.choice(nouns)
        
        # Sometimes add prompt-based modifier
        prompt_lower = prompt.lower() if prompt else ""
        if "aggressive" in prompt_lower or "metal" in prompt_lower:
            name = f"{adjective} {noun} Assault"
        elif "ambient" in prompt_lower or "space" in prompt_lower:
            name = f"{adjective} {noun} Journey"
        elif "vintage" in prompt_lower or "classic" in prompt_lower:
            name = f"{adjective} {noun} Classic"
        else:
            name = f"{adjective} {noun}"
        
        # Ensure uniqueness with a small random suffix if needed
        if random.random() < 0.2:
            name += f" {random.choice(['II', 'III', 'X', 'Plus', 'Pro'])}"
        
        return name
    
    def _apply_safety_limits(self, preset: Dict[str, Any]) -> Dict[str, Any]:
        """Apply safety limits to prevent extreme parameter values"""
        if 'parameters' not in preset:
            return preset
        
        params = preset['parameters']
        
        # Apply limits to master parameters
        for param, (min_val, max_val) in self.param_limits.items():
            if param in params:
                params[param] = max(min_val, min(max_val, params[param]))
        
        # Ensure no slot has extreme mix values that could cause clipping
        for slot in range(1, 7):
            mix_key = f'slot{slot}_mix'
            if mix_key in params:
                # Check if this is a high-gain effect
                engine = params.get(f'slot{slot}_engine', 'bypass')
                if engine in ['rodent_distortion', 'muff_fuzz', 'chaos_generator']:
                    # Limit mix for high-gain effects
                    params[mix_key] = min(0.7, params[mix_key])
        
        # Prevent multiple high-gain effects at full mix
        high_gain_count = 0
        for slot in range(1, 7):
            engine = params.get(f'slot{slot}_engine', 'bypass')
            if engine in ['rodent_distortion', 'muff_fuzz', 'wave_folder', 'bit_crusher']:
                high_gain_count += 1
                if high_gain_count > 1:
                    # Reduce mix of subsequent high-gain effects
                    mix_key = f'slot{slot}_mix'
                    if mix_key in params:
                        params[mix_key] *= 0.7
        
        return preset


# Test function
def test_alchemist():
    """Test Alchemist with string IDs"""
    alchemist = AlchemistStringIDs()
    
    test_presets = [
        {
            "parameters": {
                "slot1_engine": "vintage_tube",
                "slot2_engine": "tape_echo",
                "slot3_engine": "plate_reverb",
                "slot4_engine": "bypass"
            }
        },
        {
            "parameters": {
                "slot1_engine": "rodent_distortion",
                "slot2_engine": "noise_gate",
                "slot3_engine": "bypass",
                "slot4_engine": "bypass"
            }
        },
        {
            "parameters": {
                "slot1_engine": "granular_cloud",
                "slot2_engine": "shimmer_reverb",
                "slot3_engine": "dimension_expander",
                "slot4_engine": "bypass"
            }
        }
    ]
    
    prompts = [
        "warm vintage guitar tone",
        "aggressive metal sound",
        "spacious ambient pad"
    ]
    
    print("\n" + "="*80)
    print("TESTING ALCHEMIST WITH STRING IDs")
    print("="*80)
    
    for preset, prompt in zip(test_presets, prompts):
        print(f"\nPrompt: {prompt}")
        finalized = alchemist.finalize_preset(preset, prompt)
        print(f"Generated name: {finalized['name']}")
        print(f"Active engines:")
        for slot in range(1, 7):
            engine = finalized['parameters'].get(f'slot{slot}_engine', 'bypass')
            if engine != 'bypass':
                print(f"  Slot {slot}: {engine}")

if __name__ == "__main__":
    test_alchemist()