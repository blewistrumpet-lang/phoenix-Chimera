#!/usr/bin/env python3
"""
Compare all engines to IntelligentHarmonizer implementation quality
"""

import os
from pathlib import Path

def analyze_implementation_quality(header_path, cpp_path):
    """Deep analysis comparing to IntelligentHarmonizer standards"""
    
    # IntelligentHarmonizer Gold Standard Features
    gold_standard = {
        # Core DSP
        "Granular synthesis": False,
        "Multiple processing voices": False,
        "Circular buffering": False,
        "Window functions": False,
        "Interpolation": False,
        
        # Parameter handling
        "Thread-safe smoothing": False,
        "Parameter validation": False,
        "Proper initialization": False,
        "Update mechanism": False,
        
        # Audio quality
        "DC blocking": False,
        "Anti-aliasing": False,
        "Oversampling": False,
        "Formant preservation": False,
        
        # Advanced features  
        "Pitch detection": False,
        "Scale quantization": False,
        "Humanization": False,
        "Stereo processing": False,
        "FFT/Spectral": False,
        
        # Code quality
        "Modular design": False,
        "Clear documentation": False,
        "Magic number avoidance": False,
        "RAII/Smart pointers": False,
        "Const correctness": False,
        
        # Performance
        "Pre-allocated buffers": False,
        "SIMD optimization": False,
        "Lock-free design": False,
        "Efficient algorithms": False
    }
    
    if not os.path.exists(header_path):
        return gold_standard, 0
        
    with open(header_path, 'r') as f:
        header = f.read()
        
    cpp = ""
    if os.path.exists(cpp_path):
        with open(cpp_path, 'r') as f:
            cpp = f.read()
    
    combined = header + "\n" + cpp
    
    # Check for each feature
    if "Grain" in combined or "granular" in combined.lower():
        gold_standard["Granular synthesis"] = True
        
    if "array<" in header and "Voice" in combined:
        gold_standard["Multiple processing voices"] = True
        
    if "circular" in combined.lower() or ("writeIndex" in combined and "readIndex" in combined):
        gold_standard["Circular buffering"] = True
        
    if "window" in combined.lower() or "hann" in combined.lower() or "blackman" in combined.lower():
        gold_standard["Window functions"] = True
        
    if "interpolat" in combined.lower() or "frac" in combined:
        gold_standard["Interpolation"] = True
        
    if "SmoothParam" in header or ("atomic" in header and "smoothing" in combined.lower()):
        gold_standard["Thread-safe smoothing"] = True
        
    if "clamp" in cpp or "std::max" in cpp or "std::min" in cpp:
        gold_standard["Parameter validation"] = True
        
    if "prepareToPlay" in cpp and "reset" in cpp:
        gold_standard["Proper initialization"] = True
        
    if "update" in cpp.lower() and "target" in combined:
        gold_standard["Update mechanism"] = True
        
    if "DCBlock" in combined or "highpass" in combined.lower():
        gold_standard["DC blocking"] = True
        
    if "antiAlias" in combined or "oversample" in combined.lower():
        gold_standard["Anti-aliasing"] = True
        
    if "2x" in combined or "4x" in combined or "upsample" in combined.lower():
        gold_standard["Oversampling"] = True
        
    if "formant" in combined.lower():
        gold_standard["Formant preservation"] = True
        
    if "detectPitch" in combined or "zeroCross" in combined or "autocorrelat" in combined.lower():
        gold_standard["Pitch detection"] = True
        
    if "scale" in combined.lower() and "quantize" in combined.lower():
        gold_standard["Scale quantization"] = True
        
    if "humaniz" in combined.lower() or "vibrato" in combined.lower() or "drift" in combined:
        gold_standard["Humanization"] = True
        
    if "getNumChannels()" in cpp or "stereo" in combined.lower():
        gold_standard["Stereo processing"] = True
        
    if "fft" in combined.lower() or "spectral" in combined.lower() or "frequency domain" in combined.lower():
        gold_standard["FFT/Spectral"] = True
        
    if "struct" in header and "class" in header:
        gold_standard["Modular design"] = True
        
    if "/**" in header or combined.count("//") > 20:
        gold_standard["Clear documentation"] = True
        
    if "constexpr" in header or "static const" in header:
        gold_standard["Magic number avoidance"] = True
        
    if "unique_ptr" in combined or "shared_ptr" in combined or "std::array" in header:
        gold_standard["RAII/Smart pointers"] = True
        
    if "const override" in header or "const {" in cpp:
        gold_standard["Const correctness"] = True
        
    if "resize" in cpp or "reserve" in cpp or "std::vector" in header:
        gold_standard["Pre-allocated buffers"] = True
        
    if "__m128" in combined or "vDSP" in combined or "sse" in combined.lower():
        gold_standard["SIMD optimization"] = True
        
    if "atomic" in header or "lock_free" in combined.lower():
        gold_standard["Lock-free design"] = True
        
    if "O(1)" in combined or "lookup" in combined or "LUT" in combined:
        gold_standard["Efficient algorithms"] = True
    
    # Calculate score
    score = sum(gold_standard.values())
    max_score = len(gold_standard)
    percentage = (score / max_score) * 100
    
    return gold_standard, percentage

def main():
    base_path = Path("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Source")
    
    # First analyze IntelligentHarmonizer as reference
    harmonizer_features, harmonizer_score = analyze_implementation_quality(
        base_path / "IntelligentHarmonizer.h",
        base_path / "IntelligentHarmonizer.cpp"
    )
    
    print("=" * 100)
    print("COMPARISON TO INTELLIGENTHARMONIZER IMPLEMENTATION QUALITY")
    print("=" * 100)
    print()
    print("IntelligentHarmonizer Gold Standard Features:")
    print("-" * 50)
    for feature, present in harmonizer_features.items():
        if present:
            print(f"  ✅ {feature}")
    print(f"\nScore: {harmonizer_score:.1f}%")
    print()
    
    # Analyze all other engines
    engines = {
        "ShimmerReverb": "shimmer_reverb",
        "VintageTubePreamp": "vintage_tube",
        "TapeEcho": "tape_echo",
        "PlateReverb": "plate_reverb",
        "ClassicCompressor": "classic_compressor",
        "ParametricEQ": "parametric_eq",
        "GranularCloud": "granular_cloud",
        "SpectralFreeze": "spectral_freeze",
        "MasteringLimiter": "mastering_limiter",
        "TransientShaper": "transient_shaper",
        "NoiseGate": "noise_gate",
        "MidSideProcessor": "mid_side_processor",
        "DimensionExpander": "dimension_expander",
        "StereoChorus": "stereo_chorus",
        "SpringReverb": "spring_reverb",
        "RodentDistortion": "rodent_distortion",
        "BufferRepeat": "buffer_repeat",
        "ChaosGenerator": "chaos_generator",
        "DynamicEQ": "dynamic_eq",
        "FeedbackNetwork": "feedback_network",
        "PitchShifter": "pitch_shifter",
        "DetuneDoubler": "detune_doubler",
        "StereoWidener": "stereo_widener",
        "ResonantChorus": "resonant_chorus",
        "HarmonicExciter": "harmonic_exciter",
        "VintageOptoCompressor": "opto_compressor",
        "DigitalDelay": "digital_delay",
        "ClassicTremolo": "classic_tremolo",
        "HarmonicTremolo": "harmonic_tremolo",
        "MuffFuzz": "fuzz_face",
        "CombResonator": "comb_resonator",
        "EnvelopeFilter": "envelope_filter",
        "GatedReverb": "gated_reverb",
        "PhasedVocoder": "phased_vocoder",
        "SpectralGate": "spectral_gate",
        "StereoImager": "stereo_imager",
        "VintageConsoleEQ": "vintage_console_eq"
    }
    
    results = []
    
    for class_name, string_id in engines.items():
        header_path = base_path / f"{class_name}.h"
        cpp_path = base_path / f"{class_name}.cpp"
        
        features, score = analyze_implementation_quality(str(header_path), str(cpp_path))
        
        # Count missing features
        missing_features = [name for name, present in features.items() if not present]
        
        results.append({
            'name': string_id,
            'class': class_name,
            'score': score,
            'features': features,
            'missing': missing_features
        })
    
    # Sort by score
    results.sort(key=lambda x: x['score'], reverse=True)
    
    print("=" * 100)
    print("ENGINE RANKING BY INTELLIGENTHARMONIZER STANDARDS")
    print("=" * 100)
    print()
    
    # Group by quality tiers
    excellent = [r for r in results if r['score'] >= 75]
    good = [r for r in results if 50 <= r['score'] < 75]
    moderate = [r for r in results if 25 <= r['score'] < 50]
    poor = [r for r in results if r['score'] < 25]
    
    if excellent:
        print("🏆 EXCELLENT (75-100% of IntelligentHarmonizer features)")
        print("-" * 50)
        for engine in excellent:
            print(f"  {engine['name']:25} {engine['score']:5.1f}%")
            key_features = [k for k, v in engine['features'].items() if v and k in [
                "Granular synthesis", "FFT/Spectral", "Multiple processing voices", 
                "Lock-free design", "SIMD optimization"
            ]]
            if key_features:
                print(f"    Key features: {', '.join(key_features[:3])}")
        print()
    
    if good:
        print("✅ GOOD (50-74% of features)")
        print("-" * 50)
        for engine in good:
            print(f"  {engine['name']:25} {engine['score']:5.1f}%")
            critical_missing = [m for m in engine['missing'] if m in [
                "Thread-safe smoothing", "DC blocking", "Parameter validation"
            ]]
            if critical_missing:
                print(f"    Missing critical: {', '.join(critical_missing[:3])}")
        print()
    
    if moderate:
        print("⚠️ MODERATE (25-49% of features)")
        print("-" * 50)
        for engine in moderate:
            print(f"  {engine['name']:25} {engine['score']:5.1f}%")
            print(f"    Missing {len(engine['missing'])} features")
        print()
    
    if poor:
        print("❌ POOR (<25% of features) - NEED COMPLETE REWRITE")
        print("-" * 50)
        for engine in poor:
            print(f"  {engine['name']:25} {engine['score']:5.1f}%")
            has_features = [k for k, v in engine['features'].items() if v]
            if has_features:
                print(f"    Only has: {', '.join(has_features[:3])}")
            else:
                print(f"    Missing ALL professional features!")
        print()
    
    # Statistics
    print("=" * 100)
    print("STATISTICS VS INTELLIGENTHARMONIZER")
    print("=" * 100)
    avg_score = sum(r['score'] for r in results) / len(results)
    print(f"IntelligentHarmonizer Score: {harmonizer_score:.1f}%")
    print(f"Average Other Engine Score:  {avg_score:.1f}%")
    print(f"Quality Gap:                 {harmonizer_score - avg_score:.1f}%")
    print()
    
    # Most common missing features
    all_missing = {}
    for result in results:
        for feature in result['missing']:
            all_missing[feature] = all_missing.get(feature, 0) + 1
    
    sorted_missing = sorted(all_missing.items(), key=lambda x: x[1], reverse=True)
    
    print("MOST COMMONLY MISSING FEATURES:")
    print("-" * 50)
    for feature, count in sorted_missing[:10]:
        percentage = (count / len(results)) * 100
        print(f"  {percentage:5.1f}% lack: {feature}")
    
    print()
    print("=" * 100)
    print("UPGRADE PRIORITIES")
    print("=" * 100)
    print()
    print("To match IntelligentHarmonizer quality, prioritize adding:")
    print()
    print("1. CRITICAL (Safety & Stability):")
    print("   - Thread-safe parameter smoothing")
    print("   - DC blocking")
    print("   - Parameter validation")
    print("   - Proper initialization")
    print()
    print("2. PROFESSIONAL (Audio Quality):")
    print("   - Anti-aliasing/Oversampling")
    print("   - Interpolation")
    print("   - Window functions")
    print("   - Circular buffering")
    print()
    print("3. ADVANCED (Standout Features):")
    print("   - Multiple processing voices")
    print("   - Spectral/FFT processing")
    print("   - Humanization")
    print("   - Pitch detection")
    print()
    print("4. OPTIMIZATION (Performance):")
    print("   - Pre-allocated buffers")
    print("   - Lock-free design")
    print("   - SIMD optimization")
    print("   - Efficient algorithms")

if __name__ == "__main__":
    main()