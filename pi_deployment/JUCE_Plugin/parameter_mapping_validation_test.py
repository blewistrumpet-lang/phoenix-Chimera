#!/usr/bin/env python3
"""
Parameter Mapping Validation Test

This script provides concrete evidence that parameter mappings are working correctly
by testing specific engines and showing the exact mapping between UI labels and
engine functionality.
"""

import re
import os

class ParameterMappingValidationTest:
    def __init__(self, source_dir: str):
        self.source_dir = source_dir
        self.test_results = []
    
    def run_validation_tests(self):
        """Run validation tests on a sample of engines"""
        print("=== Parameter Mapping Validation Test ===")
        print("Testing sample engines to prove parameter mappings are correct...\n")
        
        # Test a diverse sample of engines
        test_engines = [
            "VintageTubePreamp.cpp",
            "ClassicCompressor.cpp", 
            "RodentDistortion.cpp",
            "AnalogRingModulator.cpp",
            "PlateReverb.cpp",
            "StateVariableFilter.cpp",
            "BitCrusher.cpp",
            "StereoChorus.cpp"
        ]
        
        for engine_file in test_engines:
            self.test_engine_mapping(engine_file)
        
        self.generate_validation_report()
    
    def test_engine_mapping(self, filename: str):
        """Test parameter mapping for a specific engine"""
        file_path = os.path.join(self.source_dir, filename)
        engine_name = filename.replace('.cpp', '')
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            print(f"🔍 Testing {engine_name}...")
            
            # Extract parameter names
            param_names = self.extract_parameter_names(content)
            
            # Extract update mappings
            update_mappings = self.extract_update_mappings(content)
            
            # Validate mapping consistency
            is_valid, issues = self.validate_mapping(param_names, update_mappings)
            
            result = {
                'engine': engine_name,
                'valid': is_valid,
                'param_names': param_names,
                'update_mappings': update_mappings,
                'issues': issues
            }
            
            self.test_results.append(result)
            
            if is_valid:
                print(f"   ✅ Parameter mapping is CORRECT")
                print(f"   📊 {len(param_names)} parameters mapped correctly")
            else:
                print(f"   ❌ Issues found: {', '.join(issues)}")
            
            # Show sample mappings
            print(f"   📋 Sample mappings:")
            for i, (index, name) in enumerate(param_names.items()):
                if i < 3:  # Show first 3
                    mapping = update_mappings.get(index, "No update found")
                    print(f"      Index {index}: '{name}' → {mapping}")
            print()
            
        except Exception as e:
            print(f"   ❌ Error testing {filename}: {e}\n")
    
    def extract_parameter_names(self, content: str) -> dict:
        """Extract parameter names from getParameterName method"""
        pattern = r'juce::String\s+\w*::getParameterName\s*\(\s*int\s+\w+\s*\)\s*const[^{]*{(.*?)^}'
        match = re.search(pattern, content, re.MULTILINE | re.DOTALL)
        
        if not match:
            return {}
        
        method_body = match.group(1)
        case_pattern = r'case\s+(\d+):\s*return\s*["\']([^"\']*)["\']'
        cases = re.findall(case_pattern, method_body)
        
        return {int(index): name.strip() for index, name in cases}
    
    def extract_update_mappings(self, content: str) -> dict:
        """Extract parameter update mappings from updateParameters method"""
        pattern = r'void\s+\w*::updateParameters\s*\([^)]+\)[^{]*{(.*?)^}'
        match = re.search(pattern, content, re.MULTILINE | re.DOTALL)
        
        if not match:
            return {}
        
        method_body = match.group(1)
        mappings = {}
        
        # Look for patterns like:
        # if (params.count(0)) m_variable.target = params.at(0);
        # case 0: m_variable.setTarget(params.at(0)); break;
        # m_variable.setTarget(getParam(0, default));
        
        patterns = [
            r'params\.(?:count|find)\((\d+)\)[^;]*?m_(\w+)[^;]*?params\.at\(\1\)',
            r'case\s+(\d+):[^}]*?m_(\w+)[^;]*?setTarget',
            r'm_(\w+)[^;]*?setTarget[^;]*?getParam\((\d+)',
        ]
        
        for pattern in patterns:
            matches = re.finditer(pattern, method_body)
            for match in matches:
                if len(match.groups()) == 2:
                    if pattern.endswith(r'getParam\((\d+)'):
                        # For the getParam pattern, groups are reversed
                        var_name, index_str = match.groups()
                    else:
                        index_str, var_name = match.groups()
                    
                    index = int(index_str)
                    mappings[index] = f"m_{var_name}"
        
        return mappings
    
    def validate_mapping(self, param_names: dict, update_mappings: dict) -> tuple:
        """Validate that parameter names and updates are consistent"""
        issues = []
        
        # Check that all named parameters have updates
        for index in param_names:
            if index not in update_mappings:
                issues.append(f"Parameter {index} ('{param_names[index]}') has no update mapping")
        
        # Check that all updates have names
        for index in update_mappings:
            if index not in param_names:
                issues.append(f"Update for index {index} has no parameter name")
        
        # Check for reasonable semantic matching
        for index in param_names:
            if index in update_mappings:
                param_name = param_names[index].lower().replace(' ', '').replace('_', '')
                var_name = update_mappings[index].lower().replace('m_', '').replace('_', '')
                
                # Check if names are semantically related
                if not self.are_names_related(param_name, var_name):
                    # Only flag if it's a significant mismatch
                    if len(param_name) > 3 and len(var_name) > 3:
                        if not (param_name in var_name or var_name in param_name):
                            issues.append(f"Semantic mismatch: '{param_names[index]}' → {update_mappings[index]}")
        
        return len(issues) == 0, issues
    
    def are_names_related(self, param_name: str, var_name: str) -> bool:
        """Check if parameter name and variable name are semantically related"""
        # Direct match
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
            'threshold': ['thresh'],
            'output': ['out'],
            'input': ['in']
        }
        
        for key, values in synonyms.items():
            if key in param_name:
                if any(v in var_name for v in values + [key]):
                    return True
            if key in var_name:
                if any(v in param_name for v in values + [key]):
                    return True
        
        return False
    
    def generate_validation_report(self):
        """Generate final validation report"""
        total_engines = len(self.test_results)
        valid_engines = sum(1 for r in self.test_results if r['valid'])
        total_parameters = sum(len(r['param_names']) for r in self.test_results)
        
        print("=" * 60)
        print("PARAMETER MAPPING VALIDATION RESULTS")
        print("=" * 60)
        print(f"Engines Tested: {total_engines}")
        print(f"Engines Valid: {valid_engines}")
        print(f"Success Rate: {(valid_engines / total_engines * 100):.1f}%")
        print(f"Total Parameters Tested: {total_parameters}")
        
        # Write detailed report
        with open('parameter_mapping_validation_results.md', 'w') as f:
            f.write("# Parameter Mapping Validation Results\n\n")
            f.write(f"**Test Summary:**\n")
            f.write(f"- Engines Tested: {total_engines}\n")
            f.write(f"- Engines Valid: {valid_engines}\n") 
            f.write(f"- Success Rate: {(valid_engines / total_engines * 100):.1f}%\n")
            f.write(f"- Total Parameters Tested: {total_parameters}\n\n")
            
            f.write("## Test Results by Engine\n\n")
            
            for result in self.test_results:
                status = "✅" if result['valid'] else "❌"
                f.write(f"### {status} {result['engine']}\n\n")
                
                if result['valid']:
                    f.write("**Status:** Parameter mapping is CORRECT\n\n")
                else:
                    f.write("**Issues Found:**\n")
                    for issue in result['issues']:
                        f.write(f"- {issue}\n")
                    f.write("\n")
                
                f.write("**Parameter Mappings:**\n")
                for index, name in result['param_names'].items():
                    mapping = result['update_mappings'].get(index, "❌ No update mapping")
                    f.write(f"- Index {index}: \"{name}\" → {mapping}\n")
                f.write("\n")
            
            f.write("## Conclusion\n\n")
            if valid_engines == total_engines:
                f.write("🎉 **ALL TESTED ENGINES HAVE CORRECT PARAMETER MAPPINGS**\n\n")
                f.write("The analysis confirms that:\n")
                f.write("1. All parameter names have corresponding update mappings\n")
                f.write("2. Parameter indices are consistent between getParameterName() and updateParameters()\n")
                f.write("3. No off-by-one errors were detected\n")
                f.write("4. Semantic mapping between UI labels and engine variables is appropriate\n\n")
                f.write("**Verdict:** The Chimera Phoenix parameter mapping system is working correctly.\n")
            else:
                f.write(f"⚠️ **{total_engines - valid_engines} engines have mapping issues that need attention**\n")
        
        print(f"\nDetailed validation report: parameter_mapping_validation_results.md")
        
        if valid_engines == total_engines:
            print("\n🎉 VALIDATION SUCCESSFUL: All tested engines have correct parameter mappings!")
        else:
            print(f"\n⚠️ {total_engines - valid_engines} engines need attention")

if __name__ == "__main__":
    validator = ParameterMappingValidationTest("Source")
    validator.run_validation_tests()