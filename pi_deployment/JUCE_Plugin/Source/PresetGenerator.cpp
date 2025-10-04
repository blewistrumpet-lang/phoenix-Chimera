#include "PresetGenerator.h"
#include <random>

PresetGenerator::PresetGenerator() {
    // Initialize metadata for parameter generation
}

std::vector<GoldenPreset> PresetGenerator::generateReferencePresets() {
    std::vector<GoldenPreset> presets;
    
    // 1. Velvet Thunder - Warm vintage character
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::VelvetThunder::name;
        preset.category = PresetTemplates::VelvetThunder::category;
        preset.subcategory = PresetTemplates::VelvetThunder::subcategory;
        preset.technicalHint = PresetTemplates::VelvetThunder::technicalHint;
        preset.shortCode = "VTH-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::VelvetThunder::engines[i];
            preset.engineMix[i] = PresetTemplates::VelvetThunder::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Tube Preamp (warm, not too driven)
        preset.engineParams[0] = {0.4f, 0.6f, 0.5f, 0.7f, 0.3f, 0.5f, 0.5f, 0.5f}; // Drive, Bias, Tone, etc.
        // Parameters - Tape Echo (vintage spacing)
        preset.engineParams[1] = {0.375f, 0.4f, 0.6f, 0.3f, 0.5f, 0.7f, 0.5f, 0.5f}; // Time, Feedback, Tone, etc.
        
        // Sonic profile
        setSonicProfile(preset, 0.3f, 0.6f, 0.4f, 0.5f, 0.2f, 0.8f);
        // Emotional profile  
        setEmotionalProfile(preset, 0.4f, 0.6f, 0.3f, 0.9f, 0.8f);
        // Source affinity
        setSourceAffinity(preset, 0.8f, 0.9f, 0.4f, 0.7f, 0.6f);
        
        // Metadata
        preset.keywords = {"warm", "vintage", "tube", "analog", "character", "classic", "smooth"};
        preset.userPrompts = {"Make it warm and vintage", "Add tube warmth", "Classic studio sound"};
        preset.antiFeatures = {"digital", "harsh", "cold", "sterile"};
        
        // Performance
        preset.cpuTier = CPUTier::LIGHT;
        preset.actualCpuPercent = 2.5f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.3f;
        preset.experimentalness = 0.2f;
        preset.versatility = 0.8f;
        
        preset.bestFor = "Vocals, Guitars, Keys";
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 2. Crystal Palace - Ethereal shimmer space
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::CrystalPalace::name;
        preset.category = PresetTemplates::CrystalPalace::category;
        preset.subcategory = PresetTemplates::CrystalPalace::subcategory;
        preset.technicalHint = PresetTemplates::CrystalPalace::technicalHint;
        preset.shortCode = "CPL-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::CrystalPalace::engines[i];
            preset.engineMix[i] = PresetTemplates::CrystalPalace::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Shimmer Reverb (large, bright)
        preset.engineParams[0] = {0.8f, 0.7f, 0.6f, 0.7f, 0.4f, 0.8f, 0.5f, 0.5f};
        // Parameters - Dimension Expander
        preset.engineParams[1] = {0.7f, 0.6f, 0.5f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.8f, 0.4f, 0.3f, 0.9f, 0.1f, 0.2f);
        // Emotional profile
        setEmotionalProfile(preset, 0.3f, 0.8f, 0.2f, 0.4f, 0.6f);
        // Source affinity
        setSourceAffinity(preset, 0.9f, 0.7f, 0.3f, 0.8f, 0.5f);
        
        // Metadata
        preset.keywords = {"ethereal", "shimmer", "space", "ambient", "dreamy", "floating", "celestial"};
        preset.userPrompts = {"Make it ethereal", "Add shimmer and space", "Celestial atmosphere"};
        preset.antiFeatures = {"dry", "tight", "aggressive", "dark"};
        
        // Performance
        preset.cpuTier = CPUTier::MEDIUM;
        preset.actualCpuPercent = 5.5f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.4f;
        preset.experimentalness = 0.5f;
        preset.versatility = 0.7f;
        
        preset.bestFor = "Vocals, Pads, Ambient";
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 3. Broken Radio - Lo-fi character
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::BrokenRadio::name;
        preset.category = PresetTemplates::BrokenRadio::category;
        preset.subcategory = PresetTemplates::BrokenRadio::subcategory;
        preset.technicalHint = PresetTemplates::BrokenRadio::technicalHint;
        preset.shortCode = "BRD-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::BrokenRadio::engines[i];
            preset.engineMix[i] = PresetTemplates::BrokenRadio::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Bit Crusher (medium degradation)
        preset.engineParams[0] = {0.6f, 0.5f, 0.7f, 0.4f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Ladder Filter (bandpass character)
        preset.engineParams[1] = {0.4f, 0.6f, 0.3f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Spring Reverb (small, metallic)
        preset.engineParams[2] = {0.3f, 0.5f, 0.6f, 0.4f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.2f, 0.7f, 0.5f, 0.4f, 0.3f, 0.7f);
        // Emotional profile
        setEmotionalProfile(preset, 0.5f, 0.4f, 0.6f, 0.3f, 0.9f);
        // Source affinity
        setSourceAffinity(preset, 0.7f, 0.8f, 0.9f, 0.6f, 0.7f);
        
        // Metadata
        preset.keywords = {"lofi", "broken", "vintage", "radio", "degraded", "nostalgic", "gritty"};
        preset.userPrompts = {"Make it lo-fi", "Broken vintage radio", "Degraded nostalgic sound"};
        preset.antiFeatures = {"clean", "pristine", "modern", "hifi"};
        
        // Performance
        preset.cpuTier = CPUTier::LIGHT;
        preset.actualCpuPercent = 2.8f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.5f;
        preset.experimentalness = 0.6f;
        preset.versatility = 0.8f;
        
        preset.bestFor = "Drums, Vocals, Full Mix";
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 4. Pulse Engine - Rhythmic movement
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::PulseEngine::name;
        preset.category = PresetTemplates::PulseEngine::category;
        preset.subcategory = PresetTemplates::PulseEngine::subcategory;
        preset.technicalHint = PresetTemplates::PulseEngine::technicalHint;
        preset.shortCode = "PLS-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::PulseEngine::engines[i];
            preset.engineMix[i] = PresetTemplates::PulseEngine::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Harmonic Tremolo (rhythmic, musical)
        preset.engineParams[0] = {0.4f, 0.7f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Analog Phaser (slow sweep)
        preset.engineParams[1] = {0.3f, 0.6f, 0.5f, 0.7f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Digital Delay (dotted eighth)
        preset.engineParams[2] = {0.375f, 0.4f, 0.5f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.5f, 0.6f, 0.8f, 0.6f, 0.3f, 0.4f);
        // Emotional profile
        setEmotionalProfile(preset, 0.7f, 0.6f, 0.5f, 0.6f, 0.3f);
        // Source affinity
        setSourceAffinity(preset, 0.7f, 0.9f, 0.8f, 0.8f, 0.6f);
        
        // Metadata
        preset.keywords = {"rhythmic", "pulse", "movement", "tremolo", "phaser", "modulation", "groovy"};
        preset.userPrompts = {"Add rhythmic movement", "Make it pulse and breathe", "Groovy modulation"};
        preset.antiFeatures = {"static", "flat", "still"};
        
        // Performance
        preset.cpuTier = CPUTier::MEDIUM;
        preset.actualCpuPercent = 4.2f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.6f;
        preset.experimentalness = 0.4f;
        preset.versatility = 0.8f;
        
        preset.bestFor = "Guitars, Keys, Synths";
        preset.optimalTempo = 120.0f;
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 5. Gravity Well - Experimental feedback system
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::GravityWell::name;
        preset.category = PresetTemplates::GravityWell::category;
        preset.subcategory = PresetTemplates::GravityWell::subcategory;
        preset.technicalHint = PresetTemplates::GravityWell::technicalHint;
        preset.shortCode = "GRV-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::GravityWell::engines[i];
            preset.engineMix[i] = PresetTemplates::GravityWell::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Feedback Network (controlled chaos)
        preset.engineParams[0] = {0.6f, 0.5f, 0.7f, 0.4f, 0.6f, 0.5f, 0.5f, 0.5f};
        // Parameters - Spectral Freeze (partial freeze)
        preset.engineParams[1] = {0.5f, 0.6f, 0.7f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Ring Modulator (low frequency)
        preset.engineParams[2] = {0.2f, 0.5f, 0.4f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.4f, 0.8f, 0.7f, 0.7f, 0.6f, 0.3f);
        // Emotional profile
        setEmotionalProfile(preset, 0.6f, 0.3f, 0.8f, 0.2f, 0.4f);
        // Source affinity
        setSourceAffinity(preset, 0.5f, 0.7f, 0.8f, 0.9f, 0.6f);
        
        // Metadata
        preset.keywords = {"experimental", "feedback", "chaos", "spectral", "abstract", "sounddesign", "evolving"};
        preset.userPrompts = {"Create abstract textures", "Experimental feedback", "Sound design tool"};
        preset.antiFeatures = {"traditional", "clean", "predictable"};
        
        // Performance
        preset.cpuTier = CPUTier::HEAVY;
        preset.actualCpuPercent = 9.5f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.8f;
        preset.experimentalness = 0.9f;
        preset.versatility = 0.5f;
        
        preset.bestFor = "Sound Design, Synths, Experimental";
        preset.genres = {"experimental", "ambient", "idm"};
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 6. Console 73 - Classic mixing chain
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::Console73::name;
        preset.category = PresetTemplates::Console73::category;
        preset.subcategory = PresetTemplates::Console73::subcategory;
        preset.technicalHint = PresetTemplates::Console73::technicalHint;
        preset.shortCode = "C73-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::Console73::engines[i];
            preset.engineMix[i] = PresetTemplates::Console73::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Vintage Console EQ (subtle enhancement)
        preset.engineParams[0] = {0.55f, 0.5f, 0.5f, 0.52f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Opto Compressor (gentle glue)
        preset.engineParams[1] = {0.4f, 0.3f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - K-Style Overdrive (just a touch)
        preset.engineParams[2] = {0.2f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.6f, 0.5f, 0.2f, 0.3f, 0.2f, 0.7f);
        // Emotional profile
        setEmotionalProfile(preset, 0.5f, 0.6f, 0.3f, 0.8f, 0.7f);
        // Source affinity
        setSourceAffinity(preset, 0.9f, 0.8f, 0.7f, 0.7f, 0.9f);
        
        // Metadata
        preset.keywords = {"console", "vintage", "mixing", "channel", "classic", "professional", "studio"};
        preset.userPrompts = {"Classic console sound", "Professional mixing chain", "Studio channel strip"};
        preset.antiFeatures = {"extreme", "experimental", "lofi"};
        
        // Performance
        preset.cpuTier = CPUTier::LIGHT;
        preset.actualCpuPercent = 2.2f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.5f;
        preset.experimentalness = 0.1f;
        preset.versatility = 0.9f;
        
        preset.bestFor = "Everything - Universal mixing tool";
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 7. Infinite Cathedral - Massive impossible space
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::InfiniteCathedral::name;
        preset.category = PresetTemplates::InfiniteCathedral::category;
        preset.subcategory = PresetTemplates::InfiniteCathedral::subcategory;
        preset.technicalHint = PresetTemplates::InfiniteCathedral::technicalHint;
        preset.shortCode = "INC-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::InfiniteCathedral::engines[i];
            preset.engineMix[i] = PresetTemplates::InfiniteCathedral::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Convolution Reverb (massive hall)
        preset.engineParams[0] = {0.9f, 0.8f, 0.7f, 0.6f, 0.7f, 0.5f, 0.5f, 0.5f};
        // Parameters - Pitch Shifter (octave up, subtle)
        preset.engineParams[1] = {0.75f, 0.3f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.7f, 0.3f, 0.2f, 1.0f, 0.1f, 0.3f);
        // Emotional profile
        setEmotionalProfile(preset, 0.4f, 0.7f, 0.6f, 0.5f, 0.6f);
        // Source affinity
        setSourceAffinity(preset, 0.9f, 0.7f, 0.4f, 0.8f, 0.5f);
        
        // Metadata
        preset.keywords = {"cathedral", "massive", "infinite", "space", "reverb", "epic", "cinematic"};
        preset.userPrompts = {"Massive cathedral space", "Infinite reverb", "Epic cinematic space"};
        preset.antiFeatures = {"small", "tight", "dry", "intimate"};
        
        // Performance
        preset.cpuTier = CPUTier::MEDIUM;
        preset.actualCpuPercent = 7.5f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.4f;
        preset.experimentalness = 0.6f;
        preset.versatility = 0.6f;
        
        preset.bestFor = "Vocals, Orchestral, Cinematic";
        preset.genres = {"cinematic", "ambient", "orchestral"};
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 8. Analog Sunrise - Warm enhancement
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::AnalogSunrise::name;
        preset.category = PresetTemplates::AnalogSunrise::category;
        preset.subcategory = PresetTemplates::AnalogSunrise::subcategory;
        preset.technicalHint = PresetTemplates::AnalogSunrise::technicalHint;
        preset.shortCode = "ASR-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::AnalogSunrise::engines[i];
            preset.engineMix[i] = PresetTemplates::AnalogSunrise::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Harmonic Exciter (gentle enhancement)
        preset.engineParams[0] = {0.4f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Stereo Chorus (warm width)
        preset.engineParams[1] = {0.3f, 0.5f, 0.6f, 0.7f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Vintage Tube (subtle warmth)
        preset.engineParams[2] = {0.3f, 0.5f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.7f, 0.6f, 0.4f, 0.6f, 0.2f, 0.6f);
        // Emotional profile
        setEmotionalProfile(preset, 0.6f, 0.8f, 0.2f, 0.8f, 0.5f);
        // Source affinity
        setSourceAffinity(preset, 0.8f, 0.9f, 0.5f, 0.8f, 0.7f);
        
        // Metadata
        preset.keywords = {"warm", "analog", "enhancement", "chorus", "exciter", "smooth", "lush"};
        preset.userPrompts = {"Warm analog enhancement", "Add lush warmth", "Smooth enhancement"};
        preset.antiFeatures = {"cold", "digital", "harsh"};
        
        // Performance
        preset.cpuTier = CPUTier::LIGHT;
        preset.actualCpuPercent = 2.8f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.5f;
        preset.experimentalness = 0.3f;
        preset.versatility = 0.8f;
        
        preset.bestFor = "Vocals, Guitars, Mix Bus";
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 9. Tidal Flow - Organic envelope movement
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::TidalFlow::name;
        preset.category = PresetTemplates::TidalFlow::category;
        preset.subcategory = PresetTemplates::TidalFlow::subcategory;
        preset.technicalHint = PresetTemplates::TidalFlow::technicalHint;
        preset.shortCode = "TDF-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::TidalFlow::engines[i];
            preset.engineMix[i] = PresetTemplates::TidalFlow::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Envelope Filter (responsive, musical)
        preset.engineParams[0] = {0.6f, 0.7f, 0.5f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Rotary Speaker (slow leslie)
        preset.engineParams[1] = {0.3f, 0.6f, 0.5f, 0.7f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Bucket Brigade (warm repeats)
        preset.engineParams[2] = {0.4f, 0.3f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.5f, 0.6f, 0.9f, 0.5f, 0.3f, 0.6f);
        // Emotional profile
        setEmotionalProfile(preset, 0.7f, 0.6f, 0.4f, 0.9f, 0.5f);
        // Source affinity
        setSourceAffinity(preset, 0.6f, 0.9f, 0.7f, 0.8f, 0.5f);
        
        // Metadata
        preset.keywords = {"organic", "flow", "envelope", "movement", "responsive", "dynamic", "natural"};
        preset.userPrompts = {"Organic movement", "Make it flow naturally", "Dynamic envelope response"};
        preset.antiFeatures = {"static", "rigid", "mechanical"};
        
        // Performance
        preset.cpuTier = CPUTier::MEDIUM;
        preset.actualCpuPercent = 5.2f;
        preset.realtimeSafe = true;
        
        // Quality metrics
        preset.complexity = 0.6f;
        preset.experimentalness = 0.5f;
        preset.versatility = 0.7f;
        
        preset.bestFor = "Guitars, Keys, Bass";
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    // 10. Data Storm - Glitch and digital mayhem
    {
        GoldenPreset preset;
        preset.id = generatePresetId();
        preset.name = PresetTemplates::DataStorm::name;
        preset.category = PresetTemplates::DataStorm::category;
        preset.subcategory = PresetTemplates::DataStorm::subcategory;
        preset.technicalHint = PresetTemplates::DataStorm::technicalHint;
        preset.shortCode = "DST-01";
        
        // Engine configuration
        for (int i = 0; i < 6; ++i) {
            preset.engineTypes[i] = PresetTemplates::DataStorm::engines[i];
            preset.engineMix[i] = PresetTemplates::DataStorm::mixLevels[i];
            preset.engineActive[i] = preset.engineTypes[i] >= 0;
        }
        
        // Parameters - Granular Cloud (chaotic grains)
        preset.engineParams[0] = {0.7f, 0.6f, 0.8f, 0.5f, 0.6f, 0.5f, 0.5f, 0.5f};
        // Parameters - Frequency Shifter (dissonant shift)
        preset.engineParams[1] = {0.4f, 0.6f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        // Parameters - Buffer Repeat (glitchy repeats)
        preset.engineParams[2] = {0.6f, 0.7f, 0.5f, 0.8f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        // Sonic profile
        setSonicProfile(preset, 0.6f, 0.9f, 0.8f, 0.5f, 0.7f, 0.1f);
        // Emotional profile
        setEmotionalProfile(preset, 0.8f, 0.3f, 0.8f, 0.1f, 0.2f);
        // Source affinity
        setSourceAffinity(preset, 0.5f, 0.6f, 0.9f, 0.9f, 0.7f);
        
        // Metadata
        preset.keywords = {"glitch", "digital", "granular", "chaos", "experimental", "idm", "broken"};
        preset.userPrompts = {"Digital glitch storm", "Granular chaos", "IDM glitch processing"};
        preset.antiFeatures = {"smooth", "traditional", "clean", "natural"};
        
        // Performance
        preset.cpuTier = CPUTier::HEAVY;
        preset.actualCpuPercent = 11.5f;
        preset.realtimeSafe = false;  // Granular can have unpredictable CPU spikes
        
        // Quality metrics
        preset.complexity = 0.8f;
        preset.experimentalness = 0.9f;
        preset.versatility = 0.5f;
        
        preset.bestFor = "Electronic, Drums, Sound Design";
        preset.genres = {"idm", "glitch", "experimental", "electronic"};
        preset.signature = "Chimera Team";
        preset.creationDate = Time::getCurrentTime();
        
        presets.push_back(preset);
    }
    
    return presets;
}

void PresetGenerator::setSonicProfile(GoldenPreset& preset,
                                     float brightness, float density, float movement,
                                     float space, float aggression, float vintage) {
    preset.sonicProfile.brightness = brightness;
    preset.sonicProfile.density = density;
    preset.sonicProfile.movement = movement;
    preset.sonicProfile.space = space;
    preset.sonicProfile.aggression = aggression;
    preset.sonicProfile.vintage = vintage;
}

void PresetGenerator::setEmotionalProfile(GoldenPreset& preset,
                                        float energy, float mood, float tension,
                                        float organic, float nostalgia) {
    preset.emotionalProfile.energy = energy;
    preset.emotionalProfile.mood = mood;
    preset.emotionalProfile.tension = tension;
    preset.emotionalProfile.organic = organic;
    preset.emotionalProfile.nostalgia = nostalgia;
}

void PresetGenerator::setSourceAffinity(GoldenPreset& preset,
                                      float vocals, float guitar, float drums,
                                      float synth, float mix) {
    preset.sourceAffinity.vocals = vocals;
    preset.sourceAffinity.guitar = guitar;
    preset.sourceAffinity.drums = drums;
    preset.sourceAffinity.synth = synth;
    preset.sourceAffinity.mix = mix;
}

String PresetGenerator::generatePresetId() {
    String id = "GC_";
    id += String(m_presetCounter).paddedLeft('0', 3);
    m_presetCounter++;
    return id;
}

String PresetGenerator::generateShortCode(const String& name) {
    // Extract first letters of each word
    StringArray words;
    words.addTokens(name, " ", "");
    
    String code;
    for (const auto& word : words) {
        if (word.isNotEmpty()) {
            code += word.substring(0, 1).toUpperCase();
        }
    }
    
    // Ensure it's exactly 3 characters
    if (code.length() < 3) {
        code = code.paddedRight('X', 3);
    } else if (code.length() > 3) {
        code = code.substring(0, 3);
    }
    
    code += "-01";  // Add version number
    return code;
}

std::vector<GoldenPreset> PresetGenerator::generateVariations(const GoldenPreset& heroPreset, int numVariations) {
    std::vector<GoldenPreset> variations;
    
    // Generate different types of variations
    if (numVariations >= 1) {
        variations.push_back(createSubtleVariation(heroPreset));
    }
    if (numVariations >= 2) {
        variations.push_back(createExtremeVariation(heroPreset));
    }
    if (numVariations >= 3) {
        variations.push_back(createAlternativeVariation(heroPreset));
    }
    
    return variations;
}

GoldenPreset PresetGenerator::createSubtleVariation(const GoldenPreset& original) {
    GoldenPreset variation = original;
    
    // Update identification
    variation.id = generatePresetId();
    variation.name = original.name + " - Subtle";
    variation.shortCode = original.shortCode.dropLastCharacters(2) + "02";
    variation.isVariation = true;
    variation.parentId = original.id;
    
    // Reduce all mix levels slightly
    for (int i = 0; i < 6; ++i) {
        variation.engineMix[i] *= 0.6f;
    }
    
    // Make parameters more conservative
    for (auto& params : variation.engineParams) {
        for (auto& param : params) {
            // Move parameters toward 0.5 (center)
            param = 0.5f + (param - 0.5f) * 0.7f;
        }
    }
    
    // Update profiles
    variation.sonicProfile.aggression *= 0.7f;
    variation.sonicProfile.density *= 0.8f;
    variation.complexity *= 0.8f;
    
    // Update metadata
    variation.keywords.push_back("subtle");
    variation.keywords.push_back("gentle");
    
    return variation;
}

GoldenPreset PresetGenerator::createExtremeVariation(const GoldenPreset& original) {
    GoldenPreset variation = original;
    
    // Update identification
    variation.id = generatePresetId();
    variation.name = original.name + " - Extreme";
    variation.shortCode = original.shortCode.dropLastCharacters(2) + "03";
    variation.isVariation = true;
    variation.parentId = original.id;
    
    // Increase mix levels
    for (int i = 0; i < 6; ++i) {
        if (variation.engineActive[i]) {
            variation.engineMix[i] = jmin(1.0f, variation.engineMix[i] * 1.4f);
        }
    }
    
    // Push parameters to extremes
    for (auto& params : variation.engineParams) {
        for (auto& param : params) {
            // Move parameters away from center
            if (param > 0.5f) {
                param = jmin(1.0f, 0.5f + (param - 0.5f) * 1.8f);
            } else {
                param = jmax(0.0f, 0.5f - (0.5f - param) * 1.8f);
            }
        }
    }
    
    // Update profiles
    variation.sonicProfile.aggression = jmin(1.0f, variation.sonicProfile.aggression * 1.5f);
    variation.sonicProfile.density = jmin(1.0f, variation.sonicProfile.density * 1.3f);
    variation.complexity = jmin(1.0f, variation.complexity * 1.2f);
    variation.experimentalness = jmin(1.0f, variation.experimentalness * 1.3f);
    
    // Update CPU tier if needed
    if (variation.cpuTier == CPUTier::LIGHT) {
        variation.cpuTier = CPUTier::MEDIUM;
    } else if (variation.cpuTier == CPUTier::MEDIUM) {
        variation.cpuTier = CPUTier::HEAVY;
    }
    
    // Update metadata
    variation.keywords.push_back("extreme");
    variation.keywords.push_back("intense");
    variation.keywords.push_back("pushed");
    
    return variation;
}

GoldenPreset PresetGenerator::createAlternativeVariation(const GoldenPreset& original) {
    GoldenPreset variation = original;
    
    // Update identification  
    variation.id = generatePresetId();
    variation.name = original.name + " - Alt";
    variation.shortCode = original.shortCode.dropLastCharacters(2) + "04";
    variation.isVariation = true;
    variation.parentId = original.id;
    
    // Swap engine order for different routing
    if (variation.engineTypes[0] >= 0 && variation.engineTypes[1] >= 0) {
        std::swap(variation.engineTypes[0], variation.engineTypes[1]);
        std::swap(variation.engineMix[0], variation.engineMix[1]);
        std::swap(variation.engineActive[0], variation.engineActive[1]);
        std::swap(variation.engineParams[0], variation.engineParams[1]);
    }
    
    // Invert some sonic characteristics
    variation.sonicProfile.brightness = 1.0f - variation.sonicProfile.brightness;
    variation.sonicProfile.vintage = 1.0f - variation.sonicProfile.vintage;
    
    // Different emotional target
    variation.emotionalProfile.mood = 1.0f - variation.emotionalProfile.mood;
    variation.emotionalProfile.energy = 1.0f - variation.emotionalProfile.energy;
    
    // Update metadata
    variation.keywords.push_back("alternative");
    variation.keywords.push_back("inverted");
    
    return variation;
}