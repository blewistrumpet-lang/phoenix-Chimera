#!/usr/bin/env python3
"""
Fix the corpus using REAL musical default parameters from UnifiedDefaultParameters.cpp
"""

import json
import random
import numpy as np
from pathlib import Path
from typing import Dict, Any, List, Tuple
from engine_mapping_authoritative import ENGINE_NAMES

class MusicalCorpusFixer:
    def __init__(self):
        self.corpus_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus_v3")
        self.preset_file = self.corpus_path / "faiss_index" / "presets_clean.json"
        self.backup_file = self.corpus_path / "faiss_index" / "presets_clean_backup.json"
        
        # REAL MUSICAL DEFAULT PARAMETERS FROM UnifiedDefaultParameters.cpp
        # These are the actual defaults that make musical sense for each engine
        self.engine_defaults = {
            # Dynamics & Compression (1-6)
            1: {  # Vintage Opto Compressor
                0: ("Input Gain", 0.5),
                1: ("Peak Reduction", 0.3),
                2: ("HF Emphasis", 0.0),
                3: ("Output Gain", 0.5),
                4: ("Mix", 1.0),
                5: ("Knee", 0.7),
                6: ("Tube Harmonics", 0.2),
                7: ("Stereo Link", 1.0)
            },
            2: {  # Classic VCA Compressor
                0: ("Threshold", 0.4),
                1: ("Ratio", 0.5),
                2: ("Attack", 0.2),
                3: ("Release", 0.4),
                4: ("Knee", 0.0),
                5: ("Makeup Gain", 0.5),
                6: ("Mix", 1.0)
            },
            3: {  # Transient Shaper
                0: ("Attack", 0.5),
                1: ("Sustain", 0.5),
                2: ("Sensitivity", 0.3),
                3: ("Output", 0.5)
            },
            4: {  # Noise Gate
                0: ("Threshold", 0.3),
                1: ("Attack", 0.1),
                2: ("Hold", 0.3),
                3: ("Release", 0.4),
                4: ("Range", 0.8)
            },
            5: {  # Mastering Limiter
                0: ("Threshold", 0.9),
                1: ("Release", 0.2),
                2: ("Knee", 0.0),
                3: ("Lookahead", 0.3)
            },
            6: {  # Dynamic EQ
                0: ("Frequency", 0.5),
                1: ("Threshold", 0.5),
                2: ("Ratio", 0.3),
                3: ("Attack", 0.2),
                4: ("Release", 0.4),
                5: ("Gain", 0.5),
                6: ("Mix", 1.0),
                7: ("Mode", 0.0)
            },
            
            # Filters & EQ (7-14)
            7: {  # Parametric EQ
                0: ("Low Frequency", 0.2),
                1: ("Low Gain", 0.5),
                2: ("Low Q", 0.5),
                3: ("Mid Frequency", 0.5),
                4: ("Mid Gain", 0.5),
                5: ("Mid Q", 0.5),
                6: ("High Frequency", 0.8),
                7: ("High Gain", 0.5),
                8: ("High Q", 0.5)
            },
            8: {  # Vintage Console EQ
                0: ("Low Gain", 0.5),
                1: ("Low-Mid Gain", 0.5),
                2: ("High-Mid Gain", 0.5),
                3: ("High Gain", 0.5),
                4: ("Drive", 0.0)
            },
            9: {  # Ladder Filter
                0: ("Cutoff", 0.6),
                1: ("Resonance", 0.3),
                2: ("Drive", 0.2),
                3: ("Filter Type", 0.0),
                4: ("Asymmetry", 0.0),
                5: ("Vintage Mode", 0.0),
                6: ("Mix", 1.0)
            },
            10: {  # State Variable Filter
                0: ("Cutoff", 0.5),
                1: ("Resonance", 0.4),
                2: ("Mode", 0.0),
                3: ("Key Follow", 0.0),
                4: ("Mix", 1.0)
            },
            11: {  # Comb Resonator
                0: ("Frequency", 0.5),
                1: ("Resonance", 0.4),
                2: ("Feedback", 0.3),
                3: ("Mix", 0.5)
            },
            12: {  # Formant Filter
                0: ("Formant", 0.5),
                1: ("Resonance", 0.4),
                2: ("Drive", 0.3),
                3: ("Mix", 1.0)
            },
            13: {  # Envelope Filter
                0: ("Sensitivity", 0.5),
                1: ("Attack", 0.1),
                2: ("Release", 0.3),
                3: ("Range", 0.5),
                4: ("Mix", 1.0)
            },
            14: {  # Vocal Formant
                0: ("Vowel Position", 0.3),
                1: ("Formant Intensity", 0.4),
                2: ("Gender", 0.5),
                3: ("Mix", 1.0)
            },
            
            # Distortion & Saturation (15-22)
            15: {  # Vintage Tube Preamp
                0: ("Input Gain", 0.5),
                1: ("Drive", 0.3),
                2: ("Bias", 0.5),
                3: ("Bass", 0.5),
                4: ("Mid", 0.5),
                5: ("Treble", 0.5),
                6: ("Presence", 0.5),
                7: ("Output Gain", 0.5),
                8: ("Tube Type", 0.0),
                9: ("Mix", 1.0)
            },
            16: {  # Wave Folder
                0: ("Drive", 0.4),
                1: ("Fold Amount", 0.3),
                2: ("Symmetry", 0.5),
                3: ("Output", 0.5),
                4: ("Mix", 0.7)
            },
            17: {  # Harmonic Exciter
                0: ("Harmonics", 0.2),
                1: ("Frequency", 0.7),
                2: ("Mix", 0.2)
            },
            18: {  # Bit Crusher
                0: ("Bits", 0.3),  # 8-bit
                1: ("Downsample", 0.0),
                2: ("Mix", 0.7)
            },
            19: {  # Multiband Saturator
                0: ("Low Drive", 0.3),
                1: ("Mid Drive", 0.3),
                2: ("High Drive", 0.2),
                3: ("Crossover Low", 0.3),
                4: ("Crossover High", 0.7),
                5: ("Mix", 0.7)
            },
            20: {  # Muff Fuzz
                0: ("Sustain", 0.3),
                1: ("Tone", 0.5),
                2: ("Volume", 0.5),
                3: ("Gate", 0.0),
                4: ("Mids", 0.0),
                5: ("Variant", 0.0),
                6: ("Mix", 1.0)
            },
            21: {  # Rodent Distortion
                0: ("Gain", 0.5),
                1: ("Filter", 0.4),
                2: ("Clipping", 0.3),
                3: ("Tone", 0.5),
                4: ("Output", 0.5),
                5: ("Mix", 1.0),
                6: ("Mode", 0.0),
                7: ("Presence", 0.3)
            },
            22: {  # K-Style Overdrive
                0: ("Drive", 0.3),
                1: ("Tone", 0.5),
                2: ("Level", 0.5),
                3: ("Mix", 1.0)
            },
            
            # Modulation Effects (23-33)
            23: {  # Digital Chorus
                0: ("Rate", 0.2),
                1: ("Depth", 0.3),
                2: ("Mix", 0.3),
                3: ("Feedback", 0.0)
            },
            24: {  # Resonant Chorus
                0: ("Rate", 0.2),
                1: ("Depth", 0.3),
                2: ("Resonance", 0.3),
                3: ("Mix", 0.3)
            },
            25: {  # Analog Phaser
                0: ("Rate", 0.4),
                1: ("Depth", 0.5),
                2: ("Feedback", 0.3),
                3: ("Stages", 0.5),
                4: ("Mix", 1.0)
            },
            26: {  # Ring Modulator
                0: ("Frequency", 0.3),
                1: ("Depth", 0.4),
                2: ("Shape", 0.0),
                3: ("Mix", 0.5)
            },
            27: {  # Frequency Shifter
                0: ("Shift Amount", 0.1),
                1: ("Fine Tune", 0.5),
                2: ("Feedback", 0.4),
                3: ("Mix", 0.5)
            },
            28: {  # Harmonic Tremolo
                0: ("Rate", 0.25),
                1: ("Depth", 0.5),
                2: ("Harmonics", 0.4),
                3: ("Stereo Phase", 0.25)
            },
            29: {  # Classic Tremolo
                0: ("Rate", 0.25),
                1: ("Depth", 0.5),
                2: ("Shape", 0.0),
                3: ("Stereo", 0.0),
                4: ("Type", 0.0),
                5: ("Symmetry", 0.5),
                6: ("Volume", 1.0),
                7: ("Mix", 1.0)
            },
            30: {  # Rotary Speaker
                0: ("Speed", 0.5),
                1: ("Acceleration", 0.3),
                2: ("Drive", 0.3),
                3: ("Mic Distance", 0.6),
                4: ("Stereo Width", 0.8),
                5: ("Mix", 1.0)
            },
            31: {  # Pitch Shifter
                0: ("Pitch", 0.5),  # No shift
                1: ("Formant", 0.333),
                2: ("Mix", 1.0),
                3: ("Window", 0.5),
                4: ("Gate", 0.0),
                5: ("Grain", 0.5),
                6: ("Feedback", 0.0),
                7: ("Width", 0.5)
            },
            32: {  # Detune Doubler
                0: ("Detune Amount", 0.3),
                1: ("Delay Time", 0.15),
                2: ("Stereo Width", 0.7),
                3: ("Thickness", 0.3),
                4: ("Mix", 0.5)
            },
            33: {  # Intelligent Harmonizer
                0: ("Interval", 0.5),
                1: ("Key", 0.0),
                2: ("Scale", 0.0),
                3: ("Voices", 0.0),
                4: ("Spread", 0.3),
                5: ("Humanize", 0.0),
                6: ("Formant", 0.0),
                7: ("Mix", 0.5)
            },
            
            # Reverb & Delay (34-43)
            34: {  # Tape Echo
                0: ("Time", 0.375),
                1: ("Feedback", 0.35),
                2: ("Wow & Flutter", 0.25),
                3: ("Saturation", 0.3),
                4: ("Mix", 0.35)
            },
            35: {  # Digital Delay
                0: ("Time", 0.4),
                1: ("Feedback", 0.3),
                2: ("Mix", 0.3),
                3: ("High Cut", 0.8)
            },
            36: {  # Magnetic Drum Echo
                0: ("Time", 0.4),
                1: ("Feedback", 0.3),
                2: ("Mix", 0.3)
            },
            37: {  # Bucket Brigade Delay
                0: ("Time", 0.5),
                1: ("Feedback", 0.3),
                2: ("Clock Noise", 0.4),
                3: ("High Cut", 0.6),
                4: ("Modulation", 0.3),
                5: ("Mix", 0.5)
            },
            38: {  # Buffer Repeat
                0: ("Size", 0.5),
                1: ("Rate", 0.5),
                2: ("Feedback", 0.3),
                3: ("Mix", 0.3)
            },
            39: {  # Plate Reverb
                0: ("Size", 0.5),
                1: ("Damping", 0.5),
                2: ("Predelay", 0.0),
                3: ("Mix", 0.3)
            },
            40: {  # Spring Reverb
                0: ("Springs", 0.5),
                1: ("Decay", 0.5),
                2: ("Tone", 0.5),
                3: ("Mix", 0.3)
            },
            41: {  # Convolution Reverb
                0: ("Size", 0.5),
                1: ("Decay", 0.6),
                2: ("Mix", 0.3)
            },
            42: {  # Shimmer Reverb
                0: ("Size", 0.5),
                1: ("Shimmer", 0.3),
                2: ("Damping", 0.5),
                3: ("Mix", 0.3)
            },
            43: {  # Gated Reverb
                0: ("Size", 0.5),
                1: ("Gate Time", 0.3),
                2: ("Damping", 0.5),
                3: ("Mix", 0.3)
            },
            
            # Spatial Effects (44-46)
            44: {  # Stereo Widener
                0: ("Width", 0.5),
                1: ("Bass Mono", 0.5),
                2: ("Mix", 1.0)
            },
            45: {  # Stereo Imager
                0: ("Width", 0.5),
                1: ("Rotation", 0.5),
                2: ("Center", 0.5),
                3: ("Mix", 1.0)
            },
            46: {  # Dimension Expander
                0: ("Dimension", 0.4),
                1: ("Depth", 0.3),
                2: ("Diffusion", 0.4),
                3: ("Mix", 0.5)
            },
            
            # Pitch Effects (47-48)
            47: {  # Formant Shifter
                0: ("Shift", 0.5),
                1: ("Mix", 0.5)
            },
            48: {  # Gender Bender
                0: ("Gender", 0.5),
                1: ("Formant", 0.5),
                2: ("Mix", 0.5)
            },
            
            # Spectral Effects (49-52)
            49: {  # Spectral Freeze
                0: ("Freeze", 0.0),
                1: ("Threshold", 0.5),
                2: ("Mix", 0.3)
            },
            50: {  # Spectral Gate
                0: ("Threshold", 0.5),
                1: ("Attack", 0.1),
                2: ("Release", 0.3),
                3: ("Mix", 1.0)
            },
            51: {  # Granular Cloud
                0: ("Grain Size", 0.3),
                1: ("Position", 0.5),
                2: ("Spread", 0.3),
                3: ("Mix", 0.5)
            },
            52: {  # Chaos Generator
                0: ("Chaos", 0.2),
                1: ("Rate", 0.3),
                2: ("Mix", 0.3)
            },
            
            # Utility (53-56)
            53: {  # Mid-Side Processor
                0: ("Mid Gain", 0.5),
                1: ("Side Gain", 0.5),
                2: ("Mix", 1.0)
            },
            54: {  # Gain Utility
                0: ("Gain", 0.5),
                1: ("Pan", 0.5),
                2: ("Mix", 1.0)
            },
            55: {  # Mono Maker
                0: ("Frequency", 0.2),
                1: ("Mix", 1.0)
            },
            56: {  # Phase Align
                0: ("Phase", 0.0),
                1: ("Mix", 1.0)
            }
        }
        
    def get_variance_for_parameter(self, engine_id: int, param_idx: int, 
                                  default_value: float, param_name: str,
                                  character: str) -> float:
        """Generate appropriate variance for a parameter based on its type and character"""
        
        # Parameters that should stay close to default
        if param_name.lower() in ["mix", "output", "volume", "gain", "makeup gain"]:
            # Small variance around default
            variance = random.uniform(-0.1, 0.1)
            return max(0.0, min(1.0, default_value + variance))
        
        # Parameters that can vary more
        if param_name.lower() in ["drive", "distortion", "sustain", "harmonics"]:
            if character == "aggressive":
                # Push higher for aggressive sounds
                variance = random.uniform(0.0, 0.3)
                return max(0.0, min(1.0, default_value + variance))
            elif character == "clean":
                # Pull lower for clean sounds
                variance = random.uniform(-0.2, 0.0)
                return max(0.0, min(1.0, default_value + variance))
        
        # Time-based parameters (delay, reverb size, etc)
        if param_name.lower() in ["time", "size", "decay", "predelay"]:
            if character == "ambient":
                # Longer times for ambient
                variance = random.uniform(0.0, 0.2)
            else:
                # Normal variation
                variance = random.uniform(-0.15, 0.15)
            return max(0.0, min(1.0, default_value + variance))
        
        # Rate parameters (LFO, modulation)
        if param_name.lower() in ["rate", "speed", "frequency"]:
            # Musical rates with some variance
            variance = random.uniform(-0.1, 0.1)
            return max(0.0, min(1.0, default_value + variance))
        
        # Default: moderate variance around the default
        variance = np.random.normal(0, 0.1)  # Normal distribution
        return max(0.0, min(1.0, default_value + variance))
    
    def fix_corpus(self) -> List[Dict[str, Any]]:
        """Fix the corpus using real musical defaults"""
        
        print("🎵 FIXING CORPUS WITH REAL MUSICAL DEFAULTS")
        print("=" * 80)
        
        # Load existing corpus
        with open(self.preset_file, 'r') as f:
            presets = json.load(f)
        
        # Backup first
        with open(self.backup_file, 'w') as f:
            json.dump(presets, f, indent=2)
        print(f"✅ Backed up to {self.backup_file}")
        
        fixed_count = 0
        
        for preset in presets:
            preset_name = preset.get("creative_name", "Unknown")
            character = self._detect_character(preset_name)
            
            # Fix parameters for each slot
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                
                if engine_id > 0 and engine_id in self.engine_defaults:
                    # Get the real defaults for this engine
                    engine_params = self.engine_defaults[engine_id]
                    
                    # Set parameters based on real defaults with variance
                    for param_idx, (param_name, default_value) in engine_params.items():
                        key = f"slot{slot}_param{param_idx}"
                        
                        # Generate value with musical variance
                        value = self.get_variance_for_parameter(
                            engine_id, param_idx, default_value, 
                            param_name, character
                        )
                        
                        preset[key] = round(value, 3)
                    
                    # Fill any missing parameters with 0
                    for param in range(10):
                        key = f"slot{slot}_param{param}"
                        if key not in preset:
                            preset[key] = 0.0
                    
                    fixed_count += 1
                elif engine_id > 0:
                    # Unknown engine - use conservative defaults
                    for param in range(10):
                        key = f"slot{slot}_param{param}"
                        if param == 0:  # Main parameter
                            preset[key] = round(random.uniform(0.3, 0.6), 3)
                        elif param in [4, 5, 6]:  # Mix-like parameters
                            preset[key] = round(random.uniform(0.5, 0.8), 3)
                        else:
                            preset[key] = round(random.uniform(0.2, 0.5), 3)
                    fixed_count += 1
        
        print(f"\n✅ Fixed parameters for {fixed_count} engine slots")
        print(f"   Using real musical defaults from UnifiedDefaultParameters")
        
        # Save fixed corpus
        fixed_file = self.corpus_path / "faiss_index" / "presets_musical_fixed.json"
        with open(fixed_file, 'w') as f:
            json.dump(presets, f, indent=2)
        print(f"💾 Saved musically-fixed corpus to {fixed_file}")
        
        # Verify the fix
        self._verify_fix(presets)
        
        return presets
    
    def _detect_character(self, name: str) -> str:
        """Detect character from preset name"""
        name_lower = name.lower()
        
        if any(w in name_lower for w in ["warm", "vintage", "tube", "analog", "smooth"]):
            return "warm"
        elif any(w in name_lower for w in ["metal", "brutal", "aggressive", "heavy", "harsh"]):
            return "aggressive"
        elif any(w in name_lower for w in ["clean", "pristine", "clear", "pure", "transparent"]):
            return "clean"
        elif any(w in name_lower for w in ["ambient", "space", "ethereal", "dreamy"]):
            return "ambient"
        else:
            return "neutral"
    
    def _verify_fix(self, presets: List[Dict]):
        """Verify the fix worked with musical values"""
        print("\n📊 VERIFICATION:")
        print("-" * 40)
        
        all_values = []
        param_stats = {}
        
        for preset in presets:
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    for param in range(10):
                        key = f"slot{slot}_param{param}"
                        if key in preset:
                            value = preset[key]
                            all_values.append(value)
                            
                            # Track per-engine statistics
                            if engine_id not in param_stats:
                                param_stats[engine_id] = []
                            param_stats[engine_id].append(value)
        
        if all_values:
            unique_values = len(set(all_values))
            variance = np.var(all_values)
            
            print(f"Total parameter values: {len(all_values)}")
            print(f"Unique values: {unique_values}")
            print(f"Variance: {variance:.6f}")
            print(f"Min: {min(all_values):.3f}")
            print(f"Max: {max(all_values):.3f}")
            print(f"Mean: {np.mean(all_values):.3f}")
            
            # Show some engine-specific stats
            print("\nEngine-specific parameter ranges:")
            for engine_id in list(param_stats.keys())[:5]:  # Show first 5
                values = param_stats[engine_id]
                if values:
                    engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
                    print(f"  {engine_name}: mean={np.mean(values):.2f}, std={np.std(values):.2f}")
            
            if variance > 0.01 and unique_values > 100:
                print("\n✅ SUCCESS! Parameters now have musical variance!")
                print("   - Based on real engine defaults")
                print("   - Appropriate variance for each parameter type")
                print("   - Character-aware adjustments")
            else:
                print("\n⚠️ WARNING: Variance might be too low")
        else:
            print("❌ No parameters found!")
    
    def show_examples(self, presets: List[Dict], count: int = 3):
        """Show example presets with new musical parameters"""
        print("\n📝 EXAMPLE MUSICALLY-FIXED PRESETS:")
        print("-" * 40)
        
        for preset in presets[:count]:
            print(f"\n'{preset.get('creative_name', 'Unknown')}' (Character: {self._detect_character(preset.get('creative_name', ''))})")
            
            for slot in range(1, 7):
                engine_id = preset.get(f"slot{slot}_engine", 0)
                if engine_id > 0:
                    engine_name = ENGINE_NAMES.get(engine_id, f"Unknown({engine_id})")
                    print(f"  Slot {slot}: {engine_name}")
                    
                    # Show parameters with names if available
                    params = []
                    if engine_id in self.engine_defaults:
                        for p in range(3):  # Show first 3
                            key = f"slot{slot}_param{p}"
                            if key in preset and p in self.engine_defaults[engine_id]:
                                param_name = self.engine_defaults[engine_id][p][0]
                                default = self.engine_defaults[engine_id][p][1]
                                actual = preset[key]
                                diff = actual - default
                                sign = "+" if diff > 0 else ""
                                params.append(f"{param_name}={actual:.2f} ({sign}{diff:.2f})")
                    else:
                        # Just show values
                        for p in range(3):
                            key = f"slot{slot}_param{p}"
                            if key in preset:
                                params.append(f"p{p}={preset[key]:.2f}")
                    
                    if params:
                        print(f"    {', '.join(params)}")


def main():
    print("=" * 80)
    print("🎵 MUSICAL CORPUS PARAMETER FIX")
    print("=" * 80)
    
    print("""
This fix uses REAL musical default parameters from UnifiedDefaultParameters.cpp
These defaults were carefully crafted for:
✅ Safety - No harsh or damaging sounds
✅ Musical utility - Immediate satisfaction
✅ Professional polish - Production-ready values
✅ Educational value - Teach proper parameter relationships
    """)
    
    # Fix the corpus with real defaults
    fixer = MusicalCorpusFixer()
    fixed_presets = fixer.fix_corpus()
    fixer.show_examples(fixed_presets, count=5)
    
    print("\n" + "=" * 80)
    print("🎯 NEXT STEPS")
    print("=" * 80)
    
    print("""
1. Update Oracle to use the fixed corpus:
   - Point to presets_musical_fixed.json
   - Rebuild FAISS index with new parameters
   
2. Test the improved pipeline:
   - Should see better variety in results
   - Parameters should make musical sense
   - Different presets for different prompts
   
3. Consider hybrid approach:
   - Use fixed corpus as foundation
   - Let OpenAI generate variations
   - Validate with UnifiedDefaultParameters
    """)

if __name__ == "__main__":
    main()