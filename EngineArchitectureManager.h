/**
 * ENGINE ARCHITECTURE MANAGER
 * 
 * Central authority for engine system integrity, validation, and management.
 * Ensures proper factory configuration, engine mapping, and parameter mapping.
 * 
 * Responsibilities:
 * - Validate engine factory configuration (57 engines, IDs 0-56)
 * - Assert correct engine-to-ID mappings
 * - Verify parameter mappings for each engine
 * - Monitor engine health and performance
 * - Provide architecture documentation and reporting
 * - Detect and prevent architectural violations
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <fstream>
#include <chrono>
#include <cassert>

#include "EngineBase.h"
#include "EngineFactory.h"
#include "PluginProcessor.h"

class EngineArchitectureManager {
public:
    // Singleton pattern for architecture management
    static EngineArchitectureManager& getInstance() {
        static EngineArchitectureManager instance;
        return instance;
    }
    
    // Core architecture constants
    static constexpr int TOTAL_ENGINES = 57;
    static constexpr int MIN_ENGINE_ID = 0;
    static constexpr int MAX_ENGINE_ID = 56;
    static constexpr int MAX_PARAMETERS_PER_ENGINE = 15;
    
    // Engine categories for organization
    enum class EngineCategory {
        SPECIAL,
        DYNAMICS,
        EQ_FILTER,
        DISTORTION,
        MODULATION,
        DELAY,
        REVERB,
        SPATIAL,
        UTILITY
    };
    
    // Architecture validation levels
    enum class ValidationLevel {
        BASIC,      // Quick checks
        STANDARD,   // Normal operation
        COMPREHENSIVE, // Full audit
        PARANOID    // Debug mode with extensive checks
    };
    
    // Engine metadata structure
    struct EngineMetadata {
        int id;
        std::string name;
        std::string className;
        EngineCategory category;
        int parameterCount;
        int mixParameterIndex;
        bool isPlatinum;
        bool isStudio;
        bool requiresHighCPU;
        std::vector<std::string> parameterNames;
        std::map<std::string, std::pair<float, float>> parameterRanges;
        std::string description;
        
        EngineMetadata() : id(-1), category(EngineCategory::SPECIAL), 
                           parameterCount(0), mixParameterIndex(-1),
                           isPlatinum(false), isStudio(false), 
                           requiresHighCPU(false) {}
    };
    
    // Architecture violation types
    enum class ViolationType {
        INVALID_ENGINE_ID,
        MISSING_ENGINE,
        INCORRECT_PARAMETER_COUNT,
        INVALID_MIX_INDEX,
        FACTORY_MISMATCH,
        PARAMETER_RANGE_VIOLATION,
        MEMORY_LEAK,
        THREAD_SAFETY_VIOLATION,
        PERFORMANCE_DEGRADATION
    };
    
    // Violation record
    struct ArchitectureViolation {
        ViolationType type;
        int engineID;
        std::string engineName;
        std::string description;
        std::chrono::system_clock::time_point timestamp;
        bool critical;
    };
    
    // Performance metrics
    struct EnginePerformanceMetrics {
        double averageCPU;
        double peakCPU;
        size_t memoryUsage;
        size_t processCallCount;
        size_t resetCallCount;
        std::chrono::milliseconds totalProcessTime;
        bool hasNumericalIssues;
        bool hasMemoryLeaks;
    };
    
    // Main validation and assertion methods
    bool validateArchitecture(ValidationLevel level = ValidationLevel::STANDARD);
    bool assertEngineFactory();
    bool assertEngineMapping(int engineID);
    bool assertParameterMapping(int engineID);
    bool assertAllEngines();
    
    // Engine information retrieval
    const EngineMetadata& getEngineMetadata(int engineID) const;
    std::string getEngineName(int engineID) const;
    EngineCategory getEngineCategory(int engineID) const;
    int getMixParameterIndex(int engineID) const;
    int getParameterCount(int engineID) const;
    std::vector<int> getEnginesByCategory(EngineCategory category) const;
    
    // Parameter validation
    bool validateParameterIndex(int engineID, int paramIndex) const;
    bool validateParameterValue(int engineID, int paramIndex, float value) const;
    std::pair<float, float> getParameterRange(int engineID, int paramIndex) const;
    
    // Architecture health monitoring
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const { return monitoring.load(); }
    
    // Performance tracking
    void recordEnginePerformance(int engineID, double cpuUsage, size_t memoryUsage);
    EnginePerformanceMetrics getPerformanceMetrics(int engineID) const;
    std::vector<int> getHighCPUEngines(double threshold = 10.0) const;
    
    // Violation tracking and reporting
    void recordViolation(const ArchitectureViolation& violation);
    std::vector<ArchitectureViolation> getViolations() const;
    std::vector<ArchitectureViolation> getCriticalViolations() const;
    void clearViolations();
    
    // Architecture documentation
    void generateArchitectureReport(const std::string& filepath);
    void generateEngineMapping(const std::string& filepath);
    void generateParameterMapping(const std::string& filepath);
    void generateHealthReport(const std::string& filepath);
    
    // Factory validation
    bool validateFactoryCreatesAllEngines();
    bool validateFactoryEngineNames();
    bool validateFactoryParameterCounts();
    
    // Thread safety checks
    bool isThreadSafe(int engineID);
    void enableThreadSafetyChecks(bool enable) { threadSafetyChecks = enable; }
    
    // Architecture consistency checks
    bool checkEngineConsistency(int engineID);
    bool checkParameterConsistency(int engineID);
    bool checkMixParameterConsistency();
    
    // Engine testing utilities
    bool testEngineCreation(int engineID);
    bool testEngineProcessing(int engineID);
    bool testEngineReset(int engineID);
    bool testEngineParameters(int engineID);
    
    // Automated fixes
    bool attemptAutoFix(const ArchitectureViolation& violation);
    void enableAutoFix(bool enable) { autoFixEnabled = enable; }
    
    // Logging and debugging
    void setLogLevel(int level) { logLevel = level; }
    void logArchitectureState();
    void dumpEngineInfo(int engineID);
    
    // Architecture versioning
    std::string getArchitectureVersion() const { return "3.0.0"; }
    bool isCompatibleVersion(const std::string& version) const;
    
    // Engine registration (for dynamic engines)
    bool registerEngine(const EngineMetadata& metadata);
    bool unregisterEngine(int engineID);
    
    // Critical assertions (will halt in debug mode)
    void ASSERT_VALID_ENGINE_ID(int engineID) const;
    void ASSERT_VALID_PARAMETER(int engineID, int paramIndex) const;
    void ASSERT_ENGINE_EXISTS(int engineID) const;
    void ASSERT_FACTORY_INTEGRITY() const;
    
private:
    EngineArchitectureManager();
    ~EngineArchitectureManager();
    
    // Delete copy constructor and assignment operator
    EngineArchitectureManager(const EngineArchitectureManager&) = delete;
    EngineArchitectureManager& operator=(const EngineArchitectureManager&) = delete;
    
    // Initialize engine metadata database
    void initializeMetadata();
    void loadEngineDefinitions();
    void validateMetadataIntegrity();
    
    // Internal validation methods
    bool validateEngineInternal(int engineID);
    bool validateParametersInternal(int engineID);
    bool validateFactoryInternal();
    
    // Performance monitoring thread
    void monitoringThread();
    std::thread* monitorThread;
    std::atomic<bool> monitoring;
    std::atomic<bool> stopMonitoring;
    
    // Data storage
    std::map<int, EngineMetadata> engineMetadata;
    std::map<int, EnginePerformanceMetrics> performanceMetrics;
    std::vector<ArchitectureViolation> violations;
    
    // Synchronization
    mutable std::mutex metadataMutex;
    mutable std::mutex violationMutex;
    mutable std::mutex performanceMutex;
    
    // Configuration
    ValidationLevel currentValidationLevel;
    bool autoFixEnabled;
    bool threadSafetyChecks;
    int logLevel;
    
    // Cached processor reference
    ChimeraAudioProcessor* processor;
    
    // Statistics
    std::atomic<size_t> totalValidations;
    std::atomic<size_t> failedValidations;
    std::atomic<size_t> autoFixesApplied;
    
    // Helper methods
    void logViolation(const ArchitectureViolation& violation);
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    void logDebug(const std::string& message);
    
    // Validation helpers
    bool isValidEngineID(int id) const { 
        return id >= MIN_ENGINE_ID && id <= MAX_ENGINE_ID; 
    }
    
    bool isValidParameterIndex(int index) const {
        return index >= 0 && index < MAX_PARAMETERS_PER_ENGINE;
    }
    
    // Static engine definitions (hardcoded for reliability)
    static const std::map<int, std::pair<std::string, EngineCategory>> engineDefinitions;
    static const std::map<int, int> mixParameterIndices;
    
    // Architecture checksum for integrity verification
    uint32_t calculateArchitectureChecksum() const;
    static constexpr uint32_t EXPECTED_CHECKSUM = 0xC0DE5757; // Represents 57 engines
};

// Convenience macros for architecture assertions
#define ASSERT_ENGINE_ARCHITECTURE() \
    EngineArchitectureManager::getInstance().assertAllEngines()

#define ASSERT_ENGINE_VALID(id) \
    EngineArchitectureManager::getInstance().ASSERT_VALID_ENGINE_ID(id)

#define ASSERT_PARAMETER_VALID(engineID, paramIndex) \
    EngineArchitectureManager::getInstance().ASSERT_VALID_PARAMETER(engineID, paramIndex)

#define VALIDATE_ARCHITECTURE() \
    EngineArchitectureManager::getInstance().validateArchitecture()

#define GET_ENGINE_MANAGER() \
    EngineArchitectureManager::getInstance()

// Debug mode architecture checks
#ifdef DEBUG
    #define DEBUG_ASSERT_ARCHITECTURE() ASSERT_ENGINE_ARCHITECTURE()
    #define DEBUG_VALIDATE_ENGINE(id) \
        EngineArchitectureManager::getInstance().assertEngineMapping(id)
#else
    #define DEBUG_ASSERT_ARCHITECTURE() ((void)0)
    #define DEBUG_VALIDATE_ENGINE(id) ((void)0)
#endif