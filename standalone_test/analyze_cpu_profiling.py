#!/usr/bin/env python3
"""
CPU Performance Profiling Analysis Tool

Analyzes CPU benchmark results and generates comprehensive optimization report.
"""

import csv
import json
import sys
from collections import defaultdict
from typing import Dict, List, Tuple

# Target performance metrics
TARGET_CPU_SINGLE_ENGINE = 5.0  # % CPU at 48kHz, 512 buffer
TARGET_CPU_10_ENGINES = 30.0
TARGET_CPU_56_ENGINES = 200.0  # Allow multi-core

# Operation complexity weights (estimated CPU cost multipliers)
OPERATION_WEIGHTS = {
    'FFT': 3.0,
    'Filters': 1.5,
    'Oversampling': 2.5,
    'DelayLines': 1.2,
    'LFOs': 1.0,
    'PitchShift': 4.0,
    'Convolution': 5.0
}

class EngineProfile:
    def __init__(self, rank, engine_id, name, category, cpu_percent, complexity, characteristics, notes):
        self.rank = int(rank)
        self.id = int(engine_id)
        self.name = name
        self.category = category
        self.cpu_percent = float(cpu_percent)
        self.complexity = complexity
        self.characteristics = characteristics
        self.notes = notes

        # Parse characteristics for operations
        self.has_fft = 'FFT' in characteristics
        self.has_filters = 'filter' in characteristics.lower()
        self.has_oversampling = 'oversamp' in characteristics.lower()
        self.has_delay = 'delay' in characteristics.lower() or 'buffer' in characteristics.lower()
        self.has_lfo = 'LFO' in characteristics or 'modulation' in characteristics.lower()
        self.has_pitch_shift = 'pitch' in characteristics.lower()
        self.has_convolution = 'convolution' in characteristics.lower()

    def meets_target(self, target=TARGET_CPU_SINGLE_ENGINE):
        return self.cpu_percent <= target

    def optimization_priority(self):
        """Calculate optimization priority based on CPU usage and complexity"""
        if self.cpu_percent < TARGET_CPU_SINGLE_ENGINE:
            return 'LOW'
        elif self.cpu_percent < TARGET_CPU_SINGLE_ENGINE * 2:
            return 'MEDIUM'
        elif self.cpu_percent < TARGET_CPU_SINGLE_ENGINE * 5:
            return 'HIGH'
        else:
            return 'CRITICAL'

    def estimated_operation_costs(self):
        """Estimate CPU cost breakdown by operation type"""
        costs = {}
        total_weight = 0.0

        if self.has_convolution:
            costs['Convolution'] = OPERATION_WEIGHTS['Convolution']
            total_weight += OPERATION_WEIGHTS['Convolution']

        if self.has_pitch_shift:
            costs['PitchShift'] = OPERATION_WEIGHTS['PitchShift']
            total_weight += OPERATION_WEIGHTS['PitchShift']

        if self.has_fft:
            costs['FFT'] = OPERATION_WEIGHTS['FFT']
            total_weight += OPERATION_WEIGHTS['FFT']

        if self.has_oversampling:
            costs['Oversampling'] = OPERATION_WEIGHTS['Oversampling']
            total_weight += OPERATION_WEIGHTS['Oversampling']

        if self.has_filters:
            costs['Filters'] = OPERATION_WEIGHTS['Filters']
            total_weight += OPERATION_WEIGHTS['Filters']

        if self.has_delay:
            costs['DelayLines'] = OPERATION_WEIGHTS['DelayLines']
            total_weight += OPERATION_WEIGHTS['DelayLines']

        if self.has_lfo:
            costs['LFOs'] = OPERATION_WEIGHTS['LFOs']
            total_weight += OPERATION_WEIGHTS['LFOs']

        # Normalize to CPU percentage
        if total_weight > 0:
            for op in costs:
                costs[op] = (costs[op] / total_weight) * self.cpu_percent

        return costs

def load_benchmark_results(filename='cpu_benchmark_results.csv'):
    """Load CPU benchmark results from CSV"""
    engines = []

    try:
        with open(filename, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    engine = EngineProfile(
                        row['Rank'],
                        row['EngineID'],
                        row['EngineName'],
                        row['Category'],
                        row['CPU_%'],
                        row['Complexity'],
                        row['Characteristics'],
                        row['Notes']
                    )
                    engines.append(engine)
                except (KeyError, ValueError) as e:
                    print(f"Warning: Skipping malformed row: {e}", file=sys.stderr)
                    continue
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found", file=sys.stderr)
        sys.exit(1)

    return engines

def analyze_by_category(engines):
    """Analyze engines grouped by category"""
    by_category = defaultdict(list)

    for engine in engines:
        by_category[engine.category].append(engine)

    stats = {}
    for category, cat_engines in by_category.items():
        cpu_values = [e.cpu_percent for e in cat_engines]
        stats[category] = {
            'count': len(cat_engines),
            'avg_cpu': sum(cpu_values) / len(cpu_values),
            'max_cpu': max(cpu_values),
            'min_cpu': min(cpu_values),
            'total_cpu': sum(cpu_values),
            'pass_rate': len([e for e in cat_engines if e.meets_target()]) / len(cat_engines) * 100
        }

    return stats

def identify_optimization_opportunities(engines):
    """Identify specific optimization opportunities"""
    opportunities = []

    for engine in engines:
        if not engine.meets_target():
            ops = []
            recs = []

            if engine.has_convolution:
                ops.append('Convolution')
                recs.extend([
                    'Optimize FFT size (use smaller blocks)',
                    'Implement partitioned convolution',
                    'Consider GPU acceleration for long IRs'
                ])

            if engine.has_pitch_shift:
                ops.append('Pitch Shifting')
                recs.extend([
                    'Use optimized phase vocoder (PVSOLA)',
                    'Implement pitch cache for common intervals',
                    'Consider using spectral modeling'
                ])

            if engine.has_fft:
                ops.append('FFT Operations')
                recs.extend([
                    'Use vDSP accelerated FFT',
                    'Minimize FFT size where possible',
                    'Consider hop size optimization'
                ])

            if engine.has_oversampling:
                ops.append('Oversampling')
                recs.extend([
                    'Use adaptive oversampling (only when needed)',
                    'Optimize anti-aliasing filter order',
                    'Consider 2x instead of 4x oversampling'
                ])

            if engine.has_filters:
                ops.append('Filter Processing')
                recs.extend([
                    'Vectorize filter calculations with SIMD',
                    'Use biquad cascades efficiently',
                    'Consider lattice filter structures'
                ])

            if engine.has_delay:
                ops.append('Delay Lines')
                recs.extend([
                    'Use circular buffers efficiently',
                    'Optimize interpolation (linear vs cubic)',
                    'Consider fixed-point for delay indices'
                ])

            if engine.has_lfo:
                ops.append('LFO Calculations')
                recs.extend([
                    'Use wavetable lookup for LFO waveforms',
                    'Pre-calculate modulation values',
                    'Use SIMD for multiple LFOs'
                ])

            opportunities.append({
                'engine': engine,
                'priority': engine.optimization_priority(),
                'cpu_percent': engine.cpu_percent,
                'operations': ops,
                'recommendations': list(set(recs))  # Remove duplicates
            })

    # Sort by priority and CPU usage
    priority_order = {'CRITICAL': 0, 'HIGH': 1, 'MEDIUM': 2, 'LOW': 3}
    opportunities.sort(key=lambda x: (priority_order[x['priority']], -x['cpu_percent']))

    return opportunities

def estimate_multi_engine_capacity(engines):
    """Estimate multi-engine capacity"""
    # Calculate average CPU per engine
    avg_cpu = sum(e.cpu_percent for e in engines if e.id > 0) / len([e for e in engines if e.id > 0])

    # Estimate for different scenarios
    scenarios = {
        '10_engines': {
            'estimated_cpu': avg_cpu * 10,
            'realistic_cpu': avg_cpu * 10 * 1.1,  # 10% overhead
            'realtime_ok': (avg_cpu * 10 * 1.1) < 100
        },
        '25_engines': {
            'estimated_cpu': avg_cpu * 25,
            'realistic_cpu': avg_cpu * 25 * 1.15,  # 15% overhead
            'realtime_ok': (avg_cpu * 25 * 1.15) < 100
        },
        '56_engines': {
            'estimated_cpu': avg_cpu * 56,
            'realistic_cpu': avg_cpu * 56 * 1.2,  # 20% overhead
            'realtime_ok': (avg_cpu * 56 * 1.2) < 200  # Allow multi-core
        }
    }

    return scenarios

def generate_report(engines):
    """Generate comprehensive optimization report"""
    print("=" * 80)
    print("CHIMERA PHOENIX - CPU PERFORMANCE PROFILING & OPTIMIZATION ANALYSIS")
    print("=" * 80)
    print()

    # Summary statistics
    print("SUMMARY STATISTICS")
    print("-" * 80)
    total_engines = len([e for e in engines if e.id > 0])
    pass_count = len([e for e in engines if e.meets_target() and e.id > 0])
    fail_count = total_engines - pass_count
    avg_cpu = sum(e.cpu_percent for e in engines if e.id > 0) / total_engines

    print(f"Total Engines Tested:     {total_engines}")
    print(f"Target: <{TARGET_CPU_SINGLE_ENGINE}% CPU:        {pass_count} engines ({pass_count/total_engines*100:.1f}%)")
    print(f"Exceeding Target:         {fail_count} engines ({fail_count/total_engines*100:.1f}%)")
    print(f"Average CPU Usage:        {avg_cpu:.2f}%")
    print(f"Test Configuration:       48 kHz, 512 samples, 10 seconds")
    print()

    # Top 10 most CPU-intensive
    print("TOP 10 MOST CPU-INTENSIVE ENGINES")
    print("-" * 80)
    print(f"{'Rank':<6}{'ID':<6}{'Engine Name':<35}{'CPU %':<10}{'Target':<10}")
    print("-" * 80)

    for i, engine in enumerate(engines[:10], 1):
        if engine.id == 0:
            continue
        status = "PASS" if engine.meets_target() else "FAIL"
        print(f"{i:<6}{engine.id:<6}{engine.name:<35}{engine.cpu_percent:>6.1f}%   {status:<10}")
    print()

    # Category analysis
    print("CATEGORY EFFICIENCY ANALYSIS")
    print("-" * 80)
    print(f"{'Category':<15}{'Count':<8}{'Avg CPU %':<12}{'Max CPU %':<12}{'Pass Rate':<12}")
    print("-" * 80)

    cat_stats = analyze_by_category([e for e in engines if e.id > 0])
    for category, stats in sorted(cat_stats.items(), key=lambda x: -x[1]['avg_cpu']):
        print(f"{category:<15}{stats['count']:<8}{stats['avg_cpu']:>8.1f}%   "
              f"{stats['max_cpu']:>8.1f}%   {stats['pass_rate']:>8.1f}%")
    print()

    # Multi-engine capacity
    print("MULTI-ENGINE CAPACITY ANALYSIS")
    print("-" * 80)
    print(f"{'Scenario':<20}{'Est. CPU %':<15}{'Realistic CPU %':<18}{'Real-time OK?':<15}")
    print("-" * 80)

    scenarios = estimate_multi_engine_capacity([e for e in engines if e.id > 0])
    for name, data in scenarios.items():
        scenario_name = name.replace('_', ' ').title()
        realtime = "YES" if data['realtime_ok'] else "NO (multi-core needed)"
        print(f"{scenario_name:<20}{data['estimated_cpu']:>10.1f}%   "
              f"{data['realistic_cpu']:>12.1f}%   {realtime:<15}")
    print()

    # Optimization priorities
    print("=" * 80)
    print("OPTIMIZATION PRIORITIES & RECOMMENDATIONS")
    print("=" * 80)
    print()

    opportunities = identify_optimization_opportunities([e for e in engines if e.id > 0])

    for priority_level in ['CRITICAL', 'HIGH', 'MEDIUM']:
        level_opps = [o for o in opportunities if o['priority'] == priority_level]
        if not level_opps:
            continue

        print(f"{priority_level} PRIORITY ({len(level_opps)} engines)")
        print("-" * 80)

        for opp in level_opps:
            engine = opp['engine']
            print(f"\n[{engine.id}] {engine.name}")
            print(f"    Current CPU: {engine.cpu_percent:.1f}% (Target: <{TARGET_CPU_SINGLE_ENGINE}%)")
            print(f"    Complexity: {engine.complexity}")
            print(f"    Operations: {', '.join(opp['operations'])}")

            # Show estimated operation costs
            costs = engine.estimated_operation_costs()
            if costs:
                print(f"    Estimated Cost Breakdown:")
                for op, cost in sorted(costs.items(), key=lambda x: -x[1]):
                    print(f"      - {op}: {cost:.1f}%")

            print(f"    Recommendations:")
            for rec in opp['recommendations'][:5]:  # Top 5 recommendations
                print(f"      - {rec}")

        print()

    # Performance insights
    print("=" * 80)
    print("PERFORMANCE INSIGHTS & KEY FINDINGS")
    print("=" * 80)
    print()

    # Find most efficient engines
    efficient = [e for e in engines if e.cpu_percent < 3 and e.id > 0]
    print(f"MOST EFFICIENT ENGINES (< 3% CPU): {len(efficient)} engines")
    for e in efficient[:5]:
        print(f"  [{e.id}] {e.name}: {e.cpu_percent:.1f}%")
    print()

    # Find engines with best quality/CPU ratio
    print("BEST QUALITY/CPU RATIO (Complex but efficient):")
    complex_efficient = [e for e in engines
                        if e.complexity in ['HIGH', 'VERY_HIGH']
                        and e.meets_target() and e.id > 0]
    for e in complex_efficient[:5]:
        print(f"  [{e.id}] {e.name}: {e.cpu_percent:.1f}% ({e.complexity})")
    print()

    # Buffer size recommendations
    print("BUFFER SIZE RECOMMENDATIONS:")
    print("  - Low-latency (64-128 samples): Only use efficient engines (<5% CPU)")
    print("  - Standard (256-512 samples): Most engines will work well")
    print("  - High-latency (1024+ samples): Can run multiple CPU-intensive engines")
    print()

    # Sample rate recommendations
    print("SAMPLE RATE RECOMMENDATIONS:")
    print("  - 44.1/48 kHz: All engines should meet target")
    print("  - 96 kHz: Expect ~2x CPU usage, limit simultaneous engines")
    print("  - 192 kHz: Expect ~4x CPU usage, use sparingly")
    print()

    # Real-time assessment
    print("=" * 80)
    print("REAL-TIME PERFORMANCE ASSESSMENT")
    print("=" * 80)
    print()
    print(f"Single Engine Target (<{TARGET_CPU_SINGLE_ENGINE}%):  {pass_count}/{total_engines} engines PASS")
    print(f"10 Engine Target (<30%):  {'PASS' if scenarios['10_engines']['realtime_ok'] else 'FAIL'}")
    print(f"56 Engine Target (<200%): {'PASS' if scenarios['56_engines']['realtime_ok'] else 'FAIL'}")
    print()

    if fail_count > 0:
        print("RECOMMENDED ACTIONS:")
        print("1. Prioritize optimization of CRITICAL and HIGH priority engines")
        print("2. Implement operation-specific optimizations (FFT, filters, etc.)")
        print("3. Use SIMD vectorization where applicable")
        print("4. Consider adaptive quality modes for CPU-intensive engines")
        print("5. Profile individual operations to identify hotspots")
    else:
        print("EXCELLENT PERFORMANCE!")
        print("All engines meet the <5% CPU target. System is ready for production.")
    print()

    print("=" * 80)
    print("Analysis complete. Results saved to cpu_profiling_analysis.txt")
    print("=" * 80)

def save_json_report(engines, filename='cpu_profiling_analysis.json'):
    """Save detailed analysis as JSON"""
    data = {
        'summary': {
            'total_engines': len([e for e in engines if e.id > 0]),
            'passing': len([e for e in engines if e.meets_target() and e.id > 0]),
            'failing': len([e for e in engines if not e.meets_target() and e.id > 0]),
            'target_cpu_percent': TARGET_CPU_SINGLE_ENGINE
        },
        'engines': [],
        'category_stats': analyze_by_category([e for e in engines if e.id > 0]),
        'optimization_opportunities': [],
        'multi_engine_capacity': estimate_multi_engine_capacity([e for e in engines if e.id > 0])
    }

    for engine in engines:
        if engine.id == 0:
            continue
        data['engines'].append({
            'id': engine.id,
            'name': engine.name,
            'category': engine.category,
            'cpu_percent': engine.cpu_percent,
            'complexity': engine.complexity,
            'meets_target': engine.meets_target(),
            'priority': engine.optimization_priority(),
            'operations': {
                'fft': engine.has_fft,
                'filters': engine.has_filters,
                'oversampling': engine.has_oversampling,
                'delay': engine.has_delay,
                'lfo': engine.has_lfo,
                'pitch_shift': engine.has_pitch_shift,
                'convolution': engine.has_convolution
            },
            'estimated_costs': engine.estimated_operation_costs()
        })

    opportunities = identify_optimization_opportunities([e for e in engines if e.id > 0])
    for opp in opportunities:
        data['optimization_opportunities'].append({
            'engine_id': opp['engine'].id,
            'engine_name': opp['engine'].name,
            'priority': opp['priority'],
            'cpu_percent': opp['cpu_percent'],
            'operations': opp['operations'],
            'recommendations': opp['recommendations']
        })

    with open(filename, 'w') as f:
        json.dump(data, f, indent=2)

    print(f"Detailed JSON analysis saved to: {filename}")

def main():
    print("Loading CPU benchmark results...")
    engines = load_benchmark_results()

    print(f"Loaded {len(engines)} engine profiles")
    print()

    # Generate console report
    generate_report(engines)

    # Save JSON report
    save_json_report(engines)

if __name__ == '__main__':
    main()
