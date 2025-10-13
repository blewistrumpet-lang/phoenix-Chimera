#!/usr/bin/env python3
"""
Extract complete engine and parameter details from GeneratedParameterDatabase.h
"""

import re
import json

def extract_engine_details():
    # Read the file
    with open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source/GeneratedParameterDatabase.h", "r") as f:
        content = f.read()
    
    # Extract all parameter arrays
    param_arrays = {}
    
    # Find all parameter array definitions
    # Pattern: static constexpr ParameterInfo NAME[] = { ... };
    pattern = r'static constexpr ParameterInfo (\w+)\[\] = \{([^;]+)\};'
    matches = re.findall(pattern, content, re.DOTALL)
    
    for array_name, array_content in matches:
        params = []
        # Parse each parameter line
        param_lines = array_content.strip().split('\n')
        for line in param_lines:
            line = line.strip()
            if line.startswith('{'):
                # Extract parameter details
                # Format: {"Name", default, min, max, "description", "units", skew}
                param_match = re.match(
                    r'\{"([^"]+)",\s*([\d.]+)f,\s*([\d.]+)f,\s*([\d.]+)f,\s*"([^"]*)",\s*"([^"]*)"(?:,\s*([\d.]+)f)?\}',
                    line
                )
                if param_match:
                    params.append({
                        "name": param_match.group(1),
                        "default": float(param_match.group(2)),
                        "min": float(param_match.group(3)),
                        "max": float(param_match.group(4)),
                        "description": param_match.group(5),
                        "units": param_match.group(6),
                        "skew": float(param_match.group(7)) if param_match.group(7) else 0.5
                    })
        
        if params:
            param_arrays[array_name] = params
    
    # Now extract the engine database entries
    engines = {}
    
    # Pattern for engine database entries
    engine_pattern = r'\{"([^"]+)",\s*"([^"]+)",\s*(\d+),\s*"([^"]+)",\s*\d+,\s*"([^"]+)",\s*(\d+),\s*(\w+)\}'
    engine_matches = re.findall(engine_pattern, content)
    
    for match in engine_matches:
        string_id = match[0]
        display_name = match[1]
        legacy_id = int(match[2])
        enum_name = match[3]
        category = match[4]
        param_count = int(match[5])
        param_array_name = match[6]
        
        # Get the parameter details
        params = param_arrays.get(param_array_name, [])
        
        engines[legacy_id] = {
            "id": legacy_id,
            "name": display_name,
            "string_id": string_id,
            "enum": enum_name,
            "category": category,
            "param_count": param_count,
            "parameters": params[:param_count]  # Use only the specified number of params
        }
    
    # Add special engines that might not be in the database
    if 0 not in engines:
        engines[0] = {
            "id": 0,
            "name": "None",
            "category": "Special",
            "param_count": 0,
            "parameters": []
        }
    
    # Fill in missing engines with defaults from SimplifiedEngineMapping if needed
    for i in range(57):
        if i not in engines:
            engines[i] = {
                "id": i,
                "name": f"Engine {i}",
                "category": "Unknown",
                "param_count": 10,  # Default
                "parameters": [
                    {
                        "name": f"Param {j+1}",
                        "default": 0.5,
                        "min": 0.0,
                        "max": 1.0,
                        "description": f"Parameter {j+1}",
                        "units": "percent"
                    }
                    for j in range(10)
                ]
            }
    
    return engines

def main():
    engines = extract_engine_details()
    
    # Save to JSON file
    output = {
        "engines": engines,
        "total_engines": len(engines),
        "slot_count": 6,
        "format_requirements": {
            "slots": "0-indexed (0-5)",
            "parameters": "Objects with 'name' and 'value' properties",
            "parameter_names": "Use param1, param2, etc.",
            "response_structure": "Wrapped in data.preset"
        }
    }
    
    with open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/complete_engine_specs.json", "w") as f:
        json.dump(output, f, indent=2)
    
    print(f"Extracted {len(engines)} engines with detailed parameter information")
    print(f"Saved to complete_engine_specs.json")
    
    # Print summary
    print("\nEngine Summary:")
    for engine_id in sorted(engines.keys())[:10]:
        engine = engines[engine_id]
        print(f"  {engine_id}: {engine['name']} ({engine['param_count']} params)")
        if engine['parameters']:
            for i, param in enumerate(engine['parameters'][:3]):
                print(f"    param{i+1}: {param['name']} - {param['description']}")
            if len(engine['parameters']) > 3:
                print(f"    ... and {len(engine['parameters'])-3} more")

if __name__ == "__main__":
    main()