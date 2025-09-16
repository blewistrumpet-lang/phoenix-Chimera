#!/usr/bin/env python3
"""
Golden Corpus Architectural Analysis Tool
Analyzes the corpus for various architectural inconsistencies
"""

import json
import os
import glob
from typing import Dict, List, Any, Tuple

class CorpusAnalyzer:
    def __init__(self, corpus_path: str, manifest_path: str):
        self.corpus_path = corpus_path
        self.manifest_path = manifest_path
        self.presets: List[Dict] = []
        self.issues: List[Dict] = []
        self.engine_manifest: Dict = {}
        
        # Expected structure based on parameter manifest
        self.expected_engines = 57  # 0-56
        self.expected_slots = 6
        self.expected_params_per_slot = 15
        
        self.load_manifest()
        self.load_presets()
    
    def load_manifest(self):
        """Load the parameter manifest to understand engine mappings"""
        try:
            with open(self.manifest_path, 'r') as f:
                self.engine_manifest = json.load(f)
            print(f"✓ Loaded parameter manifest with {len(self.engine_manifest.get('engine_specific_limits', {}))} engines")
        except Exception as e:
            print(f"✗ Error loading manifest: {e}")
            self.engine_manifest = {}
    
    def load_presets(self):
        """Load all preset files"""
        preset_files = glob.glob(os.path.join(self.corpus_path, "presets", "GC_*.json"))
        
        for filepath in sorted(preset_files):
            try:
                with open(filepath, 'r') as f:
                    preset = json.load(f)
                    preset['_filepath'] = filepath
                    self.presets.append(preset)
            except Exception as e:
                self.issues.append({
                    'type': 'file_error',
                    'severity': 'critical',
                    'file': filepath,
                    'issue': f"Failed to load JSON: {e}"
                })
        
        print(f"✓ Loaded {len(self.presets)} presets")
    
    def analyze_engine_numbering(self):
        """Check for engine numbering issues (should be 0-56)"""
        print("\n🔍 Analyzing engine numbering...")
        
        for preset in self.presets:
            preset_id = preset.get('id', 'unknown')
            engines = preset.get('engines', [])
            
            for i, engine in enumerate(engines):
                engine_type = engine.get('type')
                slot = engine.get('slot')
                
                # Check if engine type is in valid range
                if engine_type is not None:
                    if engine_type < 0 or engine_type > 56:
                        self.issues.append({
                            'type': 'invalid_engine_number',
                            'severity': 'high',
                            'preset': preset_id,
                            'slot': slot,
                            'engine_type': engine_type,
                            'issue': f"Engine type {engine_type} outside valid range 0-56"
                        })
                
                # Check if slot numbers make sense
                if slot is not None:
                    if slot < 0 or slot >= self.expected_slots:
                        self.issues.append({
                            'type': 'invalid_slot_number',
                            'severity': 'high',
                            'preset': preset_id,
                            'slot': slot,
                            'issue': f"Slot {slot} outside valid range 0-{self.expected_slots-1}"
                        })
    
    def analyze_parameter_format(self):
        """Check parameter format consistency"""
        print("🔍 Analyzing parameter format...")
        
        for preset in self.presets:
            preset_id = preset.get('id', 'unknown')
            engines = preset.get('engines', [])
            
            # Check if we have the right structure - old vs new format
            if engines:
                first_engine = engines[0]
                has_params_array = 'params' in first_engine
                has_individual_params = any(key.startswith('param') for key in first_engine.keys())
                
                if has_params_array and len(engines) <= 3:
                    # This looks like old format
                    self.issues.append({
                        'type': 'old_format_detected',
                        'severity': 'medium',
                        'preset': preset_id,
                        'issue': f"Using old format with params arrays and {len(engines)} engines"
                    })
                
                # Check parameter counts and ranges
                for engine in engines:
                    slot = engine.get('slot', -1)
                    params = engine.get('params', [])
                    
                    # Check parameter count
                    if len(params) != 7 and has_params_array:  # Old format had 7 params
                        self.issues.append({
                            'type': 'incorrect_param_count',
                            'severity': 'medium',
                            'preset': preset_id,
                            'slot': slot,
                            'param_count': len(params),
                            'issue': f"Expected 7 params in old format, got {len(params)}"
                        })
                    
                    # Check parameter values are in 0-1 range
                    for param_idx, param_val in enumerate(params):
                        if not isinstance(param_val, (int, float)):
                            self.issues.append({
                                'type': 'invalid_param_type',
                                'severity': 'high',
                                'preset': preset_id,
                                'slot': slot,
                                'param_index': param_idx,
                                'param_value': param_val,
                                'issue': f"Parameter {param_idx} is not numeric: {type(param_val)}"
                            })
                        elif param_val < 0.0 or param_val > 1.0:
                            self.issues.append({
                                'type': 'param_out_of_range',
                                'severity': 'medium',
                                'preset': preset_id,
                                'slot': slot,
                                'param_index': param_idx,
                                'param_value': param_val,
                                'issue': f"Parameter {param_idx} value {param_val} outside 0-1 range"
                            })
    
    def analyze_mix_parameter(self):
        """Check mix parameter issues"""
        print("🔍 Analyzing mix parameters...")
        
        for preset in self.presets:
            preset_id = preset.get('id', 'unknown')
            engines = preset.get('engines', [])
            
            for engine in engines:
                slot = engine.get('slot', -1)
                mix = engine.get('mix')
                
                if mix is None:
                    self.issues.append({
                        'type': 'missing_mix_param',
                        'severity': 'high',
                        'preset': preset_id,
                        'slot': slot,
                        'issue': "Mix parameter is missing"
                    })
                elif not isinstance(mix, (int, float)):
                    self.issues.append({
                        'type': 'invalid_mix_type',
                        'severity': 'high',
                        'preset': preset_id,
                        'slot': slot,
                        'mix_value': mix,
                        'issue': f"Mix parameter is not numeric: {type(mix)}"
                    })
                elif mix < 0.0 or mix > 1.0:
                    self.issues.append({
                        'type': 'mix_out_of_range',
                        'severity': 'medium',
                        'preset': preset_id,
                        'slot': slot,
                        'mix_value': mix,
                        'issue': f"Mix value {mix} outside 0-1 range"
                    })
    
    def analyze_engine_mapping(self):
        """Check engine IDs against engine mapping"""
        print("🔍 Analyzing engine mapping consistency...")
        
        manifest_engines = self.engine_manifest.get('engine_specific_limits', {})
        
        for preset in self.presets:
            preset_id = preset.get('id', 'unknown')
            engines = preset.get('engines', [])
            
            for engine in engines:
                engine_type = engine.get('type')
                type_name = engine.get('typeName', '')
                slot = engine.get('slot', -1)
                
                if engine_type is not None and str(engine_type) in manifest_engines:
                    expected_name = manifest_engines[str(engine_type)].get('name', '')
                    
                    # Check if typeName matches the manifest
                    if expected_name and type_name != expected_name:
                        self.issues.append({
                            'type': 'engine_name_mismatch',
                            'severity': 'medium',
                            'preset': preset_id,
                            'slot': slot,
                            'engine_type': engine_type,
                            'actual_name': type_name,
                            'expected_name': expected_name,
                            'issue': f"Engine {engine_type} name mismatch: '{type_name}' != '{expected_name}'"
                        })
    
    def analyze_creative_names(self):
        """Check for missing or invalid creative_name fields"""
        print("🔍 Analyzing creative names...")
        
        for preset in self.presets:
            preset_id = preset.get('id', 'unknown')
            name = preset.get('name')
            
            if not name:
                self.issues.append({
                    'type': 'missing_creative_name',
                    'severity': 'high',
                    'preset': preset_id,
                    'issue': "Missing 'name' field"
                })
            elif not isinstance(name, str) or len(name.strip()) == 0:
                self.issues.append({
                    'type': 'invalid_creative_name',
                    'severity': 'medium',
                    'preset': preset_id,
                    'name': name,
                    'issue': f"Invalid creative name: '{name}'"
                })
    
    def analyze_slot_structure(self):
        """Check for missing or incorrect slot structure"""
        print("🔍 Analyzing slot structure...")
        
        for preset in self.presets:
            preset_id = preset.get('id', 'unknown')
            engines = preset.get('engines', [])
            
            # Check for duplicate slots
            used_slots = []
            for engine in engines:
                slot = engine.get('slot')
                if slot in used_slots:
                    self.issues.append({
                        'type': 'duplicate_slot',
                        'severity': 'high',
                        'preset': preset_id,
                        'slot': slot,
                        'issue': f"Slot {slot} used multiple times"
                    })
                used_slots.append(slot)
            
            # Check for missing required fields in engines
            for engine in engines:
                slot = engine.get('slot', -1)
                required_fields = ['slot', 'type', 'mix']
                
                for field in required_fields:
                    if field not in engine:
                        self.issues.append({
                            'type': 'missing_engine_field',
                            'severity': 'high',
                            'preset': preset_id,
                            'slot': slot,
                            'field': field,
                            'issue': f"Missing required field '{field}' in engine"
                        })
    
    def analyze_new_vs_old_format(self):
        """Determine if presets are using new vs old format"""
        print("🔍 Analyzing format consistency...")
        
        old_format_count = 0
        new_format_count = 0
        
        for preset in self.presets:
            preset_id = preset.get('id', 'unknown')
            engines = preset.get('engines', [])
            
            if not engines:
                continue
            
            # Check characteristics of format
            max_engines = len(engines)
            has_params_array = any('params' in engine for engine in engines)
            max_slot = max((engine.get('slot', 0) for engine in engines), default=0)
            
            if max_engines <= 3 and has_params_array and max_slot <= 2:
                old_format_count += 1
                self.issues.append({
                    'type': 'old_format_preset',
                    'severity': 'info',
                    'preset': preset_id,
                    'engine_count': max_engines,
                    'max_slot': max_slot,
                    'issue': f"Preset uses old format (max {max_engines} engines, slot {max_slot})"
                })
            else:
                new_format_count += 1
        
        print(f"  - Old format presets: {old_format_count}")
        print(f"  - New format presets: {new_format_count}")
    
    def generate_report(self):
        """Generate comprehensive analysis report"""
        print("\n" + "="*80)
        print("GOLDEN CORPUS ARCHITECTURAL ANALYSIS REPORT")
        print("="*80)
        
        print(f"\n📊 OVERVIEW:")
        print(f"  - Total presets analyzed: {len(self.presets)}")
        print(f"  - Total issues found: {len(self.issues)}")
        print(f"  - Expected engine range: 0-{self.expected_engines-1}")
        print(f"  - Expected slots per preset: {self.expected_slots}")
        print(f"  - Expected params per slot: {self.expected_params_per_slot}")
        
        # Group issues by type and severity
        issues_by_type = {}
        issues_by_severity = {'critical': 0, 'high': 0, 'medium': 0, 'low': 0, 'info': 0}
        
        for issue in self.issues:
            issue_type = issue.get('type', 'unknown')
            severity = issue.get('severity', 'unknown')
            
            if issue_type not in issues_by_type:
                issues_by_type[issue_type] = []
            issues_by_type[issue_type].append(issue)
            
            if severity in issues_by_severity:
                issues_by_severity[severity] += 1
        
        print(f"\n🚨 ISSUES BY SEVERITY:")
        for severity, count in issues_by_severity.items():
            if count > 0:
                print(f"  - {severity.upper()}: {count}")
        
        print(f"\n📋 ISSUES BY TYPE:")
        for issue_type, issue_list in sorted(issues_by_type.items()):
            print(f"\n  {issue_type.upper()} ({len(issue_list)} issues):")
            
            # Show first few examples
            for issue in issue_list[:3]:
                preset = issue.get('preset', 'unknown')
                slot = issue.get('slot', '')
                slot_info = f" slot {slot}" if slot != '' else ""
                print(f"    - {preset}{slot_info}: {issue.get('issue', 'No description')}")
            
            if len(issue_list) > 3:
                print(f"    ... and {len(issue_list) - 3} more")
        
        # Specific recommendations
        print(f"\n💡 RECOMMENDATIONS:")
        
        if any(issue['type'] == 'old_format_preset' for issue in self.issues):
            print("  1. Convert old format presets to new 57-engine system")
            print("     - Expand to 6 slots with 15 parameters each")
            print("     - Update engine numbering to 0-56 range")
            print("     - Add bypass, mix, solo parameters per slot")
        
        if any(issue['type'] == 'invalid_engine_number' for issue in self.issues):
            print("  2. Fix invalid engine numbers")
            print("     - All engine types must be in range 0-56")
            print("     - Engine ID should equal dropdown index")
        
        if any(issue['type'] == 'param_out_of_range' for issue in self.issues):
            print("  3. Normalize parameter values")
            print("     - All parameters should be in range 0.0-1.0")
            print("     - Mix parameters should be in range 0.0-1.0")
        
        if any(issue['type'] == 'engine_name_mismatch' for issue in self.issues):
            print("  4. Update engine names to match manifest")
            print("     - Ensure typeName field matches parameter manifest")
        
        # Summary of format analysis
        old_format = sum(1 for issue in self.issues if issue['type'] == 'old_format_preset')
        new_format = len(self.presets) - old_format
        
        print(f"\n🔄 FORMAT STATUS:")
        print(f"  - Using correct 57-engine format: {new_format} presets")
        print(f"  - Using old format: {old_format} presets")
        
        if old_format > 0:
            print(f"  ⚠️  {old_format} presets need format conversion")
        else:
            print(f"  ✅ All presets using correct format")
        
        return {
            'total_presets': len(self.presets),
            'total_issues': len(self.issues),
            'issues_by_type': issues_by_type,
            'issues_by_severity': issues_by_severity,
            'old_format_count': old_format,
            'new_format_count': new_format
        }
    
    def run_analysis(self):
        """Run all analysis checks"""
        print("Starting Golden Corpus architectural analysis...")
        
        self.analyze_engine_numbering()
        self.analyze_parameter_format()
        self.analyze_mix_parameter()
        self.analyze_engine_mapping()
        self.analyze_creative_names()
        self.analyze_slot_structure()
        self.analyze_new_vs_old_format()
        
        return self.generate_report()


def main():
    corpus_path = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus"
    manifest_path = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/parameter_manifest.json"
    
    analyzer = CorpusAnalyzer(corpus_path, manifest_path)
    results = analyzer.run_analysis()
    
    # Save detailed results
    output_file = os.path.join(corpus_path, "analysis_results.json")
    with open(output_file, 'w') as f:
        json.dump({
            'summary': results,
            'detailed_issues': analyzer.issues
        }, f, indent=2)
    
    print(f"\n📄 Detailed results saved to: {output_file}")

if __name__ == "__main__":
    main()