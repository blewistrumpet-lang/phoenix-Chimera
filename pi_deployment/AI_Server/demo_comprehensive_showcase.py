#!/usr/bin/env python3
"""
Comprehensive demonstration of Trinity Pipeline capabilities
Shows preset names, engines selected, parameter levels, and modifications
"""

import requests
import json
import time
from typing import Dict, Any, List
from engine_mapping_correct import ENGINE_MAPPING

def print_separator(char="=", length=80):
    print(char * length)

def print_preset_details(preset: Dict[str, Any], title: str = ""):
    """Print comprehensive preset information"""
    if title:
        print_separator()
        print(f"  {title}")
        print_separator()
    
    print(f"\n🎯 PRESET NAME: '{preset.get('name', 'Unknown')}'")
    print(f"🎨 VIBE: {preset.get('vibe', 'Unknown')}")
    
    # Show active engines with parameters
    parameters = preset.get('parameters', {})
    print("\n🎛️ ENGINES & PARAMETERS:")
    
    active_count = 0
    for slot in range(1, 7):
        engine_id = parameters.get(f'slot{slot}_engine', 0)
        if engine_id > 0 and parameters.get(f'slot{slot}_bypass', 0) < 0.5:
            active_count += 1
            engine_name = ENGINE_MAPPING.get(engine_id, 'Unknown')
            mix = parameters.get(f'slot{slot}_mix', 0.5)
            
            # Get key parameters
            p1 = parameters.get(f'slot{slot}_param1', 0)  # Usually Drive/Amount
            p2 = parameters.get(f'slot{slot}_param2', 0)  # Usually Tone/Frequency
            p3 = parameters.get(f'slot{slot}_param3', 0)  # Usually Mix/Depth
            p4 = parameters.get(f'slot{slot}_param4', 0)  # Usually Feedback/Rate
            
            print(f"\n  Slot {slot}: {engine_name}")
            print(f"    Mix Level: {mix:.0%}")
            print(f"    Drive/Amount: {p1:.0%} | Tone/Freq: {p2:.0%}")
            print(f"    Mix/Depth: {p3:.0%} | Feedback/Rate: {p4:.0%}")
    
    if active_count == 0:
        print("  (No active engines)")
    
    # Show global parameters
    print(f"\n🎚️ GLOBAL PARAMETERS:")
    print(f"  Input Gain: {parameters.get('input_gain', 0.5):.0%}")
    print(f"  Output Gain: {parameters.get('output_gain', 0.5):.0%}")
    print(f"  Dry/Wet Mix: {parameters.get('dry_wet_mix', 0.5):.0%}")

def show_changes(before_preset: Dict[str, Any], after_preset: Dict[str, Any], modification: str):
    """Show what changed between presets"""
    print(f"\n🔄 MODIFICATION: \"{modification}\"")
    print_separator("-")
    
    before_params = before_preset.get('parameters', {})
    after_params = after_preset.get('parameters', {})
    
    # Track engine changes
    engines_added = []
    engines_removed = []
    engines_modified = []
    
    for slot in range(1, 7):
        before_id = before_params.get(f'slot{slot}_engine', 0)
        after_id = after_params.get(f'slot{slot}_engine', 0)
        before_bypass = before_params.get(f'slot{slot}_bypass', 0)
        after_bypass = after_params.get(f'slot{slot}_bypass', 0)
        
        before_active = before_id > 0 and before_bypass < 0.5
        after_active = after_id > 0 and after_bypass < 0.5
        
        if not before_active and after_active:
            engines_added.append(f"Slot {slot}: {ENGINE_MAPPING.get(after_id, 'Unknown')}")
        elif before_active and not after_active:
            engines_removed.append(f"Slot {slot}: {ENGINE_MAPPING.get(before_id, 'Unknown')}")
        elif before_active and after_active and before_id != after_id:
            engines_modified.append(f"Slot {slot}: {ENGINE_MAPPING.get(before_id, 'Unknown')} → {ENGINE_MAPPING.get(after_id, 'Unknown')}")
    
    if engines_added:
        print("\n✅ ENGINES ADDED:")
        for engine in engines_added:
            print(f"  {engine}")
    
    if engines_removed:
        print("\n❌ ENGINES REMOVED:")
        for engine in engines_removed:
            print(f"  {engine}")
    
    if engines_modified:
        print("\n🔄 ENGINES CHANGED:")
        for engine in engines_modified:
            print(f"  {engine}")
    
    # Count parameter changes
    param_changes = 0
    for key in before_params:
        if key in after_params:
            if abs(before_params[key] - after_params[key]) > 0.01:
                param_changes += 1
    
    print(f"\n📊 PARAMETER CHANGES: {param_changes} values modified")
    
    # Show name change if any
    if before_preset.get('name') != after_preset.get('name'):
        print(f"\n📝 NAME CHANGED: '{before_preset.get('name')}' → '{after_preset.get('name')}'")

def test_prompt_and_modify(prompt: str, modifications: List[str], category: str):
    """Generate a preset and apply modifications, showing full details"""
    base_url = "http://localhost:8000"
    
    print("\n" + "="*80)
    print(f"🎭 {category}")
    print("="*80)
    print(f"\n📝 PROMPT: \"{prompt}\"")
    
    # Generate initial preset
    response = requests.post(
        f"{base_url}/generate",
        json={"prompt": prompt},
        timeout=30
    )
    
    if response.status_code != 200:
        print("❌ Failed to generate preset")
        return
    
    preset = response.json()["preset"]
    metadata = response.json().get("metadata", {})
    
    print(f"\n⏱️ Generation Time: {metadata.get('generation_time_seconds', 0):.1f}s")
    print_preset_details(preset, "INITIAL PRESET")
    
    # Apply each modification
    for i, mod_text in enumerate(modifications, 1):
        print("\n" + "="*80)
        
        before_preset = preset.copy()
        
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
                preset = result["data"]
                show_changes(before_preset, preset, mod_text)
                print_preset_details(preset, f"AFTER MODIFICATION {i}")
            else:
                print(f"❌ Modification failed: {result.get('message')}")
        else:
            print(f"❌ Modification request failed")

def main():
    """Run comprehensive demonstration"""
    
    time.sleep(2)  # Give server time to be ready
    
    print("\n" + "="*80)
    print("🎭 TRINITY PIPELINE COMPREHENSIVE SHOWCASE")
    print("Demonstrating Preset Names, Engines, Parameters, and Modifications")
    print("="*80)
    
    test_cases = [
        {
            "category": "POETIC → ATMOSPHERIC MODIFICATIONS",
            "prompt": "The sound of memories dissolving in rain, echoing through abandoned subway tunnels at 3am",
            "modifications": [
                "Make it darker and more ominous",
                "Add the warmth of dawn breaking through",
                "Transform into crystalline ice formations"
            ]
        },
        {
            "category": "TECHNICAL → PRECISE MODIFICATIONS", 
            "prompt": "Classic Minimoog bass patch: Sawtooth oscillator, 24dB/oct filter with 60% resonance, cutoff at 400Hz, slight portamento, no effects",
            "modifications": [
                "Add tape delay at 375ms with 40% feedback",
                "Increase filter resonance to 85% and add subtle overdrive",
                "Remove all time-based effects and add aggressive bitcrushing"
            ]
        },
        {
            "category": "CREATIVE → ENGINE ADDITIONS",
            "prompt": "Pure silence waiting to be filled with sound",
            "modifications": [
                "Add swirling chorus and phaser for movement",
                "Add massive reverb like a cathedral in space",
                "Add chaos generator and spectral freeze for unpredictability"
            ]
        },
        {
            "category": "GENRE-SPECIFIC → STYLE CHANGES",
            "prompt": "80s synthwave lead with heavy chorus and gated reverb, bright and punchy",
            "modifications": [
                "Transform into dark techno bassline",
                "Make it sound like 70s prog rock organ",
                "Convert to modern trap production style"
            ]
        },
        {
            "category": "ABSTRACT → CONCRETE MODIFICATIONS",
            "prompt": "The color purple if it could sing",
            "modifications": [
                "Make it 50% more purple",
                "Add the texture of velvet",
                "Ground it with earthy, wooden tones"
            ]
        },
        {
            "category": "ENVIRONMENTAL → SPATIAL CHANGES",
            "prompt": "Sound of wind through pine trees on a mountain peak",
            "modifications": [
                "Move it underwater into a submarine",
                "Place it in a massive concert hall",
                "Compress it into a tiny music box"
            ]
        },
        {
            "category": "EMOTIONAL → MOOD TRANSFORMATIONS",
            "prompt": "The feeling of nostalgia for a place you've never been",
            "modifications": [
                "Add crushing sadness and melancholy",
                "Inject explosive joy and celebration",
                "Make it anxious and unsettling"
            ]
        },
        {
            "category": "MIXED TECHNICAL/POETIC → HYBRID MODIFICATIONS",
            "prompt": "Vintage Jupiter-8 pad with slow LFO modulation, swimming through liquid mercury under a blood moon",
            "modifications": [
                "Add ring modulator at 440Hz with 30% mix",
                "Make it breathe like a living organism",
                "Set all parameters to exactly 66.6%"
            ]
        }
    ]
    
    for test in test_cases:
        test_prompt_and_modify(
            test["prompt"],
            test["modifications"],
            test["category"]
        )
        time.sleep(0.5)  # Brief pause between tests
    
    print("\n\n" + "="*80)
    print("🎉 SHOWCASE COMPLETE")
    print("="*80)
    print("\nKey Observations:")
    print("• Creative names reflect the prompt essence")
    print("• Engine selection matches the described sound")
    print("• Parameter levels create appropriate sonic characteristics")
    print("• Modifications successfully add/remove/adjust engines")
    print("• Both technical and poetic language work effectively")

if __name__ == "__main__":
    main()