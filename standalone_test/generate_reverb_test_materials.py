#!/usr/bin/env python3
"""
Generate real-world test materials for reverb testing:
- Snare drum (percussive)
- Vocals (character testing)
- Full mix (realistic usage)
- Impulse (RT60 measurement)
"""

import numpy as np
import struct
import os

SAMPLE_RATE = 48000

def generate_snare_drum(duration=2.0):
    """Generate realistic snare drum hit"""
    samples = int(SAMPLE_RATE * duration)
    audio = np.zeros(samples)

    # Attack transient (white noise burst)
    attack_samples = int(0.005 * SAMPLE_RATE)  # 5ms
    attack = np.random.randn(attack_samples) * np.exp(-np.arange(attack_samples) / (0.002 * SAMPLE_RATE))
    audio[:attack_samples] = attack * 0.8

    # Body (tuned resonance at ~200Hz with harmonics)
    t = np.arange(samples) / SAMPLE_RATE
    fundamental = 200.0

    # Multiple resonances with different decay times
    body = 0.0
    body += 0.6 * np.sin(2 * np.pi * fundamental * t) * np.exp(-t / 0.15)
    body += 0.3 * np.sin(2 * np.pi * fundamental * 2.1 * t) * np.exp(-t / 0.08)
    body += 0.2 * np.sin(2 * np.pi * fundamental * 3.2 * t) * np.exp(-t / 0.05)

    # Snare wires (high frequency rattle)
    snare_noise = np.random.randn(samples) * 0.3
    snare_env = np.exp(-t / 0.12)
    # High-pass filter the noise (simple)
    snare_filtered = np.diff(np.concatenate([[0], snare_noise * snare_env]))

    audio += body + snare_filtered

    # Normalize
    audio = audio / np.max(np.abs(audio)) * 0.8

    return audio

def generate_vocals(duration=4.0):
    """Generate synthesized vocal-like sound"""
    samples = int(SAMPLE_RATE * duration)
    t = np.arange(samples) / SAMPLE_RATE

    # Fundamental frequency varies (simulate melody)
    f0_base = 220.0  # A3
    # Simple melody pattern
    melody = [0, 2, 4, 5, 4, 2, 0, -2]  # scale degrees
    semitones_per_note = 2
    note_duration = duration / len(melody)

    f0 = np.zeros(samples)
    for i, degree in enumerate(melody):
        start_sample = int(i * note_duration * SAMPLE_RATE)
        end_sample = int((i + 1) * note_duration * SAMPLE_RATE)
        f0[start_sample:end_sample] = f0_base * 2 ** (degree * semitones_per_note / 12.0)

    # Generate harmonics (vocal formants)
    vocal = np.zeros(samples)

    # Rich harmonic content (first 8 harmonics)
    harmonics = [1.0, 0.7, 0.5, 0.3, 0.2, 0.15, 0.1, 0.08]
    for h, amp in enumerate(harmonics, 1):
        phase = 2 * np.pi * np.cumsum(f0 * h) / SAMPLE_RATE
        vocal += amp * np.sin(phase)

    # Amplitude envelope (vibrato and dynamics)
    vibrato = 0.02 * np.sin(2 * np.pi * 5.5 * t)  # 5.5 Hz vibrato
    envelope = 0.5 + 0.3 * np.sin(2 * np.pi * 0.5 * t)  # slow amplitude variation
    envelope *= (1 + vibrato)

    # Apply gentle attack/release at note boundaries
    for i in range(len(melody)):
        start_sample = int(i * note_duration * SAMPLE_RATE)
        end_sample = int((i + 1) * note_duration * SAMPLE_RATE)
        attack_samples = int(0.05 * SAMPLE_RATE)  # 50ms attack
        release_samples = int(0.1 * SAMPLE_RATE)  # 100ms release

        if start_sample + attack_samples < end_sample:
            attack_env = np.linspace(0, 1, attack_samples)
            envelope[start_sample:start_sample + attack_samples] *= attack_env

        if end_sample - release_samples > start_sample:
            release_env = np.linspace(1, 0, release_samples)
            envelope[end_sample - release_samples:end_sample] *= release_env

    vocal *= envelope

    # Normalize
    vocal = vocal / np.max(np.abs(vocal)) * 0.7

    return vocal

def generate_full_mix(duration=5.0):
    """Generate a simple full mix with multiple elements"""
    samples = int(SAMPLE_RATE * duration)
    mix = np.zeros(samples)

    # Bass line (continuous)
    t = np.arange(samples) / SAMPLE_RATE
    bass_freq = 110.0  # A2
    bass = 0.0
    bass += 0.6 * np.sin(2 * np.pi * bass_freq * t)
    bass += 0.2 * np.sin(2 * np.pi * bass_freq * 2 * t)
    bass += 0.1 * np.sin(2 * np.pi * bass_freq * 3 * t)

    # Rhythm pattern
    rhythm_env = np.zeros(samples)
    beat_duration = 0.5  # 120 BPM
    beat_samples = int(beat_duration * SAMPLE_RATE)
    for i in range(int(duration / beat_duration)):
        start = i * beat_samples
        attack = int(0.02 * SAMPLE_RATE)
        decay = int(0.3 * SAMPLE_RATE)
        if start + decay < samples:
            rhythm_env[start:start + attack] = np.linspace(0, 1, attack)
            rhythm_env[start + attack:start + decay] = np.exp(-np.arange(decay - attack) / (0.1 * SAMPLE_RATE))

    bass *= rhythm_env
    mix += bass * 0.6

    # Chord pads (sustained)
    chord_freqs = [440.0, 554.37, 659.25]  # A major triad
    for freq in chord_freqs:
        pad = 0.3 * np.sin(2 * np.pi * freq * t)
        pad += 0.15 * np.sin(2 * np.pi * freq * 2.01 * t)  # slight detuning
        # Slow envelope
        pad_env = 0.5 + 0.3 * np.sin(2 * np.pi * 0.3 * t)
        mix += pad * pad_env * 0.3

    # Hi-hat (for high-frequency content)
    hihat = np.random.randn(samples) * 0.1
    hihat_env = np.zeros(samples)
    for i in range(int(duration / (beat_duration / 2))):
        start = int(i * beat_duration / 2 * SAMPLE_RATE)
        decay_len = int(0.05 * SAMPLE_RATE)
        if start + decay_len < samples:
            hihat_env[start:start + decay_len] = np.exp(-np.arange(decay_len) / (0.015 * SAMPLE_RATE))

    # High-pass the hihat
    hihat_filtered = np.diff(np.concatenate([[0], hihat * hihat_env]))
    mix += hihat_filtered * 0.4

    # Normalize
    mix = mix / np.max(np.abs(mix)) * 0.8

    return mix

def generate_impulse(duration=0.1):
    """Generate perfect impulse for RT60 measurement"""
    samples = int(SAMPLE_RATE * duration)
    impulse = np.zeros(samples)
    impulse[0] = 1.0

    return impulse

def write_wav_file(filename, audio):
    """Write mono WAV file"""
    # Ensure audio is in int16 range
    audio_int16 = np.clip(audio * 32767, -32768, 32767).astype(np.int16)

    with open(filename, 'wb') as f:
        # RIFF header
        f.write(b'RIFF')
        f.write(struct.pack('<I', 36 + len(audio_int16) * 2))  # File size - 8
        f.write(b'WAVE')

        # fmt chunk
        f.write(b'fmt ')
        f.write(struct.pack('<I', 16))  # Chunk size
        f.write(struct.pack('<H', 1))   # Audio format (PCM)
        f.write(struct.pack('<H', 1))   # Number of channels
        f.write(struct.pack('<I', SAMPLE_RATE))  # Sample rate
        f.write(struct.pack('<I', SAMPLE_RATE * 2))  # Byte rate
        f.write(struct.pack('<H', 2))   # Block align
        f.write(struct.pack('<H', 16))  # Bits per sample

        # data chunk
        f.write(b'data')
        f.write(struct.pack('<I', len(audio_int16) * 2))  # Data size
        f.write(audio_int16.tobytes())

def main():
    print("Generating reverb test materials...")

    # Create test_materials directory if it doesn't exist
    os.makedirs('test_materials', exist_ok=True)

    # Generate materials
    print("  - Snare drum (percussive)")
    snare = generate_snare_drum(duration=2.0)
    write_wav_file('test_materials/snare_drum.wav', snare)

    print("  - Vocals (character testing)")
    vocals = generate_vocals(duration=4.0)
    write_wav_file('test_materials/vocals.wav', vocals)

    print("  - Full mix (realistic usage)")
    mix = generate_full_mix(duration=5.0)
    write_wav_file('test_materials/full_mix.wav', mix)

    print("  - Impulse (RT60 measurement)")
    impulse = generate_impulse(duration=0.1)
    write_wav_file('test_materials/impulse.wav', impulse)

    print("\nTest materials generated successfully!")
    print("Files created in test_materials/:")
    print("  - snare_drum.wav (2.0s)")
    print("  - vocals.wav (4.0s)")
    print("  - full_mix.wav (5.0s)")
    print("  - impulse.wav (0.1s)")

if __name__ == '__main__':
    main()
