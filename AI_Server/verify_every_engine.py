#!/usr/bin/env python3
"""
COMPREHENSIVE ENGINE VERIFICATION TEST
Tests that every single engine ID maps correctly to its choice index
"""

from engine_mapping import ENGINE_ID_TO_CHOICE, engine_id_to_choice_index
import json

print("="*80)
print("COMPREHENSIVE ENGINE MAPPING VERIFICATION")
print("="*80)

# Complete list of all engines with their IDs and expected names
ALL_ENGINES = [
    (-1, "Bypass", 0),
    (0, "Vintage Tube Preamp", 48),
    (1, "Tape Echo", 2),
    (2, "Shimmer Reverb", 29),
    (3, "Plate Reverb", 3),
    (4, "Convolution Reverb", 25),
    (5, "Spring Reverb", 49),
    (6, "Vintage Opto", 42),
    (7, "Classic Compressor", 15),
    (8, "Magnetic Drum Echo", 7),
    (9, "Bucket Brigade Delay", 8),
    # (10, "Stereo Flanger", None),  # Commented out
    (11, "Digital Chorus", 17),
    (12, "Analog Phaser", 33),
    # (13, "Digital Phaser", None),  # Commented out
    (14, "Pitch Shifter", 23),
    (15, "Analog Ring Modulator", 20),
    (16, "Granular Cloud", 19),
    (17, "Vocal Formant Filter", 30),
    (18, "Dimension Expander", 32),
    (19, "Frequency Shifter", 27),
    (20, "Transient Shaper", 31),
    (21, "Harmonic Tremolo", 10),
    (22, "Classic Tremolo", 6),
    (23, "Comb Resonator", 22),
    (24, "Rotary Speaker", 11),
    (25, "Mid/Side Processor", 47),
    (26, "Vintage Console EQ", 46),
    (27, "Parametric EQ", 39),
    (28, "Ladder Filter", 13),
    (29, "State Variable Filter", 16),
    (30, "Formant Filter", 14),
    (31, "Wave Folder", 28),
    (32, "Harmonic Exciter", 36),
    (33, "Bit Crusher", 26),
    (34, "Multiband Saturator", 21),
    (35, "Muff Fuzz", 5),
    (36, "Rodent Distortion", 4),
    # (37, "Virtual Bass Amp", None),  # Commented out
    (38, "K-Style Overdrive", 1),
    (39, "Spectral Freeze", 18),
    (40, "Buffer Repeat", 45),
    (41, "Chaos Generator", 44),
    (42, "Intelligent Harmonizer", 38),
    (43, "Gated Reverb", 35),
    (44, "Detune Doubler", 12),
    (45, "Phased Vocoder", 24),
    (46, "Spectral Gate", 43),
    (47, "Noise Gate", 41),
    (48, "Envelope Filter", 34),
    (49, "Feedback Network", 37),
    (50, "Mastering Limiter", 40),
    (51, "Stereo Widener", 51),
    (52, "Resonant Chorus", 50),
    (53, "Digital Delay", 9),
    (54, "Dynamic EQ", 52),
    (55, "Stereo Imager", 53)
]

print("\n1. TESTING EACH ENGINE ID TO CHOICE INDEX MAPPING:")
print("-"*60)

errors = []
for engine_id, name, expected_choice in ALL_ENGINES:
    if expected_choice is None:
        # This engine is commented out in the plugin
        print(f"  Engine ID {engine_id:2d} ({name:30s}) - SKIPPED (commented out)")
        continue
    
    actual_choice = engine_id_to_choice_index(engine_id)
    
    if actual_choice == expected_choice:
        print(f"  ✅ Engine ID {engine_id:2d} ({name:30s}) -> Choice {actual_choice:2d}")
    else:
        error_msg = f"  ❌ Engine ID {engine_id:2d} ({name:30s}) -> Choice {actual_choice:2d} (expected {expected_choice})"
        print(error_msg)
        errors.append(error_msg)

print("\n2. TESTING PROBLEMATIC ENGINE IDS FROM PRESETS:")
print("-"*60)

# These are the engine IDs that appear in the corpus
corpus_engine_ids = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 14, 15, 16, 17, 18, 19, 20, 
                     21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 
                     38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55]

for engine_id in corpus_engine_ids:
    choice_idx = engine_id_to_choice_index(engine_id)
    engine_info = next((e for e in ALL_ENGINES if e[0] == engine_id), None)
    if engine_info:
        _, name, expected = engine_info
        if expected is not None:
            status = "✅" if choice_idx == expected else "❌"
            print(f"  Engine ID {engine_id:2d} -> Choice {choice_idx:2d} ({name}) {status}")

print("\n3. CHECKING ORACLE PRESET CONVERSION:")
print("-"*60)

# Load a sample preset from the corpus
with open("../JUCE_Plugin/GoldenCorpus/all_presets.json", 'r') as f:
    corpus_data = json.load(f)
    
# Test first preset
test_preset = corpus_data['presets'][0]
print(f"Testing preset: {test_preset['name']}")

for engine in test_preset['engines']:
    engine_id = engine['type']
    engine_name = engine['typeName']
    choice_idx = engine_id_to_choice_index(engine_id)
    print(f"  Slot {engine['slot']}: Engine ID {engine_id} ({engine_name}) -> Choice {choice_idx}")

print("\n4. VERIFYING CONVERSION IN PIPELINE:")
print("-"*60)

# Test specific cases that were failing
test_cases = [
    (0, "Vintage Tube", 48),
    (1, "Tape Echo", 2),
    (27, "Parametric EQ", 39),
    (2, "Shimmer Reverb", 29),
    (18, "Dimension Expander", 32),
    (41, "Chaos Generator", 44),
    (40, "Buffer Repeat", 45),
    (52, "Resonant Chorus", 50)
]

print("Testing conversion function:")
for engine_id, name, expected in test_cases:
    actual = engine_id_to_choice_index(engine_id)
    status = "✅" if actual == expected else "❌"
    print(f"  engine_id_to_choice_index({engine_id:2d}) = {actual:2d} (expected {expected:2d}) {status}")

print("\n" + "="*80)
print("VERIFICATION SUMMARY:")
print("="*80)

if errors:
    print(f"❌ FOUND {len(errors)} ERRORS!")
    for error in errors:
        print(error)
else:
    print("✅ ALL ENGINE MAPPINGS ARE CORRECT!")
    print("   Every engine ID maps to the correct choice index.")
    print("   The mapping table is properly configured.")
    print("\n   The issue is that Oracle is NOT converting engine IDs to choice indices!")
    print("   Oracle returns engine IDs from the corpus, but these need conversion!")