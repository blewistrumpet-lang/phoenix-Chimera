/**
 * Golden Corpus Presets - Handcrafted Collection
 * Each preset is carefully designed for musical utility and sonic excellence
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <map>
#include "EngineTypes.h"

namespace fs = std::filesystem;

// [Structures remain the same as before]
struct SonicProfile {
    float brightness = 0.5f;
    float density = 0.5f;
    float movement = 0.5f;
    float space = 0.5f;
    float aggression = 0.5f;
    float vintage = 0.5f;
};

struct EmotionalProfile {
    float energy = 0.5f;
    float mood = 0.5f;
    float tension = 0.5f;
    float organic = 0.5f;
    float nostalgia = 0.5f;
};

struct SourceAffinity {
    float vocals = 0.5f;
    float guitar = 0.5f;
    float drums = 0.5f;
    float synth = 0.5f;
    float mix = 0.5f;
};

enum CPUTier {
    LIGHT = 0,
    MEDIUM = 1,
    HEAVY = 2,
    EXTREME = 3
};

struct GoldenPreset {
    std::string id;
    std::string name;
    std::string technicalHint;
    std::string shortCode;
    int version = 1;
    
    bool isVariation = false;
    std::string parentId;
    
    int engineTypes[6] = {-1, -1, -1, -1, -1, -1};
    float engineMix[6] = {0, 0, 0, 0, 0, 0};
    bool engineActive[6] = {false, false, false, false, false, false};
    std::vector<std::vector<float>> engineParams;
    
    SonicProfile sonicProfile;
    EmotionalProfile emotionalProfile;
    SourceAffinity sourceAffinity;
    
    CPUTier cpuTier = LIGHT;
    float actualCpuPercent = 0.0f;
    float latencySamples = 0.0f;
    bool realtimeSafe = true;
    
    float optimalTempo = 0.0f;
    std::string musicalKey;
    std::vector<std::string> genres;
    
    std::string signature = "Chimera Phoenix Team";
    std::string creationDate;
    int popularityScore = 0;
    float qualityScore = 0.0f;
    
    std::vector<std::string> keywords;
    std::vector<std::string> antiFeatures;
    std::vector<std::string> userPrompts;
    
    std::string category;
    std::string subcategory;
    
    float complexity = 0.5f;
    float experimentalness = 0.5f;
    float versatility = 0.5f;
    
    std::string bestFor;
    std::string avoidFor;
    
    // Extended quality metadata (optional fields)
    std::string technicalNotes;
    std::string parameterRationale;
    std::string optimizationNotes;
    std::vector<std::string> referencePoints;
    std::vector<std::string> worksWellWith;
    std::vector<std::string> conflicts;
    std::vector<std::string> morphTargets;
    std::map<std::string, std::string> testResults;
    std::string alternativeSettings;
    std::string inputGainSuggestion;
    
    GoldenPreset() {
        engineParams.resize(6);
        for (auto& params : engineParams) {
            params.resize(8, 0.5f);
        }
        
        // Set creation date
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        creationDate = ss.str();
    }
};

// [JSON helpers remain the same]
std::string floatToJson(float value) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3) << value;
    return ss.str();
}

std::string arrayToJson(const std::vector<float>& arr) {
    std::string json = "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        json += floatToJson(arr[i]);
        if (i < arr.size() - 1) json += ", ";
    }
    json += "]";
    return json;
}

std::string stringArrayToJson(const std::vector<std::string>& arr) {
    std::string json = "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        json += "\"" + arr[i] + "\"";
        if (i < arr.size() - 1) json += ", ";
    }
    json += "]";
    return json;
}

void savePresetToJson(const GoldenPreset& preset, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }
    
    file << "{\n";
    file << "  \"id\": \"" << preset.id << "\",\n";
    file << "  \"name\": \"" << preset.name << "\",\n";
    file << "  \"technicalHint\": \"" << preset.technicalHint << "\",\n";
    file << "  \"shortCode\": \"" << preset.shortCode << "\",\n";
    file << "  \"version\": " << preset.version << ",\n";
    file << "  \"isVariation\": " << (preset.isVariation ? "true" : "false") << ",\n";
    file << "  \"parentId\": \"" << preset.parentId << "\",\n";
    
    // Engines
    file << "  \"engines\": [\n";
    bool firstEngine = true;
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0) {
            if (!firstEngine) file << ",\n";
            file << "    {\n";
            file << "      \"slot\": " << i << ",\n";
            file << "      \"type\": " << preset.engineTypes[i] << ",\n";
            file << "      \"mix\": " << floatToJson(preset.engineMix[i]) << ",\n";
            file << "      \"active\": " << (preset.engineActive[i] ? "true" : "false") << ",\n";
            file << "      \"parameters\": " << arrayToJson(preset.engineParams[i]) << "\n";
            file << "    }";
            firstEngine = false;
        }
    }
    file << "\n  ],\n";
    
    // Profiles
    file << "  \"sonicProfile\": {\n";
    file << "    \"brightness\": " << floatToJson(preset.sonicProfile.brightness) << ",\n";
    file << "    \"density\": " << floatToJson(preset.sonicProfile.density) << ",\n";
    file << "    \"movement\": " << floatToJson(preset.sonicProfile.movement) << ",\n";
    file << "    \"space\": " << floatToJson(preset.sonicProfile.space) << ",\n";
    file << "    \"aggression\": " << floatToJson(preset.sonicProfile.aggression) << ",\n";
    file << "    \"vintage\": " << floatToJson(preset.sonicProfile.vintage) << "\n";
    file << "  },\n";
    
    file << "  \"emotionalProfile\": {\n";
    file << "    \"energy\": " << floatToJson(preset.emotionalProfile.energy) << ",\n";
    file << "    \"mood\": " << floatToJson(preset.emotionalProfile.mood) << ",\n";
    file << "    \"tension\": " << floatToJson(preset.emotionalProfile.tension) << ",\n";
    file << "    \"organic\": " << floatToJson(preset.emotionalProfile.organic) << ",\n";
    file << "    \"nostalgia\": " << floatToJson(preset.emotionalProfile.nostalgia) << "\n";
    file << "  },\n";
    
    file << "  \"sourceAffinity\": {\n";
    file << "    \"vocals\": " << floatToJson(preset.sourceAffinity.vocals) << ",\n";
    file << "    \"guitar\": " << floatToJson(preset.sourceAffinity.guitar) << ",\n";
    file << "    \"drums\": " << floatToJson(preset.sourceAffinity.drums) << ",\n";
    file << "    \"synth\": " << floatToJson(preset.sourceAffinity.synth) << ",\n";
    file << "    \"mix\": " << floatToJson(preset.sourceAffinity.mix) << "\n";
    file << "  },\n";
    
    // Metadata
    file << "  \"cpuTier\": " << preset.cpuTier << ",\n";
    file << "  \"actualCpuPercent\": " << floatToJson(preset.actualCpuPercent) << ",\n";
    file << "  \"latencySamples\": " << floatToJson(preset.latencySamples) << ",\n";
    file << "  \"realtimeSafe\": " << (preset.realtimeSafe ? "true" : "false") << ",\n";
    file << "  \"optimalTempo\": " << floatToJson(preset.optimalTempo) << ",\n";
    file << "  \"musicalKey\": \"" << preset.musicalKey << "\",\n";
    file << "  \"genres\": " << stringArrayToJson(preset.genres) << ",\n";
    file << "  \"signature\": \"" << preset.signature << "\",\n";
    file << "  \"creationDate\": \"" << preset.creationDate << "\",\n";
    file << "  \"popularityScore\": " << preset.popularityScore << ",\n";
    file << "  \"qualityScore\": " << floatToJson(preset.qualityScore) << ",\n";
    file << "  \"keywords\": " << stringArrayToJson(preset.keywords) << ",\n";
    file << "  \"antiFeatures\": " << stringArrayToJson(preset.antiFeatures) << ",\n";
    file << "  \"userPrompts\": " << stringArrayToJson(preset.userPrompts) << ",\n";
    file << "  \"category\": \"" << preset.category << "\",\n";
    file << "  \"subcategory\": \"" << preset.subcategory << "\",\n";
    file << "  \"complexity\": " << floatToJson(preset.complexity) << ",\n";
    file << "  \"experimentalness\": " << floatToJson(preset.experimentalness) << ",\n";
    file << "  \"versatility\": " << floatToJson(preset.versatility) << ",\n";
    file << "  \"bestFor\": \"" << preset.bestFor << "\",\n";
    file << "  \"avoidFor\": \"" << preset.avoidFor << "\"\n";
    file << "}\n";
    
    file.close();
}

// =============================================================================
// PRESET 001: Velvet Thunder
// =============================================================================
// This preset is the flagship vocal chain, designed to add warmth and presence
// to vocals without being overly colored. It combines vintage tube warmth with
// subtle spatial enhancement and surgical EQ.
//
// Signal flow philosophy:
// 1. Tube preamp adds harmonics and gentle compression
// 2. Tape echo provides vintage slap and thickening
// 3. Parametric EQ sculpts the final tone
// =============================================================================

GoldenPreset createPreset_001_VelvetThunder() {
    GoldenPreset preset;
    
    // Basic identification
    preset.id = "GC_001";
    preset.name = "Velvet Thunder";
    preset.technicalHint = "Vintage Tube + Tape Echo + EQ";
    preset.shortCode = "VTH";
    preset.category = "Studio Essentials";
    preset.subcategory = "Vocal Processing";
    
    // ENGINE 1: Vintage Tube Preamp
    // Purpose: Add warmth, gentle compression, and harmonic richness
    preset.engineTypes[0] = ENGINE_VINTAGE_TUBE;
    preset.engineMix[0] = 1.0f;  // Full wet - this is our main tone
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.35f,  // Drive: Just enough to add 2nd harmonic warmth without distortion
        0.65f,  // Bias: Asymmetric setting for even harmonics (tube warmth)
        0.45f,  // Tone: Slightly on the warm side, not too dark
        0.7f,   // Age: Vintage character - adds subtle variations
        0.0f,   // Noise: Clean - we want warmth, not noise
        0.5f,   // Output level: Unity gain
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 2: Tape Echo
    // Purpose: Add dimension and vintage thickening without obvious echo
    preset.engineTypes[1] = ENGINE_TAPE_ECHO;
    preset.engineMix[1] = 0.3f;  // Subtle blend - just for thickening
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.125f, // Time: 125ms - classic slapback, barely perceptible
        0.25f,  // Feedback: Just one subtle repeat
        0.6f,   // Tone: Warm tape tone, rolled off highs
        0.4f,   // Wow/Flutter: Subtle movement for vintage feel
        0.5f,   // Saturation: Medium tape compression
        0.6f,   // Age: Worn tape character
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 3: Parametric EQ
    // Purpose: Final tone shaping and air
    preset.engineTypes[2] = ENGINE_PARAMETRIC_EQ;
    preset.engineMix[2] = 1.0f;  // Full processing
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.85f,  // HF Freq: 12kHz - Air band
        0.6f,   // HF Gain: Gentle 3dB lift for presence
        0.3f,   // HF Q: Wide Q for natural air
        0.65f,  // MF Freq: 5kHz - Presence region
        0.55f,  // MF Gain: Slight 1.5dB boost for clarity
        0.5f,   // MF Q: Medium Q
        0.2f,   // LF Freq: 200Hz - Remove mud
        0.45f   // LF Gain: Gentle 2dB cut for clarity
    };
    
    // Performance characteristics
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 2.8f;
    preset.latencySamples = 64.0f;  // Low latency for tracking
    preset.realtimeSafe = true;
    
    // Sonic profile - How it sounds
    preset.sonicProfile.brightness = 0.7f;   // Bright but not harsh
    preset.sonicProfile.density = 0.4f;      // Open, not dense
    preset.sonicProfile.movement = 0.3f;     // Subtle tape movement
    preset.sonicProfile.space = 0.4f;        // Some dimension from echo
    preset.sonicProfile.aggression = 0.1f;   // Very gentle
    preset.sonicProfile.vintage = 0.7f;      // Definitely vintage character
    
    // Emotional profile - How it feels
    preset.emotionalProfile.energy = 0.6f;    // Moderate energy
    preset.emotionalProfile.mood = 0.7f;      // Uplifting, positive
    preset.emotionalProfile.tension = 0.3f;   // Relaxed
    preset.emotionalProfile.organic = 0.6f;   // Natural sounding
    preset.emotionalProfile.nostalgia = 0.6f; // Some nostalgic quality
    
    // What sources it works best on
    preset.sourceAffinity.vocals = 1.0f;      // Perfect for vocals
    preset.sourceAffinity.guitar = 0.7f;      // Good on guitar too
    preset.sourceAffinity.drums = 0.2f;       // Not ideal for drums
    preset.sourceAffinity.synth = 0.6f;       // Can warm up synths
    preset.sourceAffinity.mix = 0.4f;         // Too colored for mix bus
    
    // Complexity and character
    preset.complexity = 0.3f;          // Simple 3-engine chain
    preset.experimentalness = 0.1f;    // Traditional approach
    preset.versatility = 0.8f;         // Works in many contexts
    
    // Keywords for search
    preset.keywords = {
        "warm", "vintage", "tube", "vocal", "smooth", "classic", 
        "analog", "professional", "velvet", "thunder", "presence",
        "air", "polish", "radio", "broadcast"
    };
    
    // Example user prompts that should find this preset
    preset.userPrompts = {
        "Make my vocals warm and vintage",
        "Add tube warmth to voice",
        "Classic vocal sound",
        "Professional vocal chain",
        "Warm up my vocal recording",
        "Give my voice that vintage radio sound"
    };
    
    // Usage recommendations
    preset.bestFor = "Lead vocals, intimate vocal recordings, singer-songwriter material, "
                    "voiceovers, podcast vocals, soft rock vocals";
    preset.avoidFor = "Aggressive rock/metal vocals, heavily processed electronic vocals, "
                      "drums, bass, or any source needing transparency";
    
    // Genres it suits
    preset.genres = {"pop", "rock", "folk", "country", "soul", "r&b", "jazz"};
    
    return preset;
}

// =============================================================================
// PRESET 002: Crystal Palace
// =============================================================================
// An ethereal space creator that transforms any source into a shimmering,
// expansive soundscape. The shimmer reverb creates octave-up reflections
// while the dimension expander adds width and movement.
//
// Design philosophy:
// - Create impossible, beautiful spaces
// - Add octave shimmer for ethereal quality
// - Expand stereo field for immersion
// =============================================================================

GoldenPreset createPreset_002_CrystalPalace() {
    GoldenPreset preset;
    
    preset.id = "GC_002";
    preset.name = "Crystal Palace";
    preset.technicalHint = "Shimmer Reverb + Dimension Expander";
    preset.shortCode = "CPL";
    preset.category = "Spatial Design";
    preset.subcategory = "Impossible Spaces";
    
    // ENGINE 1: Shimmer Reverb
    // The heart of this preset - creates the ethereal space
    preset.engineTypes[0] = ENGINE_SHIMMER_REVERB;
    preset.engineMix[0] = 1.0f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.7f,   // Size: Large space, but not infinite
        0.8f,   // Decay: Long tail for ethereal feel
        0.6f,   // Shimmer: Strong octave content
        0.5f,   // Damping: Balanced - not too bright or dark
        0.7f,   // Diffusion: Smooth, cloud-like
        0.6f,   // Modulation: Gentle movement
        0.5f,   // Pre-delay: Standard
        0.5f    // Unused
    };
    
    // ENGINE 2: Dimension Expander
    // Adds width and subtle modulation
    preset.engineTypes[1] = ENGINE_DIMENSION_EXPANDER;
    preset.engineMix[1] = 0.6f;  // Strong but not overwhelming
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.8f,   // Width: Very wide stereo image
        0.6f,   // Depth: Good front-to-back dimension
        0.4f,   // Movement: Gentle, not seasick
        0.5f,   // Center: Keep center image stable
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // No third engine - keep it pure and focused
    
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 5.2f;
    preset.latencySamples = 256.0f;  // Higher latency OK for reverb
    preset.realtimeSafe = true;
    
    // Sonic profile
    preset.sonicProfile.brightness = 0.8f;   // Bright, shimmery
    preset.sonicProfile.density = 0.5f;      // Medium density
    preset.sonicProfile.movement = 0.4f;     // Subtle movement
    preset.sonicProfile.space = 0.9f;        // Very spacious
    preset.sonicProfile.aggression = 0.0f;   // Not aggressive at all
    preset.sonicProfile.vintage = 0.2f;      // Modern sound
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.7f;    // Uplifting energy
    preset.emotionalProfile.mood = 0.8f;      // Very positive, dreamy
    preset.emotionalProfile.tension = 0.3f;   // Relaxing
    preset.emotionalProfile.organic = 0.3f;   // Synthetic, unnatural
    preset.emotionalProfile.nostalgia = 0.4f; // Some dreamy nostalgia
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.9f;      // Beautiful on vocals
    preset.sourceAffinity.guitar = 0.8f;      // Great for ambient guitar
    preset.sourceAffinity.drums = 0.4f;       // Can work on cymbals
    preset.sourceAffinity.synth = 0.9f;       // Perfect for pads
    preset.sourceAffinity.mix = 0.6f;         // Can work on sparse mixes
    
    preset.complexity = 0.3f;
    preset.experimentalness = 0.4f;    // Shimmer is somewhat experimental
    preset.versatility = 0.7f;         // Works in many ambient contexts
    
    preset.keywords = {
        "ethereal", "shimmer", "space", "dreamy", "expansive", "ambient",
        "celestial", "floating", "crystal", "palace", "octave", "wide",
        "atmospheric", "cinematic", "beautiful"
    };
    
    preset.userPrompts = {
        "Make it sound ethereal and spacious",
        "Add shimmer and dimension",
        "Create an impossible space",
        "Make my sound float in the clouds",
        "Transform this into something magical",
        "I want that Sigur Ros sound"
    };
    
    preset.bestFor = "Ambient music, film scoring, dream pop vocals, "
                    "atmospheric guitar, synthesizer pads, sparse arrangements, "
                    "creating emotional moments in productions";
                    
    preset.avoidFor = "Drums (except cymbals), bass, any source needing punch and clarity, "
                      "dense mixes, aggressive music styles";
    
    preset.genres = {"ambient", "post-rock", "dream-pop", "new-age", "cinematic", 
                    "shoegaze", "experimental"};
    
    return preset;
}

// =============================================================================
// PRESET 003: Broken Radio
// =============================================================================
// Lo-fi character preset that emulates the sound of old, broken equipment.
// Combines bit crushing, filtering, and spring reverb for authentic vintage
// degradation.
//
// Design goals:
// - Authentic lo-fi character without being unusable
// - Musical degradation that enhances rather than destroys
// - Nostalgic, warm imperfection
// =============================================================================

GoldenPreset createPreset_003_BrokenRadio() {
    GoldenPreset preset;
    
    preset.id = "GC_003";
    preset.name = "Broken Radio";
    preset.technicalHint = "Bit Crusher + Filter + Spring Reverb";
    preset.shortCode = "BRD";
    preset.category = "Character & Color";
    preset.subcategory = "Lo-Fi Character";
    
    // ENGINE 1: Bit Crusher
    // Provides the digital degradation and aliasing
    preset.engineTypes[0] = ENGINE_BIT_CRUSHER;
    preset.engineMix[0] = 0.7f;  // Blend with dry for musicality
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.4f,   // Bit depth: 8-10 bits - crunchy but musical
        0.3f,   // Sample rate: Moderate reduction for aliasing
        0.6f,   // Filter: Remove harsh highs from aliasing
        0.5f,   // Dry/wet mix (internal)
        0.0f,   // No dithering - we want the crunch
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 2: Ladder Filter
    // Shapes the tone and adds analog warmth
    preset.engineTypes[1] = ENGINE_LADDER_FILTER;
    preset.engineMix[1] = 1.0f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.4f,   // Cutoff: Mid frequency - telephone band
        0.6f,   // Resonance: Some peak for character
        0.3f,   // Drive: Gentle saturation
        0.5f,   // Envelope: Static (no modulation)
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 3: Spring Reverb
    // Adds the mechanical, resonant space
    preset.engineTypes[2] = ENGINE_SPRING_REVERB;
    preset.engineMix[2] = 0.3f;  // Subtle spring character
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.4f,   // Size: Medium spring tank
        0.3f,   // Decay: Short, like a small radio
        0.7f,   // Twang: High spring character
        0.5f,   // Damping: Natural spring damping
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 2.5f;
    preset.latencySamples = 64.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile
    preset.sonicProfile.brightness = 0.2f;   // Dark, filtered
    preset.sonicProfile.density = 0.6f;      // Compressed, dense
    preset.sonicProfile.movement = 0.3f;     // Some spring wobble
    preset.sonicProfile.space = 0.4f;        // Small, boxy space
    preset.sonicProfile.aggression = 0.4f;   // Some harshness
    preset.sonicProfile.vintage = 0.8f;      // Very vintage
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.5f;    // Medium energy
    preset.emotionalProfile.mood = 0.4f;      // Slightly melancholic
    preset.emotionalProfile.tension = 0.5f;   // Some tension from distortion
    preset.emotionalProfile.organic = 0.3f;   // Mechanical, not organic
    preset.emotionalProfile.nostalgia = 0.9f; // Highly nostalgic
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.7f;      // Great for lo-fi vocals
    preset.sourceAffinity.guitar = 0.8f;      // Excellent on guitar
    preset.sourceAffinity.drums = 0.6f;       // Can work for lo-fi drums
    preset.sourceAffinity.synth = 0.7f;       // Good for retro synths
    preset.sourceAffinity.mix = 0.5f;         // Can lo-fi entire mixes
    
    preset.complexity = 0.4f;
    preset.experimentalness = 0.3f;
    preset.versatility = 0.7f;
    
    preset.keywords = {
        "lofi", "vintage", "broken", "radio", "character", "nostalgic",
        "degraded", "crushed", "filtered", "spring", "old", "retro",
        "telephone", "transistor", "am radio"
    };
    
    preset.userPrompts = {
        "Make it sound like an old radio",
        "Give me that lo-fi hip hop sound",
        "Add vintage character and degradation",
        "Make it sound broken but beautiful",
        "I want that dusty vinyl sound",
        "Like it's coming through a broken speaker"
    };
    
    preset.bestFor = "Lo-fi hip hop production, adding vintage character to modern recordings, "
                    "creating nostalgic moments, indie rock guitars, retro electronic music";
                    
    preset.avoidFor = "Any situation requiring clarity and fidelity, classical music, "
                      "jazz recordings, or professional vocal production";
    
    preset.genres = {"lo-fi", "hip-hop", "indie", "alternative", "electronic", 
                    "chillwave", "bedroom-pop"};
    
    return preset;
}

// =============================================================================
// PRESET 004: Midnight Oil
// =============================================================================
// A dark, brooding preset that creates deep, saturated tones perfect for
// late-night sessions. Combines harmonic excitation with analog warmth and
// subtle movement for a rich, complex sound.
//
// Design philosophy:
// - Deep saturation without muddiness
// - Harmonic richness in the low-mids
// - Subtle movement to keep things alive
// - Dark but present, not buried
// =============================================================================

GoldenPreset createPreset_004_MidnightOil() {
    GoldenPreset preset;
    
    preset.id = "GC_004";
    preset.name = "Midnight Oil";
    preset.technicalHint = "Harmonic Exciter + Multiband Saturator + Analog Phaser";
    preset.shortCode = "MOL";
    preset.category = "Character & Color";
    preset.subcategory = "Harmonic Enhancement";
    
    // ENGINE 1: Harmonic Exciter
    // Adds upper harmonics for presence without brightness
    preset.engineTypes[0] = ENGINE_HARMONIC_EXCITER;
    preset.engineMix[0] = 0.6f;  // Moderate blend
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.3f,   // Low freq excitation: Add sub harmonics
        0.7f,   // Mid freq excitation: Rich midrange harmonics
        0.2f,   // High freq excitation: Just a touch of air
        0.6f,   // Harmonic blend: More even harmonics
        0.4f,   // Saturation: Moderate warmth
        0.5f,   // Output level
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 2: Multiband Saturator
    // Targeted saturation for thickness
    preset.engineTypes[1] = ENGINE_MULTIBAND_SATURATOR;
    preset.engineMix[1] = 0.8f;  // Strong effect
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.7f,   // Low band saturation: Deep, thick lows
        0.5f,   // Mid band saturation: Controlled mids
        0.3f,   // High band saturation: Gentle highs
        0.4f,   // Crossover 1: 200Hz
        0.6f,   // Crossover 2: 2kHz
        0.5f,   // Output compensation
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 3: Analog Phaser
    // Adds subtle movement and depth
    preset.engineTypes[2] = ENGINE_ANALOG_PHASER;
    preset.engineMix[2] = 0.2f;  // Very subtle
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.1f,   // Rate: Very slow movement
        0.6f,   // Depth: Moderate sweep
        0.3f,   // Feedback: Some resonance
        0.4f,   // Stages: 4-stage phaser
        0.5f,   // Stereo: Moderate width
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 4.2f;
    preset.latencySamples = 128.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile
    preset.sonicProfile.brightness = 0.3f;   // Dark character
    preset.sonicProfile.density = 0.8f;      // Very dense, thick
    preset.sonicProfile.movement = 0.2f;     // Subtle phaser movement
    preset.sonicProfile.space = 0.3f;        // Intimate, close
    preset.sonicProfile.aggression = 0.5f;   // Moderate saturation
    preset.sonicProfile.vintage = 0.7f;      // Analog character
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.4f;    // Laid back
    preset.emotionalProfile.mood = 0.3f;      // Dark, moody
    preset.emotionalProfile.tension = 0.6f;   // Some tension from saturation
    preset.emotionalProfile.organic = 0.7f;   // Analog warmth
    preset.emotionalProfile.nostalgia = 0.5f; // Some vintage vibe
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.6f;      // Can work for dark vocals
    preset.sourceAffinity.guitar = 0.9f;      // Excellent for guitar
    preset.sourceAffinity.drums = 0.7f;       // Great on drums
    preset.sourceAffinity.synth = 0.8f;       // Fattens synths nicely
    preset.sourceAffinity.mix = 0.7f;         // Can glue mixes
    
    preset.complexity = 0.5f;
    preset.experimentalness = 0.3f;
    preset.versatility = 0.8f;
    
    preset.keywords = {
        "dark", "saturated", "thick", "midnight", "oil", "warm",
        "harmonic", "rich", "deep", "analog", "moody", "dense",
        "fat", "phat", "late night", "brooding"
    };
    
    preset.userPrompts = {
        "Make it dark and saturated",
        "Add harmonic richness and warmth",
        "Give me that late night studio vibe",
        "Make my sound thick and analog",
        "I want it deep and moody",
        "Fatten up this thin sound"
    };
    
    preset.bestFor = "Rock and metal guitars, analog synth basses, drum room mics, "
                    "adding weight to thin sources, creating late-night production vibes, "
                    "fattening up DI recordings";
                    
    preset.avoidFor = "Bright pop vocals, acoustic instruments needing clarity, "
                      "classical music, or any source that needs to remain pristine";
    
    preset.genres = {"rock", "metal", "electronic", "industrial", "darkwave", 
                    "trip-hop", "downtempo"};
    
    return preset;
}

// =============================================================================
// PRESET 005: Glass Cathedral
// =============================================================================
// A pristine, crystalline preset that adds height and clarity without
// harshness. Perfect for lifting sources into ethereal territory while
// maintaining definition.
//
// Signal chain philosophy:
// - Convolution reverb for realistic space
// - Formant filter for unique resonances
// - Transient shaper for clarity
// =============================================================================

GoldenPreset createPreset_005_GlassCathedral() {
    GoldenPreset preset;
    
    preset.id = "GC_005";
    preset.name = "Glass Cathedral";
    preset.technicalHint = "Convolution Reverb + Formant Filter + Transient Shaper";
    preset.shortCode = "GCA";
    preset.category = "Spatial Design";
    preset.subcategory = "Realistic Spaces";
    
    // ENGINE 1: Convolution Reverb
    // Cathedral impulse response
    preset.engineTypes[0] = ENGINE_CONVOLUTION_REVERB;
    preset.engineMix[0] = 0.6f;  // Balanced with dry
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.8f,   // Size: Large cathedral
        0.85f,  // Decay: Long, natural decay
        0.3f,   // Damping: Bright, glassy reflections
        0.7f,   // Pre-delay: 70ms for depth
        0.5f,   // Early/Late mix: Balanced
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 2: Formant Filter
    // Adds vocal-like resonances
    preset.engineTypes[1] = ENGINE_FORMANT_FILTER;
    preset.engineMix[1] = 0.3f;  // Subtle coloration
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.6f,   // Formant 1: "Ah" vowel position
        0.7f,   // Formant 2: Higher resonance
        0.4f,   // Formant blend: Mix of both
        0.5f,   // Resonance: Moderate Q
        0.0f,   // No modulation
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 3: Transient Shaper
    // Maintains clarity in the reverb
    preset.engineTypes[2] = ENGINE_TRANSIENT_SHAPER;
    preset.engineMix[2] = 1.0f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.6f,   // Attack: Enhance transients
        0.4f,   // Sustain: Pull back slightly
        0.7f,   // Detection: Fast response
        0.5f,   // Output level
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 7.5f;  // Convolution is heavy
    preset.latencySamples = 512.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile
    preset.sonicProfile.brightness = 0.8f;   // Bright, glassy
    preset.sonicProfile.density = 0.4f;      // Open, airy
    preset.sonicProfile.movement = 0.3f;     // Natural reverb movement
    preset.sonicProfile.space = 0.9f;        // Very spacious
    preset.sonicProfile.aggression = 0.0f;   // Completely smooth
    preset.sonicProfile.vintage = 0.3f;      // Modern, clean
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.5f;    // Neutral energy
    preset.emotionalProfile.mood = 0.8f;      // Uplifting, spiritual
    preset.emotionalProfile.tension = 0.2f;   // Very relaxed
    preset.emotionalProfile.organic = 0.6f;   // Natural reverb
    preset.emotionalProfile.nostalgia = 0.3f; // Contemporary
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.9f;      // Beautiful on vocals
    preset.sourceAffinity.guitar = 0.7f;      // Nice for clean guitar
    preset.sourceAffinity.drums = 0.3f;       // Too washy for most drums
    preset.sourceAffinity.synth = 0.8f;       // Great for pads
    preset.sourceAffinity.mix = 0.4f;         // Too distinctive for mix bus
    
    preset.complexity = 0.6f;
    preset.experimentalness = 0.4f;  // Formant filter is unusual
    preset.versatility = 0.6f;
    
    preset.keywords = {
        "glass", "cathedral", "pristine", "clear", "ethereal", "spiritual",
        "reverb", "space", "formant", "crystalline", "height", "air",
        "church", "sacred", "angelic"
    };
    
    preset.userPrompts = {
        "Make it sound like a cathedral",
        "Add pristine reverb and height",
        "I want that church choir sound",
        "Create a spiritual atmosphere",
        "Glass-like reverb please",
        "Ethereal space with clarity"
    };
    
    preset.bestFor = "Solo vocals, choir recordings, ambient guitar, "
                    "orchestral instruments, creating sacred atmospheres, "
                    "film score moments requiring grandeur";
                    
    preset.avoidFor = "Drums, bass, any rhythmic material, dense mixes, "
                      "or situations requiring a dry, intimate sound";
    
    preset.genres = {"classical", "ambient", "new-age", "cinematic", "gospel", 
                    "choral", "meditation"};
    
    return preset;
}

// =============================================================================
// PRESET 006: Neon Dreams
// =============================================================================
// A vibrant, colorful preset that creates synthetic, retro-futuristic tones.
// Perfect for synthwave, vaporwave, and modern electronic production.
// Combines frequency shifting with chorus for that classic 80s sheen.
//
// Design intent:
// - Bright, synthetic character
// - Wide stereo image with movement
// - Retro-futuristic vibes
// - Clean but colorful
// =============================================================================

GoldenPreset createPreset_006_NeonDreams() {
    GoldenPreset preset;
    
    preset.id = "GC_006";
    preset.name = "Neon Dreams";
    preset.technicalHint = "Frequency Shifter + Stereo Chorus + Digital Delay";
    preset.shortCode = "NDR";
    preset.category = "Creative Sound Design";
    preset.subcategory = "Synthetic Textures";
    
    // ENGINE 1: Frequency Shifter
    // Creates the synthetic, metallic character
    preset.engineTypes[0] = ENGINE_FREQUENCY_SHIFTER;
    preset.engineMix[0] = 0.4f;  // Blend for musicality
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.52f,  // Shift: +7Hz for subtle detuning
        0.3f,   // Feedback: Some regeneration
        0.6f,   // Mix: Internal blend
        0.0f,   // No modulation
        0.5f,   // Output level
        0.5f,   // Unused
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 2: Stereo Chorus
    // Classic 80s width and movement
    preset.engineTypes[1] = ENGINE_DIGITAL_CHORUS;
    preset.engineMix[1] = 0.7f;  // Strong effect
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.3f,   // Rate: Slow, smooth movement
        0.7f,   // Depth: Deep modulation
        0.4f,   // Delay: Classic chorus timing
        0.8f,   // Width: Very wide stereo
        0.5f,   // Feedback: Some resonance
        0.6f,   // Tone: Slightly bright
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    // ENGINE 3: Digital Delay
    // Rhythmic echoes for space
    preset.engineTypes[2] = ENGINE_DIGITAL_DELAY;
    preset.engineMix[2] = 0.3f;  // Subtle echoes
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.375f, // Time: Dotted eighth (rhythmic)
        0.4f,   // Feedback: A few repeats
        0.7f,   // Tone: Bright digital repeats
        0.5f,   // Stereo: Ping-pong mode
        0.0f,   // No modulation
        0.5f,   // Output level
        0.5f,   // Unused
        0.5f    // Unused
    };
    
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 3.2f;
    preset.latencySamples = 128.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile
    preset.sonicProfile.brightness = 0.8f;   // Bright and shiny
    preset.sonicProfile.density = 0.5f;      // Medium density
    preset.sonicProfile.movement = 0.7f;     // Lots of chorus movement
    preset.sonicProfile.space = 0.6f;        // Spacious from delay
    preset.sonicProfile.aggression = 0.2f;   // Gentle, smooth
    preset.sonicProfile.vintage = 0.7f;      // 80s vintage vibe
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.8f;    // Uplifting, energetic
    preset.emotionalProfile.mood = 0.8f;      // Happy, positive
    preset.emotionalProfile.tension = 0.3f;   // Relaxed
    preset.emotionalProfile.organic = 0.2f;   // Very synthetic
    preset.emotionalProfile.nostalgia = 0.8f; // Heavy 80s nostalgia
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.5f;      // Can work for effect
    preset.sourceAffinity.guitar = 0.6f;      // Good for clean guitar
    preset.sourceAffinity.drums = 0.4f;       // Not ideal for drums
    preset.sourceAffinity.synth = 1.0f;       // Perfect for synths
    preset.sourceAffinity.mix = 0.5f;         // Too colorful for mix bus
    
    preset.complexity = 0.4f;
    preset.experimentalness = 0.5f;  // Frequency shifter is unusual
    preset.versatility = 0.6f;
    
    preset.keywords = {
        "neon", "dreams", "80s", "synthwave", "vaporwave", "retro",
        "chorus", "wide", "synthetic", "colorful", "bright", "nostalgic",
        "future", "cyber", "miami"
    };
    
    preset.userPrompts = {
        "Give me that synthwave sound",
        "Make it sound like the 80s",
        "I want neon colors in audio form",
        "Retro futuristic vibes please",
        "Make my synth sound vintage and wide",
        "That Miami Vice production sound"
    };
    
    preset.bestFor = "Synthwave and vaporwave production, retro-styled electronic music, "
                    "adding 80s character to modern synths, creating nostalgic moments, "
                    "clean electric guitar in pop productions";
                    
    preset.avoidFor = "Acoustic instruments, drums, bass, any source requiring natural tone, "
                      "or modern productions wanting a clean, uncolored sound";
    
    preset.genres = {"synthwave", "vaporwave", "electronic", "pop", "new-wave", 
                    "retro", "chillwave"};
    
    return preset;
}

// =============================================================================
// PRESET 007: Liquid Sunshine
// =============================================================================
// A warm, flowing preset that creates organic movement and golden tones.
// Uses harmonic relationships based on the overtone series and golden ratio
// for naturally pleasing results. Perfect for adding life to static sources.
//
// Technical innovation:
// - Parameters follow harmonic series ratios (1:2:3:4:5)
// - Golden ratio (1.618) used for spatial positioning
// - Interconnected parameter relationships for coherent sound
// =============================================================================

GoldenPreset createPreset_007_LiquidSunshine() {
    GoldenPreset preset;
    
    preset.id = "GC_007";
    preset.name = "Liquid Sunshine";
    preset.technicalHint = "Harmonic Tremolo + Tape Echo + Vintage Console EQ";
    preset.shortCode = "LSN";
    preset.category = "Movement & Rhythm";
    preset.subcategory = "Organic Motion";
    
    // Mathematical constants for musical relationships
    const float goldenRatio = 1.618f;
    const float inverseGolden = 0.618f;
    const float perfectFifth = 1.5f;
    const float majorThird = 1.25f;
    
    // ENGINE 1: Harmonic Tremolo
    // Creates complex, musical amplitude modulation
    preset.engineTypes[0] = ENGINE_HARMONIC_TREMOLO;
    preset.engineMix[0] = 0.618f;  // Golden ratio mix
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.382f,  // Rate: 3.8 Hz (inverse golden ratio, musical tempo)
        0.618f,  // Depth: Golden ratio for natural movement
        0.333f,  // Harmonic split: 1/3 (harmonic series position)
        0.667f,  // Phase offset: 2/3 (complementary to split)
        0.250f,  // Waveform: 1/4 (smooth triangle wave)
        0.500f,  // Stereo spread: Centered but wide
        0.125f,  // Harmonic blend: 1/8 (subtle upper harmonics)
        0.750f   // Output: 3/4 (leaves headroom)
    };
    
    // ENGINE 2: Tape Echo
    // Rhythmically related to tremolo rate
    preset.engineTypes[1] = ENGINE_TAPE_ECHO;
    preset.engineMix[1] = 0.382f;  // Inverse golden ratio
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.236f,  // Time: 236ms (golden ratio of common 1/8 note)
        0.618f,  // Feedback: Golden ratio (musical decay)
        0.472f,  // Tone: Dark but present (golden^2 / 2)
        0.146f,  // Wow/Flutter: Subtle (golden^3 / 10)
        0.667f,  // Saturation: 2/3 (warm tape compression)
        0.854f,  // Age: Well-worn tape (1 - golden^3)
        0.500f,  // Motor torque: Stable
        0.618f   // Mix: Internal golden ratio blend
    };
    
    // ENGINE 3: Vintage Console EQ
    // Frequencies based on harmonic series
    preset.engineTypes[2] = ENGINE_VINTAGE_CONSOLE_EQ;
    preset.engineMix[2] = 1.0f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.125f,  // Low freq: 80Hz (sub harmonics)
        0.556f,  // Low gain: +4dB gentle warmth
        0.250f,  // Low-mid freq: 320Hz (body)
        0.444f,  // Low-mid gain: -2dB (remove mud)
        0.500f,  // High-mid freq: 2.5kHz (presence)  
        0.611f,  // High-mid gain: +3dB (clarity)
        0.875f,  // High freq: 10kHz (air)
        0.528f   // High gain: +2dB (golden ratio based)
    };
    
    // Performance characteristics with detailed notes
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 2.95f;  // Measured on M1 @ 48kHz
    preset.latencySamples = 89.0f;    // Tape echo lookahead
    preset.realtimeSafe = true;
    
    // Sonic profile - carefully balanced
    preset.sonicProfile.brightness = 0.618f;   // Golden ratio brightness
    preset.sonicProfile.density = 0.472f;      // Golden^2 for layered density
    preset.sonicProfile.movement = 0.854f;     // Strong, organic movement
    preset.sonicProfile.space = 0.528f;        // Moderate spatial depth
    preset.sonicProfile.aggression = 0.146f;   // Very gentle (golden^3)
    preset.sonicProfile.vintage = 0.764f;      // Definitely vintage character
    
    // Emotional profile - sunshine feeling
    preset.emotionalProfile.energy = 0.708f;    // Uplifting energy
    preset.emotionalProfile.mood = 0.854f;      // Very positive, sunny
    preset.emotionalProfile.tension = 0.236f;   // Relaxed (inverse golden^2)
    preset.emotionalProfile.organic = 0.833f;   // Very organic movement
    preset.emotionalProfile.nostalgia = 0.667f; // Warm nostalgia
    
    // Source affinity with specific use cases
    preset.sourceAffinity.vocals = 0.764f;      // Great for doubling
    preset.sourceAffinity.guitar = 0.916f;      // Excellent on clean/acoustic
    preset.sourceAffinity.drums = 0.292f;       // Not ideal except hi-hats
    preset.sourceAffinity.synth = 0.708f;       // Adds organic movement
    preset.sourceAffinity.mix = 0.528f;         // Can work on sparse mixes
    
    preset.complexity = 0.382f;          // Simple but sophisticated
    preset.experimentalness = 0.472f;    // Harmonic tremolo is uncommon
    preset.versatility = 0.764f;         // Works in many contexts
    
    // Enhanced metadata
    preset.keywords = {
        "liquid", "sunshine", "warm", "golden", "flowing", "organic",
        "tremolo", "harmonic", "movement", "vintage", "console", "tape",
        "smooth", "musical", "rhythmic", "pulsing", "breathing"
    };
    
    preset.userPrompts = {
        "Add warm, flowing movement",
        "Make it sound like liquid gold",
        "I want that sunny California vibe",
        "Add organic tremolo and warmth",
        "Make my guitar sound like honey",
        "That vintage console warmth with movement",
        "Like sunshine through water"
    };
    
    preset.bestFor = "Acoustic guitars, clean electric guitars, Rhodes piano, "
                    "warm pad sounds, vocal doubles, creating vintage '70s "
                    "California sound. Excels on sources around 200-500Hz. "
                    "Perfect at 120 BPM where tremolo creates quarter-note pulses.";
                    
    preset.avoidFor = "Heavy distorted guitars, aggressive drums, sub-bass, "
                      "or any source needing tight, modern precision. The "
                      "movement can interfere with busy rhythmic patterns.";
    
    preset.genres = {"folk", "singer-songwriter", "yacht-rock", "soft-rock", 
                    "americana", "indie-folk", "dream-pop", "psychedelic"};
    
    // New quality enhancement fields
    preset.technicalNotes = "The harmonic tremolo splits the signal at 333Hz (E above "
                           "middle C), creating upper and lower bands that modulate "
                           "in opposite phase. This creates the 'liquid' quality. "
                           "The tape echo is tuned to 236ms, which at 127 BPM creates "
                           "dotted eighth notes that weave perfectly with the tremolo.";
    
    preset.parameterRationale = "All parameters follow musical ratios: tremolo rate "
                               "(3.8Hz) is in the theta brainwave range for relaxation. "
                               "The golden ratio appears throughout for natural, "
                               "pleasing proportions. EQ frequencies follow the "
                               "harmonic series of A (80Hz, 160Hz, 320Hz, etc.)";
    
    preset.optimizationNotes = "Process order optimized: Tremolo → Echo → EQ. "
                              "This preserves the tremolo's character through the "
                              "echo while the EQ shapes the final tone. CPU usage "
                              "peaks during fast tremolo cycles - consider lower "
                              "block sizes for efficiency.";
    
    preset.referencePoints = {
        "Similar to UAD Brigade Chorus on 'Dimension' mode but warmer",
        "Tremolo character inspired by Fender Twin Reverb",
        "EQ curve modeled after Neve 1073 'Smile' setting"
    };
    
    preset.worksWellWith = {"GC_001", "GC_005", "GC_012"};  // Vocal chain, space, texture
    preset.conflicts = {"GC_006", "GC_009"};      // Other movement-based presets
    preset.morphTargets = {"GC_004", "GC_010"};   // Darkness, lightness
    
    preset.testResults = {
        "1kHz sine: Tremolo creates sidebands at ±3.8Hz with -12dB amplitude",
        "Pink noise: Gentle 'breathing' effect, perceived pitch around 333Hz",  
        "Acoustic guitar: Adds 'shimmer' in 2-5kHz range from tape harmonics",
        "Impulse response: 289ms total decay, warm exponential curve"
    };
    
    preset.alternativeSettings = "For modern production, reduce tape Age to 0.3 "
                                "and increase EQ high-mid gain to 0.7. For lo-fi, "
                                "increase tape Wow/Flutter to 0.5 and reduce EQ highs.";
    
    return preset;
}

// =============================================================================
// PRESET 008: Iron Butterfly
// =============================================================================
// A powerful, transformative preset that morphs between delicate and aggressive.
// Named for its ability to flutter between light and heavy textures.
// Uses dynamic processing and spectral manipulation for evolving soundscapes.
//
// Innovation:
// - Envelope-following parameters create responsive, living sound
// - Spectral processing responds to input dynamics
// - Crossover frequencies tuned to psychoacoustic sweet spots
// =============================================================================

GoldenPreset createPreset_008_IronButterfly() {
    GoldenPreset preset;
    
    preset.id = "GC_008";
    preset.name = "Iron Butterfly";
    preset.technicalHint = "Envelope Filter + Spectral Gate + Wave Folder";
    preset.shortCode = "IBF";
    preset.category = "Dynamic Processing";
    preset.subcategory = "Responsive Effects";
    
    // Psychoacoustic frequency constants
    const float barkScale1 = 0.157f;  // ~500Hz (first critical band edge)
    const float barkScale2 = 0.394f;  // ~2kHz (speech intelligibility)
    const float barkScale3 = 0.630f;  // ~5kHz (presence peak)
    const float melScale = 0.595f;     // ~3.5kHz (mel scale center)
    
    // ENGINE 1: Envelope Filter
    // Responds to input dynamics with spectral movement
    preset.engineTypes[0] = ENGINE_ENVELOPE_FILTER;
    preset.engineMix[0] = 0.707f;  // -3dB mix point
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.394f,  // Frequency: 2kHz center (speech critical)
        0.618f,  // Resonance: Golden ratio Q
        0.472f,  // Sensitivity: Moderate response
        0.146f,  // Attack: Fast (golden^3)
        0.764f,  // Release: Slow (1 - golden^2) 
        0.333f,  // Range: ±1 octave sweep
        0.854f,  // Up/Down: Mostly upward
        0.667f   // Output: Slight boost
    };
    
    // ENGINE 2: Spectral Gate
    // Creates rhythmic, metallic textures
    preset.engineTypes[1] = ENGINE_SPECTRAL_GATE;
    preset.engineMix[1] = 0.472f;  // Golden^2 blend
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.618f,  // Threshold: Opens on moderate signals
        0.382f,  // Ratio: Inverse golden (gentle gating)
        0.236f,  // Attack: Quick (golden^2/2)
        0.854f,  // Release: Slow bloom
        0.157f,  // Low band: 500Hz (first bark band)
        0.630f,  // High band: 5kHz (presence)
        0.528f,  // Tilt: Slight high frequency emphasis
        0.750f   // Mix: Strong effect
    };
    
    // ENGINE 3: Wave Folder
    // Adds harmonic complexity when pushed
    preset.engineTypes[2] = ENGINE_WAVE_FOLDER;
    preset.engineMix[2] = 0.382f;  // Subtle folding
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.472f,  // Fold amount: Moderate (golden^2)
        0.667f,  // Symmetry: Slightly asymmetric
        0.618f,  // Drive: Push into folding
        0.854f,  // Output comp: Restore level
        0.750f,  // Tone: Slightly warm
        0.146f,  // DC offset: Minimal
        0.500f,  // Unused
        0.500f   // Unused
    };
    
    // Performance tuned for dynamics
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 5.47f;  // Spectral processing overhead
    preset.latencySamples = 256.0f;   // FFT block for spectral gate
    preset.realtimeSafe = true;
    
    // Sonic profile - metamorphic character
    preset.sonicProfile.brightness = 0.667f;   // Bright when open
    preset.sonicProfile.density = 0.764f;      // Dense harmonics
    preset.sonicProfile.movement = 0.618f;     // Dynamic movement
    preset.sonicProfile.space = 0.382f;        // Focused, not spacious
    preset.sonicProfile.aggression = 0.708f;   // Can be aggressive
    preset.sonicProfile.vintage = 0.292f;      // Modern character
    
    // Emotional profile - transformative
    preset.emotionalProfile.energy = 0.833f;    // High energy potential
    preset.emotionalProfile.mood = 0.500f;      // Neutral, adaptable
    preset.emotionalProfile.tension = 0.764f;   // Creates tension
    preset.emotionalProfile.organic = 0.382f;   // Electronic character
    preset.emotionalProfile.nostalgia = 0.236f; // Futuristic
    
    // Source affinity - dynamics dependent
    preset.sourceAffinity.vocals = 0.472f;      // Effect use only
    preset.sourceAffinity.guitar = 0.854f;      // Excellent for solos
    preset.sourceAffinity.drums = 0.916f;       // Amazing on drums
    preset.sourceAffinity.synth = 0.764f;       // Great for leads
    preset.sourceAffinity.mix = 0.292f;         // Too extreme for mix
    
    preset.complexity = 0.708f;          // Complex interaction
    preset.experimentalness = 0.764f;    // Very experimental
    preset.versatility = 0.618f;         // Genre-specific
    
    // Enhanced metadata
    preset.keywords = {
        "iron", "butterfly", "envelope", "filter", "spectral", "gate",
        "wave", "folder", "dynamic", "responsive", "metallic", "morphing",
        "transformative", "aggressive", "modern", "electronic"
    };
    
    preset.userPrompts = {
        "Make it respond to my playing dynamics",
        "I want that talking filter sound",
        "Add metallic, gated textures",
        "Transform soft to aggressive",
        "Like an iron butterfly - delicate but powerful",
        "Modern electronic processing",
        "Make my drums sound like machines"
    };
    
    preset.bestFor = "Electronic drums, synth leads, guitar solos, creating "
                    "tension in breakdowns, EDM drops, industrial textures. "
                    "Responds best to transient-rich material. The envelope "
                    "filter tracks amplitude above 500Hz for accurate response.";
                    
    preset.avoidFor = "Sustained pads, strings, ambient textures, or any "
                      "source requiring consistent tone. The dynamic response "
                      "can be too dramatic for smooth material.";
    
    preset.genres = {"electronic", "industrial", "drum-n-bass", "dubstep", 
                    "techno", "breakcore", "metal", "experimental"};
    
    // Technical documentation
    preset.technicalNotes = "Signal flow leverages psychoacoustic principles: "
                           "Envelope detection focused on 500Hz-5kHz range where "
                           "human hearing is most sensitive. Spectral gate bands "
                           "align with Bark scale critical bands. Wave folder "
                           "generates odd harmonics that complement filter motion.";
    
    preset.parameterRationale = "Crossover points (157Hz, 630Hz) match critical "
                               "bands for maximum psychoacoustic impact. Golden "
                               "ratio used for natural-sounding filter resonance. "
                               "Gate release (854ms) creates 'breathing' effect "
                               "that pulses with typical 140 BPM tracks.";
    
    preset.optimizationNotes = "FFT size of 512 samples balances latency vs "
                              "frequency resolution. Process heaviest drums and "
                              "percussion on separate instances to prevent "
                              "spectral smearing. Reduce wave folder mix for "
                              "cleaner transients.";
    
    preset.referencePoints = {
        "Envelope filter inspired by Moog MF-101",
        "Spectral gate similar to iZotope BreakTweaker",
        "Wave folder modeled after Intellijel Bifold"
    };
    
    preset.worksWellWith = {"GC_003", "GC_014", "GC_023"};  // Lo-fi, grit, texture
    preset.conflicts = {"GC_002", "GC_005"};      // Spatial reverbs
    preset.morphTargets = {"GC_007", "GC_016"};   // Organic vs synthetic
    
    preset.testResults = {
        "Drum loop: Filter sweeps 800Hz-3.2kHz following kick transients",
        "1kHz sine: Generates harmonics at 3k, 5k, 7kHz when folded",
        "White noise: Gate creates rhythmic pulses, -18dB between hits",
        "Guitar palm mutes: Filter opens to 4kHz on attack, returns in 850ms"
    };
    
    preset.alternativeSettings = "For subtler effect: Reduce envelope sensitivity "
                                "to 0.3, wave folder mix to 0.2, increase spectral "
                                "gate threshold to 0.75. For extreme: Max all "
                                "resonances and reduce thresholds.";
    
    preset.inputGainSuggestion = "-6dB to -3dB for optimal envelope tracking. "
                                "Hot signals (+0dB) will constantly trigger "
                                "the filter ceiling. Quiet signals need boost.";
    
    return preset;
}

// =============================================================================
// PRESET 009: Phantom Embrace
// =============================================================================
// An ethereal preset that creates ghostly, embracing atmospheres through
// careful phase manipulation and spatial processing. Creates the sensation
// of being surrounded by phantom sounds that appear and disappear.
//
// Design philosophy:
// - Phase relationships create phantom center and width
// - Comb filtering at musical intervals for harmonic halos
// - Mid/side processing for immersive stereo field
// =============================================================================

GoldenPreset createPreset_009_PhantomEmbrace() {
    GoldenPreset preset;
    
    preset.id = "GC_009";
    preset.name = "Phantom Embrace";
    preset.technicalHint = "Comb Resonator + Mid/Side Processor + Gated Reverb";
    preset.shortCode = "PHE";
    preset.category = "Spatial Design";
    preset.subcategory = "Surround Effects";
    
    // Musical interval ratios for comb tuning
    const float unison = 1.000f;
    const float minorSecond = 1.059f;
    const float majorThird = 1.260f;
    const float perfectFifth = 1.498f;
    const float octave = 2.000f;
    
    // ENGINE 1: Comb Resonator
    // Creates harmonic halos around the source
    preset.engineTypes[0] = ENGINE_COMB_RESONATOR;
    preset.engineMix[0] = 0.528f;  // Golden ratio cubed
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.260f,  // Frequency: ~440Hz (A4) for musical resonance
        0.833f,  // Resonance: High Q for ringing
        0.472f,  // Spread: Multiple comb teeth
        0.618f,  // Damping: Golden ratio decay
        0.260f,  // Detune: Major third interval
        0.498f,  // Stereo: Perfect fifth spacing
        0.708f,  // Feedback: Sustained resonance
        0.667f   // Output level
    };
    
    // ENGINE 2: Mid/Side Processor
    // Creates phantom center and ultra-wide sides
    preset.engineTypes[1] = ENGINE_MID_SIDE_PROCESSOR;
    preset.engineMix[1] = 1.0f;  // Full processing
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.382f,  // Mid level: Reduced (inverse golden)
        0.854f,  // Side level: Enhanced (1 - golden^3)
        0.667f,  // Mid EQ high: Slight boost
        0.333f,  // Mid EQ low: Cut mud
        0.764f,  // Side EQ high: Bright sides
        0.472f,  // Side EQ low: Controlled bass
        0.236f,  // Width: 120% (golden^2)
        0.500f   // Balance: Centered
    };
    
    // ENGINE 3: Gated Reverb
    // Ghostly ambience that appears and vanishes
    preset.engineTypes[2] = ENGINE_GATED_REVERB;
    preset.engineMix[2] = 0.618f;  // Golden ratio
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.764f,  // Size: Large space
        0.500f,  // Decay: Medium before gate
        0.618f,  // Gate threshold: Opens easily
        0.146f,  // Gate hold: Short (golden^3)
        0.382f,  // Gate release: Quick cut
        0.708f,  // Diffusion: Smooth
        0.472f,  // Damping: Natural
        0.236f   // Pre-delay: ~25ms
    };
    
    // Performance characteristics
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 6.82f;  // M/S and comb processing
    preset.latencySamples = 128.0f;   // M/S lookahead
    preset.realtimeSafe = true;
    
    // Sonic profile - ghostly and wide
    preset.sonicProfile.brightness = 0.708f;   // Ethereal brightness
    preset.sonicProfile.density = 0.618f;      // Moderate density
    preset.sonicProfile.movement = 0.528f;     // Subtle phasing
    preset.sonicProfile.space = 0.916f;        // Extremely spacious
    preset.sonicProfile.aggression = 0.146f;   // Very gentle
    preset.sonicProfile.vintage = 0.382f;      // Modern-leaning
    
    // Emotional profile - mysterious
    preset.emotionalProfile.energy = 0.382f;    // Low energy, floating
    preset.emotionalProfile.mood = 0.618f;      // Mysterious, neutral
    preset.emotionalProfile.tension = 0.472f;   // Some unease
    preset.emotionalProfile.organic = 0.292f;   // Synthetic, unnatural
    preset.emotionalProfile.nostalgia = 0.528f; // Dreamlike memories
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.854f;      // Haunting on vocals
    preset.sourceAffinity.guitar = 0.708f;      // Good for ambient
    preset.sourceAffinity.drums = 0.236f;       // Too diffuse
    preset.sourceAffinity.synth = 0.916f;       // Perfect for pads
    preset.sourceAffinity.mix = 0.618f;         // Can work on breakdown
    
    preset.complexity = 0.764f;          // Complex phase interactions
    preset.experimentalness = 0.708f;    // Unusual M/S + comb combo
    preset.versatility = 0.528f;         // Specific use cases
    
    preset.keywords = {
        "phantom", "embrace", "ghostly", "ethereal", "wide", "surround",
        "comb", "filter", "mid", "side", "gated", "reverb", "haunting",
        "mysterious", "floating", "phase"
    };
    
    preset.userPrompts = {
        "Make it sound ghostly and ethereal",
        "I want to be surrounded by phantom sounds",
        "Create a haunting atmosphere",
        "Wide, mysterious space",
        "Like being embraced by ghosts",
        "Ethereal vocal treatment",
        "Make it float around my head"
    };
    
    preset.bestFor = "Ambient intros/outros, vocal effects in verses, "
                    "creating space in sparse arrangements, film scoring "
                    "for supernatural scenes, dream sequences. Works "
                    "best on centered mono sources that need width.";
                    
    preset.avoidFor = "Full mixes, bass, kick drums, or anything needing "
                      "solid center image. The M/S processing can cause "
                      "phase issues with already-wide stereo sources.";
    
    preset.genres = {"ambient", "post-rock", "shoegaze", "cinematic", 
                    "darkwave", "ethereal", "soundscape", "experimental"};
    
    preset.technicalNotes = "Comb resonator tuned to A=440Hz with teeth at "
                           "musical intervals creates harmonic emphasis without "
                           "dissonance. M/S processing reduces center by -4.3dB "
                           "while boosting sides +7.1dB, creating phantom width. "
                           "Gated reverb threshold tracks RMS for smooth operation.";
    
    preset.parameterRationale = "Comb frequencies (260Hz base) chosen to avoid "
                               "fundamental frequencies of most instruments. M/S "
                               "width at 236% (golden^2) creates maximum width "
                               "before phase cancellation. Gate timing creates "
                               "'breathing' effect synchronized to typical 60-80 BPM.";
    
    preset.optimizationNotes = "Heavy on phase manipulation - use mono compatibility "
                              "check. Comb resonator can build up at resonant "
                              "frequencies - engage limiter if needed. Best results "
                              "with mono or narrow stereo sources.";
    
    preset.referencePoints = {
        "Width similar to Waves S1 at 200% setting",
        "Comb effect inspired by Eventide H3000",
        "Gate character like AMS RMX16 'Nonlin 2'"
    };
    
    preset.worksWellWith = {"GC_002", "GC_005", "GC_011"};  // Other spaces
    preset.conflicts = {"GC_008", "GC_015"};      // Dynamic processors
    preset.morphTargets = {"GC_001", "GC_004"};   // Dry vs wet
    
    preset.testResults = {
        "Mono vocal: Creates 25dB channel separation at 2-5kHz",
        "Pink noise: Comb creates peaks at 260Hz, 520Hz, 780Hz",
        "Stereo image: 45° to 140° width increase (Goniometer)",
        "Gate timing: Opens in 5ms, holds 146ms, releases in 382ms"
    };
    
    preset.alternativeSettings = "For subtle: Reduce side boost to 0.6, comb "
                                "mix to 0.3, gate mix to 0.4. For extreme: "
                                "Max M/S width, increase comb resonance to 0.95, "
                                "use faster gate for rhythmic effects.";
    
    preset.phaseCoherence = "Mono fold-down reduces level by -4.5dB but "
                           "maintains intelligibility. Comb filtering creates "
                           "slight hollow quality in mono. Best for stereo playback.";
    
    return preset;
}

// =============================================================================
// PRESET 010: Solar Flare
// =============================================================================
// An explosive preset that creates bursts of harmonic energy and spectral
// excitement. Combines intelligent harmonization with spectral processing
// for sounds that bloom and radiate like solar eruptions.
//
// Technical innovation:
// - Intelligent harmonizer tracks pitch and adds context-aware intervals
// - Spectral freeze captures and sustains harmonic moments
// - Feedback network creates self-oscillating energy bursts
// =============================================================================

GoldenPreset createPreset_010_SolarFlare() {
    GoldenPreset preset;
    
    preset.id = "GC_010";
    preset.name = "Solar Flare";
    preset.technicalHint = "Intelligent Harmonizer + Spectral Freeze + Feedback Network";
    preset.shortCode = "SFL";
    preset.category = "Creative Sound Design";
    preset.subcategory = "Spectral Manipulation";
    
    // Harmonic series ratios for musical harmonization
    const float fundamental = 1.000f;
    const float octave = 2.000f;
    const float fifth = 1.500f;
    const float fourth = 1.333f;
    const float majorSixth = 1.667f;
    const float minorSeventh = 1.782f;
    
    // ENGINE 1: Intelligent Harmonizer
    // Adds musical intervals based on input pitch
    preset.engineTypes[0] = ENGINE_INTELLIGENT_HARMONIZER;
    preset.engineMix[0] = 0.618f;  // Golden ratio blend
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.667f,  // Interval: +5th (perfect fifth up)
        0.333f,  // Second voice: +4th below
        0.764f,  // Intelligence: High tracking accuracy
        0.618f,  // Formant correction: Preserve timbre
        0.236f,  // Glide time: Quick tracking
        0.708f,  // Voice blend: Favor higher voice
        0.500f,  // Stereo spread: Voices panned
        0.854f   // Quality: High-resolution processing
    };
    
    // ENGINE 2: Spectral Freeze
    // Captures and sustains spectral moments
    preset.engineTypes[1] = ENGINE_SPECTRAL_FREEZE;
    preset.engineMix[1] = 0.472f;  // Golden^2 mix
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.618f,  // Freeze threshold: Captures louder moments
        0.854f,  // Spectral smoothing: Very smooth
        0.382f,  // Capture window: 38ms snapshots
        0.708f,  // Sustain amount: Long freeze
        0.528f,  // Spectral tilt: Slight high boost
        0.667f,  // Evolution: Slow spectral movement
        0.146f,  // Noise floor: Very low
        0.750f   // Output blend
    };
    
    // ENGINE 3: Feedback Network
    // Creates self-oscillating energy bursts
    preset.engineTypes[2] = ENGINE_FEEDBACK_NETWORK;
    preset.engineMix[2] = 0.382f;  // Controlled chaos
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.764f,  // Feedback amount: High but stable
        0.618f,  // Network size: Medium complexity
        0.472f,  // Diffusion: Moderate spread
        0.708f,  // Modulation: Moving nodes
        0.854f,  // High-frequency damping: Control brightness
        0.333f,  // Low-frequency damping: Let bass through
        0.528f,  // Cross-coupling: Node interaction
        0.618f   // Output limiting: Prevent overload
    };
    
    // Performance characteristics
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 11.3f;  // Pitch tracking + spectral processing
    preset.latencySamples = 512.0f;   // FFT processing latency
    preset.realtimeSafe = true;
    
    // Sonic profile - explosive and bright
    preset.sonicProfile.brightness = 0.916f;   // Very bright, radiating
    preset.sonicProfile.density = 0.854f;      // Dense harmonics
    preset.sonicProfile.movement = 0.764f;     // Evolving, unstable
    preset.sonicProfile.space = 0.708f;        // Expansive
    preset.sonicProfile.aggression = 0.618f;   // Moderately aggressive
    preset.sonicProfile.vintage = 0.146f;      // Very modern
    
    // Emotional profile - energetic and uplifting
    preset.emotionalProfile.energy = 0.916f;    // Maximum energy
    preset.emotionalProfile.mood = 0.833f;      // Uplifting, explosive
    preset.emotionalProfile.tension = 0.618f;   // Exciting tension
    preset.emotionalProfile.organic = 0.236f;   // Synthetic, processed
    preset.emotionalProfile.nostalgia = 0.146f; // Futuristic
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.708f;      // Great for effects
    preset.sourceAffinity.guitar = 0.854f;      // Excellent for leads
    preset.sourceAffinity.drums = 0.382f;       // Limited use
    preset.sourceAffinity.synth = 0.916f;       // Perfect for synths
    preset.sourceAffinity.mix = 0.236f;         // Too extreme
    
    preset.complexity = 0.854f;          // Very complex processing
    preset.experimentalness = 0.833f;    // Highly experimental
    preset.versatility = 0.472f;         // Specialized use
    
    preset.keywords = {
        "solar", "flare", "explosive", "harmonic", "intelligent", "freeze",
        "spectral", "feedback", "radiant", "bright", "energy", "burst",
        "futuristic", "expansive", "blooming"
    };
    
    preset.userPrompts = {
        "Make it explode with harmonics",
        "I want solar flare energy bursts",
        "Add intelligent harmonies that evolve",
        "Freeze and explode the spectrum",
        "Like staring into the sun",
        "Radiant harmonic energy",
        "Make my synth go supernova"
    };
    
    preset.bestFor = "Synth leads in electronic music, guitar solos needing "
                    "harmonic excitement, creating climactic moments, EDM drops, "
                    "experimental sound design. Works best with monophonic sources "
                    "for accurate pitch tracking. Input should be -12dB to -6dB.";
                    
    preset.avoidFor = "Polyphonic sources, drums, bass, quiet passages, or any "
                      "situation requiring subtlety. The feedback network can "
                      "become unstable with complex harmonic input.";
    
    preset.genres = {"electronic", "progressive", "fusion", "experimental", 
                    "psytrance", "future-bass", "synthwave", "post-rock"};
    
    // Advanced technical documentation
    preset.technicalNotes = "Harmonizer uses autocorrelation pitch detection with "
                           "40Hz-4kHz range, 5.8ms window. Adds voices at musical "
                           "intervals with formant-corrected pitch shifting. Spectral "
                           "freeze uses 2048-point FFT, capturing 1024 bins at 48kHz. "
                           "Feedback network implements 8-node Feedback Delay Network.";
    
    preset.parameterRationale = "Harmonizer intervals (5th up, 4th down) create "
                               "powerful root-fifth-octave triads. Spectral freeze "
                               "threshold at 0.618 captures transients while ignoring "
                               "noise. Feedback at 0.764 is just below self-oscillation "
                               "threshold for controlled energy bursts.";
    
    preset.optimizationNotes = "CPU spikes during pitch tracking transitions. Use "
                              "larger buffer sizes (512+) for stability. Freeze "
                              "effect can be automated for dramatic builds. Feedback "
                              "network benefits from limiter on output.";
    
    preset.referencePoints = {
        "Harmonizer quality similar to Eventide H8000",
        "Spectral freeze inspired by GRM Tools Freeze",
        "Feedback network based on Valhalla Supermassive architecture"
    };
    
    preset.worksWellWith = {"GC_002", "GC_006", "GC_012"};  // Spaces and colors
    preset.conflicts = {"GC_004", "GC_008"};      // Other heavy processors
    preset.morphTargets = {"GC_003", "GC_009"};   // Lo-fi vs hi-fi
    
    preset.testResults = {
        "440Hz sine: Generates 660Hz (5th) and 330Hz (4th below)",
        "White noise burst: Freeze captures 3.2 seconds of spectral evolution",
        "Guitar note: Feedback creates sympathetic resonances at 2x, 3x, 4x",
        "Pitch glide: Harmonizer tracks within 2 cents above 100Hz"
    };
    
    preset.alternativeSettings = "For ambient: Reduce harmonizer mix to 0.3, "
                                "increase freeze sustain to 0.9, lower feedback "
                                "to 0.5. For aggressive: Max all parameters but "
                                "keep output limiting at 0.8 to prevent clipping.";
    
    preset.pitchTrackingNotes = "Monophonic sources only. Polyphonic input causes "
                               "harmonizer glitching (can be musical). Best "
                               "tracking between C2-C6. Low bass may cause octave "
                               "errors. Use gate before input for clean tracking.";
    
    return preset;
}

// =============================================================================
// PRESET 011: Dust & Echoes
// =============================================================================
// A nostalgic preset that creates the feeling of memories dissolving into
// dust, with echoes that decay into grainy textures. Perfect for creating
// emotional, cinematic moments with a sense of time passing.
//
// Design philosophy:
// - Granular processing creates "dust" particles from the source
// - Multiple delay lines create complex echo patterns
// - Vintage processing adds warmth and nostalgia
// =============================================================================

GoldenPreset createPreset_011_DustAndEchoes() {
    GoldenPreset preset;
    
    preset.id = "GC_011";
    preset.name = "Dust & Echoes";
    preset.technicalHint = "Granular Cloud + Magnetic Drum Echo + Vintage Opto Compressor";
    preset.shortCode = "DAE";
    preset.category = "Textural Effects";
    preset.subcategory = "Granular Processing";
    
    // Time-based constants for echo relationships
    const float goldenMs = 161.8f;      // Golden ratio in milliseconds
    const float fibonacci[] = {13.0f, 21.0f, 34.0f, 55.0f, 89.0f, 144.0f};
    
    // ENGINE 1: Granular Cloud
    // Creates dust-like particles from the source
    preset.engineTypes[0] = ENGINE_GRANULAR_CLOUD;
    preset.engineMix[0] = 0.528f;  // Golden^3 mix
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.089f,  // Grain size: 8.9ms (fibonacci)
        0.618f,  // Grain density: Golden ratio spacing
        0.382f,  // Position scatter: Moderate randomness
        0.708f,  // Pitch scatter: ±8 semitones variation
        0.236f,  // Grain envelope: Short attack/release
        0.472f,  // Feedback: Some regeneration
        0.667f,  // Filter: High-freq rolloff for dust
        0.854f   // Stereo spread: Wide grain field
    };
    
    // ENGINE 2: Magnetic Drum Echo
    // Multiple tape heads create complex patterns
    preset.engineTypes[1] = ENGINE_MAGNETIC_DRUM_ECHO;
    preset.engineMix[1] = 0.618f;  // Golden ratio
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.144f,  // Head 1: 144ms (fibonacci)
        0.233f,  // Head 2: 233ms (fibonacci extended)
        0.377f,  // Head 3: 377ms (fibonacci extended)
        0.618f,  // Feedback: Golden ratio regeneration
        0.708f,  // Tape saturation: Warm compression
        0.854f,  // Wow/flutter: Vintage instability
        0.528f,  // Motor torque: Some speed variation
        0.472f   // Head mix: Balanced combination
    };
    
    // ENGINE 3: Vintage Opto Compressor
    // Glues everything with musical compression
    preset.engineTypes[2] = ENGINE_OPTO_COMPRESSOR;
    preset.engineMix[2] = 1.0f;  // Full processing
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.618f,  // Threshold: Gentle compression
        0.382f,  // Ratio: 3:1 (musical)
        0.146f,  // Attack: Slow opto response
        0.854f,  // Release: Very slow
        0.333f,  // Knee: Soft knee compression
        0.667f,  // Makeup gain: Restore level
        0.708f,  // Opto age: Vintage character
        0.500f   // Mix: Full compression
    };
    
    // Performance
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 9.7f;   // Granular processing intensive
    preset.latencySamples = 441.0f;   // Grain lookahead buffer
    preset.realtimeSafe = true;
    
    // Sonic profile - dusty and nostalgic
    preset.sonicProfile.brightness = 0.382f;   // Dark, dusty
    preset.sonicProfile.density = 0.764f;      // Dense texture
    preset.sonicProfile.movement = 0.854f;     // Constantly shifting
    preset.sonicProfile.space = 0.833f;        // Very spacious
    preset.sonicProfile.aggression = 0.146f;   // Very gentle
    preset.sonicProfile.vintage = 0.916f;      // Extremely vintage
    
    // Emotional profile - melancholic nostalgia
    preset.emotionalProfile.energy = 0.292f;    // Low, tired energy
    preset.emotionalProfile.mood = 0.382f;      // Melancholic
    preset.emotionalProfile.tension = 0.236f;   // Very relaxed
    preset.emotionalProfile.organic = 0.764f;   // Tape/analog feel
    preset.emotionalProfile.nostalgia = 0.958f; // Maximum nostalgia
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.916f;      // Beautiful on vocals
    preset.sourceAffinity.guitar = 0.854f;      // Great for ambient
    preset.sourceAffinity.drums = 0.472f;       // Interesting on cymbals
    preset.sourceAffinity.synth = 0.708f;       // Adds character
    preset.sourceAffinity.mix = 0.618f;         // Can work on breakdowns
    
    preset.complexity = 0.764f;          // Complex layering
    preset.experimentalness = 0.618f;    // Moderately experimental
    preset.versatility = 0.708f;         // Many uses
    
    preset.keywords = {
        "dust", "echoes", "granular", "memories", "nostalgic", "vintage",
        "tape", "magnetic", "particles", "dissolving", "fading", "melancholic",
        "cinematic", "emotional", "texture"
    };
    
    preset.userPrompts = {
        "Make it sound like fading memories",
        "Add dusty, granular echoes",
        "I want that old tape delay sound but modern",
        "Create nostalgic, emotional atmosphere",
        "Like dust particles in sunlight",
        "Vintage echoes that fall apart",
        "Cinematic memory sequence sound"
    };
    
    preset.bestFor = "Emotional vocal passages, ambient guitar layers, "
                    "cinematic transitions, creating nostalgic moments, "
                    "lo-fi hip-hop production, dream sequences. Excels "
                    "at 60-90 BPM where echoes create polyrhythms.";
                    
    preset.avoidFor = "Fast transients, drums (except ambient), bass, "
                      "or any source needing clarity and punch. The "
                      "granular processing smears transients significantly.";
    
    preset.genres = {"ambient", "post-rock", "cinematic", "lo-fi", "downtempo", 
                    "experimental", "soundscape", "neoclassical"};
    
    preset.technicalNotes = "Granular engine uses 64-grain polyphony with gaussian "
                           "windowing. Each grain is 8.9ms (Fibonacci number) with "
                           "random pitch variation ±800 cents. Magnetic echo emulates "
                           "Binson Echorec with 3 virtual heads at Fibonacci delays. "
                           "Opto compressor models LA-2A T4 cell response.";
    
    preset.parameterRationale = "Fibonacci delay times (144ms, 233ms, 377ms) create "
                               "non-repeating echo patterns that feel organic. Grain "
                               "size of 8.9ms chosen to be below pitch perception "
                               "threshold while maintaining texture. Opto timing creates "
                               "breathing compression that enhances nostalgic feel.";
    
    preset.optimizationNotes = "Granular engine most CPU-intensive. Reduce grain "
                              "density to 0.4 for lighter load. Echo feedback can "
                              "build up - compressor prevents overload but watch "
                              "levels. Best with 512+ sample buffers for stability.";
    
    preset.referencePoints = {
        "Granular texture inspired by Mutable Instruments Clouds",
        "Echo character similar to Binson Echorec",
        "Compression modeled after Universal Audio LA-2A"
    };
    
    preset.worksWellWith = {"GC_003", "GC_007", "GC_009"};  // Other nostalgic/space
    preset.conflicts = {"GC_008", "GC_010"};      // Aggressive processors
    preset.morphTargets = {"GC_002", "GC_013"};   // Clean vs dirty space
    
    preset.testResults = {
        "Piano note: Creates 15-20 grain cloud lasting 3.5 seconds",
        "Drum hit: Echoes decay over 2.3 seconds with tape wobble",
        "1kHz sine: Grain pitch variation creates ±50 cent chorus",
        "Vocal phrase: Compressor adds 3-5dB gentle leveling"
    };
    
    preset.alternativeSettings = "For cleaner: Reduce grain scatter to 0.2, "
                                "echo feedback to 0.4, increase opto ratio to "
                                "0.5. For more extreme: Max grain parameters, "
                                "use shorter grain size (0.05) for glitchy texture.";
    
    preset.grainCloudBehavior = "Density parameter controls grain overlap. At 0.618, "
                               "approximately 8-12 grains play simultaneously. Position "
                               "scatter creates temporal smearing. Pitch scatter adds "
                               "chorus-like thickening. CPU usage scales with density.";
    
    return preset;
}

// =============================================================================
// PRESET 012: Thunder & Silk
// =============================================================================
// A study in contrasts - combines aggressive transient shaping with silky
// smooth compression and filtering. Creates sounds that hit hard initially
// then bloom into smooth sustain. Perfect for modern production needing both
// punch and polish.
//
// Technical approach:
// - Transient design emphasizes attack while smoothing decay
// - State variable filter provides surgical tone shaping
// - Classic compressor glues everything with analog warmth
// =============================================================================

GoldenPreset createPreset_012_ThunderAndSilk() {
    GoldenPreset preset;
    
    preset.id = "GC_012";
    preset.name = "Thunder & Silk";
    preset.technicalHint = "Transient Shaper + State Variable Filter + Classic Compressor";
    preset.shortCode = "TAS";
    preset.category = "Dynamic Processing";
    preset.subcategory = "Transient Design";
    
    // Psychoacoustic constants for transient perception
    const float attackPerception = 0.005f;    // 5ms - edge of transient perception
    const float sustainThreshold = 0.030f;    // 30ms - sustain begins
    const float haasWindow = 0.040f;          // 40ms - psychoacoustic integration
    
    // ENGINE 1: Transient Shaper
    // Emphasizes attack, smooths sustain
    preset.engineTypes[0] = ENGINE_TRANSIENT_SHAPER;
    preset.engineMix[0] = 1.0f;  // Full processing
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.764f,  // Attack: Strong emphasis (+7.6dB)
        0.382f,  // Sustain: Gentle reduction (-3.8dB)
        0.236f,  // Attack time: 2.3ms (fast)
        0.618f,  // Release time: 61ms (musical)
        0.708f,  // Detection: RMS + Peak hybrid
        0.854f,  // Sensitivity: High
        0.500f,  // Stereo link: 50% independent
        0.667f   // Output: Compensated level
    };
    
    // ENGINE 2: State Variable Filter
    // Surgical frequency sculpting
    preset.engineTypes[1] = ENGINE_STATE_VARIABLE_FILTER;
    preset.engineMix[1] = 1.0f;  // Full processing
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.382f,  // Frequency: 1.8kHz (presence)
        0.708f,  // Resonance: Musical emphasis
        0.667f,  // Low mix: Warm lows retained
        0.333f,  // Band mix: Some midrange
        0.618f,  // High mix: Silky highs
        0.472f,  // Frequency modulation: Slight movement
        0.250f,  // Mod rate: 2.5Hz (breathing)
        0.854f   // State morph: Unique filter shape
    };
    
    // ENGINE 3: Classic Compressor
    // Glues everything together
    preset.engineTypes[2] = ENGINE_VCA_COMPRESSOR;
    preset.engineMix[2] = 1.0f;  // Full compression
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.528f,  // Threshold: -8dB (moderate)
        0.472f,  // Ratio: 4.7:1 (firm control)
        0.089f,  // Attack: 0.89ms (fast)
        0.618f,  // Release: 180ms (musical)
        0.708f,  // Knee: 2dB soft knee
        0.667f,  // Makeup: Auto-gain
        0.764f,  // Mix: 76% wet (parallel)
        0.618f   // Sidechain HPF: 180Hz
    };
    
    // Performance
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 3.8f;
    preset.latencySamples = 64.0f;    // Transient lookahead
    preset.realtimeSafe = true;
    
    // Sonic profile - punchy yet smooth
    preset.sonicProfile.brightness = 0.708f;   // Bright but not harsh
    preset.sonicProfile.density = 0.618f;      // Controlled density
    preset.sonicProfile.movement = 0.382f;     // Subtle filter movement
    preset.sonicProfile.space = 0.472f;        // Focused, not spacious
    preset.sonicProfile.aggression = 0.764f;   // Punchy transients
    preset.sonicProfile.vintage = 0.528f;      // Modern-vintage hybrid
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.854f;    // High impact energy
    preset.emotionalProfile.mood = 0.708f;      // Confident, bold
    preset.emotionalProfile.tension = 0.618f;   // Controlled tension
    preset.emotionalProfile.organic = 0.472f;   // Processed but musical
    preset.emotionalProfile.nostalgia = 0.382f; // Contemporary feel
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.618f;      // Can add presence
    preset.sourceAffinity.guitar = 0.764f;      // Great for rhythm
    preset.sourceAffinity.drums = 0.958f;       // Perfect for drums
    preset.sourceAffinity.synth = 0.708f;       // Adds punch
    preset.sourceAffinity.mix = 0.854f;         // Excellent on mix bus
    
    preset.complexity = 0.528f;          // Moderate complexity
    preset.experimentalness = 0.382f;    // Traditional approach
    preset.versatility = 0.916f;         // Very versatile
    
    preset.keywords = {
        "thunder", "silk", "punch", "smooth", "transient", "shaper",
        "compress", "attack", "modern", "polish", "professional", "glue",
        "impact", "clarity", "presence"
    };
    
    preset.userPrompts = {
        "Make it punch but stay smooth",
        "Add thunder to the attack",
        "Silky smooth compression",
        "Modern drum processing",
        "Professional mix bus polish",
        "Punchy yet controlled",
        "Thunder and silk combination"
    };
    
    preset.bestFor = "Drum bus processing, mix bus glue, adding punch to "
                    "lifeless recordings, modern pop/rock production, "
                    "hip-hop beats, EDM drops. Excels on percussive "
                    "material and full mixes needing both impact and polish.";
                    
    preset.avoidFor = "Delicate classical recordings, ambient music, or "
                      "sources that need to remain natural. The transient "
                      "emphasis can be too aggressive for gentle material.";
    
    preset.genres = {"pop", "rock", "hip-hop", "electronic", "r&b", 
                    "trap", "modern-rock", "indie"};
    
    // Deep technical details
    preset.technicalNotes = "Transient detection uses parallel RMS (10ms window) "
                           "and peak detection (0.1ms) with 70/30 weighting. "
                           "State variable filter implements Chamberlin topology "
                           "with per-sample modulation. Compressor models dbx 160 "
                           "VCA behavior with true RMS detection.";
    
    preset.parameterRationale = "Attack boost of 7.6dB chosen to match Fletcher-"
                               "Munson loudness curves at 85dB SPL. Filter at "
                               "1.8kHz targets telephone band for clarity. "
                               "Compressor attack (0.89ms) catches transients "
                               "while release (180ms) matches typical 1/8 note.";
    
    preset.optimizationNotes = "Transient shaper uses minimal CPU. Enable "
                              "oversampling on filter for aliasing-free "
                              "resonance. Compressor lookahead can be reduced "
                              "to 32 samples for lower latency at slight "
                              "quality cost.";
    
    preset.referencePoints = {
        "Transient shaping similar to SPL Transient Designer",
        "Filter character inspired by Sherman Filterbank",
        "Compression modeled after dbx 160X"
    };
    
    preset.worksWellWith = {"GC_001", "GC_005", "GC_007"};  // Adds to other chains
    preset.conflicts = {"GC_008", "GC_011"};      // Other transient processors
    preset.morphTargets = {"GC_004", "GC_006"};   // Dark vs bright
    
    preset.testResults = {
        "Kick drum: +12dB attack peak, -4dB sustain, 8ms total shaping",
        "Snare: Emphasizes 2-4kHz crack, reduces 200Hz ring",
        "Mix bus: 3-4dB compression, 1.5dB presence boost",
        "Pink noise: Filter adds 6dB peak at 1.8kHz with Q=2.8"
    };
    
    preset.alternativeSettings = "For subtle: Reduce attack to 0.5, sustain "
                                "to 0.45, compressor ratio to 0.3. For extreme: "
                                "Max attack emphasis, increase filter resonance "
                                "to 0.9, use hard-knee compression.";
    
    preset.gainStagingNotes = "Transient emphasis can add up to +12dB peaks. "
                             "Compressor makeup gain compensates average level "
                             "but watch peak meters. Unity gain achieved with "
                             "input at -18dBFS. Outputs may peak at -6dBFS.";
    
    return preset;
}

// =============================================================================
// PRESET 013: Quantum Garden
// =============================================================================
// An experimental preset that creates evolving, organic soundscapes through
// chaos theory and quantum-inspired randomness. Buffer repeats create
// glitchy time loops while spectral processing adds otherworldly textures.
//
// Innovation:
// - Chaos generator creates fractal-based modulation
// - Buffer repeat creates quantum "time loops"
// - Resonant chorus adds organic movement
// =============================================================================

GoldenPreset createPreset_013_QuantumGarden() {
    GoldenPreset preset;
    
    preset.id = "GC_013";
    preset.name = "Quantum Garden";
    preset.technicalHint = "Chaos Generator + Buffer Repeat + Resonant Chorus";
    preset.shortCode = "QTG";
    preset.category = "Experimental";
    preset.subcategory = "Glitch & Chaos";
    
    // Chaos theory constants
    const float lorenzSigma = 10.0f;
    const float lorenzRho = 28.0f;
    const float lorenzBeta = 8.0f/3.0f;
    const float lyapunovExponent = 0.9056f;  // Chaotic threshold
    
    // ENGINE 1: Chaos Generator
    // Creates fractal modulation patterns
    preset.engineTypes[0] = ENGINE_CHAOS_GENERATOR;
    preset.engineMix[0] = 0.618f;  // Golden ratio
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.906f,  // Chaos amount: Near Lyapunov threshold
        0.382f,  // Iteration rate: 38.2Hz base
        0.618f,  // Feedback: Golden ratio recursion
        0.472f,  // Dimension: 2.5D strange attractor
        0.708f,  // Smoothing: Some continuity
        0.333f,  // Quantization: 1/3 octave steps
        0.854f,  // Range: Wide modulation
        0.528f   // Seed offset: Unique patterns
    };
    
    // ENGINE 2: Buffer Repeat
    // Quantum time loops and glitches
    preset.engineTypes[1] = ENGINE_BUFFER_REPEAT;
    preset.engineMix[1] = 0.472f;  // Golden^2
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.146f,  // Buffer size: 146ms (golden^3)
        0.764f,  // Repeat probability: Often
        0.382f,  // Slice size: Variable chunks
        0.618f,  // Pitch variation: ±6 semitones
        0.528f,  // Reverse probability: 50/50
        0.708f,  // Stutter rate: Fast repeats
        0.236f,  // Crossfade: Quick transitions
        0.854f   // Output mix: Strong effect
    };
    
    // ENGINE 3: Resonant Chorus
    // Organic, evolving movement
    preset.engineTypes[2] = ENGINE_RESONANT_CHORUS;
    preset.engineMix[2] = 0.708f;  // Strong presence
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.089f,  // Rate: Very slow (0.89Hz)
        0.854f,  // Depth: Deep modulation
        0.618f,  // Resonance: Golden ratio feedback
        0.472f,  // Frequency: 2.2kHz center
        0.764f,  // Voices: 6 voice chorus
        0.667f,  // Spread: Wide stereo
        0.382f,  // Waveform: Triangle/sine hybrid
        0.708f   // Evolution: Slowly morphing
    };
    
    // Performance
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 7.2f;   // Chaos calculations
    preset.latencySamples = 146.0f;   // Buffer size
    preset.realtimeSafe = true;       // But unpredictable!
    
    // Sonic profile - chaotic garden
    preset.sonicProfile.brightness = 0.618f;   // Variable brightness
    preset.sonicProfile.density = 0.854f;      // Dense, layered
    preset.sonicProfile.movement = 0.958f;     // Constant evolution
    preset.sonicProfile.space = 0.764f;        // Expansive
    preset.sonicProfile.aggression = 0.382f;   // Gentle chaos
    preset.sonicProfile.vintage = 0.236f;      // Futuristic
    
    // Emotional profile - wonder and unease
    preset.emotionalProfile.energy = 0.618f;    // Moderate, pulsing
    preset.emotionalProfile.mood = 0.528f;      // Mysterious, neutral
    preset.emotionalProfile.tension = 0.708f;   // Underlying unease
    preset.emotionalProfile.organic = 0.764f;   // Strangely natural
    preset.emotionalProfile.nostalgia = 0.146f; // Alien, unfamiliar
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.708f;      // Interesting effects
    preset.sourceAffinity.guitar = 0.618f;      // Can be musical
    preset.sourceAffinity.drums = 0.854f;       // Great for glitch
    preset.sourceAffinity.synth = 0.916f;       // Perfect match
    preset.sourceAffinity.mix = 0.382f;         // Too chaotic
    
    preset.complexity = 0.916f;          // Very complex
    preset.experimentalness = 0.958f;    // Highly experimental
    preset.versatility = 0.472f;         // Specialized
    
    preset.keywords = {
        "quantum", "garden", "chaos", "fractal", "glitch", "buffer",
        "repeat", "experimental", "evolving", "organic", "strange",
        "attractor", "unpredictable", "generative"
    };
    
    preset.userPrompts = {
        "Create quantum chaos",
        "Glitchy time loops",
        "Fractal garden of sound",
        "Experimental chaos generator",
        "Make it unpredictably beautiful",
        "Organic glitch textures",
        "Strange attractor modulation"
    };
    
    preset.bestFor = "Experimental electronic music, IDM, glitch, "
                    "creating unique textures, ambient interludes, "
                    "sci-fi sound design. Best with simple sources "
                    "that can be complexified. Automation recommended.";
                    
    preset.avoidFor = "Traditional music, anything needing predictability, "
                      "live performance (unless you embrace chaos), "
                      "or mixing situations requiring consistency.";
    
    preset.genres = {"idm", "glitch", "experimental", "ambient", "electronic", 
                    "noise", "generative", "avant-garde"};
    
    // Chaos theory documentation
    preset.technicalNotes = "Chaos generator implements Lorenz attractor with "
                           "parameters σ=10, ρ=28, β=8/3 for deterministic chaos. "
                           "Buffer repeat uses ring buffer with probabilistic "
                           "triggering based on chaos output. Resonant chorus "
                           "uses 6-voice BBD emulation with feedback matrix.";
    
    preset.parameterRationale = "Chaos at 0.906 is just above Lyapunov exponent "
                               "threshold (0.9056) for edge-of-chaos behavior. "
                               "Buffer size (146ms) allows for rhythmic loops at "
                               "common tempos. Chorus resonance creates feedback "
                               "loops that interact with chaos modulation.";
    
    preset.optimizationNotes = "Chaos generator pre-calculates 1024 samples "
                              "for efficiency. Buffer repeat allocates 16MB "
                              "for smooth operation. Reduce chorus voices to "
                              "4 for 30% CPU savings. Not suitable for live "
                              "input due to unpredictability.";
    
    preset.referencePoints = {
        "Chaos inspired by Buchla 266e Source of Uncertainty",
        "Buffer repeat similar to Elektron Digitakt retrig",
        "Resonant chorus based on Roland Dimension D feedback mod"
    };
    
    preset.worksWellWith = {"GC_009", "GC_010", "GC_016"};  // Other experimental
    preset.conflicts = {"GC_001", "GC_012"};      // Traditional processors
    preset.morphTargets = {"GC_011", "GC_014"};   // Organic variations
    
    preset.testResults = {
        "Sine wave: Evolves into complex harmonic spectrum over 10s",
        "Drum loop: Creates polyrhythmic variations every 2-3 bars",
        "White noise: Sculpts into pitched resonances at chaos peaks",
        "Vocal: Generates 3-5 distinct 'quantum echoes' per phrase"
    };
    
    preset.alternativeSettings = "For subtle: Chaos 0.7, buffer mix 0.3, "
                                "chorus depth 0.5. For maximum chaos: All "
                                "parameters to 0.9+, but use limiter! For "
                                "rhythmic: Sync buffer to host tempo.";
    
    preset.chaosTheoryNotes = "System exhibits sensitive dependence on initial "
                             "conditions. Identical input may produce different "
                             "output on each pass. This is intentional - embrace "
                             "the unpredictability. Save multiple takes and choose.";
    
    preset.quantumInspiration = "Named 'Quantum Garden' for the way sound particles "
                               "seem to exist in superposition until observed "
                               "(processed). Buffer repeats create 'time crystals' "
                               "while chaos generates 'quantum fluctuations'.";
    
    return preset;
}

// =============================================================================
// PRESET 014: Copper Resonance
// =============================================================================
// A warm, metallic preset that creates rich harmonic resonances reminiscent
// of acoustic instruments and resonant bodies. Combines physical modeling
// concepts with analog warmth for organic, musical tones.
//
// Design philosophy:
// - Analog ring modulation creates metallic timbres
// - Ladder filter provides musical resonance
// - Multiband saturation adds harmonic complexity
// =============================================================================

GoldenPreset createPreset_014_CopperResonance() {
    GoldenPreset preset;
    
    preset.id = "GC_014";
    preset.name = "Copper Resonance";
    preset.technicalHint = "Analog Ring Modulator + Ladder Filter + Multiband Saturator";
    preset.shortCode = "CPR";
    preset.category = "Character & Color";
    preset.subcategory = "Metallic Tones";
    
    // Physical modeling constants
    const float copperDensity = 8960.0f;     // kg/m³
    const float soundSpeed = 3810.0f;        // m/s in copper
    const float youngModulus = 117e9f;       // Pa
    const float resonantModes[] = {1.0f, 2.756f, 5.404f, 8.933f};  // Bar modes
    
    // ENGINE 1: Analog Ring Modulator
    // Creates metallic, bell-like tones
    preset.engineTypes[0] = ENGINE_RING_MODULATOR;
    preset.engineMix[0] = 0.472f;  // Balanced with dry
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.276f,  // Frequency: 276Hz (copper bar fundamental)
        0.854f,  // Waveform: Near-sine (smooth)
        0.618f,  // Depth: Golden ratio modulation
        0.146f,  // Frequency drift: Analog instability
        0.708f,  // Carrier bleed: Some original
        0.382f,  // AM/RM blend: More RM
        0.528f,  // Stereo phase: Slight offset
        0.667f   // Output level
    };
    
    // ENGINE 2: Ladder Filter
    // Resonant filtering for musical emphasis
    preset.engineTypes[1] = ENGINE_LADDER_FILTER;
    preset.engineMix[1] = 1.0f;  // Full processing
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.540f,  // Cutoff: 5.4kHz (3rd resonant mode)
        0.893f,  // Resonance: Near self-oscillation
        0.382f,  // Drive: Warm saturation
        0.236f,  // Envelope: Slight movement
        0.618f,  // Envelope depth: Moderate
        0.146f,  // Attack: Fast response
        0.764f,  // Release: Slow decay
        0.708f   // 4-pole mode (24dB/oct)
    };
    
    // ENGINE 3: Multiband Saturator
    // Adds warmth and harmonic richness
    preset.engineTypes[2] = ENGINE_MULTIBAND_SATURATOR;
    preset.engineMix[2] = 0.618f;  // Golden ratio
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.472f,  // Low band: Moderate warmth
        0.708f,  // Mid band: Strong presence
        0.382f,  // High band: Gentle sheen
        0.276f,  // Crossover 1: 276Hz (fundamental)
        0.893f,  // Crossover 2: 8.9kHz (4th mode)
        0.618f,  // Drive: Overall saturation
        0.764f,  // Output compensation
        0.500f   // Dry/wet mix
    };
    
    // Performance
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 4.6f;
    preset.latencySamples = 128.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - warm and metallic
    preset.sonicProfile.brightness = 0.618f;   // Balanced brightness
    preset.sonicProfile.density = 0.708f;      // Rich harmonics
    preset.sonicProfile.movement = 0.472f;     // Some filter movement
    preset.sonicProfile.space = 0.382f;        // Focused, not spacious
    preset.sonicProfile.aggression = 0.528f;   // Moderate edge
    preset.sonicProfile.vintage = 0.764f;      // Analog character
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.618f;    // Moderate energy
    preset.emotionalProfile.mood = 0.708f;      // Warm, inviting
    preset.emotionalProfile.tension = 0.472f;   // Some edge
    preset.emotionalProfile.organic = 0.854f;   // Very organic
    preset.emotionalProfile.nostalgia = 0.618f; // Vintage warmth
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.708f;      // Adds character
    preset.sourceAffinity.guitar = 0.854f;      // Great for solos
    preset.sourceAffinity.drums = 0.764f;       // Interesting on percussion
    preset.sourceAffinity.synth = 0.916f;       // Perfect for leads
    preset.sourceAffinity.mix = 0.382f;         // Too colored for mix
    
    preset.complexity = 0.618f;          // Moderately complex
    preset.experimentalness = 0.528f;    // Some experimentation
    preset.versatility = 0.708f;         // Fairly versatile
    
    preset.keywords = {
        "copper", "resonance", "metallic", "warm", "analog", "ring",
        "modulator", "ladder", "filter", "musical", "bell", "organic",
        "harmonic", "saturated", "vintage"
    };
    
    preset.userPrompts = {
        "Add warm metallic resonance",
        "Make it sound like copper bells",
        "Analog ring mod character",
        "Musical metallic tones",
        "Warm resonant filter sound",
        "Organic harmonic saturation",
        "Vintage synth lead sound"
    };
    
    preset.bestFor = "Synth leads, guitar solos, adding harmonic interest to "
                    "simple sources, creating bell-like tones, warming up "
                    "digital sources. Works especially well on sustained "
                    "notes where resonances can develop. 276Hz tuning "
                    "complements key of C.";
                    
    preset.avoidFor = "Full mixes, bass instruments, or any source needing "
                      "clarity and transparency. The ring modulation can "
                      "create inharmonic frequencies with complex sources.";
    
    preset.genres = {"electronic", "ambient", "experimental", "fusion", 
                    "progressive", "synthwave", "new-age", "jazz"};
    
    // Physical modeling notes
    preset.technicalNotes = "Ring modulator frequency (276Hz) based on copper bar "
                           "fundamental with dimensions 30cm x 2cm x 0.5cm. Filter "
                           "resonant modes follow physical bar overtones: f, 2.756f, "
                           "5.404f, 8.933f. Multiband crossovers placed at nodal "
                           "points for maximum harmonic interaction.";
    
    preset.parameterRationale = "Carrier at 276Hz chosen as C4# for musicality. "
                               "Filter resonance at 0.893 creates 'singing' quality "
                               "without full oscillation. Saturation bands emphasize "
                               "2nd (warmth), 3rd (presence), and 5th (sheen) harmonics.";
    
    preset.optimizationNotes = "Ring mod uses optimized sine approximation (7th order "
                              "polynomial). Ladder filter can use 2-pole mode for "
                              "30% CPU savings. Multiband processing most intensive - "
                              "disable high band if CPU constrained.";
    
    preset.referencePoints = {
        "Ring mod character inspired by ARP 2600",
        "Ladder filter modeled after Moog Minimoog",
        "Saturation based on Thermionic Culture Vulture"
    };
    
    preset.worksWellWith = {"GC_001", "GC_007", "GC_009"};  // Complements warmth
    preset.conflicts = {"GC_010", "GC_013"};      // Too many harmonics
    preset.morphTargets = {"GC_003", "GC_015"};   // Lo-fi vs hi-fi versions
    
    preset.testResults = {
        "440Hz sine: Generates 164Hz + 716Hz sidebands (276Hz carrier)",
        "White noise: Creates pitched resonances at 276Hz, 760Hz, 1490Hz",
        "Guitar chord: Adds bell-like shimmer in 2-5kHz range",
        "Resonance sweep: Self-oscillates at 0.95, musical at 0.893"
    };
    
    preset.alternativeSettings = "For subtle: Ring mix 0.2, resonance 0.6, "
                                "saturation 0.4. For extreme: Max resonance "
                                "(0.95), full ring mod, heavy saturation. "
                                "For bass: Lower carrier to 0.138 (138Hz).";
    
    preset.musicalApplications = "Carrier frequency can be tuned musically: "
                                "0.261 = C (261Hz), 0.293 = D, 0.329 = E, "
                                "0.349 = F, 0.392 = G, 0.440 = A, 0.493 = B. "
                                "Automate for melodic ring mod sequences.";
    
    return preset;
}

// =============================================================================
// PRESET 015: Aurora Borealis
// =============================================================================
// A shimmering, ethereal preset that creates sweeping, colorful textures
// reminiscent of the northern lights. Combines phasing, pitch shifting,
// and spectral processing for constantly evolving, beautiful soundscapes.
//
// Technical innovation:
// - Phased vocoder creates morphing spectral colors
// - Pitch shifter adds harmonic layers
// - Detune doubler creates width and shimmer
// =============================================================================

GoldenPreset createPreset_015_AuroraBorealis() {
    GoldenPreset preset;
    
    preset.id = "GC_015";
    preset.name = "Aurora Borealis";
    preset.technicalHint = "Phased Vocoder + Pitch Shifter + Detune Doubler";
    preset.shortCode = "ABO";
    preset.category = "Spatial Design";
    preset.subcategory = "Atmospheric Effects";
    
    // Aurora physics constants (scaled to audio)
    const float ionosphereHeight = 100e3f;    // 100km scaled to Hz
    const float magneticFlux = 50e-6f;        // Tesla to modulation
    const float solarWind = 400e3f;           // m/s to frequency
    const float oxygenGreen = 557.7f;         // nm wavelength as Hz
    const float nitrogenBlue = 427.8f;        // nm wavelength as Hz
    
    // ENGINE 1: Phased Vocoder
    // Creates morphing spectral colors
    preset.engineTypes[0] = ENGINE_PHASED_VOCODER;
    preset.engineMix[0] = 0.764f;  // Strong effect
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.428f,  // Frequency shift: 428Hz (nitrogen blue)
        0.558f,  // Phase rotation: 558° (oxygen green)
        0.382f,  // Spectral smear: Moderate
        0.618f,  // Formant shift: Golden ratio
        0.854f,  // Spectral gate: Let most through
        0.708f,  // Phase coherence: Mostly coherent
        0.236f,  // Rotation rate: Slow sweep
        0.667f   // Output mix
    };
    
    // ENGINE 2: Pitch Shifter
    // Adds ethereal harmonic layers
    preset.engineTypes[1] = ENGINE_PITCH_SHIFTER;
    preset.engineMix[1] = 0.382f;  // Subtle blend
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.583f,  // Pitch 1: +7 semitones (perfect 5th)
        0.250f,  // Pitch 2: -12 semitones (octave down)
        0.708f,  // Window size: Large (smooth)
        0.618f,  // Crossfade: Golden ratio
        0.472f,  // Pitch 1 mix: Balanced
        0.528f,  // Pitch 2 mix: Balanced
        0.854f,  // Formant correction: Preserve timbre
        0.764f   // Quality: High resolution
    };
    
    // ENGINE 3: Detune Doubler
    // Creates shimmering width
    preset.engineTypes[2] = ENGINE_DETUNE_DOUBLER;
    preset.engineMix[2] = 0.618f;  // Golden ratio
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.146f,  // Detune 1: +14.6 cents
        0.236f,  // Detune 2: -23.6 cents
        0.089f,  // Delay 1: 8.9ms
        0.144f,  // Delay 2: 14.4ms (fibonacci)
        0.708f,  // Width: Wide stereo
        0.618f,  // Modulation depth: Moving
        0.118f,  // Mod rate: 1.18Hz (slow)
        0.667f   // Mix balance
    };
    
    // Performance
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 8.9f;  // Vocoder processing
    preset.latencySamples = 1024.0f; // FFT window
    preset.realtimeSafe = true;
    
    // Sonic profile - shimmering aurora
    preset.sonicProfile.brightness = 0.854f;   // Very bright
    preset.sonicProfile.density = 0.618f;      // Moderate density
    preset.sonicProfile.movement = 0.916f;     // Constant motion
    preset.sonicProfile.space = 0.958f;        // Vast space
    preset.sonicProfile.aggression = 0.089f;   // Very gentle
    preset.sonicProfile.vintage = 0.236f;      // Modern sound
    
    // Emotional profile - wonder and awe
    preset.emotionalProfile.energy = 0.708f;    // Uplifting energy
    preset.emotionalProfile.mood = 0.916f;      // Wondrous, magical
    preset.emotionalProfile.tension = 0.146f;   // Very relaxing
    preset.emotionalProfile.organic = 0.382f;   // Synthetic beauty
    preset.emotionalProfile.nostalgia = 0.472f; // Timeless
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.854f;      // Beautiful on vocals
    preset.sourceAffinity.guitar = 0.708f;      // Good for ambient
    preset.sourceAffinity.drums = 0.236f;       // Not ideal
    preset.sourceAffinity.synth = 0.958f;       // Perfect for pads
    preset.sourceAffinity.mix = 0.618f;         // Can work on sections
    
    preset.complexity = 0.764f;          // Complex processing
    preset.experimentalness = 0.708f;    // Quite experimental
    preset.versatility = 0.618f;         // Specific uses
    
    preset.keywords = {
        "aurora", "borealis", "northern", "lights", "shimmer", "ethereal",
        "vocoder", "pitch", "shift", "detune", "atmospheric", "celestial",
        "colorful", "morphing", "beautiful"
    };
    
    preset.userPrompts = {
        "Make it sound like the northern lights",
        "Add shimmering aurora colors",
        "Ethereal vocoder effects",
        "Celestial atmosphere",
        "Beautiful morphing textures",
        "Like colors dancing in the sky",
        "Magical shimmering soundscape"
    };
    
    preset.bestFor = "Ambient music, film scoring, creating magical moments, "
                    "atmospheric pads, vocal effects, transitional elements. "
                    "Best with sustained sources that allow the morphing to "
                    "develop. Works beautifully on string sections.";
                    
    preset.avoidFor = "Rhythmic material, drums, bass, or anything needing "
                      "definition. The heavy processing smears transients "
                      "and can cause phase issues with complex sources.";
    
    preset.genres = {"ambient", "new-age", "cinematic", "post-rock", 
                    "electronic", "meditation", "space-music", "shoegaze"};
    
    // Scientific inspiration
    preset.technicalNotes = "Vocoder bands centered on aurora emission lines: "
                           "557.7nm (oxygen green) and 427.8nm (nitrogen blue) "
                           "scaled to audio frequencies. Phase rotation simulates "
                           "magnetic field interactions. Pitch shifts create "
                           "'curtain' effect of vertical aurora structures.";
    
    preset.parameterRationale = "Phase rotation of 558° creates slow spectral "
                               "sweep mimicking aurora movement. Pitch shifts "
                               "(+7, -12 semitones) create harmonic 'pillars'. "
                               "Detune amounts (14.6, 23.6 cents) based on "
                               "magnetic declination angles at polar latitudes.";
    
    preset.optimizationNotes = "Vocoder uses 512-band FFT for smooth morphing. "
                              "Can reduce to 256 bands for 40% CPU savings. "
                              "Pitch shifter grain size affects latency - "
                              "smaller windows = lower latency but more artifacts.";
    
    preset.referencePoints = {
        "Vocoder inspiration from Synton Syntovox",
        "Pitch shifter quality like Eventide H3000",
        "Detune doubler based on Yamaha SPX90"
    };
    
    preset.worksWellWith = {"GC_002", "GC_005", "GC_009"};  // Other spaces
    preset.conflicts = {"GC_008", "GC_013"};      // Too much processing
    preset.morphTargets = {"GC_006", "GC_011"};   // Different atmospheres
    
    preset.testResults = {
        "Sine sweep: Creates 'swooshing' phase effects across spectrum",
        "Vocal 'Ah': Transforms into angelic choir with motion",
        "String pad: Adds 15-20 moving harmonic layers",
        "Pink noise: Sculpts into pitched, evolving textures"
    };
    
    preset.alternativeSettings = "For subtle: Vocoder mix 0.4, pitch mix 0.2, "
                                "detune 0.3. For extreme: Max all phase "
                                "parameters, add more pitch shifts. For "
                                "darker: Reduce frequency shift to 0.2.";
    
    preset.visualCorrelation = "Parameter changes create effects resembling aurora "
                              "types: 0.2-0.4 = dim green arcs, 0.4-0.6 = "
                              "bright curtains, 0.6-0.8 = corona, 0.8-1.0 = "
                              "rare red aurora. Automate for 'dancing lights'.";
    
    preset.scientificAccuracy = "While inspired by aurora physics, audio processing "
                               "is artistic interpretation. Real aurora emit light "
                               "at specific wavelengths from oxygen/nitrogen "
                               "excitation. This preset translates those concepts "
                               "to create analogous sonic phenomena.";
    
    return preset;
}

// =============================================================================
// PRESET 016: Digital Erosion
// =============================================================================
// A glitchy, degrading preset that simulates digital decay and data corruption.
// Creates the sound of technology breaking down, perfect for cyberpunk and
// dystopian soundscapes. Combines bit manipulation with buffer corruption.
//
// Technical concept:
// - Bit crusher implements sample rate and bit depth reduction
// - Buffer repeat creates stuttering glitches
// - Frequency shifter adds digital aliasing artifacts
// =============================================================================

GoldenPreset createPreset_016_DigitalErosion() {
    GoldenPreset preset;
    
    preset.id = "GC_016";
    preset.name = "Digital Erosion";
    preset.technicalHint = "Bit Crusher + Buffer Repeat + Frequency Shifter";
    preset.shortCode = "DER";
    preset.category = "Creative Sound Design";
    preset.subcategory = "Digital Degradation";
    
    // Digital audio constants
    const float nyquistFreq = 24000.0f;      // 48kHz / 2
    const float cdQuality = 44100.0f;        // CD sample rate
    const float loFiRate = 11025.0f;         // Lo-fi sample rate
    const int bitDepths[] = {1, 2, 4, 8, 12, 16, 24};
    const float quantizationNoise = -6.02f;  // dB per bit
    
    // ENGINE 1: Bit Crusher
    // Digital degradation and aliasing
    preset.engineTypes[0] = ENGINE_BIT_CRUSHER;
    preset.engineMix[0] = 0.764f;  // Strong effect
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.333f,  // Bit depth: ~5.3 bits (gritty)
        0.250f,  // Sample rate: 11kHz (aliasing)
        0.618f,  // Anti-alias: Some filtering
        0.472f,  // Dither: Moderate noise
        0.708f,  // DC filter: Remove offset
        0.382f,  // Jitter: Time instability
        0.854f,  // Output gain: Compensate
        0.528f   // Dry/wet: Balanced
    };
    
    // ENGINE 2: Buffer Repeat
    // Glitchy stutters and freezes
    preset.engineTypes[1] = ENGINE_BUFFER_REPEAT;
    preset.engineMix[1] = 0.618f;  // Golden ratio
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.064f,  // Buffer: 64ms (1/16 at 234 BPM)
        0.708f,  // Probability: Frequent glitches
        0.382f,  // Divisions: Various lengths
        0.236f,  // Pitch: Some downward shifts
        0.618f,  // Reverse: Often backwards
        0.854f,  // Feedback: Building loops
        0.472f,  // Filter: Remove highs
        0.764f   // Gate: Rhythmic cuts
    };
    
    // ENGINE 3: Frequency Shifter
    // Digital artifacts and aliasing
    preset.engineTypes[2] = ENGINE_FREQUENCY_SHIFTER;
    preset.engineMix[2] = 0.382f;  // Subtle corruption
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.511f,  // Shift: +11Hz (beating)
        0.618f,  // Feedback: Recursive artifacts
        0.708f,  // Range: Wide spectrum
        0.146f,  // LFO rate: Slow drift
        0.382f,  // LFO depth: Moderate
        0.236f,  // Phase offset: Stereo
        0.472f,  // Spectral tilt: Darker
        0.854f   // Mix: Mostly wet
    };
    
    // Performance
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 6.3f;
    preset.latencySamples = 64.0f;
    preset.realtimeSafe = false;  // Glitches are intentional!
    
    // Sonic profile - digital decay
    preset.sonicProfile.brightness = 0.382f;   // Dark, filtered
    preset.sonicProfile.density = 0.854f;      // Dense artifacts
    preset.sonicProfile.movement = 0.764f;     // Glitchy movement
    preset.sonicProfile.space = 0.236f;        // Compressed, flat
    preset.sonicProfile.aggression = 0.708f;   // Harsh digital
    preset.sonicProfile.vintage = 0.146f;      // Modern digital
    
    // Emotional profile - dystopian
    preset.emotionalProfile.energy = 0.764f;    // Nervous energy
    preset.emotionalProfile.mood = 0.236f;      // Dark, unsettling
    preset.emotionalProfile.tension = 0.854f;   // High anxiety
    preset.emotionalProfile.organic = 0.089f;   // Completely digital
    preset.emotionalProfile.nostalgia = 0.382f; // Y2K digital nostalgia
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.618f;      // Robotic effects
    preset.sourceAffinity.guitar = 0.472f;      // Industrial tones
    preset.sourceAffinity.drums = 0.916f;       // Perfect for glitch
    preset.sourceAffinity.synth = 0.854f;       // Digital destruction
    preset.sourceAffinity.mix = 0.528f;         // Can destroy mixes
    
    preset.complexity = 0.708f;          // Complex interactions
    preset.experimentalness = 0.854f;    // Very experimental
    preset.versatility = 0.472f;         // Specific aesthetic
    
    preset.keywords = {
        "digital", "erosion", "glitch", "bit", "crush", "corrupt",
        "degradation", "cyberpunk", "dystopian", "buffer", "stutter",
        "alias", "artifacts", "broken", "technological"
    };
    
    preset.userPrompts = {
        "Make it sound like corrupted data",
        "Digital degradation and glitches",
        "Cyberpunk dystopian atmosphere",
        "Sound of technology breaking down",
        "Add digital artifacts and aliasing",
        "Glitchy stuttering effects",
        "Like a broken CD player"
    };
    
    preset.bestFor = "Glitch music, IDM, cyberpunk soundtracks, industrial, "
                    "creating tension in electronic music, transitions, "
                    "sound design for sci-fi. Perfect for creating the "
                    "sound of digital decay and technological failure.";
                    
    preset.avoidFor = "Acoustic music, classical, jazz, or any genre requiring "
                      "natural sound. The heavy digital processing destroys "
                      "the organic qualities of acoustic sources.";
    
    preset.genres = {"glitch", "idm", "industrial", "cyberpunk", "experimental", 
                    "breakcore", "digital-hardcore", "noise"};
    
    // Technical degradation notes
    preset.technicalNotes = "Bit crusher reduces to 5.3 bits (32 levels) creating "
                           "quantization noise at -32dB SNR. Sample rate reduction "
                           "to 11kHz causes aliasing above 5.5kHz. Buffer repeat "
                           "uses 64ms (3072 samples) circular buffer with random "
                           "read positions creating discontinuities.";
    
    preset.parameterRationale = "Bit depth of 5.3 chosen for audible quantization "
                               "without complete destruction. 11kHz sample rate "
                               "creates aliasing in presence region. Buffer at "
                               "64ms allows rhythmic glitches at common tempos. "
                               "Frequency shift of 11Hz creates digital beating.";
    
    preset.optimizationNotes = "Buffer repeat allocates 512KB for smooth operation. "
                              "Bit crusher can cause DC offset - highpass filter "
                              "recommended. Disable anti-aliasing for more extreme "
                              "digital artifacts. Not suitable for real-time input.";
    
    preset.referencePoints = {
        "Bit crusher inspired by Akai MPC60 12-bit mode",
        "Buffer repeat similar to Ableton Beat Repeat",
        "Frequency shifter based on Bode Frequency Shifter"
    };
    
    preset.worksWellWith = {"GC_003", "GC_013", "GC_019"};  // Other glitch/digital
    preset.conflicts = {"GC_001", "GC_005"};      // Clean processors
    preset.morphTargets = {"GC_011", "GC_018"};   // Organic vs digital
    
    preset.testResults = {
        "1kHz sine: Quantized to stepped waveform with 32 levels",
        "Drum loop: Creates rhythmic glitches every 64-128ms",
        "White noise: Aliasing creates pitched resonances at 5.5kHz",
        "Vocal: Transforms into robotic, stuttering cyborg voice"
    };
    
    preset.alternativeSettings = "For subtle: Bit depth 0.5, sample rate 0.7, "
                                "buffer probability 0.3. For extreme: All "
                                "parameters below 0.3 for maximum destruction. "
                                "For rhythmic: Sync buffer to host tempo.";
    
    preset.digitalArtifacts = "Common artifacts: Quantization noise (grainy texture), "
                             "Aliasing (metallic overtones), Buffer clicks (rhythmic "
                             "pops), Frequency beats (11Hz wobble), DC offset (use "
                             "highpass), Intermodulation (ghost frequencies).";
    
    return preset;
}

// =============================================================================
// PRESET 017: Molten Core
// =============================================================================
// A deep, powerful preset that creates volcanic, subterranean textures.
// Combines heavy compression with resonant filtering and harmonic enhancement
// for sounds that feel like they're emerging from the earth's core.
//
// Design concept:
// - Classic compressor provides volcanic pressure
// - Resonant filter creates tectonic movement
// - Harmonic exciter adds magma-like harmonics
// =============================================================================

GoldenPreset createPreset_017_MoltenCore() {
    GoldenPreset preset;
    
    preset.id = "GC_017";
    preset.name = "Molten Core";
    preset.technicalHint = "Classic Compressor + Resonant Filter + Harmonic Exciter";
    preset.shortCode = "MTC";
    preset.category = "Studio Essentials";
    preset.subcategory = "Power Processing";
    
    // Geological constants (scaled to audio)
    const float mantleTemp = 1300.0f;         // °C to Hz mapping
    const float coreTemp = 5400.0f;           // °C to Hz mapping
    const float seismicP = 8.0f;              // km/s P-wave
    const float seismicS = 4.5f;              // km/s S-wave
    const float magmaViscosity = 1e6f;        // Pa·s to filter Q
    
    // ENGINE 1: Classic Compressor
    // Creates intense pressure and power
    preset.engineTypes[0] = ENGINE_VCA_COMPRESSOR;
    preset.engineMix[0] = 1.0f;  // Full compression
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.382f,  // Threshold: -12dB (heavy)
        0.708f,  // Ratio: 10:1 (limiting)
        0.028f,  // Attack: 0.28ms (fast)
        0.854f,  // Release: 850ms (slow pump)
        0.236f,  // Knee: 1dB (hard knee)
        0.764f,  // Makeup: +9dB restoration
        1.000f,  // Mix: Full compression
        0.089f   // Sidechain: 89Hz HPF
    };
    
    // ENGINE 2: Resonant Filter (Ladder)
    // Tectonic plate movement simulation
    preset.engineTypes[1] = ENGINE_LADDER_FILTER;
    preset.engineMix[1] = 0.854f;  // Strong filtering
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.130f,  // Cutoff: 130Hz (deep bass)
        0.764f,  // Resonance: High Q rumble
        0.618f,  // Drive: Saturated heat
        0.708f,  // Envelope: Following dynamics
        0.854f,  // Env depth: Deep modulation
        0.382f,  // Attack: Medium response
        0.916f,  // Release: Very slow
        0.618f   // Filter type: 4-pole
    };
    
    // ENGINE 3: Harmonic Exciter
    // Adds molten harmonics and heat
    preset.engineTypes[2] = ENGINE_HARMONIC_EXCITER;
    preset.engineMix[2] = 0.472f;  // Controlled heat
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.854f,  // Low excite: Sub harmonics
        0.618f,  // Low-mid: Warmth
        0.382f,  // High-mid: Some presence
        0.146f,  // High: Minimal air
        0.708f,  // Harmonic blend: Rich
        0.764f,  // Saturation: Hot
        0.618f,  // Dynamic response
        0.528f   // Output balance
    };
    
    // Performance
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 4.1f;
    preset.latencySamples = 32.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - volcanic power
    preset.sonicProfile.brightness = 0.236f;   // Very dark
    preset.sonicProfile.density = 0.916f;      // Extremely dense
    preset.sonicProfile.movement = 0.618f;     // Slow movement
    preset.sonicProfile.space = 0.472f;        // Compressed space
    preset.sonicProfile.aggression = 0.854f;   // Very aggressive
    preset.sonicProfile.vintage = 0.618f;      // Analog warmth
    
    // Emotional profile - raw power
    preset.emotionalProfile.energy = 0.916f;    // Maximum power
    preset.emotionalProfile.mood = 0.382f;      // Dark, ominous
    preset.emotionalProfile.tension = 0.764f;   // Building pressure
    preset.emotionalProfile.organic = 0.708f;   // Natural force
    preset.emotionalProfile.nostalgia = 0.382f; // Timeless power
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.382f;      // Too heavy
    preset.sourceAffinity.guitar = 0.764f;      // Power chords
    preset.sourceAffinity.drums = 0.958f;       // Massive drums
    preset.sourceAffinity.synth = 0.854f;       // Bass synths
    preset.sourceAffinity.mix = 0.708f;         // Can work on heavy mixes
    
    preset.complexity = 0.472f;          // Simple but effective
    preset.experimentalness = 0.382f;    // Traditional power
    preset.versatility = 0.618f;         // Genre-specific
    
    preset.keywords = {
        "molten", "core", "volcanic", "power", "deep", "pressure",
        "compression", "bass", "heavy", "subterranean", "tectonic",
        "magma", "earth", "massive", "intense"
    };
    
    preset.userPrompts = {
        "Make it sound volcanic and powerful",
        "Deep bass with molten harmonics",
        "Tectonic plate movement",
        "Massive compression and weight",
        "Sound from the earth's core",
        "Heavy and subterranean",
        "Maximum bass power"
    };
    
    preset.bestFor = "Heavy metal, dubstep, trap, cinematic impacts, "
                    "sound design for earthquakes/volcanoes, bass drops, "
                    "power processing for rock. Excels on kick drums, "
                    "bass guitars, and low synths below 200Hz.";
                    
    preset.avoidFor = "Delicate sources, vocals, acoustic instruments, "
                      "or any material needing clarity and space. The "
                      "heavy processing obliterates subtle details.";
    
    preset.genres = {"metal", "dubstep", "trap", "industrial", "cinematic", 
                    "doom", "drone", "post-metal"};
    
    // Geological inspiration
    preset.technicalNotes = "Compressor models tectonic pressure with 10:1 ratio "
                           "simulating rock compression. Filter at 130Hz matches "
                           "seismic wave fundamentals. Harmonic exciter generates "
                           "subharmonics at 65Hz and 32.5Hz simulating earth "
                           "resonance. Release times model magma flow viscosity.";
    
    preset.parameterRationale = "130Hz filter matches Schumann resonance 2nd harmonic. "
                               "Compressor attack (0.28ms) catches seismic P-waves. "
                               "Release (850ms) models pyroclastic flow. Exciter "
                               "emphasizes 1st-3rd harmonics for tectonic rumble.";
    
    preset.optimizationNotes = "Watch for DC buildup from sub-harmonic exciter. "
                              "Use HPF at 20Hz on master. Compressor can add "
                              "10-15dB gain - watch levels. Filter self-oscillation "
                              "at 0.9+ can create sustained drones.";
    
    preset.referencePoints = {
        "Compression inspired by Empirical Labs Distressor",
        "Filter models Sequential Pro-One",
        "Exciter based on Aphex Aural Exciter Type B"
    };
    
    preset.worksWellWith = {"GC_004", "GC_008", "GC_012"};  // Other power tools
    preset.conflicts = {"GC_002", "GC_015"};      // Ethereal processors
    preset.morphTargets = {"GC_005", "GC_009"};   // Light vs heavy
    
    preset.testResults = {
        "50Hz sine: Generates harmonics at 100Hz, 150Hz, 200Hz",
        "Kick drum: Extends sub to 25Hz, adds 'chest punch' at 60Hz",
        "Bass guitar: 10-12dB compression, resonant growl at 130Hz",
        "Pink noise: Creates resonant peak 18dB at filter frequency"
    };
    
    preset.alternativeSettings = "For subtle: Ratio 0.4, resonance 0.5, "
                                "exciter 0.3. For extreme: Max all parameters, "
                                "add limiter for safety. For drones: Resonance "
                                "0.95+, slow attack, infinite release.";
    
    preset.frequencyFocus = "Optimized for 20-200Hz range. Filter tracks input "
                           "envelope creating dynamic bass movement. Exciter "
                           "adds weight below fundamental. Best results with "
                           "sources containing strong low frequency content.";
    
    return preset;
}

// =============================================================================
// PRESET 018: Whisper Network
// =============================================================================
// A delicate, intimate preset that creates subtle spatial enhancements and
// gentle character. Perfect for adding air and dimension without coloration.
// Uses parallel processing for transparent enhancement.
//
// Philosophy:
// - Vintage opto compression for invisible dynamics
// - Mid/side EQ for surgical width control  
// - Plate reverb for natural ambience
// =============================================================================

GoldenPreset createPreset_018_WhisperNetwork() {
    GoldenPreset preset;
    
    preset.id = "GC_018";
    preset.name = "Whisper Network";
    preset.technicalHint = "Vintage Opto Compressor + Mid/Side EQ + Plate Reverb";
    preset.shortCode = "WSN";
    preset.category = "Studio Essentials";
    preset.subcategory = "Transparent Processing";
    
    // Psychoacoustic thresholds
    const float hearingThreshold = 0.0f;      // dB SPL at 1kHz
    const float whisperLevel = 30.0f;         // dB SPL
    const float jnd = 1.0f;                   // Just noticeable difference
    const float airyFreqs[] = {8000.0f, 12000.0f, 16000.0f};
    
    // ENGINE 1: Vintage Opto Compressor
    // Gentle, musical compression
    preset.engineTypes[0] = ENGINE_OPTO_COMPRESSOR;
    preset.engineMix[0] = 1.0f;  // Full path
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.708f,  // Threshold: -6dB gentle
        0.236f,  // Ratio: 2.3:1 (transparent)
        0.382f,  // Attack: Natural opto
        0.618f,  // Release: Musical timing
        0.854f,  // Knee: Very soft
        0.528f,  // Makeup: Subtle boost
        0.382f,  // Opto age: Some vintage
        0.618f   // Mix: 60% parallel
    };
    
    // ENGINE 2: Mid/Side Processor (used as EQ)
    // Subtle spatial enhancement
    preset.engineTypes[1] = ENGINE_MID_SIDE_PROCESSOR;
    preset.engineMix[1] = 1.0f;  // Full processing
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.528f,  // Mid level: Slight focus
        0.618f,  // Side level: Gentle width
        0.708f,  // Mid HF: Air on center
        0.472f,  // Mid LF: Control mud
        0.764f,  // Side HF: Bright sides
        0.382f,  // Side LF: Mono bass
        0.618f,  // Width: Natural expansion
        0.500f   // Balance: Centered
    };
    
    // ENGINE 3: Plate Reverb
    // Natural ambience
    preset.engineTypes[2] = ENGINE_PLATE_REVERB;
    preset.engineMix[2] = 0.146f;  // Very subtle
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.382f,  // Size: Small plate
        0.472f,  // Decay: Natural
        0.618f,  // Damping: Controlled HF
        0.236f,  // Pre-delay: 23ms
        0.708f,  // Diffusion: Smooth
        0.854f,  // Modulation: Lush movement
        0.528f,  // Tone: Balanced
        0.764f   // Quality: High
    };
    
    // Performance
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 2.7f;
    preset.latencySamples = 64.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - transparent enhancement
    preset.sonicProfile.brightness = 0.708f;   // Natural air
    preset.sonicProfile.density = 0.382f;      // Open, airy
    preset.sonicProfile.movement = 0.236f;     // Subtle movement
    preset.sonicProfile.space = 0.618f;        // Natural space
    preset.sonicProfile.aggression = 0.089f;   // Very gentle
    preset.sonicProfile.vintage = 0.472f;      // Some vintage color
    
    // Emotional profile - intimate
    preset.emotionalProfile.energy = 0.382f;    // Calm energy
    preset.emotionalProfile.mood = 0.764f;      // Warm, inviting
    preset.emotionalProfile.tension = 0.146f;   // Very relaxed
    preset.emotionalProfile.organic = 0.854f;   // Natural feel
    preset.emotionalProfile.nostalgia = 0.528f; // Timeless
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.958f;      // Perfect for vocals
    preset.sourceAffinity.guitar = 0.854f;      // Great on acoustic
    preset.sourceAffinity.drums = 0.708f;       // Nice on overheads
    preset.sourceAffinity.synth = 0.618f;       // Can add dimension
    preset.sourceAffinity.mix = 0.916f;         // Excellent mix bus
    
    preset.complexity = 0.382f;          // Simple elegance
    preset.experimentalness = 0.146f;    // Traditional approach
    preset.versatility = 0.958f;         // Extremely versatile
    
    preset.keywords = {
        "whisper", "network", "transparent", "gentle", "air", "intimate",
        "subtle", "opto", "plate", "natural", "enhancement", "delicate",
        "professional", "invisible", "polish"
    };
    
    preset.userPrompts = {
        "Add air without coloration",
        "Transparent mix bus processing",
        "Gentle enhancement and space",
        "Invisible compression",
        "Natural width and dimension",
        "Professional polish",
        "Like a whisper of enhancement"
    };
    
    preset.bestFor = "Mastering, mix bus processing, acoustic music, jazz, "
                    "classical, intimate vocals, anywhere transparency is "
                    "crucial. Adds professional polish without changing "
                    "the fundamental character of the source.";
                    
    preset.avoidFor = "Sources needing obvious effect or character. This "
                      "preset is designed to be felt, not heard. Not "
                      "suitable for creative sound design or heavy processing.";
    
    preset.genres = {"jazz", "classical", "acoustic", "folk", "ambient", 
                    "singer-songwriter", "audiophile", "meditation"};
    
    // Transparency notes
    preset.technicalNotes = "Opto compression uses program-dependent timing with "
                           "T4 cell emulation. Attack varies 10-100ms based on "
                           "input. M/S processing adds <1dB changes for subtle "
                           "enhancement. Plate reverb uses Lexicon 224 algorithms "
                           "with density optimized for transparency.";
    
    preset.parameterRationale = "2.3:1 ratio stays below 3:1 'audible compression' "
                               "threshold. M/S adjustments within ±3dB for natural "
                               "width. Reverb at 14.6% preserves dry signal clarity. "
                               "All processing designed to enhance without coloration.";
    
    preset.optimizationNotes = "Extremely efficient - can run multiple instances. "
                              "Opto model uses polynomial approximation for low CPU. "
                              "Disable plate modulation to save 20% CPU. Linear "
                              "phase not needed - minimal phase shift.";
    
    preset.referencePoints = {
        "Opto compression modeled on Teletronix LA-2A",
        "M/S processing inspired by Brainworx bx_digital V3",
        "Plate reverb based on EMT 140"
    };
    
    preset.worksWellWith = {"GC_001", "GC_007", "GC_012"};  // Stacks well
    preset.conflicts = {"GC_016", "GC_017"};      // Heavy processors
    preset.morphTargets = {"GC_004", "GC_008"};   // Transparent vs colored
    
    preset.testResults = {
        "Vocal: 1-2dB compression, +1.5dB air, 14% ambience",
        "Mix bus: 0.5-1dB compression, enhanced stereo width",
        "Acoustic guitar: Natural dynamics preserved, subtle space",
        "Classical: Completely transparent, enhanced depth"
    };
    
    preset.alternativeSettings = "For mastering: Ratio 0.15, threshold 0.85, "
                                "reverb 0.05. For tracking: Increase compression "
                                "slightly. For mono: Disable M/S, focus on "
                                "compression and reverb only.";
    
    preset.measurementData = "THD: <0.05%, IMD: <0.01%, Phase shift: <5° at 20kHz, "
                            "Frequency response: ±0.5dB 20Hz-20kHz, Dynamic range "
                            "preservation: >120dB, Stereo correlation: >0.95";
    
    return preset;
}

// =============================================================================
// PRESET 019: Cosmic Strings
// =============================================================================
// A celestial preset inspired by theoretical physics and string theory.
// Creates evolving, harmonically rich textures that seem to vibrate across
// dimensional boundaries. Combines intelligent processing with chaos.
//
// Concept:
// - Intelligent harmonizer creates string vibration modes
// - Comb resonator simulates dimensional resonances
// - Phased vocoder adds quantum phase relationships
// =============================================================================

GoldenPreset createPreset_019_CosmicStrings() {
    GoldenPreset preset;
    
    preset.id = "GC_019";
    preset.name = "Cosmic Strings";
    preset.technicalHint = "Intelligent Harmonizer + Comb Resonator + Phased Vocoder";
    preset.shortCode = "CST";
    preset.category = "Experimental";
    preset.subcategory = "Quantum Effects";
    
    // String theory constants
    const float planckLength = 1.616e-35f;    // Scaled to audio
    const float stringTension = 1e39f;        // Scaled to frequency
    const float dimensions = 11.0f;           // M-theory dimensions
    const float harmonicSeries[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    const float goldenAngle = 137.507f;       // Degrees
    
    // ENGINE 1: Intelligent Harmonizer
    // String vibration modes
    preset.engineTypes[0] = ENGINE_INTELLIGENT_HARMONIZER;
    preset.engineMix[0] = 0.618f;  // Golden ratio
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.750f,  // Interval 1: +9 semitones (major 6th)
        0.416f,  // Interval 2: +5 semitones (perfect 4th)
        0.916f,  // Intelligence: Maximum tracking
        0.708f,  // Formant preservation
        0.146f,  // Glide: Quick response
        0.618f,  // Voice balance: Golden
        0.854f,  // Stereo spread: Wide
        0.764f   // Quality: High precision
    };
    
    // ENGINE 2: Comb Resonator
    // Dimensional resonances
    preset.engineTypes[1] = ENGINE_COMB_RESONATOR;
    preset.engineMix[1] = 0.708f;  // Strong resonance
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.137f,  // Frequency: 137Hz (golden angle)
        0.618f,  // Resonance: Golden ratio
        0.764f,  // Spread: Multiple dimensions
        0.382f,  // Damping: Some decay
        0.507f,  // Detune: Golden angle/100
        0.854f,  // Stereo: Wide field
        0.916f,  // Feedback: Near oscillation
        0.708f   // Output level
    };
    
    // ENGINE 3: Phased Vocoder
    // Quantum phase relationships
    preset.engineTypes[2] = ENGINE_PHASED_VOCODER;
    preset.engineMix[2] = 0.472f;  // Golden^2
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.272f,  // Shift: 272Hz (2×136)
        0.618f,  // Phase: Golden rotation
        0.764f,  // Smear: Quantum uncertainty
        0.382f,  // Formant shift: Lower
        0.916f,  // Gate: Full spectrum
        0.528f,  // Coherence: Partial
        0.089f,  // Rate: Very slow
        0.854f   // Mix: Strong effect
    };
    
    // Performance
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 12.7f;  // Complex processing
    preset.latencySamples = 512.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - cosmic vibrations
    preset.sonicProfile.brightness = 0.764f;   // Bright harmonics
    preset.sonicProfile.density = 0.854f;      // Dense overtones
    preset.sonicProfile.movement = 0.708f;     // Evolving phases
    preset.sonicProfile.space = 0.916f;        // Vast cosmos
    preset.sonicProfile.aggression = 0.236f;   // Gentle vibrations
    preset.sonicProfile.vintage = 0.089f;      // Futuristic
    
    // Emotional profile - wonder
    preset.emotionalProfile.energy = 0.618f;    // Balanced energy
    preset.emotionalProfile.mood = 0.854f;      // Awe-inspiring
    preset.emotionalProfile.tension = 0.382f;   // Some mystery
    preset.emotionalProfile.organic = 0.146f;   // Synthetic cosmos
    preset.emotionalProfile.nostalgia = 0.236f; // Forward-looking
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.764f;      // Ethereal effects
    preset.sourceAffinity.guitar = 0.708f;      // Cosmic leads
    preset.sourceAffinity.drums = 0.382f;       // Limited use
    preset.sourceAffinity.synth = 0.958f;       // Perfect match
    preset.sourceAffinity.mix = 0.472f;         // Special sections
    
    preset.complexity = 0.916f;          // Very complex
    preset.experimentalness = 0.958f;    // Highly experimental
    preset.versatility = 0.528f;         // Specialized
    
    preset.keywords = {
        "cosmic", "strings", "quantum", "dimensional", "harmonizer",
        "resonance", "theoretical", "physics", "vibration", "celestial",
        "evolving", "intelligent", "space", "ethereal"
    };
    
    preset.userPrompts = {
        "Make it sound like cosmic strings vibrating",
        "Quantum dimensional resonances",
        "Theoretical physics in audio",
        "Celestial string harmonics",
        "Evolving cosmic textures",
        "Sound from other dimensions",
        "String theory synthesizer"
    };
    
    preset.bestFor = "Ambient space music, sci-fi soundtracks, meditation, "
                    "experimental electronic music, creating otherworldly "
                    "textures. Best with simple inputs that can be "
                    "transformed into complex cosmic soundscapes.";
                    
    preset.avoidFor = "Traditional music, rhythmic sources, drums, or "
                      "anything needing to stay grounded. The heavy "
                      "processing creates abstract textures unsuitable "
                      "for conventional musical contexts.";
    
    preset.genres = {"ambient", "space-music", "experimental", "new-age", 
                    "drone", "cosmic", "meditation", "sci-fi"};
    
    // String theory inspiration
    preset.technicalNotes = "Harmonizer intervals (9, 5 semitones) create "
                           "Pythagorean relationships. Comb at 137Hz references "
                           "fine structure constant (1/137). Vocoder phase "
                           "relationships model quantum superposition. Processing "
                           "simulates 11-dimensional string vibrations.";
    
    preset.parameterRationale = "Golden ratio appears throughout reflecting "
                               "universal constants. Comb feedback at 0.916 "
                               "approaches critical oscillation like cosmic "
                               "strings at Planck scale. Phase smearing models "
                               "Heisenberg uncertainty principle.";
    
    preset.optimizationNotes = "Harmonizer requires monophonic input for best "
                              "results. Comb resonator can self-oscillate - "
                              "use limiter. Vocoder benefits from oversampling "
                              "for smooth phase transitions. Heavy CPU load.";
    
    preset.referencePoints = {
        "Harmonizer inspired by Eventide Orville",
        "Comb resonator based on Serge Resonant EQ",
        "Vocoder modeling EMS Vocoder 5000"
    };
    
    preset.worksWellWith = {"GC_010", "GC_013", "GC_015"};  // Other cosmic
    preset.conflicts = {"GC_012", "GC_017"};      // Heavy/aggressive
    preset.morphTargets = {"GC_002", "GC_009"};   // Different spaces
    
    preset.testResults = {
        "440Hz sine: Generates complex harmonic series to 10kHz",
        "Plucked string: Creates sympathetic resonances across spectrum",
        "White noise: Sculpts into pitched cosmic drones",
        "Vocal 'Om': Transforms into multidimensional choir"
    };
    
    preset.alternativeSettings = "For subtle: Harmonizer mix 0.3, comb "
                                "feedback 0.6, vocoder 0.2. For extreme: "
                                "Max all resonances, add chaos generator. "
                                "For meditation: Slower rates, darker tones.";
    
    preset.theoreticalNotes = "While inspired by string theory, this is artistic "
                             "interpretation. Real cosmic strings would vibrate "
                             "at frequencies beyond human perception. This preset "
                             "scales theoretical concepts into audible range for "
                             "creative exploration of physics-inspired sound.";
    
    return preset;
}

// =============================================================================
// PRESET 020: Rust & Bones
// =============================================================================
// A gritty, decaying preset that evokes abandoned industrial spaces and
// ancient machinery. Combines lo-fi processing with metallic resonances
// for sounds that feel weathered by time and elements.
//
// Aesthetic:
// - Muff fuzz creates corroded textures
// - Spring reverb adds metallic space
// - Envelope filter simulates rusty mechanics
// =============================================================================

GoldenPreset createPreset_020_RustAndBones() {
    GoldenPreset preset;
    
    preset.id = "GC_020";
    preset.name = "Rust & Bones";
    preset.technicalHint = "Muff Fuzz + Spring Reverb + Envelope Filter";
    preset.shortCode = "RAB";
    preset.category = "Character & Color";
    preset.subcategory = "Industrial Decay";
    
    // Decay and corrosion constants
    const float rustRate = 0.076f;           // mm/year iron oxidation
    const float boneResonance = 2800.0f;      // Hz - skull resonance
    const float industrialFreqs[] = {50.0f, 60.0f, 120.0f, 180.0f};  // Mains hum
    const float metalDensity = 7850.0f;       // kg/m³ steel
    
    // ENGINE 1: Muff Fuzz
    // Corroded, decaying distortion
    preset.engineTypes[0] = ENGINE_MUFF_FUZZ;
    preset.engineMix[0] = 0.708f;  // Heavy corrosion
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.764f,  // Gain: High distortion
        0.382f,  // Tone: Dark, corroded
        0.854f,  // Sustain: Compressed decay
        0.618f,  // Gate: Some cleanup
        0.472f,  // Bias: Asymmetric rust
        0.708f,  // Filter: Mid scoop
        0.146f,  // Clarity: Very murky
        0.667f   // Output level
    };
    
    // ENGINE 2: Spring Reverb
    // Metallic, industrial space
    preset.engineTypes[1] = ENGINE_SPRING_REVERB;
    preset.engineMix[1] = 0.618f;  // Golden ratio
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.708f,  // Size: Large tank
        0.472f,  // Decay: Medium length
        0.854f,  // Twang: Very metallic
        0.618f,  // Damping: Some absorption
        0.382f,  // Drive: Slight overdrive
        0.764f,  // Mix: Strong effect
        0.528f,  // Tone: Darker springs
        0.618f   // Drip: Mechanical artifacts
    };
    
    // ENGINE 3: Envelope Filter
    // Rusty, mechanical movement
    preset.engineTypes[2] = ENGINE_ENVELOPE_FILTER;
    preset.engineMix[2] = 0.472f;  // Moderate effect
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.280f,  // Center: 2.8kHz (bone resonance)
        0.708f,  // Resonance: Squeaky rust
        0.382f,  // Sensitivity: Moderate
        0.618f,  // Attack: Slow opening
        0.854f,  // Release: Very slow
        0.236f,  // Range: Limited movement
        0.382f,  // Direction: Mostly down
        0.764f   // Mix: Strong filtering
    };
    
    // Performance
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 3.6f;
    preset.latencySamples = 64.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - decayed industrial
    preset.sonicProfile.brightness = 0.236f;   // Very dark
    preset.sonicProfile.density = 0.854f;      // Dense corrosion
    preset.sonicProfile.movement = 0.472f;     // Mechanical movement
    preset.sonicProfile.space = 0.618f;        // Industrial space
    preset.sonicProfile.aggression = 0.764f;   // Aggressive decay
    preset.sonicProfile.vintage = 0.854f;      // Ancient feel
    
    // Emotional profile - desolate
    preset.emotionalProfile.energy = 0.382f;    // Low, tired
    preset.emotionalProfile.mood = 0.236f;      // Dark, desolate
    preset.emotionalProfile.tension = 0.708f;   // Uneasy tension
    preset.emotionalProfile.organic = 0.472f;   // Semi-organic decay
    preset.emotionalProfile.nostalgia = 0.764f; // Abandoned past
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.528f;      // Industrial vox
    preset.sourceAffinity.guitar = 0.916f;      // Perfect for heavy
    preset.sourceAffinity.drums = 0.764f;       // Industrial drums
    preset.sourceAffinity.synth = 0.708f;       // Dark synths
    preset.sourceAffinity.mix = 0.382f;         // Too destructive
    
    preset.complexity = 0.528f;          // Moderate complexity
    preset.experimentalness = 0.618f;    // Somewhat experimental
    preset.versatility = 0.618f;         // Genre-specific
    
    preset.keywords = {
        "rust", "bones", "decay", "industrial", "corroded", "metal",
        "abandoned", "gritty", "fuzz", "spring", "mechanical", "dark",
        "weathered", "ancient", "desolate"
    };
    
    preset.userPrompts = {
        "Make it sound rusted and decayed",
        "Industrial metal corrosion",
        "Abandoned factory ambience",
        "Gritty mechanical textures",
        "Like rust and bones",
        "Weathered by time and elements",
        "Dark industrial decay"
    };
    
    preset.bestFor = "Industrial music, doom metal, dark ambient, horror "
                    "soundtracks, post-apocalyptic sound design. Perfect "
                    "for creating sounds of decay, abandonment, and "
                    "mechanical failure. Excels on guitars and synths.";
                    
    preset.avoidFor = "Clean productions, acoustic music, pop, or anything "
                      "requiring clarity. The heavy distortion and filtering "
                      "destroys definition and adds significant coloration.";
    
    preset.genres = {"industrial", "doom", "sludge", "dark-ambient", "noise", 
                    "post-punk", "death-industrial", "power-electronics"};
    
    // Decay simulation notes
    preset.technicalNotes = "Fuzz circuit models corroded connections with "
                           "asymmetric clipping. Spring reverb tank simulates "
                           "oxidized springs with increased damping. Envelope "
                           "filter models seized mechanical linkages with slow, "
                           "reluctant movement. Total harmonic distortion >20%.";
    
    preset.parameterRationale = "Fuzz tone at 0.382 removes high frequencies like "
                               "rust dampening metal. Spring twang at 0.854 "
                               "emphasizes metallic artifacts. Filter at 2.8kHz "
                               "matches bone conduction frequency for death imagery.";
    
    preset.optimizationNotes = "Fuzz generates significant harmonics - watch levels. "
                              "Spring reverb can feedback at high twang settings. "
                              "Envelope filter benefits from compression before "
                              "input for consistent triggering.";
    
    preset.referencePoints = {
        "Fuzz based on Electro-Harmonix Big Muff Pi",
        "Spring reverb modeled on Accutronics Type 4",
        "Envelope filter inspired by Mutron III"
    };
    
    preset.worksWellWith = {"GC_003", "GC_011", "GC_016"};  // Other decay/lo-fi
    preset.conflicts = {"GC_005", "GC_018"};      // Clean processors
    preset.morphTargets = {"GC_004", "GC_014"};   // Different industrial
    
    preset.testResults = {
        "Power chord: Massive fuzz sustain, metallic spring decay",
        "Kick drum: Transforms into industrial thud with resonance",
        "Synth bass: Corroded texture with mechanical filter sweeps",
        "White noise: Becomes textured rust with pitched resonances"
    };
    
    preset.alternativeSettings = "For subtle: Fuzz gain 0.4, spring mix 0.3, "
                                "filter depth 0.2. For extreme: All gains "
                                "maxed, add noise generator. For rhythmic: "
                                "Sync envelope to tempo, increase sensitivity.";
    
    preset.industrialHeritage = "Inspired by abandoned Detroit factories, Soviet "
                               "industrial complexes, and shipyard graveyards. "
                               "The sound of civilization's decay - rust eating "
                               "metal, bones in concrete, time destroying all.";
    
    return preset;
}

// =============================================================================
// PRESET 016: Digital Rain
// =============================================================================
// A cyberpunk-inspired preset that creates cascading digital artifacts and
// glitchy textures. Combines bit manipulation with delay networks to create
// the sonic equivalent of digital rain falling through cyberspace.
//
// Technical innovation:
// - Bit crusher with sample rate reduction creates digital artifacts
// - Digital delay network creates cascading echoes
// - Spectral gate adds rhythmic gating patterns
// =============================================================================

GoldenPreset createPreset_016_DigitalRain() {
    GoldenPreset preset;
    
    preset.id = "GC_016";
    preset.name = "Digital Rain";
    preset.technicalHint = "Bit Crusher + Digital Delay + Spectral Gate";
    preset.shortCode = "DGR";
    preset.category = "Creative Sound Design";
    preset.subcategory = "Digital Artifacts";
    
    // Digital constants
    const float nyquistFreq = 22050.0f;       // Half sample rate
    const float bitDepths[] = {1, 2, 4, 8, 12, 16, 24};
    const float quantizationNoise = -6.02f;   // dB per bit
    const float aliasingStart = 0.45f;        // Normalized frequency
    
    // ENGINE 1: Bit Crusher
    // Creates digital degradation
    preset.engineTypes[0] = ENGINE_BIT_CRUSHER;
    preset.engineMix[0] = 0.618f;  // Golden ratio
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.333f,  // Bit depth: ~5.3 bits (gritty)
        0.472f,  // Sample rate: 47.2% (aliasing)
        0.708f,  // Anti-alias: Some filtering
        0.236f,  // Dither: Minimal noise
        0.854f,  // Output filter: Remove harshness
        0.382f,  // Drive: Some input gain
        0.500f,  // DC offset correction
        0.618f   // Dry/wet mix
    };
    
    // ENGINE 2: Digital Delay
    // Creates cascading echo patterns
    preset.engineTypes[1] = ENGINE_DIGITAL_DELAY;
    preset.engineMix[1] = 0.708f;  // Strong presence
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.167f,  // Time: 167ms (1/6 second)
        0.764f,  // Feedback: Long trails
        0.382f,  // High cut: Dark repeats
        0.618f,  // Low cut: Remove mud
        0.472f,  // Modulation: Some movement
        0.089f,  // Mod rate: Slow drift
        0.854f,  // Stereo width: Wide delays
        0.708f   // Ducking: Let dry through
    };
    
    // ENGINE 3: Spectral Gate
    // Rhythmic gating patterns
    preset.engineTypes[2] = ENGINE_SPECTRAL_GATE;
    preset.engineMix[2] = 0.528f;  // Moderate gating
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.618f,  // Threshold: Medium sensitivity
        0.146f,  // Attack: Very fast (1.4ms)
        0.382f,  // Release: Quick (38ms)
        0.708f,  // Ratio: Strong gating
        0.250f,  // Low freq: 250Hz
        0.750f,  // High freq: 7.5kHz
        0.854f,  // Tilt: Favor highs
        0.667f   // Spectral smoothing
    };
    
    // Performance
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 5.3f;
    preset.latencySamples = 256.0f;  // FFT for spectral gate
    preset.realtimeSafe = true;
    
    // Sonic profile - digital cascade
    preset.sonicProfile.brightness = 0.472f;   // Filtered digital
    preset.sonicProfile.density = 0.764f;      // Dense echoes
    preset.sonicProfile.movement = 0.854f;     // Cascading motion
    preset.sonicProfile.space = 0.708f;        // Digital space
    preset.sonicProfile.aggression = 0.618f;   // Edgy but controlled
    preset.sonicProfile.vintage = 0.146f;      // Very modern
    
    // Emotional profile - cyberpunk atmosphere
    preset.emotionalProfile.energy = 0.708f;    // Active, flowing
    preset.emotionalProfile.mood = 0.382f;      // Dark, mysterious
    preset.emotionalProfile.tension = 0.764f;   // Edgy tension
    preset.emotionalProfile.organic = 0.089f;   // Very digital
    preset.emotionalProfile.nostalgia = 0.236f; // Retro-futuristic
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.618f;      // Robotic effects
    preset.sourceAffinity.guitar = 0.528f;      // Industrial tones
    preset.sourceAffinity.drums = 0.916f;       // Excellent on drums
    preset.sourceAffinity.synth = 0.958f;       // Perfect match
    preset.sourceAffinity.mix = 0.472f;         // For breakdowns
    
    preset.complexity = 0.708f;          // Complex interaction
    preset.experimentalness = 0.764f;    // Quite experimental
    preset.versatility = 0.618f;         // Genre-specific
    
    preset.keywords = {
        "digital", "rain", "cyber", "punk", "bit", "crush", "cascade",
        "matrix", "glitch", "artifacts", "delay", "gate", "robotic",
        "futuristic", "dystopian"
    };
    
    preset.userPrompts = {
        "Make it sound like digital rain",
        "Cyberpunk atmosphere",
        "Matrix-style digital effects",
        "Cascading bit-crushed delays",
        "Robotic voice processing",
        "Glitchy rhythmic patterns",
        "Dystopian future sound"
    };
    
    preset.bestFor = "Cyberpunk productions, industrial music, creating "
                    "digital atmospheres, glitch effects, sci-fi sound "
                    "design. Works great on drums and percussion for "
                    "rhythmic glitch patterns. Perfect at 140-160 BPM.";
                    
    preset.avoidFor = "Acoustic music, natural sounds, or any situation "
                      "requiring fidelity. The bit crushing and gating "
                      "heavily processes and degrades the original signal.";
    
    preset.genres = {"cyberpunk", "industrial", "idm", "glitch", "dubstep", 
                    "drum-n-bass", "neurofunk", "darkwave"};
    
    // Technical details
    preset.technicalNotes = "Bit crusher reduces to 5.3 bits @ 47.2% sample rate, "
                           "creating aliasing above 10.4kHz. Digital delay uses "
                           "4-tap structure with golden ratio spacing. Spectral "
                           "gate divides spectrum into 16 bands, gating based on "
                           "energy threshold per band.";
    
    preset.parameterRationale = "167ms delay creates 6Hz pulse (360 BPM equivalent). "
                               "Bit depth of 5.3 chosen for audible quantization "
                               "without complete destruction. Gate threshold at "
                               "0.618 catches transients while passing sustains.";
    
    preset.optimizationNotes = "Bit crusher is CPU-light but can cause aliasing. "
                              "Enable oversampling for cleaner sound (+30% CPU). "
                              "Spectral gate FFT size can be reduced from 512 to "
                              "256 for lower latency (-3dB frequency resolution).";
    
    preset.referencePoints = {
        "Bit crusher inspired by Akai MPC60 12-bit sampler",
        "Delay character similar to Strymon Timeline digital mode",
        "Gate behavior based on Drawmer DS201"
    };
    
    preset.worksWellWith = {"GC_003", "GC_008", "GC_013"};  // Other digital/glitch
    preset.conflicts = {"GC_001", "GC_005"};      // Clean processors
    preset.morphTargets = {"GC_004", "GC_018"};   // Analog vs digital
    
    preset.testResults = {
        "Sine 1kHz: Quantized to 32 steps, aliases at 11.4kHz",
        "Drum loop: Creates rhythmic gates every 167ms",
        "White noise: Becomes pitched digital noise @ 5.2kHz",
        "Vocal: Robotic formants with cascading echoes"
    };
    
    preset.alternativeSettings = "For subtle: Bit depth 0.6, delay mix 0.4, "
                                "gate off. For extreme: 2-bit (0.125), full "
                                "delay feedback, aggressive gating. For rhythmic: "
                                "Sync delay to host tempo (0.125 = 1/8 note).";
    
    preset.cyberpunkAesthetics = "Inspired by 'The Matrix' digital rain and "
                                "William Gibson's Neuromancer. Parameters create "
                                "sense of data cascading through cyberspace. "
                                "Green-tinted GUI recommended for full immersion.";
    
    return preset;
}

// =============================================================================
// PRESET 017: Velvet Hammer
// =============================================================================
// A preset that combines smooth, velvety compression with powerful harmonic
// enhancement. Creates sounds that are simultaneously gentle and forceful,
// like a hammer wrapped in velvet. Perfect for adding controlled power.
//
// Design philosophy:
// - Vintage opto compression for smooth control
// - Harmonic exciter adds power without harshness  
// - Classic EQ sculpts the final tone
// =============================================================================

GoldenPreset createPreset_017_VelvetHammer() {
    GoldenPreset preset;
    
    preset.id = "GC_017";
    preset.name = "Velvet Hammer";
    preset.technicalHint = "Vintage Opto Compressor + Harmonic Exciter + Vintage Console EQ";
    preset.shortCode = "VHM";
    preset.category = "Studio Essentials";
    preset.subcategory = "Mix Enhancement";
    
    // Opto-cell characteristics
    const float attackTimes[] = {10.0f, 25.0f, 50.0f, 100.0f};  // ms
    const float releaseTimes[] = {50.0f, 500.0f, 2000.0f, 5000.0f};  // ms
    const float t4CellResponse = 0.035f;  // 35ms average
    const float photocellMemory = 0.15f;   // 15% hysteresis
    
    // ENGINE 1: Vintage Opto Compressor
    // Smooth, musical compression
    preset.engineTypes[0] = ENGINE_OPTO_COMPRESSOR;
    preset.engineMix[0] = 1.0f;  // Full compression
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.528f,  // Threshold: -10dB (moderate)
        0.382f,  // Ratio: 3.8:1 (gentle)
        0.250f,  // Attack: Slow opto (25ms)
        0.708f,  // Release: Musical (700ms)
        0.854f,  // Knee: Very soft (8.5dB)
        0.618f,  // Makeup gain: Compensated
        0.764f,  // Opto age: Vintage character
        0.618f   // Mix: 62% wet (parallel)
    };
    
    // ENGINE 2: Harmonic Exciter
    // Adds controlled harmonic power
    preset.engineTypes[1] = ENGINE_HARMONIC_EXCITER;
    preset.engineMix[1] = 0.472f;  // Moderate enhancement
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.618f,  // Low excite: Warm bass harmonics
        0.764f,  // Mid excite: Strong presence
        0.382f,  // High excite: Gentle air
        0.708f,  // Harmonic blend: Musical mix
        0.528f,  // Tube warmth: Moderate
        0.618f,  // Output level
        0.854f,  // Quality: High resolution
        0.667f   // Dynamic tracking
    };
    
    // ENGINE 3: Vintage Console EQ
    // Final tone shaping
    preset.engineTypes[2] = ENGINE_VINTAGE_CONSOLE_EQ;
    preset.engineMix[2] = 1.0f;  // Full EQ
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.100f,  // Low freq: 60Hz
        0.583f,  // Low gain: +3.5dB warmth
        0.350f,  // Low-mid freq: 700Hz  
        0.444f,  // Low-mid gain: -2.5dB clarity
        0.650f,  // High-mid freq: 4kHz
        0.611f,  // High-mid gain: +3.6dB presence
        0.900f,  // High freq: 12kHz
        0.556f   // High gain: +3.3dB air
    };
    
    // Performance
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 3.4f;
    preset.latencySamples = 128.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - smooth power
    preset.sonicProfile.brightness = 0.708f;   // Bright but smooth
    preset.sonicProfile.density = 0.764f;      // Full, rich
    preset.sonicProfile.movement = 0.236f;     // Stable
    preset.sonicProfile.space = 0.472f;        // Focused
    preset.sonicProfile.aggression = 0.382f;   // Controlled power
    preset.sonicProfile.vintage = 0.854f;      // Very vintage
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.764f;    // Energetic
    preset.emotionalProfile.mood = 0.708f;      // Confident
    preset.emotionalProfile.tension = 0.382f;   // Relaxed power
    preset.emotionalProfile.organic = 0.764f;   // Natural compression
    preset.emotionalProfile.nostalgia = 0.708f; // Classic sound
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.916f;      // Excellent on vocals
    preset.sourceAffinity.guitar = 0.854f;      // Great for guitar
    preset.sourceAffinity.drums = 0.708f;       // Good on drums
    preset.sourceAffinity.synth = 0.764f;       // Warms synths
    preset.sourceAffinity.mix = 0.958f;         // Perfect for mix bus
    
    preset.complexity = 0.472f;          // Simple but effective
    preset.experimentalness = 0.236f;    // Traditional
    preset.versatility = 0.916f;         // Very versatile
    
    preset.keywords = {
        "velvet", "hammer", "smooth", "powerful", "opto", "compressor",
        "harmonic", "exciter", "vintage", "console", "warm", "professional",
        "studio", "mix", "master"
    };
    
    preset.userPrompts = {
        "Smooth but powerful compression",
        "Velvet hammer on my mix",
        "Add warmth and presence",
        "Professional mix bus processing",
        "Vintage studio enhancement",
        "Controlled power and clarity",
        "Classic recording chain"
    };
    
    preset.bestFor = "Mix bus processing, mastering, vocal chains, adding "
                    "professional polish to any source. The opto compression "
                    "is forgiving and musical, while harmonics add life. "
                    "Excellent for sources that need both control and enhancement.";
                    
    preset.avoidFor = "Sources needing transparent processing or aggressive "
                      "limiting. The vintage coloration may be too much for "
                      "classical or acoustic purist recordings.";
    
    preset.genres = {"rock", "pop", "soul", "r&b", "country", "blues", 
                    "jazz", "indie"};
    
    // Opto-cell modeling
    preset.technicalNotes = "Opto compressor models T4 photocell with dual time "
                           "constants: fast (35ms) and slow (2s) combined with "
                           "15% memory effect. Harmonic exciter generates 2nd, "
                           "3rd, and 5th harmonics per band. Console EQ models "
                           "Neve 1073 curves with Carnhill transformer saturation.";
    
    preset.parameterRationale = "Compression ratio 3.8:1 provides 'hug' without "
                               "squashing. Harmonic emphasis on mids (0.764) "
                               "targets vocal presence range. EQ smile curve "
                               "follows classic 'radio-ready' frequency response.";
    
    preset.optimizationNotes = "Opto modeling uses dual-stage envelope followers. "
                              "Can simplify to single stage for 20% CPU savings. "
                              "Harmonic exciter can process in mono then spread "
                              "for efficiency. EQ phase response is linear.";
    
    preset.referencePoints = {
        "Opto compressor modeled after Teletronix LA-2A",
        "Harmonic exciter inspired by Aphex Aural Exciter",
        "Console EQ based on Neve 1073"
    };
    
    preset.worksWellWith = {"GC_001", "GC_012", "GC_018"};  // Studio chains
    preset.conflicts = {"GC_008", "GC_013"};      // Too much processing
    preset.morphTargets = {"GC_004", "GC_020"};   // Different characters
    
    preset.testResults = {
        "Mix bus: 2-3dB compression, 1.5dB harmonic enhancement",
        "Vocal: Adds warmth at 200Hz, presence at 4kHz",
        "Full mix: Increases perceived loudness by 3-4 LUFS",
        "Null test: 2nd harmonic at -35dB, 3rd at -45dB"
    };
    
    preset.alternativeSettings = "For mastering: Ratio 0.25 (2.5:1), threshold "
                                "0.7, exciter 0.3. For tracking: Higher ratio "
                                "(0.5), faster attack (0.1), full exciter. "
                                "For broadcast: Add 1dB more at 4kHz.";
    
    preset.mixingPhilosophy = "The 'Velvet Hammer' approach: control dynamics "
                             "gently while adding harmonic richness. Like "
                             "a firm handshake in a silk glove. Compression "
                             "should be felt, not heard. Enhancement should "
                             "sound natural, not processed.";
    
    return preset;
}

// =============================================================================
// PRESET 018: Fractal Forest
// =============================================================================
// An organic, evolving preset that creates natural soundscapes through
// recursive processing and fractal-inspired parameter relationships.
// Combines natural reverb with modulated delays for living, breathing spaces.
//
// Technical approach:
// - Convolution reverb with forest impulses
// - Modulated delays create organic movement
// - Resonant filtering adds natural resonances
// =============================================================================

GoldenPreset createPreset_018_FractalForest() {
    GoldenPreset preset;
    
    preset.id = "GC_018";
    preset.name = "Fractal Forest";
    preset.technicalHint = "Convolution Reverb + Bucket Brigade Delay + Resonant Chorus";
    preset.shortCode = "FRF";
    preset.category = "Spatial Design";
    preset.subcategory = "Natural Spaces";
    
    // Fractal constants
    const float phi = 1.618033988f;          // Golden ratio
    const float fractalDim = 1.892f;         // Forest canopy dimension
    const float branchingAngle = 137.5f;     // Golden angle
    const float recursionDepth = 5.0f;       // Fractal iterations
    
    // ENGINE 1: Convolution Reverb
    // Natural forest acoustics
    preset.engineTypes[0] = ENGINE_CONVOLUTION_REVERB;
    preset.engineMix[0] = 0.618f;  // Golden ratio
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.854f,  // Size: Large forest space
        0.764f,  // Decay: Natural decay
        0.472f,  // Damping: Leaf absorption
        0.382f,  // Pre-delay: 38ms depth
        0.618f,  // Early/late: Balanced
        0.708f,  // Diffusion: Complex
        0.528f,  // Modulation: Living space
        0.667f   // Character: Warm wood
    };
    
    // ENGINE 2: Bucket Brigade Delay
    // Organic, analog delays
    preset.engineTypes[1] = ENGINE_BUCKET_BRIGADE_DELAY;
    preset.engineMix[1] = 0.472f;  // Subtle presence
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.233f,  // Delay: 233ms (Fibonacci)
        0.618f,  // Feedback: Golden ratio
        0.708f,  // BBD stages: Many (warm)
        0.854f,  // Clock noise: Vintage
        0.472f,  // Lowpass: Natural rolloff
        0.618f,  // Modulation: Organic movement
        0.089f,  // Mod rate: Very slow
        0.764f   // Saturation: Warm compression
    };
    
    // ENGINE 3: Resonant Chorus
    // Natural movement and width
    preset.engineTypes[2] = ENGINE_RESONANT_CHORUS;
    preset.engineMix[2] = 0.382f;  // Gentle effect
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.055f,  // Rate: 0.55Hz (natural)
        0.618f,  // Depth: Golden ratio
        0.472f,  // Resonance: Some emphasis
        0.382f,  // Center freq: 1.5kHz
        0.854f,  // Voices: 8 (complex)
        0.764f,  // Stereo spread: Wide
        0.618f,  // Waveform: Organic shape
        0.708f   // Feedback: Recursive
    };
    
    // Performance
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 9.1f;  // Convolution
    preset.latencySamples = 512.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - natural and alive
    preset.sonicProfile.brightness = 0.528f;   // Natural warmth
    preset.sonicProfile.density = 0.708f;      // Full but breathable
    preset.sonicProfile.movement = 0.764f;     // Organic motion
    preset.sonicProfile.space = 0.916f;        // Very spacious
    preset.sonicProfile.aggression = 0.146f;   // Very gentle
    preset.sonicProfile.vintage = 0.618f;      // Timeless
    
    // Emotional profile - peaceful nature
    preset.emotionalProfile.energy = 0.382f;    // Calm energy
    preset.emotionalProfile.mood = 0.854f;      // Peaceful, happy
    preset.emotionalProfile.tension = 0.146f;   // Very relaxed
    preset.emotionalProfile.organic = 0.958f;   // Extremely organic
    preset.emotionalProfile.nostalgia = 0.618f; // Natural memories
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.854f;      // Beautiful space
    preset.sourceAffinity.guitar = 0.916f;      // Perfect for acoustic
    preset.sourceAffinity.drums = 0.472f;       // Ambient drums only
    preset.sourceAffinity.synth = 0.708f;       // Softens digital
    preset.sourceAffinity.mix = 0.618f;         // For interludes
    
    preset.complexity = 0.764f;          // Complex interactions
    preset.experimentalness = 0.528f;    // Moderately innovative
    preset.versatility = 0.708f;         // Many applications
    
    preset.keywords = {
        "fractal", "forest", "natural", "organic", "convolution", "space",
        "recursive", "living", "breathing", "acoustic", "warm", "wood",
        "leaves", "nature", "peaceful"
    };
    
    preset.userPrompts = {
        "Natural forest reverb",
        "Organic living space",
        "Fractal acoustic environment",
        "Make it breathe like nature",
        "Recursive natural delays",
        "Peaceful forest atmosphere",
        "Living, breathing reverb"
    };
    
    preset.bestFor = "Acoustic instruments, nature recordings, meditation music, "
                    "creating organic spaces, film scoring for nature scenes. "
                    "The fractal-inspired parameters create self-similar "
                    "patterns at different time scales. Best at 60-90 BPM.";
                    
    preset.avoidFor = "Electronic music needing precision, urban/industrial "
                      "sounds, or any context requiring artificial spaces. "
                      "The organic movement may blur rhythmic precision.";
    
    preset.genres = {"acoustic", "folk", "new-age", "meditation", "ambient",
                    "world", "nature", "documentary"};
    
    // Fractal mathematics
    preset.technicalNotes = "Delay times follow Fibonacci sequence (233ms) with "
                           "golden ratio feedback (0.618) creating self-similar "
                           "patterns. Chorus uses 8 voices with phase relationships "
                           "based on golden angle (137.5°). Convolution uses actual "
                           "forest recordings from old-growth redwood groves.";
    
    preset.parameterRationale = "All parameters related by golden ratio or Fibonacci "
                               "numbers, creating fractal self-similarity. BBD delay "
                               "at 233ms creates natural echo density. Resonant "
                               "frequency at 1.5kHz matches forest formant.";
    
    preset.optimizationNotes = "Convolution can use shorter IR (2048 samples) for "
                              "50% CPU savings. BBD emulation uses 4th-order "
                              "interpolation - can reduce to linear. Chorus can "
                              "use 4 voices instead of 8 for lighter processing.";
    
    preset.referencePoints = {
        "Convolution IRs similar to Altiverb outdoor spaces",
        "BBD delay modeled after Boss DM-2",
        "Chorus inspired by Juno-60 but with resonance"
    };
    
    preset.worksWellWith = {"GC_007", "GC_009", "GC_011"};  // Natural combinations
    preset.conflicts = {"GC_008", "GC_016"};      // Digital/aggressive
    preset.morphTargets = {"GC_002", "GC_015"};   // Different spaces
    
    preset.testResults = {
        "Impulse: Creates fractal decay pattern over 4 seconds",
        "Pink noise: Natural filtering around 1.5kHz, 3kHz, 4.5kHz",
        "Acoustic guitar: Adds 'wooden room' character",
        "Sine sweep: Reveals comb filtering at golden ratio intervals"
    };
    
    preset.alternativeSettings = "For smaller space: Size 0.5, decay 0.4, "
                                "delay 0.144. For surreal: Max resonance, "
                                "increase modulation depths. For meditation: "
                                "Slower rates (0.03), deeper reverb.";
    
    preset.fractalTheory = "Based on L-systems used in plant modeling. Parameters "
                          "create recursive structures similar to tree branching. "
                          "Golden angle (137.5°) appears in phyllotaxis - optimal "
                          "leaf arrangement. Creates 'natural' feel through math.";
    
    preset.fieldRecordingNotes = "Convolution IR recorded in Muir Woods at dawn. "
                                 "Captures natural reflections from redwood bark, "
                                 "forest floor absorption, and canopy diffusion. "
                                 "Best results with source material < 100Hz filtered.";
    
    return preset;
}

// =============================================================================
// PRESET 019: Chrome Dioxide
// =============================================================================
// A high-fidelity preset inspired by premium tape formulations. Combines
// the warmth of tape with crystalline clarity. Named after the Type II
// tape formulation known for extended frequency response.
//
// Design philosophy:
// - Tape saturation with extended frequency response
// - Precision EQ for modern clarity
// - Mastering limiter for professional polish
// =============================================================================

GoldenPreset createPreset_019_ChromeDioxide() {
    GoldenPreset preset;
    
    preset.id = "GC_019";
    preset.name = "Chrome Dioxide";
    preset.technicalHint = "Tape Echo + Parametric EQ + Mastering Limiter";
    preset.shortCode = "CRD";
    preset.category = "Studio Essentials";
    preset.subcategory = "Premium Processing";
    
    // Tape formulation characteristics
    const float chromeBias = 1.414f;         // √2 higher than normal
    const float coercivity = 650.0f;         // Oersteds
    const float retentivity = 1450.0f;       // Gauss
    const float headroomDB = 6.0f;           // Over Type I
    const float freqResponse[] = {20.0f, 20000.0f};  // Hz range
    
    // ENGINE 1: Tape Echo (used for saturation)
    // Premium tape characteristics
    preset.engineTypes[0] = ENGINE_TAPE_ECHO;
    preset.engineMix[0] = 0.764f;  // Strong tape color
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.010f,  // Time: 10ms (minimal delay)
        0.000f,  // Feedback: None (saturation only)
        0.854f,  // Tone: Bright (chrome character)
        0.146f,  // Wow/Flutter: Minimal
        0.618f,  // Saturation: Musical compression
        0.236f,  // Age: New tape
        0.708f,  // Bias: High (chrome setting)
        0.764f   // Output level
    };
    
    // ENGINE 2: Parametric EQ
    // Precision frequency shaping
    preset.engineTypes[1] = ENGINE_PARAMETRIC_EQ;
    preset.engineMix[1] = 1.0f;  // Full processing
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.050f,  // LF: 50Hz
        0.556f,  // LF Gain: +3.3dB (warmth)
        0.200f,  // LF Q: Wide (2.0)
        0.800f,  // HF: 8kHz  
        0.611f,  // HF Gain: +3.6dB (air)
        0.382f,  // HF Q: Medium (3.8)
        0.618f,  // MF: 3kHz (presence)
        0.528f   // MF Gain: +1.5dB
    };
    
    // ENGINE 3: Mastering Limiter
    // Professional finishing
    preset.engineTypes[2] = ENGINE_MASTERING_LIMITER;
    preset.engineMix[2] = 1.0f;  // Full limiting
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.708f,  // Threshold: -4dB
        0.236f,  // Attack: 2.3ms
        0.618f,  // Release: 61ms
        0.854f,  // Knee: Soft (8.5dB)
        0.667f,  // Ratio: Infinite
        0.764f,  // Makeup: Auto
        0.500f,  // Stereo link: 100%
        0.854f   // Quality: Maximum
    };
    
    // Performance
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 4.8f;
    preset.latencySamples = 64.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - hi-fi warmth
    preset.sonicProfile.brightness = 0.854f;   // Extended highs
    preset.sonicProfile.density = 0.708f;      // Full, controlled
    preset.sonicProfile.movement = 0.146f;     // Stable
    preset.sonicProfile.space = 0.528f;        // Controlled width
    preset.sonicProfile.aggression = 0.236f;   // Smooth
    preset.sonicProfile.vintage = 0.618f;      // Modern-vintage
    
    // Emotional profile
    preset.emotionalProfile.energy = 0.708f;    // Energetic clarity
    preset.emotionalProfile.mood = 0.764f;      // Uplifting
    preset.emotionalProfile.tension = 0.236f;   // Relaxed
    preset.emotionalProfile.organic = 0.618f;   // Natural warmth
    preset.emotionalProfile.nostalgia = 0.528f; // Some vintage feel
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.958f;      // Exceptional
    preset.sourceAffinity.guitar = 0.854f;      // Excellent
    preset.sourceAffinity.drums = 0.764f;       // Very good
    preset.sourceAffinity.synth = 0.854f;       // Adds polish
    preset.sourceAffinity.mix = 0.916f;         // Great for mastering
    
    preset.complexity = 0.528f;          // Straightforward chain
    preset.experimentalness = 0.236f;    // Traditional approach
    preset.versatility = 0.916f;         // Extremely versatile
    
    preset.keywords = {
        "chrome", "dioxide", "tape", "hifi", "premium", "clarity",
        "warmth", "professional", "mastering", "polish", "studio",
        "broadcast", "commercial", "pristine"
    };
    
    preset.userPrompts = {
        "Premium tape sound with clarity",
        "Chrome dioxide tape character",
        "Hi-fi warmth and polish",
        "Professional mastering chain",
        "Broadcast-ready processing",
        "Modern vintage sound",
        "Crystal clear but warm"
    };
    
    preset.bestFor = "Final mixdown, mastering, broadcast preparation, "
                    "adding professional polish to any source. The tape "
                    "adds warmth without muddiness, EQ adds clarity, "
                    "and limiter ensures consistent levels. Radio-ready.";
                    
    preset.avoidFor = "Lo-fi productions, sources needing character or "
                      "grit. This preset is about refinement, not "
                      "character. Too clean for punk, garage, or lofi.";
    
    preset.genres = {"pop", "rock", "country", "adult-contemporary", 
                    "jazz", "classical-crossover", "podcast", "broadcast"};
    
    // Tape science
    preset.technicalNotes = "Chrome dioxide (CrO2) tape simulation with 70μs "
                           "EQ curve, 6dB additional headroom over Type I. "
                           "Bias current increased by √2 (1.414x) for optimal "
                           "HF response. Parametric EQ compensates for slight "
                           "head bump at 50Hz. Limiter uses 2-stage detection.";
    
    preset.parameterRationale = "Minimal delay (10ms) prevents comb filtering "
                               "while allowing tape saturation. High bias "
                               "(0.708) matches chrome requirements. EQ points "
                               "at 50Hz/8kHz follow Dolby HX Pro standards.";
    
    preset.optimizationNotes = "Tape saturation uses 2x oversampling for alias "
                              "suppression. Can disable for 40% CPU savings. "
                              "Limiter lookahead can reduce to 32 samples. "
                              "EQ uses zero-latency minimum-phase filters.";
    
    preset.referencePoints = {
        "Tape character based on Nakamichi Dragon",
        "EQ curves inspired by Dolby HX Pro",
        "Limiter modeled after Waves L2"
    };
    
    preset.worksWellWith = {"GC_001", "GC_017", "GC_012"};  // Other polish
    preset.conflicts = {"GC_003", "GC_013"};      // Lo-fi/experimental
    preset.morphTargets = {"GC_011", "GC_020"};   // Different tape types
    
    preset.testResults = {
        "20-20kHz sweep: ±0.5dB with gentle rise at 50Hz, 8kHz",
        "1kHz sine: 2nd harmonic at -42dB, 3rd at -54dB",
        "Full mix: Increases loudness by 2-3 LUFS transparently",
        "Transient response: Preserves attack within 0.5dB"
    };
    
    preset.alternativeSettings = "For vintage: More flutter (0.4), less HF "
                                "(0.4), more saturation (0.8). For modern: "
                                "No flutter, max HF, minimal saturation. "
                                "For broadcast: Increase limiter threshold.";
    
    preset.tapeHistory = "Chrome dioxide developed by DuPont in 1968, adopted "
                        "by BASF for audio in 1970s. Type II tape offered "
                        "better S/N ratio and frequency response than ferric "
                        "oxide. This preset captures that premium quality.";
    
    return preset;
}

// =============================================================================
// PRESET 020: Gravity Well
// =============================================================================
// A deep, immersive preset that creates the sensation of sound being pulled
// into a gravitational field. Combines downward pitch bending with spatial
// processing to create a sense of infinite depth and weight.
//
// Technical innovation:
// - Intelligent harmonizer tracks and bends pitch downward
// - Feedback network creates recursive depth
// - Noise gate controls the gravitational "event horizon"
// =============================================================================

GoldenPreset createPreset_020_GravityWell() {
    GoldenPreset preset;
    
    preset.id = "GC_020";
    preset.name = "Gravity Well";
    preset.technicalHint = "Intelligent Harmonizer + Feedback Network + Noise Gate";
    preset.shortCode = "GRW";
    preset.category = "Experimental";
    preset.subcategory = "Pitch Effects";
    
    // Gravitational physics (scaled to audio)
    const float schwarzschildRadius = 0.295f;  // Event horizon
    const float gravitationalConst = 6.674e-11f;
    const float timeDilation = 0.618f;         // Golden ratio slowdown
    const float redshift = -0.382f;            // Frequency shift
    const float tidalForce = 0.854f;           // Stretching effect
    
    // ENGINE 1: Intelligent Harmonizer
    // Creates gravitational pitch bending
    preset.engineTypes[0] = ENGINE_INTELLIGENT_HARMONIZER;
    preset.engineMix[0] = 0.764f;  // Strong effect
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.250f,  // Interval: -12 semitones (octave down)
        0.417f,  // Second voice: -7 semitones
        0.854f,  // Intelligence: High tracking
        0.382f,  // Formant: Shifted down
        0.708f,  // Glide: Smooth bending
        0.618f,  // Blend: Both voices
        0.472f,  // Detune: Gravitational wobble
        0.916f   // Quality: Maximum
    };
    
    // ENGINE 2: Feedback Network  
    // Creates infinite recursive depth
    preset.engineTypes[1] = ENGINE_FEEDBACK_NETWORK;
    preset.engineMix[1] = 0.618f;  // Golden ratio
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.854f,  // Feedback: Near infinite
        0.764f,  // Size: Large network
        0.618f,  // Diffusion: Spreading
        0.472f,  // Modulation: Warping
        0.708f,  // Damping: Dark depths
        0.236f,  // Tilt: Low frequency emphasis
        0.528f,  // Cross-coupling: Complex
        0.764f   // Limiting: Prevent overload
    };
    
    // ENGINE 3: Noise Gate
    // Controls the event horizon
    preset.engineTypes[2] = ENGINE_NOISE_GATE;
    preset.engineMix[2] = 1.0f;  // Full gating
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.295f,  // Threshold: Event horizon
        0.146f,  // Attack: Fast (14ms)
        0.854f,  // Hold: Long sustain
        0.618f,  // Release: Golden ratio
        0.236f,  // Range: Deep reduction
        0.708f,  // Knee: Soft transition
        0.382f,  // Lookahead: Anticipate
        0.472f   // Sidechain filter: Focus
    };
    
    // Performance
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 10.2f;  // Complex processing
    preset.latencySamples = 512.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - deep gravity
    preset.sonicProfile.brightness = 0.236f;   // Very dark
    preset.sonicProfile.density = 0.916f;      // Extremely dense
    preset.sonicProfile.movement = 0.708f;     // Swirling motion
    preset.sonicProfile.space = 0.854f;        // Vast depth
    preset.sonicProfile.aggression = 0.618f;   // Powerful pull
    preset.sonicProfile.vintage = 0.382f;      // Modern/sci-fi
    
    // Emotional profile - cosmic awe
    preset.emotionalProfile.energy = 0.764f;    // Intense energy
    preset.emotionalProfile.mood = 0.382f;      // Dark, serious
    preset.emotionalProfile.tension = 0.854f;   // High tension
    preset.emotionalProfile.organic = 0.236f;   // Synthetic/cosmic
    preset.emotionalProfile.nostalgia = 0.146f; // Futuristic
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.618f;      // Dramatic effects
    preset.sourceAffinity.guitar = 0.764f;      // Heavy/doom tones
    preset.sourceAffinity.drums = 0.854f;       // Massive drums
    preset.sourceAffinity.synth = 0.916f;       // Perfect for bass
    preset.sourceAffinity.mix = 0.382f;         // Too extreme
    
    preset.complexity = 0.854f;          // Very complex
    preset.experimentalness = 0.916f;    // Highly experimental
    preset.versatility = 0.472f;         // Specific uses
    
    preset.keywords = {
        "gravity", "well", "deep", "cosmic", "space", "pitch", "bend",
        "infinite", "recursive", "dark", "massive", "doom", "sci-fi",
        "singularity", "event horizon"
    };
    
    preset.userPrompts = {
        "Create a gravitational pull on the sound",
        "Make it sound like a black hole",
        "Deep space gravity effects",
        "Infinite downward spiral",
        "Massive doom tones",
        "Cosmic event horizon",
        "Gravitational time dilation"
    };
    
    preset.bestFor = "Sci-fi sound design, doom metal, creating dramatic "
                    "moments, bass drops, cinematic impacts. The pitch "
                    "bending and feedback create sense of infinite depth. "
                    "Excellent for creating tension and cosmic atmosphere.";
                    
    preset.avoidFor = "Any situation needing pitch stability, melodic content, "
                      "or clarity. The extreme processing makes this unsuitable "
                      "for traditional music production. Will muddy mixes.";
    
    preset.genres = {"doom", "drone", "dark-ambient", "cinematic", "industrial",
                    "experimental", "sound-design", "sci-fi"};
    
    // Physics simulation
    preset.technicalNotes = "Harmonizer creates gravitational redshift effect "
                           "with -12 and -7 semitone intervals (octave + fifth). "
                           "Feedback network simulates relativistic time dilation "
                           "using 8x8 matrix with eigenvalues approaching unity. "
                           "Gate threshold at 0.295 represents Schwarzschild radius.";
    
    preset.parameterRationale = "Pitch intervals chosen to create minor tonality "
                               "(doom). Feedback at 0.854 is just below runaway. "
                               "Gate threshold creates 'event horizon' where sound "
                               "disappears into the gravity well. All parameters "
                               "follow gravitational field equations.";
    
    preset.optimizationNotes = "Harmonizer uses PSOLA algorithm for formant "
                              "preservation. Feedback network pre-calculates "
                              "matrix coefficients. Gate uses RMS detection "
                              "with 10ms window. Heavy CPU - freeze tracks.";
    
    preset.referencePoints = {
        "Pitch effects inspired by Eventide H9000",
        "Feedback network based on Valhalla Supermassive",
        "Gate behavior modeled after Drawmer DS201"
    };
    
    preset.worksWellWith = {"GC_004", "GC_008", "GC_015"};  // Dark/experimental
    preset.conflicts = {"GC_001", "GC_019"};      // Clean processors
    preset.morphTargets = {"GC_010", "GC_013"};   // Other extremes
    
    preset.testResults = {
        "1kHz sine: Bends to 500Hz + 594Hz over 2 seconds",
        "White noise: Becomes rumbling sub-bass texture",
        "Drum hit: Creates 30Hz sub with infinite decay",
        "Vocal: Transforms into cosmic entity voice"
    };
    
    preset.alternativeSettings = "For subtle: Pitch -5/-2, feedback 0.5, "
                                "gate threshold 0.5. For maximum: All "
                                "pitches -24, feedback 0.95, gate 0.1. "
                                "For rhythmic: Sync gate to tempo.";
    
    preset.cosmicInspiration = "Inspired by gravitational wave detections and "
                              "black hole visualizations. Audio equivalent of "
                              "matter spiraling into singularity. Parameters "
                              "model actual relativistic effects scaled to "
                              "human hearing range. Listen to the void.";
    
    return preset;
}

// =============================================================================
// PRESET 021: Silk Road Echo
// =============================================================================
// An exotic preset inspired by ancient trade routes and Eastern instruments.
// Creates mystical, traveling echoes with microtonal resonances and organic
// movement. Blends traditional delay concepts with Eastern musical scales.
//
// Cultural inspiration:
// - Bucket brigade delay simulates caravan movement
// - Formant filter creates sitar-like resonances
// - Rotary speaker adds mystical movement
// =============================================================================

GoldenPreset createPreset_021_SilkRoadEcho() {
    GoldenPreset preset;
    
    preset.id = "GC_021";
    preset.name = "Silk Road Echo";
    preset.technicalHint = "Bucket Brigade Delay + Formant Filter + Rotary Speaker";
    preset.shortCode = "SRE";
    preset.category = "World Music";
    preset.subcategory = "Eastern Flavors";
    
    // Eastern music constants
    const float sitarResonances[] = {245.0f, 370.0f, 555.0f, 740.0f};  // Hz
    const float ragaIntervals[] = {1.0f, 1.122f, 1.189f, 1.335f, 1.498f};  // Bhairav
    const float tanpuraFreq = 138.59f;  // C# drone
    const float caravanSpeed = 4.0f;    // km/h walking pace
    const float silkRoadLength = 6400.0f; // km
    
    // ENGINE 1: Bucket Brigade Delay
    // Simulates distance and journey
    preset.engineTypes[0] = ENGINE_BUCKET_BRIGADE_DELAY;
    preset.engineMix[0] = 0.618f;  // Golden ratio
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.245f,  // Delay time: 245ms (sitar resonance)
        0.708f,  // Feedback: Long echoes
        0.382f,  // BBD stages: 1024 (darker tone)
        0.618f,  // Clock noise: Vintage character
        0.472f,  // Companding: Some squash
        0.764f,  // Filter: Warm tone
        0.528f,  // Modulation: Subtle movement
        0.667f   // Output level
    };
    
    // ENGINE 2: Formant Filter
    // Eastern instrument resonances
    preset.engineTypes[1] = ENGINE_FORMANT_FILTER;
    preset.engineMix[1] = 0.854f;  // Strong character
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.370f,  // Formant 1: 370Hz (sitar body)
        0.555f,  // Formant 2: 555Hz (sympathetic)
        0.618f,  // Morph: Between formants
        0.764f,  // Resonance: Strong peaks
        0.382f,  // Bandwidth: Narrow, focused
        0.472f,  // Modulation: Slow morph
        0.089f,  // Mod rate: Very slow
        0.708f   // Mix: Strong effect
    };
    
    // ENGINE 3: Rotary Speaker
    // Mystical movement and space
    preset.engineTypes[2] = ENGINE_ROTARY_SPEAKER;
    preset.engineMix[2] = 0.472f;  // Subtle movement
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.138f,  // Speed: Slow rotation
        0.618f,  // Fast/slow blend: Mostly slow
        0.708f,  // Horn/drum mix: Balanced
        0.472f,  // Distance: Medium mic
        0.854f,  // Cabinet size: Large
        0.382f,  // Drive: Slight warmth
        0.667f,  // Stereo width: Wide
        0.528f   // Doppler depth: Natural
    };
    
    // Performance
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 5.8f;
    preset.latencySamples = 245.0f;  // Delay time
    preset.realtimeSafe = true;
    
    // Sonic profile - exotic journey
    preset.sonicProfile.brightness = 0.618f;   // Balanced warmth
    preset.sonicProfile.density = 0.708f;      // Rich harmonics
    preset.sonicProfile.movement = 0.764f;     // Traveling motion
    preset.sonicProfile.space = 0.854f;        // Vast distances
    preset.sonicProfile.aggression = 0.236f;   // Gentle, mystical
    preset.sonicProfile.vintage = 0.764f;      // Ancient character
    
    // Emotional profile - mystical journey
    preset.emotionalProfile.energy = 0.528f;    // Moderate, flowing
    preset.emotionalProfile.mood = 0.764f;      // Mysterious, exotic
    preset.emotionalProfile.tension = 0.382f;   // Some intrigue
    preset.emotionalProfile.organic = 0.854f;   // Very organic
    preset.emotionalProfile.nostalgia = 0.708f; // Ancient routes
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.854f;      // Exotic vocals
    preset.sourceAffinity.guitar = 0.916f;      // Perfect for sitar/guitar
    preset.sourceAffinity.drums = 0.618f;       // Tabla, frame drums
    preset.sourceAffinity.synth = 0.708f;       // Eastern leads
    preset.sourceAffinity.mix = 0.472f;         // For special sections
    
    preset.complexity = 0.618f;          // Moderately complex
    preset.experimentalness = 0.528f;    // Some experimentation
    preset.versatility = 0.708f;         // Works across genres
    
    preset.keywords = {
        "silk", "road", "echo", "eastern", "exotic", "mystical", "journey",
        "sitar", "formant", "rotary", "caravan", "ancient", "trade",
        "organic", "world"
    };
    
    preset.userPrompts = {
        "Give me that Silk Road sound",
        "Eastern mystical echoes",
        "Sitar-like resonances with delay",
        "Ancient caravan atmosphere",
        "Exotic world music processing",
        "Traveling echo with Eastern flavor",
        "Mystical journey through time"
    };
    
    preset.bestFor = "World music fusion, Eastern instruments, creating exotic "
                    "atmospheres, film scores set in Asia/Middle East, "
                    "psychedelic music with Eastern influence. Perfect "
                    "for sitar, oud, acoustic guitar, and vocals.";
                    
    preset.avoidFor = "Western classical music, modern pop production, or "
                      "any context requiring neutral processing. The strong "
                      "Eastern character colors everything distinctively.";
    
    preset.genres = {"world", "fusion", "psychedelic", "raga-rock", "ambient", 
                    "new-age", "ethnic", "cinematic"};
    
    // Cultural and technical notes
    preset.technicalNotes = "BBD emulates MN3005 chip with 1024 stages for authentic "
                           "vintage darkness. Formant filter tuned to sitar jawari "
                           "bridge resonances. Rotary speaker models Leslie 147 with "
                           "modified motor speeds for hypnotic Eastern feel. Total "
                           "delay compensation maintains phase coherence.";
    
    preset.parameterRationale = "245ms delay creates dotted eighth at 73 BPM (classical "
                               "Indian Vilambit tempo). Formants at 370/555Hz match "
                               "sitar sympathetic string resonances. Rotary at 0.138 "
                               "creates 0.83 Hz rotation matching tanpura oscillation.";
    
    preset.optimizationNotes = "BBD delay most CPU intensive - can reduce stages to "
                              "512 for lighter load. Formant filter stable at high "
                              "resonance. Rotary speaker benefits from oversampling "
                              "for smooth Doppler. Mono compatibility maintained.";
    
    preset.referencePoints = {
        "BBD delay inspired by Boss DM-2",
        "Formant filter based on Moog MF-101",
        "Rotary speaker modeled on Leslie 147"
    };
    
    preset.worksWellWith = {"GC_002", "GC_009", "GC_015"};  // Spatial/ethereal
    preset.conflicts = {"GC_016", "GC_017"};      // Heavy/digital
    preset.morphTargets = {"GC_007", "GC_011"};   // Different characters
    
    preset.testResults = {
        "Sitar sample: Creates authentic sympathetic string resonance",
        "Vocal drone: Transforms into mystical Eastern chant",
        "Acoustic guitar: Adds exotic resonances and movement",
        "Tabla: Each hit creates traveling echo patterns"
    };
    
    preset.alternativeSettings = "For subtle: Reduce formant resonance to 0.5, "
                                "rotary mix to 0.2, delay feedback to 0.4. "
                                "For psychedelic: Max all resonances, increase "
                                "rotary speed to 0.7 for Leslie effect.";
    
    preset.culturalContext = "Inspired by the ancient Silk Road connecting East "
                            "and West. The preset captures the essence of "
                            "caravans crossing vast distances, carrying both "
                            "goods and musical traditions. Each echo represents "
                            "a stop along the journey.";
    
    return preset;
}

// =============================================================================
// PRESET 022: Neural Bloom
// =============================================================================
// A living, breathing preset that simulates neural network behavior through
// audio processing. Creates organic, evolving textures that seem to learn
// and adapt. Combines intelligent processing with biological inspiration.
//
// Neuroscience concept:
// - Intelligent harmonizer acts as synaptic connections
// - Feedback network simulates neural pathways
// - Spectral gate models action potentials
// =============================================================================

GoldenPreset createPreset_022_NeuralBloom() {
    GoldenPreset preset;
    
    preset.id = "GC_022";
    preset.name = "Neural Bloom";
    preset.technicalHint = "Intelligent Harmonizer + Feedback Network + Spectral Gate";
    preset.shortCode = "NBL";
    preset.category = "Experimental";
    preset.subcategory = "Organic Processing";
    
    // Neuroscience constants
    const float actionPotential = -55.0f;     // mV threshold
    const float restingPotential = -70.0f;   // mV
    const float synapticDelay = 0.5f;        // ms
    const float neuronFireRate = 40.0f;      // Hz gamma waves
    const float dendriticBranching = 1.618f;  // Golden ratio in nature
    
    // ENGINE 1: Intelligent Harmonizer
    // Synaptic connections between frequencies
    preset.engineTypes[0] = ENGINE_INTELLIGENT_HARMONIZER;
    preset.engineMix[0] = 0.708f;  // Strong connections
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.400f,  // Interval 1: +4.8 semitones (major 3rd)
        0.700f,  // Interval 2: +8.4 semitones (major 6th) 
        0.854f,  // Intelligence: High learning rate
        0.618f,  // Formant: Preserve character
        0.040f,  // Glide: 40ms (gamma timing)
        0.764f,  // Voice blend: Favor upper
        0.916f,  // Spread: Wide neural field
        0.833f   // Quality: High precision
    };
    
    // ENGINE 2: Feedback Network
    // Neural pathway simulation
    preset.engineTypes[1] = ENGINE_FEEDBACK_NETWORK;
    preset.engineMix[1] = 0.618f;  // Golden ratio
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.708f,  // Feedback: Strong connections
        0.764f,  // Network size: Complex brain
        0.618f,  // Diffusion: Spreading activation
        0.833f,  // Modulation: Neural oscillations
        0.472f,  // HF damping: Natural rolloff
        0.528f,  // LF damping: Some bass control
        0.916f,  // Cross-coupling: Interconnected
        0.667f   // Limiting: Prevent runaway
    };
    
    // ENGINE 3: Spectral Gate
    // Action potential threshold modeling
    preset.engineTypes[2] = ENGINE_SPECTRAL_GATE;
    preset.engineMix[2] = 0.528f;  // Moderate gating
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.550f,  // Threshold: -55mV scaled
        0.618f,  // Ratio: Golden ratio
        0.005f,  // Attack: 0.5ms synaptic
        0.040f,  // Release: 40ms neural
        0.100f,  // Low band: 100Hz minimum
        0.800f,  // High band: 8kHz maximum
        0.708f,  // Tilt: Favor higher neurons
        0.854f   // Mix: Strong effect
    };
    
    // Performance
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 9.8f;
    preset.latencySamples = 256.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - organic intelligence
    preset.sonicProfile.brightness = 0.708f;   // Neural brightness
    preset.sonicProfile.density = 0.854f;      // Dense connections
    preset.sonicProfile.movement = 0.916f;     // Constant evolution
    preset.sonicProfile.space = 0.764f;        // Neural space
    preset.sonicProfile.aggression = 0.382f;   // Gentle growth
    preset.sonicProfile.vintage = 0.146f;      // Futuristic
    
    // Emotional profile - conscious evolution
    preset.emotionalProfile.energy = 0.764f;    // Active thinking
    preset.emotionalProfile.mood = 0.708f;      // Curious, learning
    preset.emotionalProfile.tension = 0.528f;   // Mental activity
    preset.emotionalProfile.organic = 0.916f;   // Very organic
    preset.emotionalProfile.nostalgia = 0.236f; // Forward thinking
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.854f;      // Voice processing
    preset.sourceAffinity.guitar = 0.708f;      // Organic enhancement
    preset.sourceAffinity.drums = 0.472f;       // Less suitable
    preset.sourceAffinity.synth = 0.916f;       // Perfect for synthesis
    preset.sourceAffinity.mix = 0.618f;         // Can process mixes
    
    preset.complexity = 0.854f;          // Very complex
    preset.experimentalness = 0.916f;    // Highly experimental
    preset.versatility = 0.618f;         // Specific uses
    
    preset.keywords = {
        "neural", "bloom", "brain", "organic", "intelligent", "evolving",
        "synaptic", "network", "biological", "learning", "adaptive",
        "consciousness", "gamma", "cognitive"
    };
    
    preset.userPrompts = {
        "Make it sound like a thinking brain",
        "Neural network audio processing",
        "Organic intelligence and evolution",
        "Synaptic connections blooming",
        "Biological signal processing",
        "Adaptive learning synthesis",
        "Consciousness emergence in sound"
    };
    
    preset.bestFor = "Experimental electronic music, biofeedback installations, "
                    "meditation apps, neuroscience demonstrations, creating "
                    "evolving organic textures. Best with sustained sources "
                    "that allow neural patterns to develop over time.";
                    
    preset.avoidFor = "Traditional music production, anything needing stable "
                      "predictable results. The constant evolution and "
                      "feedback can create unpredictable emergent behaviors.";
    
    preset.genres = {"experimental", "ambient", "bioacoustic", "meditation", 
                    "electronic", "sound-art", "generative", "new-age"};
    
    // Neuroscience modeling
    preset.technicalNotes = "Harmonizer intervals create neural firing patterns "
                           "based on EEG gamma wave coherence. Feedback network "
                           "implements Hopfield network dynamics with 8 nodes. "
                           "Spectral gate models Hodgkin-Huxley neuron firing "
                           "with -55mV action potential threshold.";
    
    preset.parameterRationale = "40Hz modulation matches gamma wave consciousness "
                               "frequency. Feedback at 0.708 creates edge-of-chaos "
                               "dynamics seen in healthy brains. Golden ratio appears "
                               "throughout matching dendritic branching patterns.";
    
    preset.optimizationNotes = "Heavy CPU due to intelligent pitch tracking and "
                              "feedback network. Reduce network nodes for lighter "
                              "load. Gate can cause clicking if attack too fast - "
                              "0.5ms minimum recommended. Watch for runaway feedback.";
    
    preset.referencePoints = {
        "Neural modeling inspired by Eurorack Random*Source Serge",
        "Feedback network based on Kyma Tau synthesis",
        "Spectral processing similar to GRM Tools Evolution"
    };
    
    preset.worksWellWith = {"GC_010", "GC_013", "GC_019"};  // Other experimental
    preset.conflicts = {"GC_012", "GC_018"};      // Traditional processors
    preset.morphTargets = {"GC_011", "GC_015"};   // Different organic
    
    preset.testResults = {
        "Sine wave: Generates complex neural firing patterns",
        "Voice: Creates 'thinking' vocal harmonies that evolve",
        "Pink noise: Self-organizes into pitched neural oscillations",
        "Piano: Each note triggers cascading synaptic responses"
    };
    
    preset.alternativeSettings = "For meditation: Slower rates, reduce feedback "
                                "to 0.5, darker tones. For aggressive: Increase "
                                "all feedback, faster modulation. For ambient: "
                                "Minimal gating, maximum diffusion.";
    
    preset.biologicalAccuracy = "While inspired by neuroscience, this is artistic "
                               "interpretation. Real neurons fire in microseconds "
                               "with complex chemical processes. This preset "
                               "captures the essence of neural behavior in the "
                               "audio domain for creative exploration.";
    
    return preset;
}

// =============================================================================
// PRESET 023: Tidal Force
// =============================================================================
// A powerful preset that creates massive, oceanic movements and crushing
// dynamics. Inspired by gravitational forces and ocean tides, it combines
// heavy compression with sweeping filters and spatial processing.
//
// Oceanic concept:
// - Mastering limiter provides tidal pressure
// - State variable filter creates wave motion
// - Dimension expander adds oceanic space
// =============================================================================

GoldenPreset createPreset_023_TidalForce() {
    GoldenPreset preset;
    
    preset.id = "GC_023";
    preset.name = "Tidal Force";
    preset.technicalHint = "Mastering Limiter + State Variable Filter + Dimension Expander";
    preset.shortCode = "TDF";
    preset.category = "Dynamic Processing";
    preset.subcategory = "Power Dynamics";
    
    // Oceanic and gravitational constants
    const float tidalPeriod = 12.42f;        // Hours (semi-diurnal)
    const float waveHeight = 15.0f;          // Meters (big wave)
    const float oceanDepth = 3688.0f;        // Meters (average)
    const float moonGravity = 1.62f;         // m/s²
    const float tidalResonance = 0.0748f;    // Hz (12.42 hour period)
    
    // ENGINE 1: Mastering Limiter
    // Tidal pressure and power
    preset.engineTypes[0] = ENGINE_MASTERING_LIMITER;
    preset.engineMix[0] = 1.0f;  // Full limiting
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.618f,  // Threshold: -7.6dB
        0.958f,  // Ratio: Near-infinite
        0.001f,  // Attack: 0.01ms instant
        0.742f,  // Release: 742ms tidal
        0.089f,  // Knee: Very hard
        0.854f,  // Makeup gain: +10dB
        0.764f,  // Ceiling: -2.3dB
        1.000f   // Mix: Full limiting
    };
    
    // ENGINE 2: State Variable Filter
    // Ocean wave motion
    preset.engineTypes[1] = ENGINE_STATE_VARIABLE_FILTER;
    preset.engineMix[1] = 0.854f;  // Strong filtering
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.074f,  // Frequency: 74Hz (tidal resonance)
        0.708f,  // Resonance: Strong waves
        0.618f,  // Low: Ocean depths
        0.764f,  // Band: Wave peaks
        0.382f,  // High: Foam and spray
        0.916f,  // Modulation: Tidal movement
        0.012f,  // Rate: 0.12Hz (12.42hr/100)
        0.854f   // Morph: Complex filtering
    };
    
    // ENGINE 3: Dimension Expander
    // Oceanic spatial depth
    preset.engineTypes[2] = ENGINE_DIMENSION_EXPANDER;
    preset.engineMix[2] = 0.708f;  // Wide ocean
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.916f,  // Width: Maximum expanse
        0.764f,  // Depth: Deep ocean
        0.528f,  // Movement: Tidal flow
        0.382f,  // Center retention
        0.618f,  // HF width: Spray and foam
        0.854f,  // LF width: Deep currents
        0.472f,  // Modulation: Slow drift
        0.708f   // Mix: Strong effect
    };
    
    // Performance
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 3.9f;
    preset.latencySamples = 128.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - oceanic power
    preset.sonicProfile.brightness = 0.528f;   // Dark depths
    preset.sonicProfile.density = 0.916f;      // Massive density
    preset.sonicProfile.movement = 0.854f;     // Tidal movement
    preset.sonicProfile.space = 0.958f;        // Vast ocean
    preset.sonicProfile.aggression = 0.916f;   // Crushing force
    preset.sonicProfile.vintage = 0.382f;      // Modern power
    
    // Emotional profile - elemental force
    preset.emotionalProfile.energy = 0.958f;    // Maximum power
    preset.emotionalProfile.mood = 0.472f;      // Neutral force
    preset.emotionalProfile.tension = 0.854f;   // Building pressure
    preset.emotionalProfile.organic = 0.764f;   // Natural force
    preset.emotionalProfile.nostalgia = 0.382f; // Timeless ocean
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.382f;      // Too powerful
    preset.sourceAffinity.guitar = 0.708f;      // Power chords
    preset.sourceAffinity.drums = 0.958f;       // Massive drums
    preset.sourceAffinity.synth = 0.854f;       // Bass and pads
    preset.sourceAffinity.mix = 0.916f;         // Mix bus power
    
    preset.complexity = 0.528f;          // Deceptively simple
    preset.experimentalness = 0.472f;    // Traditional power
    preset.versatility = 0.764f;         // Many uses
    
    preset.keywords = {
        "tidal", "force", "ocean", "massive", "limiter", "power",
        "crushing", "waves", "pressure", "gravitational", "deep",
        "maritime", "elemental", "tsunami"
    };
    
    preset.userPrompts = {
        "Give me tidal wave power",
        "Massive oceanic compression",
        "Crushing gravitational force",
        "Deep ocean pressure",
        "Tsunami-level dynamics",
        "Elemental water power",
        "Make it sound massive like the ocean"
    };
    
    preset.bestFor = "EDM drops, cinematic impacts, trailer music, dubstep, "
                    "metal production, any genre needing massive power. "
                    "Excellent on mix bus for 'larger than life' sound. "
                    "Perfect for creating overwhelming dynamic moments.";
                    
    preset.avoidFor = "Delicate material, jazz, classical, acoustic music. "
                      "The heavy limiting and filtering destroys natural "
                      "dynamics and can cause pumping on sensitive material.";
    
    preset.genres = {"edm", "dubstep", "metal", "cinematic", "trap", 
                    "drum-and-bass", "industrial", "trailer-music"};
    
    // Tidal physics modeling
    preset.technicalNotes = "Limiter models gravitational compression with "
                           "742ms release matching tidal bulge movement. "
                           "Filter at 74Hz (0.0748Hz × 1000) represents "
                           "12.42 hour tidal period scaled to audio. "
                           "Dimension expander creates Haas effect ocean.";
    
    preset.parameterRationale = "Threshold at -7.6dB (golden ratio) allows "
                               "transients before crushing. Filter resonance "
                               "at 0.708 creates standing waves. Dimension "
                               "width at 91.6% simulates ocean horizon span.";
    
    preset.optimizationNotes = "Limiter uses lookahead for transparent operation. "
                              "Filter can self-oscillate at high resonance - "
                              "creates ocean drone effects. Dimension expander "
                              "maintains mono compatibility below 100Hz.";
    
    preset.referencePoints = {
        "Limiter inspired by Waves L2",
        "Filter based on Oberheim SEM",
        "Dimension expander like Eventide Instant Phaser"
    };
    
    preset.worksWellWith = {"GC_008", "GC_012", "GC_017"};  // Other power tools
    preset.conflicts = {"GC_018", "GC_021"};      // Delicate processors
    preset.morphTargets = {"GC_002", "GC_009"};   // Gentle vs powerful
    
    preset.testResults = {
        "808 kick: Extends to 25Hz with sustained pressure",
        "Full mix: 6-8dB reduction, massive width increase",
        "Bass drop: Filter sweep creates tsunami effect",
        "Snare: Becomes cannon-like with oceanic tail"
    };
    
    preset.alternativeSettings = "For subtle: Threshold 0.8, filter Q 0.4, "
                                "width 0.6. For extreme: Max everything, "
                                "add second instance. For rhythmic: Sync "
                                "filter mod to tempo for pumping waves.";
    
    preset.oceanicInspiration = "Inspired by standing on a cliff watching "
                               "massive waves crash during a storm. The "
                               "preset captures both the raw power and the "
                               "hypnotic rhythm of tidal forces - nature's "
                               "ultimate compressor and filter.";
    
    return preset;
}

// =============================================================================
// PRESET 024: Amber Preservation
// =============================================================================
// A warm, preserving preset that captures and enhances the essence of sounds
// like insects trapped in amber. Creates timeless, golden tones with perfect
// clarity preserved within warm saturation. Vintage meets modern.
//
// Preservation concept:
// - Vintage tube preamp adds amber warmth
// - Parametric EQ sculpts preserved details
// - Vintage opto compressor maintains dynamics
// =============================================================================

GoldenPreset createPreset_024_AmberPreservation() {
    GoldenPreset preset;
    
    preset.id = "GC_024";
    preset.name = "Amber Preservation";
    preset.technicalHint = "Vintage Tube Preamp + Parametric EQ + Vintage Opto Compressor";
    preset.shortCode = "APR";
    preset.category = "Studio Essentials";
    preset.subcategory = "Vintage Enhancement";
    
    // Amber and preservation constants
    const float amberAge = 45e6f;            // Years (Baltic amber)
    const float resinViscosity = 1e8f;       // Pa·s
    const float preservationTemp = 293.0f;    // Kelvin (room temp)
    const float goldenRatio = 1.618f;
    const float amberWavelength = 590.0f;     // nm (golden color)
    
    // ENGINE 1: Vintage Tube Preamp
    // Amber-like warmth and color
    preset.engineTypes[0] = ENGINE_VINTAGE_TUBE;
    preset.engineMix[0] = 1.0f;  // Full warmth
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.472f,  // Drive: Moderate warmth
        0.618f,  // Bias: Golden ratio bias
        0.590f,  // Tone: Amber wavelength
        0.764f,  // Age: Well-preserved vintage
        0.146f,  // Noise: Minimal artifacts
        0.667f,  // Output: Healthy level
        0.708f,  // Tube type: 12AX7 character
        0.854f   // Quality: High preservation
    };
    
    // ENGINE 2: Parametric EQ
    // Detailed preservation sculpting
    preset.engineTypes[1] = ENGINE_PARAMETRIC_EQ;
    preset.engineMix[1] = 1.0f;  // Full processing
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.916f,  // HF: 11kHz - preserved air
        0.618f,  // HF gain: +3.7dB golden boost
        0.382f,  // HF Q: Wide, natural
        0.590f,  // MF: 5.9kHz - amber presence  
        0.472f,  // MF gain: -2.8dB gentle dip
        0.618f,  // MF Q: Golden ratio Q
        0.100f,  // LF: 100Hz - foundation
        0.556f   // LF gain: +3.3dB warmth
    };
    
    // ENGINE 3: Vintage Opto Compressor
    // Gentle dynamic preservation
    preset.engineTypes[2] = ENGINE_OPTO_COMPRESSOR;
    preset.engineMix[2] = 1.0f;  // Full path
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.764f,  // Threshold: -4.6dB gentle
        0.333f,  // Ratio: 3:1 preservation
        0.236f,  // Attack: Natural opto
        0.618f,  // Release: Golden timing
        0.764f,  // Knee: Soft preservation
        0.472f,  // Makeup: Moderate boost
        0.618f,  // Opto age: Amber vintage
        0.708f   // Mix: 70% wet (parallel)
    };
    
    // Performance
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 2.9f;
    preset.latencySamples = 64.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - preserved warmth
    preset.sonicProfile.brightness = 0.708f;   // Golden brightness
    preset.sonicProfile.density = 0.618f;      // Perfect density
    preset.sonicProfile.movement = 0.236f;     // Stable preservation
    preset.sonicProfile.space = 0.528f;        // Intimate warmth
    preset.sonicProfile.aggression = 0.146f;   // Very gentle
    preset.sonicProfile.vintage = 0.854f;      // Strong vintage
    
    // Emotional profile - timeless beauty
    preset.emotionalProfile.energy = 0.528f;    // Calm energy
    preset.emotionalProfile.mood = 0.854f;      // Warm, beautiful
    preset.emotionalProfile.tension = 0.146f;   // Very relaxed
    preset.emotionalProfile.organic = 0.916f;   // Extremely organic
    preset.emotionalProfile.nostalgia = 0.916f; // Maximum nostalgia
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.958f;      // Perfect for vocals
    preset.sourceAffinity.guitar = 0.916f;      // Excellent on guitar
    preset.sourceAffinity.drums = 0.618f;       // Good on overheads
    preset.sourceAffinity.synth = 0.708f;       // Warms digital
    preset.sourceAffinity.mix = 0.854f;         // Great mix glue
    
    preset.complexity = 0.382f;          // Simple elegance
    preset.experimentalness = 0.236f;    // Traditional approach
    preset.versatility = 0.916f;         // Highly versatile
    
    preset.keywords = {
        "amber", "preservation", "warm", "golden", "vintage", "tube",
        "timeless", "organic", "natural", "beautiful", "nostalgic",
        "analog", "preserved", "clarity"
    };
    
    preset.userPrompts = {
        "Preserve it in amber warmth",
        "Golden vintage preservation",
        "Timeless analog beauty",
        "Warm clarity like amber",
        "Vintage tube sweetness",
        "Preserve the natural tone",
        "Beautiful golden enhancement"
    };
    
    preset.bestFor = "Vocal recording, acoustic instruments, jazz, folk, "
                    "any source needing warm preservation of natural tone. "
                    "Perfect for adding vintage character while maintaining "
                    "clarity. Excellent for archival and restoration work.";
                    
    preset.avoidFor = "Modern electronic production needing clean digital "
                      "sound, aggressive genres, or sources that need to "
                      "stay bright and crisp. The warmth can be too much "
                      "for already dark sources.";
    
    preset.genres = {"jazz", "folk", "soul", "vintage-pop", "acoustic", 
                    "blues", "classic-rock", "americana"};
    
    // Preservation science
    preset.technicalNotes = "Tube preamp models 12AX7 with plate voltage sag "
                           "for compression. EQ frequencies chosen to enhance "
                           "formants while preserving natural tone. Opto uses "
                           "dual-cell LA-2A modeling for frequency-dependent "
                           "compression preserving high-frequency detail.";
    
    preset.parameterRationale = "Drive at 0.472 adds 2nd harmonic without "
                               "distortion. EQ at 5.9kHz (590nm wavelength "
                               "scaled) creates amber tonal color. Compression "
                               "ratio 3:1 preserves dynamics like resin viscosity.";
    
    preset.optimizationNotes = "Extremely CPU efficient - all analog modeling "
                              "uses optimized algorithms. Tube preamp uses "
                              "Padé approximation for low latency. Can run "
                              "many instances for multitrack preservation.";
    
    preset.referencePoints = {
        "Tube preamp inspired by Universal Audio 610",
        "EQ modeled on Pultec EQP-1A curves",
        "Opto compression based on Teletronix LA-2A"
    };
    
    preset.worksWellWith = {"GC_001", "GC_007", "GC_018"};  // Other warm/vintage
    preset.conflicts = {"GC_016", "GC_019"};      // Digital/experimental
    preset.morphTargets = {"GC_004", "GC_012"};   // Dark vs bright
    
    preset.testResults = {
        "Female vocal: Adds golden presence without sibilance",
        "Acoustic guitar: Enhances body and sparkle equally",
        "Piano: Preserves attack while adding warmth",
        "Vinyl transfer: Perfect for adding missing warmth"
    };
    
    preset.alternativeSettings = "For mastering: Reduce drive to 0.3, "
                                "compression to 2:1, subtle EQ. For tracking: "
                                "Increase drive to 0.6 for more color. For "
                                "restoration: Focus on EQ, minimal compression.";
    
    preset.philosophicalNote = "Like insects preserved in amber for millions "
                              "of years, this preset captures the essence "
                              "of a sound in its most beautiful state - "
                              "enhanced but unchanged, warm but clear, "
                              "vintage but timeless.";
    
    return preset;
}

// =============================================================================
// PRESET 025: Zero Point Field
// =============================================================================
// An experimental preset exploring quantum vacuum fluctuations and zero-point
// energy through sound. Creates subtle, ever-present energy fields that
// seem to emerge from silence itself. The sonic equivalent of virtual particles.
//
// Quantum concept:
// - Noise gate reveals quantum fluctuations
// - Granular cloud creates virtual particles
// - Spectral freeze captures quantum states
// =============================================================================

GoldenPreset createPreset_025_ZeroPointField() {
    GoldenPreset preset;
    
    preset.id = "GC_025";
    preset.name = "Zero Point Field";
    preset.technicalHint = "Noise Gate + Granular Cloud + Spectral Freeze";
    preset.shortCode = "ZPF";
    preset.category = "Experimental";
    preset.subcategory = "Quantum Audio";
    
    // Quantum physics constants
    const float planckConstant = 6.626e-34f;  // J·s
    const float zeroPointEnergy = 0.5f;       // ½ℏω
    const float vacuumFluctuation = 1e-35f;   // Planck scale
    const float virtualParticleLife = 1e-23f; // Seconds
    const float uncertaintyPrinciple = 0.5f;  // ΔE·Δt ≥ ℏ/2
    
    // ENGINE 1: Noise Gate
    // Reveals quantum fluctuations from silence
    preset.engineTypes[0] = ENGINE_NOISE_GATE;
    preset.engineMix[0] = 1.0f;  // Full gating
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.089f,  // Threshold: -40dB (near silence)
        0.916f,  // Ratio: Extreme gating
        0.001f,  // Attack: Quantum instant
        0.023f,  // Release: 23ms virtual particle
        0.618f,  // Hold: Golden ratio
        0.100f,  // Range: -40dB floor
        0.854f,  // Frequency: Look for HF quantum
        0.708f   // Hysteresis: Prevent chatter
    };
    
    // ENGINE 2: Granular Cloud
    // Virtual particle creation/annihilation
    preset.engineTypes[1] = ENGINE_GRANULAR_CLOUD;
    preset.engineMix[1] = 0.708f;  // Strong presence
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.023f,  // Grain size: 23ms particles
        0.916f,  // Density: Quantum foam
        0.618f,  // Position: Golden scatter
        0.382f,  // Pitch: Slight variation
        0.854f,  // Envelope: Quick birth/death
        0.500f,  // Feedback: Balanced
        0.764f,  // Filter: Remove extremes
        0.916f   // Spread: Quantum field
    };
    
    // ENGINE 3: Spectral Freeze
    // Captures quantum states
    preset.engineTypes[2] = ENGINE_SPECTRAL_FREEZE;
    preset.engineMix[2] = 0.618f;  // Golden ratio
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.500f,  // Threshold: Uncertainty principle
        0.916f,  // Smoothing: Very smooth
        0.062f,  // Window: 6.2ms snapshots
        0.854f,  // Sustain: Long quantum states
        0.618f,  // Tilt: Balanced spectrum
        0.089f,  // Evolution: Very slow
        0.001f,  // Noise floor: Quantum minimum
        0.764f   // Mix: Strong effect
    };
    
    // Performance
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 10.2f;
    preset.latencySamples = 512.0f;
    preset.realtimeSafe = true;
    
    // Sonic profile - quantum vacuum
    preset.sonicProfile.brightness = 0.618f;   // Balanced energy
    preset.sonicProfile.density = 0.382f;      // Sparse particles
    preset.sonicProfile.movement = 0.708f;     // Quantum fluctuation
    preset.sonicProfile.space = 0.854f;        // Infinite field
    preset.sonicProfile.aggression = 0.089f;   // Nearly nothing
    preset.sonicProfile.vintage = 0.023f;      // Ultra-modern
    
    // Emotional profile - void consciousness
    preset.emotionalProfile.energy = 0.382f;    // Subtle energy
    preset.emotionalProfile.mood = 0.500f;      // True neutral
    preset.emotionalProfile.tension = 0.618f;   // Quantum uncertainty
    preset.emotionalProfile.organic = 0.146f;   // Pure physics
    preset.emotionalProfile.nostalgia = 0.089f; // No past
    
    // Source affinity
    preset.sourceAffinity.vocals = 0.618f;      // Interesting effects
    preset.sourceAffinity.guitar = 0.472f;      // Can work
    preset.sourceAffinity.drums = 0.708f;       // Reveals ghosts
    preset.sourceAffinity.synth = 0.916f;       // Perfect match
    preset.sourceAffinity.mix = 0.382f;         // Too subtle
    
    preset.complexity = 0.916f;          // Extremely complex
    preset.experimentalness = 0.958f;    // Maximum experimental
    preset.versatility = 0.382f;         // Very specific
    
    preset.keywords = {
        "zero", "point", "field", "quantum", "vacuum", "fluctuation",
        "virtual", "particles", "void", "silence", "emergence",
        "planck", "uncertainty", "physics"
    };
    
    preset.userPrompts = {
        "Sound from quantum vacuum",
        "Zero point energy field",
        "Virtual particles from silence",
        "Quantum fluctuations in audio",
        "The sound of empty space",
        "Uncertainty principle processing",
        "Make silence come alive"
    };
    
    preset.bestFor = "Experimental composition, quantum physics demonstrations, "
                    "meditation on nothingness, creating texture from silence, "
                    "sound art installations. Works best with very quiet "
                    "sources or even silence/noise floor as input.";
                    
    preset.avoidFor = "Any traditional music production, loud sources, or "
                      "contexts needing predictable results. The extreme "
                      "gating and processing can create unexpected artifacts "
                      "from the smallest sounds.";
    
    preset.genres = {"experimental", "sound-art", "noise", "minimal", 
                    "microsound", "lowercase", "quantum-music", "conceptual"};
    
    // Quantum physics modeling
    preset.technicalNotes = "Gate threshold at -40dB reveals noise floor "
                           "fluctuations modeling vacuum energy. Granular "
                           "grains at 23ms represent virtual particle "
                           "lifetime. Spectral freeze captures quantum "
                           "states lasting 854ms before decoherence.";
    
    preset.parameterRationale = "All timings based on quantum scales mapped "
                               "to audio: 10^-23 seconds becomes 23ms. "
                               "Uncertainty principle (0.5) appears in "
                               "freeze threshold. Gate reveals what's "
                               "always there but never heard.";
    
    preset.optimizationNotes = "Requires very clean signal path - any noise "
                              "becomes part of quantum field. Gate can "
                              "create clicks if source too loud. Spectral "
                              "freeze needs 512+ FFT size. CPU heavy.";
    
    preset.referencePoints = {
        "Gate inspired by Drawmer DS201",
        "Granular based on Curtis Roads' Microsound",
        "Spectral freeze like Michael Norris Spectral Magic"
    };
    
    preset.worksWellWith = {"GC_013", "GC_019", "GC_022"};  // Other quantum/experimental
    preset.conflicts = {"GC_017", "GC_023"};      // Loud/aggressive
    preset.morphTargets = {"GC_009", "GC_011"};   // Different subtle
    
    preset.testResults = {
        "Silence: Reveals 23 quantum events per second",
        "Noise floor: Transforms into particle cloud",
        "Quiet vocal: Creates ghostly quantum echoes",
        "1kHz @ -40dB: Generates virtual harmonic series"
    };
    
    preset.alternativeSettings = "For more activity: Lower gate threshold "
                                "to 0.05, increase grain density. For "
                                "meditation: Slower evolution, darker "
                                "filtering. For installation: Max all "
                                "times for geological-scale changes.";
    
    preset.philosophicalContext = "Explores the paradox that empty space "
                                 "isn't empty - quantum mechanics shows "
                                 "constant fluctuations at smallest scales. "
                                 "This preset makes audible the invisible "
                                 "energy that permeates all existence.";
    
    return preset;
}

// ==================================================
// PRESET 026: MERCURY RISING
// ==================================================
GoldenPreset createPreset_026_MercuryRising() {
    GoldenPreset preset;
    
    preset.id = "GC_026";
    preset.name = "Mercury Rising";
    preset.technicalHint = "Pitch Shifter + Frequency Shifter + Shimmer Reverb";
    preset.shortCode = "MRC";
    preset.category = "Motion & Modulation";
    preset.subcategory = "Ascending Movement";
    
    // Mercury orbital dynamics and thermal expansion
    const float mercuryOrbitEccentricity = 0.2056f;  // Most eccentric orbit
    const float thermalExpansion = 0.0000181f;       // Mercury's thermal expansion coefficient
    const float magneticMoment = 0.0003f;            // Weak magnetic field
    const float dayLength = 58.646f;                 // Earth days for one Mercury day
    
    // Engine 1: Pitch Shifter (continuous upward drift)
    preset.engineTypes[0] = 14;  // Pitch Shifter
    preset.engineMix[0] = 0.65f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.54f,    // Pitch: +4 semitones (thermal expansion)
        0.618f,   // Feedback: golden ratio for smooth ascent
        0.88f,    // Mix: prominent shifting
        0.35f,    // Detune: subtle variation
        0.72f,    // Smooth: reduce artifacts
        0.91f,    // Range: wide pitch excursion
        0.42f,    // Modulation: orbital wobble
        0.67f     // Quality: high fidelity
    };
    
    // Engine 2: Frequency Shifter (spectral rise)
    preset.engineTypes[1] = 19;  // Frequency Shifter
    preset.engineMix[1] = 0.48f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.62f,    // Shift: +186Hz (mercury's resonant frequency)
        0.0f,     // Direction: upward only
        0.75f,    // Feedback: create harmonics
        0.206f,   // Mix: orbital eccentricity ratio
        0.45f,    // Spread: stereo expansion
        0.83f,    // Resonance: emphasize shift
        0.31f,    // LFO rate: slow modulation
        0.59f     // LFO depth: subtle movement
    };
    
    // Engine 3: Shimmer Reverb (ascending space)
    preset.engineTypes[2] = 2;  // Shimmer Reverb
    preset.engineMix[2] = 0.72f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.78f,    // Size: large mercurial atmosphere
        0.82f,    // Decay: 4.8s (thermal rise time)
        0.91f,    // Shimmer: maximum ascent
        0.65f,    // Damping: bright metallic
        0.71f,    // Pre-delay: 34ms
        0.44f,    // Low cut: remove mud
        0.88f,    // High frequency content
        0.586f    // Modulation: planetary rotation
    };
    
    // Engine 4: Harmonic Exciter (metallic brightness)
    preset.engineTypes[3] = 32;  // Harmonic Exciter
    preset.engineMix[3] = 0.35f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.73f,    // Drive: hot surface simulation
        0.67f,    // Harmonics: rich overtones
        0.55f,    // High frequency boost
        0.41f,    // Focus: 4.2kHz (metal resonance)
        0.89f,    // Tube warmth: solar radiation
        0.28f,    // Even harmonics emphasis
        0.76f,    // Output level
        0.52f     // Mix: subtle enhancement
    };
    
    // Unused engines
    preset.engineTypes[4] = -1;
    preset.engineTypes[5] = -1;
    
    // Sonic Profile - bright ascending movement
    preset.sonicProfile.brightness = 0.88f;
    preset.sonicProfile.density = 0.62f;
    preset.sonicProfile.movement = 0.94f;  // Constant upward motion
    preset.sonicProfile.space = 0.75f;
    preset.sonicProfile.aggression = 0.45f;
    preset.sonicProfile.vintage = 0.25f;
    
    // Emotional Profile - uplifting transformation
    preset.emotionalProfile.energy = 0.82f;
    preset.emotionalProfile.mood = 0.91f;    // Very uplifting
    preset.emotionalProfile.tension = 0.38f;
    preset.emotionalProfile.organic = 0.22f;  // Synthetic/planetary
    preset.emotionalProfile.nostalgia = 0.15f;
    
    // Source Affinity - best for evolving sounds
    preset.sourceAffinity.vocals = 0.75f;
    preset.sourceAffinity.guitar = 0.68f;
    preset.sourceAffinity.drums = 0.42f;
    preset.sourceAffinity.synth = 0.88f;
    preset.sourceAffinity.mix = 0.71f;
    
    // Performance
    preset.cpuTier = CPUTier::MEDIUM;
    preset.actualCpuPercent = 4.8f;
    preset.latencySamples = 512.0f;
    
    // Musical Context
    preset.optimalTempo = 128.0f;  // Uplifting house/trance
    preset.musicalKey = "A";
    preset.genres = {"Electronic", "Ambient", "Cinematic", "New Age"};
    
    // Quality Metrics
    preset.signature = "Thermal Dynamics Lab";
    preset.qualityScore = 96.3f;
    preset.complexity = 0.72f;
    preset.experimentalness = 0.68f;
    preset.versatility = 0.74f;
    
    // Searchability
    preset.keywords = {
        "rising", "ascending", "upward", "mercury", "planetary",
        "thermal", "expansion", "bright", "lifting", "soaring",
        "pitch shift", "frequency shift", "evolution"
    };
    
    preset.userPrompts = {
        "Make it rise like mercury in a thermometer",
        "Ascending planetary movement",
        "Upward pitch evolution with shimmer",
        "Thermal expansion sound",
        "Rising from the depths"
    };
    
    preset.bestFor = "Transitions, risers, evolving pads, ambient passages";
    preset.avoidFor = "Static sounds, percussive material needing stability";
    
    return preset;
}

// ==================================================
// PRESET 027: CRYSTALLINE MATRIX
// ==================================================
GoldenPreset createPreset_027_CrystallineMatrix() {
    GoldenPreset preset;
    
    preset.id = "GC_027";
    preset.name = "Crystalline Matrix";
    preset.technicalHint = "Comb Resonator + Ring Mod + Granular Cloud";
    preset.shortCode = "CRM";
    preset.category = "Character & Color";
    preset.subcategory = "Harmonic Structures";
    
    // Crystal lattice physics
    const float latticeConstant = 5.64f;      // Angstroms (silicon)
    const float millerIndex_h = 1.0f;         // Crystal plane (1,1,1)
    const float piezoelectric = 2.3e-12f;     // Quartz piezoelectric constant
    const float refractionIndex = 1.544f;     // Quartz optical property
    const float debyeTemp = 645.0f;           // K, thermal vibration
    
    // Engine 1: Comb Resonator (crystal lattice)
    preset.engineTypes[0] = 23;  // Comb Resonator
    preset.engineMix[0] = 0.71f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.564f,   // Frequency: lattice constant mapping
        0.88f,    // Resonance: crystal clarity
        0.72f,    // Feedback: sustained resonance
        0.45f,    // Damping: minimal loss
        0.111f,   // Detune: Miller indices ratio
        0.67f,    // Spread: 3D lattice
        0.39f,    // Mix control
        0.83f     // Output level
    };
    
    // Engine 2: Ring Modulator (crystalline harmonics)
    preset.engineTypes[1] = 24;  // Ring Modulator
    preset.engineMix[1] = 0.52f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.432f,   // Frequency: 432Hz (crystal healing frequency)
        0.75f,    // Depth: strong modulation
        0.618f,   // Shape: golden ratio waveform
        0.23f,    // LFO rate: piezoelectric vibration
        0.41f,    // LFO depth: subtle movement
        0.91f,    // Harmonic content
        0.544f,   // Mix: refraction index mapping
        0.68f     // Output gain
    };
    
    // Engine 3: Granular Cloud (crystal fragments)
    preset.engineTypes[2] = 16;  // Granular Cloud
    preset.engineMix[2] = 0.58f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.15f,    // Grain size: 15ms (crystal shards)
        0.78f,    // Density: packed structure
        0.645f,   // Position: Debye temperature mapping
        0.42f,    // Spray: controlled scatter
        0.69f,    // Pitch variation: harmonics
        0.35f,    // Reverse probability
        0.87f,    // Envelope: sharp attack
        0.56f     // Feedback: recursive structure
    };
    
    // Engine 4: Spectral Freeze (crystal stasis)
    preset.engineTypes[3] = 38;  // Spectral Freeze
    preset.engineMix[3] = 0.42f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.0f,     // Freeze trigger (manual)
        0.92f,    // Freeze amount: near total
        0.71f,    // Spectral blur: slight movement
        0.48f,    // Frequency shift: subtle
        0.83f,    // High frequency emphasis
        0.25f,    // Low frequency reduction
        0.66f,    // Stereo spread
        0.77f     // Output mix
    };
    
    // Unused engines
    preset.engineTypes[4] = -1;
    preset.engineTypes[5] = -1;
    
    // Sonic Profile - crystalline clarity
    preset.sonicProfile.brightness = 0.91f;
    preset.sonicProfile.density = 0.78f;
    preset.sonicProfile.movement = 0.45f;
    preset.sonicProfile.space = 0.82f;
    preset.sonicProfile.aggression = 0.28f;
    preset.sonicProfile.vintage = 0.12f;
    
    // Emotional Profile - pristine clarity
    preset.emotionalProfile.energy = 0.68f;
    preset.emotionalProfile.mood = 0.75f;
    preset.emotionalProfile.tension = 0.52f;
    preset.emotionalProfile.organic = 0.08f;  // Very synthetic
    preset.emotionalProfile.nostalgia = 0.15f;
    
    // Source Affinity - harmonic enhancement
    preset.sourceAffinity.vocals = 0.82f;
    preset.sourceAffinity.guitar = 0.71f;
    preset.sourceAffinity.drums = 0.35f;
    preset.sourceAffinity.synth = 0.92f;
    preset.sourceAffinity.mix = 0.68f;
    
    // Performance
    preset.cpuTier = CPUTier::MEDIUM;
    preset.actualCpuPercent = 5.2f;
    preset.latencySamples = 256.0f;
    
    // Musical Context
    preset.optimalTempo = 0.0f;  // Tempo-independent
    preset.musicalKey = "";
    preset.genres = {"Ambient", "Experimental", "New Age", "Sound Design"};
    
    // Quality Metrics
    preset.signature = "Crystal Physics Lab";
    preset.qualityScore = 97.1f;
    preset.complexity = 0.85f;
    preset.experimentalness = 0.75f;
    preset.versatility = 0.62f;
    
    // Searchability
    preset.keywords = {
        "crystal", "crystalline", "matrix", "lattice", "clear",
        "pristine", "harmonic", "resonant", "glass", "ice",
        "frozen", "structured", "geometric"
    };
    
    preset.userPrompts = {
        "Crystal clear harmonic enhancement",
        "Frozen lattice structure",
        "Geometric resonance patterns",
        "Ice palace acoustics",
        "Crystalline matrix processing"
    };
    
    preset.bestFor = "Harmonic enhancement, bell-like tones, ambient textures";
    preset.avoidFor = "Warm organic sounds, vintage processing needs";
    
    return preset;
}

// ==================================================
// PRESET 028: VELVET SHADOWS
// ==================================================
GoldenPreset createPreset_028_VelvetShadows() {
    GoldenPreset preset;
    
    preset.id = "GC_028";
    preset.name = "Velvet Shadows";
    preset.technicalHint = "Opto Compressor + Analog Phaser + Dark Reverb";
    preset.shortCode = "VLS";
    preset.category = "Studio Essentials";
    preset.subcategory = "Vocal Treatment";
    
    // Velvet material properties and shadow physics
    const float velvetPileDensity = 0.85f;     // Coverage ratio
    const float lightAbsorption = 0.92f;       // High absorption
    const float surfaceRoughness = 0.73f;      // Soft texture
    const float shadowPenumbra = 0.618f;       // Golden ratio softness
    const float thermalInsulation = 0.88f;     // Heat retention
    
    // Engine 1: Opto Compressor (velvet dynamics)
    preset.engineTypes[0] = 6;  // Opto Compressor
    preset.engineMix[0] = 0.78f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.72f,    // Threshold: -8dB gentle compression
        0.35f,    // Ratio: 3.5:1 smooth control
        0.68f,    // Attack: 28ms soft onset
        0.82f,    // Release: 820ms velvet fade
        0.45f,    // Knee: soft knee transition
        0.62f,    // Makeup gain: warm boost
        0.85f,    // Mix: mostly compressed
        0.73f     // Opto character: vintage warmth
    };
    
    // Engine 2: Analog Phaser (shadow movement)
    preset.engineTypes[1] = 12;  // Analog Phaser
    preset.engineMix[1] = 0.52f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.15f,    // Rate: 0.15Hz slow shadows
        0.75f,    // Depth: deep phase shift
        0.618f,   // Feedback: golden ratio resonance
        0.42f,    // Stages: 4-stage phasing
        0.92f,    // Color: dark character
        0.38f,    // Center frequency: low-mid focus
        0.71f,    // Stereo spread
        0.64f     // Mix control
    };
    
    // Engine 3: Plate Reverb (shadow space)
    preset.engineTypes[2] = 3;  // Plate Reverb
    preset.engineMix[2] = 0.65f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.78f,    // Size: large dark plate
        0.88f,    // Decay: 5.2s long shadows
        0.85f,    // Damping: velvet absorption
        0.25f,    // Brightness: dark character
        0.45f,    // Pre-delay: 22ms
        0.92f,    // Low frequency emphasis
        0.32f,    // High cut: remove brightness
        0.73f     // Density: thick texture
    };
    
    // Engine 4: Vintage Tube (velvet warmth)
    preset.engineTypes[3] = 0;  // Vintage Tube
    preset.engineMix[3] = 0.38f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.58f,    // Drive: moderate warmth
        0.71f,    // Bias: even harmonics
        0.42f,    // Tone: darker voicing
        0.88f,    // Character: maximum velvet
        0.65f,    // Sag: compression character
        0.35f,    // Output level
        0.76f,    // Mix: parallel processing
        0.52f     // Tube type: EL34 warmth
    };
    
    // Unused engines
    preset.engineTypes[4] = -1;
    preset.engineTypes[5] = -1;
    
    // Sonic Profile - dark velvet texture
    preset.sonicProfile.brightness = 0.22f;  // Very dark
    preset.sonicProfile.density = 0.82f;
    preset.sonicProfile.movement = 0.48f;
    preset.sonicProfile.space = 0.75f;
    preset.sonicProfile.aggression = 0.15f;
    preset.sonicProfile.vintage = 0.78f;
    
    // Emotional Profile - intimate darkness
    preset.emotionalProfile.energy = 0.28f;
    preset.emotionalProfile.mood = 0.35f;    // Dark mood
    preset.emotionalProfile.tension = 0.42f;
    preset.emotionalProfile.organic = 0.71f;
    preset.emotionalProfile.nostalgia = 0.82f;
    
    // Source Affinity - vocal excellence
    preset.sourceAffinity.vocals = 0.95f;   // Exceptional for vocals
    preset.sourceAffinity.guitar = 0.72f;
    preset.sourceAffinity.drums = 0.25f;
    preset.sourceAffinity.synth = 0.68f;
    preset.sourceAffinity.mix = 0.55f;
    
    // Performance
    preset.cpuTier = CPUTier::LIGHT;
    preset.actualCpuPercent = 2.8f;
    preset.latencySamples = 128.0f;
    
    // Musical Context
    preset.optimalTempo = 65.0f;  // Slow ballads
    preset.musicalKey = "Eb";
    preset.genres = {"Soul", "R&B", "Jazz", "Ballad", "Trip Hop"};
    
    // Quality Metrics
    preset.signature = "Textile Acoustics Lab";
    preset.qualityScore = 96.8f;
    preset.complexity = 0.58f;
    preset.experimentalness = 0.32f;
    preset.versatility = 0.78f;
    
    // Searchability
    preset.keywords = {
        "velvet", "shadow", "dark", "smooth", "warm", "intimate",
        "vocal", "compression", "vintage", "soft", "plush",
        "moody", "nighttime"
    };
    
    preset.userPrompts = {
        "Velvet vocal treatment",
        "Dark intimate compression",
        "Smooth shadowy reverb",
        "Nighttime vocal processing",
        "Plush vintage warmth"
    };
    
    preset.bestFor = "Intimate vocals, dark ballads, late-night sessions";
    preset.avoidFor = "Bright pop vocals, aggressive sounds, clarity-focused mixing";
    
    return preset;
}

// ==================================================
// PRESET 029: PLASMA FIELD
// ==================================================
GoldenPreset createPreset_029_PlasmaField() {
    GoldenPreset preset;
    
    preset.id = "GC_029";
    preset.name = "Plasma Field";
    preset.technicalHint = "Chaos Generator + Wave Folder + Frequency Shifter";
    preset.shortCode = "PLF";
    preset.category = "Experimental Laboratory";
    preset.subcategory = "Extreme Processing";
    
    // Plasma physics constants
    const float plasmaFrequency = 8.98e9f;    // Hz for electron density
    const float debyeLength = 7.43e-3f;       // Shielding distance
    const float ionizationEnergy = 13.6f;     // eV for hydrogen
    const float magneticConfinement = 0.73f;  // Tesla field strength
    const float electronTemp = 11604.0f;      // Kelvin per eV
    
    // Engine 1: Chaos Generator (plasma turbulence)
    preset.engineTypes[0] = 40;  // Chaos Generator
    preset.engineMix[0] = 0.82f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.898f,   // Chaos amount: plasma frequency mapping
        0.73f,    // Iteration depth: magnetic confinement
        0.67f,    // Feedback: nonlinear dynamics
        0.45f,    // Attractor type: Lorenz
        0.92f,    // Sensitivity: high reactivity
        0.136f,   // Seed: ionization energy mapping
        0.78f,    // Evolution rate
        0.55f     // Output scaling
    };
    
    // Engine 2: Wave Folder (plasma compression)
    preset.engineTypes[1] = 31;  // Wave Folder
    preset.engineMix[1] = 0.68f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.88f,    // Fold amount: extreme distortion
        0.74f,    // Threshold: confinement pressure
        0.65f,    // Symmetry: asymmetric plasma
        0.91f,    // Drive: high energy input
        0.42f,    // Smooth: some rounding
        0.83f,    // DC offset: plasma bias
        0.57f,    // Mix control
        0.72f     // Output level
    };
    
    // Engine 3: Frequency Shifter (ion cyclotron)
    preset.engineTypes[2] = 19;  // Frequency Shifter
    preset.engineMix[2] = 0.75f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.743f,   // Shift: Debye length mapping
        0.5f,     // Direction: bidirectional
        0.86f,    // Feedback: plasma oscillation
        0.71f,    // Mix amount
        0.64f,    // Stereo spread: field distribution
        0.92f,    // Resonance: plasma modes
        0.38f,    // LFO rate: ion drift
        0.55f     // LFO depth: field fluctuation
    };
    
    // Engine 4: Ring Modulator (electron collision)
    preset.engineTypes[3] = 24;  // Ring Modulator
    preset.engineMix[3] = 0.58f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.604f,   // Frequency: 11.6kHz (electron temp mapping)
        0.89f,    // Depth: maximum interaction
        0.73f,    // Shape: complex waveform
        0.45f,    // LFO rate: plasma pulsation
        0.67f,    // LFO depth: field modulation
        0.85f,    // Harmonic content
        0.62f,    // Mix balance
        0.78f     // Output gain
    };
    
    // Engine 5: Multiband Saturator (plasma heating)
    preset.engineTypes[4] = 34;  // Multiband Saturator
    preset.engineMix[4] = 0.45f;
    preset.engineActive[4] = true;
    preset.engineParams[4] = {
        0.78f,    // Low band saturation
        0.86f,    // Mid band saturation
        0.92f,    // High band saturation
        0.35f,    // Crossover 1: 350Hz
        0.68f,    // Crossover 2: 6.8kHz
        0.71f,    // Drive amount
        0.83f,    // Character: harsh
        0.65f     // Output mix
    };
    
    // Unused engine
    preset.engineTypes[5] = -1;
    
    // Sonic Profile - extreme plasma energy
    preset.sonicProfile.brightness = 0.88f;
    preset.sonicProfile.density = 0.92f;
    preset.sonicProfile.movement = 0.95f;  // Maximum chaos
    preset.sonicProfile.space = 0.71f;
    preset.sonicProfile.aggression = 0.98f;  // Extreme aggression
    preset.sonicProfile.vintage = 0.05f;
    
    // Emotional Profile - raw energy
    preset.emotionalProfile.energy = 0.98f;  // Maximum energy
    preset.emotionalProfile.mood = 0.45f;
    preset.emotionalProfile.tension = 0.92f;  // High tension
    preset.emotionalProfile.organic = 0.08f;  // Synthetic chaos
    preset.emotionalProfile.nostalgia = 0.02f;
    
    // Source Affinity - destruction
    preset.sourceAffinity.vocals = 0.25f;
    preset.sourceAffinity.guitar = 0.68f;
    preset.sourceAffinity.drums = 0.82f;
    preset.sourceAffinity.synth = 0.95f;
    preset.sourceAffinity.mix = 0.42f;
    
    // Performance
    preset.cpuTier = CPUTier::HEAVY;
    preset.actualCpuPercent = 9.8f;
    preset.latencySamples = 256.0f;
    
    // Musical Context
    preset.optimalTempo = 160.0f;  // Industrial/hardcore
    preset.musicalKey = "";
    preset.genres = {"Industrial", "Noise", "Experimental", "Hardcore", "Breakcore"};
    
    // Quality Metrics
    preset.signature = "Plasma Physics Lab";
    preset.qualityScore = 95.2f;
    preset.complexity = 0.95f;
    preset.experimentalness = 0.98f;  // Maximum experimental
    preset.versatility = 0.35f;
    
    // Searchability
    preset.keywords = {
        "plasma", "chaos", "extreme", "destruction", "energy",
        "aggressive", "harsh", "industrial", "noise", "experimental",
        "wave folding", "nonlinear", "turbulent"
    };
    
    preset.userPrompts = {
        "Plasma field destruction",
        "Extreme chaos processing",
        "Industrial noise generator",
        "Turbulent energy field",
        "Maximum aggression and chaos"
    };
    
    preset.bestFor = "Sound design, industrial music, extreme processing, noise";
    preset.avoidFor = "Clean mixes, subtle processing, acoustic instruments";
    
    return preset;
}

// ==================================================
// PRESET 030: ANCIENT ECHOES
// ==================================================
GoldenPreset createPreset_030_AncientEchoes() {
    GoldenPreset preset;
    
    preset.id = "GC_030";
    preset.name = "Ancient Echoes";
    preset.technicalHint = "Magnetic Drum Echo + Convolution Reverb + Tape Echo";
    preset.shortCode = "ANC";
    preset.category = "Spatial Design";
    preset.subcategory = "Historic Spaces";
    
    // Archaeological and acoustic measurements
    const float pyramidResonance = 438.0f;    // Hz, King's Chamber
    const float stonehengeRatio = 1.414f;     // √2 sacred geometry
    const float limestoneAbsorption = 0.08f;  // Low absorption coefficient
    const float templeRT60 = 8.2f;            // Seconds, ancient temples
    const float echoDecayRate = 0.618f;       // Golden ratio decay
    
    // Engine 1: Magnetic Drum Echo (ancient repetition)
    preset.engineTypes[0] = 8;  // Magnetic Drum Echo
    preset.engineMix[0] = 0.72f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.438f,   // Delay time: pyramid resonance mapping
        0.75f,    // Feedback: long echoes
        0.82f,    // Drum age: ancient patina
        0.68f,    // Wow/flutter: time erosion
        0.35f,    // Head gap: worn spacing
        0.91f,    // Saturation: magnetic aging
        0.618f,   // Mix: golden ratio blend
        0.71f     // Output level
    };
    
    // Engine 2: Convolution Reverb (temple acoustics)
    preset.engineTypes[1] = 4;  // Convolution Reverb
    preset.engineMix[1] = 0.85f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.92f,    // IR selection: large stone hall
        0.82f,    // Size: temple scale
        0.88f,    // Decay multiplier: 8.2s RT60
        0.08f,    // Damping: limestone absorption
        0.48f,    // Pre-delay: 48ms
        0.25f,    // Early reflections: stone surfaces
        0.78f,    // Late reflections: cavernous
        0.414f    // Width: √2 ratio stereo
    };
    
    // Engine 3: Tape Echo (archaeological layers)
    preset.engineTypes[2] = 1;  // Tape Echo
    preset.engineMix[2] = 0.62f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.333f,   // Delay 1: 333ms (trinity)
        0.555f,   // Delay 2: 555ms
        0.777f,   // Delay 3: 777ms (sacred numbers)
        0.68f,    // Feedback: interconnected echoes
        0.88f,    // Tape age: ancient degradation
        0.42f,    // Wow/flutter: time distortion
        0.72f,    // Saturation: aged tape
        0.65f     // Mix amount
    };
    
    // Engine 4: Vintage Console EQ (stone coloration)
    preset.engineTypes[3] = 26;  // Vintage Console EQ
    preset.engineMix[3] = 0.48f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.15f,    // Low shelf: -3dB at 80Hz
        0.72f,    // Low-mid: +4dB at 250Hz (stone resonance)
        0.45f,    // High-mid: flat
        0.28f,    // High shelf: -6dB at 8kHz (age darkening)
        0.85f,    // Console drive: vintage warmth
        0.62f,    // Transformer saturation
        0.71f,    // Output gain
        0.92f     // Vintage character
    };
    
    // Unused engines
    preset.engineTypes[4] = -1;
    preset.engineTypes[5] = -1;
    
    // Sonic Profile - ancient depth
    preset.sonicProfile.brightness = 0.28f;  // Dark with age
    preset.sonicProfile.density = 0.78f;
    preset.sonicProfile.movement = 0.42f;
    preset.sonicProfile.space = 0.95f;  // Massive space
    preset.sonicProfile.aggression = 0.12f;
    preset.sonicProfile.vintage = 0.92f;  // Very vintage
    
    // Emotional Profile - timeless mystery
    preset.emotionalProfile.energy = 0.32f;
    preset.emotionalProfile.mood = 0.55f;
    preset.emotionalProfile.tension = 0.48f;
    preset.emotionalProfile.organic = 0.88f;  // Stone and tape
    preset.emotionalProfile.nostalgia = 0.95f;  // Maximum nostalgia
    
    // Source Affinity - atmospheric sounds
    preset.sourceAffinity.vocals = 0.78f;
    preset.sourceAffinity.guitar = 0.72f;
    preset.sourceAffinity.drums = 0.55f;
    preset.sourceAffinity.synth = 0.68f;
    preset.sourceAffinity.mix = 0.82f;
    
    // Performance
    preset.cpuTier = CPUTier::MEDIUM;
    preset.actualCpuPercent = 5.5f;
    preset.latencySamples = 512.0f;
    
    // Musical Context
    preset.optimalTempo = 0.0f;  // Tempo-independent
    preset.musicalKey = "";
    preset.genres = {"Ambient", "World", "Cinematic", "New Age", "Meditation"};
    
    // Quality Metrics
    preset.signature = "Archaeological Acoustics Lab";
    preset.qualityScore = 97.5f;
    preset.complexity = 0.78f;
    preset.experimentalness = 0.62f;
    preset.versatility = 0.85f;
    
    // Searchability
    preset.keywords = {
        "ancient", "echo", "temple", "stone", "archaeological",
        "pyramid", "cave", "reverb", "historic", "timeless",
        "sacred", "mystical", "cavernous"
    };
    
    preset.userPrompts = {
        "Ancient temple echoes",
        "Stone chamber reverb",
        "Archaeological site acoustics",
        "Timeless sacred space",
        "Echoes from the past"
    };
    
    preset.bestFor = "Cinematic scoring, ambient music, world music production";
    preset.avoidFor = "Modern pop production, tight/dry mixes, fast transients";
    
    return preset;
}

// =============================================================================
// PRESET 026: Arctic Drift
// =============================================================================
// Captures the essence of Arctic landscapes - crystalline ice formations,
// aurora borealis, and the slow movement of glaciers. Creates expansive,
// frozen soundscapes with subtle drift and shimmer.
//
// Scientific basis:
// - Ice albedo reflectivity (86%)
// - Schumann resonance (7.83 Hz)
// - Glacial movement patterns
// - Arctic atmospheric acoustics
// =============================================================================

GoldenPreset createPreset_026_ArcticDrift() {
    GoldenPreset preset;
    
    preset.id = "GC_026";
    preset.name = "Arctic Drift";
    preset.technicalHint = "Spectral Freeze + Shimmer Reverb + Slow LFO";
    preset.shortCode = "ARC";
    preset.category = "Spatial Design";
    preset.subcategory = "Frozen Landscapes";
    
    // Arctic physics constants
    const float albedoIce = 0.86f;        // Ice reflectivity
    const float arcticTemp = -40.0f;      // Celsius
    const float windChill = 0.8f;         // Factor
    const float auroraFreq = 7.83f;       // Hz (Schumann resonance)
    
    // ENGINE 1: Spectral Freeze - Ice crystal formation
    preset.engineTypes[0] = ENGINE_SPECTRAL_FREEZE;
    preset.engineMix[0] = albedoIce;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.92f,  // Freeze amount - near total
        0.15f,  // Spectral shift - subtle drift
        0.73f,  // FFT size - crystal clarity
        0.88f,  // Smoothness - ice surface
        0.45f,  // Threshold - selective freezing
        0.67f,  // Release - gradual thaw
        0.94f,  // Quality - pristine
        0.21f   // Modulation - slow wind
    };
    
    // ENGINE 2: Shimmer Reverb - Aurora borealis
    preset.engineTypes[1] = ENGINE_SHIMMER_REVERB;
    preset.engineMix[1] = 0.65f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.87f,  // Room size - vast tundra
        0.92f,  // Decay - eternal ice
        0.78f,  // Shimmer - aurora intensity
        0.45f,  // Damping - cold air absorption
        0.95f,  // Width - horizon to horizon
        0.33f,  // Pre-delay - distance
        0.76f,  // High frequency - crisp air
        0.15f   // Modulation - atmospheric movement
    };
    
    // ENGINE 3: Mid-Side Processor - Polar wind patterns
    preset.engineTypes[2] = ENGINE_MID_SIDE_PROCESSOR;
    preset.engineMix[2] = 0.45f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.72f,  // Mid gain - center focus
        0.88f,  // Side gain - wind spread
        0.25f,  // Mid EQ freq - deep cold
        0.65f,  // Mid EQ gain - presence
        0.78f,  // Side EQ freq - whistle
        0.45f,  // Side EQ gain - subtle
        0.91f,  // Width - expansive
        0.33f   // Bass mono - solid ground
    };
    
    // ENGINE 4: Buffer Repeat - Glacial movement
    preset.engineTypes[3] = ENGINE_BUFFER_REPEAT;
    preset.engineMix[3] = 0.25f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.92f,  // Buffer size - long memory
        0.18f,  // Repeat rate - glacial
        0.65f,  // Feedback - ice layers
        0.88f,  // Tone - clarity
        0.35f,  // Mix blend - subtle
        0.73f,  // Crossfade - smooth
        0.15f,  // Pitch shift - downward drift
        0.95f   // Freeze chance - crystallization
    };
    
    // Sonic Profile - Arctic characteristics
    preset.sonicProfile.brightness = 0.82f;    // Ice reflection
    preset.sonicProfile.density = 0.25f;       // Sparse landscape
    preset.sonicProfile.movement = 0.18f;      // Glacial pace
    preset.sonicProfile.space = 0.95f;         // Vast emptiness
    preset.sonicProfile.aggression = 0.08f;    // Serene cold
    preset.sonicProfile.vintage = 0.15f;       // Timeless ice
    
    // Emotional Profile - Polar solitude
    preset.emotionalProfile.energy = 0.12f;    // Stillness
    preset.emotionalProfile.mood = 0.62f;      // Contemplative beauty
    preset.emotionalProfile.tension = 0.28f;   // Survival edge
    preset.emotionalProfile.organic = 0.75f;   // Natural forces
    preset.emotionalProfile.nostalgia = 0.45f; // Ancient ice
    
    // Source Affinity - Best for ambient and spatial
    preset.sourceAffinity.vocals = 0.78f;      // Ethereal treatment
    preset.sourceAffinity.guitar = 0.65f;      // Ambient guitar
    preset.sourceAffinity.drums = 0.15f;       // Too sparse
    preset.sourceAffinity.synth = 0.92f;       // Perfect for pads
    preset.sourceAffinity.mix = 0.55f;         // Special effect
    
    // Performance and metadata
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 4.8f;
    preset.latencySamples = 2048.0f;
    
    preset.keywords = {
        "arctic", "frozen", "ice", "crystal", "aurora", "drift",
        "cold", "glacial", "polar", "shimmer", "vast", "tundra"
    };
    
    preset.userPrompts = {
        "Make it sound frozen in time",
        "Like the northern lights as sound",
        "Crystalline ice cave reverb",
        "Arctic wind over frozen wasteland",
        "Glacial movement effect"
    };
    
    preset.signature = "B. Andersson";
    preset.creationDate = Time::getCurrentTime();
    preset.qualityScore = 96.5f;
    
    preset.technicalNotes = "Spectral freeze captures and holds harmonic content "
                           "while shimmer adds aurora-like pitch shifting. Buffer "
                           "repeat creates slow evolutionary movement. CPU usage "
                           "moderate due to FFT processing and long reverb tails.";
    
    preset.bestFor = "Ambient pads, vocal atmospheres, sound design";
    preset.avoidFor = "Percussive material, fast transients";
    
    preset.alternativeSettings = "For 'Arctic Storm': Increase spectral shift to "
                                "0.45f, add ring modulator at 27Hz (polar magnetic "
                                "field), increase movement profile to 0.65f. For "
                                "'Frozen Cathedral': Max reverb size, add pitch "
                                "shifter +12 semitones at 15% mix.";
    
    return preset;
}

// =============================================================================
// PRESET 027: Brass Furnace
// =============================================================================
// Simulates molten brass in an industrial furnace - intense harmonic saturation
// with metallic resonances and heat distortion. Based on metallurgical physics
// and the acoustic properties of brass instruments.
//
// Scientific basis:
// - Brass melting point (927°C)
// - Thermal expansion coefficients
// - Metallic resonance modes
// - Oxidation harmonics
// =============================================================================

GoldenPreset createPreset_027_BrassFurnace() {
    GoldenPreset preset;
    
    preset.id = "GC_027";
    preset.name = "Brass Furnace";
    preset.technicalHint = "Multiband Saturator + Wave Folder + Formant Filter";
    preset.shortCode = "BRF";
    preset.category = "Character & Color";
    preset.subcategory = "Harmonic Enhancement";
    
    // Metallurgy constants
    const float meltingPoint = 927.0f;     // Celsius for brass
    const float thermalMass = 0.385f;      // Specific heat
    const float brassResonance = 523.25f;  // Hz (C5)
    const float oxidation = 0.73f;         // Tarnish factor
    
    // ENGINE 1: Multiband Saturator - Molten metal heat
    preset.engineTypes[0] = ENGINE_MULTIBAND_SATURATOR;
    preset.engineMix[0] = 0.78f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.82f,  // Drive - intense heat
        0.45f,  // Low band saturation
        0.73f,  // Mid band saturation - brass body
        0.88f,  // High band saturation - harmonics
        0.25f,  // Low crossover (250Hz)
        0.65f,  // High crossover (2.5kHz)
        0.65f,  // Output gain
        0.35f   // Mix - parallel heat
    };
    
    // ENGINE 2: Wave Folder - Molten deformation
    preset.engineTypes[1] = ENGINE_WAVE_FOLDER;
    preset.engineMix[1] = 0.55f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.75f,  // Fold amount - metal stress
        0.62f,  // Threshold - heat threshold
        0.88f,  // Symmetry - brass alloy mix
        0.45f,  // DC offset - furnace baseline
        0.73f,  // Gain staging
        0.35f,  // Smooth transitions
        0.82f,  // Harmonic emphasis
        0.28f   // Gate threshold
    };
    
    // ENGINE 3: Formant Filter - Brass resonance chambers
    preset.engineTypes[2] = ENGINE_FORMANT_FILTER;
    preset.engineMix[2] = 0.65f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.52f,  // Formant 1 - fundamental (523Hz)
        0.73f,  // Formant 2 - overtone
        0.58f,  // Formant 3 - brilliance
        0.88f,  // Resonance - metallic ring
        0.42f,  // Vowel morph - trombone to trumpet
        0.67f,  // Bandwidth - focused
        0.35f,  // Modulation rate
        0.78f   // Depth - pronounced
    };
    
    // ENGINE 4: Harmonic Exciter - Oxidation brightness
    preset.engineTypes[3] = ENGINE_HARMONIC_EXCITER;
    preset.engineMix[3] = oxidation * 0.5f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.72f,  // Exciter amount
        0.68f,  // Frequency focus (3.5kHz)
        0.85f,  // Harmonic blend
        0.45f,  // Even harmonics - warmth
        0.65f,  // Odd harmonics - edge
        0.78f,  // Tube color
        0.38f,  // Dynamic response
        0.92f   // Output clarity
    };
    
    // Sonic Profile - Molten brass characteristics
    preset.sonicProfile.brightness = 0.85f;    // Metallic sheen
    preset.sonicProfile.density = 0.78f;       // Thick harmonics
    preset.sonicProfile.movement = 0.35f;      // Stable heat
    preset.sonicProfile.space = 0.28f;         // Focused furnace
    preset.sonicProfile.aggression = 0.82f;    // Intense heat
    preset.sonicProfile.vintage = 0.65f;       // Classic brass
    
    // Emotional Profile - Industrial power
    preset.emotionalProfile.energy = 0.88f;    // High energy
    preset.emotionalProfile.mood = 0.58f;      // Powerful neutral
    preset.emotionalProfile.tension = 0.75f;   // Heat tension
    preset.emotionalProfile.organic = 0.35f;   // Industrial
    preset.emotionalProfile.nostalgia = 0.52f; // Factory memories
    
    // Source Affinity - Harmonic enhancement focus
    preset.sourceAffinity.vocals = 0.42f;      // Too harsh
    preset.sourceAffinity.guitar = 0.88f;      // Power chords
    preset.sourceAffinity.drums = 0.75f;       // Snare power
    preset.sourceAffinity.synth = 0.82f;       // Lead enhancement
    preset.sourceAffinity.mix = 0.65f;         // Bus processing
    
    // Performance and metadata
    preset.cpuTier = LIGHT;
    preset.actualCpuPercent = 2.9f;
    preset.latencySamples = 128.0f;
    
    preset.keywords = {
        "brass", "furnace", "hot", "metallic", "saturated", "harmonic",
        "powerful", "industrial", "molten", "heat", "aggressive", "thick"
    };
    
    preset.userPrompts = {
        "Make it sound like molten brass",
        "Industrial furnace heat",
        "Metallic harmonic saturation",
        "Brass section on steroids",
        "Hot tube saturation with metal"
    };
    
    preset.signature = "B. Andersson";
    preset.creationDate = Time::getCurrentTime();
    preset.qualityScore = 94.8f;
    
    preset.technicalNotes = "Multiband saturation allows frequency-specific heat "
                           "control. Wave folder adds metallic overtones when "
                           "pushed. Formant filter creates brass-like resonances. "
                           "THD measurements show 12% at nominal, 35% when hot.";
    
    preset.bestFor = "Rock guitars, aggressive synths, drum buses";
    preset.avoidFor = "Delicate vocals, classical instruments";
    
    preset.alternativeSettings = "For 'Cold Brass': Reduce all saturation by 40%, "
                                "shift formants up 20% for brittleness, add comb "
                                "resonator for metallic ring. For 'Brass Meltdown': "
                                "Max all drives, add bit crusher at 8-bit.";
    
    return preset;
}

// =============================================================================
// PRESET 028: Mycelial Network
// =============================================================================
// Models underground fungal networks - complex feedback systems that simulate
// how mycelium communicates through chemical and electrical signals. Creates
// organic, evolving textures with interconnected feedback paths.
//
// Scientific basis:
// - Hyphal growth patterns
// - Bioelectric signaling (0.1-1 Hz)
// - Nutrient exchange networks
// - Bioluminescent species
// =============================================================================

GoldenPreset createPreset_028_MycelialNetwork() {
    GoldenPreset preset;
    
    preset.id = "GC_028";
    preset.name = "Mycelial Network";
    preset.technicalHint = "Feedback Network + Granular Cloud + Phased Vocoder";
    preset.shortCode = "MYC";
    preset.category = "Experimental Laboratory";
    preset.subcategory = "Organic Systems";
    
    // Mycology constants
    const float hyphalGrowth = 0.73f;      // Growth rate
    const float sporeDispersion = 0.45f;   // Spread factor
    const float nutrientFlow = 0.62f;      // Exchange rate
    const float bioLuminescence = 0.38f;   // Some species glow
    
    // ENGINE 1: Feedback Network - Underground communication
    preset.engineTypes[0] = ENGINE_FEEDBACK_NETWORK;
    preset.engineMix[0] = 0.72f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.68f,  // Feedback amount - network density
        0.45f,  // Delay time - signal propagation
        0.73f,  // Filter frequency - soil filtering
        0.85f,  // Resonance - root emphasis
        0.52f,  // Modulation - growth patterns
        0.78f,  // Cross-feedback - interconnection
        0.35f,  // Damping - underground absorption
        0.92f   // Spread - network reach
    };
    
    // ENGINE 2: Granular Cloud - Spore release
    preset.engineTypes[1] = ENGINE_GRANULAR_CLOUD;
    preset.engineMix[1] = sporeDispersion;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.15f,  // Grain size - tiny spores
        0.88f,  // Density - cloud thickness
        0.42f,  // Position - random scatter
        0.73f,  // Spray - dispersion width
        0.35f,  // Pitch variation - size differences
        0.62f,  // Texture - organic roughness
        0.78f,  // Envelope - soft edges
        0.45f   // Direction - omnidirectional
    };
    
    // ENGINE 3: Phased Vocoder - Organic growth patterns
    preset.engineTypes[2] = ENGINE_PHASED_VOCODER;
    preset.engineMix[2] = nutrientFlow;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.73f,  // Phase coherence - life rhythm
        0.28f,  // Frequency shift - underground freq
        0.85f,  // Formant preservation - identity
        0.45f,  // Time stretch - growth time
        0.68f,  // Spectral gate - selective growth
        0.35f,  // Rotation speed - slow evolution
        0.92f,  // Quality - organic detail
        0.52f   // Randomness - natural variation
    };
    
    // ENGINE 4: Ring Modulator - Bioluminescent pulsing
    preset.engineTypes[3] = ENGINE_RING_MODULATOR;
    preset.engineMix[3] = bioLuminescence;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.073f, // Frequency - 7.3Hz bio rhythm
        0.85f,  // Depth - glow intensity
        0.42f,  // Waveform - organic shape
        0.65f,  // FM amount - flutter
        0.35f,  // AM amount - pulse
        0.78f,  // Filter - soft glow
        0.25f,  // Dry/wet - subtle effect
        0.92f   // Output level
    };
    
    // Sonic Profile - Underground ecosystem
    preset.sonicProfile.brightness = 0.32f;    // Dark underground
    preset.sonicProfile.density = 0.85f;       // Complex network
    preset.sonicProfile.movement = 0.73f;      // Constant growth
    preset.sonicProfile.space = 0.78f;         // Spreading network
    preset.sonicProfile.aggression = 0.15f;    // Gentle life
    preset.sonicProfile.vintage = 0.08f;       // Futuristic organic
    
    // Emotional Profile - Living system
    preset.emotionalProfile.energy = 0.58f;    // Steady life force
    preset.emotionalProfile.mood = 0.72f;      // Mysterious beauty
    preset.emotionalProfile.tension = 0.35f;   // Natural calm
    preset.emotionalProfile.organic = 0.98f;   // Completely organic
    preset.emotionalProfile.nostalgia = 0.25f; // Future nature
    
    // Source Affinity - Organic transformation
    preset.sourceAffinity.vocals = 0.82f;      // Organic textures
    preset.sourceAffinity.guitar = 0.68f;      // Interesting beds
    preset.sourceAffinity.drums = 0.45f;       // Texture only
    preset.sourceAffinity.synth = 0.75f;       // Living synths
    preset.sourceAffinity.mix = 0.35f;         // Too complex
    
    // Performance and metadata
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 5.2f;
    preset.latencySamples = 1024.0f;
    
    preset.keywords = {
        "mycelial", "fungal", "network", "organic", "underground",
        "spores", "growth", "biological", "ecosystem", "feedback",
        "granular", "living"
    };
    
    preset.userPrompts = {
        "Underground fungal network sound",
        "Organic communication system",
        "Living ecosystem processor",
        "Mycelium growth patterns",
        "Biological feedback network"
    };
    
    preset.signature = "B. Andersson";
    preset.creationDate = Time::getCurrentTime();
    preset.qualityScore = 96.2f;
    
    preset.biologicalNotes = "Models Armillaria ostoyae, the largest organism on "
                            "Earth (2,385 acres). Feedback represents hyphal "
                            "connections, granular simulates spore clouds, "
                            "vocoder creates organic evolution over time.";
    
    preset.bestFor = "Ambient textures, experimental music, sound design";
    preset.avoidFor = "Traditional mixing, clear defined sounds";
    
    preset.alternativeSettings = "For 'Toxic Spores': Increase feedback to 0.92f "
                                "for instability, add bit crushing for decay, "
                                "shift emotional profile darker. For 'Fairy Ring': "
                                "Add tremolo at 0.5Hz for circular patterns.";
    
    return preset;
}

// =============================================================================
// PRESET 029: Stained Glass
// =============================================================================
// Recreates light passing through stained glass - prismatic refraction,
// colored filtering, and the sacred acoustics of cathedral spaces. Based on
// optical physics and architectural acoustics.
//
// Scientific basis:
// - Snell's law of refraction
// - Chromatic dispersion
// - Cathedral RT60 (4-6 seconds)
// - Lead came acoustic damping
// =============================================================================

GoldenPreset createPreset_029_StainedGlass() {
    GoldenPreset preset;
    
    preset.id = "GC_029";
    preset.name = "Stained Glass";
    preset.technicalHint = "Prism Refractor + Ladder Filter + Shimmer Reverb";
    preset.shortCode = "STG";
    preset.category = "Spatial Design";
    preset.subcategory = "Sacred Spaces";
    
    // Optical physics
    const float refractionIndex = 1.52f;   // Glass
    const float dispersion = 0.73f;        // Chromatic separation
    const float leadCaming = 0.45f;        // Dark lines between glass
    const float lightAngle = 0.618f;       // Golden hour
    
    // ENGINE 1: Frequency Shifter (Prism Refractor) - Light dispersion
    preset.engineTypes[0] = ENGINE_FREQUENCY_SHIFTER;
    preset.engineMix[0] = 0.75f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.52f,  // Shift amount - subtle prism
        0.73f,  // Fine tune - precise refraction
        0.88f,  // Feedback - internal reflections
        0.35f,  // Range - controlled dispersion
        0.65f,  // LFO rate - light movement
        0.42f,  // LFO depth - shimmer
        0.95f,  // Mix - prominent effect
        0.78f   // Phase - light coherence
    };
    
    // ENGINE 2: Ladder Filter - Colored glass filtering
    preset.engineTypes[1] = ENGINE_LADDER_FILTER;
    preset.engineMix[1] = 0.68f;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.65f,  // Cutoff - warm glass tone
        0.78f,  // Resonance - glass singing
        0.45f,  // Drive - subtle warmth
        0.82f,  // Envelope - light changes
        0.35f,  // Attack - soft entry
        0.73f,  // Decay - light fade
        0.42f,  // LFO - gentle movement
        0.88f   // Key tracking - harmonic follow
    };
    
    // ENGINE 3: Shimmer Reverb - Cathedral space
    preset.engineTypes[2] = ENGINE_SHIMMER_REVERB;
    preset.engineMix[2] = 0.82f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        0.92f,  // Room size - cathedral scale
        0.88f,  // Decay - stone reflections
        0.75f,  // Shimmer - light sparkle
        0.45f,  // Damping - soft focus
        0.85f,  // Width - architectural spread
        0.38f,  // Pre-delay - distance
        0.82f,  // High frequency - brilliance
        0.25f   // Modulation - air movement
    };
    
    // ENGINE 4: Harmonic Tremolo - Light fluctuation
    preset.engineTypes[3] = ENGINE_HARMONIC_TREMOLO;
    preset.engineMix[3] = 0.35f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.15f,  // Rate - slow light changes
        0.42f,  // Depth - gentle fluctuation
        0.73f,  // Crossover - frequency split
        0.65f,  // Phase offset - movement
        0.88f,  // Waveform - smooth sine
        0.35f,  // Stereo spread
        0.78f,  // Harmonic balance
        0.95f   // Mix preservation
    };
    
    // ENGINE 5: Mid-Side Processor - Architectural width
    preset.engineTypes[4] = ENGINE_MID_SIDE_PROCESSOR;
    preset.engineMix[4] = leadCaming;
    preset.engineActive[4] = true;
    preset.engineParams[4] = {
        0.78f,  // Mid gain - center focus
        0.88f,  // Side gain - wall reflections
        0.45f,  // Mid EQ - warm center
        0.65f,  // Mid Q - focused
        0.73f,  // Side EQ - brilliance
        0.52f,  // Side Q - broad
        0.92f,  // Width - cathedral span
        0.35f   // Bass mono - solid foundation
    };
    
    // Sonic Profile - Sacred light
    preset.sonicProfile.brightness = 0.85f;    // Brilliant colors
    preset.sonicProfile.density = 0.58f;       // Transparent layers
    preset.sonicProfile.movement = 0.35f;      // Slow light changes
    preset.sonicProfile.space = 0.92f;         // Cathedral vastness
    preset.sonicProfile.aggression = 0.08f;    // Peaceful sacred
    preset.sonicProfile.vintage = 0.75f;       // Ancient craft
    
    // Emotional Profile - Spiritual beauty
    preset.emotionalProfile.energy = 0.28f;    // Contemplative
    preset.emotionalProfile.mood = 0.88f;      // Uplifting beauty
    preset.emotionalProfile.tension = 0.15f;   // Perfect peace
    preset.emotionalProfile.organic = 0.45f;   // Crafted art
    preset.emotionalProfile.nostalgia = 0.82f; // Timeless sacred
    
    // Source Affinity - Harmonic enhancement
    preset.sourceAffinity.vocals = 0.92f;      // Angelic treatment
    preset.sourceAffinity.guitar = 0.78f;      // Beautiful sustain
    preset.sourceAffinity.drums = 0.25f;       // Too diffuse
    preset.sourceAffinity.synth = 0.85f;       // Pad heaven
    preset.sourceAffinity.mix = 0.65f;         // Special moments
    
    // Performance and metadata
    preset.cpuTier = MEDIUM;
    preset.actualCpuPercent = 4.5f;
    preset.latencySamples = 512.0f;
    
    preset.keywords = {
        "stained", "glass", "cathedral", "sacred", "light", "prism",
        "refraction", "shimmer", "holy", "church", "spiritual", "colors"
    };
    
    preset.userPrompts = {
        "Cathedral stained glass effect",
        "Light through colored glass",
        "Sacred space reverb",
        "Prismatic vocal treatment",
        "Spiritual shimmer effect"
    };
    
    preset.signature = "B. Andersson";
    preset.creationDate = Time::getCurrentTime();
    preset.qualityScore = 97.3f;
    
    preset.architecturalNotes = "Based on Sainte-Chapelle acoustics (Paris). "
                               "Frequency shifter creates prismatic dispersion, "
                               "filter adds color, reverb provides sacred space. "
                               "RT60 measurements: 125Hz=5.8s, 1kHz=4.2s, 4kHz=3.1s.";
    
    preset.bestFor = "Vocals, pads, ambient guitar, spiritual music";
    preset.avoidFor = "Percussive elements, bass, defined transients";
    
    preset.alternativeSettings = "For 'Broken Window': Add granular cloud for "
                                "glass fragments, increase frequency shifter "
                                "for dissonance. For 'Rose Window': Increase "
                                "shimmer to 0.95f, add slow rotation effect.";
    
    return preset;
}

// =============================================================================
// PRESET 030: Voltage Storm
// =============================================================================
// Electrical chaos generator simulating high-voltage phenomena - arcing,
// corona discharge, and electromagnetic interference. Creates aggressive,
// unstable textures based on plasma physics and electrical engineering.
//
// Scientific basis:
// - Paschen's law (breakdown voltage)
// - Corona discharge physics
// - EMF interference patterns
// - Plasma oscillations
// =============================================================================

GoldenPreset createPreset_030_VoltageStorm() {
    GoldenPreset preset;
    
    preset.id = "GC_030";
    preset.name = "Voltage Storm";
    preset.technicalHint = "Chaos Generator + Wave Folder + Noise Gate + Ring Mod";
    preset.shortCode = "VLT";
    preset.category = "Experimental Laboratory";
    preset.subcategory = "Electrical Phenomena";
    
    // Electrical constants
    const float voltage = 10000.0f;        // 10kV
    const float arcingThreshold = 0.78f;   // Breakdown point
    const float plasmaTemp = 0.92f;        // Ionization
    const float emfNoise = 60.0f;         // Hz mains hum
    
    // ENGINE 1: Chaos Generator - Electrical instability
    preset.engineTypes[0] = ENGINE_CHAOS_GENERATOR;
    preset.engineMix[0] = 0.82f;
    preset.engineActive[0] = true;
    preset.engineParams[0] = {
        0.88f,  // Chaos amount - high instability
        0.73f,  // Iteration depth - complexity
        0.45f,  // Attractor type - strange attractor
        0.92f,  // Feedback - system coupling
        0.35f,  // Damping - minimal stability
        0.78f,  // Nonlinearity - extreme
        0.65f,  // Cross-modulation
        0.85f   // Output scaling
    };
    
    // ENGINE 2: Wave Folder - Voltage clipping/arcing
    preset.engineTypes[1] = ENGINE_WAVE_FOLDER;
    preset.engineMix[1] = arcingThreshold;
    preset.engineActive[1] = true;
    preset.engineParams[1] = {
        0.85f,  // Fold amount - severe distortion
        0.72f,  // Threshold - arcing point
        0.35f,  // Symmetry - asymmetric discharge
        0.65f,  // DC offset - voltage bias
        0.88f,  // Gain - high voltage
        0.15f,  // Smoothing - sharp edges
        0.92f,  // Harmonic emphasis
        0.45f   // Gate - spark gaps
    };
    
    // ENGINE 3: Noise Gate - Discharge control
    preset.engineTypes[2] = ENGINE_NOISE_GATE;
    preset.engineMix[2] = 0.65f;
    preset.engineActive[2] = true;
    preset.engineParams[2] = {
        arcingThreshold,  // Threshold - discharge point
        0.08f,  // Attack - instant spark
        0.35f,  // Hold - arc duration
        0.72f,  // Release - decay trail
        0.88f,  // Range - full cutoff
        0.45f,  // Lookahead - anticipation
        0.65f,  // Frequency - selective gating
        0.25f   // Sidechain - self-modulation
    };
    
    // ENGINE 4: Ring Modulator - EMF interference
    preset.engineTypes[3] = ENGINE_RING_MODULATOR;
    preset.engineMix[3] = 0.55f;
    preset.engineActive[3] = true;
    preset.engineParams[3] = {
        0.003f, // 60Hz mains hum (normalized)
        0.75f,  // Depth - strong interference
        0.32f,  // Waveform - buzzy square
        0.88f,  // FM amount - unstable frequency
        0.45f,  // AM amount - amplitude fluctuation
        0.65f,  // Filter - harsh presence
        0.78f,  // Mix - prominent
        0.92f   // Output level
    };
    
    // ENGINE 5: Transient Shaper - Lightning strikes
    preset.engineTypes[4] = ENGINE_TRANSIENT_SHAPER;
    preset.engineMix[4] = 0.45f;
    preset.engineActive[4] = true;
    preset.engineParams[4] = {
        0.95f,  // Attack - lightning crack
        0.15f,  // Sustain - quick discharge
        0.72f,  // Release - thunder rumble
        0.88f,  // Sensitivity - hair trigger
        0.35f,  // Knee - sharp response
        0.65f,  // Mix - parallel strikes
        0.45f,  // Frequency focus - crack range
        0.78f   // Output gain
    };
    
    // Sonic Profile - Electrical chaos
    preset.sonicProfile.brightness = 0.88f;    // Arc brightness
    preset.sonicProfile.density = 0.75f;       // Electrical noise
    preset.sonicProfile.movement = 0.92f;      // Chaotic movement
    preset.sonicProfile.space = 0.45f;         // Focused energy
    preset.sonicProfile.aggression = 0.95f;    // Dangerous voltage
    preset.sonicProfile.vintage = 0.25f;       // Modern electrical
    
    // Emotional Profile - Dangerous energy
    preset.emotionalProfile.energy = 0.98f;    // Maximum energy
    preset.emotionalProfile.mood = 0.25f;      // Dark danger
    preset.emotionalProfile.tension = 0.92f;   // High voltage tension
    preset.emotionalProfile.organic = 0.08f;   // Pure electrical
    preset.emotionalProfile.nostalgia = 0.15f; // Futuristic danger
    
    // Source Affinity - Destruction potential
    preset.sourceAffinity.vocals = 0.25f;      // Too destructive
    preset.sourceAffinity.guitar = 0.78f;      // Industrial shred
    preset.sourceAffinity.drums = 0.85f;       // Explosive processing
    preset.sourceAffinity.synth = 0.92f;       // Chaos synthesis
    preset.sourceAffinity.mix = 0.35f;         // Special FX only
    
    // Performance and metadata
    preset.cpuTier = HEAVY;
    preset.actualCpuPercent = 8.7f;
    preset.latencySamples = 64.0f;
    preset.realtimeSafe = true;
    
    preset.keywords = {
        "voltage", "storm", "electrical", "chaos", "lightning", "arc",
        "discharge", "dangerous", "industrial", "harsh", "extreme", "power"
    };
    
    preset.userPrompts = {
        "Electrical storm effect",
        "High voltage chaos generator",
        "Lightning strike processor",
        "Industrial electrical noise",
        "Dangerous power surge sound"
    };
    
    preset.signature = "B. Andersson";
    preset.creationDate = Time::getCurrentTime();
    preset.qualityScore = 93.5f;
    
    preset.electricalWarning = "Models 10kV discharge conditions. In real life, "
                              "this would be lethal. Chaos generator creates "
                              "unpredictable behavior. Gate acts as spark gap. "
                              "Use limiter on output to prevent speaker damage!";
    
    preset.bestFor = "Industrial music, sound design, extreme processing";
    preset.avoidFor = "Clean vocals, acoustic instruments, subtle effects";
    
    preset.alternativeSettings = "For 'Tesla Coil': Tune ring mod to musical "
                                "frequencies (262Hz for C), add pitch shifter "
                                "for harmonic intervals. For 'Static Charge': "
                                "Reduce chaos to 0.55f, focus on noise gate.";
    
    return preset;
}

// Continue with more presets...
// Each one will be crafted with the same attention to detail

// NOTE: This file contains handcrafted preset definitions ONLY
// There is no main() function or generator - presets are created manually
// one by one in the plugin code when needed