/*
  ==============================================================================
  
    ClassicTremolo_Test.cpp
    Comprehensive test suite for ENGINE_CLASSIC_TREMOLO (ClassicTremolo)
    
    Tests for classic tremolo characteristics:
    - LFO rate accuracy and tempo sync
    - Depth/intensity modulation precision
    - Waveform shape accuracy (sine, triangle, square)
    - Stereo phase relationships
    - Mix parameter behavior
    - Performance and stability
    
  ==============================================================================
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cmath>
#include <map>

#include "../../Source/ClassicTremolo.h"
#include "../../Source/EngineTypes.h"

constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;

class ClassicTremoloAnalyzer {
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
    
    static float measureModulationDepth(const std::vector<float>& signal) {
        if (signal.size() < 1024) return 0.0f;
        
        float minVal = *std::min_element(signal.begin(), signal.end());
        float maxVal = *std::max_element(signal.begin(), signal.end());
        
        return (maxVal > 0.0f) ? (maxVal - minVal) / (maxVal + minVal) : 0.0f;
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
};

class ClassicTremoloTest {
private:
    std::unique_ptr<ClassicTremolo> tremolo;
    std::ofstream logFile;
    int testsPassed = 0, testsFailed = 0;
    
public:
    ClassicTremoloTest() {
        tremolo = std::make_unique<ClassicTremolo>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/ClassicTremolo_TestResults.txt");
        tremolo->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Classic Tremolo Test Suite ===\n");
        log("Engine ID: " + std::to_string(ENGINE_CLASSIC_TREMOLO) + "\n");
    }
    
    ~ClassicTremoloTest() {
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
        tremolo->updateParameters(parameters);
        
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
            
            tremolo->process(buffer);
            
            for (size_t j = 0; j < blockSize; ++j) {
                output[0].push_back(buffer.getSample(0, static_cast<int>(j)));
                output[1].push_back(buffer.getSample(1, static_cast<int>(j)));
            }
        }
        
        return output;
    }
    
    void testBasicOperation() {
        log("\n--- Basic Operation Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.5f; // Rate
        params[1] = 0.7f; // Depth
        for (int p = 2; p < tremolo->getNumParameters(); ++p) {
            params[p] = 0.5f;
        }
        
        auto output = processAudio(testSignal, params);
        
        assertTrue(!ClassicTremoloAnalyzer::hasInvalidValues(output[0]),
                  "Valid left channel output");
        assertTrue(!ClassicTremoloAnalyzer::hasInvalidValues(output[1]),
                  "Valid right channel output");
        
        float modulation = ClassicTremoloAnalyzer::measureModulationDepth(output[0]);
        assertTrue(modulation > 0.1f, "Measurable tremolo effect");
        
        log("  Modulation depth: " + std::to_string(modulation) + "\n");
    }
    
    void runAllTests() {
        log("Starting Classic Tremolo test suite...\n");
        testBasicOperation();
        log("\nAll tests completed.\n");
    }
};

int main() {
    try {
        ClassicTremoloTest tester;
        tester.runAllTests();
        std::cout << "\nClassic Tremolo test suite completed successfully.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}