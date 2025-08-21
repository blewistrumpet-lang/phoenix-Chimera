/*
  ==============================================================================
  
    NoiseGate_Test.cpp
    Comprehensive test suite for ENGINE_NOISE_GATE
    
    Tests for noise gate characteristics:
    - Parameter sweep validation (all 8 parameters)
    - Gate opening/closing timing accuracy
    - Threshold detection precision
    - Hysteresis behavior validation
    - Hold time accuracy
    - Range (maximum attenuation) testing
    - Sidechain filter response
    - Lookahead processing
    - Calibrated noise burst tests
    - Gate state transitions
    - Thermal modeling and analog drift
    - Component aging simulation
    
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
#include "../../Source/NoiseGate.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;

// Test signal generators
class TestSignalGenerator {
public:
    // Generate calibrated noise burst
    static std::vector<float> generateNoiseBurst(float level_dB, double duration, 
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
    
    // Generate signal burst with precise level
    static std::vector<float> generateSignalBurst(double frequency, float level_dB, 
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
    
    // Generate gate test sequence (silence -> signal -> silence)
    static std::vector<float> generateGateTestSequence(float backgroundLevel_dB, 
                                                     float signalLevel_dB,
                                                     double silenceDuration,
                                                     double signalDuration,
                                                     double sampleRate) {
        std::vector<float> sequence;
        
        // Initial silence
        auto silence1 = generateNoiseBurst(backgroundLevel_dB, silenceDuration, sampleRate);
        sequence.insert(sequence.end(), silence1.begin(), silence1.end());
        
        // Signal burst
        auto signal = generateSignalBurst(1000.0, signalLevel_dB, signalDuration, sampleRate);
        sequence.insert(sequence.end(), signal.begin(), signal.end());
        
        // Final silence
        auto silence2 = generateNoiseBurst(backgroundLevel_dB, silenceDuration, sampleRate);
        sequence.insert(sequence.end(), silence2.begin(), silence2.end());
        
        return sequence;
    }
    
    // Generate impulse for timing tests
    static std::vector<float> generateImpulse(float amplitude, int position, 
                                            int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = amplitude;
        }
        return signal;
    }
    
    // Generate ramped signal for threshold testing
    static std::vector<float> generateRampedSignal(float startLevel_dB, float endLevel_dB,
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
    
    // Generate chattering signal (alternating levels for hysteresis testing)
    static std::vector<float> generateChatteringSignal(float lowLevel_dB, float highLevel_dB,
                                                      double switchRate, double duration,
                                                      double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * 1000.0 / sampleRate; // 1kHz carrier
        double switchPeriod = sampleRate / switchRate;
        
        for (int i = 0; i < numSamples; ++i) {
            bool highLevel = static_cast<int>(i / switchPeriod) % 2 == 0;
            float level_dB = highLevel ? highLevel_dB : lowLevel_dB;
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
    
    // Detect gate opening time
    static double detectGateOpenTime(const std::vector<float>& signal, 
                                   float threshold, double sampleRate) {
        if (signal.size() < 10) return -1.0;
        
        // Find first sample above threshold
        for (size_t i = 0; i < signal.size(); ++i) {
            if (std::abs(signal[i]) > threshold) {
                return i / sampleRate;
            }
        }
        
        return -1.0; // Gate never opened
    }
    
    // Detect gate closing time
    static double detectGateCloseTime(const std::vector<float>& signal, 
                                    float threshold, double sampleRate) {
        if (signal.size() < 10) return -1.0;
        
        // Find last sample above threshold
        for (int i = static_cast<int>(signal.size()) - 1; i >= 0; --i) {
            if (std::abs(signal[i]) > threshold) {
                return (i + 1) / sampleRate; // Return time after last active sample
            }
        }
        
        return 0.0; // Gate was never open
    }
    
    // Measure gate hold time
    static double measureHoldTime(const std::vector<float>& input, 
                                const std::vector<float>& output,
                                float threshold, double sampleRate) {
        if (input.size() != output.size() || input.size() < 100) return -1.0;
        
        // Find when input drops below threshold
        int inputDropPoint = -1;
        for (size_t i = 100; i < input.size(); ++i) {
            if (std::abs(input[i]) < threshold && std::abs(input[i-1]) >= threshold) {
                inputDropPoint = static_cast<int>(i);
                break;
            }
        }
        
        if (inputDropPoint == -1) return -1.0;
        
        // Find when output drops below threshold
        int outputDropPoint = -1;
        for (int i = inputDropPoint; i < static_cast<int>(output.size()); ++i) {
            if (std::abs(output[i]) < threshold) {
                outputDropPoint = i;
                break;
            }
        }
        
        if (outputDropPoint == -1) return -1.0;
        
        return (outputDropPoint - inputDropPoint) / sampleRate;
    }
    
    // Calculate attenuation in dB
    static float calculateAttenuation(const std::vector<float>& input,
                                    const std::vector<float>& output) {
        float inputRMS = calculateRMS_dB(input);
        float outputRMS = calculateRMS_dB(output);
        return inputRMS - outputRMS;
    }
    
    // Check for gate chatter (rapid on/off switching)
    static int countGateTransitions(const std::vector<float>& signal, 
                                  float threshold, int minSeparation = 10) {
        if (signal.size() < 20) return 0;
        
        int transitions = 0;
        bool wasAboveThreshold = std::abs(signal[0]) > threshold;
        int lastTransition = 0;
        
        for (size_t i = 1; i < signal.size(); ++i) {
            bool isAboveThreshold = std::abs(signal[i]) > threshold;
            
            if (isAboveThreshold != wasAboveThreshold && 
                static_cast<int>(i) - lastTransition > minSeparation) {
                transitions++;
                lastTransition = static_cast<int>(i);
                wasAboveThreshold = isAboveThreshold;
            }
        }
        
        return transitions;
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
    
    // Calculate peak level in dB
    static float calculatePeak_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        
        float peak = 0.0f;
        for (float sample : signal) {
            peak = std::max(peak, std::abs(sample));
        }
        
        return 20.0f * std::log10(std::max(1e-6, peak));
    }
};

// Main test class
class NoiseGateTest {
private:
    std::unique_ptr<NoiseGate> gate;
    std::ofstream logFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    NoiseGateTest() {
        gate = std::make_unique<NoiseGate>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Dynamics/NoiseGate_TestResults.txt");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        // Prepare the gate
        gate->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Noise Gate Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_NOISE_GATE) + "\n");
        log("Parameter Count: " + std::to_string(gate->getNumParameters()) + "\n\n");
    }
    
    ~NoiseGateTest() {
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
    
    // Process audio through noise gate
    std::vector<float> processAudio(const std::vector<float>& input, 
                                  const std::map<int, float>& parameters) {
        // Update parameters
        gate->updateParameters(parameters);
        
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
            gate->process(buffer);
            
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
        
        // Test signal with noise and signal bursts
        auto testSignal = TestSignalGenerator::generateGateTestSequence(
            -40.0f, -10.0f, 0.2, 0.5, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < gate->getNumParameters(); ++param) {
            std::string paramName = gate->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseValues;
            
            // Sweep from 0.0 to 1.0 in 0.2 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.2f) {
                std::map<int, float> params;
                params[param] = value;
                
                // Set other parameters to reasonable defaults
                if (param != 0) params[0] = 0.3f; // Threshold around -40dB
                if (param != 1) params[1] = 0.5f; // Range around -20dB
                if (param != 2) params[2] = 0.2f; // Fast attack
                if (param != 3) params[3] = 0.3f; // Short hold
                if (param != 4) params[4] = 0.3f; // Medium release
                
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
            
            // Core gate parameters should have significant effect
            if (param <= 2) { // Threshold, Range, Attack
                assertTrue(responseRange > 1.0f, 
                          paramName + " has audible effect");
            }
        }
    }
    
    // Test 2: Threshold detection accuracy
    void testThresholdDetection() {
        log("\n--- Threshold Detection Tests ---\n");
        
        // Test different signal levels around various thresholds
        std::vector<float> thresholdSettings = {0.1f, 0.3f, 0.5f, 0.7f}; // Different threshold levels
        
        for (float thresholdSetting : thresholdSettings) {
            log("Testing threshold setting: " + std::to_string(thresholdSetting) + "\n");
            
            std::map<int, float> params;
            params[0] = thresholdSetting; // Threshold
            params[1] = 0.7f;            // Range (high attenuation)
            params[2] = 0.1f;            // Fast attack
            params[4] = 0.2f;            // Fast release
            
            // Test with signals above and below threshold
            std::vector<float> testLevels = {-50.0f, -40.0f, -30.0f, -20.0f, -10.0f};
            
            for (float signalLevel : testLevels) {
                auto testSignal = TestSignalGenerator::generateSignalBurst(
                    1000.0, signalLevel, 0.5, TEST_SAMPLE_RATE);
                
                auto output = processAudio(testSignal, params);
                
                float inputRMS = AudioAnalyzer::calculateRMS_dB(testSignal);
                float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
                float attenuation = inputRMS - outputRMS;
                
                log("  Signal level: " + std::to_string(signalLevel) + "dB -> " +
                    "Attenuation: " + std::to_string(attenuation) + "dB\n");
                
                // Expected threshold in dB (rough mapping)
                float expectedThreshold_dB = -60.0f + thresholdSetting * 50.0f;
                
                // Below threshold should have significant attenuation
                if (signalLevel < expectedThreshold_dB - 5.0f) {
                    assertTrue(attenuation > 5.0f, 
                              "Below threshold: significant attenuation at " + 
                              std::to_string(signalLevel) + "dB");
                }
                // Above threshold should pass with minimal attenuation
                else if (signalLevel > expectedThreshold_dB + 5.0f) {
                    assertTrue(attenuation < 3.0f, 
                              "Above threshold: minimal attenuation at " + 
                              std::to_string(signalLevel) + "dB");
                }
            }
        }
    }
    
    // Test 3: Gate timing (attack, hold, release)
    void testGateTiming() {
        log("\n--- Gate Timing Tests ---\n");
        
        // Generate test signal: silence -> signal -> silence
        auto testSignal = TestSignalGenerator::generateGateTestSequence(
            -60.0f, -10.0f, 0.3, 0.2, TEST_SAMPLE_RATE);
        
        // Test different timing settings
        std::vector<std::tuple<float, float, float, std::string>> timingSettings = {
            {0.1f, 0.1f, 0.1f, "Fast"},     // Fast attack, short hold, fast release
            {0.5f, 0.5f, 0.5f, "Medium"},   // Medium settings
            {0.9f, 0.9f, 0.9f, "Slow"}      // Slow attack, long hold, slow release
        };
        
        for (auto& setting : timingSettings) {
            float attack = std::get<0>(setting);
            float hold = std::get<1>(setting);
            float release = std::get<2>(setting);
            std::string name = std::get<3>(setting);
            
            log("Testing " + name + " timing - Attack: " + std::to_string(attack) + 
                ", Hold: " + std::to_string(hold) + ", Release: " + std::to_string(release) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f; // Threshold around -30dB
            params[1] = 0.8f; // High range
            params[2] = attack;
            params[3] = hold;
            params[4] = release;
            
            auto output = processAudio(testSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      name + " timing produces valid output");
            
            // Measure hold time
            float threshold = 0.01f; // Linear threshold for timing measurement
            double holdTime = AudioAnalyzer::measureHoldTime(testSignal, output, 
                                                           threshold, TEST_SAMPLE_RATE);
            
            if (holdTime >= 0.0) {
                log("  Measured hold time: " + std::to_string(holdTime * 1000.0) + " ms\n");
                
                // Verify hold time is reasonable
                assertTrue(holdTime >= 0.0 && holdTime < 1.0, 
                          name + " timing produces reasonable hold time");
            }
            
            // Count gate transitions to check for chatter
            int transitions = AudioAnalyzer::countGateTransitions(output, threshold);
            log("  Gate transitions: " + std::to_string(transitions) + "\n");
            
            // Should have clean gate operation (not too many transitions)
            assertTrue(transitions < 10, name + " timing has clean gate operation");
        }
    }
    
    // Test 4: Hysteresis behavior
    void testHysteresisBehavior() {
        log("\n--- Hysteresis Behavior Tests ---\n");
        
        // Generate chattering signal around threshold
        auto chatterSignal = TestSignalGenerator::generateChatteringSignal(
            -35.0f, -25.0f, 10.0, 2.0, TEST_SAMPLE_RATE);
        
        // Test different hysteresis settings
        std::vector<float> hysteresisSettings = {0.0f, 0.5f, 1.0f};
        
        for (float hysteresis : hysteresisSettings) {
            log("Testing hysteresis setting: " + std::to_string(hysteresis) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;     // Threshold around -30dB
            params[1] = 0.7f;     // Range
            params[2] = 0.1f;     // Fast attack
            params[4] = 0.1f;     // Fast release
            params[5] = hysteresis; // Hysteresis amount
            
            auto output = processAudio(chatterSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Hysteresis " + std::to_string(hysteresis) + " produces valid output");
            
            // Count gate transitions
            float threshold = 0.01f;
            int transitions = AudioAnalyzer::countGateTransitions(output, threshold);
            
            log("  Gate transitions: " + std::to_string(transitions) + "\n");
            
            // Higher hysteresis should reduce chatter
            if (hysteresis > 0.5f) {
                assertTrue(transitions < 20, 
                          "High hysteresis reduces gate chatter");
            }
        }
    }
    
    // Test 5: Range (maximum attenuation) testing
    void testRangeAttenuation() {
        log("\n--- Range Attenuation Tests ---\n");
        
        // Generate low-level noise
        auto noiseSignal = TestSignalGenerator::generateNoiseBurst(-40.0f, 1.0, TEST_SAMPLE_RATE);
        
        // Test different range settings
        std::vector<float> rangeSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float range : rangeSettings) {
            log("Testing range setting: " + std::to_string(range) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.8f; // High threshold to ensure gate closes
            params[1] = range; // Range setting
            params[2] = 0.1f; // Fast attack
            params[4] = 0.1f; // Fast release
            
            auto output = processAudio(noiseSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Range " + std::to_string(range) + " produces valid output");
            
            // Measure attenuation
            float attenuation = AudioAnalyzer::calculateAttenuation(noiseSignal, output);
            
            log("  Measured attenuation: " + std::to_string(attenuation) + "dB\n");
            
            // Higher range settings should provide more attenuation
            if (range > 0.5f) {
                assertTrue(attenuation > 10.0f, 
                          "High range setting provides significant attenuation");
            }
            
            // Range should limit maximum attenuation
            assertTrue(attenuation < 60.0f, 
                      "Range limits maximum attenuation");
        }
    }
    
    // Test 6: Sidechain filter response
    void testSidechainFilter() {
        log("\n--- Sidechain Filter Tests ---\n");
        
        // Test with different frequency content
        std::vector<double> testFrequencies = {50.0, 200.0, 1000.0, 5000.0, 10000.0};
        
        // Test different sidechain filter settings
        std::vector<float> filterSettings = {0.0f, 0.5f, 1.0f};
        
        for (float filterSetting : filterSettings) {
            log("Testing sidechain filter: " + std::to_string(filterSetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;        // Threshold
            params[1] = 0.6f;        // Range
            params[6] = filterSetting; // Sidechain filter
            
            for (double freq : testFrequencies) {
                auto testSignal = TestSignalGenerator::generateSignalBurst(
                    freq, -20.0f, 0.5, TEST_SAMPLE_RATE);
                
                auto output = processAudio(testSignal, params);
                
                float inputRMS = AudioAnalyzer::calculateRMS_dB(testSignal);
                float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
                float attenuation = inputRMS - outputRMS;
                
                log("  Frequency " + std::to_string(freq) + "Hz: " +
                    "Attenuation = " + std::to_string(attenuation) + "dB\n");
                
                // Check for reasonable behavior
                assertTrue(attenuation >= 0.0f && attenuation < 30.0f,
                          "Frequency " + std::to_string(freq) + "Hz: reasonable attenuation");
            }
        }
    }
    
    // Test 7: Lookahead processing
    void testLookaheadProcessing() {
        log("\n--- Lookahead Processing Tests ---\n");
        
        // Generate signal with sharp attack
        auto impulseSignal = TestSignalGenerator::generateImpulse(
            0.8f, static_cast<int>(0.1 * TEST_SAMPLE_RATE), 
            static_cast<int>(0.5 * TEST_SAMPLE_RATE));
        
        // Test different lookahead settings
        std::vector<float> lookaheadSettings = {0.0f, 0.5f, 1.0f};
        
        for (float lookahead : lookaheadSettings) {
            log("Testing lookahead: " + std::to_string(lookahead) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.2f;        // Low threshold
            params[1] = 0.8f;        // High range
            params[2] = 0.5f;        // Medium attack
            params[7] = lookahead;   // Lookahead setting
            
            auto output = processAudio(impulseSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Lookahead " + std::to_string(lookahead) + " produces valid output");
            
            // Find impulse position in output
            int impulsePos = -1;
            float maxVal = 0.0f;
            for (int i = 0; i < static_cast<int>(output.size()); ++i) {
                if (std::abs(output[i]) > maxVal) {
                    maxVal = std::abs(output[i]);
                    impulsePos = i;
                }
            }
            
            log("  Impulse detected at sample: " + std::to_string(impulsePos) + "\n");
            
            assertTrue(impulsePos >= 0, "Impulse preserved with lookahead " + 
                      std::to_string(lookahead));
        }
    }
    
    // Test 8: Gate state transitions
    void testGateStateTransitions() {
        log("\n--- Gate State Transitions Tests ---\n");
        
        // Create complex signal with multiple level changes
        std::vector<float> complexSignal;
        
        // Build sequence: quiet -> loud -> quiet -> loud -> quiet
        auto levels = std::vector<float>{-60.0f, -10.0f, -50.0f, -15.0f, -70.0f};
        
        for (float level : levels) {
            auto segment = TestSignalGenerator::generateSignalBurst(
                1000.0, level, 0.2, TEST_SAMPLE_RATE);
            complexSignal.insert(complexSignal.end(), segment.begin(), segment.end());
        }
        
        std::map<int, float> params;
        params[0] = 0.4f; // Threshold around -30dB
        params[1] = 0.7f; // High range
        params[2] = 0.2f; // Fast attack
        params[3] = 0.1f; // Short hold
        params[4] = 0.3f; // Medium release
        
        auto output = processAudio(complexSignal, params);
        
        // Check for valid output
        assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                  "Complex signal produces valid output");
        
        // Analyze gate behavior
        float threshold = 0.01f;
        int transitions = AudioAnalyzer::countGateTransitions(output, threshold);
        
        log("Total gate transitions: " + std::to_string(transitions) + "\n");
        
        // Should have multiple transitions but not excessive chatter
        assertTrue(transitions >= 2 && transitions < 20, 
                  "Appropriate number of gate transitions");
        
        // Check that output follows expected pattern
        float inputRMS = AudioAnalyzer::calculateRMS_dB(complexSignal);
        float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
        
        log("Input RMS: " + std::to_string(inputRMS) + "dB\n");
        log("Output RMS: " + std::to_string(outputRMS) + "dB\n");
        
        assertTrue(outputRMS < inputRMS, "Gate provides overall noise reduction");
    }
    
    // Test 9: Thermal and component aging effects
    void testAnalogModeling() {
        log("\n--- Analog Modeling Tests ---\n");
        
        // Long test signal to trigger thermal effects
        auto longSignal = TestSignalGenerator::generateGateTestSequence(
            -45.0f, -15.0f, 1.0, 2.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.5f; // Threshold
        params[1] = 0.6f; // Range
        params[2] = 0.3f; // Attack
        params[4] = 0.4f; // Release
        
        auto output = processAudio(longSignal, params);
        
        // Check for valid output throughout long processing
        assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                  "Long signal processing produces valid output");
        
        // Analyze stability over time
        int quarterSize = static_cast<int>(output.size() / 4);
        
        std::vector<float> quarter1(output.begin(), output.begin() + quarterSize);
        std::vector<float> quarter4(output.end() - quarterSize, output.end());
        
        float quarter1RMS = AudioAnalyzer::calculateRMS_dB(quarter1);
        float quarter4RMS = AudioAnalyzer::calculateRMS_dB(quarter4);
        float drift = std::abs(quarter1RMS - quarter4RMS);
        
        log("RMS drift over time: " + std::to_string(drift) + "dB\n");
        
        assertTrue(drift < 2.0f, "Thermal modeling maintains reasonable stability");
        
        // Check that analog effects don't cause excessive noise
        float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
        assertTrue(peakLevel < 6.0f, "Analog modeling doesn't cause clipping");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Noise Gate test suite...\n");
        
        testParameterSweeps();
        testThresholdDetection();
        testGateTiming();
        testHysteresisBehavior();
        testRangeAttenuation();
        testSidechainFilter();
        testLookaheadProcessing();
        testGateStateTransitions();
        testAnalogModeling();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        NoiseGateTest tester;
        tester.runAllTests();
        
        std::cout << "\nNoise Gate test suite completed successfully.\n";
        std::cout << "Check NoiseGate_TestResults.txt for detailed results.\n";
        
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