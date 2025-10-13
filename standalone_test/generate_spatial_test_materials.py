#!/usr/bin/env python3
"""
Generate Real-World Stereo Test Materials for Spatial Engine Testing
Engines 46, 53, 56: StereoImager, MidSideProcessor_Platinum, PhaseAlignPlatinum
"""

import numpy as np
import struct

# Test configuration
SAMPLE_RATE = 48000
DURATION = 2.0  # seconds
NUM_SAMPLES = int(SAMPLE_RATE * DURATION)

def write_stereo_raw(filename, left, right):
    """Write stereo interleaved float32 RAW file"""
    interleaved = np.empty(len(left) * 2, dtype=np.float32)
    interleaved[0::2] = left
    interleaved[1::2] = right

    with open(filename, 'wb') as f:
        f.write(interleaved.tobytes())

    print(f"✓ {filename} ({len(left)} samples, {len(left)/SAMPLE_RATE:.2f}s)")

def generate_stereo_drums():
    """Generate realistic stereo drum mix with panned elements"""
    t = np.arange(NUM_SAMPLES) / SAMPLE_RATE

    # Kick (center, mono) - 120 BPM
    kick_pattern = np.zeros(NUM_SAMPLES)
    beat_interval = int(SAMPLE_RATE * 0.5)  # 120 BPM
    for i in range(0, NUM_SAMPLES, beat_interval):
        # Exponential decay kick
        decay = np.exp(-np.arange(2000) * 10.0 / SAMPLE_RATE)
        kick_osc = np.sin(2 * np.pi * 60 * np.arange(2000) / SAMPLE_RATE)
        kick_click = np.random.randn(2000) * 0.3
        kick = (kick_osc * 0.7 + kick_click * 0.3) * decay

        end_idx = min(i + len(kick), NUM_SAMPLES)
        kick_pattern[i:end_idx] += kick[:end_idx - i]

    # Snare (center, with reverb tail) - on beats 2 & 4
    snare_pattern = np.zeros(NUM_SAMPLES)
    for i in range(beat_interval, NUM_SAMPLES, beat_interval * 2):
        # Noise-based snare with tone
        snare_noise = np.random.randn(4000) * 0.6
        snare_tone = np.sin(2 * np.pi * 200 * np.arange(4000) / SAMPLE_RATE)
        decay = np.exp(-np.arange(4000) * 8.0 / SAMPLE_RATE)
        snare = (snare_noise * 0.7 + snare_tone * 0.3) * decay

        end_idx = min(i + len(snare), NUM_SAMPLES)
        snare_pattern[i:end_idx] += snare[:end_idx - i]

    # Hi-hat (panned right 70%) - 16th notes
    hihat_pattern = np.zeros(NUM_SAMPLES)
    hihat_interval = int(SAMPLE_RATE * 0.125)
    for i in range(0, NUM_SAMPLES, hihat_interval):
        # High-frequency noise burst
        hihat_noise = np.random.randn(600)
        # High-pass emphasis
        hihat_filtered = np.convolve(hihat_noise, [0.3, -0.7], mode='same')
        decay = np.exp(-np.arange(600) * 20.0 / SAMPLE_RATE)
        hihat = hihat_filtered * decay * 0.4

        end_idx = min(i + len(hihat), NUM_SAMPLES)
        hihat_pattern[i:end_idx] += hihat[:end_idx - i]

    # Room tone/ambience (stereo decorrelation)
    room_l = np.random.randn(NUM_SAMPLES) * 0.02
    room_r = np.random.randn(NUM_SAMPLES) * 0.02

    # Mix stereo image
    left = kick_pattern * 0.8 + snare_pattern * 0.7 + hihat_pattern * 0.3 + room_l
    right = kick_pattern * 0.8 + snare_pattern * 0.7 + hihat_pattern * 0.7 + room_r

    # Normalize
    peak = max(np.abs(left).max(), np.abs(right).max())
    left = left / peak * 0.8
    right = right / peak * 0.8

    return left.astype(np.float32), right.astype(np.float32)

def generate_double_tracked_guitar():
    """Generate double-tracked guitar (two takes, slight variations)"""
    t = np.arange(NUM_SAMPLES) / SAMPLE_RATE

    # Simulate guitar chord progression (power chords)
    freqs = [82.41, 110.0, 98.0, 73.42]  # E2, A2, G2, D2
    chord_duration = int(SAMPLE_RATE * 0.5)

    # Take 1 (left) - slightly ahead in time
    take1 = np.zeros(NUM_SAMPLES)
    phase1 = 0
    for i, freq in enumerate(freqs * 4):  # Repeat progression
        start_idx = i * chord_duration
        if start_idx >= NUM_SAMPLES:
            break

        end_idx = min(start_idx + chord_duration, NUM_SAMPLES)
        dur = end_idx - start_idx
        t_seg = np.arange(dur) / SAMPLE_RATE

        # Fundamental + harmonics
        fundamental = np.sin(2 * np.pi * freq * t_seg + phase1)
        harmonic2 = np.sin(2 * np.pi * freq * 2 * t_seg + phase1 * 1.3) * 0.5
        harmonic3 = np.sin(2 * np.pi * freq * 3 * t_seg + phase1 * 1.7) * 0.3

        # Envelope (attack-sustain-release)
        attack = int(dur * 0.05)
        release = int(dur * 0.2)
        envelope = np.ones(dur)
        envelope[:attack] = np.linspace(0, 1, attack)
        envelope[-release:] = np.linspace(1, 0.1, release)

        guitar = (fundamental + harmonic2 + harmonic3) * envelope
        take1[start_idx:end_idx] = guitar
        phase1 += 2 * np.pi * freq * dur / SAMPLE_RATE

    # Take 2 (right) - slightly delayed, different tone
    take2 = np.zeros(NUM_SAMPLES)
    phase2 = 0.3  # Different starting phase
    for i, freq in enumerate(freqs * 4):
        start_idx = i * chord_duration + 120  # 2.5ms delay
        if start_idx >= NUM_SAMPLES:
            break

        end_idx = min(start_idx + chord_duration, NUM_SAMPLES)
        dur = end_idx - start_idx
        t_seg = np.arange(dur) / SAMPLE_RATE

        fundamental = np.sin(2 * np.pi * freq * t_seg + phase2)
        harmonic2 = np.sin(2 * np.pi * freq * 2 * t_seg + phase2 * 1.4) * 0.45
        harmonic3 = np.sin(2 * np.pi * freq * 3 * t_seg + phase2 * 1.8) * 0.35

        attack = int(dur * 0.06)
        release = int(dur * 0.25)
        envelope = np.ones(dur)
        envelope[:attack] = np.linspace(0, 1, attack)
        envelope[-release:] = np.linspace(1, 0.1, release)

        guitar = (fundamental + harmonic2 + harmonic3) * envelope
        take2[start_idx:end_idx] = guitar
        phase2 += 2 * np.pi * freq * dur / SAMPLE_RATE

    # Normalize
    peak = max(np.abs(take1).max(), np.abs(take2).max())
    take1 = take1 / peak * 0.7
    take2 = take2 / peak * 0.7

    return take1.astype(np.float32), take2.astype(np.float32)

def generate_stereo_mix():
    """Generate full stereo mix (bass center, keys left, pad right)"""
    t = np.arange(NUM_SAMPLES) / SAMPLE_RATE

    # Bass line (center, mono)
    bass_freq = 55.0  # A1
    bass_pattern = [0, 0, 7, 5, 3, 3, 7, 5]  # Semitone offsets
    beat_dur = int(SAMPLE_RATE * 0.25)

    bass = np.zeros(NUM_SAMPLES)
    for i, offset in enumerate(bass_pattern * 8):
        start_idx = i * beat_dur
        if start_idx >= NUM_SAMPLES:
            break

        freq = bass_freq * (2 ** (offset / 12.0))
        end_idx = min(start_idx + beat_dur, NUM_SAMPLES)
        t_seg = np.arange(end_idx - start_idx) / SAMPLE_RATE

        # Square-ish wave (bass synth)
        wave = np.sin(2 * np.pi * freq * t_seg)
        wave += np.sin(2 * np.pi * freq * 2 * t_seg) * 0.3
        wave += np.sin(2 * np.pi * freq * 3 * t_seg) * 0.2

        envelope = np.exp(-t_seg * 8.0)
        bass[start_idx:end_idx] = wave * envelope

    # Keys (left channel, bright)
    keys_l = np.zeros(NUM_SAMPLES)
    chord_freqs = [261.63, 329.63, 392.0]  # C major chord (C4, E4, G4)
    for i in range(0, NUM_SAMPLES, int(SAMPLE_RATE * 0.5)):
        end_idx = min(i + int(SAMPLE_RATE * 0.5), NUM_SAMPLES)
        t_seg = np.arange(end_idx - i) / SAMPLE_RATE

        chord_sig = sum(np.sin(2 * np.pi * f * t_seg) for f in chord_freqs) / len(chord_freqs)
        envelope = np.exp(-t_seg * 3.0)
        keys_l[i:end_idx] += chord_sig * envelope * 0.4

    # Pad (right channel, warm)
    pad_r = np.zeros(NUM_SAMPLES)
    pad_freqs = [196.0, 246.94, 293.66]  # G3, B3, D4 (G major)
    for i in range(0, NUM_SAMPLES, int(SAMPLE_RATE * 1.0)):
        end_idx = min(i + int(SAMPLE_RATE * 1.0), NUM_SAMPLES)
        t_seg = np.arange(end_idx - i) / SAMPLE_RATE

        pad_sig = sum(np.sin(2 * np.pi * f * t_seg) * (1.0 + 0.1 * np.sin(2 * np.pi * 3 * t_seg))
                     for f in pad_freqs) / len(pad_freqs)
        envelope = 1.0 - np.exp(-t_seg * 2.0)  # Slow attack
        pad_r[i:end_idx] += pad_sig * envelope * 0.3

    # Mix
    left = bass * 0.7 + keys_l * 0.8
    right = bass * 0.7 + pad_r * 0.8

    # Normalize
    peak = max(np.abs(left).max(), np.abs(right).max())
    left = left / peak * 0.75
    right = right / peak * 0.75

    return left.astype(np.float32), right.astype(np.float32)

def generate_mono_source():
    """Generate mono source for width enhancement testing"""
    t = np.arange(NUM_SAMPLES) / SAMPLE_RATE

    # Rich mono signal: vocal-like formants
    fundamental = 220.0  # A3

    # Vibrato
    vibrato = 1.0 + 0.02 * np.sin(2 * np.pi * 5.5 * t)

    # Harmonics (vocal-like spectrum)
    signal = np.sin(2 * np.pi * fundamental * vibrato * t)
    signal += np.sin(2 * np.pi * fundamental * 2 * vibrato * t) * 0.5
    signal += np.sin(2 * np.pi * fundamental * 3 * vibrato * t) * 0.3
    signal += np.sin(2 * np.pi * fundamental * 4 * vibrato * t) * 0.2
    signal += np.sin(2 * np.pi * fundamental * 5 * vibrato * t) * 0.15

    # Formant-like filtering (simple resonances at 800Hz, 1200Hz, 2400Hz)
    for formant_freq in [800, 1200, 2400]:
        formant = np.sin(2 * np.pi * formant_freq * t)
        signal += formant * 0.1

    # Amplitude modulation (natural dynamics)
    dynamics = 0.7 + 0.3 * np.sin(2 * np.pi * 0.5 * t)
    signal *= dynamics

    # Normalize
    signal = signal / np.abs(signal).max() * 0.7

    return signal.astype(np.float32), signal.astype(np.float32)

def main():
    print("═" * 70)
    print("  SPATIAL ENGINE TEST MATERIALS GENERATOR")
    print("  Engines 46, 53, 56")
    print("═" * 70)
    print()

    print("[1/4] Generating Stereo Drums...")
    left, right = generate_stereo_drums()
    write_stereo_raw("spatial_test_drums_stereo.raw", left, right)
    print(f"      Stereo correlation: {np.corrcoef(left, right)[0,1]:.3f}")
    print()

    print("[2/4] Generating Double-Tracked Guitar...")
    left, right = generate_double_tracked_guitar()
    write_stereo_raw("spatial_test_guitar_double.raw", left, right)
    print(f"      Stereo correlation: {np.corrcoef(left, right)[0,1]:.3f}")
    print()

    print("[3/4] Generating Full Stereo Mix...")
    left, right = generate_stereo_mix()
    write_stereo_raw("spatial_test_full_mix.raw", left, right)
    print(f"      Stereo correlation: {np.corrcoef(left, right)[0,1]:.3f}")
    print()

    print("[4/4] Generating Mono Source...")
    left, right = generate_mono_source()
    write_stereo_raw("spatial_test_mono_source.raw", left, right)
    print(f"      Stereo correlation: {np.corrcoef(left, right)[0,1]:.3f} (expected: 1.000)")
    print()

    print("═" * 70)
    print("✓ All spatial test materials generated successfully!")
    print("═" * 70)
    print()
    print("Materials created:")
    print("  • spatial_test_drums_stereo.raw    - Stereo drum mix (kick center, hihat right)")
    print("  • spatial_test_guitar_double.raw   - Double-tracked guitar (L/R variations)")
    print("  • spatial_test_full_mix.raw        - Full mix (bass center, keys/pad panned)")
    print("  • spatial_test_mono_source.raw     - Mono vocal-like source for width testing")
    print()
    print("Use these with test_spatial_realworld for comprehensive stereo testing!")

if __name__ == "__main__":
    main()
