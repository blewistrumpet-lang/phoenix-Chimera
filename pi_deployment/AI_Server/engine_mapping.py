"""
Engine ID to Choice Index mapping for ChimeraPhoenix plugin
Direct 1:1 mapping where engine ID equals dropdown index (0-56)
Updated to match the plugin's actual implementation
"""

# Direct 1:1 mapping where engine ID = dropdown index (0-56)
# Updated to match PluginProcessor.cpp engineIDToChoiceMap
ENGINE_ID_TO_CHOICE = {
    0: 0,    # ENGINE_NONE -> "None" at index 0
    1: 1,    # ENGINE_OPTO_COMPRESSOR -> "Vintage Opto Compressor" at index 1
    2: 2,    # ENGINE_VCA_COMPRESSOR -> "Classic Compressor" at index 2
    3: 3,    # ENGINE_TRANSIENT_SHAPER -> "Transient Shaper" at index 3
    4: 4,    # ENGINE_NOISE_GATE -> "Noise Gate" at index 4
    5: 5,    # ENGINE_MASTERING_LIMITER -> "Mastering Limiter" at index 5
    6: 6,    # ENGINE_DYNAMIC_EQ -> "Dynamic EQ" at index 6
    7: 7,    # ENGINE_PARAMETRIC_EQ -> "Parametric EQ" at index 7
    8: 8,    # ENGINE_VINTAGE_CONSOLE_EQ -> "Vintage Console EQ" at index 8
    9: 9,    # ENGINE_LADDER_FILTER -> "Ladder Filter" at index 9
    10: 10,  # ENGINE_STATE_VARIABLE_FILTER -> "State Variable Filter" at index 10
    11: 11,  # ENGINE_FORMANT_FILTER -> "Formant Filter" at index 11
    12: 12,  # ENGINE_ENVELOPE_FILTER -> "Envelope Filter" at index 12
    13: 13,  # ENGINE_COMB_RESONATOR -> "Comb Resonator" at index 13
    14: 14,  # ENGINE_VOCAL_FORMANT -> "Vocal Formant Filter" at index 14
    15: 15,  # ENGINE_VINTAGE_TUBE -> "Vintage Tube Preamp" at index 15
    16: 16,  # ENGINE_WAVE_FOLDER -> "Wave Folder" at index 16
    17: 17,  # ENGINE_HARMONIC_EXCITER -> "Harmonic Exciter" at index 17
    18: 18,  # ENGINE_BIT_CRUSHER -> "Bit Crusher" at index 18
    19: 19,  # ENGINE_MULTIBAND_SATURATOR -> "Multiband Saturator" at index 19
    20: 20,  # ENGINE_MUFF_FUZZ -> "Muff Fuzz" at index 20
    21: 21,  # ENGINE_RODENT_DISTORTION -> "Rodent Distortion" at index 21
    22: 22,  # ENGINE_K_STYLE -> "K-Style Overdrive" at index 22
    23: 23,  # ENGINE_DIGITAL_CHORUS -> "Stereo Chorus" at index 23
    24: 24,  # ENGINE_RESONANT_CHORUS -> "Resonant Chorus" at index 24
    25: 25,  # ENGINE_ANALOG_PHASER -> "Analog Phaser" at index 25
    26: 26,  # ENGINE_RING_MODULATOR -> "Ring Modulator" at index 26
    27: 27,  # ENGINE_FREQUENCY_SHIFTER -> "Frequency Shifter" at index 27
    28: 28,  # ENGINE_HARMONIC_TREMOLO -> "Harmonic Tremolo" at index 28
    29: 29,  # ENGINE_CLASSIC_TREMOLO -> "Classic Tremolo" at index 29
    30: 30,  # ENGINE_ROTARY_SPEAKER -> "Rotary Speaker" at index 30
    31: 31,  # ENGINE_PITCH_SHIFTER -> "Pitch Shifter" at index 31
    32: 32,  # ENGINE_DETUNE_DOUBLER -> "Detune Doubler" at index 32
    33: 33,  # ENGINE_INTELLIGENT_HARMONIZER -> "Intelligent Harmonizer" at index 33
    34: 34,  # ENGINE_TAPE_ECHO -> "Tape Echo" at index 34
    35: 35,  # ENGINE_DIGITAL_DELAY -> "Digital Delay" at index 35
    36: 36,  # ENGINE_MAGNETIC_DRUM_ECHO -> "Magnetic Drum Echo" at index 36
    37: 37,  # ENGINE_BUCKET_BRIGADE_DELAY -> "Bucket Brigade Delay" at index 37
    38: 38,  # ENGINE_BUFFER_REPEAT -> "Buffer Repeat" at index 38
    39: 39,  # ENGINE_PLATE_REVERB -> "Plate Reverb" at index 39
    40: 40,  # ENGINE_SPRING_REVERB -> "Spring Reverb" at index 40
    41: 41,  # ENGINE_CONVOLUTION_REVERB -> "Convolution Reverb" at index 41
    42: 42,  # ENGINE_SHIMMER_REVERB -> "Shimmer Reverb" at index 42
    43: 43,  # ENGINE_GATED_REVERB -> "Gated Reverb" at index 43
    44: 44,  # ENGINE_STEREO_WIDENER -> "Stereo Widener" at index 44
    45: 45,  # ENGINE_STEREO_IMAGER -> "Stereo Imager" at index 45
    46: 46,  # ENGINE_DIMENSION_EXPANDER -> "Dimension Expander" at index 46
    47: 47,  # ENGINE_SPECTRAL_FREEZE -> "Spectral Freeze" at index 47
    48: 48,  # ENGINE_SPECTRAL_GATE -> "Spectral Gate" at index 48
    49: 49,  # ENGINE_PHASED_VOCODER -> "Phased Vocoder" at index 49
    50: 50,  # ENGINE_GRANULAR_CLOUD -> "Granular Cloud" at index 50
    51: 51,  # ENGINE_CHAOS_GENERATOR -> "Chaos Generator" at index 51
    52: 52,  # ENGINE_FEEDBACK_NETWORK -> "Feedback Network" at index 52
    53: 53,  # ENGINE_MID_SIDE_PROCESSOR -> "Mid-Side Processor" at index 53
    54: 54,  # ENGINE_GAIN_UTILITY -> "Gain Utility" at index 54
    55: 55,  # ENGINE_MONO_MAKER -> "Mono Maker" at index 55
    56: 56   # ENGINE_PHASE_ALIGN -> "Phase Align" at index 56
}

# Reverse mapping for convenience
CHOICE_TO_ENGINE_ID = {v: k for k, v in ENGINE_ID_TO_CHOICE.items()}

def engine_id_to_choice_index(engine_id: int) -> int:
    """
    Convert an engine ID to its choice index in the dropdown
    With 1:1 mapping, this is essentially a pass-through with validation
    """
    # Direct 1:1 mapping - ID equals index
    if 0 <= engine_id <= 56:
        return engine_id
    return 0  # Default to "None" if out of range

def choice_index_to_engine_id(choice_index: int) -> int:
    """
    Convert a choice index from the dropdown to its engine ID
    With 1:1 mapping, this is essentially a pass-through with validation
    """
    # Direct 1:1 mapping - index equals ID
    if 0 <= choice_index <= 56:
        return choice_index
    return 0  # Default to "None" if out of range

def convert_preset_engine_ids(preset: dict) -> dict:
    """
    Convert all engine IDs in a preset to choice indices for the plugin
    With 1:1 mapping, this mainly validates engine IDs are in range
    """
    if "parameters" not in preset:
        return preset
    
    converted = preset.copy()
    params = converted["parameters"]
    
    # Validate each slot's engine ID is in valid range
    for slot in range(1, 7):
        param_name = f"slot{slot}_engine"
        if param_name in params:
            engine_id = int(params[param_name])
            # With 1:1 mapping, just validate range
            if engine_id < 0 or engine_id > 56:
                params[param_name] = 0  # Default to "None"
            else:
                params[param_name] = engine_id  # No conversion needed
    
    return converted