"""
Trinity Ground Truth - Electronic/Hip-Hop/Experimental Focus
Based on user priorities: Engine Selection > Signal Flow > Mix Balance > Parameters
Target: Sound designers and musicians/producers
Max 5 effects to leave room for manual additions
"""

from typing import Dict, Any, List
from engine_mapping_authoritative import *
import json

class ElectronicGroundTruth:
    """Ground truth presets for electronic/experimental music production"""
    
    def __init__(self):
        self.ground_truth_presets = self._build_ground_truth()
        self.validation_rules = self._build_validation_rules()
    
    def _build_ground_truth(self) -> List[Dict[str, Any]]:
        """Build ground truth presets for electronic/experimental genres"""
        
        presets = []
        
        # 1. BASS DESIGN - Deep sub bass with character
        presets.append({
            "prompt": "deep sub bass with analog warmth",
            "engines": [
                ENGINE_LADDER_FILTER,      # Slot 1: Classic analog filter
                ENGINE_VINTAGE_TUBE,        # Slot 2: Warmth
                ENGINE_MULTIBAND_SATURATOR, # Slot 3: Controlled saturation
                ENGINE_OPTO_COMPRESSOR,     # Slot 4: Smooth compression
            ],
            "signal_flow_rationale": "Filter shapes tone → Tube adds warmth → Saturation for harmonics → Compression glues",
            "mix_levels": [0.8, 0.4, 0.3, 0.6],  # Heavy filter, subtle warmth
            "key_params": {
                "slot1_param1": 0.25,  # Low cutoff for sub
                "slot1_param2": 0.7,   # High resonance
                "slot2_param1": 0.3,   # Gentle tube drive
                "slot3_param3": 0.6,   # Focus on low bands
            },
            "validation_score": 0.95
        })
        
        # 2. GLITCH HOP - Bit crushed rhythmic destruction  
        presets.append({
            "prompt": "glitchy bit crushed percussion",
            "engines": [
                ENGINE_BIT_CRUSHER,        # Slot 1: Digital destruction
                ENGINE_RING_MODULATOR,     # Slot 2: Metallic textures
                ENGINE_BUFFER_REPEAT,      # Slot 3: Glitch repeats
                ENGINE_TAPE_ECHO,          # Slot 4: Vintage delay
            ],
            "signal_flow_rationale": "Bit crush → Ring mod for alien tones → Buffer for stutters → Tape for space",
            "mix_levels": [0.7, 0.4, 0.6, 0.3],
            "key_params": {
                "slot1_param1": 0.4,   # 8-bit reduction
                "slot1_param2": 0.6,   # Sample rate reduction
                "slot2_param1": 0.7,   # High frequency modulation
                "slot3_param1": 0.125, # 1/8 note repeats
            },
            "validation_score": 0.92
        })
        
        # 3. AMBIENT PAD - Ethereal evolving textures
        presets.append({
            "prompt": "ethereal ambient pad with movement",
            "engines": [
                ENGINE_ANALOG_PHASER,      # Slot 1: Movement
                ENGINE_SHIMMER_REVERB,     # Slot 2: Ethereal space
                ENGINE_PITCH_SHIFTER,      # Slot 3: Octave layers
                ENGINE_DIMENSION_EXPANDER, # Slot 4: Width
            ],
            "signal_flow_rationale": "Phaser creates movement → Shimmer adds magic → Pitch for depth → Dimension for space",
            "mix_levels": [0.5, 0.7, 0.3, 0.8],
            "key_params": {
                "slot1_param1": 0.1,   # Slow phaser rate
                "slot2_param1": 0.8,   # Long decay
                "slot3_param1": 0.583, # +7 semitones (5th)
                "slot4_param1": 0.9,   # Maximum width
            },
            "validation_score": 0.94
        })
        
        # 4. TRAP HI-HAT - Crispy processed trap hats
        presets.append({
            "prompt": "crispy trap hi-hats with roll effect",
            "engines": [
                ENGINE_TRANSIENT_SHAPER,   # Slot 1: Attack enhancement  
                ENGINE_PARAMETRIC_EQ,      # Slot 2: Brightness
                ENGINE_DIGITAL_DELAY,      # Slot 3: Roll effect
                ENGINE_WAVE_FOLDER,        # Slot 4: Harmonic excitement
            ],
            "signal_flow_rationale": "Transients first → EQ for presence → Delay for rolls → Folding for edge",
            "mix_levels": [0.8, 1.0, 0.4, 0.2],
            "key_params": {
                "slot1_param1": 0.8,    # Sharp attack
                "slot2_param2": 0.75,   # High frequency boost
                "slot3_param1": 0.0625, # 1/16 note delay
                "slot4_param1": 0.3,    # Subtle folding
            },
            "validation_score": 0.91
        })
        
        # 5. VOCAL CHOP - Modern vocal manipulation
        presets.append({
            "prompt": "chopped vocal with formant shift",
            "engines": [
                ENGINE_FORMANT_FILTER,     # Slot 1: Formant manipulation
                ENGINE_GRANULAR_CLOUD,     # Slot 2: Grain chopping
                ENGINE_FREQUENCY_SHIFTER,  # Slot 3: Alien harmonics
                ENGINE_PLATE_REVERB,       # Slot 4: Space
            ],
            "signal_flow_rationale": "Formant shapes character → Granular chops → Freq shift for uniqueness → Reverb glues",
            "mix_levels": [0.7, 0.8, 0.3, 0.4],
            "key_params": {
                "slot1_param1": 0.4,   # Gender shift down
                "slot2_param1": 0.3,   # Small grain size
                "slot3_param1": 0.52,  # Slight upward shift
                "slot4_param1": 0.5,   # Medium decay
            },
            "validation_score": 0.93
        })
        
        # 6. REESE BASS - Classic DnB reese bass
        presets.append({
            "prompt": "reese bass with detuned layers",
            "engines": [
                ENGINE_DETUNE_DOUBLER,     # Slot 1: Detuned unison
                ENGINE_LADDER_FILTER,      # Slot 2: Filter sweep
                ENGINE_WAVE_FOLDER,        # Slot 3: Harmonic richness
                ENGINE_DIGITAL_CHORUS,     # Slot 4: Width
            ],
            "signal_flow_rationale": "Detune creates thickness → Filter shapes → Folding adds harmonics → Chorus spreads",
            "mix_levels": [1.0, 0.9, 0.4, 0.5],
            "key_params": {
                "slot1_param1": 0.15,  # 15 cents detune
                "slot2_param1": 0.4,   # Mid-low cutoff
                "slot2_param2": 0.6,   # Moderate resonance
                "slot3_param1": 0.5,   # Medium folding
            },
            "validation_score": 0.96
        })
        
        # 7. ACID LEAD - 303-style acid line
        presets.append({
            "prompt": "acid lead with squelchy resonance",
            "engines": [
                ENGINE_LADDER_FILTER,      # Slot 1: The classic 303 filter
                ENGINE_RODENT_DISTORTION,  # Slot 2: Aggressive drive
                ENGINE_ENVELOPE_FILTER,    # Slot 3: Additional movement
                ENGINE_TAPE_ECHO,          # Slot 4: Dub delays
            ],
            "signal_flow_rationale": "Main filter for acid sound → Distortion for aggression → Env filter for extra squelch → Echo for space",
            "mix_levels": [1.0, 0.6, 0.5, 0.3],
            "key_params": {
                "slot1_param1": 0.3,   # Low starting cutoff
                "slot1_param2": 0.85,  # High resonance (squelch!)
                "slot2_param1": 0.7,   # Heavy distortion
                "slot3_param2": 0.6,   # Envelope sensitivity
            },
            "validation_score": 0.94
        })
        
        # 8. LOFI SAMPLE - Vintage sampler emulation
        presets.append({
            "prompt": "lofi sample with tape wobble",
            "engines": [
                ENGINE_BIT_CRUSHER,        # Slot 1: Sample rate reduction
                ENGINE_TAPE_ECHO,          # Slot 2: Tape wobble/saturation
                ENGINE_VINTAGE_CONSOLE_EQ, # Slot 3: Vintage coloration
                ENGINE_SPRING_REVERB,      # Slot 4: Lofi space
            ],
            "signal_flow_rationale": "Bit reduction → Tape character → Console warmth → Spring verb character",
            "mix_levels": [0.6, 0.7, 0.8, 0.2],
            "key_params": {
                "slot1_param1": 0.3,   # 12-bit
                "slot1_param2": 0.4,   # Moderate downsampling
                "slot2_param3": 0.7,   # Wow/flutter
                "slot3_param1": 0.6,   # Vintage curve
            },
            "validation_score": 0.92
        })
        
        # 9. NEUROFUNK BASS - Complex bass design
        presets.append({
            "prompt": "neurofunk bass with metallic growl",
            "engines": [
                ENGINE_RING_MODULATOR,     # Slot 1: Metallic tones
                ENGINE_COMB_RESONATOR,     # Slot 2: Resonant body
                ENGINE_MULTIBAND_SATURATOR,# Slot 3: Controlled distortion
                ENGINE_PHASED_VOCODER,     # Slot 4: Vocoded texture
            ],
            "signal_flow_rationale": "Ring mod for metallic → Comb for resonance → Multiband for control → Vocoder for talk",
            "mix_levels": [0.5, 0.7, 0.8, 0.4],
            "key_params": {
                "slot1_param1": 0.2,   # Low mod frequency
                "slot2_param1": 0.15,  # Tight comb
                "slot3_param2": 0.7,   # Mid-band focus
                "slot4_param1": 0.6,   # Vocoder character
            },
            "validation_score": 0.93
        })
        
        # 10. DUBSTEP WOBBLE - Classic wobble bass
        presets.append({
            "prompt": "dubstep wobble bass heavy",
            "engines": [
                ENGINE_STATE_VARIABLE_FILTER, # Slot 1: Filter for wobble
                ENGINE_ENVELOPE_FILTER,       # Slot 2: LFO modulation
                ENGINE_MUFF_FUZZ,             # Slot 3: Fuzz distortion
                ENGINE_OPTO_COMPRESSOR,       # Slot 4: Glue compression
            ],
            "signal_flow_rationale": "SVF for main filter → Env filter for LFO wobble → Fuzz for grit → Comp for consistency",
            "mix_levels": [1.0, 0.8, 0.5, 0.7],
            "key_params": {
                "slot1_param1": 0.25,  # Low cutoff
                "slot1_param2": 0.7,   # High Q
                "slot2_param1": 0.25,  # 1/4 note wobble rate
                "slot3_param1": 0.6,   # Medium fuzz
            },
            "validation_score": 0.95
        })
        
        # 11. TECHNO KICK - Punchy techno kick
        presets.append({
            "prompt": "punchy techno kick with sub",
            "engines": [
                ENGINE_TRANSIENT_SHAPER,   # Slot 1: Punch enhancement
                ENGINE_PARAMETRIC_EQ,      # Slot 2: Sub boost
                ENGINE_VINTAGE_TUBE,       # Slot 3: Harmonic warmth
                ENGINE_MASTERING_LIMITER,  # Slot 4: Final limiting
            ],
            "signal_flow_rationale": "Transient shapes attack → EQ for sub → Tube for harmonics → Limiter for loudness",
            "mix_levels": [0.9, 1.0, 0.3, 0.8],
            "key_params": {
                "slot1_param1": 0.9,   # Maximum punch
                "slot2_param1": 0.15,  # 60Hz boost
                "slot3_param1": 0.4,   # Subtle tube
                "slot4_param1": 0.7,   # Moderate limiting
            },
            "validation_score": 0.94
        })
        
        # 12. FUTURE GARAGE - Atmospheric garage
        presets.append({
            "prompt": "future garage atmosphere with shuffle",
            "engines": [
                ENGINE_DIGITAL_DELAY,      # Slot 1: Shuffled delay
                ENGINE_HARMONIC_TREMOLO,   # Slot 2: Rhythmic movement
                ENGINE_CONVOLUTION_REVERB, # Slot 3: Real space
                ENGINE_STEREO_WIDENER,     # Slot 4: Width
            ],
            "signal_flow_rationale": "Delay for rhythm → Tremolo for pulse → Convolution for realism → Width for space",
            "mix_levels": [0.5, 0.4, 0.6, 0.7],
            "key_params": {
                "slot1_param1": 0.375, # Dotted 8th
                "slot2_param1": 0.5,   # 1/8 tremolo
                "slot3_param1": 0.6,   # Medium room
                "slot4_param1": 0.8,   # Wide stereo
            },
            "validation_score": 0.91
        })
        
        # 13. VAPORWAVE - Nostalgic degradation
        presets.append({
            "prompt": "vaporwave nostalgic degradation",
            "engines": [
                ENGINE_RESONANT_CHORUS,    # Slot 1: 80s chorus
                ENGINE_PITCH_SHIFTER,      # Slot 2: Pitched down
                ENGINE_MAGNETIC_DRUM_ECHO, # Slot 3: Vintage echo
                ENGINE_VINTAGE_CONSOLE_EQ, # Slot 4: Retro tone
            ],
            "signal_flow_rationale": "Chorus for 80s vibe → Pitch down for slow → Drum echo character → Console warmth",
            "mix_levels": [0.7, 1.0, 0.5, 0.8],
            "key_params": {
                "slot1_param1": 0.3,   # Slow chorus
                "slot2_param1": 0.42,  # -2 semitones
                "slot3_param1": 0.5,   # Medium delay
                "slot4_param1": 0.6,   # Warm curve
            },
            "validation_score": 0.90
        })
        
        # 14. IDM GLITCH - Intelligent dance music
        presets.append({
            "prompt": "idm glitch textures complex",
            "engines": [
                ENGINE_SPECTRAL_FREEZE,    # Slot 1: Freeze grains
                ENGINE_GRANULAR_CLOUD,     # Slot 2: Granular processing
                ENGINE_CHAOS_GENERATOR,    # Slot 3: Controlled chaos
                ENGINE_FEEDBACK_NETWORK,   # Slot 4: Complex feedback
            ],
            "signal_flow_rationale": "Spectral freeze → Granular scatter → Chaos injection → Feedback complexity",
            "mix_levels": [0.6, 0.7, 0.3, 0.4],
            "key_params": {
                "slot1_param1": 0.7,   # Freeze threshold
                "slot2_param2": 0.8,   # Grain scatter
                "slot3_param1": 0.4,   # Moderate chaos
                "slot4_param1": 0.5,   # Feedback amount
            },
            "validation_score": 0.89
        })
        
        # 15. PHONK BASS - Memphis phonk bass
        presets.append({
            "prompt": "phonk bass distorted and low",
            "engines": [
                ENGINE_PARAMETRIC_EQ,      # Slot 1: Sub emphasis
                ENGINE_RODENT_DISTORTION,  # Slot 2: Aggressive distortion
                ENGINE_K_STYLE,            # Slot 3: More overdrive
                ENGINE_OPTO_COMPRESSOR,    # Slot 4: Glue
            ],
            "signal_flow_rationale": "EQ boosts lows → Rodent distortion → K-Style adds harmonics → Compression glues",
            "mix_levels": [1.0, 0.8, 0.5, 0.6],
            "key_params": {
                "slot1_param1": 0.1,   # Deep bass boost
                "slot2_param1": 0.8,   # Heavy distortion
                "slot3_param1": 0.6,   # Moderate overdrive
                "slot4_param2": 0.4,   # 4:1 ratio
            },
            "validation_score": 0.92
        })
        
        return presets
    
    def _build_validation_rules(self) -> Dict[str, Any]:
        """Build validation rules specific to electronic music"""
        
        return {
            "engine_selection": {
                "priority": 1.0,  # Highest priority as requested
                "rules": [
                    {
                        "prompt_contains": ["bass", "sub"],
                        "should_include_one_of": [
                            ENGINE_LADDER_FILTER,
                            ENGINE_STATE_VARIABLE_FILTER,
                            ENGINE_MULTIBAND_SATURATOR,
                            ENGINE_OPTO_COMPRESSOR
                        ],
                        "weight": 0.9
                    },
                    {
                        "prompt_contains": ["glitch", "stutter", "chop"],
                        "should_include_one_of": [
                            ENGINE_BUFFER_REPEAT,
                            ENGINE_GRANULAR_CLOUD,
                            ENGINE_BIT_CRUSHER,
                            ENGINE_RING_MODULATOR
                        ],
                        "weight": 0.85
                    },
                    {
                        "prompt_contains": ["ambient", "ethereal", "atmospheric"],
                        "should_include_one_of": [
                            ENGINE_SHIMMER_REVERB,
                            ENGINE_CONVOLUTION_REVERB,
                            ENGINE_DIMENSION_EXPANDER,
                            ENGINE_SPECTRAL_FREEZE
                        ],
                        "weight": 0.9
                    },
                    {
                        "prompt_contains": ["acid", "303", "squelch"],
                        "must_include": ENGINE_LADDER_FILTER,
                        "weight": 1.0
                    },
                    {
                        "prompt_contains": ["wobble", "dubstep"],
                        "should_include_one_of": [
                            ENGINE_STATE_VARIABLE_FILTER,
                            ENGINE_ENVELOPE_FILTER
                        ],
                        "weight": 0.95
                    }
                ]
            },
            
            "signal_flow": {
                "priority": 0.8,  # Second priority
                "rules": [
                    {
                        "name": "filter_before_distortion",
                        "if_has": [ENGINE_LADDER_FILTER, ENGINE_STATE_VARIABLE_FILTER],
                        "and_has": [ENGINE_RODENT_DISTORTION, ENGINE_MUFF_FUZZ, ENGINE_WAVE_FOLDER],
                        "filter_should_come_first": True,
                        "weight": 0.7
                    },
                    {
                        "name": "compression_after_distortion",
                        "if_has": [ENGINE_OPTO_COMPRESSOR, ENGINE_VCA_COMPRESSOR],
                        "and_has": [ENGINE_RODENT_DISTORTION, ENGINE_MUFF_FUZZ],
                        "compression_should_come_after": True,
                        "weight": 0.8
                    },
                    {
                        "name": "reverb_delay_last",
                        "if_has": [ENGINE_SHIMMER_REVERB, ENGINE_PLATE_REVERB, ENGINE_TAPE_ECHO],
                        "should_be_in_last_two_slots": True,
                        "weight": 0.9
                    }
                ]
            },
            
            "mix_levels": {
                "priority": 0.6,  # Third priority
                "rules": [
                    {
                        "name": "primary_effect_prominent",
                        "first_active_slot_mix": {"min": 0.7, "max": 1.0},
                        "weight": 0.7
                    },
                    {
                        "name": "subtle_modulation",
                        "for_engines": [ENGINE_DIGITAL_CHORUS, ENGINE_ANALOG_PHASER],
                        "mix_range": {"min": 0.2, "max": 0.6},
                        "weight": 0.6
                    }
                ]
            },
            
            "parameters": {
                "priority": 0.4,  # Lowest priority except for critical params
                "critical_params": [
                    {
                        "engine": ENGINE_PITCH_SHIFTER,
                        "param": "param1",  # Pitch amount
                        "importance": 1.0
                    },
                    {
                        "engine": ENGINE_LADDER_FILTER,
                        "param": "param2",  # Resonance
                        "importance": 0.9
                    }
                ]
            },
            
            "creative_guidelines": {
                "max_active_slots": 5,  # Leave room for manual addition
                "prefer_experimental": True,
                "allow_unconventional_routing": True,
                "encourage_rare_combinations": True
            }
        }
    
    def validate_preset(self, preset: Dict[str, Any], prompt: str) -> Dict[str, float]:
        """Validate a generated preset against electronic music standards"""
        
        scores = {
            "engine_selection": 0.0,
            "signal_flow": 0.0,
            "mix_levels": 0.0,
            "parameters": 0.0,
            "creativity": 0.0
        }
        
        params = preset.get("parameters", {})
        
        # Extract active engines and their positions
        active_engines = []
        for slot in range(1, 7):
            engine_id = params.get(f"slot{slot}_engine", 0)
            if engine_id > 0 and params.get(f"slot{slot}_bypass", 1.0) < 0.5:
                active_engines.append((slot, engine_id))
        
        # 1. Engine Selection Score (highest weight)
        prompt_lower = prompt.lower()
        engine_score = 0.0
        matches = 0
        
        for rule in self.validation_rules["engine_selection"]["rules"]:
            if any(keyword in prompt_lower for keyword in rule.get("prompt_contains", [])):
                if "must_include" in rule:
                    if any(e[1] == rule["must_include"] for e in active_engines):
                        engine_score += rule["weight"]
                        matches += 1
                elif "should_include_one_of" in rule:
                    if any(e[1] in rule["should_include_one_of"] for e in active_engines):
                        engine_score += rule["weight"]
                        matches += 1
        
        scores["engine_selection"] = engine_score / max(matches, 1) if matches > 0 else 0.5
        
        # 2. Signal Flow Score
        if len(active_engines) > 1:
            flow_score = 1.0  # Start with perfect score
            
            # Check filter before distortion
            filter_pos = next((s for s, e in active_engines if e in [ENGINE_LADDER_FILTER, ENGINE_STATE_VARIABLE_FILTER]), None)
            dist_pos = next((s for s, e in active_engines if e in DISTORTION_ENGINES), None)
            if filter_pos and dist_pos and filter_pos > dist_pos:
                flow_score *= 0.7  # Penalty for wrong order
            
            # Check reverb/delay at end
            reverb_delay = [(s, e) for s, e in active_engines if e in DELAY_REVERB_ENGINES]
            if reverb_delay:
                last_effect_slot = max(s for s, e in active_engines)
                if reverb_delay[0][0] < last_effect_slot - 1:
                    flow_score *= 0.8  # Penalty for early reverb/delay
            
            scores["signal_flow"] = flow_score
        else:
            scores["signal_flow"] = 1.0  # Single effect always has good flow
        
        # 3. Mix Levels Score
        mix_score = 1.0
        for slot, engine_id in active_engines:
            mix_level = params.get(f"slot{slot}_mix", 0.5)
            
            # First effect should be prominent
            if slot == active_engines[0][0]:
                if mix_level < 0.6:
                    mix_score *= 0.8
            
            # Modulation should be subtle
            if engine_id in [ENGINE_DIGITAL_CHORUS, ENGINE_ANALOG_PHASER, ENGINE_HARMONIC_TREMOLO]:
                if mix_level > 0.6:
                    mix_score *= 0.9
        
        scores["mix_levels"] = mix_score
        
        # 4. Parameter Score (only for critical params)
        param_score = 0.8  # Default decent score
        
        # Check critical parameters
        for slot, engine_id in active_engines:
            if engine_id == ENGINE_PITCH_SHIFTER:
                pitch_param = params.get(f"slot{slot}_param1", 0.5)
                # Should not be at default
                if abs(pitch_param - 0.5) < 0.01:
                    param_score *= 0.5
            
            if engine_id == ENGINE_LADDER_FILTER:
                resonance = params.get(f"slot{slot}_param2", 0.5)
                # For acid sounds, resonance should be high
                if "acid" in prompt_lower and resonance < 0.7:
                    param_score *= 0.7
        
        scores["parameters"] = param_score
        
        # 5. Creativity Score
        creativity = 0.5  # Base score
        
        # Bonus for experimental engines
        experimental_engines = [
            ENGINE_SPECTRAL_FREEZE, ENGINE_CHAOS_GENERATOR, 
            ENGINE_FEEDBACK_NETWORK, ENGINE_GRANULAR_CLOUD,
            ENGINE_PHASED_VOCODER, ENGINE_RING_MODULATOR
        ]
        
        experimental_count = sum(1 for _, e in active_engines if e in experimental_engines)
        creativity += experimental_count * 0.15
        
        # Bonus for rare combinations
        if len(active_engines) >= 3:
            engine_ids = [e for _, e in active_engines]
            # Rare combo: filter + ring mod + granular
            if (ENGINE_LADDER_FILTER in engine_ids and 
                ENGINE_RING_MODULATOR in engine_ids and 
                ENGINE_GRANULAR_CLOUD in engine_ids):
                creativity += 0.2
        
        scores["creativity"] = min(creativity, 1.0)
        
        # Calculate weighted final score
        weights = {
            "engine_selection": 0.4,  # 40% - highest priority
            "signal_flow": 0.25,       # 25% - second priority  
            "mix_levels": 0.15,        # 15% - third priority
            "parameters": 0.1,         # 10% - lowest priority
            "creativity": 0.1          # 10% - bonus for experimentation
        }
        
        final_score = sum(scores[key] * weights[key] for key in scores)
        
        return {
            "scores": scores,
            "final_score": final_score,
            "feedback": self._generate_feedback(scores, prompt, active_engines)
        }
    
    def _generate_feedback(self, scores: Dict[str, float], prompt: str, active_engines: List) -> str:
        """Generate specific feedback for improvement"""
        
        feedback = []
        
        if scores["engine_selection"] < 0.7:
            feedback.append("Engine selection doesn't match prompt intent well")
        
        if scores["signal_flow"] < 0.8:
            feedback.append("Signal flow could be optimized (filters→distortion→modulation→reverb)")
        
        if scores["mix_levels"] < 0.8:
            feedback.append("Mix levels need adjustment - primary effect should be prominent")
        
        if scores["creativity"] < 0.5:
            feedback.append("Consider more experimental engine combinations")
        
        if len(active_engines) > 5:
            feedback.append("Too many effects - limit to 5 to leave room for manual additions")
        
        return " | ".join(feedback) if feedback else "Good preset!"


# Example usage and testing
if __name__ == "__main__":
    ground_truth = ElectronicGroundTruth()
    
    # Test with a sample preset
    test_preset = {
        "parameters": {
            "slot1_engine": ENGINE_LADDER_FILTER,
            "slot1_bypass": 0.0,
            "slot1_mix": 0.8,
            "slot1_param1": 0.3,  # Cutoff
            "slot1_param2": 0.85,  # Resonance
            
            "slot2_engine": ENGINE_RODENT_DISTORTION, 
            "slot2_bypass": 0.0,
            "slot2_mix": 0.6,
            
            "slot3_engine": ENGINE_TAPE_ECHO,
            "slot3_bypass": 0.0, 
            "slot3_mix": 0.3,
            
            "slot4_engine": 0,  # Empty
            "slot5_engine": 0,  # Empty
            "slot6_engine": 0,  # Empty
        }
    }
    
    result = ground_truth.validate_preset(test_preset, "acid bass with squelchy filter")
    
    print("Validation Results:")
    print(f"Final Score: {result['final_score']:.2%}")
    print("\nDetailed Scores:")
    for key, score in result['scores'].items():
        print(f"  {key}: {score:.2%}")
    print(f"\nFeedback: {result['feedback']}")
    
    print("\n" + "="*60)
    print("GROUND TRUTH PRESETS LOADED:")
    print("="*60)
    for i, preset in enumerate(ground_truth.ground_truth_presets, 1):
        print(f"{i}. {preset['prompt']}")
        print(f"   Engines: {[get_engine_name(e) for e in preset['engines']]}")
        print(f"   Score: {preset['validation_score']:.2%}")