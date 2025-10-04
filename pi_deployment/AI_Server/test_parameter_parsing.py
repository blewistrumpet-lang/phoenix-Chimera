#!/usr/bin/env python3
"""
Test parameter parsing functionality
"""

from calculator_max_intelligence import MaxIntelligenceCalculator
import logging

logging.basicConfig(level=logging.INFO, format='%(message)s')

calc = MaxIntelligenceCalculator()

test_prompts = [
    'tape delay at 1/8 dotted with 35% feedback',
    'compression with 4:1 ratio and -10dB threshold',
    'reverb with 50% mix and long decay',
    'high pass filter at 80Hz with gentle Q',
    'vintage tube saturation with 60% drive',
    'shimmer reverb with 2.5 second decay',
    'multiband compression with crossover at 200Hz and 2kHz'
]

print("\n" + "="*70)
print("PARAMETER PARSING INTELLIGENCE TEST")
print("="*70)

for prompt in test_prompts:
    print(f'\n{"="*60}')
    print(f'Prompt: "{prompt}"')
    print("="*60)
    
    extracted = calc.parse_prompt_values(prompt)
    
    if extracted:
        print('\n✅ Extracted values:')
        for key, value in extracted.items():
            original = value.get("original", "")
            parsed_value = value.get("value", 0)
            value_type = value.get("type", "")
            
            print(f'  • {key}:')
            print(f'    Original: {original}')
            print(f'    Parsed: {parsed_value:.3f}')
            print(f'    Type: {value_type}')
            
            # Show what this would mean for parameters
            if "percentage" in key:
                hint = value.get("hint", "")
                print(f'    → Would set "{hint}" parameter to {parsed_value:.1%}')
            elif "time_subdivision" in key:
                print(f'    → Would set delay time for musical subdivision')
            elif "ratio" in key:
                actual = value.get("actual_ratio", 0)
                print(f'    → Would set compression ratio to {actual:.1f}:1')
    else:
        print('\n⚠️ No specific values extracted')

print(f'\n{"="*70}')
print("TEST COMPLETE")
print("="*70)

# Test with actual preset
print("\n\nTESTING WITH ACTUAL PRESET:")
print("="*70)

test_preset = {
    "name": "Test",
    "slots": [
        {
            "slot": 0,
            "engine_id": 34,  # Tape Echo
            "engine_name": "Tape Echo",
            "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
        },
        {
            "slot": 1,
            "engine_id": 2,  # Classic Compressor
            "engine_name": "Classic Compressor", 
            "parameters": [{"name": f"param{i+1}", "value": 0.5} for i in range(15)]
        }
    ]
}

prompt = "tape delay at 1/8 dotted with 35% feedback and 4:1 compression"
print(f'Applying to preset: "{prompt}"')
print("\nBEFORE optimization:")
print("  Tape Echo: all parameters = 0.5")
print("  Compressor: all parameters = 0.5")

# Apply basic extraction (without Claude)
extracted = calc.parse_prompt_values(prompt)

# Manually apply to show what would happen
print("\nAFTER optimization would set:")
if "time_subdivision" in extracted:
    print(f"  Tape Echo param1 (Time): {extracted['time_subdivision']['value']:.3f}")
if "percentage_feedback" in extracted:
    print(f"  Tape Echo param2 (Feedback): {extracted['percentage_feedback']['value']:.3f}")
if "ratio" in extracted:
    print(f"  Compressor param3 (Ratio): {extracted['ratio']['value']:.3f}")

print("\n✨ This shows the Calculator can intelligently parse and apply user requests!")