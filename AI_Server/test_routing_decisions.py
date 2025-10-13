#!/usr/bin/env python3
"""
Test routing decisions (without full GPT generation)
"""

from visionary_complete import CompleteVisionary

# Test prompts
TEST_PROMPTS = [
    # Should be rule-based
    "Drag me over hot coals",
    "Dark underwater pressure",
    "Crystal shimmer cascade",
    "Vintage warm analog",
    "Psychedelic swirling colors",

    # Should be knowledge-based
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
    """Test routing decisions"""
    print("=" * 80)
    print("ROUTING DECISION TEST")
    print("=" * 80)

    visionary = CompleteVisionary()

    for prompt in TEST_PROMPTS:
        # Analyze context to get routing decision
        context = visionary.analyze_prompt_context(prompt)
        routing = context.get('routing_strategy', 'unknown')

        # Color code output
        if routing == 'rule_based':
            emoji = "🎯"
            label = "RULE-BASED"
        elif routing == 'knowledge_based':
            emoji = "🧠"
            label = "KNOWLEDGE-BASED"
        else:
            emoji = "❓"
            label = "UNKNOWN"

        print(f"\n{emoji} {label:20} | {prompt}")

    print(f"\n{'=' * 80}")


if __name__ == "__main__":
    main()
