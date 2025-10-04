#!/usr/bin/env python3
"""
Preset Parser - Extracts Golden Corpus presets from C++ source and converts to JSON
More practical than C++ exporter - no compilation needed!
"""

import re
import json
import os
from pathlib import Path
from typing import Dict, List, Any
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class PresetParser:
    """Parse C++ Golden Corpus presets and convert to JSON"""
    
    def __init__(self):
        self.presets = []
        
    def parse_float(self, text: str) -> float:
        """Extract float value from C++ code"""
        # Handle scientific notation and 'f' suffix
        text = text.strip().rstrip('f')
        try:
            return float(text)
        except:
            return 0.5  # Default
    
    def parse_string(self, text: str) -> str:
        """Extract string value from C++ code"""
        # Remove quotes and handle escape sequences
        return text.strip().strip('"').replace('\\n', '\n').replace('\\"', '"')
    
    def parse_string_array(self, text: str) -> List[str]:
        """Extract string array from C++ code"""
        # Find all quoted strings
        strings = re.findall(r'"([^"]+)"', text)
        return [s.replace('\\n', '\n').replace('\\"', '"') for s in strings]
    
    def parse_engine_params(self, text: str) -> List[float]:
        """Extract engine parameters from C++ array initialization"""
        # Find all float values in the array
        values = re.findall(r'([\d.]+e?[-+]?\d*)f?\s*[,}]', text)
        return [self.parse_float(v) for v in values]
    
    def parse_preset_function(self, function_text: str) -> Dict[str, Any]:
        """Parse a single preset creation function"""
        preset = {}
        
        # Extract preset ID
        id_match = re.search(r'preset\.id\s*=\s*"([^"]+)"', function_text)
        if id_match:
            preset['id'] = id_match.group(1)
        
        # Extract basic fields
        fields = [
            ('name', r'preset\.name\s*=\s*"([^"]+)"'),
            ('technicalHint', r'preset\.technicalHint\s*=\s*"([^"]+)"'),
            ('shortCode', r'preset\.shortCode\s*=\s*"([^"]+)"'),
            ('category', r'preset\.category\s*=\s*"([^"]+)"'),
            ('subcategory', r'preset\.subcategory\s*=\s*"([^"]+)"'),
            ('signature', r'preset\.signature\s*=\s*"([^"]+)"'),
            ('bestFor', r'preset\.bestFor\s*=\s*"([^"]+)"'),
            ('avoidFor', r'preset\.avoidFor\s*=\s*"([^"]+)"'),
            ('musicalKey', r'preset\.musicalKey\s*=\s*"([^"]*)"'),
        ]
        
        for field, pattern in fields:
            match = re.search(pattern, function_text)
            if match:
                preset[field] = match.group(1)
        
        # Extract numeric fields
        numeric_fields = [
            ('actualCpuPercent', r'preset\.actualCpuPercent\s*=\s*([\d.]+)f?'),
            ('latencySamples', r'preset\.latencySamples\s*=\s*([\d.]+)f?'),
            ('qualityScore', r'preset\.qualityScore\s*=\s*([\d.]+)f?'),
            ('complexity', r'preset\.complexity\s*=\s*([\d.]+)f?'),
            ('experimentalness', r'preset\.experimentalness\s*=\s*([\d.]+)f?'),
            ('versatility', r'preset\.versatility\s*=\s*([\d.]+)f?'),
            ('optimalTempo', r'preset\.optimalTempo\s*=\s*([\d.]+)f?'),
        ]
        
        for field, pattern in numeric_fields:
            match = re.search(pattern, function_text)
            if match:
                preset[field] = self.parse_float(match.group(1))
        
        # Extract CPU tier
        tier_match = re.search(r'preset\.cpuTier\s*=\s*(?:CPUTier::)?(\w+)', function_text)
        if tier_match:
            preset['cpuTier'] = tier_match.group(1)
        
        # Extract boolean fields
        bool_match = re.search(r'preset\.realtimeSafe\s*=\s*(true|false)', function_text)
        if bool_match:
            preset['realtimeSafe'] = bool_match.group(1) == 'true'
        
        # Extract engines
        engines = []
        for i in range(6):
            # Find engine type
            type_pattern = f'preset\.engineTypes\\[{i}\\]\\s*=\\s*(-?\\d+|ENGINE_\\w+)'
            type_match = re.search(type_pattern, function_text)
            
            if type_match:
                engine_type = type_match.group(1)
                if engine_type.startswith('ENGINE_'):
                    # Map ENGINE_ constants to numbers (simplified - would need full mapping)
                    engine_type = str(i * 10)  # Placeholder
                else:
                    engine_type = int(engine_type)
                
                if engine_type >= 0:
                    # Find mix level
                    mix_pattern = f'preset\.engineMix\\[{i}\\]\\s*=\\s*([\d.]+)f?'
                    mix_match = re.search(mix_pattern, function_text)
                    
                    # Find active state
                    active_pattern = f'preset\.engineActive\\[{i}\\]\\s*=\\s*(true|false)'
                    active_match = re.search(active_pattern, function_text)
                    
                    # Find parameters
                    params_pattern = f'preset\.engineParams\\[{i}\\]\\s*=\\s*{{([^}}]+)}}'
                    params_match = re.search(params_pattern, function_text)
                    
                    if mix_match and active_match:
                        engine = {
                            'slot': i,
                            'type': int(engine_type) if isinstance(engine_type, str) else engine_type,
                            'mix': self.parse_float(mix_match.group(1)),
                            'active': active_match.group(1) == 'true',
                            'params': self.parse_engine_params(params_match.group(1)) if params_match else []
                        }
                        engines.append(engine)
        
        preset['engines'] = engines
        
        # Extract profiles
        profiles = {
            'sonicProfile': ['brightness', 'density', 'movement', 'space', 'aggression', 'vintage'],
            'emotionalProfile': ['energy', 'mood', 'tension', 'organic', 'nostalgia'],
            'sourceAffinity': ['vocals', 'guitar', 'drums', 'synth', 'mix']
        }
        
        for profile_name, fields in profiles.items():
            profile = {}
            for field in fields:
                pattern = f'preset\\.{profile_name}\\.{field}\\s*=\\s*([\d.]+)f?'
                match = re.search(pattern, function_text)
                if match:
                    profile[field] = self.parse_float(match.group(1))
                else:
                    profile[field] = 0.5  # Default
            preset[profile_name] = profile
        
        # Extract arrays
        keywords_match = re.search(r'preset\.keywords\s*=\s*{([^}]+)}', function_text)
        if keywords_match:
            preset['keywords'] = self.parse_string_array(keywords_match.group(1))
        
        prompts_match = re.search(r'preset\.userPrompts\s*=\s*{([^}]+)}', function_text)
        if prompts_match:
            preset['userPrompts'] = self.parse_string_array(prompts_match.group(1))
        
        genres_match = re.search(r'preset\.genres\s*=\s*{([^}]+)}', function_text)
        if genres_match:
            preset['genres'] = self.parse_string_array(genres_match.group(1))
        
        # Set defaults
        preset.setdefault('version', 1)
        preset.setdefault('keywords', [])
        preset.setdefault('userPrompts', [])
        preset.setdefault('genres', [])
        
        return preset
    
    def parse_cpp_file(self, filepath: str) -> List[Dict[str, Any]]:
        """Parse all presets from C++ file"""
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Find all preset creation functions
        # Pattern to match function definitions
        function_pattern = r'GoldenPreset\s+createPreset_\d+_\w+\(\)\s*{[^}]+?return\s+preset;\s*}'
        
        functions = re.findall(function_pattern, content, re.DOTALL)
        
        logger.info(f"Found {len(functions)} preset functions")
        
        presets = []
        for func in functions:
            try:
                preset = self.parse_preset_function(func)
                if 'id' in preset:
                    presets.append(preset)
                    logger.info(f"Parsed: {preset['id']} - {preset.get('name', 'Unknown')}")
            except Exception as e:
                logger.error(f"Error parsing preset: {e}")
        
        return presets
    
    def save_presets_to_json(self, presets: List[Dict[str, Any]], output_dir: str):
        """Save presets as individual JSON files"""
        output_path = Path(output_dir)
        output_path.mkdir(parents=True, exist_ok=True)
        
        # Create presets subdirectory
        presets_dir = output_path / "presets"
        presets_dir.mkdir(exist_ok=True)
        
        # Save each preset
        for preset in presets:
            preset_id = preset.get('id', 'unknown')
            filepath = presets_dir / f"{preset_id}.json"
            
            with open(filepath, 'w') as f:
                json.dump(preset, f, indent=2)
            
            logger.info(f"Saved: {filepath}")
        
        # Save metadata
        metadata = {
            "version": "1.0",
            "preset_count": len(presets),
            "categories": list(set(p.get('category', 'Unknown') for p in presets))
        }
        
        with open(output_path / "corpus_metadata.json", 'w') as f:
            json.dump(metadata, f, indent=2)
        
        logger.info(f"Export complete: {len(presets)} presets saved to {output_path}")


def main():
    parser = PresetParser()
    
    # Path to C++ source file
    cpp_file = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/GoldenCorpusPresets.cpp"
    
    # Output directory
    output_dir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus"
    
    # Parse presets
    logger.info(f"Parsing presets from: {cpp_file}")
    presets = parser.parse_cpp_file(cpp_file)
    
    # Save to JSON
    parser.save_presets_to_json(presets, output_dir)
    
    logger.info("Ready to build FAISS index!")


if __name__ == "__main__":
    main()