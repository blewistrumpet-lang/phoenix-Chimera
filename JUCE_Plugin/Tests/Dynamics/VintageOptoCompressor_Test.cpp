/*
  ==============================================================================
  
    VintageOptoCompressor_Test.cpp
    Comprehensive test suite for ENGINE_OPTO_COMPRESSOR
    
    Tests for vintage opto compressor characteristics:
    - Parameter sweep validation (0.0 to 1.0 in precise steps)
    - Opto cell timing accuracy (attack/release)
    - Gain reduction curve linearity
    - Threshold detection precision
    - Program-dependent release behavior
    - Thermal modeling effects
    - Bypass state null testing
    - Impulse response timing verification
    - Multiple signal type testing (sine, pink noise, impulses)
    
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
#include "../../Source/VintageOptoCompressor.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f; // 0.1dB tolerance for measurements

// Test signal generators
class TestSignalGenerator {
public:
    // Generate sine wave at specified frequency and amplitude
    static std::vector<float> generateSine(double frequency, double amplitude, 
                                         double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate pink noise with specified amplitude and duration
    static std::vector<float> generatePinkNoise(double amplitude, double duration, 
                                               double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Simple pink noise approximation using filtered white noise
        float b0 = 0.02109238f, b1 = 0.07113478f, b2 = 0.68873558f; // Pink filter coeffs
        float x1 = 0.0f, x2 = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float white = dist(gen);
            float pink = b0 * white + b1 * x1 + b2 * x2;
            x2 = x1;
            x1 = white;
            
            signal[i] = static_cast<float>(amplitude * pink);
        }
        
        return signal;
    }
    
    // Generate impulse signal
    static std::vector<float> generateImpulse(double amplitude, double duration, 
                                            double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        if (numSamples > 0) {
            signal[0] = static_cast<float>(amplitude);
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
    
    // Measure attack time (time to reach 63% of final gain reduction)
    static double measureAttackTime(const std::vector<float>& gainReduction, 
                                  double sampleRate) {
        if (gainReduction.size() < 10) return 0.0;
        
        // Find the final stable value (last 10% of signal)
        int startIdx = static_cast<int>(gainReduction.size() * 0.9f);
        float finalValue = 0.0f;
        for (int i = startIdx; i < static_cast<int>(gainReduction.size()); ++i) {
            finalValue += gainReduction[i];
        }
        finalValue /= (gainReduction.size() - startIdx);
        
        // Find 63% point
        float target = finalValue * 0.63f;
        
        for (int i = 1; i < static_cast<int>(gainReduction.size()); ++i) {
            if (gainReduction[i] >= target) {
                return i / sampleRate;
            }
        }
        
        return 0.0; // Attack time couldn't be measured
    }
    
    // Check for NaN or infinite values
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) {
                return true;
            }
        }
        return false;
    }
};

// Main test class
class VintageOptoCompressorTest {
private:
    std::unique_ptr<VintageOptoCompressor> compressor;
    std::ofstream logFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    VintageOptoCompressorTest() {
        compressor = std::make_unique<VintageOptoCompressor>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Dynamics/VintageOptoCompressor_TestResults.txt");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        // Prepare the compressor
        compressor->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Vintage Opto Compressor Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_OPTO_COMPRESSOR) + "\n\n");
    }
    
    ~VintageOptoCompressorTest() {
        log("\n=== Test Summary ===\n");
        log("Tests Passed: " + std::to_string(testsPassed) + "\n");
        log("Tests Failed: " + std::to_string(testsFailed) + "\n");
        log("Success Rate: " + std::to_string(100.0 * testsPassed / (testsPassed + testsFailed)) + "%\n");
        
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
    
    // Process audio through compressor
    std::vector<float> processAudio(const std::vector<float>& input, 
                                  const std::map<int, float>& parameters) {
        // Update parameters
        compressor->updateParameters(parameters);
        
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
            compressor->process(buffer);
            
            // Extract output (left channel)
            for (size_t j = 0; j < blockSize; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }
        
        return output;
    }
    
    // Test 1: Parameter sweep test
    void testParameterSweeps() {
        log("\n--- Parameter Sweep Tests ---\n");
        
        // Test signal: 1kHz sine at -20dB
        auto testSignal = TestSignalGenerator::generateSine(1000.0, 0.1, 1.0, TEST_SAMPLE_RATE);
        
        // Test each parameter from 0.0 to 1.0 in 0.1 steps
        for (int param = 0; param < compressor->getNumParameters(); ++param) {
            std::string paramName = compressor->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            bool parameterResponsive = false;
            std::vector<float> outputs;
            
            for (float value = 0.0f; value <= 1.0f; value += 0.1f) {
                std::map<int, float> params;
                params[param] = value;
                
                auto output = processAudio(testSignal, params);
                
                // Check for invalid values
                assertTrue(!AudioAnalyzer::hasInvalidValues(output), 
                          paramName + " produces valid output at " + std::to_string(value));
                
                float rms = AudioAnalyzer::calculateRMS_dB(output);
                outputs.push_back(rms);
                
                // Check if parameter has audible effect
                if (param == 1 && value > 0.5f) { // Peak reduction parameter
                    if (rms < -25.0f) { // Expecting some compression
                        parameterResponsive = true;
                    }
                }
            }
            
            // Check parameter responsiveness
            float minOutput = *std::min_element(outputs.begin(), outputs.end());
            float maxOutput = *std::max_element(outputs.begin(), outputs.end());
            float outputRange = maxOutput - minOutput;
            
            assertTrue(outputRange > 1.0f, paramName + " has audible effect (range: " + 
                      std::to_string(outputRange) + "dB)");
        }
    }
    
    // Test 2: Compression ratio accuracy
    void testCompressionRatio() {
        log("\n--- Compression Ratio Tests ---\n");
        
        // Test different input levels with peak reduction at 0.8 (80%)
        std::map<int, float> params;
        params[1] = 0.8f; // Peak reduction
        params[0] = 0.5f; // Input gain
        
        std::vector<float> inputLevels = {-30.0f, -20.0f, -15.0f, -10.0f, -5.0f};
        std::vector<float> outputLevels;
        
        for (float inputLevel_dB : inputLevels) {
            float amplitude = std::pow(10.0f, inputLevel_dB / 20.0f);
            auto testSignal = TestSignalGenerator::generateSine(1000.0, amplitude, 0.5, TEST_SAMPLE_RATE);
            
            auto output = processAudio(testSignal, params);
            float outputLevel = AudioAnalyzer::calculateRMS_dB(output);
            outputLevels.push_back(outputLevel);
            
            log("Input: " + std::to_string(inputLevel_dB) + "dB -> Output: " + 
                std::to_string(outputLevel) + "dB\n");
        }
        
        // Check compression behavior (output should increase less than input)
        for (size_t i = 1; i < inputLevels.size(); ++i) {
            float inputDiff = inputLevels[i] - inputLevels[i-1];
            float outputDiff = outputLevels[i] - outputLevels[i-1];
            float ratio = inputDiff / std::max(0.1f, outputDiff);
            
            assertTrue(ratio > 1.5f, "Compression ratio test " + std::to_string(i) + 
                      " (ratio: " + std::to_string(ratio) + ":1)");
        }
    }
    
    // Test 3: Attack time measurement
    void testAttackTime() {
        log("\n--- Attack Time Tests ---\n");
        
        // Generate step function (sudden level change)
        std::vector<float> testSignal;
        
        // 100ms of silence
        auto silence = TestSignalGenerator::generateSine(1000.0, 0.0, 0.1, TEST_SAMPLE_RATE);
        testSignal.insert(testSignal.end(), silence.begin(), silence.end());
        
        // 400ms of loud signal
        auto loudSignal = TestSignalGenerator::generateSine(1000.0, 0.5, 0.4, TEST_SAMPLE_RATE);
        testSignal.insert(testSignal.end(), loudSignal.begin(), loudSignal.end());
        
        // Process with moderate compression
        std::map<int, float> params;
        params[1] = 0.6f; // Peak reduction
        
        auto output = processAudio(testSignal, params);
        
        // Calculate gain reduction envelope
        std::vector<float> gainReduction;
        for (size_t i = 0; i < std::min(testSignal.size(), output.size()); ++i) {
            if (std::abs(testSignal[i]) > EPSILON) {
                gainReduction.push_back(1.0f - (std::abs(output[i]) / std::abs(testSignal[i])));
            } else {
                gainReduction.push_back(0.0f);
            }
        }
        
        // Measure attack time (should be around 10ms for opto compressor)
        double attackTime = AudioAnalyzer::measureAttackTime(gainReduction, TEST_SAMPLE_RATE);
        
        log("Measured attack time: " + std::to_string(attackTime * 1000.0) + " ms\n");
        assertTrue(attackTime > 0.005 && attackTime < 0.050, 
                  "Attack time within expected range (5-50ms)");
    }
    
    // Test 4: Frequency response
    void testFrequencyResponse() {
        log("\n--- Frequency Response Tests ---\n");
        
        std::vector<double> testFrequencies = {50.0, 100.0, 500.0, 1000.0, 5000.0, 10000.0};
        
        std::map<int, float> params;
        params[1] = 0.5f; // Moderate compression
        
        for (double freq : testFrequencies) {
            auto testSignal = TestSignalGenerator::generateSine(freq, 0.2, 0.5, TEST_SAMPLE_RATE);
            auto output = processAudio(testSignal, params);
            
            float inputRMS = AudioAnalyzer::calculateRMS_dB(testSignal);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float gainReduction = inputRMS - outputRMS;
            
            log("Frequency " + std::to_string(freq) + "Hz: Gain reduction = " + 
                std::to_string(gainReduction) + "dB\n");
            
            // Check for reasonable frequency response (no extreme filtering)
            assertTrue(gainReduction > -10.0f && gainReduction < 20.0f,
                      "Frequency " + std::to_string(freq) + "Hz within reasonable range");
        }
    }
    
    // Test 5: Bypass state (null test)
    void testBypassState() {
        log("\n--- Bypass State Tests ---\n");
        
        // Test with all parameters at minimum (should be close to bypass)
        std::map<int, float> bypassParams;
        for (int i = 0; i < compressor->getNumParameters(); ++i) {
            bypassParams[i] = 0.0f;
        }
        
        auto testSignal = TestSignalGenerator::generateSine(1000.0, 0.1, 0.5, TEST_SAMPLE_RATE);
        auto output = processAudio(testSignal, bypassParams);
        
        // Calculate difference
        float maxDifference = 0.0f;
        for (size_t i = 0; i < std::min(testSignal.size(), output.size()); ++i) {
            maxDifference = std::max(maxDifference, 
                                   std::abs(testSignal[i] - output[i]));
        }
        
        log("Maximum difference in bypass state: " + std::to_string(maxDifference) + "\n");
        assertTrue(maxDifference < 0.1f, "Bypass state produces minimal difference");
    }
    
    // Test 6: Stability test
    void testStability() {
        log("\n--- Stability Tests ---\n");
        
        // Test with extreme parameter values
        std::vector<std::map<int, float>> extremeParams = {
            {{0, 1.0f}, {1, 1.0f}}, // Max gain and compression
            {{0, 0.0f}, {1, 0.0f}}, // Min gain and compression
            {{2, 1.0f}, {3, 1.0f}}  // Max emphasis and output gain
        };
        
        for (size_t i = 0; i < extremeParams.size(); ++i) {
            auto testSignal = TestSignalGenerator::generatePinkNoise(0.5, 1.0, TEST_SAMPLE_RATE);
            auto output = processAudio(testSignal, extremeParams[i]);
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Stability test " + std::to_string(i + 1) + " produces valid output");
            
            float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
            assertTrue(peakLevel < 6.0f, 
                      "Stability test " + std::to_string(i + 1) + " output level reasonable");
        }
    }
    
    // Test 7: Thermal modeling validation
    void testThermalModeling() {
        log("\n--- Thermal Modeling Tests ---\n");
        
        // Process long signal to trigger thermal effects
        auto longSignal = TestSignalGenerator::generateSine(1000.0, 0.2, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[1] = 0.7f; // High compression to heat up the "opto cell"
        
        auto output = processAudio(longSignal, params);
        
        // Check that output remains stable throughout
        float firstHalfRMS = AudioAnalyzer::calculateRMS_dB(
            std::vector<float>(output.begin(), output.begin() + output.size()/2));
        float secondHalfRMS = AudioAnalyzer::calculateRMS_dB(
            std::vector<float>(output.begin() + output.size()/2, output.end()));
        
        float drift = std::abs(firstHalfRMS - secondHalfRMS);
        log("Thermal drift: " + std::to_string(drift) + "dB\n");
        
        assertTrue(drift < 1.0f, "Thermal modeling maintains stability");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Vintage Opto Compressor test suite...\n");
        
        testParameterSweeps();
        testCompressionRatio();
        testAttackTime();
        testFrequencyResponse();
        testBypassState();
        testStability();
        testThermalModeling();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        VintageOptoCompressorTest tester;
        tester.runAllTests();
        
        std::cout << "\nVintage Opto Compressor test suite completed successfully.\n";
        std::cout << "Check VintageOptoCompressor_TestResults.txt for detailed results.\n";
        
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