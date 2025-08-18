#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include <cmath>

#include "EngineBase.h"
#include "EngineFactory.h"
#include "EngineTypes.h"
#include "TestSignalGenerator.h"
#include "AudioMeasurements.h"

/**
 * Comprehensive Test Harness for Chimera Plugin
 * 
 * Tests all 57 engines (0-56) systematically with:
 * - Parameter sweep testing (all parameters 0-1)
 * - Safety checks (NaN/Inf, buffer overruns, thread safety)
 * - Audio quality tests (sine waves, white noise, transients)
 * - Performance metrics (CPU usage, latency)
 * - Mix parameter linearity test
 * - Rapid parameter change stability
 * - Bypass stability test
 * 
 * Generates detailed reports with specific recommendations for fixes.
 */

namespace ChimeraTestHarness {

    // Test result severity levels
    enum class Severity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    // Individual test result
    struct TestResult {
        std::string testName;
        bool passed = false;
        Severity severity = Severity::INFO;
        std::string message;
        std::string details;
        std::vector<std::string> recommendations;
        float score = 0.0f; // 0-100
        std::map<std::string, float> metrics;
        
        TestResult(const std::string& name) : testName(name) {}
        
        void setPass(const std::string& msg = "Test passed") {
            passed = true;
            severity = Severity::INFO;
            message = msg;
            score = 100.0f;
        }
        
        void setFail(Severity sev, const std::string& msg, const std::vector<std::string>& recs = {}) {
            passed = false;
            severity = sev;
            message = msg;
            recommendations = recs;
            score = (sev == Severity::CRITICAL) ? 0.0f : 
                   (sev == Severity::ERROR) ? 25.0f : 50.0f;
        }
    };

    // Category of test results
    struct TestCategory {
        std::string name;
        std::vector<TestResult> results;
        float overallScore = 0.0f;
        bool allPassed = false;
        
        TestCategory(const std::string& categoryName) : name(categoryName) {}
        
        void calculateScore() {
            if (results.empty()) {
                overallScore = 0.0f;
                allPassed = false;
                return;
            }
            
            float totalScore = 0.0f;
            allPassed = true;
            
            for (const auto& result : results) {
                totalScore += result.score;
                if (!result.passed) {
                    allPassed = false;
                }
            }
            
            overallScore = totalScore / results.size();
        }
        
        void addResult(const TestResult& result) {
            results.push_back(result);
            calculateScore();
        }
    };

    // Complete engine test results
    struct EngineTestResults {
        int engineID = -1;
        std::string engineName;
        bool engineCreated = false;
        
        // Test categories
        TestCategory parameterSweepTests{"Parameter Sweep Tests"};
        TestCategory safetyTests{"Safety Tests"};
        TestCategory audioQualityTests{"Audio Quality Tests"};
        TestCategory performanceTests{"Performance Tests"};
        TestCategory stabilityTests{"Stability Tests"};
        
        // Overall metrics
        float overallScore = 0.0f;
        bool allTestsPassed = false;
        std::chrono::milliseconds totalTestTime{0};
        
        // Performance metrics
        float avgCpuUsage = 0.0f;
        float maxCpuUsage = 0.0f;
        float avgLatencyMs = 0.0f;
        float maxLatencyMs = 0.0f;
        
        // Critical issues count
        int criticalIssues = 0;
        int errorIssues = 0;
        int warningIssues = 0;
        
        void calculateOverallScore() {
            std::vector<TestCategory*> categories = {
                &parameterSweepTests, &safetyTests, &audioQualityTests,
                &performanceTests, &stabilityTests
            };
            
            float totalScore = 0.0f;
            allTestsPassed = true;
            criticalIssues = errorIssues = warningIssues = 0;
            
            for (auto* category : categories) {
                category->calculateScore();
                totalScore += category->overallScore;
                if (!category->allPassed) {
                    allTestsPassed = false;
                }
                
                // Count issues
                for (const auto& result : category->results) {
                    switch (result.severity) {
                        case Severity::CRITICAL: criticalIssues++; break;
                        case Severity::ERROR: errorIssues++; break;
                        case Severity::WARNING: warningIssues++; break;
                        default: break;
                    }
                }
            }
            
            overallScore = categories.empty() ? 0.0f : totalScore / categories.size();
        }
        
        std::vector<std::string> getPrioritizedRecommendations() const {
            std::vector<std::string> recommendations;
            
            // Critical issues first
            auto addRecommendations = [&](Severity severity) {
                for (const auto* category : {&parameterSweepTests, &safetyTests, &audioQualityTests, &performanceTests, &stabilityTests}) {
                    for (const auto& result : category->results) {
                        if (result.severity == severity && !result.recommendations.empty()) {
                            for (const auto& rec : result.recommendations) {
                                recommendations.push_back("[" + category->name + "] " + rec);
                            }
                        }
                    }
                }
            };
            
            addRecommendations(Severity::CRITICAL);
            addRecommendations(Severity::ERROR);
            addRecommendations(Severity::WARNING);
            
            return recommendations;
        }
    };

    // Overall test suite results
    struct TestSuiteResults {
        std::vector<EngineTestResults> engineResults;
        std::chrono::milliseconds totalExecutionTime{0};
        
        // Summary statistics
        int totalEngines = 0;
        int workingEngines = 0;
        int failedEngines = 0;
        int enginesWithCriticalIssues = 0;
        int enginesWithErrors = 0;
        int enginesWithWarnings = 0;
        
        float averageScore = 0.0f;
        float averageCpuUsage = 0.0f;
        float worstCpuUsage = 0.0f;
        
        void calculateSummary() {
            totalEngines = engineResults.size();
            workingEngines = failedEngines = 0;
            enginesWithCriticalIssues = enginesWithErrors = enginesWithWarnings = 0;
            
            float totalScore = 0.0f;
            float totalCpu = 0.0f;
            worstCpuUsage = 0.0f;
            
            for (const auto& result : engineResults) {
                if (!result.engineCreated) {
                    failedEngines++;
                    continue;
                }
                
                workingEngines++;
                totalScore += result.overallScore;
                totalCpu += result.avgCpuUsage;
                worstCpuUsage = std::max(worstCpuUsage, result.maxCpuUsage);
                
                if (result.criticalIssues > 0) enginesWithCriticalIssues++;
                else if (result.errorIssues > 0) enginesWithErrors++;
                else if (result.warningIssues > 0) enginesWithWarnings++;
            }
            
            averageScore = workingEngines > 0 ? totalScore / workingEngines : 0.0f;
            averageCpuUsage = workingEngines > 0 ? totalCpu / workingEngines : 0.0f;
        }
        
        std::vector<EngineTestResults> getProblematicEngines() const {
            std::vector<EngineTestResults> problematic;
            
            for (const auto& result : engineResults) {
                if (!result.engineCreated || result.criticalIssues > 0 || 
                    result.errorIssues > 0 || result.overallScore < 70.0f) {
                    problematic.push_back(result);
                }
            }
            
            // Sort by severity (worst first)
            std::sort(problematic.begin(), problematic.end(), [](const auto& a, const auto& b) {
                if (a.criticalIssues != b.criticalIssues) return a.criticalIssues > b.criticalIssues;
                if (a.errorIssues != b.errorIssues) return a.errorIssues > b.errorIssues;
                return a.overallScore < b.overallScore;
            });
            
            return problematic;
        }
    };

    // Signal generator for comprehensive testing
    class ComprehensiveSignalGenerator {
    public:
        enum class SignalType {
            DC_OFFSET,
            SINE_WAVE,
            WHITE_NOISE,
            PINK_NOISE,
            IMPULSE,
            STEP,
            CHIRP_SWEEP,
            MULTITONE,
            DRUM_TRANSIENT,
            GUITAR_CHORD,
            VOCAL_FORMANTS,
            EXTREME_LEVELS,
            SILENCE,
            CUSTOM
        };
        
        static juce::AudioBuffer<float> generateSignal(SignalType type, 
                                                      double sampleRate, 
                                                      float durationSeconds,
                                                      float amplitude = 0.5f,
                                                      const std::map<std::string, float>& params = {});
        
        static std::vector<float> generateParameterSweep(int numSteps, float min = 0.0f, float max = 1.0f);
        
        static bool containsNanOrInf(const juce::AudioBuffer<float>& buffer);
        static float calculateRMS(const juce::AudioBuffer<float>& buffer);
        static float calculatePeak(const juce::AudioBuffer<float>& buffer);
        static float calculateCrestFactor(const juce::AudioBuffer<float>& buffer);
        static float calculateTHD(const juce::AudioBuffer<float>& buffer, double sampleRate, float fundamentalFreq);
        static float calculateStereoCorrelation(const juce::AudioBuffer<float>& buffer);
    };

    // Performance measurement utilities
    class PerformanceMeasurer {
    public:
        struct Measurement {
            std::chrono::nanoseconds processingTime{0};
            float cpuPercentage = 0.0f;
            size_t memoryUsage = 0;
            bool realTimeCapable = true;
        };
        
        static Measurement measureProcessingTime(std::function<void()> processingFunction,
                                                double sampleRate, int blockSize);
        
        static float calculateCpuPercentage(std::chrono::nanoseconds processingTime, 
                                           int blockSize, double sampleRate);
        
        static bool isRealTimeCapable(std::chrono::nanoseconds processingTime,
                                     int blockSize, double sampleRate, 
                                     float safetyMargin = 0.8f);
    };

    // Main test harness class
    class ComprehensiveTestHarness {
    public:
        ComprehensiveTestHarness();
        ~ComprehensiveTestHarness() = default;
        
        // Configuration
        void setSampleRate(double rate) { m_sampleRate = rate; }
        void setBlockSize(int size) { m_blockSize = size; }
        void setTestDuration(float seconds) { m_testDuration = seconds; }
        void setNumParameterSweepSteps(int steps) { m_parameterSweepSteps = steps; }
        void setVerboseOutput(bool verbose) { m_verbose = verbose; }
        void setParallelTesting(bool parallel) { m_parallelTesting = parallel; }
        void setMaxConcurrentTests(int max) { m_maxConcurrentTests = max; }
        
        // Main testing functions
        TestSuiteResults testAllEngines();
        EngineTestResults testSingleEngine(int engineID);
        
        // Individual test categories
        TestCategory runParameterSweepTests(EngineBase* engine, int engineID);
        TestCategory runSafetyTests(EngineBase* engine, int engineID);
        TestCategory runAudioQualityTests(EngineBase* engine, int engineID);
        TestCategory runPerformanceTests(EngineBase* engine, int engineID);
        TestCategory runStabilityTests(EngineBase* engine, int engineID);
        
        // Specific test implementations
        TestResult testParameterSweep(EngineBase* engine, int paramIndex);
        TestResult testNanInfSafety(EngineBase* engine);
        TestResult testBufferOverrunSafety(EngineBase* engine);
        TestResult testThreadSafety(EngineBase* engine);
        TestResult testSineWaveResponse(EngineBase* engine, float frequency = 440.0f);
        TestResult testNoiseResponse(EngineBase* engine);
        TestResult testTransientResponse(EngineBase* engine);
        TestResult testCpuUsage(EngineBase* engine);
        TestResult testLatency(EngineBase* engine);
        TestResult testMixParameterLinearity(EngineBase* engine);
        TestResult testRapidParameterChanges(EngineBase* engine);
        TestResult testBypassStability(EngineBase* engine);
        
        // Report generation
        void generateSummaryReport(const TestSuiteResults& results, const std::string& filename);
        void generateDetailedReport(const TestSuiteResults& results, const std::string& filename);
        void generateHTMLReport(const TestSuiteResults& results, const std::string& filename);
        void generateJSONReport(const TestSuiteResults& results, const std::string& filename);
        void generateCSVReport(const TestSuiteResults& results, const std::string& filename);
        
        // Console output
        void printProgressUpdate(int engineID, const std::string& engineName, 
                               const std::string& currentTest);
        void printSummaryToConsole(const TestSuiteResults& results);
        
    private:
        // Configuration
        double m_sampleRate = 48000.0;
        int m_blockSize = 512;
        float m_testDuration = 2.0f;
        int m_parameterSweepSteps = 20;
        bool m_verbose = false;
        bool m_parallelTesting = true;
        int m_maxConcurrentTests = std::thread::hardware_concurrency();
        
        // Internal state
        std::atomic<int> m_currentEngineIndex{0};
        std::atomic<bool> m_shouldStop{false};
        
        // Helper functions
        bool prepareEngine(EngineBase* engine);
        void resetEngine(EngineBase* engine);
        float calculateTestScore(const std::vector<TestResult>& results);
        std::string generateRecommendation(const TestResult& result, int engineID);
        
        // Thread-safe logging
        void logMessage(const std::string& message);
        mutable std::mutex m_logMutex;
        
        // Test signal cache
        std::map<std::string, juce::AudioBuffer<float>> m_signalCache;
        void cacheCommonSignals();
    };

    // Utility functions for report generation
    namespace ReportUtils {
        std::string formatDuration(std::chrono::milliseconds duration);
        std::string formatScore(float score);
        std::string formatPercentage(float percentage);
        std::string severityToString(Severity severity);
        std::string generateHTMLTable(const std::vector<std::vector<std::string>>& data);
        std::string escapeHTML(const std::string& text);
        std::string generateProgressBar(float percentage, int width = 50);
    }

} // namespace ChimeraTestHarness