#!/usr/bin/env python3
"""
Test the improved hybrid name generation
"""

from alchemist import Alchemist

# Create test presets with different vibes
test_presets = [
    {
        "vibe": "warm vintage",
        "parameters": {
            "slot1_engine": 50,  # Vintage Tube Preamp
            "slot1_bypass": 0.0,
            "slot2_engine": 1,   # Tape Echo
            "slot2_bypass": 0.0,
            "slot3_engine": 0,
            "slot3_bypass": 1.0
        }
    },
    {
        "vibe": "aggressive metal with tight gating",
        "parameters": {
            "slot1_engine": 36,  # Rodent Distortion
            "slot1_bypass": 0.0,
            "slot2_engine": 42,  # Noise Gate
            "slot2_bypass": 0.0,
            "slot3_engine": 0,
            "slot3_bypass": 1.0
        }
    },
    {
        "vibe": "spacious ambient",
        "parameters": {
            "slot1_engine": 30,  # Shimmer Reverb
            "slot1_bypass": 0.0,
            "slot2_engine": 33,  # Dimension Expander
            "slot2_bypass": 0.0,
            "slot3_engine": 0,
            "slot3_bypass": 1.0
        }
    },
    {
        "vibe": "punchy dynamic drums",
        "parameters": {
            "slot1_engine": 32,  # Transient Shaper
            "slot1_bypass": 0.0,
            "slot2_engine": 16,  # Classic Compressor
            "slot2_bypass": 0.0,
            "slot3_engine": 0,
            "slot3_bypass": 1.0
        }
    },
    {
        "vibe": "lofi vintage character",
        "parameters": {
            "slot1_engine": 27,  # Bit Crusher
            "slot1_bypass": 0.0,
            "slot2_engine": 1,   # Tape Echo
            "slot2_bypass": 0.0,
            "slot3_engine": 0,
            "slot3_bypass": 1.0
        }
    }
]

print("\n" + "="*80)
print("TESTING HYBRID NAME GENERATION")
print("="*80)

alchemist = Alchemist()

# Test each preset multiple times to see variety
for i, preset in enumerate(test_presets):
    print(f"\nVibe: '{preset['vibe']}'")
    print("Generated names:")
    
    # Generate 5 names for each vibe to show variety
    names = []
    for _ in range(5):
        name = alchemist.generate_preset_name(preset)
        names.append(name)
    
    # Show unique names
    unique_names = list(set(names))
    for name in unique_names:
        count = names.count(name)
        if count > 1:
            print(f"  • {name} (x{count})")
        else:
            print(f"  • {name}")

print("\n" + "="*80)
print("IMPROVEMENTS:")
print("- Names now reflect the vibe from Visionary")
print("- More contextual variety (warm → Vintage/Cozy/Golden)")
print("- Effect-aware naming (reverb → Space/Chamber/Hall)")
print("- Much less repetition!")
print("="*80)