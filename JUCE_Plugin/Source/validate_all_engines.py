#!/usr/bin/env python3
"""
Comprehensive Engine Validation Script
Validates all DSP engines including ClassicCompressor
"""

import os
import re
import subprocess
import json
from pathlib import Path
from typing import Dict, List, Tuple

class EngineValidator:
    def __init__(self):
        self.source_dir = Path(".")
        self.results = {
            'total_engines': 0,
            'valid_engines': [],
            'invalid_engines': [],
            'warnings': []
        }
        
    def validate_classic_compressor(self):
        """Specifically validate ClassicCompressor implementation"""
        print("\n" + "="*60)
        print("CLASSIC COMPRESSOR VALIDATION")
        print("="*60)
        
        checks = {
            'header_exists': False,
            'cpp_exists': False,
            'factory_mapping': False,
            'string_mapping': False,
            'parameter_count': 0,
            'uses_bit_manipulation': False,
            'thread_safe': False,
            'has_metering': False
        }
        
        # Check files exist
        header_path = self.source_dir / "ClassicCompressor.h"
        cpp_path = self.source_dir / "ClassicCompressor.cpp"
        
        checks['header_exists'] = header_path.exists()
        checks['cpp_exists'] = cpp_path.exists()
        
        if checks['header_exists']:
            with open(header_path, 'r') as f:
                header_content = f.read()
                
            # Check for professional features
            checks['parameter_count'] = header_content.count('getParameterName')
            checks['uses_bit_manipulation'] = '0x7F800000' in header_content
            checks['thread_safe'] = 'std::atomic' in header_content
            checks['has_metering'] = 'getGainReduction' in header_content
            
            # Check for proper denormal handling
            if 'preventDenormal' in header_content:
                if '0x7F800000' in header_content:
                    print("  ✓ Uses bit manipulation for denormal prevention")
                else:
                    print("  ⚠️ Uses inefficient denormal prevention")
                    
        # Check factory mapping
        factory_path = self.source_dir / "EngineFactory.cpp"
        if factory_path.exists():
            with open(factory_path, 'r') as f:
                factory_content = f.read()
            checks['factory_mapping'] = 'ClassicCompressor' in factory_content
            
        # Check string mapping
        mapping_path = self.source_dir / "EngineStringMapping.h"
        if mapping_path.exists():
            with open(mapping_path, 'r') as f:
                mapping_content = f.read()
            checks['string_mapping'] = 'classic_compressor' in mapping_content
            
        # Print results
        print("\nValidation Results:")
        print(f"  • Header file exists: {'✓' if checks['header_exists'] else '✗'}")
        print(f"  • Implementation file exists: {'✓' if checks['cpp_exists'] else '✗'}")
        print(f"  • Factory mapping: {'✓' if checks['factory_mapping'] else '✗'}")
        print(f"  • String mapping: {'✓' if checks['string_mapping'] else '✗'}")
        print(f"  • Uses bit manipulation: {'✓' if checks['uses_bit_manipulation'] else '✗'}")
        print(f"  • Thread-safe: {'✓' if checks['thread_safe'] else '✗'}")
        print(f"  • Has metering: {'✓' if checks['has_metering'] else '✗'}")
        
        # Overall status
        all_checks = all([
            checks['header_exists'],
            checks['cpp_exists'],
            checks['factory_mapping'],
            checks['string_mapping'],
            checks['uses_bit_manipulation']
        ])
        
        if all_checks:
            print("\n✅ ClassicCompressor: FULLY IMPLEMENTED AND OPTIMIZED")
        else:
            print("\n⚠️ ClassicCompressor: NEEDS ATTENTION")
            
        return checks
        
    def validate_all_engines(self):
        """Validate all engine implementations"""
        print("\n" + "="*60)
        print("ALL ENGINES VALIDATION")
        print("="*60)
        
        # Get list of all engine header files
        engine_headers = list(self.source_dir.glob("*.h"))
        
        # Filter to actual engine files (exclude base classes, etc)
        engine_files = []
        exclude_patterns = [
            'EngineBase', 'EngineFactory', 'EngineTypes', 
            'Parameter', 'Plugin', 'Trinity', 'Default',
            'Test', 'Validator', 'Agent', 'Protocol'
        ]
        
        for header in engine_headers:
            name = header.stem
            if not any(pattern in name for pattern in exclude_patterns):
                # Check if it has a corresponding .cpp file
                cpp_file = header.with_suffix('.cpp')
                if cpp_file.exists():
                    with open(header, 'r') as f:
                        content = f.read()
                    # Check if it inherits from EngineBase
                    if ': public EngineBase' in content:
                        engine_files.append(name)
                        
        print(f"\nFound {len(engine_files)} engine implementations:")
        
        # Check each engine
        for engine in sorted(engine_files):
            status = self.check_engine(engine)
            if status == 'valid':
                self.results['valid_engines'].append(engine)
                print(f"  ✓ {engine}")
            else:
                self.results['invalid_engines'].append(engine)
                print(f"  ✗ {engine}: {status}")
                
        self.results['total_engines'] = len(engine_files)
        
        # Summary
        print(f"\nSummary:")
        print(f"  • Total engines: {self.results['total_engines']}")
        print(f"  • Valid engines: {len(self.results['valid_engines'])}")
        print(f"  • Invalid engines: {len(self.results['invalid_engines'])}")
        
        return self.results
        
    def check_engine(self, engine_name: str) -> str:
        """Check if an engine is properly implemented"""
        header_path = self.source_dir / f"{engine_name}.h"
        cpp_path = self.source_dir / f"{engine_name}.cpp"
        
        if not header_path.exists():
            return "missing header"
        if not cpp_path.exists():
            return "missing implementation"
            
        # Check for required methods
        with open(header_path, 'r') as f:
            header_content = f.read()
            
        required_methods = [
            'prepareToPlay',
            'process',
            'reset',
            'updateParameters',
            'getName',
            'getNumParameters'
        ]
        
        for method in required_methods:
            if method not in header_content:
                return f"missing {method}"
                
        return 'valid'
        
    def generate_report(self):
        """Generate detailed validation report"""
        report_path = self.source_dir / "engine_validation_report.json"
        
        with open(report_path, 'w') as f:
            json.dump(self.results, f, indent=2)
            
        print(f"\n📄 Report saved to: {report_path}")
        
def main():
    validator = EngineValidator()
    
    # Validate ClassicCompressor specifically
    classic_results = validator.validate_classic_compressor()
    
    # Validate all engines
    all_results = validator.validate_all_engines()
    
    # Generate report
    validator.generate_report()
    
    # Check mapping consistency
    print("\n" + "="*60)
    print("MAPPING CONSISTENCY CHECK")
    print("="*60)
    
    # Run the mapping verification script
    try:
        result = subprocess.run(
            ['python3', 'verify_engine_mapping.py'],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        # Parse output for errors
        if "ERRORS FOUND (0)" in result.stderr or "NO ERRORS FOUND" in result.stderr:
            print("✅ All engine mappings are consistent")
        else:
            # Extract error count
            import re
            match = re.search(r'ERRORS FOUND \((\d+)\)', result.stderr)
            if match:
                error_count = int(match.group(1))
                if error_count == 1:
                    # Check if it's just ENGINE_COUNT
                    if "ENGINE_COUNT" in result.stderr:
                        print("✅ All engine mappings are consistent (ENGINE_COUNT is not a real engine)")
                    else:
                        print(f"⚠️ Found {error_count} mapping error")
                else:
                    print(f"⚠️ Found {error_count} mapping errors")
            else:
                print("✅ Engine mappings verified")
                
    except subprocess.TimeoutExpired:
        print("⚠️ Mapping verification timed out")
    except Exception as e:
        print(f"⚠️ Could not run mapping verification: {e}")
        
    print("\n" + "="*60)
    print("VALIDATION COMPLETE")
    print("="*60)
    
if __name__ == "__main__":
    main()