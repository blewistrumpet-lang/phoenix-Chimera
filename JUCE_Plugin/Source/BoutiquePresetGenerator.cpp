#include "BoutiquePresetGenerator.h"
#include <cmath>
#include <random>

//==============================================================================
// MAIN PRESET GENERATION

std::unique_ptr<GoldenPreset> BoutiquePresetGenerator::generatePreset(
    PresetArchetype archetype,
    const MusicalContext& context) {
    
    auto preset = std::make_unique<GoldenPreset>();
    
    // Generate base engine chain based on archetype
    EngineChain chain;
    
    switch (archetype) {
        case PresetArchetype::VocalPolish:
            chain = createVocalPolishChain();
            break;
        case PresetArchetype::MixGlue:
            chain = createMixGlueChain();
            break;
        case PresetArchetype::AnalogWarmth:
            chain = createAnalogWarmthChain();
            break;
        case PresetArchetype::DreamscapeAmbience:
            chain = createDreamscapeChain();
            break;
        case PresetArchetype::TapeNostalgia:
            chain = createTapeNostalgiaChain();
            break;
        case PresetArchetype::RhythmicPulse:
            chain = createRhythmicPulseChain();
            break;
        case PresetArchetype::GranularTexture:
            chain = createGranularTextureChain();
            break;
        default:
            chain = createMixGlueChain(); // Safe default
    }
    
    // Apply engine chain to preset
    for (size_t i = 0; i < chain.engines.size() && i < 6; ++i) {
        preset->engineTypes[i] = chain.engines[i];
        preset->engineMix[i] = chain.mixLevels[i];
        preset->engineActive[i] = true;
        
        // Generate parameters using the setter function
        std::vector<float> params(8, 0.5f); // Default to center
        if (i < chain.parameterSetters.size() && chain.parameterSetters[i]) {
            chain.parameterSetters[i](params);
        }
        preset->engineParams[i] = params;
    }
    
    // Generate metadata
    generateMetadata(*preset, archetype);
    generateSonicProfile(*preset, archetype);
    generateEmotionalProfile(*preset, archetype);
    generateKeywords(*preset, archetype);
    
    // Set creative name
    preset->name = generateCreativeName(archetype, context);
    
    // Musical optimization
    optimizePreset(*preset);
    ensureMusicalParameters(*preset);
    balanceFrequencySpectrum(*preset);
    
    // Update complexity
    preset->updateComplexity();
    
    return preset;
}

//==============================================================================
// VOCAL POLISH CHAIN - Professional vocal treatment

BoutiquePresetGenerator::EngineChain BoutiquePresetGenerator::createVocalPolishChain() {
    EngineChain chain;
    
    // 1. Vintage Opto Compressor - Gentle, musical compression
    chain.engines.push_back(ENGINE_VINTAGE_OPTO_COMPRESSOR);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.35f;  // Threshold - gentle compression
        params[1] = 0.3f;   // Ratio - 3:1
        params[2] = 0.2f;   // Attack - fast but musical
        params[3] = 0.4f;   // Release - auto-release feel
        params[4] = 0.7f;   // Makeup - slight boost
        params[5] = 0.6f;   // Vintage - some color
        params[6] = 0.8f;   // Sidechain HPF - ignore lows
    });
    
    // 2. Parametric EQ - Surgical and enhancement
    chain.engines.push_back(ENGINE_PARAMETRIC_EQ);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        // Air band
        params[0] = 0.85f;  // Frequency - 12kHz region
        params[1] = 0.6f;   // Gain - gentle lift
        params[2] = 0.3f;   // Q - broad
        // Presence
        params[3] = 0.65f;  // Frequency - 5kHz region
        params[4] = 0.55f;  // Gain - slight boost
        params[5] = 0.5f;   // Q - medium
    });
    
    // 3. Harmonic Exciter - Subtle brilliance
    chain.engines.push_back(ENGINE_HARMONIC_EXCITER);
    chain.mixLevels.push_back(0.15f); // Very subtle
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.3f;   // Drive - gentle
        params[1] = 0.7f;   // Frequency - high mids
        params[2] = 0.6f;   // Mix - blend carefully
        params[3] = 0.8f;   // Quality - smooth harmonics
    });
    
    // 4. Plate Reverb - Professional ambience
    chain.engines.push_back(ENGINE_PLATE_REVERB);
    chain.mixLevels.push_back(0.2f); // Subtle space
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.3f;   // Size - small plate
        params[1] = 0.4f;   // Decay - short
        params[2] = 0.7f;   // Damping - bright
        params[3] = 0.6f;   // Diffusion - smooth
        params[4] = 0.8f;   // Modulation - natural
    });
    
    return chain;
}

//==============================================================================
// MIX GLUE CHAIN - Cohesion without coloration

BoutiquePresetGenerator::EngineChain BoutiquePresetGenerator::createMixGlueChain() {
    EngineChain chain;
    
    // 1. Classic Compressor - VCA-style mix bus compression
    chain.engines.push_back(ENGINE_CLASSIC_COMPRESSOR);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.25f;  // Threshold - catch peaks
        params[1] = 0.2f;   // Ratio - 2:1 gentle
        params[2] = 0.5f;   // Attack - let transients through
        params[3] = 0.3f;   // Release - musical timing
        params[4] = 0.8f;   // Makeup - unity gain
        params[5] = 0.1f;   // Knee - soft knee
    });
    
    // 2. Vintage Console EQ - Mix bus sweetening
    chain.engines.push_back(ENGINE_VINTAGE_CONSOLE_EQ);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.2f;   // Low shelf - gentle foundation
        params[1] = 0.55f;  // Low-mid - slight cut at 300Hz
        params[2] = 0.45f;  // High-mid - slight cut at 2kHz
        params[3] = 0.6f;   // High shelf - air
        params[4] = 0.7f;   // Output - slight transformer push
    });
    
    // 3. Tape Saturation - Analog glue
    chain.engines.push_back(ENGINE_VINTAGE_TUBE_PREAMP);
    chain.mixLevels.push_back(0.3f); // Parallel processing
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.4f;   // Drive - warm not hot
        params[1] = 0.6f;   // Bias - even harmonics
        params[2] = 0.5f;   // Tone - neutral
        params[3] = 0.7f;   // Age - some vintage character
    });
    
    // 4. Dimension Expander - Width without phase issues
    chain.engines.push_back(ENGINE_DIMENSION_EXPANDER);
    chain.mixLevels.push_back(0.5f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.6f;   // Width - tasteful expansion
        params[1] = 0.4f;   // Depth - some front-back
        params[2] = 0.3f;   // Movement - subtle animation
        params[3] = 0.8f;   // Focus - maintain center
    });
    
    return chain;
}

//==============================================================================
// ANALOG WARMTH CHAIN - Vintage console sound

BoutiquePresetGenerator::EngineChain BoutiquePresetGenerator::createAnalogWarmthChain() {
    EngineChain chain;
    
    // 1. Vintage Tube Preamp - Input stage coloration
    chain.engines.push_back(ENGINE_VINTAGE_TUBE_PREAMP);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.45f;  // Drive - warm saturation
        params[1] = 0.65f;  // Bias - asymmetric for 2nd harmonic
        params[2] = 0.4f;   // Tone - slightly dark
        params[3] = 0.8f;   // Age - vintage character
        params[4] = 0.7f;   // Noise - authentic floor
    });
    
    // 2. Vintage Console EQ - Neve-style curves
    chain.engines.push_back(ENGINE_VINTAGE_CONSOLE_EQ);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.6f;   // Low shelf - 110Hz boost
        params[1] = 0.5f;   // Low-mid - flat
        params[2] = 0.45f;  // High-mid - slight 3kHz cut
        params[3] = 0.65f;  // High shelf - 10kHz lift
        params[4] = 0.6f;   // Output - transformer push
    });
    
    // 3. Tape Echo - Vintage delay with saturation
    chain.engines.push_back(ENGINE_TAPE_ECHO);
    chain.mixLevels.push_back(0.25f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.125f; // Time - slapback
        params[1] = 0.2f;   // Feedback - single repeat
        params[2] = 0.7f;   // Tone - warm repeats
        params[3] = 0.6f;   // Wow/Flutter - tape wobble
        params[4] = 0.5f;   // Saturation - tape compression
    });
    
    return chain;
}

//==============================================================================
// DREAMSCAPE CHAIN - Ethereal ambience

BoutiquePresetGenerator::EngineChain BoutiquePresetGenerator::createDreamscapeChain() {
    EngineChain chain;
    
    // 1. Shimmer Reverb - Ethereal space
    chain.engines.push_back(ENGINE_SHIMMER_REVERB);
    chain.mixLevels.push_back(0.7f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.8f;   // Size - vast space
        params[1] = 0.85f;  // Decay - long tail
        params[2] = 0.4f;   // Shimmer - octave up
        params[3] = 0.3f;   // Damping - let it ring
        params[4] = 0.7f;   // Diffusion - smooth
        params[5] = 0.6f;   // Modulation - movement
    });
    
    // 2. Analog Phaser - Sweeping motion
    chain.engines.push_back(ENGINE_ANALOG_PHASER);
    chain.mixLevels.push_back(0.4f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.15f;  // Rate - very slow
        params[1] = 0.7f;   // Depth - deep sweep
        params[2] = 0.6f;   // Feedback - resonant
        params[3] = 0.4f;   // Stages - 4-stage
        params[4] = 0.5f;   // Center frequency
    });
    
    // 3. Pitch Shifter - Subtle detuning
    chain.engines.push_back(ENGINE_PITCH_SHIFTER);
    chain.mixLevels.push_back(0.3f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.52f;  // Pitch - slight sharp (+5 cents)
        params[1] = 0.0f;   // Formant - preserve
        params[2] = 0.8f;   // Quality - best algorithm
        params[3] = 0.7f;   // Smoothing - no artifacts
    });
    
    // 4. Spectral Freeze - Texture layer
    chain.engines.push_back(ENGINE_SPECTRAL_FREEZE);
    chain.mixLevels.push_back(0.2f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.0f;   // Freeze - not frozen initially
        params[1] = 0.7f;   // Spectral blur
        params[2] = 0.6f;   // Fade time
        params[3] = 0.8f;   // Resolution
    });
    
    return chain;
}

//==============================================================================
// TAPE NOSTALGIA CHAIN - Authentic vintage tape

BoutiquePresetGenerator::EngineChain BoutiquePresetGenerator::createTapeNostalgiaChain() {
    EngineChain chain;
    
    // 1. Tape Echo - Main tape engine
    chain.engines.push_back(ENGINE_TAPE_ECHO);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.375f; // Time - classic 3/8 delay
        params[1] = 0.6f;   // Feedback - multiple repeats
        params[2] = 0.3f;   // Tone - dark repeats
        params[3] = 0.8f;   // Wow/Flutter - vintage instability
        params[4] = 0.7f;   // Saturation - tape compression
        params[5] = 0.6f;   // Age - worn tape
    });
    
    // 2. Muff Fuzz - Tape saturation to distortion
    chain.engines.push_back(ENGINE_MUFF_FUZZ);
    chain.mixLevels.push_back(0.2f); // Parallel dirt
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.3f;   // Sustain - mild breakup
        params[1] = 0.4f;   // Tone - rolled off highs
        params[2] = 0.8f;   // Volume - match level
    });
    
    // 3. Analog Filter - Tape head bump
    chain.engines.push_back(ENGINE_LADDER_FILTER);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.15f;  // Cutoff - low frequency bump
        params[1] = 0.4f;   // Resonance - gentle peak
        params[2] = 0.0f;   // Drive - clean
        params[3] = 0.0f;   // Filter type - lowpass
    });
    
    return chain;
}

//==============================================================================
// RHYTHMIC PULSE CHAIN - Tempo-synced motion

BoutiquePresetGenerator::EngineChain BoutiquePresetGenerator::createRhythmicPulseChain() {
    EngineChain chain;
    
    // 1. Classic Tremolo - Main rhythm
    chain.engines.push_back(ENGINE_CLASSIC_TREMOLO);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.5f;   // Rate - tempo sync
        params[1] = 0.6f;   // Depth - noticeable
        params[2] = 0.3f;   // Shape - slightly rounded
        params[3] = 1.0f;   // Sync - on
        params[4] = 0.25f;  // Division - 1/8 notes
    });
    
    // 2. Gated Reverb - Rhythmic space
    chain.engines.push_back(ENGINE_GATED_REVERB);
    chain.mixLevels.push_back(0.5f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.6f;   // Size - medium room
        params[1] = 0.1f;   // Gate time - tight
        params[2] = 0.7f;   // Threshold - responsive
        params[3] = 0.8f;   // Attack - instant
        params[4] = 0.5f;   // Hold - rhythmic
    });
    
    // 3. Digital Delay - Polyrhythmic layer
    chain.engines.push_back(ENGINE_DIGITAL_DELAY);
    chain.mixLevels.push_back(0.4f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.666f; // Time - dotted 1/8
        params[1] = 0.5f;   // Feedback - a few repeats
        params[2] = 0.7f;   // Filter - some HF damping
        params[3] = 1.0f;   // Sync - on
        params[4] = 0.0f;   // Modulation - stable
    });
    
    return chain;
}

//==============================================================================
// GRANULAR TEXTURE CHAIN - Experimental soundscapes

BoutiquePresetGenerator::EngineChain BoutiquePresetGenerator::createGranularTextureChain() {
    EngineChain chain;
    
    // 1. Granular Cloud - Main texture engine
    chain.engines.push_back(ENGINE_GRANULAR_CLOUD);
    chain.mixLevels.push_back(0.8f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.3f;   // Grain size - small
        params[1] = 0.7f;   // Density - thick texture
        params[2] = 0.5f;   // Position - centered
        params[3] = 0.4f;   // Spread - some randomness
        params[4] = 0.6f;   // Pitch variance
        params[5] = 0.5f;   // Reverse probability
    });
    
    // 2. Spectral Gate - Sculpting
    chain.engines.push_back(ENGINE_SPECTRAL_GATE);
    chain.mixLevels.push_back(1.0f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.4f;   // Threshold
        params[1] = 0.6f;   // Frequency tilt
        params[2] = 0.3f;   // Attack
        params[3] = 0.5f;   // Release
        params[4] = 0.7f;   // Spectral smoothing
    });
    
    // 3. Frequency Shifter - Inharmonic motion
    chain.engines.push_back(ENGINE_FREQUENCY_SHIFTER);
    chain.mixLevels.push_back(0.3f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.51f;  // Shift - slight upward
        params[1] = 0.0f;   // Feedback - none
        params[2] = 0.5f;   // Mix - balanced
    });
    
    // 4. Convolution Reverb - Space placement
    chain.engines.push_back(ENGINE_CONVOLUTION_REVERB);
    chain.mixLevels.push_back(0.4f);
    chain.parameterSetters.push_back([](std::vector<float>& params) {
        params[0] = 0.7f;   // IR selection - unusual space
        params[1] = 0.5f;   // Size
        params[2] = 0.6f;   // Damping
        params[3] = 0.4f;   // Pre-delay
    });
    
    return chain;
}

//==============================================================================
// PARAMETER GENERATION - Musical and acoustic principles

std::vector<float> BoutiquePresetGenerator::generateReverbParameters(const AcousticModel& model) {
    std::vector<float> params(8, 0.5f);
    
    // Size correlates with decay
    params[0] = model.roomSize;
    params[1] = model.decay * std::sqrt(model.roomSize); // Sabine equation approximation
    
    // Damping based on materials
    float materialDamping = model.woodResonance * 0.3f + 
                           model.metalResonance * 0.1f + 
                           model.airAbsorption * 0.6f;
    params[2] = 1.0f - materialDamping; // Invert for brightness
    
    // Diffusion and modulation
    params[3] = model.diffusion;
    params[4] = model.earlyReflections;
    params[5] = model.preDelay;
    
    // Natural variation
    params[6] = musicalRandom(0.4f, 0.6f); // Modulation amount
    params[7] = musicalRandom(0.5f, 0.7f); // Stereo width
    
    return params;
}

std::vector<float> BoutiquePresetGenerator::generateCompressorParameters(
    float ratio, float attack, float release) {
    
    std::vector<float> params(8, 0.5f);
    
    // Threshold based on ratio (higher ratios need lower thresholds)
    params[0] = 1.0f - (ratio * 0.7f);
    
    // Ratio (normalized where 0.0 = 1:1, 1.0 = inf:1)
    params[1] = ratio;
    
    // Attack and release with musical scaling
    params[2] = std::pow(attack, 2.0f);    // Exponential for better feel
    params[3] = std::pow(release, 1.5f);   // Slightly less exponential
    
    // Makeup gain approximation
    params[4] = 0.5f + (ratio * 0.3f);
    
    // Knee (softer for higher ratios)
    params[5] = 1.0f - (ratio * 0.5f);
    
    return params;
}

std::vector<float> BoutiquePresetGenerator::generateSaturationParameters(
    const HarmonicStructure& harmonics) {
    
    std::vector<float> params(8, 0.5f);
    
    // Drive based on total harmonic content
    float totalHarmonics = harmonics.evenHarmonics + harmonics.oddHarmonics;
    params[0] = juce::jlimit(0.1f, 0.9f, totalHarmonics * 0.6f);
    
    // Bias for even/odd balance
    params[1] = 0.5f + (harmonics.asymmetry * 0.5f);
    
    // Tone shaping
    float toneFreq = std::log10(harmonics.frequency / 20.0f) / std::log10(20000.0f / 20.0f);
    params[2] = juce::jlimit(0.0f, 1.0f, toneFreq);
    
    // Mix based on intermodulation
    params[3] = 1.0f - (harmonics.intermodulation * 0.5f);
    
    return params;
}

//==============================================================================
// MUSICAL RELATIONSHIPS

void BoutiquePresetGenerator::applyGoldenRatio(std::vector<float>& params) {
    const float phi = 1.618033988749f;
    const float invPhi = 0.618033988749f;
    
    // Apply golden ratio relationships between parameters
    for (size_t i = 1; i < params.size(); ++i) {
        if (i % 2 == 0) {
            // Even parameters influenced by previous
            params[i] = std::fmod(params[i-1] * phi, 1.0f);
        } else {
            // Odd parameters scaled by inverse
            params[i] = params[i] * invPhi + (1.0f - invPhi) * 0.5f;
        }
    }
}

void BoutiquePresetGenerator::applyPsychoacousticCurves(std::vector<float>& params) {
    // Apply Fletcher-Munson equal loudness curves
    // More sensitive in 2-5kHz range
    
    for (size_t i = 0; i < params.size(); ++i) {
        float freq = params[i]; // Assume normalized frequency
        
        // Boost presence region, reduce extremes
        if (freq > 0.3f && freq < 0.7f) {
            params[i] = freq * 1.2f;
        } else {
            params[i] = freq * 0.9f;
        }
        
        params[i] = juce::jlimit(0.0f, 1.0f, params[i]);
    }
}

//==============================================================================
// METADATA GENERATION

void BoutiquePresetGenerator::generateSonicProfile(GoldenPreset& preset, PresetArchetype archetype) {
    switch (archetype) {
        case PresetArchetype::VocalPolish:
            preset.sonicProfile.brightness = 0.7f;
            preset.sonicProfile.density = 0.4f;
            preset.sonicProfile.movement = 0.2f;
            preset.sonicProfile.space = 0.3f;
            preset.sonicProfile.aggression = 0.1f;
            preset.sonicProfile.vintage = 0.4f;
            break;
            
        case PresetArchetype::MixGlue:
            preset.sonicProfile.brightness = 0.5f;
            preset.sonicProfile.density = 0.6f;
            preset.sonicProfile.movement = 0.1f;
            preset.sonicProfile.space = 0.2f;
            preset.sonicProfile.aggression = 0.2f;
            preset.sonicProfile.vintage = 0.5f;
            break;
            
        case PresetArchetype::DreamscapeAmbience:
            preset.sonicProfile.brightness = 0.6f;
            preset.sonicProfile.density = 0.8f;
            preset.sonicProfile.movement = 0.7f;
            preset.sonicProfile.space = 0.9f;
            preset.sonicProfile.aggression = 0.0f;
            preset.sonicProfile.vintage = 0.3f;
            break;
            
        case PresetArchetype::TapeNostalgia:
            preset.sonicProfile.brightness = 0.3f;
            preset.sonicProfile.density = 0.7f;
            preset.sonicProfile.movement = 0.4f;
            preset.sonicProfile.space = 0.5f;
            preset.sonicProfile.aggression = 0.3f;
            preset.sonicProfile.vintage = 0.9f;
            break;
            
        default:
            // Balanced profile
            preset.sonicProfile.brightness = 0.5f;
            preset.sonicProfile.density = 0.5f;
            preset.sonicProfile.movement = 0.5f;
            preset.sonicProfile.space = 0.5f;
            preset.sonicProfile.aggression = 0.5f;
            preset.sonicProfile.vintage = 0.5f;
    }
}

void BoutiquePresetGenerator::generateKeywords(GoldenPreset& preset, PresetArchetype archetype) {
    // Base keywords for all presets
    preset.keywords = {"professional", "studio", "boutique"};
    
    // Archetype-specific keywords
    switch (archetype) {
        case PresetArchetype::VocalPolish:
            preset.keywords.insert(preset.keywords.end(), {
                "vocal", "polish", "smooth", "presence", "air", "clarity",
                "compression", "enhancement", "professional", "mixing"
            });
            break;
            
        case PresetArchetype::MixGlue:
            preset.keywords.insert(preset.keywords.end(), {
                "glue", "cohesion", "mix bus", "master", "compression",
                "analog", "warmth", "transparent", "musical", "bus"
            });
            break;
            
        case PresetArchetype::AnalogWarmth:
            preset.keywords.insert(preset.keywords.end(), {
                "analog", "warm", "vintage", "console", "tube", "tape",
                "saturation", "harmonic", "color", "character"
            });
            break;
            
        case PresetArchetype::DreamscapeAmbience:
            preset.keywords.insert(preset.keywords.end(), {
                "ambient", "ethereal", "space", "dream", "shimmer",
                "atmospheric", "cinematic", "expansive", "floating", "texture"
            });
            break;
            
        case PresetArchetype::TapeNostalgia:
            preset.keywords.insert(preset.keywords.end(), {
                "tape", "vintage", "echo", "delay", "nostalgic", "wow",
                "flutter", "saturation", "retro", "classic", "analog"
            });
            break;
            
        default:
            preset.keywords.push_back("versatile");
    }
    
    // Add technical hints based on engines
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0) {
            switch (preset.engineTypes[i]) {
                case ENGINE_PLATE_REVERB:
                    preset.keywords.push_back("reverb");
                    break;
                case ENGINE_CLASSIC_COMPRESSOR:
                case ENGINE_VINTAGE_OPTO_COMPRESSOR:
                    preset.keywords.push_back("compression");
                    break;
                case ENGINE_TAPE_ECHO:
                case ENGINE_DIGITAL_DELAY:
                    preset.keywords.push_back("delay");
                    break;
                case ENGINE_PARAMETRIC_EQ:
                case ENGINE_VINTAGE_CONSOLE_EQ:
                    preset.keywords.push_back("eq");
                    break;
            }
        }
    }
}

String BoutiquePresetGenerator::generateCreativeName(PresetArchetype archetype, 
                                                     const MusicalContext& context) {
    std::vector<String> names;
    
    switch (archetype) {
        case PresetArchetype::VocalPolish:
            names = {
                "Silk & Air", "Crystal Voice", "Velvet Touch",
                "Golden Throat", "Studio Polish", "Vocal Shimmer",
                "Presence & Clarity", "The Whisper Room"
            };
            break;
            
        case PresetArchetype::MixGlue:
            names = {
                "Bus Conductor", "Glue Factory", "Mix Adhesive",
                "Console Dreams", "Final Touch", "Master's Secret",
                "Cohesion Engine", "The Unifier"
            };
            break;
            
        case PresetArchetype::AnalogWarmth:
            names = {
                "Vintage Glow", "Tube Heritage", "Console 73",
                "Analog Sun", "Warm Circuits", "Transistor Soul",
                "Golden Era", "Vintage Honey"
            };
            break;
            
        case PresetArchetype::DreamscapeAmbience:
            names = {
                "Celestial Drift", "Dream Weaver", "Infinite Sky",
                "Ethereal Mist", "Cosmic Cathedral", "Aurora Dreams",
                "Floating Palace", "Stellar Winds"
            };
            break;
            
        case PresetArchetype::TapeNostalgia:
            names = {
                "Reel Memory", "Tape Ghosts", "Echo Chamber",
                "Vintage Loop", "Magnetic Dreams", "Flutter & Wow",
                "Oxide Love", "Tape Museum"
            };
            break;
            
        default:
            names = {"Boutique Preset"};
    }
    
    // Pick a random name from the list
    Random rng;
    return names[rng.nextInt(names.size())];
}

//==============================================================================
// QUALITY OPTIMIZATION

void BoutiquePresetGenerator::optimizePreset(GoldenPreset& preset) {
    // Ensure proper gain staging
    float totalMix = 0.0f;
    int activeEngines = 0;
    
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0 && preset.engineActive[i]) {
            totalMix += preset.engineMix[i];
            activeEngines++;
        }
    }
    
    // Normalize mix levels to prevent clipping
    if (totalMix > 1.5f && activeEngines > 1) {
        float scale = 1.5f / totalMix;
        for (int i = 0; i < 6; ++i) {
            preset.engineMix[i] *= scale;
        }
    }
}

void BoutiquePresetGenerator::ensureMusicalParameters(GoldenPreset& preset) {
    // Quantize certain parameters to musical values
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] < 0) continue;
        
        // Delay times to musical divisions
        if (preset.engineTypes[slot] == ENGINE_DIGITAL_DELAY ||
            preset.engineTypes[slot] == ENGINE_TAPE_ECHO) {
            if (preset.engineParams[slot].size() > 0) {
                // Quantize to common subdivisions
                float time = preset.engineParams[slot][0];
                float quantized = quantizeToMusicalValue(time, 8.0f);
                preset.engineParams[slot][0] = quantized;
            }
        }
        
        // EQ frequencies to musical intervals
        if (preset.engineTypes[slot] == ENGINE_PARAMETRIC_EQ) {
            if (preset.engineParams[slot].size() >= 3) {
                // Quantize frequency parameters to harmonic series
                preset.engineParams[slot][0] = quantizeToMusicalValue(
                    preset.engineParams[slot][0], 12.0f);
            }
        }
    }
}

//==============================================================================
// HELPER METHODS

float BoutiquePresetGenerator::musicalRandom(float min, float max, float bias) {
    static Random rng;
    float value = rng.nextFloat();
    
    // Apply bias (0.5 = no bias, 0 = bias toward min, 1 = bias toward max)
    value = std::pow(value, 2.0f * (1.0f - bias));
    
    return min + value * (max - min);
}

float BoutiquePresetGenerator::quantizeToMusicalValue(float value, float steps) {
    float quantized = std::round(value * steps) / steps;
    return juce::jlimit(0.0f, 1.0f, quantized);
}

//==============================================================================
// ACOUSTIC MODELS

BoutiquePresetGenerator::AcousticModel BoutiquePresetGenerator::getStudioAcoustics() {
    AcousticModel model;
    model.roomSize = 0.3f;
    model.decay = 0.2f;
    model.damping = 0.8f;
    model.diffusion = 0.9f;
    model.earlyReflections = 0.4f;
    model.preDelay = 0.01f;
    model.woodResonance = 0.7f;
    model.metalResonance = 0.1f;
    model.airAbsorption = 0.2f;
    return model;
}

BoutiquePresetGenerator::AcousticModel BoutiquePresetGenerator::getConcertHallAcoustics() {
    AcousticModel model;
    model.roomSize = 0.9f;
    model.decay = 0.8f;
    model.damping = 0.3f;
    model.diffusion = 0.95f;
    model.earlyReflections = 0.2f;
    model.preDelay = 0.04f;
    model.woodResonance = 0.8f;
    model.metalResonance = 0.1f;
    model.airAbsorption = 0.4f;
    return model;
}

//==============================================================================
// HARMONIC TEMPLATES

BoutiquePresetGenerator::HarmonicStructure BoutiquePresetGenerator::getVintageTubeHarmonics() {
    HarmonicStructure harmonics;
    harmonics.evenHarmonics = 0.7f;    // Strong 2nd harmonic
    harmonics.oddHarmonics = 0.3f;     // Some 3rd
    harmonics.intermodulation = 0.2f;
    harmonics.asymmetry = 0.3f;        // Slight asymmetry
    harmonics.frequency = 1000.0f;
    harmonics.bandwidth = 0.6f;
    return harmonics;
}

BoutiquePresetGenerator::HarmonicStructure BoutiquePresetGenerator::getAnalogTapeHarmonics() {
    HarmonicStructure harmonics;
    harmonics.evenHarmonics = 0.4f;
    harmonics.oddHarmonics = 0.6f;     // More odd harmonics
    harmonics.intermodulation = 0.3f;  // Tape compression artifacts
    harmonics.asymmetry = 0.1f;
    harmonics.frequency = 100.0f;      // Low frequency emphasis
    harmonics.bandwidth = 0.8f;        // Wide bandwidth
    return harmonics;
}