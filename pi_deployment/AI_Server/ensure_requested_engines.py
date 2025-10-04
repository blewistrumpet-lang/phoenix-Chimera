#!/usr/bin/env python3
"""
Module to ensure explicitly requested engines are ALWAYS included
"""

from typing import Dict, Any, List
from engine_mapping_authoritative import ENGINE_NAMES
import logging

logger = logging.getLogger(__name__)

def extract_requested_engines(prompt: str) -> List[int]:
    """
    Extract engines that are explicitly mentioned in the prompt
    """
    prompt_lower = prompt.lower()
    requested = []
    
    # Direct engine name mappings
    engine_keywords = {
        # Dynamics (0-5)
        "noise gate": 4,
        "classic compressor": 2,
        "vintage opto": 1,
        "opto compressor": 1,
        "mastering limiter": 5,
        "transient shaper": 3,
        
        # EQ (6-8)
        "parametric eq": 7,
        "vintage console eq": 6,
        "dynamic eq": 8,
        
        # Filters (9-14)
        "ladder filter": 9,
        "state variable filter": 10,
        "vocal formant": 11,
        "envelope filter": 12,
        "resonant filter": 13,
        "high pass filter": 14,
        
        # Distortion (15-22)
        "vintage tube": 15,
        "tube preamp": 15,
        "tube warmth": 15,
        "tape saturation": 16,
        "harmonic exciter": 17,
        "bit crusher": 18,
        "bitcrusher": 18,
        "wave shaper": 19,
        "muff fuzz": 20,
        "rodent distortion": 21,
        "k-style": 22,
        "k style": 22,
        
        # Modulation (23-33)
        "ring modulator": 23,
        "classic chorus": 24,
        "chorus": 24,
        "analog phaser": 25,
        "phaser": 25,
        "vintage flanger": 26,
        "flanger": 26,
        "classic tremolo": 27,
        "tremolo": 27,
        "vintage vibrato": 28,
        "vibrato": 28,
        "leslie": 29,
        "rotary speaker": 29,
        "ensemble": 30,
        "auto-pan": 31,
        "frequency shifter": 32,
        "bode frequency shifter": 33,
        
        # Delay (34-37)
        "tape echo": 34,
        "tape delay": 34,
        "analog delay": 35,
        "digital delay": 36,
        "reverse delay": 37,
        
        # Reverb (38-43)
        "hall reverb": 38,
        "plate reverb": 39,
        "plate": 39,
        "spring reverb": 40,
        "spring": 40,
        "room reverb": 41,
        "convolution reverb": 43,
        "shimmer reverb": 42,
        "shimmer": 42,
        
        # Spatial (44-46)
        "stereo imager": 44,
        "intelligent harmonizer": 45,
        "harmonizer": 45,
        "dimension expander": 46,
        
        # Pitch (47-50)
        "detune doubler": 47,
        "pitch shifter": 48,
        "pitch shift": 48,
        "gender bender": 49,
        "vocal gender": 49,
        "formant shifter": 50,
        
        # Special (51-52)
        "vocoder": 51,
        "chaos generator": 52,
        
        # Utility (53-56)
        "phase align": 53,
        "gain": 54,
        "mid-side encoder": 55,
        "phase rotator": 56,
        
        # Spectral
        "spectral gate": 48,  # Note: ID conflict with pitch shifter
        "gated reverb": 43
    }
    
    # Check each keyword
    for keyword, engine_id in engine_keywords.items():
        if keyword in prompt_lower:
            requested.append(engine_id)
            logger.info(f"Found explicit request for '{keyword}' -> Engine {engine_id}")
    
    # Remove duplicates
    requested = list(set(requested))
    return requested

def ensure_engines_in_preset(preset: Dict[str, Any], required_engines: List[int]) -> Dict[str, Any]:
    """
    Ensure all required engines are in the preset
    """
    if not required_engines:
        return preset
    
    logger.info(f"Ensuring engines {required_engines} are in preset")
    
    # Check which engines are already present
    current_engines = []
    for slot in range(1, 7):
        engine = preset.get(f"slot{slot}_engine", 0)
        if engine > 0:
            current_engines.append(engine)
    
    # Find missing engines
    missing = [e for e in required_engines if e not in current_engines]
    
    if not missing:
        logger.info("All required engines already present")
        return preset
    
    # Add missing engines to empty slots
    for engine_id in missing:
        added = False
        for slot in range(1, 7):
            if preset.get(f"slot{slot}_engine", 0) == 0:
                preset[f"slot{slot}_engine"] = engine_id
                logger.info(f"Added required engine {engine_id} ({ENGINE_NAMES.get(engine_id, 'Unknown')}) to slot {slot}")
                added = True
                break
        
        if not added:
            # No empty slots, replace least important engine
            # Priority: Keep dynamics (gate/comp) and requested engines
            for slot in range(6, 0, -1):  # Start from slot 6
                current = preset.get(f"slot{slot}_engine", 0)
                if current not in required_engines and current not in [1, 2, 4, 5]:  # Not dynamics
                    preset[f"slot{slot}_engine"] = engine_id
                    logger.info(f"Replaced engine in slot {slot} with required engine {engine_id}")
                    break
    
    return preset

if __name__ == "__main__":
    # Test the extraction
    test_prompts = [
        "I want shimmer reverb with plate reverb",
        "vintage tube warmth",
        "bit crusher distortion",
        "classic compressor with parametric eq"
    ]
    
    for prompt in test_prompts:
        print(f"\nPrompt: '{prompt}'")
        engines = extract_requested_engines(prompt)
        print(f"Requested engines: {engines}")
        for e in engines:
            print(f"  - {ENGINE_NAMES.get(e, f'Unknown({e})')}")