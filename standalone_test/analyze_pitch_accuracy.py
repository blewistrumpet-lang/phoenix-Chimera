#!/usr/bin/env python3
"""
analyze_pitch_accuracy.py - Pitch Accuracy Analysis and Report Generator

Analyzes pitch accuracy test results for pitch shifter engines (32-38, 49-50)
Generates comprehensive report with statistics, visualizations, and quality ratings
"""

import os
import sys
import csv
import numpy as np
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt
from pathlib import Path
from collections import defaultdict

# Quality thresholds (in cents)
PROFESSIONAL_THRESHOLD = 1.0
EXCELLENT_THRESHOLD = 5.0
GOOD_THRESHOLD = 10.0
FAIR_THRESHOLD = 20.0

class PitchTestResult:
    """Container for pitch test result"""
    def __init__(self, row):
        self.engine_id = int(row['EngineID'])
        self.engine_name = row['EngineName']
        self.input_freq = float(row['InputFreq'])
        self.semitone_shift = int(row['SemitoneShift'])
        self.expected_freq = float(row['ExpectedFreq'])
        self.measured_freq = float(row['MeasuredFreq']) if row['MeasuredFreq'] != 'N/A' else 0.0
        self.cent_error = float(row['CentError']) if row['CentError'] != 'N/A' else 9999.0
        self.valid = row['Valid'] == 'YES'
        self.error_msg = row['ErrorMsg']

class EngineStats:
    """Statistics for a single engine"""
    def __init__(self, engine_id, engine_name):
        self.engine_id = engine_id
        self.engine_name = engine_name
        self.total_tests = 0
        self.valid_tests = 0
        self.cent_errors = []
        self.results_by_shift = defaultdict(list)
        self.results_by_freq = defaultdict(list)

    def add_result(self, result):
        """Add a test result"""
        self.total_tests += 1
        if result.valid:
            self.valid_tests += 1
            abs_error = abs(result.cent_error)
            self.cent_errors.append(abs_error)
            self.results_by_shift[result.semitone_shift].append(abs_error)
            self.results_by_freq[result.input_freq].append(abs_error)

    def get_avg_error(self):
        """Get average absolute cent error"""
        return np.mean(self.cent_errors) if self.cent_errors else 9999.0

    def get_max_error(self):
        """Get maximum absolute cent error"""
        return np.max(self.cent_errors) if self.cent_errors else 9999.0

    def get_std_dev(self):
        """Get standard deviation of cent errors"""
        return np.std(self.cent_errors) if len(self.cent_errors) > 1 else 0.0

    def get_quality_rating(self):
        """Get quality rating based on average error"""
        avg_error = self.get_avg_error()
        if avg_error < PROFESSIONAL_THRESHOLD:
            return "PROFESSIONAL"
        elif avg_error < EXCELLENT_THRESHOLD:
            return "EXCELLENT"
        elif avg_error < GOOD_THRESHOLD:
            return "GOOD"
        elif avg_error < FAIR_THRESHOLD:
            return "FAIR"
        else:
            return "POOR"

    def get_success_rate(self):
        """Get percentage of valid tests"""
        return (self.valid_tests / self.total_tests * 100) if self.total_tests > 0 else 0.0

def load_results(csv_file):
    """Load test results from CSV file"""
    results = []
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            results.append(PitchTestResult(row))
    return results

def calculate_engine_stats(results):
    """Calculate statistics for each engine"""
    engine_stats = {}
    for result in results:
        if result.engine_id not in engine_stats:
            engine_stats[result.engine_id] = EngineStats(result.engine_id, result.engine_name)
        engine_stats[result.engine_id].add_result(result)
    return engine_stats

def print_header():
    """Print report header"""
    print("\n" + "═" * 80)
    print("        PITCH ACCURACY ANALYSIS REPORT - PITCH SHIFTER ENGINES")
    print("═" * 80)
    print()

def print_overall_summary(engine_stats):
    """Print overall summary statistics"""
    print("═" * 80)
    print("                        OVERALL SUMMARY")
    print("═" * 80)
    print()

    total_tests = sum(stats.total_tests for stats in engine_stats.values())
    total_valid = sum(stats.valid_tests for stats in engine_stats.values())
    total_failed = total_tests - total_valid

    print(f"  Engines Tested:      {len(engine_stats)}")
    print(f"  Total Tests:         {total_tests}")
    print(f"  Valid Measurements:  {total_valid} ({total_valid/total_tests*100:.1f}%)")
    print(f"  Failed Measurements: {total_failed} ({total_failed/total_tests*100:.1f}%)")
    print()

    # Calculate overall statistics from valid measurements
    all_errors = []
    for stats in engine_stats.values():
        all_errors.extend(stats.cent_errors)

    if all_errors:
        print(f"  Overall Statistics:")
        print(f"    Average Error:     {np.mean(all_errors):.2f} cents")
        print(f"    Median Error:      {np.median(all_errors):.2f} cents")
        print(f"    Std Deviation:     {np.std(all_errors):.2f} cents")
        print(f"    Max Error:         {np.max(all_errors):.2f} cents")
        print(f"    Min Error:         {np.min(all_errors):.2f} cents")
    print()

def print_engine_summary_table(engine_stats):
    """Print summary table for all engines"""
    print("═" * 80)
    print("                    ENGINE SUMMARY TABLE")
    print("═" * 80)
    print()

    # Header
    print(f"{'ID':<4} {'Engine Name':<30} {'Valid':<8} {'Avg Error':<12} {'Max Error':<12} {'Rating':<15}")
    print("-" * 80)

    # Sort by engine ID
    for engine_id in sorted(engine_stats.keys()):
        stats = engine_stats[engine_id]
        valid_str = f"{stats.valid_tests}/{stats.total_tests}"
        avg_error = stats.get_avg_error()
        max_error = stats.get_max_error()
        rating = stats.get_quality_rating()

        # Color code rating
        if rating == "PROFESSIONAL":
            rating_display = "✓ " + rating
        elif rating == "EXCELLENT":
            rating_display = "✓ " + rating
        elif rating == "GOOD":
            rating_display = "✓ " + rating
        elif rating == "FAIR":
            rating_display = "⚠ " + rating
        else:
            rating_display = "✗ " + rating

        print(f"{engine_id:<4} {stats.engine_name[:29]:<30} {valid_str:<8} "
              f"{avg_error:>6.2f} cents  {max_error:>6.2f} cents  {rating_display:<15}")
    print()

def print_detailed_engine_report(stats):
    """Print detailed report for a single engine"""
    print("╔" + "═" * 78 + "╗")
    print(f"║  Engine {stats.engine_id}: {stats.engine_name[:63]:<63} ║")
    print("╚" + "═" * 78 + "╝")
    print()

    print(f"  Overall Statistics:")
    print(f"    Tests:             {stats.valid_tests} / {stats.total_tests} valid "
          f"({stats.get_success_rate():.1f}%)")
    print(f"    Average Error:     {stats.get_avg_error():.2f} cents")
    print(f"    Max Error:         {stats.get_max_error():.2f} cents")
    print(f"    Std Deviation:     {stats.get_std_dev():.2f} cents")
    print(f"    Quality Rating:    {stats.get_quality_rating()}")
    print()

    # Error by semitone shift
    if stats.results_by_shift:
        print(f"  Error by Semitone Shift:")
        for shift in sorted(stats.results_by_shift.keys()):
            errors = stats.results_by_shift[shift]
            avg_error = np.mean(errors)
            status = "✓" if avg_error < EXCELLENT_THRESHOLD else "⚠" if avg_error < FAIR_THRESHOLD else "✗"
            print(f"    {shift:+3d} st:  {avg_error:>6.2f} cents  {status}")
        print()

    # Error by input frequency
    if stats.results_by_freq:
        print(f"  Error by Input Frequency:")
        for freq in sorted(stats.results_by_freq.keys()):
            errors = stats.results_by_freq[freq]
            avg_error = np.mean(errors)
            status = "✓" if avg_error < EXCELLENT_THRESHOLD else "⚠" if avg_error < FAIR_THRESHOLD else "✗"
            print(f"    {int(freq):>4d} Hz:  {avg_error:>6.2f} cents  {status}")
        print()

    # Error distribution
    if stats.cent_errors:
        errors = np.array(stats.cent_errors)
        under_1 = np.sum(errors < 1.0)
        under_5 = np.sum(errors < 5.0)
        under_10 = np.sum(errors < 10.0)
        under_20 = np.sum(errors < 20.0)
        over_20 = np.sum(errors >= 20.0)

        print(f"  Error Distribution:")
        print(f"    < 1 cent:      {under_1:>3d} ({under_1/len(errors)*100:>5.1f}%)  Professional")
        print(f"    < 5 cents:     {under_5:>3d} ({under_5/len(errors)*100:>5.1f}%)  Excellent")
        print(f"    < 10 cents:    {under_10:>3d} ({under_10/len(errors)*100:>5.1f}%)  Good")
        print(f"    < 20 cents:    {under_20:>3d} ({under_20/len(errors)*100:>5.1f}%)  Fair")
        print(f"    >= 20 cents:   {over_20:>3d} ({over_20/len(errors)*100:>5.1f}%)  Poor")
        print()

def generate_plots(engine_stats, output_dir):
    """Generate visualization plots"""
    print("Generating plots...")

    # Create output directory
    output_dir = Path(output_dir)
    output_dir.mkdir(exist_ok=True)

    # Plot 1: Average error by engine
    fig, ax = plt.subplots(figsize=(12, 6))
    engine_ids = sorted(engine_stats.keys())
    engine_names = [engine_stats[id].engine_name[:20] for id in engine_ids]
    avg_errors = [engine_stats[id].get_avg_error() for id in engine_ids]

    colors = []
    for error in avg_errors:
        if error < PROFESSIONAL_THRESHOLD:
            colors.append('#2ecc71')  # Green
        elif error < EXCELLENT_THRESHOLD:
            colors.append('#3498db')  # Blue
        elif error < GOOD_THRESHOLD:
            colors.append('#f39c12')  # Orange
        elif error < FAIR_THRESHOLD:
            colors.append('#e74c3c')  # Red
        else:
            colors.append('#95a5a6')  # Gray

    bars = ax.bar(range(len(engine_ids)), avg_errors, color=colors)
    ax.set_xlabel('Engine', fontsize=12)
    ax.set_ylabel('Average Error (cents)', fontsize=12)
    ax.set_title('Pitch Accuracy by Engine (Lower is Better)', fontsize=14, fontweight='bold')
    ax.set_xticks(range(len(engine_ids)))
    ax.set_xticklabels([f"{id}\n{name}" for id, name in zip(engine_ids, engine_names)],
                       rotation=45, ha='right', fontsize=9)
    ax.axhline(y=PROFESSIONAL_THRESHOLD, color='g', linestyle='--', alpha=0.5, label='Professional (1¢)')
    ax.axhline(y=EXCELLENT_THRESHOLD, color='b', linestyle='--', alpha=0.5, label='Excellent (5¢)')
    ax.axhline(y=GOOD_THRESHOLD, color='orange', linestyle='--', alpha=0.5, label='Good (10¢)')
    ax.axhline(y=FAIR_THRESHOLD, color='r', linestyle='--', alpha=0.5, label='Fair (20¢)')
    ax.legend(loc='upper right')
    ax.grid(axis='y', alpha=0.3)
    plt.tight_layout()
    plt.savefig(output_dir / 'pitch_accuracy_by_engine.png', dpi=150)
    plt.close()

    # Plot 2: Error distribution histogram for all engines
    fig, ax = plt.subplots(figsize=(10, 6))
    all_errors = []
    for stats in engine_stats.values():
        all_errors.extend(stats.cent_errors)

    if all_errors:
        ax.hist(all_errors, bins=50, color='steelblue', edgecolor='black', alpha=0.7)
        ax.set_xlabel('Absolute Error (cents)', fontsize=12)
        ax.set_ylabel('Frequency', fontsize=12)
        ax.set_title('Distribution of Pitch Errors (All Engines)', fontsize=14, fontweight='bold')
        ax.axvline(x=PROFESSIONAL_THRESHOLD, color='g', linestyle='--', label='Professional (1¢)')
        ax.axvline(x=EXCELLENT_THRESHOLD, color='b', linestyle='--', label='Excellent (5¢)')
        ax.axvline(x=GOOD_THRESHOLD, color='orange', linestyle='--', label='Good (10¢)')
        ax.axvline(x=FAIR_THRESHOLD, color='r', linestyle='--', label='Fair (20¢)')
        ax.legend()
        ax.grid(axis='y', alpha=0.3)
        plt.tight_layout()
        plt.savefig(output_dir / 'pitch_error_distribution.png', dpi=150)
        plt.close()

    # Plot 3: Error by semitone shift (averaged across all engines)
    fig, ax = plt.subplots(figsize=(10, 6))
    all_shifts = set()
    for stats in engine_stats.values():
        all_shifts.update(stats.results_by_shift.keys())

    shifts = sorted(all_shifts)
    avg_errors_by_shift = []

    for shift in shifts:
        shift_errors = []
        for stats in engine_stats.values():
            if shift in stats.results_by_shift:
                shift_errors.extend(stats.results_by_shift[shift])
        avg_errors_by_shift.append(np.mean(shift_errors) if shift_errors else 0)

    ax.plot(shifts, avg_errors_by_shift, marker='o', linewidth=2, markersize=8, color='steelblue')
    ax.set_xlabel('Semitone Shift', fontsize=12)
    ax.set_ylabel('Average Error (cents)', fontsize=12)
    ax.set_title('Pitch Accuracy vs Semitone Shift (All Engines)', fontsize=14, fontweight='bold')
    ax.axhline(y=EXCELLENT_THRESHOLD, color='b', linestyle='--', alpha=0.5, label='Excellent (5¢)')
    ax.axhline(y=GOOD_THRESHOLD, color='orange', linestyle='--', alpha=0.5, label='Good (10¢)')
    ax.grid(alpha=0.3)
    ax.legend()
    plt.tight_layout()
    plt.savefig(output_dir / 'pitch_error_by_shift.png', dpi=150)
    plt.close()

    # Plot 4: Error by input frequency (averaged across all engines)
    fig, ax = plt.subplots(figsize=(10, 6))
    all_freqs = set()
    for stats in engine_stats.values():
        all_freqs.update(stats.results_by_freq.keys())

    freqs = sorted(all_freqs)
    avg_errors_by_freq = []

    for freq in freqs:
        freq_errors = []
        for stats in engine_stats.values():
            if freq in stats.results_by_freq:
                freq_errors.extend(stats.results_by_freq[freq])
        avg_errors_by_freq.append(np.mean(freq_errors) if freq_errors else 0)

    ax.plot(freqs, avg_errors_by_freq, marker='o', linewidth=2, markersize=8, color='steelblue')
    ax.set_xlabel('Input Frequency (Hz)', fontsize=12)
    ax.set_ylabel('Average Error (cents)', fontsize=12)
    ax.set_title('Pitch Accuracy vs Input Frequency (All Engines)', fontsize=14, fontweight='bold')
    ax.axhline(y=EXCELLENT_THRESHOLD, color='b', linestyle='--', alpha=0.5, label='Excellent (5¢)')
    ax.axhline(y=GOOD_THRESHOLD, color='orange', linestyle='--', alpha=0.5, label='Good (10¢)')
    ax.set_xscale('log')
    ax.grid(alpha=0.3)
    ax.legend()
    plt.tight_layout()
    plt.savefig(output_dir / 'pitch_error_by_frequency.png', dpi=150)
    plt.close()

    print(f"  Plots saved to: {output_dir}/")
    print()

def save_detailed_report(engine_stats, output_file):
    """Save detailed report to text file"""
    with open(output_file, 'w') as f:
        # Redirect stdout to file
        original_stdout = sys.stdout
        sys.stdout = f

        print_header()
        print_overall_summary(engine_stats)
        print_engine_summary_table(engine_stats)

        print("═" * 80)
        print("                    DETAILED ENGINE REPORTS")
        print("═" * 80)
        print()

        for engine_id in sorted(engine_stats.keys()):
            print_detailed_engine_report(engine_stats[engine_id])

        print("═" * 80)
        print("                         END OF REPORT")
        print("═" * 80)
        print()

        # Restore stdout
        sys.stdout = original_stdout

    print(f"Detailed report saved to: {output_file}")

def main():
    # Get paths
    script_dir = Path(__file__).parent
    build_dir = script_dir / "build"
    csv_file = build_dir / "pitch_accuracy_results.csv"

    if not csv_file.exists():
        print(f"ERROR: Results file not found: {csv_file}")
        print("Please run test_pitch_accuracy first to generate results.")
        sys.exit(1)

    print_header()
    print(f"Loading results from: {csv_file}")
    print()

    # Load and analyze results
    results = load_results(csv_file)
    engine_stats = calculate_engine_stats(results)

    print(f"Loaded {len(results)} test results for {len(engine_stats)} engines")
    print()

    # Print reports
    print_overall_summary(engine_stats)
    print_engine_summary_table(engine_stats)

    print("═" * 80)
    print("                    DETAILED ENGINE REPORTS")
    print("═" * 80)
    print()

    for engine_id in sorted(engine_stats.keys()):
        print_detailed_engine_report(engine_stats[engine_id])

    # Generate plots
    generate_plots(engine_stats, build_dir / "pitch_accuracy_plots")

    # Save detailed report
    report_file = build_dir / "pitch_accuracy_report.txt"
    save_detailed_report(engine_stats, report_file)

    print("═" * 80)
    print("                    ANALYSIS COMPLETE")
    print("═" * 80)
    print()

    # Exit code based on quality
    all_errors = []
    for stats in engine_stats.values():
        all_errors.extend(stats.cent_errors)

    if all_errors:
        avg_error = np.mean(all_errors)
        if avg_error < EXCELLENT_THRESHOLD:
            print(f"✓ Overall quality: EXCELLENT (avg error: {avg_error:.2f} cents)")
            sys.exit(0)
        elif avg_error < GOOD_THRESHOLD:
            print(f"✓ Overall quality: GOOD (avg error: {avg_error:.2f} cents)")
            sys.exit(0)
        else:
            print(f"⚠ Overall quality needs improvement (avg error: {avg_error:.2f} cents)")
            sys.exit(1)
    else:
        print("✗ No valid measurements")
        sys.exit(1)

if __name__ == "__main__":
    main()
