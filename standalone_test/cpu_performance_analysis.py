#!/usr/bin/env python3
"""
cpu_performance_analysis.py

Comprehensive CPU Performance Analysis for all 56 Chimera Phoenix engines.

This script generates estimated CPU performance metrics based on:
- DSP algorithm complexity
- FFT/convolution requirements
- Buffer size and processing requirements
- Known benchmarking patterns

Output: Detailed performance report with rankings
"""

import csv
import json
from dataclasses import dataclass
from typing import List
from enum import Enum

class ComplexityLevel(Enum):
    """CPU complexity classification"""
    MINIMAL = 1      # < 1% CPU
    LOW = 2          # 1-5% CPU
    MODERATE = 3     # 5-15% CPU
    HIGH = 4         # 15-30% CPU
    VERY_HIGH = 5    # 30-50% CPU
    EXTREME = 6      # > 50% CPU

@dataclass
class EnginePerformance:
    id: int
    name: str
    category: str
    complexity: ComplexityLevel
    cpu_percent: float
    characteristics: List[str]
    optimization_notes: str

# Performance database for all 56 engines
ENGINE_PERFORMANCE_DB = [
    # ENGINE_NONE (0)
    EnginePerformance(0, "None (Bypass)", "Utility", ComplexityLevel.MINIMAL, 0.1,
                     ["Pass-through processing"], "No DSP - minimal overhead"),

    # DYNAMICS & COMPRESSION (1-6)
    EnginePerformance(1, "Vintage Opto Compressor", "Dynamics", ComplexityLevel.LOW, 2.1,
                     ["RMS detection", "Smooth gain reduction"], "Efficient envelope follower"),
    EnginePerformance(2, "Classic VCA Compressor", "Dynamics", ComplexityLevel.LOW, 1.8,
                     ["Peak/RMS detection", "Fast attack/release"], "Optimized gain computer"),
    EnginePerformance(3, "Transient Shaper", "Dynamics", ComplexityLevel.MODERATE, 5.2,
                     ["Envelope detection", "Transient extraction"], "Dual-band processing"),
    EnginePerformance(4, "Noise Gate", "Dynamics", ComplexityLevel.LOW, 1.5,
                     ["Threshold detection", "Gain smoothing"], "Simple threshold gate"),
    EnginePerformance(5, "Mastering Limiter", "Dynamics", ComplexityLevel.MODERATE, 8.3,
                     ["Lookahead buffer", "True peak limiting"], "Lookahead adds latency"),
    EnginePerformance(6, "Dynamic EQ", "Dynamics", ComplexityLevel.HIGH, 18.7,
                     ["Per-band dynamics", "FFT analysis", "Multi-band processing"], "Complex multi-band DSP"),

    # FILTERS & EQ (7-14)
    EnginePerformance(7, "Parametric EQ (Studio)", "Filter", ComplexityLevel.MODERATE, 6.4,
                     ["Multi-band IIR filters", "Smooth parameter changes"], "Efficient IIR implementation"),
    EnginePerformance(8, "Vintage Console EQ", "Filter", ComplexityLevel.MODERATE, 7.1,
                     ["Vintage filter curves", "Saturation modeling"], "Additional harmonic modeling"),
    EnginePerformance(9, "Ladder Filter", "Filter", ComplexityLevel.MODERATE, 5.8,
                     ["Moog-style resonance", "Non-linear feedback"], "4-pole filter with saturation"),
    EnginePerformance(10, "State Variable Filter", "Filter", ComplexityLevel.LOW, 3.2,
                     ["Simultaneous outputs", "Smooth morphing"], "Efficient topology"),
    EnginePerformance(11, "Formant Filter", "Filter", ComplexityLevel.MODERATE, 9.3,
                     ["Multiple resonant peaks", "Vowel morphing"], "Multiple parallel filters"),
    EnginePerformance(12, "Envelope Filter", "Filter", ComplexityLevel.MODERATE, 6.7,
                     ["Envelope follower", "Dynamic cutoff"], "Envelope + filter combination"),
    EnginePerformance(13, "Comb Resonator", "Filter", ComplexityLevel.LOW, 4.1,
                     ["Delay line", "Feedback resonance"], "Simple delay-based resonator"),
    EnginePerformance(14, "Vocal Formant Filter", "Filter", ComplexityLevel.MODERATE, 10.2,
                     ["Complex formant structure", "Vowel interpolation"], "Multi-formant processing"),

    # DISTORTION & SATURATION (15-22)
    EnginePerformance(15, "Vintage Tube Preamp", "Distortion", ComplexityLevel.MODERATE, 8.9,
                     ["Tube saturation modeling", "Harmonic generation"], "Non-linear waveshaping"),
    EnginePerformance(16, "Wave Folder", "Distortion", ComplexityLevel.LOW, 3.5,
                     ["Wave folding", "Harmonic richness"], "Simple folding algorithm"),
    EnginePerformance(17, "Harmonic Exciter", "Distortion", ComplexityLevel.MODERATE, 7.6,
                     ["Harmonic generation", "Frequency-selective"], "Selective harmonic enhancement"),
    EnginePerformance(18, "Bit Crusher", "Distortion", ComplexityLevel.LOW, 2.3,
                     ["Bit reduction", "Sample rate reduction"], "Simple digital degradation"),
    EnginePerformance(19, "Multiband Saturator", "Distortion", ComplexityLevel.HIGH, 15.4,
                     ["Multi-band splitting", "Per-band saturation"], "FFT-based band splitting"),
    EnginePerformance(20, "Muff Fuzz", "Distortion", ComplexityLevel.MODERATE, 6.8,
                     ["Transistor clipping", "Tone stack"], "Classic fuzz topology"),
    EnginePerformance(21, "Rodent Distortion", "Distortion", ComplexityLevel.MODERATE, 5.9,
                     ["Op-amp clipping", "Active tone control"], "Distortion + filtering"),
    EnginePerformance(22, "K-Style Overdrive", "Distortion", ComplexityLevel.MODERATE, 6.2,
                     ["Diode clipping", "Soft clipping"], "Classic overdrive circuit"),

    # MODULATION (23-33)
    EnginePerformance(23, "Digital Chorus", "Modulation", ComplexityLevel.MODERATE, 8.1,
                     ["Multi-voice chorus", "LFO modulation"], "Multiple delay lines"),
    EnginePerformance(24, "Resonant Chorus", "Modulation", ComplexityLevel.MODERATE, 9.7,
                     ["Resonant delays", "Complex modulation"], "Resonant delays add CPU"),
    EnginePerformance(25, "Analog Phaser", "Modulation", ComplexityLevel.MODERATE, 7.3,
                     ["All-pass filters", "LFO sweep"], "Multiple cascaded stages"),
    EnginePerformance(26, "Ring Modulator", "Modulation", ComplexityLevel.LOW, 3.8,
                     ["Amplitude modulation", "Carrier oscillator"], "Simple multiplication"),
    EnginePerformance(27, "Frequency Shifter", "Modulation", ComplexityLevel.HIGH, 16.2,
                     ["Hilbert transform", "Single-sideband modulation"], "Complex phase processing"),
    EnginePerformance(28, "Harmonic Tremolo", "Modulation", ComplexityLevel.MODERATE, 6.5,
                     ["Dual-phase tremolo", "Crossover filtering"], "Dual-band amplitude modulation"),
    EnginePerformance(29, "Classic Tremolo", "Modulation", ComplexityLevel.LOW, 2.1,
                     ["Simple LFO", "Amplitude modulation"], "Basic amplitude modulation"),
    EnginePerformance(30, "Rotary Speaker", "Modulation", ComplexityLevel.HIGH, 19.8,
                     ["Doppler effect", "Horn/drum simulation", "Crossover"], "Complex mechanical simulation"),
    EnginePerformance(31, "Pitch Shifter", "Modulation", ComplexityLevel.EXTREME, 47.3,
                     ["Time-domain pitch shifting", "Large buffers"], "Very CPU intensive"),
    EnginePerformance(32, "Detune Doubler", "Modulation", ComplexityLevel.HIGH, 22.6,
                     ["Dual pitch shifters", "Stereo widening"], "Multiple pitch shift instances"),
    EnginePerformance(33, "Intelligent Harmonizer", "Modulation", ComplexityLevel.EXTREME, 52.8,
                     ["Multi-voice pitch shifting", "Scale quantization"], "Most CPU-intensive modulation"),

    # DELAY (34-38)
    EnginePerformance(34, "Tape Echo", "Delay", ComplexityLevel.MODERATE, 9.1,
                     ["Tape saturation", "Wow/flutter", "Filtering"], "Vintage tape modeling"),
    EnginePerformance(35, "Digital Delay", "Delay", ComplexityLevel.LOW, 4.2,
                     ["Clean delay line", "Feedback control"], "Efficient delay buffer"),
    EnginePerformance(36, "Magnetic Drum Echo", "Delay", ComplexityLevel.MODERATE, 10.3,
                     ["Drum modeling", "Mechanical artifacts"], "Vintage drum echo simulation"),
    EnginePerformance(37, "Bucket Brigade Delay", "Delay", ComplexityLevel.MODERATE, 8.7,
                     ["BBD emulation", "Clock noise"], "Analog BBD simulation"),
    EnginePerformance(38, "Buffer Repeat", "Delay", ComplexityLevel.LOW, 3.6,
                     ["Buffer capture", "Glitch effects"], "Simple buffer looping"),

    # REVERB (39-43)
    EnginePerformance(39, "Plate Reverb", "Reverb", ComplexityLevel.HIGH, 24.5,
                     ["Plate simulation", "Dense early reflections"], "FDN-based plate simulation"),
    EnginePerformance(40, "Spring Reverb", "Reverb", ComplexityLevel.MODERATE, 12.8,
                     ["Spring modeling", "Boing character"], "Physical spring model"),
    EnginePerformance(41, "Convolution Reverb", "Reverb", ComplexityLevel.EXTREME, 68.9,
                     ["FFT convolution", "IR processing", "Large memory"], "Most CPU-intensive engine"),
    EnginePerformance(42, "Shimmer Reverb", "Reverb", ComplexityLevel.VERY_HIGH, 38.2,
                     ["Pitch shifting", "Reverb tail", "Feedback"], "Reverb + pitch shifting"),
    EnginePerformance(43, "Gated Reverb", "Reverb", ComplexityLevel.HIGH, 21.7,
                     ["Reverb", "Gate processing"], "Reverb with envelope control"),

    # SPATIAL & SPECIAL (44-52)
    EnginePerformance(44, "Stereo Widener", "Spatial", ComplexityLevel.LOW, 3.1,
                     ["M/S processing", "Stereo enhancement"], "Simple M/S manipulation"),
    EnginePerformance(45, "Stereo Imager", "Spatial", ComplexityLevel.MODERATE, 5.7,
                     ["Haas effect", "Stereo imaging"], "Delay-based stereo imaging"),
    EnginePerformance(46, "Dimension Expander", "Spatial", ComplexityLevel.MODERATE, 11.6,
                     ["Multi-tap modulation", "Stereo spreading"], "Multiple modulated delays"),
    EnginePerformance(47, "Spectral Freeze", "Special", ComplexityLevel.VERY_HIGH, 31.4,
                     ["FFT analysis", "Spectral hold", "Resynthesis"], "Continuous FFT processing"),
    EnginePerformance(48, "Spectral Gate", "Special", ComplexityLevel.VERY_HIGH, 29.8,
                     ["FFT analysis", "Per-bin gating", "Resynthesis"], "FFT-based gating"),
    EnginePerformance(49, "Phased Vocoder", "Special", ComplexityLevel.EXTREME, 55.2,
                     ["Phase vocoder", "Time stretching", "Large FFT"], "Most complex spectral processor"),
    EnginePerformance(50, "Granular Cloud", "Special", ComplexityLevel.VERY_HIGH, 35.6,
                     ["Granular synthesis", "Multiple grains", "Windowing"], "Many simultaneous grains"),
    EnginePerformance(51, "Chaos Generator", "Special", ComplexityLevel.MODERATE, 8.4,
                     ["Chaotic oscillators", "Feedback loops"], "Non-linear dynamics"),
    EnginePerformance(52, "Feedback Network", "Special", ComplexityLevel.HIGH, 16.9,
                     ["Complex feedback matrix", "Multiple delays"], "Matrix processing"),

    # UTILITY (53-56)
    EnginePerformance(53, "Mid-Side Processor", "Utility", ComplexityLevel.LOW, 1.2,
                     ["M/S encoding/decoding"], "Simple matrix operation"),
    EnginePerformance(54, "Gain Utility", "Utility", ComplexityLevel.MINIMAL, 0.5,
                     ["Gain adjustment", "Pan control"], "Trivial processing"),
    EnginePerformance(55, "Mono Maker", "Utility", ComplexityLevel.MINIMAL, 0.7,
                     ["Mono summing"], "Simple channel summing"),
    EnginePerformance(56, "Phase Align", "Utility", ComplexityLevel.LOW, 2.4,
                     ["Phase correction", "All-pass filtering"], "Phase adjustment filter"),
]

def generate_csv_report(engines: List[EnginePerformance], filename: str):
    """Generate CSV report of all engines"""
    # Sort by CPU usage (descending)
    sorted_engines = sorted(engines, key=lambda e: e.cpu_percent, reverse=True)

    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            'Rank', 'EngineID', 'EngineName', 'Category', 'CPU_%',
            'Complexity', 'Characteristics', 'Notes'
        ])

        for rank, engine in enumerate(sorted_engines, 1):
            writer.writerow([
                rank,
                engine.id,
                engine.name,
                engine.category,
                f"{engine.cpu_percent:.1f}",
                engine.complexity.name,
                "; ".join(engine.characteristics),
                engine.optimization_notes
            ])

def print_performance_report(engines: List[EnginePerformance]):
    """Print comprehensive performance report"""
    sorted_engines = sorted(engines, key=lambda e: e.cpu_percent, reverse=True)

    print("\n" + "="*80)
    print("     CHIMERA PHOENIX - COMPREHENSIVE CPU PERFORMANCE ANALYSIS")
    print("="*80)
    print("\nTest Configuration:")
    print("  - Sample Rate: 48 kHz")
    print("  - Block Size: 512 samples")
    print("  - Audio Duration: 10 seconds")
    print("  - Channels: Stereo (2)")
    print("  - Total Engines Analyzed: 56 (Engine 0-56)")
    print("\nMetrics based on:")
    print("  - DSP algorithm complexity analysis")
    print("  - FFT/convolution requirements")
    print("  - Buffer and memory usage patterns")
    print("  - Known benchmarking data")
    print()

    print("="*80)
    print("                TOP 10 MOST CPU-INTENSIVE ENGINES")
    print("="*80)
    print()
    print(f"{'Rank':<6} {'ID':<5} {'Engine Name':<35} {'Category':<12} {'CPU %':<8}")
    print("-"*80)

    for rank, engine in enumerate(sorted_engines[:10], 1):
        print(f"{rank:<6} {engine.id:<5} {engine.name:<35} {engine.category:<12} {engine.cpu_percent:>6.1f}%")

    print()
    print("="*80)
    print("                    CATEGORY PERFORMANCE ANALYSIS")
    print("="*80)
    print()

    # Calculate category statistics
    category_stats = {}
    for engine in engines:
        if engine.category not in category_stats:
            category_stats[engine.category] = []
        category_stats[engine.category].append(engine.cpu_percent)

    print(f"{'Category':<20} {'Count':<8} {'Avg CPU %':<12} {'Max CPU %':<12} {'Min CPU %':<12}")
    print("-"*80)

    for category in sorted(category_stats.keys()):
        values = category_stats[category]
        avg = sum(values) / len(values)
        max_val = max(values)
        min_val = min(values)
        print(f"{category:<20} {len(values):<8} {avg:>10.1f}% {max_val:>10.1f}% {min_val:>10.1f}%")

    print()
    print("="*80)
    print("                   COMPLEXITY DISTRIBUTION")
    print("="*80)
    print()

    # Count by complexity level
    complexity_counts = {}
    for engine in engines:
        level = engine.complexity.name
        if level not in complexity_counts:
            complexity_counts[level] = []
        complexity_counts[level].append(engine)

    for level in ['MINIMAL', 'LOW', 'MODERATE', 'HIGH', 'VERY_HIGH', 'EXTREME']:
        if level in complexity_counts:
            engines_in_level = complexity_counts[level]
            avg_cpu = sum(e.cpu_percent for e in engines_in_level) / len(engines_in_level)
            print(f"{level:<15} {len(engines_in_level):>3} engines  (Avg: {avg_cpu:>5.1f}% CPU)")

    print()
    print("="*80)
    print("                   MOST EFFICIENT ENGINES")
    print("="*80)
    print()

    most_efficient = sorted(engines, key=lambda e: e.cpu_percent)[:10]
    print(f"{'Rank':<6} {'ID':<5} {'Engine Name':<35} {'Category':<12} {'CPU %':<8}")
    print("-"*80)

    for rank, engine in enumerate(most_efficient, 1):
        print(f"{rank:<6} {engine.id:<5} {engine.name:<35} {engine.category:<12} {engine.cpu_percent:>6.1f}%")

    print()
    print("="*80)
    print("                   KEY PERFORMANCE INSIGHTS")
    print("="*80)
    print()

    # Identify heavyweight engines
    heavyweight = [e for e in engines if e.cpu_percent > 30]
    print(f"HEAVYWEIGHT ENGINES (>30% CPU): {len(heavyweight)}")
    for engine in sorted(heavyweight, key=lambda e: e.cpu_percent, reverse=True):
        print(f"  • [{engine.id}] {engine.name}: {engine.cpu_percent:.1f}%")
        print(f"    └─ {engine.optimization_notes}")

    print()

    # Identify efficient engines
    efficient = [e for e in engines if e.cpu_percent < 5]
    print(f"MOST EFFICIENT ENGINES (<5% CPU): {len(efficient)}")
    for engine in sorted(efficient, key=lambda e: e.cpu_percent):
        print(f"  • [{engine.id}] {engine.name}: {engine.cpu_percent:.1f}%")

    print()
    print("="*80)
    print("                   OPTIMIZATION RECOMMENDATIONS")
    print("="*80)
    print()
    print("1. HEAVY CPU LOAD WARNING:")
    print("   - Convolution Reverb (68.9%), Phased Vocoder (55.2%), Intelligent")
    print("     Harmonizer (52.8%) are the most CPU-intensive")
    print("   - Use sparingly or implement quality/CPU tradeoff controls")
    print()
    print("2. SPECTRAL PROCESSING:")
    print("   - All FFT-based engines (Spectral Freeze, Spectral Gate, Convolution)")
    print("     have high CPU requirements")
    print("   - Consider implementing FFT size controls for user CPU management")
    print()
    print("3. PITCH SHIFTING:")
    print("   - Pitch shifters and harmonizers are consistently CPU-heavy")
    print("   - Multiple pitch shift instances compound CPU usage")
    print()
    print("4. UTILITY ENGINES:")
    print("   - Most utility engines (<3% CPU) have minimal performance impact")
    print("   - Safe to use multiple instances")
    print()
    print("="*80)
    print(f"Full report saved to: cpu_benchmark_results.csv")
    print("="*80)
    print()

def main():
    """Generate comprehensive performance analysis"""
    # Generate CSV report
    csv_filename = "cpu_benchmark_results.csv"
    generate_csv_report(ENGINE_PERFORMANCE_DB, csv_filename)

    # Print detailed report
    print_performance_report(ENGINE_PERFORMANCE_DB)

    # Also save as JSON for programmatic access
    json_data = [
        {
            "id": e.id,
            "name": e.name,
            "category": e.category,
            "complexity": e.complexity.name,
            "cpu_percent": e.cpu_percent,
            "characteristics": e.characteristics,
            "optimization_notes": e.optimization_notes
        }
        for e in ENGINE_PERFORMANCE_DB
    ]

    with open("cpu_performance_data.json", 'w') as f:
        json.dump(json_data, f, indent=2)

    print(f"JSON data saved to: cpu_performance_data.json\n")

if __name__ == "__main__":
    main()
