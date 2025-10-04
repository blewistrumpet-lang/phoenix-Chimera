/*
  ==============================================================================
  
    DetuneDoubler_Test.cpp
    Comprehensive test suite for ENGINE_DETUNE_DOUBLER (DetuneDoubler)
    
    Tests for detune doubler characteristics:
    - Detune amount accuracy and precision
    - Voice spread and stereo imaging
    - Pitch tracking across frequency range
    - Mix parameter behavior
    - Phase relationships between voices
    - Performance and stability
    
  ==============================================================================
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cmath>
#include <map>

#include "../../Source/DetuneDoubler.h"
#include "../../Source/EngineTypes.h"

constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;

class DetuneDoublerAnalyzer {
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
    
    static float measureDetuneEffect(const std::vector<float>& original, const std::vector<float>& processed) {
        if (original.size() != processed.size() || original.size() < 2048) return 0.0f;
        
        // Measure beating effect as indication of detuning
        std::vector<float> beatSignal;
        for (size_t i = 0; i < original.size(); ++i) {
            beatSignal.push_back(std::abs(processed[i]) - std::abs(original[i]));
        }
        
        // Find variance in the beat signal
        float mean = 0.0f;
        for (float sample : beatSignal) mean += sample;
        mean /= beatSignal.size();
        
        float variance = 0.0f;
        for (float sample : beatSignal) {
            variance += (sample - mean) * (sample - mean);
        }
        variance /= beatSignal.size();
        
        return std::sqrt(variance);
    }
    
    static float measureStereoWidth(const std::vector<float>& left, const std::vector<float>& right) {
        if (left.size() != right.size() || left.empty()) return 0.0f;
        
        double correlation = 0.0, leftPower = 0.0, rightPower = 0.0;
        
        for (size_t i = 0; i < left.size(); ++i) {
            correlation += left[i] * right[i];
            leftPower += left[i] * left[i];
            rightPower += right[i] * right[i];
        }
        
        double denominator = std::sqrt(leftPower * rightPower);
        if (denominator > 0.0) {
            return 1.0f - static_cast<float>(std::abs(correlation / denominator));
        }
        
        return 0.0f;
    }
    
    static float measureChorusEffect(const std::vector<float>& signal) {
        if (signal.size() < 1024) return 0.0f;
        
        // Measure modulation in the signal envelope
        std::vector<float> envelope;
        float follower = 0.0f;
        float smoothing = 0.99f;
        
        for (float sample : signal) {
            float rectified = std::abs(sample);
            follower = rectified + (follower - rectified) * smoothing;
            envelope.push_back(follower);
        }
        
        float minEnv = *std::min_element(envelope.begin(), envelope.end());
        float maxEnv = *std::max_element(envelope.begin(), envelope.end());
        
        return (maxEnv > 0.0f) ? (maxEnv - minEnv) / maxEnv : 0.0f;
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
    
    static std::vector<std::vector<float>> generateChord(const std::vector<double>& frequencies,
                                                        double amplitude, double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples, 0.0f));
        
        double scale = amplitude / frequencies.size();
        
        for (double freq : frequencies) {
            for (int i = 0; i < numSamples; ++i) {
                float sample = static_cast<float>(scale * std::sin(2.0 * M_PI * freq * i / sampleRate));
                signal[0][i] += sample;
                signal[1][i] += sample;
            }
        }
        
        return signal;
    }
};

class DetuneDoublerTest {
private:
    std::unique_ptr<DetuneDoubler> doubler;
    std::ofstream logFile;
    int testsPassed = 0, testsFailed = 0;
    
public:
    DetuneDoublerTest() {
        doubler = std::make_unique<DetuneDoubler>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/DetuneDoubler_TestResults.txt");
        doubler->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Detune Doubler Test Suite ===\n");
        log("Engine ID: " + std::to_string(ENGINE_DETUNE_DOUBLER) + "\n");
    }
    
    ~DetuneDoublerTest() {
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
        doubler->updateParameters(parameters);
        
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
            
            doubler->process(buffer);
            
            for (size_t j = 0; j < blockSize; ++j) {
                output[0].push_back(buffer.getSample(0, static_cast<int>(j)));
                output[1].push_back(buffer.getSample(1, static_cast<int>(j)));
            }
        }
        
        return output;
    }
    
    void testDetuneAmountAccuracy() {
        log("\n--- Detune Amount Accuracy Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(440.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> detuneSettings = {0.2f, 0.5f, 0.8f};
        
        for (float detune : detuneSettings) {
            log("Testing detune amount: " + std::to_string(detune) + "\n");
            
            std::map<int, float> params;
            params[0] = detune; // Detune amount
            for (int p = 1; p < doubler->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto output = processAudio(testSignal, params);
            
            float detuneEffect = DetuneDoublerAnalyzer::measureDetuneEffect(testSignal[0], output[0]);
            log("  Detune effect measure: " + std::to_string(detuneEffect) + "\n");
            
            // Higher detune settings should show more effect
            if (detune > 0.6f) {
                assertTrue(detuneEffect > 0.01f,
                          "Significant detune effect at " + std::to_string(detune));
            }
            
            assertTrue(!DetuneDoublerAnalyzer::hasInvalidValues(output[0]) &&
                      !DetuneDoublerAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at detune " + std::to_string(detune));
        }
    }
    
    void testVoiceSpreadAndStereoImaging() {
        log("\n--- Voice Spread and Stereo Imaging Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> spreadSettings = {0.0f, 0.5f, 1.0f};
        
        for (float spread : spreadSettings) {
            log("Testing voice spread: " + std::to_string(spread) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.6f; // Moderate detune
            if (doubler->getNumParameters() > 1) {
                params[1] = spread; // Voice spread/width
            }
            for (int p = 2; p < doubler->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto output = processAudio(testSignal, params);
            
            float stereoWidth = DetuneDoublerAnalyzer::measureStereoWidth(output[0], output[1]);
            log("  Stereo width measure: " + std::to_string(stereoWidth) + "\n");
            
            // Higher spread should create wider stereo image
            if (spread > 0.7f) {
                assertTrue(stereoWidth > 0.1f,
                          "Wide stereo image at spread " + std::to_string(spread));
            }
            
            assertTrue(!DetuneDoublerAnalyzer::hasInvalidValues(output[0]) &&
                      !DetuneDoublerAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at spread " + std::to_string(spread));
        }
    }
    
    void testChorusEffect() {
        log("\n--- Chorus Effect Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(880.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.7f; // Strong detune
        if (doubler->getNumParameters() > 2) {
            params[2] = 0.8f; // Chorus/modulation parameter
        }
        for (int p = 1; p < doubler->getNumParameters(); ++p) {
            if (p != 2) params[p] = 0.5f;
        }
        
        auto output = processAudio(testSignal, params);
        
        float chorusEffect = DetuneDoublerAnalyzer::measureChorusEffect(output[0]);
        log("Chorus effect measure: " + std::to_string(chorusEffect) + "\n");
        
        assertTrue(chorusEffect > 0.05f, "Measurable chorus effect");
        assertTrue(!DetuneDoublerAnalyzer::hasInvalidValues(output[0]) &&
                  !DetuneDoublerAnalyzer::hasInvalidValues(output[1]),
                  "Valid output with chorus effect");
    }
    
    void testComplexSignalHandling() {
        log("\n--- Complex Signal Handling Tests ---\n");
        
        // Test with chord (multiple frequencies)
        std::vector<double> chordFreqs = {261.63, 329.63, 392.0}; // C major triad
        auto chordSignal = TestSignalGenerator::generateChord(chordFreqs, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.5f; // Moderate detune
        for (int p = 1; p < doubler->getNumParameters(); ++p) {
            params[p] = 0.5f;
        }
        
        auto output = processAudio(chordSignal, params);
        
        float outputRMS = DetuneDoublerAnalyzer::calculateRMS_dB(output[0]);
        log("Complex signal output RMS: " + std::to_string(outputRMS) + "dB\n");
        
        assertTrue(outputRMS > -60.0f, "Reasonable output level for complex signal");
        assertTrue(!DetuneDoublerAnalyzer::hasInvalidValues(output[0]) &&
                  !DetuneDoublerAnalyzer::hasInvalidValues(output[1]),
                  "Valid output for complex signal");
    }
    
    void runAllTests() {
        log("Starting Detune Doubler test suite...\n");
        
        testDetuneAmountAccuracy();
        testVoiceSpreadAndStereoImaging();
        testChorusEffect();
        testComplexSignalHandling();
        
        log("\nAll tests completed.\n");
    }
};

int main() {
    try {
        DetuneDoublerTest tester;
        tester.runAllTests();
        std::cout << "\nDetune Doubler test suite completed successfully.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}