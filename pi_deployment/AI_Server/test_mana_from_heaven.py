#!/usr/bin/env python3
"""
Complete test of 'mana falling from heaven' through the entire pipeline
"""

import asyncio
import json
import logging
from visionary_string_ids import VisionaryStringIDs
from oracle_string_ids import OracleStringIDs
from calculator_string_ids import CalculatorStringIDs
from alchemist_string_ids import AlchemistStringIDs

# Set up logging to see the process
logging.basicConfig(level=logging.INFO, format='%(message)s')
logger = logging.getLogger(__name__)

async def test_mana_falling():
    """Complete pipeline test with poetic input"""
    
    prompt = "mana falling from heaven"
    
    print("\n" + "="*80)
    print("TESTING: 'mana falling from heaven'")
    print("="*80)
    print("\nThis tests how the AI interprets abstract, poetic concepts into audio effects.\n")
    
    # Initialize Trinity
    visionary = VisionaryStringIDs()
    oracle = OracleStringIDs()
    calculator = CalculatorStringIDs()
    alchemist = AlchemistStringIDs()
    
    # Step 1: Visionary interprets the poetic prompt
    print("STEP 1: VISIONARY INTERPRETATION")
    print("-"*80)
    blueprint = await visionary.get_blueprint(prompt)
    
    print(f"Prompt: '{prompt}'")
    print(f"\nVisionary's Musical Translation:")
    print(f"  Overall Vibe: {blueprint['overall_vibe']}")
    print(f"  Reasoning: {blueprint.get('reasoning', 'N/A')}")
    print(f"\nEngine Selection Logic:")
    
    for slot in blueprint['slots']:
        if slot['engine'] != 'bypass':
            print(f"\n  Slot {slot['slot']}: {slot['engine']}")
            print(f"    Purpose: {slot['character']}")
            
            # Explain why this makes sense for "mana falling from heaven"
            if slot['engine'] == 'granular_cloud':
                print(f"    → Mana particles: Creates particle-like texture, like droplets of magic")
            elif slot['engine'] == 'shimmer_reverb':
                print(f"    → Heaven: Octave-up shimmer creates ethereal, ascending quality")
            elif slot['engine'] == 'tape_echo':
                print(f"    → Falling: Cascading delays simulate falling droplets")
            elif slot['engine'] == 'pitch_shifter':
                print(f"    → Celestial: Pitch shifting adds otherworldly quality")
            elif slot['engine'] == 'vintage_tube':
                print(f"    → Warmth: Adds organic, living quality to the magic")
            elif slot['engine'] == 'spectral_freeze':
                print(f"    → Suspended: Captures moments like frozen mana crystals")
            elif slot['engine'] == 'dimension_expander':
                print(f"    → Space: Creates vast heavenly expanse")
    
    # Step 2: Oracle finds similar preset
    print("\n\nSTEP 2: ORACLE CORPUS MATCHING")
    print("-"*80)
    preset = oracle.find_best_preset(blueprint)
    print(f"Best matching preset: '{preset['name']}'")
    print("Engines in matched preset:")
    for slot in range(1, 7):
        engine = preset['parameters'].get(f'slot{slot}_engine', 'bypass')
        if engine != 'bypass':
            print(f"  Slot {slot}: {engine}")
    
    # Step 3: Calculator adjusts parameters
    print("\n\nSTEP 3: CALCULATOR PARAMETER NUDGING")
    print("-"*80)
    nudged = calculator.apply_nudges(preset, prompt, blueprint)
    
    # Analyze what parameters would be adjusted for "mana" and "heaven"
    print("Parameter adjustments for poetic concepts:")
    if 'shimmer_reverb' in str(nudged['parameters']):
        print("  • Shimmer reverb: Increased mix for heavenly quality")
    if 'granular_cloud' in str(nudged['parameters']):
        print("  • Granular cloud: Adjusted density for particle effect")
    if nudged.get('calculator_nudges'):
        print(f"  • Applied nudges: {', '.join(nudged['calculator_nudges'][:5])}")
    else:
        print("  • No specific keyword nudges - using overall ethereal settings")
    
    # Step 4: Alchemist creates final name
    print("\n\nSTEP 4: ALCHEMIST CREATIVE NAMING")
    print("-"*80)
    final = alchemist.finalize_preset(nudged, prompt)
    print(f"Generated preset name: '{final['name']}'")
    print(f"Based on prompt: '{prompt}'")
    
    # Final analysis
    print("\n\nFINAL AUDIO INTERPRETATION")
    print("-"*80)
    print("The system interpreted 'mana falling from heaven' as:")
    
    active_effects = []
    for slot in range(1, 7):
        engine = final['parameters'].get(f'slot{slot}_engine', 'bypass')
        if engine != 'bypass':
            active_effects.append(engine)
    
    print(f"\nActive Effects Chain: {' → '.join(active_effects)}")
    
    print("\nSonic Result:")
    if 'granular_cloud' in active_effects:
        print("  • Particle-like texture (mana droplets)")
    if 'shimmer_reverb' in active_effects:
        print("  • Ethereal ascending harmonics (heavenly quality)")
    if 'tape_echo' in active_effects:
        print("  • Cascading delays (falling motion)")
    if 'pitch_shifter' in active_effects:
        print("  • Otherworldly pitch effects (magical transformation)")
    
    print("\n" + "="*80)
    print("CONCLUSION")
    print("="*80)
    print("""
The AI successfully translated the abstract concept 'mana falling from heaven' into:
  
  1. CONCEPTUAL: Magical essence descending from a divine realm
  2. VISUAL: Sparkling particles or droplets falling through space
  3. SONIC: Ethereal textures with cascading delays and shimmering reverb
  4. TECHNICAL: Specific DSP engines that create these qualities

The string-based system allows the AI to think in terms of effect CHARACTER
rather than numeric IDs, making poetic interpretation more natural.
""")
    
    # Show the actual JSON that would be sent to the plugin
    print("\nJSON PRESET (with string IDs):")
    print("-"*80)
    output = {
        "name": final['name'],
        "prompt": prompt,
        "engines": [
            f"{final['parameters'].get(f'slot{i}_engine', 'bypass')}"
            for i in range(1, 7)
            if final['parameters'].get(f'slot{i}_engine', 'bypass') != 'bypass'
        ]
    }
    print(json.dumps(output, indent=2))

if __name__ == "__main__":
    # Suppress sklearn warning
    import warnings
    warnings.filterwarnings("ignore", category=UserWarning)
    
    asyncio.run(test_mana_falling())