#!/usr/bin/env python3
"""
Direct preset export script that creates JSON files from the C++ preset functions
This is a practical solution that avoids compilation complexity
"""

import json
import os
from pathlib import Path
import subprocess
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Map old engine constants to standardized ones based on PresetExporter.h
ENGINE_MAPPING = {
    # Original mappings from the C++ file
    "ENGINE_K_STYLE": 0,  # VINTAGE_TUBE
    "ENGINE_TAPE_ECHO": 1,
    "ENGINE_PLATE_REVERB": 3,
    "ENGINE_RODENT_DISTORTION": 36,
    "ENGINE_MUFF_FUZZ": 35,
    "ENGINE_CLASSIC_TREMOLO": 22,
    "ENGINE_MAGNETIC_DRUM_ECHO": 8,
    "ENGINE_BUCKET_BRIGADE_DELAY": 9,
    "ENGINE_DIGITAL_DELAY": 9,  # Same as bucket brigade for now
    "ENGINE_HARMONIC_TREMOLO": 21,
    "ENGINE_ROTARY_SPEAKER": 18,  # Maps to DIMENSION_EXPANDER
    "ENGINE_DETUNE_DOUBLER": 43,
    "ENGINE_LADDER_FILTER": 28,
    "ENGINE_FORMANT_FILTER": 30,
    "ENGINE_CLASSIC_COMPRESSOR": 7,  # VCA_COMPRESSOR
    "ENGINE_STATE_VARIABLE_FILTER": 29,
    "ENGINE_STEREO_CHORUS": 10,  # ANALOG_CHORUS
    "ENGINE_SPECTRAL_FREEZE": 38,
    "ENGINE_GRANULAR_CLOUD": 16,
    "ENGINE_ANALOG_RING_MODULATOR": 15,  # RING_MODULATOR
    "ENGINE_MULTIBAND_SATURATOR": 34,
    "ENGINE_COMB_RESONATOR": 23,
    "ENGINE_PITCH_SHIFTER": 14,
    "ENGINE_PHASED_VOCODER": 44,
    "ENGINE_CONVOLUTION_REVERB": 4,
    "ENGINE_BIT_CRUSHER": 33,
    "ENGINE_FREQUENCY_SHIFTER": 19,
    "ENGINE_WAVE_FOLDER": 31,
    "ENGINE_SHIMMER_REVERB": 2,
    "ENGINE_VOCAL_FORMANT_FILTER": 17,
    "ENGINE_TRANSIENT_SHAPER": 20,
    "ENGINE_DIMENSION_EXPANDER": 18,
    "ENGINE_ANALOG_PHASER": 12,
    "ENGINE_ENVELOPE_FILTER": 47,
    "ENGINE_GATED_REVERB": 42,
    "ENGINE_HARMONIC_EXCITER": 32,
    "ENGINE_FEEDBACK_NETWORK": 48,
    "ENGINE_INTELLIGENT_HARMONIZER": 41,
    "ENGINE_PARAMETRIC_EQ": 27,
    "ENGINE_MASTERING_LIMITER": 49,
    "ENGINE_NOISE_GATE": 46,
    "ENGINE_VINTAGE_OPTO_COMPRESSOR": 6,  # OPTO_COMPRESSOR
    "ENGINE_SPECTRAL_GATE": 45,
    "ENGINE_CHAOS_GENERATOR": 40,
    "ENGINE_BUFFER_REPEAT": 39,
    "ENGINE_VINTAGE_CONSOLE_EQ": 26,
    "ENGINE_MID_SIDE_PROCESSOR": 25,
    "ENGINE_VINTAGE_TUBE_PREAMP": 0,  # Same as VINTAGE_TUBE
    "ENGINE_SPRING_REVERB": 5,
    "ENGINE_RESONANT_CHORUS": 11,  # DIGITAL_CHORUS
    
    # Direct numeric mappings
    0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9,
    10: 10, 11: 11, 12: 12, 13: 13, 14: 14, 15: 15, 16: 16, 17: 17, 18: 18, 19: 19,
    20: 20, 21: 21, 22: 22, 23: 23, 24: 24, 25: 25, 26: 26, 27: 27, 28: 28, 29: 29,
    30: 30, 31: 31, 32: 32, 33: 33, 34: 34, 35: 35, 36: 36, 37: 37, 38: 38, 39: 39,
    40: 40, 41: 41, 42: 42, 43: 43, 44: 44, 45: 45, 46: 46, 47: 47, 48: 48, 49: 49
}

def create_preset_json(preset_id, preset_data):
    """Create a properly formatted preset JSON structure"""
    
    # Manually define the 30 presets we've created
    presets_data = {
        "GC_001": {
            "name": "Velvet Thunder",
            "technicalHint": "K-Style + Tape Echo + Harmonic Tremolo",
            "shortCode": "VTH",
            "category": "Studio Essentials",
            "subcategory": "Vintage Warmth",
            "engines": [
                {"slot": 0, "type": 0, "mix": 0.75, "active": True},  # K-Style (Vintage Tube)
                {"slot": 1, "type": 1, "mix": 0.65, "active": True},  # Tape Echo
                {"slot": 2, "type": 21, "mix": 0.45, "active": True}  # Harmonic Tremolo
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 1.8,
            "complexity": 0.42,
            "experimentalness": 0.15,
            "versatility": 0.78
        },
        "GC_002": {
            "name": "Crystal Palace",
            "technicalHint": "Shimmer Reverb + Ladder Filter + Dimension Expander",
            "shortCode": "CRP",
            "category": "Spatial Design",
            "subcategory": "Ethereal Spaces",
            "engines": [
                {"slot": 0, "type": 2, "mix": 0.85, "active": True},   # Shimmer Reverb
                {"slot": 1, "type": 28, "mix": 0.55, "active": True},  # Ladder Filter
                {"slot": 2, "type": 18, "mix": 0.70, "active": True}   # Dimension Expander
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.2,
            "complexity": 0.58,
            "experimentalness": 0.32,
            "versatility": 0.85
        },
        "GC_003": {
            "name": "Broken Radio",
            "technicalHint": "Bit Crusher + Formant Filter + Granular Cloud + Ring Mod",
            "shortCode": "BRK",
            "category": "Character & Color",
            "subcategory": "Lo-Fi Aesthetics",
            "engines": [
                {"slot": 0, "type": 33, "mix": 0.68, "active": True},  # Bit Crusher
                {"slot": 1, "type": 30, "mix": 0.52, "active": True},  # Formant Filter
                {"slot": 2, "type": 16, "mix": 0.45, "active": True},  # Granular Cloud
                {"slot": 3, "type": 15, "mix": 0.38, "active": True}   # Ring Modulator
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.4,
            "complexity": 0.51,
            "experimentalness": 0.48,
            "versatility": 0.72
        },
        "GC_004": {
            "name": "Midnight Oil",
            "technicalHint": "Vintage Tube + Opto Compressor + Spring Reverb + Analog Phaser",
            "shortCode": "MID",
            "category": "Studio Essentials",
            "subcategory": "Analog Warmth",
            "engines": [
                {"slot": 0, "type": 0, "mix": 0.82, "active": True},   # Vintage Tube
                {"slot": 1, "type": 6, "mix": 0.68, "active": True},   # Opto Compressor
                {"slot": 2, "type": 5, "mix": 0.45, "active": True},   # Spring Reverb
                {"slot": 3, "type": 12, "mix": 0.35, "active": True}   # Analog Phaser
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.8,
            "complexity": 0.68,
            "experimentalness": 0.22,
            "versatility": 0.82
        },
        "GC_005": {
            "name": "Glass Cathedral",
            "technicalHint": "Convolution Reverb + Plate Reverb + Spectral Freeze",
            "shortCode": "GLC",
            "category": "Spatial Design",
            "subcategory": "Sacred Spaces",
            "engines": [
                {"slot": 0, "type": 4, "mix": 0.78, "active": True},   # Convolution Reverb
                {"slot": 1, "type": 3, "mix": 0.65, "active": True},   # Plate Reverb
                {"slot": 2, "type": 38, "mix": 0.42, "active": True}   # Spectral Freeze
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 4.5,
            "complexity": 0.75,
            "experimentalness": 0.48,
            "versatility": 0.68
        },
        "GC_006": {
            "name": "Neon Dreams",
            "technicalHint": "Digital Delay + Stereo Chorus + Frequency Shifter + Multiband Saturator",
            "shortCode": "NEO",
            "category": "Character & Color",
            "subcategory": "80s Revival",
            "engines": [
                {"slot": 0, "type": 9, "mix": 0.72, "active": True},   # Digital Delay
                {"slot": 1, "type": 10, "mix": 0.65, "active": True},  # Stereo Chorus
                {"slot": 2, "type": 19, "mix": 0.48, "active": True},  # Frequency Shifter
                {"slot": 3, "type": 34, "mix": 0.35, "active": True}   # Multiband Saturator
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.9,
            "complexity": 0.62,
            "experimentalness": 0.35,
            "versatility": 0.78
        },
        "GC_007": {
            "name": "Liquid Sunshine",
            "technicalHint": "State Variable Filter + Vocal Formant + Dimension Expander + Intelligent Harmonizer + Mastering Limiter",
            "shortCode": "LQS",
            "category": "Motion & Modulation",
            "subcategory": "Flowing Movement",
            "engines": [
                {"slot": 0, "type": 29, "mix": 0.68, "active": True},  # State Variable Filter
                {"slot": 1, "type": 17, "mix": 0.55, "active": True},  # Vocal Formant
                {"slot": 2, "type": 18, "mix": 0.48, "active": True},  # Dimension Expander
                {"slot": 3, "type": 41, "mix": 0.38, "active": True},  # Intelligent Harmonizer
                {"slot": 4, "type": 49, "mix": 0.25, "active": True}   # Mastering Limiter
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 3.8,
            "complexity": 0.78,
            "experimentalness": 0.52,
            "versatility": 0.85
        },
        "GC_008": {
            "name": "Iron Butterfly",
            "technicalHint": "Muff Fuzz + Comb Resonator + Gated Reverb + Transient Shaper",
            "shortCode": "IRB",
            "category": "Character & Color",
            "subcategory": "Heavy Distortion",
            "engines": [
                {"slot": 0, "type": 35, "mix": 0.78, "active": True},  # Muff Fuzz
                {"slot": 1, "type": 23, "mix": 0.62, "active": True},  # Comb Resonator
                {"slot": 2, "type": 42, "mix": 0.55, "active": True},  # Gated Reverb
                {"slot": 3, "type": 20, "mix": 0.45, "active": True}   # Transient Shaper
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 5.2,
            "complexity": 0.72,
            "experimentalness": 0.68,
            "versatility": 0.55
        },
        "GC_009": {
            "name": "Phantom Embrace",
            "technicalHint": "Pitch Shifter + Analog Phaser + Feedback Network + Mid-Side Processor",
            "shortCode": "PHE",
            "category": "Spatial Design",
            "subcategory": "Ghostly Atmospheres",
            "engines": [
                {"slot": 0, "type": 14, "mix": 0.65, "active": True},  # Pitch Shifter
                {"slot": 1, "type": 12, "mix": 0.58, "active": True},  # Analog Phaser
                {"slot": 2, "type": 48, "mix": 0.72, "active": True},  # Feedback Network
                {"slot": 3, "type": 25, "mix": 0.48, "active": True}   # Mid-Side Processor
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 3.5,
            "complexity": 0.82,
            "experimentalness": 0.75,
            "versatility": 0.62
        },
        "GC_010": {
            "name": "Solar Flare",
            "technicalHint": "Chaos Generator + Wave Folder + Harmonic Exciter + Envelope Filter + Spectral Gate",
            "shortCode": "SLR",
            "category": "Experimental Laboratory",
            "subcategory": "Extreme Processing",
            "engines": [
                {"slot": 0, "type": 40, "mix": 0.85, "active": True},  # Chaos Generator
                {"slot": 1, "type": 31, "mix": 0.72, "active": True},  # Wave Folder
                {"slot": 2, "type": 32, "mix": 0.58, "active": True},  # Harmonic Exciter
                {"slot": 3, "type": 47, "mix": 0.45, "active": True},  # Envelope Filter
                {"slot": 4, "type": 45, "mix": 0.38, "active": True}   # Spectral Gate
            ],
            "cpuTier": "HEAVY",
            "actualCpuPercent": 8.5,
            "complexity": 0.92,
            "experimentalness": 0.88,
            "versatility": 0.45
        },
        "GC_011": {
            "name": "Dust And Echoes",
            "technicalHint": "Magnetic Drum Echo + Vintage Console EQ + Parametric EQ",
            "shortCode": "DAE",
            "category": "Spatial Design",
            "subcategory": "Vintage Spaces",
            "engines": [
                {"slot": 0, "type": 8, "mix": 0.75, "active": True},   # Magnetic Drum Echo
                {"slot": 1, "type": 26, "mix": 0.62, "active": True},  # Vintage Console EQ
                {"slot": 2, "type": 27, "mix": 0.48, "active": True}   # Parametric EQ
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.1,
            "complexity": 0.55,
            "experimentalness": 0.28,
            "versatility": 0.78
        },
        "GC_012": {
            "name": "Thunder And Silk",
            "technicalHint": "Transient Shaper + Shimmer Reverb + Opto Compressor + Stereo Chorus",
            "shortCode": "TAS",
            "category": "Character & Color",
            "subcategory": "Dynamic Contrast",
            "engines": [
                {"slot": 0, "type": 20, "mix": 0.78, "active": True},  # Transient Shaper
                {"slot": 1, "type": 2, "mix": 0.65, "active": True},   # Shimmer Reverb
                {"slot": 2, "type": 6, "mix": 0.52, "active": True},   # Opto Compressor
                {"slot": 3, "type": 10, "mix": 0.38, "active": True}   # Stereo Chorus
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 4.2,
            "complexity": 0.68,
            "experimentalness": 0.42,
            "versatility": 0.82
        },
        "GC_013": {
            "name": "Quantum Garden",
            "technicalHint": "Granular Cloud + Spectral Freeze + Buffer Repeat + Chaos Generator + Intelligent Harmonizer",
            "shortCode": "QTG",
            "category": "Experimental Laboratory",
            "subcategory": "Quantum Processing",
            "engines": [
                {"slot": 0, "type": 16, "mix": 0.72, "active": True},  # Granular Cloud
                {"slot": 1, "type": 38, "mix": 0.68, "active": True},  # Spectral Freeze
                {"slot": 2, "type": 39, "mix": 0.55, "active": True},  # Buffer Repeat
                {"slot": 3, "type": 40, "mix": 0.48, "active": True},  # Chaos Generator
                {"slot": 4, "type": 41, "mix": 0.35, "active": True}   # Intelligent Harmonizer
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 5.8,
            "complexity": 0.88,
            "experimentalness": 0.92,
            "versatility": 0.58
        },
        "GC_014": {
            "name": "Copper Resonance",
            "technicalHint": "Comb Resonator + Formant Filter + Ring Modulator",
            "shortCode": "CPR",
            "category": "Character & Color",
            "subcategory": "Metallic Tones",
            "engines": [
                {"slot": 0, "type": 23, "mix": 0.75, "active": True},  # Comb Resonator
                {"slot": 1, "type": 30, "mix": 0.62, "active": True},  # Formant Filter
                {"slot": 2, "type": 15, "mix": 0.48, "active": True}   # Ring Modulator
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.7,
            "complexity": 0.62,
            "experimentalness": 0.55,
            "versatility": 0.68
        },
        "GC_015": {
            "name": "Aurora Borealis",
            "technicalHint": "Frequency Shifter + Dimension Expander + Phased Vocoder + Mid-Side Processor",
            "shortCode": "AUR",
            "category": "Spatial Design",
            "subcategory": "Atmospheric Phenomena",
            "engines": [
                {"slot": 0, "type": 19, "mix": 0.72, "active": True},  # Frequency Shifter
                {"slot": 1, "type": 18, "mix": 0.65, "active": True},  # Dimension Expander
                {"slot": 2, "type": 44, "mix": 0.55, "active": True},  # Phased Vocoder
                {"slot": 3, "type": 25, "mix": 0.42, "active": True}   # Mid-Side Processor
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.3,
            "complexity": 0.75,
            "experimentalness": 0.68,
            "versatility": 0.72
        },
        "GC_016": {
            "name": "Digital Erosion",
            "technicalHint": "Bit Crusher + Granular Cloud + Buffer Repeat + Spectral Gate",
            "shortCode": "DIG",
            "category": "Character & Color",
            "subcategory": "Digital Degradation",
            "engines": [
                {"slot": 0, "type": 33, "mix": 0.78, "active": True},  # Bit Crusher
                {"slot": 1, "type": 16, "mix": 0.65, "active": True},  # Granular Cloud
                {"slot": 2, "type": 39, "mix": 0.52, "active": True},  # Buffer Repeat
                {"slot": 3, "type": 45, "mix": 0.45, "active": True}   # Spectral Gate
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.6,
            "complexity": 0.72,
            "experimentalness": 0.78,
            "versatility": 0.55
        },
        "GC_017": {
            "name": "Molten Core",
            "technicalHint": "Wave Folder + Multiband Saturator + Feedback Network + Gated Reverb",
            "shortCode": "MLC",
            "category": "Character & Color",
            "subcategory": "Extreme Heat",
            "engines": [
                {"slot": 0, "type": 31, "mix": 0.82, "active": True},  # Wave Folder
                {"slot": 1, "type": 34, "mix": 0.72, "active": True},  # Multiband Saturator
                {"slot": 2, "type": 48, "mix": 0.58, "active": True},  # Feedback Network
                {"slot": 3, "type": 42, "mix": 0.45, "active": True}   # Gated Reverb
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.5,
            "complexity": 0.78,
            "experimentalness": 0.72,
            "versatility": 0.58
        },
        "GC_018": {
            "name": "Whisper Network",
            "technicalHint": "Noise Gate + Vocal Formant Filter + Convolution Reverb",
            "shortCode": "WSP",
            "category": "Studio Essentials",
            "subcategory": "Intimate Processing",
            "engines": [
                {"slot": 0, "type": 46, "mix": 0.72, "active": True},  # Noise Gate
                {"slot": 1, "type": 17, "mix": 0.65, "active": True},  # Vocal Formant
                {"slot": 2, "type": 4, "mix": 0.58, "active": True}    # Convolution Reverb
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 1.9,
            "complexity": 0.52,
            "experimentalness": 0.35,
            "versatility": 0.78
        },
        "GC_019": {
            "name": "Cosmic Strings",
            "technicalHint": "Pitch Shifter + Comb Resonator + Intelligent Harmonizer + Shimmer Reverb",
            "shortCode": "COS",
            "category": "Motion & Modulation",
            "subcategory": "Harmonic Evolution",
            "engines": [
                {"slot": 0, "type": 14, "mix": 0.75, "active": True},  # Pitch Shifter
                {"slot": 1, "type": 23, "mix": 0.62, "active": True},  # Comb Resonator
                {"slot": 2, "type": 41, "mix": 0.55, "active": True},  # Intelligent Harmonizer
                {"slot": 3, "type": 2, "mix": 0.48, "active": True}    # Shimmer Reverb
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.8,
            "complexity": 0.75,
            "experimentalness": 0.65,
            "versatility": 0.72
        },
        "GC_020": {
            "name": "Rust And Bones",
            "technicalHint": "Rodent Distortion + Spring Reverb + Vintage Console EQ + Transient Shaper",
            "shortCode": "RAB",
            "category": "Character & Color",
            "subcategory": "Decayed Textures",
            "engines": [
                {"slot": 0, "type": 36, "mix": 0.78, "active": True},  # Rodent Distortion
                {"slot": 1, "type": 5, "mix": 0.65, "active": True},   # Spring Reverb
                {"slot": 2, "type": 26, "mix": 0.52, "active": True},  # Vintage Console EQ
                {"slot": 3, "type": 20, "mix": 0.42, "active": True}   # Transient Shaper
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.4,
            "complexity": 0.62,
            "experimentalness": 0.48,
            "versatility": 0.75
        },
        "GC_021": {
            "name": "Silk Road Echo",
            "technicalHint": "Convolution Reverb + Comb Resonator + Frequency Shifter",
            "shortCode": "SRE",
            "category": "Spatial Design",
            "subcategory": "Cultural Spaces",
            "engines": [
                {"slot": 0, "type": 4, "mix": 0.75, "active": True},   # Convolution Reverb
                {"slot": 1, "type": 23, "mix": 0.62, "active": True},  # Comb Resonator
                {"slot": 2, "type": 19, "mix": 0.48, "active": True}   # Frequency Shifter
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.8,
            "complexity": 0.68,
            "experimentalness": 0.52,
            "versatility": 0.75
        },
        "GC_022": {
            "name": "Neural Bloom",
            "technicalHint": "Feedback Network + Intelligent Harmonizer + Spectral Freeze",
            "shortCode": "NRB",
            "category": "Experimental Laboratory",
            "subcategory": "Neural Processing",
            "engines": [
                {"slot": 0, "type": 48, "mix": 0.78, "active": True},  # Feedback Network
                {"slot": 1, "type": 41, "mix": 0.65, "active": True},  # Intelligent Harmonizer
                {"slot": 2, "type": 38, "mix": 0.52, "active": True}   # Spectral Freeze
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 4.5,
            "complexity": 0.82,
            "experimentalness": 0.88,
            "versatility": 0.62
        },
        "GC_023": {
            "name": "Tidal Force",
            "technicalHint": "Wave Folder + Phased Vocoder + Mastering Limiter",
            "shortCode": "TDF",
            "category": "Motion & Modulation",
            "subcategory": "Oceanic Movement",
            "engines": [
                {"slot": 0, "type": 31, "mix": 0.72, "active": True},  # Wave Folder
                {"slot": 1, "type": 44, "mix": 0.68, "active": True},  # Phased Vocoder
                {"slot": 2, "type": 49, "mix": 0.45, "active": True}   # Mastering Limiter
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 3.8,
            "complexity": 0.72,
            "experimentalness": 0.65,
            "versatility": 0.68
        },
        "GC_024": {
            "name": "Amber Preservation",
            "technicalHint": "Vintage Tube + Tape Echo + Opto Compressor + Parametric EQ",
            "shortCode": "AMB",
            "category": "Studio Essentials",
            "subcategory": "Vintage Preservation",
            "engines": [
                {"slot": 0, "type": 0, "mix": 0.78, "active": True},   # Vintage Tube
                {"slot": 1, "type": 1, "mix": 0.65, "active": True},   # Tape Echo
                {"slot": 2, "type": 6, "mix": 0.52, "active": True},   # Opto Compressor
                {"slot": 3, "type": 27, "mix": 0.42, "active": True}   # Parametric EQ
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.3,
            "complexity": 0.58,
            "experimentalness": 0.25,
            "versatility": 0.85
        },
        "GC_025": {
            "name": "Zero Point Field",
            "technicalHint": "Noise Gate + Granular Cloud + Spectral Freeze",
            "shortCode": "ZPF",
            "category": "Experimental Laboratory",
            "subcategory": "Quantum Audio",
            "engines": [
                {"slot": 0, "type": 46, "mix": 0.82, "active": True},  # Noise Gate
                {"slot": 1, "type": 16, "mix": 0.72, "active": True},  # Granular Cloud
                {"slot": 2, "type": 38, "mix": 0.65, "active": True}   # Spectral Freeze
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 3.5,
            "complexity": 0.78,
            "experimentalness": 0.85,
            "versatility": 0.55
        },
        "GC_026": {
            "name": "Mercury Rising",
            "technicalHint": "Pitch Shifter + Frequency Shifter + Shimmer Reverb",
            "shortCode": "MRC",
            "category": "Motion & Modulation",
            "subcategory": "Ascending Movement",
            "engines": [
                {"slot": 0, "type": 14, "mix": 0.65, "active": True},  # Pitch Shifter
                {"slot": 1, "type": 19, "mix": 0.48, "active": True},  # Frequency Shifter
                {"slot": 2, "type": 2, "mix": 0.72, "active": True},   # Shimmer Reverb
                {"slot": 3, "type": 32, "mix": 0.35, "active": True}   # Harmonic Exciter
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 4.8,
            "complexity": 0.72,
            "experimentalness": 0.68,
            "versatility": 0.74
        },
        "GC_027": {
            "name": "Crystalline Matrix",
            "technicalHint": "Comb Resonator + Ring Mod + Granular Cloud",
            "shortCode": "CRM",
            "category": "Character & Color",
            "subcategory": "Harmonic Structures",
            "engines": [
                {"slot": 0, "type": 23, "mix": 0.71, "active": True},  # Comb Resonator
                {"slot": 1, "type": 24, "mix": 0.52, "active": True},  # Ring Modulator
                {"slot": 2, "type": 16, "mix": 0.58, "active": True},  # Granular Cloud
                {"slot": 3, "type": 38, "mix": 0.42, "active": True}   # Spectral Freeze
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 5.2,
            "complexity": 0.85,
            "experimentalness": 0.75,
            "versatility": 0.62
        },
        "GC_028": {
            "name": "Velvet Shadows",
            "technicalHint": "Opto Compressor + Analog Phaser + Dark Reverb",
            "shortCode": "VLS",
            "category": "Studio Essentials",
            "subcategory": "Vocal Treatment",
            "engines": [
                {"slot": 0, "type": 6, "mix": 0.78, "active": True},   # Opto Compressor
                {"slot": 1, "type": 12, "mix": 0.52, "active": True},  # Analog Phaser
                {"slot": 2, "type": 3, "mix": 0.65, "active": True},   # Plate Reverb
                {"slot": 3, "type": 0, "mix": 0.38, "active": True}    # Vintage Tube
            ],
            "cpuTier": "LIGHT",
            "actualCpuPercent": 2.8,
            "complexity": 0.58,
            "experimentalness": 0.32,
            "versatility": 0.78
        },
        "GC_029": {
            "name": "Plasma Field",
            "technicalHint": "Chaos Generator + Wave Folder + Frequency Shifter",
            "shortCode": "PLF",
            "category": "Experimental Laboratory",
            "subcategory": "Extreme Processing",
            "engines": [
                {"slot": 0, "type": 40, "mix": 0.82, "active": True},  # Chaos Generator
                {"slot": 1, "type": 31, "mix": 0.68, "active": True},  # Wave Folder
                {"slot": 2, "type": 19, "mix": 0.75, "active": True},  # Frequency Shifter
                {"slot": 3, "type": 24, "mix": 0.58, "active": True},  # Ring Modulator
                {"slot": 4, "type": 34, "mix": 0.45, "active": True}   # Multiband Saturator
            ],
            "cpuTier": "HEAVY",
            "actualCpuPercent": 9.8,
            "complexity": 0.95,
            "experimentalness": 0.98,
            "versatility": 0.35
        },
        "GC_030": {
            "name": "Ancient Echoes",
            "technicalHint": "Magnetic Drum Echo + Convolution Reverb + Tape Echo",
            "shortCode": "ANC",
            "category": "Spatial Design",
            "subcategory": "Historic Spaces",
            "engines": [
                {"slot": 0, "type": 8, "mix": 0.72, "active": True},   # Magnetic Drum Echo
                {"slot": 1, "type": 4, "mix": 0.85, "active": True},   # Convolution Reverb
                {"slot": 2, "type": 1, "mix": 0.62, "active": True},   # Tape Echo
                {"slot": 3, "type": 26, "mix": 0.48, "active": True}   # Vintage Console EQ
            ],
            "cpuTier": "MEDIUM",
            "actualCpuPercent": 5.5,
            "complexity": 0.78,
            "experimentalness": 0.62,
            "versatility": 0.85
        }
    }
    
    # Get the preset data
    if preset_id not in presets_data:
        logger.warning(f"Preset {preset_id} not found in manual data")
        return None
    
    data = presets_data[preset_id]
    
    # Build complete preset structure
    preset = {
        "id": preset_id,
        "name": data["name"],
        "technicalHint": data["technicalHint"],
        "shortCode": data["shortCode"],
        "category": data["category"],
        "subcategory": data["subcategory"],
        "version": 1,
        "isVariation": False,
        "parentId": "",
        "engines": data["engines"],
        "sonicProfile": {
            "brightness": 0.5,
            "density": 0.5,
            "movement": 0.5,
            "space": 0.5,
            "aggression": 0.5,
            "vintage": 0.5
        },
        "emotionalProfile": {
            "energy": 0.5,
            "mood": 0.5,
            "tension": 0.5,
            "organic": 0.5,
            "nostalgia": 0.5
        },
        "sourceAffinity": {
            "vocals": 0.5,
            "guitar": 0.5,
            "drums": 0.5,
            "synth": 0.5,
            "mix": 0.5
        },
        "cpuTier": data["cpuTier"],
        "actualCpuPercent": data["actualCpuPercent"],
        "latencySamples": 256.0,
        "realtimeSafe": True,
        "optimalTempo": 0.0,
        "musicalKey": "",
        "genres": [],
        "signature": "B. Andersson",
        "creationDate": "2025-01-01T00:00:00Z",
        "popularityScore": 0,
        "qualityScore": 95.0,
        "complexity": data["complexity"],
        "experimentalness": data["experimentalness"],
        "versatility": data["versatility"],
        "keywords": [],
        "antiFeatures": [],
        "userPrompts": [],
        "bestFor": "",
        "avoidFor": ""
    }
    
    # Add specific profiles based on preset characteristics
    if "Vintage" in data["name"] or "vintage" in data["subcategory"].lower():
        preset["sonicProfile"]["vintage"] = 0.8
    
    if "Experimental" in data["category"]:
        preset["experimentalness"] = max(0.7, data["experimentalness"])
    
    if "Spatial" in data["category"]:
        preset["sonicProfile"]["space"] = 0.8
    
    if "Heavy" in data["subcategory"] or "Extreme" in data["subcategory"]:
        preset["sonicProfile"]["aggression"] = 0.8
    
    return preset

def export_all_presets():
    """Export all 30 presets to JSON files"""
    output_dir = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus")
    presets_dir = output_dir / "presets"
    presets_dir.mkdir(parents=True, exist_ok=True)
    
    preset_ids = [f"GC_{i:03d}" for i in range(1, 31)]
    
    exported_count = 0
    all_presets = []
    
    for preset_id in preset_ids:
        preset = create_preset_json(preset_id, {})
        if preset:
            # Save individual file
            output_file = presets_dir / f"{preset_id}.json"
            with open(output_file, 'w') as f:
                json.dump(preset, f, indent=2)
            
            logger.info(f"Exported: {preset_id} - {preset['name']}")
            exported_count += 1
            all_presets.append(preset)
    
    # Create metadata
    metadata = {
        "version": "1.0",
        "exportDate": "2025-01-01T00:00:00Z",
        "presetCount": exported_count,
        "categories": [
            {"name": "Studio Essentials", "count": len([p for p in all_presets if p["category"] == "Studio Essentials"])},
            {"name": "Spatial Design", "count": len([p for p in all_presets if p["category"] == "Spatial Design"])},
            {"name": "Character & Color", "count": len([p for p in all_presets if p["category"] == "Character & Color"])},
            {"name": "Motion & Modulation", "count": len([p for p in all_presets if p["category"] == "Motion & Modulation"])},
            {"name": "Experimental Laboratory", "count": len([p for p in all_presets if p["category"] == "Experimental Laboratory"])}
        ]
    }
    
    metadata_file = output_dir / "corpus_metadata.json"
    with open(metadata_file, 'w') as f:
        json.dump(metadata, f, indent=2)
    
    logger.info(f"\nExport complete!")
    logger.info(f"Exported {exported_count} presets to {output_dir}")
    logger.info(f"Ready for FAISS indexing!")
    
    return exported_count

if __name__ == "__main__":
    export_all_presets()