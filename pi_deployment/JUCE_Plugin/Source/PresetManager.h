#pragma once

#include <JuceHeader.h>
#include "GoldenPreset.h"
#include "PresetSerializer.h"
#include "PresetValidator.h"
#include "BoutiquePresetGenerator.h"

/**
 * PresetManager - Central management system for the Golden Corpus
 * Handles loading, saving, searching, and organizing all 250 presets
 */
class PresetManager {
public:
    PresetManager();
    ~PresetManager();
    
    // === INITIALIZATION ===
    
    // Initialize with default corpus location
    bool initialize();
    
    // Initialize with custom corpus path
    bool initialize(const File& corpusDirectory);
    
    // Generate the complete Golden Corpus (250 presets)
    bool generateCompleteCorpus();
    
    // === PRESET ACCESS ===
    
    // Get preset by ID
    GoldenPreset* getPreset(const String& presetId);
    const GoldenPreset* getPreset(const String& presetId) const;
    
    // Get preset by index
    GoldenPreset* getPresetByIndex(int index);
    const GoldenPreset* getPresetByIndex(int index) const;
    
    // Get all presets in category
    std::vector<GoldenPreset*> getPresetsInCategory(const String& category);
    
    // Get preset count
    int getPresetCount() const { return corpus.size(); }
    
    // === SEARCH & DISCOVERY ===
    
    // Search by keywords
    std::vector<GoldenPreset*> searchByKeywords(const StringArray& keywords);
    
    // Find similar presets
    std::vector<GoldenPreset*> findSimilarPresets(const String& presetId, int maxResults = 5);
    
    // Get presets by CPU tier
    std::vector<GoldenPreset*> getPresetsByCPUTier(CPUTier tier);
    
    // Get presets suitable for source type
    std::vector<GoldenPreset*> getPresetsForSource(const String& sourceType);
    
    // === CORPUS MANAGEMENT ===
    
    // Save entire corpus
    bool saveCorpus();
    bool saveCorpusToPath(const File& directory);
    
    // Load corpus
    bool loadCorpus();
    bool loadCorpusFromPath(const File& directory);
    
    // Export for Python/FAISS
    bool exportForFAISS(const File& outputFile);
    
    // === VALIDATION & QUALITY ===
    
    // Validate entire corpus
    std::vector<PresetValidator::ValidationResult> validateAllPresets();
    
    // Get corpus statistics
    struct CorpusStatistics {
        int totalPresets = 0;
        int uniquePresets = 0;
        int variations = 0;
        std::map<String, int> categoryCounts;
        std::map<CPUTier, int> cpuTierCounts;
        float averageComplexity = 0.0f;
        float averageCPU = 0.0f;
        std::vector<String> mostUsedEngines;
        std::vector<String> leastUsedEngines;
    };
    
    CorpusStatistics getStatistics() const;
    
    // === PRESET CREATION ===
    
    // Add custom preset to corpus
    bool addPreset(std::unique_ptr<GoldenPreset> preset);
    
    // Generate variations of existing preset
    std::vector<String> generateVariations(const String& parentId, int count = 3);
    
    // === CATEGORIES ===
    
    // Get all categories
    std::vector<String> getAllCategories() const;
    
    // Get subcategories for category
    std::vector<String> getSubcategories(const String& category) const;
    
    // === SORTING & FILTERING ===
    
    enum class SortOrder {
        Alphabetical,
        Category,
        CPUUsage,
        Complexity,
        Popularity,
        DateCreated
    };
    
    // Get sorted preset list
    std::vector<GoldenPreset*> getSortedPresets(SortOrder order);
    
    // Filter presets
    std::vector<GoldenPreset*> filterPresets(
        std::function<bool(const GoldenPreset&)> predicate
    );
    
    // === USER INTERACTION ===
    
    // Track preset usage
    void recordPresetUse(const String& presetId);
    
    // Rate preset
    void ratePreset(const String& presetId, float rating);
    
    // Get most popular presets
    std::vector<GoldenPreset*> getMostPopularPresets(int count = 10);
    
    // === LISTENERS ===
    
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void presetAdded(const String& presetId) {}
        virtual void presetModified(const String& presetId) {}
        virtual void presetRemoved(const String& presetId) {}
        virtual void corpusReloaded() {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Core storage
    std::vector<std::unique_ptr<GoldenPreset>> corpus;
    std::map<String, GoldenPreset*> idMap;  // Fast lookup by ID
    
    // Organization
    std::map<String, std::vector<GoldenPreset*>> categoryMap;
    std::map<String, std::vector<String>> parentChildMap;  // Variations
    
    // Paths
    File corpusDirectory;
    File corpusFile;
    
    // State
    bool initialized = false;
    mutable CriticalSection corpusLock;
    
    // Listeners
    ListenerList<Listener> listeners;
    
    // Helper methods
    void rebuildIndices();
    void notifyPresetAdded(const String& presetId);
    void notifyPresetModified(const String& presetId);
    void notifyCorpusReloaded();
    
    // Preset generation helpers
    void generateStudioEssentials();
    void generateSpatialDesigns();
    void generateCharacterColors();
    void generateMotionModulation();
    void generateExperimental();
    
    // ID generation
    String generateNextId() const;
    bool isIdUnique(const String& id) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};