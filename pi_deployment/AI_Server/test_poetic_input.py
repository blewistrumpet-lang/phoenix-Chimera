#!/usr/bin/env python3
"""
Test how the system handles poetic/creative inputs
"""

import asyncio
import json
from visionary_string_ids import VisionaryStringIDs
from oracle_string_ids import OracleStringIDs
from calculator_string_ids import CalculatorStringIDs
from alchemist_string_ids import AlchemistStringIDs

async def test_poetic_prompts():
    """Test the pipeline with poetic, non-technical prompts"""
    
    # Initialize components
    visionary = VisionaryStringIDs()
    oracle = OracleStringIDs()
    calculator = CalculatorStringIDs()
    alchemist = AlchemistStringIDs()
    
    # Test prompts - from poetic to technical
    test_prompts = [
        "mana falling from heaven",
        "the sound of forgotten dreams",
        "crimson sunset melting into purple twilight",
        "ancient whispers in a crystal cave",
        "electric butterflies in a neon garden",
        "the weight of silence before thunder"
    ]
    
    print("="*80)
    print("TESTING POETIC INPUTS - How Trinity Interprets Creative Prompts")
    print("="*80)
    
    for prompt in test_prompts:
        print(f"\n{'='*80}")
        print(f"PROMPT: \"{prompt}\"")
        print("="*80)
        
        # Step 1: Visionary interpretation
        print("\n1. VISIONARY INTERPRETATION:")
        print("-"*40)
        blueprint = await visionary.get_blueprint(prompt)
        
        print(f"Overall Vibe: {blueprint.get('overall_vibe', 'unknown')}")
        print("Engines Selected:")
        for slot in blueprint['slots']:
            if slot['engine'] != 'bypass':
                print(f"  Slot {slot['slot']}: {slot['engine']}")
                # Show why this engine was chosen
                if slot['engine'] == 'shimmer_reverb':
                    print(f"    → Ethereal, heavenly quality")
                elif slot['engine'] == 'granular_cloud':
                    print(f"    → Textural, particle-like")
                elif slot['engine'] == 'dimension_expander':
                    print(f"    → Spatial, expansive")
                elif slot['engine'] == 'spectral_freeze':
                    print(f"    → Frozen, suspended time")
                elif slot['engine'] == 'plate_reverb':
                    print(f"    → Spacious, resonant")
                elif slot['engine'] == 'vintage_tube':
                    print(f"    → Warm, organic")
                elif slot['engine'] == 'tape_echo':
                    print(f"    → Nostalgic, memory-like")
                elif slot['engine'] == 'chaos_generator':
                    print(f"    → Unpredictable, dynamic")
        
        # Step 2: Oracle matching
        print("\n2. ORACLE CORPUS MATCHING:")
        print("-"*40)
        preset = oracle.find_best_preset(blueprint)
        print(f"Matched Preset: {preset.get('name', 'Unknown')}")
        
        # Step 3: Calculator interpretation
        print("\n3. CALCULATOR KEYWORD ANALYSIS:")
        print("-"*40)
        # Analyze what keywords might be extracted
        keywords_found = []
        prompt_lower = prompt.lower()
        
        keyword_interpretations = {
            "heaven": ["spacious", "ethereal", "reverb"],
            "falling": ["downward", "delay", "pitch shift"],
            "mana": ["magical", "shimmer", "mystical"],
            "dreams": ["ambient", "soft", "reverb"],
            "whispers": ["quiet", "gentle", "filter"],
            "crystal": ["bright", "clear", "harmonic"],
            "electric": ["energetic", "modulation", "excitement"],
            "silence": ["gate", "dynamics", "space"],
            "thunder": ["powerful", "compression", "impact"],
            "sunset": ["warm", "vintage", "fading"],
            "twilight": ["mysterious", "ambient", "transition"]
        }
        
        for word, interpretations in keyword_interpretations.items():
            if word in prompt_lower:
                keywords_found.extend(interpretations)
        
        if keywords_found:
            unique_keywords = list(set(keywords_found))[:5]
            print(f"Interpreted as: {', '.join(unique_keywords)}")
        else:
            print("No direct keyword matches - using overall vibe")
        
        nudged = calculator.apply_nudges(preset, prompt, blueprint)
        if nudged.get('calculator_nudges'):
            print(f"Applied nudges: {', '.join(nudged['calculator_nudges'][:3])}")
        
        # Step 4: Alchemist naming
        print("\n4. ALCHEMIST CREATIVE NAMING:")
        print("-"*40)
        final = alchemist.finalize_preset(nudged, prompt)
        print(f"Generated Name: \"{final['name']}\"")
        
        # Final interpretation
        print("\n5. MUSICAL INTERPRETATION:")
        print("-"*40)
        active_engines = []
        for slot in range(1, 7):
            engine = final['parameters'].get(f'slot{slot}_engine', 'bypass')
            if engine != 'bypass':
                active_engines.append(engine)
        
        if 'shimmer_reverb' in active_engines and 'granular_cloud' in active_engines:
            print("→ Creates an ethereal, particle-like texture suggesting divine essence")
        elif 'shimmer_reverb' in active_engines:
            print("→ Adds heavenly, ascending quality with octave-up shimmer")
        elif 'spectral_freeze' in active_engines:
            print("→ Captures and suspends moments like frozen droplets")
        elif 'tape_echo' in active_engines and 'plate_reverb' in active_engines:
            print("→ Creates nostalgic space with echoing memories")
        elif 'vintage_tube' in active_engines:
            print("→ Adds warmth and organic character")
        else:
            print(f"→ Combines {', '.join(active_engines[:3])} for unique character")

# Run specific test for "mana falling from heaven"
async def test_mana_from_heaven():
    """Detailed test of the specific prompt"""
    print("\n" + "="*80)
    print("DETAILED ANALYSIS: 'mana falling from heaven'")
    print("="*80)
    
    visionary = VisionaryStringIDs()
    
    # Get blueprint with OpenAI if available, otherwise simulation
    blueprint = await visionary.get_blueprint("mana falling from heaven")
    
    print("\nVISIONARY'S INTERPRETATION:")
    print(json.dumps(blueprint, indent=2))
    
    print("\nWHY THESE ENGINES?")
    print("-"*40)
    
    # Analyze the selection logic
    selections = {
        "shimmer_reverb": "Ethereal quality, octave-up shimmer = heavenly/divine",
        "granular_cloud": "Particle-like texture = falling droplets/mana particles",
        "dimension_expander": "3D space = vast heavenly expanse",
        "plate_reverb": "Classic space = cathedral/sacred atmosphere",
        "spectral_freeze": "Suspended time = magical moment captured",
        "pitch_shifter": "Downward pitch = falling motion",
        "tape_echo": "Repeating delays = multiple droplets falling"
    }
    
    for slot in blueprint['slots']:
        engine = slot.get('engine', 'bypass')
        if engine != 'bypass' and engine in selections:
            print(f"{engine}: {selections[engine]}")

if __name__ == "__main__":
    asyncio.run(test_poetic_prompts())
    asyncio.run(test_mana_from_heaven())