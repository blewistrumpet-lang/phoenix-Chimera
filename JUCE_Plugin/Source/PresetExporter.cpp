#include "PresetExporter.h"

// Static registry storage
std::map<String, PresetRegistry::PresetCreator>& PresetRegistry::getRegistry()
{
    static std::map<String, PresetCreator> registry;
    return registry;
}

//==============================================================================
// PresetExporter Implementation

String PresetExporter::exportPresetToJson(const GoldenPreset& preset, bool prettyPrint)
{
    // Use JUCE's var and JSON for proper serialization
    var presetJson;
    
    // Basic info
    presetJson.getDynamicObject()->setProperty("id", preset.id);
    presetJson.getDynamicObject()->setProperty("name", preset.name);
    presetJson.getDynamicObject()->setProperty("technicalHint", preset.technicalHint);
    presetJson.getDynamicObject()->setProperty("shortCode", preset.shortCode);
    presetJson.getDynamicObject()->setProperty("category", preset.category);
    presetJson.getDynamicObject()->setProperty("subcategory", preset.subcategory);
    presetJson.getDynamicObject()->setProperty("version", preset.version);
    
    // Hierarchy
    presetJson.getDynamicObject()->setProperty("isVariation", preset.isVariation);
    presetJson.getDynamicObject()->setProperty("parentId", preset.parentId);
    
    // Engines array
    var enginesArray;
    for (int i = 0; i < 6; ++i)
    {
        if (preset.engineTypes[i] >= 0 && preset.engineActive[i])
        {
            enginesArray.append(engineToJson(preset, i));
        }
    }
    presetJson.getDynamicObject()->setProperty("engines", enginesArray);
    
    // Profiles
    presetJson.getDynamicObject()->setProperty("sonicProfile", sonicProfileToJson(preset.sonicProfile));
    presetJson.getDynamicObject()->setProperty("emotionalProfile", emotionalProfileToJson(preset.emotionalProfile));
    presetJson.getDynamicObject()->setProperty("sourceAffinity", sourceAffinityToJson(preset.sourceAffinity));
    
    // Performance
    presetJson.getDynamicObject()->setProperty("cpuTier", cpuTierToString(preset.cpuTier));
    presetJson.getDynamicObject()->setProperty("actualCpuPercent", preset.actualCpuPercent);
    presetJson.getDynamicObject()->setProperty("latencySamples", preset.latencySamples);
    presetJson.getDynamicObject()->setProperty("realtimeSafe", preset.realtimeSafe);
    
    // Musical context
    presetJson.getDynamicObject()->setProperty("optimalTempo", preset.optimalTempo);
    presetJson.getDynamicObject()->setProperty("musicalKey", preset.musicalKey);
    presetJson.getDynamicObject()->setProperty("genres", stringArrayToJson(preset.genres));
    
    // Quality metrics
    presetJson.getDynamicObject()->setProperty("signature", preset.signature);
    presetJson.getDynamicObject()->setProperty("creationDate", preset.creationDate.toISO8601(true));
    presetJson.getDynamicObject()->setProperty("popularityScore", preset.popularityScore);
    presetJson.getDynamicObject()->setProperty("qualityScore", preset.qualityScore);
    
    // Complexity metrics
    presetJson.getDynamicObject()->setProperty("complexity", preset.complexity);
    presetJson.getDynamicObject()->setProperty("experimentalness", preset.experimentalness);
    presetJson.getDynamicObject()->setProperty("versatility", preset.versatility);
    
    // Searchability
    presetJson.getDynamicObject()->setProperty("keywords", stringArrayToJson(preset.keywords));
    presetJson.getDynamicObject()->setProperty("antiFeatures", stringArrayToJson(preset.antiFeatures));
    presetJson.getDynamicObject()->setProperty("userPrompts", stringArrayToJson(preset.userPrompts));
    
    // Usage hints
    presetJson.getDynamicObject()->setProperty("bestFor", preset.bestFor);
    presetJson.getDynamicObject()->setProperty("avoidFor", preset.avoidFor);
    
    // Convert to JSON string
    return JSON::toString(presetJson, prettyPrint);
}

var PresetExporter::engineToJson(const GoldenPreset& preset, int slotIndex)
{
    var engine;
    engine.getDynamicObject()->setProperty("slot", slotIndex);
    engine.getDynamicObject()->setProperty("type", preset.engineTypes[slotIndex]);
    engine.getDynamicObject()->setProperty("typeName", getEngineTypeNames()[preset.engineTypes[slotIndex]]);
    engine.getDynamicObject()->setProperty("mix", preset.engineMix[slotIndex]);
    engine.getDynamicObject()->setProperty("active", preset.engineActive[slotIndex]);
    
    // Parameters array
    var paramsArray;
    for (float param : preset.engineParams[slotIndex])
    {
        paramsArray.append(param);
    }
    engine.getDynamicObject()->setProperty("params", paramsArray);
    
    return engine;
}

var PresetExporter::sonicProfileToJson(const SonicProfile& profile)
{
    var json;
    json.getDynamicObject()->setProperty("brightness", profile.brightness);
    json.getDynamicObject()->setProperty("density", profile.density);
    json.getDynamicObject()->setProperty("movement", profile.movement);
    json.getDynamicObject()->setProperty("space", profile.space);
    json.getDynamicObject()->setProperty("aggression", profile.aggression);
    json.getDynamicObject()->setProperty("vintage", profile.vintage);
    return json;
}

var PresetExporter::emotionalProfileToJson(const EmotionalProfile& profile)
{
    var json;
    json.getDynamicObject()->setProperty("energy", profile.energy);
    json.getDynamicObject()->setProperty("mood", profile.mood);
    json.getDynamicObject()->setProperty("tension", profile.tension);
    json.getDynamicObject()->setProperty("organic", profile.organic);
    json.getDynamicObject()->setProperty("nostalgia", profile.nostalgia);
    return json;
}

var PresetExporter::sourceAffinityToJson(const SourceAffinity& affinity)
{
    var json;
    json.getDynamicObject()->setProperty("vocals", affinity.vocals);
    json.getDynamicObject()->setProperty("guitar", affinity.guitar);
    json.getDynamicObject()->setProperty("drums", affinity.drums);
    json.getDynamicObject()->setProperty("synth", affinity.synth);
    json.getDynamicObject()->setProperty("mix", affinity.mix);
    return json;
}

var PresetExporter::stringArrayToJson(const std::vector<String>& strings)
{
    var array;
    for (const auto& str : strings)
    {
        array.append(str);
    }
    return array;
}

String PresetExporter::cpuTierToString(CPUTier tier)
{
    switch (tier)
    {
        case CPUTier::LIGHT:   return "LIGHT";
        case CPUTier::MEDIUM:  return "MEDIUM";
        case CPUTier::HEAVY:   return "HEAVY";
        case CPUTier::EXTREME: return "EXTREME";
        default:               return "UNKNOWN";
    }
}

StringArray PresetExporter::getEngineTypeNames()
{
    StringArray names;
    names.resize(ENGINE_COUNT);
    
    // Use the unified getEngineTypeName function from EngineTypes.h
    for (int i = 0; i < ENGINE_COUNT; ++i)
    {
        names.set(i, getEngineTypeName(i));
    }
    
    return names;
}

int PresetExporter::getEngineTypeFromName(const String& engineName)
{
    StringArray names = getEngineTypeNames();
    for (int i = 0; i < names.size(); ++i)
    {
        if (names[i] == engineName)
            return i;
    }
    return -1;
}

int PresetExporter::exportPresetsToDirectory(const std::vector<GoldenPreset>& presets, 
                                            const File& outputDirectory)
{
    outputDirectory.createDirectory();
    
    File presetsDir = outputDirectory.getChildFile("presets");
    presetsDir.createDirectory();
    
    int successCount = 0;
    
    for (const auto& preset : presets)
    {
        String json = exportPresetToJson(preset, true);
        File outputFile = presetsDir.getChildFile(preset.id + ".json");
        
        if (outputFile.replaceWithText(json))
        {
            successCount++;
            DBG("Exported: " + preset.id + " - " + preset.name);
        }
        else
        {
            DBG("Failed to export: " + preset.id);
        }
    }
    
    // Create metadata
    createCorpusMetadata(presets, outputDirectory);
    
    return successCount;
}

bool PresetExporter::exportPresetsToSingleFile(const std::vector<GoldenPreset>& presets,
                                              const File& outputFile)
{
    var presetsArray;
    
    for (const auto& preset : presets)
    {
        String jsonStr = exportPresetToJson(preset, false);
        var presetVar = JSON::parse(jsonStr);
        if (presetVar.isObject())
        {
            presetsArray.append(presetVar);
        }
    }
    
    var root;
    root.getDynamicObject()->setProperty("version", "1.0");
    root.getDynamicObject()->setProperty("presetCount", (int)presets.size());
    root.getDynamicObject()->setProperty("presets", presetsArray);
    
    String jsonOutput = JSON::toString(root, true);
    return outputFile.replaceWithText(jsonOutput);
}

void PresetExporter::createCorpusMetadata(const std::vector<GoldenPreset>& presets,
                                         const File& outputDirectory)
{
    var metadata;
    
    // Basic info
    metadata.getDynamicObject()->setProperty("version", "1.0");
    metadata.getDynamicObject()->setProperty("exportDate", Time::getCurrentTime().toISO8601(true));
    metadata.getDynamicObject()->setProperty("presetCount", (int)presets.size());
    
    // Category distribution
    std::map<String, int> categoryCount;
    std::map<String, int> cpuTierCount;
    std::set<String> allKeywords;
    std::set<String> allGenres;
    
    for (const auto& preset : presets)
    {
        categoryCount[preset.category]++;
        cpuTierCount[cpuTierToString(preset.cpuTier)]++;
        
        for (const auto& keyword : preset.keywords)
            allKeywords.insert(keyword);
        
        for (const auto& genre : preset.genres)
            allGenres.insert(genre);
    }
    
    // Convert to JSON
    var categories;
    for (const auto& [cat, count] : categoryCount)
    {
        var catObj;
        catObj.getDynamicObject()->setProperty("name", cat);
        catObj.getDynamicObject()->setProperty("count", count);
        categories.append(catObj);
    }
    metadata.getDynamicObject()->setProperty("categories", categories);
    
    var cpuTiers;
    for (const auto& [tier, count] : cpuTierCount)
    {
        var tierObj;
        tierObj.getDynamicObject()->setProperty("tier", tier);
        tierObj.getDynamicObject()->setProperty("count", count);
        cpuTiers.append(tierObj);
    }
    metadata.getDynamicObject()->setProperty("cpuTiers", cpuTiers);
    
    // Keywords and genres
    var keywordsArray;
    for (const auto& kw : allKeywords)
        keywordsArray.append(kw);
    metadata.getDynamicObject()->setProperty("allKeywords", keywordsArray);
    
    var genresArray;
    for (const auto& genre : allGenres)
        genresArray.append(genre);
    metadata.getDynamicObject()->setProperty("allGenres", genresArray);
    
    // Engine usage statistics
    std::map<int, int> engineUsage;
    for (const auto& preset : presets)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (preset.engineTypes[i] >= 0 && preset.engineActive[i])
            {
                engineUsage[preset.engineTypes[i]]++;
            }
        }
    }
    
    var engineStats;
    StringArray engineNames = getEngineTypeNames();
    for (const auto& [engineType, count] : engineUsage)
    {
        var engineObj;
        engineObj.getDynamicObject()->setProperty("type", engineType);
        engineObj.getDynamicObject()->setProperty("name", engineNames[engineType]);
        engineObj.getDynamicObject()->setProperty("usageCount", count);
        engineStats.append(engineObj);
    }
    metadata.getDynamicObject()->setProperty("engineUsage", engineStats);
    
    // Save metadata
    File metadataFile = outputDirectory.getChildFile("corpus_metadata.json");
    String metadataJson = JSON::toString(metadata, true);
    metadataFile.replaceWithText(metadataJson);
}

//==============================================================================
// PresetRegistry Implementation

void PresetRegistry::registerPreset(const String& id, PresetCreator creator)
{
    getRegistry()[id] = creator;
}

StringArray PresetRegistry::getAllPresetIds()
{
    StringArray ids;
    for (const auto& [id, creator] : getRegistry())
    {
        ids.add(id);
    }
    ids.sort(false);
    return ids;
}

GoldenPreset PresetRegistry::createPreset(const String& id)
{
    auto& registry = getRegistry();
    auto it = registry.find(id);
    
    if (it != registry.end())
    {
        return it->second();
    }
    
    // Return empty preset if not found
    GoldenPreset empty;
    empty.id = "INVALID";
    empty.name = "Not Found";
    return empty;
}

std::vector<GoldenPreset> PresetRegistry::createAllPresets()
{
    std::vector<GoldenPreset> presets;
    
    for (const auto& [id, creator] : getRegistry())
    {
        try
        {
            presets.push_back(creator());
        }
        catch (const std::exception& e)
        {
            DBG("Error creating preset " + id + ": " + String(e.what()));
        }
    }
    
    return presets;
}

bool PresetRegistry::hasPreset(const String& id)
{
    return getRegistry().find(id) != getRegistry().end();
}

int PresetRegistry::getPresetCount()
{
    return (int)getRegistry().size();
}

void PresetRegistry::clearRegistry()
{
    getRegistry().clear();
}