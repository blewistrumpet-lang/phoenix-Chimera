#!/usr/bin/env python3
"""
Test hybrid logic without full GPT (just show the constraints)
"""

from visionary_complete import CompleteVisionary
from engine_selector import engine_selector

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


def main():
    """Test hybrid constraint extraction"""
    print("=" * 80)
    print("HYBRID APPROACH - CONSTRAINT EXTRACTION TEST")
    print("=" * 80)

    visionary = CompleteVisionary()

    for prompt in KNOWLEDGE_PROMPTS:
        print(f"\n📝 Prompt: {prompt}")

        # Step 1: Extract character even from knowledge prompts
        character = engine_selector.detect_character(prompt)
        print(f"   🎭 Character detected: {character}")

        # Step 2: Get forbidden engines
        if character != "neutral":
            rule = engine_selector.RULES.get(character, {})
            forbidden = rule.get("forbidden", [])
            forbidden_names = [visionary.engines.get(str(eid), {}).get('name', f'Engine {eid}')
                             for eid in forbidden]

            if forbidden_names:
                print(f"   🚫 Forbidden: {', '.join(forbidden_names)}")
            else:
                print(f"   ✅ No forbidden engines")
        else:
            print(f"   ✅ Neutral character - no constraints")

        print(f"   💡 GPT would select engines with these constraints...")

    print(f"\n{'=' * 80}")


if __name__ == "__main__":
    main()
