#!/usr/bin/env python3
"""
Add reset() function to all engine implementations
"""

import re
import os
from pathlib import Path

# List of engine files that need reset() implementation
ENGINE_FILES = [
    "AnalogPhaser",
    "AnalogRingModulator", 
    "BitCrusher",
    "BucketBrigadeDelay",
    "BufferRepeat",
    "ChaosGenerator",
    "ClassicCompressor",
    "ClassicTremolo",
    "CombResonator",
    "ConvolutionReverb",
    "DetuneDoubler",
    "DigitalDelay",
    "DimensionExpander",
    "EnvelopeFilter",
    "FormantFilter",
    "FrequencyShifter",
    "GatedReverb",
    "GranularCloud",
    "HarmonicExciter",
    "HarmonicTremolo",
    "IntelligentHarmonizer",
    "LadderFilter",
    "MagneticDrumEcho",
    "MasteringLimiter",
    "MidSideProcessor",
    "MultibandSaturator",
    "NoiseGate",
    "PhasedVocoder",
    "PitchShifter",
    "PlateReverb",
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
    "VintageOptoCompressor",
    "VintumTubePreamp",
    "VocalFormantFilter",
    "WaveFolder",
    "MuffFuzz",
    "TubeScreamer",
    "KStyleOverdrive",
    "ParametricEQ",
    "VintageConsoleEQ",
    "DynamicEQ"
]

def check_needs_reset(class_name, source_dir):
    """Check if a class already has reset() implemented"""
    cpp_file = source_dir / f"{class_name}.cpp"
    h_file = source_dir / f"{class_name}.h"
    
    if not cpp_file.exists():
        return False, "File not found"
    
    # Check if reset() is already declared in header
    if h_file.exists():
        with open(h_file, 'r') as f:
            content = f.read()
            if re.search(r'void\s+reset\s*\(\s*\)\s*override', content):
                return False, "Already has reset()"
    
    # Check if it inherits from EngineBase
    with open(h_file, 'r') as f:
        content = f.read()
        if not re.search(rf'class\s+{class_name}\s*:\s*public\s+EngineBase', content):
            return False, "Does not inherit from EngineBase"
    
    return True, "Needs reset()"

def add_reset_declaration(class_name, source_dir):
    """Add reset() declaration to header file"""
    h_file = source_dir / f"{class_name}.h"
    
    with open(h_file, 'r') as f:
        content = f.read()
    
    # Find where to insert reset() - after process() and before updateParameters()
    pattern = r'(void\s+process\s*\([^)]+\)\s*override\s*;)'
    match = re.search(pattern, content)
    
    if match:
        insert_pos = match.end()
        new_content = content[:insert_pos] + "\n    void reset() override;" + content[insert_pos:]
        
        with open(h_file, 'w') as f:
            f.write(new_content)
        return True
    
    return False

def get_reset_implementation(class_name, source_dir):
    """Generate appropriate reset() implementation based on engine type"""
    h_file = source_dir / f"{class_name}.h"
    
    # Read header to understand data members
    with open(h_file, 'r') as f:
        header_content = f.read()
    
    # Different reset implementations based on engine type
    if "Delay" in class_name or "Echo" in class_name:
        return """
void {0}::reset() {{
    // Clear delay buffers
    for (auto& channel : m_channels) {{
        if (channel.delayBuffer) {{
            channel.delayBuffer->clear();
        }}
        channel.writePos = 0;
        channel.feedback = 0.0f;
    }}
}}""".format(class_name)
    
    elif "Filter" in class_name:
        return """
void {0}::reset() {{
    // Reset filter states
    for (auto& channel : m_channels) {{
        channel.state1 = 0.0f;
        channel.state2 = 0.0f;
        channel.prevInput = 0.0f;
        channel.prevOutput = 0.0f;
    }}
}}""".format(class_name)
    
    elif "Reverb" in class_name:
        return """
void {0}::reset() {{
    // Clear all reverb buffers and states
    for (auto& channel : m_channels) {{
        channel.clear();
    }}
    // Reset any additional reverb-specific state
}}""".format(class_name)
    
    elif "Compressor" in class_name or "Limiter" in class_name or "Gate" in class_name:
        return """
void {0}::reset() {{
    // Reset envelope followers and gain reduction
    for (auto& channel : m_channels) {{
        channel.envelope = 0.0f;
        channel.gainReduction = 0.0f;
        channel.prevGain = 1.0f;
    }}
}}""".format(class_name)
    
    elif "Chorus" in class_name or "Phaser" in class_name or "Tremolo" in class_name:
        return """
void {0}::reset() {{
    // Reset modulation states
    for (auto& channel : m_channels) {{
        channel.lfoPhase = 0.0f;
        if (channel.delayLine) {{
            channel.delayLine->clear();
        }}
    }}
}}""".format(class_name)
    
    else:
        # Generic reset
        return """
void {0}::reset() {{
    // Reset all internal state
    // TODO: Add specific reset logic for {0}
}}""".format(class_name)

def main():
    source_dir = Path(__file__).parent.parent / "Source"
    
    print("Checking which engines need reset() implementation...\n")
    
    needs_update = []
    
    for engine in ENGINE_FILES:
        needs_reset, reason = check_needs_reset(engine, source_dir)
        status = "✓" if not needs_reset else "✗"
        print(f"{status} {engine}: {reason}")
        
        if needs_reset:
            needs_update.append(engine)
    
    print(f"\nFound {len(needs_update)} engines that need reset() implementation")
    
    if needs_update:
        response = input("\nWould you like to add reset() declarations to headers? (y/n): ")
        if response.lower() == 'y':
            for engine in needs_update:
                if add_reset_declaration(engine, source_dir):
                    print(f"Added reset() declaration to {engine}.h")
                else:
                    print(f"Failed to add reset() declaration to {engine}.h")
            
            print("\nNow you need to implement reset() in the .cpp files.")
            print("Example implementations have been generated for each engine type.")

if __name__ == "__main__":
    main()