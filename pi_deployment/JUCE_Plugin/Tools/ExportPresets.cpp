/**
 * Export Presets Tool - Command line utility to export Golden Corpus to JSON
 * Compile with: g++ -std=c++17 ExportPresets.cpp -o export_presets
 */

#include "../Source/PresetExporter.cpp"
#include "../Source/GoldenCorpusPresets.cpp"
#include <iostream>

// Register all presets
void registerAllPresets() {
    // Register each preset with the registry
    REGISTER_PRESET("GC_001", createPreset_001_VelvetThunder)
    REGISTER_PRESET("GC_002", createPreset_002_CrystalPalace)
    REGISTER_PRESET("GC_003", createPreset_003_BrokenRadio)
    REGISTER_PRESET("GC_004", createPreset_004_MidnightOil)
    REGISTER_PRESET("GC_005", createPreset_005_GlassCathedral)
    REGISTER_PRESET("GC_006", createPreset_006_NeonDreams)
    REGISTER_PRESET("GC_007", createPreset_007_LiquidSunshine)
    REGISTER_PRESET("GC_008", createPreset_008_IronButterfly)
    REGISTER_PRESET("GC_009", createPreset_009_PhantomEmbrace)
    REGISTER_PRESET("GC_010", createPreset_010_SolarFlare)
    REGISTER_PRESET("GC_011", createPreset_011_DustAndEchoes)
    REGISTER_PRESET("GC_012", createPreset_012_ThunderAndSilk)
    REGISTER_PRESET("GC_013", createPreset_013_QuantumGarden)
    REGISTER_PRESET("GC_014", createPreset_014_CopperResonance)
    REGISTER_PRESET("GC_015", createPreset_015_AuroraBorealis)
    REGISTER_PRESET("GC_016", createPreset_016_DigitalErosion)
    REGISTER_PRESET("GC_017", createPreset_017_VelvetHammer)
    REGISTER_PRESET("GC_018", createPreset_018_WhisperNetwork)
    REGISTER_PRESET("GC_019", createPreset_019_CosmicStrings)
    REGISTER_PRESET("GC_020", createPreset_020_RustAndBones)
    REGISTER_PRESET("GC_021", createPreset_021_SilkRoadEcho)
    REGISTER_PRESET("GC_022", createPreset_022_NeuralBloom)
    REGISTER_PRESET("GC_023", createPreset_023_TidalForce)
    REGISTER_PRESET("GC_024", createPreset_024_AmberPreservation)
    REGISTER_PRESET("GC_025", createPreset_025_ZeroPointField)
    REGISTER_PRESET("GC_026", createPreset_026_ArcticDrift)
    REGISTER_PRESET("GC_027", createPreset_027_BrassFurnace)
    REGISTER_PRESET("GC_028", createPreset_028_MycelialNetwork)
    REGISTER_PRESET("GC_029", createPreset_029_StainedGlass)
    REGISTER_PRESET("GC_030", createPreset_030_VoltageStorm)
}

int main(int argc, char* argv[]) {
    std::cout << "Golden Corpus Preset Exporter\n";
    std::cout << "=============================\n\n";
    
    // Register all presets
    registerAllPresets();
    
    std::cout << "Registered " << PresetRegistry::getPresetCount() << " presets\n";
    
    // Set output directory
    String outputPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/GoldenCorpus";
    if (argc > 1) {
        outputPath = argv[1];
    }
    
    File outputDir(outputPath);
    std::cout << "Output directory: " << outputDir.getFullPathName() << "\n\n";
    
    // Create all presets
    std::cout << "Creating presets...\n";
    std::vector<GoldenPreset> allPresets = PresetRegistry::createAllPresets();
    
    // Export to individual files
    std::cout << "Exporting to JSON files...\n";
    int exported = PresetExporter::exportPresetsToDirectory(allPresets, outputDir);
    
    std::cout << "\nExport complete!\n";
    std::cout << "Exported " << exported << " presets to " << outputDir.getFullPathName() << "\n";
    
    // Also export as single file for convenience
    File singleFile = outputDir.getChildFile("all_presets.json");
    if (PresetExporter::exportPresetsToSingleFile(allPresets, singleFile)) {
        std::cout << "Also saved all presets to: " << singleFile.getFullPathName() << "\n";
    }
    
    std::cout << "\nReady for FAISS indexing!\n";
    
    return 0;
}