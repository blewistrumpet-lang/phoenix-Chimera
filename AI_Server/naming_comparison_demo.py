#!/usr/bin/env python3
"""
Demonstrate the naming problem and solution
"""

from preset_namer import IntelligentPresetNamer

def show_current_problem():
    """Show the current naming patterns"""
    print("=" * 70)
    print("CURRENT NAMING PROBLEM")
    print("=" * 70)

    print("\n1. When GPT-4 Works (follows examples too closely):")
    print("-" * 50)

    gpt_examples = [
        "Velvet Thunder",
        "Crystal Cascade",
        "Midnight Resonance",
        "Obsidian Chamber",
        "Golden Whisper",
        "Sapphire Dreams",
        "Crimson Echo"
    ]

    for name in gpt_examples:
        print(f"  • {name:25} [Adjective + Noun pattern]")

    print("\n2. When GPT-4 Fails (fallback pattern):")
    print("-" * 50)

    fallback_examples = [
        ("warm tape", "Subtle Audio Preset"),
        ("aggressive fuzz", "Aggressive Audio Preset"),
        ("test", "Moderate Audio Preset"),
        ("smooth reverb", "Subtle Audio Preset"),
        ("heavy metal", "Aggressive Audio Preset"),
    ]

    for prompt, name in fallback_examples:
        print(f"  '{prompt:20}' → {name:25} [Intensity + Audio + Preset]")

    print("\n❌ Problem: Names are repetitive and predictable!")


def show_intelligent_solution():
    """Show the intelligent naming solution"""
    print("\n" + "=" * 70)
    print("INTELLIGENT NAMING SOLUTION")
    print("=" * 70)

    namer = IntelligentPresetNamer()

    print("\nVaried naming based on prompt context:")
    print("-" * 50)

    test_cases = [
        # (prompt, expected_style)
        ("warm vintage tape delay", "vintage"),
        ("aggressive metal distortion", "aggressive"),
        ("test", "technical"),
        ("300ms ping pong delay", "technical"),
        ("ambient space pad", "ethereal"),
        ("smooth jazz reverb", "smooth"),
        ("bit crusher", "creative"),
        ("heavy bass boost", "aggressive"),
        ("clean digital delay", "technical"),
        ("ethereal shimmer", "ethereal"),
    ]

    for prompt, style in test_cases:
        name = namer.generate_name(prompt, [], {})
        print(f"  '{prompt:30}' → {name:20} [{style}]")

    print("\n✅ Solution: Context-aware naming with multiple strategies!")


def show_pattern_variety():
    """Show the variety of patterns available"""
    print("\n" + "=" * 70)
    print("AVAILABLE NAMING PATTERNS")
    print("=" * 70)

    patterns = [
        ("Technical", ["Preset 42", "Unit 731", "Channel 9", "Program A7"]),
        ("Vintage", ["1973", "Analog Dreams", "Studio B", "Old School Echo"]),
        ("Aggressive", ["Brutal Force", "Death Machine", "Maximum Damage", "The Savage"]),
        ("Ethereal", ["Celestial Dreams", "Star Field", "Aurora Prime", "Floating"]),
        ("Creative", ["Breaking Glass", "Silent Storm", "Neo-Verb", "What If"]),
        ("Single Word", ["Monolith", "Zenith", "Apex", "Prime"]),
    ]

    for category, examples in patterns:
        print(f"\n{category}:")
        for ex in examples:
            print(f"  • {ex}")


def generate_batch_comparison():
    """Generate a batch of names for comparison"""
    print("\n" + "=" * 70)
    print("BATCH GENERATION COMPARISON")
    print("=" * 70)

    namer = IntelligentPresetNamer()

    prompt = "vintage reverb"

    print(f"\nGenerating 20 names for: '{prompt}'")
    print("-" * 50)

    print("\nOld System (would generate):")
    old_names = [
        "Vintage Reverb Preset",  # Fallback
        "Golden Echo",            # GPT pattern
        "Velvet Chamber",         # GPT pattern
        "Crystal Hall",           # GPT pattern
        "Vintage Audio Preset",   # Fallback
    ]

    for i, name in enumerate(old_names, 1):
        print(f"  {i:2}. {name}")
    print("  ... (all similar patterns)")

    print("\nIntelligent System generates:")
    new_names = []
    for i in range(20):
        name = namer.generate_name(prompt, [{"engine_name": "Reverb"}], {"character": "vintage"})
        new_names.append(name)

    # Show first 10
    for i, name in enumerate(new_names[:10], 1):
        print(f"  {i:2}. {name}")

    # Analysis
    unique_count = len(set(new_names))
    print(f"\nUniqueness: {unique_count}/20 unique names")

    # Pattern variety
    patterns_found = set()
    for name in new_names:
        if any(c.isdigit() for c in name):
            patterns_found.add("numeric")
        if len(name.split()) == 1:
            patterns_found.add("single-word")
        elif len(name.split()) == 2:
            patterns_found.add("two-word")
        else:
            patterns_found.add("multi-word")
        if "-" in name:
            patterns_found.add("compound")

    print(f"Pattern variety: {patterns_found}")


if __name__ == "__main__":
    show_current_problem()
    show_intelligent_solution()
    show_pattern_variety()
    generate_batch_comparison()

    print("\n" + "=" * 70)
    print("SUMMARY")
    print("=" * 70)
    print("""
The Intelligent Preset Namer solves the repetitive naming problem by:

1. Analyzing prompt context (technical, artistic, genre, etc.)
2. Selecting appropriate naming strategy
3. Using varied vocabulary and patterns
4. Tracking recent names to avoid duplicates
5. Providing fallback variations when needed

This results in names that:
- Match the preset's actual character
- Show much more variety
- Feel more creative and engaging
- Avoid the "Adjective + Noun" trap
""")