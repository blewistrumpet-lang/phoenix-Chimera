#!/usr/bin/env python3
"""
Analyze Spring & Gated Reverb Impulse Response Data
Engines 42-43: SpringReverb and GatedReverb
"""

import csv
import math
import sys
from pathlib import Path

def load_impulse_data(filename):
    """Load impulse response data from CSV"""
    samples_l = []
    samples_r = []

    with open(filename, 'r') as f:
        reader = csv.reader(f)
        header = next(reader, None)  # Skip header
        for row in reader:
            if len(row) >= 4:
                try:
                    # Format: sample,time_s,left,right
                    samples_l.append(float(row[2]))
                    samples_r.append(float(row[3]))
                except ValueError:
                    continue

    return samples_l, samples_r

def measure_rt60(samples, sample_rate=48000):
    """Measure RT60 (time for 60dB decay)"""
    # Find peak
    peak = max(abs(s) for s in samples)
    if peak < 1e-6:
        return 0.0

    peak_idx = 0
    for i, s in enumerate(samples):
        if abs(s) == peak:
            peak_idx = i
            break

    # Find where signal drops to -60dB (1/1000 of peak)
    threshold = peak * 0.001

    for i in range(peak_idx, len(samples)):
        if abs(samples[i]) < threshold:
            return (i - peak_idx) / sample_rate

    return (len(samples) - peak_idx) / sample_rate

def measure_edt(samples, sample_rate=48000):
    """Measure Early Decay Time (first 10dB)"""
    peak = max(abs(s) for s in samples)
    if peak < 1e-6:
        return 0.0

    peak_idx = 0
    for i, s in enumerate(samples):
        if abs(s) == peak:
            peak_idx = i
            break

    # -10dB threshold
    threshold = peak * 0.316

    for i in range(peak_idx, len(samples)):
        if abs(samples[i]) < threshold:
            return (i - peak_idx) / sample_rate

    return 0.0

def measure_stereo_width(left, right):
    """Measure stereo width via inter-channel correlation"""
    if len(left) != len(right):
        return 0.0

    sum_ll = sum(l*l for l in left)
    sum_rr = sum(r*r for r in right)
    sum_lr = sum(l*r for l, r in zip(left, right))

    denom = math.sqrt(sum_ll * sum_rr)
    if denom < 1e-10:
        return 0.0

    return sum_lr / denom

def detect_gating(samples, sample_rate=48000):
    """Detect gating behavior"""
    # Find peak
    peak = max(abs(s) for s in samples)
    if peak < 1e-6:
        return False, 0.0

    # Look for sudden drops in amplitude
    window_size = 512
    prev_rms = peak

    for i in range(window_size, len(samples), window_size):
        window = samples[i:i+window_size]
        if len(window) == 0:
            continue
        rms = math.sqrt(sum(s*s for s in window) / len(window))

        # Check for sudden drop (more than 20dB)
        if rms < prev_rms and prev_rms > 1e-10 and rms > 1e-10:
            drop_db = 20 * math.log10(prev_rms / rms)
            if drop_db > 20 and rms < prev_rms * 0.1:
                threshold_db = 20 * math.log10(rms / peak) if rms > 0 else -999.0
                return True, threshold_db

        if rms > 1e-10:
            prev_rms = rms

    return False, 0.0

def analyze_engine(engine_id, engine_name):
    """Analyze a single reverb engine"""
    filename = f"impulse_engine_{engine_id}.csv"

    if not Path(filename).exists():
        print(f"\n{'='*70}")
        print(f"Engine {engine_id}: {engine_name}")
        print(f"{'='*70}")
        print(f"ERROR: No impulse data found ({filename})")
        print("RESULT: FAIL - Unable to test")
        return False

    # Load data
    left, right = load_impulse_data(filename)

    if not left or not right:
        print(f"\n{'='*70}")
        print(f"Engine {engine_id}: {engine_name}")
        print(f"{'='*70}")
        print(f"ERROR: Empty or invalid impulse data")
        print("RESULT: FAIL - Invalid data")
        return False

    # Calculate metrics
    rt60_l = measure_rt60(left)
    rt60_r = measure_rt60(right)
    rt60 = (rt60_l + rt60_r) / 2

    edt_l = measure_edt(left)
    edt_r = measure_edt(right)
    edt = (edt_l + edt_r) / 2

    stereo_width = measure_stereo_width(left, right)

    has_gating, gate_threshold = detect_gating(left)

    peak_l = max(abs(s) for s in left)
    peak_r = max(abs(s) for s in right)
    peak = max(peak_l, peak_r)

    # Tail amplitude (last 10%)
    tail_start = int(len(left) * 0.9)
    tail_l = left[tail_start:]
    tail_rms = math.sqrt(sum(s*s for s in tail_l) / len(tail_l))

    # DC offset
    dc_offset = abs(sum(left) / len(left))

    # Print results
    print(f"\n{'='*70}")
    print(f"Engine {engine_id}: {engine_name}")
    print(f"{'='*70}\n")

    print("IMPULSE RESPONSE ANALYSIS:")
    print(f"  Sample count:    {len(left):,} samples (~{len(left)/48000:.2f}s @ 48kHz)")
    print(f"  Peak amplitude:  {peak:.4f}")
    print(f"  Tail amplitude:  {tail_rms:.6e}")
    print(f"  DC offset:       {dc_offset:.6e}")

    print("\nDECAY CHARACTERISTICS (RT60):")
    print(f"  RT60:            {rt60:.3f} seconds", end="")
    if rt60 < 0.05:
        print(" (too short)")
    elif rt60 > 10.0:
        print(" (very long)")
    else:
        print(" (normal)")

    print(f"  Early Decay:     {edt:.3f} seconds")

    if edt > 0:
        decay_ratio = (rt60 - edt) / edt if edt > 0 else 0
        print(f"  Decay linearity: {decay_ratio:.2f}")

    print("\nSTEREO WIDTH:")
    print(f"  Correlation:     {stereo_width:.3f}", end="")
    if stereo_width > 0.7:
        print(" (narrow/mono)")
    elif stereo_width < 0.0:
        print(" (wide/decorrelated)")
    else:
        print(" (good stereo)")

    print("\nARTIFACT DETECTION:")
    print(f"  Gating:          {'DETECTED' if has_gating else 'None'}", end="")
    if has_gating:
        print(f" (threshold: {gate_threshold:.1f} dB)")
    else:
        print("")

    # PASS/FAIL criteria
    print("\n" + "="*70)
    print("TEST RESULTS:")
    print("="*70)

    pass_rt60 = 0.05 < rt60 < 15.0
    pass_stereo = -0.9 < stereo_width < 0.9
    pass_peak = 0.001 < peak < 100.0
    pass_dc = dc_offset < 0.01

    print(f"  RT60 Valid:      {'PASS' if pass_rt60 else 'FAIL':6} (0.05s - 15s)")
    print(f"  Stereo Image:    {'PASS' if pass_stereo else 'FAIL':6} (-0.9 to 0.9)")
    print(f"  Peak Amplitude:  {'PASS' if pass_peak else 'FAIL':6} (0.001 - 100.0)")
    print(f"  DC Offset:       {'PASS' if pass_dc else 'FAIL':6} (< 0.01)")

    # Special check for GatedReverb (should have gating)
    if engine_id == 43:
        pass_gating = has_gating
        print(f"  Gating Present:  {'PASS' if pass_gating else 'FAIL':6} (expected for GatedReverb)")
        overall = pass_rt60 and pass_stereo and pass_peak and pass_dc and pass_gating
    else:
        overall = pass_rt60 and pass_stereo and pass_peak and pass_dc

    print("\n" + "-"*70)
    print(f"  OVERALL:         {'PASSED' if overall else 'FAILED':6}")
    print("-"*70 + "\n")

    return overall

def main():
    """Main analysis function"""
    print("\n" + "="*70)
    print("  ChimeraPhoenix Spring & Gated Reverb Test Suite")
    print("  Engines 42-43: Impulse Response & Reverb Metrics Analysis")
    print("="*70)

    results = {}

    # Engine 42: SpringReverb
    results[42] = analyze_engine(42, "Spring Reverb")

    # Engine 43: GatedReverb
    results[43] = analyze_engine(43, "Gated Reverb")

    # Final summary
    print("\n" + "="*70)
    print("FINAL SUMMARY")
    print("="*70)

    for engine_id, name in [(42, "Spring Reverb"), (43, "Gated Reverb")]:
        result = results.get(engine_id, False)
        status = "PASSED" if result else "FAILED"
        print(f"  Engine {engine_id}: {name:20} {status}")

    all_pass = all(results.values())
    print("\n" + "-"*70)
    print(f"  ALL TESTS:       {'PASSED' if all_pass else 'FAILED'}")
    print("-"*70 + "\n")

    return 0 if all_pass else 1

if __name__ == "__main__":
    sys.exit(main())
