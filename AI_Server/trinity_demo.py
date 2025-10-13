#!/usr/bin/env python3
"""
Trinity Pipeline Demo - Shows how prompts flow through the system
"""

import json
from typing import Dict, Any

def demonstrate_trinity_flow():
    """
    Demonstrate how the Trinity processes various prompts
    """
    
    print("=" * 80)
    print("🎭 TRINITY PIPELINE DEMONSTRATION")
    print("=" * 80)
    
    # Example prompts to demonstrate
    demo_prompts = [
        "Warm vintage guitar tone with tape echo",
        "Aggressive metal rhythm with tight gate", 
        "Ethereal ambient pad with shimmer reverb",
        "Punchy drum bus compression",
        "Lo-fi bedroom producer sound with bit crushing"
    ]
    
    for prompt in demo_prompts:
        print(f"\n{'='*80}")
        print(f"📝 USER PROMPT: '{prompt}'")
        print("=" * 80)
        demonstrate_single_prompt(prompt)
        print("\n" + "─" * 80)

def demonstrate_single_prompt(prompt: str):
    """
    Show step-by-step processing of a single prompt
    """
    
    # STEP 1: VISIONARY
    print("\n⭐ STEP 1: VISIONARY (AI Creative Mind)")
    print("-" * 40)
    
    visionary_output = simulate_visionary(prompt)
    
    print("🤖 Visionary Analysis:")
    print(f"  - Detected character: {visionary_output['character']}")
    print(f"  - Detected intensity: {visionary_output['intensity']}")
    print(f"  - Genre context: {visionary_output['genre']}")
    
    print("\n📦 Visionary generates complete preset:")
    print(f"  Name: '{visionary_output['preset']['name']}'")
    print(f"  Engines selected:")
    for slot in visionary_output['preset']['slots']:
        params_preview = str(slot['parameters'][:3])[:-1] + ", ...]"
        print(f"    Slot {slot['slot']}: {slot['engine_name']}")
        print(f"      Parameters: {params_preview}")
        print(f"      Reason: {slot['reason']}")
    
    # STEP 2: CALCULATOR
    print("\n🧮 STEP 2: CALCULATOR (AI Musical Intelligence)")
    print("-" * 40)
    
    calculator_output = simulate_calculator(visionary_output['preset'], prompt)
    
    print("🎼 Calculator Refinements:")
    print(f"  Signal chain: {calculator_output['signal_chain_analysis']}")
    print(f"  Parameter adjustments: {calculator_output['parameter_adjustments']}")
    print(f"  Musical reasoning: {calculator_output['musical_reasoning']}")
    
    print("\n📐 Optimized signal chain order:")
    for slot in calculator_output['preset']['slots']:
        print(f"    {slot['slot']}. {slot['engine_name']} → ", end="")
    print("Output")
    
    # STEP 3: ALCHEMIST
    print("\n⚗️ STEP 3: ALCHEMIST (Safety Engineer)")
    print("-" * 40)
    
    alchemist_output = simulate_alchemist(calculator_output['preset'])
    
    print("🛡️ Safety Validation:")
    for check, status in alchemist_output['checks'].items():
        print(f"  {status} {check}")
    
    if alchemist_output['modifications']:
        print("\n🔧 Safety modifications:")
        for mod in alchemist_output['modifications']:
            print(f"    - {mod}")
    
    # FINAL OUTPUT
    print("\n✨ FINAL PRESET (Plugin-Ready)")
    print("-" * 40)
    
    final = alchemist_output['final_preset']
    print(f"Name: {final['name']}")
    print(f"Safety Certified: {'✅' if final['safety_certified'] else '❌'}")
    print("\nPlugin Format:")
    print(f"  slot1_engine: {final.get('slot1_engine', 0)}")
    print(f"  slot1_param0: {final.get('slot1_param0', 0.0):.3f}")
    print(f"  slot1_param1: {final.get('slot1_param1', 0.0):.3f}")
    print(f"  ... (all parameters ready for plugin)")

def simulate_visionary(prompt: str) -> Dict[str, Any]:
    """
    Simulate what Visionary would generate (in reality, this calls OpenAI)
    """
    
    # Analyze prompt
    prompt_lower = prompt.lower()
    
    character = "balanced"
    if "warm" in prompt_lower or "vintage" in prompt_lower:
        character = "warm"
    elif "aggressive" in prompt_lower or "metal" in prompt_lower:
        character = "aggressive"
    elif "ethereal" in prompt_lower or "ambient" in prompt_lower:
        character = "spacious"
    elif "punchy" in prompt_lower:
        character = "punchy"
    elif "lo-fi" in prompt_lower:
        character = "lofi"
    
    intensity = "moderate"
    if "subtle" in prompt_lower:
        intensity = "subtle"
    elif "extreme" in prompt_lower or "heavy" in prompt_lower:
        intensity = "heavy"
    
    # Generate appropriate preset based on prompt
    presets = {
        "warm": {
            "name": "Vintage Warmth",
            "slots": [
                {
                    "slot": 1,
                    "engine_id": 15,
                    "engine_name": "Vintage Tube Preamp",
                    "parameters": [0.5, 0.3, 0.5, 0.5, 0.6, 0.4, 0.5, 0.5, 0.0, 1.0],
                    "reason": "Adds tube warmth and harmonic richness"
                },
                {
                    "slot": 2,
                    "engine_id": 34,
                    "engine_name": "Tape Echo",
                    "parameters": [0.375, 0.35, 0.25, 0.3, 0.35, 0.6, 0.1, 0.3, 0.0, 0.0],
                    "reason": "Vintage tape delay character"
                },
                {
                    "slot": 3,
                    "engine_id": 39,
                    "engine_name": "Plate Reverb",
                    "parameters": [0.5, 0.5, 0.0, 0.3, 0.5, 0.7, 0.2, 0.8, 0.1, 0.0],
                    "reason": "Classic studio ambience"
                }
            ]
        },
        "aggressive": {
            "name": "Metal Destroyer",
            "slots": [
                {
                    "slot": 1,
                    "engine_id": 4,
                    "engine_name": "Noise Gate",
                    "parameters": [0.4, 0.05, 0.3, 0.2, 0.9, 0.2, 0.5, 0.0, 0.0, 0.0],
                    "reason": "Tight gating for precise rhythm"
                },
                {
                    "slot": 2,
                    "engine_id": 21,
                    "engine_name": "Rodent Distortion",
                    "parameters": [0.7, 0.4, 0.5, 0.5, 0.5, 1.0, 0.0, 0.3, 0.0, 0.0],
                    "reason": "Aggressive distortion tone"
                },
                {
                    "slot": 3,
                    "engine_id": 7,
                    "engine_name": "Parametric EQ",
                    "parameters": [0.2, 0.4, 0.5, 0.5, 0.6, 0.5, 0.8, 0.5, 0.5, 0.0],
                    "reason": "Sculpt the metal tone"
                },
                {
                    "slot": 4,
                    "engine_id": 43,
                    "engine_name": "Gated Reverb",
                    "parameters": [0.6, 0.3, 0.5, 0.3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                    "reason": "Big room sound with tight cutoff"
                }
            ]
        },
        "spacious": {
            "name": "Ethereal Dreamscape",
            "slots": [
                {
                    "slot": 1,
                    "engine_id": 23,
                    "engine_name": "Digital Chorus",
                    "parameters": [0.2, 0.3, 0.3, 0.0, 0.3, 0.8, 0.8, 0.5, 0.0, 0.0],
                    "reason": "Add movement and width"
                },
                {
                    "slot": 2,
                    "engine_id": 42,
                    "engine_name": "Shimmer Reverb",
                    "parameters": [0.7, 0.4, 0.5, 0.35, 1.0, 0.5, 0.3, 0.0, 0.0, 0.0],
                    "reason": "Ethereal pitched reverb tails"
                },
                {
                    "slot": 3,
                    "engine_id": 46,
                    "engine_name": "Dimension Expander",
                    "parameters": [0.6, 0.4, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                    "reason": "Create 3D spatial depth"
                }
            ]
        },
        "punchy": {
            "name": "Drum Bus Punch",
            "slots": [
                {
                    "slot": 1,
                    "engine_id": 2,
                    "engine_name": "Classic Compressor",
                    "parameters": [0.3, 0.6, 0.2, 0.3, 0.0, 0.5, 1.0, 0.2, 0.0, 0.0],
                    "reason": "Punchy VCA compression"
                },
                {
                    "slot": 2,
                    "engine_id": 3,
                    "engine_name": "Transient Shaper",
                    "parameters": [0.6, 0.4, 0.3, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                    "reason": "Enhance attack transients"
                },
                {
                    "slot": 3,
                    "engine_id": 7,
                    "engine_name": "Parametric EQ",
                    "parameters": [0.15, 0.6, 0.4, 0.35, 0.5, 0.5, 0.75, 0.55, 0.5, 0.0],
                    "reason": "Punchy frequency shaping"
                }
            ]
        },
        "lofi": {
            "name": "Bedroom Lo-Fi",
            "slots": [
                {
                    "slot": 1,
                    "engine_id": 18,
                    "engine_name": "Bit Crusher",
                    "parameters": [0.3, 0.2, 0.7, 0.5, 0.5, 0.0, 1.0, 0.0, 0.0, 0.0],
                    "reason": "Digital degradation and character"
                },
                {
                    "slot": 2,
                    "engine_id": 9,
                    "engine_name": "Ladder Filter",
                    "parameters": [0.4, 0.3, 0.2, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0],
                    "reason": "Warm analog filtering"
                },
                {
                    "slot": 3,
                    "engine_id": 34,
                    "engine_name": "Tape Echo",
                    "parameters": [0.25, 0.4, 0.4, 0.4, 0.3, 0.5, 0.2, 0.5, 0.0, 0.0],
                    "reason": "Vintage tape wobble"
                },
                {
                    "slot": 4,
                    "engine_id": 40,
                    "engine_name": "Spring Reverb",
                    "parameters": [0.4, 0.4, 0.5, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
                    "reason": "Lo-fi spring character"
                }
            ]
        }
    }
    
    # Detect genre
    genre = "general"
    if "metal" in prompt_lower:
        genre = "metal"
    elif "jazz" in prompt_lower or "vintage" in prompt_lower:
        genre = "jazz/vintage"
    elif "ambient" in prompt_lower or "ethereal" in prompt_lower:
        genre = "ambient"
    elif "drum" in prompt_lower:
        genre = "drums"
    elif "bedroom" in prompt_lower or "lo-fi" in prompt_lower:
        genre = "lofi"
    
    # Select appropriate preset
    if "warm" in prompt_lower or "vintage" in prompt_lower:
        preset = presets["warm"]
    elif "aggressive" in prompt_lower or "metal" in prompt_lower:
        preset = presets["aggressive"]
    elif "ethereal" in prompt_lower or "ambient" in prompt_lower:
        preset = presets["spacious"]
    elif "punchy" in prompt_lower or "drum" in prompt_lower:
        preset = presets["punchy"]
    elif "lo-fi" in prompt_lower or "bedroom" in prompt_lower:
        preset = presets["lofi"]
    else:
        preset = presets["warm"]  # Default
    
    return {
        "character": character,
        "intensity": intensity,
        "genre": genre,
        "preset": preset
    }

def simulate_calculator(preset: Dict, prompt: str) -> Dict[str, Any]:
    """
    Simulate Calculator's refinement (in reality, this calls OpenAI)
    """
    
    # Analyze current signal chain
    current_order = [slot['engine_name'] for slot in preset['slots']]
    
    # Determine if reordering needed
    needs_reordering = False
    signal_chain_analysis = "Signal chain is already optimal"
    
    # Check for specific issues
    for i, slot in enumerate(preset['slots']):
        if slot['engine_id'] == 4 and i > 0:  # Gate not first
            needs_reordering = True
            signal_chain_analysis = "Moving gate to first position for clean input"
            break
        if slot['engine_id'] in [39, 40, 41, 42, 43] and i < len(preset['slots']) - 2:  # Reverb too early
            needs_reordering = True
            signal_chain_analysis = "Moving reverb to end of chain"
            break
    
    # Reorder if needed
    if needs_reordering:
        # Sort by engine category
        category_order = {
            4: 0,  # Gate
            1:1, 2:1, 3:1, 5:1,  # Dynamics
            7:2, 8:2, 9:2,  # EQ/Filter
            15:3, 16:3, 17:3, 18:3, 20:3, 21:3, 22:3,  # Distortion
            23:4, 24:4, 25:4,  # Modulation
            34:5, 35:5, 36:5,  # Delay
            39:6, 40:6, 41:6, 42:6, 43:6,  # Reverb
            44:7, 45:7, 46:7  # Spatial
        }
        
        preset['slots'].sort(key=lambda x: category_order.get(x['engine_id'], 8))
        
        # Renumber slots
        for i, slot in enumerate(preset['slots']):
            slot['slot'] = i + 1
    
    # Parameter adjustments based on character
    parameter_adjustments = "No adjustments needed"
    
    if "warm" in prompt.lower():
        parameter_adjustments = "Reduced highs by 15%, increased tube harmonics"
        # Adjust parameters for warmth
        for slot in preset['slots']:
            if slot['engine_id'] == 15:  # Tube
                slot['parameters'][6] = min(1.0, slot['parameters'][6] + 0.2)
    
    elif "aggressive" in prompt.lower():
        parameter_adjustments = "Increased drive by 20%, faster attack times"
        for slot in preset['slots']:
            if slot['engine_id'] in [20, 21]:  # Distortions
                slot['parameters'][0] = min(1.0, slot['parameters'][0] * 1.2)
    
    elif "subtle" in prompt.lower():
        parameter_adjustments = "Reduced all mix levels by 30%"
        for slot in preset['slots']:
            # Find mix parameter (varies by engine)
            mix_indices = {15: 9, 18: 2, 23: 2, 34: 4, 39: 3, 42: 3}
            if slot['engine_id'] in mix_indices:
                idx = mix_indices[slot['engine_id']]
                if idx < len(slot['parameters']):
                    slot['parameters'][idx] *= 0.7
    
    # Musical reasoning
    musical_reasoning = f"Optimized for {prompt.split()[0]} sound with proper gain staging and frequency balance"
    
    return {
        "signal_chain_analysis": signal_chain_analysis,
        "parameter_adjustments": parameter_adjustments,
        "musical_reasoning": musical_reasoning,
        "preset": preset
    }

def simulate_alchemist(preset: Dict) -> Dict[str, Any]:
    """
    Simulate Alchemist's safety validation
    """
    
    modifications = []
    
    # Check for safety issues
    for slot in preset['slots']:
        params = slot['parameters']
        
        # Check parameter ranges
        for i in range(len(params)):
            if params[i] > 1.0:
                params[i] = 1.0
                modifications.append(f"Clamped {slot['engine_name']} param {i} to 1.0")
            elif params[i] < 0.0:
                params[i] = 0.0
                modifications.append(f"Clamped {slot['engine_name']} param {i} to 0.0")
        
        # Check feedback safety
        if slot['engine_id'] in [34, 35, 36, 37]:  # Delays
            if params[1] > 0.85:  # Feedback parameter
                params[1] = 0.85
                modifications.append(f"Limited {slot['engine_name']} feedback to prevent runaway")
    
    # Convert to plugin format
    final_preset = {
        "name": preset['name'],
        "safety_certified": True
    }
    
    for i, slot in enumerate(preset['slots'], 1):
        if i > 6:
            break
        final_preset[f"slot{i}_engine"] = slot['engine_id']
        for j, param in enumerate(slot['parameters']):
            final_preset[f"slot{i}_param{j}"] = round(param, 3)
    
    # Fill empty slots
    for i in range(len(preset['slots']) + 1, 7):
        final_preset[f"slot{i}_engine"] = 0
        for j in range(10):
            final_preset[f"slot{i}_param{j}"] = 0.0
    
    return {
        "checks": {
            "Parameter ranges": "✅",
            "Gain staging": "✅",
            "Feedback safety": "✅",
            "CPU optimization": "✅",
            "Mono compatibility": "✅"
        },
        "modifications": modifications[:3],  # Show first 3
        "final_preset": final_preset
    }

if __name__ == "__main__":
    demonstrate_trinity_flow()