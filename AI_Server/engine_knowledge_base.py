#!/usr/bin/env python3
"""
Comprehensive Engine Knowledge Base for Trinity Pipeline
This is the single source of truth for all engine capabilities, parameters, and relationships.
"""

from typing import Dict, List, Any, Optional, Tuple
from enum import Enum

class EngineCategory(Enum):
    """Engine categories for signal chain ordering"""
    DYNAMICS = 1      # Compressors, Gates, Limiters (process first)
    EQ = 2           # EQs and Filters (shape tone early)
    DISTORTION = 3   # Distortion, Saturation, Fuzz (add harmonics)
    MODULATION = 4   # Chorus, Phaser, Flanger (add movement)
    PITCH = 5        # Pitch shifters, Harmonizers (pitch effects)
    DELAY = 6        # Delays and Echoes (time-based)
    REVERB = 7       # Reverbs (ambience last)
    SPATIAL = 8      # Stereo wideners (final touches)
    UTILITY = 9      # Gain, Phase (anywhere as needed)
    SPECTRAL = 10    # Spectral effects (special processing)

# Original semantic knowledge
ENGINE_KNOWLEDGE = {
    0: {
        "name": "None",
        "function": "Bypass/passthrough with no processing",
        "character": "Transparent - no coloration",
        "use_cases": ["Empty slot", "Disable processing"],
        "technical": "Direct signal pass with no DSP"
    },
    
    # DYNAMICS & COMPRESSION (1-6)
    1: {
        "name": "Vintage Opto Compressor",
        "function": "Smooth optical compression inspired by LA-2A",
        "character": "Warm, musical, slow attack, natural sustain",
        "use_cases": ["Vocals", "Bass", "Mix bus glue"],
        "technical": "Models light-dependent resistor behavior with frequency-dependent attack"
    },
    2: {
        "name": "Classic Compressor", 
        "function": "VCA-style compression for precise dynamic control",
        "character": "Clean, punchy, fast response, transparent",
        "use_cases": ["Drums", "Percussion", "Mix bus", "Mastering"],
        "technical": "Voltage-controlled amplifier with adjustable knee and ratio"
    },
    3: {
        "name": "Transient Shaper",
        "function": "Enhances or softens attack transients",
        "character": "Punch control without traditional compression",
        "use_cases": ["Drums", "Percussion", "Plucked instruments"],
        "technical": "Envelope follower with separate attack/sustain processing"
    },
    4: {
        "name": "Noise Gate",
        "function": "Removes noise and bleed between notes",
        "character": "Clean gating with adjustable range",
        "use_cases": ["Remove amp noise", "Clean up drums", "Tighten bass"],
        "technical": "Threshold-based gate with hysteresis and look-ahead"
    },
    5: {
        "name": "Mastering Limiter",
        "function": "Transparent peak limiting for loudness maximization",
        "character": "Clean, transparent, no pumping",
        "use_cases": ["Final mastering", "Mix bus protection", "Broadcast limiting"],
        "technical": "Look-ahead peak detection with intelligent release"
    },
    6: {
        "name": "Dynamic EQ",
        "function": "Frequency-selective compression/expansion",
        "character": "Surgical dynamic control of problem frequencies",
        "use_cases": ["De-essing", "Resonance control", "Dynamic brightening"],
        "technical": "Multiband dynamics with parametric EQ targeting"
    },
    
    # FILTERS & EQ (7-14)  
    7: {
        "name": "Parametric EQ",
        "function": "Precise frequency shaping with 3 bands",
        "character": "Clean, surgical, transparent",
        "use_cases": ["Mixing", "Corrective EQ", "Tone shaping"],
        "technical": "Bell and shelf filters with adjustable Q"
    },
    8: {
        "name": "Vintage Console EQ",
        "function": "Musical EQ modeled after classic consoles",
        "character": "Warm, musical, broad strokes",
        "use_cases": ["Mix bus", "Drums", "Full mixes"],
        "technical": "Models Neve/SSL-style inductor EQ curves"
    },
    9: {
        "name": "Ladder Filter",
        "function": "Moog-style resonant low-pass filter",
        "character": "Warm, fat, self-oscillating resonance",
        "use_cases": ["Synth bass", "Filter sweeps", "Acid sounds"],
        "technical": "4-pole cascade with nonlinear feedback"
    },
    10: {
        "name": "State Variable Filter",
        "function": "Versatile filter with multiple modes",
        "character": "Clean, precise, versatile",
        "use_cases": ["Sound design", "DJ filtering", "Creative processing"],
        "technical": "Simultaneous LP/BP/HP outputs with morphing"
    },
    11: {
        "name": "Formant Filter",
        "function": "Vowel-like resonances for vocal effects",
        "character": "Vocal, talking, organic",
        "use_cases": ["Talk box effects", "Vocal processing", "Synths"],
        "technical": "Multiple bandpass filters modeling vocal tract"
    },
    12: {
        "name": "Envelope Filter",
        "function": "Auto-wah following input dynamics",
        "character": "Funky, responsive, expressive",
        "use_cases": ["Funk guitar", "Bass", "Synth leads"],
        "technical": "Envelope follower controlling filter cutoff"
    },
    13: {
        "name": "Comb Resonator",
        "function": "Creates pitched resonances and metallic tones",
        "character": "Metallic, pitched, resonant",
        "use_cases": ["Special effects", "Karplus-Strong synthesis", "Drones"],
        "technical": "Delay line with feedback creating comb filtering"
    },
    14: {
        "name": "Vocal Formant Filter",
        "function": "Advanced formant shaping for vocal character",
        "character": "Vocal transformation, gender shifting",
        "use_cases": ["Vocal effects", "Harmonizer support", "Choir effects"],
        "technical": "Formant-preserving pitch shift technology"
    },
    
    # DISTORTION & SATURATION (15-22)
    15: {
        "name": "Vintage Tube Preamp",
        "function": "Warm tube saturation and harmonic enhancement",
        "character": "Warm, smooth, musical, even harmonics",
        "use_cases": ["Warming digital sources", "Drums", "Mix bus"],
        "technical": "12AX7 tube stage modeling with adjustable bias"
    },
    16: {
        "name": "Wave Folder",
        "function": "Folds waveform back on itself for complex harmonics",
        "character": "Aggressive, metallic, bell-like overtones",
        "use_cases": ["Extreme distortion", "Sound design", "West Coast synthesis"],
        "technical": "Waveform folding creates reflected harmonics"
    },
    17: {
        "name": "Harmonic Exciter",
        "function": "Adds brightness and presence through harmonic generation",
        "character": "Bright, airy, present, clarity",
        "use_cases": ["Vocal air", "Mix brightness", "Presence boost"],
        "technical": "High-frequency harmonic synthesis"
    },
    18: {
        "name": "Bit Crusher",
        "function": "Digital degradation and lo-fi effects",
        "character": "Digital, crunchy, retro, 8-bit",
        "use_cases": ["Lo-fi effects", "Retro gaming", "Glitch"],
        "technical": "Bit depth and sample rate reduction"
    },
    19: {
        "name": "Multiband Saturator",
        "function": "Frequency-selective saturation across 3 bands",
        "character": "Controlled warmth, frequency-specific drive",
        "use_cases": ["Mastering", "Mix bus", "Drum bus"],
        "technical": "3-band split with independent saturation"
    },
    20: {
        "name": "Muff Fuzz",
        "function": "Big Muff-style fuzz distortion",
        "character": "Thick, sustained, creamy fuzz",
        "use_cases": ["Lead guitar", "Bass", "Synths"],
        "technical": "Cascaded clipping stages with tone control"
    },
    21: {
        "name": "Rodent Distortion",
        "function": "RAT-style distortion pedal",
        "character": "Aggressive, cutting, tight low end",
        "use_cases": ["Rock guitar", "Punk", "Industrial"],
        "technical": "Op-amp clipping with aggressive filtering"
    },
    22: {
        "name": "K-Style Overdrive",
        "function": "Klon-style transparent overdrive",
        "character": "Transparent, mid-forward, amp-like",
        "use_cases": ["Blues guitar", "Boost", "Amp pushing"],
        "technical": "Clean blend with mid-focused overdrive"
    },
    
    # MODULATION EFFECTS (23-33)
    23: {
        "name": "Digital Chorus",
        "function": "Classic stereo chorus for width and movement",
        "character": "Lush, wide, shimmering",
        "use_cases": ["80s sounds", "Pad widening", "Clean guitar"],
        "technical": "Dual delay lines with LFO modulation"
    },
    24: {
        "name": "Resonant Chorus",
        "function": "Chorus with resonant feedback for unique textures",
        "character": "Metallic, resonant, unique",
        "use_cases": ["Experimental", "Ambient", "Sound design"],
        "technical": "Chorus with resonant comb filtering"
    },
    25: {
        "name": "Analog Phaser",
        "function": "Classic phase shifting for swooshing effects",
        "character": "Swoosh, vintage, psychedelic",
        "use_cases": ["70s sounds", "Guitar", "Rhodes"],
        "technical": "All-pass filter cascade with LFO"
    },
    26: {
        "name": "Ring Modulator",
        "function": "Creates metallic, bell-like tones",
        "character": "Robotic, metallic, atonal",
        "use_cases": ["Sci-fi effects", "Experimental", "Daleks"],
        "technical": "Amplitude modulation creating sum/difference frequencies"
    },
    27: {
        "name": "Frequency Shifter",
        "function": "Shifts all frequencies by fixed amount",
        "character": "Dissonant, ethereal, otherworldly",
        "use_cases": ["Psychedelic effects", "Feedback control", "Sound design"],
        "technical": "Hilbert transform-based frequency translation"
    },
    28: {
        "name": "Harmonic Tremolo",
        "function": "Frequency-split tremolo for complex modulation",
        "character": "Complex, vintage, Fender-like",
        "use_cases": ["Vintage amp tremolo", "Guitar", "Organ"],
        "technical": "Crossover-split amplitude modulation"
    },
    29: {
        "name": "Classic Tremolo",
        "function": "Simple amplitude modulation",
        "character": "Vintage, simple, rhythmic",
        "use_cases": ["Vintage sounds", "Rhythmic effects", "Surf guitar"],
        "technical": "LFO controlling amplitude"
    },
    30: {
        "name": "Rotary Speaker",
        "function": "Leslie rotating speaker simulation",
        "character": "Swirling, doppler, organic",
        "use_cases": ["Organ", "Guitar", "Psychedelic"],
        "technical": "Doppler shift and amplitude modulation"
    },
    31: {
        "name": "Pitch Shifter",
        "function": "Real-time pitch shifting up or down",
        "character": "Harmonizing, octaves, pitch correction",
        "use_cases": ["Harmonies", "Octave effects", "Formant shift"],
        "technical": "Phase vocoder or granular pitch shifting"
    },
    32: {
        "name": "Detune Doubler",
        "function": "Creates width through micro-pitch shifting",
        "character": "Wide, thick, chorused",
        "use_cases": ["Vocal thickening", "Guitar doubling", "Widening"],
        "technical": "Dual pitch shifters with micro-detuning"
    },
    33: {
        "name": "Intelligent Harmonizer",
        "function": "Scale-aware harmony generation",
        "character": "Musical harmonies, intelligent intervals",
        "use_cases": ["Vocal harmonies", "Lead doubling", "Chord generation"],
        "technical": "Pitch detection with scale-quantized shifting"
    },
    
    # REVERB & DELAY (34-43)
    34: {
        "name": "Tape Echo",
        "function": "Vintage tape delay with wow/flutter",
        "character": "Warm, degraded, vintage, analog",
        "use_cases": ["Dub", "Vintage delays", "Ambient"],
        "technical": "Tape saturation, wow/flutter modulation"
    },
    35: {
        "name": "Digital Delay",
        "function": "Clean, precise digital delay",
        "character": "Clean, precise, modern",
        "use_cases": ["Rhythmic delays", "Tempo-synced effects", "Clarity"],
        "technical": "Digital delay line with filtering"
    },
    36: {
        "name": "Magnetic Drum Echo",
        "function": "Rare drum-based echo unit",
        "character": "Unique, vintage, mechanical",
        "use_cases": ["Vintage effects", "Dub", "Experimental"],
        "technical": "Models Echorec-style magnetic drum"
    },
    37: {
        "name": "Bucket Brigade Delay",
        "function": "Analog BBD delay with characteristic darkness",
        "character": "Dark, analog, degraded, musical",
        "use_cases": ["Analog delays", "Vintage sounds", "Ambience"],
        "technical": "BBD chip emulation with clock noise"
    },
    38: {
        "name": "Buffer Repeat",
        "function": "Glitchy buffer sampling and looping",
        "character": "Glitchy, stuttering, rhythmic",
        "use_cases": ["Glitch effects", "Beat repeat", "IDM"],
        "technical": "Buffer capture with pitch/reverse control"
    },
    39: {
        "name": "Plate Reverb",
        "function": "Classic studio plate reverb",
        "character": "Bright, dense, vintage studio",
        "use_cases": ["Vocals", "Drums", "Classic mixing"],
        "technical": "Physical modeling of plate vibrations"
    },
    40: {
        "name": "Spring Reverb",
        "function": "Guitar amp spring reverb",
        "character": "Boingy, metallic, surf",
        "use_cases": ["Guitar", "Surf", "Vintage effects"],
        "technical": "Spring tension modeling with characteristic 'boing'"
    },
    41: {
        "name": "Convolution Reverb",
        "function": "Impulse response-based real space simulation",
        "character": "Realistic, authentic spaces",
        "use_cases": ["Film scoring", "Classical", "Realistic spaces"],
        "technical": "Convolution with real space impulse responses"
    },
    42: {
        "name": "Shimmer Reverb",
        "function": "Reverb with pitched overtones",
        "character": "Ethereal, angelic, ambient",
        "use_cases": ["Ambient", "Pads", "Cinematic"],
        "technical": "Reverb with octave-up pitch shifting in feedback"
    },
    43: {
        "name": "Gated Reverb",
        "function": "80s-style gated reverb",
        "character": "Big, punchy, abrupt cutoff",
        "use_cases": ["80s drums", "Phil Collins snare", "Big impacts"],
        "technical": "Reverb with noise gate on tail"
    },
    
    # SPATIAL & SPECIAL EFFECTS (44-52)
    44: {
        "name": "Stereo Widener",
        "function": "Enhances stereo width",
        "character": "Wide, spacious, open",
        "use_cases": ["Mix widening", "Stereo enhancement", "Mastering"],
        "technical": "M/S processing with Haas effect"
    },
    45: {
        "name": "Stereo Imager",
        "function": "Multiband stereo width control",
        "character": "Controlled width, mono-compatible",
        "use_cases": ["Mastering", "Mix placement", "Width control"],
        "technical": "Frequency-dependent M/S processing"
    },
    46: {
        "name": "Dimension Expander",
        "function": "Creates depth and dimension",
        "character": "3D, spacious, depth",
        "use_cases": ["Mix depth", "3D placement", "Ambience"],
        "technical": "Micro-delays and phase manipulation"
    },
    47: {
        "name": "Spectral Freeze",
        "function": "Freezes spectral content for drones",
        "character": "Frozen, sustained, ethereal",
        "use_cases": ["Ambient", "Transitions", "Sound design"],
        "technical": "FFT-based spectral hold"
    },
    48: {
        "name": "Spectral Gate",
        "function": "Frequency-selective gating",
        "character": "Filtered, gated, rhythmic",
        "use_cases": ["Trance gates", "Rhythmic filtering", "Creative gating"],
        "technical": "FFT-based amplitude gating"
    },
    49: {
        "name": "Phased Vocoder",
        "function": "Complex spectral processing",
        "character": "Robotic, vocoded, spectral",
        "use_cases": ["Vocoding", "Robot voices", "Spectral effects"],
        "technical": "Phase vocoder for time/pitch manipulation"
    },
    50: {
        "name": "Granular Cloud",
        "function": "Granular synthesis for textures",
        "character": "Textural, cloudy, evolving",
        "use_cases": ["Ambient textures", "Sound design", "Experimental"],
        "technical": "Granular resynthesis with position/pitch control"
    },
    51: {
        "name": "Chaos Generator",
        "function": "Controlled random modulation",
        "character": "Chaotic, random, evolving",
        "use_cases": ["Generative music", "Modulation source", "Experimental"],
        "technical": "Lorenz attractor-based chaos"
    },
    52: {
        "name": "Feedback Network",
        "function": "Complex feedback routing",
        "character": "Resonant, complex, self-oscillating",
        "use_cases": ["Experimental", "Feedback effects", "Drones"],
        "technical": "Multi-tap feedback delay network"
    },
    
    # UTILITY (53-56)
    53: {
        "name": "Mid-Side Processor",
        "function": "Separate processing of mid/side signals",
        "character": "Width control, surgical",
        "use_cases": ["Mastering", "Width adjustment", "Problem solving"],
        "technical": "M/S encoding/decoding with independent processing"
    },
    54: {
        "name": "Gain Utility",
        "function": "Simple gain and pan control",
        "character": "Transparent, utility",
        "use_cases": ["Level matching", "Gain staging", "Basic control"],
        "technical": "Simple amplitude and pan adjustment"
    },
    55: {
        "name": "Mono Maker",
        "function": "Frequency-selective mono summing",
        "character": "Mono-compatible, controlled",
        "use_cases": ["Bass mono", "Vinyl mastering", "Mono compatibility"],
        "technical": "Crossover-based stereo-to-mono below frequency"
    },
    56: {
        "name": "Phase Align",
        "function": "Phase correction and alignment",
        "character": "Phase-coherent, corrective",
        "use_cases": ["Multi-mic alignment", "Phase issues", "Comb filtering fix"],
        "technical": "All-pass filtering and micro-delay"
    }
}

def get_engine_capability(engine_id: int) -> dict:
    """Get complete capability information for an engine"""
    return ENGINE_KNOWLEDGE.get(engine_id, {
        "name": "Unknown",
        "function": "Unknown engine",
        "character": "Unknown characteristics",
        "use_cases": [],
        "technical": "No information available"
    })

def find_engines_for_use_case(use_case: str) -> list:
    """Find engines suitable for a specific use case"""
    matches = []
    use_case_lower = use_case.lower()
    
    for engine_id, info in ENGINE_KNOWLEDGE.items():
        # Check if use case matches
        for uc in info.get("use_cases", []):
            if use_case_lower in uc.lower():
                matches.append((engine_id, info["name"], uc))
        
        # Also check function and character
        if use_case_lower in info.get("function", "").lower():
            matches.append((engine_id, info["name"], "function match"))
        if use_case_lower in info.get("character", "").lower():
            matches.append((engine_id, info["name"], "character match"))
    
    return matches

def describe_signal_chain(engine_ids: list) -> str:
    """Describe what a chain of engines will do to the signal"""
    descriptions = []
    
    for engine_id in engine_ids:
        if engine_id == 0:
            continue
        info = get_engine_capability(engine_id)
        descriptions.append(f"{info['name']}: {info['function']}")
    
    return " → ".join(descriptions)

if __name__ == "__main__":
    # Test the knowledge base
    print("🧠 ENGINE KNOWLEDGE BASE TEST")
    print("=" * 60)
    
    # Test specific engines
    test_engines = [15, 16, 17, 18, 39, 42]
    for engine_id in test_engines:
        info = get_engine_capability(engine_id)
        print(f"\nEngine {engine_id}: {info['name']}")
        print(f"  Function: {info['function']}")
        print(f"  Character: {info['character']}")
    
    # Test use case search
    print("\n\nUSE CASE: 'Vocals'")
    print("-" * 30)
    matches = find_engines_for_use_case("vocals")
    for engine_id, name, context in matches[:5]:
        print(f"  {name} (ID {engine_id}): {context}")
    
    # Test signal chain description
    print("\n\nSIGNAL CHAIN: [15, 39, 42]")
    print("-" * 30)
    chain_desc = describe_signal_chain([15, 39, 42])
    print(f"  {chain_desc}")