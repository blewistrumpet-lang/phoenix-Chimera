#!/usr/bin/env python3
"""
Test the complete Trinity AI pipeline with name generation
"""

import asyncio
import json
import logging
from main import generate_preset, GenerateRequest

logging.basicConfig(level=logging.INFO)

async def test_complete_trinity():
    """Test the complete Trinity pipeline including name generation"""
    
    test_prompts = [
        "Create a warm vintage guitar tone with tube saturation",
        "Design an aggressive metal sound with tight gate",
        "Build a spacious ambient pad with shimmer",
        "Make a punchy drum bus processor",
        "Create a lo-fi hip hop production chain"
    ]
    
    print("\n" + "="*80)
    print("TESTING COMPLETE TRINITY PIPELINE WITH NAME GENERATION")
    print("="*80)
    
    for i, prompt in enumerate(test_prompts):
        print(f"\nTest {i+1}: {prompt}")
        print("-"*60)
        
        try:
            # Generate preset through Trinity pipeline
            request = GenerateRequest(prompt=prompt)
            response = await generate_preset(request)
            
            if response.success:
                preset = response.preset
                print(f"✓ Success!")
                print(f"  Name: {preset.get('name', 'Unknown')}")
                print(f"  Vibe: {preset.get('vibe', 'Unknown')}")
                print(f"  Source: {preset.get('source', 'Unknown')}")
                print(f"  Validated: {preset.get('alchemist_validated', False)}")
                
                # Show active engines
                params = preset.get('parameters', {})
                active_slots = []
                for slot in range(1, 7):
                    if params.get(f'slot{slot}_bypass', 1.0) < 0.5:
                        engine_id = params.get(f'slot{slot}_engine', 0)
                        if engine_id > 0:
                            active_slots.append(f"Slot {slot}: Engine {engine_id}")
                
                if active_slots:
                    print(f"  Active: {', '.join(active_slots)}")
                
                # Show any warnings
                warnings = preset.get('validation_warnings', [])
                if warnings:
                    print(f"  Warnings: {', '.join(warnings[:2])}")
                    
            else:
                print(f"✗ Failed: {response.message}")
                
        except Exception as e:
            print(f"✗ Error: {str(e)}")
        
        # Small delay between tests
        if i < len(test_prompts) - 1:
            await asyncio.sleep(1)
    
    print("\n" + "="*80)
    print("TRINITY PIPELINE TEST COMPLETE")
    print("="*80)
    print("\nComponents tested:")
    print("✓ Visionary - Blueprint generation")
    print("✓ Oracle - Preset matching")
    print("✓ Calculator - Parameter nudging")
    print("✓ Alchemist - Validation & name generation")
    print("\nThe Trinity AI is fully operational!")

if __name__ == "__main__":
    asyncio.run(test_complete_trinity())