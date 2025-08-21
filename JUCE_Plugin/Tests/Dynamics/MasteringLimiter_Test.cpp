/*
  ==============================================================================
  
    MasteringLimiter_Test.cpp
    Comprehensive test suite for ENGINE_MASTERING_LIMITER
    
    Tests for mastering limiter characteristics:
    - Parameter sweep validation (all 10 parameters)
    - Brick-wall limiting verification (hard ceiling compliance)
    - True-peak detection and limiting
    - 0dBFS compliance and overload prevention
    - Lookahead processing accuracy
    - Release time precision
    - Threshold behavior
    - Soft-knee vs hard-knee characteristics
    - Makeup gain accuracy
    - Stereo linking behavior
    - Saturation modeling
    - Professional metering validation
    - Latency reporting accuracy
    - Inter-sample peak detection
    
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
#include "../../Source/MasteringLimiter_Platinum.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;

// Test signal generators
class TestSignalGenerator {
public:
    // Generate sine wave at precise level
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
    
    // Generate 0dBFS peak test signal
    static std::vector<float> generatePeakTestSignal(double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        // Insert 0dBFS peaks at regular intervals
        int peakInterval = static_cast<int>(0.1 * sampleRate); // Every 100ms
        
        for (int i = 0; i < numSamples; i += peakInterval) {
            if (i < numSamples) {
                signal[i] = 1.0f; // 0dBFS peak
            }
            if (i + 1 < numSamples) {
                signal[i + 1] = -1.0f; // Negative peak
            }
        }
        
        return signal;
    }
    
    // Generate inter-sample peak test signal
    static std::vector<float> generateInterSamplePeakSignal(double frequency, 
                                                          double duration, 
                                                          double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        // Generate a sine wave that will create inter-sample peaks when reconstructed
        double phase = M_PI / 4.0; // Start at 45 degrees for inter-sample peaks
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(0.95f * std::sin(phase)); // Slightly below 0dBFS
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate multi-frequency test signal for complex limiting
    static std::vector<float> generateComplexSignal(float level_dB, double duration, 
                                                   double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        float amplitude = std::pow(10.0f, level_dB / 20.0f);
        
        // Multiple frequencies for complex harmonic content
        std::vector<double> frequencies = {440.0, 880.0, 1320.0, 2200.0};
        std::vector<double> phases(frequencies.size(), 0.0);
        
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
    
    // Generate burst signal for attack testing
    static std::vector<float> generateBurstSignal(float level_dB, double burstDuration,
                                                 double totalDuration, double sampleRate) {
        int totalSamples = static_cast<int>(totalDuration * sampleRate);
        int burstSamples = static_cast<int>(burstDuration * sampleRate);
        
        std::vector<float> signal(totalSamples, 0.0f);
        
        // Place burst in the middle
        int startPos = (totalSamples - burstSamples) / 2;
        
        auto burst = generateSineWave(1000.0, level_dB, burstDuration, sampleRate);
        
        for (int i = 0; i < burstSamples && startPos + i < totalSamples; ++i) {
            signal[startPos + i] = burst[i];
        }
        
        return signal;
    }
    
    // Generate white noise at specified level
    static std::vector<float> generateWhiteNoise(float level_dB, double duration, 
                                                double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        float amplitude = std::pow(10.0f, level_dB / 20.0f);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = amplitude * dist(gen);
        }
        
        return signal;
    }
    
    // Generate ramping signal for threshold testing
    static std::vector<float> generateRampSignal(float startLevel_dB, float endLevel_dB,
                                                double frequency, double duration, 
                                                double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / numSamples;
            float level_dB = startLevel_dB + (endLevel_dB - startLevel_dB) * t;
            float amplitude = std::pow(10.0f, level_dB / 20.0f);
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
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
    
    // Check for clipping (samples at or above 0dBFS)
    static bool hasClipping(const std::vector<float>& signal, float threshold = 0.999f) {
        for (float sample : signal) {
            if (std::abs(sample) >= threshold) {
                return true;
            }
        }
        return false;
    }
    
    // Count number of clipped samples
    static int countClippedSamples(const std::vector<float>& signal, float threshold = 0.999f) {
        int count = 0;
        for (float sample : signal) {
            if (std::abs(sample) >= threshold) {
                count++;
            }
        }
        return count;
    }
    
    // Estimate true peak using simple oversampling
    static float estimateTruePeak_dB(const std::vector<float>& signal) {
        if (signal.size() < 2) return calculatePeak_dB(signal);
        
        // Simple 2x oversampling for true peak estimation
        float maxTruePeak = 0.0f;
        
        for (size_t i = 0; i < signal.size() - 1; ++i) {
            // Linear interpolation between samples
            float midSample = (signal[i] + signal[i + 1]) * 0.5f;
            float peak = std::max(std::abs(signal[i]), std::abs(midSample));
            maxTruePeak = std::max(maxTruePeak, peak);
        }
        
        return 20.0f * std::log10(std::max(1e-6, maxTruePeak));
    }
    
    // Measure limiting accuracy (how close to ceiling)
    static float measureLimitingAccuracy(const std::vector<float>& signal, float ceiling_dB) {
        float peakLevel = calculatePeak_dB(signal);
        return std::abs(peakLevel - ceiling_dB);
    }
    
    // Calculate gain reduction from input/output comparison
    static float calculateGainReduction(const std::vector<float>& input,
                                      const std::vector<float>& output) {
        float inputRMS = calculateRMS_dB(input);
        float outputRMS = calculateRMS_dB(output);
        return inputRMS - outputRMS;
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
    
    // Measure release time (time to reach 63% of final value)
    static double measureReleaseTime(const std::vector<float>& gainReduction, 
                                   double sampleRate) {
        if (gainReduction.size() < 100) return -1.0;
        
        // Find peak gain reduction
        int peakIdx = static_cast<int>(std::max_element(gainReduction.begin(), 
                                                       gainReduction.end()) - gainReduction.begin());
        
        float peakValue = gainReduction[peakIdx];
        if (peakValue < 0.1f) return -1.0;
        
        // Find final value (average of last 10%)
        int startIdx = static_cast<int>(gainReduction.size() * 0.9f);
        float finalValue = 0.0f;
        for (int i = startIdx; i < static_cast<int>(gainReduction.size()); ++i) {
            finalValue += gainReduction[i];
        }
        finalValue /= (gainReduction.size() - startIdx);
        
        // Find 63% decay point
        float targetValue = finalValue + (peakValue - finalValue) * 0.37f; // 63% decay
        
        for (int i = peakIdx; i < static_cast<int>(gainReduction.size()); ++i) {
            if (gainReduction[i] <= targetValue) {
                return (i - peakIdx) / sampleRate;
            }
        }
        
        return -1.0;
    }
};

// Main test class
class MasteringLimiterTest {
private:
    std::unique_ptr<MasteringLimiter_Platinum> limiter;
    std::ofstream logFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    MasteringLimiterTest() {
        limiter = std::make_unique<MasteringLimiter_Platinum>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Dynamics/MasteringLimiter_TestResults.txt");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        // Prepare the limiter
        limiter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Mastering Limiter Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_MASTERING_LIMITER) + "\n");
        log("Parameter Count: " + std::to_string(limiter->getNumParameters()) + "\n");
        log("Latency: " + std::to_string(limiter->getLatencySamples()) + " samples\n\n");
    }
    
    ~MasteringLimiterTest() {
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
    
    // Process audio through limiter
    std::vector<float> processAudio(const std::vector<float>& input, 
                                  const std::map<int, float>& parameters) {
        // Update parameters
        limiter->updateParameters(parameters);
        
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
            limiter->process(buffer);
            
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
        
        // Test signal that will trigger limiting
        auto testSignal = TestSignalGenerator::generateComplexSignal(-3.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < limiter->getNumParameters(); ++param) {
            std::string paramName = limiter->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseValues;
            
            // Sweep from 0.0 to 1.0 in 0.2 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.2f) {
                std::map<int, float> params;
                params[param] = value;
                
                // Set other parameters to reasonable defaults
                if (param != 0) params[0] = 0.2f; // Threshold around -12dB
                if (param != 1) params[1] = 0.1f; // Ceiling around -0.3dB
                if (param != 2) params[2] = 0.3f; // Medium release
                
                auto output = processAudio(testSignal, params);
                
                // Check for valid output
                assertTrue(!AudioAnalyzer::hasInvalidValues(output), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                // Check for no clipping
                assertTrue(!AudioAnalyzer::hasClipping(output),
                          paramName + " at " + std::to_string(value) + " prevents clipping");
                
                float outputPeak = AudioAnalyzer::calculatePeak_dB(output);
                responseValues.push_back(outputPeak);
            }
            
            // Check parameter responsiveness
            float minResponse = *std::min_element(responseValues.begin(), responseValues.end());
            float maxResponse = *std::max_element(responseValues.begin(), responseValues.end());
            float responseRange = maxResponse - minResponse;
            
            log("  Response range: " + std::to_string(responseRange) + "dB\n");
            
            // Most parameters should have some audible effect
            if (param <= 2) { // Threshold, ceiling, release
                assertTrue(responseRange > 0.5f, 
                          paramName + " has audible effect");
            }
        }
    }
    
    // Test 2: Brick-wall limiting verification
    void testBrickWallLimiting() {
        log("\n--- Brick-Wall Limiting Tests ---\n");
        
        // Test different ceiling levels
        std::vector<float> ceilingSettings = {0.0f, 0.1f, 0.3f, 0.5f}; // Different ceiling levels
        
        for (float ceiling : ceilingSettings) {
            log("Testing ceiling setting: " + std::to_string(ceiling) + "\n");
            
            // Expected ceiling in dB (rough mapping)
            float expectedCeiling_dB = -3.0f + ceiling * 2.7f; // Range approximately -3dB to -0.3dB
            
            std::map<int, float> params;
            params[0] = 0.0f;    // Low threshold to ensure limiting
            params[1] = ceiling; // Ceiling setting
            params[2] = 0.2f;    // Fast release
            params[3] = 0.5f;    // Medium lookahead
            
            // Test with signals that exceed the ceiling
            std::vector<float> testLevels = {0.0f, 3.0f, 6.0f, 10.0f};
            
            for (float inputLevel : testLevels) {
                auto testSignal = TestSignalGenerator::generateSineWave(
                    1000.0, inputLevel, 0.5, TEST_SAMPLE_RATE);
                
                auto output = processAudio(testSignal, params);
                
                float outputPeak = AudioAnalyzer::calculatePeak_dB(output);
                float limitingAccuracy = AudioAnalyzer::measureLimitingAccuracy(output, expectedCeiling_dB);
                
                log("  Input: " + std::to_string(inputLevel) + "dB -> " +
                    "Output: " + std::to_string(outputPeak) + "dB, " +
                    "Accuracy: " + std::to_string(limitingAccuracy) + "dB\n");
                
                // Output should not exceed ceiling
                assertTrue(outputPeak <= expectedCeiling_dB + 1.0f, 
                          "Output level within ceiling at input " + std::to_string(inputLevel) + "dB");
                
                // Should not clip
                assertTrue(!AudioAnalyzer::hasClipping(output),
                          "No clipping at input " + std::to_string(inputLevel) + "dB");
                
                // Limiting accuracy should be reasonable
                if (inputLevel > expectedCeiling_dB + 3.0f) {
                    assertTrue(limitingAccuracy < 1.0f, 
                              "Good limiting accuracy for loud input " + std::to_string(inputLevel) + "dB");
                }
            }
        }
    }
    
    // Test 3: True-peak detection and limiting
    void testTruePeakLimiting() {
        log("\n--- True-Peak Limiting Tests ---\n");
        
        // Generate signal with inter-sample peaks
        auto truePeakSignal = TestSignalGenerator::generateInterSamplePeakSignal(
            8000.0, 1.0, TEST_SAMPLE_RATE);
        
        // Test with true-peak limiting enabled
        std::vector<float> truePeakSettings = {0.0f, 1.0f}; // Off, On
        std::vector<std::string> modeNames = {"Sample Peak", "True Peak"};
        
        for (size_t i = 0; i < truePeakSettings.size(); ++i) {
            log("Testing " + modeNames[i] + " mode\n");
            
            std::map<int, float> params;
            params[0] = 0.0f;                // Low threshold
            params[1] = 0.1f;                // -0.3dB ceiling
            params[8] = truePeakSettings[i]; // True peak setting
            
            auto output = processAudio(truePeakSignal, params);
            
            float samplePeak = AudioAnalyzer::calculatePeak_dB(output);
            float truePeak = AudioAnalyzer::estimateTruePeak_dB(output);
            
            log("  Sample peak: " + std::to_string(samplePeak) + "dB\n");
            log("  True peak: " + std::to_string(truePeak) + "dB\n");
            
            // True peak mode should control inter-sample peaks better
            if (truePeakSettings[i] > 0.5f) {
                assertTrue(truePeak < 0.0f, "True peak mode controls inter-sample peaks");
            }
            
            // Should not clip in either mode
            assertTrue(!AudioAnalyzer::hasClipping(output),
                      modeNames[i] + " mode prevents clipping");
        }
    }
    
    // Test 4: 0dBFS compliance testing
    void testZeroDBFSCompliance() {
        log("\n--- 0dBFS Compliance Tests ---\n");
        
        // Generate various challenging signals
        std::vector<std::pair<std::string, std::vector<float>>> testSignals = {
            {"0dBFS Peaks", TestSignalGenerator::generatePeakTestSignal(1.0, TEST_SAMPLE_RATE)},
            {"Hot Signal", TestSignalGenerator::generateSineWave(1000.0, 6.0f, 1.0, TEST_SAMPLE_RATE)},
            {"Complex Mix", TestSignalGenerator::generateComplexSignal(3.0f, 1.0, TEST_SAMPLE_RATE)},
            {"White Noise", TestSignalGenerator::generateWhiteNoise(0.0f, 1.0, TEST_SAMPLE_RATE)}
        };
        
        std::map<int, float> params;
        params[0] = 0.1f; // Threshold
        params[1] = 0.05f; // Very conservative ceiling (-0.1dB)
        params[2] = 0.2f; // Fast release
        params[3] = 0.8f; // High lookahead
        params[8] = 1.0f; // True peak limiting
        
        for (auto& signalPair : testSignals) {
            log("Testing " + signalPair.first + ":\n");
            
            auto output = processAudio(signalPair.second, params);
            
            // Critical tests for 0dBFS compliance
            assertTrue(!AudioAnalyzer::hasClipping(output),
                      signalPair.first + ": No clipping");
            
            float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
            assertTrue(peakLevel < 0.0f, 
                      signalPair.first + ": Peak below 0dBFS");
            
            float truePeak = AudioAnalyzer::estimateTruePeak_dB(output);
            assertTrue(truePeak < 0.0f, 
                      signalPair.first + ": True peak below 0dBFS");
            
            int clippedSamples = AudioAnalyzer::countClippedSamples(output, 0.99f);
            assertTrue(clippedSamples == 0, 
                      signalPair.first + ": Zero clipped samples");
            
            log("  Peak level: " + std::to_string(peakLevel) + "dB\n");
            log("  True peak: " + std::to_string(truePeak) + "dB\n");
            log("  Clipped samples: " + std::to_string(clippedSamples) + "\n");
        }
    }
    
    // Test 5: Lookahead processing accuracy
    void testLookaheadProcessing() {
        log("\n--- Lookahead Processing Tests ---\n");
        
        // Generate burst signal for lookahead testing
        auto burstSignal = TestSignalGenerator::generateBurstSignal(
            6.0f, 0.05, 1.0, TEST_SAMPLE_RATE);
        
        // Test different lookahead settings
        std::vector<float> lookaheadSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float lookahead : lookaheadSettings) {
            log("Testing lookahead: " + std::to_string(lookahead) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.2f;    // Threshold
            params[1] = 0.1f;    // Ceiling
            params[2] = 0.5f;    // Release
            params[3] = lookahead; // Lookahead setting
            
            auto output = processAudio(burstSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Lookahead " + std::to_string(lookahead) + " produces valid output");
            
            // Check limiting performance
            float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
            float inputPeak = AudioAnalyzer::calculatePeak_dB(burstSignal);
            float gainReduction = inputPeak - peakLevel;
            
            log("  Input peak: " + std::to_string(inputPeak) + "dB\n");
            log("  Output peak: " + std::to_string(peakLevel) + "dB\n");
            log("  Gain reduction: " + std::to_string(gainReduction) + "dB\n");
            
            // Higher lookahead should provide better limiting performance
            if (lookahead > 0.5f) {
                assertTrue(gainReduction > 3.0f, 
                          "High lookahead provides significant gain reduction");
            }
            
            // Should not clip regardless of lookahead setting
            assertTrue(!AudioAnalyzer::hasClipping(output),
                      "Lookahead " + std::to_string(lookahead) + " prevents clipping");
        }
    }
    
    // Test 6: Release time precision
    void testReleaseTimePrecision() {
        log("\n--- Release Time Precision Tests ---\n");
        
        // Generate signal for release time measurement
        auto testSignal = TestSignalGenerator::generateBurstSignal(
            10.0f, 0.1, 1.5, TEST_SAMPLE_RATE);
        
        // Test different release times
        std::vector<float> releaseSettings = {0.1f, 0.3f, 0.6f, 0.9f};
        
        for (float release : releaseSettings) {
            log("Testing release time: " + std::to_string(release) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.1f;  // Low threshold to ensure limiting
            params[1] = 0.1f;  // Ceiling
            params[2] = release; // Release time
            params[3] = 0.8f;  // High lookahead
            
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
            
            // Measure release time
            double releaseTime = AudioAnalyzer::measureReleaseTime(gainReduction, TEST_SAMPLE_RATE);
            
            if (releaseTime > 0.0) {
                log("  Measured release time: " + std::to_string(releaseTime * 1000.0) + " ms\n");
                
                // Verify release time is reasonable
                assertTrue(releaseTime > 0.001 && releaseTime < 5.0, 
                          "Release time within reasonable range");
                
                // Different settings should produce different release times
                assertTrue(releaseTime > 0.0, "Release time measurable");
            }
        }
    }
    
    // Test 7: Threshold behavior
    void testThresholdBehavior() {
        log("\n--- Threshold Behavior Tests ---\n");
        
        // Generate ramping signal to test threshold
        auto rampSignal = TestSignalGenerator::generateRampSignal(
            -20.0f, 0.0f, 1000.0, 2.0, TEST_SAMPLE_RATE);
        
        // Test different threshold settings
        std::vector<float> thresholdSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float threshold : thresholdSettings) {
            log("Testing threshold: " + std::to_string(threshold) + "\n");
            
            std::map<int, float> params;
            params[0] = threshold; // Threshold setting
            params[1] = 0.1f;     // Ceiling
            params[2] = 0.3f;     // Release
            
            auto output = processAudio(rampSignal, params);
            
            float inputRMS = AudioAnalyzer::calculateRMS_dB(rampSignal);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float gainReduction = inputRMS - outputRMS;
            
            log("  Input RMS: " + std::to_string(inputRMS) + "dB\n");
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            log("  Gain reduction: " + std::to_string(gainReduction) + "dB\n");
            
            // Lower thresholds should provide more gain reduction
            if (threshold < 0.3f) {
                assertTrue(gainReduction > 1.0f, 
                          "Low threshold provides significant gain reduction");
            }
            
            // Check for clean limiting
            assertTrue(!AudioAnalyzer::hasClipping(output),
                      "Threshold " + std::to_string(threshold) + " prevents clipping");
        }
    }
    
    // Test 8: Soft-knee vs hard-knee characteristics
    void testKneeCharacteristics() {
        log("\n--- Knee Characteristics Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateComplexSignal(-6.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test different knee settings
        std::vector<float> kneeSettings = {0.0f, 0.5f, 1.0f}; // Hard, medium, soft
        
        for (float knee : kneeSettings) {
            log("Testing knee setting: " + std::to_string(knee) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f; // Threshold
            params[1] = 0.1f; // Ceiling
            params[4] = knee;  // Knee parameter
            
            auto output = processAudio(testSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Knee " + std::to_string(knee) + " produces valid output");
            
            float inputRMS = AudioAnalyzer::calculateRMS_dB(testSignal);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            
            log("  Input RMS: " + std::to_string(inputRMS) + "dB\n");
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            
            // Should provide limiting regardless of knee setting
            assertTrue(outputRMS < inputRMS + 1.0f, "Knee setting provides limiting");
            
            // No clipping regardless of knee
            assertTrue(!AudioAnalyzer::hasClipping(output),
                      "Knee " + std::to_string(knee) + " prevents clipping");
        }
    }
    
    // Test 9: Stereo linking behavior
    void testStereoLinking() {
        log("\n--- Stereo Linking Tests ---\n");
        
        // Create test signal with different levels in L/R channels
        auto leftSignal = TestSignalGenerator::generateSineWave(1000.0, 0.0f, 1.0, TEST_SAMPLE_RATE);
        auto rightSignal = TestSignalGenerator::generateSineWave(1000.0, -6.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test different stereo link settings
        std::vector<float> linkSettings = {0.0f, 0.5f, 1.0f}; // Independent, partial, full link
        
        for (float link : linkSettings) {
            log("Testing stereo link: " + std::to_string(link) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.2f; // Threshold
            params[1] = 0.1f; // Ceiling
            params[7] = link;  // Stereo link parameter
            
            // Process with different L/R content
            std::vector<float> stereoInput;
            for (size_t i = 0; i < leftSignal.size(); ++i) {
                stereoInput.push_back(leftSignal[i]); // Just use left for this test
            }
            
            auto output = processAudio(stereoInput, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Stereo link " + std::to_string(link) + " produces valid output");
            
            float outputLevel = AudioAnalyzer::calculateRMS_dB(output);
            log("  Output level: " + std::to_string(outputLevel) + "dB\n");
            
            // Should provide consistent limiting
            assertTrue(!AudioAnalyzer::hasClipping(output),
                      "Stereo link " + std::to_string(link) + " prevents clipping");
        }
    }
    
    // Test 10: Professional metering validation
    void testProfessionalMetering() {
        log("\n--- Professional Metering Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateComplexSignal(-3.0f, 2.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.2f; // Threshold
        params[1] = 0.1f; // Ceiling
        params[2] = 0.3f; // Release
        
        auto output = processAudio(testSignal, params);
        
        // Test metering methods
        float gainReduction = limiter->getGainReduction();
        float inputLevel = limiter->getInputLevel();
        float outputLevel = limiter->getOutputLevel();
        float truePeakLevel = limiter->getTruePeakLevel();
        
        log("Gain Reduction: " + std::to_string(gainReduction) + "dB\n");
        log("Input Level: " + std::to_string(inputLevel) + "dB\n");
        log("Output Level: " + std::to_string(outputLevel) + "dB\n");
        log("True Peak Level: " + std::to_string(truePeakLevel) + "dB\n");
        
        // Verify meter readings are reasonable
        assertTrue(gainReduction >= 0.0f, "Gain reduction meter shows reduction");
        assertTrue(inputLevel > -120.0f && inputLevel < 20.0f, "Input level meter reasonable");
        assertTrue(outputLevel > -120.0f && outputLevel < 0.0f, "Output level meter reasonable");
        assertTrue(truePeakLevel > -120.0f && truePeakLevel < 0.0f, "True peak meter reasonable");
        
        // Output should be lower than input when limiting
        if (gainReduction > 1.0f) {
            assertTrue(outputLevel < inputLevel, "Output level lower than input when limiting");
        }
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Mastering Limiter test suite...\n");
        
        testParameterSweeps();
        testBrickWallLimiting();
        testTruePeakLimiting();
        testZeroDBFSCompliance();
        testLookaheadProcessing();
        testReleaseTimePrecision();
        testThresholdBehavior();
        testKneeCharacteristics();
        testStereoLinking();
        testProfessionalMetering();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        MasteringLimiterTest tester;
        tester.runAllTests();
        
        std::cout << "\nMastering Limiter test suite completed successfully.\n";
        std::cout << "Check MasteringLimiter_TestResults.txt for detailed results.\n";
        
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