#!/usr/bin/env python3
"""
Detailed test showing preset names, engines, and signal chain order
"""

import requests
import json
from engine_mapping_authoritative import ENGINE_NAMES

def test_detailed():
    """Show detailed output for various prompts"""
    
    print("🎼 DETAILED PRESET GENERATION TEST")
    print("=" * 80)
    
    test_prompts = [
        "Create warm vintage vocals like Billie Eilish",
        "Aggressive metal guitar with heavy distortion and tight gating", 
        "Ethereal ambient pad with shimmer reverb and lots of space",
        "Clean jazz piano with natural dynamics and room sound",
        "Modern pop vocals - bright, compressed, and polished",
        "Lo-fi hip hop beat with bit crusher and tape saturation",
        "Cinematic strings with hall reverb and dimension expander",
        "Funky bass with envelope filter and classic chorus",
        "Acoustic guitar with spring reverb and tape echo",
        "Electronic synth lead with analog phaser and delay"
    ]
    
    for i, prompt in enumerate(test_prompts, 1):
        print(f"\n{'='*80}")
        print(f"Test {i}: {prompt}")
        print("-" * 80)
        
        try:
            response = requests.post(
                "http://localhost:8000/generate",
                json={"prompt": prompt},
                timeout=10
            )
            
            if response.status_code == 200:
                data = response.json()
                preset = data.get("preset", {})
                metadata = data.get("metadata", {})
                
                # Preset name
                preset_name = preset.get("name", "Unknown")
                print(f"✨ PRESET NAME: '{preset_name}'")
                
                # Signal flow explanation
                signal_flow = preset.get("signal_flow", "No flow information")
                print(f"\n📊 SIGNAL FLOW:")
                # Format the signal flow for readability
                if "→" in signal_flow:
                    parts = signal_flow.replace("Signal flow: ", "").split("→")
                    for j, part in enumerate(parts, 1):
                        print(f"   {j}. {part.strip()}")
                else:
                    print(f"   {signal_flow}")
                
                # Engines in order
                print(f"\n🎛️ ENGINES (in slot order):")
                engines_list = []
                for slot in range(1, 7):
                    engine_id = preset.get(f"slot{slot}_engine", 0)
                    if engine_id > 0:
                        engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
                        engines_list.append((slot, engine_id, engine_name))
                        
                        # Get key parameters for this engine
                        param_info = []
                        for p in range(3):  # Just show first 3 params
                            param_val = preset.get(f"slot{slot}_param{p}", 0)
                            if isinstance(param_val, (int, float)) and param_val != 0.5:
                                param_info.append(f"p{p}={param_val:.2f}")
                        
                        param_str = f" [{', '.join(param_info)}]" if param_info else ""
                        print(f"   Slot {slot}: {engine_name}{param_str}")
                
                # Categories of engines
                print(f"\n📦 ENGINE CATEGORIES:")
                dynamics = [e for s, id, e in engines_list if id in range(0, 6)]
                eq = [e for s, id, e in engines_list if id in range(6, 9)]
                filters = [e for s, id, e in engines_list if id in range(9, 15)]
                distortion = [e for s, id, e in engines_list if id in range(15, 23)]
                modulation = [e for s, id, e in engines_list if id in range(23, 34)]
                delay = [e for s, id, e in engines_list if id in range(34, 38)]
                reverb = [e for s, id, e in engines_list if id in range(38, 44)]
                spatial = [e for s, id, e in engines_list if id in range(44, 47)]
                pitch = [e for s, id, e in engines_list if id in range(47, 51)]
                
                if dynamics: print(f"   Dynamics: {', '.join(dynamics)}")
                if eq: print(f"   EQ: {', '.join(eq)}")
                if filters: print(f"   Filters: {', '.join(filters)}")
                if distortion: print(f"   Distortion: {', '.join(distortion)}")
                if modulation: print(f"   Modulation: {', '.join(modulation)}")
                if delay: print(f"   Delay: {', '.join(delay)}")
                if reverb: print(f"   Reverb: {', '.join(reverb)}")
                if spatial: print(f"   Spatial: {', '.join(spatial)}")
                if pitch: print(f"   Pitch: {', '.join(pitch)}")
                
                # Metadata
                if metadata.get("required_engines"):
                    print(f"\n🎯 REQUIRED ENGINES IDENTIFIED: {len(metadata['required_engines'])}")
                    for eng_id in metadata['required_engines']:
                        print(f"   - {ENGINE_NAMES.get(eng_id, f'Unknown({eng_id})')}")
                
                # Match rate
                if "match_rate" in metadata:
                    print(f"\n📈 Match Rate: {metadata['match_rate']:.0f}%")
                
            else:
                print(f"❌ Failed: {response.status_code}")
                print(f"   {response.text[:200]}")
                
        except Exception as e:
            print(f"❌ Error: {str(e)}")
    
    print("\n" + "=" * 80)
    print("✅ Test Complete")

if __name__ == "__main__":
    test_detailed()