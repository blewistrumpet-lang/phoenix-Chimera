#!/usr/bin/env python3
"""
Fix engine IDs in GeneratedParameterDatabase.h to match EngineTypes.h
"""

# Correct engine IDs from EngineTypes.h
CORRECT_ENGINE_IDS = {
    "Vintage Tube Preamp": 15,      # Was 0, should be 15
    "K-Style Overdrive": 22,         # Was 38, should be 22
    "Tape Echo": 34,                 # Was 1, should be 34
    "Shimmer Reverb": 42,            # Was 2, should be 42
    "Plate Reverb": 39,              # Was 3, should be 39
    "Spring Reverb": 40,             # Was 5, should be 40
    "Convolution Reverb": 41,        # Was 41, correct
    "Gated Reverb": 43,              # Was 43, correct
    "Noise Gate": 4,                 # Was 4, correct
    "Mastering Limiter": 5,          # Was 5, correct
    "Vintage Opto": 1,               # ENGINE_OPTO_COMPRESSOR
    "Classic Compressor": 2,         # ENGINE_VCA_COMPRESSOR
    "Transient Shaper": 3,           # ENGINE_TRANSIENT_SHAPER
    "Dynamic EQ": 6,                 # ENGINE_DYNAMIC_EQ
    "Parametric EQ": 7,              # ENGINE_PARAMETRIC_EQ
    "Vintage Console EQ": 8,         # ENGINE_VINTAGE_CONSOLE_EQ
    "Ladder Filter Pro": 9,          # ENGINE_LADDER_FILTER
    "Envelope Filter": 12,           # ENGINE_ENVELOPE_FILTER
    "Comb Resonator": 13,            # ENGINE_COMB_RESONATOR
    "Harmonic Exciter": 17,          # ENGINE_HARMONIC_EXCITER
    "BitCrusher": 18,                # ENGINE_BIT_CRUSHER
    "Muff Fuzz": 20,                 # ENGINE_MUFF_FUZZ
    "Rodent Distortion": 21,         # ENGINE_RODENT_DISTORTION
    "Stereo Chorus": 23,             # ENGINE_DIGITAL_CHORUS
    "Resonant Chorus": 24,           # ENGINE_RESONANT_CHORUS
    "Ring Modulator": 26,            # ENGINE_RING_MODULATOR
    "Harmonic Tremolo": 28,          # ENGINE_HARMONIC_TREMOLO
    "Classic Tremolo": 29,           # ENGINE_CLASSIC_TREMOLO
    "Rotary Speaker": 30,            # ENGINE_ROTARY_SPEAKER
    "Pitch Shifter": 31,             # ENGINE_PITCH_SHIFTER
    "Detune Doubler": 32,            # ENGINE_DETUNE_DOUBLER
    "Intelligent Harmonizer": 33,    # ENGINE_INTELLIGENT_HARMONIZER
    "Digital Delay": 35,             # ENGINE_DIGITAL_DELAY
    "Buffer Repeat": 38,             # ENGINE_BUFFER_REPEAT
    "Stereo Widener": 44,            # ENGINE_STEREO_WIDENER
    "Stereo Imager": 45,             # ENGINE_STEREO_IMAGER
    "Dimension Expander": 46,        # ENGINE_DIMENSION_EXPANDER
    "Spectral Freeze": 47,           # ENGINE_SPECTRAL_FREEZE
    "Spectral Gate": 48,             # ENGINE_SPECTRAL_GATE
    "Phased Vocoder": 49,            # ENGINE_PHASED_VOCODER
    "Granular Cloud": 50,            # ENGINE_GRANULAR_CLOUD
    "Chaos Generator": 51,           # ENGINE_CHAOS_GENERATOR
    "Feedback Network": 52,          # ENGINE_FEEDBACK_NETWORK
    "Mid/Side Processor": 53,        # ENGINE_MID_SIDE_PROCESSOR
    "Gain Utility": 54,              # ENGINE_GAIN_UTILITY
    "Mono Maker": 55,                # ENGINE_MONO_MAKER
    "Phase Align": 56,                # ENGINE_PHASE_ALIGN
}

# Read the file
with open('JUCE_Plugin/Source/GeneratedParameterDatabase.h', 'r') as f:
    content = f.read()

# Fix each engine entry in the database
lines = content.split('\n')
new_lines = []
for line in lines:
    # Look for engine database entries
    if '", ' in line and 'engineDatabase' not in line:
        for engine_name, correct_id in CORRECT_ENGINE_IDS.items():
            if f'"{engine_name}"' in line:
                # Extract parts
                parts = line.split(', ')
                if len(parts) >= 3:
                    # Replace the legacy ID (third field)
                    parts[2] = str(correct_id)
                    line = ', '.join(parts)
                    print(f"Fixed {engine_name}: ID -> {correct_id}")
                break
    new_lines.append(line)

# Write back
with open('JUCE_Plugin/Source/GeneratedParameterDatabase.h', 'w') as f:
    f.write('\n'.join(new_lines))

print("\nEngine IDs have been fixed in GeneratedParameterDatabase.h")
print("The database now matches EngineTypes.h definitions")