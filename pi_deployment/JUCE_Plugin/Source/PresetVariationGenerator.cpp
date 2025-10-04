#include "PresetVariationGenerator.h"
#include <random>

//==============================================================================
// MAIN GENERATION METHODS

std::vector<std::unique_ptr<GoldenPreset>> PresetVariationGenerator::generateVariations(
    const GoldenPreset& parent,
    const std::vector<VariationType>& types) {
    
    std::vector<std::unique_ptr<GoldenPreset>> variations;
    
    for (auto type : types) {
        auto variation = generateVariation(parent, type);
        if (variation && variation->validate()) {
            variations.push_back(std::move(variation));
        }
    }
    
    return variations;
}

std::unique_ptr<GoldenPreset> PresetVariationGenerator::generateVariation(
    const GoldenPreset& parent,
    VariationType type) {
    
    // Create deep copy of parent
    auto variation = std::make_unique<GoldenPreset>();
    
    // Copy all fields
    *variation = parent; // Assuming copy assignment is implemented
    
    // Mark as variation
    variation->isVariation = true;
    variation->parentId = parent.id;
    
    // Generate new ID (would need ID generation logic)
    int varNum = parent.variationIds.size() + 1;
    variation->id = parent.id.substring(0, 5) + String(varNum);
    
    // Apply variation based on type
    switch (type) {
        case VariationType::Subtle:
            applySubtleVariation(*variation);
            break;
        case VariationType::Moderate:
            applyModerateVariation(*variation);
            break;
        case VariationType::Extreme:
            applyExtremeVariation(*variation);
            break;
        case VariationType::Dark:
            applyDarkVariation(*variation);
            break;
        case VariationType::Bright:
            applyBrightVariation(*variation);
            break;
        case VariationType::Spacious:
            applySpaciousVariation(*variation);
            break;
        case VariationType::Intimate:
            applyIntimateVariation(*variation);
            break;
        case VariationType::Aggressive:
            applyAggressiveVariation(*variation);
            break;
        case VariationType::Gentle:
            applyGentleVariation(*variation);
            break;
        case VariationType::Vintage:
            applyVintageVariation(*variation);
            break;
        case VariationType::Modern:
            applyModernVariation(*variation);
            break;
        case VariationType::Minimal:
            applyMinimalVariation(*variation);
            break;
        case VariationType::Maximal:
            applyMaximalVariation(*variation);
            break;
        case VariationType::Rhythmic:
            applyRhythmicVariation(*variation);
            break;
        case VariationType::Ambient:
            applyAmbientVariation(*variation);
            break;
    }
    
    // Update metadata
    updateMetadataForVariation(*variation, parent, type);
    
    // Update complexity
    variation->updateComplexity();
    
    return variation;
}

std::vector<std::unique_ptr<GoldenPreset>> PresetVariationGenerator::generateComplementarySet(
    const GoldenPreset& parent,
    int count) {
    
    std::vector<VariationType> types;
    
    // Choose complementary variation types based on parent characteristics
    
    // If parent is bright, add a dark variation
    if (parent.sonicProfile.brightness > 0.7f) {
        types.push_back(VariationType::Dark);
    } else if (parent.sonicProfile.brightness < 0.3f) {
        types.push_back(VariationType::Bright);
    }
    
    // If parent is dense, add a minimal variation
    if (parent.sonicProfile.density > 0.7f) {
        types.push_back(VariationType::Minimal);
    } else if (parent.sonicProfile.density < 0.3f) {
        types.push_back(VariationType::Maximal);
    }
    
    // If parent has little space, add spacious
    if (parent.sonicProfile.space < 0.3f) {
        types.push_back(VariationType::Spacious);
    } else if (parent.sonicProfile.space > 0.7f) {
        types.push_back(VariationType::Intimate);
    }
    
    // Add a moderate variation for balance
    if (types.size() < count) {
        types.push_back(VariationType::Moderate);
    }
    
    // Add subtle variation if still need more
    if (types.size() < count) {
        types.push_back(VariationType::Subtle);
    }
    
    // Limit to requested count
    if (types.size() > count) {
        types.resize(count);
    }
    
    return generateVariations(parent, types);
}

//==============================================================================
// SUBTLE VARIATION - Small tweaks that maintain character

void PresetVariationGenerator::applySubtleVariation(GoldenPreset& preset) {
    Random rng;
    
    // Vary parameters by ±10%
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] >= 0 && preset.engineActive[slot]) {
            for (size_t i = 0; i < preset.engineParams[slot].size(); ++i) {
                if (shouldVaryParameter(preset.engineTypes[slot], i, VariationType::Subtle)) {
                    float variance = (rng.nextFloat() - 0.5f) * 0.2f; // ±10%
                    morphParameter(preset.engineParams[slot][i], variance);
                }
            }
            
            // Slight mix adjustment
            float mixVariance = (rng.nextFloat() - 0.5f) * 0.1f;
            morphParameter(preset.engineMix[slot], mixVariance);
        }
    }
}

//==============================================================================
// DARK VARIATION - Reduce brightness, increase warmth

void PresetVariationGenerator::applyDarkVariation(GoldenPreset& preset) {
    // Adjust EQ-type engines
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] == ENGINE_PARAMETRIC_EQ ||
            preset.engineTypes[slot] == ENGINE_VINTAGE_CONSOLE_EQ) {
            
            auto& params = preset.engineParams[slot];
            
            // Reduce high frequencies
            if (params.size() > 3) {
                params[1] *= 0.7f;  // Reduce HF gain
                params[3] *= 0.8f;  // Reduce MF gain
            }
        }
        
        // Adjust filter cutoff
        if (preset.engineTypes[slot] == ENGINE_LADDER_FILTER ||
            preset.engineTypes[slot] == ENGINE_STATE_VARIABLE_FILTER) {
            if (preset.engineParams[slot].size() > 0) {
                preset.engineParams[slot][0] *= 0.7f; // Lower cutoff
            }
        }
        
        // Increase saturation
        if (preset.engineTypes[slot] == ENGINE_VINTAGE_TUBE_PREAMP ||
            preset.engineTypes[slot] == ENGINE_TAPE_ECHO) {
            if (preset.engineParams[slot].size() > 0) {
                preset.engineParams[slot][0] *= 1.3f; // More drive
                preset.engineParams[slot][0] = juce::jlimit(0.0f, 1.0f, preset.engineParams[slot][0]);
            }
        }
        
        // Adjust reverb tone
        if (preset.engineTypes[slot] == ENGINE_PLATE_REVERB ||
            preset.engineTypes[slot] == ENGINE_SHIMMER_REVERB) {
            if (preset.engineParams[slot].size() > 2) {
                preset.engineParams[slot][2] *= 1.3f; // More damping
                preset.engineParams[slot][2] = juce::jlimit(0.0f, 1.0f, preset.engineParams[slot][2]);
            }
        }
    }
    
    // Update sonic profile
    preset.sonicProfile.brightness *= 0.6f;
    preset.sonicProfile.vintage *= 1.2f;
    preset.sonicProfile.vintage = juce::jlimit(0.0f, 1.0f, preset.sonicProfile.vintage);
}

//==============================================================================
// BRIGHT VARIATION - Increase clarity and air

void PresetVariationGenerator::applyBrightVariation(GoldenPreset& preset) {
    // Add or enhance high frequency content
    bool hasEQ = false;
    
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] == ENGINE_PARAMETRIC_EQ ||
            preset.engineTypes[slot] == ENGINE_VINTAGE_CONSOLE_EQ) {
            hasEQ = true;
            
            auto& params = preset.engineParams[slot];
            // Boost high frequencies
            if (params.size() > 3) {
                params[1] *= 1.3f;  // Boost HF
                params[1] = juce::jlimit(0.0f, 1.0f, params[1]);
            }
        }
        
        // Open up filters
        if (preset.engineTypes[slot] == ENGINE_LADDER_FILTER) {
            if (preset.engineParams[slot].size() > 0) {
                preset.engineParams[slot][0] = 0.7f + (preset.engineParams[slot][0] * 0.3f);
            }
        }
        
        // Reduce saturation for clarity
        if (preset.engineTypes[slot] == ENGINE_VINTAGE_TUBE_PREAMP) {
            if (preset.engineParams[slot].size() > 0) {
                preset.engineParams[slot][0] *= 0.7f;
            }
        }
    }
    
    // If no EQ present, consider adding harmonic exciter
    if (!hasEQ) {
        for (int slot = 0; slot < 6; ++slot) {
            if (preset.engineTypes[slot] < 0) {
                preset.engineTypes[slot] = ENGINE_HARMONIC_EXCITER;
                preset.engineMix[slot] = 0.3f;
                preset.engineActive[slot] = true;
                preset.engineParams[slot] = {
                    0.3f,   // Drive
                    0.8f,   // Frequency - highs
                    0.5f,   // Mix
                    0.9f    // Quality
                };
                break;
            }
        }
    }
    
    preset.sonicProfile.brightness *= 1.4f;
    preset.sonicProfile.brightness = juce::jlimit(0.0f, 1.0f, preset.sonicProfile.brightness);
}

//==============================================================================
// SPACIOUS VARIATION - Expand the spatial characteristics

void PresetVariationGenerator::applySpaciousVariation(GoldenPreset& preset) {
    bool hasReverb = false;
    bool hasDelay = false;
    
    for (int slot = 0; slot < 6; ++slot) {
        // Enhance existing reverbs
        if (preset.engineTypes[slot] == ENGINE_PLATE_REVERB ||
            preset.engineTypes[slot] == ENGINE_SHIMMER_REVERB ||
            preset.engineTypes[slot] == ENGINE_CONVOLUTION_REVERB) {
            
            hasReverb = true;
            auto& params = preset.engineParams[slot];
            
            if (params.size() > 1) {
                params[0] *= 1.3f;  // Larger size
                params[1] *= 1.4f;  // Longer decay
                params[0] = juce::jlimit(0.0f, 1.0f, params[0]);
                params[1] = juce::jlimit(0.0f, 1.0f, params[1]);
            }
            
            // Increase mix
            preset.engineMix[slot] *= 1.5f;
            preset.engineMix[slot] = juce::jlimit(0.0f, 0.8f, preset.engineMix[slot]);
        }
        
        // Enhance delays
        if (preset.engineTypes[slot] == ENGINE_DIGITAL_DELAY ||
            preset.engineTypes[slot] == ENGINE_TAPE_ECHO) {
            hasDelay = true;
            
            if (preset.engineParams[slot].size() > 1) {
                preset.engineParams[slot][1] *= 1.3f; // More feedback
                preset.engineParams[slot][1] = juce::jlimit(0.0f, 0.8f, preset.engineParams[slot][1]);
            }
        }
        
        // Add dimension expander
        if (preset.engineTypes[slot] == ENGINE_DIMENSION_EXPANDER) {
            if (preset.engineParams[slot].size() > 0) {
                preset.engineParams[slot][0] = 0.8f; // Wide stereo
            }
        }
    }
    
    // Add reverb if none exists
    if (!hasReverb) {
        for (int slot = 0; slot < 6; ++slot) {
            if (preset.engineTypes[slot] < 0) {
                preset.engineTypes[slot] = ENGINE_PLATE_REVERB;
                preset.engineMix[slot] = 0.4f;
                preset.engineActive[slot] = true;
                preset.engineParams[slot] = {
                    0.7f,   // Size
                    0.8f,   // Decay
                    0.4f,   // Damping
                    0.7f,   // Diffusion
                    0.5f    // Modulation
                };
                break;
            }
        }
    }
    
    preset.sonicProfile.space = juce::jlimit(0.0f, 1.0f, preset.sonicProfile.space * 1.5f);
    preset.sonicProfile.density *= 1.2f;
}

//==============================================================================
// AGGRESSIVE VARIATION - Add power and presence

void PresetVariationGenerator::applyAggressiveVariation(GoldenPreset& preset) {
    for (int slot = 0; slot < 6; ++slot) {
        // Increase compression
        if (preset.engineTypes[slot] == ENGINE_CLASSIC_COMPRESSOR ||
            preset.engineTypes[slot] == ENGINE_VINTAGE_OPTO_COMPRESSOR) {
            
            auto& params = preset.engineParams[slot];
            if (params.size() > 2) {
                params[0] *= 0.7f;  // Lower threshold
                params[1] *= 1.5f;  // Higher ratio
                params[1] = juce::jlimit(0.0f, 1.0f, params[1]);
            }
        }
        
        // Add distortion
        if (preset.engineTypes[slot] == ENGINE_VINTAGE_TUBE_PREAMP ||
            preset.engineTypes[slot] == ENGINE_MUFF_FUZZ ||
            preset.engineTypes[slot] == ENGINE_RODENT_DISTORTION) {
            
            if (preset.engineParams[slot].size() > 0) {
                preset.engineParams[slot][0] *= 1.5f; // More drive
                preset.engineParams[slot][0] = juce::jlimit(0.0f, 0.9f, preset.engineParams[slot][0]);
            }
            
            preset.engineMix[slot] *= 1.2f;
            preset.engineMix[slot] = juce::jlimit(0.0f, 1.0f, preset.engineMix[slot]);
        }
        
        // Boost presence frequencies
        if (preset.engineTypes[slot] == ENGINE_PARAMETRIC_EQ) {
            auto& params = preset.engineParams[slot];
            if (params.size() > 4) {
                params[3] = 0.65f;  // 5kHz region
                params[4] = 0.7f;   // Boost presence
            }
        }
    }
    
    // Add transient shaper if slot available
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] < 0) {
            preset.engineTypes[slot] = ENGINE_TRANSIENT_SHAPER;
            preset.engineMix[slot] = 0.7f;
            preset.engineActive[slot] = true;
            preset.engineParams[slot] = {
                0.7f,   // Attack boost
                0.4f,   // Sustain reduction
                0.5f,   // Output
                0.3f    // Smoothing
            };
            break;
        }
    }
    
    preset.sonicProfile.aggression = juce::jlimit(0.0f, 1.0f, preset.sonicProfile.aggression * 2.0f);
    preset.emotionalProfile.energy *= 1.4f;
    preset.emotionalProfile.tension *= 1.3f;
}

//==============================================================================
// MINIMAL VARIATION - Simplify and reduce

void PresetVariationGenerator::applyMinimalVariation(GoldenPreset& preset) {
    // Count active engines
    int activeCount = 0;
    std::vector<int> activeSlots;
    
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] >= 0 && preset.engineActive[slot]) {
            activeCount++;
            activeSlots.push_back(slot);
        }
    }
    
    // Disable some engines if more than 2 active
    if (activeCount > 2) {
        Random rng;
        
        // Keep the most important engines (usually first 2)
        for (int i = 2; i < activeSlots.size(); ++i) {
            if (rng.nextFloat() > 0.3f) { // 70% chance to disable
                preset.engineActive[activeSlots[i]] = false;
            }
        }
    }
    
    // Reduce mix levels
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineActive[slot]) {
            preset.engineMix[slot] *= 0.7f;
        }
    }
    
    // Simplify parameters - move toward neutral
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] >= 0) {
            for (auto& param : preset.engineParams[slot]) {
                // Move 30% closer to center (0.5)
                param = param + (0.5f - param) * 0.3f;
            }
        }
    }
    
    preset.sonicProfile.density *= 0.6f;
    preset.complexity = preset.getActiveEngineCount() / 6.0f;
}

//==============================================================================
// RHYTHMIC VARIATION - Add tempo-synced elements

void PresetVariationGenerator::applyRhythmicVariation(GoldenPreset& preset) {
    bool hasRhythmic = false;
    
    // Look for delays and modulation to make rhythmic
    for (int slot = 0; slot < 6; ++slot) {
        if (preset.engineTypes[slot] == ENGINE_DIGITAL_DELAY ||
            preset.engineTypes[slot] == ENGINE_TAPE_ECHO) {
            
            hasRhythmic = true;
            auto& params = preset.engineParams[slot];
            
            if (params.size() > 4) {
                // Set to common subdivision
                params[0] = 0.375f;  // Dotted 1/8
                params[4] = 1.0f;    // Sync on
            }
        }
        
        if (preset.engineTypes[slot] == ENGINE_CLASSIC_TREMOLO ||
            preset.engineTypes[slot] == ENGINE_HARMONIC_TREMOLO) {
            
            hasRhythmic = true;
            auto& params = preset.engineParams[slot];
            
            if (params.size() > 4) {
                params[3] = 1.0f;   // Sync on
                params[4] = 0.25f;  // 1/8 notes
            }
        }
    }
    
    // Add rhythmic element if none exists
    if (!hasRhythmic) {
        for (int slot = 0; slot < 6; ++slot) {
            if (preset.engineTypes[slot] < 0) {
                preset.engineTypes[slot] = ENGINE_GATED_REVERB;
                preset.engineMix[slot] = 0.5f;
                preset.engineActive[slot] = true;
                preset.engineParams[slot] = {
                    0.5f,   // Size
                    0.1f,   // Gate time - tight
                    0.6f,   // Threshold
                    0.9f,   // Attack - fast
                    0.4f    // Hold
                };
                break;
            }
        }
    }
    
    preset.sonicProfile.movement = juce::jlimit(0.0f, 1.0f, preset.sonicProfile.movement * 1.5f);
    preset.keywords.add("rhythmic");
    preset.keywords.add("synced");
}

//==============================================================================
// METADATA UPDATES

void PresetVariationGenerator::updateMetadataForVariation(
    GoldenPreset& preset,
    const GoldenPreset& parent,
    VariationType type) {
    
    // Generate variation name
    preset.name = generateVariationName(parent.name, type);
    
    // Update technical hint
    String typeStr;
    switch (type) {
        case VariationType::Dark: typeStr = " - Dark"; break;
        case VariationType::Bright: typeStr = " - Bright"; break;
        case VariationType::Spacious: typeStr = " - Spacious"; break;
        case VariationType::Intimate: typeStr = " - Intimate"; break;
        case VariationType::Aggressive: typeStr = " - Aggressive"; break;
        case VariationType::Gentle: typeStr = " - Gentle"; break;
        case VariationType::Vintage: typeStr = " - Vintage"; break;
        case VariationType::Modern: typeStr = " - Modern"; break;
        case VariationType::Minimal: typeStr = " - Minimal"; break;
        case VariationType::Maximal: typeStr = " - Maximal"; break;
        case VariationType::Rhythmic: typeStr = " - Rhythmic"; break;
        case VariationType::Ambient: typeStr = " - Ambient"; break;
        default: typeStr = " - Variation"; break;
    }
    
    preset.technicalHint += typeStr;
    
    // Adjust profiles
    adjustSonicProfile(preset, type);
    adjustEmotionalProfile(preset, type);
    
    // Add variation-specific keywords
    preset.keywords.add("variation");
    preset.keywords.add(typeStr.substring(3).toLowerCase()); // Remove " - "
    
    // Update timestamp
    preset.creationTimestamp = Time::getCurrentTime().toMilliseconds();
}

String PresetVariationGenerator::generateVariationName(const String& parentName, VariationType type) {
    switch (type) {
        case VariationType::Dark:
            return parentName + " Noir";
        case VariationType::Bright:
            return parentName + " Brilliance";
        case VariationType::Spacious:
            return parentName + " Expansive";
        case VariationType::Intimate:
            return parentName + " Close";
        case VariationType::Aggressive:
            return parentName + " Fierce";
        case VariationType::Gentle:
            return parentName + " Soft";
        case VariationType::Vintage:
            return parentName + " Retro";
        case VariationType::Modern:
            return parentName + " Neo";
        case VariationType::Minimal:
            return parentName + " Essential";
        case VariationType::Maximal:
            return parentName + " Ultra";
        case VariationType::Rhythmic:
            return parentName + " Pulse";
        case VariationType::Ambient:
            return parentName + " Drift";
        case VariationType::Subtle:
            return parentName + " Alt";
        case VariationType::Moderate:
            return parentName + " Mod";
        case VariationType::Extreme:
            return parentName + " X";
        default:
            return parentName + " Variation";
    }
}

void PresetVariationGenerator::adjustSonicProfile(GoldenPreset& preset, VariationType type) {
    // Profile adjustments are handled in specific variation methods
    // This ensures consistency
}

void PresetVariationGenerator::adjustEmotionalProfile(GoldenPreset& preset, VariationType type) {
    switch (type) {
        case VariationType::Dark:
            preset.emotionalProfile.mood *= 0.6f;
            preset.emotionalProfile.nostalgia *= 1.2f;
            break;
            
        case VariationType::Bright:
            preset.emotionalProfile.mood *= 1.3f;
            preset.emotionalProfile.energy *= 1.2f;
            break;
            
        case VariationType::Aggressive:
            preset.emotionalProfile.energy *= 1.5f;
            preset.emotionalProfile.tension *= 1.4f;
            break;
            
        case VariationType::Gentle:
            preset.emotionalProfile.energy *= 0.6f;
            preset.emotionalProfile.tension *= 0.5f;
            preset.emotionalProfile.mood *= 1.2f;
            break;
            
        case VariationType::Ambient:
            preset.emotionalProfile.tension *= 0.3f;
            preset.emotionalProfile.organic *= 1.3f;
            break;
            
        default:
            break;
    }
    
    // Clamp all values
    preset.emotionalProfile.energy = juce::jlimit(0.0f, 1.0f, preset.emotionalProfile.energy);
    preset.emotionalProfile.mood = juce::jlimit(0.0f, 1.0f, preset.emotionalProfile.mood);
    preset.emotionalProfile.tension = juce::jlimit(0.0f, 1.0f, preset.emotionalProfile.tension);
    preset.emotionalProfile.organic = juce::jlimit(0.0f, 1.0f, preset.emotionalProfile.organic);
    preset.emotionalProfile.nostalgia = juce::jlimit(0.0f, 1.0f, preset.emotionalProfile.nostalgia);
}

//==============================================================================
// HELPER METHODS

void PresetVariationGenerator::morphParameter(float& param, float amount, float min, float max) {
    param += amount;
    param = juce::jlimit(min, max, param);
}

bool PresetVariationGenerator::shouldVaryParameter(int engineType, int paramIndex, VariationType type) {
    // Some parameters shouldn't vary much
    
    // Don't vary sync settings
    if (engineType == ENGINE_DIGITAL_DELAY && paramIndex == 4) { // Sync parameter
        return false;
    }
    
    // Be careful with feedback
    if ((engineType == ENGINE_DIGITAL_DELAY || engineType == ENGINE_TAPE_ECHO) && paramIndex == 1) {
        return type != VariationType::Subtle; // Only vary in non-subtle variations
    }
    
    return true;
}

float PresetVariationGenerator::getVariationAmount(VariationType type) {
    switch (type) {
        case VariationType::Subtle:
            return 0.1f;  // ±10%
        case VariationType::Moderate:
            return 0.25f; // ±25%
        case VariationType::Extreme:
            return 0.5f;  // ±50%
        default:
            return 0.2f;  // ±20%
    }
}