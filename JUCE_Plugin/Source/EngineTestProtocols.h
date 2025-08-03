#pragma once
#include <JuceHeader.h>
#include "EngineBase.h"
#include "TestSignalGenerator.h"
#include "AudioMeasurements.h"
#include <memory>
#include <vector>

/**
 * Engine Test Protocols
 * Specific test procedures for different engine categories
 */
class EngineTestProtocols {
public:
    // Test result structure
    struct TestResult {
        juce::String testName;
        bool passed;
        float measuredValue;
        float expectedMin;
        float expectedMax;
        juce::String notes;
    };
    
    struct EngineTestReport {
        juce::String engineName;
        int engineID;
        std::vector<TestResult> results;
        float cpuUsage;
        float latency;
        bool overallPass;
        
        void addResult(const juce::String& test, bool pass, float value, 
                      float min = 0, float max = 0, const juce::String& note = "") {
            results.push_back({test, pass, value, min, max, note});
            if (!pass) overallPass = false;
        }
    };
    
    // Main test functions for different engine categories
    static EngineTestReport testDynamicsEngine(EngineBase* engine, int engineID);
    static EngineTestReport testFilterEngine(EngineBase* engine, int engineID);
    static EngineTestReport testTimeBasedEngine(EngineBase* engine, int engineID);
    static EngineTestReport testModulationEngine(EngineBase* engine, int engineID);
    static EngineTestReport testDistortionEngine(EngineBase* engine, int engineID);
    
    // Generic tests applicable to all engines
    static EngineTestReport runBasicTests(EngineBase* engine, int engineID);
    
    // Comprehensive test for any engine
    static EngineTestReport runComprehensiveTest(EngineBase* engine, int engineID);
    
private:
    // Helper functions
    static juce::AudioBuffer<float> processEngine(EngineBase* engine, 
                                                  const juce::AudioBuffer<float>& input);
    
    static bool testSilenceInSilenceOut(EngineBase* engine);
    static bool testUnityGain(EngineBase* engine);
    static bool testFrequencyResponse(EngineBase* engine);
    static bool testDynamicRange(EngineBase* engine);
    static float measureCPUUsage(EngineBase* engine);
    
    // Engine category detection
    enum class EngineCategory {
        Dynamics,
        Filter,
        TimeBased,
        Modulation,
        Distortion,
        Other
    };
    
    static EngineCategory detectEngineCategory(int engineID);
    
    // Test parameters
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr int BLOCK_SIZE = 512;
    static constexpr float TEST_DURATION = 1.0f; // seconds
    static constexpr float MAX_CPU_PERCENT = 5.0f;
    static constexpr float SILENCE_THRESHOLD = -80.0f; // dB
};