#pragma once

#include <JuceHeader.h>
#include "GoldenPreset.h"
#include "EngineFactory.h"
#include <map>

/**
 * Validates Golden Corpus presets for quality, performance, and correctness
 * Ensures all presets meet boutique quality standards
 */
class PresetValidator {
public:
    // Validation result structure
    struct ValidationResult {
        bool passed = true;
        float qualityScore = 0.0f;  // 0-100
        StringArray errors;
        StringArray warnings;
        StringArray info;
        
        // Performance metrics
        float measuredCpuPercent = 0.0f;
        float measuredLatencySamples = 0.0f;
        bool audioQualityPassed = true;
        
        // Coverage metrics
        bool parameterCoveragePassed = true;
        bool sonicUniquenessScore = 0.0f;
    };
    
    // Main validation methods
    ValidationResult validatePreset(const GoldenPreset& preset);
    ValidationResult validateCorpus(const std::vector<GoldenPreset>& presets);
    
    // Individual test categories
    bool validateStructure(const GoldenPreset& preset, ValidationResult& result);
    bool validateAudioQuality(const GoldenPreset& preset, ValidationResult& result);
    bool validatePerformance(const GoldenPreset& preset, ValidationResult& result);
    bool validateMetadata(const GoldenPreset& preset, ValidationResult& result);
    bool validateParameters(const GoldenPreset& preset, ValidationResult& result);
    
    // Corpus-wide validation
    bool validateUniqueness(const std::vector<GoldenPreset>& presets, 
                          std::vector<ValidationResult>& results);
    bool validateCoverage(const std::vector<GoldenPreset>& presets,
                         std::vector<ValidationResult>& results);
    bool validateDistribution(const std::vector<GoldenPreset>& presets,
                            std::vector<ValidationResult>& results);
    
    // Configuration
    void setSampleRate(double sampleRate) { m_sampleRate = sampleRate; }
    void setBlockSize(int blockSize) { m_blockSize = blockSize; }
    void setVerbose(bool verbose) { m_verbose = verbose; }
    
    // Quality thresholds
    struct QualityThresholds {
        float minQualityScore = 90.0f;      // Minimum acceptable quality
        float maxCpuLight = 3.0f;           // CPU threshold for LIGHT tier
        float maxCpuMedium = 8.0f;          // CPU threshold for MEDIUM tier
        float maxCpuHeavy = 15.0f;          // CPU threshold for HEAVY tier
        float maxCpuExtreme = 25.0f;        // CPU threshold for EXTREME tier
        float maxDcOffset = -60.0f;         // Maximum DC offset in dB
        float maxLatencyMs = 10.0f;         // Maximum latency in ms
        float minSonicUniqueness = 0.15f;   // Minimum uniqueness score
    };
    
    QualityThresholds thresholds;
    
    // Generate validation report
    String generateReport(const ValidationResult& result);
    String generateCorpusReport(const std::vector<ValidationResult>& results);
    
private:
    double m_sampleRate = 48000.0;
    int m_blockSize = 512;
    bool m_verbose = false;
    
    // Test signal generation
    AudioBuffer<float> generateTestSignal(int numSamples);
    AudioBuffer<float> generateSilence(int numSamples);
    AudioBuffer<float> generateImpulse(int numSamples);
    AudioBuffer<float> generateWhiteNoise(int numSamples);
    AudioBuffer<float> generateSineWave(int numSamples, float frequency);
    
    // Audio analysis
    float measureRMS(const AudioBuffer<float>& buffer);
    float measurePeak(const AudioBuffer<float>& buffer);
    float measureDCOffset(const AudioBuffer<float>& buffer);
    float measureTHD(const AudioBuffer<float>& buffer);
    
    // Performance measurement
    float measureCPUUsage(std::unique_ptr<EngineBase>& engine,
                         const AudioBuffer<float>& testSignal);
    float measureLatency(std::unique_ptr<EngineBase>& engine);
    
    // Parameter validation
    bool validateParameterRanges(const GoldenPreset& preset);
    bool validateParameterResponse(std::unique_ptr<EngineBase>& engine,
                                  const GoldenPreset& preset);
    
    // Similarity calculation for uniqueness testing
    float calculateSimilarity(const GoldenPreset& a, const GoldenPreset& b);
    float calculateVectorDistance(const std::vector<float>& a, 
                                const std::vector<float>& b);
    
    // Coverage analysis
    std::map<String, int> analyzeEngineUsage(const std::vector<GoldenPreset>& presets);
    std::map<String, int> analyzeCategoryDistribution(const std::vector<GoldenPreset>& presets);
    std::vector<float> analyzeParameterSpaceCoverage(const std::vector<GoldenPreset>& presets);
    
    // Helper to create and configure engine from preset
    std::unique_ptr<EngineBase> createEngineFromPreset(const GoldenPreset& preset, int slot);
};