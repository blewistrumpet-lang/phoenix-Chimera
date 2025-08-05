"""
Generated Engine Parameter Defaults for ChimeraPhoenix
Generated from parameter_database.json on 2025-08-04 02:06:38
DO NOT EDIT MANUALLY - Edit parameter_database.json and regenerate
"""

ENGINE_DEFAULTS = {
    # BYPASS (special case, no parameters)
    # Note: Bypass is handled specially in the plugin
    
    # 0: ENGINE_VINTAGE_TUBE
    0: {
        "name": "Vintage Tube Preamp",
        "params": {
            "param1": {"name": "Input Gain", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Drive", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Bias", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Bass", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mid", "default": 0.5, "min": 0.0, "max": 1.0},
            "param6": {"name": "Treble", "default": 0.5, "min": 0.0, "max": 1.0},
            "param7": {"name": "Presence", "default": 0.5, "min": 0.0, "max": 1.0},
            "param8": {"name": "Output Gain", "default": 0.5, "min": 0.0, "max": 1.0},
            "param9": {"name": "Tube Type", "default": 0.0, "min": 0.0, "max": 1.0},
            "param10": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 1: ENGINE_TAPE_ECHO
    1: {
        "name": "Tape Echo",
        "params": {
            "param1": {"name": "Time", "default": 0.375, "min": 0.0, "max": 1.0},
            "param2": {"name": "Feedback", "default": 0.35, "min": 0.0, "max": 1.0},
            "param3": {"name": "Wow & Flutter", "default": 0.25, "min": 0.0, "max": 1.0},
            "param4": {"name": "Saturation", "default": 0.3, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mix", "default": 0.35, "min": 0.0, "max": 1.0},
        }
    },
    
    # 2: ENGINE_SHIMMER_REVERB
    2: {
        "name": "Shimmer Reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Shimmer", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Damping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 3: ENGINE_PLATE_REVERB
    3: {
        "name": "Plate Reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Damping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Predelay", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 5: ENGINE_SPRING_REVERB
    5: {
        "name": "Spring Reverb",
        "params": {
            "param1": {"name": "Springs", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Decay", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 6: ENGINE_OPTO_COMPRESSOR
    6: {
        "name": "Vintage Opto",
        "params": {
            "param1": {"name": "Threshold", "default": 0.7, "min": 0.0, "max": 1.0},
            "param2": {"name": "Ratio", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Speed", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Makeup", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 7: ENGINE_VCA_COMPRESSOR
    7: {
        "name": "Classic Compressor",
        "params": {
            "param1": {"name": "Threshold", "default": 0.7, "min": 0.0, "max": 1.0},
            "param2": {"name": "Ratio", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Attack", "default": 0.2, "min": 0.0, "max": 1.0},
            "param4": {"name": "Release", "default": 0.4, "min": 0.0, "max": 1.0},
            "param5": {"name": "Knee", "default": 0.0, "min": 0.0, "max": 1.0},
            "param6": {"name": "Makeup", "default": 0.5, "min": 0.0, "max": 1.0},
            "param7": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 11: ENGINE_DIGITAL_CHORUS
    11: {
        "name": "Stereo Chorus",
        "params": {
            "param1": {"name": "Rate", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Feedback", "default": 0.0, "min": 0.0, "max": 0.7},
        }
    },
    
    # 14: ENGINE_PITCH_SHIFTER
    14: {
        "name": "Pitch Shifter",
        "params": {
            "param1": {"name": "Pitch", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Fine", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 16: ENGINE_GRANULAR_CLOUD
    16: {
        "name": "Granular Cloud",
        "params": {
            "param1": {"name": "Grains", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Position", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Pitch", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 18: ENGINE_DIMENSION_EXPANDER
    18: {
        "name": "Dimension Expander",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Width", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 20: ENGINE_TRANSIENT_SHAPER
    20: {
        "name": "Transient Shaper",
        "params": {
            "param1": {"name": "Attack", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Sustain", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 21: ENGINE_HARMONIC_TREMOLO
    21: {
        "name": "Harmonic Tremolo",
        "params": {
            "param1": {"name": "Rate", "default": 0.25, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Harmonics", "default": 0.4, "min": 0.0, "max": 1.0},
            "param4": {"name": "Stereo Phase", "default": 0.25, "min": 0.0, "max": 1.0},
        }
    },
    
    # 22: ENGINE_CLASSIC_TREMOLO
    22: {
        "name": "Classic Tremolo",
        "params": {
            "param1": {"name": "Rate", "default": 0.25, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Shape", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Stereo", "default": 0.0, "min": 0.0, "max": 1.0},
            "param5": {"name": "Type", "default": 0.0, "min": 0.0, "max": 1.0},
            "param6": {"name": "Symmetry", "default": 0.5, "min": 0.0, "max": 1.0},
            "param7": {"name": "Volume", "default": 1.0, "min": 0.0, "max": 1.0},
            "param8": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 23: ENGINE_COMB_RESONATOR
    23: {
        "name": "Comb Resonator",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Resonance", "default": 0.5, "min": 0.0, "max": 0.95},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 25: ENGINE_MID_SIDE_PROCESSOR
    25: {
        "name": "Mid/Side Processor",
        "params": {
            "param1": {"name": "Mid Level", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Side Level", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Width", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 26: ENGINE_VINTAGE_CONSOLE_EQ
    26: {
        "name": "Vintage Console EQ",
        "params": {
            "param1": {"name": "Low", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Low Mid", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "High Mid", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "High", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Drive", "default": 0.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 27: ENGINE_PARAMETRIC_EQ
    27: {
        "name": "Parametric EQ",
        "params": {
            "param1": {"name": "Freq 1", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Gain 1", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Q 1", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Freq 2", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Gain 2", "default": 0.5, "min": 0.0, "max": 1.0},
            "param6": {"name": "Q 2", "default": 0.5, "min": 0.0, "max": 1.0},
            "param7": {"name": "Freq 3", "default": 0.8, "min": 0.0, "max": 1.0},
            "param8": {"name": "Gain 3", "default": 0.5, "min": 0.0, "max": 1.0},
            "param9": {"name": "Q 3", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 32: ENGINE_HARMONIC_EXCITER
    32: {
        "name": "Harmonic Exciter",
        "params": {
            "param1": {"name": "Harmonics", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Frequency", "default": 0.7, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 35: ENGINE_MUFF_FUZZ
    35: {
        "name": "Muff Fuzz",
        "params": {
            "param1": {"name": "Sustain", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Volume", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Gate", "default": 0.0, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mids", "default": 0.0, "min": 0.0, "max": 1.0},
            "param6": {"name": "Variant", "default": 0.0, "min": 0.0, "max": 1.0},
            "param7": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 36: ENGINE_RODENT_DISTORTION
    36: {
        "name": "Rodent Distortion",
        "params": {
            "param1": {"name": "Gain", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Filter", "default": 0.4, "min": 0.0, "max": 1.0},
            "param3": {"name": "Clipping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Output", "default": 0.5, "min": 0.0, "max": 1.0},
            "param6": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
            "param7": {"name": "Mode", "default": 0.0, "min": 0.0, "max": 1.0},
            "param8": {"name": "Presence", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 38: ENGINE_K_STYLE
    38: {
        "name": "K-Style Overdrive",
        "params": {
            "param1": {"name": "Drive", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Level", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 39: ENGINE_SPECTRAL_FREEZE
    39: {
        "name": "Spectral Freeze",
        "params": {
            "param1": {"name": "Freeze", "default": 0.0, "min": 0.0, "max": 1.0},
            "param2": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 40: ENGINE_BUFFER_REPEAT
    40: {
        "name": "Buffer Repeat",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Rate", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.85},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 41: ENGINE_CHAOS_GENERATOR
    41: {
        "name": "Chaos Generator",
        "params": {
            "param1": {"name": "Rate", "default": 0.1, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.1, "min": 0.0, "max": 1.0},
            "param3": {"name": "Type", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Smoothing", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Target", "default": 0.0, "min": 0.0, "max": 1.0},
            "param6": {"name": "Sync", "default": 0.0, "min": 0.0, "max": 1.0},
            "param7": {"name": "Seed", "default": 0.5, "min": 0.0, "max": 1.0},
            "param8": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 42: ENGINE_INTELLIGENT_HARMONIZER
    42: {
        "name": "Intelligent Harmonizer",
        "params": {
            "param1": {"name": "Interval", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Key", "default": 0.0, "min": 0.0, "max": 1.0},
            "param3": {"name": "Scale", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Voices", "default": 0.0, "min": 0.0, "max": 1.0},
            "param5": {"name": "Spread", "default": 0.3, "min": 0.0, "max": 1.0},
            "param6": {"name": "Humanize", "default": 0.0, "min": 0.0, "max": 1.0},
            "param7": {"name": "Formant", "default": 0.0, "min": 0.0, "max": 1.0},
            "param8": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 43: ENGINE_GATED_REVERB
    43: {
        "name": "Gated Reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Gate Time", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Damping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 44: ENGINE_DETUNE_DOUBLER
    44: {
        "name": "Detune Doubler",
        "params": {
            "param1": {"name": "Detune Amount", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Delay Time", "default": 0.15, "min": 0.0, "max": 1.0},
            "param3": {"name": "Stereo Width", "default": 0.7, "min": 0.0, "max": 1.0},
            "param4": {"name": "Thickness", "default": 0.3, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 45: ENGINE_PHASED_VOCODER
    45: {
        "name": "Phased Vocoder",
        "params": {
            "param1": {"name": "Bands", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Shift", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Formant", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 46: ENGINE_SPECTRAL_GATE
    46: {
        "name": "Spectral Gate",
        "params": {
            "param1": {"name": "Threshold", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Frequency", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Q", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 47: ENGINE_NOISE_GATE
    47: {
        "name": "Noise Gate",
        "params": {
            "param1": {"name": "Threshold", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Attack", "default": 0.1, "min": 0.0, "max": 1.0},
            "param3": {"name": "Hold", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Release", "default": 0.4, "min": 0.0, "max": 1.0},
            "param5": {"name": "Range", "default": 0.8, "min": 0.0, "max": 1.0},
        }
    },
    
    # 48: ENGINE_ENVELOPE_FILTER
    48: {
        "name": "Envelope Filter",
        "params": {
            "param1": {"name": "Sensitivity", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Attack", "default": 0.1, "min": 0.0, "max": 1.0},
            "param3": {"name": "Release", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Range", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 49: ENGINE_FEEDBACK_NETWORK
    49: {
        "name": "Feedback Network",
        "params": {
            "param1": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.85},
            "param2": {"name": "Delay", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Modulation", "default": 0.2, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 50: ENGINE_MASTERING_LIMITER
    50: {
        "name": "Mastering Limiter",
        "params": {
            "param1": {"name": "Threshold", "default": 0.9, "min": 0.0, "max": 1.0},
            "param2": {"name": "Release", "default": 0.2, "min": 0.0, "max": 1.0},
            "param3": {"name": "Knee", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Lookahead", "default": 0.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 51: ENGINE_STEREO_WIDENER
    51: {
        "name": "Stereo Widener",
        "params": {
            "param1": {"name": "Width", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Bass Mono", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 52: ENGINE_RESONANT_CHORUS
    52: {
        "name": "Resonant Chorus",
        "params": {
            "param1": {"name": "Rate", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Resonance", "default": 0.3, "min": 0.0, "max": 0.9},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 53: ENGINE_DIGITAL_DELAY
    53: {
        "name": "Digital Delay",
        "params": {
            "param1": {"name": "Time", "default": 0.4, "min": 0.0, "max": 1.0},
            "param2": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.9},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "High Cut", "default": 0.8, "min": 0.0, "max": 1.0},
        }
    },
    
    # 54: ENGINE_DYNAMIC_EQ
    54: {
        "name": "Dynamic EQ",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5, "min": 0.0, "max": 0.9},
            "param2": {"name": "Threshold", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Ratio", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Attack", "default": 0.2, "min": 0.0, "max": 1.0},
            "param5": {"name": "Release", "default": 0.4, "min": 0.0, "max": 1.0},
            "param6": {"name": "Gain", "default": 0.5, "min": 0.0, "max": 1.0},
            "param7": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
            "param8": {"name": "Mode", "default": 0.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 55: ENGINE_STEREO_IMAGER
    55: {
        "name": "Stereo Imager",
        "params": {
            "param1": {"name": "Width", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Center", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Rotation", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 56: ENGINE_ROTARY_SPEAKER
    56: {
        "name": "Rotary Speaker",
        "params": {
            "param1": {"name": "Speed", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Acceleration", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Drive", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mic Distance", "default": 0.6, "min": 0.0, "max": 1.0},
            "param5": {"name": "Stereo Width", "default": 0.8, "min": 0.0, "max": 1.0},
            "param6": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 57: ENGINE_LADDER_FILTER
    57: {
        "name": "Ladder Filter Pro",
        "params": {
            "param1": {"name": "Cutoff", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Resonance", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Drive", "default": 0.2, "min": 0.0, "max": 1.0},
            "param4": {"name": "Filter Type", "default": 0.0, "min": 0.0, "max": 1.0},
            "param5": {"name": "Asymmetry", "default": 0.0, "min": 0.0, "max": 1.0},
            "param6": {"name": "Vintage Mode", "default": 0.0, "min": 0.0, "max": 1.0},
            "param7": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
}
