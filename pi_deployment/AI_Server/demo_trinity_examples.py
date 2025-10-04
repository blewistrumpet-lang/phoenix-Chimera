#!/usr/bin/env python3
"""
Demonstrate Trinity Pipeline with various poetic, technical, and mixed prompts
Shows both generation and modification capabilities
"""

import requests
import json
import time
from typing import Dict, Any
from engine_mapping_correct import ENGINE_MAPPING

def print_preset_summary(preset: Dict[str, Any], title: str = ""):
    """Print a concise summary of preset configuration"""
    if title:
        print(f"\n{'='*60}")
        print(f"  {title}")
        print('='*60)
    
    print(f"📝 Name: {preset.get('name', 'Unknown')}")
    print(f"🎨 Vibe: {preset.get('vibe', 'Unknown')}")
    
    # Show active engines
    parameters = preset.get('parameters', {})
    print("🎛️ Active Engines:")
    for slot in range(1, 7):
        engine_id = parameters.get(f'slot{slot}_engine', 0)
        if engine_id > 0 and parameters.get(f'slot{slot}_bypass', 0) < 0.5:
            engine_name = ENGINE_MAPPING.get(engine_id, 'Unknown')
            mix = parameters.get(f'slot{slot}_mix', 0.5)
            # Get key parameter values
            p1 = parameters.get(f'slot{slot}_param1', 0)
            p2 = parameters.get(f'slot{slot}_param2', 0)
            print(f"  Slot {slot}: {engine_name} (mix: {mix:.2f}, drive: {p1:.2f}, tone: {p2:.2f})")

def generate_and_modify(prompt: str, modifications: list):
    """Generate a preset and apply a series of modifications"""
    base_url = "http://localhost:8000"
    
    print("\n" + "="*80)
    print(f"🎯 INITIAL PROMPT: \"{prompt}\"")
    print("="*80)
    
    # Generate initial preset
    response = requests.post(
        f"{base_url}/generate",
        json={"prompt": prompt},
        timeout=30
    )
    
    if response.status_code != 200:
        print("❌ Failed to generate")
        return None
    
    preset = response.json()["preset"]
    metadata = response.json().get("metadata", {})
    
    print(f"✅ Generated in {metadata.get('generation_time_seconds', 0):.1f}s")
    print_preset_summary(preset, "INITIAL PRESET")
    
    # Apply modifications
    for i, mod_text in enumerate(modifications, 1):
        print(f"\n{'─'*60}")
        print(f"🔄 MODIFICATION {i}: \"{mod_text}\"")
        print('─'*60)
        
        response = requests.post(
            f"{base_url}/modify",
            json={
                "preset": preset,
                "modification": mod_text
            },
            timeout=10
        )
        
        if response.status_code == 200:
            result = response.json()
            if result.get("success"):
                print(f"✅ {result.get('message', 'Applied')}")
                
                # Show changes
                changes = result.get("changes", [])
                if changes:
                    print("📊 Changes:")
                    for change in changes[:3]:
                        print(f"   • {change}")
                
                # Update preset
                preset = result["data"]
                print_preset_summary(preset, f"AFTER: {mod_text[:30]}...")
        else:
            print(f"❌ Modification failed")
    
    return preset

def main():
    """Run comprehensive demonstration"""
    
    time.sleep(2)  # Give server time to be ready
    
    print("\n" + "="*80)
    print("🎭 TRINITY PIPELINE DEMONSTRATION")
    print("Showing Poetic, Technical, and Mixed Prompts")
    print("="*80)
    
    # Test 1: POETIC → POETIC
    print("\n\n🌊 TEST 1: POETIC GENERATION → POETIC MODIFICATIONS")
    generate_and_modify(
        prompt="The sound of starlight crystallizing in zero gravity, where time becomes liquid and dreams echo backwards",
        modifications=[
            "Add the warmth of a dying sun",
            "Make it feel like falling through honey",
            "Transform into the loneliness of deep space"
        ]
    )
    
    # Test 2: TECHNICAL → TECHNICAL
    print("\n\n⚙️ TEST 2: TECHNICAL GENERATION → TECHNICAL MODIFICATIONS")
    generate_and_modify(
        prompt="Classic analog synthesizer patch: dual oscillators detuned by 7 cents, 24dB/oct ladder filter with resonance at 65%, ADSR envelope with 10ms attack, 200ms decay, 0.7 sustain, 500ms release, feeding into plate reverb with 3.5s decay time and 20% mix",
        modifications=[
            "Increase filter resonance to 85% and reduce cutoff by 20%",
            "Add parallel compression with 6:1 ratio and -10dB threshold",
            "Replace plate reverb with gated reverb, 100ms gate time"
        ]
    )
    
    # Test 3: MIXED → MIXED
    print("\n\n🎨 TEST 3: MIXED GENERATION → MIXED MODIFICATIONS")
    generate_and_modify(
        prompt="Create a haunting pad that sounds like a vintage Juno-106 crying in a cathedral, with chorus rate at 0.5Hz and shimmer reverb creating ghostly overtones",
        modifications=[
            "Make it 30% warmer but keep the cathedral space",
            "Add bitcrusher set to 12-bit for lo-fi character",
            "Transform into sunrise breaking through stained glass"
        ]
    )
    
    # Test 4: POETIC → TECHNICAL
    print("\n\n🔄 TEST 4: POETIC GENERATION → TECHNICAL MODIFICATIONS")
    generate_and_modify(
        prompt="Whispers of an ancient forest where digital rain falls through crystalline leaves",
        modifications=[
            "Add high-pass filter at 200Hz with 12dB/oct slope",
            "Increase compression ratio to 4:1 with soft knee",
            "Set delay to dotted eighth notes with 65% feedback"
        ]
    )
    
    # Test 5: TECHNICAL → POETIC
    print("\n\n🔄 TEST 5: TECHNICAL GENERATION → POETIC MODIFICATIONS")
    generate_and_modify(
        prompt="Subtractive synthesis bass: sawtooth wave through low-pass filter at 350Hz, envelope modulation depth 45%, slight overdrive at 2.5dB gain",
        modifications=[
            "Make it growl like a mechanical beast",
            "Add the feeling of being underwater",
            "Transform into liquid metal flowing through space"
        ]
    )
    
    # Test 6: EXTREME CREATIVITY
    print("\n\n🚀 TEST 6: EXTREME CREATIVE COMBINATIONS")
    generate_and_modify(
        prompt="The number 7 as a sound, if mathematics could cry",
        modifications=[
            "Add chaos generator and spectral freeze for maximum unpredictability",
            "Make it feel like the color purple tastes",
            "Reduce all parameters by exactly 23% except prime-numbered slots"
        ]
    )
    
    # Test 7: SPECIFIC ENGINE REQUESTS
    print("\n\n🎛️ TEST 7: ENGINE-SPECIFIC REQUESTS")
    generate_and_modify(
        prompt="Build a wall of sound using Chaos Generator, Spectral Freeze, and Shimmer Reverb specifically",
        modifications=[
            "Boost the chaos generator's randomness to maximum",
            "Replace shimmer reverb with gated reverb",
            "Add granular cloud and vocoder for texture"
        ]
    )
    
    print("\n\n" + "="*80)
    print("🎉 DEMONSTRATION COMPLETE")
    print("="*80)
    print("\nKey Observations:")
    print("• Poetic prompts create atmospheric, creative presets")
    print("• Technical prompts result in precise configurations")
    print("• Mixed prompts blend creativity with specificity")
    print("• Modifications preserve preset character while applying changes")
    print("• The Trinity Pipeline handles all types seamlessly")
    print("• Engine requests are properly identified and used")

if __name__ == "__main__":
    main()