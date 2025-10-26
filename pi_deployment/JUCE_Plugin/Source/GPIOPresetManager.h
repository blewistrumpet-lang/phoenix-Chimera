#pragma once

#include "JuceHeader.h"
#include "ABStateEngine.h"

/**
 * GPIO Preset Manager - Simple 10-Slot RAM Presets
 * Week 2 Phase 2 of Trinity GPIO Plan
 *
 * Lightweight preset manager specifically for GPIO hardware control.
 * Manages 10 preset slots in RAM with JSON persistence.
 * Each preset stores both A and B parameter banks.
 *
 * Features:
 * - 10 preset slots (0-9)
 * - Browse with E1 in PRESET mode
 * - Load with E1 button press
 * - Quick save with E2 button press
 * - Atomic JSON save (temp file → rename)
 * - Preset index cache for reboot persistence
 */
class GPIOPresetManager
{
public:
    // Single preset - stores complete state including both A/B banks
    struct Preset {
        juce::String name = "Empty";
        ABStateEngine::ParamBank bankA;
        ABStateEngine::ParamBank bankB;
        bool valid = false;  // false = empty slot

        Preset() {
            // Initialize with default values
            bankA.input_gain = 1.0f;
            bankA.mix_wetdry = 0.5f;
            bankA.output_level = 1.0f;

            bankB.input_gain = 1.0f;
            bankB.mix_wetdry = 0.5f;
            bankB.output_level = 1.0f;
        }
    };

    GPIOPresetManager();
    ~GPIOPresetManager() = default;

    // Preset browsing and selection
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    void setCurrentPresetIndex(int index);

    // Get preset info
    const Preset& getPreset(int index) const;
    juce::String getPresetName(int index) const;
    bool isPresetValid(int index) const;

    // Save current state to a preset slot
    void savePreset(int index,
                    const juce::String& name,
                    const ABStateEngine::ParamBank& bankA,
                    const ABStateEngine::ParamBank& bankB);

    // Load preset (returns false if slot is empty)
    bool loadPreset(int index,
                    ABStateEngine::ParamBank& outBankA,
                    ABStateEngine::ParamBank& outBankB);

    // Clear a preset slot
    void clearPreset(int index);

    // JSON persistence
    bool loadFromJSON(const juce::File& file);
    bool saveToJSON(const juce::File& file);

    // Preset index cache (remembers last loaded preset)
    bool loadPresetIndexCache(const juce::File& file);
    bool savePresetIndexCache(const juce::File& file);

    // Generate default preset name
    static juce::String generatePresetName(int index);

private:
    static constexpr int NUM_PRESET_SLOTS = 10;
    Preset presets[NUM_PRESET_SLOTS];
    int currentPresetIndex = 0;

    // Validate preset index
    bool isValidIndex(int index) const {
        return index >= 0 && index < NUM_PRESET_SLOTS;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GPIOPresetManager)
};

// Implementation

inline GPIOPresetManager::GPIOPresetManager()
{
    // Initialize all slots as empty
    for (int i = 0; i < NUM_PRESET_SLOTS; ++i) {
        presets[i].name = generatePresetName(i);
        presets[i].valid = false;
    }

    DBG("GPIOPresetManager: Initialized with " << NUM_PRESET_SLOTS << " empty slots");
}

inline void GPIOPresetManager::setCurrentPresetIndex(int index)
{
    if (!isValidIndex(index)) {
        DBG("GPIOPresetManager: Invalid preset index " << index);
        return;
    }

    currentPresetIndex = index;
    DBG("GPIOPresetManager: Current preset index set to " << index);
}

inline const GPIOPresetManager::Preset& GPIOPresetManager::getPreset(int index) const
{
    static Preset emptyPreset;
    if (!isValidIndex(index)) {
        return emptyPreset;
    }
    return presets[index];
}

inline juce::String GPIOPresetManager::getPresetName(int index) const
{
    if (!isValidIndex(index)) {
        return "Invalid";
    }
    return presets[index].name;
}

inline bool GPIOPresetManager::isPresetValid(int index) const
{
    if (!isValidIndex(index)) {
        return false;
    }
    return presets[index].valid;
}

inline void GPIOPresetManager::savePreset(int index,
                                          const juce::String& name,
                                          const ABStateEngine::ParamBank& bankA,
                                          const ABStateEngine::ParamBank& bankB)
{
    if (!isValidIndex(index)) {
        DBG("GPIOPresetManager: Cannot save to invalid index " << index);
        return;
    }

    presets[index].name = name.isEmpty() ? generatePresetName(index) : name;
    presets[index].bankA = bankA;
    presets[index].bankB = bankB;
    presets[index].valid = true;

    DBG("GPIOPresetManager: Saved preset " << index << " - '" << presets[index].name << "'");
    DBG("  Bank A: gain=" << bankA.input_gain << " mix=" << bankA.mix_wetdry << " out=" << bankA.output_level);
    DBG("  Bank B: gain=" << bankB.input_gain << " mix=" << bankB.mix_wetdry << " out=" << bankB.output_level);
}

inline bool GPIOPresetManager::loadPreset(int index,
                                          ABStateEngine::ParamBank& outBankA,
                                          ABStateEngine::ParamBank& outBankB)
{
    if (!isValidIndex(index)) {
        DBG("GPIOPresetManager: Cannot load from invalid index " << index);
        return false;
    }

    if (!presets[index].valid) {
        DBG("GPIOPresetManager: Preset slot " << index << " is empty");
        return false;
    }

    outBankA = presets[index].bankA;
    outBankB = presets[index].bankB;

    DBG("GPIOPresetManager: Loaded preset " << index << " - '" << presets[index].name << "'");
    DBG("  Bank A: gain=" << outBankA.input_gain << " mix=" << outBankA.mix_wetdry << " out=" << outBankA.output_level);
    DBG("  Bank B: gain=" << outBankB.input_gain << " mix=" << outBankB.mix_wetdry << " out=" << outBankB.output_level);

    return true;
}

inline void GPIOPresetManager::clearPreset(int index)
{
    if (!isValidIndex(index)) {
        return;
    }

    presets[index].name = generatePresetName(index);
    presets[index].valid = false;
    DBG("GPIOPresetManager: Cleared preset " << index);
}

inline bool GPIOPresetManager::loadFromJSON(const juce::File& file)
{
    if (!file.existsAsFile()) {
        DBG("GPIOPresetManager: Preset file does not exist: " << file.getFullPathName());
        return false;
    }

    try {
        juce::String jsonText = file.loadFileAsString();
        auto jsonVar = juce::JSON::parse(jsonText);

        if (!jsonVar.isObject()) {
            DBG("GPIOPresetManager: Invalid JSON format");
            return false;
        }

        auto root = jsonVar.getDynamicObject();
        auto presetsArray = root->getProperty("presets");

        if (!presetsArray.isArray()) {
            DBG("GPIOPresetManager: No presets array found");
            return false;
        }

        auto* arr = presetsArray.getArray();
        int numLoaded = 0;

        for (int i = 0; i < juce::jmin(NUM_PRESET_SLOTS, arr->size()); ++i) {
            auto presetObj = (*arr)[i].getDynamicObject();
            if (presetObj == nullptr) continue;

            presets[i].name = presetObj->getProperty("name").toString();
            presets[i].valid = presetObj->getProperty("valid");

            // Load Bank A
            auto bankAObj = presetObj->getProperty("bankA").getDynamicObject();
            if (bankAObj) {
                presets[i].bankA.input_gain = bankAObj->getProperty("input_gain");
                presets[i].bankA.mix_wetdry = bankAObj->getProperty("mix_wetdry");
                presets[i].bankA.output_level = bankAObj->getProperty("output_level");
            }

            // Load Bank B
            auto bankBObj = presetObj->getProperty("bankB").getDynamicObject();
            if (bankBObj) {
                presets[i].bankB.input_gain = bankBObj->getProperty("input_gain");
                presets[i].bankB.mix_wetdry = bankBObj->getProperty("mix_wetdry");
                presets[i].bankB.output_level = bankBObj->getProperty("output_level");
            }

            if (presets[i].valid) {
                numLoaded++;
            }
        }

        DBG("GPIOPresetManager: Loaded " << numLoaded << " presets from " << file.getFullPathName());
        return true;
    }
    catch (...) {
        DBG("GPIOPresetManager: Exception loading JSON");
        return false;
    }
}

inline bool GPIOPresetManager::saveToJSON(const juce::File& file)
{
    try {
        // Create parent directory if needed
        file.getParentDirectory().createDirectory();

        // Build JSON structure
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        juce::Array<juce::var> presetsArray;

        for (int i = 0; i < NUM_PRESET_SLOTS; ++i) {
            juce::DynamicObject::Ptr presetObj = new juce::DynamicObject();
            presetObj->setProperty("name", presets[i].name);
            presetObj->setProperty("valid", presets[i].valid);

            // Save Bank A
            juce::DynamicObject::Ptr bankAObj = new juce::DynamicObject();
            bankAObj->setProperty("input_gain", presets[i].bankA.input_gain);
            bankAObj->setProperty("mix_wetdry", presets[i].bankA.mix_wetdry);
            bankAObj->setProperty("output_level", presets[i].bankA.output_level);
            presetObj->setProperty("bankA", juce::var(bankAObj.get()));

            // Save Bank B
            juce::DynamicObject::Ptr bankBObj = new juce::DynamicObject();
            bankBObj->setProperty("input_gain", presets[i].bankB.input_gain);
            bankBObj->setProperty("mix_wetdry", presets[i].bankB.mix_wetdry);
            bankBObj->setProperty("output_level", presets[i].bankB.output_level);
            presetObj->setProperty("bankB", juce::var(bankBObj.get()));

            presetsArray.add(juce::var(presetObj.get()));
        }

        root->setProperty("presets", presetsArray);
        juce::var jsonVar(root.get());
        juce::String jsonText = juce::JSON::toString(jsonVar, true);

        // Atomic save: write to temp file first
        juce::File tempFile = file.getSiblingFile(file.getFileNameWithoutExtension() + "_tmp.json");

        if (!tempFile.replaceWithText(jsonText)) {
            DBG("GPIOPresetManager: Failed to write temp file");
            return false;
        }

        // Atomic rename
        if (!tempFile.moveFileTo(file)) {
            DBG("GPIOPresetManager: Failed to move temp file to final location");
            return false;
        }

        DBG("GPIOPresetManager: Saved presets to " << file.getFullPathName());
        return true;
    }
    catch (...) {
        DBG("GPIOPresetManager: Exception saving JSON");
        return false;
    }
}

inline bool GPIOPresetManager::loadPresetIndexCache(const juce::File& file)
{
    if (!file.existsAsFile()) {
        return false;
    }

    try {
        juce::String jsonText = file.loadFileAsString();
        auto jsonVar = juce::JSON::parse(jsonText);

        if (!jsonVar.isObject()) {
            return false;
        }

        auto root = jsonVar.getDynamicObject();
        int cachedIndex = root->getProperty("last_preset_index");

        if (isValidIndex(cachedIndex)) {
            currentPresetIndex = cachedIndex;
            DBG("GPIOPresetManager: Loaded cached preset index: " << cachedIndex);
            return true;
        }
    }
    catch (...) {
        DBG("GPIOPresetManager: Exception loading preset index cache");
    }

    return false;
}

inline bool GPIOPresetManager::savePresetIndexCache(const juce::File& file)
{
    try {
        file.getParentDirectory().createDirectory();

        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("last_preset_index", currentPresetIndex);

        juce::var jsonVar(root.get());
        juce::String jsonText = juce::JSON::toString(jsonVar, true);

        if (file.replaceWithText(jsonText)) {
            DBG("GPIOPresetManager: Saved preset index cache: " << currentPresetIndex);
            return true;
        }
    }
    catch (...) {
        DBG("GPIOPresetManager: Exception saving preset index cache");
    }

    return false;
}

inline juce::String GPIOPresetManager::generatePresetName(int index)
{
    return "Preset " + juce::String(index + 1);
}
