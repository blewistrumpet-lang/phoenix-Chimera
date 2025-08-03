#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include "EngineBase.h"
#include "EngineTypes.h"
#include "EngineTestAgents.h"
#include "ParameterSweepTest.h"
#include "TestSignalGenerator.h"
#include "AudioMeasurements.h"

/**
 * Comprehensive Engine Validation System
 * 
 * Master validation system that systematically tests all 56 engines using
 * specialized test agents and parameter sweep testing. Generates detailed
 * HTML reports with graphs, measurements, and visual proof that each engine
 * is processing audio correctly.
 */

namespace ComprehensiveEngineValidator {

    // Validation levels
    enum class ValidationLevel {
        BASIC,      // Basic function tests only
        STANDARD,   // Function tests + parameter sweeps
        COMPREHENSIVE, // Everything + detailed analysis
        STRESS_TEST    // All tests + stress testing
    };

    // Test categories
    enum class TestCategory {
        FUNCTIONALITY, // Basic function and stability
        PARAMETERS,    // Parameter effectiveness
        AUDIO_QUALITY, // Audio quality metrics
        PERFORMANCE,   // Performance and efficiency
        COMPATIBILITY  // Compatibility and edge cases
    };

    // Individual test result
    struct ValidationTest {
        std::string testName;
        std::string category;
        std::string description;
        bool passed = false;
        float score = 0.0f; // 0-100
        std::string details;
        std::string failureReason;
        float executionTimeMs = 0.0f;
        
        // Additional data for reporting
        std::map<std::string, float> measurements;
        std::vector<float> plotData;
        std::string plotType; // "line", "bar", "spectrogram", etc.
    };

    // Engine validation result
    struct EngineValidationResult {
        std::string engineName;
        int engineType = -1;
        std::string engineCategory;
        
        // Test results by category
        std::vector<ValidationTest> functionalityTests;
        std::vector<ValidationTest> parameterTests;
        std::vector<ValidationTest> audioQualityTests;
        std::vector<ValidationTest> performanceTests;
        std::vector<ValidationTest> compatibilityTests;
        
        // Overall metrics
        bool overallPassed = false;
        float overallScore = 0.0f; // 0-100
        int totalTests = 0;
        int passedTests = 0;
        float totalExecutionTimeMs = 0.0f;
        
        // Specialized test results
        EngineTestAgents::EngineTestSuite agentTestResults;
        ParameterSweepTest::EngineSweepResults sweepResults;
        
        // Analysis and recommendations
        std::string qualityAssessment;
        std::vector<std::string> issues;
        std::vector<std::string> recommendations;
        std::string technicalSummary;
        
        void calculateOverallMetrics();
        std::string generateSummaryReport() const;
    };

    // Batch validation results for all engines
    struct BatchValidationResults {
        std::vector<EngineValidationResult> engineResults;
        
        // Batch statistics
        int totalEngines = 0;
        int passedEngines = 0;
        float averageScore = 0.0f;
        float totalBatchTimeMs = 0.0f;
        
        // Category statistics
        struct CategoryStats {
            int totalTests = 0;
            int passedTests = 0;
            float averageScore = 0.0f;
            std::vector<std::string> commonIssues;
        };
        
        std::map<std::string, CategoryStats> categoryStats;
        
        // Issue analysis
        std::vector<std::string> criticalIssues;
        std::vector<std::string> globalRecommendations;
        
        void calculateBatchStatistics();
        std::string generateExecutiveSummary() const;
    };

    // Main validator class
    class EngineValidator {
    public:
        EngineValidator();
        ~EngineValidator() = default;
        
        // Configuration
        void setValidationLevel(ValidationLevel level) { m_validationLevel = level; }
        void setSampleRate(double sampleRate) { m_sampleRate = sampleRate; }
        void setOutputDirectory(const std::string& dir) { m_outputDirectory = dir; }
        void setGenerateHTMLReports(bool generate) { m_generateHTML = generate; }
        void setGenerateDetailedPlots(bool generate) { m_generatePlots = generate; }
        void setStressTestDuration(float seconds) { m_stressTestDuration = seconds; }
        
        // Single engine validation
        EngineValidationResult validateEngine(int engineType, ValidationLevel level = ValidationLevel::STANDARD);
        EngineValidationResult validateEngine(std::unique_ptr<EngineBase> engine, int engineType, 
                                            ValidationLevel level = ValidationLevel::STANDARD);
        
        // Batch validation
        BatchValidationResults validateAllEngines(ValidationLevel level = ValidationLevel::STANDARD);
        BatchValidationResults validateEngineList(const std::vector<int>& engineTypes, 
                                                 ValidationLevel level = ValidationLevel::STANDARD);
        
        // Report generation
        std::string generateHTMLReport(const EngineValidationResult& result);
        std::string generateBatchHTMLReport(const BatchValidationResults& results);
        void saveReports(const BatchValidationResults& results);
        
        // Progress callbacks
        void setProgressCallback(std::function<void(const std::string&, float)> callback) { 
            m_progressCallback = callback; 
        }
        
    private:
        ValidationLevel m_validationLevel = ValidationLevel::STANDARD;
        double m_sampleRate = 44100.0;
        std::string m_outputDirectory = "validation_reports";
        bool m_generateHTML = true;
        bool m_generatePlots = true;
        float m_stressTestDuration = 10.0f;
        
        std::function<void(const std::string&, float)> m_progressCallback;
        
        // Test execution methods
        std::vector<ValidationTest> runFunctionalityTests(EngineBase* engine, int engineType);
        std::vector<ValidationTest> runParameterTests(EngineBase* engine, int engineType);
        std::vector<ValidationTest> runAudioQualityTests(EngineBase* engine, int engineType);
        std::vector<ValidationTest> runPerformanceTests(EngineBase* engine, int engineType);
        std::vector<ValidationTest> runCompatibilityTests(EngineBase* engine, int engineType);
        std::vector<ValidationTest> runStressTests(EngineBase* engine, int engineType);
        
        // Specific test implementations
        ValidationTest testBasicFunctionality(EngineBase* engine);
        ValidationTest testParameterRange(EngineBase* engine, int paramIndex);
        ValidationTest testSilenceHandling(EngineBase* engine);
        ValidationTest testLatency(EngineBase* engine);
        ValidationTest testStability(EngineBase* engine);
        ValidationTest testDynamicRange(EngineBase* engine);
        ValidationTest testFrequencyResponse(EngineBase* engine);
        ValidationTest testTHDLevel(EngineBase* engine);
        ValidationTest testNoiseFloor(EngineBase* engine);
        ValidationTest testCPUUsage(EngineBase* engine);
        ValidationTest testMemoryUsage(EngineBase* engine);
        ValidationTest testSampleRateCompatibility(EngineBase* engine);
        ValidationTest testBufferSizeCompatibility(EngineBase* engine);
        ValidationTest testExtremeParameters(EngineBase* engine);
        ValidationTest testLongDurationStability(EngineBase* engine);
        
        // Analysis helpers
        void analyzeEnginePerformance(EngineValidationResult& result);
        void generateRecommendations(EngineValidationResult& result);
        std::string assessAudioQuality(const std::vector<ValidationTest>& tests);
        
        // Report generation helpers
        std::string generateTestTable(const std::vector<ValidationTest>& tests, const std::string& title);
        std::string generatePlotlyChart(const ValidationTest& test);
        std::string generateMetricsSection(const EngineValidationResult& result);
        std::string generateIssuesSection(const EngineValidationResult& result);
        
        // Utility methods
        void reportProgress(const std::string& message, float percentage = -1.0f);
        std::string formatDuration(float milliseconds);
        std::string formatScore(float score);
        bool createOutputDirectory();
    };

    // Specialized test suites for different validation scenarios
    namespace ValidationSuites {
        
        // Quality Assurance Suite
        class QualityAssuranceSuite {
        public:
            static std::vector<ValidationTest> runQATests(EngineBase* engine, int engineType);
            
        private:
            static ValidationTest testSignalIntegrity(EngineBase* engine);
            static ValidationTest testParameterConsistency(EngineBase* engine);
            static ValidationTest testOutputStability(EngineBase* engine);
            static ValidationTest testBoundaryConditions(EngineBase* engine);
        };
        
        // Performance Benchmarking Suite
        class PerformanceSuite {
        public:
            static std::vector<ValidationTest> runPerformanceTests(EngineBase* engine, double sampleRate);
            
        private:
            static ValidationTest benchmarkProcessingSpeed(EngineBase* engine, double sampleRate);
            static ValidationTest measureMemoryFootprint(EngineBase* engine);
            static ValidationTest testRealTimeCapability(EngineBase* engine, double sampleRate);
            static ValidationTest analyzeComputationalComplexity(EngineBase* engine);
        };
        
        // Compatibility Testing Suite
        class CompatibilitySuite {
        public:
            static std::vector<ValidationTest> runCompatibilityTests(EngineBase* engine);
            
        private:
            static ValidationTest testMultipleSampleRates(EngineBase* engine);
            static ValidationTest testVariableBufferSizes(EngineBase* engine);
            static ValidationTest testMonoStereoCompatibility(EngineBase* engine);
            static ValidationTest testParameterAutomation(EngineBase* engine);
        };
    }

    // Report templates and styling
    namespace ReportTemplates {
        
        extern const char* HTML_TEMPLATE_HEADER;
        extern const char* HTML_TEMPLATE_FOOTER;
        extern const char* CSS_STYLES;
        extern const char* JAVASCRIPT_CHARTS;
        
        std::string generateEngineReportHTML(const EngineValidationResult& result);
        std::string generateBatchReportHTML(const BatchValidationResults& results);
        std::string generateExecutiveSummaryHTML(const BatchValidationResults& results);
    }

    // Validation utilities
    namespace ValidationUtils {
        
        // Test result analysis
        bool isTestPassed(const ValidationTest& test, float threshold = 70.0f);
        float calculateCategoryScore(const std::vector<ValidationTest>& tests);
        std::string classifyIssue(const ValidationTest& test);
        
        // Audio analysis utilities
        struct AudioQualityMetrics {
            float dynamicRange = 0.0f;
            float signalToNoise = 0.0f;
            float totalHarmonicDistortion = 0.0f;
            float frequencyResponseFlatness = 0.0f;
            float phaseCoherence = 0.0f;
        };
        
        AudioQualityMetrics analyzeAudioQuality(const juce::AudioBuffer<float>& signal, double sampleRate);
        
        // Performance analysis utilities
        struct PerformanceMetrics {
            float averageCPULoad = 0.0f;
            float peakCPULoad = 0.0f;
            size_t memoryUsage = 0;
            float averageLatency = 0.0f;
            bool realTimeCapable = false;
        };
        
        PerformanceMetrics measurePerformance(EngineBase* engine, double sampleRate, float durationSeconds);
        
        // Statistical utilities
        float calculateConfidenceInterval(const std::vector<float>& measurements, float confidenceLevel = 0.95f);
        bool isStatisticallySignificant(const std::vector<float>& control, const std::vector<float>& test);
    }

} // namespace ComprehensiveEngineValidator