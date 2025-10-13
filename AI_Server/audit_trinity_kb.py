#!/usr/bin/env python3
"""
Comprehensive Trinity Engine Knowledge Base Audit Script
Performs detailed validation and generates fix report
"""

import json
import sys
from typing import Dict, List, Tuple, Any
from collections import defaultdict

class TrinityKnowledgeBaseAuditor:
    def __init__(self, filepath):
        self.filepath = filepath
        self.data = None
        self.issues = {
            'critical': [],
            'data_quality': [],
            'completeness': [],
            'recommendations': []
        }
        self.stats = defaultdict(int)

    def load_data(self):
        """Load the JSON knowledge base"""
        try:
            with open(self.filepath, 'r', encoding='utf-8') as f:
                self.data = json.load(f)
            return True
        except Exception as e:
            print(f"ERROR: Failed to load file: {e}")
            return False

    def audit_engine(self, engine_id: int, engine: Dict) -> Dict[str, List]:
        """Audit a single engine for all issues"""
        engine_issues = {
            'critical': [],
            'data_quality': [],
            'completeness': []
        }

        # Check required top-level fields
        required_fields = ['id', 'name', 'category', 'parameters', 'param_count', 'mix_param_index']
        for field in required_fields:
            if field not in engine:
                engine_issues['critical'].append(f"Missing required field: {field}")

        # Validate ID
        if engine.get('id') != engine_id:
            engine_issues['critical'].append(
                f"Engine ID mismatch: expected {engine_id}, got {engine.get('id')}"
            )

        # Validate parameters
        params = engine.get('parameters', [])
        param_count = engine.get('param_count', 0)
        actual_param_count = len(params)

        if param_count != actual_param_count:
            engine_issues['critical'].append(
                f"param_count ({param_count}) doesn't match actual parameters ({actual_param_count})"
            )

        # Validate mix_param_index
        mix_idx = engine.get('mix_param_index', -1)
        if mix_idx != -1 and mix_idx >= actual_param_count:
            engine_issues['critical'].append(
                f"mix_param_index ({mix_idx}) >= param_count ({actual_param_count})"
            )

            # Try to find the actual mix parameter
            for idx, param in enumerate(params):
                param_name = param.get('name', '').lower()
                if 'mix' in param_name or 'blend' in param_name or 'amount' in param_name:
                    engine_issues['recommendations'] = engine_issues.get('recommendations', [])
                    engine_issues['recommendations'].append(
                        f"Suggested mix_param_index: {idx} (parameter: '{param.get('name')}')"
                    )
                    break

        # Check for duplicate parameter names
        param_names = [p.get('name', '') for p in params]
        duplicates = [name for name in param_names if param_names.count(name) > 1]
        if duplicates:
            engine_issues['critical'].append(
                f"Duplicate parameter names: {set(duplicates)}"
            )

        # Validate each parameter
        for idx, param in enumerate(params):
            param_issues = self.validate_parameter(idx, param)
            for severity, issues in param_issues.items():
                if issues:
                    engine_issues[severity].extend([f"Param {idx} ({param.get('name', 'unknown')}): {issue}"
                                                    for issue in issues])

        # Check optional but important fields
        optional_fields = ['function', 'character', 'typical_usage']
        for field in optional_fields:
            if field not in engine:
                engine_issues['completeness'].append(f"Missing optional field: {field}")

        # Validate combines_well_with
        if 'combines_well_with' in engine:
            combos = engine['combines_well_with']
            if not isinstance(combos, list):
                engine_issues['data_quality'].append("combines_well_with should be an array")
            else:
                # Check for valid engine IDs
                for combo_id in combos:
                    if not isinstance(combo_id, int) or combo_id < 0 or combo_id > 56:
                        engine_issues['data_quality'].append(
                            f"Invalid engine ID in combines_well_with: {combo_id}"
                        )

        return engine_issues

    def validate_parameter(self, idx: int, param: Dict) -> Dict[str, List]:
        """Validate a single parameter"""
        issues = {
            'critical': [],
            'data_quality': [],
            'completeness': []
        }

        # Check required fields
        required = ['name', 'description', 'default', 'min', 'max']
        for field in required:
            if field not in param:
                issues['critical'].append(f"Missing required field: {field}")
                return issues  # Can't validate further without these

        name = param['name']
        default = param['default']
        min_val = param['min']
        max_val = param['max']

        # Validate name quality
        if name.strip() == '':
            issues['data_quality'].append("Empty parameter name")
        elif name.lower().startswith('param ') or name.lower() == 'parameter':
            issues['data_quality'].append(f"Generic parameter name: '{name}'")

        # Validate description quality
        desc = param.get('description', '')
        if desc.strip() == '':
            issues['data_quality'].append("Empty description")
        elif len(desc) < 10:
            issues['data_quality'].append(f"Very short description: '{desc}'")

        # Validate numeric ranges
        try:
            if min_val >= max_val:
                issues['critical'].append(f"min ({min_val}) >= max ({max_val})")

            if default < min_val or default > max_val:
                issues['critical'].append(
                    f"default ({default}) outside range [{min_val}, {max_val}]"
                )
        except (TypeError, ValueError) as e:
            issues['critical'].append(f"Invalid numeric values: {e}")

        return issues

    def run_full_audit(self):
        """Run complete audit"""
        if not self.load_data():
            return False

        print("=" * 80)
        print("TRINITY ENGINE KNOWLEDGE BASE AUDIT")
        print("=" * 80)
        print()

        engines = self.data.get('engines', {})

        # Handle both dict and list formats
        if isinstance(engines, dict):
            engine_list = []
            for engine_id_key, engine_data in engines.items():
                # If engine_data is a dict, use it; if it's a string, parse it
                if isinstance(engine_data, dict):
                    engine_list.append(engine_data)
                else:
                    print(f"Warning: Engine {engine_id_key} has unexpected data type: {type(engine_data)}")
            engines = engine_list

        print(f"Total engines found: {len(engines)}")
        print()

        # Check for sequential IDs
        expected_ids = set(range(57))  # 0-56
        actual_ids = set(e.get('id', -1) for e in engines if isinstance(e, dict))
        missing_ids = expected_ids - actual_ids
        extra_ids = actual_ids - expected_ids

        if missing_ids:
            self.issues['critical'].append(f"Missing engine IDs: {sorted(missing_ids)}")
        if extra_ids:
            self.issues['critical'].append(f"Unexpected engine IDs: {sorted(extra_ids)}")

        # Audit each engine
        engines_with_issues = []
        for engine in engines:
            if not isinstance(engine, dict):
                continue

            engine_id = engine.get('id', -1)
            engine_name = engine.get('name', f'Unknown_{engine_id}')

            engine_issues = self.audit_engine(engine_id, engine)

            has_issues = any(len(issues) > 0 for issues in engine_issues.values())
            if has_issues:
                engines_with_issues.append((engine_id, engine_name, engine_issues))

            # Collect statistics
            for severity in ['critical', 'data_quality', 'completeness']:
                self.stats[f'{severity}_issues'] += len(engine_issues[severity])

        # Report findings
        self.print_summary(engines_with_issues)
        self.print_detailed_issues(engines_with_issues)

        return True

    def print_summary(self, engines_with_issues):
        """Print audit summary"""
        print("\n" + "=" * 80)
        print("AUDIT SUMMARY")
        print("=" * 80)
        print(f"Total engines audited: {len(self.data.get('engines', []))}")
        print(f"Engines with issues: {len(engines_with_issues)}")
        print()
        print(f"Critical issues:     {self.stats['critical_issues']}")
        print(f"Data quality issues: {self.stats['data_quality_issues']}")
        print(f"Completeness issues: {self.stats['completeness_issues']}")
        print()

        if self.issues['critical']:
            print("GLOBAL CRITICAL ISSUES:")
            for issue in self.issues['critical']:
                print(f"  - {issue}")
            print()

    def print_detailed_issues(self, engines_with_issues):
        """Print detailed issue report"""
        if not engines_with_issues:
            print("\n" + "=" * 80)
            print("NO ISSUES FOUND - Knowledge base is valid!")
            print("=" * 80)
            return

        print("\n" + "=" * 80)
        print("DETAILED ISSUE REPORT")
        print("=" * 80)

        for engine_id, engine_name, issues in engines_with_issues:
            print(f"\nEngine {engine_id}: {engine_name}")
            print("-" * 80)

            if issues.get('critical'):
                print("  CRITICAL ISSUES:")
                for issue in issues['critical']:
                    print(f"    - {issue}")

            if issues.get('data_quality'):
                print("  DATA QUALITY ISSUES:")
                for issue in issues['data_quality']:
                    print(f"    - {issue}")

            if issues.get('completeness'):
                print("  COMPLETENESS ISSUES:")
                for issue in issues['completeness']:
                    print(f"    - {issue}")

            if issues.get('recommendations'):
                print("  RECOMMENDATIONS:")
                for rec in issues['recommendations']:
                    print(f"    - {rec}")

    def generate_fixed_version(self, output_path):
        """Generate corrected version with fixes applied"""
        print("\n" + "=" * 80)
        print("GENERATING FIXED VERSION")
        print("=" * 80)

        fixed_data = json.loads(json.dumps(self.data))  # Deep copy
        fixes_applied = 0

        engines = fixed_data.get('engines', {})

        # Handle both dict and list formats
        if isinstance(engines, dict):
            engine_list = list(engines.values())
        else:
            engine_list = engines

        for engine in engine_list:
            if not isinstance(engine, dict):
                continue

            # Fix param_count
            actual_count = len(engine.get('parameters', []))
            if engine.get('param_count') != actual_count:
                print(f"Engine {engine['id']} ({engine['name']}): Fixing param_count {engine['param_count']} -> {actual_count}")
                engine['param_count'] = actual_count
                fixes_applied += 1

            # Fix mix_param_index if out of bounds
            mix_idx = engine.get('mix_param_index', -1)
            if mix_idx >= actual_count:
                # Try to find mix parameter
                new_mix_idx = -1
                for idx, param in enumerate(engine.get('parameters', [])):
                    param_name = param.get('name', '').lower()
                    if 'mix' in param_name or 'blend' in param_name or 'amount' in param_name:
                        new_mix_idx = idx
                        break

                print(f"Engine {engine['id']} ({engine['name']}): Fixing mix_param_index {mix_idx} -> {new_mix_idx}")
                engine['mix_param_index'] = new_mix_idx
                fixes_applied += 1

        # Save fixed version
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(fixed_data, f, indent=2, ensure_ascii=False)

        print(f"\nTotal fixes applied: {fixes_applied}")
        print(f"Fixed version saved to: {output_path}")

        return fixes_applied > 0

def main():
    input_file = '/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/trinity_engine_knowledge_COMPLETE.json'
    output_file = '/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/AI_Server/trinity_engine_knowledge_COMPLETE_FIXED.json'

    auditor = TrinityKnowledgeBaseAuditor(input_file)

    if auditor.run_full_audit():
        if auditor.stats['critical_issues'] > 0 or auditor.stats['data_quality_issues'] > 0:
            auditor.generate_fixed_version(output_file)

    return 0

if __name__ == '__main__':
    sys.exit(main())
