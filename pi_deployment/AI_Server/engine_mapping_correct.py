"""
Correct Engine Mapping for Chimera Phoenix
Based on actual parameter_manifest.json
"""

ENGINE_MAPPING = {
    # Dynamics (0-13)
    0: "None",
    1: "Vintage Opto Compressor",
    2: "Classic Compressor",
    3: "Mastering Limiter",
    4: "Noise Gate",
    5: "Transient Shaper",
    6: "Multiband Compressor",
    7: "Vintage VCA",
    8: "Parallel Compressor",
    9: "Tube Limiter",
    10: "Expander",
    11: "Envelope Follower",
    12: "Dynamic EQ",
    13: "Glue Compressor",
    
    # Distortion (14-22)
    14: "Tube Saturation",
    15: "Vintage Tube Preamp",
    16: "Analog Tape Emulation",
    17: "K-Style Overdrive",
    18: "BitCrusher",
    19: "Harmonic Exciter Platinum",
    20: "WaveFolder",
    21: "Fuzz Pedal",
    22: "Vintage Distortion",
    
    # Modulation (23-27)
    23: "Classic Chorus",
    24: "Analog Phaser",
    25: "Vintage Flanger",
    26: "Classic Tremolo",
    27: "Frequency Shifter",
    
    # Filter (28-33)
    28: "Parametric EQ",
    29: "Graphic EQ",
    30: "Vintage EQ",
    31: "Ladder Filter",
    32: "Vocal Formant Filter",
    33: "Envelope Filter",
    
    # Pitch (34-39)
    34: "Simple Pitch Shift",
    35: "Intelligent Harmonizer",
    36: "Formant Shifter",
    37: "Ring Modulator",
    38: "Pitch Correction",
    39: "Vocoder",
    
    # Time-Based (40-46)
    40: "Digital Delay",
    41: "Tape Delay",
    42: "Plate Reverb",
    43: "Spring Reverb",
    44: "Shimmer Reverb",
    45: "Gated Reverb",
    46: "Convolution Reverb",
    
    # Utility (47-51)
    47: "Stereo Imager",
    48: "Auto Panner",
    49: "Gain",
    50: "Phase Rotator",
    51: "Mid/Side Processor",
    
    # Special/Experimental (52-56)
    52: "Dimension Expander",
    53: "Bucket Brigade Delay",
    54: "Spectral Freeze",
    55: "Granular Cloud",
    56: "Chaos Generator"
}

# Reverse mapping for name to ID lookup
ENGINE_NAME_TO_ID = {v.lower(): k for k, v in ENGINE_MAPPING.items()}

# Common aliases
ENGINE_ALIASES = {
    "chaos": 56,
    "spectral": 54,
    "granular": 55,
    "gated": 45,
    "shimmer": 44,
    "plate": 42,
    "spring": 43,
    "bit crusher": 18,
    "bitcrush": 18,
    "harmonizer": 35,
    "formant": 32,
    "vocoder": 39,
    "ring mod": 37,
    "frequency shift": 27,
    "freq shifter": 27,
    "dimension": 52,
    "bucket": 53,
    "tape": 41,
    "digital delay": 40,
    "chorus": 23,
    "phaser": 24,
    "flanger": 25,
    "tremolo": 26
}

def get_engine_id(name: str) -> int:
    """Get engine ID from name or alias"""
    name_lower = name.lower().strip()
    
    # Check aliases first
    for alias, engine_id in ENGINE_ALIASES.items():
        if alias in name_lower:
            return engine_id
    
    # Check full names
    for full_name, engine_id in ENGINE_NAME_TO_ID.items():
        if name_lower in full_name or full_name in name_lower:
            return engine_id
    
    return 0  # Default to None/bypass