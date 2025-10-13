#!/usr/bin/env python3
"""
Generate real-world musical test materials for distortion engine testing
Creates: guitar DI, bass, drums, synth - all optimized for distortion/saturation analysis
"""

import numpy as np
import struct
import sys

SAMPLE_RATE = 48000
DURATION = 2.0  # 2 seconds each
NUM_SAMPLES = int(SAMPLE_RATE * DURATION)

def write_raw_stereo(filename, left, right):
    """Write stereo float32 raw audio (interleaved)"""
    with open(filename, 'wb') as f:
        for l, r in zip(left, right):
            f.write(struct.pack('f', l))
            f.write(struct.pack('f', r))
    print(f"Created: {filename}")

def generate_guitar_di():
    """
    Clean guitar DI with realistic picking dynamics
    - Fundamental around 82-330 Hz (E2-E4 range)
    - Rich harmonic content
    - Attack transients
    - Dynamic variation
    """
    t = np.linspace(0, DURATION, NUM_SAMPLES)
    signal = np.zeros(NUM_SAMPLES)

    # Guitar chord progression: E, A, D, G (power chords)
    note_duration = DURATION / 4
    notes = [82.41, 110.0, 146.83, 196.0]  # E2, A2, D3, G3

    for i, freq in enumerate(notes):
        start_idx = int(i * note_duration * SAMPLE_RATE)
        end_idx = int((i + 1) * note_duration * SAMPLE_RATE)
        note_t = t[start_idx:end_idx]

        # Fundamental + harmonics (like a real guitar)
        note = 0.4 * np.sin(2 * np.pi * freq * note_t)                    # Fundamental
        note += 0.25 * np.sin(2 * np.pi * freq * 2 * note_t)              # 2nd harmonic
        note += 0.15 * np.sin(2 * np.pi * freq * 3 * note_t)              # 3rd harmonic
        note += 0.08 * np.sin(2 * np.pi * freq * 4 * note_t)              # 4th harmonic
        note += 0.04 * np.sin(2 * np.pi * freq * 5 * note_t + 0.3)        # 5th harmonic (phase varied)
        note += 0.02 * np.sin(2 * np.pi * freq * 7 * note_t + 0.7)        # 7th harmonic

        # Attack envelope (fast attack, medium decay, sustain)
        attack_samples = int(0.005 * SAMPLE_RATE)  # 5ms attack
        decay_samples = int(0.05 * SAMPLE_RATE)     # 50ms decay
        sustain_level = 0.7

        envelope = np.ones(len(note_t))
        if len(note_t) > attack_samples:
            envelope[:attack_samples] = np.linspace(0, 1, attack_samples)
        if len(note_t) > attack_samples + decay_samples:
            envelope[attack_samples:attack_samples+decay_samples] = np.linspace(1, sustain_level, decay_samples)
            envelope[attack_samples+decay_samples:] = sustain_level

        # Natural release
        release_samples = int(0.1 * SAMPLE_RATE)
        if len(note_t) > release_samples:
            envelope[-release_samples:] *= np.linspace(1, 0, release_samples)

        # Add pick attack transient
        pick_attack = np.zeros(len(note_t))
        pick_attack[:50] = np.random.randn(50) * 0.15 * np.linspace(1, 0, 50)

        signal[start_idx:end_idx] = (note * envelope) + pick_attack

    # Normalize to -6dB peak to leave headroom for distortion
    peak = np.max(np.abs(signal))
    if peak > 0:
        signal = signal * (0.5 / peak)

    return signal, signal

def generate_bass_guitar():
    """
    Bass guitar DI with strong low-end fundamentals
    - Deep fundamentals 41-100 Hz
    - Less harmonic content than guitar
    - Punchy transients
    """
    t = np.linspace(0, DURATION, NUM_SAMPLES)
    signal = np.zeros(NUM_SAMPLES)

    # Bass line: E1, A1, D2, G1
    note_duration = DURATION / 4
    notes = [41.20, 55.0, 73.42, 49.0]  # Bass frequencies

    for i, freq in enumerate(notes):
        start_idx = int(i * note_duration * SAMPLE_RATE)
        end_idx = int((i + 1) * note_duration * SAMPLE_RATE)
        note_t = t[start_idx:end_idx]

        # Bass has strong fundamental, fewer harmonics
        note = 0.6 * np.sin(2 * np.pi * freq * note_t)
        note += 0.2 * np.sin(2 * np.pi * freq * 2 * note_t)
        note += 0.1 * np.sin(2 * np.pi * freq * 3 * note_t)
        note += 0.05 * np.sin(2 * np.pi * freq * 4 * note_t + 0.5)

        # Pluck envelope
        attack_samples = int(0.003 * SAMPLE_RATE)  # 3ms attack
        decay_samples = int(0.08 * SAMPLE_RATE)
        sustain_level = 0.6

        envelope = np.ones(len(note_t))
        if len(note_t) > attack_samples:
            envelope[:attack_samples] = np.linspace(0, 1, attack_samples)
        if len(note_t) > attack_samples + decay_samples:
            envelope[attack_samples:attack_samples+decay_samples] = np.linspace(1, sustain_level, decay_samples)
            envelope[attack_samples+decay_samples:] = sustain_level

        # Natural release
        release_samples = int(0.15 * SAMPLE_RATE)
        if len(note_t) > release_samples:
            envelope[-release_samples:] *= np.linspace(1, 0, release_samples)

        # Pluck transient
        pluck = np.zeros(len(note_t))
        pluck[:30] = np.random.randn(30) * 0.2 * np.linspace(1, 0, 30)

        signal[start_idx:end_idx] = (note * envelope) + pluck

    # Normalize
    peak = np.max(np.abs(signal))
    if peak > 0:
        signal = signal * (0.5 / peak)

    return signal, signal

def generate_drums():
    """
    Drum hits for saturation testing
    - Kick, snare, hi-hat pattern
    - Transient-rich
    - Wide frequency spectrum
    """
    signal = np.zeros(NUM_SAMPLES)
    t = np.linspace(0, DURATION, NUM_SAMPLES)

    # 16th note grid at 120 BPM
    bpm = 120
    sixteenth_duration = 60.0 / bpm / 4
    sixteenth_samples = int(sixteenth_duration * SAMPLE_RATE)

    # Simple pattern: kick on 1 and 3, snare on 2 and 4, hats on every 8th
    for beat in range(int(DURATION * bpm / 60 * 4)):  # 16th notes
        idx = beat * sixteenth_samples
        if idx >= NUM_SAMPLES - 2000:
            break

        # Kick on quarter notes (0, 4, 8, 12...)
        if beat % 4 == 0:
            # Kick: 50-100 Hz body, punchy transient
            kick_t = np.linspace(0, 0.15, int(0.15 * SAMPLE_RATE))
            kick = 0.7 * np.sin(2 * np.pi * 60 * kick_t) * np.exp(-kick_t * 12)
            kick += 0.3 * np.sin(2 * np.pi * 100 * kick_t) * np.exp(-kick_t * 15)
            # Click transient
            kick[:200] += np.random.randn(200) * 0.3 * np.linspace(1, 0, 200)
            end_idx = min(idx + len(kick), NUM_SAMPLES)
            signal[idx:end_idx] += kick[:end_idx-idx]

        # Snare on 2 and 4 (beats 2 and 6, 10, 14...)
        if beat % 8 == 4:
            # Snare: 200 Hz body + noise
            snare_t = np.linspace(0, 0.1, int(0.1 * SAMPLE_RATE))
            snare = 0.4 * np.sin(2 * np.pi * 200 * snare_t) * np.exp(-snare_t * 20)
            snare += 0.4 * np.random.randn(len(snare_t)) * np.exp(-snare_t * 15)
            end_idx = min(idx + len(snare), NUM_SAMPLES)
            signal[idx:end_idx] += snare[:end_idx-idx]

        # Hi-hat on every 8th note
        if beat % 2 == 0:
            # Hi-hat: filtered noise
            hihat_t = np.linspace(0, 0.05, int(0.05 * SAMPLE_RATE))
            hihat = np.random.randn(len(hihat_t)) * 0.15 * np.exp(-hihat_t * 40)
            # High-pass filter (simple)
            hihat = np.diff(np.concatenate([[0], hihat])) * 2
            end_idx = min(idx + len(hihat), NUM_SAMPLES)
            signal[idx:end_idx] += hihat[:end_idx-idx]

    # Normalize
    peak = np.max(np.abs(signal))
    if peak > 0:
        signal = signal * (0.5 / peak)

    return signal, signal

def generate_synth():
    """
    Synth pad/lead for bitcrusher and saturation
    - Clean waveforms (saw, square)
    - Bright harmonics
    - Good for testing aliasing
    """
    t = np.linspace(0, DURATION, NUM_SAMPLES)
    signal = np.zeros(NUM_SAMPLES)

    # Synth lead melody: C4, E4, G4, C5
    note_duration = DURATION / 4
    notes = [261.63, 329.63, 392.0, 523.25]

    for i, freq in enumerate(notes):
        start_idx = int(i * note_duration * SAMPLE_RATE)
        end_idx = int((i + 1) * note_duration * SAMPLE_RATE)
        note_t = t[start_idx:end_idx]

        # Sawtooth wave (band-limited approximation with 20 harmonics)
        note = np.zeros(len(note_t))
        for h in range(1, 21):
            if freq * h < SAMPLE_RATE / 2.5:  # Band-limit
                note += ((-1)**h / h) * np.sin(2 * np.pi * freq * h * note_t)
        note *= 0.5

        # ADSR envelope
        attack_samples = int(0.02 * SAMPLE_RATE)
        decay_samples = int(0.05 * SAMPLE_RATE)
        sustain_level = 0.7
        release_samples = int(0.08 * SAMPLE_RATE)

        envelope = np.ones(len(note_t))
        if len(note_t) > attack_samples:
            envelope[:attack_samples] = np.linspace(0, 1, attack_samples)
        if len(note_t) > attack_samples + decay_samples:
            envelope[attack_samples:attack_samples+decay_samples] = np.linspace(1, sustain_level, decay_samples)
            envelope[attack_samples+decay_samples:] = sustain_level
        if len(note_t) > release_samples:
            envelope[-release_samples:] *= np.linspace(1, 0, release_samples)

        # Add subtle vibrato
        vibrato_rate = 5.0  # Hz
        vibrato_depth = 0.003  # Small pitch variation
        vibrato = 1 + vibrato_depth * np.sin(2 * np.pi * vibrato_rate * note_t)

        signal[start_idx:end_idx] = note * envelope * vibrato

    # Normalize
    peak = np.max(np.abs(signal))
    if peak > 0:
        signal = signal * (0.5 / peak)

    return signal, signal

def main():
    print("Generating real-world test materials for distortion engines...")
    print(f"Sample rate: {SAMPLE_RATE} Hz")
    print(f"Duration: {DURATION} seconds")
    print()

    # Generate all materials
    guitar_l, guitar_r = generate_guitar_di()
    write_raw_stereo("distortion_test_guitar_di.raw", guitar_l, guitar_r)

    bass_l, bass_r = generate_bass_guitar()
    write_raw_stereo("distortion_test_bass.raw", bass_l, bass_r)

    drums_l, drums_r = generate_drums()
    write_raw_stereo("distortion_test_drums.raw", drums_l, drums_r)

    synth_l, synth_r = generate_synth()
    write_raw_stereo("distortion_test_synth.raw", synth_l, synth_r)

    print()
    print("All test materials created successfully!")
    print("Ready for distortion engine testing.")

if __name__ == "__main__":
    main()
