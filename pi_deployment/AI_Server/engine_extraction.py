"""
Comprehensive engine extraction from natural language
Maps user intent to specific engine IDs with high accuracy
"""

from typing import List, Dict, Set
import re
import logging
from engine_mapping_authoritative import ENGINE_NAMES

logger = logging.getLogger(__name__)

# Comprehensive mappings of keywords/phrases to engine IDs
ENGINE_KEYWORDS = {
    # === DYNAMICS (0-5) ===
    "noise gate": [4],
    "gate": [4],
    "gating": [4],
    "classic compressor": [2],
    "compressor": [2],
    "compression": [2],
    "vintage opto": [1],
    "opto": [1],
    "optical compressor": [1],
    "la-2a": [1],
    "mastering limiter": [5],
    "limiter": [5],
    "limiting": [5],
    "transient shaper": [3],
    "transient": [3],
    
    # === EQ (6-8) ===
    "parametric eq": [7],
    "parametric": [7],
    "vintage console": [6],
    "console eq": [6],
    "neve": [6],
    "api": [6],
    "dynamic eq": [8],
    
    # === FILTERS (9-14) ===
    "ladder filter": [9],
    "ladder": [9],
    "moog": [9],
    "state variable": [10],
    "vocal formant": [11],
    "formant filter": [11],
    "talk box": [11],
    "envelope filter": [12],
    "auto-wah": [12],
    "resonant filter": [13],
    "high pass": [14],
    "hpf": [14],
    
    # === DISTORTION (15-22) ===
    "vintage tube": [15],
    "tube": [15],
    "tube warmth": [15],
    "tube preamp": [15],
    "vacuum tube": [15],
    "valve": [15],
    "tape saturation": [16],
    "tape": [16],
    "harmonic exciter": [17],
    "exciter": [17],
    "bit crusher": [18],
    "bitcrusher": [18],
    "bit crush": [18],
    "8-bit": [18],
    "lo-fi": [18],
    "wave shaper": [19],
    "waveshaper": [19],
    "muff fuzz": [20],
    "muff": [20],
    "fuzz": [20],
    "rodent": [21],
    "rodent distortion": [21],
    "rat": [21],
    "k-style": [22],
    "k style": [22],
    "klon": [22],
    "overdrive": [22],
    
    # === MODULATION (23-33) ===
    "ring modulator": [23],
    "ring mod": [23],
    "classic chorus": [24],
    "chorus": [24],
    "analog phaser": [25],
    "phaser": [25],
    "phase": [25],
    "vintage flanger": [26],
    "flanger": [26],
    "flange": [26],
    "classic tremolo": [27],
    "tremolo": [27],
    "vintage vibrato": [28],
    "vibrato": [28],
    "leslie": [29],
    "rotary speaker": [29],
    "rotary": [29],
    "ensemble": [30],
    "auto-pan": [31],
    "auto pan": [31],
    "autopan": [31],
    "panning": [31],
    "frequency shifter": [32],
    "freq shifter": [32],
    "bode": [33],
    
    # === DELAY (34-37) ===
    "tape echo": [34],
    "tape delay": [34],
    "echo": [34],
    "analog delay": [35],
    "analog echo": [35],
    "digital delay": [36],
    "reverse delay": [37],
    "reverse echo": [37],
    "backwards": [37],
    
    # === REVERB (38-43) ===
    "hall reverb": [38],
    "hall": [38],
    "concert hall": [38],
    "plate reverb": [39],
    "plate": [39],
    "emt": [39],
    "spring reverb": [40],
    "spring": [40],
    "room reverb": [41],
    "room": [41],
    "shimmer reverb": [42],
    "shimmer": [42],
    "shimmering": [42],
    "convolution": [43],
    "impulse response": [43],
    "ir reverb": [43],
    "gated reverb": [43],
    "gated": [43],
    
    # === SPATIAL (44-46) ===
    "stereo imager": [44],
    "stereo widener": [44],
    "widener": [44],
    "intelligent harmonizer": [45],
    "harmonizer": [45],
    "harmony": [45],
    "dimension expander": [46],
    "dimension": [46],
    
    # === PITCH (47-50) ===
    "detune": [47],
    "doubler": [47],
    "pitch shifter": [48],
    "pitch shift": [48],
    "pitch": [48],
    "gender bender": [49],
    "gender": [49],
    "formant shifter": [50],
    "formant shift": [50],
    
    # === SPECIAL (51-52) ===
    "vocoder": [51],
    "talk box": [51],
    "chaos generator": [52],
    "chaos": [52],
    "random": [52],
    
    # === UTILITY (53-56) ===
    "phase align": [53],
    "phase alignment": [53],
    "gain": [54],
    "volume": [54],
    "mid-side": [55],
    "m/s": [55],
    "mid side": [55],
    "phase rotator": [56],
    
    # === CHARACTER DESCRIPTORS -> ENGINES ===
    "warm": [15, 1, 39],  # Tube, Opto, Plate
    "vintage": [15, 1, 34, 40, 6],  # Tube, Opto, Tape Echo, Spring, Console EQ
    "aggressive": [22, 21, 20, 4],  # K-Style, Rodent, Muff, Gate
    "clean": [2, 7, 54],  # Compressor, Parametric EQ, Gain
    "ambient": [42, 39, 46, 35],  # Shimmer, Plate, Dimension, Analog Delay
    "spacious": [42, 39, 44, 46],  # Shimmer, Plate, Imager, Dimension
    "tight": [4, 2, 3],  # Gate, Compressor, Transient
    "ethereal": [42, 39, 37],  # Shimmer, Plate, Reverse Delay
    "lofi": [18, 16],  # Bit Crusher, Tape
    "modern": [8, 36, 5],  # Dynamic EQ, Digital Delay, Limiter
    "bright": [17, 7],  # Exciter, Parametric EQ
    "dark": [9, 10],  # Ladder, State Variable filters
}

def extract_required_engines(prompt: str) -> List[int]:
    """
    Extract all engines that are explicitly or implicitly requested in the prompt
    
    Returns list of engine IDs that MUST be included
    """
    prompt_lower = prompt.lower()
    required_engines: Set[int] = set()
    
    # Method 1: Direct engine name matching
    for engine_id, engine_name in ENGINE_NAMES.items():
        if engine_id == 0:  # Skip "None"
            continue
        
        # Check if full engine name is mentioned
        if engine_name.lower() in prompt_lower:
            required_engines.add(engine_id)
            logger.info(f"Found direct match: '{engine_name}' -> Engine {engine_id}")
    
    # Method 2: Keyword matching
    for keyword, engine_ids in ENGINE_KEYWORDS.items():
        if keyword in prompt_lower:
            for engine_id in engine_ids:
                required_engines.add(engine_id)
                logger.info(f"Found keyword match: '{keyword}' -> Engine {engine_id}")
    
    # Method 3: Special phrase patterns
    special_patterns = [
        (r"need[s]?\s+(\w+\s+)?(reverb|delay|compression|distortion)", {
            "reverb": [39, 42, 40],  # Default to Plate, Shimmer, Spring
            "delay": [34, 35],  # Tape Echo, Analog Delay
            "compression": [2, 1],  # Classic, Opto
            "distortion": [15, 21, 22]  # Tube, Rodent, K-Style
        }),
        (r"give me\s+(\w+\s+)?(reverb|delay|compression|distortion)", {
            "reverb": [39],
            "delay": [34],
            "compression": [2],
            "distortion": [15]
        }),
        (r"(specifically|exactly|definitely)\s+(\w+)", {})  # Emphasizes requirement
    ]
    
    for pattern, mappings in special_patterns:
        matches = re.finditer(pattern, prompt_lower)
        for match in matches:
            effect_type = match.group(2) if match.lastindex >= 2 else match.group(1)
            if effect_type and effect_type in mappings:
                for engine_id in mappings[effect_type]:
                    required_engines.add(engine_id)
                    logger.info(f"Found pattern match: '{effect_type}' -> Engine {engine_id}")
    
    # Method 4: Handle "with" and "and" constructions
    # "vintage tube with plate reverb" -> both are required
    if " with " in prompt_lower or " and " in prompt_lower:
        # Split on conjunctions and process each part
        parts = re.split(r'\s+(?:with|and)\s+', prompt_lower)
        for part in parts:
            for keyword, engine_ids in ENGINE_KEYWORDS.items():
                if keyword in part:
                    for engine_id in engine_ids:
                        required_engines.add(engine_id)
    
    # Method 5: Numbers in prompt (e.g., "Use engine 15")
    number_matches = re.finditer(r'engine\s+(\d+)', prompt_lower)
    for match in number_matches:
        engine_id = int(match.group(1))
        if 0 < engine_id <= 56:  # Valid engine range
            required_engines.add(engine_id)
            logger.info(f"Found explicit engine number: {engine_id}")
    
    result = list(required_engines)
    
    if result:
        engine_names = [ENGINE_NAMES.get(e, f"Unknown({e})") for e in result]
        logger.info(f"Total required engines: {engine_names}")
    
    return result

def test_extraction():
    """Test the extraction with various prompts"""
    test_prompts = [
        "I need vintage tube warmth with plate reverb",
        "Give me shimmer reverb specifically",
        "Aggressive metal with noise gate and k-style overdrive",
        "Clean sound with classic compressor and parametric eq",
        "Use bit crusher for lo-fi sound",
        "Ethereal ambient with shimmer and dimension expander",
        "Tape echo and spring reverb for vintage vibe",
        "Engine 15 and engine 39 please"
    ]
    
    for prompt in test_prompts:
        print(f"\nPrompt: '{prompt}'")
        engines = extract_required_engines(prompt)
        print(f"Required engines: {engines}")
        if engines:
            names = [ENGINE_NAMES.get(e, f"Unknown({e})") for e in engines]
            print(f"  -> {', '.join(names)}")

if __name__ == "__main__":
    import logging
    logging.basicConfig(level=logging.INFO)
    test_extraction()