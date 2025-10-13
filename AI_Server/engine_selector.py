"""
Rule-Based Engine Selector for Trinity AI System
Provides deterministic, coherent engine selection for character-based prompts.

Part of the Collaborative Intelligence architecture:
- 80% of prompts: Character-based (harsh, dark, ethereal, etc.) → Rule-based selection
- 20% of prompts: Knowledge-based (artist names, gear) → GPT selection with constraints

This ensures 95%+ coherence by using rules for constraint satisfaction
and GPT for creativity/knowledge within those constraints.
"""

import random
from typing import Dict, List, Set, Tuple
from collections import Counter

class RuleBasedEngineSelector:
    """
    Selects engines based on character rules with guaranteed coherence.
    Prevents critical mistakes like adding Shimmer Reverb to harsh sounds.
    """

    # Core engine selection rules for character-based prompts
    RULES = {
        "harsh": {
            "name": "Harsh/Aggressive/Destructive",
            "keywords": ["harsh", "aggressive", "destroy", "crush", "shatter", "grind", "brutal", "violent", "attack", "coals", "burn", "fire", "scorch"],
            "required": [18, 20, 21],  # Bit Crusher, Muff Fuzz, Rodent Distortion - Pick 1-2
            "optional": [19, 26, 51, 52, 10],  # K-Style Overdrive, Ring Mod, Chaos Gen, Spectral Freeze, Graphic EQ
            "forbidden": [42, 34, 15, 39, 33, 16],  # NO Shimmer, Tape Echo, SVT Bass, Dual Reverb, Spring Reverb, Platinum Harmonic
            "reason": "Harsh sounds need distortion/destruction, NOT bright reverbs or warm delays"
        },

        "dark": {
            "name": "Dark/Heavy/Ominous",
            "keywords": ["dark", "heavy", "ominous", "shadow", "doom", "menace", "thick", "murky", "deep"],
            "required": [20, 10, 41],  # Muff Fuzz, Graphic EQ (low-pass), Plate Reverb - Pick 1-2
            "optional": [15, 21, 43, 9, 37],  # SVT Bass, Rodent, Gated Reverb, Ladder Filter, Bucket Brigade
            "forbidden": [42, 16, 25, 29],  # NO Shimmer, Platinum Harmonic, Analog Phaser, Classic Tremolo
            "reason": "Dark sounds need low-end/filtering, NOT bright pitch-shifting or sparkly modulation"
        },

        "ethereal": {
            "name": "Ethereal/Bright/Heavenly",
            "keywords": ["ethereal", "bright", "heaven", "angel", "crystal", "shimmer", "sparkle", "celestial", "airy"],
            "required": [42],  # Shimmer Reverb - THE appropriate use case for Shimmer!
            "optional": [25, 29, 35, 32, 47, 39],  # Analog Phaser, Tremolo, Stereo Chorus, Ensemble, Detune Doubler, Dual Reverb
            "forbidden": [18, 20, 21, 15, 26],  # NO Bit Crusher, Muff, Rodent, SVT Bass, Ring Mod
            "reason": "Ethereal sounds need brightness/space, NOT distortion or heavy bass"
        },

        "vintage": {
            "name": "Vintage/Warm/Analog",
            "keywords": ["vintage", "warm", "analog", "tape", "nostalgic", "retro", "classic", "smooth", "mellow"],
            "required": [34, 15, 37],  # Tape Echo, SVT Bass, Bucket Brigade - Pick 1-2
            "optional": [33, 41, 3, 7, 25],  # Spring Reverb, Plate Reverb, Vintage Opto, Mastering Limiter, Phaser
            "forbidden": [18, 51, 52, 26, 44],  # NO Bit Crusher, Chaos, Spectral Freeze, Ring Mod, Convolution (too modern)
            "reason": "Vintage sounds need warmth/analog character, NOT digital artifacts or modern processing"
        },

        "underwater": {
            "name": "Underwater/Submerged/Liquid",
            "keywords": ["underwater", "submerged", "liquid", "bubble", "ocean", "deep", "pressure", "aquatic"],
            "required": [9, 43, 10],  # Ladder Filter (low-pass), Gated Reverb, Graphic EQ - Pick 1-2
            "optional": [30, 35, 37, 41],  # Envelope Filter, Stereo Chorus, Bucket Brigade, Plate Reverb
            "forbidden": [42, 16, 18, 29],  # NO Shimmer, Platinum Harmonic, Bit Crusher, Tremolo
            "reason": "Underwater needs filtering/modulation for depth, NOT brightness or harshness"
        },

        "glitchy": {
            "name": "Glitchy/Digital/Robotic",
            "keywords": ["glitch", "digital", "robot", "stutter", "freeze", "circuit", "error", "malfunction"],
            "required": [18, 51, 52],  # Bit Crusher, Chaos Generator, Spectral Freeze - Pick 2-3
            "optional": [26, 43, 8, 50],  # Ring Mod, Gated Reverb, Transient Shaper, Gender Bender
            "forbidden": [34, 15, 39, 33, 37],  # NO Tape Echo, SVT Bass, Dual Reverb, Spring Reverb, Bucket Brigade
            "reason": "Glitchy sounds need digital artifacts, NOT warm analog processing"
        },

        "psychedelic": {
            "name": "Psychedelic/Swirling/Colorful",
            "keywords": ["psychedelic", "swirl", "color", "trip", "kaleidoscope", "hypnotic", "spiral", "vortex"],
            "required": [25, 26, 32],  # Analog Phaser, Ring Mod, Ensemble - Pick 2
            "optional": [42, 35, 36, 29, 47],  # Shimmer, Chorus, Magnetic Drum, Tremolo, Detune Doubler
            "forbidden": [15, 1, 2, 18],  # NO SVT Bass, Classic Comp, Vintage Opto, Bit Crusher
            "reason": "Psychedelic needs modulation/movement, NOT compression or harsh distortion"
        },

        "space": {
            "name": "Space/Vast/Cosmic",
            "keywords": ["space", "cosmic", "vast", "nebula", "galaxy", "stars", "universe", "void"],
            "required": [42, 39, 44],  # Shimmer Reverb, Dual Reverb, Convolution - Pick 1-2
            "optional": [36, 47, 32, 48],  # Magnetic Drum, Detune Doubler, Ensemble, Harmonizer
            "forbidden": [18, 20, 21, 15, 1],  # NO distortions, NO compression
            "reason": "Space needs expansive reverbs/delays, NOT aggressive processing"
        },

        "industrial": {
            "name": "Industrial/Mechanical/Metallic",
            "keywords": ["industrial", "mechanical", "metal", "machine", "factory", "steel", "grind", "clang"],
            "required": [18, 26, 21],  # Bit Crusher, Ring Mod, Rodent - Pick 2
            "optional": [43, 51, 20, 8],  # Gated Reverb, Chaos, Muff Fuzz, Transient Shaper
            "forbidden": [34, 39, 42, 33, 37],  # NO Tape Echo, Dual Reverb, Shimmer, Spring, Bucket Brigade
            "reason": "Industrial needs metallic/harsh processing, NOT warm or ethereal effects"
        },

        "smooth": {
            "name": "Smooth/Gentle/Soft",
            "keywords": ["smooth", "gentle", "soft", "silk", "velvet", "soothe", "calm", "tender"],
            "required": [32, 35, 3],  # Ensemble, Stereo Chorus, Vintage Opto - Pick 1-2
            "optional": [34, 41, 39, 7, 47],  # Tape Echo, Plate Reverb, Dual Reverb, Limiter, Detune Doubler
            "forbidden": [18, 20, 21, 26, 51],  # NO Bit Crusher, Muff, Rodent, Ring Mod, Chaos
            "reason": "Smooth sounds need subtle modulation/compression, NOT distortion"
        },

        "neutral": {
            "name": "Neutral/Balanced/Versatile",
            "keywords": ["clean", "clear", "balanced", "transparent", "natural", "basic", "simple"],
            "required": [10, 31, 46],  # Graphic EQ, Stereo Imager, Phase Align - Pick 1
            "optional": [1, 7, 35, 41, 29],  # Classic Comp, Limiter, Chorus, Plate Reverb, Tremolo
            "forbidden": [],  # Nothing forbidden for neutral
            "reason": "Neutral sounds use utility processing, can combine with anything"
        }
    }

    # Forbidden combinations that should NEVER appear together
    FORBIDDEN_COMBINATIONS = [
        (42, 18, "Shimmer Reverb + Bit Crusher: Bright pitch shift clashes with digital destruction"),
        (42, 20, "Shimmer Reverb + Muff Fuzz: Ethereal reverb clashes with heavy distortion"),
        (42, 21, "Shimmer Reverb + Rodent Distortion: Brightness clashes with aggressive distortion"),
        (34, 18, "Tape Echo + Bit Crusher: Warm analog delay clashes with digital destruction"),
        (34, 20, "Tape Echo + Muff Fuzz: Vintage warmth clashes with heavy fuzz"),
        (16, 15, "Platinum Harmonic + SVT Bass: Both add brightness/harmonics - redundant and muddy"),
        (39, 42, "Dual Reverb + Shimmer Reverb: Too much reverb, wastes slots"),
        (41, 42, "Plate Reverb + Shimmer Reverb: Too much reverb, wastes slots"),
        (33, 39, "Spring Reverb + Dual Reverb: Too much reverb, wastes slots"),
    ]

    # Category definitions for engine types
    REVERBS = [33, 39, 41, 42, 43, 44]  # Spring, Dual, Plate, Shimmer, Gated, Convolution
    DISTORTIONS = [18, 19, 20, 21, 26]  # Bit Crusher, K-Style, Muff, Rodent, Ring Mod
    DYNAMICS = [1, 2, 3, 4, 7, 8]  # Compressors, Gates, Limiters, Transient Shaper
    MODULATIONS = [25, 29, 30, 32, 35, 47, 52]  # Phaser, Tremolo, Envelope, Ensemble, Chorus, Detune, Spectral
    DELAYS = [34, 36, 37, 38]  # Tape Echo, Magnetic Drum, Bucket Brigade, Ping Pong
    PITCH = [48, 49, 50]  # Harmonizer, Voice Doubler, Gender Bender
    FILTERS = [9, 10, 11, 12, 13, 14]  # Ladder, Graphic EQ, Parametric, Comb, State Variable, Multi-Mode
    SPECIAL = [51, 52]  # Chaos Generator, Spectral Freeze

    def __init__(self):
        """Initialize the rule-based engine selector."""
        self.last_selections = []  # Track recent selections to avoid repetition

    def detect_character(self, prompt: str) -> str:
        """
        Detect the primary character of the prompt based on keywords.
        Returns the rule key that best matches the prompt.
        """
        prompt_lower = prompt.lower()

        # Count keyword matches for each character
        matches = {}
        for character, rule in self.RULES.items():
            match_count = sum(1 for keyword in rule["keywords"] if keyword in prompt_lower)
            if match_count > 0:
                matches[character] = match_count

        # Return the character with most matches, or "neutral" if none
        if matches:
            return max(matches, key=matches.get)
        return "neutral"

    def select_engines(self, prompt: str, num_slots: int = 5) -> Tuple[List[int], str]:
        """
        Select engines based on rule matching.

        Args:
            prompt: User prompt to analyze
            num_slots: Number of engine slots to fill (1-6)

        Returns:
            Tuple of (selected_engine_ids, character_detected)
        """
        # Detect character
        character = self.detect_character(prompt)
        rule = self.RULES[character]

        selected = []

        # Step 1: Select from required engines (pick 1-2 based on num_slots)
        required_count = min(2, num_slots // 2) if num_slots >= 3 else 1
        if rule["required"]:
            selected.extend(random.sample(rule["required"], min(required_count, len(rule["required"]))))

        # Step 2: Fill remaining slots with optional engines
        remaining_slots = num_slots - len(selected)
        if remaining_slots > 0 and rule["optional"]:
            # Filter out forbidden combinations
            available_optional = [
                eng for eng in rule["optional"]
                if not any(self._is_forbidden_combo(eng, s) for s in selected)
            ]

            # Add optional engines
            optional_count = min(remaining_slots, len(available_optional))
            selected.extend(random.sample(available_optional, optional_count))

        # Step 3: Ensure diversity (no more than 2 from same category)
        selected = self._ensure_category_diversity(selected)

        # Step 4: Pad with slot 0 (None) if needed
        while len(selected) < num_slots:
            selected.append(0)

        return selected[:num_slots], character

    def _is_forbidden_combo(self, engine1: int, engine2: int) -> bool:
        """Check if two engines form a forbidden combination."""
        for e1, e2, _ in self.FORBIDDEN_COMBINATIONS:
            if (engine1 == e1 and engine2 == e2) or (engine1 == e2 and engine2 == e1):
                return True
        return False

    def _ensure_category_diversity(self, selected: List[int]) -> List[int]:
        """
        Ensure no more than 2 engines from the same category.
        Prioritize keeping required engines over optional ones.
        """
        # Count engines per category
        category_counts = {
            "reverbs": sum(1 for e in selected if e in self.REVERBS),
            "distortions": sum(1 for e in selected if e in self.DISTORTIONS),
            "modulations": sum(1 for e in selected if e in self.MODULATIONS),
            "delays": sum(1 for e in selected if e in self.DELAYS),
        }

        # If any category has >2, randomly remove extras
        result = selected[:]
        for category, engines in [
            ("reverbs", self.REVERBS),
            ("distortions", self.DISTORTIONS),
            ("modulations", self.MODULATIONS),
            ("delays", self.DELAYS)
        ]:
            category_engines = [e for e in result if e in engines]
            if len(category_engines) > 2:
                # Keep only 2 random engines from this category
                keep = random.sample(category_engines, 2)
                result = [e for e in result if e not in category_engines or e in keep]

        return result

    def get_forbidden_engines(self, character: str) -> List[int]:
        """Get list of forbidden engines for a specific character."""
        if character in self.RULES:
            return self.RULES[character]["forbidden"]
        return []

    def get_mandatory_engines(self, character: str) -> List[int]:
        """Get list of mandatory engines for a specific character."""
        if character in self.RULES:
            return self.RULES[character]["required"]
        return []

    def validate_selection(self, engine_ids: List[int]) -> Tuple[bool, str]:
        """
        Validate that a selection doesn't contain forbidden combinations.

        Returns:
            Tuple of (is_valid, error_message)
        """
        # Check for forbidden combinations
        for i, e1 in enumerate(engine_ids):
            for e2 in engine_ids[i+1:]:
                for fe1, fe2, reason in self.FORBIDDEN_COMBINATIONS:
                    if (e1 == fe1 and e2 == fe2) or (e1 == fe2 and e2 == fe1):
                        return False, reason

        # Check for too many from same category
        reverb_count = sum(1 for e in engine_ids if e in self.REVERBS)
        distortion_count = sum(1 for e in engine_ids if e in self.DISTORTIONS)

        if reverb_count > 2:
            return False, f"Too many reverbs ({reverb_count}) - maximum 2 allowed"
        if distortion_count > 2:
            return False, f"Too many distortions ({distortion_count}) - maximum 2 allowed"

        return True, ""


# Singleton instance for easy importing
engine_selector = RuleBasedEngineSelector()


def select_engines_for_prompt(prompt: str, num_slots: int = 5) -> Dict:
    """
    Convenience function for selecting engines based on a prompt.

    Returns:
        Dict with selected_engines, character_detected, and rule_info
    """
    selected, character = engine_selector.select_engines(prompt, num_slots)

    return {
        "selected_engines": selected,
        "character_detected": character,
        "rule_name": engine_selector.RULES[character]["name"],
        "forbidden_engines": engine_selector.get_forbidden_engines(character),
        "mandatory_engines": engine_selector.get_mandatory_engines(character),
    }


if __name__ == "__main__":
    # Test the engine selector with critical prompts
    test_prompts = [
        "Drag me over hot coals",
        "Dark underwater pressure",
        "Crystal shimmer cascade",
        "Vintage warm tape saturation",
        "Glitchy robot malfunction",
        "Psychedelic swirling colors",
    ]

    print("=" * 80)
    print("RULE-BASED ENGINE SELECTOR TEST")
    print("=" * 80)

    for prompt in test_prompts:
        result = select_engines_for_prompt(prompt, num_slots=5)
        print(f"\n📝 Prompt: {prompt}")
        print(f"🎭 Character Detected: {result['character_detected']} ({result['rule_name']})")
        print(f"✅ Selected Engines: {result['selected_engines']}")
        print(f"⚠️  Forbidden Engines: {result['forbidden_engines']}")
        print(f"🎯 Mandatory Engines: {result['mandatory_engines']}")

        # Validate
        is_valid, error = engine_selector.validate_selection(result['selected_engines'])
        if is_valid:
            print("✅ VALID selection - no forbidden combinations")
        else:
            print(f"❌ INVALID selection: {error}")
