#!/usr/bin/env python3
"""
Script to help fix SmoothParam issues across all engine files
"""

import os
import re

def find_files_needing_smoothparam():
    """Find all .h files that might need SmoothParam"""
    source_dir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source"
    files_to_check = []
    
    # Engines that we know need SmoothParam added
    engines_missing_smoothparam = [
        "RotarySpeaker",
        "HarmonicTremolo", 
        "DetuneDoubler",
        "CombResonator",
        "PhasedVocoder",
        "SpectralFreeze",
        "StereoChorus"  # Has wrong implementation
    ]
    
    for engine in engines_missing_smoothparam:
        header = os.path.join(source_dir, f"{engine}.h")
        if os.path.exists(header):
            files_to_check.append(header)
            
    return files_to_check

def create_smoothparam_fix(header_file):
    """Generate the changes needed for a header file"""
    
    # Read the file
    with open(header_file, 'r') as f:
        content = f.read()
    
    # Find all float member variables that look like parameters
    param_pattern = r'float\s+(m_\w+)\s*=\s*[\d.]+f?\s*;'
    params = re.findall(param_pattern, content)
    
    # Filter to likely parameter names
    param_names = []
    for param in params:
        # Skip things that don't look like user parameters
        if any(skip in param.lower() for skip in ['samplerate', 'buffer', 'state', 'prev', 'last']):
            continue
        param_names.append(param)
    
    print(f"\n{os.path.basename(header_file)}:")
    print(f"  Found parameters: {param_names}")
    
    # Generate the fix
    print("\n  1. Add after includes:")
    print('    #include "SmoothParam.h"')
    
    print("\n  2. Replace float declarations with:")
    for param in param_names:
        print(f"    SmoothParam {param};")
    
    print("\n  3. In the .cpp file, update all usages to use .current:")
    for param in param_names:
        print(f"    - Replace '{param}' with '{param}.current' in calculations")
        print(f"    - Add '{param}.update();' in process loop")
    
    print("\n  4. In prepareToPlay(), add:")
    for param in param_names:
        print(f"    {param}.setSmoothingTime(20.0f, sampleRate);")

if __name__ == "__main__":
    files = find_files_needing_smoothparam()
    print(f"Files that need SmoothParam fixes: {len(files)}")
    
    for file in files:
        create_smoothparam_fix(file)