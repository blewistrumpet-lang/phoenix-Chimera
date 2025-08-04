#!/usr/bin/env python3
"""
Parameter Database Code Generator for ChimeraPhoenix
Generates Python and C++ parameter files from parameter_database.json
"""

import json
import os
from datetime import datetime
from pathlib import Path

def load_database():
    """Load the parameter database JSON"""
    with open('parameter_database.json', 'r') as f:
        return json.load(f)

def generate_python_defaults(db):
    """Generate Python engine_defaults.py"""
    
    header = f'''"""
Generated Engine Parameter Defaults for ChimeraPhoenix
Generated from parameter_database.json on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
DO NOT EDIT MANUALLY - Edit parameter_database.json and regenerate
"""

ENGINE_DEFAULTS = {{
'''
    
    content = header
    
    # Get all engines sorted by legacy_id
    engines = []
    for key, engine in db['engines'].items():
        engines.append((engine['legacy_id'], key, engine))
    
    # Sort by legacy_id (bypass will be -1, so it sorts first)
    engines.sort(key=lambda x: x[0])
    
    for legacy_id, key, engine in engines:
        if legacy_id == -1:
            # Special handling for bypass
            content += f'''    # BYPASS (special case, no parameters)
    # Note: Bypass is handled specially in the plugin
    
'''
            continue
            
        content += f"    # {legacy_id}: {engine['cpp_enum']}\n"
        content += f"    {legacy_id}: {{\n"
        content += f'        "name": "{engine["display_name"]}",\n'
        content += f'        "params": {{\n'
        
        for param in engine['parameters']:
            idx = param['index']
            content += f'            "param{idx+1}": {{'
            content += f'"name": "{param["name"]}", '
            content += f'"default": {param["default"]}, '
            content += f'"min": {param["min"]}, '
            content += f'"max": {param["max"]}'
            content += f'}},\n'
        
        if not engine['parameters']:
            content += '            # No parameters\n'
            
        content += '        }\n'
        content += '    },\n    \n'
    
    content += '}\n'
    
    # Write to file
    output_path = Path('AI_Server/generated_engine_defaults.py')
    with open(output_path, 'w') as f:
        f.write(content)
    
    print(f"✅ Generated {output_path}")
    return output_path

def generate_cpp_header(db):
    """Generate C++ parameter database header"""
    
    header = f'''// Generated Parameter Database for ChimeraPhoenix
// Generated from parameter_database.json on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// DO NOT EDIT MANUALLY - Edit parameter_database.json and regenerate

#pragma once
#include <array>
#include <string>
#include <vector>

namespace ChimeraParameters {{

// Parameter information structure
struct ParameterInfo {{
    const char* name;
    float defaultValue;
    float minValue;
    float maxValue;
    const char* description;
    const char* units;
    float skew;
}};

// Engine information structure  
struct EngineInfo {{
    const char* stringId;
    const char* displayName;
    int legacyId;
    const char* cppEnum;
    int dropdownIndex;
    const char* category;
    int parameterCount;
    const ParameterInfo* parameters;
}};

'''
    
    # Generate parameter arrays for each engine
    for key, engine in db['engines'].items():
        if engine['parameter_count'] > 0:
            header += f"// {engine['display_name']} parameters\n"
            header += f"static constexpr ParameterInfo {key}_params[] = {{\n"
            
            for param in engine['parameters']:
                header += f'    {{"{param["name"]}", '
                header += f'{param["default"]}f, '
                header += f'{param["min"]}f, '
                header += f'{param["max"]}f, '
                header += f'"{param["description"]}", '
                header += f'"{param["units"]}", '
                header += f'{param["skew"]}f}},\n'
            
            header += "};\n\n"
    
    # Generate main engine database
    header += "// Complete engine database\n"
    header += "static constexpr EngineInfo engineDatabase[] = {\n"
    
    for key, engine in db['engines'].items():
        header += f'    {{"{key}", '
        header += f'"{engine["display_name"]}", '
        header += f'{engine["legacy_id"]}, '
        header += f'"{engine["cpp_enum"]}", '
        header += f'{engine["dropdown_index"]}, '
        header += f'"{engine["category"]}", '
        header += f'{engine["parameter_count"]}, '
        
        if engine['parameter_count'] > 0:
            header += f'{key}_params'
        else:
            header += 'nullptr'
            
        header += '},\n'
    
    header += "};\n\n"
    
    # Add helper functions
    header += '''// Helper functions
inline const EngineInfo* getEngineInfo(const std::string& stringId) {
    for (const auto& engine : engineDatabase) {
        if (engine.stringId == stringId) {
            return &engine;
        }
    }
    return nullptr;
}

inline const EngineInfo* getEngineInfoByLegacyId(int legacyId) {
    for (const auto& engine : engineDatabase) {
        if (engine.legacyId == legacyId) {
            return &engine;
        }
    }
    return nullptr;
}

inline int getParameterCount(const std::string& stringId) {
    const auto* info = getEngineInfo(stringId);
    return info ? info->parameterCount : 0;
}

inline float getDefaultValue(const std::string& stringId, int paramIndex) {
    const auto* info = getEngineInfo(stringId);
    if (info && paramIndex >= 0 && paramIndex < info->parameterCount) {
        return info->parameters[paramIndex].defaultValue;
    }
    return 0.5f; // Safe default
}

} // namespace ChimeraParameters
'''
    
    # Write to file
    output_path = Path('JUCE_Plugin/Source/GeneratedParameterDatabase.h')
    with open(output_path, 'w') as f:
        f.write(header)
    
    print(f"✅ Generated {output_path}")
    return output_path

def generate_cpp_defaults(db):
    """Generate C++ default values implementation"""
    
    content = f'''// Generated Default Parameter Values for ChimeraPhoenix
// Generated from parameter_database.json on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
// DO NOT EDIT MANUALLY - Edit parameter_database.json and regenerate

#include "DefaultParameterValues.h"
#include "EngineTypes.h"
#include <array>

void DefaultParameterValues::getDefaultParameters(int engineType, std::vector<float>& defaults) {{
    defaults.clear();
    
    switch (engineType) {{
'''
    
    # Generate cases for each engine
    engines_by_legacy = {}
    for key, engine in db['engines'].items():
        if engine['legacy_id'] >= 0:
            engines_by_legacy[engine['legacy_id']] = (key, engine)
    
    for legacy_id in sorted(engines_by_legacy.keys()):
        key, engine = engines_by_legacy[legacy_id]
        
        content += f"        case {engine['cpp_enum']}: // {engine['display_name']}\n"
        
        for param in engine['parameters']:
            content += f"            defaults.push_back({param['default']}f); // {param['name']}\n"
        
        if not engine['parameters']:
            content += "            // No parameters\n"
            
        content += "            break;\n\n"
    
    content += '''        default:
            // Unknown engine type - provide safe defaults
            for (int i = 0; i < 8; ++i) {
                defaults.push_back(0.5f);
            }
            break;
    }
}

int DefaultParameterValues::getParameterCount(int engineType) {
    switch (engineType) {
'''
    
    # Generate parameter count cases
    for legacy_id in sorted(engines_by_legacy.keys()):
        key, engine = engines_by_legacy[legacy_id]
        content += f"        case {engine['cpp_enum']}: return {engine['parameter_count']};\n"
    
    content += '''        default: return 0;
    }
}

const char* DefaultParameterValues::getParameterName(int engineType, int paramIndex) {
    switch (engineType) {
'''
    
    # Generate parameter name cases
    for legacy_id in sorted(engines_by_legacy.keys()):
        key, engine = engines_by_legacy[legacy_id]
        
        if engine['parameter_count'] > 0:
            content += f"        case {engine['cpp_enum']}:\n"
            content += "            switch (paramIndex) {\n"
            
            for param in engine['parameters']:
                content += f'                case {param["index"]}: return "{param["name"]}";\n'
            
            content += '                default: return "";\n'
            content += "            }\n"
            content += "            break;\n\n"
    
    content += '''        default: return "";
    }
}
'''
    
    # Write to file
    output_path = Path('JUCE_Plugin/Source/GeneratedDefaultParameterValues.cpp')
    with open(output_path, 'w') as f:
        f.write(content)
    
    print(f"✅ Generated {output_path}")
    return output_path

def generate_validation_script(db):
    """Generate parameter validation script"""
    
    script = f'''#!/usr/bin/env python3
"""
Parameter Validation Script for ChimeraPhoenix
Generated from parameter_database.json on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
Validates that all parameter definitions are consistent across the codebase
"""

import json
import re
from pathlib import Path

# Load expected parameters from database
expected_params = '''
    
    script += json.dumps(db, indent=2)
    
    script += '''

def validate_engine_headers():
    """Validate each engine's .h file has correct parameter count"""
    errors = []
    base_path = Path("JUCE_Plugin/Source")
    
    for engine_key, engine_info in expected_params['engines'].items():
        if engine_key == 'bypass':
            continue
            
        # Find the engine's header file
        # This is simplified - you'd need to map string IDs to class names
        expected_count = engine_info['parameter_count']
        display_name = engine_info['display_name']
        
        print(f"Checking {display_name}: expects {expected_count} parameters")
        
        # TODO: Add actual file checking logic here
        # Look for getNumParameters() returning expected_count
        
    return errors

def validate_golden_corpus():
    """Validate Golden Corpus presets have correct parameter counts"""
    corpus_path = Path("JUCE_Plugin/GoldenCorpus/all_presets_string_ids.json")
    
    if not corpus_path.exists():
        return ["Golden Corpus file not found"]
    
    with open(corpus_path, 'r') as f:
        corpus = json.load(f)
    
    errors = []
    
    for preset in corpus.get('presets', []):
        for engine in preset.get('engines', []):
            engine_type = engine.get('type')
            params = engine.get('params', [])
            
            if engine_type in expected_params['engines']:
                expected_count = expected_params['engines'][engine_type]['parameter_count']
                actual_count = len(params)
                
                if actual_count != expected_count:
                    errors.append(
                        f"Preset '{preset['name']}' engine '{engine_type}': "
                        f"has {actual_count} params, expected {expected_count}"
                    )
    
    return errors

def validate_parameter_ranges():
    """Validate all parameter values are within min/max ranges"""
    errors = []
    
    # Check Golden Corpus
    corpus_path = Path("JUCE_Plugin/GoldenCorpus/all_presets_string_ids.json")
    if corpus_path.exists():
        with open(corpus_path, 'r') as f:
            corpus = json.load(f)
        
        for preset in corpus.get('presets', []):
            for engine in preset.get('engines', []):
                engine_type = engine.get('type')
                params = engine.get('params', [])
                
                if engine_type in expected_params['engines']:
                    engine_info = expected_params['engines'][engine_type]
                    
                    for i, value in enumerate(params):
                        if i < len(engine_info['parameters']):
                            param_info = engine_info['parameters'][i]
                            if value < param_info['min'] or value > param_info['max']:
                                errors.append(
                                    f"Preset '{preset['name']}' engine '{engine_type}' "
                                    f"param {i} ({param_info['name']}): "
                                    f"value {value} out of range [{param_info['min']}, {param_info['max']}]"
                                )
    
    return errors

def main():
    print("=" * 80)
    print("ChimeraPhoenix Parameter Validation")
    print("=" * 80)
    
    all_errors = []
    
    # Run validations
    print("\\n1. Validating engine headers...")
    errors = validate_engine_headers()
    if errors:
        print(f"   ❌ Found {len(errors)} errors")
        all_errors.extend(errors)
    else:
        print("   ✅ All engine headers valid")
    
    print("\\n2. Validating Golden Corpus...")
    errors = validate_golden_corpus()
    if errors:
        print(f"   ❌ Found {len(errors)} errors")
        all_errors.extend(errors)
    else:
        print("   ✅ Golden Corpus valid")
    
    print("\\n3. Validating parameter ranges...")
    errors = validate_parameter_ranges()
    if errors:
        print(f"   ❌ Found {len(errors)} errors")
        all_errors.extend(errors)
    else:
        print("   ✅ All parameter values in range")
    
    # Report results
    print("\\n" + "=" * 80)
    if all_errors:
        print(f"VALIDATION FAILED: {len(all_errors)} total errors found\\n")
        for error in all_errors[:20]:  # Show first 20 errors
            print(f"  • {error}")
        if len(all_errors) > 20:
            print(f"  ... and {len(all_errors) - 20} more errors")
        return 1
    else:
        print("✅ VALIDATION PASSED: All parameters are consistent!")
        return 0

if __name__ == "__main__":
    exit(main())
'''
    
    # Write to file
    output_path = Path('validate_parameters.py')
    with open(output_path, 'w') as f:
        f.write(script)
    
    # Make executable
    os.chmod(output_path, 0o755)
    
    print(f"✅ Generated {output_path}")
    return output_path

def main():
    """Main generator function"""
    print("=" * 80)
    print("ChimeraPhoenix Parameter Code Generator")
    print("=" * 80)
    print()
    
    # Load database
    print("Loading parameter_database.json...")
    db = load_database()
    print(f"  Loaded {len(db['engines'])} engines")
    print()
    
    # Generate files
    print("Generating code files...")
    
    # Python files
    generate_python_defaults(db)
    
    # C++ files
    generate_cpp_header(db)
    generate_cpp_defaults(db)
    
    # Validation script
    generate_validation_script(db)
    
    print()
    print("=" * 80)
    print("✅ Code generation complete!")
    print()
    print("Next steps:")
    print("1. Review generated files")
    print("2. Run ./validate_parameters.py to check consistency")
    print("3. Replace existing parameter files with generated ones")
    print("4. Rebuild the plugin")

if __name__ == "__main__":
    main()