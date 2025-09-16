#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import csv

def load_spectrogram(filename):
    data = []
    with open(filename, 'r') as f:
        reader = csv.reader(f)
        for row in reader:
            data.append([float(x) for x in row])
    return np.array(data).T

def analyze_artifacts(spec):
    """Identify specific artifact patterns in spectrogram"""
    # Look for horizontal lines (constant frequencies - could be residual carrier)
    horizontal_energy = np.mean(np.abs(np.diff(spec, axis=1)), axis=1)
    
    # Look for vertical lines (clicks/discontinuities)
    vertical_energy = np.mean(np.abs(np.diff(spec, axis=0)), axis=0)
    
    # Find modulation artifacts (beating patterns)
    time_variation = np.std(spec, axis=1)
    
    return horizontal_energy, vertical_energy, time_variation

# Analyze the tritone case (most problematic)
spec = load_spectrogram('spectrogram_sine_0.7071.csv')

# Create figure with multiple analyses
fig, axes = plt.subplots(3, 2, figsize=(14, 10))

# 1. Spectrogram
ax = axes[0, 0]
im = ax.imshow(spec, aspect='auto', origin='lower', cmap='viridis', 
               vmin=-80, vmax=-20)
ax.set_title('Spectrogram - Tritone Down (0.7071)')
ax.set_xlabel('Time (frames)')
ax.set_ylabel('Frequency (bins)')
plt.colorbar(im, ax=ax, label='dB')

# 2. Time slice at fundamental
ax = axes[0, 1]
fundamental_bin = 15  # ~155Hz at 48kHz/2048
ax.plot(spec[fundamental_bin, :])
ax.set_title(f'Amplitude at Fundamental (~155Hz)')
ax.set_xlabel('Time (frames)')
ax.set_ylabel('Amplitude (dB)')
ax.grid(True)

# 3. Frequency spectrum (average)
ax = axes[1, 0]
avg_spectrum = np.mean(spec, axis=1)
freqs = np.arange(len(avg_spectrum)) * (48000 / 2048)
ax.plot(freqs[:100], avg_spectrum[:100])
ax.set_title('Average Spectrum')
ax.set_xlabel('Frequency (Hz)')
ax.set_ylabel('Magnitude (dB)')
ax.grid(True)

# 4. Click detection
ax = axes[1, 1]
h_energy, v_energy, t_var = analyze_artifacts(spec)
ax.plot(v_energy)
ax.set_title('Click/Discontinuity Detection')
ax.set_xlabel('Time (frames)')
ax.set_ylabel('Spectral Flux')
ax.axhline(y=np.mean(v_energy) + 2*np.std(v_energy), color='r', 
           linestyle='--', label='Threshold')
ax.legend()
ax.grid(True)

# 5. Subharmonic analysis
ax = axes[2, 0]
# Look for energy at F0/2 and F0/3
f0_bin = 15
half_bin = f0_bin // 2
third_bin = f0_bin // 3
ax.plot(spec[f0_bin, :], label='F0 (155Hz)')
ax.plot(spec[half_bin, :], label='F0/2 (78Hz)')
ax.plot(spec[third_bin, :], label='F0/3 (52Hz)')
ax.set_title('Subharmonic Content')
ax.set_xlabel('Time (frames)')
ax.set_ylabel('Amplitude (dB)')
ax.legend()
ax.grid(True)

# 6. Modulation/beating detection
ax = axes[2, 1]
# Compute envelope of fundamental
fundamental_envelope = spec[f0_bin, :]
from scipy.signal import hilbert
analytic = hilbert(fundamental_envelope)
envelope = np.abs(analytic)
ax.plot(envelope)
ax.set_title('Amplitude Modulation at Fundamental')
ax.set_xlabel('Time (frames)')
ax.set_ylabel('Envelope')
ax.grid(True)

plt.tight_layout()
plt.savefig('artifact_analysis.png', dpi=150)
print("Saved artifact analysis to artifact_analysis.png")

# Print specific findings
print("\n=== ARTIFACT ANALYSIS RESULTS ===")
print(f"Average click energy: {np.mean(v_energy):.2f}")
print(f"Peak click energy: {np.max(v_energy):.2f}")
print(f"Number of potential clicks: {np.sum(v_energy > np.mean(v_energy) + 2*np.std(v_energy))}")

f0_power = np.mean(spec[f0_bin, :])
sub_power = np.mean(spec[half_bin, :])
print(f"\nFundamental power: {f0_power:.2f} dB")
print(f"Subharmonic (F0/2) power: {sub_power:.2f} dB")
print(f"Subharmonic ratio: {sub_power - f0_power:.2f} dB")

print(f"\nAmplitude modulation depth: {np.std(envelope):.2f}")
print(f"Modulation frequency estimate: ~{np.abs(f0_bin - half_bin) * (48000/2048):.1f} Hz")