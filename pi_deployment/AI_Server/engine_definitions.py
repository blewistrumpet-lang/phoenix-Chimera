"""
Engine Definitions for ChimeraPhoenix
This is the SINGLE SOURCE OF TRUTH for all engine information.
All components (C++, Python, presets) reference these string identifiers.
"""

# Complete engine registry with string identifiers
ENGINES = {
    "bypass": {
        "name": "Bypass",
        "legacy_id": -1,
        "dropdown_index": 0,
        "category": "Utility",
        "cpu_tier": "none"
    },
    "vintage_tube": {
        "name": "Vintage Tube Preamp",
        "legacy_id": 0,
        "dropdown_index": 48,
        "category": "Saturation",
        "cpu_tier": "light",
        "description": "Warm tube saturation with harmonic enhancement"
    },
    "tape_echo": {
        "name": "Tape Echo",
        "legacy_id": 1,
        "dropdown_index": 2,
        "category": "Delay",
        "cpu_tier": "medium",
        "description": "Analog tape delay with wow and flutter"
    },
    "shimmer_reverb": {
        "name": "Shimmer Reverb",
        "legacy_id": 2,
        "dropdown_index": 29,
        "category": "Reverb",
        "cpu_tier": "heavy",
        "description": "Ethereal reverb with octave-up shimmer"
    },
    "plate_reverb": {
        "name": "Plate Reverb",
        "legacy_id": 3,
        "dropdown_index": 3,
        "category": "Reverb",
        "cpu_tier": "medium",
        "description": "Classic studio plate reverb"
    },
    "convolution_reverb": {
        "name": "Convolution Reverb",
        "legacy_id": 4,
        "dropdown_index": 25,
        "category": "Reverb",
        "cpu_tier": "heavy",
        "description": "Real space convolution reverb"
    },
    "spring_reverb": {
        "name": "Spring Reverb",
        "legacy_id": 5,
        "dropdown_index": 49,
        "category": "Reverb",
        "cpu_tier": "light",
        "description": "Guitar amp spring reverb emulation"
    },
    "vintage_opto": {
        "name": "Vintage Opto",
        "legacy_id": 6,
        "dropdown_index": 42,
        "category": "Dynamics",
        "cpu_tier": "light",
        "description": "Optical compressor with smooth response"
    },
    "classic_compressor": {
        "name": "Classic Compressor",
        "legacy_id": 7,
        "dropdown_index": 15,
        "category": "Dynamics",
        "cpu_tier": "light",
        "description": "VCA-style studio compressor"
    },
    "magnetic_drum_echo": {
        "name": "Magnetic Drum Echo",
        "legacy_id": 8,
        "dropdown_index": 7,
        "category": "Delay",
        "cpu_tier": "medium",
        "description": "Vintage drum echo machine"
    },
    "bucket_brigade": {
        "name": "Bucket Brigade Delay",
        "legacy_id": 9,
        "dropdown_index": 8,
        "category": "Delay",
        "cpu_tier": "medium",
        "description": "Analog BBD delay with character"
    },
    "digital_chorus": {
        "name": "Digital Chorus",
        "legacy_id": 11,
        "dropdown_index": 17,
        "category": "Modulation",
        "cpu_tier": "light",
        "description": "Stereo chorus effect"
    },
    "analog_phaser": {
        "name": "Analog Phaser",
        "legacy_id": 12,
        "dropdown_index": 33,
        "category": "Modulation",
        "cpu_tier": "light",
        "description": "Classic analog phaser"
    },
    "pitch_shifter": {
        "name": "Pitch Shifter",
        "legacy_id": 14,
        "dropdown_index": 23,
        "category": "Pitch",
        "cpu_tier": "heavy",
        "description": "Real-time pitch shifting"
    },
    "ring_modulator": {
        "name": "Analog Ring Modulator",
        "legacy_id": 15,
        "dropdown_index": 20,
        "category": "Modulation",
        "cpu_tier": "light",
        "description": "Ring modulation for metallic tones"
    },
    "granular_cloud": {
        "name": "Granular Cloud",
        "legacy_id": 16,
        "dropdown_index": 19,
        "category": "Texture",
        "cpu_tier": "heavy",
        "description": "Granular synthesis texture generator"
    },
    "vocal_formant": {
        "name": "Vocal Formant Filter",
        "legacy_id": 17,
        "dropdown_index": 30,
        "category": "Filter",
        "cpu_tier": "medium",
        "description": "Vowel formant filter"
    },
    "dimension_expander": {
        "name": "Dimension Expander",
        "legacy_id": 18,
        "dropdown_index": 32,
        "category": "Spatial",
        "cpu_tier": "medium",
        "description": "Stereo width and dimension enhancement"
    },
    "frequency_shifter": {
        "name": "Frequency Shifter",
        "legacy_id": 19,
        "dropdown_index": 27,
        "category": "Modulation",
        "cpu_tier": "medium",
        "description": "Linear frequency shifting"
    },
    "transient_shaper": {
        "name": "Transient Shaper",
        "legacy_id": 20,
        "dropdown_index": 31,
        "category": "Dynamics",
        "cpu_tier": "light",
        "description": "Attack and sustain control"
    },
    "harmonic_tremolo": {
        "name": "Harmonic Tremolo",
        "legacy_id": 21,
        "dropdown_index": 10,
        "category": "Modulation",
        "cpu_tier": "light",
        "description": "Frequency-split tremolo"
    },
    "classic_tremolo": {
        "name": "Classic Tremolo",
        "legacy_id": 22,
        "dropdown_index": 6,
        "category": "Modulation",
        "cpu_tier": "light",
        "description": "Amplitude modulation tremolo"
    },
    "comb_resonator": {
        "name": "Comb Resonator",
        "legacy_id": 23,
        "dropdown_index": 22,
        "category": "Filter",
        "cpu_tier": "light",
        "description": "Tuned comb filter resonator"
    },
    "rotary_speaker": {
        "name": "Rotary Speaker",
        "legacy_id": 24,
        "dropdown_index": 11,
        "category": "Modulation",
        "cpu_tier": "medium",
        "description": "Leslie rotating speaker emulation"
    },
    "mid_side_processor": {
        "name": "Mid/Side Processor",
        "legacy_id": 25,
        "dropdown_index": 47,
        "category": "Spatial",
        "cpu_tier": "light",
        "description": "M/S encoding and processing"
    },
    "vintage_console_eq": {
        "name": "Vintage Console EQ",
        "legacy_id": 26,
        "dropdown_index": 46,
        "category": "EQ",
        "cpu_tier": "light",
        "description": "Classic console channel EQ"
    },
    "parametric_eq": {
        "name": "Parametric EQ",
        "legacy_id": 27,
        "dropdown_index": 39,
        "category": "EQ",
        "cpu_tier": "light",
        "description": "3-band parametric equalizer"
    },
    "ladder_filter": {
        "name": "Ladder Filter",
        "legacy_id": 28,
        "dropdown_index": 13,
        "category": "Filter",
        "cpu_tier": "light",
        "description": "Moog-style ladder filter"
    },
    "state_variable_filter": {
        "name": "State Variable Filter",
        "legacy_id": 29,
        "dropdown_index": 16,
        "category": "Filter",
        "cpu_tier": "light",
        "description": "Multi-mode SVF filter"
    },
    "formant_filter": {
        "name": "Formant Filter",
        "legacy_id": 30,
        "dropdown_index": 14,
        "category": "Filter",
        "cpu_tier": "medium",
        "description": "Vowel formant synthesis"
    },
    "wave_folder": {
        "name": "Wave Folder",
        "legacy_id": 31,
        "dropdown_index": 28,
        "category": "Distortion",
        "cpu_tier": "light",
        "description": "West Coast wave folding"
    },
    "harmonic_exciter": {
        "name": "Harmonic Exciter",
        "legacy_id": 32,
        "dropdown_index": 36,
        "category": "Enhancement",
        "cpu_tier": "light",
        "description": "High frequency harmonic enhancement"
    },
    "bit_crusher": {
        "name": "Bit Crusher",
        "legacy_id": 33,
        "dropdown_index": 26,
        "category": "Distortion",
        "cpu_tier": "light",
        "description": "Sample rate and bit depth reduction"
    },
    "multiband_saturator": {
        "name": "Multiband Saturator",
        "legacy_id": 34,
        "dropdown_index": 21,
        "category": "Saturation",
        "cpu_tier": "medium",
        "description": "Frequency-dependent saturation"
    },
    "muff_fuzz": {
        "name": "Muff Fuzz",
        "legacy_id": 35,
        "dropdown_index": 5,
        "category": "Distortion",
        "cpu_tier": "light",
        "description": "Big Muff style fuzz"
    },
    "rodent_distortion": {
        "name": "Rodent Distortion",
        "legacy_id": 36,
        "dropdown_index": 4,
        "category": "Distortion",
        "cpu_tier": "light",
        "description": "RAT-style distortion pedal"
    },
    "k_style_overdrive": {
        "name": "K-Style Overdrive",
        "legacy_id": 38,
        "dropdown_index": 1,
        "category": "Distortion",
        "cpu_tier": "light",
        "description": "Tube Screamer style overdrive"
    },
    "spectral_freeze": {
        "name": "Spectral Freeze",
        "legacy_id": 39,
        "dropdown_index": 18,
        "category": "Spectral",
        "cpu_tier": "heavy",
        "description": "FFT spectral freezing"
    },
    "buffer_repeat": {
        "name": "Buffer Repeat",
        "legacy_id": 40,
        "dropdown_index": 45,
        "category": "Glitch",
        "cpu_tier": "light",
        "description": "Buffer stutter and repeat"
    },
    "chaos_generator": {
        "name": "Chaos Generator",
        "legacy_id": 41,
        "dropdown_index": 44,
        "category": "Experimental",
        "cpu_tier": "medium",
        "description": "Chaotic modulation generator"
    },
    "intelligent_harmonizer": {
        "name": "Intelligent Harmonizer",
        "legacy_id": 42,
        "dropdown_index": 38,
        "category": "Pitch",
        "cpu_tier": "heavy",
        "description": "Key-aware harmony generation"
    },
    "gated_reverb": {
        "name": "Gated Reverb",
        "legacy_id": 43,
        "dropdown_index": 35,
        "category": "Reverb",
        "cpu_tier": "medium",
        "description": "80s style gated reverb"
    },
    "detune_doubler": {
        "name": "Detune Doubler",
        "legacy_id": 44,
        "dropdown_index": 12,
        "category": "Pitch",
        "cpu_tier": "light",
        "description": "Chorus-like detuned doubling"
    },
    "phased_vocoder": {
        "name": "Phased Vocoder",
        "legacy_id": 45,
        "dropdown_index": 24,
        "category": "Spectral",
        "cpu_tier": "heavy",
        "description": "Phase vocoder processing"
    },
    "spectral_gate": {
        "name": "Spectral Gate",
        "legacy_id": 46,
        "dropdown_index": 43,
        "category": "Dynamics",
        "cpu_tier": "medium",
        "description": "Frequency-dependent gating"
    },
    "noise_gate": {
        "name": "Noise Gate",
        "legacy_id": 47,
        "dropdown_index": 41,
        "category": "Dynamics",
        "cpu_tier": "light",
        "description": "Traditional noise gate"
    },
    "envelope_filter": {
        "name": "Envelope Filter",
        "legacy_id": 48,
        "dropdown_index": 34,
        "category": "Filter",
        "cpu_tier": "light",
        "description": "Auto-wah envelope follower"
    },
    "feedback_network": {
        "name": "Feedback Network",
        "legacy_id": 49,
        "dropdown_index": 37,
        "category": "Experimental",
        "cpu_tier": "medium",
        "description": "Complex feedback routing"
    },
    "mastering_limiter": {
        "name": "Mastering Limiter",
        "legacy_id": 50,
        "dropdown_index": 40,
        "category": "Dynamics",
        "cpu_tier": "medium",
        "description": "Transparent peak limiting"
    },
    "stereo_widener": {
        "name": "Stereo Widener",
        "legacy_id": 51,
        "dropdown_index": 51,
        "category": "Spatial",
        "cpu_tier": "light",
        "description": "Stereo field enhancement"
    },
    "resonant_chorus": {
        "name": "Resonant Chorus",
        "legacy_id": 52,
        "dropdown_index": 50,
        "category": "Modulation",
        "cpu_tier": "medium",
        "description": "Chorus with resonant feedback"
    },
    "digital_delay": {
        "name": "Digital Delay",
        "legacy_id": 53,
        "dropdown_index": 9,
        "category": "Delay",
        "cpu_tier": "light",
        "description": "Clean digital delay line"
    },
    "dynamic_eq": {
        "name": "Dynamic EQ",
        "legacy_id": 54,
        "dropdown_index": 52,
        "category": "EQ",
        "cpu_tier": "medium",
        "description": "Frequency-dependent compression"
    },
    "stereo_imager": {
        "name": "Stereo Imager",
        "legacy_id": 55,
        "dropdown_index": 53,
        "category": "Spatial",
        "cpu_tier": "light",
        "description": "Precise stereo field control"
    }
}

# Helper functions
def get_engine_by_legacy_id(legacy_id: int) -> dict:
    """Get engine info by legacy numeric ID"""
    for engine_key, engine_info in ENGINES.items():
        if engine_info.get("legacy_id") == legacy_id:
            return {"key": engine_key, **engine_info}
    return None

def get_engine_by_dropdown_index(index: int) -> dict:
    """Get engine info by dropdown position"""
    for engine_key, engine_info in ENGINES.items():
        if engine_info.get("dropdown_index") == index:
            return {"key": engine_key, **engine_info}
    return None

def get_engine_key(identifier) -> str:
    """
    Get engine key from various identifiers.
    Can handle: string key, legacy ID, dropdown index
    """
    if isinstance(identifier, str):
        if identifier in ENGINES:
            return identifier
    elif isinstance(identifier, int):
        # Try legacy ID first
        engine = get_engine_by_legacy_id(identifier)
        if engine:
            return engine["key"]
        # Try dropdown index
        engine = get_engine_by_dropdown_index(identifier)
        if engine:
            return engine["key"]
    return None

def validate_engine_key(key: str) -> bool:
    """Check if an engine key is valid"""
    return key in ENGINES

# Categories for grouping
CATEGORIES = {
    "Saturation": ["vintage_tube", "multiband_saturator"],
    "Distortion": ["k_style_overdrive", "rodent_distortion", "muff_fuzz", "wave_folder", "bit_crusher"],
    "Delay": ["tape_echo", "magnetic_drum_echo", "bucket_brigade", "digital_delay"],
    "Reverb": ["plate_reverb", "shimmer_reverb", "convolution_reverb", "spring_reverb", "gated_reverb"],
    "Modulation": ["digital_chorus", "analog_phaser", "classic_tremolo", "harmonic_tremolo", 
                   "rotary_speaker", "detune_doubler", "ring_modulator", "frequency_shifter", "resonant_chorus"],
    "Filter": ["ladder_filter", "state_variable_filter", "formant_filter", "vocal_formant", 
               "comb_resonator", "envelope_filter"],
    "EQ": ["parametric_eq", "vintage_console_eq", "dynamic_eq"],
    "Dynamics": ["classic_compressor", "vintage_opto", "transient_shaper", "noise_gate", 
                 "spectral_gate", "mastering_limiter"],
    "Spatial": ["dimension_expander", "mid_side_processor", "stereo_widener", "stereo_imager"],
    "Pitch": ["pitch_shifter", "intelligent_harmonizer", "detune_doubler"],
    "Spectral": ["spectral_freeze", "phased_vocoder"],
    "Texture": ["granular_cloud"],
    "Enhancement": ["harmonic_exciter"],
    "Glitch": ["buffer_repeat"],
    "Experimental": ["chaos_generator", "feedback_network"],
    "Utility": ["bypass"]
}

# Export lists for convenience
ALL_ENGINE_KEYS = list(ENGINES.keys())
ACTIVE_ENGINE_KEYS = [k for k in ALL_ENGINE_KEYS if k != "bypass"]