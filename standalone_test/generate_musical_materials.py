#!/usr/bin/env python3
"""
Real-World Musical Material Generator
Generates realistic audio test materials for subjective engine testing
"""

import numpy as np
import wave
import struct
import os
from dataclasses import dataclass
from typing import List, Tuple

@dataclass
class AudioMaterial:
    """Container for generated audio material"""
    name: str
    description: str
    data: np.ndarray  # Stereo audio [samples, 2]
    sample_rate: int
    purpose: str
    test_targets: List[str]

class MusicalMaterialGenerator:
    """Generates realistic musical test materials"""

    def __init__(self, sample_rate: int = 48000):
        self.sample_rate = sample_rate
        self.materials = []

    def generate_all_materials(self) -> List[AudioMaterial]:
        """Generate all test materials"""
        print("Generating realistic musical test materials...\n")

        self.materials = [
            self.generate_drum_loop(),
            self.generate_bass_line(),
            self.generate_vocal_sample(),
            self.generate_guitar_chord(),
            self.generate_piano_notes(),
            self.generate_white_noise_burst(),
            self.generate_pink_noise_sustained()
        ]

        return self.materials

    def generate_drum_loop(self) -> AudioMaterial:
        """Generate 120 BPM drum loop with kick, snare, hi-hats"""
        print("Generating drum loop (120 BPM, 4 bars)...")

        bpm = 120
        bars = 4
        beats_per_bar = 4
        duration = (60.0 / bpm) * beats_per_bar * bars
        samples = int(duration * self.sample_rate)

        audio = np.zeros((samples, 2))
        beat_duration = 60.0 / bpm

        # Helper to create kick drum
        def create_kick(start_sample):
            duration_samples = int(0.15 * self.sample_rate)
            t = np.arange(duration_samples) / self.sample_rate

            # Pitch sweep from 100Hz to 40Hz
            freq_start = 100
            freq_end = 40
            freq = freq_start * np.exp(np.log(freq_end/freq_start) * t / t[-1])

            # Generate tone with pitch sweep
            phase = 2 * np.pi * np.cumsum(freq) / self.sample_rate
            kick = np.sin(phase)

            # Envelope
            envelope = np.exp(-5 * t)
            kick *= envelope

            # Add click attack
            click = np.random.randn(int(0.005 * self.sample_rate)) * 0.3
            click *= np.linspace(1, 0, len(click))

            # Combine
            kick[:len(click)] += click
            kick = kick * 0.8

            # Write to both channels
            end_sample = min(start_sample + duration_samples, samples)
            actual_samples = end_sample - start_sample
            audio[start_sample:end_sample, 0] += kick[:actual_samples]
            audio[start_sample:end_sample, 1] += kick[:actual_samples]

        # Helper to create snare drum
        def create_snare(start_sample):
            duration_samples = int(0.12 * self.sample_rate)
            t = np.arange(duration_samples) / self.sample_rate

            # Tone component (180Hz fundamental)
            tone = np.sin(2 * np.pi * 180 * t)
            tone += 0.5 * np.sin(2 * np.pi * 360 * t)

            # Noise component (snare rattle)
            noise = np.random.randn(duration_samples) * 0.7

            # Mix and envelope
            snare = tone * 0.3 + noise * 0.7
            envelope = np.exp(-8 * t)
            snare *= envelope * 0.6

            # Write to both channels
            end_sample = min(start_sample + duration_samples, samples)
            actual_samples = end_sample - start_sample
            audio[start_sample:end_sample, 0] += snare[:actual_samples]
            audio[start_sample:end_sample, 1] += snare[:actual_samples]

        # Helper to create hi-hat
        def create_hihat(start_sample, open=False):
            duration_samples = int((0.15 if open else 0.05) * self.sample_rate)
            t = np.arange(duration_samples) / self.sample_rate

            # High-frequency noise burst
            hihat = np.random.randn(duration_samples)

            # High-pass filter simulation (emphasize high frequencies)
            for i in range(1, len(hihat)):
                hihat[i] = 0.9 * hihat[i] + 0.1 * hihat[i-1]

            # Envelope
            if open:
                envelope = np.exp(-3 * t)
            else:
                envelope = np.exp(-15 * t)

            hihat *= envelope * 0.3

            # Stereo: slightly different for each channel
            hihat_l = hihat + np.random.randn(duration_samples) * 0.05
            hihat_r = hihat + np.random.randn(duration_samples) * 0.05

            # Write to channels
            end_sample = min(start_sample + duration_samples, samples)
            actual_samples = end_sample - start_sample
            audio[start_sample:end_sample, 0] += hihat_l[:actual_samples]
            audio[start_sample:end_sample, 1] += hihat_r[:actual_samples]

        # Create pattern (16th note grid)
        sixteenth_note = beat_duration / 4

        for bar in range(bars):
            for beat in range(beats_per_bar):
                beat_sample = int((bar * beats_per_bar + beat) * beat_duration * self.sample_rate)

                # Kick on 1 and 3
                if beat in [0, 2]:
                    create_kick(beat_sample)

                # Snare on 2 and 4
                if beat in [1, 3]:
                    create_snare(beat_sample)

                # Hi-hats on every 8th note
                for eighth in range(2):
                    hihat_sample = beat_sample + int(eighth * 2 * sixteenth_note * self.sample_rate)
                    open_hihat = (beat == 3 and eighth == 1)  # Open hi-hat occasionally
                    create_hihat(hihat_sample, open=open_hihat)

        # Normalize
        max_val = np.max(np.abs(audio))
        if max_val > 0:
            audio = audio * 0.8 / max_val

        return AudioMaterial(
            name="drum_loop_120bpm",
            description="120 BPM drum loop with kick, snare, hi-hats (4 bars)",
            data=audio,
            sample_rate=self.sample_rate,
            purpose="Test dynamics, transients, compression, and multiband processing",
            test_targets=["Dynamics", "Compressors", "Transient Shapers", "EQ", "Distortion"]
        )

    def generate_bass_line(self) -> AudioMaterial:
        """Generate bass line in E1-E2 range (40-80Hz)"""
        print("Generating bass line (E1-E2 range)...")

        duration = 4.0  # seconds
        samples = int(duration * self.sample_rate)
        audio = np.zeros((samples, 2))

        # Bass notes: E1 (41.2Hz), G1 (49Hz), A1 (55Hz), B1 (61.7Hz), E2 (82.4Hz)
        notes = [41.2, 49.0, 55.0, 61.7, 82.4, 61.7, 49.0, 41.2]
        note_duration = duration / len(notes)

        for i, freq in enumerate(notes):
            start_sample = int(i * note_duration * self.sample_rate)
            note_samples = int(note_duration * self.sample_rate)
            t = np.arange(note_samples) / self.sample_rate

            # Generate bass note with harmonics
            fundamental = np.sin(2 * np.pi * freq * t)
            harmonic2 = 0.5 * np.sin(2 * np.pi * freq * 2 * t)
            harmonic3 = 0.3 * np.sin(2 * np.pi * freq * 3 * t)
            harmonic4 = 0.15 * np.sin(2 * np.pi * freq * 4 * t)

            bass_note = fundamental + harmonic2 + harmonic3 + harmonic4

            # Envelope (pluck-style)
            attack = int(0.01 * self.sample_rate)
            decay = int(0.1 * self.sample_rate)
            sustain_level = 0.7

            envelope = np.ones(note_samples)
            envelope[:attack] = np.linspace(0, 1, attack)
            envelope[attack:attack+decay] = np.linspace(1, sustain_level, decay)
            envelope[attack+decay:] = sustain_level

            # Release at end
            release = int(0.05 * self.sample_rate)
            envelope[-release:] *= np.linspace(1, 0, release)

            bass_note *= envelope * 0.7

            # Write to both channels (mono bass)
            end_sample = min(start_sample + note_samples, samples)
            actual_samples = end_sample - start_sample
            audio[start_sample:end_sample, 0] += bass_note[:actual_samples]
            audio[start_sample:end_sample, 1] += bass_note[:actual_samples]

        return AudioMaterial(
            name="bass_line_e1_e2",
            description="Bass line with fundamental 40-80Hz",
            data=audio,
            sample_rate=self.sample_rate,
            purpose="Test sub-frequency handling, filters, EQ, distortion",
            test_targets=["Filters", "EQ", "Distortion", "Subharmonics", "Phase"]
        )

    def generate_vocal_sample(self) -> AudioMaterial:
        """Generate vocal-like sample with formants"""
        print("Generating vocal sample...")

        duration = 3.0
        samples = int(duration * self.sample_rate)

        # Generate vocal phrase: "Ahhh" -> "Eeee" -> "Ohhh"
        audio = np.zeros((samples, 2))

        # Fundamental frequency sweep (male vocal range: 100-150Hz)
        t = np.arange(samples) / self.sample_rate

        # Vibrato
        vibrato_rate = 5.5  # Hz
        vibrato_depth = 0.02  # 2% depth
        vibrato = 1.0 + vibrato_depth * np.sin(2 * np.pi * vibrato_rate * t)

        # Fundamental frequency pattern
        f0 = 120  # Base frequency
        f0_contour = f0 * (1.0 + 0.15 * np.sin(2 * np.pi * 0.5 * t)) * vibrato

        # Generate fundamental
        phase = 2 * np.pi * np.cumsum(f0_contour) / self.sample_rate
        vocal = np.sin(phase)

        # Add harmonics with formant shaping
        for harmonic in range(2, 8):
            harmonic_amp = 1.0 / (harmonic ** 1.5)
            vocal += harmonic_amp * np.sin(harmonic * phase)

        # Formant filtering (simulate vowel sounds)
        # Create time-varying formants
        formant1_freq = 700 + 300 * np.sin(2 * np.pi * 0.3 * t)  # 700-1000 Hz
        formant2_freq = 1200 + 400 * np.sin(2 * np.pi * 0.25 * t)  # 1200-1600 Hz

        # Envelope (breath-like)
        attack = int(0.1 * self.sample_rate)
        release = int(0.3 * self.sample_rate)

        envelope = np.ones(samples)
        envelope[:attack] = np.linspace(0, 1, attack)
        envelope[-release:] *= np.linspace(1, 0, release)

        # Add breathiness (noise)
        breathiness = np.random.randn(samples) * 0.05
        breathiness *= envelope

        vocal = vocal * envelope * 0.6 + breathiness

        # Add sibilance (high-frequency bursts at start/end)
        sibilance_samples = int(0.15 * self.sample_rate)
        sibilance = np.random.randn(sibilance_samples)

        # High-pass the sibilance
        for i in range(1, len(sibilance)):
            sibilance[i] = 0.95 * sibilance[i] - 0.9 * sibilance[i-1]

        sibilance *= np.linspace(0.3, 0, sibilance_samples)
        vocal[:sibilance_samples] += sibilance * 0.15
        vocal[-sibilance_samples:] += sibilance[::-1] * 0.12

        # Normalize
        vocal = vocal / np.max(np.abs(vocal)) * 0.7

        # Stereo: slight difference for realism
        audio[:, 0] = vocal
        audio[:, 1] = vocal * 0.98  # Slight amplitude difference

        return AudioMaterial(
            name="vocal_sample_formants",
            description="Vocal-like sample with formants, vibrato, sibilance",
            data=audio,
            sample_rate=self.sample_rate,
            purpose="Test pitch shifters, formant processing, de-essers, dynamics",
            test_targets=["Pitch Shifters", "Formant", "De-esser", "Compressor", "EQ"]
        )

    def generate_guitar_chord(self) -> AudioMaterial:
        """Generate acoustic guitar chord"""
        print("Generating guitar chord (acoustic)...")

        duration = 4.0
        samples = int(duration * self.sample_rate)
        audio = np.zeros((samples, 2))

        # E major chord: E2 (82.4Hz), B2 (123.5Hz), E3 (164.8Hz), G#3 (207.7Hz), B3 (247Hz), E4 (329.6Hz)
        chord_notes = [82.4, 123.5, 164.8, 207.7, 247.0, 329.6]
        note_amps = [0.9, 0.7, 0.85, 0.6, 0.7, 0.8]  # Simulate string volume differences

        t = np.arange(samples) / self.sample_rate

        for freq, amp in zip(chord_notes, note_amps):
            # Generate string with harmonics
            string = np.zeros(samples)

            # Add harmonics (up to 10th)
            for harmonic in range(1, 11):
                harmonic_amp = amp / (harmonic ** 1.2)

                # Each harmonic decays differently
                decay_rate = 0.5 + 0.3 * harmonic
                harmonic_envelope = np.exp(-decay_rate * t)

                # Add slight detuning to harmonics for realism
                detune = 1.0 + np.random.randn() * 0.002

                string += harmonic_amp * np.sin(2 * np.pi * freq * harmonic * detune * t) * harmonic_envelope

            # Attack envelope (guitar pick attack)
            attack_samples = int(0.005 * self.sample_rate)
            attack_env = np.ones(samples)
            attack_env[:attack_samples] = np.linspace(0, 1, attack_samples) ** 0.5

            string *= attack_env

            # Add to stereo field (strings at different positions)
            pan = np.random.uniform(0.3, 0.7)
            audio[:, 0] += string * np.sqrt(1 - pan)
            audio[:, 1] += string * np.sqrt(pan)

        # Add body resonance (200-400 Hz bump)
        body_resonance = np.random.randn(samples) * 0.02
        body_resonance = np.convolve(body_resonance, np.hamming(1024), mode='same')
        audio[:, 0] += body_resonance
        audio[:, 1] += body_resonance

        # Normalize
        max_val = np.max(np.abs(audio))
        if max_val > 0:
            audio = audio * 0.75 / max_val

        return AudioMaterial(
            name="guitar_chord_emajor",
            description="Acoustic guitar E major chord (full frequency range)",
            data=audio,
            sample_rate=self.sample_rate,
            purpose="Test reverbs, delays, modulation, stereo processing",
            test_targets=["Reverb", "Delay", "Chorus", "Stereo Width", "Modulation"]
        )

    def generate_piano_notes(self) -> AudioMaterial:
        """Generate piano notes across frequency range"""
        print("Generating piano notes (C1, C4, C7)...")

        duration = 6.0  # 2 seconds per note
        samples = int(duration * self.sample_rate)
        audio = np.zeros((samples, 2))

        # Piano notes: C1 (32.7Hz), C4 (261.6Hz), C7 (2093Hz)
        piano_notes = [
            (32.7, 0.0, "C1 - Very low"),
            (261.6, 2.0, "C4 - Middle C"),
            (2093, 4.0, "C7 - Very high")
        ]

        for freq, start_time, label in piano_notes:
            start_sample = int(start_time * self.sample_rate)
            note_duration = 1.8  # seconds
            note_samples = int(note_duration * self.sample_rate)

            t = np.arange(note_samples) / self.sample_rate

            # Generate piano note with inharmonicity
            note = np.zeros(note_samples)

            # Piano has inharmonicity (higher partials are sharper)
            inharmonicity = 0.0001 * (freq / 261.6) ** 2

            # Add harmonics
            num_harmonics = max(3, int(20000 / freq))
            for harmonic in range(1, num_harmonics):
                # Inharmonic partial frequency
                partial_freq = freq * harmonic * np.sqrt(1 + inharmonicity * harmonic ** 2)

                if partial_freq > 20000:
                    break

                # Harmonic amplitude (piano has strong odd harmonics)
                if harmonic % 2 == 1:
                    harmonic_amp = 0.7 / (harmonic ** 1.0)
                else:
                    harmonic_amp = 0.5 / (harmonic ** 1.2)

                # Each harmonic has different decay
                decay_rate = 0.3 + 0.1 * harmonic
                harmonic_envelope = np.exp(-decay_rate * t)

                note += harmonic_amp * np.sin(2 * np.pi * partial_freq * t) * harmonic_envelope

            # Attack envelope (piano hammer strike)
            attack_samples = int(0.01 * self.sample_rate)
            attack_env = np.ones(note_samples)
            attack_env[:attack_samples] = (np.linspace(0, 1, attack_samples)) ** 2

            note *= attack_env

            # Add hammer noise for attack
            hammer_samples = int(0.003 * self.sample_rate)
            hammer_noise = np.random.randn(hammer_samples) * 0.05
            hammer_noise *= np.linspace(1, 0, hammer_samples)
            note[:hammer_samples] += hammer_noise

            # Normalize note
            note = note / np.max(np.abs(note)) * 0.7

            # Write to stereo (slightly different for each channel)
            end_sample = min(start_sample + note_samples, samples)
            actual_samples = end_sample - start_sample
            audio[start_sample:end_sample, 0] += note[:actual_samples]
            audio[start_sample:end_sample, 1] += note[:actual_samples] * 0.95

        return AudioMaterial(
            name="piano_notes_c1_c4_c7",
            description="Piano notes across full frequency range (C1, C4, C7)",
            data=audio,
            sample_rate=self.sample_rate,
            purpose="Test full frequency range, reverb density, delay feedback",
            test_targets=["Reverb", "Delay", "EQ", "Filters", "Dynamics"]
        )

    def generate_white_noise_burst(self) -> AudioMaterial:
        """Generate white noise burst"""
        print("Generating white noise burst...")

        duration = 2.0
        samples = int(duration * self.sample_rate)

        # Generate white noise
        noise = np.random.randn(samples, 2) * 0.5

        # Envelope: 0.5s burst with fade in/out
        burst_duration = 0.5
        burst_samples = int(burst_duration * self.sample_rate)

        envelope = np.zeros(samples)

        # Burst in middle
        start_burst = int(0.5 * self.sample_rate)
        end_burst = start_burst + burst_samples

        fade_samples = int(0.05 * self.sample_rate)
        envelope[start_burst:start_burst+fade_samples] = np.linspace(0, 1, fade_samples)
        envelope[start_burst+fade_samples:end_burst-fade_samples] = 1.0
        envelope[end_burst-fade_samples:end_burst] = np.linspace(1, 0, fade_samples)

        noise[:, 0] *= envelope
        noise[:, 1] *= envelope

        return AudioMaterial(
            name="white_noise_burst",
            description="White noise burst (0.5s, full spectrum)",
            data=noise,
            sample_rate=self.sample_rate,
            purpose="Test spectral processing, gate/expander behavior",
            test_targets=["Spectral Processing", "Gates", "Expanders", "Filters", "EQ"]
        )

    def generate_pink_noise_sustained(self) -> AudioMaterial:
        """Generate sustained pink noise"""
        print("Generating pink noise sustained...")

        duration = 3.0
        samples = int(duration * self.sample_rate)

        # Generate white noise
        white_noise = np.random.randn(samples, 2)

        # Convert to pink noise using Paul Kellet's algorithm
        pink_noise = np.zeros((samples, 2))

        for channel in range(2):
            b0, b1, b2, b3, b4, b5, b6 = 0, 0, 0, 0, 0, 0, 0

            for i in range(samples):
                white = white_noise[i, channel]
                b0 = 0.99886 * b0 + white * 0.0555179
                b1 = 0.99332 * b1 + white * 0.0750759
                b2 = 0.96900 * b2 + white * 0.1538520
                b3 = 0.86650 * b3 + white * 0.3104856
                b4 = 0.55000 * b4 + white * 0.5329522
                b5 = -0.7616 * b5 - white * 0.0168980

                pink_noise[i, channel] = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362
                b6 = white * 0.115926

        # Envelope: fade in and out
        fade_samples = int(0.1 * self.sample_rate)
        envelope = np.ones(samples)
        envelope[:fade_samples] = np.linspace(0, 1, fade_samples)
        envelope[-fade_samples:] = np.linspace(1, 0, fade_samples)

        pink_noise[:, 0] *= envelope
        pink_noise[:, 1] *= envelope

        # Normalize
        pink_noise = pink_noise / np.max(np.abs(pink_noise)) * 0.6

        return AudioMaterial(
            name="pink_noise_sustained",
            description="Sustained pink noise (3s, balanced spectrum)",
            data=pink_noise,
            sample_rate=self.sample_rate,
            purpose="Test frequency response accuracy, check for artifacts",
            test_targets=["Frequency Response", "Filters", "EQ", "Dynamics", "Distortion"]
        )

    def save_materials(self, output_dir: str = "real_world_test_materials"):
        """Save all materials as WAV files"""
        os.makedirs(output_dir, exist_ok=True)

        print(f"\nSaving test materials to {output_dir}/")

        for material in self.materials:
            filename = os.path.join(output_dir, f"{material.name}.wav")
            self._save_wav(filename, material.data, material.sample_rate)
            print(f"  Saved: {filename}")

        # Save manifest
        manifest_path = os.path.join(output_dir, "MATERIALS_MANIFEST.txt")
        with open(manifest_path, 'w') as f:
            f.write("REAL-WORLD MUSICAL TEST MATERIALS\n")
            f.write("=" * 70 + "\n\n")

            for i, material in enumerate(self.materials, 1):
                f.write(f"{i}. {material.name}\n")
                f.write(f"   Description: {material.description}\n")
                f.write(f"   Duration: {len(material.data) / material.sample_rate:.2f}s\n")
                f.write(f"   Purpose: {material.purpose}\n")
                f.write(f"   Test Targets: {', '.join(material.test_targets)}\n")
                f.write("\n")

        print(f"  Saved: {manifest_path}")
        print(f"\nGenerated {len(self.materials)} test materials successfully!")

    def _save_wav(self, filename: str, audio: np.ndarray, sample_rate: int):
        """Save audio data as WAV file"""
        # Convert to 16-bit PCM
        audio_int16 = np.clip(audio * 32767, -32768, 32767).astype(np.int16)

        with wave.open(filename, 'w') as wav_file:
            wav_file.setnchannels(2)  # Stereo
            wav_file.setsampwidth(2)  # 16-bit
            wav_file.setframerate(sample_rate)

            # Interleave stereo channels
            for sample in audio_int16:
                wav_file.writeframes(struct.pack('hh', sample[0], sample[1]))

def main():
    """Generate all musical test materials"""
    print("=" * 70)
    print("REAL-WORLD MUSICAL MATERIAL GENERATOR")
    print("=" * 70)
    print()

    generator = MusicalMaterialGenerator(sample_rate=48000)
    generator.generate_all_materials()
    generator.save_materials()

    print("\n" + "=" * 70)
    print("GENERATION COMPLETE")
    print("=" * 70)

if __name__ == "__main__":
    main()
