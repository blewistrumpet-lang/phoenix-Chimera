#!/usr/bin/env python3
"""
Real Parameter Issues Finder

This script looks for actual parameter mapping problems, not false positives.
It focuses on finding engines where the parameter indices don't match between
getParameterName() and updateParameters().
"""

import re
import os
import glob
from dataclasses import dataclass
from typing import Dict, List, Tuple, Optional

@dataclass
class RealIssue:
    engine_name: str
    issue_type: str
    description: str
    severity: str  # "critical", "warning", "info"

class RealParameterIssuesFinder:
    def __init__(self, source_dir: str):
        self.source_dir = source_dir
        self.issues: List[RealIssue] = []
    
    def find_all_issues(self):
        """Find real parameter mapping issues"""
        print("=== Searching for Real Parameter Mapping Issues ===")
        
        cpp_files = glob.glob(os.path.join(self.source_dir, "*.cpp"))
        engine_files = [f for f in cpp_files if self.is_engine_file(f)]
        
        print(f"Checking {len(engine_files)} engine files...")
        
        for file_path in engine_files:
            self.check_engine_file(file_path)
        
        self.generate_real_issues_report()
    
    def is_engine_file(self, file_path: str) -> bool:
        """Check if file is an engine implementation"""
        filename = os.path.basename(file_path)
        
        skip_files = [
            'PluginProcessor.cpp', 'PluginEditor.cpp', 'PresetManager.cpp',
            'PresetGenerator.cpp', 'EngineFactory.cpp', 'CompleteEngineMetadata.cpp',
            'UnifiedDefaultParameters.cpp', 'GoldenCorpusGenerator.cpp',
            'TestDefaultsLogic.cpp', 'fix_ui_and_audio.cpp', 'PresetExporter.cpp',
            'PresetSerializer.cpp', 'PresetVariationGenerator.cpp', 'BoutiquePresetGenerator.cpp',
            'GenerateDetailedCorpus.cpp', 'GenerateGoldenCorpus.cpp', 'GoldenCorpusBuilder.cpp',
            'GoldenCorpusPresets.cpp', 'EngineMetadataInit.cpp', 'SpectralFreezeTest.cpp'
        ]
        
        return filename not in skip_files and not filename.startswith('Test')
    
    def check_engine_file(self, file_path: str):
        """Check a single engine file for real issues"""
        filename = os.path.basename(file_path)
        engine_name = filename.replace('.cpp', '')
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Check for missing implementations
            self.check_missing_implementations(engine_name, content)
            
            # Check for real index mismatches
            self.check_index_mismatches(engine_name, content)
            
            # Check for suspicious patterns
            self.check_suspicious_patterns(engine_name, content)
            
        except Exception as e:
            self.issues.append(RealIssue(
                engine_name=engine_name,
                issue_type="File Error",
                description=f"Could not analyze file: {e}",
                severity="warning"
            ))
    
    def check_missing_implementations(self, engine_name: str, content: str):
        """Check for missing getParameterName or updateParameters implementations"""
        
        # Check for getParameterName
        param_name_pattern = r'juce::String\s+\w*::getParameterName\s*\(\s*int\s+\w+\s*\)\s*const'
        has_param_name = bool(re.search(param_name_pattern, content))
        
        # Check for updateParameters
        update_params_pattern = r'void\s+\w*::updateParameters\s*\(\s*const\s+std::map<int,\s*float>&'
        has_update_params = bool(re.search(update_params_pattern, content))
        
        if not has_param_name:
            self.issues.append(RealIssue(
                engine_name=engine_name,
                issue_type="Missing Implementation",
                description="Missing getParameterName() implementation",
                severity="critical"
            ))
        
        if not has_update_params:
            self.issues.append(RealIssue(
                engine_name=engine_name,
                issue_type="Missing Implementation", 
                description="Missing updateParameters() implementation",
                severity="critical"
            ))
    
    def check_index_mismatches(self, engine_name: str, content: str):
        """Check for real index mismatches between parameter names and updates"""
        
        # Extract parameter names with their indices
        param_names = self.extract_parameter_names_accurate(content)
        if not param_names:
            return
        
        # Extract update parameter indices  
        update_indices = self.extract_update_indices_accurate(content)
        if not update_indices:
            return
        
        # Check for critical mismatches
        name_indices = set(param_names.keys())
        update_indices_set = set(update_indices)
        
        # Critical: Parameters have names but no updates
        missing_updates = name_indices - update_indices_set
        if missing_updates:
            self.issues.append(RealIssue(
                engine_name=engine_name,
                issue_type="Index Mismatch",
                description=f"Parameters have names but no updates: {sorted(missing_updates)}",
                severity="critical"
            ))
        
        # Warning: Parameters have updates but no names  
        missing_names = update_indices_set - name_indices
        if missing_names:
            self.issues.append(RealIssue(
                engine_name=engine_name,
                issue_type="Index Mismatch",
                description=f"Parameters have updates but no names: {sorted(missing_names)}",
                severity="warning"
            ))
        
        # Check for potential off-by-one errors in mapping
        self.check_off_by_one_errors(engine_name, param_names, content)
    
    def extract_parameter_names_accurate(self, content: str) -> Dict[int, str]:
        """Accurately extract parameter names with indices"""
        # Find getParameterName method
        pattern = r'juce::String\s+\w*::getParameterName\s*\(\s*int\s+\w+\s*\)\s*const\s*{(.*?)^}'
        match = re.search(pattern, content, re.MULTILINE | re.DOTALL)
        
        if not match:
            return {}
        
        method_body = match.group(1)
        
        # Extract case statements
        case_pattern = r'case\s+(\d+):\s*return\s*["\']([^"\']*)["\']'
        cases = re.findall(case_pattern, method_body)
        
        return {int(index): name.strip() for index, name in cases}
    
    def extract_update_indices_accurate(self, content: str) -> List[int]:
        """Accurately extract parameter update indices"""
        # Find updateParameters method
        pattern = r'void\s+\w*::updateParameters\s*\(\s*const\s+std::map<int,\s*float>&\s+\w+\s*\)\s*{(.*?)^}'
        match = re.search(pattern, content, re.MULTILINE | re.DOTALL)
        
        if not match:
            return []
        
        method_body = match.group(1)
        
        indices = []
        
        # Look for patterns like params.count(N), params.find(N), params.at(N), params[N]
        index_patterns = [
            r'params\.(?:count|find|at)\((\d+)\)',
            r'params\[(\d+)\]',
            r'case\s+(\d+):.*break',  # Switch case pattern
        ]
        
        for pattern in index_patterns:
            matches = re.findall(pattern, method_body)
            indices.extend(int(m) for m in matches)
        
        return sorted(set(indices))
    
    def check_off_by_one_errors(self, engine_name: str, param_names: Dict[int, str], content: str):
        """Check for potential off-by-one errors by looking at parameter assignments"""
        
        # Look for patterns where index X assigns to a variable that sounds like it should be index Y
        update_pattern = r'params\.(?:at|find)\((\d+)\)[^;]*?m_(\w+)'
        matches = re.findall(update_pattern, content)
        
        for index_str, var_name in matches:
            index = int(index_str)
            var_name_lower = var_name.lower()
            
            # Check if any parameter name at a different index matches this variable better
            for param_index, param_name in param_names.items():
                if param_index != index:
                    param_name_lower = param_name.lower().replace(' ', '').replace('_', '')
                    
                    # Simple heuristic: if variable name is very similar to a different parameter name
                    if (var_name_lower in param_name_lower or param_name_lower in var_name_lower) and \
                       len(var_name_lower) > 3:  # Avoid false positives on short names
                        
                        # Check if the current index also has a reasonable name
                        current_param_name = param_names.get(index, "")
                        current_name_lower = current_param_name.lower().replace(' ', '').replace('_', '')
                        
                        # Only flag if the mismatch seems significant
                        if var_name_lower not in current_name_lower:
                            self.issues.append(RealIssue(
                                engine_name=engine_name,
                                issue_type="Potential Off-by-One",
                                description=f"Index {index} ('{current_param_name}') assigns to m_{var_name}, but '{param_name}' is at index {param_index}",
                                severity="warning"
                            ))
    
    def check_suspicious_patterns(self, engine_name: str, content: str):
        """Check for suspicious patterns that indicate real issues"""
        
        # Check for hardcoded parameter counts that don't match actual parameters
        param_names = self.extract_parameter_names_accurate(content)
        if param_names:
            max_index = max(param_names.keys())
            expected_count = max_index + 1
            
            # Look for getNumParameters() implementation
            num_params_pattern = r'int\s+getNumParameters\s*\(\s*\)\s*const\s*override\s*{\s*return\s+(\d+)'
            match = re.search(num_params_pattern, content)
            
            if match:
                reported_count = int(match.group(1))
                if reported_count != expected_count:
                    self.issues.append(RealIssue(
                        engine_name=engine_name,
                        issue_type="Count Mismatch",
                        description=f"getNumParameters() returns {reported_count} but found parameters 0-{max_index} ({expected_count} total)",
                        severity="critical"
                    ))
        
        # Check for empty parameter names
        if param_names:
            for index, name in param_names.items():
                if not name or name.isspace():
                    self.issues.append(RealIssue(
                        engine_name=engine_name,
                        issue_type="Empty Parameter Name",
                        description=f"Parameter {index} has empty name",
                        severity="warning"
                    ))
    
    def generate_real_issues_report(self):
        """Generate report of real issues found"""
        critical_issues = [i for i in self.issues if i.severity == "critical"]
        warning_issues = [i for i in self.issues if i.severity == "warning"]
        
        print(f"\n=== REAL PARAMETER MAPPING ISSUES FOUND ===")
        print(f"Critical Issues: {len(critical_issues)}")
        print(f"Warning Issues: {len(warning_issues)}")
        print(f"Total Issues: {len(self.issues)}")
        
        # Group by engine
        engines_with_issues = {}
        for issue in self.issues:
            if issue.engine_name not in engines_with_issues:
                engines_with_issues[issue.engine_name] = []
            engines_with_issues[issue.engine_name].append(issue)
        
        # Generate detailed report
        with open('real_parameter_issues_report.md', 'w') as f:
            f.write("# Real Parameter Mapping Issues Report\n\n")
            f.write(f"**Critical Issues:** {len(critical_issues)}\n")
            f.write(f"**Warning Issues:** {len(warning_issues)}\n")
            f.write(f"**Total Issues:** {len(self.issues)}\n")
            f.write(f"**Engines Affected:** {len(engines_with_issues)}\n\n")
            
            if critical_issues:
                f.write("## 🚨 Critical Issues (Must Fix)\n\n")
                for issue in critical_issues:
                    f.write(f"### {issue.engine_name} - {issue.issue_type}\n")
                    f.write(f"{issue.description}\n\n")
            
            if warning_issues:
                f.write("## ⚠️ Warning Issues (Should Fix)\n\n")
                for issue in warning_issues:
                    f.write(f"### {issue.engine_name} - {issue.issue_type}\n")
                    f.write(f"{issue.description}\n\n")
            
            f.write("## Summary by Engine\n\n")
            for engine_name, issues in sorted(engines_with_issues.items()):
                critical_count = sum(1 for i in issues if i.severity == "critical")
                warning_count = sum(1 for i in issues if i.severity == "warning")
                
                status = "🚨" if critical_count > 0 else "⚠️" if warning_count > 0 else "✅"
                f.write(f"- {status} **{engine_name}**: {critical_count} critical, {warning_count} warnings\n")
        
        print(f"\nDetailed report written to: real_parameter_issues_report.md")
        
        # Print summary of most critical issues
        if critical_issues:
            print("\n=== CRITICAL ISSUES REQUIRING IMMEDIATE ATTENTION ===")
            for issue in critical_issues[:10]:  # Show first 10 critical issues
                print(f"- {issue.engine_name}: {issue.description}")

if __name__ == "__main__":
    finder = RealParameterIssuesFinder("Source")
    finder.find_all_issues()