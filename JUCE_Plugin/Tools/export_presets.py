#!/usr/bin/env python3
"""
Export Golden Corpus presets to JSON by parsing the C++ source
"""

import json
import re
import os
from pathlib import Path

# Engine type mapping from EngineTypes.h
ENGINE_TYPES = {
    0: "Vintage Tube",
    1: "Tape Echo", 
    2: "Shimmer Reverb",
    3: "Plate Reverb",
    4: "Convolution Reverb",
    5: "Spring Reverb",
    6: "Opto Compressor",
    7: "VCA Compressor",
    8: "Magnetic Drum Echo",
    9: "Bucket Brigade Delay",
    10: "Analog Chorus",
    11: "Digital Chorus",
    12: "Analog Phaser",
    13: "Digital Phaser",
    14: "Pitch Shifter",
    15: "Ring Modulator",
    16: "Granular Cloud",
    17: "Vocal Formant Filter",
    18: "Dimension Expander",
    19: "Frequency Shifter",
    20: "Transient Shaper",
    21: "Harmonic Tremolo",
    22: "Classic Tremolo",
    23: "Comb Resonator",
    24: "Rotary Speaker",
    25: "Mid-Side Processor",
    26: "Vintage Console EQ",
    27: "Parametric EQ",
    28: "Ladder Filter",
    29: "State Variable Filter",
    30: "Formant Filter",
    31: "Wave Folder",
    32: "Harmonic Exciter",
    33: "Bit Crusher",
    34: "Multiband Saturator",
    35: "Muff Fuzz",
    36: "Rodent Distortion",
    37: "Tube Screamer",
    38: "K-Style Overdrive",
    39: "Spectral Freeze",
    40: "Buffer Repeat",
    41: "Chaos Generator",
    42: "Intelligent Harmonizer",
    43: "Gated Reverb",
    44: "Detune Doubler",
    45: "Phased Vocoder",
    46: "Spectral Gate",
    47: "Noise Gate",
    48: "Envelope Filter",
    49: "Feedback Network",
    50: "Mastering Limiter",
    51: "Stereo Widener",
    52: "Resonant Chorus",
    53: "Digital Delay",
    54: "Dynamic EQ",
    55: "Stereo Imager"
}

def parse_preset_function(content, function_name):
    """Parse a single preset creation function"""
    # Find the function - look for the function start and capture until "return preset;"
    pattern = rf"GoldenPreset {function_name}\(\)\s*\{{.*?return preset;\s*\}}"
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        print(f"Could not find function: {function_name}")
        return None
    
    func_content = match.group(0)
    
    # Extract basic info
    preset = {
        "id": re.search(r'preset\.id\s*=\s*"([^"]+)"', func_content).group(1),
        "name": re.search(r'preset\.name\s*=\s*"([^"]+)"', func_content).group(1),
        "technicalHint": re.search(r'preset\.technicalHint\s*=\s*"([^"]+)"', func_content).group(1) if re.search(r'preset\.technicalHint\s*=\s*"([^"]+)"', func_content) else "",
        "category": re.search(r'preset\.category\s*=\s*"([^"]+)"', func_content).group(1) if re.search(r'preset\.category\s*=\s*"([^"]+)"', func_content) else "Uncategorized",
        "subcategory": re.search(r'preset\.subcategory\s*=\s*"([^"]+)"', func_content).group(1) if re.search(r'preset\.subcategory\s*=\s*"([^"]+)"', func_content) else "",
        "engines": []
    }
    
    # Extract engine configurations
    for i in range(6):
        engine_type_match = re.search(rf'preset\.engineTypes\[{i}\]\s*=\s*ENGINE_(\w+);', func_content)
        if engine_type_match:
            engine_name = engine_type_match.group(1).replace("_", " ").title()
            
            # Find engine type number
            engine_type = None
            for type_id, type_name in ENGINE_TYPES.items():
                if engine_name.lower() in type_name.lower() or type_name.lower() in engine_name.lower():
                    engine_type = type_id
                    break
            
            if engine_type is None:
                # Try to find by constant name
                const_match = re.search(rf'ENGINE_{engine_type_match.group(1)}\s*=\s*(\d+)', content)
                if const_match:
                    engine_type = int(const_match.group(1))
                else:
                    print(f"Warning: Could not find engine type for ENGINE_{engine_type_match.group(1)}")
                    continue
            
            # Extract mix and active
            mix_match = re.search(rf'preset\.engineMix\[{i}\]\s*=\s*([\d.]+)f?;', func_content)
            active_match = re.search(rf'preset\.engineActive\[{i}\]\s*=\s*(true|false);', func_content)
            
            if mix_match and active_match and active_match.group(1) == "true":
                # Extract parameters
                params_match = re.search(rf'preset\.engineParams\[{i}\]\s*=\s*\{{([^}}]+)\}}', func_content)
                params = []
                if params_match:
                    params_str = params_match.group(1)
                    # Remove comments and extract floats
                    param_values = re.findall(r'([\d.]+)f?\s*(?:,|$)', params_str)
                    params = [float(p) for p in param_values]
                
                preset["engines"].append({
                    "slot": i,
                    "type": engine_type,
                    "typeName": ENGINE_TYPES.get(engine_type, f"Unknown ({engine_type})"),
                    "mix": float(mix_match.group(1)),
                    "params": params
                })
    
    # Extract profiles
    sonic_profile = {}
    for attr in ["brightness", "density", "movement", "space", "aggression", "vintage"]:
        match = re.search(rf'preset\.sonicProfile\.{attr}\s*=\s*([\d.]+)f?;', func_content)
        if match:
            sonic_profile[attr] = float(match.group(1))
    
    if sonic_profile:
        preset["sonicProfile"] = sonic_profile
    
    # Extract keywords
    keywords_match = re.search(r'preset\.keywords\s*=\s*\{([^}]+)\}', func_content)
    if keywords_match:
        keywords_str = keywords_match.group(1)
        keywords = [k.strip().strip('"') for k in keywords_str.split(',')]
        preset["keywords"] = keywords
    
    # Extract CPU tier
    cpu_match = re.search(r'preset\.cpuTier\s*=\s*(\w+);', func_content)
    if cpu_match:
        preset["cpuTier"] = cpu_match.group(1)
    
    return preset

def main():
    # Read the C++ source
    cpp_file = Path(__file__).parent.parent / "Source" / "GoldenCorpusPresets.cpp"
    with open(cpp_file, 'r') as f:
        content = f.read()
    
    # Output directory
    output_dir = Path(__file__).parent.parent / "GoldenCorpus"
    presets_dir = output_dir / "presets"
    presets_dir.mkdir(parents=True, exist_ok=True)
    
    # List of preset functions to parse
    preset_functions = [
        "createPreset_001_VelvetThunder",
        "createPreset_002_CrystalPalace",
        "createPreset_003_BrokenRadio",
        "createPreset_004_MidnightOil",
        "createPreset_005_GlassCathedral",
        "createPreset_006_NeonDreams",
        "createPreset_007_LiquidSunshine",
        "createPreset_008_IronButterfly",
        "createPreset_009_PhantomEmbrace",
        "createPreset_010_SolarFlare",
        "createPreset_011_DustAndEchoes",
        "createPreset_012_ThunderAndSilk",
        "createPreset_013_QuantumGarden",
        "createPreset_014_CopperResonance",
        "createPreset_015_AuroraBorealis",
        "createPreset_016_DigitalErosion",
        "createPreset_017_VelvetHammer",
        "createPreset_018_WhisperNetwork",
        "createPreset_019_CosmicStrings",
        "createPreset_020_RustAndBones",
        "createPreset_021_SilkRoadEcho",
        "createPreset_022_NeuralBloom",
        "createPreset_023_TidalForce",
        "createPreset_024_AmberPreservation",
        "createPreset_025_ZeroPointField",
        "createPreset_026_ArcticDrift",
        "createPreset_027_BrassFurnace",
        "createPreset_028_MycelialNetwork",
        "createPreset_029_StainedGlass",
        "createPreset_030_VoltageStorm"
    ]
    
    all_presets = []
    exported = 0
    
    for func_name in preset_functions:
        preset = parse_preset_function(content, func_name)
        if preset:
            # Save individual preset
            preset_file = presets_dir / f"{preset['id']}.json"
            with open(preset_file, 'w') as f:
                json.dump(preset, f, indent=2)
            
            all_presets.append(preset)
            exported += 1
            print(f"Exported: {preset['id']} - {preset['name']}")
    
    # Save all presets in one file
    all_presets_file = output_dir / "all_presets.json"
    with open(all_presets_file, 'w') as f:
        json.dump({
            "version": "1.0",
            "presetCount": len(all_presets),
            "presets": all_presets
        }, f, indent=2)
    
    # Create metadata
    categories = {}
    for preset in all_presets:
        cat = preset.get('category', 'Uncategorized')
        categories[cat] = categories.get(cat, 0) + 1
    
    metadata = {
        "version": "1.0",
        "presetCount": len(all_presets),
        "categories": [{"name": cat, "count": count} for cat, count in categories.items()]
    }
    
    metadata_file = output_dir / "corpus_metadata.json"
    with open(metadata_file, 'w') as f:
        json.dump(metadata, f, indent=2)
    
    print(f"\nExport complete!")
    print(f"Exported {exported} presets to {output_dir}")
    print(f"Ready for FAISS indexing!")

if __name__ == "__main__":
    main()