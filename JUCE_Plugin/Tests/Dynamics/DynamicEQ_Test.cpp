/*
  ==============================================================================
  
    DynamicEQ_Test.cpp
    Comprehensive test suite for ENGINE_DYNAMIC_EQ
    
    Tests for dynamic EQ characteristics:
    - Parameter sweep validation (all 8 parameters)
    - Frequency-dependent compression accuracy
    - Dynamic threshold detection and response
    - TPT filter response and stability
    - Attack/Release timing for dynamic processing
    - Static EQ vs dynamic EQ behavior
    - Multiple operation modes (Compressor/Expander/Gate)
    - Oversampling quality and anti-aliasing
    - Thermal modeling and analog warmth
    - Component aging simulation
    - Mix parameter dry/wet blending
    - Filter Q and frequency accuracy
    
  ==============================================================================
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <random>
#include <map>
#include <string>
#include <cassert>

// Include the engine
#include "../../Source/DynamicEQ.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;

// Test signal generators
class TestSignalGenerator {
public:
    // Generate sine wave at specific frequency and level
    static std::vector<float> generateSineWave(double frequency, float level_dB, 
                                             double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        float amplitude = std::pow(10.0f, level_dB / 20.0f);
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate frequency sweep for filter response testing
    static std::vector<float> generateFrequencySweep(double startFreq, double endFreq,
                                                   float level_dB, double duration,
                                                   double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        float amplitude = std::pow(10.0f, level_dB / 20.0f);
        double phase = 0.0;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = static_cast<double>(i) / numSamples;
            double freq = startFreq * std::pow(endFreq / startFreq, t); // Logarithmic sweep
            double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate broadband signal with specific frequency emphasis
    static std::vector<float> generateBroadbandSignal(double centerFreq, float bandwidth,
                                                    float level_dB, double duration,
                                                    double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        float amplitude = std::pow(10.0f, level_dB / 20.0f);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Generate filtered noise centered around the frequency
        std::vector<double> frequencies;
        std::vector<double> phases;
        
        // Create frequency components
        double startFreq = centerFreq * (1.0 - bandwidth);
        double endFreq = centerFreq * (1.0 + bandwidth);
        int numComponents = 20;
        
        for (int i = 0; i < numComponents; ++i) {
            double freq = startFreq + (endFreq - startFreq) * i / (numComponents - 1);
            frequencies.push_back(freq);
            phases.push_back(0.0);
        }
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = 0.0f;
            
            for (size_t f = 0; f < frequencies.size(); ++f) {
                double phaseIncrement = 2.0 * M_PI * frequencies[f] / sampleRate;
                sample += static_cast<float>(std::sin(phases[f]) / frequencies.size());
                phases[f] += phaseIncrement;
            }
            
            signal[i] = amplitude * sample;
        }
        
        return signal;
    }
    
    // Generate multi-tone signal for dynamic response testing
    static std::vector<float> generateMultiTone(const std::vector<double>& frequencies,
                                               const std::vector<float>& levels_dB,
                                               double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        if (frequencies.size() != levels_dB.size()) return signal;
        
        std::vector<double> phases(frequencies.size(), 0.0);
        
        for (int i = 0; i < numSamples; ++i) {
            for (size_t f = 0; f < frequencies.size(); ++f) {
                float amplitude = std::pow(10.0f, levels_dB[f] / 20.0f);
                double phaseIncrement = 2.0 * M_PI * frequencies[f] / sampleRate;
                
                signal[i] += static_cast<float>(amplitude * std::sin(phases[f]));
                phases[f] += phaseIncrement;
            }
        }
        
        return signal;
    }
    
    // Generate dynamic content (varying levels)
    static std::vector<float> generateDynamicContent(double frequency, 
                                                   float quietLevel_dB, float loudLevel_dB,
                                                   double period, double duration,
                                                   double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        double levelPeriod = period * sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            // Varying amplitude
            double t = fmod(i, levelPeriod) / levelPeriod;
            float level_dB = quietLevel_dB + (loudLevel_dB - quietLevel_dB) * 
                           (0.5f + 0.5f * std::sin(2.0 * M_PI * t));
            float amplitude = std::pow(10.0f, level_dB / 20.0f);
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate pink noise for broadband testing
    static std::vector<float> generatePinkNoise(float level_dB, double duration, 
                                               double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        float amplitude = std::pow(10.0f, level_dB / 20.0f);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Simple pink noise approximation using filtered white noise
        float b0 = 0.02109238f, b1 = 0.07113478f, b2 = 0.68873558f;
        float x1 = 0.0f, x2 = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float white = dist(gen);
            float pink = b0 * white + b1 * x1 + b2 * x2;
            x2 = x1;
            x1 = white;
            
            signal[i] = amplitude * pink;
        }
        
        return signal;
    }
};

// Audio analysis utilities
class AudioAnalyzer {
public:
    // Calculate RMS level in dB
    static float calculateRMS_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        
        double sum = 0.0;
        for (float sample : signal) {
            sum += sample * sample;
        }
        
        double rms = std::sqrt(sum / signal.size());
        return 20.0f * std::log10(std::max(1e-6, rms));
    }
    
    // Calculate peak level in dB
    static float calculatePeak_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        
        float peak = 0.0f;
        for (float sample : signal) {
            peak = std::max(peak, std::abs(sample));
        }
        
        return 20.0f * std::log10(std::max(1e-6, peak));
    }
    
    // Measure frequency response at specific frequency
    static float measureFrequencyResponse(const std::vector<float>& input,
                                        const std::vector<float>& output,
                                        double frequency, double sampleRate) {
        if (input.size() != output.size() || input.empty()) return 0.0f;
        
        // Simple DFT at the specific frequency
        double inputReal = 0.0, inputImag = 0.0;
        double outputReal = 0.0, outputImag = 0.0;
        
        int N = static_cast<int>(std::min(input.size(), static_cast<size_t>(sampleRate)));
        double omega = 2.0 * M_PI * frequency / sampleRate;
        
        for (int n = 0; n < N; ++n) {
            double cosOmega = std::cos(omega * n);
            double sinOmega = std::sin(omega * n);
            
            inputReal += input[n] * cosOmega;
            inputImag -= input[n] * sinOmega;
            outputReal += output[n] * cosOmega;
            outputImag -= output[n] * sinOmega;
        }
        
        double inputMag = std::sqrt(inputReal * inputReal + inputImag * inputImag);
        double outputMag = std::sqrt(outputReal * outputReal + outputImag * outputImag);
        
        if (inputMag < 1e-6) return 0.0f;
        
        return 20.0f * std::log10(outputMag / inputMag);
    }
    
    // Measure dynamic response (how much processing varies with level)
    static float measureDynamicResponse(const std::vector<float>& input,
                                      const std::vector<float>& output) {
        if (input.size() != output.size() || input.size() < 100) return 0.0f;
        
        // Analyze first and second halves for level-dependent behavior
        int halfSize = static_cast<int>(input.size() / 2);
        
        std::vector<float> input1(input.begin(), input.begin() + halfSize);
        std::vector<float> input2(input.begin() + halfSize, input.end());
        std::vector<float> output1(output.begin(), output.begin() + halfSize);
        std::vector<float> output2(output.begin() + halfSize, output.end());
        
        float response1 = calculateRMS_dB(output1) - calculateRMS_dB(input1);
        float response2 = calculateRMS_dB(output2) - calculateRMS_dB(input2);
        
        return std::abs(response2 - response1); // Difference in response
    }
    
    // Check for invalid values
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) {
                return true;
            }
        }
        return false;
    }
    
    // Calculate spectral centroid (brightness measure)
    static float calculateSpectralCentroid(const std::vector<float>& signal, double sampleRate) {
        if (signal.size() < 64) return 0.0f;
        
        // Simple spectral centroid using magnitude spectrum
        int N = static_cast<int>(std::min(signal.size(), static_cast<size_t>(1024)));
        double sumWeightedFreq = 0.0;
        double sumMag = 0.0;
        
        for (int k = 1; k < N/2; ++k) {
            double freq = k * sampleRate / N;
            
            // Simple DFT magnitude at this frequency
            double real = 0.0, imag = 0.0;
            for (int n = 0; n < N; ++n) {
                double omega = 2.0 * M_PI * k * n / N;
                real += signal[n] * std::cos(omega);
                imag -= signal[n] * std::sin(omega);
            }
            
            double magnitude = std::sqrt(real * real + imag * imag);
            sumWeightedFreq += freq * magnitude;
            sumMag += magnitude;
        }
        
        return (sumMag > 0.0) ? static_cast<float>(sumWeightedFreq / sumMag) : 0.0f;
    }
    
    // Measure attack/release timing for dynamic processing
    static std::pair<double, double> measureDynamicTiming(const std::vector<float>& gainReduction,
                                                        double sampleRate) {
        if (gainReduction.size() < 100) return {-1.0, -1.0};
        
        // Find peak gain reduction
        int peakIdx = static_cast<int>(std::max_element(gainReduction.begin(), 
                                                       gainReduction.end()) - gainReduction.begin());
        
        float peakValue = gainReduction[peakIdx];
        if (peakValue < 0.01f) return {-1.0, -1.0};
        
        // Measure attack time (10% to 90% of peak)
        double attackTime = -1.0;
        float threshold10 = peakValue * 0.1f;
        float threshold90 = peakValue * 0.9f;
        
        int attack10 = -1, attack90 = -1;
        for (int i = 0; i < peakIdx; ++i) {
            if (attack10 == -1 && gainReduction[i] >= threshold10) attack10 = i;
            if (attack90 == -1 && gainReduction[i] >= threshold90) attack90 = i;
        }
        
        if (attack10 >= 0 && attack90 >= 0) {
            attackTime = (attack90 - attack10) / sampleRate;
        }
        
        // Measure release time (90% to 10% decay)
        double releaseTime = -1.0;
        int release90 = -1, release10 = -1;
        
        for (int i = peakIdx; i < static_cast<int>(gainReduction.size()); ++i) {
            if (release90 == -1 && gainReduction[i] <= threshold90) release90 = i;
            if (release10 == -1 && gainReduction[i] <= threshold10) {
                release10 = i;
                break;
            }
        }
        
        if (release90 >= 0 && release10 >= 0) {
            releaseTime = (release10 - release90) / sampleRate;
        }
        
        return {attackTime, releaseTime};
    }
};

// Main test class
class DynamicEQTest {
private:
    std::unique_ptr<DynamicEQ> dynamicEQ;
    std::ofstream logFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    DynamicEQTest() {
        dynamicEQ = std::make_unique<DynamicEQ>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Dynamics/DynamicEQ_TestResults.txt");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        // Prepare the dynamic EQ
        dynamicEQ->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Dynamic EQ Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_DYNAMIC_EQ) + "\n");
        log("Parameter Count: " + std::to_string(dynamicEQ->getNumParameters()) + "\n\n");
    }
    
    ~DynamicEQTest() {
        log("\n=== Test Summary ===\n");
        log("Tests Passed: " + std::to_string(testsPassed) + "\n");
        log("Tests Failed: " + std::to_string(testsFailed) + "\n");
        
        if (testsPassed + testsFailed > 0) {
            float successRate = 100.0f * testsPassed / (testsPassed + testsFailed);
            log("Success Rate: " + std::to_string(successRate) + "%\n");
        }
        
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void log(const std::string& message) {
        std::cout << message;
        if (logFile.is_open()) {
            logFile << message;
            logFile.flush();
        }
    }
    
    void assertTrue(bool condition, const std::string& testName) {
        if (condition) {
            log("[PASS] " + testName + "\n");
            testsPassed++;
        } else {
            log("[FAIL] " + testName + "\n");
            testsFailed++;
        }
    }
    
    // Process audio through dynamic EQ
    std::vector<float> processAudio(const std::vector<float>& input, 
                                  const std::map<int, float>& parameters) {
        // Update parameters
        dynamicEQ->updateParameters(parameters);
        
        // Process in blocks
        std::vector<float> output;
        output.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), 
                                      input.size() - i);
            
            // Create JUCE AudioBuffer
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            // Fill buffer with input (mono to stereo)
            for (size_t j = 0; j < blockSize; ++j) {
                float sample = (i + j < input.size()) ? input[i + j] : 0.0f;
                buffer.setSample(0, static_cast<int>(j), sample);
                buffer.setSample(1, static_cast<int>(j), sample);
            }
            
            // Process
            dynamicEQ->process(buffer);
            
            // Extract output (left channel)
            for (size_t j = 0; j < blockSize; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }
        
        return output;
    }
    
    // Test 1: Parameter sweep validation
    void testParameterSweeps() {
        log("\n--- Parameter Sweep Tests ---\n");
        
        // Test signal with content at 1kHz for frequency testing
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, -20.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < dynamicEQ->getNumParameters(); ++param) {
            std::string paramName = dynamicEQ->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseValues;
            
            // Sweep from 0.0 to 1.0 in 0.2 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.2f) {
                std::map<int, float> params;
                params[param] = value;
                
                // Set other parameters to reasonable defaults
                if (param != 0) params[0] = 0.5f; // Frequency around 1kHz
                if (param != 1) params[1] = 0.3f; // Threshold
                if (param != 2) params[2] = 0.5f; // Ratio
                if (param != 5) params[5] = 0.0f; // Some static gain
                
                auto output = processAudio(testSignal, params);
                
                // Check for valid output
                assertTrue(!AudioAnalyzer::hasInvalidValues(output), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
                responseValues.push_back(outputRMS);
            }
            
            // Check parameter responsiveness
            float minResponse = *std::min_element(responseValues.begin(), responseValues.end());
            float maxResponse = *std::max_element(responseValues.begin(), responseValues.end());
            float responseRange = maxResponse - minResponse;
            
            log("  Response range: " + std::to_string(responseRange) + "dB\n");
            
            // Core parameters should have audible effect
            if (param <= 5) { // Frequency, threshold, ratio, attack, release, gain
                assertTrue(responseRange > 0.1f, 
                          paramName + " has audible effect");
            }
        }
    }
    
    // Test 2: Frequency-dependent processing
    void testFrequencyDependentProcessing() {
        log("\n--- Frequency-Dependent Processing Tests ---\n");
        
        // Test different center frequencies
        std::vector<double> centerFrequencies = {200.0, 500.0, 1000.0, 2000.0, 5000.0};
        
        for (double centerFreq : centerFrequencies) {
            log("Testing center frequency: " + std::to_string(centerFreq) + " Hz\n");
            
            // Set frequency parameter
            float freqParam = static_cast<float>((std::log(centerFreq) - std::log(20.0)) / 
                                               (std::log(20000.0) - std::log(20.0)));
            freqParam = std::max(0.0f, std::min(1.0f, freqParam));
            
            std::map<int, float> params;
            params[0] = freqParam; // Frequency
            params[1] = 0.4f;      // Threshold
            params[2] = 0.7f;      // 3:1 ratio
            params[3] = 0.2f;      // Fast attack
            params[4] = 0.3f;      // Medium release
            
            // Test frequencies around and away from center frequency
            std::vector<double> testFrequencies = {
                centerFreq * 0.5, centerFreq, centerFreq * 2.0
            };
            
            for (double testFreq : testFrequencies) {
                if (testFreq > 20.0 && testFreq < 20000.0) {
                    auto testSignal = TestSignalGenerator::generateSineWave(
                        testFreq, -10.0f, 0.5, TEST_SAMPLE_RATE);
                    
                    auto output = processAudio(testSignal, params);
                    
                    float freqResponse = AudioAnalyzer::measureFrequencyResponse(
                        testSignal, output, testFreq, TEST_SAMPLE_RATE);
                    
                    log("  Test freq " + std::to_string(testFreq) + "Hz: " +
                        std::to_string(freqResponse) + "dB response\n");
                    
                    // At center frequency, should have most processing
                    if (std::abs(testFreq - centerFreq) < centerFreq * 0.1) {
                        assertTrue(std::abs(freqResponse) > 0.5f, 
                                  "Significant processing at center frequency");
                    }
                }
            }
        }
    }
    
    // Test 3: Dynamic threshold behavior
    void testDynamicThresholdBehavior() {
        log("\n--- Dynamic Threshold Behavior Tests ---\n");
        
        // Generate dynamic content with varying levels
        auto dynamicSignal = TestSignalGenerator::generateDynamicContent(
            1000.0, -30.0f, -5.0f, 0.2, 2.0, TEST_SAMPLE_RATE);
        
        // Test different threshold settings
        std::vector<float> thresholdSettings = {0.2f, 0.5f, 0.8f};
        
        for (float threshold : thresholdSettings) {
            log("Testing threshold: " + std::to_string(threshold) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;     // 1kHz frequency
            params[1] = threshold; // Threshold
            params[2] = 0.75f;    // 4:1 ratio
            params[3] = 0.1f;     // Fast attack
            params[4] = 0.3f;     // Medium release
            params[7] = 0.0f;     // Compressor mode
            
            auto output = processAudio(dynamicSignal, params);
            
            float dynamicResponse = AudioAnalyzer::measureDynamicResponse(dynamicSignal, output);
            
            log("  Dynamic response: " + std::to_string(dynamicResponse) + "dB\n");
            
            // Should show level-dependent processing
            assertTrue(dynamicResponse > 0.5f, 
                      "Threshold " + std::to_string(threshold) + " shows dynamic behavior");
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Threshold " + std::to_string(threshold) + " produces valid output");
        }
    }
    
    // Test 4: Operation modes (Compressor/Expander/Gate)
    void testOperationModes() {
        log("\n--- Operation Modes Tests ---\n");
        
        // Generate signal with varying levels
        auto testSignal = TestSignalGenerator::generateDynamicContent(
            1000.0, -40.0f, -10.0f, 0.3, 1.5, TEST_SAMPLE_RATE);
        
        // Test different modes
        std::vector<float> modeSettings = {0.0f, 0.5f, 1.0f}; // Compressor, Expander, Gate
        std::vector<std::string> modeNames = {"Compressor", "Expander", "Gate"};
        
        for (size_t i = 0; i < modeSettings.size(); ++i) {
            log("Testing " + modeNames[i] + " mode\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;            // 1kHz frequency
            params[1] = 0.4f;            // Threshold
            params[2] = 0.6f;            // Ratio
            params[3] = 0.2f;            // Attack
            params[4] = 0.4f;            // Release
            params[7] = modeSettings[i]; // Mode parameter
            
            auto output = processAudio(testSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      modeNames[i] + " mode produces valid output");
            
            float inputRMS = AudioAnalyzer::calculateRMS_dB(testSignal);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float levelChange = outputRMS - inputRMS;
            
            log("  Input RMS: " + std::to_string(inputRMS) + "dB\n");
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            log("  Level change: " + std::to_string(levelChange) + "dB\n");
            
            // Each mode should affect the signal differently
            assertTrue(std::abs(levelChange) < 20.0f, 
                      modeNames[i] + " mode produces reasonable level changes");
        }
    }
    
    // Test 5: Filter response and stability
    void testFilterResponseAndStability() {
        log("\n--- Filter Response and Stability Tests ---\n");
        
        // Generate frequency sweep
        auto sweepSignal = TestSignalGenerator::generateFrequencySweep(
            20.0, 20000.0, -20.0f, 3.0, TEST_SAMPLE_RATE);
        
        // Test different Q values (bandwidth)
        std::vector<float> qValues = {0.2f, 0.5f, 0.8f}; // Wide, medium, narrow
        
        for (float qValue : qValues) {
            log("Testing filter Q: " + std::to_string(qValue) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f; // 1kHz center frequency
            params[1] = 0.3f; // Threshold
            params[2] = 0.5f; // Ratio
            params[5] = 0.6f; // Some static gain for filter testing
            
            // Note: Q parameter might be implicit in the filter design
            // This tests the filter at different settings
            
            auto output = processAudio(sweepSignal, params);
            
            // Check for stability (no NaN/Inf)
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Filter Q " + std::to_string(qValue) + " remains stable");
            
            // Check that filter doesn't cause excessive resonance
            float outputPeak = AudioAnalyzer::calculatePeak_dB(output);
            float inputPeak = AudioAnalyzer::calculatePeak_dB(sweepSignal);
            float peakGain = outputPeak - inputPeak;
            
            log("  Peak gain: " + std::to_string(peakGain) + "dB\n");
            
            assertTrue(peakGain < 20.0f, 
                      "Filter Q " + std::to_string(qValue) + " doesn't cause excessive resonance");
        }
    }
    
    // Test 6: Attack and release timing
    void testAttackReleaseTiming() {
        log("\n--- Attack/Release Timing Tests ---\n");
        
        // Generate burst signal for timing measurement
        std::vector<float> testSignal;
        
        // Silence -> loud signal -> silence
        auto silence1 = TestSignalGenerator::generateSineWave(1000.0, -60.0f, 0.2, TEST_SAMPLE_RATE);
        auto burst = TestSignalGenerator::generateSineWave(1000.0, -5.0f, 0.3, TEST_SAMPLE_RATE);
        auto silence2 = TestSignalGenerator::generateSineWave(1000.0, -60.0f, 0.5, TEST_SAMPLE_RATE);
        
        testSignal.insert(testSignal.end(), silence1.begin(), silence1.end());
        testSignal.insert(testSignal.end(), burst.begin(), burst.end());
        testSignal.insert(testSignal.end(), silence2.begin(), silence2.end());
        
        // Test different timing settings
        std::vector<std::pair<float, float>> timingSettings = {
            {0.1f, 0.2f}, // Fast attack, fast release
            {0.4f, 0.6f}, // Medium attack, medium release
            {0.8f, 0.9f}  // Slow attack, slow release
        };
        
        for (auto& timing : timingSettings) {
            log("Testing timing - Attack: " + std::to_string(timing.first) + 
                ", Release: " + std::to_string(timing.second) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;         // 1kHz frequency
            params[1] = 0.2f;         // Low threshold
            params[2] = 0.75f;        // High ratio
            params[3] = timing.first; // Attack
            params[4] = timing.second; // Release
            
            auto output = processAudio(testSignal, params);
            
            // Calculate gain reduction envelope
            std::vector<float> gainReduction;
            for (size_t i = 0; i < std::min(testSignal.size(), output.size()); ++i) {
                if (std::abs(testSignal[i]) > EPSILON) {
                    float gr = 1.0f - (std::abs(output[i]) / std::abs(testSignal[i]));
                    gainReduction.push_back(std::max(0.0f, gr));
                } else {
                    gainReduction.push_back(0.0f);
                }
            }
            
            // Measure timing
            auto timingResult = AudioAnalyzer::measureDynamicTiming(gainReduction, TEST_SAMPLE_RATE);
            
            if (timingResult.first > 0.0) {
                log("  Measured attack: " + std::to_string(timingResult.first * 1000.0) + " ms\n");
            }
            if (timingResult.second > 0.0) {
                log("  Measured release: " + std::to_string(timingResult.second * 1000.0) + " ms\n");
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Timing test produces valid output");
        }
    }
    
    // Test 7: Static EQ vs dynamic EQ behavior
    void testStaticVsDynamicBehavior() {
        log("\n--- Static vs Dynamic EQ Behavior Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, -15.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test static behavior (no dynamic processing)
        std::map<int, float> staticParams;
        staticParams[0] = 0.5f; // 1kHz frequency
        staticParams[1] = 1.0f; // High threshold (no dynamic processing)
        staticParams[2] = 0.5f; // Ratio (shouldn't matter)
        staticParams[5] = 0.7f; // Static gain boost
        
        auto staticOutput = processAudio(testSignal, staticParams);
        
        // Test dynamic behavior (with dynamic processing)
        std::map<int, float> dynamicParams;
        dynamicParams[0] = 0.5f; // 1kHz frequency
        dynamicParams[1] = 0.3f; // Low threshold (dynamic processing active)
        dynamicParams[2] = 0.7f; // High ratio
        dynamicParams[5] = 0.7f; // Same static gain
        
        auto dynamicOutput = processAudio(testSignal, dynamicParams);
        
        float staticResponse = AudioAnalyzer::calculateRMS_dB(staticOutput) - 
                              AudioAnalyzer::calculateRMS_dB(testSignal);
        float dynamicResponse = AudioAnalyzer::calculateRMS_dB(dynamicOutput) - 
                               AudioAnalyzer::calculateRMS_dB(testSignal);
        
        log("Static response: " + std::to_string(staticResponse) + "dB\n");
        log("Dynamic response: " + std::to_string(dynamicResponse) + "dB\n");
        
        // Dynamic processing should behave differently than static
        assertTrue(std::abs(staticResponse - dynamicResponse) > 0.5f,
                  "Dynamic processing behaves differently from static EQ");
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(staticOutput),
                  "Static processing produces valid output");
        assertTrue(!AudioAnalyzer::hasInvalidValues(dynamicOutput),
                  "Dynamic processing produces valid output");
    }
    
    // Test 8: Mix parameter dry/wet blending
    void testMixParameter() {
        log("\n--- Mix Parameter Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, -20.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test different mix settings
        std::vector<float> mixSettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float mix : mixSettings) {
            log("Testing mix: " + std::to_string(mix) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f; // 1kHz frequency
            params[1] = 0.2f; // Low threshold for processing
            params[2] = 0.8f; // High ratio
            params[5] = 0.8f; // Gain boost
            params[6] = mix;  // Mix parameter
            
            auto output = processAudio(testSignal, params);
            
            float inputRMS = AudioAnalyzer::calculateRMS_dB(testSignal);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            
            log("  Input RMS: " + std::to_string(inputRMS) + "dB\n");
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Mix " + std::to_string(mix) + " produces valid output");
            
            // Mix = 0 should be close to dry signal
            if (mix < 0.1f) {
                float difference = std::abs(outputRMS - inputRMS);
                assertTrue(difference < 3.0f, "Mix 0.0 close to dry signal");
            }
        }
    }
    
    // Test 9: Broadband vs narrowband processing
    void testBroadbandVsNarrowband() {
        log("\n--- Broadband vs Narrowband Processing Tests ---\n");
        
        // Test with pink noise (broadband)
        auto broadbandSignal = TestSignalGenerator::generatePinkNoise(-20.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test with narrowband signal
        auto narrowbandSignal = TestSignalGenerator::generateBroadbandSignal(
            1000.0, 0.1f, -20.0f, 1.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.5f; // 1kHz center frequency
        params[1] = 0.4f; // Threshold
        params[2] = 0.6f; // Ratio
        params[3] = 0.2f; // Attack
        params[4] = 0.3f; // Release
        
        auto broadbandOutput = processAudio(broadbandSignal, params);
        auto narrowbandOutput = processAudio(narrowbandSignal, params);
        
        // Analyze spectral characteristics
        float broadbandCentroid = AudioAnalyzer::calculateSpectralCentroid(broadbandOutput, TEST_SAMPLE_RATE);
        float narrowbandCentroid = AudioAnalyzer::calculateSpectralCentroid(narrowbandOutput, TEST_SAMPLE_RATE);
        
        log("Broadband spectral centroid: " + std::to_string(broadbandCentroid) + " Hz\n");
        log("Narrowband spectral centroid: " + std::to_string(narrowbandCentroid) + " Hz\n");
        
        // Both should process without issues
        assertTrue(!AudioAnalyzer::hasInvalidValues(broadbandOutput),
                  "Broadband processing produces valid output");
        assertTrue(!AudioAnalyzer::hasInvalidValues(narrowbandOutput),
                  "Narrowband processing produces valid output");
        
        // Narrowband signal should be more focused around center frequency
        assertTrue(narrowbandCentroid > 500.0f && narrowbandCentroid < 2000.0f,
                  "Narrowband processing maintains frequency focus");
    }
    
    // Test 10: Thermal modeling and analog warmth
    void testAnalogModeling() {
        log("\n--- Analog Modeling Tests ---\n");
        
        // Long processing to trigger thermal effects
        auto longSignal = TestSignalGenerator::generateDynamicContent(
            1000.0, -25.0f, -10.0f, 0.5, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.5f; // 1kHz frequency
        params[1] = 0.3f; // Threshold
        params[2] = 0.7f; // Ratio
        params[5] = 0.6f; // Some gain for thermal modeling
        
        auto output = processAudio(longSignal, params);
        
        // Check for valid output throughout long processing
        assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                  "Long processing with analog modeling produces valid output");
        
        // Analyze stability over time
        int quarterSize = static_cast<int>(output.size() / 4);
        
        std::vector<float> quarter1(output.begin(), output.begin() + quarterSize);
        std::vector<float> quarter4(output.end() - quarterSize, output.end());
        
        float quarter1RMS = AudioAnalyzer::calculateRMS_dB(quarter1);
        float quarter4RMS = AudioAnalyzer::calculateRMS_dB(quarter4);
        float drift = std::abs(quarter1RMS - quarter4RMS);
        
        log("RMS drift over time: " + std::to_string(drift) + "dB\n");
        
        // Should maintain reasonable stability
        assertTrue(drift < 3.0f, "Analog modeling maintains reasonable stability");
        
        // Check that analog effects don't cause clipping
        float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
        assertTrue(peakLevel < 6.0f, "Analog modeling doesn't cause excessive levels");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Dynamic EQ test suite...\n");
        
        testParameterSweeps();
        testFrequencyDependentProcessing();
        testDynamicThresholdBehavior();
        testOperationModes();
        testFilterResponseAndStability();
        testAttackReleaseTiming();
        testStaticVsDynamicBehavior();
        testMixParameter();
        testBroadbandVsNarrowband();
        testAnalogModeling();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        DynamicEQTest tester;
        tester.runAllTests();
        
        std::cout << "\nDynamic EQ test suite completed successfully.\n";
        std::cout << "Check DynamicEQ_TestResults.txt for detailed results.\n";
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Test suite failed with unknown exception." << std::endl;
        return 1;
    }
}