#!/usr/bin/env python3
"""
Batch add reset() to remaining engines
"""

import re
from pathlib import Path

# Remaining engines that need reset()
REMAINING_ENGINES = [
    "AnalogRingModulator",
    "BitCrusher", 
    "BucketBrigadeDelay",
    "BufferRepeat",
    "ChaosGenerator",
    "ClassicTremolo",
    "CombResonator",
    "ConvolutionReverb",
    "DetuneDoubler",
    "DigitalDelay",
    "DimensionExpander",
    "DynamicEQ",
    "EnvelopeFilter",
    "FormantFilter",
    "FrequencyShifter",
    "GatedReverb",
    "GranularCloud",
    "HarmonicExciter",
    "HarmonicTremolo",
    "IntelligentHarmonizer",
    "KStyleOverdrive",
    "MagneticDrumEcho",
    "MasteringLimiter",
    "MidSideProcessor",
    "MuffFuzz",
    "MultibandSaturator",
    "NoiseGate",
    "ParametricEQ",
    "PhasedVocoder",
    "PitchShifter",
    "ResonantChorus",
    "RodentDistortion",
    "RotarySpeaker",
    "ShimmerReverb",
    "SpectralFreeze",
    "SpectralGate",
    "SpringReverb",
    "StateVariableFilter",
    "StereoChorus",
    "StereoImager",
    "StereoWidener",
    "TransientShaper",
    "VintageConsoleEQ",
    "VintageOptoCompressor",
    "VintageTubePreamp",
    "VocalFormantFilter"
]

def add_reset_to_header(engine_name, source_dir):
    """Add reset() declaration to header"""
    h_file = source_dir / f"{engine_name}.h"
    
    if not h_file.exists():
        print(f"Warning: {h_file} not found")
        return False
        
    with open(h_file, 'r') as f:
        content = f.read()
    
    # Check if already has reset
    if re.search(r'void\s+reset\s*\(\s*\)\s*override', content):
        print(f"{engine_name} already has reset()")
        return False
    
    # Find where to insert - after process()
    pattern = r'(void\s+process\s*\([^)]+\)\s*override\s*;)'
    match = re.search(pattern, content)
    
    if not match:
        print(f"Could not find process() in {engine_name}")
        return False
    
    insert_pos = match.end()
    new_content = content[:insert_pos] + "\n    void reset() override;" + content[insert_pos:]
    
    with open(h_file, 'w') as f:
        f.write(new_content)
    
    return True

def add_reset_to_cpp(engine_name, source_dir):
    """Add reset() implementation to cpp"""
    cpp_file = source_dir / f"{engine_name}.cpp"
    h_file = source_dir / f"{engine_name}.h"
    
    if not cpp_file.exists():
        print(f"Warning: {cpp_file} not found")
        return False
    
    # Read header to understand structure
    reset_impl = generate_reset_implementation(engine_name, h_file)
    
    with open(cpp_file, 'r') as f:
        content = f.read()
    
    # Check if already has reset
    if re.search(rf'void\s+{engine_name}::reset\s*\(\s*\)', content):
        print(f"{engine_name} already has reset() implementation")
        return False
    
    # Find a good place to insert - after prepareToPlay or process
    pattern = rf'(void\s+{engine_name}::(?:prepareToPlay|process)\s*\([^{{]+\{{[^}}]+\}})'
    match = re.search(pattern, content, re.DOTALL)
    
    if match:
        insert_pos = match.end()
        new_content = content[:insert_pos] + "\n" + reset_impl + content[insert_pos:]
        
        with open(cpp_file, 'w') as f:
            f.write(new_content)
        return True
    
    return False

def generate_reset_implementation(engine_name, h_file):
    """Generate appropriate reset implementation"""
    
    # Read header to check for common patterns
    with open(h_file, 'r') as f:
        header = f.read()
    
    # Check for common member patterns
    has_channels = "m_channels" in header or "m_channelStates" in header
    has_buffers = "buffer" in header.lower() or "delay" in header.lower()
    has_filters = "filter" in header.lower()
    has_lfo = "lfo" in header.lower() or "phase" in header.lower()
    
    impl = f"\nvoid {engine_name}::reset() {{\n"
    
    if "Delay" in engine_name or "Echo" in engine_name:
        impl += """    // Clear delay buffers and reset indices
    for (auto& channel : m_channelStates) {
        channel.delayBuffer.clear();
        channel.writeIndex = 0;
        channel.readIndex = 0;
        channel.feedback = 0.0f;
    }
"""
    elif "Filter" in engine_name:
        impl += """    // Reset filter states
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
"""
    elif "Reverb" in engine_name:
        impl += """    // Clear all reverb buffers
    for (auto& channel : m_channelStates) {
        channel.clear();
    }
    // Reset any additional reverb state
"""
    elif "Compressor" in engine_name or "Limiter" in engine_name or "Gate" in engine_name:
        impl += """    // Reset dynamics processing state
    for (auto& channel : m_channelStates) {
        channel.envelope = 0.0f;
        channel.gainReduction = 0.0f;
    }
"""
    elif "Chorus" in engine_name or "Phaser" in engine_name or "Tremolo" in engine_name:
        impl += """    // Reset modulation state
    m_lfoPhase = 0.0f;
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
"""
    elif "Distortion" in engine_name or "Overdrive" in engine_name or "Fuzz" in engine_name:
        impl += """    // Reset distortion state
    for (auto& channel : m_channelStates) {
        channel.reset();
    }
"""
    else:
        # Generic implementation
        impl += """    // Reset all internal state
    // TODO: Implement specific reset logic for """ + engine_name + """
"""
    
    impl += "}\n"
    return impl

def main():
    source_dir = Path(__file__).parent.parent / "Source"
    
    print("Adding reset() to remaining engines...\n")
    
    success_count = 0
    
    for engine in REMAINING_ENGINES:
        print(f"Processing {engine}...")
        
        # Add to header
        if add_reset_to_header(engine, source_dir):
            print(f"  ✓ Added reset() declaration to {engine}.h")
            
            # Add to cpp
            if add_reset_to_cpp(engine, source_dir):
                print(f"  ✓ Added reset() implementation to {engine}.cpp")
                success_count += 1
            else:
                print(f"  ✗ Failed to add implementation to {engine}.cpp")
        else:
            print(f"  - Skipped {engine}")
    
    print(f"\nSuccessfully added reset() to {success_count} engines")

if __name__ == "__main__":
    main()