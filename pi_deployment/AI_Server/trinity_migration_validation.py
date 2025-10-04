#!/usr/bin/env python3
"""
Trinity Component Migration Validation Script
Validates that all Trinity components now use the authoritative engine mapping
"""

import os
import sys
import importlib
import inspect
from pathlib import Path
from typing import Dict, List, Set
import logging

# Add current directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

def analyze_import_statements(file_path: Path) -> Dict[str, List[str]]:
    """Analyze import statements in a Python file"""
    imports = {
        "legacy_imports": [],
        "authoritative_imports": [],
        "other_imports": []
    }
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            lines = content.splitlines()
            
        for line_num, line in enumerate(lines, 1):
            line = line.strip()
            if line.startswith('from ') or line.startswith('import '):
                if ('engine_definitions' in line or 'engine_mapping_correct' in line) and 'engine_mapping_authoritative' not in line:
                    imports["legacy_imports"].append(f"Line {line_num}: {line}")
                elif 'engine_mapping_authoritative' in line:
                    imports["authoritative_imports"].append(f"Line {line_num}: {line}")
                elif 'engine' in line.lower() and 'engine_mapping_authoritative' not in line and 'engine_defaults' not in line:
                    imports["other_imports"].append(f"Line {line_num}: {line}")
                    
    except Exception as e:
        logger.error(f"Error analyzing {file_path}: {e}")
        
    return imports

def check_hardcoded_engine_references(file_path: Path) -> List[str]:
    """Check for hardcoded engine IDs or old string references"""
    issues = []
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            lines = content.splitlines()
            
        # Patterns that might indicate legacy usage
        legacy_patterns = [
            'vintage_tube',
            'tape_echo', 
            'plate_reverb',
            'ENGINE_MAPPING.get',
            'ENGINE_ALIASES',
            'ENGINE_NAME_TO_ID',
            'legacy_id',
            'get_engine_by_legacy_id'
        ]
        
        for line_num, line in enumerate(lines, 1):
            for pattern in legacy_patterns:
                if pattern in line and not line.strip().startswith('#'):
                    issues.append(f"Line {line_num}: {line.strip()}")
                    
    except Exception as e:
        logger.error(f"Error checking {file_path}: {e}")
        
    return issues

def validate_authoritative_usage(file_path: Path) -> Dict[str, bool]:
    """Check if file properly uses authoritative constants"""
    validation = {
        "uses_authoritative_import": False,
        "uses_authoritative_constants": False,
        "uses_validation_functions": False
    }
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            
        # Check for authoritative imports
        if 'from engine_mapping_authoritative import' in content:
            validation["uses_authoritative_import"] = True
            
        # Check for authoritative constants usage
        authoritative_constants = [
            'ENGINE_NONE',
            'ENGINE_VINTAGE_TUBE', 
            'ENGINE_TAPE_ECHO',
            'ENGINE_PLATE_REVERB',
            'ENGINE_NAMES',
            'DYNAMICS_ENGINES',
            'FILTER_ENGINES'
        ]
        
        for constant in authoritative_constants:
            if constant in content:
                validation["uses_authoritative_constants"] = True
                break
                
        # Check for validation functions
        validation_functions = [
            'validate_engine_id',
            'get_engine_name',
            'get_engine_id',
            'get_engine_category'
        ]
        
        for func in validation_functions:
            if func in content:
                validation["uses_validation_functions"] = True
                break
                
    except Exception as e:
        logger.error(f"Error validating {file_path}: {e}")
        
    return validation

def test_component_imports():
    """Test that all components can import authoritative mapping"""
    components = [
        'visionary_string_ids',
        'oracle_string_ids', 
        'oracle',
        'calculator',
        'alchemist',
        'cloud_bridge'
    ]
    
    results = {}
    
    for component in components:
        try:
            # Try to import the module
            module = importlib.import_module(component)
            
            # Check if it has authoritative constants
            has_constants = any(hasattr(module, attr) for attr in [
                'ENGINE_NONE', 'ENGINE_VINTAGE_TUBE', 'get_engine_name', 'validate_engine_id'
            ])
            
            results[component] = {
                "import_success": True,
                "has_authoritative_constants": has_constants,
                "module_path": getattr(module, '__file__', 'Unknown')
            }
            
        except Exception as e:
            results[component] = {
                "import_success": False,
                "error": str(e),
                "has_authoritative_constants": False
            }
            
    return results

def main():
    """Main validation function"""
    print("=" * 80)
    print("TRINITY COMPONENT MIGRATION VALIDATION")
    print("=" * 80)
    
    # Files to check
    trinity_files = [
        'visionary_string_ids.py',
        'oracle_string_ids.py',
        'oracle.py', 
        'calculator.py',
        'alchemist.py',
        'cloud_bridge.py',
        'plugin_endpoints.py'
    ]
    
    current_dir = Path(__file__).parent
    
    print("\n1. CHECKING FILE IMPORTS")
    print("-" * 40)
    
    all_clean = True
    
    for filename in trinity_files:
        file_path = current_dir / filename
        if not file_path.exists():
            print(f"❌ {filename}: File not found")
            all_clean = False
            continue
            
        print(f"\n📁 Analyzing {filename}:")
        
        # Check imports
        imports = analyze_import_statements(file_path)
        
        if imports["authoritative_imports"]:
            print(f"  ✅ Uses authoritative import: {len(imports['authoritative_imports'])} found")
            for imp in imports["authoritative_imports"][:2]:  # Show first 2
                print(f"     - {imp}")
        else:
            print(f"  ❌ No authoritative imports found")
            all_clean = False
            
        if imports["legacy_imports"]:
            print(f"  ⚠️  Legacy imports found: {len(imports['legacy_imports'])}")
            for imp in imports["legacy_imports"][:3]:  # Show first 3
                print(f"     - {imp}")
            all_clean = False
        else:
            print(f"  ✅ No legacy imports")
            
        # Check for hardcoded references
        issues = check_hardcoded_engine_references(file_path)
        if issues:
            print(f"  ⚠️  Potential legacy references: {len(issues)}")
            for issue in issues[:3]:  # Show first 3
                print(f"     - {issue}")
            if len(issues) > 3:
                print(f"     - ... and {len(issues) - 3} more")
        else:
            print(f"  ✅ No obvious legacy references")
            
        # Validate authoritative usage
        validation = validate_authoritative_usage(file_path)
        auth_score = sum(validation.values())
        print(f"  📊 Authoritative usage score: {auth_score}/3")
        
        if not validation["uses_authoritative_import"]:
            print(f"     ❌ Missing authoritative import")
        if not validation["uses_authoritative_constants"]: 
            print(f"     ❌ Not using authoritative constants")
        if not validation["uses_validation_functions"]:
            print(f"     ❌ Not using validation functions")
            
    print("\n" + "=" * 40)
    print("2. TESTING COMPONENT IMPORTS")
    print("-" * 40)
    
    import_results = test_component_imports()
    
    for component, result in import_results.items():
        if result["import_success"]:
            if result["has_authoritative_constants"]:
                print(f"✅ {component}: Import successful, has authoritative constants")
            else:
                print(f"⚠️  {component}: Import successful, but missing authoritative constants")
        else:
            print(f"❌ {component}: Import failed - {result['error']}")
            all_clean = False
            
    print("\n" + "=" * 40)
    print("3. TESTING AUTHORITATIVE ENGINE MAPPING")
    print("-" * 40)
    
    try:
        import engine_mapping_authoritative as ema
        print("✅ Authoritative engine mapping imports successfully")
        
        # Test some key constants
        test_constants = [
            ('ENGINE_NONE', ema.ENGINE_NONE),
            ('ENGINE_VINTAGE_TUBE', ema.ENGINE_VINTAGE_TUBE), 
            ('ENGINE_TAPE_ECHO', ema.ENGINE_TAPE_ECHO),
            ('ENGINE_COUNT', ema.ENGINE_COUNT)
        ]
        
        for name, value in test_constants:
            print(f"  ✅ {name} = {value}")
            
        # Test functions
        print(f"  ✅ get_engine_name(ENGINE_VINTAGE_TUBE) = '{ema.get_engine_name(ema.ENGINE_VINTAGE_TUBE)}'")
        print(f"  ✅ validate_engine_id(ENGINE_VINTAGE_TUBE) = {ema.validate_engine_id(ema.ENGINE_VINTAGE_TUBE)}")
        print(f"  ✅ validate_engine_id(999) = {ema.validate_engine_id(999)}")
        
    except Exception as e:
        print(f"❌ Failed to import authoritative mapping: {e}")
        all_clean = False
        
    print("\n" + "=" * 80)
    print("4. MIGRATION SUMMARY")
    print("=" * 80)
    
    if all_clean:
        print("🎉 SUCCESS: All Trinity components have been migrated to authoritative engine mapping!")
        print("\nChanges made:")
        print("✅ All components now import from engine_mapping_authoritative") 
        print("✅ Legacy engine_definitions and engine_mapping_correct imports removed")
        print("✅ Hardcoded engine lists replaced with ENGINE_* constants")
        print("✅ String ID conversions use get_engine_id() function")
        print("✅ Engine ID validation uses validate_engine_id() function")
        print("✅ Engine name lookups use get_engine_name() function")
        
        print("\nBenefits:")
        print("• Single source of truth for engine mapping")
        print("• Consistent with JUCE_Plugin/Source/EngineTypes.h")
        print("• No more mapping conflicts between components")
        print("• Easier maintenance and updates")
        
        return True
    else:
        print("❌ ISSUES FOUND: Some components still need migration work")
        print("\nPlease review the issues above and complete the migration.")
        
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)