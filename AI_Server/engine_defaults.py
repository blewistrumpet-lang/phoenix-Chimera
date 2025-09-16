"""
Engine Parameter Defaults for ChimeraPhoenix
Based on PluginProcessor.cpp engine mapping (1:1, ID = index)
Total: 57 engines (0-56)
Generated from plugin source code
"""

ENGINE_DEFAULTS = {
    # 0: ENGINE_NONE
    0: {
        "name": "None",
        "category": "bypass",
        "params": {}  # No processing
    },
    
    # 1: ENGINE_OPTO_COMPRESSOR
    1: {
        "name": "Vintage Opto Compressor",
        "category": "dynamics",
        "params": {
            "param1": {"name": "Threshold", "default": 0.5},
            "param2": {"name": "Ratio", "default": 0.3},
            "param3": {"name": "Attack", "default": 0.01},
            "param4": {"name": "Release", "default": 0.1},
            "param5": {"name": "Makeup", "default": 0.7}
        }
    },
    
    # 2: ENGINE_VCA_COMPRESSOR
    2: {
        "name": "Classic Compressor",
        "category": "dynamics",
        "params": {
            "param1": {"name": "Threshold", "default": 0.5},
            "param2": {"name": "Ratio", "default": 0.3},
            "param3": {"name": "Attack", "default": 0.01},
            "param4": {"name": "Release", "default": 0.1},
            "param5": {"name": "Makeup", "default": 0.7},
            "param6": {"name": "Knee", "default": 0.5}
        }
    },
    
    # 3: ENGINE_TRANSIENT_SHAPER
    3: {
        "name": "Transient Shaper",
        "category": "dynamics",
        "params": {
            "param1": {"name": "Attack", "default": 0.5},
            "param2": {"name": "Sustain", "default": 0.5},
            "param3": {"name": "Output", "default": 0.5}
        }
    },
    
    # 4: ENGINE_NOISE_GATE
    4: {
        "name": "Noise Gate",
        "category": "dynamics",
        "params": {
            "param1": {"name": "Threshold", "default": 0.3},
            "param2": {"name": "Attack", "default": 0.01},
            "param3": {"name": "Hold", "default": 0.1},
            "param4": {"name": "Release", "default": 0.1},
            "param5": {"name": "Range", "default": 1.0}
        }
    },
    
    # 5: ENGINE_MASTERING_LIMITER
    5: {
        "name": "Mastering Limiter",
        "category": "dynamics",
        "params": {
            "param1": {"name": "Ceiling", "default": 0.9},
            "param2": {"name": "Release", "default": 0.01},
            "param3": {"name": "Knee", "default": 0.0}
        }
    },
    
    # 6: ENGINE_DYNAMIC_EQ
    6: {
        "name": "Dynamic EQ",
        "category": "eq",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5},
            "param2": {"name": "Gain", "default": 0.5},
            "param3": {"name": "Q", "default": 0.3},
            "param4": {"name": "Threshold", "default": 0.5}
        }
    },
    
    # 7: ENGINE_PARAMETRIC_EQ
    7: {
        "name": "Parametric EQ",
        "category": "eq",
        "params": {
            "param1": {"name": "Low Freq", "default": 0.3},
            "param2": {"name": "Low Gain", "default": 0.5},
            "param3": {"name": "Mid Freq", "default": 0.5},
            "param4": {"name": "Mid Gain", "default": 0.5},
            "param5": {"name": "Mid Q", "default": 0.5},
            "param6": {"name": "High Freq", "default": 0.7},
            "param7": {"name": "High Gain", "default": 0.5}
        }
    },
    
    # 8: ENGINE_VINTAGE_CONSOLE_EQ
    8: {
        "name": "Vintage Console EQ",
        "category": "eq",
        "params": {
            "param1": {"name": "Low", "default": 0.5},
            "param2": {"name": "Low Mid", "default": 0.5},
            "param3": {"name": "High Mid", "default": 0.5},
            "param4": {"name": "High", "default": 0.5},
            "param5": {"name": "Drive", "default": 0.0}
        }
    },
    
    # 9: ENGINE_LADDER_FILTER
    9: {
        "name": "Ladder Filter",
        "category": "filter",
        "params": {
            "param1": {"name": "Cutoff", "default": 0.5},
            "param2": {"name": "Resonance", "default": 0.3},
            "param3": {"name": "Drive", "default": 0.0},
            "param4": {"name": "Envelope", "default": 0.5}
        }
    },
    
    # 10: ENGINE_STATE_VARIABLE_FILTER
    10: {
        "name": "State Variable Filter",
        "category": "filter",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5},
            "param2": {"name": "Resonance", "default": 0.3},
            "param3": {"name": "Mode", "default": 0.0},
            "param4": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 11: ENGINE_FORMANT_FILTER
    11: {
        "name": "Formant Filter",
        "category": "filter",
        "params": {
            "param1": {"name": "Vowel", "default": 0.5},
            "param2": {"name": "Morph", "default": 0.5},
            "param3": {"name": "Resonance", "default": 0.3}
        }
    },
    
    # 12: ENGINE_ENVELOPE_FILTER
    12: {
        "name": "Envelope Filter",
        "category": "filter",
        "params": {
            "param1": {"name": "Sensitivity", "default": 0.5},
            "param2": {"name": "Frequency", "default": 0.5},
            "param3": {"name": "Resonance", "default": 0.3},
            "param4": {"name": "Attack", "default": 0.1},
            "param5": {"name": "Release", "default": 0.3}
        }
    },
    
    # 13: ENGINE_COMB_RESONATOR
    13: {
        "name": "Comb Resonator",
        "category": "filter",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5},
            "param2": {"name": "Feedback", "default": 0.5},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 14: ENGINE_VOCAL_FORMANT
    14: {
        "name": "Vocal Formant Filter",
        "category": "filter",
        "params": {
            "param1": {"name": "Formant", "default": 0.5},
            "param2": {"name": "Gender", "default": 0.5},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 15: ENGINE_VINTAGE_TUBE
    15: {
        "name": "Vintage Tube Preamp",
        "category": "distortion",
        "params": {
            "param1": {"name": "Drive", "default": 0.3},
            "param2": {"name": "Tone", "default": 0.5},
            "param3": {"name": "Output", "default": 0.7}
        }
    },
    
    # 16: ENGINE_WAVE_FOLDER
    16: {
        "name": "Wave Folder",
        "category": "distortion",
        "params": {
            "param1": {"name": "Fold", "default": 0.5},
            "param2": {"name": "Symmetry", "default": 0.5},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 17: ENGINE_HARMONIC_EXCITER
    17: {
        "name": "Harmonic Exciter",
        "category": "distortion",
        "params": {
            "param1": {"name": "Amount", "default": 0.3},
            "param2": {"name": "Frequency", "default": 0.7},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 18: ENGINE_BIT_CRUSHER
    18: {
        "name": "Bit Crusher",
        "category": "distortion",
        "params": {
            "param1": {"name": "Bit Depth", "default": 0.8},
            "param2": {"name": "Sample Rate", "default": 0.8},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 19: ENGINE_MULTIBAND_SATURATOR
    19: {
        "name": "Multiband Saturator",
        "category": "distortion",
        "params": {
            "param1": {"name": "Low Drive", "default": 0.5},
            "param2": {"name": "Mid Drive", "default": 0.5},
            "param3": {"name": "High Drive", "default": 0.5},
            "param4": {"name": "Output", "default": 0.5}
        }
    },
    
    # 20: ENGINE_MUFF_FUZZ
    20: {
        "name": "Muff Fuzz",
        "category": "distortion",
        "params": {
            "param1": {"name": "Sustain", "default": 0.7},
            "param2": {"name": "Tone", "default": 0.5},
            "param3": {"name": "Volume", "default": 0.5}
        }
    },
    
    # 21: ENGINE_RODENT_DISTORTION
    21: {
        "name": "Rodent Distortion",
        "category": "distortion",
        "params": {
            "param1": {"name": "Distortion", "default": 0.7},
            "param2": {"name": "Filter", "default": 0.5},
            "param3": {"name": "Volume", "default": 0.5}
        }
    },
    
    # 22: ENGINE_K_STYLE
    22: {
        "name": "K-Style Overdrive",
        "category": "distortion",
        "params": {
            "param1": {"name": "Drive", "default": 0.5},
            "param2": {"name": "Tone", "default": 0.5},
            "param3": {"name": "Level", "default": 0.5}
        }
    },
    
    # 23: ENGINE_DIGITAL_CHORUS
    23: {
        "name": "Stereo Chorus",
        "category": "modulation",
        "params": {
            "param1": {"name": "Rate", "default": 0.5},
            "param2": {"name": "Depth", "default": 0.5},
            "param3": {"name": "Mix", "default": 0.5},
            "param4": {"name": "Feedback", "default": 0.0}
        }
    },
    
    # 24: ENGINE_RESONANT_CHORUS
    24: {
        "name": "Resonant Chorus",
        "category": "modulation",
        "params": {
            "param1": {"name": "Rate", "default": 0.5},
            "param2": {"name": "Depth", "default": 0.5},
            "param3": {"name": "Resonance", "default": 0.3},
            "param4": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 25: ENGINE_ANALOG_PHASER
    25: {
        "name": "Analog Phaser",
        "category": "modulation",
        "params": {
            "param1": {"name": "Rate", "default": 0.5},
            "param2": {"name": "Depth", "default": 0.5},
            "param3": {"name": "Feedback", "default": 0.3},
            "param4": {"name": "Stages", "default": 0.5},
            "param5": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 26: ENGINE_RING_MODULATOR
    26: {
        "name": "Ring Modulator",
        "category": "modulation",
        "params": {
            "param1": {"name": "Frequency", "default": 0.5},
            "param2": {"name": "Fine Tune", "default": 0.0},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 27: ENGINE_FREQUENCY_SHIFTER
    27: {
        "name": "Frequency Shifter",
        "category": "modulation",
        "params": {
            "param1": {"name": "Shift", "default": 0.5},
            "param2": {"name": "Fine", "default": 0.0},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 28: ENGINE_HARMONIC_TREMOLO
    28: {
        "name": "Harmonic Tremolo",
        "category": "modulation",
        "params": {
            "param1": {"name": "Rate", "default": 0.5},
            "param2": {"name": "Depth", "default": 0.5},
            "param3": {"name": "Crossover", "default": 0.5}
        }
    },
    
    # 29: ENGINE_CLASSIC_TREMOLO
    29: {
        "name": "Classic Tremolo",
        "category": "modulation",
        "params": {
            "param1": {"name": "Rate", "default": 0.5},
            "param2": {"name": "Depth", "default": 0.5},
            "param3": {"name": "Shape", "default": 0.0}
        }
    },
    
    # 30: ENGINE_ROTARY_SPEAKER
    30: {
        "name": "Rotary Speaker",
        "category": "modulation",
        "params": {
            "param1": {"name": "Speed", "default": 0.5},
            "param2": {"name": "Doppler", "default": 0.5},
            "param3": {"name": "Mix", "default": 0.5},
            "param4": {"name": "Distance", "default": 0.5}
        }
    },
    
    # 31: ENGINE_PITCH_SHIFTER
    31: {
        "name": "Pitch Shifter",
        "category": "pitch",
        "params": {
            "param1": {"name": "Pitch", "default": 0.5},
            "param2": {"name": "Fine", "default": 0.0},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 32: ENGINE_DETUNE_DOUBLER
    32: {
        "name": "Detune Doubler",
        "category": "pitch",
        "params": {
            "param1": {"name": "Detune", "default": 0.3},
            "param2": {"name": "Delay", "default": 0.01},
            "param3": {"name": "Width", "default": 0.5},
            "param4": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 33: ENGINE_INTELLIGENT_HARMONIZER
    33: {
        "name": "Intelligent Harmonizer",
        "category": "pitch",
        "params": {
            "param1": {"name": "Interval", "default": 0.5},
            "param2": {"name": "Key", "default": 0.5},
            "param3": {"name": "Mix", "default": 0.5},
            "param4": {"name": "Feedback", "default": 0.0}
        }
    },
    
    # 34: ENGINE_TAPE_ECHO
    34: {
        "name": "Tape Echo",
        "category": "delay",
        "params": {
            "param1": {"name": "Time", "default": 0.3},
            "param2": {"name": "Feedback", "default": 0.3},
            "param3": {"name": "Mix", "default": 0.5},
            "param4": {"name": "Wow", "default": 0.2},
            "param5": {"name": "Flutter", "default": 0.2}
        }
    },
    
    # 35: ENGINE_DIGITAL_DELAY
    35: {
        "name": "Digital Delay",
        "category": "delay",
        "params": {
            "param1": {"name": "Time", "default": 0.3},
            "param2": {"name": "Feedback", "default": 0.3},
            "param3": {"name": "Mix", "default": 0.5},
            "param4": {"name": "Sync", "default": 0.0}
        }
    },
    
    # 36: ENGINE_MAGNETIC_DRUM_ECHO
    36: {
        "name": "Magnetic Drum Echo",
        "category": "delay",
        "params": {
            "param1": {"name": "Time", "default": 0.3},
            "param2": {"name": "Feedback", "default": 0.3},
            "param3": {"name": "Mix", "default": 0.5},
            "param4": {"name": "Saturation", "default": 0.3}
        }
    },
    
    # 37: ENGINE_BUCKET_BRIGADE_DELAY
    37: {
        "name": "Bucket Brigade Delay",
        "category": "delay",
        "params": {
            "param1": {"name": "Time", "default": 0.3},
            "param2": {"name": "Feedback", "default": 0.3},
            "param3": {"name": "Mix", "default": 0.5},
            "param4": {"name": "Modulation", "default": 0.5}
        }
    },
    
    # 38: ENGINE_BUFFER_REPEAT
    38: {
        "name": "Buffer Repeat",
        "category": "delay",
        "params": {
            "param1": {"name": "Size", "default": 0.5},
            "param2": {"name": "Pitch", "default": 0.0},
            "param3": {"name": "Mix", "default": 1.0}
        }
    },
    
    # 39: ENGINE_PLATE_REVERB
    39: {
        "name": "Plate Reverb",
        "category": "reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5},
            "param2": {"name": "Damping", "default": 0.5},
            "param3": {"name": "Pre-Delay", "default": 0.2},
            "param4": {"name": "Mix", "default": 0.4}
        }
    },
    
    # 40: ENGINE_SPRING_REVERB
    40: {
        "name": "Spring Reverb",
        "category": "reverb",
        "params": {
            "param1": {"name": "Tension", "default": 0.5},
            "param2": {"name": "Damping", "default": 0.5},
            "param3": {"name": "Boing", "default": 0.3},
            "param4": {"name": "Mix", "default": 0.4}
        }
    },
    
    # 41: ENGINE_CONVOLUTION_REVERB
    41: {
        "name": "Convolution Reverb",
        "category": "reverb",
        "params": {
            "param1": {"name": "IR Select", "default": 0.5},
            "param2": {"name": "Size", "default": 0.5},
            "param3": {"name": "Pre-Delay", "default": 0.0},
            "param4": {"name": "Mix", "default": 0.4}
        }
    },
    
    # 42: ENGINE_SHIMMER_REVERB
    42: {
        "name": "Shimmer Reverb",
        "category": "reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.5},
            "param2": {"name": "Shimmer", "default": 0.5},
            "param3": {"name": "Damping", "default": 0.5},
            "param4": {"name": "Mix", "default": 0.4}
        }
    },
    
    # 43: ENGINE_GATED_REVERB
    43: {
        "name": "Gated Reverb",
        "category": "reverb",
        "params": {
            "param1": {"name": "Size", "default": 0.3},
            "param2": {"name": "Gate Time", "default": 0.1},
            "param3": {"name": "Pre-Delay", "default": 0.5},
            "param4": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 44: ENGINE_STEREO_WIDENER
    44: {
        "name": "Stereo Widener",
        "category": "spatial",
        "params": {
            "param1": {"name": "Width", "default": 0.5},
            "param2": {"name": "Bass Mono", "default": 0.5},
            "param3": {"name": "Mix", "default": 1.0}
        }
    },
    
    # 45: ENGINE_STEREO_IMAGER
    45: {
        "name": "Stereo Imager",
        "category": "spatial",
        "params": {
            "param1": {"name": "Width", "default": 0.5},
            "param2": {"name": "Rotation", "default": 0.5},
            "param3": {"name": "Bass Mono", "default": 0.5},
            "param4": {"name": "Mix", "default": 1.0}
        }
    },
    
    # 46: ENGINE_DIMENSION_EXPANDER
    46: {
        "name": "Dimension Expander",
        "category": "spatial",
        "params": {
            "param1": {"name": "Size", "default": 0.5},
            "param2": {"name": "Amount", "default": 0.5},
            "param3": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 47: ENGINE_SPECTRAL_FREEZE
    47: {
        "name": "Spectral Freeze",
        "category": "special",
        "params": {
            "param1": {"name": "Freeze", "default": 0.0},
            "param2": {"name": "Smoothing", "default": 0.5},
            "param3": {"name": "Mix", "default": 1.0}
        }
    },
    
    # 48: ENGINE_SPECTRAL_GATE
    48: {
        "name": "Spectral Gate",
        "category": "special",
        "params": {
            "param1": {"name": "Threshold", "default": 0.5},
            "param2": {"name": "Frequency", "default": 0.5},
            "param3": {"name": "Q", "default": 0.5},
            "param4": {"name": "Mix", "default": 1.0}
        }
    },
    
    # 49: ENGINE_PHASED_VOCODER
    49: {
        "name": "Phased Vocoder",
        "category": "special",
        "params": {
            "param1": {"name": "Formant", "default": 0.5},
            "param2": {"name": "Pitch", "default": 0.5},
            "param3": {"name": "Timbre", "default": 0.5},
            "param4": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 50: ENGINE_GRANULAR_CLOUD
    50: {
        "name": "Granular Cloud",
        "category": "special",
        "params": {
            "param1": {"name": "Grain Size", "default": 0.5},
            "param2": {"name": "Position", "default": 0.5},
            "param3": {"name": "Density", "default": 0.5},
            "param4": {"name": "Pitch", "default": 0.0},
            "param5": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 51: ENGINE_CHAOS_GENERATOR
    51: {
        "name": "Chaos Generator",
        "category": "special",
        "params": {
            "param1": {"name": "Chaos", "default": 0.5},
            "param2": {"name": "Rate", "default": 0.5},
            "param3": {"name": "Smooth", "default": 0.5},
            "param4": {"name": "Mix", "default": 0.3}
        }
    },
    
    # 52: ENGINE_FEEDBACK_NETWORK
    52: {
        "name": "Feedback Network",
        "category": "special",
        "params": {
            "param1": {"name": "Feedback", "default": 0.5},
            "param2": {"name": "Delay", "default": 0.5},
            "param3": {"name": "Filter", "default": 0.5},
            "param4": {"name": "Mix", "default": 0.5}
        }
    },
    
    # 53: ENGINE_MID_SIDE_PROCESSOR
    53: {
        "name": "Mid-Side Processor",
        "category": "utility",
        "params": {
            "param1": {"name": "Mid Level", "default": 0.5},
            "param2": {"name": "Side Level", "default": 0.5},
            "param3": {"name": "Mid EQ", "default": 0.5},
            "param4": {"name": "Side EQ", "default": 0.5}
        }
    },
    
    # 54: ENGINE_GAIN_UTILITY
    54: {
        "name": "Gain Utility",
        "category": "utility",
        "params": {
            "param1": {"name": "Gain", "default": 0.7},
            "param2": {"name": "Pan", "default": 0.5},
            "param3": {"name": "Phase", "default": 0.0}
        }
    },
    
    # 55: ENGINE_MONO_MAKER
    55: {
        "name": "Mono Maker",
        "category": "utility",
        "params": {
            "param1": {"name": "Frequency", "default": 0.2},
            "param2": {"name": "Amount", "default": 1.0}
        }
    },
    
    # 56: ENGINE_PHASE_ALIGN
    56: {
        "name": "Phase Align",
        "category": "utility",
        "params": {
            "param1": {"name": "Delay", "default": 0.0},
            "param2": {"name": "Phase", "default": 0.0},
            "param3": {"name": "Polarity", "default": 0.0}
        }
    }
}

# Helper functions
def get_engine_defaults(engine_id):
    """Get default parameter values for a specific engine"""
    if engine_id in ENGINE_DEFAULTS:
        return ENGINE_DEFAULTS[engine_id]
    return {"name": "Unknown", "params": {}}

def get_engine_name(engine_id):
    """Get the name of an engine by ID"""
    if engine_id in ENGINE_DEFAULTS:
        return ENGINE_DEFAULTS[engine_id]["name"]
    return "Unknown Engine"

def get_engine_category(engine_id):
    """Get the category of an engine by ID"""
    if engine_id in ENGINE_DEFAULTS:
        return ENGINE_DEFAULTS[engine_id].get("category", "unknown")
    return "unknown"

def get_parameter_default(engine_id, param_num):
    """Get the default value for a specific parameter of an engine"""
    if engine_id in ENGINE_DEFAULTS:
        params = ENGINE_DEFAULTS[engine_id].get("params", {})
        param_key = f"param{param_num}"
        if param_key in params:
            return params[param_key].get("default", 0.5)
    return 0.5

def get_engine_id_by_name(name):
    """Find engine ID by its name (case insensitive)"""
    name_lower = name.lower()
    for engine_id, engine_data in ENGINE_DEFAULTS.items():
        if engine_data["name"].lower() == name_lower:
            return engine_id
    return -1  # Not found