#!/usr/bin/env python3
"""
analyze_8_engines.py - Comprehensive regression analysis for 8 modified engines

Analyzes impulse responses and generates test matrix showing PASS/FAIL status
for engines: 39, 40, 52, 32, 49, 20, 33, 41
"""

import os
import sys
import csv
import numpy as np
from pathlib import Path

# Engine definitions
ENGINES = {
    39: "Spring Reverb",
    40: "Shimmer Reverb",
    52: "Pitch Shifter",
    32: "Harmonizer",
    49: "Detune Doubler",
    20: "Muff Fuzz",
    33: "Octave Up",
    41: "Convolution Reverb"
}

class EngineTestResult:
    def __init__(self, engine_id, engine_name):
        self.engine_id = engine_id
        self.engine_name = engine_name
        self.impulse_exists = False
        self.has_output = False
        self.peak_level = 0.0
        self.rms_level = 0.0
        self.has_nan = False
        self.has_inf = False
        self.excessive_peak = False
        self.silence = False
        self.passed = False
        self.fail_reason = ""

    def analyze(self, build_dir):
        """Analyze impulse response file for this engine"""
        impulse_file = build_dir / f"impulse_engine_{self.engine_id}.csv"

        if not impulse_file.exists():
            self.fail_reason = "No impulse response file"
            return

        self.impulse_exists = True

        try:
            # Read CSV file
            left_channel = []
            right_channel = []

            with open(impulse_file, 'r') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    try:
                        # Try both possible column names
                        left_val = row.get('L') or row.get('left')
                        right_val = row.get('R') or row.get('right')
                        if left_val and right_val:
                            left_channel.append(float(left_val))
                            right_channel.append(float(right_val))
                    except (ValueError, KeyError, TypeError):
                        continue

            if len(left_channel) == 0:
                self.fail_reason = "Empty impulse response"
                return

            # Convert to numpy arrays
            left = np.array(left_channel)
            right = np.array(right_channel)

            # Check for NaN
            if np.any(np.isnan(left)) or np.any(np.isnan(right)):
                self.has_nan = True
                self.fail_reason = "Contains NaN"
                return

            # Check for Inf
            if np.any(np.isinf(left)) or np.any(np.isinf(right)):
                self.has_inf = True
                self.fail_reason = "Contains Inf"
                return

            # Calculate peak level
            self.peak_level = max(np.max(np.abs(left)), np.max(np.abs(right)))

            # Calculate RMS level
            rms_left = np.sqrt(np.mean(left**2))
            rms_right = np.sqrt(np.mean(right**2))
            self.rms_level = (rms_left + rms_right) / 2.0

            # Check for silence (no meaningful output)
            if self.peak_level < 1e-6:
                self.silence = True
                self.fail_reason = "Silence (no output)"
                return

            # Check for excessive levels
            if self.peak_level > 10.0:
                self.excessive_peak = True
                self.fail_reason = f"Excessive peak: {self.peak_level:.2f}"
                return

            # Check that there is actual output
            if self.rms_level < 1e-6:
                self.fail_reason = "No meaningful signal"
                return

            self.has_output = True

            # All checks passed
            self.passed = True

        except Exception as e:
            self.fail_reason = f"Analysis error: {str(e)}"
            return

def print_header():
    """Print test header"""
    print("\n" + "═" * 80)
    print("     COMPREHENSIVE REGRESSION TEST - 8 MODIFIED ENGINES")
    print("═" * 80)
    print("\nEngines under test:")
    for engine_id, name in ENGINES.items():
        print(f"  • Engine {engine_id}: {name}")
    print()

def print_test_matrix(results):
    """Print detailed test matrix"""
    print("\n" + "═" * 80)
    print("                          TEST MATRIX")
    print("═" * 80)
    print()
    print(f"{'ID':<4} {'Engine':<25} {'Impulse':<8} {'Output':<8} {'Quality':<10} {'Result':<10}")
    print("-" * 80)

    for result in results:
        impulse_status = "✓ PASS" if result.impulse_exists else "✗ FAIL"
        output_status = "✓ PASS" if result.has_output else "✗ FAIL"

        quality = "✓ OK"
        if result.has_nan:
            quality = "✗ NaN"
        elif result.has_inf:
            quality = "✗ Inf"
        elif result.excessive_peak:
            quality = "✗ PEAK"
        elif result.silence:
            quality = "✗ SILENT"

        overall = "✓ PASS" if result.passed else "✗ FAIL"

        print(f"{result.engine_id:<4} {result.engine_name[:24]:<25} {impulse_status:<8} {output_status:<8} {quality:<10} {overall:<10}")

        if not result.passed and result.fail_reason:
            print(f"     └─ {result.fail_reason}")

    print()

def print_detailed_metrics(results):
    """Print detailed metrics"""
    print("═" * 80)
    print("                      DETAILED METRICS")
    print("═" * 80)
    print()
    print(f"{'ID':<4} {'Engine':<25} {'Peak':<12} {'RMS':<12} {'Status':<10}")
    print("-" * 80)

    for result in results:
        if result.impulse_exists:
            peak_str = f"{result.peak_level:.4f}"
            rms_str = f"{result.rms_level:.6f}"
            status = "✓ PASS" if result.passed else "✗ FAIL"
            print(f"{result.engine_id:<4} {result.engine_name[:24]:<25} {peak_str:<12} {rms_str:<12} {status:<10}")
        else:
            print(f"{result.engine_id:<4} {result.engine_name[:24]:<25} {'N/A':<12} {'N/A':<12} {'✗ FAIL':<10}")

    print()

def print_summary(results):
    """Print summary statistics"""
    total = len(results)
    passed = sum(1 for r in results if r.passed)
    failed = total - passed
    pass_rate = (passed / total) * 100 if total > 0 else 0

    print("═" * 80)
    print("                          SUMMARY")
    print("═" * 80)
    print(f"  Total Engines:     {total}")
    print(f"  Passed:            {passed} ({pass_rate:.1f}%)")
    print(f"  Failed:            {failed}")
    print()

    if passed == total:
        print("  ✓ ALL TESTS PASSED - NO REGRESSIONS DETECTED")
    else:
        print("  ✗ REGRESSIONS DETECTED")
        print("\n  Failed engines:")
        for result in results:
            if not result.passed:
                print(f"    • Engine {result.engine_id} ({result.engine_name}): {result.fail_reason}")

    print()

def identify_regressions(results):
    """Identify specific regressions"""
    print("═" * 80)
    print("                    REGRESSION ANALYSIS")
    print("═" * 80)
    print()

    regressions = []
    warnings = []

    for result in results:
        if not result.passed:
            regressions.append((result.engine_id, result.engine_name, result.fail_reason))
        elif result.peak_level > 5.0:
            warnings.append((result.engine_id, result.engine_name, f"High peak level: {result.peak_level:.2f}"))

    if regressions:
        print("⚠  CRITICAL REGRESSIONS:")
        for eng_id, name, reason in regressions:
            print(f"   • Engine {eng_id} ({name}): {reason}")
        print()

    if warnings:
        print("⚠  WARNINGS:")
        for eng_id, name, reason in warnings:
            print(f"   • Engine {eng_id} ({name}): {reason}")
        print()

    if not regressions and not warnings:
        print("✓ No regressions or warnings detected")
        print()

def save_csv_report(results, output_file):
    """Save results to CSV file"""
    with open(output_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['EngineID', 'EngineName', 'ImpulseExists', 'HasOutput',
                        'PeakLevel', 'RMSLevel', 'HasNaN', 'HasInf', 'ExcessivePeak',
                        'Passed', 'FailReason'])

        for result in results:
            writer.writerow([
                result.engine_id,
                result.engine_name,
                'Yes' if result.impulse_exists else 'No',
                'Yes' if result.has_output else 'No',
                f"{result.peak_level:.6f}",
                f"{result.rms_level:.6f}",
                'Yes' if result.has_nan else 'No',
                'Yes' if result.has_inf else 'No',
                'Yes' if result.excessive_peak else 'No',
                'PASS' if result.passed else 'FAIL',
                result.fail_reason
            ])

    print(f"CSV report saved: {output_file}")

def main():
    # Get build directory
    script_dir = Path(__file__).parent
    build_dir = script_dir / "build"

    if not build_dir.exists():
        print(f"ERROR: Build directory not found: {build_dir}")
        sys.exit(1)

    print_header()

    # Analyze each engine
    results = []
    for engine_id, engine_name in ENGINES.items():
        print(f"Analyzing Engine {engine_id} ({engine_name})...", end=" ")
        result = EngineTestResult(engine_id, engine_name)
        result.analyze(build_dir)
        results.append(result)

        if result.passed:
            print("✓ PASS")
        else:
            print(f"✗ FAIL ({result.fail_reason})")

    # Print reports
    print_test_matrix(results)
    print_detailed_metrics(results)
    print_summary(results)
    identify_regressions(results)

    # Save CSV report
    csv_file = build_dir / "regression_test_8engines.csv"
    save_csv_report(results, csv_file)

    # Exit code
    all_passed = all(r.passed for r in results)
    sys.exit(0 if all_passed else 1)

if __name__ == "__main__":
    main()
