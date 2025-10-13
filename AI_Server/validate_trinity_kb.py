#!/usr/bin/env python3
"""
Trinity Engine Knowledge Base Validator
Quick validation script for checking knowledge base integrity
Usage: python3 validate_trinity_kb.py <path_to_json>
"""

import json
import sys
from pathlib import Path


def validate_kb(filepath):
    """Quick validation of Trinity knowledge base"""

    print(f"Validating: {filepath}")
    print("=" * 60)

    # Load data
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception as e:
        print(f"ERROR: Failed to load JSON: {e}")
        return False

    # Get engines
    engines = data.get('engines', {})
    if isinstance(engines, dict):
        engine_list = list(engines.values())
    else:
        engine_list = engines

    print(f"Total engines: {len(engine_list)}\n")

    # Track issues
    critical = []
    warnings = []

    # Validate each engine
    for engine in engine_list:
        if not isinstance(engine, dict):
            continue

        eng_id = engine.get('id', -1)
        eng_name = engine.get('name', 'Unknown')

        # Check required fields
        required = ['id', 'name', 'category', 'parameters', 'param_count', 'mix_param_index']
        for field in required:
            if field not in engine:
                critical.append(f"Engine {eng_id} ({eng_name}): Missing field '{field}'")

        # Validate param_count
        params = engine.get('parameters', [])
        param_count = engine.get('param_count', 0)
        if len(params) != param_count:
            critical.append(
                f"Engine {eng_id} ({eng_name}): param_count mismatch "
                f"(declared: {param_count}, actual: {len(params)})"
            )

        # Validate mix_param_index
        mix_idx = engine.get('mix_param_index', -1)
        if mix_idx >= len(params):
            critical.append(
                f"Engine {eng_id} ({eng_name}): mix_param_index ({mix_idx}) "
                f"out of bounds (max: {len(params) - 1})"
            )

        # Check for generic parameter names
        generic_count = 0
        for param in params:
            name = param.get('name', '')
            if name.startswith('Param ') or name == 'Parameter':
                generic_count += 1

        if generic_count > 0:
            warnings.append(
                f"Engine {eng_id} ({eng_name}): {generic_count} generic parameter name(s)"
            )

        # Validate parameter ranges
        for idx, param in enumerate(params):
            default = param.get('default')
            min_val = param.get('min')
            max_val = param.get('max')

            if None not in [default, min_val, max_val]:
                if default < min_val or default > max_val:
                    critical.append(
                        f"Engine {eng_id} ({eng_name}), Param {idx}: "
                        f"default ({default}) outside range [{min_val}, {max_val}]"
                    )

    # Report results
    print("VALIDATION RESULTS:")
    print("-" * 60)

    if not critical and not warnings:
        print("✓ PASS - No issues found")
        print("\nKnowledge base is valid and ready to use.")
        return True

    if critical:
        print(f"\n✗ CRITICAL ISSUES ({len(critical)}):")
        for issue in critical:
            print(f"  - {issue}")

    if warnings:
        print(f"\n⚠ WARNINGS ({len(warnings)}):")
        for warning in warnings[:10]:  # Show first 10
            print(f"  - {warning}")
        if len(warnings) > 10:
            print(f"  ... and {len(warnings) - 10} more")

    print("\n" + "=" * 60)
    if critical:
        print("STATUS: FAILED - Critical issues must be fixed")
        return False
    else:
        print("STATUS: PASSED with warnings - Functional but could be improved")
        return True


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 validate_trinity_kb.py <path_to_json>")
        print("\nExample:")
        print("  python3 validate_trinity_kb.py trinity_engine_knowledge_COMPLETE.json")
        return 1

    filepath = Path(sys.argv[1])

    if not filepath.exists():
        print(f"ERROR: File not found: {filepath}")
        return 1

    result = validate_kb(filepath)
    return 0 if result else 1


if __name__ == '__main__':
    sys.exit(main())
