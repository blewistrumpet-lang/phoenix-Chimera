#!/usr/bin/env python3
"""
Generate the MVP Golden Corpus for Project Chimera Phoenix
Creates presets for the three founding engines: K-Style Overdrive, Tape Echo, and Plate Reverb
"""

import json
from typing import Dict, Any, List

def create_preset(name: str, vibe: str, slot1_engine: int, slot2_engine: int, 
                 slot1_params: Dict[str, float], slot2_params: Dict[str, float]) -> Dict[str, Any]:
    """Helper function to create a preset with proper structure"""
    
    preset = {
        "name": name,
        "vibe": vibe,
        "source": "founding_corpus",
        "parameters": {}
    }
    
    # Set slot 1 parameters
    preset["parameters"]["slot1_engine"] = slot1_engine + 1 if slot1_engine >= 0 else 0
    preset["parameters"]["slot1_bypass"] = 0.0 if slot1_engine >= 0 else 1.0
    
    for i in range(1, 11):
        param_key = f"param{i}"
        if param_key in slot1_params:
            preset["parameters"][f"slot1_param{i}"] = slot1_params[param_key]
        else:
            preset["parameters"][f"slot1_param{i}"] = 0.5
    
    # Set slot 2 parameters
    preset["parameters"]["slot2_engine"] = slot2_engine + 1 if slot2_engine >= 0 else 0
    preset["parameters"]["slot2_bypass"] = 0.0 if slot2_engine >= 0 else 1.0
    
    for i in range(1, 11):
        param_key = f"param{i}"
        if param_key in slot2_params:
            preset["parameters"][f"slot2_param{i}"] = slot2_params[param_key]
        else:
            preset["parameters"][f"slot2_param{i}"] = 0.5
    
    return preset

def generate_corpus() -> List[Dict[str, Any]]:
    """Generate the complete MVP corpus"""
    corpus = []
    
    # K-Style Overdrive Presets
    corpus.append(create_preset(
        name="Warm Vintage Drive",
        vibe="warm vintage",
        slot1_engine=0,  # K-Style
        slot2_engine=-1, # Bypass
        slot1_params={
            "param1": 0.4,   # Drive
            "param2": 0.3,   # Tone (darker)
            "param3": 0.7    # Level
        },
        slot2_params={}
    ))
    
    corpus.append(create_preset(
        name="Aggressive Lead",
        vibe="aggressive bright",
        slot1_engine=0,  # K-Style
        slot2_engine=-1, # Bypass
        slot1_params={
            "param1": 0.8,   # Drive (high)
            "param2": 0.7,   # Tone (bright)
            "param3": 0.6    # Level
        },
        slot2_params={}
    ))
    
    corpus.append(create_preset(
        name="Subtle Warmth",
        vibe="subtle warm",
        slot1_engine=0,  # K-Style
        slot2_engine=-1, # Bypass
        slot1_params={
            "param1": 0.2,   # Drive (low)
            "param2": 0.4,   # Tone
            "param3": 0.8    # Level
        },
        slot2_params={}
    ))
    
    # Tape Echo Presets
    corpus.append(create_preset(
        name="Vintage Slap",
        vibe="vintage rhythmic",
        slot1_engine=-1, # Bypass
        slot2_engine=1,  # Tape Echo
        slot1_params={},
        slot2_params={
            "param1": 0.15,  # Time (short)
            "param2": 0.2,   # Feedback (low)
            "param3": 0.5,   # Mix
            "param4": 0.3,   # Wow
            "param5": 0.2    # Flutter
        }
    ))
    
    corpus.append(create_preset(
        name="Ambient Echo",
        vibe="spacious ethereal",
        slot1_engine=-1, # Bypass
        slot2_engine=1,  # Tape Echo
        slot1_params={},
        slot2_params={
            "param1": 0.6,   # Time (long)
            "param2": 0.5,   # Feedback (medium)
            "param3": 0.4,   # Mix
            "param4": 0.1,   # Wow
            "param5": 0.1    # Flutter
        }
    ))
    
    corpus.append(create_preset(
        name="Dub Delay",
        vibe="deep rhythmic",
        slot1_engine=-1, # Bypass
        slot2_engine=1,  # Tape Echo
        slot1_params={},
        slot2_params={
            "param1": 0.4,   # Time
            "param2": 0.7,   # Feedback (high)
            "param3": 0.6,   # Mix
            "param4": 0.4,   # Wow
            "param5": 0.3    # Flutter
        }
    ))
    
    # Plate Reverb Presets
    corpus.append(create_preset(
        name="Small Room",
        vibe="intimate natural",
        slot1_engine=-1, # Bypass
        slot2_engine=2,  # Plate Reverb
        slot1_params={},
        slot2_params={
            "param1": 0.2,   # Size (small)
            "param2": 0.6,   # Damping
            "param3": 0.1,   # Predelay
            "param4": 0.3    # Mix
        }
    ))
    
    corpus.append(create_preset(
        name="Concert Hall",
        vibe="spacious grand",
        slot1_engine=-1, # Bypass
        slot2_engine=2,  # Plate Reverb
        slot1_params={},
        slot2_params={
            "param1": 0.8,   # Size (large)
            "param2": 0.3,   # Damping
            "param3": 0.3,   # Predelay
            "param4": 0.4    # Mix
        }
    ))
    
    corpus.append(create_preset(
        name="Metallic Sheen",
        vibe="bright metallic",
        slot1_engine=-1, # Bypass
        slot2_engine=2,  # Plate Reverb
        slot1_params={},
        slot2_params={
            "param1": 0.5,   # Size
            "param2": 0.1,   # Damping (low = bright)
            "param3": 0.0,   # Predelay
            "param4": 0.5    # Mix
        }
    ))
    
    # Combination Presets
    corpus.append(create_preset(
        name="Driven Echo",
        vibe="gritty vintage",
        slot1_engine=0,  # K-Style
        slot2_engine=1,  # Tape Echo
        slot1_params={
            "param1": 0.5,   # Drive
            "param2": 0.4,   # Tone
            "param3": 0.6    # Level
        },
        slot2_params={
            "param1": 0.3,   # Time
            "param2": 0.4,   # Feedback
            "param3": 0.4,   # Mix
            "param4": 0.2,   # Wow
            "param5": 0.15   # Flutter
        }
    ))
    
    corpus.append(create_preset(
        name="Ambient Drive",
        vibe="warm spacious",
        slot1_engine=0,  # K-Style
        slot2_engine=2,  # Plate Reverb
        slot1_params={
            "param1": 0.3,   # Drive
            "param2": 0.35,  # Tone
            "param3": 0.7    # Level
        },
        slot2_params={
            "param1": 0.6,   # Size
            "param2": 0.4,   # Damping
            "param3": 0.2,   # Predelay
            "param4": 0.35   # Mix
        }
    ))
    
    corpus.append(create_preset(
        name="Echo Chamber",
        vibe="deep atmospheric",
        slot1_engine=1,  # Tape Echo
        slot2_engine=2,  # Plate Reverb
        slot1_params={
            "param1": 0.4,   # Time
            "param2": 0.3,   # Feedback
            "param3": 0.5,   # Mix
            "param4": 0.1,   # Wow
            "param5": 0.1    # Flutter
        },
        slot2_params={
            "param1": 0.7,   # Size
            "param2": 0.5,   # Damping
            "param3": 0.1,   # Predelay
            "param4": 0.3    # Mix
        }
    ))
    
    return corpus

def main():
    """Generate and save the corpus"""
    corpus = generate_corpus()
    
    # Save to JSON file
    with open('golden_corpus.json', 'w') as f:
        json.dump(corpus, f, indent=2)
    
    print(f"Generated MVP Golden Corpus with {len(corpus)} presets")
    print("Saved to: golden_corpus.json")
    
    # Print summary
    print("\nPreset Summary:")
    for preset in corpus:
        slot1_engine = preset['parameters'].get('slot1_engine', 0)
        slot2_engine = preset['parameters'].get('slot2_engine', 0)
        engines = []
        if slot1_engine > 0:
            engines.append(['Bypass', 'K-Style', 'Tape Echo', 'Plate Reverb'][slot1_engine])
        if slot2_engine > 0:
            engines.append(['Bypass', 'K-Style', 'Tape Echo', 'Plate Reverb'][slot2_engine])
        
        print(f"  - {preset['name']}: {' -> '.join(engines) if engines else 'No effects'} ({preset['vibe']})")

if __name__ == "__main__":
    main()