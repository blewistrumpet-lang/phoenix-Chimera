#include "PresetValidator.h"
#include "EngineTypes.h"
#include <algorithm>
#include <cmath>

//==============================================================================
// MAIN VALIDATION

PresetValidator::ValidationResult PresetValidator::validatePreset(
    const GoldenPreset& preset,
    const QualityThresholds& thresholds) {
    
    ValidationResult result;
    
    // 1. Structural validation
    if (!validateStructure(preset, result)) {
        result.passed = false;
        return result;
    }
    
    // 2. Engine configuration
    if (!validateEngineConfiguration(preset, result)) {
        result.passed = false;
        return result;
    }
    
    // 3. Parameter validation
    if (!validateParameters(preset, result)) {
        result.passed = false;
        return result;
    }
    
    // 4. Metadata validation
    if (!validateMetadata(preset, result)) {
        result.metadataQuality = 0.0f;
    } else {
        result.metadataQuality = 100.0f;
    }
    
    // 5. Audio quality testing
    validateAudioQuality(preset, result);
    
    // 6. Performance validation
    validatePerformance(preset, result, thresholds);
    
    // Calculate overall quality score
    result.qualityScore = (
        result.audioQuality * 0.3f +
        result.parameterQuality * 0.2f +
        result.metadataQuality * 0.2f +
        result.cpuEfficiency * 0.2f +
        result.uniquenessScore * 0.1f
    );
    
    // Determine pass/fail
    result.passed = result.errors.empty() && 
                   result.qualityScore >= thresholds.minQualityScore;
    
    return result;
}

//==============================================================================
// STRUCTURAL VALIDATION

bool PresetValidator::validateStructure(const GoldenPreset& preset, ValidationResult& result) {
    bool valid = true;
    
    // Check ID format
    if (!isIDValid(preset.id)) {
        result.errors.add("Invalid preset ID format. Expected 'GC_XXX'");
        valid = false;
    }
    
    // Check name
    if (preset.name.isEmpty()) {
        result.errors.add("Preset name is empty");
        valid = false;
    } else if (preset.name.length() > 50) {
        result.warnings.add("Preset name is very long (>50 chars)");
    }
    
    // Check category
    if (preset.category.isEmpty()) {
        result.errors.add("Preset category is empty");
        valid = false;
    }
    
    // Validate hierarchy
    if (preset.isVariation && preset.parentId.isEmpty()) {
        result.errors.add("Variation preset missing parent ID");
        valid = false;
    }
    
    // Check technical hint
    if (preset.technicalHint.isEmpty()) {
        result.warnings.add("Technical hint is empty - consider adding one for clarity");
    }
    
    return valid;
}

//==============================================================================
// ENGINE VALIDATION

bool PresetValidator::validateEngineConfiguration(const GoldenPreset& preset, 
                                                 ValidationResult& result) {
    bool valid = true;
    int activeCount = 0;
    
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0) {
            // Validate engine type ID
            if (preset.engineTypes[i] >= ENGINE_COUNT) {
                result.errors.add("Invalid engine type ID in slot " + String(i));
                valid = false;
            }
            
            // Check mix level
            if (preset.engineMix[i] < 0.0f || preset.engineMix[i] > 1.0f) {
                result.errors.add("Mix level out of range in slot " + String(i));
                valid = false;
            }
            
            if (preset.engineActive[i]) {
                activeCount++;
                
                // Warn about very low mix levels
                if (preset.engineMix[i] < 0.05f) {
                    result.warnings.add("Very low mix level (" + 
                                      String(preset.engineMix[i], 2) + 
                                      ") in slot " + String(i));
                }
            }
        } else if (preset.engineTypes[i] < -1) {
            result.errors.add("Invalid engine type in slot " + String(i));
            valid = false;
        }
    }
    
    // Check for at least one active engine
    if (activeCount == 0) {
        result.errors.add("No active engines in preset");
        valid = false;
    }
    
    return valid;
}

//==============================================================================
// PARAMETER VALIDATION

bool PresetValidator::validateParameters(const GoldenPreset& preset, 
                                       ValidationResult& result) {
    bool valid = true;
    float totalVariance = 0.0f;
    int paramCount = 0;
    
    for (int i = 0; i < 6; ++i) {
        if (preset.engineTypes[i] >= 0) {
            const auto& params = preset.engineParams[i];
            
            // Check parameter count
            if (params.empty()) {
                result.warnings.add("No parameters for engine in slot " + String(i));
            } else if (params.size() > 8) {
                result.warnings.add("Too many parameters (" + String(params.size()) + 
                                  ") for engine in slot " + String(i));
            }
            
            // Validate parameter ranges and calculate variance
            float slotVariance = 0.0f;
            for (float param : params) {
                if (param < 0.0f || param > 1.0f) {
                    result.errors.add("Parameter out of range [0,1] in slot " + String(i));
                    valid = false;
                }
                
                // Calculate variance from center (0.5)
                slotVariance += std::abs(param - 0.5f);
                paramCount++;
            }
            
            if (!params.empty()) {
                totalVariance += slotVariance / params.size();
            }
        }
    }
    
    // Calculate parameter quality based on variance
    if (paramCount > 0) {
        float avgVariance = totalVariance / preset.getActiveEngineCount();
        result.parameterQuality = juce::jlimit(0.0f, 100.0f, avgVariance * 200.0f);
        
        if (avgVariance < 0.1f) {
            result.warnings.add("Parameters are very centered - consider more varied settings");
        }
    }
    
    return valid;
}

//==============================================================================
// METADATA VALIDATION

bool PresetValidator::validateMetadata(const GoldenPreset& preset, 
                                     ValidationResult& result) {
    bool valid = true;
    
    // Check keywords
    if (preset.keywords.empty()) {
        result.warnings.add("No keywords defined - preset may be hard to find");
    } else if (preset.keywords.size() < 3) {
        result.suggestions.add("Consider adding more keywords for better searchability");
    }
    
    // Check user prompts
    if (preset.userPrompts.empty()) {
        result.warnings.add("No example user prompts - AI training may be limited");
    }
    
    // Validate sonic profile
    const auto& sonic = preset.sonicProfile;
    if (sonic.brightness == 0.5f && sonic.density == 0.5f && 
        sonic.movement == 0.5f && sonic.space == 0.5f) {
        result.warnings.add("Sonic profile is all default values - consider profiling");
    }
    
    // Validate CPU tier matches actual usage
    if (preset.actualCpuPercent > 0.0f) {
        bool tierMismatch = false;
        switch (preset.cpuTier) {
            case CPUTier::LIGHT:
                tierMismatch = preset.actualCpuPercent > 3.0f;
                break;
            case CPUTier::MEDIUM:
                tierMismatch = preset.actualCpuPercent > 8.0f;
                break;
            case CPUTier::HEAVY:
                tierMismatch = preset.actualCpuPercent > 15.0f;
                break;
            case CPUTier::EXTREME:
                tierMismatch = preset.actualCpuPercent > 25.0f;
                break;
        }
        
        if (tierMismatch) {
            result.warnings.add("CPU tier doesn't match actual usage (" + 
                              String(preset.actualCpuPercent, 1) + "%)");
        }
    }
    
    return valid;
}

//==============================================================================
// AUDIO QUALITY TESTING

bool PresetValidator::validateAudioQuality(const GoldenPreset& preset, 
                                          ValidationResult& result) {
    // Generate test signal
    const int sampleRate = 48000;
    const int numSamples = sampleRate; // 1 second
    auto testSignal = generateTestSignal(numSamples, 2);
    
    // Process through preset
    auto output = processPreset(preset, testSignal, sampleRate);
    
    // Measure DC offset
    float dcOffset = 0.0f;
    for (int ch = 0; ch < output.getNumChannels(); ++ch) {
        const float* samples = output.getReadPointer(ch);
        float channelDC = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            channelDC += samples[i];
        }
        channelDC /= numSamples;
        dcOffset = std::max(dcOffset, std::abs(channelDC));
    }
    
    float dcOffsetDB = 20.0f * std::log10(dcOffset + 1e-10f);
    
    if (dcOffsetDB > -60.0f) {
        result.warnings.add("DC offset detected: " + String(dcOffsetDB, 1) + " dB");
        result.audioQuality -= 10.0f;
    }
    
    // Check headroom
    float peakLevel = 0.0f;
    for (int ch = 0; ch < output.getNumChannels(); ++ch) {
        const float* samples = output.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            peakLevel = std::max(peakLevel, std::abs(samples[i]));
        }
    }
    
    if (peakLevel > 0.95f) {
        result.warnings.add("Low headroom - peak level at " + 
                          String(20.0f * std::log10(peakLevel), 1) + " dB");
        result.audioQuality -= 5.0f;
    }
    
    // Test with silence (check noise floor)
    auto silence = generateSilence(numSamples, 2);
    auto noiseOutput = processPreset(preset, silence, sampleRate);
    
    float noiseLevel = 0.0f;
    for (int ch = 0; ch < noiseOutput.getNumChannels(); ++ch) {
        const float* samples = noiseOutput.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            noiseLevel = std::max(noiseLevel, std::abs(samples[i]));
        }
    }
    
    float noiseFloorDB = 20.0f * std::log10(noiseLevel + 1e-10f);
    
    if (noiseFloorDB > -90.0f) {
        result.suggestions.add("Noise floor at " + String(noiseFloorDB, 1) + 
                             " dB - consider optimizing");
    }
    
    // Base audio quality score
    result.audioQuality = juce::jlimit(0.0f, 100.0f, 100.0f + result.audioQuality);
    
    return true;
}

//==============================================================================
// PERFORMANCE VALIDATION

bool PresetValidator::validatePerformance(const GoldenPreset& preset,
                                        ValidationResult& result,
                                        const QualityThresholds& thresholds) {
    // If CPU usage was pre-measured, validate against tier
    if (preset.actualCpuPercent > 0.0f) {
        float maxCPU = 0.0f;
        switch (preset.cpuTier) {
            case CPUTier::LIGHT:
                maxCPU = thresholds.maxCPULight;
                break;
            case CPUTier::MEDIUM:
                maxCPU = thresholds.maxCPUMedium;
                break;
            case CPUTier::HEAVY:
                maxCPU = thresholds.maxCPUHeavy;
                break;
            case CPUTier::EXTREME:
                maxCPU = thresholds.maxCPUExtreme;
                break;
        }
        
        if (preset.actualCpuPercent > maxCPU) {
            result.errors.add("CPU usage (" + String(preset.actualCpuPercent, 1) + 
                            "%) exceeds tier limit (" + String(maxCPU, 1) + "%)");
            result.cpuEfficiency = 0.0f;
        } else {
            // Calculate efficiency score (lower CPU = higher score)
            result.cpuEfficiency = 100.0f * (1.0f - (preset.actualCpuPercent / maxCPU));
        }
    } else {
        // Estimate based on engine count
        int engineCount = preset.getActiveEngineCount();
        float estimatedCPU = engineCount * 2.5f; // Rough estimate
        
        result.suggestions.add("CPU usage not measured - estimated at " + 
                             String(estimatedCPU, 1) + "%");
        result.cpuEfficiency = 100.0f * (1.0f - (estimatedCPU / 25.0f));
    }
    
    // Validate latency
    if (preset.latencySamples > 0.0f) {
        float latencyMs = preset.latencySamples / 48.0f; // Assume 48kHz
        if (latencyMs > thresholds.maxLatencyMs) {
            result.warnings.add("High latency: " + String(latencyMs, 1) + " ms");
        }
    }
    
    return true;
}

//==============================================================================
// UNIQUENESS VALIDATION

bool PresetValidator::validateUniqueness(const GoldenPreset& preset,
                                       const std::vector<std::unique_ptr<GoldenPreset>>& corpus,
                                       ValidationResult& result) {
    // Check name uniqueness
    int nameCount = 0;
    for (const auto& other : corpus) {
        if (other && other->name == preset.name && other->id != preset.id) {
            nameCount++;
        }
    }
    
    if (nameCount > 0) {
        result.errors.add("Preset name '" + preset.name + "' is not unique");
        return false;
    }
    
    // Find similar presets
    auto similar = findSimilarPresets(preset, corpus, 0.95f);
    
    if (!similar.empty()) {
        result.warnings.add("Very similar to: " + similar[0]);
        result.uniquenessScore = 70.0f;
    } else {
        result.uniquenessScore = 100.0f;
    }
    
    return true;
}

//==============================================================================
// HELPER METHODS

bool PresetValidator::isIDValid(const String& id) {
    if (id.length() != 6) return false;
    if (!id.startsWith("GC_")) return false;
    
    // Check last 3 characters are digits
    for (int i = 3; i < 6; ++i) {
        if (!CharacterFunctions::isDigit(id[i])) {
            return false;
        }
    }
    
    return true;
}

std::vector<String> PresetValidator::findSimilarPresets(
    const GoldenPreset& preset,
    const std::vector<std::unique_ptr<GoldenPreset>>& corpus,
    float threshold) {
    
    std::vector<String> similar;
    auto presetVector = preset.toFAISSVector();
    
    for (const auto& other : corpus) {
        if (!other || other->id == preset.id) continue;
        
        auto otherVector = other->toFAISSVector();
        float similarity = compareVectors(presetVector, otherVector);
        
        if (similarity > threshold) {
            similar.push_back(other->name + " (" + String(similarity * 100, 1) + "%)");
        }
    }
    
    return similar;
}

float PresetValidator::compareVectors(const std::vector<float>& a, 
                                    const std::vector<float>& b) {
    if (a.size() != b.size()) return 0.0f;
    
    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    for (size_t i = 0; i < a.size(); ++i) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }
    
    if (normA == 0.0f || normB == 0.0f) return 0.0f;
    
    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}

//==============================================================================
// TEST SIGNAL GENERATION

AudioBuffer<float> PresetValidator::generateTestSignal(int numSamples, int numChannels) {
    AudioBuffer<float> buffer(numChannels, numSamples);
    
    // Pink noise generator
    Random random;
    
    for (int ch = 0; ch < numChannels; ++ch) {
        float* samples = buffer.getWritePointer(ch);
        
        // Simple pink noise approximation
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float white = random.nextFloat() * 2.0f - 1.0f;
            
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f;
            b4 = 0.55000f * b4 + white * 0.5329522f;
            b5 = -0.7616f * b5 - white * 0.0168980f;
            
            samples[i] = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f) * 0.11f;
            b6 = white * 0.115926f;
        }
    }
    
    return buffer;
}

AudioBuffer<float> PresetValidator::generateSilence(int numSamples, int numChannels) {
    AudioBuffer<float> buffer(numChannels, numSamples);
    buffer.clear();
    return buffer;
}

//==============================================================================
// PROCESSING HELPER

AudioBuffer<float> PresetValidator::processPreset(const GoldenPreset& preset,
                                                 const AudioBuffer<float>& input,
                                                 double sampleRate) {
    // Create output buffer
    AudioBuffer<float> output(input.getNumChannels(), input.getNumSamples());
    output.clear();
    
    // For validation, we would need to create and process through actual engines
    // This is a simplified version - in practice, you'd instantiate the engines
    
    // Copy input to output for now (placeholder)
    for (int ch = 0; ch < input.getNumChannels(); ++ch) {
        output.copyFrom(ch, 0, input, ch, 0, input.getNumSamples());
    }
    
    // Apply basic gain based on mix levels
    for (int ch = 0; ch < output.getNumChannels(); ++ch) {
        float totalGain = 0.0f;
        int activeEngines = 0;
        
        for (int i = 0; i < 6; ++i) {
            if (preset.engineTypes[i] >= 0 && preset.engineActive[i]) {
                totalGain += preset.engineMix[i];
                activeEngines++;
            }
        }
        
        if (activeEngines > 0) {
            output.applyGain(ch, 0, output.getNumSamples(), totalGain / activeEngines);
        }
    }
    
    return output;
}

//==============================================================================
// REPORT GENERATION

String PresetValidator::generateValidationReport(const ValidationResult& result) {
    String report;
    report << "=== PRESET VALIDATION REPORT ===\n\n";
    
    report << "Overall Result: " << (result.passed ? "PASSED" : "FAILED") << "\n";
    report << "Quality Score: " << String(result.qualityScore, 1) << "%\n\n";
    
    report << "Component Scores:\n";
    report << "  Audio Quality: " << String(result.audioQuality, 1) << "%\n";
    report << "  Parameter Quality: " << String(result.parameterQuality, 1) << "%\n";
    report << "  Metadata Quality: " << String(result.metadataQuality, 1) << "%\n";
    report << "  CPU Efficiency: " << String(result.cpuEfficiency, 1) << "%\n";
    report << "  Uniqueness: " << String(result.uniquenessScore, 1) << "%\n\n";
    
    if (!result.errors.empty()) {
        report << "ERRORS:\n";
        for (const auto& error : result.errors) {
            report << "  ✗ " << error << "\n";
        }
        report << "\n";
    }
    
    if (!result.warnings.empty()) {
        report << "WARNINGS:\n";
        for (const auto& warning : result.warnings) {
            report << "  ⚠ " << warning << "\n";
        }
        report << "\n";
    }
    
    if (!result.suggestions.empty()) {
        report << "SUGGESTIONS:\n";
        for (const auto& suggestion : result.suggestions) {
            report << "  → " << suggestion << "\n";
        }
    }
    
    return report;
}