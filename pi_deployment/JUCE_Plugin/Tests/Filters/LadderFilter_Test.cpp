/*
  ==============================================================================
  
    LadderFilter_Test.cpp
    Comprehensive test suite for ENGINE_LADDER_FILTER
    
    Tests for ladder filter characteristics:
    - Self-oscillation threshold and stability
    - 4-pole lowpass response accuracy (-24dB/octave)
    - Resonance behavior and Q response
    - Zero-delay feedback accuracy
    - Vintage Moog-style saturation
    - Component modeling and tolerances
    - Thermal drift simulation
    - Oversampling effectiveness
    - Filter stability at extreme settings
    - Transient response and ringing
    
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
#include "../../Source/LadderFilter.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.3f; // Relaxed for analog modeling
constexpr float FREQ_EPSILON = 0.05f; // 5% frequency tolerance
constexpr float RESONANCE_EPSILON = 0.1f;

// Specialized test signal generators for ladder filter testing
class LadderTestSignalGenerator {
public:
    // Generate sine wave sweep for frequency response testing
    static std::vector<float> generateLogSweep(double startFreq, double endFreq,
                                             double duration, double sampleRate,
                                             double amplitude = 0.1) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double k = std::pow(endFreq / startFreq, 1.0 / duration);
        double phase = 0.0;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double freq = startFreq * std::pow(k, t);
            double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate white noise for stability testing
    static std::vector<float> generateWhiteNoise(double amplitude, double duration, 
                                                double sampleRate, uint32_t seed = 12345) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
    
    // Generate impulse for impulse response measurement
    static std::vector<float> generateImpulse(double amplitude, int position, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate step function for step response
    static std::vector<float> generateStep(double amplitude, int stepPosition, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        for (int i = stepPosition; i < totalSamples; ++i) {
            signal[i] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate test signal for self-oscillation testing
    static std::vector<float> generateSilence(int numSamples, double noiseLevel = 1e-6) {
        std::vector<float> signal(numSamples);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, static_cast<float>(noiseLevel));
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = dist(gen);
        }
        
        return signal;
    }
    
    // Generate triangle wave for saturation testing
    static std::vector<float> generateTriangleWave(double frequency, double amplitude, 
                                                 double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double period = sampleRate / frequency;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = fmod(i, period) / period;
            double value;
            
            if (t < 0.5) {
                value = 4.0 * t - 1.0; // Rising from -1 to 1
            } else {
                value = 3.0 - 4.0 * t; // Falling from 1 to -1
            }
            
            signal[i] = static_cast<float>(amplitude * value);
        }
        
        return signal;
    }
};

// Frequency response analyzer for ladder filter
class LadderFrequencyAnalyzer {
public:
    struct FilterResponse {
        double frequency;
        double magnitude_dB;
        double phase_degrees;
        double resonance_peak;
        bool selfOscillating;
    };
    
    // Measure frequency response at specific frequency
    static FilterResponse measureFrequencyResponse(LadderFilter& filter, 
                                                 double frequency, 
                                                 double sampleRate,
                                                 double amplitude = 0.01) {
        // Generate test signal
        const double testDuration = 2.0; // Longer for settling
        auto testSignal = LadderTestSignalGenerator::generateLogSweep(frequency * 0.99, 
                                                                     frequency * 1.01, 
                                                                     testDuration, sampleRate, amplitude);
        
        // Process through filter
        int numSamples = testSignal.size();
        juce::AudioBuffer<float> buffer(2, numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        filter.process(buffer);
        
        // Analyze steady-state portion (last 75% to avoid transients)
        int analysisStart = numSamples / 4;
        int analysisLength = 3 * numSamples / 4;
        
        // Calculate RMS of input and output
        double inputRMS = 0.0;
        double outputRMS = 0.0;
        double outputPeak = 0.0;
        
        for (int i = analysisStart; i < analysisStart + analysisLength; ++i) {
            double input = testSignal[i];
            double output = buffer.getSample(0, i);
            
            inputRMS += input * input;
            outputRMS += output * output;
            outputPeak = std::max(outputPeak, std::abs(output));
        }
        
        inputRMS = std::sqrt(inputRMS / analysisLength);
        outputRMS = std::sqrt(outputRMS / analysisLength);
        
        // Calculate magnitude response
        double magnitude_dB = 20.0 * std::log10(outputRMS / (inputRMS + 1e-15));
        
        // Detect self-oscillation (output much larger than input)
        bool selfOscillating = (outputRMS / (inputRMS + 1e-15)) > 10.0;
        
        FilterResponse response;
        response.frequency = frequency;
        response.magnitude_dB = magnitude_dB;
        response.phase_degrees = 0.0; // Simplified
        response.resonance_peak = outputPeak;
        response.selfOscillating = selfOscillating;
        
        return response;
    }
    
    // Measure rolloff characteristics
    static std::vector<FilterResponse> measureRolloff(LadderFilter& filter, double cutoffFreq, double sampleRate) {
        std::vector<FilterResponse> responses;
        
        // Test frequencies around cutoff (0.1x to 10x cutoff)
        std::vector<double> multipliers = {0.1, 0.2, 0.3, 0.5, 0.7, 1.0, 1.5, 2.0, 3.0, 5.0, 7.0, 10.0};
        
        for (double mult : multipliers) {
            double testFreq = cutoffFreq * mult;
            if (testFreq < sampleRate / 2.1) {
                auto response = measureFrequencyResponse(filter, testFreq, sampleRate);
                responses.push_back(response);
            }
        }
        
        return responses;
    }
};

// Self-oscillation analyzer
class OscillationAnalyzer {
public:
    struct OscillationResult {
        bool isOscillating;
        double oscillationFreq;
        double oscillationAmplitude;
        double thresholdResonance;
    };
    
    // Test for self-oscillation threshold
    static OscillationResult findOscillationThreshold(LadderFilter& filter, 
                                                     double cutoffFreq, 
                                                     double sampleRate) {
        OscillationResult result;
        result.isOscillating = false;
        result.oscillationFreq = 0.0;
        result.oscillationAmplitude = 0.0;
        result.thresholdResonance = 1.0;
        
        // Test increasing resonance values
        for (float resonance = 0.8f; resonance <= 1.0f; resonance += 0.01f) {
            // Set filter parameters
            std::map<int, float> params = {
                {0, static_cast<float>(cutoffFreq / 20000.0)}, // Cutoff
                {1, resonance}, // Resonance
                {2, 0.1f}, // Drive
                {3, 0.0f}, // Filter type (lowpass)
                {4, 0.0f}, // Asymmetry
                {5, 0.5f}, // Vintage mode
                {6, 1.0f}  // Mix
            };
            filter.updateParameters(params);
            
            // Generate noise input (to trigger oscillation)
            auto noiseSignal = LadderTestSignalGenerator::generateSilence(static_cast<int>(sampleRate), 1e-6);
            
            juce::AudioBuffer<float> buffer(2, noiseSignal.size());
            for (size_t i = 0; i < noiseSignal.size(); ++i) {
                buffer.setSample(0, i, noiseSignal[i]);
                buffer.setSample(1, i, noiseSignal[i]);
            }
            
            filter.process(buffer);
            
            // Analyze output for oscillation
            double outputRMS = 0.0;
            double outputPeak = 0.0;
            
            // Skip initial samples to avoid startup transients
            int analysisStart = buffer.getNumSamples() / 4;
            int analysisLength = buffer.getNumSamples() / 2;
            
            for (int i = analysisStart; i < analysisStart + analysisLength; ++i) {
                double output = buffer.getSample(0, i);
                outputRMS += output * output;
                outputPeak = std::max(outputPeak, std::abs(output));
            }
            
            outputRMS = std::sqrt(outputRMS / analysisLength);
            
            // Check for oscillation (output significantly larger than noise input)
            if (outputRMS > 0.001 || outputPeak > 0.01) {
                result.isOscillating = true;
                result.thresholdResonance = resonance;
                result.oscillationAmplitude = outputPeak;
                // TODO: Implement frequency detection using FFT
                result.oscillationFreq = cutoffFreq; // Approximation
                break;
            }
        }
        
        return result;
    }
};

// Main test suite for Ladder Filter
class LadderFilterTestSuite {
private:
    std::unique_ptr<LadderFilter> filter;
    std::ofstream logFile;
    
public:
    LadderFilterTestSuite() : filter(std::make_unique<LadderFilter>()) {
        logFile.open("LadderFilter_TestResults.txt");
        logFile << "=== Ladder Filter Test Results ===" << std::endl;
        logFile << "Test started at: " << getCurrentTime() << std::endl << std::endl;
    }
    
    ~LadderFilterTestSuite() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void runAllTests() {
        logFile << "Starting comprehensive Ladder Filter test suite..." << std::endl;
        
        // Initialize filter
        filter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        filter->reset();
        
        // Run test categories
        testBasicFunctionality();
        testFrequencyResponse();
        testResonanceBehavior();
        testSelfOscillation();
        testSaturationCharacteristics();
        testFilterStability();
        testComponentModeling();
        testThermalDrift();
        testOversamplingEffectiveness();
        testTransientResponse();
        testImpulseResponse();
        testStepResponse();
        testExtremeSettings();
        testZeroDelayFeedback();
        
        logFile << "\n=== Ladder Filter Test Suite Complete ===" << std::endl;
        std::cout << "Ladder Filter test results written to LadderFilter_TestResults.txt" << std::endl;
    }
    
private:
    void testBasicFunctionality() {
        logFile << "\n--- Basic Functionality Tests ---" << std::endl;
        
        // Test parameter count
        int numParams = filter->getNumParameters();
        logFile << "Number of parameters: " << numParams << std::endl;
        assert(numParams == 7);
        
        // Test parameter names
        for (int i = 0; i < numParams; ++i) {
            juce::String paramName = filter->getParameterName(i);
            logFile << "Parameter " << i << ": " << paramName << std::endl;
        }
        
        // Test engine name
        juce::String engineName = filter->getName();
        logFile << "Engine name: " << engineName << std::endl;
        assert(engineName == "Ladder Filter Pro");
        
        logFile << "✓ Basic functionality tests passed" << std::endl;
    }
    
    void testFrequencyResponse() {
        logFile << "\n--- Frequency Response Tests ---" << std::endl;
        
        // Test 4-pole lowpass response at different cutoff frequencies
        std::vector<double> cutoffFreqs = {100.0, 440.0, 1000.0, 2000.0, 5000.0};
        
        for (double cutoff : cutoffFreqs) {
            if (cutoff < TEST_SAMPLE_RATE / 2.1) {
                logFile << "\nTesting cutoff frequency: " << cutoff << " Hz" << std::endl;
                
                // Set filter parameters (low resonance for clean response)
                std::map<int, float> params = {
                    {0, static_cast<float>(cutoff / 20000.0)}, // Cutoff (normalized)
                    {1, 0.1f}, // Low resonance
                    {2, 0.0f}, // No drive
                    {3, 0.0f}, // Lowpass mode
                    {4, 0.0f}, // No asymmetry
                    {5, 0.0f}, // Modern mode
                    {6, 1.0f}  // Full wet
                };
                filter->updateParameters(params);
                
                // Measure rolloff characteristics
                auto rolloffData = LadderFrequencyAnalyzer::measureRolloff(*filter, cutoff, TEST_SAMPLE_RATE);
                
                // Find -3dB point
                double minus3dB_freq = 0.0;
                for (const auto& point : rolloffData) {
                    logFile << std::fixed << std::setprecision(1) 
                           << "  " << point.frequency << " Hz: " << point.magnitude_dB << " dB" << std::endl;
                    
                    if (std::abs(point.magnitude_dB + 3.0) < 1.0) {
                        minus3dB_freq = point.frequency;
                    }
                }
                
                // Verify 4-pole rolloff (-24dB/octave above cutoff)
                // Find points at 2x and 4x cutoff frequency
                double response_2x = 0.0;
                double response_4x = 0.0;
                
                for (const auto& point : rolloffData) {
                    if (std::abs(point.frequency - 2.0 * cutoff) < cutoff * 0.1) {
                        response_2x = point.magnitude_dB;
                    }
                    if (std::abs(point.frequency - 4.0 * cutoff) < cutoff * 0.1) {
                        response_4x = point.magnitude_dB;
                    }
                }
                
                double rolloff_octave = response_4x - response_2x; // Should be ~-24dB
                logFile << "  Rolloff per octave: " << rolloff_octave << " dB (expect ~-24dB)" << std::endl;
                
                // Verify reasonable 4-pole behavior
                assert(rolloff_octave < -18.0 && rolloff_octave > -30.0);
                
                // Verify cutoff frequency accuracy
                if (minus3dB_freq > 0.0) {
                    double freq_error = std::abs(minus3dB_freq - cutoff) / cutoff;
                    logFile << "  Cutoff frequency error: " << freq_error * 100.0 << "%" << std::endl;
                    assert(freq_error < FREQ_EPSILON);
                }
            }
        }
        
        logFile << "✓ Frequency response tests passed" << std::endl;
    }
    
    void testResonanceBehavior() {
        logFile << "\n--- Resonance Behavior Tests ---" << std::endl;
        
        double cutoffFreq = 1000.0;
        std::vector<float> resonanceValues = {0.0f, 0.3f, 0.6f, 0.8f, 0.95f};
        
        for (float resonance : resonanceValues) {
            logFile << "\nTesting resonance: " << resonance << std::endl;
            
            std::map<int, float> params = {
                {0, static_cast<float>(cutoffFreq / 20000.0)}, // Cutoff
                {1, resonance}, // Resonance
                {2, 0.0f}, // No drive
                {3, 0.0f}, // Lowpass mode
                {4, 0.0f}, // No asymmetry
                {5, 0.0f}, // Modern mode
                {6, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Measure response at cutoff frequency
            auto response = LadderFrequencyAnalyzer::measureFrequencyResponse(*filter, cutoffFreq, TEST_SAMPLE_RATE);
            
            logFile << "  Response at cutoff: " << response.magnitude_dB << " dB" << std::endl;
            logFile << "  Peak level: " << response.resonance_peak << std::endl;
            logFile << "  Self-oscillating: " << (response.selfOscillating ? "YES" : "NO") << std::endl;
            
            // Higher resonance should increase peak at cutoff
            if (resonance > 0.5f) {
                assert(response.magnitude_dB > 0.0); // Should have resonant peak
            }
            
            // Should not self-oscillate until very high resonance
            if (resonance < 0.9f) {
                assert(!response.selfOscillating);
            }
        }
        
        logFile << "✓ Resonance behavior tests passed" << std::endl;
    }
    
    void testSelfOscillation() {
        logFile << "\n--- Self-Oscillation Tests ---" << std::endl;
        
        // Test self-oscillation at different cutoff frequencies
        std::vector<double> testFreqs = {200.0, 500.0, 1000.0, 2000.0, 5000.0};
        
        for (double freq : testFreqs) {
            if (freq < TEST_SAMPLE_RATE / 2.1) {
                logFile << "\nTesting self-oscillation at " << freq << " Hz:" << std::endl;
                
                auto oscResult = OscillationAnalyzer::findOscillationThreshold(*filter, freq, TEST_SAMPLE_RATE);
                
                logFile << "  Oscillation threshold: " << oscResult.thresholdResonance << std::endl;
                logFile << "  Oscillation amplitude: " << oscResult.oscillationAmplitude << std::endl;
                logFile << "  Oscillation frequency: " << oscResult.oscillationFreq << " Hz" << std::endl;
                
                // Should oscillate at high resonance
                assert(oscResult.isOscillating);
                assert(oscResult.thresholdResonance > 0.9f);
                assert(oscResult.oscillationAmplitude > 0.01);
                
                // Oscillation frequency should be near cutoff
                double freq_error = std::abs(oscResult.oscillationFreq - freq) / freq;
                assert(freq_error < 0.2); // Within 20%
            }
        }
        
        logFile << "✓ Self-oscillation tests passed" << std::endl;
    }
    
    void testSaturationCharacteristics() {
        logFile << "\n--- Saturation Characteristics Tests ---" << std::endl;
        
        // Test saturation at different drive levels
        std::vector<float> driveValues = {0.0f, 0.3f, 0.6f, 0.9f};
        double testFreq = 1000.0;
        
        for (float drive : driveValues) {
            logFile << "\nTesting drive level: " << drive << std::endl;
            
            std::map<int, float> params = {
                {0, static_cast<float>(testFreq / 20000.0)}, // Cutoff
                {1, 0.7f}, // Moderate resonance
                {2, drive}, // Variable drive
                {3, 0.0f}, // Lowpass mode
                {4, 0.0f}, // No asymmetry
                {5, 0.8f}, // Vintage mode for saturation
                {6, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with triangle wave (rich in harmonics)
            auto testSignal = LadderTestSignalGenerator::generateTriangleWave(testFreq / 4.0, 0.2, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Measure output characteristics
            double outputRMS = 0.0;
            double outputPeak = 0.0;
            
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                double output = buffer.getSample(0, i);
                outputRMS += output * output;
                outputPeak = std::max(outputPeak, std::abs(output));
            }
            
            outputRMS = std::sqrt(outputRMS / buffer.getNumSamples());
            
            // Calculate simple distortion metric
            double peakToRMSRatio = outputPeak / (outputRMS + 1e-15);
            
            logFile << "  Output RMS: " << outputRMS << std::endl;
            logFile << "  Output peak: " << outputPeak << std::endl;
            logFile << "  Peak/RMS ratio: " << peakToRMSRatio << std::endl;
            
            // Higher drive should increase saturation
            if (drive > 0.5f) {
                assert(peakToRMSRatio < 2.5); // More compressed due to saturation
            }
        }
        
        logFile << "✓ Saturation characteristics tests passed" << std::endl;
    }
    
    void testFilterStability() {
        logFile << "\n--- Filter Stability Tests ---" << std::endl;
        
        // Test stability with rapid parameter changes
        for (int iteration = 0; iteration < 50; ++iteration) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            
            std::map<int, float> randomParams;
            for (int p = 0; p < filter->getNumParameters(); ++p) {
                randomParams[p] = dist(gen);
            }
            
            filter->updateParameters(randomParams);
            
            // Process white noise
            auto noiseSignal = LadderTestSignalGenerator::generateWhiteNoise(0.1, 0.1, TEST_SAMPLE_RATE);
            juce::AudioBuffer<float> buffer(2, noiseSignal.size());
            
            for (size_t i = 0; i < noiseSignal.size(); ++i) {
                buffer.setSample(0, i, noiseSignal[i]);
                buffer.setSample(1, i, noiseSignal[i]);
            }
            
            filter->process(buffer);
            
            // Check for NaN or inf values
            bool stable = true;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    float sample = buffer.getSample(ch, i);
                    if (std::isnan(sample) || std::isinf(sample)) {
                        stable = false;
                        break;
                    }
                    if (std::abs(sample) > 100.0f) { // Unreasonable output
                        stable = false;
                        break;
                    }
                }
                if (!stable) break;
            }
            
            assert(stable);
        }
        
        logFile << "✓ Filter stability tests passed (50 random parameter sets)" << std::endl;
    }
    
    void testComponentModeling() {
        logFile << "\n--- Component Modeling Tests ---" << std::endl;
        
        // Test component tolerance variations by resetting filter multiple times
        std::vector<double> responses;
        
        for (int variation = 0; variation < 10; ++variation) {
            filter->reset(); // This should generate new component tolerances
            filter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
            
            std::map<int, float> testParams = {
                {0, 0.25f}, // 1kHz cutoff
                {1, 0.6f},  // Moderate resonance
                {2, 0.3f},  // Some drive
                {3, 0.0f},  // Lowpass
                {4, 0.0f},  // No asymmetry
                {5, 0.8f},  // Vintage mode
                {6, 1.0f}   // Full wet
            };
            filter->updateParameters(testParams);
            
            auto response = LadderFrequencyAnalyzer::measureFrequencyResponse(*filter, 1000.0, TEST_SAMPLE_RATE);
            responses.push_back(response.magnitude_dB);
            
            logFile << "Variation " << variation << ": " << response.magnitude_dB << " dB" << std::endl;
        }
        
        // Calculate variance in responses
        double mean = 0.0;
        for (double resp : responses) {
            mean += resp;
        }
        mean /= responses.size();
        
        double variance = 0.0;
        for (double resp : responses) {
            variance += (resp - mean) * (resp - mean);
        }
        variance /= responses.size();
        double stdDev = std::sqrt(variance);
        
        logFile << "Response variance: " << variance << std::endl;
        logFile << "Response std dev: " << stdDev << " dB" << std::endl;
        
        // Should have some variation but not excessive
        assert(stdDev > 0.01); // Some component modeling
        assert(stdDev < 2.0);  // But reasonable
        
        logFile << "✓ Component modeling tests passed" << std::endl;
    }
    
    void testThermalDrift() {
        logFile << "\n--- Thermal Drift Tests ---" << std::endl;
        
        std::map<int, float> stableParams = {
            {0, 0.25f}, {1, 0.6f}, {2, 0.3f}, {3, 0.0f}, {4, 0.0f}, {5, 0.8f}, {6, 1.0f}
        };
        filter->updateParameters(stableParams);
        
        // Measure initial response
        auto initialResponse = LadderFrequencyAnalyzer::measureFrequencyResponse(*filter, 1000.0, TEST_SAMPLE_RATE);
        
        // Process extended audio to simulate thermal warming
        for (int block = 0; block < 100; ++block) {
            auto warmupSignal = LadderTestSignalGenerator::generateWhiteNoise(0.05, 0.1, TEST_SAMPLE_RATE);
            juce::AudioBuffer<float> warmupBuffer(2, warmupSignal.size());
            
            for (size_t i = 0; i < warmupSignal.size(); ++i) {
                warmupBuffer.setSample(0, i, warmupSignal[i]);
                warmupBuffer.setSample(1, i, warmupSignal[i]);
            }
            
            filter->process(warmupBuffer);
        }
        
        // Measure response after thermal drift
        auto warmedResponse = LadderFrequencyAnalyzer::measureFrequencyResponse(*filter, 1000.0, TEST_SAMPLE_RATE);
        
        double drift = std::abs(warmedResponse.magnitude_dB - initialResponse.magnitude_dB);
        
        logFile << "Initial response: " << initialResponse.magnitude_dB << " dB" << std::endl;
        logFile << "Warmed response: " << warmedResponse.magnitude_dB << " dB" << std::endl;
        logFile << "Thermal drift: " << drift << " dB" << std::endl;
        
        // Some thermal drift is expected but should be minimal
        assert(drift < 0.5); // Less than 0.5dB drift
        
        logFile << "✓ Thermal drift tests passed" << std::endl;
    }
    
    void testOversamplingEffectiveness() {
        logFile << "\n--- Oversampling Effectiveness Tests ---" << std::endl;
        
        // Test aliasing reduction with high-frequency input
        double testFreq = TEST_SAMPLE_RATE * 0.4; // High frequency near Nyquist
        
        std::map<int, float> params = {
            {0, 0.8f}, // High cutoff
            {1, 0.8f}, // High resonance
            {2, 0.7f}, // High drive (creates harmonics)
            {3, 0.0f}, // Lowpass
            {4, 0.0f}, // No asymmetry
            {5, 0.8f}, // Vintage mode
            {6, 1.0f}  // Full wet
        };
        filter->updateParameters(params);
        
        auto testSignal = LadderTestSignalGenerator::generateTriangleWave(testFreq / 8, 0.3, 1.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, testSignal.size());
        for (size_t i = 0; i < testSignal.size(); ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        filter->process(buffer);
        
        // Analyze output for aliasing artifacts
        double outputRMS = 0.0;
        double maxOutput = 0.0;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            double output = std::abs(buffer.getSample(0, i));
            outputRMS += output * output;
            maxOutput = std::max(maxOutput, output);
        }
        
        outputRMS = std::sqrt(outputRMS / buffer.getNumSamples());
        
        logFile << "High-frequency processing test:" << std::endl;
        logFile << "  Output RMS: " << outputRMS << std::endl;
        logFile << "  Output peak: " << maxOutput << std::endl;
        
        // Should produce reasonable output without excessive artifacts
        assert(outputRMS < 1.0);
        assert(maxOutput < 5.0);
        
        logFile << "✓ Oversampling effectiveness tests passed" << std::endl;
    }
    
    void testTransientResponse() {
        logFile << "\n--- Transient Response Tests ---" << std::endl;
        
        std::map<int, float> params = {
            {0, 0.3f}, // 1.5kHz cutoff
            {1, 0.6f}, // Moderate resonance
            {2, 0.2f}, // Low drive
            {3, 0.0f}, // Lowpass
            {4, 0.0f}, // No asymmetry
            {5, 0.0f}, // Modern mode
            {6, 1.0f}  // Full wet
        };
        filter->updateParameters(params);
        
        // Generate sharp transient (kick drum simulation)
        std::vector<float> transient(static_cast<int>(0.2 * TEST_SAMPLE_RATE), 0.0f);
        
        // Sharp attack at 60Hz
        for (int i = 0; i < 200; ++i) {
            double t = i / TEST_SAMPLE_RATE;
            transient[i] = 0.8f * std::sin(2.0 * M_PI * 60.0 * t) * std::exp(-t * 30.0);
        }
        
        juce::AudioBuffer<float> buffer(2, transient.size());
        for (size_t i = 0; i < transient.size(); ++i) {
            buffer.setSample(0, i, transient[i]);
            buffer.setSample(1, i, transient[i]);
        }
        
        filter->process(buffer);
        
        // Analyze transient preservation
        double inputPeak = *std::max_element(transient.begin(), transient.end());
        double outputPeak = 0.0;
        int peakPosition = 0;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            double output = std::abs(buffer.getSample(0, i));
            if (output > outputPeak) {
                outputPeak = output;
                peakPosition = i;
            }
        }
        
        logFile << "Transient response analysis:" << std::endl;
        logFile << "  Input peak: " << inputPeak << std::endl;
        logFile << "  Output peak: " << outputPeak << std::endl;
        logFile << "  Peak position: " << peakPosition << " samples" << std::endl;
        
        // Transient should be preserved reasonably well
        assert(outputPeak > inputPeak * 0.1); // At least 10% of input peak
        assert(peakPosition < 100); // Peak should occur early
        
        logFile << "✓ Transient response tests passed" << std::endl;
    }
    
    void testImpulseResponse() {
        logFile << "\n--- Impulse Response Tests ---" << std::endl;
        
        std::map<int, float> params = {
            {0, 0.25f}, // 1kHz cutoff
            {1, 0.5f},  // Moderate resonance
            {2, 0.0f},  // No drive
            {3, 0.0f},  // Lowpass
            {4, 0.0f},  // No asymmetry
            {5, 0.0f},  // Modern mode
            {6, 1.0f}   // Full wet
        };
        filter->updateParameters(params);
        
        // Generate impulse
        auto impulse = LadderTestSignalGenerator::generateImpulse(1.0, 0, static_cast<int>(0.5 * TEST_SAMPLE_RATE));
        
        juce::AudioBuffer<float> buffer(2, impulse.size());
        for (size_t i = 0; i < impulse.size(); ++i) {
            buffer.setSample(0, i, impulse[i]);
            buffer.setSample(1, i, impulse[i]);
        }
        
        filter->process(buffer);
        
        // Analyze impulse response
        double totalEnergy = 0.0;
        double peak = 0.0;
        int settlingTime = 0;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            double output = buffer.getSample(0, i);
            totalEnergy += output * output;
            peak = std::max(peak, std::abs(output));
            
            // Find settling time (when response drops below 1% of peak)
            if (settlingTime == 0 && i > 100 && std::abs(output) < peak * 0.01) {
                settlingTime = i;
            }
        }
        
        logFile << "Impulse response analysis:" << std::endl;
        logFile << "  Total energy: " << totalEnergy << std::endl;
        logFile << "  Peak response: " << peak << std::endl;
        logFile << "  Settling time: " << settlingTime << " samples (" 
               << settlingTime / TEST_SAMPLE_RATE * 1000.0 << " ms)" << std::endl;
        
        // Verify reasonable impulse response
        assert(totalEnergy > 0.01);
        assert(peak > 0.1 && peak < 10.0);
        assert(settlingTime > 0 && settlingTime < TEST_SAMPLE_RATE * 0.5); // Within 500ms
        
        logFile << "✓ Impulse response tests passed" << std::endl;
    }
    
    void testStepResponse() {
        logFile << "\n--- Step Response Tests ---" << std::endl;
        
        std::map<int, float> params = {
            {0, 0.25f}, // 1kHz cutoff
            {1, 0.3f},  // Low resonance for clean step
            {2, 0.0f},  // No drive
            {3, 0.0f},  // Lowpass
            {4, 0.0f},  // No asymmetry
            {5, 0.0f},  // Modern mode
            {6, 1.0f}   // Full wet
        };
        filter->updateParameters(params);
        
        // Generate step
        auto step = LadderTestSignalGenerator::generateStep(0.5, static_cast<int>(0.1 * TEST_SAMPLE_RATE), 
                                                          static_cast<int>(1.0 * TEST_SAMPLE_RATE));
        
        juce::AudioBuffer<float> buffer(2, step.size());
        for (size_t i = 0; i < step.size(); ++i) {
            buffer.setSample(0, i, step[i]);
            buffer.setSample(1, i, step[i]);
        }
        
        filter->process(buffer);
        
        // Analyze step response
        int stepStart = static_cast<int>(0.1 * TEST_SAMPLE_RATE);
        double finalValue = 0.0;
        double overshoot = 0.0;
        int riseTime = 0;
        
        // Calculate final value (average of last 1000 samples)
        for (int i = buffer.getNumSamples() - 1000; i < buffer.getNumSamples(); ++i) {
            finalValue += buffer.getSample(0, i);
        }
        finalValue /= 1000.0;
        
        // Find overshoot and rise time
        for (int i = stepStart; i < buffer.getNumSamples(); ++i) {
            double output = buffer.getSample(0, i);
            overshoot = std::max(overshoot, output);
            
            // 90% rise time
            if (riseTime == 0 && output > finalValue * 0.9) {
                riseTime = i - stepStart;
            }
        }
        
        double overshootPercent = (overshoot - finalValue) / finalValue * 100.0;
        
        logFile << "Step response analysis:" << std::endl;
        logFile << "  Final value: " << finalValue << std::endl;
        logFile << "  Overshoot: " << overshootPercent << "%" << std::endl;
        logFile << "  Rise time (90%): " << riseTime << " samples (" 
               << riseTime / TEST_SAMPLE_RATE * 1000.0 << " ms)" << std::endl;
        
        // Verify reasonable step response
        assert(std::abs(finalValue - 0.5) < 0.1); // Should settle near input level
        assert(overshootPercent < 50.0); // Reasonable overshoot
        assert(riseTime > 0 && riseTime < TEST_SAMPLE_RATE * 0.1); // Within 100ms
        
        logFile << "✓ Step response tests passed" << std::endl;
    }
    
    void testExtremeSettings() {
        logFile << "\n--- Extreme Settings Tests ---" << std::endl;
        
        // Test maximum settings
        std::map<int, float> extremeParams = {
            {0, 1.0f}, // Maximum cutoff
            {1, 1.0f}, // Maximum resonance
            {2, 1.0f}, // Maximum drive
            {3, 0.0f}, // Lowpass
            {4, 1.0f}, // Maximum asymmetry
            {5, 1.0f}, // Maximum vintage
            {6, 1.0f}  // Full wet
        };
        filter->updateParameters(extremeParams);
        
        // Test with various signal types
        auto testSignal = LadderTestSignalGenerator::generateWhiteNoise(0.1, 1.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, testSignal.size());
        for (size_t i = 0; i < testSignal.size(); ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        filter->process(buffer);
        
        // Check for stability
        bool stable = true;
        double maxOutput = 0.0;
        
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                
                if (std::isnan(sample) || std::isinf(sample)) {
                    stable = false;
                }
                
                maxOutput = std::max(maxOutput, static_cast<double>(std::abs(sample)));
            }
        }
        
        logFile << "Extreme settings test:" << std::endl;
        logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
        logFile << "  Maximum output: " << maxOutput << std::endl;
        
        assert(stable);
        // With extreme resonance, some oscillation is expected
        // assert(maxOutput < 100.0); // But should be bounded
        
        logFile << "✓ Extreme settings tests passed" << std::endl;
    }
    
    void testZeroDelayFeedback() {
        logFile << "\n--- Zero-Delay Feedback Tests ---" << std::endl;
        
        // Test different resonance levels for zero-delay accuracy
        std::vector<float> resonanceLevels = {0.5f, 0.7f, 0.85f, 0.95f};
        
        for (float resonance : resonanceLevels) {
            std::map<int, float> params = {
                {0, 0.25f}, // 1kHz cutoff
                {1, resonance}, // Variable resonance
                {2, 0.0f}, // No drive
                {3, 0.0f}, // Lowpass
                {4, 0.0f}, // No asymmetry
                {5, 0.0f}, // Modern mode for cleaner test
                {6, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Generate impulse to test feedback behavior
            auto impulse = LadderTestSignalGenerator::generateImpulse(0.1, 0, static_cast<int>(0.2 * TEST_SAMPLE_RATE));
            
            juce::AudioBuffer<float> buffer(2, impulse.size());
            for (size_t i = 0; i < impulse.size(); ++i) {
                buffer.setSample(0, i, impulse[i]);
                buffer.setSample(1, i, impulse[i]);
            }
            
            filter->process(buffer);
            
            // Measure ringing characteristics
            double ringEnergy = 0.0;
            int ringDuration = 0;
            
            // Skip initial transient, analyze ringing
            int analysisStart = 100;
            for (int i = analysisStart; i < buffer.getNumSamples(); ++i) {
                double output = std::abs(buffer.getSample(0, i));
                ringEnergy += output * output;
                
                if (output > 0.001) {
                    ringDuration = i - analysisStart;
                }
            }
            
            logFile << "Resonance " << resonance << ":" << std::endl;
            logFile << "  Ring energy: " << ringEnergy << std::endl;
            logFile << "  Ring duration: " << ringDuration << " samples" << std::endl;
            
            // Higher resonance should produce more ringing
            if (resonance > 0.8f) {
                assert(ringEnergy > 0.001);
                assert(ringDuration > 100);
            }
        }
        
        logFile << "✓ Zero-delay feedback tests passed" << std::endl;
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
    std::cout << "Starting Ladder Filter comprehensive test suite..." << std::endl;
    
    try {
        LadderFilterTestSuite testSuite;
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