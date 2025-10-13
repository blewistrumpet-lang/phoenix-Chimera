#!/bin/bash
# Render golden reference audio from v3.0-golden-reference
# These represent the known-good DSP baseline

RENDERER="./tools/offline_renderer/offline_renderer"

# Render key engines across different categories
echo "Rendering golden reference audio..."

# Reverbs (38-42)
$RENDERER fixtures/impulse.wav golden/PlateReverb_impulse.wav 38
$RENDERER fixtures/sine1k.wav golden/PlateReverb_sine1k.wav 38

# Dynamics (15-21)
$RENDERER fixtures/sine1k.wav golden/ClassicCompressor_sine1k.wav 15
$RENDERER fixtures/noise.wav golden/NoiseGate_noise.wav 18

# Distortion (6-14)
$RENDERER fixtures/sine1k.wav golden/BitCrusher_sine1k.wav 6
$RENDERER fixtures/sweep.wav golden/RodentDistortion_sweep.wav 11

# Pitch (46-48)  
$RENDERER fixtures/sine1k.wav golden/PitchShifter_sine1k.wav 46

# Modulation (22-30)
$RENDERER fixtures/sine1k.wav golden/ClassicChorus_sine1k.wav 22

echo "✅ Golden reference audio generated"
ls -lh golden/
