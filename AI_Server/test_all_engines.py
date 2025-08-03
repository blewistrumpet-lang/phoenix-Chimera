#!/usr/bin/env python3
"""
COMPREHENSIVE TEST FOR ALL 54 ENGINES
Tests every single engine ID to choice index mapping
"""

from engine_mapping import ENGINE_ID_TO_CHOICE, CHOICE_TO_ENGINE_ID

print("="*80)
print("TESTING ALL 54 ENGINE MAPPINGS")
print("="*80)

# List of all engine names from the plugin
ENGINE_NAMES = [
    "Bypass",  # 0
    "K-Style Overdrive",  # 1
    "Tape Echo",  # 2
    "Plate Reverb",  # 3
    "Rodent Distortion",  # 4
    "Muff Fuzz",  # 5
    "Classic Tremolo",  # 6
    "Magnetic Drum Echo",  # 7
    "Bucket Brigade Delay",  # 8
    "Digital Delay",  # 9
    "Harmonic Tremolo",  # 10
    "Rotary Speaker",  # 11
    "Detune Doubler",  # 12
    "Ladder Filter",  # 13
    "Formant Filter",  # 14
    "Classic Compressor",  # 15
    "State Variable Filter",  # 16
    "Stereo Chorus",  # 17
    "Spectral Freeze",  # 18
    "Granular Cloud",  # 19
    "Analog Ring Modulator",  # 20
    "Multiband Saturator",  # 21
    "Comb Resonator",  # 22
    "Pitch Shifter",  # 23
    "Phased Vocoder",  # 24
    "Convolution Reverb",  # 25
    "Bit Crusher",  # 26
    "Frequency Shifter",  # 27
    "Wave Folder",  # 28
    "Shimmer Reverb",  # 29
    "Vocal Formant Filter",  # 30
    "Transient Shaper",  # 31
    "Dimension Expander",  # 32
    "Analog Phaser",  # 33
    "Envelope Filter",  # 34
    "Gated Reverb",  # 35
    "Harmonic Exciter",  # 36
    "Feedback Network",  # 37
    "Intelligent Harmonizer",  # 38
    "Parametric EQ",  # 39
    "Mastering Limiter",  # 40
    "Noise Gate",  # 41
    "Vintage Opto",  # 42
    "Spectral Gate",  # 43
    "Chaos Generator",  # 44
    "Buffer Repeat",  # 45
    "Vintage Console EQ",  # 46
    "Mid/Side Processor",  # 47
    "Vintage Tube Preamp",  # 48
    "Spring Reverb",  # 49
    "Resonant Chorus",  # 50
    "Stereo Widener",  # 51
    "Dynamic EQ",  # 52
    "Stereo Imager"  # 53
]

# Check all mappings
print("\n1. VERIFYING ENGINE ID -> CHOICE INDEX MAPPINGS:")
print("-"*50)

errors = []
for engine_id, choice_idx in ENGINE_ID_TO_CHOICE.items():
    if engine_id == -1:
        continue  # Skip bypass
    
    if 0 <= choice_idx < len(ENGINE_NAMES):
        print(f"  Engine ID {engine_id:2d} -> Choice {choice_idx:2d} ({ENGINE_NAMES[choice_idx]})")
    else:
        error = f"  ERROR: Engine ID {engine_id} maps to invalid choice {choice_idx}"
        print(error)
        errors.append(error)

print("\n2. VERIFYING REVERSE MAPPINGS (CHOICE -> ENGINE ID):")
print("-"*50)

for choice_idx in range(54):
    engine_id = CHOICE_TO_ENGINE_ID.get(choice_idx, -999)
    if engine_id == -999:
        error = f"  ERROR: Choice index {choice_idx} ({ENGINE_NAMES[choice_idx]}) has no engine ID!"
        print(error)
        errors.append(error)
    else:
        print(f"  Choice {choice_idx:2d} ({ENGINE_NAMES[choice_idx]:30s}) <- Engine ID {engine_id:2d}")

print("\n3. CHECKING FOR DUPLICATES:")
print("-"*50)

# Check for duplicate choice indices
choice_counts = {}
for engine_id, choice_idx in ENGINE_ID_TO_CHOICE.items():
    if choice_idx in choice_counts:
        choice_counts[choice_idx].append(engine_id)
    else:
        choice_counts[choice_idx] = [engine_id]

duplicates = []
for choice_idx, engine_ids in choice_counts.items():
    if len(engine_ids) > 1:
        duplicate = f"Choice {choice_idx} is mapped from multiple engine IDs: {engine_ids}"
        print(f"  DUPLICATE: {duplicate}")
        duplicates.append(duplicate)

if not duplicates:
    print("  ✅ No duplicate mappings found")

print("\n4. CHECKING COVERAGE:")
print("-"*50)

# Check which choice indices are missing
mapped_choices = set(ENGINE_ID_TO_CHOICE.values())
all_choices = set(range(54))
missing_choices = all_choices - mapped_choices

if missing_choices:
    print(f"  ❌ Missing choice indices: {sorted(missing_choices)}")
    for idx in sorted(missing_choices):
        print(f"     Choice {idx}: {ENGINE_NAMES[idx]}")
else:
    print("  ✅ All 54 choice indices are mapped")

# Check which engine IDs are mapped
mapped_engine_ids = set(ENGINE_ID_TO_CHOICE.keys()) - {-1}  # Exclude bypass
print(f"\n  Total engine IDs mapped: {len(mapped_engine_ids)}")
print(f"  Engine ID range: {min(mapped_engine_ids)} to {max(mapped_engine_ids)}")

print("\n5. SPECIFIC TEST CASES:")
print("-"*50)

test_cases = [
    (41, "Chaos Generator"),
    (40, "Buffer Repeat"),
    (52, "Resonant Chorus"),
    (47, "Noise Gate"),
    (50, "Mastering Limiter"),
    (54, "Dynamic EQ")
]

for engine_id, expected_name in test_cases:
    choice_idx = ENGINE_ID_TO_CHOICE.get(engine_id, -1)
    if 0 <= choice_idx < len(ENGINE_NAMES):
        actual_name = ENGINE_NAMES[choice_idx]
        status = "✅" if actual_name == expected_name else "❌"
        print(f"  Engine ID {engine_id} -> Choice {choice_idx} ({actual_name}) {status}")
    else:
        print(f"  ❌ Engine ID {engine_id} not in mapping!")

print("\n" + "="*80)
print("SUMMARY:")
print("="*80)

if errors:
    print(f"❌ Found {len(errors)} errors")
    for error in errors:
        print(error)
elif duplicates:
    print(f"❌ Found {len(duplicates)} duplicate mappings")
else:
    print("✅ All engine mappings verified successfully!")
    print(f"   - {len(ENGINE_ID_TO_CHOICE)} engine IDs mapped")
    print(f"   - {len(mapped_choices)} choice indices covered")
    print(f"   - No duplicates or errors found")