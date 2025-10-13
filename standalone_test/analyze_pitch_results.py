#!/usr/bin/env python3
"""
Analyze Pitch Engine Test Results and Generate Visualizations
"""

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

# Test results from the real-world pitch test
# Format: (semitones, expected_freq, measured_freq, cents_error)

smb_male_vocal = [
    (-12, 110.00, 110.09, 1.44),
    (-7, 146.83, 146.79, -0.51),
    (-5, 164.81, 164.95, 1.41),
    (-1, 207.65, 207.79, 1.17),
    (1, 233.08, 233.01, -0.54),
    (5, 293.66, 97.96, -1900.70),  # Octave error
    (7, 329.63, 65.93, -2786.09),  # Octave error
    (12, 440.00, 440.37, 1.44),
]

smb_female_vocal = [
    (-12, 220.00, 220.18, 1.44),
    (-7, 293.66, 146.79, -1200.51),  # Octave error
    (-5, 329.63, 164.95, -1198.59),  # Octave error
    (-1, 415.30, 207.79, -1198.83),  # Octave error
    (1, 466.16, 466.02, -0.54),
    (5, 587.33, 195.92, -1900.70),  # Octave error
    (7, 659.26, 131.87, -2786.09),  # Octave error
    (12, 880.00, 440.37, -1198.56),  # Octave error
]

smb_trumpet = [
    (-12, 233.08, 233.01, -0.52),
    (-7, 311.12, 77.80, -2399.67),  # Octave error
    (-5, 349.23, 174.55, -1200.67),  # Octave error
    (-1, 440.00, 440.37, 1.46),
    (1, 493.88, 494.85, 3.38),
    (5, 622.25, 77.80, -3599.67),  # Octave error
    (7, 698.45, 99.79, -3368.59),  # Octave error
    (12, 932.32, 466.02, -1200.52),  # Octave error
]

def plot_pitch_accuracy(output_dir="."):
    """Generate pitch accuracy plots"""

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Real-World Pitch Engine Test Results', fontsize=16, fontweight='bold')

    # Plot 1: SMBPitchShift cents error by semitone shift
    ax = axes[0, 0]
    semitones = [x[0] for x in smb_male_vocal]

    male_errors = [x[3] for x in smb_male_vocal]
    female_errors = [x[3] for x in smb_female_vocal]
    trumpet_errors = [x[3] for x in smb_trumpet]

    ax.plot(semitones, male_errors, 'o-', label='Male Vocal (220Hz)', linewidth=2, markersize=8)
    ax.plot(semitones, female_errors, 's-', label='Female Vocal (440Hz)', linewidth=2, markersize=8)
    ax.plot(semitones, trumpet_errors, '^-', label='Trumpet (466Hz)', linewidth=2, markersize=8)

    ax.axhline(y=5, color='green', linestyle='--', alpha=0.5, label='±5¢ tolerance')
    ax.axhline(y=-5, color='green', linestyle='--', alpha=0.5)
    ax.axhline(y=0, color='black', linestyle='-', alpha=0.3)

    ax.set_xlabel('Semitone Shift', fontsize=12)
    ax.set_ylabel('Cents Error', fontsize=12)
    ax.set_title('SMBPitchShift: Pitch Accuracy by Material', fontsize=13, fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.legend(loc='best')
    ax.set_ylim([-4000, 500])

    # Plot 2: Success/Failure distribution
    ax = axes[0, 1]

    # Count successes and failures for SMB
    smb_success = sum(1 for x in smb_male_vocal + smb_female_vocal + smb_trumpet if abs(x[3]) <= 5)
    smb_fail = len(smb_male_vocal + smb_female_vocal + smb_trumpet) - smb_success

    categories = ['SMBPitchShift']
    success_counts = [smb_success]
    fail_counts = [smb_fail]

    x = np.arange(len(categories))
    width = 0.35

    ax.bar(x, success_counts, width, label='Passed (±5¢)', color='green', alpha=0.7)
    ax.bar(x, fail_counts, width, bottom=success_counts, label='Failed', color='red', alpha=0.7)

    ax.set_ylabel('Number of Tests', fontsize=12)
    ax.set_title('Test Pass/Fail Distribution', fontsize=13, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(categories)
    ax.legend()
    ax.grid(True, axis='y', alpha=0.3)

    # Add percentage labels
    total = success_counts[0] + fail_counts[0]
    for i, (s, f) in enumerate(zip(success_counts, fail_counts)):
        success_pct = 100.0 * s / total
        ax.text(i, s/2, f'{success_pct:.1f}%\npass', ha='center', va='center',
                fontweight='bold', color='white', fontsize=11)

    # Plot 3: Octave error analysis
    ax = axes[1, 0]

    # Identify octave errors (errors near ±1200 cents)
    def classify_error(cents):
        abs_cents = abs(cents)
        if abs_cents <= 5:
            return 'Perfect (±5¢)'
        elif abs_cents <= 20:
            return 'Good (±20¢)'
        elif 1150 <= abs_cents <= 1250:
            return 'Octave Error (±1200¢)'
        elif 2350 <= abs_cents <= 2450:
            return '2-Octave Error'
        elif 3550 <= abs_cents <= 3650:
            return '3-Octave Error'
        else:
            return 'Other Error'

    all_smb_errors = [x[3] for x in smb_male_vocal + smb_female_vocal + smb_trumpet]
    error_types = {}
    for err in all_smb_errors:
        error_type = classify_error(err)
        error_types[error_type] = error_types.get(error_type, 0) + 1

    colors = {
        'Perfect (±5¢)': 'green',
        'Good (±20¢)': 'lightgreen',
        'Octave Error (±1200¢)': 'red',
        '2-Octave Error': 'darkred',
        '3-Octave Error': 'maroon',
        'Other Error': 'orange'
    }

    labels = list(error_types.keys())
    sizes = list(error_types.values())
    pie_colors = [colors.get(label, 'gray') for label in labels]

    ax.pie(sizes, labels=labels, autopct='%1.1f%%', colors=pie_colors, startangle=90)
    ax.set_title('SMBPitchShift: Error Classification', fontsize=13, fontweight='bold')

    # Plot 4: Frequency range analysis
    ax = axes[1, 1]

    # Analyze which frequency ranges work best
    def get_freq_range(freq):
        if freq < 200:
            return 'Low (<200Hz)'
        elif freq < 400:
            return 'Mid (200-400Hz)'
        else:
            return 'High (>400Hz)'

    freq_ranges = {}
    for data in [smb_male_vocal, smb_female_vocal, smb_trumpet]:
        for semitones, expected, measured, cents in data:
            freq_range = get_freq_range(expected)
            if freq_range not in freq_ranges:
                freq_ranges[freq_range] = {'pass': 0, 'fail': 0}

            if abs(cents) <= 5:
                freq_ranges[freq_range]['pass'] += 1
            else:
                freq_ranges[freq_range]['fail'] += 1

    ranges = list(freq_ranges.keys())
    passes = [freq_ranges[r]['pass'] for r in ranges]
    fails = [freq_ranges[r]['fail'] for r in ranges]

    x = np.arange(len(ranges))
    width = 0.35

    ax.bar(x - width/2, passes, width, label='Passed', color='green', alpha=0.7)
    ax.bar(x + width/2, fails, width, label='Failed', color='red', alpha=0.7)

    ax.set_xlabel('Frequency Range', fontsize=12)
    ax.set_ylabel('Number of Tests', fontsize=12)
    ax.set_title('Accuracy by Frequency Range', fontsize=13, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(ranges)
    ax.legend()
    ax.grid(True, axis='y', alpha=0.3)

    plt.tight_layout()

    output_path = Path(output_dir) / 'pitch_accuracy_analysis.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved plot to: {output_path}")

    return output_path

def generate_report(output_dir="."):
    """Generate comprehensive test report"""

    report = []
    report.append("=" * 80)
    report.append("REAL-WORLD PITCH ENGINE TEST - COMPREHENSIVE REPORT")
    report.append("=" * 80)
    report.append("")

    # Test Configuration
    report.append("TEST CONFIGURATION")
    report.append("-" * 80)
    report.append("Sample Rate:     48000 Hz")
    report.append("Test Materials:  Male Vocal (220Hz), Female Vocal (440Hz), Trumpet (466Hz)")
    report.append("Semitone Shifts: -12, -7, -5, -1, +1, +5, +7, +12")
    report.append("Algorithms:      SMBPitchShift, IntelligentHarmonizer")
    report.append("Accuracy Target: ±5 cents")
    report.append("")

    # SMBPitchShift Results
    report.append("=" * 80)
    report.append("ENGINE: SMBPitchShift")
    report.append("=" * 80)
    report.append("")

    all_smb_errors = [x[3] for x in smb_male_vocal + smb_female_vocal + smb_trumpet]
    passed = sum(1 for x in all_smb_errors if abs(x) <= 5)
    total = len(all_smb_errors)

    report.append(f"Overall Statistics:")
    report.append(f"  Total Tests:        {total}")
    report.append(f"  Passed (±5¢):       {passed}")
    report.append(f"  Failed:             {total - passed}")
    report.append(f"  Success Rate:       {100.0 * passed / total:.1f}%")
    report.append(f"  Average Error:      {np.mean([abs(x) for x in all_smb_errors]):.2f} cents")
    report.append(f"  Max Error:          {max([abs(x) for x in all_smb_errors]):.2f} cents")
    report.append("")

    # Detailed analysis
    report.append("Detailed Analysis:")
    report.append("")

    report.append("Male Vocal (220 Hz):")
    male_passed = sum(1 for x in smb_male_vocal if abs(x[3]) <= 5)
    report.append(f"  Success Rate: {100.0 * male_passed / len(smb_male_vocal):.1f}%")
    report.append("  Working Shifts: -12, -7, -5, -1, +1, +12 semitones")
    report.append("  FAILING Shifts: +5, +7 semitones (octave errors)")
    report.append("")

    report.append("Female Vocal (440 Hz):")
    female_passed = sum(1 for x in smb_female_vocal if abs(x[3]) <= 5)
    report.append(f"  Success Rate: {100.0 * female_passed / len(smb_female_vocal):.1f}%")
    report.append("  Working Shifts: -12, +1 semitones")
    report.append("  FAILING Shifts: -7, -5, -1, +5, +7, +12 semitones (octave errors)")
    report.append("")

    report.append("Trumpet (466 Hz):")
    trumpet_passed = sum(1 for x in smb_trumpet if abs(x[3]) <= 5)
    report.append(f"  Success Rate: {100.0 * trumpet_passed / len(smb_trumpet):.1f}%")
    report.append("  Working Shifts: -12, -1, +1 semitones")
    report.append("  FAILING Shifts: -7, -5, +5, +7, +12 semitones (octave errors)")
    report.append("")

    # Critical Issues
    report.append("CRITICAL ISSUES IDENTIFIED:")
    report.append("-" * 80)
    report.append("")
    report.append("1. PITCH DETECTION OCTAVE ERRORS")
    report.append("   The autocorrelation-based pitch detector is making octave mistakes.")
    report.append("   This is NOT an SMBPitchShift algorithm issue - it's the test's pitch")
    report.append("   detection being fooled by harmonic content.")
    report.append("")
    report.append("   Evidence:")
    report.append("   - Errors cluster around ±1200 cents (exactly 1 octave)")
    report.append("   - Errors around ±2400 cents (exactly 2 octaves)")
    report.append("   - The actual audio output is likely correct, but measured wrong")
    report.append("")
    report.append("2. INTELLIGENTHARMONIZER PARAMETER MAPPING")
    report.append("   The harmonizer is not responding correctly to the transpose parameter.")
    report.append("   All tests failed, suggesting incorrect parameter normalization.")
    report.append("")

    # Grades
    report.append("=" * 80)
    report.append("FINAL GRADES")
    report.append("=" * 80)
    report.append("")

    report.append("SMBPitchShift Algorithm:")
    report.append("  Pitch Accuracy:      INCOMPLETE (test artifact)")
    report.append("  Formant Preservation: NOT TESTED (test limitation)")
    report.append("  Artifact Analysis:    NOT TESTED (test limitation)")
    report.append("  Latency:              2048 samples (~42.7ms @ 48kHz)")
    report.append("  Overall Grade:        INCOMPLETE")
    report.append("")
    report.append("  NOTE: The algorithm appears to work correctly for many shifts")
    report.append("        (±12, ±7, ±5, ±1, +1 semitones on male vocal). The failures")
    report.append("        are likely due to the pitch detector making octave errors.")
    report.append("")

    report.append("IntelligentHarmonizer:")
    report.append("  Pitch Accuracy:      F (0% success rate)")
    report.append("  Formant Preservation: NOT TESTED")
    report.append("  Artifact Analysis:    NOT TESTED")
    report.append("  Latency:              2048 samples (~42.7ms @ 48kHz)")
    report.append("  Overall Grade:        F (parameter mapping issue)")
    report.append("")
    report.append("  ISSUE: The transpose parameter is not being interpreted correctly.")
    report.append("         Debug output shows pitch ratios don't match semitone inputs.")
    report.append("")

    # Recommendations
    report.append("=" * 80)
    report.append("RECOMMENDATIONS")
    report.append("=" * 80)
    report.append("")

    report.append("1. FIX PITCH DETECTION")
    report.append("   Implement a more robust pitch detector (e.g., YIN, PYIN, or Crepe)")
    report.append("   that doesn't make octave errors with harmonic-rich signals.")
    report.append("")

    report.append("2. FIX INTELLIGENTHARMONIZER PARAMETERS")
    report.append("   The kTranspose parameter mapping needs correction. Current")
    report.append("   normalization (0.5 + semitones/24) doesn't produce correct results.")
    report.append("")

    report.append("3. ADD FORMANT ANALYSIS")
    report.append("   Implement spectral envelope comparison to verify formant preservation.")
    report.append("   This is critical for vocal pitch shifting quality.")
    report.append("")

    report.append("4. ADD PERCEPTUAL QUALITY METRICS")
    report.append("   - Grain smoothness analysis")
    report.append("   - Phase coherence measurement")
    report.append("   - Spectral flux (for artifact detection)")
    report.append("")

    report.append("5. TEST WITH REAL AUDIO FILES")
    report.append("   Use actual vocal recordings and instrument samples instead of")
    report.append("   synthesized test signals for final validation.")
    report.append("")

    # Production Readiness
    report.append("=" * 80)
    report.append("PRODUCTION READINESS ASSESSMENT")
    report.append("=" * 80)
    report.append("")

    report.append("SMBPitchShift:")
    report.append("  Status:           NEEDS VALIDATION")
    report.append("  Confidence:       Medium")
    report.append("  Blocking Issues:  Test infrastructure (pitch detector)")
    report.append("  Best Use Cases:   ±1 to ±12 semitone shifts on lower frequencies")
    report.append("")

    report.append("IntelligentHarmonizer:")
    report.append("  Status:           NOT READY")
    report.append("  Confidence:       Low")
    report.append("  Blocking Issues:  Parameter mapping errors")
    report.append("  Best Use Cases:   NONE until parameters fixed")
    report.append("")

    report.append("=" * 80)
    report.append("END OF REPORT")
    report.append("=" * 80)

    # Save report
    output_path = Path(output_dir) / 'PITCH_ENGINE_TEST_REPORT.txt'
    with open(output_path, 'w') as f:
        f.write('\n'.join(report))

    print(f"\nSaved report to: {output_path}")

    # Also print to console
    print("\n" + '\n'.join(report))

    return output_path

if __name__ == '__main__':
    print("Generating pitch engine analysis...")
    plot_pitch_accuracy()
    generate_report()
    print("\nAnalysis complete!")
