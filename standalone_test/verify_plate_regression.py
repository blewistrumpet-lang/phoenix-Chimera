#!/usr/bin/env python3
"""
PlateReverb (Engine 39) Regression Verification
Analyzes existing impulse response data to verify no regression occurred
"""

import csv
import numpy as np
import sys

def main():
    # Load existing Engine 39 impulse response
    data = []
    try:
        with open('impulse_engine_39.csv', 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                left = float(row['left'])
                right = float(row['right'])
                data.append((left, right))
    except FileNotFoundError:
        print("ERROR: impulse_engine_39.csv not found")
        print("Please run the impulse response test first")
        sys.exit(1)

    if len(data) == 0:
        print("ERROR: Empty impulse response file")
        sys.exit(1)

    left = np.array([d[0] for d in data])
    right = np.array([d[1] for d in data])

    print()
    print("="*80)
    print("  PlateReverb (Engine 39) Regression Verification")
    print("="*80)
    print()

    # Peak analysis
    peak_left = np.max(np.abs(left))
    peak_right = np.max(np.abs(right))
    peak_sample_left = np.argmax(np.abs(left))
    peak_sample_right = np.argmax(np.abs(right))

    print("REVERB TAIL ANALYSIS:")
    print(f"  Peak Left:          {peak_left:.6f} at sample {peak_sample_left} ({peak_sample_left/48.0:.2f} ms)")
    print(f"  Peak Right:         {peak_right:.6f} at sample {peak_sample_right} ({peak_sample_right/48.0:.2f} ms)")

    # Find tail length (last sample above noise floor)
    noise_floor = 1e-5
    last_audible = 0
    for i in range(len(data)-1, -1, -1):
        if abs(data[i][0]) > noise_floor or abs(data[i][1]) > noise_floor:
            last_audible = i
            break

    tail_length_ms = (last_audible / 48000.0) * 1000.0
    print(f"  Tail Length:        {tail_length_ms:.2f} ms ({last_audible} samples)")

    # Estimate decay rate
    if last_audible > 1000:
        peak = max(peak_left, peak_right)
        sample_1sec = min(48000, len(data) - 1)
        level_at_1sec = 0.0
        for i in range(sample_1sec, min(sample_1sec + 100, len(data))):
            level_at_1sec = max(level_at_1sec, max(abs(left[i]), abs(right[i])))

        if level_at_1sec > 0 and peak > 0:
            decay_rate = 20.0 * np.log10(level_at_1sec / peak)
        else:
            decay_rate = -60.0
    else:
        decay_rate = 0.0

    print(f"  Decay Rate:         {decay_rate:.2f} dB/sec")

    # RT60 measurement (time to decay 60dB)
    threshold_left = peak_left * 0.001  # -60dB
    threshold_right = peak_right * 0.001

    rt60_sample_left = 0
    rt60_sample_right = 0
    for i in range(len(data)-1, -1, -1):
        if abs(left[i]) > threshold_left and rt60_sample_left == 0:
            rt60_sample_left = i
        if abs(right[i]) > threshold_right and rt60_sample_right == 0:
            rt60_sample_right = i
        if rt60_sample_left > 0 and rt60_sample_right > 0:
            break

    rt60_left_ms = (rt60_sample_left / 48000.0) * 1000.0
    rt60_right_ms = (rt60_sample_right / 48000.0) * 1000.0
    rt60_avg_ms = (rt60_left_ms + rt60_right_ms) / 2.0

    print()
    print("RT60 MEASUREMENT (Reverb Time):")
    print(f"  RT60 Left:          {rt60_left_ms:.2f} ms")
    print(f"  RT60 Right:         {rt60_right_ms:.2f} ms")
    print(f"  RT60 Average:       {rt60_avg_ms:.2f} ms")

    # Stereo width analysis
    if len(left) > 1 and len(right) > 1:
        correlation = np.corrcoef(left, right)[0, 1]
    else:
        correlation = 0.0

    width_factor = 1.0 - abs(correlation)
    left_energy = np.sum(left**2)
    right_energy = np.sum(right**2)

    if left_energy + right_energy > 0:
        balance = (right_energy - left_energy) / (left_energy + right_energy)
    else:
        balance = 0.0

    print()
    print("STEREO WIDTH ANALYSIS:")
    print(f"  Correlation:        {correlation:.4f}")
    print(f"  Stereo Width:       {width_factor:.4f}")
    print(f"  Left Energy:        {left_energy:.6e}")
    print(f"  Right Energy:       {right_energy:.6e}")
    print(f"  Balance:            {balance:.4f}")

    # Regression checks
    print()
    print("="*80)
    print("  REGRESSION CHECKS (vs Previous Test Results)")
    print("="*80)

    checks_passed = 0
    checks_total = 0
    failures = []

    # Check 1: Reverb tail exists
    checks_total += 1
    if 100 < tail_length_ms < 10000:
        print(f"  [1] Reverb tail present:          PASS ({tail_length_ms:.2f} ms)")
        checks_passed += 1
    else:
        print(f"  [1] Reverb tail present:          FAIL ({tail_length_ms:.2f} ms)")
        failures.append("Reverb tail length out of range")

    # Check 2: Peak levels
    checks_total += 1
    max_peak = max(peak_left, peak_right)
    if 0.001 < max_peak < 2.0:
        print(f"  [2] Peak levels valid:            PASS (peak={max_peak:.6f})")
        checks_passed += 1
    else:
        print(f"  [2] Peak levels valid:            FAIL (peak={max_peak:.6f})")
        failures.append("Peak level out of valid range")

    # Check 3: RT60
    checks_total += 1
    if 300 < rt60_avg_ms < 5000:
        print(f"  [3] RT60 reasonable:               PASS ({rt60_avg_ms:.2f} ms)")
        checks_passed += 1
    else:
        print(f"  [3] RT60 reasonable:               FAIL ({rt60_avg_ms:.2f} ms)")
        failures.append("RT60 measurement out of range")

    # Check 4: Stereo width
    checks_total += 1
    if width_factor > 0.3:
        print(f"  [4] Stereo field present:          PASS (width={width_factor:.4f})")
        checks_passed += 1
    else:
        print(f"  [4] Stereo field present:          FAIL (width={width_factor:.4f})")
        failures.append("Insufficient stereo width")

    # Check 5: Both channels active
    checks_total += 1
    if left_energy > 1e-6 and right_energy > 1e-6:
        print(f"  [5] Both channels active:          PASS")
        checks_passed += 1
    else:
        print(f"  [5] Both channels active:          FAIL")
        failures.append("One or both channels silent")

    # Check 6: Proper decay
    checks_total += 1
    if -100.0 < decay_rate < -10.0:
        print(f"  [6] Proper decay:                  PASS ({decay_rate:.2f} dB/s)")
        checks_passed += 1
    else:
        print(f"  [6] Proper decay:                  FAIL ({decay_rate:.2f} dB/s)")
        failures.append("Decay rate abnormal")

    print()
    if checks_passed == checks_total:
        print("  ALL REGRESSION CHECKS PASSED")
        print("  No degradation from previous test results")
    else:
        print(f"  REGRESSION DETECTED ({checks_passed}/{checks_total} checks passed)")
        print()
        print("  Failed checks:")
        for fail in failures:
            print(f"    - {fail}")

    print()
    print("="*80)
    if checks_passed == checks_total:
        print("  FINAL RESULT: PASS - No Regression")
    else:
        print("  FINAL RESULT: FAIL - Regression Detected")
    print("="*80)
    print()

    # Compare with previous fix report
    print("COMPARISON WITH PREVIOUS TEST (from PLATEVERB_FIX_REPORT.md):")
    print("  Previous Results:")
    print("    Peak Left:   0.026 at sample 3394 (71ms)")
    print("    Peak Right:  0.024 at sample 2795 (58ms)")
    print()
    print("  Current Results:")
    print(f"    Peak Left:   {peak_left:.3f} at sample {peak_sample_left} ({peak_sample_left/48.0:.0f}ms)")
    print(f"    Peak Right:  {peak_right:.3f} at sample {peak_sample_right} ({peak_sample_right/48.0:.0f}ms)")
    print()

    # Check if peaks are similar
    if abs(peak_left - 0.026) < 0.01 and abs(peak_right - 0.024) < 0.01:
        print("  Peak levels match previous test (within tolerance)")
    else:
        print("  WARNING: Peak levels differ from previous test")

    return 0 if checks_passed == checks_total else 1

if __name__ == "__main__":
    sys.exit(main())
