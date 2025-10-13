#!/usr/bin/env python3
"""
Spectral Effects Test Material Generator
Generates specialized audio materials for testing spectral/FFT engines
"""

import numpy as np
import struct
import os

SAMPLE_RATE = 48000
DURATION = 2.0  # seconds

def save_raw_stereo(filename, audio_left, audio_right):
    """Save stereo audio as raw 32-bit float"""
    # Ensure same length
    length = min(len(audio_left), len(audio_right))
    audio_left = audio_left[:length]
    audio_right = audio_right[:length]

    # Interleave stereo
    stereo = np.zeros(length * 2, dtype=np.float32)
    stereo[0::2] = audio_left
    stereo[1::2] = audio_right

    with open(filename, 'wb') as f:
        f.write(stereo.tobytes())

    print(f"Saved {filename}: {length} samples ({length/SAMPLE_RATE:.2f}s), {os.path.getsize(filename)} bytes")

def generate_sustained_pad():
    """Generate sustained pad for SpectralFreeze testing"""
    print("\n1. Generating sustained pad (for SpectralFreeze)...")

    samples = int(DURATION * SAMPLE_RATE)
    t = np.arange(samples) / SAMPLE_RATE

    # Rich harmonic pad with slow evolving timbre
    audio = np.zeros(samples, dtype=np.float32)

    # Base frequency: 220Hz (A3)
    base_freq = 220.0

    # Add harmonics with evolving amplitudes
    harmonics = [1, 2, 3, 4, 5, 6, 7, 8]
    amplitudes = [1.0, 0.5, 0.33, 0.25, 0.2, 0.16, 0.14, 0.12]

    for h, amp in zip(harmonics, amplitudes):
        freq = base_freq * h
        # Add slow phase modulation for shimmer
        phase_mod = 0.05 * np.sin(2 * np.pi * 0.2 * t)
        audio += amp * np.sin(2 * np.pi * freq * t + phase_mod)

    # Add slow amplitude envelope
    env = 0.5 + 0.5 * np.sin(2 * np.pi * 0.3 * t)
    audio *= env

    # Add subtle detuned layer
    detune = 1.002
    for h, amp in zip(harmonics[:5], amplitudes[:5]):
        freq = base_freq * h * detune
        audio += amp * 0.3 * np.sin(2 * np.pi * freq * t)

    # Normalize
    audio = audio / np.max(np.abs(audio)) * 0.7

    # Create stereo with slight width
    left = audio
    right = audio * 0.95 + np.roll(audio, 100) * 0.05

    save_raw_stereo('spectral_test_sustained_pad.raw', left, right)
    return left, right

def generate_vocal_like():
    """Generate vocal-like sound for Robotizer/PhasedVocoder testing"""
    print("\n2. Generating vocal-like sound (for PhasedVocoder/Robotizer)...")

    samples = int(DURATION * SAMPLE_RATE)
    t = np.arange(samples) / SAMPLE_RATE

    audio = np.zeros(samples, dtype=np.float32)

    # Simulate vowel formants (like "ah" sound)
    # Fundamental frequency sweeps from 150Hz to 200Hz
    f0_start = 150.0
    f0_end = 200.0
    f0 = f0_start + (f0_end - f0_start) * (t / DURATION)

    # Add vibrato
    vibrato = 5.0 * np.sin(2 * np.pi * 5.5 * t)
    f0 += vibrato

    # Generate phase-continuous fundamental
    phase = np.cumsum(2 * np.pi * f0 / SAMPLE_RATE)

    # Add harmonics with formant-like filtering
    formant_peaks = [700, 1220, 2600]  # Approximate "ah" formants
    formant_widths = [100, 150, 200]

    for harmonic in range(1, 21):
        harmonic_freq = harmonic * f0

        # Calculate formant amplitude for this harmonic
        formant_amp = 0.0
        for peak, width in zip(formant_peaks, formant_widths):
            # Gaussian-like formant shape
            distance = np.abs(harmonic * (f0_start + f0_end) / 2 - peak)
            formant_amp += np.exp(-(distance ** 2) / (2 * width ** 2))

        # Add harmonic with formant shaping
        amp = formant_amp / (harmonic ** 0.7)
        audio += amp * np.sin(harmonic * phase)

    # Add breathiness (noise)
    breath = np.random.randn(samples) * 0.05
    # Filter breath to formant regions
    from scipy import signal
    for peak in formant_peaks:
        b, a = signal.butter(2, [peak - 200, peak + 200], btype='band', fs=SAMPLE_RATE)
        breath_filtered = signal.lfilter(b, a, breath)
        audio += breath_filtered * 0.1

    # Amplitude envelope (phrase shape)
    env = np.ones(samples)
    attack_samples = int(0.1 * SAMPLE_RATE)
    release_samples = int(0.2 * SAMPLE_RATE)
    env[:attack_samples] = np.linspace(0, 1, attack_samples)
    env[-release_samples:] = np.linspace(1, 0, release_samples)
    audio *= env

    # Normalize
    audio = audio / np.max(np.abs(audio)) * 0.6

    # Mono for clarity
    save_raw_stereo('spectral_test_vocal_like.raw', audio, audio)
    return audio, audio

def generate_noisy_signal():
    """Generate noisy signal for SpectralGate testing"""
    print("\n3. Generating noisy signal (for SpectralGate_Platinum)...")

    samples = int(DURATION * SAMPLE_RATE)
    t = np.arange(samples) / SAMPLE_RATE

    # Musical signal: simple melody
    melody_freqs = [440, 493.88, 523.25, 587.33, 523.25, 493.88, 440, 0]  # A-B-C-D-C-B-A-rest
    note_duration = DURATION / len(melody_freqs)

    signal = np.zeros(samples)
    for i, freq in enumerate(melody_freqs):
        start_sample = int(i * note_duration * SAMPLE_RATE)
        end_sample = int((i + 1) * note_duration * SAMPLE_RATE)

        if freq > 0:  # Not a rest
            note_t = np.arange(end_sample - start_sample) / SAMPLE_RATE
            # Simple tone with harmonics
            note = np.sin(2 * np.pi * freq * note_t)
            note += 0.3 * np.sin(2 * np.pi * freq * 2 * note_t)
            note += 0.2 * np.sin(2 * np.pi * freq * 3 * note_t)

            # Envelope
            note_env = np.exp(-3 * note_t)
            note *= note_env

            signal[start_sample:end_sample] = note

    # Normalize signal
    signal = signal / np.max(np.abs(signal)) * 0.5

    # Add various types of noise
    # 1. White noise (broadband)
    white_noise = np.random.randn(samples) * 0.15

    # 2. Pink noise (1/f characteristic)
    white = np.random.randn(samples)
    # Simple pink filter using cumsum approximation
    pink_noise = np.cumsum(white)
    pink_noise = pink_noise / np.max(np.abs(pink_noise)) * 0.1

    # 3. Burst noise (intermittent)
    burst_noise = np.zeros(samples)
    num_bursts = 10
    for _ in range(num_bursts):
        burst_start = np.random.randint(0, samples - 4800)
        burst_len = np.random.randint(960, 4800)
        burst_noise[burst_start:burst_start + burst_len] = np.random.randn(burst_len) * 0.2

    # Combine signal + noise
    audio = signal + white_noise + pink_noise + burst_noise

    # Normalize
    audio = audio / np.max(np.abs(audio)) * 0.7
    audio = audio.astype(np.float32)

    # Stereo with different noise on each channel
    left = audio
    right = signal / np.max(np.abs(signal)) * 0.5 + np.random.randn(samples) * 0.15
    right = right / np.max(np.abs(right)) * 0.7
    right = right.astype(np.float32)

    save_raw_stereo('spectral_test_noisy_signal.raw', left, right)
    return left, right

def generate_feedback_rich():
    """Generate feedback-rich signal for FeedbackNetwork testing"""
    print("\n4. Generating feedback-rich signal (for FeedbackNetwork)...")

    samples = int(DURATION * SAMPLE_RATE)
    t = np.arange(samples) / SAMPLE_RATE

    # Start with impulse-like attacks
    audio = np.zeros(samples)

    # Add series of impulse attacks at different pitches
    attack_times = [0.0, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75]
    pitches = [196, 220, 246.94, 261.63, 293.66, 329.63, 349.23, 392]  # G3-G4 scale

    for attack_time, pitch in zip(attack_times, pitches):
        start_sample = int(attack_time * SAMPLE_RATE)

        # Create ringing tone (like struck resonator)
        ring_duration = 0.5
        ring_samples = int(ring_duration * SAMPLE_RATE)
        if start_sample + ring_samples > samples:
            ring_samples = samples - start_sample

        ring_t = np.arange(ring_samples) / SAMPLE_RATE

        # Multiple resonances
        ring = np.sin(2 * np.pi * pitch * ring_t)
        ring += 0.5 * np.sin(2 * np.pi * pitch * 2.01 * ring_t)  # Slightly detuned
        ring += 0.3 * np.sin(2 * np.pi * pitch * 3.02 * ring_t)

        # Decay envelope
        decay_env = np.exp(-3 * ring_t)
        ring *= decay_env

        # Sharp attack
        attack_samples = int(0.001 * SAMPLE_RATE)
        attack_env = np.ones(ring_samples)
        attack_env[:attack_samples] = np.linspace(0, 1, attack_samples)
        ring *= attack_env

        audio[start_sample:start_sample + ring_samples] += ring

    # Add some inharmonic components (like metallic resonance)
    for i in range(5):
        freq = 100 + i * 177  # Inharmonic series
        phase_offset = np.random.rand() * 2 * np.pi
        metallic = 0.1 * np.sin(2 * np.pi * freq * t + phase_offset)
        metallic *= np.exp(-2 * t)
        audio += metallic

    # Normalize
    audio = audio / np.max(np.abs(audio)) * 0.7
    audio = audio.astype(np.float32)

    # Create stereo with slight delay/phase
    left = audio
    right = np.roll(audio, 24)  # 0.5ms delay

    save_raw_stereo('spectral_test_feedback_rich.raw', left, right)
    return left, right

def generate_impulse_sweep():
    """Generate impulse sweep for FFT analysis"""
    print("\n5. Generating impulse sweep (for FFT artifact analysis)...")

    samples = int(DURATION * SAMPLE_RATE)
    audio = np.zeros(samples, dtype=np.float32)

    # Add impulses at regular intervals
    impulse_interval = 0.25  # 4 impulses per second
    impulse_samples = int(impulse_interval * SAMPLE_RATE)

    for i in range(int(DURATION / impulse_interval)):
        pos = i * impulse_samples
        if pos < samples - 10:
            # Create impulse (click)
            audio[pos] = 1.0
            # Add decay tail for visual analysis
            tail_len = 480  # 10ms
            if pos + tail_len < samples:
                tail = np.exp(-np.linspace(0, 5, tail_len))
                audio[pos:pos + tail_len] = tail

    # Normalize
    audio = audio * 0.8

    save_raw_stereo('spectral_test_impulse_sweep.raw', audio, audio)
    return audio, audio

def generate_frequency_sweep():
    """Generate sine sweep for frequency response testing"""
    print("\n6. Generating frequency sweep (for FFT frequency resolution)...")

    samples = int(DURATION * SAMPLE_RATE)
    t = np.arange(samples) / SAMPLE_RATE

    # Logarithmic frequency sweep from 50Hz to 10kHz
    f_start = 50.0
    f_end = 10000.0

    # Calculate instantaneous frequency
    freq = f_start * (f_end / f_start) ** (t / DURATION)

    # Generate phase-continuous sweep
    phase = np.cumsum(2 * np.pi * freq / SAMPLE_RATE)
    audio = np.sin(phase).astype(np.float32)

    # Apply gentle envelope
    env = np.ones(samples)
    fade_samples = int(0.05 * SAMPLE_RATE)
    env[:fade_samples] = np.linspace(0, 1, fade_samples)
    env[-fade_samples:] = np.linspace(1, 0, fade_samples)
    audio *= env * 0.7

    save_raw_stereo('spectral_test_frequency_sweep.raw', audio, audio)
    return audio, audio

def main():
    print("=" * 70)
    print("SPECTRAL EFFECTS TEST MATERIAL GENERATOR")
    print("=" * 70)
    print(f"Sample Rate: {SAMPLE_RATE} Hz")
    print(f"Duration: {DURATION} seconds")
    print(f"Format: 32-bit float stereo (raw)")
    print()

    # Generate all materials
    materials = [
        generate_sustained_pad(),
        generate_vocal_like(),
        generate_noisy_signal(),
        generate_feedback_rich(),
        generate_impulse_sweep(),
        generate_frequency_sweep()
    ]

    print("\n" + "=" * 70)
    print("TEST MATERIAL GENERATION COMPLETE")
    print("=" * 70)
    print("\nGenerated files:")
    print("  1. spectral_test_sustained_pad.raw      - For SpectralFreeze (47)")
    print("  2. spectral_test_vocal_like.raw         - For PhasedVocoder (49)")
    print("  3. spectral_test_noisy_signal.raw       - For SpectralGate_Platinum (48)")
    print("  4. spectral_test_feedback_rich.raw      - For FeedbackNetwork (52)")
    print("  5. spectral_test_impulse_sweep.raw      - For FFT artifact analysis")
    print("  6. spectral_test_frequency_sweep.raw    - For frequency resolution")
    print()

if __name__ == '__main__':
    try:
        from scipy import signal
    except ImportError:
        print("Warning: scipy not available, some features will be limited")
        # Provide dummy signal module
        class DummySignal:
            @staticmethod
            def butter(*args, **kwargs):
                return [1], [1]
            @staticmethod
            def lfilter(b, a, x):
                return x * 0.1
        signal = DummySignal()

    main()
