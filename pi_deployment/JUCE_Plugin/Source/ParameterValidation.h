#pragma once

#include <vector>
#include <string>
#include <map>
#include "EngineBase.h"
#include "EngineTypes.h"

/**
 * Parameter Validation System for Chimera Phoenix
 * 
 * This system ensures parameter naming consistency across:
 * - Engine implementations
 * - UI components  
 * - Preset systems
 * - AI parameter database
 * 
 * Run validation during:
 * - Build time (via unit tests)
 * - Plugin initialization (debug builds)
 * - CI/CD pipeline
 */

class ParameterValidation {
public:
    struct ValidationResult {
        bool passed = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        
        void addError(const std::string& error) {
            errors.push_back(error);
            passed = false;
        }
        
        void addWarning(const std::string& warning) {
            warnings.push_back(warning);
        }
    };
    
    struct ParameterRule {
        std::string name;
        float minDefault;
        float maxDefault;
        bool required;
        std::vector<std::string> allowedNames;  // Acceptable variations
    };
    
    // Common parameter patterns across engine categories
    static std::map<std::string, ParameterRule> getCommonRules() {
        return {
            {"Mix", {"Mix", 0.0f, 1.0f, false, {"Mix", "Dry/Wet", "Blend"}}},
            {"Drive", {"Drive", 0.0f, 0.5f, false, {"Drive", "Gain", "Input"}}},
            {"Output", {"Output", 0.3f, 0.7f, false, {"Output", "Level", "Volume"}}},
            {"Frequency", {"Frequency", 0.2f, 0.8f, false, {"Frequency", "Freq", "Cutoff"}}},
            {"Resonance", {"Resonance", 0.0f, 0.7f, false, {"Resonance", "Q", "Feedback"}}},
            {"Time", {"Time", 0.0f, 1.0f, false, {"Time", "Delay", "Length"}}},
            {"Feedback", {"Feedback", 0.0f, 0.7f, false, {"Feedback", "Regen", "Repeats"}}},
            {"Rate", {"Rate", 0.1f, 0.5f, false, {"Rate", "Speed", "Frequency"}}},
            {"Depth", {"Depth", 0.0f, 0.5f, false, {"Depth", "Amount", "Intensity"}}},
            {"Threshold", {"Threshold", 0.3f, 0.7f, false, {"Threshold", "Gate", "Level"}}}
        };
    }
    
    /**
     * Validate a single engine's parameter definitions
     */
    static ValidationResult validateEngine(EngineBase* engine, int engineId) {
        ValidationResult result;
        
        if (!engine) {
            result.addError("Engine is null");
            return result;
        }
        
        int paramCount = engine->getNumParameters();
        if (paramCount < 1 || paramCount > 20) {
            result.addError("Invalid parameter count: " + std::to_string(paramCount));
        }
        
        // Check each parameter
        for (int i = 0; i < paramCount; i++) {
            juce::String paramName = engine->getParameterName(i);
            
            // Check for empty or generic names
            if (paramName.isEmpty()) {
                result.addError("Parameter " + std::to_string(i) + " has empty name");
            }
            else if (paramName.startsWith("Param ")) {
                result.addError("Parameter " + std::to_string(i) + " has generic name: " + paramName.toStdString());
            }
            
            // Check for consistency with common patterns
            validateCommonParameter(paramName.toStdString(), result);
        }
        
        // Check for required parameters based on engine type
        validateRequiredParameters(engine, engineId, result);
        
        return result;
    }
    
    /**
     * Validate all engines in the plugin
     */
    static ValidationResult validateAllEngines() {
        ValidationResult result;
        int failedCount = 0;
        
        // This will be populated by iterating through all engine types
        for (int engineId = 1; engineId < ENGINE_COUNT; engineId++) {
            // Note: Need EngineFactory to create instances
            // This is pseudo-code showing the validation flow
            result.addWarning("Full validation requires EngineFactory integration");
        }
        
        return result;
    }
    
    /**
     * Check consistency between different parameter sources
     */
    static ValidationResult validateConsistency() {
        ValidationResult result;
        
        // Check that EngineBase::getParameterName matches:
        // 1. GeneratedParameterDatabase.h
        // 2. UnifiedDefaultParameters
        // 3. AI Server parameter definitions
        // 4. UI parameter labels
        
        result.addWarning("Consistency check requires all systems to be integrated");
        
        return result;
    }
    
    /**
     * Generate report for CI/CD or debugging
     */
    static std::string generateReport(const ValidationResult& result) {
        std::string report = "=== Parameter Validation Report ===\n\n";
        
        if (result.passed) {
            report += "✅ All validations PASSED\n\n";
        } else {
            report += "❌ Validation FAILED\n\n";
        }
        
        if (!result.errors.empty()) {
            report += "ERRORS (" + std::to_string(result.errors.size()) + "):\n";
            for (const auto& error : result.errors) {
                report += "  • " + error + "\n";
            }
            report += "\n";
        }
        
        if (!result.warnings.empty()) {
            report += "WARNINGS (" + std::to_string(result.warnings.size()) + "):\n";
            for (const auto& warning : result.warnings) {
                report += "  • " + warning + "\n";
            }
        }
        
        return report;
    }
    
private:
    static void validateCommonParameter(const std::string& name, ValidationResult& result) {
        auto rules = getCommonRules();
        
        // Check if this looks like a common parameter
        for (const auto& [pattern, rule] : rules) {
            bool matches = false;
            for (const auto& allowed : rule.allowedNames) {
                if (name == allowed) {
                    matches = true;
                    break;
                }
            }
            
            // Check for close but wrong variations
            if (!matches && similarityScore(name, pattern) > 0.7) {
                result.addWarning("Parameter '" + name + "' is similar to standard '" + pattern + 
                                "' - consider using standard name");
            }
        }
    }
    
    static void validateRequiredParameters(EngineBase* engine, int engineId, ValidationResult& result) {
        // Check for required parameters based on engine category
        std::string engineName = getEngineTypeName(engineId);
        
        // Reverbs should have Mix, Size/Room, Damping
        if (engineName.find("Reverb") != std::string::npos) {
            bool hasMix = false;
            bool hasSize = false;
            
            for (int i = 0; i < engine->getNumParameters(); i++) {
                std::string param = engine->getParameterName(i).toStdString();
                if (param == "Mix" || param == "Dry/Wet") hasMix = true;
                if (param == "Size" || param == "Room") hasSize = true;
            }
            
            if (!hasMix) result.addWarning("Reverb engine missing Mix parameter");
            if (!hasSize) result.addWarning("Reverb engine missing Size/Room parameter");
        }
        
        // Add more category-specific checks...
    }
    
    static float similarityScore(const std::string& a, const std::string& b) {
        // Simple similarity check (could use Levenshtein distance)
        if (a == b) return 1.0f;
        
        std::string lowerA = a;
        std::string lowerB = b;
        std::transform(lowerA.begin(), lowerA.end(), lowerA.begin(), ::tolower);
        std::transform(lowerB.begin(), lowerB.end(), lowerB.begin(), ::tolower);
        
        if (lowerA == lowerB) return 0.9f;
        if (lowerA.find(lowerB) != std::string::npos || lowerB.find(lowerA) != std::string::npos) {
            return 0.7f;
        }
        
        return 0.0f;
    }
};