#pragma once
#include "EngineFactory.h"
#include "EngineBase.h"
#include <vector>
#include <chrono>

/**
 * Engine Test Runner
 * Runs tests on all engines and generates a report
 */
class EngineTestRunner {
public:
    struct TestResult {
        juce::String engineName;
        int engineID;
        bool silenceTest;
        bool unityGainTest;
        bool stabilityTest;
        float cpuUsage;
        float peakOutput;
        float rmsOutput;
        juce::String notes;
        
        bool passed() const {
            return silenceTest && unityGainTest && stabilityTest;
        }
    };
    
    struct TestSummary {
        std::vector<TestResult> results;
        int totalEngines = 0;
        int passedEngines = 0;
        int failedEngines = 0;
        float averageCPU = 0.0f;
        
        float getPassRate() const {
            return totalEngines > 0 ? (passedEngines * 100.0f / totalEngines) : 0.0f;
        }
    };
    
    // Run all tests
    static TestSummary runAllTests();
    
    // Test individual engine
    static TestResult testEngine(int engineID);
    
    // Generate reports
    static void generateHTMLReport(const TestSummary& summary, const juce::File& outputFile);
    static void printConsoleReport(const TestSummary& summary);
    
private:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr int BLOCK_SIZE = 512;
    
    // Test functions
    static bool testSilence(EngineBase* engine);
    static bool testUnityGain(EngineBase* engine);
    static bool testStability(EngineBase* engine);
    static float measureCPU(EngineBase* engine);
    
    // Helper to generate test signal
    static juce::AudioBuffer<float> generateTestSignal(int type, float duration);
};