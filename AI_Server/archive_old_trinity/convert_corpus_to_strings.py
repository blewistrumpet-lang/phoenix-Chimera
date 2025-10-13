#!/usr/bin/env python3
"""
Convert Golden Corpus presets from numeric IDs to string identifiers
This is a one-time migration script
"""

import json
from pathlib import Path
from engine_definitions import get_engine_by_legacy_id, ENGINES

def convert_preset_to_string_ids(preset):
    """Convert a single preset to use string IDs"""
    converted_preset = preset.copy()
    
    # Convert engines array
    if "engines" in preset:
        converted_engines = []
        for engine in preset["engines"]:
            engine_copy = engine.copy()
            
            # Convert numeric type to string ID
            if "type" in engine:
                legacy_id = engine["type"]
                engine_info = get_engine_by_legacy_id(legacy_id)
                
                if engine_info:
                    # Replace numeric type with string ID
                    engine_copy["type"] = engine_info["key"]
                    engine_copy["typeName"] = engine_info["name"]
                    print(f"  Converting engine: {legacy_id} ({engine.get('typeName', 'Unknown')}) -> '{engine_info['key']}'")
                else:
                    print(f"  WARNING: Unknown engine ID {legacy_id}")
                    continue
            
            converted_engines.append(engine_copy)
        
        converted_preset["engines"] = converted_engines
    
    return converted_preset

def convert_corpus_file(input_path, output_path):
    """Convert entire corpus file to use string IDs"""
    print(f"Loading corpus from: {input_path}")
    
    with open(input_path, 'r') as f:
        corpus = json.load(f)
    
    print(f"Found {len(corpus.get('presets', []))} presets to convert")
    print("-" * 60)
    
    # Convert each preset
    converted_presets = []
    for i, preset in enumerate(corpus.get("presets", [])):
        print(f"\nConverting preset {i+1}: {preset.get('name', 'Unknown')}")
        converted = convert_preset_to_string_ids(preset)
        converted_presets.append(converted)
    
    # Create output corpus
    output_corpus = corpus.copy()
    output_corpus["presets"] = converted_presets
    output_corpus["version"] = "2.0"  # Bump version to indicate string IDs
    output_corpus["format"] = "string_ids"  # Add format indicator
    
    # Save converted corpus
    print(f"\nSaving converted corpus to: {output_path}")
    with open(output_path, 'w') as f:
        json.dump(output_corpus, f, indent=2)
    
    print(f"\n✅ Successfully converted {len(converted_presets)} presets to string IDs")
    
    # Verify conversion
    verify_conversion(output_corpus)

def verify_conversion(corpus):
    """Verify that all presets use valid string IDs"""
    print("\n" + "=" * 60)
    print("VERIFICATION")
    print("=" * 60)
    
    errors = []
    
    for preset in corpus.get("presets", []):
        preset_name = preset.get("name", "Unknown")
        
        for engine in preset.get("engines", []):
            engine_type = engine.get("type")
            
            # Check if type is a string
            if not isinstance(engine_type, str):
                errors.append(f"Preset '{preset_name}': Engine type is not a string: {engine_type}")
                continue
            
            # Check if it's a valid engine key
            if engine_type not in ENGINES:
                errors.append(f"Preset '{preset_name}': Invalid engine key: {engine_type}")
    
    if errors:
        print("❌ VERIFICATION FAILED:")
        for error in errors:
            print(f"  - {error}")
    else:
        print("✅ VERIFICATION PASSED: All presets use valid string IDs")
    
    # Print statistics
    print("\nStatistics:")
    engine_usage = {}
    for preset in corpus.get("presets", []):
        for engine in preset.get("engines", []):
            engine_type = engine.get("type")
            if engine_type:
                engine_usage[engine_type] = engine_usage.get(engine_type, 0) + 1
    
    print(f"  Total unique engines used: {len(engine_usage)}")
    print(f"  Most used engines:")
    for engine_key, count in sorted(engine_usage.items(), key=lambda x: x[1], reverse=True)[:5]:
        engine_name = ENGINES.get(engine_key, {}).get("name", "Unknown")
        print(f"    - {engine_key} ({engine_name}): {count} times")

def main():
    # Paths
    corpus_dir = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus")
    input_file = corpus_dir / "all_presets.json"
    output_file = corpus_dir / "all_presets_string_ids.json"
    
    if not input_file.exists():
        print(f"❌ Corpus file not found: {input_file}")
        return
    
    # Create backup
    backup_file = corpus_dir / "all_presets_numeric_backup.json"
    if not backup_file.exists():
        print(f"Creating backup at: {backup_file}")
        import shutil
        shutil.copy(input_file, backup_file)
    
    # Convert the corpus
    convert_corpus_file(input_file, output_file)
    
    print("\n" + "=" * 60)
    print("CONVERSION COMPLETE")
    print("=" * 60)
    print(f"Original corpus (numeric IDs): {input_file}")
    print(f"Backup of original: {backup_file}")
    print(f"Converted corpus (string IDs): {output_file}")
    print("\nNext steps:")
    print("1. Test the converted corpus with updated Trinity pipeline")
    print("2. Once verified, replace all_presets.json with all_presets_string_ids.json")
    print("3. Update all Python components to use string IDs")
    print("4. Update C++ plugin to handle string IDs from server")

if __name__ == "__main__":
    main()