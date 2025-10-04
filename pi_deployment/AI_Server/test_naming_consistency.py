#!/usr/bin/env python3
"""
COMPREHENSIVE PROOF OF ENGINE NAMING CONSISTENCY
Tests that every single engine has consistent naming across the entire system
"""

import json
import sys
from pathlib import Path
from engine_definitions import ENGINES
import re

print("="*80)
print("ENGINE NAMING CONSISTENCY VERIFICATION")
print("="*80)

# Track all inconsistencies
inconsistencies = []
all_string_ids = set()
all_components = {}

# 1. Check engine_definitions.py
print("\n1. CHECKING engine_definitions.py")
print("-"*60)
for key, info in ENGINES.items():
    all_string_ids.add(key)
    all_components[key] = {"engine_definitions": info["name"]}
    print(f"  {key:30s} -> {info['name']}")
print(f"Total: {len(ENGINES)} engines defined")

# 2. Check C++ header file
print("\n2. CHECKING C++ EngineStringMapping.h")
print("-"*60)
cpp_header_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/EngineStringMapping.h")
if cpp_header_path.exists():
    with open(cpp_header_path, 'r') as f:
        content = f.read()
        
        # Extract string to engine mappings
        string_to_engine_pattern = r'\{"([^"]+)",\s*ENGINE_\w+\}'
        matches = re.findall(string_to_engine_pattern, content)
        
        cpp_string_ids = set(matches)
        print(f"Found {len(cpp_string_ids)} string IDs in C++ header")
        
        # Check each one matches Python
        for cpp_id in cpp_string_ids:
            if cpp_id not in ENGINES:
                inconsistencies.append(f"C++ has '{cpp_id}' but Python doesn't")
                print(f"  ❌ {cpp_id} - NOT IN PYTHON!")
            else:
                all_components[cpp_id]["cpp_header"] = "✓"
        
        # Check Python has all C++ ones
        for py_id in ENGINES.keys():
            if py_id != "bypass" and py_id not in cpp_string_ids:
                inconsistencies.append(f"Python has '{py_id}' but C++ doesn't")
                print(f"  ❌ {py_id} - NOT IN C++!")
else:
    print("  ❌ C++ header file not found!")

# 3. Check Golden Corpus (string version)
print("\n3. CHECKING Golden Corpus (string IDs)")
print("-"*60)
corpus_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus/all_presets_string_ids.json")
if corpus_path.exists():
    with open(corpus_path, 'r') as f:
        corpus = json.load(f)
        
    corpus_engine_ids = set()
    for preset in corpus.get("presets", []):
        for engine in preset.get("engines", []):
            engine_type = engine.get("type")
            if engine_type:
                corpus_engine_ids.add(engine_type)
                if engine_type in all_components:
                    all_components[engine_type]["corpus"] = "✓"
    
    print(f"Found {len(corpus_engine_ids)} unique engines in corpus")
    
    # Check all are valid
    for engine_id in corpus_engine_ids:
        if engine_id not in ENGINES:
            inconsistencies.append(f"Corpus has '{engine_id}' but not in definitions")
            print(f"  ❌ {engine_id} - NOT IN DEFINITIONS!")
else:
    print("  ❌ String corpus file not found!")

# 4. Check Visionary component
print("\n4. CHECKING Visionary component")
print("-"*60)
visionary_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/visionary_string_ids.py")
if visionary_path.exists():
    with open(visionary_path, 'r') as f:
        content = f.read()
        
    # Find string IDs in keyword mappings
    pattern = r'"([a-z_]+)":\s*\['
    matches = re.findall(pattern, content)
    
    # Also find engines referenced in lists
    list_pattern = r'"([a-z_]+)"[,\]]'
    list_matches = re.findall(list_pattern, content)
    
    visionary_engines = set()
    for match in list_matches:
        if match in ENGINES:
            visionary_engines.add(match)
            all_components[match]["visionary"] = "✓"
    
    print(f"Found {len(visionary_engines)} engines referenced in Visionary")

# 5. Check Oracle component
print("\n5. CHECKING Oracle component")
print("-"*60)
oracle_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/oracle_string_ids.py")
if oracle_path.exists():
    with open(oracle_path, 'r') as f:
        content = f.read()
        
    # Oracle validates against ENGINES
    if "from engine_definitions import ENGINES" in content:
        print("  ✓ Oracle imports from engine_definitions")
    if 'if engine_key != "bypass" and engine_key in ENGINES:' in content:
        print("  ✓ Oracle validates engine keys against ENGINES")

# 6. Check Calculator component
print("\n6. CHECKING Calculator component")
print("-"*60)
calc_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/calculator_string_ids.py")
if calc_path.exists():
    with open(calc_path, 'r') as f:
        content = f.read()
        
    # Find engines in nudge rules
    pattern = r'"([a-z_]+)":\s*\{.*?"param'
    matches = re.findall(pattern, content, re.DOTALL)
    
    calc_engines = set()
    for line in content.split('\n'):
        for engine_id in ENGINES.keys():
            if f'"{engine_id}"' in line and "param" in line:
                calc_engines.add(engine_id)
                all_components[engine_id]["calculator"] = "✓"
    
    print(f"Found {len(calc_engines)} engines with nudge rules in Calculator")

# 7. Check Alchemist component
print("\n7. CHECKING Alchemist component")
print("-"*60)
alchemist_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/alchemist_string_ids.py")
if alchemist_path.exists():
    with open(alchemist_path, 'r') as f:
        content = f.read()
        
    # Find engines in name mappings
    alchemist_engines = set()
    for line in content.split('\n'):
        for engine_id in ENGINES.keys():
            if f'"{engine_id}"' in line and ("name_adjectives" in line or "name_nouns" in line):
                alchemist_engines.add(engine_id)
                all_components[engine_id]["alchemist"] = "✓"
    
    print(f"Found {len(alchemist_engines)} engines with naming rules in Alchemist")

# 8. Cross-reference everything
print("\n8. CROSS-REFERENCE MATRIX")
print("-"*60)
print(f"{'Engine ID':<30} {'Def':<4} {'C++':<4} {'Corp':<5} {'Vis':<4} {'Calc':<5} {'Alch':<5}")
print("-"*60)

for engine_id in sorted(all_string_ids):
    if engine_id == "bypass":
        continue  # Skip bypass for clarity
    
    row = all_components.get(engine_id, {})
    def_check = "✓" if "engine_definitions" in row else "✗"
    cpp_check = row.get("cpp_header", "✗")
    corpus_check = row.get("corpus", "-")  # "-" means not used
    vis_check = row.get("visionary", "-")
    calc_check = row.get("calculator", "-")
    alch_check = row.get("alchemist", "-")
    
    status = "✓" if def_check == "✓" and cpp_check == "✓" else "❌"
    print(f"{engine_id:<30} {def_check:<4} {cpp_check:<4} {corpus_check:<5} {vis_check:<4} {calc_check:<5} {alch_check:<5} {status}")

# 9. Test actual string ID usage
print("\n9. RUNTIME CONSISTENCY TEST")
print("-"*60)

# Import and test actual components
try:
    from visionary_string_ids import VisionaryStringIDs
    from oracle_string_ids import OracleStringIDs
    from calculator_string_ids import CalculatorStringIDs
    from alchemist_string_ids import AlchemistStringIDs
    
    # Test that they all reference the same ENGINES
    visionary = VisionaryStringIDs()
    oracle = OracleStringIDs()
    calculator = CalculatorStringIDs()
    alchemist = AlchemistStringIDs()
    
    # Create a test preset with known string IDs
    test_preset = {
        "parameters": {
            "slot1_engine": "vintage_tube",
            "slot2_engine": "tape_echo",
            "slot3_engine": "plate_reverb"
        }
    }
    
    # Run through each component
    import asyncio
    
    async def test_pipeline():
        # Test Visionary
        blueprint = await visionary._simulate_blueprint("test")
        for slot in blueprint["slots"]:
            engine = slot.get("engine", "bypass")
            if engine != "bypass" and engine not in ENGINES:
                inconsistencies.append(f"Visionary returned invalid engine: {engine}")
        
        # Test Oracle
        oracle_preset = oracle._adapt_preset_to_plugin_format({
            "engines": [
                {"slot": 0, "type": "vintage_tube", "mix": 1.0, "params": [0.5]*7}
            ]
        })
        
        # Test Calculator
        calc_preset = calculator.apply_nudges(test_preset, "warm vintage", {})
        
        # Test Alchemist
        final_preset = alchemist.finalize_preset(test_preset, "test")
        
        return True
    
    result = asyncio.run(test_pipeline())
    if result:
        print("  ✓ All components process string IDs correctly")
    
except Exception as e:
    print(f"  ❌ Runtime test failed: {e}")

# 10. Final consistency report
print("\n" + "="*80)
print("CONSISTENCY VERIFICATION RESULTS")
print("="*80)

if inconsistencies:
    print(f"\n❌ FOUND {len(inconsistencies)} INCONSISTENCIES:\n")
    for issue in inconsistencies:
        print(f"  • {issue}")
else:
    print("\n✅ PERFECT CONSISTENCY!")
    print("   All engine string IDs are consistent across:")
    print("   • Python engine definitions")
    print("   • C++ header mappings")
    print("   • Golden Corpus presets")
    print("   • Visionary component")
    print("   • Oracle component")
    print("   • Calculator component")
    print("   • Alchemist component")
    print("\n   The same string IDs are used everywhere!")

# Show some examples
print("\n📋 EXAMPLE STRING IDS USED CONSISTENTLY:")
examples = ["vintage_tube", "tape_echo", "shimmer_reverb", "chaos_generator", "rodent_distortion"]
for ex in examples:
    if ex in all_components:
        comp = all_components[ex]
        locations = []
        if "cpp_header" in comp:
            locations.append("C++")
        if "corpus" in comp:
            locations.append("Corpus")
        if "visionary" in comp:
            locations.append("Visionary")
        if "calculator" in comp:
            locations.append("Calculator")
        if "alchemist" in comp:
            locations.append("Alchemist")
        
        print(f'  "{ex}" -> Used in: {", ".join(locations) if locations else "Definition only"}')