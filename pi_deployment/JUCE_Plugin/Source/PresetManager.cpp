#include "PresetManager.h"
#include <algorithm>

PresetManager::PresetManager() {
    // Default corpus location
    corpusDirectory = File::getSpecialLocation(File::userApplicationDataDirectory)
                        .getChildFile("ChimeraPhoenix")
                        .getChildFile("GoldenCorpus");
}

PresetManager::~PresetManager() {
    // Save any pending changes
    if (initialized) {
        saveCorpus();
    }
}

//==============================================================================
// INITIALIZATION

bool PresetManager::initialize() {
    return initialize(corpusDirectory);
}

bool PresetManager::initialize(const File& directory) {
    ScopedLock lock(corpusLock);
    
    corpusDirectory = directory;
    corpusFile = corpusDirectory.getChildFile("golden_corpus.json");
    
    // Create directory if needed
    if (!corpusDirectory.exists()) {
        corpusDirectory.createDirectory();
    }
    
    // Try to load existing corpus
    if (corpusFile.exists()) {
        if (loadCorpus()) {
            initialized = true;
            return true;
        }
    }
    
    // Generate new corpus if none exists
    DBG("No corpus found, generating new Golden Corpus...");
    if (generateCompleteCorpus()) {
        saveCorpus();
        initialized = true;
        return true;
    }
    
    return false;
}

//==============================================================================
// CORPUS GENERATION

bool PresetManager::generateCompleteCorpus() {
    ScopedLock lock(corpusLock);
    
    corpus.clear();
    
    DBG("Generating 250 Golden Corpus presets...");
    
    // Generate each category
    generateStudioEssentials();      // 50 presets
    generateSpatialDesigns();        // 50 presets  
    generateCharacterColors();       // 50 presets
    generateMotionModulation();      // 50 presets
    generateExperimental();          // 50 presets
    
    // Validate all presets
    int validCount = 0;
    for (const auto& preset : corpus) {
        auto result = PresetValidator::validatePreset(*preset);
        if (result.passed) {
            validCount++;
            preset->qualityScore = result.qualityScore;
        } else {
            DBG("Preset " << preset->id << " failed validation: " << result.getSummary());
        }
    }
    
    DBG("Generated " << corpus.size() << " presets, " << validCount << " passed validation");
    
    // Rebuild indices
    rebuildIndices();
    
    // Notify listeners
    notifyCorpusReloaded();
    
    return corpus.size() == 250;
}

void PresetManager::generateStudioEssentials() {
    String category = "Studio Essentials";
    int startId = 1;
    
    // Vocal chains (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::VocalPolish
        );
        
        preset->id = String::formatted("GC_%03d", startId + i);
        preset->category = category;
        preset->subcategory = "Vocal Processing";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        preset->signature = "Chimera Phoenix Generator";
        
        // Add variations to some presets
        if (i == 0 || i == 5) {
            preset->name += " [Classic]";
        } else if (i == 2 || i == 7) {
            preset->name += " [Modern]";
        }
        
        corpus.push_back(std::move(preset));
    }
    
    // Mix bus processors (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::MixGlue
        );
        
        preset->id = String::formatted("GC_%03d", startId + 10 + i);
        preset->category = category;
        preset->subcategory = "Mix Bus";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        corpus.push_back(std::move(preset));
    }
    
    // Instrument sweeteners (10)
    for (int i = 0; i < 10; ++i) {
        BoutiquePresetGenerator::MusicalContext context;
        
        // Vary by instrument type
        if (i < 3) {
            context.sourceType = "Guitar";
            preset->subcategory = "Guitar Enhancement";
        } else if (i < 6) {
            context.sourceType = "Keys";
            preset->subcategory = "Keyboard Polish";
        } else {
            context.sourceType = "Bass";
            preset->subcategory = "Bass Enhancement";
        }
        
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::AnalogWarmth,
            context
        );
        
        preset->id = String::formatted("GC_%03d", startId + 20 + i);
        preset->category = category;
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        corpus.push_back(std::move(preset));
    }
    
    // Corrective tools (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::SurgicalCorrection
        );
        
        preset->id = String::formatted("GC_%03d", startId + 30 + i);
        preset->category = category;
        preset->subcategory = "Problem Solvers";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        corpus.push_back(std::move(preset));
    }
    
    // Classic channel strips (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::AnalogWarmth
        );
        
        preset->id = String::formatted("GC_%03d", startId + 40 + i);
        preset->category = category;
        preset->subcategory = "Channel Strips";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        // Name them after famous consoles
        std::vector<String> consoleNames = {
            "SSL Legacy", "Neve Heritage", "API Punch", "Trident Soul",
            "Harrison Warmth", "MCI Character", "Amek Precision",
            "Focusrite Red", "Euphonix Digital", "Mackie Modern"
        };
        preset->name = consoleNames[i];
        
        corpus.push_back(std::move(preset));
    }
}

void PresetManager::generateSpatialDesigns() {
    String category = "Spatial Design";
    int startId = 51;
    
    // Natural spaces (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::IntimateRoom
        );
        
        preset->id = String::formatted("GC_%03d", startId + i);
        preset->category = category;
        preset->subcategory = "Natural Spaces";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        corpus.push_back(std::move(preset));
    }
    
    // Impossible spaces (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::DreamscapeAmbience
        );
        
        preset->id = String::formatted("GC_%03d", startId + 10 + i);
        preset->category = category;
        preset->subcategory = "Impossible Spaces";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        corpus.push_back(std::move(preset));
    }
    
    // Continue with other spatial subcategories...
}

void PresetManager::generateCharacterColors() {
    String category = "Character & Color";
    int startId = 101;
    
    // Tape sounds (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::TapeNostalgia
        );
        
        preset->id = String::formatted("GC_%03d", startId + i);
        preset->category = category;
        preset->subcategory = "Tape Character";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        corpus.push_back(std::move(preset));
    }
    
    // Continue with other character subcategories...
}

void PresetManager::generateMotionModulation() {
    String category = "Motion & Modulation";
    int startId = 151;
    
    // Rhythmic processors (10)
    for (int i = 0; i < 10; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::RhythmicPulse
        );
        
        preset->id = String::formatted("GC_%03d", startId + i);
        preset->category = category;
        preset->subcategory = "Rhythmic Motion";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        
        corpus.push_back(std::move(preset));
    }
    
    // Continue with other motion subcategories...
}

void PresetManager::generateExperimental() {
    String category = "Experimental Laboratory";
    int startId = 201;
    
    // Granular experiments (12)
    for (int i = 0; i < 12; ++i) {
        auto preset = BoutiquePresetGenerator::generatePreset(
            BoutiquePresetGenerator::PresetArchetype::GranularTexture
        );
        
        preset->id = String::formatted("GC_%03d", startId + i);
        preset->category = category;
        preset->subcategory = "Granular Synthesis";
        preset->creationTimestamp = Time::getCurrentTime().toMilliseconds();
        preset->experimentalness = 0.8f + (i * 0.02f); // Increasingly experimental
        
        corpus.push_back(std::move(preset));
    }
    
    // Continue with other experimental subcategories...
}

//==============================================================================
// PRESET ACCESS

GoldenPreset* PresetManager::getPreset(const String& presetId) {
    ScopedLock lock(corpusLock);
    
    auto it = idMap.find(presetId);
    if (it != idMap.end()) {
        return it->second;
    }
    return nullptr;
}

const GoldenPreset* PresetManager::getPreset(const String& presetId) const {
    ScopedLock lock(corpusLock);
    
    auto it = idMap.find(presetId);
    if (it != idMap.end()) {
        return it->second;
    }
    return nullptr;
}

GoldenPreset* PresetManager::getPresetByIndex(int index) {
    ScopedLock lock(corpusLock);
    
    if (index >= 0 && index < corpus.size()) {
        return corpus[index].get();
    }
    return nullptr;
}

std::vector<GoldenPreset*> PresetManager::getPresetsInCategory(const String& category) {
    ScopedLock lock(corpusLock);
    
    auto it = categoryMap.find(category);
    if (it != categoryMap.end()) {
        return it->second;
    }
    return {};
}

//==============================================================================
// SEARCH & DISCOVERY

std::vector<GoldenPreset*> PresetManager::searchByKeywords(const StringArray& keywords) {
    ScopedLock lock(corpusLock);
    
    std::vector<GoldenPreset*> results;
    
    for (const auto& preset : corpus) {
        int matches = 0;
        
        for (const auto& keyword : keywords) {
            // Check preset keywords
            for (const auto& presetKeyword : preset->keywords) {
                if (presetKeyword.containsIgnoreCase(keyword)) {
                    matches++;
                    break;
                }
            }
            
            // Also check name and category
            if (preset->name.containsIgnoreCase(keyword) ||
                preset->category.containsIgnoreCase(keyword)) {
                matches++;
            }
        }
        
        // Return presets that match at least half the keywords
        if (matches >= (keywords.size() + 1) / 2) {
            results.push_back(preset.get());
        }
    }
    
    return results;
}

std::vector<GoldenPreset*> PresetManager::findSimilarPresets(const String& presetId, int maxResults) {
    ScopedLock lock(corpusLock);
    
    auto sourcePreset = getPreset(presetId);
    if (!sourcePreset) {
        return {};
    }
    
    // Use validator's similarity function
    auto similar = PresetValidator::findSimilarPresets(*sourcePreset, corpus, 0.8f);
    
    std::vector<GoldenPreset*> results;
    for (const auto& similarName : similar) {
        // Extract preset name from similarity string
        String name = similarName.upToFirstOccurrenceOf(" (", false, false);
        
        // Find preset by name
        for (const auto& preset : corpus) {
            if (preset->name == name && preset->id != presetId) {
                results.push_back(preset.get());
                if (results.size() >= maxResults) {
                    return results;
                }
                break;
            }
        }
    }
    
    return results;
}

//==============================================================================
// SAVING & LOADING

bool PresetManager::saveCorpus() {
    return saveCorpusToPath(corpusDirectory);
}

bool PresetManager::saveCorpusToPath(const File& directory) {
    ScopedLock lock(corpusLock);
    
    File targetFile = directory.getChildFile("golden_corpus.json");
    
    // Save as JSON
    if (PresetSerializer::saveCorpusToJSON(corpus, targetFile)) {
        DBG("Saved corpus to: " << targetFile.getFullPathName());
        
        // Also save individual presets for easier editing
        File presetsDir = directory.getChildFile("presets");
        presetsDir.createDirectory();
        
        for (const auto& preset : corpus) {
            File presetFile = presetsDir.getChildFile(preset->id + ".json");
            PresetSerializer::savePresetToJSON(*preset, presetFile);
        }
        
        return true;
    }
    
    return false;
}

bool PresetManager::loadCorpus() {
    return loadCorpusFromPath(corpusDirectory);
}

bool PresetManager::loadCorpusFromPath(const File& directory) {
    ScopedLock lock(corpusLock);
    
    File sourceFile = directory.getChildFile("golden_corpus.json");
    
    if (!sourceFile.exists()) {
        DBG("Corpus file not found: " << sourceFile.getFullPathName());
        return false;
    }
    
    auto loadedCorpus = PresetSerializer::loadCorpusFromJSON(sourceFile);
    
    if (loadedCorpus.empty()) {
        DBG("Failed to load corpus or corpus is empty");
        return false;
    }
    
    corpus = std::move(loadedCorpus);
    rebuildIndices();
    
    DBG("Loaded " << corpus.size() << " presets from corpus");
    
    notifyCorpusReloaded();
    return true;
}

//==============================================================================
// EXPORT

bool PresetManager::exportForFAISS(const File& outputFile) {
    ScopedLock lock(corpusLock);
    
    DynamicObject::Ptr root(new DynamicObject());
    Array<var> presets;
    
    for (const auto& preset : corpus) {
        String faissJson = PresetSerializer::exportForFAISS(*preset);
        var presetVar = JSON::parse(faissJson);
        if (!presetVar.isVoid()) {
            presets.add(presetVar);
        }
    }
    
    root->setProperty("presets", presets);
    root->setProperty("count", corpus.size());
    root->setProperty("version", 1);
    
    String jsonString = JSON::toString(var(root.get()), false); // Compact
    return outputFile.replaceWithText(jsonString);
}

//==============================================================================
// STATISTICS

PresetManager::CorpusStatistics PresetManager::getStatistics() const {
    ScopedLock lock(corpusLock);
    
    CorpusStatistics stats;
    stats.totalPresets = corpus.size();
    
    std::map<int, int> engineUsage;
    float totalComplexity = 0.0f;
    float totalCPU = 0.0f;
    
    for (const auto& preset : corpus) {
        // Count unique vs variations
        if (preset->isVariation) {
            stats.variations++;
        } else {
            stats.uniquePresets++;
        }
        
        // Category counts
        stats.categoryCounts[preset->category]++;
        
        // CPU tier counts
        stats.cpuTierCounts[preset->cpuTier]++;
        
        // Complexity and CPU
        totalComplexity += preset->complexity;
        totalCPU += preset->actualCpuPercent;
        
        // Engine usage
        for (int i = 0; i < 6; ++i) {
            if (preset->engineTypes[i] >= 0) {
                engineUsage[preset->engineTypes[i]]++;
            }
        }
    }
    
    if (stats.totalPresets > 0) {
        stats.averageComplexity = totalComplexity / stats.totalPresets;
        stats.averageCPU = totalCPU / stats.totalPresets;
    }
    
    // Find most/least used engines
    std::vector<std::pair<int, int>> enginePairs(engineUsage.begin(), engineUsage.end());
    std::sort(enginePairs.begin(), enginePairs.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Convert engine IDs to names (would need engine name lookup)
    for (int i = 0; i < 5 && i < enginePairs.size(); ++i) {
        stats.mostUsedEngines.push_back("Engine_" + String(enginePairs[i].first));
    }
    
    for (int i = enginePairs.size() - 1; i >= 0 && stats.leastUsedEngines.size() < 5; --i) {
        stats.leastUsedEngines.push_back("Engine_" + String(enginePairs[i].first));
    }
    
    return stats;
}

//==============================================================================
// HELPER METHODS

void PresetManager::rebuildIndices() {
    idMap.clear();
    categoryMap.clear();
    parentChildMap.clear();
    
    for (const auto& preset : corpus) {
        // ID map
        idMap[preset->id] = preset.get();
        
        // Category map
        categoryMap[preset->category].push_back(preset.get());
        
        // Parent-child relationships
        if (preset->isVariation && !preset->parentId.isEmpty()) {
            parentChildMap[preset->parentId].push_back(preset->id);
        }
    }
}

String PresetManager::generateNextId() const {
    int maxId = 0;
    
    for (const auto& preset : corpus) {
        if (preset->id.startsWith("GC_")) {
            int id = preset->id.substring(3).getIntValue();
            maxId = std::max(maxId, id);
        }
    }
    
    return String::formatted("GC_%03d", maxId + 1);
}

//==============================================================================
// LISTENER NOTIFICATION

void PresetManager::addListener(Listener* listener) {
    listeners.add(listener);
}

void PresetManager::removeListener(Listener* listener) {
    listeners.remove(listener);
}

void PresetManager::notifyPresetAdded(const String& presetId) {
    listeners.call(&Listener::presetAdded, presetId);
}

void PresetManager::notifyCorpusReloaded() {
    listeners.call(&Listener::corpusReloaded);
}