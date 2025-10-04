#!/usr/bin/env python3
"""
Chimera Phoenix Parameter Mapping Analyzer

This script analyzes the source code to identify potential parameter mapping issues
by comparing getParameterName() and updateParameters() implementations.
"""

import re
import os
import glob
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass

@dataclass
class ParameterMapping:
    index: int
    name: str
    variable_name: Optional[str] = None
    
@dataclass
class EngineAnalysis:
    engine_name: str
    file_path: str
    parameter_names: List[ParameterMapping]
    parameter_updates: List[ParameterMapping]
    issues: List[str]
    confidence: float

class ParameterMappingAnalyzer:
    def __init__(self, source_dir: str):
        self.source_dir = source_dir
        self.results: List[EngineAnalysis] = []
        
        # Common patterns that indicate potential issues
        self.suspicious_patterns = [
            r'Param\s*\d+',
            r'Parameter\s*\d+', 
            r'Default',
            r'Unknown',
            r'Test'
        ]
        
        # Expected common parameter types
        self.common_parameters = {
            'gain': ['input', 'output', 'makeup', 'drive'],
            'mix': ['wet', 'dry', 'blend'],
            'frequency': ['cutoff', 'freq', 'hz'],
            'time': ['delay', 'attack', 'release', 'hold'],
            'level': ['threshold', 'amplitude', 'volume']
        }
    
    def analyze_all_engines(self):
        """Analyze all engine source files"""
        cpp_files = glob.glob(os.path.join(self.source_dir, "*.cpp"))
        
        print(f"=== Chimera Phoenix Parameter Mapping Analysis ===")
        print(f"Analyzing {len(cpp_files)} source files...")
        
        for file_path in cpp_files:
            if self.is_engine_file(file_path):
                self.analyze_engine_file(file_path)
        
        self.generate_report()
    
    def is_engine_file(self, file_path: str) -> bool:
        """Check if file is an engine implementation"""
        filename = os.path.basename(file_path)
        
        # Skip utility and non-engine files
        skip_files = [
            'PluginProcessor.cpp',
            'PluginEditor.cpp', 
            'PresetManager.cpp',
            'PresetGenerator.cpp',
            'EngineFactory.cpp',
            'CompleteEngineMetadata.cpp',
            'UnifiedDefaultParameters.cpp',
            'GoldenCorpusGenerator.cpp',
            'TestDefaultsLogic.cpp'
        ]
        
        return filename not in skip_files and not filename.startswith('Test')
    
    def analyze_engine_file(self, file_path: str):
        """Analyze a single engine file for parameter mapping issues"""
        filename = os.path.basename(file_path)
        engine_name = filename.replace('.cpp', '')
        
        print(f"Analyzing {engine_name}...")
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            analysis = EngineAnalysis(
                engine_name=engine_name,
                file_path=file_path,
                parameter_names=[],
                parameter_updates=[],
                issues=[],
                confidence=1.0
            )
            
            # Extract parameter names
            analysis.parameter_names = self.extract_parameter_names(content)
            
            # Extract parameter update mappings
            analysis.parameter_updates = self.extract_parameter_updates(content)
            
            # Check for issues
            self.check_mapping_consistency(analysis)
            self.check_suspicious_names(analysis)
            self.check_common_patterns(analysis)
            
            self.results.append(analysis)
            
        except Exception as e:
            print(f"Error analyzing {filename}: {e}")
    
    def extract_parameter_names(self, content: str) -> List[ParameterMapping]:
        """Extract parameter names from getParameterName() method"""
        # Find getParameterName method
        pattern = r'juce::String\s+\w*::getParameterName\s*\(\s*int\s+index\s*\)\s*const\s*{(.*?)^}'
        match = re.search(pattern, content, re.MULTILINE | re.DOTALL)
        
        if not match:
            return []
        
        method_body = match.group(1)
        
        # Extract case statements
        case_pattern = r'case\s+(\d+):\s*return\s*["\']([^"\']*)["\']'
        cases = re.findall(case_pattern, method_body)
        
        mappings = []
        for index_str, name in cases:
            mappings.append(ParameterMapping(
                index=int(index_str),
                name=name.strip()
            ))
        
        return sorted(mappings, key=lambda x: x.index)
    
    def extract_parameter_updates(self, content: str) -> List[ParameterMapping]:
        """Extract parameter update mappings from updateParameters() method"""
        # Find updateParameters method
        pattern = r'void\s+\w*::updateParameters\s*\(\s*const\s+std::map<int,\s*float>&\s+\w+\s*\)\s*{(.*?)^}'
        match = re.search(pattern, content, re.MULTILINE | re.DOTALL)
        
        if not match:
            return []
        
        method_body = match.group(1)
        
        mappings = []
        
        # Pattern 1: Direct index access like params.at(0) or params[0]
        direct_pattern = r'(?:params\.(?:at|find)\((\d+)\)|params\[(\d+)\])\s*[^=]*=\s*([^;]+)'
        direct_matches = re.findall(direct_pattern, method_body)
        
        for match in direct_matches:
            index = int(match[0] or match[1])
            target = match[2].strip()
            
            # Extract variable name
            var_match = re.search(r'm_(\w+)', target)
            var_name = var_match.group(1) if var_match else target
            
            mappings.append(ParameterMapping(
                index=index,
                name='',  # Will be filled from getParameterName
                variable_name=var_name
            ))
        
        # Pattern 2: Case statements in switch
        case_pattern = r'case\s+(\d+):\s*([^:;]+);'
        case_matches = re.findall(case_pattern, method_body)
        
        for index_str, assignment in case_matches:
            index = int(index_str)
            
            # Extract variable name from assignment
            var_match = re.search(r'm_(\w+)', assignment)
            var_name = var_match.group(1) if var_match else assignment.strip()
            
            mappings.append(ParameterMapping(
                index=index,
                name='',
                variable_name=var_name
            ))
        
        return sorted(mappings, key=lambda x: x.index)
    
    def check_mapping_consistency(self, analysis: EngineAnalysis):
        """Check if parameter names and update mappings are consistent"""
        name_indices = {p.index for p in analysis.parameter_names}
        update_indices = {p.index for p in analysis.parameter_updates}
        
        # Check for missing mappings
        missing_names = update_indices - name_indices
        missing_updates = name_indices - update_indices
        
        if missing_names:
            analysis.issues.append(f"Parameters with updates but no names: {sorted(missing_names)}")
            analysis.confidence -= 0.2
        
        if missing_updates:
            analysis.issues.append(f"Parameters with names but no updates: {sorted(missing_updates)}")
            analysis.confidence -= 0.2
        
        # Check for index gaps
        if analysis.parameter_names:
            max_index = max(p.index for p in analysis.parameter_names)
            expected_indices = set(range(max_index + 1))
            actual_indices = {p.index for p in analysis.parameter_names}
            
            missing_indices = expected_indices - actual_indices
            if missing_indices:
                analysis.issues.append(f"Missing parameter indices: {sorted(missing_indices)}")
                analysis.confidence -= 0.1
    
    def check_suspicious_names(self, analysis: EngineAnalysis):
        """Check for suspicious parameter names"""
        for param in analysis.parameter_names:
            for pattern in self.suspicious_patterns:
                if re.search(pattern, param.name, re.IGNORECASE):
                    analysis.issues.append(f"Suspicious parameter name at index {param.index}: '{param.name}'")
                    analysis.confidence -= 0.15
            
            if not param.name or param.name.isspace():
                analysis.issues.append(f"Empty parameter name at index {param.index}")
                analysis.confidence -= 0.2
    
    def check_common_patterns(self, analysis: EngineAnalysis):
        """Check for common parameter mapping patterns and potential issues"""
        # Check Mix parameter position
        mix_params = [p for p in analysis.parameter_names if 'mix' in p.name.lower()]
        if mix_params:
            mix_param = mix_params[0]
            total_params = len(analysis.parameter_names)
            
            # Mix is commonly last or near-last parameter
            if total_params > 3 and mix_param.index < total_params - 3:
                analysis.issues.append(f"Mix parameter at index {mix_param.index}, expected near end (total: {total_params})")
                analysis.confidence -= 0.05
        
        # Check Gain parameter patterns
        gain_params = [p for p in analysis.parameter_names if 'gain' in p.name.lower()]
        for gain_param in gain_params:
            # Input gain commonly at index 0, output gain near end
            if 'input' in gain_param.name.lower() and gain_param.index != 0:
                analysis.issues.append(f"Input gain at index {gain_param.index}, commonly at index 0")
                analysis.confidence -= 0.03
        
        # Check for parameter name/variable name mismatches
        for name_param in analysis.parameter_names:
            update_param = next((p for p in analysis.parameter_updates if p.index == name_param.index), None)
            if update_param and update_param.variable_name:
                name_lower = name_param.name.lower().replace(' ', '').replace('_', '')
                var_lower = update_param.variable_name.lower().replace('_', '')
                
                # Check if they're related
                if not self.names_are_related(name_lower, var_lower):
                    analysis.issues.append(f"Potential name/variable mismatch at index {name_param.index}: '{name_param.name}' -> '{update_param.variable_name}'")
                    analysis.confidence -= 0.1
    
    def names_are_related(self, param_name: str, var_name: str) -> bool:
        """Check if a parameter name and variable name are semantically related"""
        # Direct substring match
        if param_name in var_name or var_name in param_name:
            return True
        
        # Common synonyms
        synonyms = {
            'gain': ['level', 'amplitude', 'volume'],
            'mix': ['wet', 'dry', 'blend'],
            'frequency': ['freq', 'cutoff', 'hz'],
            'time': ['delay', 'duration'],
            'feedback': ['fb', 'regen'],
            'resonance': ['q', 'quality'],
            'attack': ['atk'],
            'release': ['rel'],
            'threshold': ['thresh']
        }
        
        for key, values in synonyms.items():
            if key in param_name and any(v in var_name for v in values):
                return True
            if key in var_name and any(v in param_name for v in values):
                return True
        
        return False
    
    def generate_report(self):
        """Generate comprehensive analysis report"""
        total_engines = len(self.results)
        engines_with_issues = sum(1 for r in self.results if r.issues)
        total_issues = sum(len(r.issues) for r in self.results)
        
        print(f"\n=== PARAMETER MAPPING ANALYSIS REPORT ===")
        print(f"Total Engines Analyzed: {total_engines}")
        print(f"Engines with Issues: {engines_with_issues}")
        print(f"Total Issues Found: {total_issues}")
        print(f"Success Rate: {((total_engines - engines_with_issues) / total_engines * 100):.1f}%")
        
        # Write detailed report
        with open('parameter_mapping_analysis_report.md', 'w') as f:
            f.write("# Chimera Phoenix Parameter Mapping Analysis Report\n\n")
            f.write(f"**Total Engines Analyzed:** {total_engines}\n")
            f.write(f"**Engines with Issues:** {engines_with_issues}\n")
            f.write(f"**Total Issues Found:** {total_issues}\n")
            f.write(f"**Success Rate:** {((total_engines - engines_with_issues) / total_engines * 100):.1f}%\n\n")
            
            f.write("## Issues by Engine\n\n")
            
            # Sort by confidence (lowest first = most issues)
            sorted_results = sorted(self.results, key=lambda x: x.confidence)
            
            for result in sorted_results:
                if result.issues:
                    f.write(f"### ❌ {result.engine_name} (Confidence: {result.confidence:.2f})\n\n")
                    for issue in result.issues:
                        f.write(f"- {issue}\n")
                    f.write("\n")
                    
                    # Show parameter mappings
                    f.write("**Parameter Mappings:**\n")
                    for param in result.parameter_names:
                        update_param = next((p for p in result.parameter_updates if p.index == param.index), None)
                        var_info = f" → {update_param.variable_name}" if update_param and update_param.variable_name else ""
                        f.write(f"- Index {param.index}: \"{param.name}\"{var_info}\n")
                    f.write("\n")
                else:
                    f.write(f"### ✅ {result.engine_name} - No issues detected\n\n")
            
            f.write("## Summary of Common Issues\n\n")
            
            # Analyze common issue patterns
            issue_patterns = {}
            for result in self.results:
                for issue in result.issues:
                    # Extract issue type
                    if "Suspicious parameter name" in issue:
                        issue_patterns["Suspicious Names"] = issue_patterns.get("Suspicious Names", 0) + 1
                    elif "Mix parameter at index" in issue:
                        issue_patterns["Mix Position"] = issue_patterns.get("Mix Position", 0) + 1
                    elif "mismatch" in issue:
                        issue_patterns["Name/Variable Mismatch"] = issue_patterns.get("Name/Variable Mismatch", 0) + 1
                    elif "Missing parameter" in issue:
                        issue_patterns["Missing Parameters"] = issue_patterns.get("Missing Parameters", 0) + 1
            
            for issue_type, count in sorted(issue_patterns.items(), key=lambda x: x[1], reverse=True):
                f.write(f"- **{issue_type}:** {count} occurrences\n")
            
            f.write("\n## Recommendations\n\n")
            f.write("1. **Fix Generic Parameter Names**: Replace \"Param X\" with descriptive names\n")
            f.write("2. **Standardize Mix Parameter Position**: Consider moving Mix to consistent position\n")
            f.write("3. **Align Parameter Names with Variables**: Ensure UI names match internal variable purposes\n")
            f.write("4. **Fill Parameter Gaps**: Add names for all parameter indices\n")
            f.write("5. **Add Parameter Documentation**: Document expected functionality for each parameter\n")
        
        print("\nDetailed report written to: parameter_mapping_analysis_report.md")
        
        # Print worst offenders
        print("\n=== ENGINES NEEDING ATTENTION ===")
        worst_engines = [r for r in sorted_results if r.confidence < 0.8][:10]
        
        for result in worst_engines:
            print(f"- {result.engine_name}: {len(result.issues)} issues (confidence: {result.confidence:.2f})")
            for issue in result.issues[:3]:  # Show first 3 issues
                print(f"  • {issue}")
            if len(result.issues) > 3:
                print(f"  • ... and {len(result.issues) - 3} more")

if __name__ == "__main__":
    analyzer = ParameterMappingAnalyzer("Source")
    analyzer.analyze_all_engines()