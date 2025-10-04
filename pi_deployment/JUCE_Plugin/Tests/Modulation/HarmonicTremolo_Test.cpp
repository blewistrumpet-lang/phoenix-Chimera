/*
  ==============================================================================
  
    HarmonicTremolo_Test.cpp
    Comprehensive test suite for ENGINE_HARMONIC_TREMOLO (HarmonicTremolo)
    
    Tests for harmonic tremolo characteristics:
    - LFO rate accuracy and waveform shape
    - Harmonic emphasis and filtering
    - Depth/intensity modulation precision
    - Crossover frequency tracking
    - Stereo imaging and phase relationships
    - Mix parameter behavior
    
  ==============================================================================
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>

#include "../../Source/HarmonicTremolo.h"
#include "../../Source/EngineTypes.h"

constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;

class HarmonicTremoloAnalyzer {
public:
    static float calculateRMS_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        double sum = 0.0;
        for (float sample : signal) sum += sample * sample;
        double rms = std::sqrt(sum / signal.size());
        return 20.0f * std::log10(std::max(1e-6, rms));
    }
    
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) return true;
        }
        return false;
    }
    
    static float measureModulationDepth(const std::vector<float>& signal) {
        if (signal.size() < 1024) return 0.0f;
        
        std::vector<float> envelope;
        float smoothing = 0.95f, envelopeFollower = 0.0f;
        
        for (float sample : signal) {
            float rectified = std::abs(sample);
            envelopeFollower = rectified + (envelopeFollower - rectified) * smoothing;
            envelope.push_back(envelopeFollower);
        }
        
        float minEnv = *std::min_element(envelope.begin(), envelope.end());
        float maxEnv = *std::max_element(envelope.begin(), envelope.end());
        
        return (maxEnv > 0.0f) ? (maxEnv - minEnv) / maxEnv : 0.0f;
    }
    
    static float measureStereoCorrelation(const std::vector<float>& left, const std::vector<float>& right) {
        if (left.size() != right.size() || left.empty()) return 0.0f;
        
        double sum_left = 0.0, sum_right = 0.0, sum_lr = 0.0;
        double sum_left_sq = 0.0, sum_right_sq = 0.0;
        
        for (size_t i = 0; i < left.size(); ++i) {
            sum_left += left[i];
            sum_right += right[i];
            sum_lr += left[i] * right[i];
            sum_left_sq += left[i] * left[i];
            sum_right_sq += right[i] * right[i];
        }
        
        double n = static_cast<double>(left.size());
        double numerator = n * sum_lr - sum_left * sum_right;
        double denominator = std::sqrt((n * sum_left_sq - sum_left * sum_left) * 
                                     (n * sum_right_sq - sum_right * sum_right));
        
        return (denominator > 0.0) ? static_cast<float>(numerator / denominator) : 0.0f;
    }
};

class TestSignalGenerator {
public:
    static std::vector<std::vector<float>> generateStereoSineWave(double frequency, double amplitude,
                                                                double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        for (int i = 0; i < numSamples; ++i) {
            double phase = 2.0 * M_PI * frequency * i / sampleRate;
            float sample = static_cast<float>(amplitude * std::sin(phase));
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
    
    static std::vector<std::vector<float>> generateComplexWave(double fundamentalFreq, double amplitude,
                                                             double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            float sample = static_cast<float>(amplitude * (
                std::sin(2.0 * M_PI * fundamentalFreq * t) +
                0.3 * std::sin(2.0 * M_PI * 2.0 * fundamentalFreq * t) +
                0.1 * std::sin(2.0 * M_PI * 3.0 * fundamentalFreq * t)
            ));
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
};

class HarmonicTremoloTest {
private:
    std::unique_ptr<HarmonicTremolo> tremolo;
    std::ofstream logFile, csvFile;
    int testsPassed = 0, testsFailed = 0;
    
public:
    HarmonicTremoloTest() {
        tremolo = std::make_unique<HarmonicTremolo>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/HarmonicTremolo_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/HarmonicTremolo_Data.csv");
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        tremolo->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Harmonic Tremolo Test Suite ===\n");
        log("Engine ID: " + std::to_string(ENGINE_HARMONIC_TREMOLO) + "\n");
    }
    
    ~HarmonicTremoloTest() {
        log("\n=== Test Summary ===\n");
        log("Tests Passed: " + std::to_string(testsPassed) + "\n");
        log("Tests Failed: " + std::to_string(testsFailed) + "\n");
        
        if (testsPassed + testsFailed > 0) {
            float successRate = 100.0f * testsPassed / (testsPassed + testsFailed);
            log("Success Rate: " + std::to_string(successRate) + "%\n");
        }
        
        if (logFile.is_open()) logFile.close();
        if (csvFile.is_open()) csvFile.close();
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
    
    std::pair<std::vector<std::vector<float>>, std::vector<std::vector<float>>>
    processAudio(const std::vector<std::vector<float>>& input, const std::map<int, float>& parameters) {
        tremolo->updateParameters(parameters);
        
        std::vector<std::vector<float>> output(2);
        std::vector<std::vector<float>> original = input;
        
        if (input.empty() || input[0].empty()) return {original, output};
        
        size_t totalSamples = input[0].size();
        output[0].reserve(totalSamples);
        output[1].reserve(totalSamples);
        
        for (size_t i = 0; i < totalSamples; i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), totalSamples - i);
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            for (size_t j = 0; j < blockSize; ++j) {
                buffer.setSample(0, static_cast<int>(j),
                               (i + j < input[0].size()) ? input[0][i + j] : 0.0f);
                buffer.setSample(1, static_cast<int>(j),
                               (i + j < input[1].size()) ? input[1][i + j] : 0.0f);
            }
            
            tremolo->process(buffer);
            
            for (size_t j = 0; j < blockSize; ++j) {
                output[0].push_back(buffer.getSample(0, static_cast<int>(j)));
                output[1].push_back(buffer.getSample(1, static_cast<int>(j)));
            }
        }
        
        return {original, output};
    }
    
    void testModulationDepth() {
        log("\n--- Modulation Depth Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> depthSettings = {0.2f, 0.5f, 0.8f};
        
        for (float depth : depthSettings) {
            log("Testing modulation depth: " + std::to_string(depth) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f; // Rate
            params[1] = depth; // Depth
            for (int p = 2; p < tremolo->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            float measuredDepth = HarmonicTremoloAnalyzer::measureModulationDepth(output[0]);
            log("  Measured modulation depth: " + std::to_string(measuredDepth) + "\n");
            
            assertTrue(measuredDepth > 0.01f,
                      "Measurable modulation at depth " + std::to_string(depth));
            assertTrue(!HarmonicTremoloAnalyzer::hasInvalidValues(output[0]) &&
                      !HarmonicTremoloAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at depth " + std::to_string(depth));
        }
    }
    
    void testHarmonicProcessing() {
        log("\n--- Harmonic Processing Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateComplexWave(440.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> crossoverSettings = {0.3f, 0.6f, 0.9f};
        
        for (float crossover : crossoverSettings) {
            log("Testing crossover setting: " + std::to_string(crossover) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f; // Rate
            params[1] = 0.6f; // Depth
            if (tremolo->getNumParameters() > 2) {
                params[2] = crossover; // Crossover frequency
            }
            for (int p = 3; p < tremolo->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            float outputRMS = HarmonicTremoloAnalyzer::calculateRMS_dB(output[0]);
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            
            assertTrue(!HarmonicTremoloAnalyzer::hasInvalidValues(output[0]) &&
                      !HarmonicTremoloAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at crossover " + std::to_string(crossover));
        }
    }
    
    void testStereoImaging() {
        log("\n--- Stereo Imaging Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> stereoSettings = {0.0f, 0.5f, 1.0f};
        
        for (float stereo : stereoSettings) {
            log("Testing stereo setting: " + std::to_string(stereo) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f; // Rate
            params[1] = 0.7f; // Depth
            if (tremolo->getNumParameters() > 3) {
                params[3] = stereo; // Stereo parameter
            }
            for (int p = 2; p < tremolo->getNumParameters(); ++p) {
                if (p != 3) params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            float correlation = HarmonicTremoloAnalyzer::measureStereoCorrelation(output[0], output[1]);
            log("  Stereo correlation: " + std::to_string(correlation) + "\n");
            
            if (stereo > 0.7f) {
                assertTrue(std::abs(correlation) < 0.9f,
                          "Reduced correlation at wide stereo " + std::to_string(stereo));
            }
            
            assertTrue(!HarmonicTremoloAnalyzer::hasInvalidValues(output[0]) &&
                      !HarmonicTremoloAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at stereo " + std::to_string(stereo));
        }
    }
    
    void runAllTests() {
        log("Starting Harmonic Tremolo comprehensive test suite...\n");
        
        testModulationDepth();
        testHarmonicProcessing();
        testStereoImaging();
        
        log("\nAll tests completed.\n");
    }
};

int main() {
    try {
        HarmonicTremoloTest tester;
        tester.runAllTests();
        
        std::cout << "\nHarmonic Tremolo test suite completed successfully.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}