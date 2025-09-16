#!/usr/bin/env python3
"""
Parameter Migration Helper for Chimera Phoenix
Helps migrate engines to the new EngineParameterRegistry system
"""

import os
import re
import json
from pathlib import Path
from typing import Dict, List, Tuple

class ParameterMigrator:
    def __init__(self, source_dir: str):
        self.source_dir = Path(source_dir)
        self.engines_to_migrate = []
        self.migration_report = []
        
    def scan_engines(self) -> List[str]:
        """Find all engines that need migration"""
        unmigrated = []
        
        for cpp_file in self.source_dir.glob("*.cpp"):
            if self._needs_migration(cpp_file):
                unmigrated.append(cpp_file.stem)
                
        return unmigrated
    
    def _needs_migration(self, filepath: Path) -> bool:
        """Check if engine uses old parameter system"""
        content = filepath.read_text()
        
        # Look for old-style getParameterName implementation
        old_pattern = r'getParameterName\s*\([^)]*\)\s*const\s*\{[^}]*case\s+\d+:'
        if re.search(old_pattern, content):
            # Check if NOT using registry
            if "EngineParameterRegistry" not in content:
                return True
        
        return False
    
    def extract_parameters(self, engine_name: str) -> List[Dict]:
        """Extract parameter definitions from existing engine"""
        cpp_file = self.source_dir / f"{engine_name}.cpp"
        h_file = self.source_dir / f"{engine_name}.h"
        
        parameters = []
        
        # Extract from getParameterName
        cpp_content = cpp_file.read_text() if cpp_file.exists() else ""
        
        # Find getParameterName implementation
        param_pattern = r'case\s+(\d+):\s*return\s*"([^"]+)"'
        matches = re.findall(param_pattern, cpp_content)
        
        for index, name in matches:
            param = {
                'index': int(index),
                'name': name,
                'default': 0.5,  # Will be updated below
                'min': 0.0,
                'max': 1.0,
                'units': '',
                'description': ''
            }
            parameters.append(param)
        
        # Try to find default values from updateParameters
        update_pattern = r'params\.find\((\d+)\)[^{]*\{[^}]*=\s*([\d.]+)'
        defaults = re.findall(update_pattern, cpp_content)
        
        for index, default_val in defaults:
            idx = int(index)
            for param in parameters:
                if param['index'] == idx:
                    try:
                        param['default'] = float(default_val)
                    except:
                        pass
        
        # Look for common patterns to infer parameter types
        for param in parameters:
            name_lower = param['name'].lower()
            
            # Infer units and ranges
            if 'mix' in name_lower or 'wet' in name_lower:
                param['units'] = '%'
                param['description'] = 'Dry/wet mix'
            elif 'freq' in name_lower or 'cutoff' in name_lower:
                param['units'] = 'Hz'
                param['description'] = 'Filter frequency'
            elif 'gain' in name_lower or 'drive' in name_lower:
                param['units'] = 'dB'
                param['description'] = 'Gain amount'
            elif 'time' in name_lower or 'delay' in name_lower:
                param['units'] = 'ms'
                param['description'] = 'Time parameter'
            elif 'feedback' in name_lower:
                param['units'] = '%'
                param['max'] = 0.95
                param['description'] = 'Feedback amount'
            elif 'rate' in name_lower or 'speed' in name_lower:
                param['units'] = 'Hz'
                param['description'] = 'Modulation rate'
                
        return parameters
    
    def generate_registration_code(self, engine_name: str, engine_id: int, 
                                  parameters: List[Dict]) -> str:
        """Generate the registration code for an engine"""
        
        code = f"""
    // Register parameters with centralized system
    REGISTER_ENGINE_PARAMS({engine_id}, "{engine_name}")"""
        
        for param in parameters:
            name = param['name']
            default = param['default']
            units = param['units']
            desc = param['description']
            
            if units or desc:
                code += f'\n        .param("{name}", {default:.2f}f, "{units}", "{desc}")'
            else:
                code += f'\n        .param("{name}", {default:.2f}f)'
        
        code += "\n        .commit();\n"
        
        return code
    
    def migrate_engine(self, engine_name: str, engine_id: int, auto_apply: bool = False):
        """Migrate a single engine to new system"""
        
        print(f"\n=== Migrating {engine_name} ===")
        
        # Extract current parameters
        parameters = self.extract_parameters(engine_name)
        
        if not parameters:
            print(f"Warning: No parameters found for {engine_name}")
            return False
        
        print(f"Found {len(parameters)} parameters:")
        for param in parameters:
            print(f"  [{param['index']}] {param['name']} = {param['default']:.2f}")
        
        # Generate registration code
        reg_code = self.generate_registration_code(engine_name, engine_id, parameters)
        
        print("\nGenerated registration code:")
        print(reg_code)
        
        if auto_apply:
            # Insert into constructor
            cpp_file = self.source_dir / f"{engine_name}.cpp"
            if cpp_file.exists():
                content = cpp_file.read_text()
                
                # Find constructor
                constructor_pattern = rf'{engine_name}::{engine_name}\s*\([^)]*\)\s*\{{'
                match = re.search(constructor_pattern, content)
                
                if match:
                    # Insert registration at beginning of constructor
                    insert_pos = match.end()
                    new_content = content[:insert_pos] + reg_code + content[insert_pos:]
                    
                    # Add include if needed
                    if '#include "EngineParameterRegistry.h"' not in new_content:
                        include_pos = new_content.find('#include')
                        new_content = (new_content[:include_pos] + 
                                     '#include "EngineParameterRegistry.h"\n' + 
                                     new_content[include_pos:])
                    
                    # Write back
                    cpp_file.write_text(new_content)
                    print(f"✅ Updated {engine_name}.cpp")
                    
                    # Update getParameterName to use registry
                    self._update_parameter_getter(cpp_file, engine_id)
                    
                    return True
        
        return False
    
    def _update_parameter_getter(self, cpp_file: Path, engine_id: int):
        """Update getParameterName to use registry"""
        content = cpp_file.read_text()
        
        # Replace old implementation with registry lookup
        new_impl = f"""juce::String {cpp_file.stem}::getParameterName(int index) const {{
    return EngineParameterRegistry::getInstance().getParameterName({engine_id}, index);
}}"""
        
        # Find and replace old implementation
        pattern = r'juce::String[^{]*getParameterName[^{]*\{[^}]*\}'
        content = re.sub(pattern, new_impl, content, flags=re.DOTALL)
        
        cpp_file.write_text(content)
    
    def generate_test_file(self):
        """Generate test file for parameter validation"""
        
        test_code = """#include <gtest/gtest.h>
#include "EngineParameterRegistry.h"
#include "ParameterValidation.h"
#include "EngineFactory.h"

class ParameterTest : public ::testing::Test {
protected:
    void SetUp() override {
        EngineParameterRegistry::getInstance().clear();
    }
};

TEST_F(ParameterTest, AllEnginesHaveValidParameters) {
    // Test each engine
    for (int engineId = 1; engineId < ENGINE_COUNT; engineId++) {
        auto engine = EngineFactory::createEngine(engineId);
        if (engine) {
            auto result = ParameterValidation::validateEngine(engine.get(), engineId);
            
            EXPECT_TRUE(result.passed) 
                << "Engine " << getEngineTypeName(engineId) 
                << " failed validation: " 
                << ParameterValidation::generateReport(result);
        }
    }
}

TEST_F(ParameterTest, AllEnginesRegistered) {
    // Initialize all engines to trigger registration
    for (int engineId = 1; engineId < ENGINE_COUNT; engineId++) {
        auto engine = EngineFactory::createEngine(engineId);
    }
    
    // Check registration
    auto unregistered = EngineParameterRegistry::getInstance().getUnregisteredEngines();
    
    EXPECT_TRUE(unregistered.empty()) 
        << "Unregistered engines: " << unregistered.size();
        
    for (int id : unregistered) {
        ADD_FAILURE() << "Engine not registered: " << getEngineTypeName(id);
    }
}

TEST_F(ParameterTest, ParameterNamesNotGeneric) {
    for (int engineId = 1; engineId < ENGINE_COUNT; engineId++) {
        auto& registry = EngineParameterRegistry::getInstance();
        
        if (registry.isEngineRegistered(engineId)) {
            auto params = registry.getParameters(engineId);
            
            for (size_t i = 0; i < params.size(); i++) {
                EXPECT_FALSE(params[i].name.find("Param ") == 0)
                    << "Engine " << getEngineTypeName(engineId)
                    << " has generic parameter name at index " << i;
            }
        }
    }
}
"""
        
        test_file = self.source_dir.parent / "Tests" / "ParameterTests.cpp"
        test_file.parent.mkdir(exist_ok=True)
        test_file.write_text(test_code)
        print(f"Generated test file: {test_file}")
    
    def generate_report(self) -> str:
        """Generate migration status report"""
        
        report = "=== Parameter Migration Report ===\n\n"
        
        unmigrated = self.scan_engines()
        
        report += f"Engines needing migration: {len(unmigrated)}\n\n"
        
        for engine in unmigrated:
            report += f"  • {engine}\n"
        
        # Check which engines are using the new system
        migrated = []
        for cpp_file in self.source_dir.glob("*.cpp"):
            if "EngineParameterRegistry" in cpp_file.read_text():
                migrated.append(cpp_file.stem)
        
        report += f"\nEngines already migrated: {len(migrated)}\n\n"
        for engine in migrated:
            report += f"  ✅ {engine}\n"
        
        return report


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Migrate Chimera engines to new parameter system')
    parser.add_argument('--source', default='../Source', help='Source directory')
    parser.add_argument('--engine', help='Specific engine to migrate')
    parser.add_argument('--auto', action='store_true', help='Auto-apply changes')
    parser.add_argument('--report', action='store_true', help='Generate status report')
    parser.add_argument('--test', action='store_true', help='Generate test file')
    
    args = parser.parse_args()
    
    migrator = ParameterMigrator(args.source)
    
    if args.report:
        print(migrator.generate_report())
    elif args.test:
        migrator.generate_test_file()
    elif args.engine:
        # Need to get engine ID - could read from EngineTypes.h
        engine_id = 18  # Example: BIT_CRUSHER
        migrator.migrate_engine(args.engine, engine_id, args.auto)
    else:
        # Show unmigrated engines
        unmigrated = migrator.scan_engines()
        print(f"Found {len(unmigrated)} engines needing migration:")
        for engine in unmigrated:
            print(f"  • {engine}")


if __name__ == "__main__":
    main()