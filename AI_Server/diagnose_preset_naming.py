#!/usr/bin/env python3
"""
Diagnose Preset Naming System
Check what names the Visionary generates and identify patterns
"""

import asyncio
import json
import logging
from visionary_complete import CompleteVisionary

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("NamingDiagnosis")

# Test prompts covering different styles
TEST_PROMPTS = [
    "warm vintage tape delay",
    "aggressive fuzz distortion",
    "smooth vocal reverb",
    "lo-fi bit crusher",
    "clean stereo delay",
    "heavy metal crunch",
    "ambient atmospheric pad",
    "funky wah filter",
    "dreamy shimmer",
    "thunderous bass",
]

async def test_naming():
    """Test preset naming across different prompts"""

    logger.info("=" * 60)
    logger.info("PRESET NAMING DIAGNOSIS")
    logger.info("=" * 60)

    visionary = CompleteVisionary()

    names = []

    for i, prompt in enumerate(TEST_PROMPTS, 1):
        logger.info(f"\n[{i}/{len(TEST_PROMPTS)}] Testing: '{prompt}'")

        try:
            preset = await visionary.generate_complete_preset(prompt)

            if preset and not preset.get("error"):
                name = preset.get("name", "UNNAMED")
                names.append({
                    "prompt": prompt,
                    "name": name,
                    "description": preset.get("description", "")
                })
                logger.info(f"  → Generated: '{name}'")
            else:
                logger.warning(f"  → Failed to generate")
                names.append({
                    "prompt": prompt,
                    "name": "FAILED",
                    "description": ""
                })

        except Exception as e:
            logger.error(f"  → Exception: {e}")
            names.append({
                "prompt": prompt,
                "name": "ERROR",
                "description": str(e)
            })

        # Small delay between requests
        await asyncio.sleep(1)

    # Analysis
    logger.info("\n" + "=" * 60)
    logger.info("NAMING ANALYSIS")
    logger.info("=" * 60)

    logger.info(f"\nTotal prompts tested: {len(TEST_PROMPTS)}")
    successful = [n for n in names if n["name"] not in ["FAILED", "ERROR", "UNNAMED"]]
    logger.info(f"Successful generations: {len(successful)}")

    if successful:
        logger.info("\nGenerated Names:")
        for item in successful:
            logger.info(f"  '{item['prompt']}' → '{item['name']}'")

        # Pattern analysis
        logger.info("\nPattern Analysis:")

        # Word count
        word_counts = {}
        for item in successful:
            word_count = len(item["name"].split())
            word_counts[word_count] = word_counts.get(word_count, 0) + 1

        logger.info(f"  Word count distribution: {word_counts}")

        # Common words
        all_words = []
        for item in successful:
            words = item["name"].lower().split()
            all_words.extend(words)

        from collections import Counter
        common_words = Counter(all_words).most_common(5)
        logger.info(f"  Most common words: {common_words}")

        # Check for repetitive patterns
        if len(successful) >= 3:
            # Check if names sound similar
            logger.info("\n  Similarity check:")
            for i in range(len(successful) - 1):
                name1 = successful[i]["name"].lower()
                name2 = successful[i+1]["name"].lower()

                # Simple similarity: shared words
                words1 = set(name1.split())
                words2 = set(name2.split())
                shared = words1 & words2
                if shared:
                    logger.info(f"    '{successful[i]['name']}' and '{successful[i+1]['name']}' share: {shared}")

    # Save results
    with open("naming_diagnosis_results.json", "w") as f:
        json.dump({
            "test_date": "2025-10-17",
            "results": names,
            "analysis": {
                "total": len(TEST_PROMPTS),
                "successful": len(successful),
                "failed": len([n for n in names if n["name"] == "FAILED"]),
                "errors": len([n for n in names if n["name"] == "ERROR"])
            }
        }, f, indent=2)

    logger.info("\n✅ Results saved to naming_diagnosis_results.json")


async def check_naming_prompt():
    """Show the current naming instructions in the prompt"""

    logger.info("\n" + "=" * 60)
    logger.info("CURRENT NAMING INSTRUCTIONS")
    logger.info("=" * 60)

    visionary = CompleteVisionary()

    # Extract naming instruction from system prompt
    system_prompt = visionary.create_system_prompt()

    # Find the naming section
    if "IMPORTANT: Create a unique" in system_prompt:
        start = system_prompt.find("IMPORTANT: Create a unique")
        end = system_prompt.find("Return JSON", start)
        naming_section = system_prompt[start:end].strip()

        logger.info("\n" + naming_section)
    else:
        logger.warning("Could not find naming instructions in system prompt")

    # Show example names from prompt
    logger.info("\nExample names provided to AI:")
    if "Examples:" in system_prompt:
        start = system_prompt.find("Examples:") + 9
        end = system_prompt.find("\n", start)
        examples = system_prompt[start:end].strip()
        logger.info(f"  {examples}")


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1 and sys.argv[1] == "check":
        # Just show current naming instructions
        asyncio.run(check_naming_prompt())
    else:
        # Full naming test
        asyncio.run(test_naming())
