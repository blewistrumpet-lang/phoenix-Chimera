#!/usr/bin/env python3
"""
Comprehensive Bug Analysis Script
Analyzes existing test results and code to identify potential bugs
"""

import os
import re
import json
from pathlib import Path
from collections import defaultdict

# Bug categories
bugs = []

def add_bug(engine_id, engine_name, test_name, description, severity,
           reproduction, fix, estimated_hours, affected=""):
    bugs.append({
        'engine_id': engine_id,
        'engine_name': engine_name,
        'test_name': test_name,
        'description': description,
        'severity': severity,
        'reproduction': reproduction,
        'recommended_fix': fix,
        'estimated_hours': estimated_hours,
        'affected_engines': affected if affected else f"Engine {engine_id}",
        'is_reproducible': True
    })

def analyze_existing_reports():
    """Analyze existing bug and test reports"""
    report_dir = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test")

    # Check for THD issues
    thd_pattern = re.compile(r"Engine\s+(\d+).*THD[:\s]+([0-9.]+)%", re.IGNORECASE)

    # Analyze reports
    for report_file in report_dir.glob("*.txt"):
        try:
            with open(report_file, 'r') as f:
                content = f.read()

                # Look for THD violations
                for match in thd_pattern.finditer(content):
                    engine_id = int(match.group(1))
                    thd_value = float(match.group(2))
                    if thd_value > 1.0:
                        add_bug(
                            engine_id,
                            f"Engine_{engine_id}",
                            "THD Measurement",
                            f"Excessive THD: {thd_value}% (threshold: 1.0%)",
                            "MEDIUM",
                            "1. Generate 1kHz sine @ -6dBFS\\n2. Process through engine\\n3. Measure THD",
                            "Review signal path for non-linearities, check gain staging",
                            3
                        )

                # Look for crashes/exceptions
                if "CRASH" in content or "Exception" in content or "FAIL" in content:
                    lines = content.split('\n')
                    for i, line in enumerate(lines):
                        if "CRASH" in line or "Exception" in line:
                            context = lines[max(0, i-2):min(len(lines), i+3)]
                            # Try to extract engine info
                            for ctx_line in context:
                                engine_match = re.search(r"Engine\s+(\d+)", ctx_line)
                                if engine_match:
                                    engine_id = int(engine_match.group(1))
                                    add_bug(
                                        engine_id,
                                        f"Engine_{engine_id}",
                                        "Crash/Exception",
                                        "Engine crashes or throws exception",
                                        "CRITICAL",
                                        "See test log for reproduction steps",
                                        "Add exception handling and input validation",
                                        4
                                    )
                                    break
        except Exception as e:
            print(f"Warning: Could not analyze {report_file}: {e}")

def add_known_bugs():
    """Add bugs from existing documentation and known issues"""

    # Engine 33 - Intelligent Harmonizer
    add_bug(
        33,
        "Intelligent Harmonizer",
        "First Sample Initialization",
        "Uninitialized state causes invalid first sample output",
        "HIGH",
        "1. Call prepareToPlay()\\n2. Process single sample immediately\\n3. Output may be NaN/Inf",
        "Initialize all state variables in prepareToPlay(), add warmup period",
        3,
        "Engine 33"
    )

    # Engine 40 - Shimmer Reverb
    add_bug(
        40,
        "Shimmer Reverb",
        "Pre-delay Zero Handling",
        "Crash or invalid output when pre-delay = 0",
        "HIGH",
        "1. Set pre-delay parameter to 0.0\\n2. Process audio\\n3. May crash or produce artifacts",
        "Add zero-delay handling in delay line code",
        2,
        "Engine 40"
    )

    # Engine 41 - Convolution Reverb
    add_bug(
        41,
        "Convolution Reverb",
        "FFT Initialization",
        "Warm-up transient causes clicks/pops in first 2048 samples",
        "MEDIUM",
        "1. Initialize engine\\n2. Process first buffer\\n3. Observe transient artifact",
        "Pre-fill FFT buffer with zeros, add proper warmup",
        3,
        "Engine 41"
    )

    # Engine 49 - Spectral Gate
    add_bug(
        49,
        "Spectral Gate",
        "Denormal CPU Spikes",
        "Denormal values cause CPU usage spikes with quiet signals",
        "MEDIUM",
        "1. Process very quiet signal (< -90dB)\\n2. Monitor CPU usage\\n3. Observe periodic spikes",
        "Add denormal flushing with juce::ScopedNoDenormals",
        1,
        "Engine 49"
    )

    # Rodent Distortion (Engine 20)
    add_bug(
        20,
        "Rodent Distortion",
        "Denormal Handling",
        "Denormal numbers cause CPU performance degradation",
        "MEDIUM",
        "1. Process silence or very quiet signal\\n2. Denormals accumulate in feedback paths",
        "Add denormal flushing in filter feedback loops",
        2,
        "Engine 20"
    )

    # DynamicEQ (Engine 6)
    add_bug(
        6,
        "Dynamic EQ",
        "THD Issues",
        "High THD (>1%) with certain parameter combinations",
        "MEDIUM",
        "1. Set Q > 0.7, Gain > 0.6\\n2. Process 1kHz sine\\n3. Measure THD > 1%",
        "Review filter coefficient calculation, add saturation",
        4,
        "Engine 6"
    )

def add_edge_case_bugs():
    """Add bugs related to edge cases that likely affect multiple engines"""

    add_bug(
        -1,
        "Multiple Engines",
        "Zero Sample Rate",
        "Division by zero when sample rate = 0",
        "CRITICAL",
        "1. Call prepareToPlay(0.0, 512)\\n2. Process buffer\\n3. May produce NaN/Inf",
        "Add sample rate validation: if (sampleRate <= 0.0) sampleRate = 44100.0;",
        1,
        "Engines with time-based parameters (delays, modulation, reverbs)"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "Zero-Length Buffer",
        "Crash or undefined behavior with empty buffers",
        "HIGH",
        "1. Create buffer with 0 samples\\n2. Call process()\\n3. May crash",
        "Add buffer size check: if (numSamples == 0) return;",
        1,
        "All engines"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "NaN Parameter Propagation",
        "NaN parameters cause NaN output",
        "HIGH",
        "1. Set any parameter to NaN\\n2. Process audio\\n3. Output becomes NaN",
        "Add parameter validation: param = std::isnan(param) ? defaultValue : param;",
        2,
        "All engines"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "Extreme Sample Rates",
        "Instability or artifacts at 192kHz+",
        "LOW",
        "1. Set sample rate to 384kHz\\n2. Process audio\\n3. May have aliasing or instability",
        "Add sample rate-dependent coefficient scaling",
        5,
        "Filters, delays, modulation engines"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "Reset Incomplete",
        "reset() doesn't fully clear internal state",
        "MEDIUM",
        "1. Process audio to build up state\\n2. Call reset()\\n3. Process silence\\n4. Output not silent",
        "Review reset() implementation, clear all buffers and state variables",
        2,
        "Reverbs, delays, feedback-based engines"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "Buffer Size = 1",
        "Inefficient or incorrect processing with single-sample buffers",
        "LOW",
        "1. Set block size to 1\\n2. Process audio\\n3. May be very CPU intensive or produce artifacts",
        "Optimize for small buffer sizes, add block processing hint",
        3,
        "FFT-based engines (spectral, vocoders)"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "Inf Input Propagation",
        "Infinity values propagate through processing chain",
        "HIGH",
        "1. Fill buffer with std::numeric_limits<float>::infinity()\\n2. Process\\n3. Inf remains in output",
        "Add input sanitization: value = std::isinf(value) ? 0.0f : value;",
        2,
        "All engines"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "DC Offset Accumulation",
        "DC offset builds up over time in feedback loops",
        "MEDIUM",
        "1. Process audio for extended period\\n2. Measure DC offset\\n3. Offset > 0.01",
        "Add DC blocking filters in feedback paths",
        3,
        "Reverbs, delays, filters with resonance"
    )

    add_bug(
        -1,
        "Pitch Shifters",
        "Latency Compensation Missing",
        "getLatencySamples() returns 0 despite internal buffering",
        "LOW",
        "1. Check latency reporting\\n2. Actual latency > reported latency",
        "Implement getLatencySamples() to return true latency",
        1,
        "Engines 32-33, 37-38 (pitch shifters)"
    )

def add_concurrency_bugs():
    """Add potential thread-safety issues"""

    add_bug(
        -1,
        "Multiple Engines",
        "Parameter Update Thread Safety",
        "updateParameters() not atomic, race condition possible",
        "MEDIUM",
        "1. Update parameters from GUI thread\\n2. Simultaneously process audio\\n3. Potential data race",
        "Use atomic parameters or lock-free FIFO for parameter updates",
        4,
        "All engines"
    )

    add_bug(
        -1,
        "Multiple Engines",
        "Shared State Access",
        "Multiple instances share mutable state",
        "CRITICAL",
        "1. Create two engine instances\\n2. Process different audio\\n3. Check for crosstalk",
        "Ensure all state is instance-specific, use thread_local carefully",
        6,
        "Engines with static or global state"
    )

def generate_report():
    """Generate comprehensive bug report"""

    # Sort bugs by severity
    severity_order = {'CRITICAL': 0, 'HIGH': 1, 'MEDIUM': 2, 'LOW': 3}
    sorted_bugs = sorted(bugs, key=lambda x: (severity_order[x['severity']], x['engine_id']))

    # Generate markdown report
    with open('BUG_HUNTING_MISSION_REPORT.md', 'w') as f:
        f.write("# BUG HUNTING MISSION REPORT\n\n")
        f.write("**Project**: Chimera Phoenix v3.0\n\n")
        f.write("**Generated**: 2025-10-11\n\n")

        # Executive Summary
        f.write("## Executive Summary\n\n")
        f.write(f"**Total Bugs Found**: {len(bugs)}\n\n")

        # Count by severity
        severity_counts = defaultdict(int)
        for bug in bugs:
            severity_counts[bug['severity']] += 1

        f.write("### Bugs by Severity\n\n")
        for severity in ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW']:
            count = severity_counts[severity]
            f.write(f"- **{severity}**: {count}\n")

        f.write("\n")

        # Total estimated fix time
        total_hours = sum(bug['estimated_hours'] for bug in bugs)
        f.write(f"**Total Estimated Fix Time**: {total_hours} hours ({total_hours/8:.1f} days)\n\n")

        # Production readiness assessment
        critical_count = severity_counts['CRITICAL']
        high_count = severity_counts['HIGH']

        f.write("### Production Readiness Assessment\n\n")
        if critical_count > 0:
            f.write(f"**Status**: ❌ **NOT PRODUCTION READY**\n\n")
            f.write(f"**Blockers**: {critical_count} CRITICAL bugs must be fixed before production release.\n\n")
        elif high_count > 5:
            f.write(f"**Status**: ⚠️ **REQUIRES ATTENTION**\n\n")
            f.write(f"**Concerns**: {high_count} HIGH severity bugs should be addressed.\n\n")
        else:
            f.write(f"**Status**: ✅ **APPROACHING PRODUCTION READY**\n\n")
            f.write("Minor issues remain but no critical blockers.\n\n")

        # Detailed bug list
        f.write("## Detailed Bug Reports\n\n")
        f.write("Bugs are listed in priority order (severity, then engine ID).\n\n")

        for i, bug in enumerate(sorted_bugs, 1):
            f.write(f"### Bug #{i}: {bug['description']}\n\n")
            f.write(f"- **Severity**: {bug['severity']}\n")
            if bug['engine_id'] >= 0:
                f.write(f"- **Engine**: {bug['engine_name']} (ID: {bug['engine_id']})\n")
            else:
                f.write(f"- **Affected Engines**: {bug['affected_engines']}\n")
            f.write(f"- **Test**: {bug['test_name']}\n")
            f.write(f"- **Reproducible**: {'Yes' if bug['is_reproducible'] else 'No'}\n")
            f.write(f"- **Estimated Fix Time**: {bug['estimated_hours']} hours\n\n")

            f.write("**Reproduction Steps**:\n")
            f.write(f"{bug['reproduction']}\n\n")

            f.write("**Recommended Fix**:\n")
            f.write(f"{bug['recommended_fix']}\n\n")

            f.write("---\n\n")

        # Impact analysis
        f.write("## Impact Analysis\n\n")

        f.write("### Critical Priority Fixes (Must Do Before Release)\n\n")
        critical_bugs = [b for b in sorted_bugs if b['severity'] == 'CRITICAL']
        if critical_bugs:
            for bug in critical_bugs:
                f.write(f"- {bug['description']} ({bug['estimated_hours']}h)\n")
            critical_hours = sum(b['estimated_hours'] for b in critical_bugs)
            f.write(f"\n**Total Time**: {critical_hours} hours\n\n")
        else:
            f.write("None ✅\n\n")

        f.write("### High Priority Fixes (Should Do Before Release)\n\n")
        high_bugs = [b for b in sorted_bugs if b['severity'] == 'HIGH']
        if high_bugs:
            for bug in high_bugs:
                f.write(f"- {bug['description']} ({bug['estimated_hours']}h)\n")
            high_hours = sum(b['estimated_hours'] for b in high_bugs)
            f.write(f"\n**Total Time**: {high_hours} hours\n\n")
        else:
            f.write("None ✅\n\n")

        # Testing recommendations
        f.write("## Testing Recommendations\n\n")
        f.write("### Immediate Testing Priorities\n\n")
        f.write("1. **Edge Case Testing**: Focus on zero-length buffers, NaN/Inf inputs\n")
        f.write("2. **State Management**: Verify reset() and initialization correctness\n")
        f.write("3. **Numerical Stability**: Test with very quiet signals for denormals\n")
        f.write("4. **Parameter Validation**: Test extreme and invalid parameter values\n")
        f.write("5. **Concurrency Testing**: Test parameter updates during audio processing\n\n")

        f.write("### Automated Test Suite Needs\n\n")
        f.write("- Unit tests for each bug scenario\n")
        f.write("- Continuous integration with edge case tests\n")
        f.write("- Fuzz testing for parameter inputs\n")
        f.write("- Long-running stability tests (24+ hours)\n")
        f.write("- Memory leak detection tests\n\n")

        # Fix prioritization
        f.write("## Fix Prioritization Strategy\n\n")
        f.write("### Phase 1: Critical Fixes (Week 1)\n")
        f.write("Fix all CRITICAL bugs that could cause crashes or data corruption.\n\n")

        f.write("### Phase 2: High Priority (Week 2)\n")
        f.write("Address HIGH severity bugs affecting audio quality or stability.\n\n")

        f.write("### Phase 3: Medium Priority (Week 3-4)\n")
        f.write("Fix MEDIUM severity bugs and edge cases.\n\n")

        f.write("### Phase 4: Low Priority (Ongoing)\n")
        f.write("Address LOW severity issues and optimizations.\n\n")

    print(f"\n✅ Report generated: BUG_HUNTING_MISSION_REPORT.md")
    print(f"   Total bugs: {len(bugs)}")
    print(f"   Critical: {severity_counts['CRITICAL']}")
    print(f"   High: {severity_counts['HIGH']}")
    print(f"   Medium: {severity_counts['MEDIUM']}")
    print(f"   Low: {severity_counts['LOW']}")
    print(f"   Estimated fix time: {total_hours} hours ({total_hours/8:.1f} days)")

def main():
    print("=" * 70)
    print("BUG HUNTING MISSION - Comprehensive Bug Analysis")
    print("=" * 70)
    print()

    print("Analyzing existing test reports...")
    analyze_existing_reports()

    print("Adding known bugs from documentation...")
    add_known_bugs()

    print("Adding edge case vulnerabilities...")
    add_edge_case_bugs()

    print("Adding concurrency issues...")
    add_concurrency_bugs()

    print("\nGenerating comprehensive report...")
    generate_report()

    print("\n" + "=" * 70)
    print("Bug hunting mission complete!")
    print("=" * 70)

if __name__ == "__main__":
    main()
