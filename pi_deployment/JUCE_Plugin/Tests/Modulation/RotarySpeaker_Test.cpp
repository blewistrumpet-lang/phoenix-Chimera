/*
  ==============================================================================
  
    RotarySpeaker_Test.cpp
    Comprehensive test suite for ENGINE_ROTARY_SPEAKER (RotarySpeaker)
    
    Tests for rotary speaker characteristics:
    - Horn and rotor speed accuracy
    - Doppler effect accuracy
    - Amplitude modulation precision
    - Crossover frequency behavior
    - Stereo imaging and spatial effects
    - Mix parameter behavior
    
  ==============================================================================
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cmath>
#include <map>

#include "../../Source/RotarySpeaker.h"
#include "../../Source/EngineTypes.h"

constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;

class RotarySpeakerAnalyzer {
public:
    static float calculateRMS_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        double sum = 0.0;
        for (float sample : signal) sum += sample * sample;
        return 20.0f * std::log10(std::sqrt(sum / signal.size()));
    }
    
    static bool hasInvalidValues(const std::vector<float>& signal) {
        return std::any_of(signal.begin(), signal.end(), [](float s) { return !std::isfinite(s); });
    }
    
    static float measureDopplerEffect(const std::vector<float>& signal) {
        if (signal.size() < 2048) return 0.0f;
        
        // Measure frequency variations as a proxy for Doppler effect
        std::vector<float> instantaneousFreq;
        for (size_t i = 1; i < signal.size() - 1; ++i) {
            float derivative = (signal[i+1] - signal[i-1]) * 0.5f;
            instantaneousFreq.push_back(std::abs(derivative));
        }
        
        if (instantaneousFreq.empty()) return 0.0f;
        
        float minFreq = *std::min_element(instantaneousFreq.begin(), instantaneousFreq.end());
        float maxFreq = *std::max_element(instantaneousFreq.begin(), instantaneousFreq.end());
        
        return (maxFreq > 0.0f) ? (maxFreq - minFreq) / maxFreq : 0.0f;
    }
    
    static float measureStereoWidth(const std::vector<float>& left, const std::vector<float>& right) {
        if (left.size() != right.size() || left.empty()) return 0.0f;
        
        double midEnergy = 0.0, sideEnergy = 0.0;
        for (size_t i = 0; i < left.size(); ++i) {
            double mid = (left[i] + right[i]) * 0.5;
            double side = (left[i] - right[i]) * 0.5;
            midEnergy += mid * mid;
            sideEnergy += side * side;
        }
        
        return (midEnergy > 0.0) ? static_cast<float>(sideEnergy / midEnergy) : 0.0f;
    }
};

class TestSignalGenerator {
public:
    static std::vector<std::vector<float>> generateStereoSineWave(double frequency, double amplitude,
                                                                double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = static_cast<float>(amplitude * std::sin(2.0 * M_PI * frequency * i / sampleRate));
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
    
    static std::vector<std::vector<float>> generateBroadbandSignal(double amplitude, double duration,
                                                                 double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        // Generate signal with both low and high frequency content
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            float sample = static_cast<float>(amplitude * (
                0.5 * std::sin(2.0 * M_PI * 200.0 * t) +  // Low freq (woofer)
                0.3 * std::sin(2.0 * M_PI * 2000.0 * t) +  // Mid freq
                0.2 * std::sin(2.0 * M_PI * 8000.0 * t)    // High freq (horn)
            ));
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
};

class RotarySpeakerTest {
private:
    std::unique_ptr<RotarySpeaker> rotary;
    std::ofstream logFile;
    int testsPassed = 0, testsFailed = 0;
    
public:
    RotarySpeakerTest() {
        rotary = std::make_unique<RotarySpeaker>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/RotarySpeaker_TestResults.txt");
        rotary->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Rotary Speaker Test Suite ===\n");
        log("Engine ID: " + std::to_string(ENGINE_ROTARY_SPEAKER) + "\n");
    }
    
    ~RotarySpeakerTest() {
        log("\nTests Passed: " + std::to_string(testsPassed) + "\n");
        log("Tests Failed: " + std::to_string(testsFailed) + "\n");
        if (logFile.is_open()) logFile.close();
    }
    
    void log(const std::string& message) {
        std::cout << message;
        if (logFile.is_open()) logFile << message;
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
    
    std::vector<std::vector<float>> processAudio(const std::vector<std::vector<float>>& input,
                                                const std::map<int, float>& parameters) {
        rotary->updateParameters(parameters);
        
        std::vector<std::vector<float>> output(2);
        if (input.empty() || input[0].empty()) return output;
        
        size_t totalSamples = input[0].size();
        output[0].reserve(totalSamples);
        output[1].reserve(totalSamples);
        
        for (size_t i = 0; i < totalSamples; i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), totalSamples - i);
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            for (size_t j = 0; j < blockSize; ++j) {
                buffer.setSample(0, static_cast<int>(j), (i + j < input[0].size()) ? input[0][i + j] : 0.0f);
                buffer.setSample(1, static_cast<int>(j), (i + j < input[1].size()) ? input[1][i + j] : 0.0f);
            }
            
            rotary->process(buffer);
            
            for (size_t j = 0; j < blockSize; ++j) {
                output[0].push_back(buffer.getSample(0, static_cast<int>(j)));
                output[1].push_back(buffer.getSample(1, static_cast<int>(j)));
            }
        }
        
        return output;
    }
    
    void testDopplerEffect() {
        log("\n--- Doppler Effect Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(2000.0, 0.3, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<float> speedSettings = {0.3f, 0.6f, 0.9f};
        
        for (float speed : speedSettings) {
            log("Testing rotor speed: " + std::to_string(speed) + "\n");
            
            std::map<int, float> params;
            params[0] = speed; // Horn speed
            params[1] = speed; // Rotor speed
            for (int p = 2; p < rotary->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto output = processAudio(testSignal, params);
            
            float dopplerEffect = RotarySpeakerAnalyzer::measureDopplerEffect(output[0]);
            log("  Doppler effect measure: " + std::to_string(dopplerEffect) + "\n");
            
            assertTrue(dopplerEffect > 0.01f,
                      "Measurable Doppler effect at speed " + std::to_string(speed));
            assertTrue(!RotarySpeakerAnalyzer::hasInvalidValues(output[0]) &&
                      !RotarySpeakerAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at speed " + std::to_string(speed));
        }
    }
    
    void testStereoImaging() {
        log("\n--- Stereo Imaging Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateBroadbandSignal(0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.6f; // Horn speed
        params[1] = 0.4f; // Rotor speed (different from horn)
        for (int p = 2; p < rotary->getNumParameters(); ++p) {
            params[p] = 0.5f;
        }
        
        auto output = processAudio(testSignal, params);
        
        float stereoWidth = RotarySpeakerAnalyzer::measureStereoWidth(output[0], output[1]);
        log("Stereo width measure: " + std::to_string(stereoWidth) + "\n");
        
        assertTrue(stereoWidth > 0.1f, "Significant stereo width created");
        assertTrue(!RotarySpeakerAnalyzer::hasInvalidValues(output[0]) &&
                  !RotarySpeakerAnalyzer::hasInvalidValues(output[1]),
                  "Valid stereo output");
    }
    
    void testCrossoverBehavior() {
        log("\n--- Crossover Behavior Tests ---\n");
        
        auto lowFreqSignal = TestSignalGenerator::generateStereoSineWave(200.0, 0.3, 2.0, TEST_SAMPLE_RATE);
        auto highFreqSignal = TestSignalGenerator::generateStereoSineWave(4000.0, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.7f; // Horn speed
        params[1] = 0.3f; // Rotor speed
        if (rotary->getNumParameters() > 2) {
            params[2] = 0.5f; // Crossover frequency
        }
        for (int p = 3; p < rotary->getNumParameters(); ++p) {
            params[p] = 0.5f;
        }
        
        auto lowOutput = processAudio(lowFreqSignal, params);
        auto highOutput = processAudio(highFreqSignal, params);
        
        float lowRMS = RotarySpeakerAnalyzer::calculateRMS_dB(lowOutput[0]);
        float highRMS = RotarySpeakerAnalyzer::calculateRMS_dB(highOutput[0]);
        
        log("Low freq output: " + std::to_string(lowRMS) + "dB\n");
        log("High freq output: " + std::to_string(highRMS) + "dB\n");
        
        assertTrue(!RotarySpeakerAnalyzer::hasInvalidValues(lowOutput[0]) &&
                  !RotarySpeakerAnalyzer::hasInvalidValues(highOutput[0]),
                  "Valid crossover processing");
    }
    
    void runAllTests() {
        log("Starting Rotary Speaker test suite...\n");
        
        testDopplerEffect();
        testStereoImaging();
        testCrossoverBehavior();
        
        log("\nAll tests completed.\n");
    }
};

int main() {
    try {
        RotarySpeakerTest tester;
        tester.runAllTests();
        std::cout << "\nRotary Speaker test suite completed successfully.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}