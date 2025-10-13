#!/usr/bin/env python3
"""
Deep investigation into corpus quality issues
"""

import json
import numpy as np
from pathlib import Path
from collections import defaultdict, Counter
from engine_mapping_authoritative import ENGINE_NAMES

def investigate_corpus():
    """Deep dive into what's wrong with the corpus"""
    
    print("🔍 CORPUS INVESTIGATION")
    print("=" * 80)
    
    # Load the corpus
    corpus_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3")
    preset_file = corpus_path / "faiss_index" / "presets_clean.json"
    
    if not preset_file.exists():
        print(f"❌ Corpus file not found: {preset_file}")
        return
    
    with open(preset_file, 'r') as f:
        presets = json.load(f)
    
    print(f"Loaded {len(presets)} presets\n")
    
    # 1. CHECK PARAMETER VALUES
    print("1. PARAMETER ANALYSIS:")
    print("-" * 40)
    
    all_param_values = []
    param_by_position = defaultdict(list)
    presets_with_params = []
    
    for i, preset in enumerate(presets):
        preset_params = {}
        has_params = False
        
        for slot in range(1, 7):
            for param in range(10):
                key = f"slot{slot}_param{param}"
                if key in preset:
                    value = preset[key]
                    all_param_values.append(value)
                    param_by_position[f"param{param}"].append(value)
                    preset_params[key] = value
                    has_params = True
        
        if has_params:
            presets_with_params.append({
                "name": preset.get("creative_name", f"Preset {i}"),
                "params": preset_params
            })
    
    # Show first few presets with their parameters
    print("Sample presets and their parameters:")
    for i, p in enumerate(presets_with_params[:3]):
        print(f"\n'{p['name']}':")
        # Show first few params
        shown = 0
        for key, value in sorted(p['params'].items())[:5]:
            print(f"  {key}: {value}")
            shown += 1
        if len(p['params']) > 5:
            print(f"  ... and {len(p['params']) - 5} more parameters")
    
    # Analyze parameter statistics
    if all_param_values:
        print(f"\n📊 Parameter Statistics:")
        print(f"  Total parameter values found: {len(all_param_values)}")
        print(f"  Unique values: {len(set(all_param_values))}")
        print(f"  Min value: {min(all_param_values)}")
        print(f"  Max value: {max(all_param_values)}")
        print(f"  Mean value: {np.mean(all_param_values):.4f}")
        print(f"  Variance: {np.var(all_param_values):.6f}")
        
        # Show value distribution
        value_counts = Counter(all_param_values)
        print(f"\n  Most common values:")
        for value, count in value_counts.most_common(5):
            percentage = (count / len(all_param_values)) * 100
            print(f"    {value}: {count} times ({percentage:.1f}%)")
    else:
        print("  ⚠️ NO PARAMETERS FOUND IN PRESETS!")
    
    # 2. CHECK ENGINES
    print("\n2. ENGINE ANALYSIS:")
    print("-" * 40)
    
    engine_usage = Counter()
    engines_per_preset = []
    
    for preset in presets:
        preset_engines = []
        for slot in range(1, 7):
            engine_id = preset.get(f"slot{slot}_engine", 0)
            if engine_id > 0:
                preset_engines.append(engine_id)
                engine_usage[engine_id] += 1
        
        engines_per_preset.append(len(preset_engines))
        
        # Show a sample preset's engines
        if len(preset_engines) > 0 and len(engines_per_preset) <= 3:
            print(f"\n'{preset.get('creative_name', 'Unknown')}':")
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    print(f"  Slot {slot}: {ENGINE_NAMES.get(engine_id, f'Unknown({engine_id})')}")
    
    avg_engines = np.mean(engines_per_preset) if engines_per_preset else 0
    print(f"\n📊 Engine Statistics:")
    print(f"  Average engines per preset: {avg_engines:.2f}")
    print(f"  Min engines: {min(engines_per_preset) if engines_per_preset else 0}")
    print(f"  Max engines: {max(engines_per_preset) if engines_per_preset else 0}")
    
    # 3. CHECK NAMES
    print("\n3. NAMING ANALYSIS:")
    print("-" * 40)
    
    names = [p.get("creative_name", "") for p in presets]
    name_counts = Counter(names)
    
    print("Most common names:")
    for name, count in name_counts.most_common(10):
        if count > 1:
            print(f"  '{name}': {count} times ⚠️ DUPLICATE")
        else:
            print(f"  '{name}': {count} time")
    
    # 4. CHECK STRUCTURE
    print("\n4. PRESET STRUCTURE ANALYSIS:")
    print("-" * 40)
    
    # Check what fields are present
    all_keys = set()
    for preset in presets:
        all_keys.update(preset.keys())
    
    print("Fields found in presets:")
    for key in sorted(all_keys)[:20]:
        if not key.startswith("slot"):
            count = sum(1 for p in presets if key in p)
            print(f"  {key}: in {count}/{len(presets)} presets")
    
    # 5. DIAGNOSIS
    print("\n" + "=" * 80)
    print("🔬 DIAGNOSIS: WHY THE CORPUS IS 'USELESS'")
    print("=" * 80)
    
    problems = []
    
    if len(all_param_values) == 0:
        problems.append("NO PARAMETERS: Presets have no parameter values!")
    elif np.var(all_param_values) < 0.001:
        problems.append(f"ZERO VARIANCE: All parameters are the same value ({np.mean(all_param_values):.4f})")
    
    duplicate_names = sum(1 for count in name_counts.values() if count > 1)
    if duplicate_names > len(presets) * 0.2:
        problems.append(f"DUPLICATE NAMES: {duplicate_names} presets share names")
    
    if avg_engines < 2:
        problems.append(f"TOO FEW ENGINES: Average {avg_engines:.1f} engines per preset")
    
    if problems:
        print("\n❌ CRITICAL PROBLEMS FOUND:")
        for i, problem in enumerate(problems, 1):
            print(f"{i}. {problem}")
    else:
        print("\n✅ Corpus appears structurally sound")
    
    return presets, problems

def test_openai_only_pipeline():
    """Test if we can bypass the corpus entirely"""
    
    print("\n" + "=" * 80)
    print("🤖 TESTING OPENAI-ONLY APPROACH")
    print("=" * 80)
    
    print("""
HYPOTHESIS: Can OpenAI generate complete presets without corpus?

CURRENT ARCHITECTURE:
1. Visionary (OpenAI) → Creative interpretation
2. Oracle (Corpus) → Find similar preset
3. Calculator → Adjust parameters
4. Alchemist → Validate

PROPOSED ARCHITECTURE:
1. Visionary (OpenAI) → Generate COMPLETE preset
2. Validator → Ensure safety/validity
3. Alchemist → Optimize signal chain

ADVANTAGES:
✅ No corpus maintenance
✅ Infinite variety
✅ Always up-to-date
✅ Context-aware generation

DISADVANTAGES:
❌ No proven starting points
❌ May generate invalid combinations
❌ Slower (API calls)
❌ Costs money per request
❌ Less predictable quality
    """)
    
    print("\n🧪 EXPERIMENT: Generate preset with OpenAI only")
    
    # Example of what OpenAI could generate
    example_preset = {
        "name": "Ethereal Space Cathedral",
        "slot1_engine": 42,  # Shimmer Reverb
        "slot1_param0": 0.7,  # Size
        "slot1_param5": 0.4,  # Mix
        "slot2_engine": 46,  # Dimension Expander
        "slot2_param0": 0.5,  # Width
        "slot3_engine": 35,  # Digital Delay
        "slot3_param0": 0.6,  # Time
        "slot3_param1": 0.3,  # Feedback
        "slot3_param5": 0.25, # Mix
        # Generated based on "ethereal space" understanding
    }
    
    print("\nOpenAI could generate:")
    print(json.dumps(example_preset, indent=2)[:500])

def analyze_corpus_necessity():
    """Analyze if corpus is still necessary"""
    
    print("\n" + "=" * 80)
    print("📊 CORPUS NECESSITY ANALYSIS")
    print("=" * 80)
    
    print("""
DO WE STILL NEED THE CORPUS?

Arguments FOR keeping corpus:
1. RELIABILITY: Known-good starting points
2. SPEED: FAISS search is fast (0.1s)
3. QUALITY: Professional presets as foundation
4. CONSISTENCY: Predictable results
5. OFFLINE: Works without internet/API

Arguments AGAINST corpus:
1. MAINTENANCE: Constant curation needed
2. LIMITED: Only 150 presets (currently broken)
3. STATIC: Can't adapt to new requests
4. BIAS: Returns same presets repeatedly
5. COMPLEXITY: Extra component to maintain

HYBRID APPROACH (RECOMMENDED):
1. Use OpenAI as PRIMARY generator
2. Use corpus as FALLBACK/VALIDATION
3. Learn from successful generations
4. Build corpus dynamically from good results
    """)
    
    print("\n💡 RECOMMENDATION:")
    print("""
Phase 1: Fix existing corpus (1 week)
- Add parameter variance
- Fix naming
- Validate quality

Phase 2: Implement OpenAI generation (1 week)
- Extend Visionary to generate full presets
- Add strong validation
- Test quality

Phase 3: Compare approaches (1 week)
- A/B test corpus vs OpenAI
- Measure quality, speed, cost
- User preference testing

Phase 4: Implement best solution
- Either pure OpenAI
- Or hybrid approach
- Or improved corpus
    """)

def create_corpus_fix_plan():
    """Create actionable plan to fix corpus"""
    
    print("\n" + "=" * 80)
    print("🔧 HOW TO FIX THE CORPUS")
    print("=" * 80)
    
    print("""
IMMEDIATE FIXES NEEDED:

1. ADD PARAMETER VARIANCE:
   ```python
   for preset in corpus:
       for slot in range(1, 7):
           if preset[f"slot{slot}_engine"] > 0:
               # Generate varied parameters
               for param in range(10):
                   if param == 0:  # Main parameter
                       value = random.uniform(0.3, 0.8)
                   elif param == 5:  # Mix parameter  
                       value = random.uniform(0.2, 0.5)
                   else:
                       value = random.uniform(0.0, 1.0)
                   preset[f"slot{slot}_param{param}"] = round(value, 3)
   ```

2. FIX DUPLICATE NAMES:
   ```python
   used_names = set()
   for preset in corpus:
       original_name = preset["creative_name"]
       if original_name in used_names:
           # Generate unique variation
           preset["creative_name"] = generate_unique_name(original_name)
       used_names.add(preset["creative_name"])
   ```

3. ADD GENRE TAGS:
   ```python
   for preset in corpus:
       preset["genre"] = detect_genre(preset["creative_name"])
       preset["tags"] = extract_tags(preset)
   ```

4. VALIDATE & SCORE:
   ```python
   validator = PresetValidator()
   valid_corpus = []
   for preset in corpus:
       valid, errors, score = validator.validate_preset(preset)
       if valid and score > 0.7:
           preset["quality_score"] = score
           valid_corpus.append(preset)
   ```

5. REBUILD FAISS INDEX:
   ```python
   embeddings = generate_embeddings(valid_corpus)
   index = faiss.IndexFlatL2(384)
   index.add(embeddings)
   faiss.write_index(index, "corpus_fixed.index")
   ```
    """)

def main():
    # Investigate corpus issues
    presets, problems = investigate_corpus()
    
    # Test OpenAI-only approach
    test_openai_only_pipeline()
    
    # Analyze if corpus is necessary
    analyze_corpus_necessity()
    
    # Create fix plan
    create_corpus_fix_plan()
    
    print("\n" + "=" * 80)
    print("✅ INVESTIGATION COMPLETE")
    print("=" * 80)
    
    print("\n🎯 KEY FINDINGS:")
    if problems:
        for problem in problems:
            print(f"  • {problem}")
    
    print("\n📝 NEXT STEPS:")
    print("1. Fix corpus parameters (CRITICAL)")
    print("2. Test OpenAI-only generation")
    print("3. Compare both approaches")
    print("4. Implement best solution")

if __name__ == "__main__":
    main()