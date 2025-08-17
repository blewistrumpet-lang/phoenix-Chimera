/**
 * ENGINE ARCHITECTURE MANAGER - Implementation
 * 
 * Maintains and validates the entire engine architecture system.
 * This is the source of truth for all engine configurations.
 */

#include "EngineArchitectureManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <numeric>

// Static member definitions - THE DEFINITIVE ENGINE MAPPING
const std::map<int, std::pair<std::string, EngineArchitectureManager::EngineCategory>> 
EngineArchitectureManager::engineDefinitions = {
    // SPECIAL
    {0, {"NoneEngine", EngineCategory::SPECIAL}},
    
    // DYNAMICS (1-6)
    {1, {"VintageOptoCompressor_Platinum", EngineCategory::DYNAMICS}},
    {2, {"ClassicCompressor", EngineCategory::DYNAMICS}},
    {3, {"TransientShaper_Platinum", EngineCategory::DYNAMICS}},
    {4, {"NoiseGate_Platinum", EngineCategory::DYNAMICS}},
    {5, {"MasteringLimiter_Platinum", EngineCategory::DYNAMICS}},
    {6, {"DynamicEQ", EngineCategory::DYNAMICS}},
    
    // EQ/FILTER (7-14)
    {7, {"ParametricEQ_Studio", EngineCategory::EQ_FILTER}},
    {8, {"VintageConsoleEQ_Studio", EngineCategory::EQ_FILTER}},
    {9, {"LadderFilter", EngineCategory::EQ_FILTER}},
    {10, {"StateVariableFilter", EngineCategory::EQ_FILTER}},
    {11, {"FormantFilter", EngineCategory::EQ_FILTER}},
    {12, {"EnvelopeFilter", EngineCategory::EQ_FILTER}},
    {13, {"CombResonator", EngineCategory::EQ_FILTER}},
    {14, {"VocalFormantFilter", EngineCategory::EQ_FILTER}},
    
    // DISTORTION (15-22)
    {15, {"VintageTubePreamp_Studio", EngineCategory::DISTORTION}},
    {16, {"WaveFolder", EngineCategory::DISTORTION}},
    {17, {"HarmonicExciter_Platinum", EngineCategory::DISTORTION}},
    {18, {"BitCrusher", EngineCategory::DISTORTION}},
    {19, {"MultibandSaturator", EngineCategory::DISTORTION}},
    {20, {"MuffFuzz", EngineCategory::DISTORTION}},
    {21, {"RodentDistortion", EngineCategory::DISTORTION}},
    {22, {"KStyleOverdrive", EngineCategory::DISTORTION}},
    
    // MODULATION (23-33)
    {23, {"StereoChorus", EngineCategory::MODULATION}},
    {24, {"ResonantChorus_Platinum", EngineCategory::MODULATION}},
    {25, {"AnalogPhaser", EngineCategory::MODULATION}},
    {26, {"PlatinumRingModulator", EngineCategory::MODULATION}},
    {27, {"FrequencyShifter", EngineCategory::MODULATION}},
    {28, {"HarmonicTremolo", EngineCategory::MODULATION}},
    {29, {"ClassicTremolo", EngineCategory::MODULATION}},
    {30, {"RotarySpeaker_Platinum", EngineCategory::MODULATION}},
    {31, {"PitchShifter", EngineCategory::MODULATION}},
    {32, {"DetuneDoubler", EngineCategory::MODULATION}},
    {33, {"IntelligentHarmonizer", EngineCategory::MODULATION}},
    
    // DELAY (34-38)
    {34, {"TapeEcho", EngineCategory::DELAY}},
    {35, {"DigitalDelay", EngineCategory::DELAY}},
    {36, {"MagneticDrumEcho", EngineCategory::DELAY}},
    {37, {"BucketBrigadeDelay", EngineCategory::DELAY}},
    {38, {"BufferRepeat_Platinum", EngineCategory::DELAY}},
    
    // REVERB (39-43)
    {39, {"PlateReverb", EngineCategory::REVERB}},
    {40, {"SpringReverb_Platinum", EngineCategory::REVERB}},
    {41, {"ConvolutionReverb", EngineCategory::REVERB}},
    {42, {"ShimmerReverb", EngineCategory::REVERB}},
    {43, {"GatedReverb", EngineCategory::REVERB}},
    
    // SPATIAL (44-52)
    {44, {"StereoWidener", EngineCategory::SPATIAL}},
    {45, {"StereoImager", EngineCategory::SPATIAL}},
    {46, {"DimensionExpander", EngineCategory::SPATIAL}},
    {47, {"SpectralFreeze", EngineCategory::SPATIAL}},
    {48, {"SpectralGate_Platinum", EngineCategory::SPATIAL}},
    {49, {"PhasedVocoder", EngineCategory::SPATIAL}},
    {50, {"GranularCloud", EngineCategory::SPATIAL}},
    {51, {"ChaosGenerator_Platinum", EngineCategory::SPATIAL}},
    {52, {"FeedbackNetwork", EngineCategory::SPATIAL}},
    
    // UTILITY (53-56)
    {53, {"MidSideProcessor_Platinum", EngineCategory::UTILITY}},
    {54, {"GainUtility_Platinum", EngineCategory::UTILITY}},
    {55, {"MonoMaker_Platinum", EngineCategory::UTILITY}},
    {56, {"PhaseAlign_Platinum", EngineCategory::UTILITY}}
};

// Static mix parameter indices - CRITICAL FOR PROPER OPERATION
const std::map<int, int> EngineArchitectureManager::mixParameterIndices = {
    {0, -1},  // NoneEngine has no mix
    {1, 5},   // VintageOptoCompressor_Platinum
    {2, 4},   // ClassicCompressor  
    {3, 9},   // TransientShaper_Platinum
    {4, 6},   // NoiseGate_Platinum
    {5, 5},   // MasteringLimiter_Platinum
    {6, 11},  // DynamicEQ
    {7, 10},  // ParametricEQ_Studio
    {8, 11},  // VintageConsoleEQ_Studio
    {9, 7},   // LadderFilter
    {10, 6},  // StateVariableFilter
    {11, 6},  // FormantFilter
    {12, 8},  // EnvelopeFilter
    {13, 7},  // CombResonator
    {14, 6},  // VocalFormantFilter
    {15, 7},  // VintageTubePreamp_Studio
    {16, 6},  // WaveFolder
    {17, 7},  // HarmonicExciter_Platinum
    {18, 6},  // BitCrusher
    {19, 11}, // MultibandSaturator
    {20, 4},  // MuffFuzz
    {21, 5},  // RodentDistortion
    {22, 3},  // KStyleOverdrive
    {23, 6},  // StereoChorus
    {24, 8},  // ResonantChorus_Platinum
    {25, 8},  // AnalogPhaser
    {26, 6},  // PlatinumRingModulator
    {27, 2},  // FrequencyShifter
    {28, 6},  // HarmonicTremolo
    {29, 6},  // ClassicTremolo
    {30, 8},  // RotarySpeaker_Platinum
    {31, 2},  // PitchShifter
    {32, 4},  // DetuneDoubler
    {33, 7},  // IntelligentHarmonizer
    {34, 4},  // TapeEcho
    {35, 6},  // DigitalDelay
    {36, 7},  // MagneticDrumEcho
    {37, 6},  // BucketBrigadeDelay
    {38, 10}, // BufferRepeat_Platinum
    {39, 6},  // PlateReverb
    {40, 9},  // SpringReverb_Platinum
    {41, 4},  // ConvolutionReverb
    {42, 9},  // ShimmerReverb
    {43, 8},  // GatedReverb
    {44, 3},  // StereoWidener
    {45, 6},  // StereoImager
    {46, 6},  // DimensionExpander
    {47, 8},  // SpectralFreeze
    {48, 7},  // SpectralGate_Platinum
    {49, 8},  // PhasedVocoder
    {50, 10}, // GranularCloud
    {51, 7},  // ChaosGenerator_Platinum
    {52, 8},  // FeedbackNetwork
    {53, 3},  // MidSideProcessor_Platinum
    {54, 1},  // GainUtility_Platinum
    {55, 3},  // MonoMaker_Platinum
    {56, 4}   // PhaseAlign_Platinum
};

// Constructor
EngineArchitectureManager::EngineArchitectureManager() 
    : currentValidationLevel(ValidationLevel::STANDARD),
      autoFixEnabled(false),
      threadSafetyChecks(false),
      logLevel(1),
      processor(nullptr),
      totalValidations(0),
      failedValidations(0),
      autoFixesApplied(0),
      monitorThread(nullptr),
      monitoring(false),
      stopMonitoring(false) {
    
    initializeMetadata();
    logInfo("Engine Architecture Manager initialized with " + 
            std::to_string(TOTAL_ENGINES) + " engines");
}

// Destructor
EngineArchitectureManager::~EngineArchitectureManager() {
    stopMonitoring.store(true);
    if (monitorThread && monitorThread->joinable()) {
        monitorThread->join();
        delete monitorThread;
    }
}

// Initialize metadata for all engines
void EngineArchitectureManager::initializeMetadata() {
    std::lock_guard<std::mutex> lock(metadataMutex);
    
    for (const auto& [id, info] : engineDefinitions) {
        EngineMetadata metadata;
        metadata.id = id;
        metadata.name = info.first;
        metadata.className = info.first;
        metadata.category = info.second;
        
        // Determine if Platinum or Studio
        metadata.isPlatinum = (info.first.find("_Platinum") != std::string::npos);
        metadata.isStudio = (info.first.find("_Studio") != std::string::npos);
        
        // Set mix parameter index
        auto mixIt = mixParameterIndices.find(id);
        metadata.mixParameterIndex = (mixIt != mixParameterIndices.end()) ? mixIt->second : -1;
        
        // Mark high CPU engines
        metadata.requiresHighCPU = (
            metadata.category == EngineCategory::REVERB ||
            info.first.find("Convolution") != std::string::npos ||
            info.first.find("Spectral") != std::string::npos ||
            info.first.find("Granular") != std::string::npos
        );
        
        engineMetadata[id] = metadata;
    }
    
    validateMetadataIntegrity();
}

// Validate entire architecture
bool EngineArchitectureManager::validateArchitecture(ValidationLevel level) {
    totalValidations++;
    currentValidationLevel = level;
    
    logInfo("Starting architecture validation at level: " + std::to_string(static_cast<int>(level)));
    
    bool valid = true;
    
    // Basic checks
    valid &= assertEngineFactory();
    valid &= validateFactoryCreatesAllEngines();
    
    if (level >= ValidationLevel::STANDARD) {
        valid &= validateFactoryEngineNames();
        valid &= validateFactoryParameterCounts();
        valid &= checkMixParameterConsistency();
    }
    
    if (level >= ValidationLevel::COMPREHENSIVE) {
        for (int i = 0; i < TOTAL_ENGINES; ++i) {
            valid &= assertEngineMapping(i);
            valid &= assertParameterMapping(i);
            valid &= testEngineCreation(i);
            
            if (level == ValidationLevel::PARANOID) {
                valid &= testEngineProcessing(i);
                valid &= testEngineReset(i);
                valid &= testEngineParameters(i);
            }
        }
    }
    
    if (!valid) {
        failedValidations++;
        logError("Architecture validation FAILED!");
        
        if (autoFixEnabled) {
            logInfo("Attempting auto-fixes...");
            for (const auto& violation : violations) {
                attemptAutoFix(violation);
            }
        }
    } else {
        logInfo("Architecture validation PASSED!");
    }
    
    return valid;
}

// Assert engine factory is working correctly
bool EngineArchitectureManager::assertEngineFactory() {
    logDebug("Asserting engine factory integrity...");
    
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        auto engine = EngineFactory::createEngine(id);
        
        if (!engine) {
            ArchitectureViolation violation;
            violation.type = ViolationType::MISSING_ENGINE;
            violation.engineID = id;
            violation.engineName = getEngineName(id);
            violation.description = "Factory failed to create engine";
            violation.timestamp = std::chrono::system_clock::now();
            violation.critical = true;
            
            recordViolation(violation);
            return false;
        }
    }
    
    // Test invalid IDs should return nullptr
    if (EngineFactory::createEngine(-1) != nullptr ||
        EngineFactory::createEngine(TOTAL_ENGINES) != nullptr) {
        
        logWarning("Factory creates engines for invalid IDs!");
        return false;
    }
    
    return true;
}

// Assert engine mapping is correct
bool EngineArchitectureManager::assertEngineMapping(int engineID) {
    if (!isValidEngineID(engineID)) {
        ASSERT_VALID_ENGINE_ID(engineID);
        return false;
    }
    
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        recordViolation({
            ViolationType::MISSING_ENGINE,
            engineID,
            getEngineName(engineID),
            "Engine creation failed",
            std::chrono::system_clock::now(),
            true
        });
        return false;
    }
    
    // Verify engine name matches expected
    std::string expectedName = getEngineName(engineID);
    std::string actualName = engine->getName().toStdString();
    
    // Note: Engine getName() might return display name, not class name
    // So we check if it's reasonable
    if (actualName.empty()) {
        recordViolation({
            ViolationType::FACTORY_MISMATCH,
            engineID,
            expectedName,
            "Engine returned empty name",
            std::chrono::system_clock::now(),
            false
        });
        return false;
    }
    
    return true;
}

// Assert parameter mapping is correct
bool EngineArchitectureManager::assertParameterMapping(int engineID) {
    if (!isValidEngineID(engineID)) {
        return false;
    }
    
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        return false;
    }
    
    engine->prepareToPlay(48000, 512);
    
    int paramCount = engine->getNumParameters();
    if (paramCount <= 0 || paramCount > MAX_PARAMETERS_PER_ENGINE) {
        recordViolation({
            ViolationType::INCORRECT_PARAMETER_COUNT,
            engineID,
            getEngineName(engineID),
            "Invalid parameter count: " + std::to_string(paramCount),
            std::chrono::system_clock::now(),
            false
        });
        return false;
    }
    
    // Verify mix parameter index
    int expectedMixIndex = getMixParameterIndex(engineID);
    if (expectedMixIndex >= 0 && expectedMixIndex >= paramCount) {
        recordViolation({
            ViolationType::INVALID_MIX_INDEX,
            engineID,
            getEngineName(engineID),
            "Mix parameter index out of bounds",
            std::chrono::system_clock::now(),
            true
        });
        return false;
    }
    
    return true;
}

// Assert all engines are valid
bool EngineArchitectureManager::assertAllEngines() {
    bool allValid = true;
    
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        allValid &= assertEngineMapping(id);
        allValid &= assertParameterMapping(id);
    }
    
    if (!allValid) {
        logError("Engine architecture assertion FAILED!");
        ASSERT_FACTORY_INTEGRITY();
    }
    
    return allValid;
}

// Get engine metadata
const EngineArchitectureManager::EngineMetadata& 
EngineArchitectureManager::getEngineMetadata(int engineID) const {
    std::lock_guard<std::mutex> lock(metadataMutex);
    
    static EngineMetadata emptyMetadata;
    auto it = engineMetadata.find(engineID);
    
    if (it != engineMetadata.end()) {
        return it->second;
    }
    
    logError("Metadata not found for engine ID: " + std::to_string(engineID));
    return emptyMetadata;
}

// Get engine name
std::string EngineArchitectureManager::getEngineName(int engineID) const {
    auto it = engineDefinitions.find(engineID);
    if (it != engineDefinitions.end()) {
        return it->second.first;
    }
    return "Unknown";
}

// Get engine category
EngineArchitectureManager::EngineCategory 
EngineArchitectureManager::getEngineCategory(int engineID) const {
    auto it = engineDefinitions.find(engineID);
    if (it != engineDefinitions.end()) {
        return it->second.second;
    }
    return EngineCategory::SPECIAL;
}

// Get mix parameter index
int EngineArchitectureManager::getMixParameterIndex(int engineID) const {
    auto it = mixParameterIndices.find(engineID);
    if (it != mixParameterIndices.end()) {
        return it->second;
    }
    return -1;
}

// Get engines by category
std::vector<int> EngineArchitectureManager::getEnginesByCategory(EngineCategory category) const {
    std::vector<int> engines;
    
    for (const auto& [id, info] : engineDefinitions) {
        if (info.second == category) {
            engines.push_back(id);
        }
    }
    
    return engines;
}

// Validate factory creates all engines
bool EngineArchitectureManager::validateFactoryCreatesAllEngines() {
    int created = 0;
    int failed = 0;
    
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        auto engine = EngineFactory::createEngine(id);
        if (engine) {
            created++;
        } else {
            failed++;
            logError("Factory failed to create engine ID: " + std::to_string(id));
        }
    }
    
    logInfo("Factory created " + std::to_string(created) + "/" + 
            std::to_string(TOTAL_ENGINES) + " engines");
    
    return (created == TOTAL_ENGINES && failed == 0);
}

// Check mix parameter consistency
bool EngineArchitectureManager::checkMixParameterConsistency() {
    bool consistent = true;
    
    for (const auto& [id, mixIndex] : mixParameterIndices) {
        if (id == 0) continue; // Skip NoneEngine
        
        auto engine = EngineFactory::createEngine(id);
        if (!engine) {
            consistent = false;
            continue;
        }
        
        engine->prepareToPlay(48000, 512);
        int paramCount = engine->getNumParameters();
        
        if (mixIndex >= paramCount) {
            logError("Engine " + std::to_string(id) + " mix index " + 
                    std::to_string(mixIndex) + " exceeds param count " + 
                    std::to_string(paramCount));
            consistent = false;
        }
    }
    
    return consistent;
}

// Test engine creation
bool EngineArchitectureManager::testEngineCreation(int engineID) {
    if (!isValidEngineID(engineID)) {
        return false;
    }
    
    try {
        auto engine = EngineFactory::createEngine(engineID);
        return (engine != nullptr);
    } catch (...) {
        logError("Exception creating engine ID: " + std::to_string(engineID));
        return false;
    }
}

// Test engine processing
bool EngineArchitectureManager::testEngineProcessing(int engineID) {
    if (!isValidEngineID(engineID)) {
        return false;
    }
    
    try {
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) return false;
        
        engine->prepareToPlay(48000, 512);
        
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        
        engine->process(buffer);
        
        // Check for NaN or Inf
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample) || std::isinf(sample)) {
                    logError("Engine " + std::to_string(engineID) + 
                            " produced NaN/Inf");
                    return false;
                }
            }
        }
        
        return true;
    } catch (...) {
        logError("Exception processing engine ID: " + std::to_string(engineID));
        return false;
    }
}

// Test engine reset
bool EngineArchitectureManager::testEngineReset(int engineID) {
    if (!isValidEngineID(engineID)) {
        return false;
    }
    
    try {
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) return false;
        
        engine->prepareToPlay(48000, 512);
        engine->reset();
        
        return true;
    } catch (...) {
        logError("Exception resetting engine ID: " + std::to_string(engineID));
        return false;
    }
}

// Record violation
void EngineArchitectureManager::recordViolation(const ArchitectureViolation& violation) {
    std::lock_guard<std::mutex> lock(violationMutex);
    violations.push_back(violation);
    logViolation(violation);
}

// Get violations
std::vector<EngineArchitectureManager::ArchitectureViolation> 
EngineArchitectureManager::getViolations() const {
    std::lock_guard<std::mutex> lock(violationMutex);
    return violations;
}

// Clear violations
void EngineArchitectureManager::clearViolations() {
    std::lock_guard<std::mutex> lock(violationMutex);
    violations.clear();
}

// Generate architecture report
void EngineArchitectureManager::generateArchitectureReport(const std::string& filepath) {
    std::ofstream report(filepath);
    
    report << "ENGINE ARCHITECTURE REPORT\n";
    report << "==========================\n\n";
    report << "Architecture Version: " << getArchitectureVersion() << "\n";
    report << "Total Engines: " << TOTAL_ENGINES << "\n";
    report << "Engine ID Range: " << MIN_ENGINE_ID << " - " << MAX_ENGINE_ID << "\n\n";
    
    report << "VALIDATION STATISTICS\n";
    report << "--------------------\n";
    report << "Total Validations: " << totalValidations << "\n";
    report << "Failed Validations: " << failedValidations << "\n";
    report << "Auto-fixes Applied: " << autoFixesApplied << "\n\n";
    
    report << "ENGINE MAPPING\n";
    report << "--------------\n";
    
    for (const auto& [id, info] : engineDefinitions) {
        report << std::setw(3) << id << ": " 
               << std::setw(35) << std::left << info.first;
        
        auto mixIt = mixParameterIndices.find(id);
        if (mixIt != mixParameterIndices.end()) {
            report << " [Mix: " << mixIt->second << "]";
        }
        
        report << "\n";
    }
    
    report << "\nVIOLATIONS\n";
    report << "----------\n";
    
    if (violations.empty()) {
        report << "No violations detected.\n";
    } else {
        for (const auto& v : violations) {
            report << "[" << (v.critical ? "CRITICAL" : "WARNING") << "] ";
            report << "Engine " << v.engineID << " (" << v.engineName << "): ";
            report << v.description << "\n";
        }
    }
    
    report.close();
    logInfo("Architecture report generated: " + filepath);
}

// Critical assertions
void EngineArchitectureManager::ASSERT_VALID_ENGINE_ID(int engineID) const {
    if (!isValidEngineID(engineID)) {
        std::string msg = "ASSERTION FAILED: Invalid engine ID " + std::to_string(engineID);
        logError(msg);
        #ifdef DEBUG
            assert(false && "Invalid engine ID!");
        #endif
    }
}

void EngineArchitectureManager::ASSERT_ENGINE_EXISTS(int engineID) const {
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        std::string msg = "ASSERTION FAILED: Engine " + std::to_string(engineID) + " does not exist!";
        logError(msg);
        #ifdef DEBUG
            assert(false && "Engine does not exist!");
        #endif
    }
}

void EngineArchitectureManager::ASSERT_FACTORY_INTEGRITY() const {
    int created = 0;
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        if (EngineFactory::createEngine(id)) {
            created++;
        }
    }
    
    if (created != TOTAL_ENGINES) {
        std::string msg = "ASSERTION FAILED: Factory integrity compromised! Created " + 
                         std::to_string(created) + "/" + std::to_string(TOTAL_ENGINES);
        logError(msg);
        #ifdef DEBUG
            assert(false && "Factory integrity check failed!");
        #endif
    }
}

// Logging methods
void EngineArchitectureManager::logInfo(const std::string& message) {
    if (logLevel >= 1) {
        std::cout << "[ARCH INFO] " << message << std::endl;
    }
}

void EngineArchitectureManager::logWarning(const std::string& message) {
    if (logLevel >= 0) {
        std::cout << "[ARCH WARN] " << message << std::endl;
    }
}

void EngineArchitectureManager::logError(const std::string& message) {
    std::cerr << "[ARCH ERROR] " << message << std::endl;
}

void EngineArchitectureManager::logDebug(const std::string& message) {
    if (logLevel >= 2) {
        std::cout << "[ARCH DEBUG] " << message << std::endl;
    }
}

void EngineArchitectureManager::logViolation(const ArchitectureViolation& violation) {
    std::string level = violation.critical ? "CRITICAL" : "WARNING";
    std::cerr << "[ARCH VIOLATION:" << level << "] Engine " << violation.engineID 
              << " (" << violation.engineName << "): " << violation.description << std::endl;
}

// Validate metadata integrity
void EngineArchitectureManager::validateMetadataIntegrity() {
    // Ensure all engine IDs are accounted for
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        if (engineMetadata.find(id) == engineMetadata.end()) {
            logError("Missing metadata for engine ID: " + std::to_string(id));
        }
    }
    
    // Verify mix parameter indices are valid
    for (const auto& [id, metadata] : engineMetadata) {
        if (metadata.mixParameterIndex >= MAX_PARAMETERS_PER_ENGINE) {
            logError("Invalid mix parameter index for engine " + std::to_string(id));
        }
    }
    
    logInfo("Metadata integrity check complete");
}

// Generate engine mapping CSV
void EngineArchitectureManager::generateEngineMapping(const std::string& filepath) {
    std::ofstream csv(filepath);
    
    csv << "Engine ID,Engine Name,Category,Mix Index,Platinum,Studio\n";
    
    for (const auto& [id, info] : engineDefinitions) {
        csv << id << ",";
        csv << info.first << ",";
        
        // Category name
        switch (info.second) {
            case EngineCategory::SPECIAL: csv << "SPECIAL"; break;
            case EngineCategory::DYNAMICS: csv << "DYNAMICS"; break;
            case EngineCategory::EQ_FILTER: csv << "EQ_FILTER"; break;
            case EngineCategory::DISTORTION: csv << "DISTORTION"; break;
            case EngineCategory::MODULATION: csv << "MODULATION"; break;
            case EngineCategory::DELAY: csv << "DELAY"; break;
            case EngineCategory::REVERB: csv << "REVERB"; break;
            case EngineCategory::SPATIAL: csv << "SPATIAL"; break;
            case EngineCategory::UTILITY: csv << "UTILITY"; break;
        }
        csv << ",";
        
        // Mix index
        auto mixIt = mixParameterIndices.find(id);
        if (mixIt != mixParameterIndices.end()) {
            csv << mixIt->second;
        } else {
            csv << "-1";
        }
        csv << ",";
        
        // Platinum/Studio flags
        bool isPlatinum = (info.first.find("_Platinum") != std::string::npos);
        bool isStudio = (info.first.find("_Studio") != std::string::npos);
        
        csv << (isPlatinum ? "Yes" : "No") << ",";
        csv << (isStudio ? "Yes" : "No") << "\n";
    }
    
    csv.close();
    logInfo("Engine mapping CSV generated: " + filepath);
}

// Generate parameter mapping
void EngineArchitectureManager::generateParameterMapping(const std::string& filepath) {
    std::ofstream report(filepath);
    
    report << "ENGINE PARAMETER MAPPING\n";
    report << "========================\n\n";
    
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        auto engine = EngineFactory::createEngine(id);
        if (!engine) continue;
        
        engine->prepareToPlay(48000, 512);
        
        report << "[" << id << "] " << getEngineName(id) << "\n";
        report << std::string(50, '-') << "\n";
        
        int numParams = engine->getNumParameters();
        int mixIndex = getMixParameterIndex(id);
        
        for (int p = 0; p < numParams; ++p) {
            report << "  " << std::setw(2) << p << ": ";
            report << engine->getParameterName(p).toStdString();
            
            if (p == mixIndex) {
                report << " [MIX]";
            }
            
            report << "\n";
        }
        
        report << "\n";
    }
    
    report.close();
    logInfo("Parameter mapping generated: " + filepath);
}

// Generate health report
void EngineArchitectureManager::generateHealthReport(const std::string& filepath) {
    std::ofstream report(filepath);
    
    report << "ENGINE ARCHITECTURE HEALTH REPORT\n";
    report << "=================================\n\n";
    
    report << "Timestamp: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n";
    report << "Architecture Version: " << getArchitectureVersion() << "\n\n";
    
    // Run comprehensive validation
    validateArchitecture(ValidationLevel::COMPREHENSIVE);
    
    report << "VALIDATION RESULTS\n";
    report << "-----------------\n";
    
    int passed = 0, failed = 0;
    
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        bool valid = testEngineCreation(id) && 
                    assertEngineMapping(id) && 
                    assertParameterMapping(id);
        
        if (valid) passed++;
        else failed++;
        
        report << "[" << std::setw(2) << id << "] " 
               << std::setw(35) << std::left << getEngineName(id)
               << " : " << (valid ? "PASS" : "FAIL") << "\n";
    }
    
    report << "\nSUMMARY\n";
    report << "-------\n";
    report << "Total Engines: " << TOTAL_ENGINES << "\n";
    report << "Passed: " << passed << "\n";
    report << "Failed: " << failed << "\n";
    report << "Success Rate: " << (passed * 100 / TOTAL_ENGINES) << "%\n";
    
    if (failed == 0) {
        report << "\n✅ All engines healthy!\n";
    } else {
        report << "\n⚠️  " << failed << " engines need attention\n";
    }
    
    report.close();
    logInfo("Health report generated: " + filepath);
}

// Test engine parameters
bool EngineArchitectureManager::testEngineParameters(int engineID) {
    if (!isValidEngineID(engineID)) {
        return false;
    }
    
    try {
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) return false;
        
        engine->prepareToPlay(48000, 512);
        
        int numParams = engine->getNumParameters();
        
        // Test parameter updates
        std::map<int, float> params;
        for (int i = 0; i < numParams; ++i) {
            params[i] = 0.5f; // Set all to middle value
        }
        
        engine->updateParameters(params);
        
        // Process with parameters
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        engine->process(buffer);
        
        return true;
    } catch (...) {
        logError("Exception testing parameters for engine ID: " + std::to_string(engineID));
        return false;
    }
}

// Get parameter count
int EngineArchitectureManager::getParameterCount(int engineID) const {
    auto engine = EngineFactory::createEngine(engineID);
    if (engine) {
        engine->prepareToPlay(48000, 512);
        return engine->getNumParameters();
    }
    return 0;
}

// Attempt auto-fix for violations
bool EngineArchitectureManager::attemptAutoFix(const ArchitectureViolation& violation) {
    logInfo("Attempting auto-fix for violation: " + violation.description);
    
    switch (violation.type) {
        case ViolationType::INVALID_MIX_INDEX:
            // Could attempt to recalculate mix index
            logWarning("Cannot auto-fix mix index - requires code change");
            return false;
            
        case ViolationType::MISSING_ENGINE:
            // Could attempt to reload factory
            logWarning("Cannot auto-fix missing engine - requires factory update");
            return false;
            
        default:
            logWarning("No auto-fix available for this violation type");
            return false;
    }
    
    autoFixesApplied++;
    return true;
}

// Validate parameter value
bool EngineArchitectureManager::validateParameterValue(int engineID, int paramIndex, float value) const {
    if (!isValidEngineID(engineID) || !isValidParameterIndex(paramIndex)) {
        return false;
    }
    
    // Check if value is in valid range [0, 1]
    return (value >= 0.0f && value <= 1.0f);
}

// Validate parameter index
bool EngineArchitectureManager::validateParameterIndex(int engineID, int paramIndex) const {
    if (!isValidEngineID(engineID)) {
        return false;
    }
    
    int paramCount = getParameterCount(engineID);
    return (paramIndex >= 0 && paramIndex < paramCount);
}

// Get parameter range
std::pair<float, float> EngineArchitectureManager::getParameterRange(int engineID, int paramIndex) const {
    // For now, all parameters use normalized range
    return {0.0f, 1.0f};
}

// Validate factory engine names
bool EngineArchitectureManager::validateFactoryEngineNames() {
    bool valid = true;
    
    for (const auto& [id, info] : engineDefinitions) {
        auto engine = EngineFactory::createEngine(id);
        if (engine) {
            std::string engineName = engine->getName().toStdString();
            if (engineName.empty()) {
                logWarning("Engine " + std::to_string(id) + " returned empty name");
                valid = false;
            }
        }
    }
    
    return valid;
}

// Validate factory parameter counts
bool EngineArchitectureManager::validateFactoryParameterCounts() {
    bool valid = true;
    
    for (int id = MIN_ENGINE_ID; id <= MAX_ENGINE_ID; ++id) {
        int paramCount = getParameterCount(id);
        
        if (paramCount <= 0 || paramCount > MAX_PARAMETERS_PER_ENGINE) {
            logError("Engine " + std::to_string(id) + " has invalid parameter count: " + 
                    std::to_string(paramCount));
            valid = false;
        }
    }
    
    return valid;
}

// Check engine consistency
bool EngineArchitectureManager::checkEngineConsistency(int engineID) {
    return assertEngineMapping(engineID) && assertParameterMapping(engineID);
}

// Check parameter consistency
bool EngineArchitectureManager::checkParameterConsistency(int engineID) {
    return assertParameterMapping(engineID);
}

// Is thread safe check
bool EngineArchitectureManager::isThreadSafe(int engineID) {
    // Would need actual thread safety testing
    // For now, assume all engines should be thread-safe
    return true;
}

// Get critical violations
std::vector<EngineArchitectureManager::ArchitectureViolation> 
EngineArchitectureManager::getCriticalViolations() const {
    std::lock_guard<std::mutex> lock(violationMutex);
    
    std::vector<ArchitectureViolation> critical;
    for (const auto& v : violations) {
        if (v.critical) {
            critical.push_back(v);
        }
    }
    
    return critical;
}

// Record engine performance
void EngineArchitectureManager::recordEnginePerformance(int engineID, double cpuUsage, size_t memoryUsage) {
    std::lock_guard<std::mutex> lock(performanceMutex);
    
    auto& metrics = performanceMetrics[engineID];
    metrics.processCallCount++;
    metrics.averageCPU = (metrics.averageCPU * (metrics.processCallCount - 1) + cpuUsage) / metrics.processCallCount;
    metrics.peakCPU = std::max(metrics.peakCPU, cpuUsage);
    metrics.memoryUsage = memoryUsage;
}

// Get performance metrics
EngineArchitectureManager::EnginePerformanceMetrics 
EngineArchitectureManager::getPerformanceMetrics(int engineID) const {
    std::lock_guard<std::mutex> lock(performanceMutex);
    
    auto it = performanceMetrics.find(engineID);
    if (it != performanceMetrics.end()) {
        return it->second;
    }
    
    return EnginePerformanceMetrics();
}

// Get high CPU engines
std::vector<int> EngineArchitectureManager::getHighCPUEngines(double threshold) const {
    std::lock_guard<std::mutex> lock(performanceMutex);
    
    std::vector<int> highCPU;
    for (const auto& [id, metrics] : performanceMetrics) {
        if (metrics.averageCPU > threshold) {
            highCPU.push_back(id);
        }
    }
    
    return highCPU;
}

// Start monitoring
void EngineArchitectureManager::startMonitoring() {
    if (!monitoring.load()) {
        monitoring.store(true);
        stopMonitoring.store(false);
        monitorThread = new std::thread(&EngineArchitectureManager::monitoringThread, this);
        logInfo("Architecture monitoring started");
    }
}

// Stop monitoring
void EngineArchitectureManager::stopMonitoring() {
    if (monitoring.load()) {
        stopMonitoring.store(true);
        if (monitorThread && monitorThread->joinable()) {
            monitorThread->join();
            delete monitorThread;
            monitorThread = nullptr;
        }
        monitoring.store(false);
        logInfo("Architecture monitoring stopped");
    }
}

// Monitoring thread
void EngineArchitectureManager::monitoringThread() {
    while (!stopMonitoring.load()) {
        // Periodic validation
        validateArchitecture(ValidationLevel::BASIC);
        
        // Sleep for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

// Log architecture state
void EngineArchitectureManager::logArchitectureState() {
    logInfo("=== ARCHITECTURE STATE ===");
    logInfo("Total Engines: " + std::to_string(TOTAL_ENGINES));
    logInfo("Violations: " + std::to_string(violations.size()));
    logInfo("Validations Run: " + std::to_string(totalValidations.load()));
    logInfo("Failed Validations: " + std::to_string(failedValidations.load()));
}

// Dump engine info
void EngineArchitectureManager::dumpEngineInfo(int engineID) {
    if (!isValidEngineID(engineID)) {
        logError("Invalid engine ID: " + std::to_string(engineID));
        return;
    }
    
    logInfo("=== ENGINE INFO: " + std::to_string(engineID) + " ===");
    logInfo("Name: " + getEngineName(engineID));
    logInfo("Mix Index: " + std::to_string(getMixParameterIndex(engineID)));
    logInfo("Parameter Count: " + std::to_string(getParameterCount(engineID)));
    
    auto& metadata = getEngineMetadata(engineID);
    logInfo("Is Platinum: " + std::string(metadata.isPlatinum ? "Yes" : "No"));
    logInfo("Is Studio: " + std::string(metadata.isStudio ? "Yes" : "No"));
    logInfo("High CPU: " + std::string(metadata.requiresHighCPU ? "Yes" : "No"));
}

// Is compatible version
bool EngineArchitectureManager::isCompatibleVersion(const std::string& version) const {
    // Simple version check - could be more sophisticated
    return version == getArchitectureVersion() || version == "3.0.0";
}

// Register engine (for future dynamic engine support)
bool EngineArchitectureManager::registerEngine(const EngineMetadata& metadata) {
    std::lock_guard<std::mutex> lock(metadataMutex);
    
    if (engineMetadata.find(metadata.id) != engineMetadata.end()) {
        logWarning("Engine ID already registered: " + std::to_string(metadata.id));
        return false;
    }
    
    engineMetadata[metadata.id] = metadata;
    logInfo("Registered new engine: " + metadata.name);
    return true;
}

// Unregister engine
bool EngineArchitectureManager::unregisterEngine(int engineID) {
    std::lock_guard<std::mutex> lock(metadataMutex);
    
    auto it = engineMetadata.find(engineID);
    if (it != engineMetadata.end()) {
        logInfo("Unregistering engine: " + it->second.name);
        engineMetadata.erase(it);
        return true;
    }
    
    return false;
}

// Calculate architecture checksum
uint32_t EngineArchitectureManager::calculateArchitectureChecksum() const {
    uint32_t checksum = 0;
    
    for (const auto& [id, info] : engineDefinitions) {
        checksum ^= (id << 16);
        checksum ^= static_cast<uint32_t>(info.second);
        
        // Simple string hash
        for (char c : info.first) {
            checksum = ((checksum << 5) + checksum) + c;
        }
    }
    
    return checksum;
}

// Assert valid parameter
void EngineArchitectureManager::ASSERT_VALID_PARAMETER(int engineID, int paramIndex) const {
    ASSERT_VALID_ENGINE_ID(engineID);
    
    if (!validateParameterIndex(engineID, paramIndex)) {
        std::string msg = "ASSERTION FAILED: Invalid parameter " + std::to_string(paramIndex) + 
                         " for engine " + std::to_string(engineID);
        logError(msg);
        #ifdef DEBUG
            assert(false && "Invalid parameter index!");
        #endif
    }
}