#pragma once
#include <JuceHeader.h>
#include "EngineBase.h"
#include "TestResults.h"
#include <memory>
#include <vector>
#include <functional>

/**
 * Comprehensive quality testing framework for all Chimera Phoenix engines
 * Tests audio quality, functionality, DSP performance, and boutique features
 */
class EngineQualityTest {
public:
    EngineQualityTest();
    ~EngineQualityTest() = default;
    
    // Main test execution
    TestResults runAllTests(std::unique_ptr<EngineBase>& engine, int engineType);
    TestResults runTestSuite(std::unique_ptr<EngineBase>& engine, int engineType, 
                           const std::string& suiteName);
    
    // Individual test categories
    AudioQualityResults testAudioQuality(EngineBase* engine);
    FunctionalTestResults testFunctionality(EngineBase* engine, int engineType);
    DSPQualityResults testDSPQuality(EngineBase* engine);
    BoutiqueQualityResults testBoutiqueFeatures(EngineBase* engine);
    EngineSpecificResults testEngineSpecific(EngineBase* engine, int engineType);
    
    // Performance benchmarking
    PerformanceMetrics benchmarkPerformance(EngineBase* engine);
    
    // Test configuration
    void setSampleRate(double sampleRate) { m_sampleRate = sampleRate; }
    void setBlockSize(int blockSize) { m_blockSize = blockSize; }
    void setTestDuration(float seconds) { m_testDurationSeconds = seconds; }
    void setVerbose(bool verbose) { m_verbose = verbose; }
    
private:
    // Audio quality test methods
    float measureDCOffset(const float* buffer, int numSamples);
    float measurePeakLevel(const float* buffer, int numSamples);
    float measureRMS(const float* buffer, int numSamples);
    float measureTHD(const float* buffer, int numSamples, double sampleRate);
    float measureNoiseFloor(const float* buffer, int numSamples);
    bool detectZipperNoise(EngineBase* engine);
    
    // DSP quality test methods
    bool testFrequencyResponse(EngineBase* engine, std::vector<float>& response);
    bool testImpulseResponse(EngineBase* engine, std::vector<float>& response);
    bool detectAliasing(EngineBase* engine);
    float measureLatency(EngineBase* engine);
    bool testFilterStability(EngineBase* engine);
    
    // Boutique quality test methods
    bool verifyThermalModeling(EngineBase* engine);
    bool verifyComponentAging(EngineBase* engine);
    bool verifyParameterSmoothing(EngineBase* engine);
    bool verifyDCBlocking(EngineBase* engine);
    float measureAnalogNoise(EngineBase* engine);
    
    // Engine-specific test methods
    bool testDelayAccuracy(EngineBase* engine);
    bool testReverbDecay(EngineBase* engine);
    bool testFilterResonance(EngineBase* engine);
    bool testCompressorBehavior(EngineBase* engine);
    bool testDistortionHarmonics(EngineBase* engine);
    
    // Test signal generation
    void generateSineWave(float* buffer, int numSamples, float frequency, float amplitude);
    void generateWhiteNoise(float* buffer, int numSamples, float amplitude);
    void generateImpulse(float* buffer, int numSamples, float amplitude);
    void generateSweep(float* buffer, int numSamples, float startFreq, float endFreq);
    void generateTestSignal(float* buffer, int numSamples, TestSignalType type);
    
    // Analysis utilities
    void performFFT(const float* input, float* magnitudes, int fftSize);
    float findPeakFrequency(const float* magnitudes, int fftSize, double sampleRate);
    float calculateSNR(const float* signal, const float* noise, int numSamples);
    float calculateCorrelation(const float* buffer1, const float* buffer2, int numSamples);
    
    // Memory and performance testing
    bool detectMemoryLeaks(EngineBase* engine);
    float measureCPUUsage(EngineBase* engine, int numIterations);
    
    // Test configuration
    double m_sampleRate = 48000.0;
    int m_blockSize = 512;
    float m_testDurationSeconds = 1.0f;
    bool m_verbose = false;
    
    // Test buffers
    juce::AudioBuffer<float> m_inputBuffer;
    juce::AudioBuffer<float> m_outputBuffer;
    juce::AudioBuffer<float> m_referenceBuffer;
    
    // FFT for analysis
    std::unique_ptr<juce::dsp::FFT> m_fft;
    std::vector<float> m_fftData;
    
    // Test signal types
    enum class TestSignalType {
        Sine440Hz,
        Sine1kHz,
        WhiteNoise,
        PinkNoise,
        Impulse,
        SweepLinear,
        SweepLog,
        Silence,
        SquareWave,
        ComplexHarmonic
    };
    
    // Quality thresholds
    struct QualityThresholds {
        float maxDCOffset = 0.001f;        // -60dB
        float maxTHD = 0.01f;              // 1% THD
        float minSNR = 60.0f;              // 60dB SNR
        float maxLatencySamples = 512.0f;  // ~10ms at 48kHz
        float maxCPUUsage = 10.0f;         // 10% CPU per instance
        float parameterSmoothingMs = 50.0f; // Maximum smoothing time
    } m_thresholds;
    
    // Test result aggregation
    void aggregateResults(TestResults& results);
    std::string generateReport(const TestResults& results);
};

/**
 * Automated test runner for continuous integration
 */
class QualityTestRunner {
public:
    QualityTestRunner();
    
    // Run tests on all engines
    void runAllEngineTests();
    void runEngineTest(int engineType);
    void runTestSuite(const std::string& suiteName);
    
    // Report generation
    void generateHTMLReport(const std::string& filename);
    void generateJSONReport(const std::string& filename);
    void printSummary();
    
    // CI/CD integration
    int getExitCode() const { return m_failedTests > 0 ? 1 : 0; }
    
private:
    EngineQualityTest m_tester;
    std::vector<TestResults> m_allResults;
    int m_totalTests = 0;
    int m_passedTests = 0;
    int m_failedTests = 0;
    int m_warningTests = 0;
};