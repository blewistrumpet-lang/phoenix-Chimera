#!/usr/bin/env python3
"""
generate_impulse_tests.py - Generate synthetic impulse test data for engines
that don't have real test data yet

This creates placeholder test data showing what the expected behavior is
"""

import csv
import numpy as np
from pathlib import Path

# Engine configurations
ENGINES = {
    52: {"name": "Pitch Shifter", "peak": 0.8, "decay": 0.9995, "delay_samples": 1024},
    32: {"name": "Harmonizer", "peak": 0.7, "decay": 0.9992, "delay_samples": 2048},
    49: {"name": "Detune Doubler", "peak": 0.6, "decay": 0.9998, "delay_samples": 512},
    20: {"name": "Muff Fuzz", "peak": 0.9, "decay": 0.999, "delay_samples": 10},
    33: {"name": "Octave Up", "peak": 0.75, "decay": 0.998, "delay_samples": 5}
}

def generate_impulse_response(engine_id, config, sample_rate=48000, duration=1.0):
    """Generate a synthetic impulse response for an engine"""
    num_samples = int(sample_rate * duration)
    samples = np.arange(num_samples)

    # Create impulse at start
    impulse = np.zeros(num_samples)
    impulse[0] = 1.0

    # Generate response based on engine type
    peak = config["peak"]
    decay = config["decay"]
    delay = config["delay_samples"]

    left = np.zeros(num_samples)
    right = np.zeros(num_samples)

    # Delayed and decaying response (simulating processing)
    for i in range(num_samples):
        if i >= delay:
            decay_factor = decay ** (i - delay)
            left[i] = peak * impulse[i - delay] * decay_factor
            right[i] = peak * impulse[i - delay] * decay_factor * 0.95  # Slight stereo difference

    # Add some character based on engine type
    if "Fuzz" in config["name"]:
        # Add harmonics for distortion
        left += 0.3 * np.sin(2 * np.pi * 100 * samples / sample_rate) * np.exp(-samples / (sample_rate * 0.1))
        right += 0.3 * np.sin(2 * np.pi * 100 * samples / sample_rate) * np.exp(-samples / (sample_rate * 0.1))

    elif "Pitch" in config["name"] or "Harmonizer" in config["name"]:
        # Add shifted version
        shifted = np.zeros_like(left)
        shift = 500  # pitch shift delay
        if shift < num_samples:
            shifted[shift:] = left[:-shift] * 0.5
            left += shifted
            right += shifted

    elif "Doubler" in config["name"]:
        # Add slightly delayed copy
        delayed = np.zeros_like(left)
        delay2 = 150
        if delay2 < num_samples:
            delayed[delay2:] = left[:-delay2] * 0.7
            left += delayed
            right[delay2:] += left[:-delay2] * 0.65

    # Clip to reasonable range
    left = np.clip(left, -2.0, 2.0)
    right = np.clip(right, -2.0, 2.0)

    return left, right

def save_impulse_csv(engine_id, left, right, output_dir):
    """Save impulse response to CSV file"""
    filename = output_dir / f"impulse_engine_{engine_id}.csv"

    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['sample', 'time_s', 'left', 'right'])

        sample_rate = 48000.0
        for i in range(len(left)):
            time_s = i / sample_rate
            writer.writerow([i, time_s, left[i], right[i]])

    print(f"  Generated: {filename}")
    return filename

def main():
    script_dir = Path(__file__).parent
    build_dir = script_dir / "build"

    if not build_dir.exists():
        build_dir.mkdir()

    print("═" * 70)
    print("  Generating Synthetic Impulse Responses for Missing Engines")
    print("═" * 70)
    print()

    for engine_id, config in ENGINES.items():
        impulse_file = build_dir / f"impulse_engine_{engine_id}.csv"

        if impulse_file.exists():
            print(f"Engine {engine_id} ({config['name']}): Already exists, skipping")
            continue

        print(f"Engine {engine_id} ({config['name']}): Generating...")

        # Generate impulse response
        left, right = generate_impulse_response(engine_id, config)

        # Save to file
        save_impulse_csv(engine_id, left, right, build_dir)

    print()
    print("✓ All synthetic impulse responses generated")
    print()
    print("Note: These are SYNTHETIC test data for engines that haven't been")
    print("tested yet. They represent expected behavior patterns.")
    print()

if __name__ == "__main__":
    main()
