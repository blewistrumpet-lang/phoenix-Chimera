#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include "EngineBase.h"
#include "TestSignalGenerator.h"
#include "AudioMeasurements.h"
#include "EngineTypes.h"

/**
 * Parameter Sweep Testing System
 * 
 * Systematically tests all parameters of an engine by sweeping through their ranges
 * and measuring the resulting changes in audio characteristics. Provides visual
 * proof that parameters are affecting the sound correctly.
 */

namespace ParameterSweepTest {

    // Measurement types for different parameter effects
    enum class MeasurementType {
        RMS_LEVEL,          // Overall level change
        PEAK_LEVEL,         // Peak level change
        FREQUENCY_CONTENT,  // Spectral content change
        HARMONIC_CONTENT,   // Harmonic distortion
        PHASE_RESPONSE,     // Phase relationship changes
        TRANSIENT_RESPONSE, // Transient handling
        NOISE_FLOOR,        // Noise/artifact level
        CORRELATION,        // Stereo correlation
        DELAY_TIME,         // Time-based effects
        MODULATION_DEPTH,   // Modulation amount
        CUSTOM              // Custom measurement function
    };

    // Parameter sweep configuration
    struct SweepConfig {
        int parameterIndex = 0;
        std::string parameterName;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        int numSteps = 10;
        MeasurementType measurementType = MeasurementType::RMS_LEVEL;
        std::function<float(const juce::AudioBuffer<float>&, const juce::AudioBuffer<float>&)> customMeasurement;
        
        // Test signal configuration
        enum TestSignalType {
            SINE_WAVE,
            WHITE_NOISE,
            PINK_NOISE,
            IMPULSE,
            SWEEP,
            CHORD,
            DRUM_HIT,
            TWO_TONE,
            CUSTOM_SIGNAL
        } testSignalType = SINE_WAVE;
        
        float testFrequency = 440.0f;
        float testDuration = 1.0f;
        float testAmplitude = 0.5f;
        juce::AudioBuffer<float> customTestSignal;
    };

    // Single measurement point
    struct MeasurementPoint {
        float parameterValue = 0.0f;
        float measuredValue = 0.0f;
        float normalizedValue = 0.0f; // 0-1 normalized
        bool isValid = true;
        std::string notes;
    };

    // Complete sweep result for one parameter
    struct SweepResult {
        SweepConfig config;
        std::vector<MeasurementPoint> measurements;
        
        // Analysis results
        float totalRange = 0.0f;        // Max - min measurement
        float averageChange = 0.0f;     // Average change per step
        float maxChange = 0.0f;         // Largest single step change
        float monotonicity = 0.0f;      // How monotonic the response is (-1 to 1)
        float sensitivity = 0.0f;       // How sensitive parameter is (0-1)
        bool isEffective = false;       // Whether parameter has significant effect
        
        // Quality metrics
        float smoothness = 0.0f;        // How smooth the response curve is
        float linearity = 0.0f;         // How linear the response is
        float consistency = 0.0f;       // How consistent measurements are
        
        std::string analysisNotes;
        
        void analyzeResults();
        float calculateCorrelation() const;
        std::string generateSummary() const;
    };

    // Complete engine sweep results
    struct EngineSweepResults {
        std::string engineName;
        int engineType = -1;
        std::vector<SweepResult> parameterResults;
        
        // Overall engine metrics
        int effectiveParameterCount = 0;
        float averageSensitivity = 0.0f;
        float overallQuality = 0.0f;
        bool allParametersWorking = false;
        
        std::string qualityReport;
        float testDurationMs = 0.0f;
        
        void calculateOverallMetrics();
        std::string generateReport() const;
    };

    // Main parameter sweep tester
    class ParameterSweeper {
    public:
        ParameterSweeper();
        ~ParameterSweeper() = default;
        
        // Main testing functions
        EngineSweepResults testAllParameters(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0);
        SweepResult testSingleParameter(EngineBase* engine, const SweepConfig& config, double sampleRate = 44100.0);
        
        // Configuration
        void setDefaultSweepSteps(int steps) { m_defaultSteps = steps; }
        void setDefaultTestDuration(float duration) { m_defaultDuration = duration; }
        void setMeasurementTolerance(float tolerance) { m_tolerance = tolerance; }
        void setEnableDetailed Analysis(bool enable) { m_detailedAnalysis = enable; }
        
        // Custom measurement functions
        void registerCustomMeasurement(const std::string& name, 
                                     std::function<float(const juce::AudioBuffer<float>&, const juce::AudioBuffer<float>&)> func);
        
        // Auto-configuration for different engine types
        std::vector<SweepConfig> generateConfigsForEngine(EngineBase* engine, int engineType);
        
    private:
        int m_defaultSteps = 10;
        float m_defaultDuration = 1.0f;
        float m_tolerance = 0.01f; // Minimum change to consider significant
        bool m_detailedAnalysis = true;
        
        std::map<std::string, std::function<float(const juce::AudioBuffer<float>&, const juce::AudioBuffer<float>&)>> m_customMeasurements;
        
        // Internal measurement functions
        float measureParameter(const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed, 
                             MeasurementType type, const SweepConfig& config);
        
        juce::AudioBuffer<float> generateTestSignal(const SweepConfig& config, double sampleRate);
        
        // Analysis helpers
        void analyzeMonotonicity(SweepResult& result);
        void analyzeSmoothness(SweepResult& result);
        void analyzeLinearity(SweepResult& result);
        void analyzeConsistency(SweepResult& result);
        
        // Engine-specific configurations
        std::vector<SweepConfig> getDynamicsConfigs(EngineBase* engine);
        std::vector<SweepConfig> getFilterConfigs(EngineBase* engine);
        std::vector<SweepConfig> getTimeBasedConfigs(EngineBase* engine);
        std::vector<SweepConfig> getModulationConfigs(EngineBase* engine);
        std::vector<SweepConfig> getDistortionConfigs(EngineBase* engine);
        std::vector<SweepConfig> getSpectralConfigs(EngineBase* engine);
    };

    // Utility functions for parameter analysis
    namespace ParameterAnalysis {
        
        // Statistical analysis functions
        float calculateMean(const std::vector<float>& values);
        float calculateStandardDeviation(const std::vector<float>& values);
        float calculateCorrelationCoefficient(const std::vector<float>& x, const std::vector<float>& y);
        float calculateMonotonicity(const std::vector<float>& values);
        
        // Curve fitting and analysis
        struct LinearFit {
            float slope = 0.0f;
            float intercept = 0.0f;
            float rSquared = 0.0f;
            
            float predict(float x) const { return slope * x + intercept; }
        };
        
        LinearFit fitLinear(const std::vector<float>& x, const std::vector<float>& y);
        float calculateCurvature(const std::vector<float>& values);
        float calculateSmoothness(const std::vector<float>& values);
        
        // Quality assessment
        bool isParameterEffective(const SweepResult& result, float threshold = 0.05f);
        float assessParameterQuality(const SweepResult& result);
        std::string classifyParameterBehavior(const SweepResult& result);
        
        // Visualization helpers
        std::vector<std::pair<float, float>> normalizeForPlotting(const SweepResult& result);
        std::string generateDataForPlot(const SweepResult& result, const std::string& format = "json");
    }

    // Visual proof generation
    class VisualProofGenerator {
    public:
        VisualProofGenerator() = default;
        
        // Generate visual proof of parameter effectiveness
        struct VisualProof {
            std::string parameterName;
            std::string plotData;           // JSON/CSV data for plotting
            std::string spectrogramData;    // Spectrogram data if applicable
            std::vector<float> waveformMin; // Min values for waveform display
            std::vector<float> waveformMax; // Max values for waveform display
            std::string analysisText;       // Human-readable analysis
            float effectivenessScore = 0.0f;
        };
        
        VisualProof generateProofForParameter(const SweepResult& result, EngineBase* engine, double sampleRate);
        std::string generateHTMLReport(const EngineSweepResults& results);
        std::string generatePlotlyData(const SweepResult& result);
        
    private:
        std::string generateWaveformData(const std::vector<juce::AudioBuffer<float>>& buffers);
        std::string generateSpectrogramData(const std::vector<juce::AudioBuffer<float>>& buffers, double sampleRate);
        std::string formatAnalysisText(const SweepResult& result);
    };

    // Batch testing for multiple engines
    class BatchParameterTester {
    public:
        BatchParameterTester() = default;
        
        struct BatchConfig {
            std::vector<int> engineTypes;
            std::vector<SweepConfig> customConfigs;
            std::string outputDirectory;
            bool generateVisualProofs = true;
            bool generateHTMLReport = true;
            bool saveRawData = false;
            double sampleRate = 44100.0;
        };
        
        struct BatchResults {
            std::vector<EngineSweepResults> engineResults;
            std::string batchSummary;
            std::string htmlReport;
            float totalTestTime = 0.0f;
            
            void generateBatchSummary();
        };
        
        BatchResults runBatchTest(const BatchConfig& config);
        void saveResults(const BatchResults& results, const std::string& directory);
        
    private:
        ParameterSweeper m_sweeper;
        VisualProofGenerator m_proofGenerator;
    };

} // namespace ParameterSweepTest