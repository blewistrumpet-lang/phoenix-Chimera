"""
Complete engine parameter definitions and safe defaults for all 56 engines.
This is used by the Alchemist to ensure all parameters are within safe ranges.
"""

ENGINE_DEFAULTS = {
    # 0: ENGINE_BYPASS
    0: {
        "name": "Bypass",
        "params": {}  # No parameters
    },
    
    # 1: ENGINE_K_STYLE
    1: {
        "name": "K-Style Overdrive",
        "params": {
            "param1": {"name": "Drive", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Output", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 2: ENGINE_TAPE_ECHO
    2: {
        "name": "Tape Echo",
        "params": {
            "param1": {"name": "Time", "default": 0.4, "min": 0.0, "max": 1.0},
            "param2": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.85},  # Limited to prevent runaway
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Wow", "default": 0.2, "min": 0.0, "max": 1.0},
            "param5": {"name": "Flutter", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 3: ENGINE_PLATE_REVERB
    3: {
        "name": "Plate Reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Damping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Width", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 4: ENGINE_RODENT_DISTORTION
    4: {
        "name": "Rodent Distortion",
        "params": {
            "param1": {"name": "Gain", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Filter", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Volume", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mode", "default": 0.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 5: ENGINE_MUFF_FUZZ
    5: {
        "name": "Muff Fuzz",
        "params": {
            "param1": {"name": "Sustain", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Volume", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 6: ENGINE_CLASSIC_TREMOLO
    6: {
        "name": "Classic Tremolo",
        "params": {
            "param1": {"name": "Rate", "default": 0.25, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Waveform", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Stereo", "default": 0.0, "min": 0.0, "max": 1.0},
            "param5": {"name": "Volume", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 7: ENGINE_MAGNETIC_DRUM_ECHO
    7: {
        "name": "Magnetic Drum Echo",
        "params": {
            "param1": {"name": "Time", "default": 0.4, "min": 0.0, "max": 1.0},
            "param2": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.85},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Character", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 8: ENGINE_BUCKET_BRIGADE_DELAY
    8: {
        "name": "Bucket Brigade Delay",
        "params": {
            "param1": {"name": "Time", "default": 0.4, "min": 0.0, "max": 1.0},
            "param2": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.85},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Modulation", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 9: ENGINE_DIGITAL_DELAY
    9: {
        "name": "Digital Delay",
        "params": {
            "param1": {"name": "Time", "default": 0.4, "min": 0.0, "max": 1.0},
            "param2": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.9},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "HP Filter", "default": 0.0, "min": 0.0, "max": 1.0},
            "param5": {"name": "LP Filter", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 10: ENGINE_HARMONIC_TREMOLO
    10: {
        "name": "Harmonic Tremolo",
        "params": {
            "param1": {"name": "Rate", "default": 0.25, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Crossover", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Phase", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 11: ENGINE_ROTARY_SPEAKER
    11: {
        "name": "Rotary Speaker",
        "params": {
            "param1": {"name": "Speed", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Intensity", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Distance", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 12: ENGINE_DETUNE_DOUBLER
    12: {
        "name": "Detune Doubler",
        "params": {
            "param1": {"name": "Detune", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Delay", "default": 0.1, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Width", "default": 0.7, "min": 0.0, "max": 1.0},
        }
    },
    
    # 13: ENGINE_LADDER_FILTER
    13: {
        "name": "Ladder Filter",
        "params": {
            "param1": {"name": "Cutoff", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Resonance", "default": 0.0, "min": 0.0, "max": 0.95},
            "param3": {"name": "Drive", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 14: ENGINE_FORMANT_FILTER
    14: {
        "name": "Formant Filter",
        "params": {
            "param1": {"name": "Vowel", "default": 0.0, "min": 0.0, "max": 1.0},
            "param2": {"name": "Resonance", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 15: ENGINE_CLASSIC_COMPRESSOR
    15: {
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
    
    # 16: ENGINE_STATE_VARIABLE_FILTER
    16: {
        "name": "State Variable Filter",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Resonance", "default": 0.0, "min": 0.0, "max": 0.95},
            "param3": {"name": "Type", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 17: ENGINE_STEREO_CHORUS
    17: {
        "name": "Stereo Chorus",
        "params": {
            "param1": {"name": "Rate", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Feedback", "default": 0.0, "min": 0.0, "max": 0.7},
        }
    },
    
    # 18: ENGINE_SPECTRAL_FREEZE
    18: {
        "name": "Spectral Freeze",
        "params": {
            "param1": {"name": "Freeze", "default": 0.0, "min": 0.0, "max": 1.0},
            "param2": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 19: ENGINE_GRANULAR_CLOUD
    19: {
        "name": "Granular Cloud",
        "params": {
            "param1": {"name": "Grains", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Position", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Pitch", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 20: ENGINE_ANALOG_RING_MODULATOR
    20: {
        "name": "Analog Ring Modulator",
        "params": {
            "param1": {"name": "Frequency", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Amount", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 21: ENGINE_MULTIBAND_SATURATOR
    21: {
        "name": "Multiband Saturator",
        "params": {
            "param1": {"name": "Low Drive", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Mid Drive", "default": 0.2, "min": 0.0, "max": 1.0},
            "param3": {"name": "High Drive", "default": 0.2, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 22: ENGINE_COMB_RESONATOR
    22: {
        "name": "Comb Resonator",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Resonance", "default": 0.5, "min": 0.0, "max": 0.95},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 23: ENGINE_PITCH_SHIFTER
    23: {
        "name": "Pitch Shifter",
        "params": {
            "param1": {"name": "Pitch", "default": 0.5, "min": 0.0, "max": 1.0},  # 0.5 = no shift
            "param2": {"name": "Fine", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 24: ENGINE_PHASED_VOCODER
    24: {
        "name": "Phased Vocoder",
        "params": {
            "param1": {"name": "Bands", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Shift", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Formant", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 25: ENGINE_CONVOLUTION_REVERB
    25: {
        "name": "Convolution Reverb",
        "params": {
            "param1": {"name": "IR Select", "default": 0.0, "min": 0.0, "max": 1.0},
            "param2": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Damping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 26: ENGINE_BIT_CRUSHER
    26: {
        "name": "Bit Crusher",
        "params": {
            "param1": {"name": "Bit Depth", "default": 0.9, "min": 0.0, "max": 1.0},  # Higher = less crushing
            "param2": {"name": "Sample Rate", "default": 0.9, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 27: ENGINE_FREQUENCY_SHIFTER
    27: {
        "name": "Frequency Shifter",
        "params": {
            "param1": {"name": "Shift", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Fine", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 28: ENGINE_WAVE_FOLDER
    28: {
        "name": "Wave Folder",
        "params": {
            "param1": {"name": "Fold", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Asymmetry", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Drive", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 29: ENGINE_SHIMMER_REVERB
    29: {
        "name": "Shimmer Reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Shimmer", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Damping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 30: ENGINE_VOCAL_FORMANT_FILTER
    30: {
        "name": "Vocal Formant Filter",
        "params": {
            "param1": {"name": "Vowel 1", "default": 0.0, "min": 0.0, "max": 1.0},
            "param2": {"name": "Vowel 2", "default": 0.0, "min": 0.0, "max": 1.0},
            "param3": {"name": "Morph", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Resonance", "default": 0.3, "min": 0.0, "max": 0.9},
            "param5": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 31: ENGINE_TRANSIENT_SHAPER
    31: {
        "name": "Transient Shaper",
        "params": {
            "param1": {"name": "Attack", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Sustain", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 32: ENGINE_DIMENSION_EXPANDER
    32: {
        "name": "Dimension Expander",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Width", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 33: ENGINE_ANALOG_PHASER
    33: {
        "name": "Analog Phaser",
        "params": {
            "param1": {"name": "Rate", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.9},
            "param4": {"name": "Mix", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 34: ENGINE_ENVELOPE_FILTER
    34: {
        "name": "Envelope Filter",
        "params": {
            "param1": {"name": "Sensitivity", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Attack", "default": 0.1, "min": 0.0, "max": 1.0},
            "param3": {"name": "Release", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Range", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Mix", "default": 1.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 35: ENGINE_GATED_REVERB
    35: {
        "name": "Gated Reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Gate Time", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Damping", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 36: ENGINE_HARMONIC_EXCITER
    36: {
        "name": "Harmonic Exciter",
        "params": {
            "param1": {"name": "Harmonics", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Frequency", "default": 0.7, "min": 0.0, "max": 1.0},
            "param3": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 37: ENGINE_FEEDBACK_NETWORK
    37: {
        "name": "Feedback Network",
        "params": {
            "param1": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.85},
            "param2": {"name": "Delay", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Modulation", "default": 0.2, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.2, "min": 0.0, "max": 1.0},
        }
    },
    
    # 38: ENGINE_INTELLIGENT_HARMONIZER
    38: {
        "name": "Intelligent Harmonizer",
        "params": {
            "param1": {"name": "Interval", "default": 0.0, "min": 0.0, "max": 1.0},
            "param2": {"name": "Key", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Scale", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 39: ENGINE_PARAMETRIC_EQ
    39: {
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
    
    # 40: ENGINE_MASTERING_LIMITER
    40: {
        "name": "Mastering Limiter",
        "params": {
            "param1": {"name": "Threshold", "default": 0.9, "min": 0.0, "max": 1.0},
            "param2": {"name": "Release", "default": 0.2, "min": 0.0, "max": 1.0},
            "param3": {"name": "Knee", "default": 0.0, "min": 0.0, "max": 1.0},
            "param4": {"name": "Lookahead", "default": 0.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 41: ENGINE_NOISE_GATE
    41: {
        "name": "Noise Gate",
        "params": {
            "param1": {"name": "Threshold", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Attack", "default": 0.1, "min": 0.0, "max": 1.0},
            "param3": {"name": "Hold", "default": 0.3, "min": 0.0, "max": 1.0},
            "param4": {"name": "Release", "default": 0.4, "min": 0.0, "max": 1.0},
            "param5": {"name": "Range", "default": 0.8, "min": 0.0, "max": 1.0},
        }
    },
    
    # 42: ENGINE_VINTAGE_OPTO_COMPRESSOR
    42: {
        "name": "Vintage Opto Compressor",
        "params": {
            "param1": {"name": "Threshold", "default": 0.7, "min": 0.0, "max": 1.0},
            "param2": {"name": "Ratio", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Speed", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Makeup", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 43: ENGINE_SPECTRAL_GATE
    43: {
        "name": "Spectral Gate",
        "params": {
            "param1": {"name": "Threshold", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Frequency", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Q", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 44: ENGINE_CHAOS_GENERATOR
    44: {
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
    
    # 45: ENGINE_BUFFER_REPEAT
    45: {
        "name": "Buffer Repeat",
        "params": {
            "param1": {"name": "Size", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Rate", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Feedback", "default": 0.3, "min": 0.0, "max": 0.85},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 46: ENGINE_VINTAGE_CONSOLE_EQ
    46: {
        "name": "Vintage Console EQ",
        "params": {
            "param1": {"name": "Low", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Low Mid", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "High Mid", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "High", "default": 0.5, "min": 0.0, "max": 1.0},
            "param5": {"name": "Drive", "default": 0.0, "min": 0.0, "max": 1.0},
        }
    },
    
    # 47: ENGINE_MID_SIDE_PROCESSOR
    47: {
        "name": "Mid/Side Processor",
        "params": {
            "param1": {"name": "Mid Level", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Side Level", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Width", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 48: ENGINE_VINTAGE_TUBE_PREAMP
    48: {
        "name": "Vintage Tube Preamp",
        "params": {
            "param1": {"name": "Drive", "default": 0.3, "min": 0.0, "max": 1.0},
            "param2": {"name": "Bias", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Output", "default": 0.5, "min": 0.0, "max": 1.0},
        }
    },
    
    # 49: ENGINE_SPRING_REVERB
    49: {
        "name": "Spring Reverb",
        "params": {
            "param1": {"name": "Springs", "default": 0.5, "min": 0.0, "max": 1.0},
            "param2": {"name": "Decay", "default": 0.5, "min": 0.0, "max": 1.0},
            "param3": {"name": "Tone", "default": 0.5, "min": 0.0, "max": 1.0},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
        }
    },
    
    # 50: ENGINE_RESONANT_CHORUS
    50: {
        "name": "Resonant Chorus",
        "params": {
            "param1": {"name": "Rate", "default": 0.2, "min": 0.0, "max": 1.0},
            "param2": {"name": "Depth", "default": 0.3, "min": 0.0, "max": 1.0},
            "param3": {"name": "Resonance", "default": 0.3, "min": 0.0, "max": 0.9},
            "param4": {"name": "Mix", "default": 0.3, "min": 0.0, "max": 1.0},
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
    
    # 52: Not used (was ENGINE_INTELLIGENT_COMPRESSOR)
    52: {
        "name": "Reserved",
        "params": {}
    },
    
    # 53: Not used
    53: {
        "name": "Reserved",
        "params": {}
    },
    
    # 54: ENGINE_DYNAMIC_EQ
    54: {
        "name": "Dynamic EQ",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5, "min": 0.0, "max": 1.0},
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
}