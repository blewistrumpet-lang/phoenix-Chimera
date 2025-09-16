#!/usr/bin/env python3
"""
Simple test of OpenAI connection
"""

import asyncio
import json
from openai import OpenAI
import os

async def test_direct_openai():
    """Test OpenAI API directly"""
    print("\n" + "="*80)
    print("TESTING DIRECT OPENAI CONNECTION")
    print("="*80)
    
    # Get API key
    api_key = os.getenv("OPENAI_API_KEY")
    
    # Create client
    client = OpenAI(api_key=api_key)
    
    # Build a simple version of the Visionary prompt
    system_prompt = """You are the Visionary AI for Chimera Phoenix. Output ONLY valid JSON.
    
Available engines (partial list):
- 0: Vintage Tube (warm, saturated)
- 1: Tape Echo (vintage, delay)
- 36: Rodent Distortion (aggressive, modern)
- 7: VCA Compressor (punchy, clean)
- 2: Shimmer Reverb (ethereal, spacious)

Return this exact JSON structure:
{
    "slots": [
        {"slot": 1, "engine_id": <int>, "character": "<string>"},
        {"slot": 2, "engine_id": <int>, "character": "<string>"},
        {"slot": 3, "engine_id": <int>, "character": "<string>"},
        {"slot": 4, "engine_id": -1, "character": "bypass"},
        {"slot": 5, "engine_id": -1, "character": "bypass"},
        {"slot": 6, "engine_id": -1, "character": "bypass"}
    ],
    "overall_vibe": "<string>"
}"""

    # Test prompts
    prompts = [
        "Create a warm vintage tone",
        "Build an aggressive metal sound with compression",
        "Design a spacious ambient soundscape"
    ]
    
    for i, prompt in enumerate(prompts):
        print(f"\nTest {i+1}: {prompt}")
        print("-"*80)
        
        try:
            response = client.chat.completions.create(
                model="gpt-3.5-turbo",
                messages=[
                    {"role": "system", "content": system_prompt},
                    {"role": "user", "content": prompt}
                ],
                temperature=0.7,
                max_tokens=500,
                response_format={"type": "json_object"}
            )
            
            content = response.choices[0].message.content
            blueprint = json.loads(content)
            
            print(f"✓ Success! OpenAI returned:")
            print(f"  Vibe: {blueprint.get('overall_vibe', 'Unknown')}")
            
            # Show active engines
            for slot in blueprint.get('slots', []):
                if slot.get('engine_id', -1) >= 0:
                    print(f"  Slot {slot['slot']}: Engine {slot['engine_id']} ({slot['character']})")
                    
        except Exception as e:
            print(f"✗ Error: {str(e)}")

async def test_through_bridge():
    """Test through the bridge server (assuming it's already running)"""
    print("\n" + "="*80)
    print("TESTING THROUGH BRIDGE SERVER")
    print("="*80)
    
    from visionary_enhanced import VisionaryEnhanced
    
    visionary = VisionaryEnhanced()
    
    # Test a simple prompt
    prompt = "Create an aggressive metal guitar tone"
    
    try:
        blueprint = await visionary.get_blueprint(prompt)
        print(f"\nPrompt: {prompt}")
        print(f"Response vibe: {blueprint.get('overall_vibe', 'Unknown')}")
        
        # Check if this is simulation or real OpenAI
        if blueprint.get('overall_vibe') == 'balanced warmth':
            print("⚠️  Got default blueprint - likely using simulation")
        else:
            print("✓ Got custom blueprint - likely from OpenAI!")
            
    except Exception as e:
        print(f"Error: {str(e)}")

async def main():
    # Test direct connection first
    await test_direct_openai()
    
    # Then test through bridge
    print("\n" + "="*60)
    print("NOTE: Make sure openai_bridge_server.py is running!")
    print("Run in another terminal: python3 openai_bridge_server.py")
    print("="*60)
    
    await asyncio.sleep(2)
    await test_through_bridge()

if __name__ == "__main__":
    asyncio.run(main())