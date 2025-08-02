#!/usr/bin/env python3
"""
Parameter Audit Script for Project Chimera v3.0 Phoenix
Compares actual engine parameter counts with metadata definitions
"""

import re
import os

def extract_parameter_count_from_header(header_path):
    """Extract the parameter count from an engine header file"""
    try:
        with open(header_path, 'r') as f:
            content = f.read()
            
        # Look for getNumParameters() return statement
        match = re.search(r'getNumParameters\(\)\s*const\s*override\s*{\s*return\s+(\d+);', content)
        if match:
            return int(match.group(1))
        return None
    except FileNotFoundError:
        return None

def extract_parameter_count_from_metadata(metadata_path, engine_name):
    """Extract parameter count for a specific engine from metadata file"""
    try:
        with open(metadata_path, 'r') as f:
            content = f.read()
            
        # Find the engine section
        engine_pattern = rf'ENGINE_{engine_name.upper().replace(" ", "_").replace("-", "_")}'
        
        # Find the start of this engine's definition
        start_match = re.search(rf'{engine_pattern}.*?"([^"]*)"', content)
        if not start_match:
            return None, None
            
        engine_display_name = start_match.group(1)
        start_pos = start_match.start()
        
        # Find the end of this engine's definition (next .build() call)
        build_pattern = r'\.build\(\)\s*\);'
        build_matches = list(re.finditer(build_pattern, content[start_pos:]))
        if not build_matches:
            return None, engine_display_name
            
        end_pos = start_pos + build_matches[0].end()
        engine_section = content[start_pos:end_pos]
        
        # Count .addParameter calls
        param_matches = re.findall(r'\.addParameter\(', engine_section)
        return len(param_matches), engine_display_name
        
    except FileNotFoundError:
        return None, None

def main():
    source_dir = "Source"
    metadata_file = "Source/CompleteEngineMetadata.cpp"
    
    # Get all engine header files
    engine_files = []
    for filename in os.listdir(source_dir):
        if filename.endswith('.h') and filename not in ['EngineBase.h', 'EngineQualityTest.h', 'BypassEngine.h']:
            engine_files.append(filename)
    
    print("Engine Parameter Count Audit")
    print("=" * 50)
    print(f"{'Engine Name':<25} {'Header':<8} {'Metadata':<10} {'Status':<10}")
    print("-" * 50)
    
    mismatches = []
    
    for engine_file in sorted(engine_files):
        engine_name = engine_file.replace('.h', '')
        header_path = os.path.join(source_dir, engine_file)
        
        # Get actual parameter count
        header_count = extract_parameter_count_from_header(header_path)
        
        # Get metadata parameter count  
        metadata_count, display_name = extract_parameter_count_from_metadata(metadata_file, engine_name)
        
        if header_count is None:
            status = "NO_HEADER"
        elif metadata_count is None:
            status = "NO_METADATA"
        elif header_count != metadata_count:
            status = "MISMATCH"
            mismatches.append({
                'engine': engine_name,
                'display_name': display_name or engine_name,
                'header_count': header_count,
                'metadata_count': metadata_count
            })
        else:
            status = "OK"
        
        display_name_short = (display_name or engine_name)[:24]
        header_str = str(header_count) if header_count is not None else "N/A"
        metadata_str = str(metadata_count) if metadata_count is not None else "N/A"
        
        print(f"{display_name_short:<25} {header_str:<8} {metadata_str:<10} {status:<10}")
    
    print("\nMismatches Found:")
    print("=" * 50)
    for mismatch in mismatches:
        print(f"• {mismatch['display_name']}")
        print(f"  Header: {mismatch['header_count']} parameters")
        print(f"  Metadata: {mismatch['metadata_count']} parameters")
        print(f"  Difference: {mismatch['metadata_count'] - mismatch['header_count']:+d}")
        print()

if __name__ == "__main__":
    main()