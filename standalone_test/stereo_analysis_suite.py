#!/usr/bin/env python3
"""
stereo_analysis_suite.py - Comprehensive Stereo Analysis for ALL 56 Engines

This script performs detailed stereo analysis including:
- L/R Correlation (Pearson correlation coefficient)
- Stereo Width (difference between channels)
- Phase Coherence (phase relationship between channels)
- Mid/Side Analysis
- Mono Compatibility Check
- Identification of mono-collapsed engines (<0.8 correlation when should be stereo)

Generates:
- Detailed stereo quality report (CSV)
- Visual stereo quality matrix
- Identification of problem engines
"""

import os
import sys
import csv
import numpy as np
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import json

# All 56 engines with metadata
ALL_ENGINES = {
    # ENGINE_NONE (0)
    0: {"name": "None (Bypass)", "category": "Utility", "expected_stereo": False},

    # DYNAMICS & COMPRESSION (1-6)
    1: {"name": "Vintage Opto Compressor", "category": "Dynamics", "expected_stereo": True},
    2: {"name": "Classic VCA Compressor", "category": "Dynamics", "expected_stereo": True},
    3: {"name": "Transient Shaper", "category": "Dynamics", "expected_stereo": True},
    4: {"name": "Noise Gate", "category": "Dynamics", "expected_stereo": True},
    5: {"name": "Mastering Limiter", "category": "Dynamics", "expected_stereo": True},
    6: {"name": "Dynamic EQ", "category": "Dynamics", "expected_stereo": True},

    # FILTERS & EQ (7-14)
    7: {"name": "Parametric EQ (Studio)", "category": "Filter", "expected_stereo": True},
    8: {"name": "Vintage Console EQ", "category": "Filter", "expected_stereo": True},
    9: {"name": "Ladder Filter", "category": "Filter", "expected_stereo": True},
    10: {"name": "State Variable Filter", "category": "Filter", "expected_stereo": True},
    11: {"name": "Formant Filter", "category": "Filter", "expected_stereo": True},
    12: {"name": "Envelope Filter", "category": "Filter", "expected_stereo": True},
    13: {"name": "Comb Resonator", "category": "Filter", "expected_stereo": True},
    14: {"name": "Vocal Formant Filter", "category": "Filter", "expected_stereo": True},

    # DISTORTION & SATURATION (15-22)
    15: {"name": "Vintage Tube Preamp", "category": "Distortion", "expected_stereo": True},
    16: {"name": "Wave Folder", "category": "Distortion", "expected_stereo": True},
    17: {"name": "Harmonic Exciter", "category": "Distortion", "expected_stereo": True},
    18: {"name": "Bit Crusher", "category": "Distortion", "expected_stereo": True},
    19: {"name": "Multiband Saturator", "category": "Distortion", "expected_stereo": True},
    20: {"name": "Muff Fuzz", "category": "Distortion", "expected_stereo": True},
    21: {"name": "Rodent Distortion", "category": "Distortion", "expected_stereo": True},
    22: {"name": "K-Style Overdrive", "category": "Distortion", "expected_stereo": True},

    # MODULATION (23-33)
    23: {"name": "Digital Chorus", "category": "Modulation", "expected_stereo": True},
    24: {"name": "Resonant Chorus", "category": "Modulation", "expected_stereo": True},
    25: {"name": "Analog Phaser", "category": "Modulation", "expected_stereo": True},
    26: {"name": "Ring Modulator", "category": "Modulation", "expected_stereo": True},
    27: {"name": "Frequency Shifter", "category": "Modulation", "expected_stereo": True},
    28: {"name": "Harmonic Tremolo", "category": "Modulation", "expected_stereo": True},
    29: {"name": "Classic Tremolo", "category": "Modulation", "expected_stereo": True},
    30: {"name": "Rotary Speaker", "category": "Modulation", "expected_stereo": True},
    31: {"name": "Pitch Shifter", "category": "Modulation", "expected_stereo": True},
    32: {"name": "Detune Doubler", "category": "Modulation", "expected_stereo": True},
    33: {"name": "Intelligent Harmonizer", "category": "Modulation", "expected_stereo": True},

    # DELAY (34-38)
    34: {"name": "Tape Echo", "category": "Delay", "expected_stereo": True},
    35: {"name": "Digital Delay", "category": "Delay", "expected_stereo": True},
    36: {"name": "Magnetic Drum Echo", "category": "Delay", "expected_stereo": True},
    37: {"name": "Bucket Brigade Delay", "category": "Delay", "expected_stereo": True},
    38: {"name": "Buffer Repeat", "category": "Delay", "expected_stereo": True},

    # REVERB (39-43)
    39: {"name": "Plate Reverb", "category": "Reverb", "expected_stereo": True},
    40: {"name": "Spring Reverb", "category": "Reverb", "expected_stereo": True},
    41: {"name": "Convolution Reverb", "category": "Reverb", "expected_stereo": True},
    42: {"name": "Shimmer Reverb", "category": "Reverb", "expected_stereo": True},
    43: {"name": "Gated Reverb", "category": "Reverb", "expected_stereo": True},

    # SPATIAL & SPECIAL (44-52)
    44: {"name": "Stereo Widener", "category": "Spatial", "expected_stereo": True},
    45: {"name": "Stereo Imager", "category": "Spatial", "expected_stereo": True},
    46: {"name": "Dimension Expander", "category": "Spatial", "expected_stereo": True},
    47: {"name": "Spectral Freeze", "category": "Special", "expected_stereo": True},
    48: {"name": "Spectral Gate", "category": "Special", "expected_stereo": True},
    49: {"name": "Phased Vocoder", "category": "Special", "expected_stereo": True},
    50: {"name": "Granular Cloud", "category": "Special", "expected_stereo": True},
    51: {"name": "Chaos Generator", "category": "Special", "expected_stereo": True},
    52: {"name": "Feedback Network", "category": "Special", "expected_stereo": True},

    # UTILITY (53-56)
    53: {"name": "Mid-Side Processor", "category": "Utility", "expected_stereo": True},
    54: {"name": "Gain Utility", "category": "Utility", "expected_stereo": True},
    55: {"name": "Mono Maker", "category": "Utility", "expected_stereo": False},  # Intentionally mono
    56: {"name": "Phase Align", "category": "Utility", "expected_stereo": True}
}


class StereoAnalysisResult:
    """Results of stereo analysis for a single engine"""

    def __init__(self, engine_id: int, engine_name: str, category: str, expected_stereo: bool):
        self.engine_id = engine_id
        self.engine_name = engine_name
        self.category = category
        self.expected_stereo = expected_stereo

        # Analysis metrics
        self.has_data = False
        self.lr_correlation = 0.0
        self.stereo_width = 0.0
        self.phase_coherence = 0.0
        self.mid_level = 0.0
        self.side_level = 0.0
        self.mid_side_ratio = 0.0
        self.mono_compatibility = 0.0
        self.peak_l = 0.0
        self.peak_r = 0.0
        self.rms_l = 0.0
        self.rms_r = 0.0
        self.balance_ratio = 1.0  # L/R balance

        # Quality flags
        self.is_mono_collapsed = False
        self.is_phase_reversed = False
        self.is_significantly_imbalanced = False
        self.has_stereo_content = False

        # Overall status
        self.status = "UNKNOWN"
        self.quality_grade = "?"
        self.warnings = []
        self.error_message = ""


def calculate_correlation(left: np.ndarray, right: np.ndarray) -> float:
    """Calculate Pearson correlation coefficient between L and R channels"""
    if len(left) == 0 or len(right) == 0:
        return 0.0

    # Remove DC offset
    left_centered = left - np.mean(left)
    right_centered = right - np.mean(right)

    # Calculate correlation
    numerator = np.sum(left_centered * right_centered)
    denominator = np.sqrt(np.sum(left_centered**2) * np.sum(right_centered**2))

    if denominator < 1e-10:
        return 1.0  # Both channels silent

    correlation = numerator / denominator
    return np.clip(correlation, -1.0, 1.0)


def calculate_stereo_width(left: np.ndarray, right: np.ndarray) -> float:
    """Calculate stereo width (0 = mono, 1 = full stereo, >1 = wide stereo)"""
    mid = (left + right) / 2.0
    side = (left - right) / 2.0

    mid_rms = np.sqrt(np.mean(mid**2))
    side_rms = np.sqrt(np.mean(side**2))

    if mid_rms < 1e-10:
        return 0.0

    # Stereo width metric
    width = side_rms / mid_rms
    return width


def calculate_phase_coherence(left: np.ndarray, right: np.ndarray) -> float:
    """Calculate phase coherence (1.0 = in phase, -1.0 = out of phase)"""
    # Use cross-correlation to measure phase relationship
    correlation = np.correlate(left, right, mode='valid')[0]
    auto_corr_l = np.correlate(left, left, mode='valid')[0]
    auto_corr_r = np.correlate(right, right, mode='valid')[0]

    if auto_corr_l < 1e-10 or auto_corr_r < 1e-10:
        return 1.0

    coherence = correlation / np.sqrt(auto_corr_l * auto_corr_r)
    return np.clip(coherence, -1.0, 1.0)


def calculate_mid_side_analysis(left: np.ndarray, right: np.ndarray) -> Tuple[float, float, float]:
    """Calculate mid/side levels and ratio"""
    mid = (left + right) / 2.0
    side = (left - right) / 2.0

    mid_rms = np.sqrt(np.mean(mid**2))
    side_rms = np.sqrt(np.mean(side**2))

    # Mid/side ratio (in dB)
    if mid_rms > 1e-10 and side_rms > 1e-10:
        ratio = 20 * np.log10(side_rms / mid_rms)
    else:
        ratio = -100.0

    return mid_rms, side_rms, ratio


def calculate_mono_compatibility(left: np.ndarray, right: np.ndarray) -> float:
    """Calculate mono compatibility (1.0 = perfect, 0.0 = phase cancellation)"""
    mono_sum = left + right
    mono_rms = np.sqrt(np.mean(mono_sum**2))

    left_rms = np.sqrt(np.mean(left**2))
    right_rms = np.sqrt(np.mean(right**2))

    expected_mono_rms = left_rms + right_rms

    if expected_mono_rms < 1e-10:
        return 1.0

    compatibility = mono_rms / expected_mono_rms
    return np.clip(compatibility, 0.0, 2.0)


def analyze_engine_stereo(engine_id: int, stereo_file: Path) -> StereoAnalysisResult:
    """Perform comprehensive stereo analysis on an engine's output"""

    metadata = ALL_ENGINES.get(engine_id, {
        "name": f"Unknown Engine {engine_id}",
        "category": "Unknown",
        "expected_stereo": True
    })

    result = StereoAnalysisResult(
        engine_id=engine_id,
        engine_name=metadata["name"],
        category=metadata["category"],
        expected_stereo=metadata["expected_stereo"]
    )

    # Check if file exists
    if not stereo_file.exists():
        result.error_message = "No stereo data file found"
        result.status = "NO_DATA"
        return result

    try:
        # Read stereo data
        left_channel = []
        right_channel = []

        with open(stereo_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    # Try various column name formats
                    left_val = row.get('L') or row.get('left') or row.get('Left')
                    right_val = row.get('R') or row.get('right') or row.get('Right')

                    if left_val and right_val:
                        left_channel.append(float(left_val))
                        right_channel.append(float(right_val))
                except (ValueError, KeyError, TypeError):
                    continue

        if len(left_channel) == 0:
            result.error_message = "Empty stereo data"
            result.status = "EMPTY_DATA"
            return result

        # Convert to numpy arrays
        left = np.array(left_channel)
        right = np.array(right_channel)

        result.has_data = True

        # Check for NaN or Inf
        if np.any(np.isnan(left)) or np.any(np.isnan(right)):
            result.error_message = "Contains NaN values"
            result.status = "ERROR"
            return result

        if np.any(np.isinf(left)) or np.any(np.isinf(right)):
            result.error_message = "Contains Inf values"
            result.status = "ERROR"
            return result

        # Calculate peak levels
        result.peak_l = np.max(np.abs(left))
        result.peak_r = np.max(np.abs(right))

        # Calculate RMS levels
        result.rms_l = np.sqrt(np.mean(left**2))
        result.rms_r = np.sqrt(np.mean(right**2))

        # Check for silence
        if result.peak_l < 1e-6 and result.peak_r < 1e-6:
            result.error_message = "Silent output"
            result.status = "SILENT"
            return result

        # Calculate L/R balance
        if result.rms_r > 1e-10:
            result.balance_ratio = result.rms_l / result.rms_r
        else:
            result.balance_ratio = 100.0 if result.rms_l > 1e-10 else 1.0

        # STEREO ANALYSIS METRICS

        # 1. L/R Correlation
        result.lr_correlation = calculate_correlation(left, right)

        # 2. Stereo Width
        result.stereo_width = calculate_stereo_width(left, right)

        # 3. Phase Coherence
        result.phase_coherence = calculate_phase_coherence(left, right)

        # 4. Mid/Side Analysis
        result.mid_level, result.side_level, result.mid_side_ratio = calculate_mid_side_analysis(left, right)

        # 5. Mono Compatibility
        result.mono_compatibility = calculate_mono_compatibility(left, right)

        # QUALITY FLAGS

        # Check for mono collapse (high correlation when should be stereo)
        if result.expected_stereo and result.lr_correlation > 0.95:
            result.is_mono_collapsed = True
            result.warnings.append("MONO_COLLAPSED: Correlation > 0.95")

        # Check for significant stereo content
        if result.stereo_width > 0.1:
            result.has_stereo_content = True

        # Check for phase reversal
        if result.phase_coherence < -0.5:
            result.is_phase_reversed = True
            result.warnings.append("PHASE_REVERSED: Coherence < -0.5")

        # Check for channel imbalance
        if result.balance_ratio < 0.7 or result.balance_ratio > 1.43:  # More than 3dB difference
            result.is_significantly_imbalanced = True
            result.warnings.append(f"IMBALANCED: L/R ratio = {result.balance_ratio:.2f}")

        # OVERALL STATUS
        if result.expected_stereo:
            if result.is_mono_collapsed:
                result.status = "FAIL_MONO_COLLAPSED"
                result.quality_grade = "F"
            elif result.lr_correlation < 0.8 and result.has_stereo_content:
                result.status = "PASS"
                result.quality_grade = "A"
            elif result.lr_correlation < 0.9 and result.has_stereo_content:
                result.status = "PASS"
                result.quality_grade = "B"
            elif result.has_stereo_content:
                result.status = "WARNING"
                result.quality_grade = "C"
            else:
                result.status = "WARNING"
                result.quality_grade = "D"
        else:
            # Engine expected to be mono
            if result.lr_correlation > 0.95:
                result.status = "PASS"
                result.quality_grade = "A"
            else:
                result.status = "WARNING"
                result.quality_grade = "B"
                result.warnings.append("Expected mono but has stereo content")

    except Exception as e:
        result.error_message = f"Analysis error: {str(e)}"
        result.status = "ERROR"
        return result

    return result


def print_header():
    """Print analysis header"""
    print("\n" + "=" * 100)
    print(" " * 25 + "COMPREHENSIVE STEREO ANALYSIS SUITE")
    print(" " * 30 + "ALL 56 CHIMERA ENGINES")
    print("=" * 100)
    print()


def print_stereo_matrix(results: List[StereoAnalysisResult]):
    """Print detailed stereo quality matrix"""
    print("\n" + "=" * 140)
    print(" " * 50 + "STEREO QUALITY MATRIX")
    print("=" * 140)
    print()

    # Header
    print(f"{'ID':<4} {'Engine':<30} {'Cat':<12} {'Corr':<8} {'Width':<8} {'Phase':<8} {'M/S':<8} {'Grade':<7} {'Status':<20}")
    print("-" * 140)

    for result in results:
        if not result.has_data:
            status_str = result.status
            print(f"{result.engine_id:<4} {result.engine_name[:29]:<30} {result.category[:11]:<12} "
                  f"{'N/A':<8} {'N/A':<8} {'N/A':<8} {'N/A':<8} {'?':<7} {status_str:<20}")
            continue

        # Format metrics
        corr_str = f"{result.lr_correlation:.3f}"
        width_str = f"{result.stereo_width:.3f}"
        phase_str = f"{result.phase_coherence:.3f}"
        ms_str = f"{result.mid_side_ratio:.1f}dB" if result.mid_side_ratio > -90 else "-inf"

        # Color-code status
        status_display = result.status
        if result.is_mono_collapsed:
            status_display = "MONO COLLAPSED"

        print(f"{result.engine_id:<4} {result.engine_name[:29]:<30} {result.category[:11]:<12} "
              f"{corr_str:<8} {width_str:<8} {phase_str:<8} {ms_str:<8} {result.quality_grade:<7} {status_display:<20}")

        # Print warnings indented
        if result.warnings:
            for warning in result.warnings:
                print(f"     Warning: {warning}")

    print()


def print_mono_collapsed_engines(results: List[StereoAnalysisResult]):
    """Print list of mono-collapsed engines"""
    print("=" * 100)
    print(" " * 30 + "MONO COLLAPSED ENGINES")
    print(" " * 25 + "(Correlation > 0.95, Expected Stereo)")
    print("=" * 100)
    print()

    collapsed = [r for r in results if r.is_mono_collapsed and r.expected_stereo]

    if not collapsed:
        print("  No mono-collapsed engines detected")
        print()
        return

    print(f"  Found {len(collapsed)} mono-collapsed engines:\n")
    print(f"  {'ID':<6} {'Engine':<35} {'Category':<15} {'Correlation':<12}")
    print("  " + "-" * 80)

    for result in collapsed:
        print(f"  {result.engine_id:<6} {result.engine_name[:34]:<35} {result.category[:14]:<15} {result.lr_correlation:.4f}")

    print()


def print_phase_issues(results: List[StereoAnalysisResult]):
    """Print engines with phase issues"""
    print("=" * 100)
    print(" " * 35 + "PHASE ISSUES")
    print("=" * 100)
    print()

    phase_problems = [r for r in results if r.is_phase_reversed or r.phase_coherence < 0.3]

    if not phase_problems:
        print("  No significant phase issues detected")
        print()
        return

    print(f"  Found {len(phase_problems)} engines with phase issues:\n")
    print(f"  {'ID':<6} {'Engine':<35} {'Phase Coherence':<17} {'Issue':<20}")
    print("  " + "-" * 80)

    for result in phase_problems:
        issue = "Phase Reversed" if result.is_phase_reversed else "Low Coherence"
        print(f"  {result.engine_id:<6} {result.engine_name[:34]:<35} {result.phase_coherence:>15.3f}   {issue:<20}")

    print()


def print_category_summary(results: List[StereoAnalysisResult]):
    """Print summary by category"""
    print("=" * 100)
    print(" " * 30 + "CATEGORY SUMMARY")
    print("=" * 100)
    print()

    # Group by category
    categories = {}
    for result in results:
        if result.category not in categories:
            categories[result.category] = []
        categories[result.category].append(result)

    print(f"  {'Category':<20} {'Total':<8} {'Pass':<8} {'Warning':<10} {'Fail':<8} {'Avg Corr':<12} {'Avg Width':<12}")
    print("  " + "-" * 85)

    for category in sorted(categories.keys()):
        engines = categories[category]
        total = len(engines)
        passed = sum(1 for e in engines if e.status == "PASS")
        warned = sum(1 for e in engines if e.status == "WARNING")
        failed = sum(1 for e in engines if "FAIL" in e.status)

        # Calculate averages (only for engines with data)
        engines_with_data = [e for e in engines if e.has_data]
        avg_corr = np.mean([e.lr_correlation for e in engines_with_data]) if engines_with_data else 0.0
        avg_width = np.mean([e.stereo_width for e in engines_with_data]) if engines_with_data else 0.0

        print(f"  {category:<20} {total:<8} {passed:<8} {warned:<10} {failed:<8} {avg_corr:<12.3f} {avg_width:<12.3f}")

    print()


def print_overall_summary(results: List[StereoAnalysisResult]):
    """Print overall summary"""
    print("=" * 100)
    print(" " * 35 + "OVERALL SUMMARY")
    print("=" * 100)
    print()

    total = len(results)
    analyzed = sum(1 for r in results if r.has_data)
    passed = sum(1 for r in results if r.status == "PASS")
    warned = sum(1 for r in results if r.status == "WARNING")
    failed = sum(1 for r in results if "FAIL" in r.status)
    no_data = sum(1 for r in results if not r.has_data)

    mono_collapsed = sum(1 for r in results if r.is_mono_collapsed)
    phase_issues = sum(1 for r in results if r.is_phase_reversed)
    imbalanced = sum(1 for r in results if r.is_significantly_imbalanced)

    print(f"  Total Engines:            {total}")
    print(f"  Analyzed:                 {analyzed}")
    print(f"  No Data:                  {no_data}")
    print()
    print(f"  PASSED:                   {passed} ({100.0 * passed / analyzed if analyzed > 0 else 0:.1f}%)")
    print(f"  WARNINGS:                 {warned} ({100.0 * warned / analyzed if analyzed > 0 else 0:.1f}%)")
    print(f"  FAILED:                   {failed} ({100.0 * failed / analyzed if analyzed > 0 else 0:.1f}%)")
    print()
    print(f"  Mono Collapsed:           {mono_collapsed}")
    print(f"  Phase Issues:             {phase_issues}")
    print(f"  Significantly Imbalanced: {imbalanced}")
    print()

    if failed > 0:
        print("  STATUS: ISSUES DETECTED")
    elif warned > 0:
        print("  STATUS: WARNINGS PRESENT")
    else:
        print("  STATUS: ALL ENGINES PASS")
    print()


def save_csv_report(results: List[StereoAnalysisResult], output_file: Path):
    """Save detailed CSV report"""
    with open(output_file, 'w', newline='') as f:
        writer = csv.writer(f)

        # Header
        writer.writerow([
            'EngineID', 'EngineName', 'Category', 'ExpectedStereo',
            'LRCorrelation', 'StereoWidth', 'PhaseCoherence',
            'MidLevel', 'SideLevel', 'MidSideRatio_dB',
            'MonoCompatibility', 'PeakL', 'PeakR', 'RMS_L', 'RMS_R',
            'BalanceRatio', 'HasStereoContent', 'IsMonoCollapsed',
            'IsPhaseReversed', 'IsImbalanced', 'QualityGrade',
            'Status', 'Warnings', 'ErrorMessage'
        ])

        # Data rows
        for result in results:
            writer.writerow([
                result.engine_id,
                result.engine_name,
                result.category,
                'Yes' if result.expected_stereo else 'No',
                f"{result.lr_correlation:.6f}" if result.has_data else 'N/A',
                f"{result.stereo_width:.6f}" if result.has_data else 'N/A',
                f"{result.phase_coherence:.6f}" if result.has_data else 'N/A',
                f"{result.mid_level:.6f}" if result.has_data else 'N/A',
                f"{result.side_level:.6f}" if result.has_data else 'N/A',
                f"{result.mid_side_ratio:.3f}" if result.has_data else 'N/A',
                f"{result.mono_compatibility:.6f}" if result.has_data else 'N/A',
                f"{result.peak_l:.6f}" if result.has_data else 'N/A',
                f"{result.peak_r:.6f}" if result.has_data else 'N/A',
                f"{result.rms_l:.6f}" if result.has_data else 'N/A',
                f"{result.rms_r:.6f}" if result.has_data else 'N/A',
                f"{result.balance_ratio:.6f}" if result.has_data else 'N/A',
                'Yes' if result.has_stereo_content else 'No',
                'Yes' if result.is_mono_collapsed else 'No',
                'Yes' if result.is_phase_reversed else 'No',
                'Yes' if result.is_significantly_imbalanced else 'No',
                result.quality_grade,
                result.status,
                '; '.join(result.warnings) if result.warnings else '',
                result.error_message
            ])

    print(f"Detailed CSV report saved: {output_file}")


def save_json_report(results: List[StereoAnalysisResult], output_file: Path):
    """Save JSON report for programmatic access"""
    data = {
        "analysis_type": "stereo_quality",
        "total_engines": len(results),
        "engines": []
    }

    for result in results:
        engine_data = {
            "id": result.engine_id,
            "name": result.engine_name,
            "category": result.category,
            "expected_stereo": result.expected_stereo,
            "has_data": result.has_data,
            "metrics": {
                "lr_correlation": result.lr_correlation if result.has_data else None,
                "stereo_width": result.stereo_width if result.has_data else None,
                "phase_coherence": result.phase_coherence if result.has_data else None,
                "mid_side_ratio_db": result.mid_side_ratio if result.has_data else None,
                "mono_compatibility": result.mono_compatibility if result.has_data else None,
                "balance_ratio": result.balance_ratio if result.has_data else None
            },
            "flags": {
                "has_stereo_content": result.has_stereo_content,
                "is_mono_collapsed": result.is_mono_collapsed,
                "is_phase_reversed": result.is_phase_reversed,
                "is_imbalanced": result.is_significantly_imbalanced
            },
            "quality_grade": result.quality_grade,
            "status": result.status,
            "warnings": result.warnings,
            "error": result.error_message
        }
        data["engines"].append(engine_data)

    with open(output_file, 'w') as f:
        json.dump(data, f, indent=2)

    print(f"JSON report saved: {output_file}")


def main():
    # Get test directory
    script_dir = Path(__file__).parent

    print_header()

    print("Analyzing stereo data for all 56 engines...")
    print()

    # Analyze each engine
    results = []
    for engine_id in range(57):  # 0-56
        if engine_id not in ALL_ENGINES:
            continue

        # Look for stereo data file
        stereo_file = script_dir / f"stereo_engine_{engine_id}.csv"

        print(f"  [{engine_id:2d}] {ALL_ENGINES[engine_id]['name'][:40]:<40} ... ", end="", flush=True)

        result = analyze_engine_stereo(engine_id, stereo_file)
        results.append(result)

        if result.has_data:
            print(f"Corr={result.lr_correlation:.3f}, Width={result.stereo_width:.3f}, Grade={result.quality_grade}")
        else:
            print(f"[{result.status}]")

    # Print reports
    print_stereo_matrix(results)
    print_mono_collapsed_engines(results)
    print_phase_issues(results)
    print_category_summary(results)
    print_overall_summary(results)

    # Save reports
    csv_file = script_dir / "stereo_quality_report.csv"
    save_csv_report(results, csv_file)

    json_file = script_dir / "stereo_quality_report.json"
    save_json_report(results, json_file)

    print()
    print("=" * 100)
    print("Analysis complete!")
    print("=" * 100)
    print()

    # Exit code based on results
    failed = sum(1 for r in results if "FAIL" in r.status)
    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
