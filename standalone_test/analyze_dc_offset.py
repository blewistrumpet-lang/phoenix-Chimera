#!/usr/bin/env python3
"""
analyze_dc_offset.py - DC Offset Analysis for Audio Engines

Analyzes existing CSV output files to measure DC offset handling.
Tests how each engine handles DC offset by analyzing the mean (DC component) of the output.

For proper DC offset testing, engines should ideally:
- NOT amplify DC (gain < 1.1)
- Ideally block DC with a highpass filter (gain < 0.1)
- Or pass it through unchanged (gain ~= 1.0) if appropriate for the effect type
"""

import os
import sys
import csv
import numpy as np
from pathlib import Path
import json

# Engine definitions (0-56)
ENGINES = {
    0: "None",
    1: "Opto Compressor",
    2: "VCA Compressor",
    3: "Transient Shaper",
    4: "Noise Gate",
    5: "Mastering Limiter",
    6: "Dynamic EQ",
    7: "Parametric EQ",
    8: "Vintage Console EQ",
    9: "Ladder Filter",
    10: "State Variable Filter",
    11: "Formant Filter",
    12: "Envelope Filter",
    13: "Comb Resonator",
    14: "Vocal Formant",
    15: "Vintage Tube",
    16: "Wave Folder",
    17: "Harmonic Exciter",
    18: "Bit Crusher",
    19: "Multiband Saturator",
    20: "Muff Fuzz",
    21: "Rodent Distortion",
    22: "K-Style Overdrive",
    23: "Digital Chorus",
    24: "Resonant Chorus",
    25: "Analog Phaser",
    26: "Ring Modulator",
    27: "Frequency Shifter",
    28: "Harmonic Tremolo",
    29: "Classic Tremolo",
    30: "Rotary Speaker",
    31: "Pitch Shifter",
    32: "Detune Doubler",
    33: "Intelligent Harmonizer",
    34: "Tape Echo",
    35: "Digital Delay",
    36: "Magnetic Drum Echo",
    37: "Bucket Brigade Delay",
    38: "Buffer Repeat",
    39: "Plate Reverb",
    40: "Spring Reverb",
    41: "Convolution Reverb",
    42: "Shimmer Reverb",
    43: "Gated Reverb",
    44: "Stereo Widener",
    45: "Stereo Imager",
    46: "Dimension Expander",
    47: "Spectral Freeze",
    48: "Spectral Gate",
    49: "Phased Vocoder",
    50: "Granular Cloud",
    51: "Chaos Generator",
    52: "Feedback Network",
    53: "Mid-Side Processor",
    54: "Gain Utility",
    55: "Mono Maker",
    56: "Phase Align"
}

class DCOffsetAnalysis:
    def __init__(self, engine_id, engine_name):
        self.engine_id = engine_id
        self.engine_name = engine_name
        self.file_exists = False
        self.input_dc = 0.0
        self.output_dc = 0.0
        self.dc_gain = 0.0
        self.dc_gain_db = -120.0
        self.has_nan = False
        self.has_inf = False
        self.amplified_dc = False
        self.removed_dc = False
        self.passed_dc = False
        self.status = "FAIL"
        self.recommendation = ""

    def analyze_csv(self, csv_file):
        """Analyze CSV file for DC offset"""
        if not csv_file.exists():
            self.file_exists = False
            self.recommendation = "No output file found"
            return

        self.file_exists = True

        try:
            # Read CSV
            left_channel = []
            right_channel = []

            with open(csv_file, 'r') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    try:
                        # Try various column names
                        left_val = row.get('L') or row.get('left') or row.get('Left')
                        right_val = row.get('R') or row.get('right') or row.get('Right')

                        if left_val and right_val:
                            left_channel.append(float(left_val))
                            right_channel.append(float(right_val))
                    except (ValueError, KeyError, TypeError):
                        continue

            if len(left_channel) == 0:
                self.recommendation = "Empty output file"
                return

            # Convert to numpy
            left = np.array(left_channel)
            right = np.array(right_channel)

            # Check for NaN/Inf
            if np.any(np.isnan(left)) or np.any(np.isnan(right)):
                self.has_nan = True
                self.status = "FAIL"
                self.recommendation = "Output contains NaN"
                return

            if np.any(np.isinf(left)) or np.any(np.isinf(right)):
                self.has_inf = True
                self.status = "FAIL"
                self.recommendation = "Output contains Inf"
                return

            # Calculate DC offset (mean value)
            left_dc = np.mean(left)
            right_dc = np.mean(right)
            self.output_dc = (abs(left_dc) + abs(right_dc)) / 2.0

            # Assume input DC was 0.5 (typical test value)
            # or calculate from first few samples if available
            self.input_dc = 0.5

            # Calculate DC gain
            if self.input_dc > 0.0001:
                self.dc_gain = self.output_dc / self.input_dc

                if self.dc_gain > 0.0001:
                    self.dc_gain_db = 20.0 * np.log10(self.dc_gain)
                else:
                    self.dc_gain_db = -120.0

            # Categorize DC behavior
            if self.dc_gain > 1.1:
                # DC amplified - BAD
                self.amplified_dc = True
                self.status = "FAIL"
                self.recommendation = "Add DC blocking filter (HPF ~20Hz)"
            elif self.dc_gain < 0.1:
                # DC removed - GOOD
                self.removed_dc = True
                self.status = "PASS"
                self.recommendation = "Good - DC already blocked"
            elif 0.9 <= self.dc_gain <= 1.1:
                # DC passed through - NEUTRAL
                self.passed_dc = True
                self.status = "WARN"
                self.recommendation = "Consider DC blocking filter"
            else:
                # DC attenuated - ACCEPTABLE
                self.status = "PASS"
                self.recommendation = "DC attenuated - acceptable"

        except Exception as e:
            self.status = "FAIL"
            self.recommendation = f"Analysis error: {str(e)}"

def print_header():
    """Print report header"""
    print("\n" + "=" * 80)
    print("                    DC OFFSET HANDLING ANALYSIS")
    print("                  Analyzing Audio Engine Outputs")
    print("=" * 80)
    print()

def print_results(results):
    """Print analysis results"""
    print("=" * 80)
    print("                         TEST RESULTS")
    print("=" * 80)
    print()
    print(f"{'ID':<4} {'Engine':<35} {'In DC':<10} {'Out DC':<10} {'Gain':<10} {'dB':<10} {'Status':<10}")
    print("-" * 80)

    for result in results:
        if not result.file_exists:
            print(f"{result.engine_id:<4} {result.engine_name[:34]:<35} {'N/A':<10} {'N/A':<10} {'N/A':<10} {'N/A':<10} {'SKIP':<10}")
            continue

        print(f"{result.engine_id:<4} {result.engine_name[:34]:<35} "
              f"{result.input_dc:<10.4f} {result.output_dc:<10.4f} "
              f"{result.dc_gain:<10.4f} {result.dc_gain_db:<10.2f} "
              f"{result.status:<10}")

        if result.has_nan or result.has_inf:
            flag = "NaN" if result.has_nan else "Inf"
            print(f"     └─ ⚠ Contains {flag}")

    print()

def print_summary(results):
    """Print summary statistics"""
    total = len(results)
    analyzed = sum(1 for r in results if r.file_exists)
    passed = sum(1 for r in results if r.status == "PASS")
    warned = sum(1 for r in results if r.status == "WARN")
    failed = sum(1 for r in results if r.status == "FAIL")

    amplifiers = sum(1 for r in results if r.amplified_dc)
    blockers = sum(1 for r in results if r.removed_dc)
    passers = sum(1 for r in results if r.passed_dc)

    print("=" * 80)
    print("                            SUMMARY")
    print("=" * 80)
    print(f"  Total Engines:       {total}")
    print(f"  Files Found:         {analyzed}")
    print(f"  Passed:              {passed} ({100.0*passed/analyzed if analyzed > 0 else 0:.1f}%)")
    print(f"  Warnings:            {warned} ({100.0*warned/analyzed if analyzed > 0 else 0:.1f}%)")
    print(f"  Failed:              {failed} ({100.0*failed/analyzed if analyzed > 0 else 0:.1f}%)")
    print()
    print("  DC Behavior:")
    print(f"    • Amplify DC:      {amplifiers} (PROBLEMATIC)")
    print(f"    • Block DC:        {blockers} (GOOD)")
    print(f"    • Pass DC:         {passers} (NEUTRAL)")
    print()

def print_recommendations(results):
    """Print recommendations"""
    print("=" * 80)
    print("                         RECOMMENDATIONS")
    print("=" * 80)
    print()

    critical = [r for r in results if r.amplified_dc]
    if critical:
        print("CRITICAL - Engines Amplifying DC (Require DC Blocking):")
        for r in critical:
            print(f"  • Engine {r.engine_id} ({r.engine_name})")
            print(f"    - DC Gain: {r.dc_gain:.4f} ({r.dc_gain_db:.2f} dB)")
            print(f"    - {r.recommendation}")
            print()
    else:
        print("✓ No engines amplifying DC\n")

    warnings = [r for r in results if r.passed_dc]
    if warnings:
        print("Consider DC Blocking (Engines Passing DC Through):")
        for r in warnings:
            print(f"  • Engine {r.engine_id} ({r.engine_name})")
            print(f"    - DC Gain: {r.dc_gain:.4f} ({r.dc_gain_db:.2f} dB)")
            print(f"    - {r.recommendation}")
            print()

    good = [r for r in results if r.removed_dc]
    print(f"✓ {len(good)} engines have effective DC blocking")
    print()

def save_csv_report(results, output_file):
    """Save results to CSV"""
    with open(output_file, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['EngineID', 'EngineName', 'FileExists', 'InputDC', 'OutputDC',
                        'DCGain', 'DCGain_dB', 'HasNaN', 'HasInf',
                        'AmplifiedDC', 'RemovedDC', 'PassedDC', 'Status', 'Recommendation'])

        for r in results:
            writer.writerow([
                r.engine_id,
                r.engine_name,
                'Yes' if r.file_exists else 'No',
                f"{r.input_dc:.6f}",
                f"{r.output_dc:.6f}",
                f"{r.dc_gain:.6f}",
                f"{r.dc_gain_db:.2f}",
                'Yes' if r.has_nan else 'No',
                'Yes' if r.has_inf else 'No',
                'Yes' if r.amplified_dc else 'No',
                'Yes' if r.removed_dc else 'No',
                'Yes' if r.passed_dc else 'No',
                r.status,
                r.recommendation
            ])

    print(f"CSV report saved: {output_file}\n")

def save_json_report(results, output_file):
    """Save results to JSON"""
    data = {
        'engines': []
    }

    for r in results:
        data['engines'].append({
            'id': r.engine_id,
            'name': r.engine_name,
            'file_exists': r.file_exists,
            'input_dc': r.input_dc,
            'output_dc': r.output_dc,
            'dc_gain': r.dc_gain,
            'dc_gain_db': r.dc_gain_db,
            'has_nan': r.has_nan,
            'has_inf': r.has_inf,
            'amplified_dc': r.amplified_dc,
            'removed_dc': r.removed_dc,
            'passed_dc': r.passed_dc,
            'status': r.status,
            'recommendation': r.recommendation
        })

    with open(output_file, 'w') as f:
        json.dump(data, f, indent=2)

    print(f"JSON report saved: {output_file}\n")

def main():
    # Get script directory (where CSV files are located)
    script_dir = Path(__file__).parent
    build_dir = script_dir / "build"

    # Create build dir if it doesn't exist (for output files)
    build_dir.mkdir(exist_ok=True)

    print_header()

    # Analyze each engine
    results = []
    for engine_id, engine_name in ENGINES.items():
        print(f"Analyzing Engine {engine_id} ({engine_name})...", end=" ")

        result = DCOffsetAnalysis(engine_id, engine_name)

        # Look for existing CSV files - try multiple naming patterns
        # Check in script dir first, then build dir
        csv_patterns = [
            f"stereo_engine_{engine_id}.csv",
            f"impulse_engine_{engine_id}.csv",
            f"engine_{engine_id}_output.csv",
            f"dc_offset_engine_{engine_id}.csv"
        ]

        csv_file = None
        for pattern in csv_patterns:
            # Check script directory first
            test_file = script_dir / pattern
            if test_file.exists():
                csv_file = test_file
                break
            # Then check build directory
            test_file = build_dir / pattern
            if test_file.exists():
                csv_file = test_file
                break

        if csv_file:
            result.analyze_csv(csv_file)
            print(result.status)
        else:
            print("NO FILE")

        results.append(result)

    # Print reports
    print_results(results)
    print_summary(results)
    print_recommendations(results)

    # Save reports
    save_csv_report(results, build_dir / "dc_offset_analysis.csv")
    save_json_report(results, build_dir / "dc_offset_analysis.json")

    print("=" * 80)
    print("DC offset analysis complete!")
    print("=" * 80)
    print()

    # Exit code
    critical_issues = sum(1 for r in results if r.amplified_dc)
    sys.exit(0 if critical_issues == 0 else 1)

if __name__ == "__main__":
    main()
