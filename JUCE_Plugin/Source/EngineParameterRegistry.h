#pragma once

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include "EngineTypes.h"

/**
 * Centralized Parameter Registry for Chimera Phoenix
 * 
 * Single source of truth for all engine parameter definitions.
 * Engines register their parameters here at startup.
 * UI and presets query this registry for parameter information.
 */

class EngineParameterRegistry {
public:
    struct ParameterDefinition {
        std::string name;
        std::string displayName;  // For UI
        float defaultValue;
        float minValue;
        float maxValue;
        std::string units;
        std::string description;
        float skewFactor;
        bool isDiscrete;
        std::vector<std::string> discreteValues;  // For combo boxes
        
        ParameterDefinition() 
            : defaultValue(0.5f), minValue(0.0f), maxValue(1.0f), 
              skewFactor(1.0f), isDiscrete(false) {}
    };
    
    struct EngineDefinition {
        int engineId;
        std::string engineName;
        std::string category;
        std::vector<ParameterDefinition> parameters;
        std::string description;
        bool isRegistered = false;
    };
    
    // Singleton access
    static EngineParameterRegistry& getInstance() {
        static EngineParameterRegistry instance;
        return instance;
    }
    
    /**
     * Register an engine's parameters (called by each engine at startup)
     */
    void registerEngine(int engineId, const EngineDefinition& definition) {
        std::lock_guard<std::mutex> lock(mutex_);
        engines_[engineId] = definition;
        engines_[engineId].isRegistered = true;
    }
    
    /**
     * Quick registration helper for engines
     */
    class EngineRegistrar {
    public:
        EngineRegistrar(int engineId, const std::string& name) 
            : engineId_(engineId) {
            def_.engineId = engineId;
            def_.engineName = name;
        }
        
        EngineRegistrar& param(const std::string& name, float defaultVal, 
                               const std::string& units = "", 
                               const std::string& description = "") {
            ParameterDefinition p;
            p.name = name;
            p.displayName = name;
            p.defaultValue = defaultVal;
            p.units = units;
            p.description = description;
            def_.parameters.push_back(p);
            return *this;
        }
        
        EngineRegistrar& param(const std::string& name, float defaultVal,
                              float min, float max,
                              const std::string& units = "") {
            ParameterDefinition p;
            p.name = name;
            p.displayName = name;
            p.defaultValue = defaultVal;
            p.minValue = min;
            p.maxValue = max;
            p.units = units;
            def_.parameters.push_back(p);
            return *this;
        }
        
        void commit() {
            EngineParameterRegistry::getInstance().registerEngine(engineId_, def_);
        }
        
    private:
        int engineId_;
        EngineDefinition def_;
    };
    
    /**
     * Get parameter name for an engine
     */
    std::string getParameterName(int engineId, int paramIndex) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = engines_.find(engineId);
        if (it != engines_.end() && it->second.isRegistered) {
            if (paramIndex >= 0 && paramIndex < it->second.parameters.size()) {
                return it->second.parameters[paramIndex].displayName;
            }
        }
        
        // Fallback to generic name
        return "Param " + std::to_string(paramIndex + 1);
    }
    
    /**
     * Get all parameter definitions for an engine
     */
    std::vector<ParameterDefinition> getParameters(int engineId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = engines_.find(engineId);
        if (it != engines_.end() && it->second.isRegistered) {
            return it->second.parameters;
        }
        
        return {};
    }
    
    /**
     * Get default value for a parameter
     */
    float getDefaultValue(int engineId, int paramIndex) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = engines_.find(engineId);
        if (it != engines_.end() && it->second.isRegistered) {
            if (paramIndex >= 0 && paramIndex < it->second.parameters.size()) {
                return it->second.parameters[paramIndex].defaultValue;
            }
        }
        
        return 0.5f;  // Safe default
    }
    
    /**
     * Check if engine is registered
     */
    bool isEngineRegistered(int engineId) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = engines_.find(engineId);
        return it != engines_.end() && it->second.isRegistered;
    }
    
    /**
     * Get list of unregistered engines (for debugging)
     */
    std::vector<int> getUnregisteredEngines() const {
        std::vector<int> unregistered;
        
        for (int id = 1; id < ENGINE_COUNT; id++) {
            if (!isEngineRegistered(id)) {
                unregistered.push_back(id);
            }
        }
        
        return unregistered;
    }
    
    /**
     * Export to JSON for external tools/AI
     */
    std::string exportToJSON() const;
    
    /**
     * Import from JSON (for testing or external updates)
     */
    bool importFromJSON(const std::string& json);
    
    /**
     * Clear all registrations (mainly for testing)
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        engines_.clear();
    }
    
private:
    EngineParameterRegistry() = default;
    ~EngineParameterRegistry() = default;
    
    // Prevent copying
    EngineParameterRegistry(const EngineParameterRegistry&) = delete;
    EngineParameterRegistry& operator=(const EngineParameterRegistry&) = delete;
    
    mutable std::mutex mutex_;
    std::map<int, EngineDefinition> engines_;
};

/**
 * Macro for easy engine registration in constructors
 */
#define REGISTER_ENGINE_PARAMS(engineId, engineName) \
    EngineParameterRegistry::EngineRegistrar(engineId, engineName)

/**
 * Example usage in an engine constructor:
 * 
 * BitCrusher::BitCrusher() {
 *     REGISTER_ENGINE_PARAMS(ENGINE_BIT_CRUSHER, "Bit Crusher")
 *         .param("Bit Depth", 0.7f, "bits", "Reduce bit resolution")
 *         .param("Sample Rate", 0.5f, "Hz", "Reduce sample rate")  
 *         .param("Mix", 1.0f, "%", "Dry/wet mix")
 *         .commit();
 * }
 */