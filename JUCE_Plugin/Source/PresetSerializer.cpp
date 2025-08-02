#include "PresetSerializer.h"

juce::var PresetSerializer::presetToJSON(const GoldenPreset& preset) {
    auto* obj = new DynamicObject();
    
    // Identification
    obj->setProperty("id", preset.id);
    obj->setProperty("name", preset.name);
    obj->setProperty("technicalHint", preset.technicalHint);
    obj->setProperty("shortCode", preset.shortCode);
    obj->setProperty("version", preset.version);
    
    // Hierarchy
    obj->setProperty("isVariation", preset.isVariation);
    obj->setProperty("parentId", preset.parentId);
    
    // Engine configuration
    auto engines = Array<var>();
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0) {
            auto* engine = new DynamicObject();
            engine->setProperty("slot", i);
            engine->setProperty("type", preset.engineTypes[i]);
            engine->setProperty("mix", preset.engineMix[i]);
            engine->setProperty("active", preset.engineActive[i]);
            engine->setProperty("parameters", engineParamsToJSON(preset.engineParams[i]));
            engines.add(var(engine));
        }
    }
    obj->setProperty("engines", engines);
    
    // AI Metadata
    obj->setProperty("sonicProfile", sonicProfileToJSON(preset.sonicProfile));
    obj->setProperty("emotionalProfile", emotionalProfileToJSON(preset.emotionalProfile));
    obj->setProperty("sourceAffinity", sourceAffinityToJSON(preset.sourceAffinity));
    
    // Performance
    obj->setProperty("cpuTier", static_cast<int>(preset.cpuTier));
    obj->setProperty("actualCpuPercent", preset.actualCpuPercent);
    obj->setProperty("latencySamples", preset.latencySamples);
    obj->setProperty("realtimeSafe", preset.realtimeSafe);
    
    // Musical context
    obj->setProperty("optimalTempo", preset.optimalTempo);
    obj->setProperty("musicalKey", preset.musicalKey);
    
    auto genres = Array<var>();
    for (const auto& genre : preset.genres) {
        genres.add(genre);
    }
    obj->setProperty("genres", genres);
    
    // Quality & tracking
    obj->setProperty("signature", preset.signature);
    obj->setProperty("creationDate", preset.creationDate.toISO8601(true));
    obj->setProperty("popularityScore", preset.popularityScore);
    obj->setProperty("qualityScore", preset.qualityScore);
    
    // Searchability
    auto keywords = Array<var>();
    for (const auto& keyword : preset.keywords) {
        keywords.add(keyword);
    }
    obj->setProperty("keywords", keywords);
    
    auto antiFeatures = Array<var>();
    for (const auto& anti : preset.antiFeatures) {
        antiFeatures.add(anti);
    }
    obj->setProperty("antiFeatures", antiFeatures);
    
    auto userPrompts = Array<var>();
    for (const auto& prompt : preset.userPrompts) {
        userPrompts.add(prompt);
    }
    obj->setProperty("userPrompts", userPrompts);
    
    // Category
    obj->setProperty("category", preset.category);
    obj->setProperty("subcategory", preset.subcategory);
    
    // Complexity metrics
    obj->setProperty("complexity", preset.complexity);
    obj->setProperty("experimentalness", preset.experimentalness);
    obj->setProperty("versatility", preset.versatility);
    
    // Usage hints
    obj->setProperty("bestFor", preset.bestFor);
    obj->setProperty("avoidFor", preset.avoidFor);
    
    return var(obj);
}

GoldenPreset PresetSerializer::presetFromJSON(const juce::var& json) {
    GoldenPreset preset;
    
    if (auto* obj = json.getDynamicObject()) {
        // Identification
        preset.id = obj->getProperty("id").toString();
        preset.name = obj->getProperty("name").toString();
        preset.technicalHint = obj->getProperty("technicalHint").toString();
        preset.shortCode = obj->getProperty("shortCode").toString();
        preset.version = obj->getProperty("version");
        
        // Hierarchy
        preset.isVariation = obj->getProperty("isVariation");
        preset.parentId = obj->getProperty("parentId").toString();
        
        // Engine configuration
        preset.engineTypes.fill(-1);
        preset.engineMix.fill(0.0f);
        preset.engineActive.fill(false);
        
        if (auto* engines = obj->getProperty("engines").getArray()) {
            for (const auto& engineVar : *engines) {
                if (auto* engine = engineVar.getDynamicObject()) {
                    int slot = engine->getProperty("slot");
                    if (slot >= 0 && slot < 6) {
                        preset.engineTypes[slot] = engine->getProperty("type");
                        preset.engineMix[slot] = engine->getProperty("mix");
                        preset.engineActive[slot] = engine->getProperty("active");
                        preset.engineParams[slot] = engineParamsFromJSON(engine->getProperty("parameters"));
                    }
                }
            }
        }
        
        // AI Metadata
        preset.sonicProfile = sonicProfileFromJSON(obj->getProperty("sonicProfile"));
        preset.emotionalProfile = emotionalProfileFromJSON(obj->getProperty("emotionalProfile"));
        preset.sourceAffinity = sourceAffinityFromJSON(obj->getProperty("sourceAffinity"));
        
        // Performance
        preset.cpuTier = static_cast<CPUTier>(int(obj->getProperty("cpuTier")));
        preset.actualCpuPercent = obj->getProperty("actualCpuPercent");
        preset.latencySamples = obj->getProperty("latencySamples");
        preset.realtimeSafe = obj->getProperty("realtimeSafe");
        
        // Musical context
        preset.optimalTempo = obj->getProperty("optimalTempo");
        preset.musicalKey = obj->getProperty("musicalKey").toString();
        
        preset.genres.clear();
        if (auto* genres = obj->getProperty("genres").getArray()) {
            for (const auto& genre : *genres) {
                preset.genres.push_back(genre.toString());
            }
        }
        
        // Quality & tracking
        preset.signature = obj->getProperty("signature").toString();
        preset.creationDate = Time::fromISO8601(obj->getProperty("creationDate").toString());
        preset.popularityScore = obj->getProperty("popularityScore");
        preset.qualityScore = obj->getProperty("qualityScore");
        
        // Searchability
        preset.keywords.clear();
        if (auto* keywords = obj->getProperty("keywords").getArray()) {
            for (const auto& keyword : *keywords) {
                preset.keywords.push_back(keyword.toString());
            }
        }
        
        preset.antiFeatures.clear();
        if (auto* antiFeatures = obj->getProperty("antiFeatures").getArray()) {
            for (const auto& anti : *antiFeatures) {
                preset.antiFeatures.push_back(anti.toString());
            }
        }
        
        preset.userPrompts.clear();
        if (auto* prompts = obj->getProperty("userPrompts").getArray()) {
            for (const auto& prompt : *prompts) {
                preset.userPrompts.push_back(prompt.toString());
            }
        }
        
        // Category
        preset.category = obj->getProperty("category").toString();
        preset.subcategory = obj->getProperty("subcategory").toString();
        
        // Complexity metrics
        preset.complexity = obj->getProperty("complexity");
        preset.experimentalness = obj->getProperty("experimentalness");
        preset.versatility = obj->getProperty("versatility");
        
        // Usage hints
        preset.bestFor = obj->getProperty("bestFor").toString();
        preset.avoidFor = obj->getProperty("avoidFor").toString();
    }
    
    return preset;
}

bool PresetSerializer::savePresetToFile(const GoldenPreset& preset, const File& file) {
    auto json = presetToJSON(preset);
    auto jsonString = JSON::toString(json, true);  // Pretty print
    return file.replaceWithText(jsonString);
}

GoldenPreset PresetSerializer::loadPresetFromFile(const File& file) {
    if (!file.existsAsFile()) {
        return GoldenPreset();  // Return default preset
    }
    
    auto jsonString = file.loadFileAsString();
    auto json = JSON::parse(jsonString);
    
    if (json.isVoid()) {
        return GoldenPreset();  // Parse error
    }
    
    return presetFromJSON(json);
}

bool PresetSerializer::saveCorpusToJSON(const std::vector<GoldenPreset>& corpus, const File& file) {
    auto* root = new DynamicObject();
    root->setProperty("version", BINARY_FORMAT_VERSION);
    root->setProperty("presetCount", static_cast<int>(corpus.size()));
    root->setProperty("timestamp", Time::getCurrentTime().toISO8601(true));
    
    auto presets = Array<var>();
    for (const auto& preset : corpus) {
        presets.add(presetToJSON(preset));
    }
    root->setProperty("presets", presets);
    
    auto jsonString = JSON::toString(var(root), true);
    return file.replaceWithText(jsonString);
}

std::vector<GoldenPreset> PresetSerializer::loadCorpusFromJSON(const File& file) {
    std::vector<GoldenPreset> corpus;
    
    if (!file.existsAsFile()) {
        return corpus;
    }
    
    auto jsonString = file.loadFileAsString();
    auto json = JSON::parse(jsonString);
    
    if (auto* root = json.getDynamicObject()) {
        if (auto* presets = root->getProperty("presets").getArray()) {
            for (const auto& presetVar : *presets) {
                corpus.push_back(presetFromJSON(presetVar));
            }
        }
    }
    
    return corpus;
}

// Helper method implementations
juce::var PresetSerializer::sonicProfileToJSON(const SonicProfile& profile) {
    auto* obj = new DynamicObject();
    obj->setProperty("brightness", profile.brightness);
    obj->setProperty("density", profile.density);
    obj->setProperty("movement", profile.movement);
    obj->setProperty("space", profile.space);
    obj->setProperty("aggression", profile.aggression);
    obj->setProperty("vintage", profile.vintage);
    return var(obj);
}

SonicProfile PresetSerializer::sonicProfileFromJSON(const juce::var& json) {
    SonicProfile profile;
    if (auto* obj = json.getDynamicObject()) {
        profile.brightness = obj->getProperty("brightness");
        profile.density = obj->getProperty("density");
        profile.movement = obj->getProperty("movement");
        profile.space = obj->getProperty("space");
        profile.aggression = obj->getProperty("aggression");
        profile.vintage = obj->getProperty("vintage");
    }
    return profile;
}

juce::var PresetSerializer::emotionalProfileToJSON(const EmotionalProfile& profile) {
    auto* obj = new DynamicObject();
    obj->setProperty("energy", profile.energy);
    obj->setProperty("mood", profile.mood);
    obj->setProperty("tension", profile.tension);
    obj->setProperty("organic", profile.organic);
    obj->setProperty("nostalgia", profile.nostalgia);
    return var(obj);
}

EmotionalProfile PresetSerializer::emotionalProfileFromJSON(const juce::var& json) {
    EmotionalProfile profile;
    if (auto* obj = json.getDynamicObject()) {
        profile.energy = obj->getProperty("energy");
        profile.mood = obj->getProperty("mood");
        profile.tension = obj->getProperty("tension");
        profile.organic = obj->getProperty("organic");
        profile.nostalgia = obj->getProperty("nostalgia");
    }
    return profile;
}

juce::var PresetSerializer::sourceAffinityToJSON(const SourceAffinity& affinity) {
    auto* obj = new DynamicObject();
    obj->setProperty("vocals", affinity.vocals);
    obj->setProperty("guitar", affinity.guitar);
    obj->setProperty("drums", affinity.drums);
    obj->setProperty("synth", affinity.synth);
    obj->setProperty("mix", affinity.mix);
    return var(obj);
}

SourceAffinity PresetSerializer::sourceAffinityFromJSON(const juce::var& json) {
    SourceAffinity affinity;
    if (auto* obj = json.getDynamicObject()) {
        affinity.vocals = obj->getProperty("vocals");
        affinity.guitar = obj->getProperty("guitar");
        affinity.drums = obj->getProperty("drums");
        affinity.synth = obj->getProperty("synth");
        affinity.mix = obj->getProperty("mix");
    }
    return affinity;
}

juce::var PresetSerializer::engineParamsToJSON(const std::vector<float>& params) {
    auto array = Array<var>();
    for (float param : params) {
        array.add(param);
    }
    return array;
}

std::vector<float> PresetSerializer::engineParamsFromJSON(const juce::var& json) {
    std::vector<float> params;
    if (auto* array = json.getArray()) {
        for (const auto& param : *array) {
            params.push_back(param);
        }
    }
    return params;
}

bool PresetSerializer::validatePresetJSON(const juce::var& json, String& errorMessage) {
    if (!json.getDynamicObject()) {
        errorMessage = "Invalid JSON structure";
        return false;
    }
    
    auto* obj = json.getDynamicObject();
    
    // Check required fields
    if (obj->getProperty("id").toString().isEmpty()) {
        errorMessage = "Missing preset ID";
        return false;
    }
    
    if (obj->getProperty("name").toString().isEmpty()) {
        errorMessage = "Missing preset name";
        return false;
    }
    
    // Validate engines
    if (auto* engines = obj->getProperty("engines").getArray()) {
        if (engines->size() == 0) {
            errorMessage = "Preset must have at least one engine";
            return false;
        }
        
        for (const auto& engineVar : *engines) {
            if (auto* engine = engineVar.getDynamicObject()) {
                int type = engine->getProperty("type");
                if (type < 0 || type >= 50) {
                    errorMessage = "Invalid engine type: " + String(type);
                    return false;
                }
            }
        }
    } else {
        errorMessage = "Missing engines array";
        return false;
    }
    
    return true;
}