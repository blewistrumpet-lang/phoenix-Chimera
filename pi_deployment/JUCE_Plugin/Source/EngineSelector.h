#pragma once

#include "JuceHeader.h"

/**
 * Engine Selection State Manager for SLOT Mode
 *
 * Manages hardware-based engine browsing and selection for the 6 DSP slots.
 * Used when SW1 is in DOWN position (SLOT mode).
 */
class EngineSelector
{
public:
    // Engine categories matching the 57 engines
    enum class Category {
        DYNAMICS,       // 6 engines (Compressors, Gates, Limiters)
        FILTERS,        // 8 engines (EQs, Filters)
        DISTORTION,     // 8 engines (Tubes, Fuzz, Saturation)
        MODULATION,     // 11 engines (Chorus, Phaser, Pitch)
        REVERB,         // 10 engines (Reverbs, Delays)
        SPATIAL,        // 9 engines (Stereo, Vocoder, Special)
        UTILITY,        // 4 engines (Gain, M/S, Phase)
        NONE            // Empty slot
    };

    // Engine info structure
    struct EngineInfo {
        int id;                 // Engine ID from EngineIDs.h
        juce::String name;      // Display name
        Category category;      // Category it belongs to
    };

    EngineSelector();
    ~EngineSelector() = default;

    // Slot selection (controlled by E1)
    int getCurrentSlot() const { return currentSlot; }
    void setCurrentSlot(int slot);
    void incrementSlot(int delta);

    // Category browsing (controlled by E2)
    Category getCurrentCategory() const { return currentCategory; }
    void setCurrentCategory(Category cat);
    void incrementCategory(int delta);
    juce::String getCategoryName() const;
    int getCategoryEngineCount() const;

    // Engine browsing within category (controlled by E3)
    int getCurrentEngineIndex() const { return currentEngineInCategory; }
    void setCurrentEngineIndex(int index);
    void incrementEngine(int delta);

    // Get selected engine info
    int getSelectedEngineID() const;
    juce::String getSelectedEngineName() const;

    // Get current slot's loaded engine
    void setSlotEngineID(int slot, int engineID);
    int getSlotEngineID(int slot) const;
    juce::String getSlotEngineName(int slot) const;

    // Display helpers
    juce::String getSlotDisplayName(int slot) const;
    juce::String getStatusString() const;

    // Reset to defaults
    void reset();

private:
    // Current selection state
    int currentSlot = 0;                    // 0-5 (6 slots)
    Category currentCategory = Category::DYNAMICS;
    int currentEngineInCategory = 0;        // Index within category

    // Track what's loaded in each slot
    int slotEngines[6] = {0, 0, 0, 0, 0, 0};  // ENGINE_NONE initially

    // Engine database (populated in constructor)
    std::vector<EngineInfo> allEngines;
    std::map<Category, std::vector<int>> enginesByCategory;  // Category -> engine indices

    // Helper to build engine database
    void initializeEngineDatabase();
    std::vector<int> getEnginesInCategory(Category cat) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineSelector)
};

// ============================================================================
// Inline Implementation

inline void EngineSelector::setCurrentSlot(int slot) {
    currentSlot = juce::jlimit(0, 5, slot);
    DBG("EngineSelector: Current slot set to " << (currentSlot + 1));
}

inline void EngineSelector::incrementSlot(int delta) {
    setCurrentSlot(currentSlot + delta);
}

inline void EngineSelector::setCurrentCategory(Category cat) {
    currentCategory = cat;
    currentEngineInCategory = 0;  // Reset to first engine in new category
    DBG("EngineSelector: Category changed to " << getCategoryName());
}

inline void EngineSelector::incrementCategory(int delta) {
    int catIndex = static_cast<int>(currentCategory);
    catIndex = (catIndex + delta + 8) % 8;  // Wrap around (8 categories including NONE)
    setCurrentCategory(static_cast<Category>(catIndex));
}

inline juce::String EngineSelector::getCategoryName() const {
    switch (currentCategory) {
        case Category::DYNAMICS:    return "Dynamics";
        case Category::FILTERS:     return "Filters";
        case Category::DISTORTION:  return "Distortion";
        case Category::MODULATION:  return "Modulation";
        case Category::REVERB:      return "Reverb";
        case Category::SPATIAL:     return "Spatial";
        case Category::UTILITY:     return "Utility";
        case Category::NONE:        return "None";
        default:                    return "Unknown";
    }
}

inline void EngineSelector::setCurrentEngineIndex(int index) {
    auto engines = getEnginesInCategory(currentCategory);
    if (!engines.empty()) {
        currentEngineInCategory = juce::jlimit(0, static_cast<int>(engines.size() - 1), index);
        DBG("EngineSelector: Engine index in category set to " << currentEngineInCategory);
    }
}

inline void EngineSelector::incrementEngine(int delta) {
    setCurrentEngineIndex(currentEngineInCategory + delta);
}

inline int EngineSelector::getSelectedEngineID() const {
    if (currentCategory == Category::NONE) {
        return 0;  // ENGINE_NONE
    }

    auto engines = getEnginesInCategory(currentCategory);
    if (currentEngineInCategory < engines.size()) {
        return allEngines[engines[currentEngineInCategory]].id;
    }
    return 0;
}

inline juce::String EngineSelector::getSelectedEngineName() const {
    if (currentCategory == Category::NONE) {
        return "None";
    }

    auto engines = getEnginesInCategory(currentCategory);
    if (currentEngineInCategory < engines.size()) {
        return allEngines[engines[currentEngineInCategory]].name;
    }
    return "Unknown";
}

inline void EngineSelector::setSlotEngineID(int slot, int engineID) {
    if (slot >= 0 && slot < 6) {
        slotEngines[slot] = engineID;
        DBG("EngineSelector: Slot " << (slot + 1) << " set to engine ID " << engineID);
    }
}

inline int EngineSelector::getSlotEngineID(int slot) const {
    if (slot >= 0 && slot < 6) {
        return slotEngines[slot];
    }
    return 0;
}

inline juce::String EngineSelector::getSlotDisplayName(int slot) const {
    return "Slot " + juce::String(slot + 1);
}

inline juce::String EngineSelector::getStatusString() const {
    return getSlotDisplayName(currentSlot) + " | " +
           getCategoryName() + " | " +
           getSelectedEngineName();
}

inline std::vector<int> EngineSelector::getEnginesInCategory(Category cat) const {
    auto it = enginesByCategory.find(cat);
    if (it != enginesByCategory.end()) {
        return it->second;
    }
    return {};
}

inline void EngineSelector::reset() {
    currentSlot = 0;
    currentCategory = Category::DYNAMICS;
    currentEngineInCategory = 0;
    for (int i = 0; i < 6; ++i) {
        slotEngines[i] = 0;  // ENGINE_NONE
    }
    DBG("EngineSelector: Reset to defaults");
}