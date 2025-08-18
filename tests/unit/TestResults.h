#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>

/**
 * Data structures for storing comprehensive test results
 */

// Individual test result
struct TestResult {
    std::string testName;
    bool passed = false;
    float value = 0.0f;
    float threshold = 0.0f;
    std::string message;
    std::chrono::milliseconds duration;
    
    enum class Severity {
        Pass,
        Warning,
        Fail,
        Critical
    } severity = Severity::Pass;
};

// Audio quality test results
struct AudioQualityResults {
    TestResult dcOffset;
    TestResult peakLevel;
    TestResult rmsLevel;
    TestResult thd;
    TestResult noiseFloor;
    TestResult zipperNoise;
    TestResult gainStaging;
    TestResult stereoImaging;
    
    bool allPassed() const {
        return dcOffset.passed && peakLevel.passed && rmsLevel.passed &&
               thd.passed && noiseFloor.passed && zipperNoise.passed &&
               gainStaging.passed && stereoImaging.passed;
    }
    
    std::vector<TestResult> getAllTests() const {
        return {dcOffset, peakLevel, rmsLevel, thd, noiseFloor, 
                zipperNoise, gainStaging, stereoImaging};
    }
};

// Functional test results
struct FunctionalTestResults {
    TestResult parameterResponse;
    TestResult parameterRanges;
    TestResult extremeParameters;
    TestResult stereoHandling;
    TestResult bypassBehavior;
    TestResult memoryLeaks;
    TestResult threadSafety;
    TestResult stateRecall;
    
    bool allPassed() const {
        return parameterResponse.passed && parameterRanges.passed &&
               extremeParameters.passed && stereoHandling.passed &&
               bypassBehavior.passed && memoryLeaks.passed &&
               threadSafety.passed && stateRecall.passed;
    }
    
    std::vector<TestResult> getAllTests() const {
        return {parameterResponse, parameterRanges, extremeParameters,
                stereoHandling, bypassBehavior, memoryLeaks,
                threadSafety, stateRecall};
    }
};

// DSP quality test results
struct DSPQualityResults {
    TestResult frequencyResponse;
    TestResult impulseResponse;
    TestResult aliasingDetection;
    TestResult latencyMeasurement;
    TestResult filterStability;
    TestResult phaseCoherence;
    TestResult oversamplingQuality;
    TestResult interpolationQuality;
    
    bool allPassed() const {
        return frequencyResponse.passed && impulseResponse.passed &&
               aliasingDetection.passed && latencyMeasurement.passed &&
               filterStability.passed && phaseCoherence.passed &&
               oversamplingQuality.passed && interpolationQuality.passed;
    }
    
    std::vector<TestResult> getAllTests() const {
        return {frequencyResponse, impulseResponse, aliasingDetection,
                latencyMeasurement, filterStability, phaseCoherence,
                oversamplingQuality, interpolationQuality};
    }
};

// Boutique quality test results
struct BoutiqueQualityResults {
    TestResult thermalModeling;
    TestResult componentAging;
    TestResult parameterSmoothing;
    TestResult dcBlocking;
    TestResult analogNoise;
    TestResult componentTolerance;
    TestResult vintageCharacter;
    TestResult warmthAndColor;
    
    bool allPassed() const {
        return thermalModeling.passed && componentAging.passed &&
               parameterSmoothing.passed && dcBlocking.passed &&
               analogNoise.passed && componentTolerance.passed &&
               vintageCharacter.passed && warmthAndColor.passed;
    }
    
    std::vector<TestResult> getAllTests() const {
        return {thermalModeling, componentAging, parameterSmoothing,
                dcBlocking, analogNoise, componentTolerance,
                vintageCharacter, warmthAndColor};
    }
};

// Engine-specific test results
struct EngineSpecificResults {
    std::vector<TestResult> specificTests;
    
    // Engine type specific tests
    TestResult delayTiming;      // For delay engines
    TestResult reverbDecay;      // For reverb engines
    TestResult filterResonance;  // For filter engines
    TestResult compressionRatio; // For dynamics engines
    TestResult harmonicContent;  // For distortion engines
    TestResult modulationDepth;  // For modulation engines
    
    bool allPassed() const {
        for (const auto& test : specificTests) {
            if (!test.passed) return false;
        }
        return true;
    }
    
    std::vector<TestResult> getAllTests() const {
        return specificTests;
    }
};

// Performance metrics
struct PerformanceMetrics {
    float cpuUsagePercent = 0.0f;
    float memoryUsageMB = 0.0f;
    float processingLatencySamples = 0.0f;
    float processingLatencyMs = 0.0f;
    int maximumPolyphony = 0;
    float efficiencyScore = 0.0f; // 0-100 score
    
    TestResult cpuTest;
    TestResult memoryTest;
    TestResult latencyTest;
    TestResult efficiencyTest;
    
    bool meetsRequirements() const {
        return cpuTest.passed && memoryTest.passed && 
               latencyTest.passed && efficiencyTest.passed;
    }
};

// Complete test results for an engine
struct TestResults {
    // Engine identification
    int engineType = -1;
    std::string engineName;
    std::string version;
    std::chrono::system_clock::time_point testTimestamp;
    
    // Test categories
    AudioQualityResults audioQuality;
    FunctionalTestResults functionality;
    DSPQualityResults dspQuality;
    BoutiqueQualityResults boutiqueQuality;
    EngineSpecificResults engineSpecific;
    PerformanceMetrics performance;
    
    // Overall results
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    int warningTests = 0;
    float overallScore = 0.0f; // 0-100
    
    bool passed = false;
    std::string summary;
    std::vector<std::string> recommendations;
    
    // Detailed test log
    std::vector<std::string> testLog;
    
    // Calculate overall results
    void calculateOverallResults() {
        totalTests = 0;
        passedTests = 0;
        failedTests = 0;
        warningTests = 0;
        
        // Count all test results
        auto countTests = [&](const std::vector<TestResult>& tests) {
            for (const auto& test : tests) {
                totalTests++;
                if (test.passed) passedTests++;
                else if (test.severity == TestResult::Severity::Warning) warningTests++;
                else failedTests++;
            }
        };
        
        countTests(audioQuality.getAllTests());
        countTests(functionality.getAllTests());
        countTests(dspQuality.getAllTests());
        countTests(boutiqueQuality.getAllTests());
        countTests(engineSpecific.getAllTests());
        
        // Calculate score
        if (totalTests > 0) {
            overallScore = (float(passedTests) / float(totalTests)) * 100.0f;
        }
        
        // Determine pass/fail
        passed = (failedTests == 0) && (overallScore >= 90.0f);
    }
    
    // Generate summary
    std::string generateSummary() const {
        std::string result = engineName + " Test Results:\n";
        result += "Total Tests: " + std::to_string(totalTests) + "\n";
        result += "Passed: " + std::to_string(passedTests) + "\n";
        result += "Failed: " + std::to_string(failedTests) + "\n";
        result += "Warnings: " + std::to_string(warningTests) + "\n";
        result += "Overall Score: " + std::to_string(int(overallScore)) + "%\n";
        result += "Status: " + std::string(passed ? "PASSED" : "FAILED") + "\n";
        return result;
    }
};

// Test suite results (collection of engine tests)
struct TestSuiteResults {
    std::string suiteName;
    std::vector<TestResults> engineResults;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    
    int totalEnginesTested = 0;
    int passedEngines = 0;
    int failedEngines = 0;
    
    void calculateSummary() {
        totalEnginesTested = engineResults.size();
        passedEngines = 0;
        failedEngines = 0;
        
        for (const auto& result : engineResults) {
            if (result.passed) passedEngines++;
            else failedEngines++;
        }
    }
    
    float getPassRate() const {
        if (totalEnginesTested == 0) return 0.0f;
        return (float(passedEngines) / float(totalEnginesTested)) * 100.0f;
    }
};