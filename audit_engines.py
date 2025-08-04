#!/usr/bin/env python3
"""
Comprehensive Engine Quality Audit for ChimeraPhoenix
Analyzes all 54 engines and categorizes them by upgrade priority
"""

import os
import re
from pathlib import Path

# Define all engines with their string IDs
ENGINES = {
    "vintage_tube": "VintageTubePreamp",
    "tape_saturation": "TapeSaturation",
    "rodent_distortion": "RodentDistortion", 
    "fuzz_face": "MuffFuzz",
    "spring_reverb": "SpringReverb",
    "opto_compressor": "VintageOptoCompressor",
    "slap_delay": "SlapDelay",
    "moog_filter": "MoogFilter",
    "vintage_phaser": "VintagePhaser",
    "wah_wah": "WahWah",
    "plate_reverb": "PlateReverb",
    "k_style": "KStyle",
    "hall_reverb": "HallReverb",
    "shimmer_reverb": "ShimmerReverb",
    "room_reverb": "RoomReverb",
    "stereo_chorus": "StereoChorus",
    "tape_echo": "TapeEcho",
    "digital_delay": "DigitalDelay",
    "classic_tremolo": "ClassicTremolo",
    "harmonic_tremolo": "HarmonicTremolo",
    "adaptive_limiter": "AdaptiveLimiter",
    "dimension_expander": "DimensionExpander",
    "harmonic_exciter": "HarmonicExciter",
    "mid_side_processor": "MidSideProcessor",
    "vintage_console_eq": "VintageConsoleEQ",
    "parametric_eq": "ParametricEQ",
    "transient_shaper": "TransientShaper",
    "classic_compressor": "ClassicCompressor",
    "vocal_formant": "VocalFormant",
    "comb_resonator": "CombResonator",
    "envelope_filter": "EnvelopeFilter",
    "pitch_shifter": "PitchShifter",
    "spectral_freeze": "SpectralFreeze",
    "granular_cloud": "GranularCloud",
    "buffer_repeat": "BufferRepeat",
    "chaos_generator": "ChaosGenerator",
    "intelligent_harmonizer": "IntelligentHarmonizer",
    "gated_reverb": "GatedReverb",
    "detune_doubler": "DetuneDoubler",
    "phased_vocoder": "PhasedVocoder",
    "spectral_gate": "SpectralGate",
    "noise_gate": "NoiseGate",
    "feedback_network": "FeedbackNetwork",
    "mastering_limiter": "MasteringLimiter",
    "stereo_widener": "StereoWidener",
    "resonant_chorus": "ResonantChorus",
    "dynamic_eq": "DynamicEQ",
    "stereo_imager": "StereoImager",
    "bypass": "Bypass"
}

def analyze_engine(header_path, cpp_path):
    """Analyze an engine's implementation quality"""
    quality_score = 0
    features = []
    issues = []
    
    if not os.path.exists(header_path):
        return None
        
    with open(header_path, 'r') as f:
        header_content = f.read()
        
    cpp_content = ""
    if os.path.exists(cpp_path):
        with open(cpp_path, 'r') as f:
            cpp_content = f.read()
    
    # Check for quality indicators
    
    # 1. DC Blocking
    if 'DCBlocker' in header_content or 'dc' in header_content.lower():
        quality_score += 10
        features.append("DC Blocking")
    else:
        issues.append("No DC blocking")
    
    # 2. Parameter Smoothing
    if 'SmoothParam' in header_content or 'smoothing' in header_content.lower():
        quality_score += 10
        features.append("Parameter smoothing")
    else:
        issues.append("No parameter smoothing")
    
    # 3. Anti-aliasing
    if 'antiAlias' in header_content or 'oversample' in header_content.lower():
        quality_score += 15
        features.append("Anti-aliasing")
    elif 'filter' in header_content.lower():
        quality_score += 5
        features.append("Basic filtering")
    
    # 4. Dry/Wet Mix
    if 'm_mix' in header_content or 'mix' in cpp_content.lower():
        quality_score += 10
        features.append("Dry/wet mix")
    else:
        issues.append("No dry/wet mix")
    
    # 5. Thread Safety
    if 'atomic' in header_content or 'lock' in header_content:
        quality_score += 10
        features.append("Thread-safe")
    
    # 6. Buffer pre-allocation
    if 'prepareToPlay' in cpp_content:
        quality_score += 10
        features.append("Proper initialization")
    else:
        issues.append("No prepareToPlay")
    
    # 7. Advanced DSP techniques
    if 'fft' in header_content.lower() or 'spectral' in header_content.lower():
        quality_score += 15
        features.append("Spectral processing")
    elif 'granular' in header_content.lower():
        quality_score += 15
        features.append("Granular synthesis")
    elif 'convolution' in header_content.lower():
        quality_score += 15
        features.append("Convolution")
    
    # 8. Proper bounds checking
    if 'clamp' in cpp_content or 'std::max' in cpp_content:
        quality_score += 5
        features.append("Bounds checking")
    else:
        issues.append("No bounds checking")
    
    # 9. Comments/Documentation
    if cpp_content.count('//') > 10 or '/**' in header_content:
        quality_score += 5
        features.append("Well documented")
    else:
        issues.append("Lacks documentation")
    
    # 10. Complex implementation
    lines_of_code = cpp_content.count('\n')
    if lines_of_code > 300:
        quality_score += 10
        features.append(f"Comprehensive ({lines_of_code} LOC)")
    elif lines_of_code > 150:
        quality_score += 5
        features.append(f"Moderate complexity ({lines_of_code} LOC)")
    else:
        issues.append(f"Simple implementation ({lines_of_code} LOC)")
    
    return {
        'score': quality_score,
        'features': features,
        'issues': issues,
        'loc': lines_of_code
    }

def main():
    base_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source")
    
    results = {}
    
    for string_id, class_name in ENGINES.items():
        if string_id == "bypass":
            continue
            
        header_path = base_path / f"{class_name}.h"
        cpp_path = base_path / f"{class_name}.cpp"
        
        analysis = analyze_engine(str(header_path), str(cpp_path))
        if analysis:
            results[string_id] = {
                'class': class_name,
                **analysis
            }
    
    # Sort by quality score
    sorted_engines = sorted(results.items(), key=lambda x: x[1]['score'])
    
    print("=" * 80)
    print("CHIMERA PHOENIX ENGINE QUALITY AUDIT")
    print("=" * 80)
    print()
    
    # Categorize engines
    needs_complete_rewrite = []
    needs_major_upgrade = []
    needs_minor_upgrade = []
    professional_quality = []
    
    for engine_id, data in sorted_engines:
        if data['score'] >= 80:
            professional_quality.append((engine_id, data))
        elif data['score'] >= 60:
            needs_minor_upgrade.append((engine_id, data))
        elif data['score'] >= 40:
            needs_major_upgrade.append((engine_id, data))
        else:
            needs_complete_rewrite.append((engine_id, data))
    
    # Print categories
    print("🔴 NEEDS COMPLETE REWRITE (Score < 40)")
    print("-" * 40)
    for engine_id, data in needs_complete_rewrite:
        print(f"  {engine_id:25} Score: {data['score']:3}  Issues: {', '.join(data['issues'][:3])}")
    
    print("\n🟡 NEEDS MAJOR UPGRADE (Score 40-59)")
    print("-" * 40)
    for engine_id, data in needs_major_upgrade:
        print(f"  {engine_id:25} Score: {data['score']:3}  Issues: {', '.join(data['issues'][:3])}")
    
    print("\n🟢 NEEDS MINOR UPGRADE (Score 60-79)")
    print("-" * 40)
    for engine_id, data in needs_minor_upgrade:
        print(f"  {engine_id:25} Score: {data['score']:3}  Has: {', '.join(data['features'][:3])}")
    
    print("\n✅ PROFESSIONAL QUALITY (Score 80+)")
    print("-" * 40)
    for engine_id, data in professional_quality:
        print(f"  {engine_id:25} Score: {data['score']:3}  Features: {', '.join(data['features'][:3])}")
    
    # Summary statistics
    print("\n" + "=" * 80)
    print("SUMMARY")
    print("=" * 80)
    print(f"Total Engines Analyzed: {len(results)}")
    print(f"Professional Quality:    {len(professional_quality):2} engines")
    print(f"Minor Upgrades Needed:   {len(needs_minor_upgrade):2} engines")
    print(f"Major Upgrades Needed:   {len(needs_major_upgrade):2} engines")
    print(f"Complete Rewrites:       {len(needs_complete_rewrite):2} engines")
    print(f"Average Quality Score:   {sum(d['score'] for d in results.values()) / len(results):.1f}")
    
    # Upgrade priority list
    print("\n" + "=" * 80)
    print("RECOMMENDED UPGRADE ORDER")
    print("=" * 80)
    print("\nPhase 1: Core Effects (Most Used)")
    priority_1 = ["vintage_tube", "tape_saturation", "plate_reverb", "classic_compressor", 
                  "parametric_eq", "tape_echo", "stereo_chorus"]
    for engine in priority_1:
        if engine in results:
            print(f"  1. {engine:25} Current Score: {results[engine]['score']}")
    
    print("\nPhase 2: Character Effects")
    priority_2 = ["shimmer_reverb", "spring_reverb", "rodent_distortion", "dimension_expander",
                  "granular_cloud", "spectral_freeze"]
    for engine in priority_2:
        if engine in results:
            print(f"  2. {engine:25} Current Score: {results[engine]['score']}")
    
    print("\nPhase 3: Utility Processors")
    priority_3 = ["noise_gate", "transient_shaper", "stereo_widener", "mastering_limiter",
                  "dynamic_eq", "mid_side_processor"]
    for engine in priority_3:
        if engine in results:
            print(f"  3. {engine:25} Current Score: {results[engine]['score']}")
    
    print("\nPhase 4: Creative/Experimental")
    priority_4 = ["chaos_generator", "buffer_repeat", "feedback_network", "phased_vocoder"]
    for engine in priority_4:
        if engine in results:
            print(f"  4. {engine:25} Current Score: {results[engine]['score']}")

if __name__ == "__main__":
    main()