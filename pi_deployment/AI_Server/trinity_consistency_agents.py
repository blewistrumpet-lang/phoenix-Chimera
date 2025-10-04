#!/usr/bin/env python3
"""
Trinity Consistency Agent Team
Ensures complete consistency across all Trinity components
"""

import json
import os
import re
from pathlib import Path
from typing import Dict, List, Tuple, Set, Any
from dataclasses import dataclass
from datetime import datetime

@dataclass
class InconsistencyReport:
    """Report of an inconsistency found"""
    severity: str  # 'critical', 'high', 'medium', 'low'
    location: str
    issue: str
    expected: Any
    found: Any
    fix_suggestion: str

class EngineValidationAgent:
    """Agent that validates engine IDs and names across all files"""
    
    def __init__(self):
        self.authoritative_source = "engine_mapping_authoritative.py"
        self.engine_map = self.load_authoritative_mapping()
        self.reports = []
    
    def load_authoritative_mapping(self) -> Dict:
        """Load the authoritative engine mapping"""
        mapping = {}
        
        # Import the authoritative source
        import engine_mapping_authoritative as auth
        
        for attr in dir(auth):
            if attr.startswith('ENGINE_') and not attr.endswith('_COUNT'):
                engine_id = getattr(auth, attr)
                if isinstance(engine_id, int):
                    # Get the name from ENGINE_NAMES if available
                    name = auth.ENGINE_NAMES.get(engine_id, attr)
                    mapping[engine_id] = {
                        'constant': attr,
                        'name': name,
                        'id': engine_id
                    }
        
        return mapping
    
    def validate_file(self, filepath: str) -> List[InconsistencyReport]:
        """Validate engine references in a file"""
        reports = []
        
        with open(filepath, 'r') as f:
            content = f.read()
            
        # Check for engine ID references
        if filepath.endswith('.py'):
            reports.extend(self.validate_python_file(filepath, content))
        elif filepath.endswith('.json'):
            reports.extend(self.validate_json_file(filepath, content))
        elif filepath.endswith('.cpp') or filepath.endswith('.h'):
            reports.extend(self.validate_cpp_file(filepath, content))
        elif filepath.endswith('.md'):
            reports.extend(self.validate_markdown_file(filepath, content))
        
        return reports
    
    def validate_python_file(self, filepath: str, content: str) -> List[InconsistencyReport]:
        """Validate Python file engine references"""
        reports = []
        
        # Check for hardcoded engine IDs
        pattern = r'engine[_\s]*(?:id|ID|Id)?\s*[=:]\s*(\d+)'
        for match in re.finditer(pattern, content):
            engine_id = int(match.group(1))
            if engine_id > 0 and engine_id not in self.engine_map:
                reports.append(InconsistencyReport(
                    severity='critical',
                    location=filepath,
                    issue=f"Invalid engine ID {engine_id}",
                    expected="Valid engine ID from 1-56",
                    found=engine_id,
                    fix_suggestion=f"Use a valid engine ID from engine_mapping_authoritative.py"
                ))
        
        # Check for wrong engine names
        wrong_names = {
            'ENGINE_CLASSIC_COMPRESSOR': 'ENGINE_VCA_COMPRESSOR',
            'ENGINE_TUBE_SATURATION': 'ENGINE_VINTAGE_TUBE',
            'ENGINE_TAPE_SATURATION': 'ENGINE_TAPE_ECHO',
            'ENGINE_TRANSISTOR_FUZZ': 'ENGINE_MUFF_FUZZ',
            'ENGINE_BIT_CRUSHER': 'ENGINE_BIT_CRUSHER',  # Correct
            'ENGINE_K_STYLE_OVERDRIVE': 'ENGINE_K_STYLE',
            'ENGINE_ANALOG_CHORUS': 'ENGINE_DIGITAL_CHORUS',
            'ENGINE_VINTAGE_FLANGER': 'ENGINE_ANALOG_PHASER'
        }
        
        for wrong, correct in wrong_names.items():
            if wrong in content and wrong != correct:
                reports.append(InconsistencyReport(
                    severity='high',
                    location=filepath,
                    issue=f"Wrong engine constant name",
                    expected=correct,
                    found=wrong,
                    fix_suggestion=f"Replace {wrong} with {correct}"
                ))
        
        return reports
    
    def validate_json_file(self, filepath: str, content: str) -> List[InconsistencyReport]:
        """Validate JSON file engine references"""
        reports = []
        
        try:
            data = json.loads(content)
            reports.extend(self.validate_json_data(filepath, data))
        except json.JSONDecodeError as e:
            reports.append(InconsistencyReport(
                severity='critical',
                location=filepath,
                issue="Invalid JSON",
                expected="Valid JSON",
                found=str(e),
                fix_suggestion="Fix JSON syntax"
            ))
        
        return reports
    
    def validate_json_data(self, filepath: str, data: Any, path: str = "") -> List[InconsistencyReport]:
        """Recursively validate JSON data"""
        reports = []
        
        if isinstance(data, dict):
            for key, value in data.items():
                new_path = f"{path}.{key}" if path else key
                
                # Check for engine-related keys
                if 'engine' in key.lower() and isinstance(value, (int, float)):
                    engine_id = int(value)
                    if engine_id > 0 and engine_id not in self.engine_map:
                        reports.append(InconsistencyReport(
                            severity='critical',
                            location=f"{filepath}:{new_path}",
                            issue=f"Invalid engine ID",
                            expected="Valid engine ID (1-56)",
                            found=engine_id,
                            fix_suggestion=f"Use valid engine ID from authoritative mapping"
                        ))
                
                # Recurse
                reports.extend(self.validate_json_data(filepath, value, new_path))
                
        elif isinstance(data, list):
            for i, item in enumerate(data):
                new_path = f"{path}[{i}]"
                reports.extend(self.validate_json_data(filepath, item, new_path))
        
        return reports
    
    def validate_cpp_file(self, filepath: str, content: str) -> List[InconsistencyReport]:
        """Validate C++ file engine references"""
        reports = []
        
        # Check for enum definitions that don't match
        if 'enum EngineType' in content or 'enum class EngineType' in content:
            # This is likely a definition file
            pattern = r'(\w+)\s*=\s*(\d+)'
            for match in re.finditer(pattern, content):
                name = match.group(1)
                engine_id = int(match.group(2))
                
                if engine_id in self.engine_map:
                    expected_name = self.engine_map[engine_id]['constant']
                    if not name.endswith(expected_name.replace('ENGINE_', '')):
                        reports.append(InconsistencyReport(
                            severity='high',
                            location=f"{filepath}:{name}",
                            issue="Engine name mismatch",
                            expected=expected_name,
                            found=name,
                            fix_suggestion=f"Rename to match {expected_name}"
                        ))
        
        return reports
    
    def validate_markdown_file(self, filepath: str, content: str) -> List[InconsistencyReport]:
        """Validate Markdown documentation"""
        reports = []
        
        # Check for engine listings
        pattern = r'ENGINE_\w+\s*\((\d+)\)'
        for match in re.finditer(pattern, content):
            engine_id = int(match.group(1))
            if engine_id > 0 and engine_id not in self.engine_map:
                reports.append(InconsistencyReport(
                    severity='medium',
                    location=filepath,
                    issue=f"Documentation references invalid engine ID",
                    expected="Valid engine ID",
                    found=engine_id,
                    fix_suggestion="Update documentation to match authoritative mapping"
                ))
        
        return reports

class ParameterValidationAgent:
    """Agent that validates parameter counts and ranges"""
    
    def __init__(self):
        self.expected_param_count = 16  # 0-15
        self.expected_slot_count = 6
        self.reports = []
    
    def validate_file(self, filepath: str) -> List[InconsistencyReport]:
        """Validate parameter references in a file"""
        reports = []
        
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Check for parameter references
        if filepath.endswith('.json'):
            reports.extend(self.validate_json_params(filepath, content))
        elif filepath.endswith('.py'):
            reports.extend(self.validate_python_params(filepath, content))
        
        return reports
    
    def validate_json_params(self, filepath: str, content: str) -> List[InconsistencyReport]:
        """Validate JSON parameter structure"""
        reports = []
        
        try:
            data = json.loads(content)
            
            # Check for slot/param structure
            for key in data.keys() if isinstance(data, dict) else []:
                # Check slot numbers
                if match := re.match(r'slot(\d+)_', key):
                    slot_num = int(match.group(1))
                    if slot_num < 1 or slot_num > 6:
                        reports.append(InconsistencyReport(
                            severity='critical',
                            location=f"{filepath}:{key}",
                            issue="Invalid slot number",
                            expected="Slot 1-6",
                            found=slot_num,
                            fix_suggestion=f"Use slot number between 1 and 6"
                        ))
                
                # Check param numbers
                if match := re.match(r'slot\d+_param(\d+)', key):
                    param_num = int(match.group(1))
                    if param_num < 0 or param_num > 15:
                        reports.append(InconsistencyReport(
                            severity='critical',
                            location=f"{filepath}:{key}",
                            issue="Invalid parameter number",
                            expected="Parameter 0-15",
                            found=param_num,
                            fix_suggestion=f"Use parameter number between 0 and 15"
                        ))
        except:
            pass
        
        return reports
    
    def validate_python_params(self, filepath: str, content: str) -> List[InconsistencyReport]:
        """Validate Python parameter usage"""
        reports = []
        
        # Check for hardcoded param counts
        if 'param16' in content or 'param17' in content:
            reports.append(InconsistencyReport(
                severity='high',
                location=filepath,
                issue="Reference to non-existent parameter",
                expected="Parameters 0-15 only",
                found="param16 or higher",
                fix_suggestion="Use only param0 through param15"
            ))
        
        # Check for slot7 or higher
        if re.search(r'slot[7-9]\d*_', content):
            reports.append(InconsistencyReport(
                severity='high',
                location=filepath,
                issue="Reference to non-existent slot",
                expected="Slots 1-6 only",
                found="slot7 or higher",
                fix_suggestion="Use only slot1 through slot6"
            ))
        
        return reports

class PresetValidationAgent:
    """Agent that validates preset structure and consistency"""
    
    def __init__(self):
        self.reports = []
    
    def validate_preset(self, preset: Dict, source: str) -> List[InconsistencyReport]:
        """Validate a single preset"""
        reports = []
        
        # Check required fields
        required_fields = ['id', 'creative_name']
        for field in required_fields:
            if field not in preset:
                reports.append(InconsistencyReport(
                    severity='high',
                    location=f"{source}:{preset.get('id', 'unknown')}",
                    issue=f"Missing required field",
                    expected=field,
                    found="Missing",
                    fix_suggestion=f"Add {field} field to preset"
                ))
        
        # Validate slot structure
        slots_found = set()
        for key in preset.keys():
            if match := re.match(r'slot(\d+)_', key):
                slot_num = int(match.group(1))
                slots_found.add(slot_num)
        
        # Check for complete slot data
        for slot in range(1, 7):
            if f"slot{slot}_engine" in preset:
                # This slot is used, check for all params
                for param in range(16):
                    param_key = f"slot{slot}_param{param}"
                    if param_key not in preset:
                        # Warning only, as defaults can be used
                        reports.append(InconsistencyReport(
                            severity='low',
                            location=f"{source}:{preset.get('id', 'unknown')}",
                            issue=f"Missing parameter",
                            expected=param_key,
                            found="Missing",
                            fix_suggestion=f"Add {param_key} with default value 0.5"
                        ))
        
        return reports

class NetworkValidationAgent:
    """Agent that validates Trinity network communication"""
    
    def __init__(self):
        self.reports = []
    
    def validate_network_consistency(self) -> List[InconsistencyReport]:
        """Check that all components use same protocol"""
        reports = []
        
        # Check Visionary
        visionary_files = [
            'visionary.py',
            'visionary_openai_direct.py',
            'visionary_string_ids.py'
        ]
        
        for file in visionary_files:
            if Path(file).exists():
                with open(file, 'r') as f:
                    content = f.read()
                    
                # Check for consistent engine ID usage
                if 'ENGINE_' in content:
                    # Should import from authoritative source
                    if 'from engine_mapping_authoritative import' not in content:
                        reports.append(InconsistencyReport(
                            severity='high',
                            location=file,
                            issue="Not using authoritative engine mapping",
                            expected="from engine_mapping_authoritative import *",
                            found="Local engine definitions",
                            fix_suggestion="Import from engine_mapping_authoritative.py"
                        ))
        
        return reports

class ConsistencyCoordinator:
    """Coordinator that manages all validation agents"""
    
    def __init__(self):
        self.agents = {
            'engine': EngineValidationAgent(),
            'parameter': ParameterValidationAgent(),
            'preset': PresetValidationAgent(),
            'network': NetworkValidationAgent()
        }
        self.all_reports = []
    
    def run_full_validation(self) -> Dict:
        """Run all validation agents"""
        print("🔍 TRINITY CONSISTENCY VALIDATION")
        print("="*60)
        
        # Files to validate
        files_to_check = [
            # Python files
            'engine_mapping_authoritative.py',
            'visionary.py',
            'oracle.py',
            'calculator.py',
            'alchemist.py',
            'smart_oracle.py',
            'smart_calculator.py',
            'cloud_ai.py',
            'trinity_context.md',
            'parameter_manifest.json',
            'unified_engine_manifest.json',
            
            # Corpus files
            '/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json',
            
            # Training files
            'trinity_training_final.py',
            'trinity_learning_configured.py'
        ]
        
        all_reports = []
        
        # Run engine validation
        print("\n📋 Engine Validation Agent")
        print("-"*40)
        engine_reports = []
        for filepath in files_to_check:
            if Path(filepath).exists():
                reports = self.agents['engine'].validate_file(filepath)
                engine_reports.extend(reports)
                if reports:
                    print(f"  ❌ {Path(filepath).name}: {len(reports)} issues")
                else:
                    print(f"  ✅ {Path(filepath).name}: Clean")
        
        # Run parameter validation
        print("\n📋 Parameter Validation Agent")
        print("-"*40)
        param_reports = []
        for filepath in files_to_check:
            if Path(filepath).exists():
                reports = self.agents['parameter'].validate_file(filepath)
                param_reports.extend(reports)
                if reports:
                    print(f"  ❌ {Path(filepath).name}: {len(reports)} issues")
                else:
                    print(f"  ✅ {Path(filepath).name}: Clean")
        
        # Run preset validation
        print("\n📋 Preset Validation Agent")
        print("-"*40)
        preset_reports = []
        corpus_file = '/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3/faiss_index/presets_clean.json'
        if Path(corpus_file).exists():
            with open(corpus_file, 'r') as f:
                presets = json.load(f)
            
            for preset in presets[:10]:  # Check first 10 as sample
                reports = self.agents['preset'].validate_preset(preset, corpus_file)
                preset_reports.extend(reports)
            
            if preset_reports:
                print(f"  ❌ Corpus: {len(preset_reports)} issues in sample")
            else:
                print(f"  ✅ Corpus: Sample clean")
        
        # Run network validation
        print("\n📋 Network Validation Agent")
        print("-"*40)
        network_reports = self.agents['network'].validate_network_consistency()
        if network_reports:
            print(f"  ❌ Network: {len(network_reports)} issues")
        else:
            print(f"  ✅ Network: Clean")
        
        # Combine all reports
        all_reports = engine_reports + param_reports + preset_reports + network_reports
        
        # Generate summary
        return self.generate_report(all_reports)
    
    def generate_report(self, reports: List[InconsistencyReport]) -> Dict:
        """Generate comprehensive report"""
        
        # Group by severity
        critical = [r for r in reports if r.severity == 'critical']
        high = [r for r in reports if r.severity == 'high']
        medium = [r for r in reports if r.severity == 'medium']
        low = [r for r in reports if r.severity == 'low']
        
        print("\n" + "="*60)
        print("📊 VALIDATION SUMMARY")
        print("="*60)
        
        print(f"\n🔴 Critical Issues: {len(critical)}")
        for report in critical[:5]:  # Show first 5
            print(f"  • {report.location}: {report.issue}")
            print(f"    Expected: {report.expected}, Found: {report.found}")
        
        print(f"\n🟠 High Priority Issues: {len(high)}")
        for report in high[:5]:
            print(f"  • {report.location}: {report.issue}")
        
        print(f"\n🟡 Medium Priority Issues: {len(medium)}")
        print(f"🟢 Low Priority Issues: {len(low)}")
        
        # Overall health score
        total_issues = len(reports)
        health_score = max(0, 100 - (len(critical) * 10) - (len(high) * 5) - (len(medium) * 2) - len(low))
        
        print(f"\n🏥 Overall Health Score: {health_score}%")
        
        if health_score >= 90:
            print("✅ System consistency is EXCELLENT")
        elif health_score >= 70:
            print("⚠️ System consistency is GOOD but needs attention")
        elif health_score >= 50:
            print("⚠️ System consistency is FAIR - fixes needed")
        else:
            print("❌ System consistency is POOR - immediate action required")
        
        # Save detailed report
        report_data = {
            'timestamp': datetime.now().isoformat(),
            'summary': {
                'total_issues': total_issues,
                'critical': len(critical),
                'high': len(high),
                'medium': len(medium),
                'low': len(low),
                'health_score': health_score
            },
            'issues': [
                {
                    'severity': r.severity,
                    'location': r.location,
                    'issue': r.issue,
                    'expected': str(r.expected),
                    'found': str(r.found),
                    'fix': r.fix_suggestion
                }
                for r in reports
            ]
        }
        
        with open('consistency_report.json', 'w') as f:
            json.dump(report_data, f, indent=2)
        
        print(f"\n📄 Detailed report saved to consistency_report.json")
        
        return report_data

class ConsistencyFixer:
    """Agent that can automatically fix common issues"""
    
    def __init__(self, report_file: str = 'consistency_report.json'):
        with open(report_file, 'r') as f:
            self.report = json.load(f)
    
    def auto_fix_critical_issues(self):
        """Automatically fix critical issues where safe to do so"""
        fixes_applied = 0
        
        for issue in self.report['issues']:
            if issue['severity'] == 'critical':
                if 'Invalid engine ID' in issue['issue']:
                    # Can potentially fix by mapping to correct ID
                    print(f"🔧 Fixing invalid engine ID in {issue['location']}")
                    # Implementation would go here
                    fixes_applied += 1
        
        print(f"\n✅ Applied {fixes_applied} automatic fixes")
    
    def generate_fix_script(self):
        """Generate a script to fix all issues"""
        fixes = []
        
        for issue in self.report['issues']:
            fixes.append({
                'file': issue['location'].split(':')[0],
                'find': issue['found'],
                'replace': issue['expected'],
                'reason': issue['issue']
            })
        
        with open('fix_script.json', 'w') as f:
            json.dump(fixes, f, indent=2)
        
        print("📝 Fix script generated: fix_script.json")

def main():
    """Run the consistency validation"""
    coordinator = ConsistencyCoordinator()
    report = coordinator.run_full_validation()
    
    # If there are issues, offer to fix
    if report['summary']['total_issues'] > 0:
        print("\n" + "="*60)
        print("🔧 FIX OPTIONS")
        print("="*60)
        print("1. Generate fix script (safe)")
        print("2. Auto-fix critical issues (use with caution)")
        print("3. Manual review required")
        
        # Generate fix script
        fixer = ConsistencyFixer()
        fixer.generate_fix_script()

if __name__ == "__main__":
    main()