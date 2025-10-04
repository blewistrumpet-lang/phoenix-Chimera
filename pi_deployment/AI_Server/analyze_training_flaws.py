#!/usr/bin/env python3
"""
Analyze ALL the flaws in our training approach
Let's be thorough this time
"""

from engine_mapping_authoritative import *
import json

print("\n" + "="*80)
print("COMPREHENSIVE TRAINING SYSTEM ANALYSIS")
print("="*80)

# 1. Check which engines I referenced that don't exist
print("\n1. NON-EXISTENT ENGINES I REFERENCED:")
print("-" * 40)

fake_engines = [
    "ENGINE_SUB_BASS",
    "ENGINE_AUTO_WAH", 
    "ENGINE_VIBRATO",
    "ENGINE_MULTIBAND_COMPRESSOR",
    "ENGINE_KSTYLE_OVERDRIVE",
    "ENGINE_BUCKET_BRIGADE"
]

for fake in fake_engines:
    try:
        value = eval(fake)
        print(f"  ✓ {fake} exists (value: {value})")
    except:
        print(f"  ✗ {fake} DOES NOT EXIST!")

# 2. Analyze what engines would actually be used for bass
print("\n2. ACTUAL BASS-CAPABLE ENGINES:")
print("-" * 40)

bass_relevant = []

# Check each real engine
all_engines = [(name, value) for name, value in globals().items() 
               if name.startswith('ENGINE_') and isinstance(value, int) and 0 < value < 57]

for name, engine_id in all_engines:
    engine_name = get_engine_name(engine_id)
    category = get_engine_category(engine_id)
    
    # Determine if bass-relevant
    is_bass = False
    reason = ""
    
    if category == "Filters & EQ":
        is_bass = True
        reason = "Can shape bass frequencies"
    elif category == "Distortion" and "tube" in engine_name.lower():
        is_bass = True
        reason = "Adds warmth/harmonics to bass"
    elif "mono" in engine_name.lower():
        is_bass = True
        reason = "Centers sub bass"
    elif category == "Dynamics":
        is_bass = True
        reason = "Can control bass dynamics"
    elif "harmonic" in engine_name.lower():
        is_bass = True
        reason = "Can enhance bass harmonics"
        
    if is_bass:
        bass_relevant.append((engine_id, engine_name, reason))

print(f"Found {len(bass_relevant)} bass-capable engines:")
for eid, name, reason in bass_relevant[:10]:
    print(f"  {eid:2d}. {name}: {reason}")

# 3. Check prompt-to-engine logic
print("\n3. PROMPT KEYWORD TO ENGINE MAPPING ISSUES:")
print("-" * 40)

test_mappings = {
    "reverb": ["Should map to reverb engines"],
    "delay": ["Should map to delay/echo engines"],
    "distortion": ["Should map to distortion engines"],
    "filter": ["Should map to filter engines"],
    "compression": ["Should map to dynamics engines"],
    "wobble": ["Should map to modulation/filter engines"],
    "glitch": ["Should map to buffer/granular engines"],
    "warm": ["Should map to tube/tape engines"]
}

for keyword, expected in test_mappings.items():
    matching_engines = []
    
    for name, engine_id in all_engines:
        engine_name = get_engine_name(engine_id)
        category = get_engine_category(engine_id)
        
        if keyword == "reverb" and "reverb" in engine_name.lower():
            matching_engines.append(engine_name)
        elif keyword == "delay" and ("delay" in engine_name.lower() or "echo" in engine_name.lower()):
            matching_engines.append(engine_name)
        elif keyword == "distortion" and category == "Distortion":
            matching_engines.append(engine_name)
        elif keyword == "filter" and "filter" in engine_name.lower():
            matching_engines.append(engine_name)
        elif keyword == "compression" and category == "Dynamics":
            matching_engines.append(engine_name)
        elif keyword == "wobble" and ("filter" in engine_name.lower() or category == "Modulation"):
            matching_engines.append(engine_name)
        elif keyword == "glitch" and ("buffer" in engine_name.lower() or "granular" in engine_name.lower()):
            matching_engines.append(engine_name)
        elif keyword == "warm" and ("tube" in engine_name.lower() or "tape" in engine_name.lower()):
            matching_engines.append(engine_name)
    
    print(f"\n'{keyword}' → {len(matching_engines)} engines:")
    for eng in matching_engines[:3]:
        print(f"    • {eng}")

# 4. Check scoring function logic flaws
print("\n4. SCORING FUNCTION LOGIC FLAWS:")
print("-" * 40)

flaws = [
    "Using non-existent ENGINE_SUB_BASS",
    "Only rewarding exact keyword matches",
    "Not understanding creative engine combinations",
    "Penalizing >5 engines but not rewarding 3-4 range strongly enough",
    "Not checking if engines work well together",
    "Ignoring signal flow order",
    "Not considering genre-specific conventions",
    "Binary scoring (0 or 0.3) instead of gradual",
    "Not learning from actual successful presets"
]

for i, flaw in enumerate(flaws, 1):
    print(f"  {i}. {flaw}")

# 5. Training methodology issues
print("\n5. TRAINING METHODOLOGY ISSUES:")
print("-" * 40)

issues = [
    "Testing only 5-10 prompts per genome (not enough data)",
    "Population of 20 might be too small",
    "Mutation rate might be destroying good traits",
    "Not preserving diversity in population",
    "Evolution optimizing for wrong metric",
    "No validation set to prevent overfitting",
    "Not using actual user feedback",
    "Config values mutating randomly without understanding impact"
]

for i, issue in enumerate(issues, 1):
    print(f"  {i}. {issue}")

# 6. What we should be measuring
print("\n6. WHAT WE SHOULD ACTUALLY MEASURE:")
print("-" * 40)

metrics = [
    "Engine relevance to prompt keywords AND intent",
    "Creative but musically valid combinations",
    "Signal flow order (e.g., compression before reverb)",
    "Genre-appropriate selections",
    "Parameter coherence (not just engine selection)",
    "Diversity of solutions for similar prompts",
    "Consistency across similar prompts",
    "Actual audio quality (if we could measure it)"
]

for i, metric in enumerate(metrics, 1):
    print(f"  {i}. {metric}")

print("\n" + "="*80)
print("CONCLUSION:")
print("="*80)
print("""
The training system has multiple fundamental flaws:

1. CRITICAL: Referenced non-existent engines (ENGINE_SUB_BASS)
2. MAJOR: Scoring function too simplistic and literal
3. MAJOR: Not enough test data per generation
4. MODERATE: Population genetics not tuned properly
5. MODERATE: No understanding of musical context

We need a complete redesign of the scoring system that:
- Uses only real engines that exist
- Understands semantic relationships (EQ can handle bass)
- Rewards creative but valid combinations
- Considers signal flow and genre conventions
- Uses gradual scoring, not binary
""")