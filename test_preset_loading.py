#!/usr/bin/env python3
"""Test script to verify engine ID mapping in preset loading"""

import json

# Test preset with various engine IDs
test_preset = {
    "name": "Engine Mapping Test",
    "engines": [
        {
            "slot": 1,
            "engine": 0,  # Vintage Tube (was being mapped to Bypass incorrectly)
            "params": {
                "slot1_param1": 0.3,
                "slot1_param2": 0.5,
                "slot1_param3": 0.7,
                "slot1_param4": 0.2,
                "slot1_param5": 0.0,
                "slot1_param6": 0.5,
                "slot1_param7": 0.5,
                "slot1_param8": 0.5,
                "slot1_param9": 0.5,
                "slot1_param10": 0.5
            }
        },
        {
            "slot": 2,
            "engine": 1,  # Tape Echo
            "params": {
                "slot2_param1": 0.7,
                "slot2_param2": 0.4,
                "slot2_param3": 0.6,
                "slot2_param4": 0.3,
                "slot2_param5": 0.5,
                "slot2_param6": 0.5,
                "slot2_param7": 0.5,
                "slot2_param8": 0.5,
                "slot2_param9": 0.5,
                "slot2_param10": 0.5
            }
        },
        {
            "slot": 3,
            "engine": 41,  # Chaos Generator (was being mapped incorrectly)
            "params": {
                "slot3_param1": 0.5,
                "slot3_param2": 0.6,
                "slot3_param3": 0.4,
                "slot3_param4": 0.7,
                "slot3_param5": 0.3,
                "slot3_param6": 0.5,
                "slot3_param7": 0.5,
                "slot3_param8": 0.5,
                "slot3_param9": 0.5,
                "slot3_param10": 0.5
            }
        }
    ]
}

# Save test preset
with open('test_engine_mapping_preset.json', 'w') as f:
    json.dump(test_preset, f, indent=2)

print("Test preset created: test_engine_mapping_preset.json")
print("\nExpected results after loading:")
print("- Slot 1: Should load 'Vintage Tube Preamp' (NOT Bypass)")
print("- Slot 2: Should load 'Tape Echo'")
print("- Slot 3: Should load 'Chaos Generator'")
print("\nIf you see 'Bypass' in any slot, the mapping is still broken!")