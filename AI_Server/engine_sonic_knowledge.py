"""
Comprehensive Sonic Knowledge Database for Trinity AI
Defines sonic character, compatibility, and usage rules for all engines
"""

# Sonic signature values: 0.0 = absent/negative, 1.0 = maximum/positive
SONIC_SIGNATURES = {
    # REVERBS (39-43)
    39: {  # Plate Reverb
        "name": "Plate Reverb",
        "signature": {"brightness": 0.5, "warmth": 0.6, "cleanliness": 0.8, "aggression": 0.1, "space": 0.7},
        "character": "smooth, vintage, jazz-appropriate reverb",
        "emotional_tags": ["smooth", "vintage", "jazz", "warm", "mellow", "professional"],
        "use_for": ["jazz", "vocals", "smooth", "vintage", "warm"],
        "avoid_for": ["harsh", "destroyed", "experimental", "glitchy"],
        "frequency_emphasis": "mid",
        "typical_prompts": ["jazz", "smooth", "vintage", "professional", "studio"],
    },
    40: {  # Spring Reverb
        "name": "Spring Reverb",
        "signature": {"brightness": 0.6, "warmth": 0.7, "cleanliness": 0.6, "aggression": 0.3, "space": 0.5},
        "character": "metallic, surf rock, vintage guitar reverb",
        "emotional_tags": ["surf", "vintage", "guitar", "metallic", "retro", "twangy"],
        "use_for": ["surf", "vintage guitar", "retro", "spring", "metallic"],
        "avoid_for": ["modern", "pristine", "ethereal"],
        "frequency_emphasis": "mid-high",
        "typical_prompts": ["surf", "spring", "vintage", "guitar", "metallic"],
    },
    41: {  # Convolution Reverb
        "name": "Convolution Reverb",
        "signature": {"brightness": 0.4, "warmth": 0.5, "cleanliness": 0.7, "aggression": 0.2, "space": 0.9},
        "character": "realistic spaces, deep, can be dark or bright depending on IR",
        "emotional_tags": ["realistic", "deep", "cathedral", "cave", "underwater", "spacious"],
        "use_for": ["dark", "underwater", "cathedral", "realistic space", "deep", "cave"],
        "avoid_for": ["bright psychedelic", "sparkly", "synthetic"],
        "frequency_emphasis": "full",
        "typical_prompts": ["cathedral", "underwater", "cave", "realistic", "deep", "dark"],
    },
    42: {  # Shimmer Reverb - THE PROBLEM CHILD
        "name": "Shimmer Reverb",
        "signature": {"brightness": 0.95, "warmth": 0.3, "cleanliness": 0.9, "aggression": 0.0, "space": 0.9},
        "character": "BRIGHT ethereal reverb with UPWARD PITCH SHIFT - adds sparkle and heaven-like quality",
        "emotional_tags": ["ethereal", "heavenly", "bright", "sparkle", "crystal", "angelic", "psychedelic"],
        "use_for": ["ethereal", "bright", "crystal", "shimmer", "heaven", "sparkle", "psychedelic colors"],
        "avoid_for": ["harsh", "dark", "underwater", "industrial", "glitchy", "robot", "burning", "crushing", "aggressive"],
        "frequency_emphasis": "high",
        "typical_prompts": ["crystal", "shimmer", "ethereal", "heaven", "bright", "sparkle", "psychedelic"],
        "CRITICAL_AVOID": True,  # Flag for special handling
        "avoid_reason": "Bright upward pitch shift CLASHES with harsh/dark/aggressive sounds",
    },
    43: {  # Gated Reverb
        "name": "Gated Reverb",
        "signature": {"brightness": 0.6, "warmth": 0.4, "cleanliness": 0.7, "aggression": 0.6, "space": 0.3},
        "character": "short, punchy, 80s drum reverb",
        "emotional_tags": ["punchy", "tight", "80s", "drums", "gated", "percussive"],
        "use_for": ["tight", "punchy", "drums", "80s", "gated", "percussive"],
        "avoid_for": ["ambient", "ethereal", "long decay", "spacious"],
        "frequency_emphasis": "mid",
        "typical_prompts": ["80s", "gated", "punchy", "tight", "drums"],
    },

    # DELAYS/ECHOES (34-38)
    34: {  # Tape Echo - THE OTHER PROBLEM CHILD
        "name": "Tape Echo",
        "signature": {"brightness": 0.4, "warmth": 0.9, "cleanliness": 0.6, "aggression": 0.2, "space": 0.6},
        "character": "WARM, VINTAGE, ANALOG delay with tape saturation - adds nostalgia",
        "emotional_tags": ["warm", "vintage", "analog", "nostalgic", "tape", "retro", "gentle"],
        "use_for": ["warm", "vintage", "gentle", "nostalgic", "tape", "smooth", "mellow"],
        "avoid_for": ["harsh", "destroy", "crush", "industrial", "digital", "aggressive", "burning"],
        "frequency_emphasis": "mid",
        "typical_prompts": ["vintage", "warm", "tape", "nostalgic", "gentle", "smooth"],
        "CRITICAL_AVOID": True,  # Flag for special handling
        "avoid_reason": "Warmth and vintage character CONTRADICTS destruction/harshness",
    },

    # DISTORTION (17-22, 30-31)
    18: {  # Bit Crusher
        "name": "Bit Crusher",
        "signature": {"brightness": 0.8, "warmth": 0.1, "cleanliness": 0.0, "aggression": 0.95, "space": 0.1},
        "character": "DIGITAL DESTRUCTION - reduces bit depth and sample rate for harsh lo-fi",
        "emotional_tags": ["harsh", "digital", "destroyed", "lo-fi", "aggressive", "glitchy", "crushed"],
        "use_for": ["harsh", "destroy", "crush", "glitchy", "digital", "aggressive", "burning"],
        "avoid_for": ["vintage", "warm", "smooth", "jazz", "gentle", "ethereal"],
        "frequency_emphasis": "high",
        "typical_prompts": ["crush", "destroy", "harsh", "digital", "glitchy", "aggressive"],
    },
    20: {  # Muff Fuzz
        "name": "Muff Fuzz",
        "signature": {"brightness": 0.6, "warmth": 0.4, "cleanliness": 0.1, "aggression": 0.9, "space": 0.2},
        "character": "HEAVY FUZZ DISTORTION - thick, sustained, aggressive",
        "emotional_tags": ["heavy", "fuzz", "aggressive", "thick", "sustained", "crushing"],
        "use_for": ["harsh", "aggressive", "heavy", "fuzz", "crushing", "burning"],
        "avoid_for": ["gentle", "clean", "pristine", "jazz", "smooth"],
        "frequency_emphasis": "mid",
        "typical_prompts": ["fuzz", "heavy", "aggressive", "crushing", "burning"],
    },
    21: {  # Rodent Distortion
        "name": "Rodent Distortion",
        "signature": {"brightness": 0.7, "warmth": 0.3, "cleanliness": 0.2, "aggression": 0.85, "aggression": 0.2},
        "character": "MID-FOCUSED AGGRESSIVE DISTORTION - cutting, harsh",
        "emotional_tags": ["aggressive", "harsh", "cutting", "mid-heavy", "distorted"],
        "use_for": ["harsh", "aggressive", "cutting", "distorted", "burning"],
        "avoid_for": ["gentle", "warm", "smooth", "clean"],
        "frequency_emphasis": "mid",
        "typical_prompts": ["harsh", "aggressive", "distorted", "cutting"],
    },

    # MODULATION (23-29)
    26: {  # Ring Modulator
        "name": "Ring Modulator",
        "signature": {"brightness": 0.7, "warmth": 0.2, "cleanliness": 0.3, "aggression": 0.8, "space": 0.3},
        "character": "INHARMONIC METALLIC MODULATION - creates dissonant bell-like tones",
        "emotional_tags": ["metallic", "robotic", "dissonant", "inharmonic", "weird", "alien"],
        "use_for": ["robot", "metallic", "weird", "experimental", "glitchy", "alien", "harsh"],
        "avoid_for": ["warm", "gentle", "smooth", "musical", "harmonic"],
        "frequency_emphasis": "full",
        "typical_prompts": ["robot", "metallic", "alien", "weird", "glitchy"],
    },

    # PITCH/VOCAL (45-51)
    49: {  # Phased Vocoder
        "name": "Phased Vocoder",
        "signature": {"brightness": 0.6, "warmth": 0.3, "cleanliness": 0.5, "aggression": 0.4, "space": 0.4},
        "character": "ROBOTIC VOICE EFFECT - creates mechanical, synthetic vocal quality",
        "emotional_tags": ["robotic", "vocoded", "synthetic", "mechanical", "voice", "digital"],
        "use_for": ["robot", "vocoded", "synthetic", "mechanical", "voice", "glitchy"],
        "avoid_for": ["natural", "organic", "warm", "analog"],
        "frequency_emphasis": "mid",
        "typical_prompts": ["robot", "vocoded", "voice", "mechanical", "synthetic"],
    },

    # SPECIAL/EXPERIMENTAL (51-52)
    51: {  # Chaos Generator
        "name": "Chaos Generator",
        "signature": {"brightness": 0.7, "warmth": 0.2, "cleanliness": 0.1, "aggression": 0.9, "space": 0.5},
        "character": "UNPREDICTABLE CHAOS - random modulation and destruction",
        "emotional_tags": ["chaotic", "unpredictable", "destructive", "experimental", "wild", "random"],
        "use_for": ["chaos", "destroy", "experimental", "unpredictable", "wild", "harsh"],
        "avoid_for": ["controlled", "smooth", "gentle", "musical", "predictable"],
        "frequency_emphasis": "full",
        "typical_prompts": ["chaos", "destroy", "experimental", "wild", "unpredictable"],
    },

    # VINTAGE (15, 22)
    15: {  # Vintage Tube Preamp
        "name": "Vintage Tube Preamp",
        "signature": {"brightness": 0.4, "warmth": 0.95, "cleanliness": 0.6, "aggression": 0.2, "space": 0.2},
        "character": "WARM TUBE SATURATION - adds analog warmth and gentle harmonic richness",
        "emotional_tags": ["warm", "vintage", "tube", "analog", "smooth", "rich", "harmonic"],
        "use_for": ["warm", "vintage", "smooth", "analog", "tube", "gentle"],
        "avoid_for": ["harsh", "digital", "cold", "aggressive", "destroyed"],
        "frequency_emphasis": "mid",
        "typical_prompts": ["warm", "vintage", "tube", "analog", "smooth"],
    },
}

# Forbidden combinations - engines that should NEVER be used together
FORBIDDEN_COMBINATIONS = [
    # Bright/Ethereal vs Dark/Harsh
    (42, 18, "Shimmer Reverb + Bit Crusher: Bright pitch shift clashes with digital destruction"),
    (42, 20, "Shimmer Reverb + Muff Fuzz: Ethereal reverb clashes with heavy distortion"),
    (42, 21, "Shimmer Reverb + Rodent Distortion: Bright shimmer clashes with harsh distortion"),
    (42, 26, "Shimmer Reverb + Ring Modulator: Ethereal reverb clashes with metallic dissonance"),
    (42, 51, "Shimmer Reverb + Chaos Generator: Gentle reverb clashes with chaotic destruction"),

    # Warm/Vintage vs Digital/Harsh
    (34, 18, "Tape Echo + Bit Crusher: Warm analog delay clashes with digital destruction"),
    (34, 20, "Tape Echo + Muff Fuzz: Vintage delay clashes with heavy fuzz"),
    (34, 21, "Tape Echo + Rodent Distortion: Warm echo clashes with harsh distortion"),
    (15, 18, "Vintage Tube + Bit Crusher: Analog warmth clashes with digital destruction"),
    (15, 31, "Vintage Tube + Bit Crusher: Warm tube clashes with digital crushing"),
]

# Prompt keyword → forbidden engines mapping
PROMPT_BASED_FORBIDDEN = {
    "harsh": [42, 34, 15, 22, 39],  # No shimmer, tape echo, vintage tube, plate
    "dark": [42, 17],  # No shimmer reverb, harmonic exciter (both brighten)
    "underwater": [42, 17],  # No bright effects
    "destroy": [42, 34, 15, 22, 39],  # No gentle/warm effects
    "crush": [42, 34, 15, 22],
    "burning": [42, 34, 39],
    "industrial": [42, 34, 15],
    "glitchy": [42, 34, 15],  # No organic/warm effects
    "robot": [42, 34, 15, 22],  # No organic/warm effects
    "aggressive": [42, 34, 39],
    "gentle": [18, 20, 21, 30, 31, 51],  # No harsh distortion
    "smooth": [18, 20, 21, 30, 31, 51],
    "jazz": [18, 20, 21, 30, 31],
    "ethereal": [18, 20, 21, 30, 31],  # OK to use shimmer
    "crystal": [18, 20, 21, 30],  # OK to use shimmer
    "shimmer": [],  # Explicitly allows shimmer reverb
    "bright": [41],  # Don't use convolution (too dark), shimmer is OK
}


def get_forbidden_engines_for_prompt(prompt: str) -> list:
    """
    Given a user prompt, return list of engines that should be FORBIDDEN.

    Returns:
        List of (engine_id, reason) tuples
    """
    forbidden = []
    prompt_lower = prompt.lower()

    for keyword, engine_ids in PROMPT_BASED_FORBIDDEN.items():
        if keyword in prompt_lower:
            for eid in engine_ids:
                if eid in SONIC_SIGNATURES:
                    engine_name = SONIC_SIGNATURES[eid]["name"]
                    reason = f"Prompt contains '{keyword}' which is incompatible with {engine_name}"
                    forbidden.append((eid, reason))

    return forbidden


def validate_engine_combination(engine_ids: list, prompt: str) -> tuple:
    """
    Validate that chosen engines don't have forbidden combinations.

    Returns:
        (is_valid, list_of_errors)
    """
    errors = []

    # Check forbidden combinations between engines
    for e1, e2, reason in FORBIDDEN_COMBINATIONS:
        if e1 in engine_ids and e2 in engine_ids:
            errors.append({
                "severity": "critical",
                "type": "forbidden_combination",
                "engines": [e1, e2],
                "reason": reason
            })

    # Check prompt-based forbidden engines
    forbidden = get_forbidden_engines_for_prompt(prompt)
    for eid, reason in forbidden:
        if eid in engine_ids:
            errors.append({
                "severity": "critical",
                "type": "prompt_forbidden",
                "engine": eid,
                "reason": reason
            })

    is_valid = len(errors) == 0
    return is_valid, errors


def get_golden_examples() -> str:
    """
    Return golden example presets that demonstrate perfect sonic coherence.
    """
    return """
GOLDEN EXAMPLE PRESETS (Learn from these):

1. HARSH/AGGRESSIVE ("Drag me over hot coals"):
   ✅ GOOD: Bit Crusher (18) + Rodent Distortion (21) + Ring Modulator (26) + Chaos Generator (51)
   ❌ BAD: Shimmer Reverb (42) - too gentle/bright
   ❌ BAD: Tape Echo (34) - adds warmth, contradicts harshness
   WHY GOOD: All engines add harshness/aggression/inharmonicity
   PARAMETERS: All high values (0.7-1.0) for extreme effect

2. DARK/UNDERWATER ("Dark underwater pressure"):
   ✅ GOOD: Comb Resonator (13) + Spectral Gate (48) + Convolution Reverb (41)
   ❌ BAD: Shimmer Reverb (42) - too bright for dark sounds
   ❌ BAD: Harmonic Exciter (17) - adds brightness
   WHY GOOD: All engines darken/deepen the sound
   PARAMETERS: Low-mid values (0.3-0.6) for controlled darkness

3. VINTAGE/WARM ("Smooth jazz lounge"):
   ✅ GOOD: Vintage Opto Compressor (1) + Vintage Tube Preamp (15) + Tape Echo (34) + Plate Reverb (39)
   ❌ BAD: Bit Crusher (18) - digital destruction clashes with analog warmth
   ❌ BAD: Ring Modulator (26) - too harsh/metallic
   WHY GOOD: All engines add analog warmth/smoothness
   PARAMETERS: Mid values (0.4-0.6) for balanced vintage character

4. ETHEREAL/BRIGHT ("Crystal shimmer cascade"):
   ✅ GOOD: Harmonic Exciter (17) + Resonant Chorus (24) + Shimmer Reverb (42) + Tape Echo (34)
   ❌ BAD: Dark filters - kill sparkle
   ❌ BAD: Heavy compression - reduces dynamics
   WHY GOOD: All engines add brightness/air/space
   PARAMETERS: Mid-high values (0.5-0.7) for sparkle without harshness
   NOTE: This is one of the FEW cases where Shimmer Reverb is appropriate!

5. GLITCHY/EXPERIMENTAL ("Glitchy robot voices from space"):
   ✅ GOOD: Phased Vocoder (49) + Ring Modulator (26) + Buffer Repeat (38) + Chaos Generator (51)
   ❌ BAD: Shimmer Reverb (42) - too organic/ethereal for robots
   ❌ BAD: Tape Echo (34) - too warm/analog for digital glitches
   WHY GOOD: All engines add digital/mechanical/chaotic character
   PARAMETERS: Varied (0.4-0.8) for controlled chaos

PATTERN RECOGNITION:
✅ Notice how EVERY engine in each example supports the SAME emotional intent
✅ Notice what's FORBIDDEN - not just missing, but actively harmful
✅ Notice frequency complementarity - effects don't fight for same freq range
✅ Notice parameter ranges match intensity - harsh = high values, gentle = low values
"""
