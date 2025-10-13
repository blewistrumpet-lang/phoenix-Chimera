#!/usr/bin/env python3
"""
Test knowledge-based routing with GPT
"""

import asyncio
import json
from visionary_complete import CompleteVisionary

# Knowledge-based test prompts
KNOWLEDGE_PROMPTS = [
    "Michael Jackson Thriller vocals",
    "Prince purple rain guitar tone",
    "Radiohead Kid A production",
    "1176 compression on vocals",
    "Lexicon 480L reverb",
    "Neve console warmth",
    "Lo-fi hip hop beats",
    "Daft punk vocoder effect",
]


async def test_knowledge_prompt(visionary: CompleteVisionary, prompt: str):
    """Test a single knowledge-based prompt"""
    print(f"\n{'=' * 80}")
    print(f"📝 Prompt: {prompt}")
    print(f"{'=' * 80}")

    try:
        preset = await visionary.generate_complete_preset(prompt)

        # Extract engine IDs and names
        engines = []
        for slot in preset.get('slots', []):
            engine_id = slot.get('engine_id', 0)
            engine_name = slot.get('engine_name', 'Unknown')
            if engine_id != 0:
                engines.append(f"{engine_name} ({engine_id})")

        print(f"✅ Preset: {preset.get('name', 'Unnamed')}")
        print(f"   Engines: {', '.join(engines)}")

        return preset

    except Exception as e:
        print(f"❌ ERROR: {str(e)}")
        import traceback
        traceback.print_exc()
        return None


async def main():
    """Run all knowledge-based tests"""
    print("=" * 80)
    print("KNOWLEDGE-BASED ROUTING TEST")
    print("Testing GPT selection with artist/gear-specific prompts")
    print("=" * 80)

    # Initialize Visionary with OpenAI
    visionary = CompleteVisionary()

    # Check if OpenAI is configured
    if not visionary.client:
        print("\n⚠️  WARNING: No OpenAI API key configured!")
        print("   Knowledge-based routing requires GPT.")
        print("   Set OPENAI_API_KEY in .env file to test.")
        return

    # Run tests
    for prompt in KNOWLEDGE_PROMPTS:
        preset = await test_knowledge_prompt(visionary, prompt)

        # Brief pause between tests
        await asyncio.sleep(1)

    print(f"\n{'=' * 80}")
    print("Knowledge-based tests complete!")
    print(f"{'=' * 80}")


if __name__ == "__main__":
    asyncio.run(main())
