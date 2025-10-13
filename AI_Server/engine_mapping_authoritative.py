"""
Engine Mapping Authoritative - Single Source of Truth
This file mirrors JUCE_Plugin/Source/EngineTypes.h exactly
ALL Trinity components must use this mapping
"""

# Engine IDs - Direct mirror of EngineTypes.h
ENGINE_NONE = 0

# DYNAMICS & COMPRESSION (1-6)
ENGINE_OPTO_COMPRESSOR = 1
ENGINE_VCA_COMPRESSOR = 2
ENGINE_TRANSIENT_SHAPER = 3
ENGINE_NOISE_GATE = 4
ENGINE_MASTERING_LIMITER = 5
ENGINE_DYNAMIC_EQ = 6

# FILTERS & EQ (7-14)
ENGINE_PARAMETRIC_EQ = 7
ENGINE_VINTAGE_CONSOLE_EQ = 8
ENGINE_LADDER_FILTER = 9
ENGINE_STATE_VARIABLE_FILTER = 10
ENGINE_FORMANT_FILTER = 11
ENGINE_ENVELOPE_FILTER = 12
ENGINE_COMB_RESONATOR = 13
ENGINE_VOCAL_FORMANT = 14

# DISTORTION & SATURATION (15-22)
ENGINE_VINTAGE_TUBE = 15
ENGINE_WAVE_FOLDER = 16
ENGINE_HARMONIC_EXCITER = 17
ENGINE_BIT_CRUSHER = 18
ENGINE_MULTIBAND_SATURATOR = 19
ENGINE_MUFF_FUZZ = 20
ENGINE_RODENT_DISTORTION = 21
ENGINE_K_STYLE = 22

# MODULATION EFFECTS (23-33)
ENGINE_DIGITAL_CHORUS = 23
ENGINE_RESONANT_CHORUS = 24
ENGINE_ANALOG_PHASER = 25
ENGINE_RING_MODULATOR = 26
ENGINE_FREQUENCY_SHIFTER = 27
ENGINE_HARMONIC_TREMOLO = 28
ENGINE_CLASSIC_TREMOLO = 29
ENGINE_ROTARY_SPEAKER = 30
ENGINE_PITCH_SHIFTER = 31
ENGINE_DETUNE_DOUBLER = 32
ENGINE_INTELLIGENT_HARMONIZER = 33

# REVERB & DELAY (34-43)
ENGINE_TAPE_ECHO = 34
ENGINE_DIGITAL_DELAY = 35
ENGINE_MAGNETIC_DRUM_ECHO = 36
ENGINE_BUCKET_BRIGADE_DELAY = 37
ENGINE_BUFFER_REPEAT = 38
ENGINE_PLATE_REVERB = 39
ENGINE_SPRING_REVERB = 40
ENGINE_CONVOLUTION_REVERB = 41
ENGINE_SHIMMER_REVERB = 42
ENGINE_GATED_REVERB = 43

# SPATIAL & SPECIAL EFFECTS (44-52)
ENGINE_STEREO_WIDENER = 44
ENGINE_STEREO_IMAGER = 45
ENGINE_DIMENSION_EXPANDER = 46
ENGINE_SPECTRAL_FREEZE = 47
ENGINE_SPECTRAL_GATE = 48
ENGINE_PHASED_VOCODER = 49
ENGINE_GRANULAR_CLOUD = 50
ENGINE_CHAOS_GENERATOR = 51
ENGINE_FEEDBACK_NETWORK = 52

# UTILITY (53-56)
ENGINE_MID_SIDE_PROCESSOR = 53
ENGINE_GAIN_UTILITY = 54
ENGINE_MONO_MAKER = 55
ENGINE_PHASE_ALIGN = 56

ENGINE_COUNT = 57

# ID to Name mapping (for display)
ENGINE_NAMES = {
    ENGINE_NONE: "None",
    ENGINE_OPTO_COMPRESSOR: "Vintage Opto Compressor",
    ENGINE_VCA_COMPRESSOR: "Classic Compressor",
    ENGINE_TRANSIENT_SHAPER: "Transient Shaper",
    ENGINE_NOISE_GATE: "Noise Gate",
    ENGINE_MASTERING_LIMITER: "Mastering Limiter",
    ENGINE_DYNAMIC_EQ: "Dynamic EQ",
    ENGINE_PARAMETRIC_EQ: "Parametric EQ",
    ENGINE_VINTAGE_CONSOLE_EQ: "Vintage Console EQ",
    ENGINE_LADDER_FILTER: "Ladder Filter",
    ENGINE_STATE_VARIABLE_FILTER: "State Variable Filter",
    ENGINE_FORMANT_FILTER: "Formant Filter",
    ENGINE_ENVELOPE_FILTER: "Envelope Filter",
    ENGINE_COMB_RESONATOR: "Comb Resonator",
    ENGINE_VOCAL_FORMANT: "Vocal Formant Filter",
    ENGINE_VINTAGE_TUBE: "Vintage Tube Preamp",
    ENGINE_WAVE_FOLDER: "Wave Folder",
    ENGINE_HARMONIC_EXCITER: "Harmonic Exciter",
    ENGINE_BIT_CRUSHER: "Bit Crusher",
    ENGINE_MULTIBAND_SATURATOR: "Multiband Saturator",
    ENGINE_MUFF_FUZZ: "Muff Fuzz",
    ENGINE_RODENT_DISTORTION: "Rodent Distortion",
    ENGINE_K_STYLE: "K-Style Overdrive",
    ENGINE_DIGITAL_CHORUS: "Stereo Chorus",
    ENGINE_RESONANT_CHORUS: "Resonant Chorus",
    ENGINE_ANALOG_PHASER: "Analog Phaser",
    ENGINE_RING_MODULATOR: "Ring Modulator",
    ENGINE_FREQUENCY_SHIFTER: "Frequency Shifter",
    ENGINE_HARMONIC_TREMOLO: "Harmonic Tremolo",
    ENGINE_CLASSIC_TREMOLO: "Classic Tremolo",
    ENGINE_ROTARY_SPEAKER: "Rotary Speaker",
    ENGINE_PITCH_SHIFTER: "Pitch Shifter",
    ENGINE_DETUNE_DOUBLER: "Detune Doubler",
    ENGINE_INTELLIGENT_HARMONIZER: "Intelligent Harmonizer",
    ENGINE_TAPE_ECHO: "Tape Echo",
    ENGINE_DIGITAL_DELAY: "Digital Delay",
    ENGINE_MAGNETIC_DRUM_ECHO: "Magnetic Drum Echo",
    ENGINE_BUCKET_BRIGADE_DELAY: "Bucket Brigade Delay",
    ENGINE_BUFFER_REPEAT: "Buffer Repeat",
    ENGINE_PLATE_REVERB: "Plate Reverb",
    ENGINE_SPRING_REVERB: "Spring Reverb",
    ENGINE_CONVOLUTION_REVERB: "Convolution Reverb",
    ENGINE_SHIMMER_REVERB: "Shimmer Reverb",
    ENGINE_GATED_REVERB: "Gated Reverb",
    ENGINE_STEREO_WIDENER: "Stereo Widener",
    ENGINE_STEREO_IMAGER: "Stereo Imager",
    ENGINE_DIMENSION_EXPANDER: "Dimension Expander",
    ENGINE_SPECTRAL_FREEZE: "Spectral Freeze",
    ENGINE_SPECTRAL_GATE: "Spectral Gate",
    ENGINE_PHASED_VOCODER: "Phased Vocoder",
    ENGINE_GRANULAR_CLOUD: "Granular Cloud",
    ENGINE_CHAOS_GENERATOR: "Chaos Generator",
    ENGINE_FEEDBACK_NETWORK: "Feedback Network",
    ENGINE_MID_SIDE_PROCESSOR: "Mid-Side Processor",
    ENGINE_GAIN_UTILITY: "Gain Utility",
    ENGINE_MONO_MAKER: "Mono Maker",
    ENGINE_PHASE_ALIGN: "Phase Align"
}

# Name to ID mapping (for lookups)
ENGINE_IDS = {v: k for k, v in ENGINE_NAMES.items()}

# Category mappings
DYNAMICS_ENGINES = list(range(1, 7))
FILTER_ENGINES = list(range(7, 15))
DISTORTION_ENGINES = list(range(15, 23))
MODULATION_ENGINES = list(range(23, 34))
DELAY_REVERB_ENGINES = list(range(34, 44))
SPATIAL_ENGINES = list(range(44, 53))
UTILITY_ENGINES = list(range(53, 57))

def get_engine_id(name):
    """Get engine ID from name (case-insensitive)"""
    name_upper = name.upper().replace(" ", "_").replace("-", "_")
    for key, value in ENGINE_NAMES.items():
        if value.upper().replace(" ", "_").replace("-", "_") == name_upper:
            return key
    return ENGINE_NONE

def get_engine_name(engine_id):
    """Get engine name from ID"""
    return ENGINE_NAMES.get(engine_id, "Unknown")

def get_engine_category(engine_id):
    """Get category for an engine ID"""
    if engine_id in DYNAMICS_ENGINES:
        return "Dynamics"
    elif engine_id in FILTER_ENGINES:
        return "Filters & EQ"
    elif engine_id in DISTORTION_ENGINES:
        return "Distortion"
    elif engine_id in MODULATION_ENGINES:
        return "Modulation"
    elif engine_id in DELAY_REVERB_ENGINES:
        return "Delay & Reverb"
    elif engine_id in SPATIAL_ENGINES:
        return "Spatial"
    elif engine_id in UTILITY_ENGINES:
        return "Utility"
    else:
        return "None"

def validate_engine_id(engine_id):
    """Check if engine ID is valid"""
    return 0 <= engine_id < ENGINE_COUNT

def get_all_engines():
    """Get list of all engine IDs and names"""
    return [(id, name) for id, name in ENGINE_NAMES.items() if id != ENGINE_NONE]