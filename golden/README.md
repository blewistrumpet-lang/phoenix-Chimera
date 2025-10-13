# Golden Reference Audio

This directory contains **golden reference audio files** representing the known-good DSP baseline from the `v3.0-golden-reference` tag.

## Purpose

These files provide bit-exact regression testing to ensure DSP changes don't introduce unintended artifacts or behavior changes. Any deviation from these golden files indicates a DSP modification that requires review.

## Source

All golden audio was rendered from the **v3.0-golden-reference** tag using the offline renderer tool with `CHIMERA_NEW_DSP=0` (stable baseline DSP).

## Test Coverage

### Reverbs (Engine IDs 38-42)
- `PlateReverb_impulse.wav` - Plate reverb impulse response (Engine ID 38)
- `PlateReverb_sine1k.wav` - Plate reverb with 1kHz sine tone (Engine ID 38)

### Dynamics (Engine IDs 15-21)
- `ClassicCompressor_sine1k.wav` - Classic compressor on 1kHz sine (Engine ID 15)
- `NoiseGate_noise.wav` - Noise gate on white noise (Engine ID 18)

### Distortion (Engine IDs 6-14)
- `BitCrusher_sine1k.wav` - Bit crusher on 1kHz sine (Engine ID 6)
- `RodentDistortion_sweep.wav` - Rodent distortion on frequency sweep (Engine ID 11)

### Pitch (Engine IDs 46-48)
- `PitchShifter_sine1k.wav` - Pitch shifter on 1kHz sine (Engine ID 46)

### Modulation (Engine IDs 22-30)
- `ClassicChorus_sine1k.wav` - Classic chorus on 1kHz sine (Engine ID 22)

## Regenerating Golden Audio

To regenerate the golden reference files (e.g., after intentional DSP improvements):

```bash
# Build the offline renderer
cd tools/offline_renderer
./build.sh

# Render golden audio
./render_goldens.sh
```

## Comparison Testing

Future scripts will compare new renders against these golden files using:
- RMS deviation
- FFT spectral analysis
- LUFS loudness delta
- Peak detection

Thresholds for acceptable deviation will be defined per-engine based on expected precision.

## File Format

All golden files are:
- 24-bit PCM WAV
- 48 kHz sample rate
- Mono (summed from stereo processing)
- Stored in Git LFS

## Engine ID Reference

See `JUCE_Plugin/Source/PluginProcessor.cpp` for complete engine ID mappings.
