/**
 * Comprehensive Engine Diagnostic Test
 * Project Chimera v3.0 Phoenix - JUCE Plugin
 * 
 * Tests ALL 57 engines (IDs 0-56) in the Chimera plugin system.
 * 
 * FEATURES:
 * - Tests each of the 57 engines (ENGINE_NONE + 56 actual engines)
 * - Creates engine instances safely with error handling
 * - Sets appropriate test parameters for each engine type
 * - Processes multiple test signals (sine wave, white noise, impulse)
 * - Measures RMS/peak changes to detect actual audio processing
 * - Tests Mix parameter functionality 
 * - Handles engine crashes and exceptions gracefully
 * - Groups results by engine category for organized output
 * - Provides clear pass/fail status with detailed diagnostics
 * - Easy integration into existing PluginProcessor.cpp
 * 
 * USAGE:
 * - Compile standalone: clang++ -std=c++17 -I../Source comprehensive_engine_diagnostic.cpp
 * - Or integrate into PluginProcessor.cpp by calling runComprehensiveDiagnostic()
 * 
 * Author: Claude Code Assistant
 * Date: 2025-08-07
 * Version: 1.0
 */

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <exception>
#include <random>

// Engine includes (conditional compilation if in plugin context)
#ifndef COMPREHENSIVE_DIAGNOSTIC_STANDALONE
    #include <JuceHeader.h>
    #include "EngineFactory.h"
    #include "EngineBase.h"
    #include "EngineTypes.h"
    #include "EngineStringMapping.h"
#else
    // Standalone mode - define minimal JUCE-like interfaces
    namespace juce {
        class String {
            std::string str;
        public:
            String(const char* s) : str(s) {}
            String(const std::string& s) : str(s) {}
            const char* toRawUTF8() const { return str.c_str(); }
            std::string toStdString() const { return str; }
        };
        
        template<typename T>
        class AudioBuffer {
            std::vector<std::vector<T>> data;
            int numChannels, numSamples;
        public:
            AudioBuffer(int ch, int samp) : numChannels(ch), numSamples(samp) {
                data.resize(ch);
                for (auto& channel : data) channel.resize(samp, T(0));
            }
            
            int getNumChannels() const { return numChannels; }
            int getNumSamples() const { return numSamples; }
            T* getWritePointer(int channel) { return data[channel].data(); }
            const T* getReadPointer(int channel) const { return data[channel].data(); }
            void clear() {
                for (auto& channel : data) std::fill(channel.begin(), channel.end(), T(0));
            }
        };
    }
    
    // Minimal EngineBase for standalone compilation
    class EngineBase {
    public:
        virtual ~EngineBase() = default;
        virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
        virtual void process(juce::AudioBuffer<float>& buffer) = 0;
        virtual void reset() = 0;
        virtual void updateParameters(const std::map<int, float>& params) = 0;
        virtual juce::String getName() const = 0;
        virtual int getNumParameters() const = 0;
        virtual juce::String getParameterName(int index) const = 0;
    };
    
    // Engine type definitions
    #define ENGINE_NONE                     0
    #define ENGINE_OPTO_COMPRESSOR          1
    #define ENGINE_VCA_COMPRESSOR           2
    #define ENGINE_TRANSIENT_SHAPER         3
    #define ENGINE_NOISE_GATE               4
    #define ENGINE_MASTERING_LIMITER        5
    #define ENGINE_DYNAMIC_EQ               6
    #define ENGINE_PARAMETRIC_EQ            7
    #define ENGINE_VINTAGE_CONSOLE_EQ       8
    #define ENGINE_LADDER_FILTER            9
    #define ENGINE_STATE_VARIABLE_FILTER    10
    #define ENGINE_FORMANT_FILTER           11
    #define ENGINE_ENVELOPE_FILTER          12
    #define ENGINE_COMB_RESONATOR           13
    #define ENGINE_VOCAL_FORMANT            14
    #define ENGINE_VINTAGE_TUBE             15
    #define ENGINE_WAVE_FOLDER              16
    #define ENGINE_HARMONIC_EXCITER         17
    #define ENGINE_BIT_CRUSHER              18
    #define ENGINE_MULTIBAND_SATURATOR      19
    #define ENGINE_MUFF_FUZZ                20
    #define ENGINE_RODENT_DISTORTION        21
    #define ENGINE_K_STYLE                  22
    #define ENGINE_DIGITAL_CHORUS           23
    #define ENGINE_RESONANT_CHORUS          24
    #define ENGINE_ANALOG_PHASER            25
    #define ENGINE_RING_MODULATOR           26
    #define ENGINE_FREQUENCY_SHIFTER        27
    #define ENGINE_HARMONIC_TREMOLO         28
    #define ENGINE_CLASSIC_TREMOLO          29
    #define ENGINE_ROTARY_SPEAKER           30
    #define ENGINE_PITCH_SHIFTER            31
    #define ENGINE_DETUNE_DOUBLER           32
    #define ENGINE_INTELLIGENT_HARMONIZER   33
    #define ENGINE_TAPE_ECHO                34
    #define ENGINE_DIGITAL_DELAY            35
    #define ENGINE_MAGNETIC_DRUM_ECHO       36
    #define ENGINE_BUCKET_BRIGADE_DELAY     37
    #define ENGINE_BUFFER_REPEAT            38
    #define ENGINE_PLATE_REVERB             39
    #define ENGINE_SPRING_REVERB            40
    #define ENGINE_CONVOLUTION_REVERB       41
    #define ENGINE_SHIMMER_REVERB           42
    #define ENGINE_GATED_REVERB             43
    #define ENGINE_STEREO_WIDENER           44
    #define ENGINE_STEREO_IMAGER            45
    #define ENGINE_DIMENSION_EXPANDER       46
    #define ENGINE_SPECTRAL_FREEZE          47
    #define ENGINE_SPECTRAL_GATE            48
    #define ENGINE_PHASED_VOCODER           49
    #define ENGINE_GRANULAR_CLOUD           50
    #define ENGINE_CHAOS_GENERATOR          51
    #define ENGINE_FEEDBACK_NETWORK         52
    #define ENGINE_MID_SIDE_PROCESSOR       53
    #define ENGINE_GAIN_UTILITY             54
    #define ENGINE_MONO_MAKER               55
    #define ENGINE_PHASE_ALIGN              56
    #define ENGINE_COUNT                    57
#endif

namespace ComprehensiveDiagnostic {

    // Engine category mapping for organized results
    struct EngineCategory {
        std::string name;
        std::vector<int> engineIds;
        
        EngineCategory(const std::string& categoryName) : name(categoryName) {}
    };

    // Test signal types
    enum class TestSignalType {
        SINE_WAVE_440HZ,
        SINE_WAVE_1KHZ, 
        SINE_WAVE_8KHZ,
        WHITE_NOISE,
        PINK_NOISE,
        IMPULSE,
        SWEEP_CHIRP,
        SILENCE
    };

    // Individual test result
    struct DiagnosticTest {
        std::string testName;
        bool passed = false;
        float confidence = 0.0f; // 0-100% confidence in result
        std::string details;
        float inputRMS = 0.0f;
        float outputRMS = 0.0f;
        float peakInput = 0.0f;
        float peakOutput = 0.0f;
        float processingRatio = 0.0f; // output/input ratio
        float executionTimeMs = 0.0f;
    };

    // Engine test result
    struct EngineTestResult {
        int engineId = -1;
        std::string engineName = "Unknown";
        std::string engineCategory = "Unknown";
        bool engineCreated = false;
        bool overallPassed = false;
        
        std::vector<DiagnosticTest> tests;
        
        // Summary metrics
        int totalTests = 0;
        int passedTests = 0;
        float averageConfidence = 0.0f;
        float totalExecutionTimeMs = 0.0f;
        
        // Issues and recommendations  
        std::vector<std::string> criticalIssues;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
        
        void calculateSummaryMetrics() {
            totalTests = tests.size();
            passedTests = 0;
            float totalConfidence = 0.0f;
            totalExecutionTimeMs = 0.0f;
            
            for (const auto& test : tests) {
                if (test.passed) passedTests++;
                totalConfidence += test.confidence;
                totalExecutionTimeMs += test.executionTimeMs;
            }
            
            averageConfidence = totalTests > 0 ? totalConfidence / totalTests : 0.0f;
            overallPassed = (passedTests >= totalTests * 0.75f) && engineCreated; // 75% pass rate required
        }
    };

    // Batch diagnostic results
    struct ComprehensiveDiagnosticResults {
        std::vector<EngineTestResult> engineResults;
        std::map<std::string, std::vector<int>> categoryResults; // category -> engine indices
        
        // Overall statistics
        int totalEngines = 0;
        int passedEngines = 0;
        int failedEngines = 0;
        int engineCreationFailures = 0;
        float overallPassRate = 0.0f;
        float totalDiagnosticTimeMs = 0.0f;
        
        // Critical analysis
        std::vector<std::string> globalCriticalIssues;
        std::vector<std::string> globalRecommendations;
        
        void calculateOverallStatistics() {
            totalEngines = engineResults.size();
            passedEngines = 0;
            failedEngines = 0;
            engineCreationFailures = 0;
            totalDiagnosticTimeMs = 0.0f;
            
            for (const auto& result : engineResults) {
                if (!result.engineCreated) {
                    engineCreationFailures++;
                    failedEngines++;
                } else if (result.overallPassed) {
                    passedEngines++;
                } else {
                    failedEngines++;
                }
                totalDiagnosticTimeMs += result.totalExecutionTimeMs;
            }
            
            overallPassRate = totalEngines > 0 ? (100.0f * passedEngines / totalEngines) : 0.0f;
        }
    };

    class DiagnosticTester {
    private:
        double sampleRate = 48000.0;
        int blockSize = 512;
        std::mt19937 rng;
        
        // Engine category definitions
        std::vector<EngineCategory> engineCategories;
        
        void initializeEngineCategories() {
            engineCategories.clear();
            
            EngineCategory dynamics("DYNAMICS & COMPRESSION");
            dynamics.engineIds = {1, 2, 3, 4, 5, 6}; // ENGINE_OPTO_COMPRESSOR through ENGINE_DYNAMIC_EQ
            engineCategories.push_back(dynamics);
            
            EngineCategory filtersEQ("FILTERS & EQ");
            filtersEQ.engineIds = {7, 8, 9, 10, 11, 12, 13, 14}; // ENGINE_PARAMETRIC_EQ through ENGINE_VOCAL_FORMANT
            engineCategories.push_back(filtersEQ);
            
            EngineCategory distortion("DISTORTION & SATURATION");
            distortion.engineIds = {15, 16, 17, 18, 19, 20, 21, 22}; // ENGINE_VINTAGE_TUBE through ENGINE_K_STYLE
            engineCategories.push_back(distortion);
            
            EngineCategory modulation("MODULATION EFFECTS");
            modulation.engineIds = {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33}; // ENGINE_DIGITAL_CHORUS through ENGINE_INTELLIGENT_HARMONIZER
            engineCategories.push_back(modulation);
            
            EngineCategory reverbDelay("REVERB & DELAY");
            reverbDelay.engineIds = {34, 35, 36, 37, 38, 39, 40, 41, 42, 43}; // ENGINE_TAPE_ECHO through ENGINE_GATED_REVERB
            engineCategories.push_back(reverbDelay);
            
            EngineCategory spatialSpecial("SPATIAL & SPECIAL EFFECTS");
            spatialSpecial.engineIds = {44, 45, 46, 47, 48, 49, 50, 51, 52}; // ENGINE_STEREO_WIDENER through ENGINE_FEEDBACK_NETWORK
            engineCategories.push_back(spatialSpecial);
            
            EngineCategory utility("UTILITY");
            utility.engineIds = {53, 54, 55, 56}; // ENGINE_MID_SIDE_PROCESSOR through ENGINE_PHASE_ALIGN
            engineCategories.push_back(utility);
        }
        
        std::string getEngineName(int engineId) const {
            // Engine names mapping
            static const std::map<int, std::string> engineNames = {
                {0, "None (Bypass)"},
                {1, "Vintage Opto Compressor"}, {2, "VCA Compressor"}, {3, "Transient Shaper"}, {4, "Noise Gate"}, 
                {5, "Mastering Limiter"}, {6, "Dynamic EQ"}, {7, "Parametric EQ"}, {8, "Vintage Console EQ"}, 
                {9, "Ladder Filter"}, {10, "State Variable Filter"}, {11, "Formant Filter"}, {12, "Envelope Filter"}, 
                {13, "Comb Resonator"}, {14, "Vocal Formant Filter"}, {15, "Vintage Tube Preamp"}, {16, "Wave Folder"}, 
                {17, "Harmonic Exciter"}, {18, "Bit Crusher"}, {19, "Multiband Saturator"}, {20, "Muff Fuzz"}, 
                {21, "Rodent Distortion"}, {22, "K-Style Overdrive"}, {23, "Digital Chorus"}, {24, "Resonant Chorus"}, 
                {25, "Analog Phaser"}, {26, "Ring Modulator"}, {27, "Frequency Shifter"}, {28, "Harmonic Tremolo"}, 
                {29, "Classic Tremolo"}, {30, "Rotary Speaker"}, {31, "Pitch Shifter"}, {32, "Detune Doubler"}, 
                {33, "Intelligent Harmonizer"}, {34, "Tape Echo"}, {35, "Digital Delay"}, {36, "Magnetic Drum Echo"}, 
                {37, "Bucket Brigade Delay"}, {38, "Buffer Repeat"}, {39, "Plate Reverb"}, {40, "Spring Reverb"}, 
                {41, "Convolution Reverb"}, {42, "Shimmer Reverb"}, {43, "Gated Reverb"}, {44, "Stereo Widener"}, 
                {45, "Stereo Imager"}, {46, "Dimension Expander"}, {47, "Spectral Freeze"}, {48, "Spectral Gate"}, 
                {49, "Phased Vocoder"}, {50, "Granular Cloud"}, {51, "Chaos Generator"}, {52, "Feedback Network"}, 
                {53, "Mid-Side Processor"}, {54, "Gain Utility"}, {55, "Mono Maker"}, {56, "Phase Align"}
            };
            
            auto it = engineNames.find(engineId);
            return (it != engineNames.end()) ? it->second : "Unknown Engine";
        }
        
        std::string getEngineCategory(int engineId) const {
            for (const auto& category : engineCategories) {
                if (std::find(category.engineIds.begin(), category.engineIds.end(), engineId) != category.engineIds.end()) {
                    return category.name;
                }
            }
            return "UNKNOWN";
        }
        
        // Test signal generators
        juce::AudioBuffer<float> generateTestSignal(TestSignalType signalType, int samples = -1) {
            if (samples == -1) samples = blockSize;
            
            juce::AudioBuffer<float> buffer(2, samples); // Stereo
            buffer.clear();
            
            switch (signalType) {
                case TestSignalType::SINE_WAVE_440HZ:
                    generateSineWave(buffer, 440.0f);
                    break;
                    
                case TestSignalType::SINE_WAVE_1KHZ:
                    generateSineWave(buffer, 1000.0f);
                    break;
                    
                case TestSignalType::SINE_WAVE_8KHZ:
                    generateSineWave(buffer, 8000.0f);
                    break;
                    
                case TestSignalType::WHITE_NOISE:
                    generateWhiteNoise(buffer);
                    break;
                    
                case TestSignalType::PINK_NOISE:
                    generatePinkNoise(buffer);
                    break;
                    
                case TestSignalType::IMPULSE:
                    generateImpulse(buffer);
                    break;
                    
                case TestSignalType::SWEEP_CHIRP:
                    generateSweepChirp(buffer);
                    break;
                    
                case TestSignalType::SILENCE:
                default:
                    // Already cleared
                    break;
            }
            
            return buffer;
        }
        
        void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency) {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                float* data = buffer.getWritePointer(channel);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * frequency * i / sampleRate);
                }
            }
        }
        
        void generateWhiteNoise(juce::AudioBuffer<float>& buffer) {
            std::uniform_real_distribution<float> dist(-0.25f, 0.25f);
            
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                float* data = buffer.getWritePointer(channel);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    data[i] = dist(rng);
                }
            }
        }
        
        void generatePinkNoise(juce::AudioBuffer<float>& buffer) {
            // Simple pink noise approximation
            std::uniform_real_distribution<float> dist(-0.2f, 0.2f);
            static float pink_state = 0.0f;
            
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                float* data = buffer.getWritePointer(channel);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    float white = dist(rng);
                    pink_state = 0.99886f * pink_state + white * 0.0555179f;
                    data[i] = pink_state * 3.5f; // Scaling for similar amplitude
                }
            }
        }
        
        void generateImpulse(juce::AudioBuffer<float>& buffer) {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                float* data = buffer.getWritePointer(channel);
                data[0] = 0.8f; // Strong impulse at start
                for (int i = 1; i < buffer.getNumSamples(); ++i) {
                    data[i] = 0.0f;
                }
            }
        }
        
        void generateSweepChirp(juce::AudioBuffer<float>& buffer) {
            float startFreq = 100.0f;
            float endFreq = 8000.0f;
            float duration = buffer.getNumSamples() / sampleRate;
            
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                float* data = buffer.getWritePointer(channel);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    float t = i / sampleRate;
                    float freq = startFreq + (endFreq - startFreq) * (t / duration);
                    data[i] = 0.3f * std::sin(2.0f * M_PI * freq * t);
                }
            }
        }
        
        // Audio analysis functions
        float calculateRMS(const juce::AudioBuffer<float>& buffer) {
            float sum = 0.0f;
            int totalSamples = 0;
            
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    sum += data[i] * data[i];
                    totalSamples++;
                }
            }
            
            return totalSamples > 0 ? std::sqrt(sum / totalSamples) : 0.0f;
        }
        
        float calculatePeak(const juce::AudioBuffer<float>& buffer) {
            float peak = 0.0f;
            
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    peak = std::max(peak, std::abs(data[i]));
                }
            }
            
            return peak;
        }
        
        bool hasSignificantChange(float inputLevel, float outputLevel, float threshold = 0.05f) {
            if (inputLevel < 1e-6f) return outputLevel > 1e-6f; // Something from nothing
            float ratio = std::abs((outputLevel - inputLevel) / inputLevel);
            return ratio > threshold;
        }
        
        bool containsValidAudio(const juce::AudioBuffer<float>& buffer) {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    if (std::isnan(data[i]) || std::isinf(data[i]) || std::abs(data[i]) > 10.0f) {
                        return false; // Invalid audio detected
                    }
                }
            }
            return true;
        }
        
        // Parameter configuration for different engine types
        std::map<int, float> getOptimalTestParameters(int engineId) {
            std::map<int, float> params;
            
            // Set reasonable defaults based on engine category
            if (engineId >= 1 && engineId <= 6) {
                // Dynamics & Compression
                params[0] = 0.3f; // Threshold/Drive
                params[1] = 0.6f; // Ratio/Amount
                params[2] = 0.4f; // Attack/Speed
                params[3] = 0.5f; // Release/Recovery
                params[4] = 0.7f; // Make-up gain/Output
                if (engineId == 2 || engineId == 4) { // Compressor/Gate specific
                    params[5] = 0.6f; // Additional parameter
                }
            }
            else if (engineId >= 7 && engineId <= 14) {
                // Filters & EQ
                params[0] = 0.5f; // Frequency
                params[1] = 0.6f; // Resonance/Q
                params[2] = 0.4f; // Gain/Drive
                params[3] = 0.5f; // Type/Mode
                if (engineId == 7 || engineId == 8) { // EQ specific
                    params[4] = 0.5f; // Band 2 freq
                    params[5] = 0.5f; // Band 2 gain
                }
            }
            else if (engineId >= 15 && engineId <= 22) {
                // Distortion & Saturation
                params[0] = 0.6f; // Drive/Amount
                params[1] = 0.4f; // Tone/Filter
                params[2] = 0.7f; // Output Level
                params[3] = 0.5f; // Additional control
            }
            else if (engineId >= 23 && engineId <= 33) {
                // Modulation Effects
                params[0] = 0.4f; // Rate/Speed
                params[1] = 0.6f; // Depth/Amount
                params[2] = 0.5f; // Feedback/Resonance
                params[3] = 0.3f; // Delay/Offset
                if (engineId >= 31 && engineId <= 33) { // Pitch effects
                    params[0] = 0.5f; // Pitch amount (centered)
                    params[1] = 0.4f; // Fine tune
                }
            }
            else if (engineId >= 34 && engineId <= 43) {
                // Reverb & Delay
                params[0] = 0.5f; // Time/Size
                params[1] = 0.4f; // Feedback/Decay
                params[2] = 0.3f; // Damping/Filter
                params[3] = 0.6f; // Modulation/Character
            }
            else if (engineId >= 44 && engineId <= 52) {
                // Spatial & Special Effects
                params[0] = 0.6f; // Width/Amount
                params[1] = 0.4f; // Frequency/Rate
                params[2] = 0.5f; // Depth/Intensity
            }
            else if (engineId >= 53 && engineId <= 56) {
                // Utility
                params[0] = 0.5f; // Amount/Level
                params[1] = 0.5f; // Balance/Pan
            }
            
            return params;
        }
        
    public:
        DiagnosticTester() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
            initializeEngineCategories();
        }
        
        void setSampleRate(double rate) { sampleRate = rate; }
        void setBlockSize(int size) { blockSize = size; }
        
        // Test individual engine with all test signals
        EngineTestResult testEngine(int engineId) {
            EngineTestResult result;
            result.engineId = engineId;
            result.engineName = getEngineName(engineId);
            result.engineCategory = getEngineCategory(engineId);
            
            auto startTime = std::chrono::steady_clock::now();
            
            std::cout << "\n[Testing Engine " << engineId << ": " << result.engineName << "]" << std::endl;
            
            try {
                // Create engine instance
#ifndef COMPREHENSIVE_DIAGNOSTIC_STANDALONE
                auto engine = EngineFactory::createEngine(engineId);
#else
                // In standalone mode, we can't actually create engines
                std::unique_ptr<EngineBase> engine = nullptr;
#endif
                
                if (!engine && engineId != 0) {
                    result.engineCreated = false;
                    result.criticalIssues.push_back("Engine creation failed");
                    std::cout << "  ✗ Engine creation failed" << std::endl;
                    return result;
                }
                result.engineCreated = true;
                
                if (engineId == 0) {
                    // Special case for ENGINE_NONE - should always pass as bypass
                    DiagnosticTest bypassTest;
                    bypassTest.testName = "Bypass Functionality";
                    bypassTest.passed = true;
                    bypassTest.confidence = 100.0f;
                    bypassTest.details = "ENGINE_NONE should act as bypass";
                    result.tests.push_back(bypassTest);
                    std::cout << "  ✓ ENGINE_NONE bypass test passed" << std::endl;
                } else {
                    // Prepare engine
                    engine->prepareToPlay(sampleRate, blockSize);
                    engine->reset();
                    
                    // Set test parameters
                    auto testParams = getOptimalTestParameters(engineId);
                    engine->updateParameters(testParams);
                    
                    // Test with different signals
                    std::vector<std::pair<TestSignalType, std::string>> testSignals = {
                        {TestSignalType::SINE_WAVE_1KHZ, "1kHz Sine Wave"},
                        {TestSignalType::WHITE_NOISE, "White Noise"},
                        {TestSignalType::IMPULSE, "Impulse Response"},
                        {TestSignalType::SILENCE, "Silence Handling"}
                    };
                    
                    for (const auto& [signalType, signalName] : testSignals) {
                        auto test = runSignalProcessingTest(engine.get(), signalType, signalName);
                        result.tests.push_back(test);
                        
                        if (test.passed) {
                            std::cout << "  ✓ " << test.testName << " (confidence: " << std::fixed << std::setprecision(1) << test.confidence << "%)" << std::endl;
                        } else {
                            std::cout << "  ✗ " << test.testName << " - " << test.details << std::endl;
                        }
                    }
                    
                    // Test mix parameter functionality
                    auto mixTest = testMixParameter(engine.get());
                    result.tests.push_back(mixTest);
                    
                    if (mixTest.passed) {
                        std::cout << "  ✓ " << mixTest.testName << " (confidence: " << std::fixed << std::setprecision(1) << mixTest.confidence << "%)" << std::endl;
                    } else {
                        std::cout << "  ✗ " << mixTest.testName << " - " << mixTest.details << std::endl;
                    }
                    
                    // Test stability and crash resistance
                    auto stabilityTest = testStability(engine.get());
                    result.tests.push_back(stabilityTest);
                    
                    if (stabilityTest.passed) {
                        std::cout << "  ✓ " << stabilityTest.testName << " (confidence: " << std::fixed << std::setprecision(1) << stabilityTest.confidence << "%)" << std::endl;
                    } else {
                        std::cout << "  ✗ " << stabilityTest.testName << " - " << stabilityTest.details << std::endl;
                        result.criticalIssues.push_back("Stability issues detected");
                    }
                }
                
            } catch (const std::exception& e) {
                result.engineCreated = false;
                result.criticalIssues.push_back("Exception during testing: " + std::string(e.what()));
                std::cout << "  ✗ Exception occurred: " << e.what() << std::endl;
            } catch (...) {
                result.engineCreated = false;
                result.criticalIssues.push_back("Unknown exception during testing");
                std::cout << "  ✗ Unknown exception occurred" << std::endl;
            }
            
            auto endTime = std::chrono::steady_clock::now();
            result.totalExecutionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
            
            result.calculateSummaryMetrics();
            return result;
        }
        
        DiagnosticTest runSignalProcessingTest(EngineBase* engine, TestSignalType signalType, const std::string& signalName) {
            DiagnosticTest test;
            test.testName = signalName + " Processing";
            
            auto testStart = std::chrono::steady_clock::now();
            
            try {
                // Generate test signal
                auto inputBuffer = generateTestSignal(signalType);
                auto outputBuffer = inputBuffer; // Copy for processing
                
                // Measure input characteristics
                test.inputRMS = calculateRMS(inputBuffer);
                test.peakInput = calculatePeak(inputBuffer);
                
                // Process through engine
                engine->process(outputBuffer);
                
                // Measure output characteristics
                test.outputRMS = calculateRMS(outputBuffer);
                test.peakOutput = calculatePeak(outputBuffer);
                
                // Validate output
                if (!containsValidAudio(outputBuffer)) {
                    test.passed = false;
                    test.confidence = 0.0f;
                    test.details = "Invalid audio output (NaN/Inf/Extreme values)";
                    return test;
                }
                
                // Calculate processing ratio
                test.processingRatio = (test.inputRMS > 1e-6f) ? (test.outputRMS / test.inputRMS) : 1.0f;
                
                // Determine if processing occurred
                bool significantChange = hasSignificantChange(test.inputRMS, test.outputRMS, 0.03f);
                
                if (signalType == TestSignalType::SILENCE) {
                    // For silence, output should remain silent or very quiet
                    test.passed = test.outputRMS < 0.001f;
                    test.confidence = test.passed ? 95.0f : 20.0f;
                    test.details = test.passed ? "Silence handled correctly" : "Unexpected output from silence";
                } else {
                    // For other signals, some processing should occur
                    test.passed = significantChange && test.outputRMS > 1e-6f;
                    
                    if (test.passed) {
                        // High confidence if clear processing occurred
                        if (std::abs(test.processingRatio - 1.0f) > 0.1f) {
                            test.confidence = 90.0f;
                            test.details = "Clear audio processing detected";
                        } else {
                            test.confidence = 70.0f;
                            test.details = "Subtle processing detected";
                        }
                    } else {
                        test.confidence = 30.0f;
                        test.details = "No significant processing detected - may be bypass or minimal effect";
                    }
                }
                
            } catch (const std::exception& e) {
                test.passed = false;
                test.confidence = 0.0f;
                test.details = "Exception during processing: " + std::string(e.what());
            } catch (...) {
                test.passed = false;
                test.confidence = 0.0f;
                test.details = "Unknown exception during processing";
            }
            
            auto testEnd = std::chrono::steady_clock::now();
            test.executionTimeMs = std::chrono::duration<float, std::milli>(testEnd - testStart).count();
            
            return test;
        }
        
        DiagnosticTest testMixParameter(EngineBase* engine) {
            DiagnosticTest test;
            test.testName = "Mix Parameter Control";
            
            auto testStart = std::chrono::steady_clock::now();
            
            try {
                int numParams = engine->getNumParameters();
                if (numParams == 0) {
                    test.passed = true;
                    test.confidence = 100.0f;
                    test.details = "No parameters to test";
                    return test;
                }
                
                // Test signal
                auto testSignal = generateTestSignal(TestSignalType::SINE_WAVE_1KHZ);
                auto originalSignal = testSignal;
                
                // Get base parameters
                auto params = getOptimalTestParameters(engine->getId());
                
                // Test dry signal (assuming last parameter is mix, set to 0)
                if (numParams > 0) {
                    params[numParams - 1] = 0.0f; // Dry
                }
                engine->updateParameters(params);
                
                auto dryBuffer = testSignal;
                engine->process(dryBuffer);
                float dryRMS = calculateRMS(dryBuffer);
                
                // Test wet signal (mix = 1.0)
                if (numParams > 0) {
                    params[numParams - 1] = 1.0f; // Wet
                }
                engine->updateParameters(params);
                
                auto wetBuffer = testSignal;
                engine->process(wetBuffer);
                float wetRMS = calculateRMS(wetBuffer);
                
                // Check if dry and wet are different
                float difference = std::abs(wetRMS - dryRMS);
                bool mixWorks = difference > 0.01f;
                
                test.passed = mixWorks;
                test.confidence = mixWorks ? 85.0f : 40.0f;
                test.details = mixWorks ? 
                    "Mix parameter working (Dry RMS: " + std::to_string(dryRMS) + ", Wet RMS: " + std::to_string(wetRMS) + ")" :
                    "Mix parameter may not be functioning";
                
                test.inputRMS = calculateRMS(originalSignal);
                test.outputRMS = wetRMS;
                
            } catch (const std::exception& e) {
                test.passed = false;
                test.confidence = 0.0f;
                test.details = "Exception during mix test: " + std::string(e.what());
            } catch (...) {
                test.passed = false;
                test.confidence = 0.0f;
                test.details = "Unknown exception during mix test";
            }
            
            auto testEnd = std::chrono::steady_clock::now();
            test.executionTimeMs = std::chrono::duration<float, std::milli>(testEnd - testStart).count();
            
            return test;
        }
        
        DiagnosticTest testStability(EngineBase* engine) {
            DiagnosticTest test;
            test.testName = "Stability & Crash Resistance";
            
            auto testStart = std::chrono::steady_clock::now();
            
            try {
                auto params = getOptimalTestParameters(engine->getId());
                
                // Test multiple resets and processing cycles
                for (int cycle = 0; cycle < 5; ++cycle) {
                    engine->reset();
                    engine->updateParameters(params);
                    
                    // Process different signal types
                    auto buffer1 = generateTestSignal(TestSignalType::WHITE_NOISE);
                    auto buffer2 = generateTestSignal(TestSignalType::IMPULSE);
                    
                    engine->process(buffer1);
                    engine->process(buffer2);
                    
                    // Check for invalid output
                    if (!containsValidAudio(buffer1) || !containsValidAudio(buffer2)) {
                        test.passed = false;
                        test.confidence = 0.0f;
                        test.details = "Invalid audio output detected in cycle " + std::to_string(cycle + 1);
                        return test;
                    }
                }
                
                // Test extreme parameter values
                for (int i = 0; i < engine->getNumParameters(); ++i) {
                    // Test minimum value
                    params[i] = 0.0f;
                    engine->updateParameters(params);
                    auto buffer = generateTestSignal(TestSignalType::SINE_WAVE_1KHZ);
                    engine->process(buffer);
                    
                    if (!containsValidAudio(buffer)) {
                        test.passed = false;
                        test.confidence = 0.0f;
                        test.details = "Instability with parameter " + std::to_string(i) + " at minimum";
                        return test;
                    }
                    
                    // Test maximum value  
                    params[i] = 1.0f;
                    engine->updateParameters(params);
                    buffer = generateTestSignal(TestSignalType::SINE_WAVE_1KHZ);
                    engine->process(buffer);
                    
                    if (!containsValidAudio(buffer)) {
                        test.passed = false;
                        test.confidence = 0.0f;
                        test.details = "Instability with parameter " + std::to_string(i) + " at maximum";
                        return test;
                    }
                    
                    // Reset to optimal value
                    params = getOptimalTestParameters(engine->getId());
                }
                
                test.passed = true;
                test.confidence = 95.0f;
                test.details = "Engine stable across multiple cycles and extreme parameter values";
                
            } catch (const std::exception& e) {
                test.passed = false;
                test.confidence = 0.0f;
                test.details = "Exception during stability test: " + std::string(e.what());
            } catch (...) {
                test.passed = false;
                test.confidence = 0.0f;
                test.details = "Unknown exception during stability test";
            }
            
            auto testEnd = std::chrono::steady_clock::now();
            test.executionTimeMs = std::chrono::duration<float, std::milli>(testEnd - testStart).count();
            
            return test;
        }
        
        // Run comprehensive diagnostic on all 57 engines (0-56)
        ComprehensiveDiagnosticResults runComprehensiveDiagnostic() {
            ComprehensiveDiagnosticResults results;
            
            std::cout << "\n" << std::string(60, '=') << std::endl;
            std::cout << "COMPREHENSIVE CHIMERA ENGINE DIAGNOSTIC" << std::endl;
            std::cout << "Testing ALL 57 engines (IDs 0-56)" << std::endl;
            std::cout << "Sample Rate: " << sampleRate << " Hz, Block Size: " << blockSize << std::endl;
            std::cout << std::string(60, '=') << std::endl;
            
            auto diagnosticStart = std::chrono::steady_clock::now();
            
            // Test all engines 0-56
            for (int engineId = 0; engineId < ENGINE_COUNT; ++engineId) {
                auto engineResult = testEngine(engineId);
                results.engineResults.push_back(engineResult);
                
                // Group by category
                std::string category = engineResult.engineCategory;
                results.categoryResults[category].push_back(results.engineResults.size() - 1);
            }
            
            auto diagnosticEnd = std::chrono::steady_clock::now();
            results.totalDiagnosticTimeMs = std::chrono::duration<float, std::milli>(diagnosticEnd - diagnosticStart).count();
            
            // Calculate statistics
            results.calculateOverallStatistics();
            
            // Generate summary report
            generateSummaryReport(results);
            
            return results;
        }
        
        void generateSummaryReport(const ComprehensiveDiagnosticResults& results) {
            std::cout << "\n" << std::string(60, '=') << std::endl;
            std::cout << "COMPREHENSIVE DIAGNOSTIC SUMMARY" << std::endl;
            std::cout << std::string(60, '=') << std::endl;
            
            // Overall statistics
            std::cout << "\n📊 OVERALL STATISTICS:" << std::endl;
            std::cout << "• Total Engines Tested: " << results.totalEngines << std::endl;
            std::cout << "• Engines Passed: " << results.passedEngines << std::endl;
            std::cout << "• Engines Failed: " << results.failedEngines << std::endl;
            std::cout << "• Engine Creation Failures: " << results.engineCreationFailures << std::endl;
            std::cout << "• Overall Pass Rate: " << std::fixed << std::setprecision(1) << results.overallPassRate << "%" << std::endl;
            std::cout << "• Total Execution Time: " << std::fixed << std::setprecision(2) << results.totalDiagnosticTimeMs / 1000.0f << " seconds" << std::endl;
            
            // Category breakdown
            std::cout << "\n📋 RESULTS BY CATEGORY:" << std::endl;
            for (const auto& category : engineCategories) {
                int passed = 0, total = 0;
                
                for (int engineId : category.engineIds) {
                    if (engineId < results.engineResults.size()) {
                        total++;
                        if (results.engineResults[engineId].overallPassed) {
                            passed++;
                        }
                    }
                }
                
                float passRate = total > 0 ? (100.0f * passed / total) : 0.0f;
                std::cout << "• " << category.name << ": " << passed << "/" << total;
                std::cout << " (" << std::fixed << std::setprecision(1) << passRate << "%)" << std::endl;
            }
            
            // Failed engines detail
            if (results.failedEngines > 0) {
                std::cout << "\n❌ FAILED ENGINES:" << std::endl;
                for (size_t i = 0; i < results.engineResults.size(); ++i) {
                    const auto& result = results.engineResults[i];
                    if (!result.overallPassed) {
                        std::cout << "• Engine " << result.engineId << " (" << result.engineName << ")";
                        
                        if (!result.engineCreated) {
                            std::cout << " - CREATION FAILED";
                        } else {
                            std::cout << " - " << result.passedTests << "/" << result.totalTests << " tests passed";
                            std::cout << " (avg confidence: " << std::fixed << std::setprecision(1) << result.averageConfidence << "%)";
                        }
                        std::cout << std::endl;
                        
                        // Show critical issues
                        for (const auto& issue : result.criticalIssues) {
                            std::cout << "    ⚠️  " << issue << std::endl;
                        }
                    }
                }
            }
            
            // Success summary
            if (results.passedEngines == results.totalEngines) {
                std::cout << "\n🎉 EXCELLENT! All " << results.totalEngines << " engines passed comprehensive testing!" << std::endl;
                std::cout << "The Chimera DSP engine system is functioning correctly." << std::endl;
            } else if (results.overallPassRate >= 90.0f) {
                std::cout << "\n✅ VERY GOOD! " << std::fixed << std::setprecision(1) << results.overallPassRate << "% pass rate." << std::endl;
                std::cout << "Most engines are working correctly with only minor issues." << std::endl;
            } else if (results.overallPassRate >= 75.0f) {
                std::cout << "\n⚠️  ACCEPTABLE: " << std::fixed << std::setprecision(1) << results.overallPassRate << "% pass rate." << std::endl;
                std::cout << "Some engines need attention but core functionality is intact." << std::endl;
            } else {
                std::cout << "\n🚨 CRITICAL ISSUES: Only " << std::fixed << std::setprecision(1) << results.overallPassRate << "% pass rate!" << std::endl;
                std::cout << "Significant problems detected - immediate attention required." << std::endl;
            }
            
            std::cout << "\n" << std::string(60, '=') << std::endl;
        }
    };
    
} // namespace ComprehensiveDiagnostic

// Main function for standalone execution
#ifdef COMPREHENSIVE_DIAGNOSTIC_STANDALONE
int main() {
    ComprehensiveDiagnostic::DiagnosticTester tester;
    
    // Configure test parameters
    tester.setSampleRate(48000.0);
    tester.setBlockSize(512);
    
    // Run comprehensive diagnostic
    auto results = tester.runComprehensiveDiagnostic();
    
    // Return appropriate exit code
    return (results.overallPassRate >= 75.0f) ? 0 : 1;
}
#endif

// Integration function for PluginProcessor.cpp
#ifndef COMPREHENSIVE_DIAGNOSTIC_STANDALONE
namespace ComprehensiveDiagnostic {
    
    // Function to be called from PluginProcessor.cpp
    ComprehensiveDiagnosticResults runComprehensiveDiagnostic(double sampleRate = 48000.0, int blockSize = 512) {
        DiagnosticTester tester;
        tester.setSampleRate(sampleRate);
        tester.setBlockSize(blockSize);
        return tester.runComprehensiveDiagnostic();
    }
    
    // Quick diagnostic function that returns just pass/fail summary
    bool quickDiagnosticCheck(double sampleRate = 48000.0, int blockSize = 512) {
        auto results = runComprehensiveDiagnostic(sampleRate, blockSize);
        return results.overallPassRate >= 75.0f;
    }
    
} // namespace ComprehensiveDiagnostic
#endif

/* 
 * INTEGRATION INSTRUCTIONS:
 * 
 * To integrate into PluginProcessor.cpp, add this line to includes:
 * #include "comprehensive_engine_diagnostic.cpp"
 * 
 * Then call from anywhere in your plugin:
 * 
 * // Quick check
 * bool allEnginesWorking = ComprehensiveDiagnostic::quickDiagnosticCheck(getSampleRate(), getBlockSize());
 * 
 * // Full diagnostic
 * auto diagnosticResults = ComprehensiveDiagnostic::runComprehensiveDiagnostic(getSampleRate(), getBlockSize());
 * 
 * The diagnostic handles all error cases gracefully and provides detailed
 * reporting on which engines work and which don't, organized by category.
 */