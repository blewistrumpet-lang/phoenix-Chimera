/**
 * Standalone Preset Exporter - No JUCE dependencies
 * Compile with: g++ -std=c++17 ExportPresetsStandalone.cpp -o export_presets_standalone
 */

#include "../Source/EngineTypes.h"
#include "../Source/GoldenCorpusPresets.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

// Convert preset to JSON string
std::string presetToJson(const GoldenPreset& preset) {
    std::stringstream json;
    json << "{\n";
    
    // Basic info
    json << "  \"id\": \"" << preset.id << "\",\n";
    json << "  \"name\": \"" << preset.name << "\",\n";
    json << "  \"technicalHint\": \"" << preset.technicalHint << "\",\n";
    json << "  \"shortCode\": \"" << preset.shortCode << "\",\n";
    json << "  \"category\": \"" << preset.category << "\",\n";
    json << "  \"subcategory\": \"" << preset.subcategory << "\",\n";
    json << "  \"version\": " << preset.version << ",\n";
    
    // Hierarchy
    json << "  \"isVariation\": " << (preset.isVariation ? "true" : "false") << ",\n";
    json << "  \"parentId\": \"" << preset.parentId << "\",\n";
    
    // Engines
    json << "  \"engines\": [\n";
    bool firstEngine = true;
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0 && preset.engineActive[i]) {
            if (!firstEngine) json << ",\n";
            firstEngine = false;
            
            json << "    {\n";
            json << "      \"slot\": " << i << ",\n";
            json << "      \"type\": " << preset.engineTypes[i] << ",\n";
            json << "      \"typeName\": \"" << getEngineTypeName(preset.engineTypes[i]) << "\",\n";
            json << "      \"mix\": " << std::fixed << std::setprecision(3) << preset.engineMix[i] << ",\n";
            json << "      \"active\": true,\n";
            json << "      \"params\": [";
            
            // Parameters
            for (size_t p = 0; p < preset.engineParams[i].size(); ++p) {
                if (p > 0) json << ", ";
                json << std::fixed << std::setprecision(3) << preset.engineParams[i][p];
            }
            json << "]\n";
            json << "    }";
        }
    }
    json << "\n  ],\n";
    
    // Profiles
    json << "  \"sonicProfile\": {\n";
    json << "    \"brightness\": " << std::fixed << std::setprecision(3) << preset.sonicProfile.brightness << ",\n";
    json << "    \"density\": " << preset.sonicProfile.density << ",\n";
    json << "    \"movement\": " << preset.sonicProfile.movement << ",\n";
    json << "    \"space\": " << preset.sonicProfile.space << ",\n";
    json << "    \"aggression\": " << preset.sonicProfile.aggression << ",\n";
    json << "    \"vintage\": " << preset.sonicProfile.vintage << "\n";
    json << "  },\n";
    
    json << "  \"emotionalProfile\": {\n";
    json << "    \"energy\": " << std::fixed << std::setprecision(3) << preset.emotionalProfile.energy << ",\n";
    json << "    \"mood\": " << preset.emotionalProfile.mood << ",\n";
    json << "    \"tension\": " << preset.emotionalProfile.tension << ",\n";
    json << "    \"organic\": " << preset.emotionalProfile.organic << ",\n";
    json << "    \"nostalgia\": " << preset.emotionalProfile.nostalgia << "\n";
    json << "  },\n";
    
    json << "  \"sourceAffinity\": {\n";
    json << "    \"vocals\": " << std::fixed << std::setprecision(3) << preset.sourceAffinity.vocals << ",\n";
    json << "    \"guitar\": " << preset.sourceAffinity.guitar << ",\n";
    json << "    \"drums\": " << preset.sourceAffinity.drums << ",\n";
    json << "    \"synth\": " << preset.sourceAffinity.synth << ",\n";
    json << "    \"mix\": " << preset.sourceAffinity.mix << "\n";
    json << "  },\n";
    
    // Performance
    json << "  \"cpuTier\": \"";
    switch (preset.cpuTier) {
        case LIGHT: json << "LIGHT"; break;
        case MEDIUM: json << "MEDIUM"; break;
        case HEAVY: json << "HEAVY"; break;
        case EXTREME: json << "EXTREME"; break;
    }
    json << "\",\n";
    json << "  \"actualCpuPercent\": " << preset.actualCpuPercent << ",\n";
    json << "  \"latencySamples\": " << preset.latencySamples << ",\n";
    json << "  \"realtimeSafe\": " << (preset.realtimeSafe ? "true" : "false") << ",\n";
    
    // Musical context
    json << "  \"optimalTempo\": " << preset.optimalTempo << ",\n";
    json << "  \"musicalKey\": \"" << preset.musicalKey << "\",\n";
    
    // Genres
    json << "  \"genres\": [";
    for (size_t i = 0; i < preset.genres.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << preset.genres[i] << "\"";
    }
    json << "],\n";
    
    // Quality
    json << "  \"signature\": \"" << preset.signature << "\",\n";
    json << "  \"creationDate\": \"" << preset.creationDate << "\",\n";
    json << "  \"popularityScore\": " << preset.popularityScore << ",\n";
    json << "  \"qualityScore\": " << preset.qualityScore << ",\n";
    
    // Complexity
    json << "  \"complexity\": " << preset.complexity << ",\n";
    json << "  \"experimentalness\": " << preset.experimentalness << ",\n";
    json << "  \"versatility\": " << preset.versatility << ",\n";
    
    // Keywords
    json << "  \"keywords\": [";
    for (size_t i = 0; i < preset.keywords.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << preset.keywords[i] << "\"";
    }
    json << "],\n";
    
    // Anti-features
    json << "  \"antiFeatures\": [";
    for (size_t i = 0; i < preset.antiFeatures.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << preset.antiFeatures[i] << "\"";
    }
    json << "],\n";
    
    // User prompts
    json << "  \"userPrompts\": [";
    for (size_t i = 0; i < preset.userPrompts.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << preset.userPrompts[i] << "\"";
    }
    json << "],\n";
    
    // Usage hints
    json << "  \"bestFor\": \"" << preset.bestFor << "\",\n";
    json << "  \"avoidFor\": \"" << preset.avoidFor << "\"\n";
    
    json << "}";
    
    return json.str();
}

int main(int argc, char* argv[]) {
    std::cout << "Golden Corpus Preset Exporter (Standalone)\n";
    std::cout << "==========================================\n\n";
    
    // Create output directory
    std::string outputPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus";
    if (argc > 1) {
        outputPath = argv[1];
    }
    
    fs::create_directories(outputPath);
    fs::create_directories(outputPath + "/presets");
    
    std::cout << "Output directory: " << outputPath << "\n\n";
    
    // Create all presets
    std::vector<GoldenPreset> allPresets;
    
    // Add all 30 presets
    allPresets.push_back(createPreset_001_VelvetThunder());
    allPresets.push_back(createPreset_002_CrystalPalace());
    allPresets.push_back(createPreset_003_BrokenRadio());
    allPresets.push_back(createPreset_004_MidnightOil());
    allPresets.push_back(createPreset_005_GlassCathedral());
    allPresets.push_back(createPreset_006_NeonDreams());
    allPresets.push_back(createPreset_007_LiquidSunshine());
    allPresets.push_back(createPreset_008_IronButterfly());
    allPresets.push_back(createPreset_009_PhantomEmbrace());
    allPresets.push_back(createPreset_010_SolarFlare());
    allPresets.push_back(createPreset_011_DustAndEchoes());
    allPresets.push_back(createPreset_012_ThunderAndSilk());
    allPresets.push_back(createPreset_013_QuantumGarden());
    allPresets.push_back(createPreset_014_CopperResonance());
    allPresets.push_back(createPreset_015_AuroraBorealis());
    allPresets.push_back(createPreset_016_DigitalErosion());
    allPresets.push_back(createPreset_017_VelvetHammer());
    allPresets.push_back(createPreset_018_WhisperNetwork());
    allPresets.push_back(createPreset_019_CosmicStrings());
    allPresets.push_back(createPreset_020_RustAndBones());
    allPresets.push_back(createPreset_021_SilkRoadEcho());
    allPresets.push_back(createPreset_022_NeuralBloom());
    allPresets.push_back(createPreset_023_TidalForce());
    allPresets.push_back(createPreset_024_AmberPreservation());
    allPresets.push_back(createPreset_025_ZeroPointField());
    allPresets.push_back(createPreset_026_ArcticDrift());
    allPresets.push_back(createPreset_027_BrassFurnace());
    allPresets.push_back(createPreset_028_MycelialNetwork());
    allPresets.push_back(createPreset_029_StainedGlass());
    allPresets.push_back(createPreset_030_VoltageStorm());
    
    std::cout << "Created " << allPresets.size() << " presets\n";
    
    // Export individual files
    int exported = 0;
    for (const auto& preset : allPresets) {
        std::string filename = outputPath + "/presets/" + preset.id + ".json";
        std::ofstream file(filename);
        if (file.is_open()) {
            file << presetToJson(preset);
            file.close();
            exported++;
            std::cout << "Exported: " << preset.id << " - " << preset.name << "\n";
        }
    }
    
    // Export all presets to single file
    std::string allPresetsFile = outputPath + "/all_presets.json";
    std::ofstream allFile(allPresetsFile);
    if (allFile.is_open()) {
        allFile << "{\n";
        allFile << "  \"version\": \"1.0\",\n";
        allFile << "  \"presetCount\": " << allPresets.size() << ",\n";
        allFile << "  \"presets\": [\n";
        
        for (size_t i = 0; i < allPresets.size(); ++i) {
            if (i > 0) allFile << ",\n";
            allFile << presetToJson(allPresets[i]);
        }
        
        allFile << "\n  ]\n";
        allFile << "}\n";
        allFile.close();
        
        std::cout << "\nAlso saved all presets to: " << allPresetsFile << "\n";
    }
    
    // Create metadata file
    std::string metadataFile = outputPath + "/corpus_metadata.json";
    std::ofstream metaFile(metadataFile);
    if (metaFile.is_open()) {
        metaFile << "{\n";
        metaFile << "  \"version\": \"1.0\",\n";
        metaFile << "  \"exportDate\": \"" << __DATE__ << " " << __TIME__ << "\",\n";
        metaFile << "  \"presetCount\": " << allPresets.size() << ",\n";
        
        // Category distribution
        std::map<std::string, int> categoryCount;
        for (const auto& preset : allPresets) {
            categoryCount[preset.category]++;
        }
        
        metaFile << "  \"categories\": [\n";
        bool first = true;
        for (const auto& [cat, count] : categoryCount) {
            if (!first) metaFile << ",\n";
            first = false;
            metaFile << "    {\"name\": \"" << cat << "\", \"count\": " << count << "}";
        }
        metaFile << "\n  ]\n";
        metaFile << "}\n";
        metaFile.close();
    }
    
    std::cout << "\n";
    std::cout << "Export complete!\n";
    std::cout << "Exported " << exported << " presets to " << outputPath << "\n";
    std::cout << "\nReady for FAISS indexing!\n";
    
    return 0;
}