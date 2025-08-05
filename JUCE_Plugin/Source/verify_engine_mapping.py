#!/usr/bin/env python3
"""
Engine Mapping Verification Script
Ensures all DSP engines are properly mapped across the entire system.
"""

import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple
from collections import defaultdict
import json

class EngineMapValidator:
    def __init__(self, source_dir: str = "."):
        self.source_dir = Path(source_dir)
        self.engines = {}
        self.errors = []
        self.warnings = []
        
    def parse_engine_types(self) -> Dict[str, int]:
        """Parse EngineTypes.h to get all engine definitions"""
        engines = {}
        legacy_mappings = {}
        
        engine_types_path = self.source_dir / "EngineTypes.h"
        if not engine_types_path.exists():
            self.errors.append(f"EngineTypes.h not found at {engine_types_path}")
            return engines
            
        with open(engine_types_path, 'r') as f:
            content = f.read()
            
        # Parse engine definitions
        pattern = r'#define\s+(ENGINE_\w+)\s+(\d+)'
        for match in re.finditer(pattern, content):
            name = match.group(1)
            value = int(match.group(2))
            engines[name] = value
            
        # Parse legacy mappings
        pattern = r'#define\s+(ENGINE_\w+)\s+(ENGINE_\w+)'
        for match in re.finditer(pattern, content):
            old_name = match.group(1)
            new_name = match.group(2)
            if new_name in engines:
                legacy_mappings[old_name] = engines[new_name]
                
        # Merge legacy mappings
        engines.update(legacy_mappings)
        
        print(f"✓ Found {len(engines)} engine definitions in EngineTypes.h")
        return engines
        
    def parse_engine_factory(self) -> Dict[int, str]:
        """Parse EngineFactory.cpp to get engine implementations"""
        implementations = {}
        
        factory_path = self.source_dir / "EngineFactory.cpp"
        if not factory_path.exists():
            self.errors.append(f"EngineFactory.cpp not found at {factory_path}")
            return implementations
            
        with open(factory_path, 'r') as f:
            content = f.read()
            
        # Find all include statements to get class names
        includes = set()
        pattern = r'#include\s+"(\w+)\.h"'
        for match in re.finditer(pattern, content):
            includes.add(match.group(1))
            
        # Parse switch cases (handle namespaced classes)
        pattern = r'case\s+(ENGINE_\w+):\s*\n\s*return\s+std::make_unique<(?:(\w+)::)?(\w+)>'
        for match in re.finditer(pattern, content):
            engine_const = match.group(1)
            namespace = match.group(2) if match.group(2) else ""
            class_name = match.group(3)
            
            if engine_const in self.engines:
                engine_id = self.engines[engine_const]
                implementations[engine_id] = class_name
                
        print(f"✓ Found {len(implementations)} engine implementations in EngineFactory.cpp")
        return implementations
        
    def parse_string_mappings(self) -> Tuple[Dict[str, int], Dict[int, str]]:
        """Parse EngineStringMapping.h for string-to-engine mappings"""
        str_to_engine = {}
        engine_to_str = {}
        dropdown_indices = {}
        
        mapping_path = self.source_dir / "EngineStringMapping.h"
        if not mapping_path.exists():
            self.errors.append(f"EngineStringMapping.h not found at {mapping_path}")
            return str_to_engine, engine_to_str
            
        with open(mapping_path, 'r') as f:
            content = f.read()
            
        # Parse string to engine mappings
        pattern = r'\{"([^"]+)",\s*(ENGINE_\w+)\}'
        for match in re.finditer(pattern, content):
            str_id = match.group(1)
            engine_const = match.group(2)
            if engine_const in self.engines:
                str_to_engine[str_id] = self.engines[engine_const]
                
        # Parse engine to string mappings
        pattern = r'\{(ENGINE_\w+),\s*"([^"]+)"\}'
        for match in re.finditer(pattern, content):
            engine_const = match.group(1)
            str_id = match.group(2)
            if engine_const in self.engines:
                engine_to_str[self.engines[engine_const]] = str_id
                
        # Parse dropdown indices
        pattern = r'\{"([^"]+)",\s*(\d+)\}'
        for match in re.finditer(pattern, content):
            str_id = match.group(1)
            index = int(match.group(2))
            dropdown_indices[str_id] = index
                
        print(f"✓ Found {len(str_to_engine)} string-to-engine mappings")
        print(f"✓ Found {len(engine_to_str)} engine-to-string mappings")
        print(f"✓ Found {len(dropdown_indices)} dropdown index mappings")
        
        return str_to_engine, engine_to_str
        
    def check_header_files(self) -> Dict[str, bool]:
        """Check if all engine header files exist"""
        header_status = {}
        
        for engine_id, class_name in self.implementations.items():
            header_path = self.source_dir / f"{class_name}.h"
            cpp_path = self.source_dir / f"{class_name}.cpp"
            
            header_exists = header_path.exists()
            cpp_exists = cpp_path.exists()
            
            header_status[class_name] = {
                'header': header_exists,
                'cpp': cpp_exists,
                'engine_id': engine_id
            }
            
            if not header_exists:
                self.errors.append(f"Missing header: {class_name}.h for engine ID {engine_id}")
            if not cpp_exists:
                self.errors.append(f"Missing implementation: {class_name}.cpp for engine ID {engine_id}")
                
        return header_status
        
    def validate_mappings(self):
        """Main validation function"""
        print("\n" + "="*60)
        print("ENGINE MAPPING VALIDATION REPORT")
        print("="*60)
        
        # Parse all files
        self.engines = self.parse_engine_types()
        self.implementations = self.parse_engine_factory()
        self.str_to_engine, self.engine_to_str = self.parse_string_mappings()
        header_status = self.check_header_files()
        
        print(f"\n📊 SUMMARY:")
        print(f"  • Total engine constants: {len(self.engines)}")
        print(f"  • Total implementations: {len(self.implementations)}")
        print(f"  • Total string mappings: {len(self.str_to_engine)}")
        
        # Check for missing implementations
        print(f"\n🔍 CHECKING FOR ISSUES:")
        
        for engine_name, engine_id in self.engines.items():
            if engine_id == 0:  # Skip BYPASS
                continue
                
            # Skip commented out engines
            if engine_name in ['ENGINE_ANALOG_CHORUS', 'ENGINE_DIGITAL_PHASER', 'ENGINE_TUBE_SCREAMER']:
                continue
                
            # Check implementation
            if engine_id not in self.implementations:
                self.errors.append(f"No implementation for {engine_name} (ID: {engine_id})")
                
            # Check string mapping
            if engine_id not in self.engine_to_str:
                self.warnings.append(f"No string mapping for {engine_name} (ID: {engine_id})")
                
        # Check for orphaned implementations
        for engine_id, class_name in self.implementations.items():
            engine_name = None
            for name, id_val in self.engines.items():
                if id_val == engine_id:
                    engine_name = name
                    break
                    
            if not engine_name:
                self.warnings.append(f"Implementation {class_name} has no engine constant (ID: {engine_id})")
                
        # Report results
        if self.errors:
            print(f"\n❌ ERRORS FOUND ({len(self.errors)}):")
            for error in self.errors:
                print(f"  • {error}")
        else:
            print(f"\n✅ NO ERRORS FOUND!")
            
        if self.warnings:
            print(f"\n⚠️  WARNINGS ({len(self.warnings)}):")
            for warning in self.warnings:
                print(f"  • {warning}")
                
        # Print detailed mapping for ClassicCompressor
        print(f"\n🔧 CLASSIC COMPRESSOR MAPPING:")
        vca_id = self.engines.get('ENGINE_VCA_COMPRESSOR')
        if vca_id:
            print(f"  • Engine ID: {vca_id}")
            print(f"  • Implementation: {self.implementations.get(vca_id, 'NOT FOUND')}")
            print(f"  • String ID: {self.engine_to_str.get(vca_id, 'NOT FOUND')}")
            
            # Check if files exist
            if vca_id in self.implementations:
                class_name = self.implementations[vca_id]
                if class_name in header_status:
                    status = header_status[class_name]
                    print(f"  • Header exists: {'✓' if status['header'] else '✗'}")
                    print(f"  • CPP exists: {'✓' if status['cpp'] else '✗'}")
                    
        # Generate mapping report
        self.generate_report()
        
    def generate_report(self):
        """Generate a detailed mapping report"""
        report_path = self.source_dir / "engine_mapping_report.json"
        
        report = {
            'summary': {
                'total_engines': len(self.engines),
                'total_implementations': len(self.implementations),
                'total_string_mappings': len(self.str_to_engine),
                'errors': len(self.errors),
                'warnings': len(self.warnings)
            },
            'engines': {},
            'errors': self.errors,
            'warnings': self.warnings
        }
        
        # Build detailed engine info
        for engine_name, engine_id in self.engines.items():
            if engine_id == 0:  # Skip BYPASS
                continue
                
            implementation = self.implementations.get(engine_id, None)
            string_id = self.engine_to_str.get(engine_id, None)
            
            report['engines'][engine_name] = {
                'id': engine_id,
                'implementation': implementation,
                'string_id': string_id,
                'has_header': False,
                'has_cpp': False
            }
            
            if implementation:
                header_path = self.source_dir / f"{implementation}.h"
                cpp_path = self.source_dir / f"{implementation}.cpp"
                report['engines'][engine_name]['has_header'] = header_path.exists()
                report['engines'][engine_name]['has_cpp'] = cpp_path.exists()
                
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
            
        print(f"\n📄 Detailed report saved to: {report_path}")
        
    def fix_mappings(self):
        """Attempt to fix common mapping issues"""
        print(f"\n🔧 ATTEMPTING AUTO-FIX...")
        
        fixes_made = []
        
        # Find engines with implementation but no string mapping
        for engine_id, class_name in self.implementations.items():
            if engine_id not in self.engine_to_str:
                # Generate a snake_case string ID from class name
                string_id = re.sub(r'([A-Z])', r'_\1', class_name).lower().strip('_')
                fixes_made.append(f"Add string mapping: {string_id} -> {class_name}")
                
        if fixes_made:
            print(f"  Suggested fixes:")
            for fix in fixes_made:
                print(f"    • {fix}")
        else:
            print(f"  No automatic fixes available")
            
if __name__ == "__main__":
    # Run validation
    validator = EngineMapValidator()
    validator.validate_mappings()
    
    # Check for specific engine if provided
    if len(sys.argv) > 1:
        engine_name = sys.argv[1]
        print(f"\n🔍 Checking specific engine: {engine_name}")
        
        if f"ENGINE_{engine_name.upper()}" in validator.engines:
            engine_id = validator.engines[f"ENGINE_{engine_name.upper()}"]
            print(f"  • Found with ID: {engine_id}")
            
            if engine_id in validator.implementations:
                print(f"  • Implementation: {validator.implementations[engine_id]}")
            else:
                print(f"  • ❌ No implementation found!")
                
            if engine_id in validator.engine_to_str:
                print(f"  • String ID: {validator.engine_to_str[engine_id]}")
            else:
                print(f"  • ❌ No string mapping found!")
        else:
            print(f"  • ❌ Engine not found in EngineTypes.h")
            
    sys.exit(1 if validator.errors else 0)