#!/usr/bin/env python3
"""
Test with a perfect hardcoded preset to verify exact format needed
"""

import requests
import json

# This is the EXACT format the plugin expects based on TrinityManager.cpp analysis
perfect_preset = {
    "success": True,
    "type": "preset",
    "message": "Test preset loaded successfully",
    "data": {
        "preset": {
            "name": "Perfect Test Preset",
            "description": "Hardcoded preset for testing",
            "slots": [
                {
                    "slot": 1,
                    "engine_id": 15,  # Vintage Tube Preamp
                    "engine_name": "Vintage Tube Preamp",
                    "parameters": [
                        {"name": "param1", "value": 0.5},
                        {"name": "param2", "value": 0.3},
                        {"name": "param3", "value": 0.4},
                        {"name": "param4", "value": 0.6},
                        {"name": "param5", "value": 0.2},
                        {"name": "param6", "value": 0.5},
                        {"name": "param7", "value": 0.5},
                        {"name": "param8", "value": 0.5},
                        {"name": "param9", "value": 0.0},
                        {"name": "param10", "value": 1.0}
                    ]
                },
                {
                    "slot": 2,
                    "engine_id": 23,  # Stereo Chorus
                    "engine_name": "Stereo Chorus",
                    "parameters": [
                        {"name": "param1", "value": 0.3},
                        {"name": "param2", "value": 0.5},
                        {"name": "param3", "value": 0.4},
                        {"name": "param4", "value": 0.5},
                        {"name": "param5", "value": 0.5},
                        {"name": "param6", "value": 0.5},
                        {"name": "param7", "value": 0.5},
                        {"name": "param8", "value": 0.5},
                        {"name": "param9", "value": 0.0},
                        {"name": "param10", "value": 0.0}
                    ]
                },
                {
                    "slot": 3,
                    "engine_id": 39,  # Plate Reverb
                    "engine_name": "Plate Reverb",
                    "parameters": [
                        {"name": "param1", "value": 0.2},  # Mix
                        {"name": "param2", "value": 0.6},  # Size
                        {"name": "param3", "value": 0.5},  # Decay
                        {"name": "param4", "value": 0.4},  # Damping
                        {"name": "param5", "value": 0.5},
                        {"name": "param6", "value": 0.5},
                        {"name": "param7", "value": 0.5},
                        {"name": "param8", "value": 0.5},
                        {"name": "param9", "value": 0.0},
                        {"name": "param10", "value": 0.0}
                    ]
                }
            ]
        }
    }
}

# Test endpoint that returns this perfect preset
if __name__ == "__main__":
    print("Testing perfect preset format...")
    print(json.dumps(perfect_preset, indent=2))
    
    # Save to file for testing
    with open("perfect_preset.json", "w") as f:
        json.dump(perfect_preset, f, indent=2)
    
    print("\nPerfect preset saved to perfect_preset.json")
    print("\nTo test, create an endpoint that returns this exact format")