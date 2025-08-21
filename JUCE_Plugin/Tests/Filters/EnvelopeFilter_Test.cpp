/*
  ==============================================================================
  
    EnvelopeFilter_Test.cpp
    Comprehensive test suite for ENGINE_ENVELOPE_FILTER
    
    Tests for envelope filter characteristics:
    - Envelope follower response timing and accuracy
    - Filter cutoff frequency modulation by envelope
    - Attack and release parameter behavior
    - Sensitivity and range parameter interaction
    - Filter mode operation (LP/HP/BP/Notch)
    - Up/down direction control
    - Signal tracking and envelope extraction
    - Dynamic range and responsiveness
    - Stability across all envelope ranges
    
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
#include <complex>

// Include the engine
#include "../../Source/EnvelopeFilter.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 1.0f;
constexpr float TIMING_TOLERANCE = 0.2f; // 20% tolerance for timing measurements
constexpr float ENVELOPE_TOLERANCE = 0.15f; // 15% tolerance for envelope tracking

// Test signal generators for envelope filter testing
class EnvelopeTestSignalGenerator {
public:
    // Generate burst signal with specific envelope shape
    static std::vector<float> generateBurst(double frequency, double amplitude, 
                                          double burstDuration, double totalDuration,
                                          double attackTime, double releaseTime,
                                          double sampleRate) {
        int totalSamples = static_cast<int>(totalDuration * sampleRate);
        int burstSamples = static_cast<int>(burstDuration * sampleRate);
        int attackSamples = static_cast<int>(attackTime * sampleRate);
        int releaseSamples = static_cast<int>(releaseTime * sampleRate);
        
        std::vector<float> signal(totalSamples, 0.0f);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < burstSamples && i < totalSamples; ++i) {
            // Calculate envelope
            double envelope = 1.0;
            if (i < attackSamples) {
                envelope = static_cast<double>(i) / attackSamples;
            } else if (i > burstSamples - releaseSamples) {
                int releasePos = i - (burstSamples - releaseSamples);
                envelope = 1.0 - static_cast<double>(releasePos) / releaseSamples;
            }
            
            signal[i] = static_cast<float>(amplitude * envelope * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate step envelope (sudden level change)
    static std::vector<float> generateStepEnvelope(double frequency, double lowLevel, 
                                                 double highLevel, double stepTime,
                                                 double totalDuration, double sampleRate) {
        int totalSamples = static_cast<int>(totalDuration * sampleRate);
        int stepSample = static_cast<int>(stepTime * sampleRate);
        
        std::vector<float> signal(totalSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < totalSamples; ++i) {
            double amplitude = (i < stepSample) ? lowLevel : highLevel;
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate amplitude modulated signal
    static std::vector<float> generateAMSignal(double carrierFreq, double modFreq, 
                                             double modDepth, double amplitude,
                                             double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double carrierPhase = 0.0;
        double modPhase = 0.0;
        double carrierIncrement = 2.0 * M_PI * carrierFreq / sampleRate;
        double modIncrement = 2.0 * M_PI * modFreq / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            double modulation = 1.0 + modDepth * std::sin(modPhase);
            signal[i] = static_cast<float>(amplitude * modulation * std::sin(carrierPhase));
            
            carrierPhase += carrierIncrement;
            modPhase += modIncrement;
        }
        
        return signal;
    }
    
    // Generate percussive signal (kick drum-like)
    static std::vector<float> generatePercussive(double startFreq, double endFreq,
                                                double decayTime, double amplitude,
                                                double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        double phase = 0.0;
        double decayRate = std::exp(-1.0 / (decayTime * sampleRate));
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double frequency = endFreq + (startFreq - endFreq) * std::exp(-t / decayTime);
            double envelope = amplitude * std::exp(-t / decayTime);
            
            double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
            signal[i] = static_cast<float>(envelope * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate noise burst
    static std::vector<float> generateNoiseBurst(double amplitude, double burstStart,
                                                double burstDuration, double totalDuration,
                                                double sampleRate, uint32_t seed = 456) {
        int totalSamples = static_cast<int>(totalDuration * sampleRate);
        int burstStartSample = static_cast<int>(burstStart * sampleRate);
        int burstSamples = static_cast<int>(burstDuration * sampleRate);
        
        std::vector<float> signal(totalSamples, 0.0f);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = burstStartSample; i < burstStartSample + burstSamples && i < totalSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
    
    // Generate swept envelope (changing modulation rate)
    static std::vector<float> generateSweptEnvelope(double carrierFreq, double startModFreq,
                                                   double endModFreq, double modDepth,
                                                   double amplitude, double duration,
                                                   double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double carrierPhase = 0.0;
        double modPhase = 0.0;
        double carrierIncrement = 2.0 * M_PI * carrierFreq / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / duration / numSamples;
            double modFreq = startModFreq + (endModFreq - startModFreq) * t;
            double modIncrement = 2.0 * M_PI * modFreq / sampleRate;
            
            double modulation = 1.0 + modDepth * std::sin(modPhase);
            signal[i] = static_cast<float>(amplitude * modulation * std::sin(carrierPhase));
            
            carrierPhase += carrierIncrement;
            modPhase += modIncrement;
        }
        
        return signal;
    }
};

// Envelope analysis tools
class EnvelopeAnalyzer {
public:
    struct EnvelopeMetrics {
        double attackTime;
        double releaseTime;
        double peakLevel;
        double sustainLevel;
        double dynamicRange;
        std::vector<double> envelopeData;
    };
    
    // Extract envelope from signal
    static std::vector<double> extractEnvelope(const std::vector<float>& signal, 
                                             double sampleRate, 
                                             double windowMs = 5.0) {
        int windowSize = static_cast<int>(windowMs * 0.001 * sampleRate);
        std::vector<double> envelope;
        
        for (int i = 0; i < static_cast<int>(signal.size()) - windowSize; i += windowSize / 2) {
            double rms = 0.0;
            for (int j = i; j < i + windowSize; ++j) {
                rms += signal[j] * signal[j];
            }
            envelope.push_back(std::sqrt(rms / windowSize));
        }
        
        return envelope;
    }
    
    // Analyze envelope characteristics
    static EnvelopeMetrics analyzeEnvelope(const std::vector<float>& signal, 
                                          double sampleRate,
                                          double triggerThreshold = 0.1) {
        EnvelopeMetrics metrics;
        metrics.envelopeData = extractEnvelope(signal, sampleRate);
        
        if (metrics.envelopeData.empty()) {
            metrics.attackTime = metrics.releaseTime = 0.0;
            metrics.peakLevel = metrics.sustainLevel = metrics.dynamicRange = 0.0;
            return metrics;
        }
        
        // Find peak
        auto peakIt = std::max_element(metrics.envelopeData.begin(), metrics.envelopeData.end());
        metrics.peakLevel = *peakIt;
        int peakIndex = peakIt - metrics.envelopeData.begin();
        
        // Find attack time (10% to 90% of peak)
        double attackStart = metrics.peakLevel * 0.1;
        double attackEnd = metrics.peakLevel * 0.9;
        int attackStartIndex = -1, attackEndIndex = -1;
        
        for (int i = 0; i < peakIndex; ++i) {
            if (attackStartIndex == -1 && metrics.envelopeData[i] >= attackStart) {
                attackStartIndex = i;
            }
            if (metrics.envelopeData[i] >= attackEnd) {
                attackEndIndex = i;
            }
        }
        
        if (attackStartIndex != -1 && attackEndIndex != -1) {
            double windowDuration = 2.5 * 0.001; // 2.5ms window overlap
            metrics.attackTime = (attackEndIndex - attackStartIndex) * windowDuration;
        } else {
            metrics.attackTime = 0.0;
        }
        
        // Find release time (90% to 10% of peak after peak)
        double releaseStart = metrics.peakLevel * 0.9;
        double releaseEnd = metrics.peakLevel * 0.1;
        int releaseStartIndex = -1, releaseEndIndex = -1;
        
        for (int i = peakIndex; i < static_cast<int>(metrics.envelopeData.size()); ++i) {
            if (releaseStartIndex == -1 && metrics.envelopeData[i] <= releaseStart) {
                releaseStartIndex = i;
            }
            if (metrics.envelopeData[i] <= releaseEnd) {
                releaseEndIndex = i;
                break;
            }
        }
        
        if (releaseStartIndex != -1 && releaseEndIndex != -1) {
            double windowDuration = 2.5 * 0.001;
            metrics.releaseTime = (releaseEndIndex - releaseStartIndex) * windowDuration;
        } else {
            metrics.releaseTime = 0.0;
        }
        
        // Calculate sustain level (average level in middle portion)
        int sustainStart = peakIndex + static_cast<int>(metrics.envelopeData.size() * 0.1);
        int sustainEnd = static_cast<int>(metrics.envelopeData.size() * 0.8);
        
        if (sustainStart < sustainEnd) {
            double sustainSum = 0.0;
            for (int i = sustainStart; i < sustainEnd; ++i) {
                sustainSum += metrics.envelopeData[i];
            }
            metrics.sustainLevel = sustainSum / (sustainEnd - sustainStart);
        } else {
            metrics.sustainLevel = metrics.peakLevel;
        }
        
        // Calculate dynamic range
        double minLevel = *std::min_element(metrics.envelopeData.begin(), metrics.envelopeData.end());
        metrics.dynamicRange = 20.0 * std::log10(metrics.peakLevel / (minLevel + 1e-15));
        
        return metrics;
    }
    
    // Measure envelope following accuracy
    static double measureFollowingAccuracy(const std::vector<double>& inputEnvelope,
                                         const std::vector<double>& outputEnvelope) {
        if (inputEnvelope.size() != outputEnvelope.size() || inputEnvelope.empty()) {
            return 0.0;
        }
        
        double correlation = 0.0;
        double inputMean = 0.0, outputMean = 0.0;
        
        // Calculate means
        for (size_t i = 0; i < inputEnvelope.size(); ++i) {
            inputMean += inputEnvelope[i];
            outputMean += outputEnvelope[i];
        }
        inputMean /= inputEnvelope.size();
        outputMean /= outputEnvelope.size();
        
        // Calculate correlation coefficient
        double numerator = 0.0, inputVar = 0.0, outputVar = 0.0;
        for (size_t i = 0; i < inputEnvelope.size(); ++i) {
            double inputDev = inputEnvelope[i] - inputMean;
            double outputDev = outputEnvelope[i] - outputMean;
            
            numerator += inputDev * outputDev;
            inputVar += inputDev * inputDev;
            outputVar += outputDev * outputDev;
        }
        
        double denominator = std::sqrt(inputVar * outputVar);
        return (denominator > 1e-15) ? numerator / denominator : 0.0;
    }
};

// Main test suite for Envelope Filter
class EnvelopeFilterTestSuite {
private:
    std::unique_ptr<EnvelopeFilter> filter;
    std::ofstream logFile;
    
public:
    EnvelopeFilterTestSuite() : filter(std::make_unique<EnvelopeFilter>()) {
        logFile.open("EnvelopeFilter_TestResults.txt");
        logFile << "=== Envelope Filter Test Results ===" << std::endl;
        logFile << "Test started at: " << getCurrentTime() << std::endl << std::endl;
    }
    
    ~EnvelopeFilterTestSuite() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void runAllTests() {
        logFile << "Starting comprehensive Envelope Filter test suite..." << std::endl;
        
        // Initialize filter
        filter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        filter->reset();
        
        // Run test categories
        testBasicFunctionality();
        testEnvelopeFollowing();
        testAttackReleaseTiming();
        testSensitivityResponse();
        testRangeControl();
        testFilterModeOperation();
        testDirectionControl();
        testSignalTracking();
        testDynamicResponse();
        testParameterInteraction();
        testStabilityLimits();
        testRealWorldSignals();
        
        logFile << "\n=== Envelope Filter Test Suite Complete ===" << std::endl;
        std::cout << "Envelope Filter test results written to EnvelopeFilter_TestResults.txt" << std::endl;
    }
    
private:
    void testBasicFunctionality() {
        logFile << "\n--- Basic Functionality Tests ---" << std::endl;
        
        // Test parameter count
        int numParams = filter->getNumParameters();
        logFile << "Number of parameters: " << numParams << std::endl;
        assert(numParams == 8);
        
        // Test parameter names
        for (int i = 0; i < numParams; ++i) {
            juce::String paramName = filter->getParameterName(i);
            logFile << "Parameter " << i << ": " << paramName << std::endl;
        }
        
        // Test engine name
        juce::String engineName = filter->getName();
        logFile << "Engine name: " << engineName << std::endl;
        assert(engineName == "Envelope Filter");
        
        logFile << "✓ Basic functionality tests passed" << std::endl;
    }
    
    void testEnvelopeFollowing() {
        logFile << "\n--- Envelope Following Tests ---" << std::endl;
        
        // Test envelope following with different input signals
        std::vector<std::string> signalTypes = {"Burst", "AM Signal", "Percussive", "Noise Burst"};
        
        std::vector<std::vector<float>> testSignals = {
            EnvelopeTestSignalGenerator::generateBurst(1000.0, 0.5, 1.0, 3.0, 0.1, 0.3, TEST_SAMPLE_RATE),
            EnvelopeTestSignalGenerator::generateAMSignal(1000.0, 5.0, 0.8, 0.3, 3.0, TEST_SAMPLE_RATE),
            EnvelopeTestSignalGenerator::generatePercussive(200.0, 60.0, 0.5, 0.6, 3.0, TEST_SAMPLE_RATE),
            EnvelopeTestSignalGenerator::generateNoiseBurst(0.4, 0.5, 1.0, 3.0, TEST_SAMPLE_RATE)
        };
        
        for (size_t i = 0; i < signalTypes.size(); ++i) {
            logFile << "\nTesting envelope following with " << signalTypes[i] << ":" << std::endl;
            
            // Set up envelope filter for following test
            std::map<int, float> params = {
                {0, 0.7f}, // High sensitivity
                {1, 0.3f}, // Medium attack
                {2, 0.5f}, // Medium release
                {3, 0.8f}, // Wide range
                {4, 0.6f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Process signal
            juce::AudioBuffer<float> buffer(2, testSignals[i].size());
            for (size_t j = 0; j < testSignals[i].size(); ++j) {
                buffer.setSample(0, j, testSignals[i][j]);
                buffer.setSample(1, j, testSignals[i][j]);
            }
            
            filter->process(buffer);
            
            // Extract input and output envelopes
            auto inputEnvelope = EnvelopeAnalyzer::extractEnvelope(testSignals[i], TEST_SAMPLE_RATE);
            
            std::vector<float> outputSignal(testSignals[i].size());
            for (size_t j = 0; j < testSignals[i].size(); ++j) {
                outputSignal[j] = buffer.getSample(0, j);
            }
            auto outputEnvelope = EnvelopeAnalyzer::extractEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            // Measure following accuracy
            double accuracy = EnvelopeAnalyzer::measureFollowingAccuracy(inputEnvelope, outputEnvelope);
            
            // Analyze envelope characteristics
            auto inputMetrics = EnvelopeAnalyzer::analyzeEnvelope(testSignals[i], TEST_SAMPLE_RATE);
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Input envelope - Peak: " << inputMetrics.peakLevel 
                   << ", Attack: " << inputMetrics.attackTime * 1000.0 << " ms"
                   << ", Release: " << inputMetrics.releaseTime * 1000.0 << " ms" << std::endl;
            
            logFile << "  Output envelope - Peak: " << outputMetrics.peakLevel 
                   << ", Attack: " << outputMetrics.attackTime * 1000.0 << " ms"
                   << ", Release: " << outputMetrics.releaseTime * 1000.0 << " ms" << std::endl;
            
            logFile << "  Following accuracy (correlation): " << accuracy << std::endl;
            
            // Verify envelope following
            assert(accuracy > 0.3); // Reasonable correlation
            assert(outputMetrics.peakLevel > 0.01); // Should produce output
            assert(outputMetrics.dynamicRange > 10.0); // Should have dynamic response
        }
        
        logFile << "✓ Envelope following tests passed" << std::endl;
    }
    
    void testAttackReleaseTiming() {
        logFile << "\n--- Attack/Release Timing Tests ---" << std::endl;
        
        // Test different attack/release settings
        std::vector<std::pair<float, float>> timingSettings = {
            {0.1f, 0.1f}, // Fast attack, fast release
            {0.1f, 0.9f}, // Fast attack, slow release
            {0.9f, 0.1f}, // Slow attack, fast release
            {0.9f, 0.9f}  // Slow attack, slow release
        };
        
        for (const auto& setting : timingSettings) {
            float attackParam = setting.first;
            float releaseParam = setting.second;
            
            logFile << "\nTesting Attack=" << attackParam 
                   << ", Release=" << releaseParam << ":" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.6f}, // Moderate sensitivity
                {1, attackParam}, // Variable attack
                {2, releaseParam}, // Variable release
                {3, 0.7f}, // Good range
                {4, 0.5f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Generate step envelope signal for clear timing measurement
            auto stepSignal = EnvelopeTestSignalGenerator::generateStepEnvelope(
                1000.0, 0.1, 0.7, 1.0, 4.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, stepSignal.size());
            for (size_t i = 0; i < stepSignal.size(); ++i) {
                buffer.setSample(0, i, stepSignal[i]);
                buffer.setSample(1, i, stepSignal[i]);
            }
            
            filter->process(buffer);
            
            // Extract output signal
            std::vector<float> outputSignal(stepSignal.size());
            for (size_t i = 0; i < stepSignal.size(); ++i) {
                outputSignal[i] = buffer.getSample(0, i);
            }
            
            // Analyze timing
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Measured attack time: " << outputMetrics.attackTime * 1000.0 << " ms" << std::endl;
            logFile << "  Measured release time: " << outputMetrics.releaseTime * 1000.0 << " ms" << std::endl;
            
            // Verify timing relationships
            // Fast settings should be faster than slow settings
            if (attackParam < 0.5f) {
                assert(outputMetrics.attackTime < 0.1); // Fast attack should be < 100ms
            } else {
                assert(outputMetrics.attackTime > 0.05); // Slow attack should be > 50ms
            }
            
            // Note: Release timing is harder to measure precisely with this method
            // but we can verify the filter responds to parameter changes
            assert(outputMetrics.releaseTime > 0.0); // Should have measurable release
        }
        
        logFile << "✓ Attack/release timing tests passed" << std::endl;
    }
    
    void testSensitivityResponse() {
        logFile << "\n--- Sensitivity Response Tests ---" << std::endl;
        
        std::vector<float> sensitivityValues = {0.1f, 0.3f, 0.6f, 0.9f};
        
        for (float sensitivity : sensitivityValues) {
            logFile << "\nTesting sensitivity: " << sensitivity << std::endl;
            
            std::map<int, float> params = {
                {0, sensitivity}, // Variable sensitivity
                {1, 0.2f}, // Fast attack
                {2, 0.3f}, // Fast release
                {3, 0.8f}, // Wide range
                {4, 0.5f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with different input levels
            std::vector<double> inputLevels = {0.1, 0.3, 0.6, 0.9};
            
            for (double level : inputLevels) {
                auto testSignal = EnvelopeTestSignalGenerator::generateBurst(
                    800.0, level, 0.5, 2.0, 0.05, 0.2, TEST_SAMPLE_RATE);
                
                juce::AudioBuffer<float> buffer(2, testSignal.size());
                for (size_t i = 0; i < testSignal.size(); ++i) {
                    buffer.setSample(0, i, testSignal[i]);
                    buffer.setSample(1, i, testSignal[i]);
                }
                
                filter->process(buffer);
                
                // Measure response
                std::vector<float> outputSignal(testSignal.size());
                for (size_t i = 0; i < testSignal.size(); ++i) {
                    outputSignal[i] = buffer.getSample(0, i);
                }
                
                auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
                
                logFile << "    Input level " << level << ": Output peak " 
                       << outputMetrics.peakLevel << ", Dynamic range " 
                       << outputMetrics.dynamicRange << " dB" << std::endl;
                
                // Higher sensitivity should produce stronger response to lower levels
                if (sensitivity > 0.7f) {
                    assert(outputMetrics.peakLevel > 0.05); // Should respond to low levels
                    assert(outputMetrics.dynamicRange > 15.0); // Good dynamic range
                }
            }
        }
        
        logFile << "✓ Sensitivity response tests passed" << std::endl;
    }
    
    void testRangeControl() {
        logFile << "\n--- Range Control Tests ---" << std::endl;
        
        std::vector<float> rangeValues = {0.2f, 0.5f, 0.8f};
        std::vector<std::string> rangeNames = {"Narrow", "Medium", "Wide"};
        
        for (size_t i = 0; i < rangeValues.size(); ++i) {
            logFile << "\nTesting " << rangeNames[i] << " range (" << rangeValues[i] << "):" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.6f}, // Moderate sensitivity
                {1, 0.2f}, // Fast attack
                {2, 0.3f}, // Fast release
                {3, rangeValues[i]}, // Variable range
                {4, 0.5f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with varying amplitude signal
            auto testSignal = EnvelopeTestSignalGenerator::generateAMSignal(
                1000.0, 3.0, 0.9, 0.5, 3.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                buffer.setSample(0, j, testSignal[j]);
                buffer.setSample(1, j, testSignal[j]);
            }
            
            filter->process(buffer);
            
            // Measure frequency modulation range
            std::vector<float> outputSignal(testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                outputSignal[j] = buffer.getSample(0, j);
            }
            
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Dynamic range: " << outputMetrics.dynamicRange << " dB" << std::endl;
            logFile << "  Peak/sustain ratio: " << outputMetrics.peakLevel / (outputMetrics.sustainLevel + 1e-15) << std::endl;
            
            // Wider range should produce more dramatic modulation
            if (i == 2) { // Wide range
                assert(outputMetrics.dynamicRange > 20.0); // Should have wide dynamic range
            } else if (i == 0) { // Narrow range
                assert(outputMetrics.dynamicRange < 30.0); // Should be more limited
            }
        }
        
        logFile << "✓ Range control tests passed" << std::endl;
    }
    
    void testFilterModeOperation() {
        logFile << "\n--- Filter Mode Operation Tests ---" << std::endl;
        
        // Test different filter modes
        std::vector<float> modeValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        std::vector<std::string> modeNames = {"Lowpass", "Highpass", "Bandpass", "Notch", "Allpass"};
        
        for (size_t i = 0; i < modeValues.size(); ++i) {
            logFile << "\nTesting " << modeNames[i] << " mode:" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.6f}, // Moderate sensitivity
                {1, 0.3f}, // Medium attack
                {2, 0.4f}, // Medium release
                {3, 0.7f}, // Good range
                {4, 0.6f}, // Moderate resonance
                {5, modeValues[i]}, // Variable filter type
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with harmonic-rich signal that will be modulated
            auto testSignal = EnvelopeTestSignalGenerator::generateBurst(
                500.0, 0.4, 1.0, 2.5, 0.1, 0.3, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                buffer.setSample(0, j, testSignal[j]);
                buffer.setSample(1, j, testSignal[j]);
            }
            
            filter->process(buffer);
            
            // Analyze frequency characteristics
            std::vector<float> outputSignal(testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                outputSignal[j] = buffer.getSample(0, j);
            }
            
            // Simple spectral analysis
            double lowFreqEnergy = measureFrequencyEnergy(outputSignal, 100.0, 500.0, TEST_SAMPLE_RATE);
            double midFreqEnergy = measureFrequencyEnergy(outputSignal, 500.0, 2000.0, TEST_SAMPLE_RATE);
            double highFreqEnergy = measureFrequencyEnergy(outputSignal, 2000.0, 8000.0, TEST_SAMPLE_RATE);
            
            logFile << "  Low freq energy: " << lowFreqEnergy << std::endl;
            logFile << "  Mid freq energy: " << midFreqEnergy << std::endl;
            logFile << "  High freq energy: " << highFreqEnergy << std::endl;
            
            // Verify mode-appropriate frequency characteristics
            switch (i) {
                case 0: // Lowpass
                    assert(lowFreqEnergy > highFreqEnergy); // More low than high
                    break;
                case 1: // Highpass
                    assert(highFreqEnergy > lowFreqEnergy); // More high than low
                    break;
                case 2: // Bandpass
                    assert(midFreqEnergy > lowFreqEnergy * 0.5); // Mid should be prominent
                    assert(midFreqEnergy > highFreqEnergy * 0.5);
                    break;
                // Note: Notch and Allpass are harder to verify with this simple method
            }
            
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            assert(outputMetrics.peakLevel > 0.01); // Should produce output in all modes
        }
        
        logFile << "✓ Filter mode operation tests passed" << std::endl;
    }
    
    void testDirectionControl() {
        logFile << "\n--- Direction Control Tests ---" << std::endl;
        
        // Test up vs down direction
        std::vector<float> directionValues = {0.0f, 1.0f};
        std::vector<std::string> directionNames = {"Up", "Down"};
        
        for (size_t i = 0; i < directionValues.size(); ++i) {
            logFile << "\nTesting " << directionNames[i] << " direction:" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.7f}, // High sensitivity
                {1, 0.2f}, // Fast attack
                {2, 0.4f}, // Medium release
                {3, 0.8f}, // Wide range
                {4, 0.5f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, directionValues[i]}, // Variable direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Generate signal with clear envelope
            auto testSignal = EnvelopeTestSignalGenerator::generateBurst(
                800.0, 0.6, 1.5, 3.0, 0.1, 0.5, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                buffer.setSample(0, j, testSignal[j]);
                buffer.setSample(1, j, testSignal[j]);
            }
            
            filter->process(buffer);
            
            // Extract and analyze output
            std::vector<float> outputSignal(testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                outputSignal[j] = buffer.getSample(0, j);
            }
            
            auto inputMetrics = EnvelopeAnalyzer::analyzeEnvelope(testSignal, TEST_SAMPLE_RATE);
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            // Analyze correlation with input envelope
            auto inputEnvelope = EnvelopeAnalyzer::extractEnvelope(testSignal, TEST_SAMPLE_RATE);
            auto outputEnvelope = EnvelopeAnalyzer::extractEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            double correlation = EnvelopeAnalyzer::measureFollowingAccuracy(inputEnvelope, outputEnvelope);
            
            logFile << "  Input peak: " << inputMetrics.peakLevel << std::endl;
            logFile << "  Output peak: " << outputMetrics.peakLevel << std::endl;
            logFile << "  Envelope correlation: " << correlation << std::endl;
            
            // Both directions should track the envelope, but may behave differently
            assert(std::abs(correlation) > 0.2); // Should correlate with input envelope
            assert(outputMetrics.peakLevel > 0.05); // Should produce reasonable output
            
            // Down direction might show inverse correlation
            if (i == 1 && correlation < 0) {
                logFile << "  Detected inverse correlation (down direction)" << std::endl;
            }
        }
        
        logFile << "✓ Direction control tests passed" << std::endl;
    }
    
    void testSignalTracking() {
        logFile << "\n--- Signal Tracking Tests ---" << std::endl;
        
        // Test tracking with different signal characteristics
        std::vector<std::string> signalTypes = {"Slow AM", "Fast AM", "Percussive", "Swept"};
        
        std::vector<std::vector<float>> testSignals = {
            EnvelopeTestSignalGenerator::generateAMSignal(1000.0, 1.0, 0.8, 0.4, 4.0, TEST_SAMPLE_RATE),
            EnvelopeTestSignalGenerator::generateAMSignal(1000.0, 10.0, 0.8, 0.4, 4.0, TEST_SAMPLE_RATE),
            EnvelopeTestSignalGenerator::generatePercussive(150.0, 50.0, 0.8, 0.6, 4.0, TEST_SAMPLE_RATE),
            EnvelopeTestSignalGenerator::generateSweptEnvelope(1000.0, 0.5, 8.0, 0.9, 0.4, 4.0, TEST_SAMPLE_RATE)
        };
        
        for (size_t i = 0; i < signalTypes.size(); ++i) {
            logFile << "\nTesting tracking with " << signalTypes[i] << ":" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.6f}, // Moderate sensitivity
                {1, 0.15f}, // Fast attack for tracking
                {2, 0.25f}, // Fast release for tracking
                {3, 0.8f}, // Wide range
                {4, 0.6f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, testSignals[i].size());
            for (size_t j = 0; j < testSignals[i].size(); ++j) {
                buffer.setSample(0, j, testSignals[i][j]);
                buffer.setSample(1, j, testSignals[i][j]);
            }
            
            filter->process(buffer);
            
            // Extract envelopes for comparison
            auto inputEnvelope = EnvelopeAnalyzer::extractEnvelope(testSignals[i], TEST_SAMPLE_RATE);
            
            std::vector<float> outputSignal(testSignals[i].size());
            for (size_t j = 0; j < testSignals[i].size(); ++j) {
                outputSignal[j] = buffer.getSample(0, j);
            }
            auto outputEnvelope = EnvelopeAnalyzer::extractEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            // Measure tracking accuracy
            double correlation = EnvelopeAnalyzer::measureFollowingAccuracy(inputEnvelope, outputEnvelope);
            
            auto inputMetrics = EnvelopeAnalyzer::analyzeEnvelope(testSignals[i], TEST_SAMPLE_RATE);
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Tracking correlation: " << correlation << std::endl;
            logFile << "  Input dynamic range: " << inputMetrics.dynamicRange << " dB" << std::endl;
            logFile << "  Output dynamic range: " << outputMetrics.dynamicRange << " dB" << std::endl;
            
            // Verify tracking performance
            assert(correlation > 0.2); // Should track input envelope
            assert(outputMetrics.dynamicRange > 10.0); // Should have dynamic response
            assert(outputMetrics.peakLevel > 0.02); // Should produce reasonable output
        }
        
        logFile << "✓ Signal tracking tests passed" << std::endl;
    }
    
    void testDynamicResponse() {
        logFile << "\n--- Dynamic Response Tests ---" << std::endl;
        
        // Test response to different dynamic ranges
        std::vector<std::pair<double, double>> dynamicRanges = {
            {0.1, 0.3}, // Small dynamic range
            {0.05, 0.6}, // Medium dynamic range
            {0.01, 0.9}  // Large dynamic range
        };
        
        for (const auto& range : dynamicRanges) {
            double minLevel = range.first;
            double maxLevel = range.second;
            double dynamicRange_dB = 20.0 * std::log10(maxLevel / minLevel);
            
            logFile << "\nTesting dynamic range: " << dynamicRange_dB << " dB" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.5f}, // Moderate sensitivity
                {1, 0.2f}, // Fast attack
                {2, 0.3f}, // Fast release
                {3, 0.8f}, // Wide range
                {4, 0.5f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Generate signal with specific dynamic range
            auto testSignal = EnvelopeTestSignalGenerator::generateStepEnvelope(
                1000.0, minLevel, maxLevel, 1.0, 3.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Analyze dynamic response
            std::vector<float> outputSignal(testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                outputSignal[i] = buffer.getSample(0, i);
            }
            
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Input dynamic range: " << dynamicRange_dB << " dB" << std::endl;
            logFile << "  Output dynamic range: " << outputMetrics.dynamicRange << " dB" << std::endl;
            logFile << "  Dynamic compression ratio: " << dynamicRange_dB / (outputMetrics.dynamicRange + 1e-15) << std::endl;
            
            // Verify dynamic response scales appropriately
            assert(outputMetrics.dynamicRange > 5.0); // Should maintain some dynamics
            assert(outputMetrics.peakLevel > outputMetrics.sustainLevel * 1.5); // Should have level variation
            
            // Larger input dynamic ranges should generally produce larger output dynamic ranges
            if (dynamicRange_dB > 30.0) {
                assert(outputMetrics.dynamicRange > 15.0);
            }
        }
        
        logFile << "✓ Dynamic response tests passed" << std::endl;
    }
    
    void testParameterInteraction() {
        logFile << "\n--- Parameter Interaction Tests ---" << std::endl;
        
        // Test key parameter interactions
        auto testSignal = EnvelopeTestSignalGenerator::generateAMSignal(
            800.0, 4.0, 0.7, 0.4, 3.0, TEST_SAMPLE_RATE);
        
        // Test sensitivity vs range interaction
        std::vector<std::pair<float, float>> sensitivityRangePairs = {
            {0.3f, 0.3f}, // Low sensitivity, narrow range
            {0.3f, 0.8f}, // Low sensitivity, wide range
            {0.8f, 0.3f}, // High sensitivity, narrow range
            {0.8f, 0.8f}  // High sensitivity, wide range
        };
        
        for (const auto& pair : sensitivityRangePairs) {
            float sensitivity = pair.first;
            float range = pair.second;
            
            logFile << "\nTesting Sensitivity=" << sensitivity 
                   << ", Range=" << range << ":" << std::endl;
            
            std::map<int, float> params = {
                {0, sensitivity}, // Variable sensitivity
                {1, 0.2f}, // Fast attack
                {2, 0.3f}, // Fast release
                {3, range}, // Variable range
                {4, 0.5f}, // Moderate resonance
                {5, 0.0f}, // Lowpass mode
                {6, 0.0f}, // Up direction
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Analyze combined effect
            std::vector<float> outputSignal(testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                outputSignal[i] = buffer.getSample(0, i);
            }
            
            auto outputMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Output dynamic range: " << outputMetrics.dynamicRange << " dB" << std::endl;
            logFile << "  Peak level: " << outputMetrics.peakLevel << std::endl;
            
            // High sensitivity + wide range should produce maximum effect
            if (sensitivity > 0.7f && range > 0.7f) {
                assert(outputMetrics.dynamicRange > 20.0);
                assert(outputMetrics.peakLevel > 0.1);
            }
            
            // Low sensitivity + narrow range should produce minimal effect
            if (sensitivity < 0.4f && range < 0.4f) {
                assert(outputMetrics.dynamicRange < 35.0); // More limited
            }
        }
        
        logFile << "✓ Parameter interaction tests passed" << std::endl;
    }
    
    void testStabilityLimits() {
        logFile << "\n--- Stability and Limits Tests ---" << std::endl;
        
        // Test extreme parameter combinations
        std::vector<std::map<int, float>> extremeSettings = {
            // All maximum
            {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}, 
             {4, 1.0f}, {5, 1.0f}, {6, 1.0f}, {7, 1.0f}},
            
            // All minimum (except mix)
            {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, 
             {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 1.0f}},
            
            // High sensitivity, fast times
            {{0, 1.0f}, {1, 0.0f}, {2, 0.0f}, {3, 1.0f}, 
             {4, 0.8f}, {5, 0.5f}, {6, 0.5f}, {7, 1.0f}}
        };
        
        for (size_t i = 0; i < extremeSettings.size(); ++i) {
            logFile << "\nTesting extreme setting " << i + 1 << ":" << std::endl;
            
            filter->updateParameters(extremeSettings[i]);
            
            // Test with high-level signal
            auto testSignal = EnvelopeTestSignalGenerator::generateBurst(
                1000.0, 0.8, 1.0, 2.0, 0.05, 0.2, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                buffer.setSample(0, j, testSignal[j]);
                buffer.setSample(1, j, testSignal[j]);
            }
            
            filter->process(buffer);
            
            // Check for stability
            bool stable = true;
            double maxOutput = 0.0;
            
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                float sample = buffer.getSample(0, j);
                
                if (std::isnan(sample) || std::isinf(sample)) {
                    stable = false;
                    break;
                }
                
                maxOutput = std::max(maxOutput, static_cast<double>(std::abs(sample)));
            }
            
            logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
            logFile << "  Max output: " << maxOutput << std::endl;
            
            assert(stable);
            assert(maxOutput < 100.0); // Should not explode
        }
        
        logFile << "✓ Stability and limits tests passed" << std::endl;
    }
    
    void testRealWorldSignals() {
        logFile << "\n--- Real World Signals Tests ---" << std::endl;
        
        // Test with realistic musical signals
        std::map<int, float> musicalParams = {
            {0, 0.6f}, // Good sensitivity
            {1, 0.25f}, // Medium attack
            {2, 0.4f}, // Medium release
            {3, 0.7f}, // Good range
            {4, 0.6f}, // Musical resonance
            {5, 0.0f}, // Lowpass mode
            {6, 0.0f}, // Up direction
            {7, 0.8f}  // Mostly wet but keep some dry
        };
        filter->updateParameters(musicalParams);
        
        // Test with guitar-like percussive signal
        logFile << "\nTesting with guitar-like signal:" << std::endl;
        auto guitarSignal = EnvelopeTestSignalGenerator::generatePercussive(
            330.0, 82.5, 1.5, 0.5, 4.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, guitarSignal.size());
        for (size_t i = 0; i < guitarSignal.size(); ++i) {
            buffer.setSample(0, i, guitarSignal[i]);
            buffer.setSample(1, i, guitarSignal[i]);
        }
        
        filter->process(buffer);
        
        std::vector<float> outputSignal(guitarSignal.size());
        for (size_t i = 0; i < guitarSignal.size(); ++i) {
            outputSignal[i] = buffer.getSample(0, i);
        }
        
        auto guitarMetrics = EnvelopeAnalyzer::analyzeEnvelope(outputSignal, TEST_SAMPLE_RATE);
        
        logFile << "  Guitar-like signal results:" << std::endl;
        logFile << "    Attack time: " << guitarMetrics.attackTime * 1000.0 << " ms" << std::endl;
        logFile << "    Release time: " << guitarMetrics.releaseTime * 1000.0 << " ms" << std::endl;
        logFile << "    Dynamic range: " << guitarMetrics.dynamicRange << " dB" << std::endl;
        
        // Test with synthesizer-like signal
        logFile << "\nTesting with synthesizer-like signal:" << std::endl;
        auto synthSignal = EnvelopeTestSignalGenerator::generateBurst(
            440.0, 0.6, 2.0, 4.0, 0.2, 1.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> synthBuffer(2, synthSignal.size());
        for (size_t i = 0; i < synthSignal.size(); ++i) {
            synthBuffer.setSample(0, i, synthSignal[i]);
            synthBuffer.setSample(1, i, synthSignal[i]);
        }
        
        filter->process(synthBuffer);
        
        std::vector<float> synthOutputSignal(synthSignal.size());
        for (size_t i = 0; i < synthSignal.size(); ++i) {
            synthOutputSignal[i] = synthBuffer.getSample(0, i);
        }
        
        auto synthMetrics = EnvelopeAnalyzer::analyzeEnvelope(synthOutputSignal, TEST_SAMPLE_RATE);
        
        logFile << "  Synthesizer-like signal results:" << std::endl;
        logFile << "    Attack time: " << synthMetrics.attackTime * 1000.0 << " ms" << std::endl;
        logFile << "    Release time: " << synthMetrics.releaseTime * 1000.0 << " ms" << std::endl;
        logFile << "    Dynamic range: " << synthMetrics.dynamicRange << " dB" << std::endl;
        
        // Verify musical performance
        assert(guitarMetrics.peakLevel > 0.05); // Should respond to guitar signal
        assert(guitarMetrics.dynamicRange > 15.0); // Should have good dynamics
        assert(synthMetrics.peakLevel > 0.05); // Should respond to synth signal
        assert(synthMetrics.dynamicRange > 15.0); // Should have good dynamics
        
        logFile << "✓ Real world signals tests passed" << std::endl;
    }
    
    // Helper function to measure energy in frequency range
    double measureFrequencyEnergy(const std::vector<float>& signal, 
                                double lowFreq, double highFreq, double sampleRate) {
        double energy = 0.0;
        int numFreqs = 20;
        
        for (int i = 0; i < numFreqs; ++i) {
            double freq = lowFreq + (highFreq - lowFreq) * i / (numFreqs - 1);
            double real = 0.0, imag = 0.0;
            double omega = 2.0 * M_PI * freq / sampleRate;
            
            for (size_t j = 0; j < signal.size(); ++j) {
                double phase = omega * j;
                real += signal[j] * std::cos(phase);
                imag += signal[j] * std::sin(phase);
            }
            
            energy += (real * real + imag * imag) / (signal.size() * signal.size());
        }
        
        return energy / numFreqs;
    }
    
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// Main test runner
int main() {
    std::cout << "Starting Envelope Filter comprehensive test suite..." << std::endl;
    
    try {
        EnvelopeFilterTestSuite testSuite;
        testSuite.runAllTests();
        
        std::cout << "All tests completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}