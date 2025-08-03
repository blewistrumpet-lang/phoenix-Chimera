"""
Engine ID to Choice Index mapping for ChimeraPhoenix plugin
This maps the actual engine IDs (0-55) to their position in the dropdown menu
"""

# Direct mapping from engine ID to choice index
# Based on PluginProcessor.cpp engineIDToChoiceMap
ENGINE_ID_TO_CHOICE = {
    -1: 0,   # ENGINE_BYPASS -> "Bypass" at index 0
    38: 1,   # ENGINE_K_STYLE -> "K-Style Overdrive" at index 1
    1: 2,    # ENGINE_TAPE_ECHO -> "Tape Echo" at index 2
    3: 3,    # ENGINE_PLATE_REVERB -> "Plate Reverb" at index 3
    36: 4,   # ENGINE_RODENT_DISTORTION -> "Rodent Distortion" at index 4
    35: 5,   # ENGINE_MUFF_FUZZ -> "Muff Fuzz" at index 5
    22: 6,   # ENGINE_CLASSIC_TREMOLO -> "Classic Tremolo" at index 6
    8: 7,    # ENGINE_MAGNETIC_DRUM_ECHO -> "Magnetic Drum Echo" at index 7
    9: 8,    # ENGINE_BUCKET_BRIGADE_DELAY -> "Bucket Brigade Delay" at index 8
    53: 9,   # ENGINE_DIGITAL_DELAY -> "Digital Delay" at index 9
    21: 10,  # ENGINE_HARMONIC_TREMOLO -> "Harmonic Tremolo" at index 10
    24: 11,  # ENGINE_ROTARY_SPEAKER -> "Rotary Speaker" at index 11
    44: 12,  # ENGINE_DETUNE_DOUBLER -> "Detune Doubler" at index 12
    28: 13,  # ENGINE_LADDER_FILTER -> "Ladder Filter" at index 13
    30: 14,  # ENGINE_FORMANT_FILTER -> "Formant Filter" at index 14
    7: 15,   # ENGINE_VCA_COMPRESSOR -> "Classic Compressor" at index 15
    29: 16,  # ENGINE_STATE_VARIABLE_FILTER -> "State Variable Filter" at index 16
    11: 17,  # ENGINE_DIGITAL_CHORUS -> "Stereo Chorus" at index 17
    39: 18,  # ENGINE_SPECTRAL_FREEZE -> "Spectral Freeze" at index 18
    16: 19,  # ENGINE_GRANULAR_CLOUD -> "Granular Cloud" at index 19
    15: 20,  # ENGINE_RING_MODULATOR -> "Analog Ring Modulator" at index 20
    34: 21,  # ENGINE_MULTIBAND_SATURATOR -> "Multiband Saturator" at index 21
    23: 22,  # ENGINE_COMB_RESONATOR -> "Comb Resonator" at index 22
    14: 23,  # ENGINE_PITCH_SHIFTER -> "Pitch Shifter" at index 23
    45: 24,  # ENGINE_PHASED_VOCODER -> "Phased Vocoder" at index 24
    4: 25,   # ENGINE_CONVOLUTION_REVERB -> "Convolution Reverb" at index 25
    33: 26,  # ENGINE_BIT_CRUSHER -> "Bit Crusher" at index 26
    19: 27,  # ENGINE_FREQUENCY_SHIFTER -> "Frequency Shifter" at index 27
    31: 28,  # ENGINE_WAVE_FOLDER -> "Wave Folder" at index 28
    2: 29,   # ENGINE_SHIMMER_REVERB -> "Shimmer Reverb" at index 29
    17: 30,  # ENGINE_VOCAL_FORMANT -> "Vocal Formant Filter" at index 30
    20: 31,  # ENGINE_TRANSIENT_SHAPER -> "Transient Shaper" at index 31
    18: 32,  # ENGINE_DIMENSION_EXPANDER -> "Dimension Expander" at index 32
    12: 33,  # ENGINE_ANALOG_PHASER -> "Analog Phaser" at index 33
    48: 34,  # ENGINE_ENVELOPE_FILTER -> "Envelope Filter" at index 34
    43: 35,  # ENGINE_GATED_REVERB -> "Gated Reverb" at index 35
    32: 36,  # ENGINE_HARMONIC_EXCITER -> "Harmonic Exciter" at index 36
    49: 37,  # ENGINE_FEEDBACK_NETWORK -> "Feedback Network" at index 37
    42: 38,  # ENGINE_INTELLIGENT_HARMONIZER -> "Intelligent Harmonizer" at index 38
    27: 39,  # ENGINE_PARAMETRIC_EQ -> "Parametric EQ" at index 39
    50: 40,  # ENGINE_MASTERING_LIMITER -> "Mastering Limiter" at index 40
    47: 41,  # ENGINE_NOISE_GATE -> "Noise Gate" at index 41
    6: 42,   # ENGINE_OPTO_COMPRESSOR -> "Vintage Opto" at index 42
    46: 43,  # ENGINE_SPECTRAL_GATE -> "Spectral Gate" at index 43
    41: 44,  # ENGINE_CHAOS_GENERATOR -> "Chaos Generator" at index 44
    40: 45,  # ENGINE_BUFFER_REPEAT -> "Buffer Repeat" at index 45
    26: 46,  # ENGINE_VINTAGE_CONSOLE_EQ -> "Vintage Console EQ" at index 46
    25: 47,  # ENGINE_MID_SIDE_PROCESSOR -> "Mid/Side Processor" at index 47
    0: 48,   # ENGINE_VINTAGE_TUBE -> "Vintage Tube Preamp" at index 48
    5: 49,   # ENGINE_SPRING_REVERB -> "Spring Reverb" at index 49
    52: 50,  # ENGINE_RESONANT_CHORUS -> "Resonant Chorus" at index 50
    51: 51,  # ENGINE_STEREO_WIDENER -> "Stereo Widener" at index 51
    54: 52,  # ENGINE_DYNAMIC_EQ -> "Dynamic EQ" at index 52
    55: 53   # ENGINE_STEREO_IMAGER -> "Stereo Imager" at index 53
}

# Reverse mapping for convenience
CHOICE_TO_ENGINE_ID = {v: k for k, v in ENGINE_ID_TO_CHOICE.items()}

def engine_id_to_choice_index(engine_id: int) -> int:
    """
    Convert an engine ID (0-55, or -1 for bypass) to its choice index in the dropdown (0-53)
    """
    return ENGINE_ID_TO_CHOICE.get(engine_id, 0)  # Default to bypass if not found

def choice_index_to_engine_id(choice_index: int) -> int:
    """
    Convert a choice index from the dropdown (0-53) to its engine ID (0-55, or -1 for bypass)
    """
    return CHOICE_TO_ENGINE_ID.get(choice_index, -1)  # Default to bypass if not found

def convert_preset_engine_ids(preset: dict) -> dict:
    """
    Convert all engine IDs in a preset to choice indices for the plugin
    """
    if "parameters" not in preset:
        return preset
    
    converted = preset.copy()
    params = converted["parameters"]
    
    # Convert each slot's engine ID to choice index
    for slot in range(1, 7):
        param_name = f"slot{slot}_engine"
        if param_name in params:
            engine_id = int(params[param_name])
            choice_index = engine_id_to_choice_index(engine_id)
            params[param_name] = choice_index
    
    return converted