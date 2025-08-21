// Dynamics Team Validation Suite
// Engines 1-6: Compressors, Limiters, Gates, Dynamic EQ
// Team Lead: Agent Dynamo

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "../../JUCE_Plugin/Source/EngineFactory.h"
#include "../../JUCE_Plugin/Source/UnifiedDefaultParameters.h"
#include "../../JUCE_Plugin/Source/EngineMetadata.h"
#include "../../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"

class DynamicsValidator {
public:
    struct ValidationResult {
        int engineId;
        std::string engineName;
        bool parameterCountValid;
        bool mixIndexValid;
        bool audioProcessingValid;
        bool metadataValid;
        bool defaultsValid;
        std::vector<std::string> errors;
        
        bool isValid() const {
            return parameterCountValid && mixIndexValid && 
                   audioProcessingValid && metadataValid && defaultsValid;
        }
    };
    
    // Dynamics engines: IDs 1-6
    const std::vector<int> DYNAMICS_ENGINES = {1, 2, 3, 4, 5, 6};
    
    // Expected parameter counts
    const std::map<int, int> EXPECTED_PARAM_COUNTS = {
        {1, 10},  // VintageOptoCompressor_Platinum
        {2, 10},  // ClassicCompressor
        {3, 10},  // TransientShaper_Platinum
        {4, 8},   // NoiseGate_Platinum
        {5, 8},   // MasteringLimiter_Platinum
        {6, 8}    // DynamicEQ
    };
    
    // Expected mix indices (from UnifiedDefaultParameters)
    const std::map<int, int> EXPECTED_MIX_INDICES = {
        {1, 5},   // VintageOptoCompressor_Platinum
        {2, 6},   // ClassicCompressor
        {3, 9},   // TransientShaper_Platinum
        {4, 6},   // NoiseGate_Platinum
        {5, 5},   // MasteringLimiter_Platinum
        {6, 6}    // DynamicEQ
    };
    
    ValidationResult validateEngine(int engineId) {
        ValidationResult result;
        result.engineId = engineId;
        result.engineName = getEngineName(engineId);
        
        // 1. Validate parameter count
        result.parameterCountValid = validateParameterCount(engineId, result.errors);
        
        // 2. Validate mix index
        result.mixIndexValid = validateMixIndex(engineId, result.errors);
        
        // 3. Validate audio processing
        result.audioProcessingValid = validateAudioProcessing(engineId, result.errors);
        
        // 4. Validate metadata
        result.metadataValid = validateMetadata(engineId, result.errors);
        
        // 5. Validate defaults
        result.defaultsValid = validateDefaults(engineId, result.errors);
        
        return result;
    }
    
    void runFullValidation() {
        std::cout << "==================================" << std::endl;
        std::cout << "DYNAMICS TEAM VALIDATION SUITE" << std::endl;
        std::cout << "==================================" << std::endl;
        
        std::vector<ValidationResult> results;
        int passCount = 0;
        int failCount = 0;
        
        for (int engineId : DYNAMICS_ENGINES) {
            std::cout << "\nValidating Engine " << engineId << "..." << std::endl;
            ValidationResult result = validateEngine(engineId);
            results.push_back(result);
            
            if (result.isValid()) {
                std::cout << "✅ " << result.engineName << " - PASSED" << std::endl;
                passCount++;
            } else {
                std::cout << "❌ " << result.engineName << " - FAILED" << std::endl;
                for (const auto& error : result.errors) {
                    std::cout << "   ⚠️  " << error << std::endl;
                }
                failCount++;
            }
        }
        
        // Summary
        std::cout << "\n==================================" << std::endl;
        std::cout << "VALIDATION SUMMARY" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Total Engines: " << DYNAMICS_ENGINES.size() << std::endl;
        std::cout << "Passed: " << passCount << std::endl;
        std::cout << "Failed: " << failCount << std::endl;
        
        // Known issues
        std::cout << "\n==================================" << std::endl;
        std::cout << "KNOWN ISSUES TO FIX" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "1. ClassicCompressor (ID 2): EAM claims mix at 4, should be 6" << std::endl;
        std::cout << "2. DynamicEQ (ID 6): EAM claims mix at 11, should be 6" << std::endl;
        
        // Exit code
        exit(failCount > 0 ? 1 : 0);
    }
    
private:
    std::string getEngineName(int engineId) {
        switch(engineId) {
            case 1: return "VintageOptoCompressor_Platinum";
            case 2: return "ClassicCompressor";
            case 3: return "TransientShaper_Platinum";
            case 4: return "NoiseGate_Platinum";
            case 5: return "MasteringLimiter_Platinum";
            case 6: return "DynamicEQ";
            default: return "Unknown";
        }
    }
    
    bool validateParameterCount(int engineId, std::vector<std::string>& errors) {
        auto it = EXPECTED_PARAM_COUNTS.find(engineId);
        if (it == EXPECTED_PARAM_COUNTS.end()) {
            errors.push_back("Engine ID not in expected parameter counts");
            return false;
        }
        
        int expectedCount = it->second;
        int actualCount = UnifiedDefaultParameters::getEngineParameterCount(engineId);
        
        if (actualCount != expectedCount) {
            errors.push_back("Parameter count mismatch: expected " + 
                           std::to_string(expectedCount) + ", got " + 
                           std::to_string(actualCount));
            return false;
        }
        return true;
    }
    
    bool validateMixIndex(int engineId, std::vector<std::string>& errors) {
        auto it = EXPECTED_MIX_INDICES.find(engineId);
        if (it == EXPECTED_MIX_INDICES.end()) {
            errors.push_back("Engine ID not in expected mix indices");
            return false;
        }
        
        int expectedIndex = it->second;
        int actualIndex = UnifiedDefaultParameters::getMixParameterIndex(engineId);
        
        if (actualIndex != expectedIndex) {
            errors.push_back("Mix index mismatch: expected " + 
                           std::to_string(expectedIndex) + ", got " + 
                           std::to_string(actualIndex));
            return false;
        }
        return true;
    }
    
    bool validateAudioProcessing(int engineId, std::vector<std::string>& errors) {
        // Create engine and test basic audio processing
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            errors.push_back("Failed to create engine instance");
            return false;
        }
        
        // Initialize with test parameters
        engine->prepareToPlay(44100, 512);
        
        // Create test buffer
        juce::AudioBuffer<float> buffer(2, 512);
        
        // Fill with test signal
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < 512; ++i) {
                data[i] = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f) * 0.5f;
            }
        }
        
        // Store original RMS
        float originalRMS = buffer.getRMSLevel(0, 0, 512);
        
        // Process
        try {
            engine->processBlock(buffer, 512);
        } catch (...) {
            errors.push_back("Exception thrown during audio processing");
            return false;
        }
        
        // Check for NaN/Inf
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < 512; ++i) {
                if (std::isnan(data[i]) || std::isinf(data[i])) {
                    errors.push_back("Invalid audio output (NaN/Inf detected)");
                    return false;
                }
            }
        }
        
        // For dynamics processors, verify they affect the signal
        float processedRMS = buffer.getRMSLevel(0, 0, 512);
        if (engineId != 3 && engineId != 4) { // Except TransientShaper and NoiseGate
            if (std::abs(originalRMS - processedRMS) < 0.001f) {
                errors.push_back("No audio processing detected");
                return false;
            }
        }
        
        return true;
    }
    
    bool validateMetadata(int engineId, std::vector<std::string>& errors) {
        // Check if engine metadata exists and is consistent
        try {
            auto metadata = EngineMetadata::getMetadata(engineId);
            if (metadata.engineId != engineId) {
                errors.push_back("Metadata engine ID mismatch");
                return false;
            }
            if (metadata.name.empty()) {
                errors.push_back("Empty engine name in metadata");
                return false;
            }
        } catch (...) {
            errors.push_back("Failed to retrieve engine metadata");
            return false;
        }
        return true;
    }
    
    bool validateDefaults(int engineId, std::vector<std::string>& errors) {
        // Validate default parameters
        auto defaults = UnifiedDefaultParameters::getDefaults(engineId);
        if (defaults.empty() && engineId != 0) { // Only NoneEngine should have no defaults
            errors.push_back("No default parameters found");
            return false;
        }
        
        // Check parameter ranges
        for (const auto& param : defaults) {
            if (param < 0.0f || param > 1.0f) {
                errors.push_back("Default parameter out of range [0,1]: " + 
                               std::to_string(param));
                return false;
            }
        }
        
        return true;
    }
};

int main() {
    DynamicsValidator validator;
    validator.runFullValidation();
    return 0;
}