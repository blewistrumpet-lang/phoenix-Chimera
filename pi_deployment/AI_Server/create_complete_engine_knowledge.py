#!/usr/bin/env python3
"""
Create the COMPLETE engine knowledge base for Trinity Pipeline
This includes everything the AI needs to understand the plugin
"""

import json

# Mix parameter indices from UnifiedDefaultParameters.cpp
MIX_PARAM_INDICES = {
    0: -1,   # ENGINE_NONE - no parameters
    1: 4,    # ENGINE_OPTO_COMPRESSOR
    2: 6,    # ENGINE_VCA_COMPRESSOR
    3: 9,    # ENGINE_TRANSIENT_SHAPER
    4: -1,   # ENGINE_NOISE_GATE - no mix
    5: 9,    # ENGINE_MASTERING_LIMITER
    6: 6,    # ENGINE_DYNAMIC_EQ
    7: 13,   # ENGINE_PARAMETRIC_EQ
    8: -1,   # ENGINE_VINTAGE_CONSOLE_EQ - no mix
    9: 6,    # ENGINE_LADDER_FILTER
    10: 9,   # ENGINE_STATE_VARIABLE_FILTER
    11: 5,   # ENGINE_FORMANT_FILTER
    12: 7,   # ENGINE_ENVELOPE_FILTER
    13: 7,   # ENGINE_COMB_RESONATOR
    14: 7,   # ENGINE_VOCAL_FORMANT
    15: -1,  # ENGINE_VINTAGE_TUBE - no mix
    16: 7,   # ENGINE_WAVE_FOLDER
    17: 7,   # ENGINE_HARMONIC_EXCITER
    18: 3,   # ENGINE_BIT_CRUSHER
    19: 6,   # ENGINE_MULTIBAND_SATURATOR
    20: 6,   # ENGINE_MUFF_FUZZ
    21: 5,   # ENGINE_RODENT_DISTORTION
    22: 3,   # ENGINE_K_STYLE
    23: 5,   # ENGINE_DIGITAL_CHORUS
    24: 7,   # ENGINE_RESONANT_CHORUS
    25: 7,   # ENGINE_ANALOG_PHASER
    26: -1,  # ENGINE_RING_MODULATOR - no mix
    27: 2,   # ENGINE_FREQUENCY_SHIFTER
    28: -1,  # ENGINE_HARMONIC_TREMOLO - no mix
    29: 7,   # ENGINE_CLASSIC_TREMOLO
    30: 5,   # ENGINE_ROTARY_SPEAKER
    31: 2,   # ENGINE_PITCH_SHIFTER
    32: 4,   # ENGINE_DETUNE_DOUBLER
    33: 4,   # ENGINE_INTELLIGENT_HARMONIZER
    34: 4,   # ENGINE_TAPE_ECHO
    35: 2,   # ENGINE_DIGITAL_DELAY
    36: 7,   # ENGINE_MAGNETIC_DRUM_ECHO
    37: 5,   # ENGINE_BUCKET_BRIGADE_DELAY
    38: 7,   # ENGINE_BUFFER_REPEAT
    39: 3,   # ENGINE_PLATE_REVERB (verified working)
    40: 7,   # ENGINE_SPRING_REVERB
    41: 0,   # ENGINE_CONVOLUTION_REVERB
    42: 9,   # ENGINE_SHIMMER_REVERB
    43: 7,   # ENGINE_GATED_REVERB
    44: -1,  # ENGINE_STEREO_WIDENER - no mix
    45: -1,  # ENGINE_STEREO_IMAGER - no mix
    46: -1,  # ENGINE_DIMENSION_EXPANDER - no mix
    47: -1,  # ENGINE_SPECTRAL_FREEZE - no mix
    48: 7,   # ENGINE_SPECTRAL_GATE
    49: 6,   # ENGINE_PHASED_VOCODER
    50: -1,  # ENGINE_GRANULAR_CLOUD - no mix
    51: 7,   # ENGINE_CHAOS_GENERATOR
    52: 6,   # ENGINE_FEEDBACK_NETWORK
    53: -1,  # ENGINE_MID_SIDE_PROCESSOR - no mix
    54: -1,  # ENGINE_GAIN_UTILITY - no mix
    55: -1,  # ENGINE_MONO_MAKER - no mix
    56: -1,  # ENGINE_PHASE_ALIGN - no mix
}

# Musical context mappings
MUSICAL_CONTEXTS = {
    "warm": {
        "engines": [15, 34, 39, 40],  # Vintage Tube, Tape Echo, Plate/Spring Reverb
        "description": "Analog warmth and vintage character"
    },
    "bright": {
        "engines": [17, 7, 42],  # Harmonic Exciter, Parametric EQ, Shimmer
        "description": "Enhanced highs and presence"
    },
    "aggressive": {
        "engines": [22, 20, 21, 18],  # K-Style, Muff Fuzz, Rodent, Bit Crusher
        "description": "Heavy distortion and edge"
    },
    "spacious": {
        "engines": [46, 42, 41, 39],  # Dimension Expander, Shimmer, Convolution, Plate
        "description": "Wide, ambient space"
    },
    "tight": {
        "engines": [4, 3, 5],  # Noise Gate, Transient Shaper, Limiter
        "description": "Controlled dynamics"
    },
    "vintage": {
        "engines": [15, 1, 8, 34, 40],  # Vintage Tube, Opto, Console EQ, Tape, Spring
        "description": "Classic analog sound"
    },
    "modern": {
        "engines": [35, 18, 47, 48],  # Digital Delay, Bit Crusher, Spectral effects
        "description": "Digital precision and effects"
    },
    "psychedelic": {
        "engines": [25, 26, 51, 42],  # Phaser, Ring Mod, Chaos, Shimmer
        "description": "Trippy, experimental sounds"
    }
}

# Engine combinations that work well together
GOOD_COMBINATIONS = {
    "vocal_chain": [7, 2, 32, 39],  # EQ → Compressor → Doubler → Reverb
    "guitar_lead": [22, 15, 34, 39],  # Overdrive → Tube → Tape → Reverb
    "drum_bus": [3, 2, 5],  # Transient → Compressor → Limiter
    "ambient_pad": [9, 23, 46, 42],  # Filter → Chorus → Expander → Shimmer
    "bass_tight": [7, 2, 19],  # EQ → Compressor → Multiband Saturator
    "master_bus": [7, 6, 53, 5],  # EQ → Dynamic EQ → M/S → Limiter
}

# Engines to avoid combining
AVOID_COMBINATIONS = [
    [18, 19],  # Bit Crusher + Multiband Saturator (too much distortion)
    [31, 32, 33],  # Multiple pitch shifters (artifacts)
    [41, 42, 43],  # Multiple heavy reverbs (muddy)
    [20, 21, 22],  # Multiple fuzzes (harsh)
]

def create_complete_engine_knowledge():
    """Create the comprehensive engine knowledge base"""
    
    # Load the basic engine specs we already created
    with open("complete_engine_specs.json", "r") as f:
        basic_data = json.load(f)
    
    # Enhance each engine with complete knowledge
    enhanced_engines = {}
    
    for engine_id_str, engine_data in basic_data["engines"].items():
        engine_id = int(engine_id_str)
        
        # Add mix parameter index
        mix_index = MIX_PARAM_INDICES.get(engine_id, -1)
        
        # Determine typical usage based on category
        category = engine_data.get("category", "Unknown")
        typical_usage = get_typical_usage(engine_id, category)
        
        # Determine good combinations
        combines_well = get_good_combinations(engine_id)
        
        # Add frequency impact
        freq_impact = get_frequency_impact(engine_id, category)
        
        # Add gain impact
        gain_impact = get_gain_impact(engine_id, category)
        
        # Create enhanced engine entry
        enhanced_engines[engine_id] = {
            "id": engine_id,
            "name": engine_data.get("name", f"Engine {engine_id}"),
            "category": category,
            "param_count": engine_data.get("param_count", 10),
            "mix_param_index": mix_index,
            "has_mix": mix_index >= 0,
            "parameters": engine_data.get("parameters", []),
            "typical_usage": typical_usage,
            "combines_well_with": combines_well,
            "frequency_impact": freq_impact,
            "gain_impact": gain_impact,
            "cpu_load": get_cpu_load(engine_id, category),
            "sweet_spots": get_sweet_spots(engine_id),
            "signal_chain_position": get_chain_position(engine_id, category)
        }
    
    # Create the complete knowledge base
    knowledge_base = {
        "engines": enhanced_engines,
        "total_engines": 57,
        "slot_count": 6,
        "musical_contexts": MUSICAL_CONTEXTS,
        "good_combinations": GOOD_COMBINATIONS,
        "avoid_combinations": AVOID_COMBINATIONS,
        "signal_chain_order": [
            "Input",
            "Noise Gate",
            "EQ/Filter",
            "Compression",
            "Distortion/Saturation", 
            "Modulation",
            "Delay",
            "Reverb",
            "Utility/Output"
        ],
        "format_requirements": {
            "slots": "0-indexed (0-5)",
            "parameters": "Objects with 'name' and 'value' properties",
            "parameter_names": "param1, param2, ... paramN",
            "response_structure": "data.preset wrapper required"
        },
        "parameter_guidelines": {
            "mix_parameters": "Control dry/wet balance (0=dry, 1=wet)",
            "threshold_parameters": "Lower = more effect",
            "feedback_parameters": ">0.75 can self-oscillate",
            "resonance_parameters": "High values can be harsh",
            "time_parameters": "Sync to tempo when possible"
        }
    }
    
    return knowledge_base

def get_typical_usage(engine_id, category):
    """Get typical usage for an engine"""
    usage_map = {
        # Dynamics
        1: "Smooth optical compression for vocals/bass",
        2: "Punchy VCA compression for drums/mix bus",
        3: "Enhance attack/sustain of drums",
        4: "Remove noise between notes",
        5: "Final limiting for loudness",
        6: "Frequency-specific compression",
        
        # EQ/Filters
        7: "Surgical EQ corrections",
        8: "Musical analog-style EQ",
        9: "Classic synthesizer filtering",
        10: "Versatile filter types",
        11: "Vocal formant shaping",
        12: "Dynamic filter following input",
        13: "Metallic resonance effects",
        14: "Vowel-like filtering",
        
        # Distortion
        15: "Warm tube saturation",
        16: "Harsh digital folding",
        17: "Add harmonics and brightness",
        18: "Lo-fi digital degradation",
        19: "Frequency-specific saturation",
        20: "Classic fuzz tone",
        21: "Aggressive distortion",
        22: "Smooth overdrive",
        
        # Modulation
        23: "Classic stereo widening",
        24: "Rich resonant chorus",
        25: "Swooshing phase effects",
        26: "Metallic ring modulation",
        27: "Pitch shifting without changing tempo",
        28: "Complex tremolo variations",
        29: "Simple amplitude modulation",
        30: "Leslie speaker simulation",
        31: "Pitch shifting up/down",
        32: "Thickening via detuning",
        33: "Intelligent harmony generation",
        
        # Time-based
        34: "Vintage tape delay character",
        35: "Clean digital delays",
        36: "Unique drum echo sound",
        37: "Analog delay character",
        38: "Glitch/stutter effects",
        39: "Classic plate reverb",
        40: "Boingy spring reverb",
        41: "Realistic space simulation",
        42: "Ethereal shimmer effects",
        43: "80s gated reverb",
        
        # Spatial/Special
        44: "Widen stereo image",
        45: "Precise stereo control",
        46: "Add dimension and width",
        47: "Freeze spectral content",
        48: "Frequency-based gating",
        49: "Vocoder-like effects",
        50: "Granular textures",
        51: "Chaotic modulation",
        52: "Complex feedback loops",
        
        # Utility
        53: "Mid/Side processing",
        54: "Gain staging",
        55: "Mono compatibility",
        56: "Phase correction"
    }
    return usage_map.get(engine_id, f"Process audio with {category} effect")

def get_good_combinations(engine_id):
    """Get engines that combine well with this one"""
    # Based on category and typical use
    combinations = {
        # Compressors work with EQ
        1: [7, 8, 39],
        2: [7, 8, 5],
        # EQs work with everything
        7: [2, 15, 39],
        8: [1, 15, 34],
        # Distortions work with filters and delays
        15: [9, 34, 39],
        22: [15, 34, 39],
        # Delays work with reverbs
        34: [39, 40],
        35: [39, 42],
        # Reverbs work with most things
        39: [7, 2, 23],
        42: [23, 46]
    }
    return combinations.get(engine_id, [])

def get_frequency_impact(engine_id, category):
    """Describe frequency impact of engine"""
    if category == "EQ":
        return "Shapes frequency content"
    elif category in ["Filter", "Filters"]:
        return "Removes/emphasizes frequencies"
    elif category in ["Distortion", "Saturation"]:
        return "Adds harmonics and overtones"
    elif "Reverb" in category:
        return "Adds frequency diffusion"
    elif engine_id == 18:  # Bit Crusher
        return "Reduces high frequency content"
    else:
        return "Minimal frequency change"

def get_gain_impact(engine_id, category):
    """Describe gain impact of engine"""
    if category == "Dynamics":
        return "Controls dynamic range"
    elif category in ["Distortion", "Saturation"]:
        return "Can add significant gain"
    elif engine_id == 54:  # Gain Utility
        return "Direct gain control"
    else:
        return "Minimal gain change with mix control"

def get_cpu_load(engine_id, category):
    """Estimate CPU load"""
    heavy = [41, 42, 50, 51]  # Convolution, Shimmer, Granular, Chaos
    if engine_id in heavy:
        return "heavy"
    elif category in ["Reverb", "Delay"]:
        return "moderate"
    else:
        return "light"

def get_sweet_spots(engine_id):
    """Get parameter sweet spots for common uses"""
    # Just a few examples
    if engine_id == 2:  # Compressor
        return {
            "gentle": {"threshold": 0.7, "ratio": 0.3, "attack": 0.5},
            "punchy": {"threshold": 0.4, "ratio": 0.6, "attack": 0.2}
        }
    elif engine_id == 39:  # Plate Reverb
        return {
            "subtle": {"mix": 0.2, "size": 0.3, "damping": 0.6},
            "spacious": {"mix": 0.4, "size": 0.7, "damping": 0.4}
        }
    else:
        return {
            "default": {"param1": 0.5, "mix": 0.5}
        }

def get_chain_position(engine_id, category):
    """Suggest position in signal chain"""
    position_map = {
        "Dynamics": 3,
        "EQ": 2,
        "Filter": 2,
        "Filters": 2,
        "Distortion": 4,
        "Saturation": 4,
        "Modulation": 5,
        "Delay": 6,
        "Reverb": 7,
        "Time": 6,
        "Spatial": 8,
        "Special": 5,
        "Utility": 9,
        "Enhancement": 4
    }
    
    # Special cases
    if engine_id == 4:  # Noise Gate
        return 1  # First in chain
    elif engine_id == 5:  # Limiter
        return 9  # Last in chain
    
    return position_map.get(category, 5)

def main():
    """Generate the complete knowledge base"""
    knowledge = create_complete_engine_knowledge()
    
    # Save to file
    output_path = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/trinity_engine_knowledge.json"
    with open(output_path, "w") as f:
        json.dump(knowledge, f, indent=2)
    
    print(f"Created complete engine knowledge base: {output_path}")
    print(f"Total engines: {len(knowledge['engines'])}")
    print(f"Musical contexts: {len(knowledge['musical_contexts'])}")
    
    # Show summary
    print("\nKey insights included:")
    print("- Mix parameter indices for all engines")
    print("- Typical usage descriptions")
    print("- Engine combinations (good and bad)")
    print("- Signal chain positioning")
    print("- Frequency and gain impacts")
    print("- CPU load estimates")
    print("- Parameter sweet spots")
    
    return knowledge

if __name__ == "__main__":
    main()