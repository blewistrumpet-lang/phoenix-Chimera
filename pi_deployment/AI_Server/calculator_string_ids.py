"""
Calculator component updated to use string engine identifiers
"""

import logging
from typing import Dict, Any, List
from engine_definitions import ENGINES

logger = logging.getLogger(__name__)

class CalculatorStringIDs:
    """
    Calculator that applies intelligent parameter nudges using string IDs
    """
    
    def __init__(self):
        # Define nudge rules based on keywords and engine types
        self.nudge_rules = {
            "warm": {
                "vintage_tube": {"param1": 0.65, "param2": 0.7},  # More drive, warmth
                "vintage_console_eq": {"param1": 0.3, "param3": 0.6},  # Boost lows/mids
                "tape_echo": {"param3": 0.6}  # More saturation
            },
            "bright": {
                "parametric_eq": {"param1": 0.7, "param2": 0.8},  # Boost highs
                "harmonic_exciter": {"param1": 0.7, "param2": 0.6},
                "shimmer_reverb": {"param2": 0.8}
            },
            "aggressive": {
                "rodent_distortion": {"param1": 0.8, "param2": 0.7},
                "noise_gate": {"param1": 0.3, "param2": 0.2},  # Tight gating
                "classic_compressor": {"param1": 0.3, "param2": 0.7}  # High ratio
            },
            "spacious": {
                "plate_reverb": {"param1": 0.7, "param2": 0.8},
                "shimmer_reverb": {"param1": 0.8, "param3": 0.7},
                "dimension_expander": {"param1": 0.8, "param2": 0.7}
            },
            "vintage": {
                "vintage_tube": {"param1": 0.55, "param3": 0.6},
                "vintage_opto": {"param1": 0.4, "param2": 0.6},
                "spring_reverb": {"param1": 0.6, "param2": 0.5}
            },
            "tight": {
                "noise_gate": {"param1": 0.2, "param2": 0.1},
                "classic_compressor": {"param1": 0.2, "param2": 0.8},
                "transient_shaper": {"param1": 0.7, "param2": 0.3}
            },
            "ambient": {
                "granular_cloud": {"param1": 0.7, "param2": 0.8, "param3": 0.6},
                "spectral_freeze": {"param1": 0.6, "param2": 0.7},
                "shimmer_reverb": {"param1": 0.9, "param2": 0.8}
            }
        }
    
    def apply_nudges(self, preset: Dict[str, Any], prompt: str, blueprint: Dict[str, Any]) -> Dict[str, Any]:
        """Apply parameter nudges based on prompt keywords"""
        nudged = preset.copy()
        prompt_lower = prompt.lower()
        applied_nudges = []
        
        # Check each keyword
        for keyword, engine_nudges in self.nudge_rules.items():
            if keyword in prompt_lower:
                # Apply nudges to matching engines in the preset
                for slot in range(1, 7):
                    engine_key = nudged['parameters'].get(f'slot{slot}_engine', 'bypass')
                    
                    if engine_key in engine_nudges:
                        # Apply the nudges for this engine
                        for param, value in engine_nudges[engine_key].items():
                            param_key = f'slot{slot}_{param}'
                            old_value = nudged['parameters'].get(param_key, 0.5)
                            # Blend with existing value (70% nudge, 30% original)
                            nudged['parameters'][param_key] = (0.7 * value + 0.3 * old_value)
                            applied_nudges.append(f"{engine_key}.{param} -> {value}")
                            logger.info(f"Nudged {param_key}: {old_value:.2f} -> {nudged['parameters'][param_key]:.2f}")
        
        # Add metadata about nudges
        nudged['calculator_nudges'] = applied_nudges[:10]  # Limit to 10 for logging
        
        # Adjust mix levels based on intent
        if "subtle" in prompt_lower:
            for slot in range(1, 7):
                mix_key = f'slot{slot}_mix'
                if mix_key in nudged['parameters']:
                    nudged['parameters'][mix_key] *= 0.5
        elif "extreme" in prompt_lower or "heavy" in prompt_lower:
            for slot in range(1, 7):
                mix_key = f'slot{slot}_mix'
                if mix_key in nudged['parameters']:
                    nudged['parameters'][mix_key] = min(1.0, nudged['parameters'][mix_key] * 1.3)
        
        logger.info(f"Applied {len(applied_nudges)} nudges based on keywords")
        return nudged


# Test function
def test_calculator():
    """Test Calculator with string IDs"""
    calc = CalculatorStringIDs()
    
    # Test preset with string IDs
    test_preset = {
        "name": "Test Preset",
        "parameters": {
            "slot1_engine": "vintage_tube",
            "slot1_bypass": 0.0,
            "slot1_mix": 0.8,
            "slot1_param1": 0.5,
            "slot1_param2": 0.5,
            "slot1_param3": 0.5,
            "slot2_engine": "plate_reverb",
            "slot2_bypass": 0.0,
            "slot2_mix": 0.3,
            "slot2_param1": 0.5,
            "slot2_param2": 0.5,
            "slot3_engine": "bypass",
            "slot3_bypass": 1.0
        }
    }
    
    test_blueprint = {
        "overall_vibe": "warm vintage"
    }
    
    print("\n" + "="*80)
    print("TESTING CALCULATOR WITH STRING IDS")
    print("="*80)
    
    prompts = [
        "warm vintage guitar tone",
        "bright spacious pad",
        "aggressive tight metal"
    ]
    
    for prompt in prompts:
        print(f"\nPrompt: {prompt}")
        nudged = calc.apply_nudges(test_preset, prompt, test_blueprint)
        print(f"Applied nudges: {nudged.get('calculator_nudges', [])}")

if __name__ == "__main__":
    test_calculator()