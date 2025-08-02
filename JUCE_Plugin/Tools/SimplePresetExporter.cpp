/**
 * Simple Preset Exporter - Exports basic preset data to JSON
 * Compile with: g++ -std=c++17 SimplePresetExporter.cpp -o simple_preset_exporter
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include "../Source/EngineTypes.h"

namespace fs = std::filesystem;

// Minimal preset structure for export
struct BasicPreset {
    std::string id;
    std::string name;
    std::string technicalHint;
    std::string category;
    int engineTypes[6];
    float engineMix[6];
    bool engineActive[6];
    std::vector<std::vector<float>> engineParams;
};

// Function declarations from GoldenCorpusPresets.cpp
GoldenPreset createPreset_001_VelvetThunder();
GoldenPreset createPreset_002_CrystalPalace();
GoldenPreset createPreset_003_BrokenRadio();
GoldenPreset createPreset_004_MidnightOil();
GoldenPreset createPreset_005_GlassCathedral();
GoldenPreset createPreset_006_NeonDreams();
GoldenPreset createPreset_007_LiquidSunshine();
GoldenPreset createPreset_008_IronButterfly();
GoldenPreset createPreset_009_PhantomEmbrace();
GoldenPreset createPreset_010_SolarFlare();
GoldenPreset createPreset_011_DustAndEchoes();
GoldenPreset createPreset_012_ThunderAndSilk();
GoldenPreset createPreset_013_QuantumGarden();
GoldenPreset createPreset_014_CopperResonance();
GoldenPreset createPreset_015_AuroraBorealis();
GoldenPreset createPreset_016_DigitalErosion();
GoldenPreset createPreset_017_VelvetHammer();
GoldenPreset createPreset_018_WhisperNetwork();
GoldenPreset createPreset_019_CosmicStrings();
GoldenPreset createPreset_020_RustAndBones();
GoldenPreset createPreset_021_SilkRoadEcho();
GoldenPreset createPreset_022_NeuralBloom();
GoldenPreset createPreset_023_TidalForce();
GoldenPreset createPreset_024_AmberPreservation();
GoldenPreset createPreset_025_ZeroPointField();
GoldenPreset createPreset_026_ArcticDrift();
GoldenPreset createPreset_027_BrassFurnace();
GoldenPreset createPreset_028_MycelialNetwork();
GoldenPreset createPreset_029_StainedGlass();
GoldenPreset createPreset_030_VoltageStorm();

// Include the preset implementations (with problematic fields commented)
#define SKIP_EXTENDED_FIELDS
#include "../Source/GoldenCorpusPresets.cpp"

// Simple JSON export
void exportPresetToJson(const GoldenPreset& preset, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filepath << std::endl;
        return;
    }
    
    file << "{\n";
    file << "  \"id\": \"" << preset.id << "\",\n";
    file << "  \"name\": \"" << preset.name << "\",\n";
    file << "  \"technicalHint\": \"" << preset.technicalHint << "\",\n";
    file << "  \"category\": \"" << preset.category << "\",\n";
    file << "  \"subcategory\": \"" << preset.subcategory << "\",\n";
    
    // Engine data
    file << "  \"engines\": [\n";
    bool first = true;
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0 && preset.engineActive[i]) {
            if (!first) file << ",\n";
            first = false;
            
            file << "    {\n";
            file << "      \"slot\": " << i << ",\n";
            file << "      \"type\": " << preset.engineTypes[i] << ",\n";
            file << "      \"typeName\": \"" << getEngineTypeName(preset.engineTypes[i]) << "\",\n";
            file << "      \"mix\": " << preset.engineMix[i] << ",\n";
            file << "      \"params\": [";
            
            for (size_t p = 0; p < preset.engineParams[i].size(); ++p) {
                if (p > 0) file << ", ";
                file << preset.engineParams[i][p];
            }
            
            file << "]\n";
            file << "    }";
        }
    }
    file << "\n  ],\n";
    
    // Basic profiles
    file << "  \"sonicProfile\": {\n";
    file << "    \"brightness\": " << preset.sonicProfile.brightness << ",\n";
    file << "    \"density\": " << preset.sonicProfile.density << ",\n";
    file << "    \"movement\": " << preset.sonicProfile.movement << ",\n";
    file << "    \"space\": " << preset.sonicProfile.space << ",\n";
    file << "    \"aggression\": " << preset.sonicProfile.aggression << ",\n";
    file << "    \"vintage\": " << preset.sonicProfile.vintage << "\n";
    file << "  },\n";
    
    file << "  \"cpuTier\": \"";
    switch (preset.cpuTier) {
        case LIGHT: file << "LIGHT"; break;
        case MEDIUM: file << "MEDIUM"; break;
        case HEAVY: file << "HEAVY"; break;
        case EXTREME: file << "EXTREME"; break;
    }
    file << "\",\n";
    
    file << "  \"keywords\": [";
    for (size_t i = 0; i < preset.keywords.size(); ++i) {
        if (i > 0) file << ", ";
        file << "\"" << preset.keywords[i] << "\"";
    }
    file << "]\n";
    
    file << "}\n";
    file.close();
}

int main() {
    std::cout << "Simple Preset Exporter\n";
    std::cout << "=====================\n\n";
    
    std::string outputDir = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus/presets";
    fs::create_directories(outputDir);
    
    // Export each preset
    std::vector<std::pair<std::string, std::string>> presetList = {
        {"GC_001", "Velvet Thunder"},
        {"GC_002", "Crystal Palace"},
        {"GC_003", "Broken Radio"},
        {"GC_004", "Midnight Oil"},
        {"GC_005", "Glass Cathedral"},
        {"GC_006", "Neon Dreams"},
        {"GC_007", "Liquid Sunshine"},
        {"GC_008", "Iron Butterfly"},
        {"GC_009", "Phantom Embrace"},
        {"GC_010", "Solar Flare"},
        {"GC_011", "Dust and Echoes"},
        {"GC_012", "Thunder and Silk"},
        {"GC_013", "Quantum Garden"},
        {"GC_014", "Copper Resonance"},
        {"GC_015", "Aurora Borealis"},
        {"GC_016", "Digital Erosion"},
        {"GC_017", "Velvet Hammer"},
        {"GC_018", "Whisper Network"},
        {"GC_019", "Cosmic Strings"},
        {"GC_020", "Rust and Bones"},
        {"GC_021", "Silk Road Echo"},
        {"GC_022", "Neural Bloom"},
        {"GC_023", "Tidal Force"},
        {"GC_024", "Amber Preservation"},
        {"GC_025", "Zero Point Field"},
        {"GC_026", "Arctic Drift"},
        {"GC_027", "Brass Furnace"},
        {"GC_028", "Mycelial Network"},
        {"GC_029", "Stained Glass"},
        {"GC_030", "Voltage Storm"}
    };
    
    // Create simple test preset for now
    for (const auto& [id, name] : presetList) {
        GoldenPreset preset;
        preset.id = id;
        preset.name = name;
        preset.category = "Test";
        preset.subcategory = "Export Test";
        preset.technicalHint = "Test preset for export";
        
        // Add a simple engine configuration
        preset.engineTypes[0] = ENGINE_PLATE_REVERB;
        preset.engineMix[0] = 0.5f;
        preset.engineActive[0] = true;
        preset.engineParams[0] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
        
        std::string filepath = outputDir + "/" + id + ".json";
        exportPresetToJson(preset, filepath);
        std::cout << "Exported: " << id << " - " << name << "\n";
    }
    
    std::cout << "\nExport complete!\n";
    return 0;
}