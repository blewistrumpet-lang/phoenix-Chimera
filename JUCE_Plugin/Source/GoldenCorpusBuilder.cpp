#include "PresetManager.h"
#include "BoutiquePresetGenerator.h"

/**
 * Example implementation of the first 10 reference standard presets
 * These serve as the gold standard for the entire corpus
 */

namespace GoldenCorpusBuilder {

// Helper to create a preset with full manual control
std::unique_ptr<GoldenPreset> createManualPreset(
    const String& id,
    const String& name,
    const String& technicalHint,
    const String& category,
    const String& subcategory) {
    
    auto preset = std::make_unique<GoldenPreset>();
    preset->id = id;
    preset->name = name;
    preset->technicalHint = technicalHint;
    preset->shortCode = id.substring(3);
    preset->category = category;
    preset->subcategory = subcategory;
    preset->version = 1;
    preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
    preset->signature = "Chimera Phoenix Team";
    
    return preset;
}

//==============================================================================
// PRESET 1: "Velvet Thunder" - The flagship vocal preset

std::unique_ptr<GoldenPreset> createVelvetThunder() {
    auto preset = createManualPreset(
        "GC_001",
        "Velvet Thunder",
        "Vintage Tube + Tape Echo",
        "Studio Essentials",
        "Vocal Processing"
    );
    
    // Engine 1: Vintage Tube Preamp - Warm foundation
    preset->engineTypes[0] = ENGINE_VINTAGE_TUBE_PREAMP;
    preset->engineMix[0] = 1.0f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.35f,  // Drive - gentle warmth
        0.65f,  // Bias - asymmetric for 2nd harmonic richness  
        0.45f,  // Tone - slightly warm
        0.7f,   // Age - vintage character
        0.0f    // Noise - clean
    };
    
    // Engine 2: Tape Echo - Spatial depth
    preset->engineTypes[1] = ENGINE_TAPE_ECHO;
    preset->engineMix[1] = 0.3f;  // Subtle blend
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.125f, // Time - slapback (125ms)
        0.25f,  // Feedback - single repeat
        0.6f,   // Tone - warm echoes
        0.4f,   // Wow/Flutter - subtle movement
        0.5f,   // Saturation - tape compression
        0.6f    // Age - worn tape
    };
    
    // Engine 3: Parametric EQ - Polish
    preset->engineTypes[2] = ENGINE_PARAMETRIC_EQ;
    preset->engineMix[2] = 1.0f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.85f,  // HF Freq - 12kHz air
        0.6f,   // HF Gain - gentle lift
        0.3f,   // HF Q - broad
        0.65f,  // MF Freq - 5kHz presence
        0.55f,  // MF Gain - slight boost
        0.5f,   // MF Q - medium
        0.2f,   // LF Freq - 200Hz
        0.45f   // LF Gain - slight cut for clarity
    };
    
    // Metadata
    preset->cpuTier = CPUTier::LIGHT;
    preset->actualCpuPercent = 2.8f;
    preset->latencySamples = 64.0f;
    preset->realtimeSafe = true;
    
    // Sonic profile
    preset->sonicProfile.brightness = 0.7f;
    preset->sonicProfile.density = 0.4f;
    preset->sonicProfile.movement = 0.3f;
    preset->sonicProfile.space = 0.4f;
    preset->sonicProfile.aggression = 0.1f;
    preset->sonicProfile.vintage = 0.7f;
    
    // Emotional profile
    preset->emotionalProfile.energy = 0.6f;
    preset->emotionalProfile.mood = 0.7f;
    preset->emotionalProfile.tension = 0.2f;
    preset->emotionalProfile.organic = 0.8f;
    preset->emotionalProfile.nostalgia = 0.6f;
    
    // Source affinity
    preset->sourceAffinity.vocals = 1.0f;
    preset->sourceAffinity.guitar = 0.6f;
    preset->sourceAffinity.drums = 0.2f;
    preset->sourceAffinity.synth = 0.5f;
    preset->sourceAffinity.mix = 0.3f;
    
    // Keywords
    preset->keywords = {
        "vocal", "warm", "vintage", "tube", "tape", "echo", "smooth",
        "professional", "polish", "air", "presence", "analog", "classic"
    };
    
    // User prompts
    preset->userPrompts = {
        "Make my vocals sound warm and professional",
        "Add vintage character to my voice",
        "Classic vocal sound with subtle echo",
        "Warm tube vocals with air"
    };
    
    preset->complexity = 0.5f;
    preset->experimentalness = 0.2f;
    preset->versatility = 0.7f;
    
    return preset;
}

//==============================================================================
// PRESET 2: "Crystal Palace" - Ethereal space designer

std::unique_ptr<GoldenPreset> createCrystalPalace() {
    auto preset = createManualPreset(
        "GC_002",
        "Crystal Palace",
        "Shimmer Reverb + Dimension Expander",
        "Spatial Design",
        "Ethereal Spaces"
    );
    
    // Engine 1: Shimmer Reverb - Main space
    preset->engineTypes[0] = ENGINE_SHIMMER_REVERB;
    preset->engineMix[0] = 0.7f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.85f,  // Size - vast hall
        0.9f,   // Decay - very long
        0.5f,   // Shimmer - octave up blend
        0.3f,   // Damping - bright reflections
        0.8f,   // Diffusion - smooth
        0.6f,   // Modulation - gentle movement
        0.7f,   // Pre-delay
        0.8f    // Width
    };
    
    // Engine 2: Dimension Expander - Extra width
    preset->engineTypes[1] = ENGINE_DIMENSION_EXPANDER;
    preset->engineMix[1] = 0.5f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.8f,   // Width - expansive
        0.6f,   // Depth - 3D placement
        0.3f,   // Movement - subtle animation
        0.7f,   // Focus - maintain center
        0.5f    // Phase coherence
    };
    
    // Engine 3: Harmonic Exciter - Sparkle
    preset->engineTypes[2] = ENGINE_HARMONIC_EXCITER;
    preset->engineMix[2] = 0.2f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.3f,   // Drive - gentle
        0.8f,   // Frequency - high frequencies
        0.6f,   // Mix - subtle blend
        0.9f,   // Quality - smooth harmonics
        0.0f    // Odd/even balance
    };
    
    // Metadata
    preset->cpuTier = CPUTier::MEDIUM;
    preset->actualCpuPercent = 5.2f;
    preset->latencySamples = 512.0f;
    preset->realtimeSafe = true;
    
    // Sonic profile
    preset->sonicProfile.brightness = 0.8f;
    preset->sonicProfile.density = 0.7f;
    preset->sonicProfile.movement = 0.6f;
    preset->sonicProfile.space = 0.95f;
    preset->sonicProfile.aggression = 0.0f;
    preset->sonicProfile.vintage = 0.2f;
    
    // Keywords
    preset->keywords = {
        "ethereal", "shimmer", "space", "reverb", "ambient", "crystal",
        "expansive", "dreamy", "floating", "celestial", "wide", "atmospheric"
    };
    
    preset->sourceAffinity.vocals = 0.8f;
    preset->sourceAffinity.guitar = 0.7f;
    preset->sourceAffinity.drums = 0.3f;
    preset->sourceAffinity.synth = 0.9f;
    preset->sourceAffinity.mix = 0.6f;
    
    return preset;
}

//==============================================================================
// PRESET 3: "Broken Radio" - Lo-fi character

std::unique_ptr<GoldenPreset> createBrokenRadio() {
    auto preset = createManualPreset(
        "GC_003",
        "Broken Radio",
        "Bit Crusher + Analog Filter + Spring Reverb",
        "Character & Color",
        "Lo-Fi Processing"
    );
    
    // Engine 1: Bit Crusher - Digital degradation
    preset->engineTypes[0] = ENGINE_BIT_CRUSHER;
    preset->engineMix[0] = 0.6f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.4f,   // Bit depth - 8-10 bits
        0.3f,   // Sample rate - moderate reduction
        0.5f,   // Dither
        0.7f,   // Output filter
        0.2f    // Aliasing
    };
    
    // Engine 2: Ladder Filter - Radio bandwidth
    preset->engineTypes[1] = ENGINE_LADDER_FILTER;
    preset->engineMix[1] = 1.0f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.3f,   // Cutoff - telephone band
        0.6f,   // Resonance - peaked response
        0.3f,   // Drive - slight distortion
        0.0f,   // Type - lowpass
        0.4f,   // Envelope
        0.5f    // Keyboard tracking
    };
    
    // Engine 3: Spring Reverb - Cheap reverb tank
    preset->engineTypes[2] = ENGINE_SPRING_REVERB;
    preset->engineMix[2] = 0.4f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.4f,   // Size - small spring
        0.3f,   // Decay - short
        0.7f,   // Twang - spring character
        0.6f,   // Damping
        0.8f    // Shake - spring artifacts
    };
    
    preset->cpuTier = CPUTier::LIGHT;
    preset->actualCpuPercent = 2.1f;
    
    preset->keywords = {
        "lofi", "radio", "vintage", "degraded", "character", "retro",
        "crushed", "filtered", "nostalgic", "am", "transistor", "broken"
    };
    
    return preset;
}

//==============================================================================
// PRESET 4: "Pulse Engine" - Rhythmic motion designer

std::unique_ptr<GoldenPreset> createPulseEngine() {
    auto preset = createManualPreset(
        "GC_004",
        "Pulse Engine",
        "Harmonic Tremolo + Analog Phaser + Delay",
        "Motion & Modulation",
        "Rhythmic Processing"
    );
    
    // Engine 1: Harmonic Tremolo - Main rhythm
    preset->engineTypes[0] = ENGINE_HARMONIC_TREMOLO;
    preset->engineMix[0] = 1.0f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.5f,   // Rate - tempo sync
        0.7f,   // Depth - pronounced
        0.6f,   // Harmonic blend
        0.3f,   // Shape - slightly squared
        1.0f,   // Sync - on
        0.25f,  // Division - 1/8 notes
        0.0f    // Phase
    };
    
    // Engine 2: Analog Phaser - Sweeping motion
    preset->engineTypes[1] = ENGINE_ANALOG_PHASER;
    preset->engineMix[1] = 0.5f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.3f,   // Rate - slow sweep
        0.6f,   // Depth
        0.5f,   // Feedback
        0.5f,   // Stages - 4-stage
        0.6f,   // Center frequency
        0.4f    // Stereo spread
    };
    
    // Engine 3: Digital Delay - Polyrhythm
    preset->engineTypes[2] = ENGINE_DIGITAL_DELAY;
    preset->engineMix[2] = 0.4f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.666f, // Time - dotted 1/8
        0.5f,   // Feedback
        0.6f,   // High cut
        0.3f,   // Low cut
        1.0f,   // Sync
        0.0f,   // Modulation
        0.7f    // Width
    };
    
    preset->cpuTier = CPUTier::MEDIUM;
    preset->actualCpuPercent = 4.5f;
    
    return preset;
}

//==============================================================================
// PRESET 5: "Gravity Well" - Experimental sound design

std::unique_ptr<GoldenPreset> createGravityWell() {
    auto preset = createManualPreset(
        "GC_005",
        "Gravity Well",
        "Feedback Network + Spectral Freeze + Ring Mod",
        "Experimental Laboratory",
        "Sound Design"
    );
    
    // Complex 4-engine configuration
    preset->engineTypes[0] = ENGINE_FEEDBACK_NETWORK;
    preset->engineMix[0] = 0.8f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.7f,   // Feedback amount
        0.4f,   // Delay time
        0.6f,   // Filter frequency
        0.8f,   // Resonance
        0.5f,   // Modulation
        0.3f    // Chaos
    };
    
    preset->engineTypes[1] = ENGINE_SPECTRAL_FREEZE;
    preset->engineMix[1] = 0.6f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.0f,   // Freeze threshold
        0.7f,   // Spectral smear
        0.5f,   // Fade time
        0.8f,   // Resolution
        0.4f    // Frequency shift
    };
    
    preset->engineTypes[2] = ENGINE_ANALOG_RING_MODULATOR;
    preset->engineMix[2] = 0.4f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.15f,  // Frequency - low for rumble
        0.0f,   // Fine tune
        0.6f,   // Mix
        0.3f,   // Shape
        0.7f    // Carrier blend
    };
    
    preset->engineTypes[3] = ENGINE_CONVOLUTION_REVERB;
    preset->engineMix[3] = 0.5f;
    preset->engineActive[3] = true;
    preset->engineParams[3] = {
        0.8f,   // IR selection - unusual space
        0.7f,   // Size
        0.4f,   // Damping
        0.3f,   // Pre-delay
        0.6f    // ER/LR balance
    };
    
    preset->cpuTier = CPUTier::HEAVY;
    preset->actualCpuPercent = 9.8f;
    preset->experimentalness = 0.9f;
    
    return preset;
}

//==============================================================================
// PRESET 6: "Console 73" - Classic mixing desk

std::unique_ptr<GoldenPreset> createConsole73() {
    auto preset = createManualPreset(
        "GC_006",
        "Console 73",
        "Vintage Console EQ + Opto Compressor + Tape Saturation",
        "Studio Essentials",
        "Channel Strips"
    );
    
    // Recreate famous console sound
    preset->engineTypes[0] = ENGINE_VINTAGE_CONSOLE_EQ;
    preset->engineMix[0] = 1.0f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.6f,   // Low shelf - 110Hz boost
        0.5f,   // Low-mid - flat
        0.45f,  // High-mid - slight 3kHz cut  
        0.65f,  // High shelf - 10kHz lift
        0.6f    // Output transformer drive
    };
    
    preset->engineTypes[1] = ENGINE_VINTAGE_OPTO_COMPRESSOR;
    preset->engineMix[1] = 1.0f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.4f,   // Threshold
        0.4f,   // Ratio - 4:1
        0.3f,   // Attack - medium
        0.5f,   // Release - auto
        0.7f,   // Makeup
        0.7f,   // Vintage character
        0.0f    // Sidechain
    };
    
    preset->engineTypes[2] = ENGINE_TAPE_ECHO;
    preset->engineMix[2] = 0.0f;  // Available but off by default
    preset->engineActive[2] = false;
    preset->engineParams[2] = {
        0.375f, // Time
        0.3f,   // Feedback
        0.5f,   // Tone
        0.4f,   // Wow/Flutter
        0.6f    // Saturation
    };
    
    preset->cpuTier = CPUTier::LIGHT;
    preset->actualCpuPercent = 2.5f;
    preset->versatility = 0.9f;
    
    return preset;
}

//==============================================================================
// PRESET 7: "Infinite Cathedral" - Massive space creator

std::unique_ptr<GoldenPreset> createInfiniteCathedral() {
    auto preset = createManualPreset(
        "GC_007",
        "Infinite Cathedral",
        "Convolution Reverb + Pitch Shifter",
        "Spatial Design",
        "Epic Spaces"
    );
    
    preset->engineTypes[0] = ENGINE_CONVOLUTION_REVERB;
    preset->engineMix[0] = 0.8f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.9f,   // IR - cathedral
        0.8f,   // Size stretch
        0.3f,   // Damping
        0.1f,   // Pre-delay
        0.7f,   // ER/LR balance
        0.9f    // Width
    };
    
    preset->engineTypes[1] = ENGINE_PITCH_SHIFTER;
    preset->engineMix[1] = 0.3f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.583f, // Pitch - +7 semitones (perfect 5th)
        0.0f,   // Formant preservation
        0.9f,   // Quality - best
        0.8f,   // Smoothing
        0.5f    // Stereo spread
    };
    
    preset->cpuTier = CPUTier::MEDIUM;
    preset->actualCpuPercent = 6.8f;
    
    preset->sonicProfile.space = 1.0f;
    preset->sonicProfile.density = 0.9f;
    
    return preset;
}

//==============================================================================
// PRESET 8: "Analog Sunrise" - Warm enhancement

std::unique_ptr<GoldenPreset> createAnalogSunrise() {
    auto preset = createManualPreset(
        "GC_008", 
        "Analog Sunrise",
        "Harmonic Exciter + Stereo Chorus + Vintage Tube",
        "Character & Color",
        "Harmonic Enhancement"
    );
    
    preset->engineTypes[0] = ENGINE_HARMONIC_EXCITER;
    preset->engineMix[0] = 0.7f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.4f,   // Drive
        0.7f,   // Frequency - upper mids
        0.5f,   // Mix
        0.8f,   // Quality
        0.3f,   // Even harmonics
        0.7f    // Tube mode
    };
    
    preset->engineTypes[1] = ENGINE_STEREO_CHORUS;
    preset->engineMix[1] = 0.5f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.2f,   // Rate - slow
        0.4f,   // Depth
        0.3f,   // Delay
        0.6f,   // Feedback
        0.8f,   // Width
        0.5f    // Mix
    };
    
    preset->engineTypes[2] = ENGINE_VINTAGE_TUBE_PREAMP;
    preset->engineMix[2] = 1.0f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.5f,   // Drive
        0.6f,   // Bias
        0.6f,   // Tone - warm
        0.8f,   // Age
        0.0f    // Noise
    };
    
    return preset;
}

//==============================================================================
// PRESET 9: "Tidal Flow" - Organic movement

std::unique_ptr<GoldenPreset> createTidalFlow() {
    auto preset = createManualPreset(
        "GC_009",
        "Tidal Flow", 
        "Envelope Filter + Rotary Speaker + Bucket Brigade",
        "Motion & Modulation",
        "Organic Movement"
    );
    
    preset->engineTypes[0] = ENGINE_ENVELOPE_FILTER;
    preset->engineMix[0] = 0.8f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.3f,   // Sensitivity
        0.6f,   // Range
        0.5f,   // Attack
        0.7f,   // Release
        0.4f,   // Resonance
        0.0f    // Filter type - LP
    };
    
    preset->engineTypes[1] = ENGINE_ROTARY_SPEAKER;
    preset->engineMix[1] = 0.6f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.4f,   // Speed - slow
        0.7f,   // Depth
        0.5f,   // Horn/drum balance
        0.6f,   // Distance
        0.8f,   // Stereo spread
        0.3f    // Drive
    };
    
    preset->engineTypes[2] = ENGINE_BUCKET_BRIGADE_DELAY;
    preset->engineMix[2] = 0.4f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.25f,  // Delay time
        0.5f,   // Feedback
        0.4f,   // Filter
        0.6f,   // Modulation
        0.7f,   // BBD character
        0.5f    // Noise
    };
    
    preset->sonicProfile.movement = 0.9f;
    preset->sonicProfile.organic = 0.9f;
    
    return preset;
}

//==============================================================================
// PRESET 10: "Data Storm" - Glitch masterpiece

std::unique_ptr<GoldenPreset> createDataStorm() {
    auto preset = createManualPreset(
        "GC_010",
        "Data Storm",
        "Granular Cloud + Frequency Shifter + Buffer Repeat",
        "Experimental Laboratory", 
        "Glitch & IDM"
    );
    
    // Full 6-engine glitch machine
    preset->engineTypes[0] = ENGINE_GRANULAR_CLOUD;
    preset->engineMix[0] = 0.7f;
    preset->engineActive[0] = true;
    preset->engineParams[0] = {
        0.2f,   // Grain size - small
        0.8f,   // Density - thick
        0.5f,   // Position
        0.6f,   // Spread
        0.7f,   // Pitch variance
        0.3f,   // Reverse probability
        0.5f    // Envelope
    };
    
    preset->engineTypes[1] = ENGINE_FREQUENCY_SHIFTER;
    preset->engineMix[1] = 0.5f;
    preset->engineActive[1] = true;
    preset->engineParams[1] = {
        0.52f,  // Shift amount
        0.3f,   // Feedback
        0.6f,   // Mix
        0.0f,   // Shift direction
        0.8f    // Quality
    };
    
    preset->engineTypes[2] = ENGINE_BUFFER_REPEAT;
    preset->engineMix[2] = 0.6f;
    preset->engineActive[2] = true;
    preset->engineParams[2] = {
        0.125f, // Buffer size - 1/8
        0.7f,   // Repeat probability
        0.5f,   // Pitch shift
        0.8f,   // Stutter
        0.4f,   // Reverse
        1.0f    // Sync
    };
    
    preset->engineTypes[3] = ENGINE_BIT_CRUSHER;
    preset->engineMix[3] = 0.4f;
    preset->engineActive[3] = true;
    preset->engineParams[3] = {
        0.6f,   // Bit depth
        0.5f,   // Sample rate
        0.3f,   // Dither
        0.7f    // Filter
    };
    
    preset->engineTypes[4] = ENGINE_SPECTRAL_GATE;
    preset->engineMix[4] = 0.8f;
    preset->engineActive[4] = true;
    preset->engineParams[4] = {
        0.4f,   // Threshold
        0.7f,   // Tilt
        0.2f,   // Attack
        0.3f,   // Release
        0.8f    // Smoothing
    };
    
    preset->engineTypes[5] = ENGINE_DIGITAL_DELAY;
    preset->engineMix[5] = 0.3f;
    preset->engineActive[5] = true;
    preset->engineParams[5] = {
        0.333f, // Triplet delay
        0.6f,   // Feedback
        0.4f,   // Filter
        0.0f,   // Low cut
        1.0f,   // Sync
        0.3f    // Modulation
    };
    
    preset->cpuTier = CPUTier::HEAVY;
    preset->actualCpuPercent = 12.5f;
    preset->experimentalness = 1.0f;
    
    preset->keywords = {
        "glitch", "idm", "experimental", "granular", "buffer",
        "stutter", "complex", "rhythmic", "electronic", "cutting-edge"
    };
    
    return preset;
}

//==============================================================================
// Create all 10 reference presets

std::vector<std::unique_ptr<GoldenPreset>> createFirst10ReferencePresets() {
    std::vector<std::unique_ptr<GoldenPreset>> presets;
    
    presets.push_back(createVelvetThunder());
    presets.push_back(createCrystalPalace());
    presets.push_back(createBrokenRadio());
    presets.push_back(createPulseEngine());
    presets.push_back(createGravityWell());
    presets.push_back(createConsole73());
    presets.push_back(createInfiniteCathedral());
    presets.push_back(createAnalogSunrise());
    presets.push_back(createTidalFlow());
    presets.push_back(createDataStorm());
    
    // Validate and finalize each preset
    for (auto& preset : presets) {
        preset->updateComplexity();
        
        // Ensure all have proper metadata
        if (preset->bestFor.isEmpty()) {
            if (preset->sourceAffinity.vocals > 0.8f) {
                preset->bestFor = "Vocals";
            } else if (preset->sourceAffinity.mix > 0.8f) {
                preset->bestFor = "Full Mix";
            } else {
                preset->bestFor = "Universal";
            }
        }
        
        // Set quality score based on manual curation
        preset->qualityScore = 95.0f; // These are reference standard
    }
    
    return presets;
}

} // namespace GoldenCorpusBuilder