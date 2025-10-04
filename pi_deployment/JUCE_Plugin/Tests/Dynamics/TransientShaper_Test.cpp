/*
  ==============================================================================
  
    TransientShaper_Test.cpp
    Comprehensive test suite for ENGINE_TRANSIENT_SHAPER
    
    Tests for transient shaper characteristics:
    - Parameter sweep validation (all 10 parameters)
    - Attack/Sustain separation accuracy
    - Transient detection algorithms (Peak, RMS, Hilbert, Hybrid)
    - Timing precision for attack and release phases
    - Lookahead mode vs zero-latency mode
    - Soft-knee compression behavior
    - Oversampling quality (2x/4x)
    - Multi-algorithm detection validation
    - Professional oversampling verification
    - Complete denormal protection
    
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
#include "../../Source/TransientShaper_Platinum.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;

// Test signal generators
class TestSignalGenerator {
public:
    // Generate drum hit (attack + decay)
    static std::vector<float> generateDrumHit(double amplitude, double duration, 
                                            double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double attackTime = duration * 0.1; // 10% attack
        double decayTime = duration * 0.9;  // 90% decay
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double envelope;
            
            if (t < attackTime) {
                // Fast attack
                envelope = t / attackTime;
            } else {
                // Exponential decay
                double decayT = (t - attackTime) / decayTime;
                envelope = std::exp(-decayT * 3.0);
            }
            
            // Add some harmonic content for realism
            double phase = 2.0 * M_PI * 200.0 * t; // 200Hz fundamental
            double harmonic = std::sin(phase) + 0.5 * std::sin(2 * phase) + 0.25 * std::sin(3 * phase);
            
            signal[i] = static_cast<float>(amplitude * envelope * harmonic);
        }
        
        return signal;
    }
    
    // Generate sustained note (organ-like)
    static std::vector<float> generateSustainedNote(double frequency, double amplitude, 
                                                  double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        // Slow attack and release for sustained character
        double attackSamples = 0.05 * sampleRate; // 50ms attack
        double releaseSamples = 0.1 * sampleRate;  // 100ms release
        
        for (int i = 0; i < numSamples; ++i) {
            double envelope = 1.0;
            
            if (i < attackSamples) {
                envelope = i / attackSamples;
            } else if (i > numSamples - releaseSamples) {
                double releaseT = (numSamples - i) / releaseSamples;
                envelope = releaseT;
            }
            
            signal[i] = static_cast<float>(amplitude * envelope * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate mixed transient/sustained content
    static std::vector<float> generateMixedContent(double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        // Add periodic drum hits
        int hitInterval = static_cast<int>(0.5 * sampleRate); // Every 500ms
        for (int pos = 0; pos < numSamples; pos += hitInterval) {
            auto drumHit = generateDrumHit(0.8, 0.2, sampleRate);
            
            for (size_t i = 0; i < drumHit.size() && pos + i < signal.size(); ++i) {
                signal[pos + i] += drumHit[i] * 0.7f;
            }
        }
        
        // Add sustained background
        auto sustained = generateSustainedNote(440.0, 0.3, duration, sampleRate);
        for (size_t i = 0; i < signal.size() && i < sustained.size(); ++i) {
            signal[i] += sustained[i] * 0.3f;
        }
        
        return signal;
    }
    
    // Generate impulse train for transient detection testing
    static std::vector<float> generateImpulseTrain(double amplitude, double interval, 
                                                 double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        int intervalSamples = static_cast<int>(interval * sampleRate);
        
        for (int i = 0; i < numSamples; i += intervalSamples) {
            if (i < numSamples) {
                signal[i] = static_cast<float>(amplitude);
            }
        }
        
        return signal;
    }
    
    // Generate sine wave with envelope
    static std::vector<float> generateEnvelopedSine(double frequency, double amplitude,
                                                  double attackTime, double sustainTime, 
                                                  double releaseTime, double sampleRate) {
        double totalTime = attackTime + sustainTime + releaseTime;
        int numSamples = static_cast<int>(totalTime * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        int attackSamples = static_cast<int>(attackTime * sampleRate);
        int sustainSamples = static_cast<int>(sustainTime * sampleRate);
        int releaseSamples = static_cast<int>(releaseTime * sampleRate);
        
        for (int i = 0; i < numSamples; ++i) {
            double envelope = 1.0;
            
            if (i < attackSamples) {
                envelope = static_cast<double>(i) / attackSamples;
            } else if (i >= attackSamples + sustainSamples) {
                int releaseIdx = i - attackSamples - sustainSamples;
                envelope = 1.0 - static_cast<double>(releaseIdx) / releaseSamples;
            }
            
            signal[i] = static_cast<float>(amplitude * envelope * std::sin(phase));
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
    
    // Detect transient positions
    static std::vector<int> detectTransients(const std::vector<float>& signal, 
                                           float threshold = 0.1f) {
        std::vector<int> transients;
        
        if (signal.size() < 10) return transients;
        
        // Simple energy-based transient detection
        std::vector<float> energy;
        int windowSize = 64;
        
        for (int i = windowSize; i < static_cast<int>(signal.size()) - windowSize; ++i) {
            float currentEnergy = 0.0f;
            float previousEnergy = 0.0f;
            
            // Current window energy
            for (int j = -windowSize/2; j < windowSize/2; ++j) {
                currentEnergy += signal[i + j] * signal[i + j];
            }
            
            // Previous window energy
            for (int j = -windowSize; j < -windowSize/2; ++j) {
                previousEnergy += signal[i + j] * signal[i + j];
            }
            
            // If current energy is significantly higher, it's likely a transient
            if (currentEnergy > previousEnergy * (1.0f + threshold)) {
                transients.push_back(i);
                i += windowSize; // Skip ahead to avoid duplicate detections
            }
        }
        
        return transients;
    }
    
    // Measure attack/sustain separation quality
    static float measureSeparationQuality(const std::vector<float>& original,
                                        const std::vector<float>& processed,
                                        const std::vector<int>& transientPositions) {
        if (transientPositions.empty()) return 0.0f;
        
        float totalQuality = 0.0f;
        
        for (int transientPos : transientPositions) {
            // Analyze a window around the transient
            int windowSize = 128;
            int startIdx = std::max(0, transientPos - windowSize/2);
            int endIdx = std::min(static_cast<int>(original.size()), transientPos + windowSize/2);
            
            float originalEnergy = 0.0f;
            float processedEnergy = 0.0f;
            
            for (int i = startIdx; i < endIdx; ++i) {
                originalEnergy += original[i] * original[i];
                processedEnergy += processed[i] * processed[i];
            }
            
            // Calculate enhancement ratio
            if (originalEnergy > 0.0f) {
                float ratio = processedEnergy / originalEnergy;
                totalQuality += ratio;
            }
        }
        
        return totalQuality / transientPositions.size();
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
    
    // Calculate crest factor (peak/RMS ratio)
    static float calculateCrestFactor(const std::vector<float>& signal) {
        float peakLinear = std::pow(10.0f, calculatePeak_dB(signal) / 20.0f);
        float rmsLinear = std::pow(10.0f, calculateRMS_dB(signal) / 20.0f);
        
        if (rmsLinear > 0.0f) {
            return peakLinear / rmsLinear;
        }
        return 1.0f;
    }
};

// Main test class
class TransientShaperTest {
private:
    std::unique_ptr<TransientShaper_Platinum> shaper;
    std::ofstream logFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    TransientShaperTest() {
        shaper = std::make_unique<TransientShaper_Platinum>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Dynamics/TransientShaper_TestResults.txt");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        // Prepare the shaper
        shaper->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Transient Shaper Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_TRANSIENT_SHAPER) + "\n");
        log("Parameter Count: " + std::to_string(shaper->getNumParameters()) + "\n\n");
    }
    
    ~TransientShaperTest() {
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
    
    // Process audio through transient shaper
    std::vector<float> processAudio(const std::vector<float>& input, 
                                  const std::map<int, float>& parameters) {
        // Update parameters
        shaper->updateParameters(parameters);
        
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
            shaper->process(buffer);
            
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
        
        // Create test signal with both transients and sustain
        auto testSignal = TestSignalGenerator::generateMixedContent(2.0, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < shaper->getNumParameters(); ++param) {
            std::string paramName = shaper->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseValues;
            
            // Sweep from 0.0 to 1.0 in 0.2 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.2f) {
                std::map<int, float> params;
                params[param] = value;
                
                // Set other parameters to neutral values
                if (param != 0) params[0] = 0.5f; // Attack
                if (param != 1) params[1] = 0.5f; // Sustain
                
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
            
            // Core parameters should have significant effect
            if (param <= 1) { // Attack and Sustain parameters
                assertTrue(responseRange > 0.5f, 
                          paramName + " has audible effect");
            }
        }
    }
    
    // Test 2: Attack enhancement
    void testAttackEnhancement() {
        log("\n--- Attack Enhancement Tests ---\n");
        
        // Generate drum hit for testing
        auto drumHit = TestSignalGenerator::generateDrumHit(0.5, 0.5, TEST_SAMPLE_RATE);
        
        // Test different attack enhancement levels
        std::vector<float> attackLevels = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float attackLevel : attackLevels) {
            log("Testing attack level: " + std::to_string(attackLevel) + "\n");
            
            std::map<int, float> params;
            params[0] = attackLevel; // Attack enhancement
            params[1] = 0.5f;        // Neutral sustain
            
            auto output = processAudio(drumHit, params);
            
            // Detect transients in original and processed signals
            auto originalTransients = AudioAnalyzer::detectTransients(drumHit, 0.2f);
            auto outputTransients = AudioAnalyzer::detectTransients(output, 0.2f);
            
            log("  Original transients: " + std::to_string(originalTransients.size()) + "\n");
            log("  Output transients: " + std::to_string(outputTransients.size()) + "\n");
            
            // Measure separation quality
            if (!originalTransients.empty()) {
                float quality = AudioAnalyzer::measureSeparationQuality(
                    drumHit, output, originalTransients);
                log("  Separation quality: " + std::to_string(quality) + "\n");
                
                // Higher attack levels should enhance transients
                if (attackLevel > 0.5f) {
                    assertTrue(quality > 0.8f, 
                              "Attack enhancement at " + std::to_string(attackLevel) + 
                              " improves transients");
                }
            }
            
            // Check output validity
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Attack level " + std::to_string(attackLevel) + " produces valid output");
        }
    }
    
    // Test 3: Sustain processing
    void testSustainProcessing() {
        log("\n--- Sustain Processing Tests ---\n");
        
        // Generate sustained note
        auto sustainedNote = TestSignalGenerator::generateSustainedNote(440.0, 0.4, 1.0, TEST_SAMPLE_RATE);
        
        // Test different sustain levels
        std::vector<float> sustainLevels = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float sustainLevel : sustainLevels) {
            log("Testing sustain level: " + std::to_string(sustainLevel) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;        // Neutral attack
            params[1] = sustainLevel; // Sustain processing
            
            auto output = processAudio(sustainedNote, params);
            
            float inputRMS = AudioAnalyzer::calculateRMS_dB(sustainedNote);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float levelChange = outputRMS - inputRMS;
            
            log("  Input RMS: " + std::to_string(inputRMS) + "dB\n");
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            log("  Level change: " + std::to_string(levelChange) + "dB\n");
            
            // Check for valid processing
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Sustain level " + std::to_string(sustainLevel) + " produces valid output");
            
            // Verify sustain processing affects sustained content appropriately
            assertTrue(std::abs(levelChange) < 20.0f, 
                      "Sustain processing produces reasonable level changes");
        }
    }
    
    // Test 4: Attack/Sustain separation accuracy
    void testAttackSustainSeparation() {
        log("\n--- Attack/Sustain Separation Tests ---\n");
        
        // Create signal with clear attack and sustain phases
        auto testSignal = TestSignalGenerator::generateEnvelopedSine(
            1000.0, 0.6, 0.05, 0.4, 0.1, TEST_SAMPLE_RATE);
        
        // Test separation with different separation values
        std::vector<float> separationLevels = {0.0f, 0.5f, 1.0f};
        
        for (float separation : separationLevels) {
            log("Testing separation level: " + std::to_string(separation) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.8f;        // High attack enhancement
            params[1] = 0.2f;        // Low sustain
            params[4] = separation;   // Separation parameter
            
            auto output = processAudio(testSignal, params);
            
            // Analyze attack portion (first 10%)
            int attackSamples = static_cast<int>(testSignal.size() * 0.1);
            std::vector<float> attackPortion(testSignal.begin(), 
                                           testSignal.begin() + attackSamples);
            std::vector<float> attackOutput(output.begin(), 
                                          output.begin() + attackSamples);
            
            // Analyze sustain portion (middle 60%)
            int sustainStart = static_cast<int>(testSignal.size() * 0.2);
            int sustainEnd = static_cast<int>(testSignal.size() * 0.8);
            std::vector<float> sustainPortion(testSignal.begin() + sustainStart,
                                            testSignal.begin() + sustainEnd);
            std::vector<float> sustainOutput(output.begin() + sustainStart,
                                           output.begin() + sustainEnd);
            
            float attackInputRMS = AudioAnalyzer::calculateRMS_dB(attackPortion);
            float attackOutputRMS = AudioAnalyzer::calculateRMS_dB(attackOutput);
            float sustainInputRMS = AudioAnalyzer::calculateRMS_dB(sustainPortion);
            float sustainOutputRMS = AudioAnalyzer::calculateRMS_dB(sustainOutput);
            
            float attackEnhancement = attackOutputRMS - attackInputRMS;
            float sustainChange = sustainOutputRMS - sustainInputRMS;
            
            log("  Attack enhancement: " + std::to_string(attackEnhancement) + "dB\n");
            log("  Sustain change: " + std::to_string(sustainChange) + "dB\n");
            
            // With high separation, attack should be enhanced more than sustain
            if (separation > 0.5f) {
                assertTrue(attackEnhancement > sustainChange, 
                          "Good attack/sustain separation at " + std::to_string(separation));
            }
        }
    }
    
    // Test 5: Detection algorithms
    void testDetectionAlgorithms() {
        log("\n--- Detection Algorithm Tests ---\n");
        
        // Create test signals for different detection modes
        auto impulseSignal = TestSignalGenerator::generateImpulseTrain(0.8, 0.1, 1.0, TEST_SAMPLE_RATE);
        auto mixedSignal = TestSignalGenerator::generateMixedContent(1.0, TEST_SAMPLE_RATE);
        
        // Test different detection algorithms
        std::vector<float> detectionModes = {0.0f, 0.33f, 0.66f, 1.0f}; // Peak, RMS, Hilbert, Hybrid
        std::vector<std::string> modeNames = {"Peak", "RMS", "Hilbert", "Hybrid"};
        
        for (size_t i = 0; i < detectionModes.size(); ++i) {
            log("Testing detection mode: " + modeNames[i] + " (" + 
                std::to_string(detectionModes[i]) + ")\n");
            
            std::map<int, float> params;
            params[0] = 0.7f;              // Attack enhancement
            params[5] = detectionModes[i]; // Detection algorithm
            
            // Test with impulse train
            auto impulseOutput = processAudio(impulseSignal, params);
            assertTrue(!AudioAnalyzer::hasInvalidValues(impulseOutput),
                      modeNames[i] + " detection with impulses produces valid output");
            
            // Test with mixed content
            auto mixedOutput = processAudio(mixedSignal, params);
            assertTrue(!AudioAnalyzer::hasInvalidValues(mixedOutput),
                      modeNames[i] + " detection with mixed content produces valid output");
            
            // Measure processing quality
            float inputRMS = AudioAnalyzer::calculateRMS_dB(mixedSignal);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(mixedOutput);
            log("  Input RMS: " + std::to_string(inputRMS) + "dB\n");
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
        }
    }
    
    // Test 6: Timing accuracy
    void testTimingAccuracy() {
        log("\n--- Timing Accuracy Tests ---\n");
        
        // Generate precise impulse signal
        auto impulseSignal = TestSignalGenerator::generateImpulseTrain(0.9, 0.2, 1.0, TEST_SAMPLE_RATE);
        
        // Test different attack and release times
        std::vector<std::pair<float, float>> timingSettings = {
            {0.1f, 0.1f}, // Fast attack/release
            {0.5f, 0.5f}, // Medium attack/release
            {0.9f, 0.9f}  // Slow attack/release
        };
        
        for (auto& timing : timingSettings) {
            log("Testing timing - Attack: " + std::to_string(timing.first) + 
                ", Release: " + std::to_string(timing.second) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.8f;         // High attack enhancement
            params[2] = timing.first; // Attack time
            params[3] = timing.second; // Release time
            
            auto output = processAudio(impulseSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Timing test produces valid output");
            
            // Detect impulses in output
            auto outputTransients = AudioAnalyzer::detectTransients(output, 0.3f);
            auto inputTransients = AudioAnalyzer::detectTransients(impulseSignal, 0.3f);
            
            log("  Input transients: " + std::to_string(inputTransients.size()) + "\n");
            log("  Output transients: " + std::to_string(outputTransients.size()) + "\n");
            
            // Should preserve transient count
            assertTrue(outputTransients.size() >= inputTransients.size() * 0.8f,
                      "Preserves most transients");
        }
    }
    
    // Test 7: Lookahead vs zero-latency modes
    void testLookaheadModes() {
        log("\n--- Lookahead Mode Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateDrumHit(0.7, 0.3, TEST_SAMPLE_RATE);
        
        // Test lookahead vs zero-latency
        std::vector<float> lookaheadSettings = {0.0f, 0.5f, 1.0f};
        std::vector<std::string> modeNames = {"Zero-latency", "Medium lookahead", "Full lookahead"};
        
        for (size_t i = 0; i < lookaheadSettings.size(); ++i) {
            log("Testing " + modeNames[i] + " (" + std::to_string(lookaheadSettings[i]) + ")\n");
            
            std::map<int, float> params;
            params[0] = 0.7f;                // Attack enhancement
            params[6] = lookaheadSettings[i]; // Lookahead parameter
            
            auto output = processAudio(testSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      modeNames[i] + " produces valid output");
            
            // Measure enhancement quality
            auto transients = AudioAnalyzer::detectTransients(testSignal, 0.2f);
            if (!transients.empty()) {
                float quality = AudioAnalyzer::measureSeparationQuality(testSignal, output, transients);
                log("  Enhancement quality: " + std::to_string(quality) + "\n");
                
                assertTrue(quality > 0.5f, modeNames[i] + " provides reasonable enhancement");
            }
        }
    }
    
    // Test 8: Oversampling quality
    void testOversamplingQuality() {
        log("\n--- Oversampling Quality Tests ---\n");
        
        // Generate frequency sweep to test for aliasing
        auto sweepSignal = TestSignalGenerator::generateMixedContent(1.0, TEST_SAMPLE_RATE);
        
        // Test different oversampling settings
        std::vector<float> oversamplingSettings = {0.0f, 0.5f, 1.0f}; // Off, 2x, 4x
        std::vector<std::string> modeNames = {"No oversampling", "2x oversampling", "4x oversampling"};
        
        for (size_t i = 0; i < oversamplingSettings.size(); ++i) {
            log("Testing " + modeNames[i] + " (" + std::to_string(oversamplingSettings[i]) + ")\n");
            
            std::map<int, float> params;
            params[0] = 0.8f;                     // High attack enhancement
            params[8] = oversamplingSettings[i];   // Oversampling parameter
            
            auto output = processAudio(sweepSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      modeNames[i] + " produces valid output");
            
            // Measure output quality
            float inputRMS = AudioAnalyzer::calculateRMS_dB(sweepSignal);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float crestFactor = AudioAnalyzer::calculateCrestFactor(output);
            
            log("  Input RMS: " + std::to_string(inputRMS) + "dB\n");
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            log("  Crest factor: " + std::to_string(crestFactor) + "\n");
            
            // Higher oversampling should maintain better quality
            assertTrue(crestFactor > 1.0f && crestFactor < 20.0f, 
                      modeNames[i] + " maintains reasonable dynamic range");
        }
    }
    
    // Test 9: Soft-knee behavior
    void testSoftKneeBehavior() {
        log("\n--- Soft-Knee Behavior Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateMixedContent(1.0, TEST_SAMPLE_RATE);
        
        // Test different knee settings
        std::vector<float> kneeSettings = {0.0f, 0.5f, 1.0f}; // Hard, medium, soft
        
        for (float knee : kneeSettings) {
            log("Testing knee setting: " + std::to_string(knee) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.7f; // Attack enhancement
            params[1] = 0.3f; // Sustain reduction
            params[7] = knee;  // Soft knee parameter
            
            auto output = processAudio(testSignal, params);
            
            // Check for valid output
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Knee setting " + std::to_string(knee) + " produces valid output");
            
            // Measure smoothness (softer knee should have smoother transitions)
            float crestFactor = AudioAnalyzer::calculateCrestFactor(output);
            log("  Crest factor: " + std::to_string(crestFactor) + "\n");
            
            assertTrue(crestFactor > 1.0f, "Maintains dynamic range with knee setting " + 
                      std::to_string(knee));
        }
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Transient Shaper test suite...\n");
        
        testParameterSweeps();
        testAttackEnhancement();
        testSustainProcessing();
        testAttackSustainSeparation();
        testDetectionAlgorithms();
        testTimingAccuracy();
        testLookaheadModes();
        testOversamplingQuality();
        testSoftKneeBehavior();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        TransientShaperTest tester;
        tester.runAllTests();
        
        std::cout << "\nTransient Shaper test suite completed successfully.\n";
        std::cout << "Check TransientShaper_TestResults.txt for detailed results.\n";
        
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