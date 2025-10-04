/*
  ==============================================================================
  
    StateVariableFilter_Test.cpp
    Comprehensive test suite for ENGINE_STATE_VARIABLE_FILTER
    
    Tests for state variable filter characteristics:
    - Multi-mode operation (LP/HP/BP/Notch)
    - Mode switching continuity and artifacts
    - Zero-delay feedback topology accuracy
    - Resonance behavior and stability
    - Cascaded multi-pole configurations
    - Envelope following functionality
    - Drive and saturation characteristics
    - Analog modeling features
    - Parameter smoothing effectiveness
    
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
#include "../../Source/StateVariableFilter.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.3f;
constexpr float FREQ_EPSILON = 0.05f; // 5% frequency tolerance
constexpr float CONTINUITY_EPSILON = 0.1f; // For mode switching tests

// Test signal generators for SVF testing
class SVFTestSignalGenerator {
public:
    // Generate sine wave for frequency response testing
    static std::vector<float> generateSineWave(double frequency, double amplitude, 
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
    
    // Generate frequency sweep for comprehensive response testing
    static std::vector<float> generateChirp(double startFreq, double endFreq,
                                          double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double freq = startFreq + (endFreq - startFreq) * t / duration;
            double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
            
            signal[i] = static_cast<float>(0.1 * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate multi-tone signal for testing mode separation
    static std::vector<float> generateMultiTone(const std::vector<double>& frequencies,
                                              const std::vector<double>& amplitudes,
                                              double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        for (size_t f = 0; f < frequencies.size() && f < amplitudes.size(); ++f) {
            double phase = 0.0;
            double phaseIncrement = 2.0 * M_PI * frequencies[f] / sampleRate;
            
            for (int i = 0; i < numSamples; ++i) {
                signal[i] += static_cast<float>(amplitudes[f] * std::sin(phase));
                phase += phaseIncrement;
            }
        }
        
        return signal;
    }
    
    // Generate impulse with configurable position
    static std::vector<float> generateImpulse(double amplitude, int position, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate white noise
    static std::vector<float> generateWhiteNoise(double amplitude, double duration, 
                                                double sampleRate, uint32_t seed = 42) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
    
    // Generate envelope-following test signal (varying amplitude)
    static std::vector<float> generateEnvelopeSignal(double carrierFreq, double modFreq,
                                                   double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double carrierPhase = 0.0;
        double modPhase = 0.0;
        double carrierIncrement = 2.0 * M_PI * carrierFreq / sampleRate;
        double modIncrement = 2.0 * M_PI * modFreq / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            double envelope = 0.5 + 0.4 * std::sin(modPhase);
            signal[i] = static_cast<float>(envelope * std::sin(carrierPhase));
            
            carrierPhase += carrierIncrement;
            modPhase += modIncrement;
        }
        
        return signal;
    }
};

// Multi-mode frequency response analyzer
class SVFResponseAnalyzer {
public:
    struct ModeResponse {
        double frequency;
        double magnitude_dB;
        double phase_degrees;
        int filterMode; // 0=LP, 1=HP, 2=BP, 3=Notch
    };
    
    // Measure response for specific mode
    static ModeResponse measureModeResponse(StateVariableFilter& filter, 
                                          double frequency, 
                                          int mode,
                                          double sampleRate,
                                          double amplitude = 0.1) {
        // Set filter to specified mode
        std::map<int, float> modeParams = {
            {0, static_cast<float>(frequency / 20000.0)}, // Frequency (normalized)
            {1, 0.5f}, // Moderate resonance
            {2, 0.1f}, // Low drive
            {3, static_cast<float>(mode) / 8.0f}, // Filter type
            {4, 0.0f}, // Slope (1-pole)
            {5, 0.0f}, // No envelope
            {6, 0.01f}, // Fast attack
            {7, 0.1f}, // Fast release
            {8, 0.0f}, // No analog modeling
            {9, 1.0f}  // Full wet
        };
        filter.updateParameters(modeParams);
        
        // Generate test signal
        const double testDuration = 1.5;
        auto testSignal = SVFTestSignalGenerator::generateSineWave(frequency, amplitude, 
                                                                 testDuration, sampleRate);
        
        // Process through filter
        int numSamples = testSignal.size();
        juce::AudioBuffer<float> buffer(2, numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        filter.process(buffer);
        
        // Analyze steady-state response (skip initial transient)
        int analysisStart = numSamples / 3;
        int analysisLength = numSamples / 3;
        
        double inputRMS = 0.0;
        double outputRMS = 0.0;
        
        for (int i = analysisStart; i < analysisStart + analysisLength; ++i) {
            double input = testSignal[i];
            double output = buffer.getSample(0, i);
            
            inputRMS += input * input;
            outputRMS += output * output;
        }
        
        inputRMS = std::sqrt(inputRMS / analysisLength);
        outputRMS = std::sqrt(outputRMS / analysisLength);
        
        double magnitude_dB = 20.0 * std::log10(outputRMS / (inputRMS + 1e-15));
        
        ModeResponse response;
        response.frequency = frequency;
        response.magnitude_dB = magnitude_dB;
        response.phase_degrees = 0.0; // Simplified
        response.filterMode = mode;
        
        return response;
    }
    
    // Measure frequency response across range for specific mode
    static std::vector<ModeResponse> measureFullModeResponse(StateVariableFilter& filter,
                                                           int mode, 
                                                           double sampleRate) {
        std::vector<ModeResponse> responses;
        
        // Standard test frequencies
        std::vector<double> testFreqs = {
            50, 100, 200, 400, 800, 1000, 1600, 2000, 3200, 5000, 8000, 12000, 16000
        };
        
        for (double freq : testFreqs) {
            if (freq < sampleRate / 2.1) {
                auto response = measureModeResponse(filter, freq, mode, sampleRate);
                responses.push_back(response);
            }
        }
        
        return responses;
    }
};

// Mode switching continuity analyzer
class ModeSwitchingAnalyzer {
public:
    struct SwitchingResult {
        double maxDiscontinuity;
        double averageDiscontinuity;
        bool hasArtifacts;
        std::vector<double> transitionTimes;
    };
    
    // Test mode switching continuity
    static SwitchingResult analyzeModeSwitch(StateVariableFilter& filter,
                                           double testFreq,
                                           const std::vector<int>& modeSequence,
                                           double sampleRate) {
        SwitchingResult result;
        result.maxDiscontinuity = 0.0;
        result.averageDiscontinuity = 0.0;
        result.hasArtifacts = false;
        
        // Generate continuous test signal
        double totalDuration = modeSequence.size() * 0.5; // 0.5s per mode
        auto testSignal = SVFTestSignalGenerator::generateSineWave(testFreq, 0.1, totalDuration, sampleRate);
        
        juce::AudioBuffer<float> buffer(2, testSignal.size());
        for (size_t i = 0; i < testSignal.size(); ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        // Process with mode changes
        int samplesPerMode = static_cast<int>(0.5 * sampleRate);
        std::vector<double> discontinuities;
        
        for (size_t modeIndex = 0; modeIndex < modeSequence.size(); ++modeIndex) {
            int startSample = modeIndex * samplesPerMode;
            int endSample = std::min(startSample + samplesPerMode, static_cast<int>(testSignal.size()));
            
            // Set new mode
            std::map<int, float> params = {
                {0, static_cast<float>(testFreq / 20000.0)}, // Frequency
                {1, 0.6f}, // Moderate resonance
                {2, 0.1f}, // Low drive
                {3, static_cast<float>(modeSequence[modeIndex]) / 8.0f}, // Mode
                {4, 0.0f}, // Single-pole
                {5, 0.0f}, // No envelope
                {6, 0.01f}, {7, 0.1f}, // Attack/release
                {8, 0.0f}, // No analog
                {9, 1.0f}  // Full wet
            };
            filter.updateParameters(params);
            
            // Process this segment
            for (int i = startSample; i < endSample; ++i) {
                juce::AudioBuffer<float> sampleBuffer(2, 1);
                sampleBuffer.setSample(0, 0, testSignal[i]);
                sampleBuffer.setSample(1, 0, testSignal[i]);
                
                filter.process(sampleBuffer);
                
                buffer.setSample(0, i, sampleBuffer.getSample(0, 0));
                buffer.setSample(1, i, sampleBuffer.getSample(1, 0));
            }
            
            // Check for discontinuity at mode boundary
            if (modeIndex > 0) {
                int boundaryIndex = startSample;
                if (boundaryIndex > 10 && boundaryIndex < buffer.getNumSamples() - 10) {
                    double beforeValue = buffer.getSample(0, boundaryIndex - 1);
                    double afterValue = buffer.getSample(0, boundaryIndex);
                    double discontinuity = std::abs(afterValue - beforeValue);
                    
                    discontinuities.push_back(discontinuity);
                    result.maxDiscontinuity = std::max(result.maxDiscontinuity, discontinuity);
                }
            }
        }
        
        // Calculate average discontinuity
        if (!discontinuities.empty()) {
            double sum = 0.0;
            for (double disc : discontinuities) {
                sum += disc;
            }
            result.averageDiscontinuity = sum / discontinuities.size();
        }
        
        // Check for artifacts (excessive discontinuities)
        result.hasArtifacts = result.maxDiscontinuity > CONTINUITY_EPSILON;
        
        return result;
    }
};

// Main test suite for State Variable Filter
class StateVariableFilterTestSuite {
private:
    std::unique_ptr<StateVariableFilter> filter;
    std::ofstream logFile;
    
public:
    StateVariableFilterTestSuite() : filter(std::make_unique<StateVariableFilter>()) {
        logFile.open("StateVariableFilter_TestResults.txt");
        logFile << "=== State Variable Filter Test Results ===" << std::endl;
        logFile << "Test started at: " << getCurrentTime() << std::endl << std::endl;
    }
    
    ~StateVariableFilterTestSuite() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void runAllTests() {
        logFile << "Starting comprehensive State Variable Filter test suite..." << std::endl;
        
        // Initialize filter
        filter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        filter->reset();
        
        // Run test categories
        testBasicFunctionality();
        testMultiModeOperation();
        testModeSwitchingContinuity();
        testResonanceBehavior();
        testCascadedConfiguration();
        testEnvelopeFollowing();
        testDriveCharacteristics();
        testAnalogModeling();
        testParameterSmoothing();
        testFrequencyTracking();
        testStabilityAndLimits();
        testTransientResponse();
        testZeroDelayTopology();
        
        logFile << "\n=== State Variable Filter Test Suite Complete ===" << std::endl;
        std::cout << "State Variable Filter test results written to StateVariableFilter_TestResults.txt" << std::endl;
    }
    
private:
    void testBasicFunctionality() {
        logFile << "\n--- Basic Functionality Tests ---" << std::endl;
        
        // Test parameter count
        int numParams = filter->getNumParameters();
        logFile << "Number of parameters: " << numParams << std::endl;
        assert(numParams == 10);
        
        // Test parameter names
        for (int i = 0; i < numParams; ++i) {
            juce::String paramName = filter->getParameterName(i);
            logFile << "Parameter " << i << ": " << paramName << std::endl;
        }
        
        // Test engine name
        juce::String engineName = filter->getName();
        logFile << "Engine name: " << engineName << std::endl;
        assert(engineName == "State Variable Filter");
        
        logFile << "✓ Basic functionality tests passed" << std::endl;
    }
    
    void testMultiModeOperation() {
        logFile << "\n--- Multi-Mode Operation Tests ---" << std::endl;
        
        // Test each filter mode at 1kHz
        double testFreq = 1000.0;
        std::vector<std::string> modeNames = {"Lowpass", "Highpass", "Bandpass", "Notch"};
        
        for (int mode = 0; mode < 4; ++mode) {
            logFile << "\nTesting " << modeNames[mode] << " mode:" << std::endl;
            
            auto fullResponse = SVFResponseAnalyzer::measureFullModeResponse(*filter, mode, TEST_SAMPLE_RATE);
            
            // Log frequency response
            for (const auto& point : fullResponse) {
                logFile << "  " << point.frequency << " Hz: " << point.magnitude_dB << " dB" << std::endl;
            }
            
            // Verify mode-specific characteristics
            verifyModeCharacteristics(fullResponse, mode, testFreq);
        }
        
        logFile << "✓ Multi-mode operation tests passed" << std::endl;
    }
    
    void verifyModeCharacteristics(const std::vector<SVFResponseAnalyzer::ModeResponse>& response,
                                 int mode, double centerFreq) {
        // Find responses at key frequencies
        double lowFreqResponse = -100.0;  // 100 Hz
        double centerFreqResponse = -100.0;  // 1000 Hz
        double highFreqResponse = -100.0;  // 10000 Hz
        
        for (const auto& point : response) {
            if (std::abs(point.frequency - 100.0) < 50.0) {
                lowFreqResponse = point.magnitude_dB;
            }
            if (std::abs(point.frequency - centerFreq) < centerFreq * 0.1) {
                centerFreqResponse = point.magnitude_dB;
            }
            if (std::abs(point.frequency - 10000.0) < 1000.0) {
                highFreqResponse = point.magnitude_dB;
            }
        }
        
        switch (mode) {
            case 0: // Lowpass
                logFile << "  Lowpass verification: Low=" << lowFreqResponse 
                       << "dB, High=" << highFreqResponse << "dB" << std::endl;
                assert(lowFreqResponse > highFreqResponse - 5.0); // Low freq should pass better
                break;
                
            case 1: // Highpass
                logFile << "  Highpass verification: Low=" << lowFreqResponse 
                       << "dB, High=" << highFreqResponse << "dB" << std::endl;
                assert(highFreqResponse > lowFreqResponse - 5.0); // High freq should pass better
                break;
                
            case 2: // Bandpass
                logFile << "  Bandpass verification: Low=" << lowFreqResponse 
                       << "dB, Center=" << centerFreqResponse 
                       << "dB, High=" << highFreqResponse << "dB" << std::endl;
                assert(centerFreqResponse > lowFreqResponse); // Center should be highest
                assert(centerFreqResponse > highFreqResponse);
                break;
                
            case 3: // Notch
                logFile << "  Notch verification: Low=" << lowFreqResponse 
                       << "dB, Center=" << centerFreqResponse 
                       << "dB, High=" << highFreqResponse << "dB" << std::endl;
                assert(centerFreqResponse < lowFreqResponse); // Center should be lowest
                assert(centerFreqResponse < highFreqResponse);
                break;
        }
    }
    
    void testModeSwitchingContinuity() {
        logFile << "\n--- Mode Switching Continuity Tests ---" << std::endl;
        
        double testFreq = 1000.0;
        
        // Test various mode switching sequences
        std::vector<std::vector<int>> testSequences = {
            {0, 1, 0, 1}, // LP -> HP -> LP -> HP
            {0, 2, 3, 1}, // LP -> BP -> Notch -> HP
            {2, 2, 2, 3}, // BP -> BP -> BP -> Notch (including no-change)
            {3, 0, 1, 2}  // Notch -> LP -> HP -> BP
        };
        
        for (size_t seq = 0; seq < testSequences.size(); ++seq) {
            logFile << "\nTesting mode sequence " << seq + 1 << ": ";
            for (int mode : testSequences[seq]) {
                logFile << mode << " ";
            }
            logFile << std::endl;
            
            auto result = ModeSwitchingAnalyzer::analyzeModeSwitch(*filter, testFreq, 
                                                                 testSequences[seq], TEST_SAMPLE_RATE);
            
            logFile << "  Max discontinuity: " << result.maxDiscontinuity << std::endl;
            logFile << "  Average discontinuity: " << result.averageDiscontinuity << std::endl;
            logFile << "  Has artifacts: " << (result.hasArtifacts ? "YES" : "NO") << std::endl;
            
            // Verify smooth transitions
            assert(result.maxDiscontinuity < CONTINUITY_EPSILON);
            assert(!result.hasArtifacts);
        }
        
        logFile << "✓ Mode switching continuity tests passed" << std::endl;
    }
    
    void testResonanceBehavior() {
        logFile << "\n--- Resonance Behavior Tests ---" << std::endl;
        
        double testFreq = 1000.0;
        std::vector<float> resonanceValues = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};
        
        // Test resonance in bandpass mode (most obvious)
        for (float resonance : resonanceValues) {
            logFile << "\nTesting resonance: " << resonance << std::endl;
            
            std::map<int, float> params = {
                {0, static_cast<float>(testFreq / 20000.0)}, // Frequency
                {1, resonance}, // Variable resonance
                {2, 0.0f}, // No drive
                {3, 0.25f}, // Bandpass mode
                {4, 0.0f}, // Single-pole
                {5, 0.0f}, // No envelope
                {6, 0.01f}, {7, 0.1f}, // Attack/release
                {8, 0.0f}, // No analog
                {9, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Measure Q by looking at -3dB bandwidth
            auto centerResponse = SVFResponseAnalyzer::measureModeResponse(*filter, testFreq, 2, TEST_SAMPLE_RATE);
            auto lowerResponse = SVFResponseAnalyzer::measureModeResponse(*filter, testFreq * 0.8, 2, TEST_SAMPLE_RATE);
            auto upperResponse = SVFResponseAnalyzer::measureModeResponse(*filter, testFreq * 1.25, 2, TEST_SAMPLE_RATE);
            
            double centerGain = centerResponse.magnitude_dB;
            double bandwidth = 0.0; // Simplified bandwidth estimation
            
            logFile << "  Center gain: " << centerGain << " dB" << std::endl;
            logFile << "  Lower (-20%) response: " << lowerResponse.magnitude_dB << " dB" << std::endl;
            logFile << "  Upper (+25%) response: " << upperResponse.magnitude_dB << " dB" << std::endl;
            
            // Higher resonance should increase center gain and decrease bandwidth
            if (resonance > 0.5f) {
                assert(centerGain > -10.0); // Should have reasonable peak
                assert(centerGain > lowerResponse.magnitude_dB + 3.0); // Clear peak
                assert(centerGain > upperResponse.magnitude_dB + 3.0);
            }
        }
        
        logFile << "✓ Resonance behavior tests passed" << std::endl;
    }
    
    void testCascadedConfiguration() {
        logFile << "\n--- Cascaded Configuration Tests ---" << std::endl;
        
        double testFreq = 1000.0;
        
        // Test different pole configurations (1-pole, 2-pole, 4-pole)
        std::vector<float> slopeValues = {0.0f, 0.5f, 1.0f}; // Representing 1, 2, 4 poles
        std::vector<std::string> slopeNames = {"1-pole", "2-pole", "4-pole"};
        
        for (size_t i = 0; i < slopeValues.size(); ++i) {
            logFile << "\nTesting " << slopeNames[i] << " configuration:" << std::endl;
            
            std::map<int, float> params = {
                {0, static_cast<float>(testFreq / 20000.0)}, // Frequency
                {1, 0.3f}, // Low resonance for clean test
                {2, 0.0f}, // No drive
                {3, 0.0f}, // Lowpass mode
                {4, slopeValues[i]}, // Variable slope
                {5, 0.0f}, // No envelope
                {6, 0.01f}, {7, 0.1f}, // Attack/release
                {8, 0.0f}, // No analog
                {9, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Measure rolloff characteristics
            auto response_1x = SVFResponseAnalyzer::measureModeResponse(*filter, testFreq, 0, TEST_SAMPLE_RATE);
            auto response_2x = SVFResponseAnalyzer::measureModeResponse(*filter, testFreq * 2.0, 0, TEST_SAMPLE_RATE);
            auto response_4x = SVFResponseAnalyzer::measureModeResponse(*filter, testFreq * 4.0, 0, TEST_SAMPLE_RATE);
            
            double rolloff_octave = response_4x.magnitude_dB - response_2x.magnitude_dB;
            
            logFile << "  Response at 1x: " << response_1x.magnitude_dB << " dB" << std::endl;
            logFile << "  Response at 2x: " << response_2x.magnitude_dB << " dB" << std::endl;
            logFile << "  Response at 4x: " << response_4x.magnitude_dB << " dB" << std::endl;
            logFile << "  Rolloff per octave: " << rolloff_octave << " dB" << std::endl;
            
            // Verify expected rolloff rates
            double expectedRolloff = -6.0 * (1 + i); // -6dB/oct * number of poles
            double rolloffError = std::abs(rolloff_octave - expectedRolloff);
            
            logFile << "  Expected rolloff: " << expectedRolloff << " dB/oct" << std::endl;
            logFile << "  Rolloff error: " << rolloffError << " dB/oct" << std::endl;
            
            assert(rolloffError < 6.0); // Within reasonable tolerance
        }
        
        logFile << "✓ Cascaded configuration tests passed" << std::endl;
    }
    
    void testEnvelopeFollowing() {
        logFile << "\n--- Envelope Following Tests ---" << std::endl;
        
        // Generate test signal with varying amplitude
        auto envelopeSignal = SVFTestSignalGenerator::generateEnvelopeSignal(1000.0, 5.0, 2.0, TEST_SAMPLE_RATE);
        
        // Test different envelope settings
        std::vector<std::pair<float, float>> envelopeSettings = {
            {0.01f, 0.1f},  // Fast attack, fast release
            {0.1f, 0.5f},   // Medium attack, medium release
            {0.5f, 0.9f}    // Slow attack, slow release
        };
        
        for (const auto& setting : envelopeSettings) {
            float attack = setting.first;
            float release = setting.second;
            
            logFile << "\nTesting envelope following - Attack: " << attack 
                   << ", Release: " << release << std::endl;
            
            std::map<int, float> params = {
                {0, 0.25f}, // 1kHz base frequency
                {1, 0.6f},  // Moderate resonance
                {2, 0.0f},  // No drive
                {3, 0.0f},  // Lowpass mode
                {4, 0.0f},  // Single-pole
                {5, 0.8f},  // Strong envelope following
                {6, attack}, // Variable attack
                {7, release}, // Variable release
                {8, 0.0f},  // No analog
                {9, 1.0f}   // Full wet
            };
            filter->updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, envelopeSignal.size());
            for (size_t i = 0; i < envelopeSignal.size(); ++i) {
                buffer.setSample(0, i, envelopeSignal[i]);
                buffer.setSample(1, i, envelopeSignal[i]);
            }
            
            filter->process(buffer);
            
            // Analyze envelope response
            double maxInput = 0.0;
            double maxOutput = 0.0;
            double avgOutput = 0.0;
            
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                double input = std::abs(envelopeSignal[i]);
                double output = std::abs(buffer.getSample(0, i));
                
                maxInput = std::max(maxInput, input);
                maxOutput = std::max(maxOutput, output);
                avgOutput += output;
            }
            
            avgOutput /= buffer.getNumSamples();
            
            logFile << "  Max input: " << maxInput << std::endl;
            logFile << "  Max output: " << maxOutput << std::endl;
            logFile << "  Average output: " << avgOutput << std::endl;
            
            // Envelope following should modulate the output
            assert(maxOutput > avgOutput * 1.5); // Output should vary with envelope
        }
        
        logFile << "✓ Envelope following tests passed" << std::endl;
    }
    
    void testDriveCharacteristics() {
        logFile << "\n--- Drive Characteristics Tests ---" << std::endl;
        
        std::vector<float> driveValues = {0.0f, 0.3f, 0.6f, 0.9f};
        double testFreq = 1000.0;
        
        for (float drive : driveValues) {
            logFile << "\nTesting drive level: " << drive << std::endl;
            
            std::map<int, float> params = {
                {0, static_cast<float>(testFreq / 20000.0)}, // Frequency
                {1, 0.5f}, // Moderate resonance
                {2, drive}, // Variable drive
                {3, 0.0f}, // Lowpass mode
                {4, 0.0f}, // Single-pole
                {5, 0.0f}, // No envelope
                {6, 0.01f}, {7, 0.1f}, // Attack/release
                {8, 0.0f}, // No analog
                {9, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with sine wave
            auto testSignal = SVFTestSignalGenerator::generateSineWave(testFreq, 0.2, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Measure harmonic distortion (simplified)
            double inputRMS = 0.0;
            double outputRMS = 0.0;
            double outputPeak = 0.0;
            
            for (size_t i = 0; i < testSignal.size(); ++i) {
                inputRMS += testSignal[i] * testSignal[i];
                double output = buffer.getSample(0, i);
                outputRMS += output * output;
                outputPeak = std::max(outputPeak, std::abs(output));
            }
            
            inputRMS = std::sqrt(inputRMS / testSignal.size());
            outputRMS = std::sqrt(outputRMS / testSignal.size());
            
            double gainChange_dB = 20.0 * std::log10(outputRMS / (inputRMS + 1e-15));
            double crestFactor = outputPeak / (outputRMS + 1e-15);
            
            logFile << "  Gain change: " << gainChange_dB << " dB" << std::endl;
            logFile << "  Crest factor: " << crestFactor << std::endl;
            
            // Higher drive should compress crest factor due to saturation
            if (drive > 0.5f) {
                assert(crestFactor < 2.0); // Some compression
            }
        }
        
        logFile << "✓ Drive characteristics tests passed" << std::endl;
    }
    
    void testAnalogModeling() {
        logFile << "\n--- Analog Modeling Tests ---" << std::endl;
        
        double testFreq = 1000.0;
        
        // Test analog vs digital modes
        std::vector<float> analogValues = {0.0f, 0.5f, 1.0f};
        std::vector<std::string> analogNames = {"Digital", "Hybrid", "Full Analog"};
        
        for (size_t i = 0; i < analogValues.size(); ++i) {
            logFile << "\nTesting " << analogNames[i] << " mode:" << std::endl;
            
            std::map<int, float> params = {
                {0, static_cast<float>(testFreq / 20000.0)}, // Frequency
                {1, 0.7f}, // High resonance to emphasize differences
                {2, 0.3f}, // Some drive
                {3, 0.0f}, // Lowpass mode
                {4, 0.0f}, // Single-pole
                {5, 0.0f}, // No envelope
                {6, 0.01f}, {7, 0.1f}, // Attack/release
                {8, analogValues[i]}, // Variable analog modeling
                {9, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with white noise to reveal analog character
            auto noiseSignal = SVFTestSignalGenerator::generateWhiteNoise(0.1, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, noiseSignal.size());
            for (size_t i = 0; i < noiseSignal.size(); ++i) {
                buffer.setSample(0, i, noiseSignal[i]);
                buffer.setSample(1, i, noiseSignal[i]);
            }
            
            filter->process(buffer);
            
            // Measure noise characteristics
            double inputRMS = 0.0;
            double outputRMS = 0.0;
            double outputVariance = 0.0;
            
            for (size_t j = 0; j < noiseSignal.size(); ++j) {
                inputRMS += noiseSignal[j] * noiseSignal[j];
                double output = buffer.getSample(0, j);
                outputRMS += output * output;
            }
            
            inputRMS = std::sqrt(inputRMS / noiseSignal.size());
            outputRMS = std::sqrt(outputRMS / noiseSignal.size());
            
            // Calculate variance for analog character detection
            double outputMean = 0.0;
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                outputMean += buffer.getSample(0, j);
            }
            outputMean /= buffer.getNumSamples();
            
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                double deviation = buffer.getSample(0, j) - outputMean;
                outputVariance += deviation * deviation;
            }
            outputVariance /= buffer.getNumSamples();
            
            double noiseReduction_dB = 20.0 * std::log10(outputRMS / (inputRMS + 1e-15));
            
            logFile << "  Noise reduction: " << noiseReduction_dB << " dB" << std::endl;
            logFile << "  Output variance: " << outputVariance << std::endl;
            
            // Analog modeling should introduce some character
            if (analogValues[i] > 0.5f) {
                assert(outputVariance > 1e-8); // Some analog character
            }
        }
        
        logFile << "✓ Analog modeling tests passed" << std::endl;
    }
    
    void testParameterSmoothing() {
        logFile << "\n--- Parameter Smoothing Tests ---" << std::endl;
        
        // Test rapid parameter changes for smooth transitions
        double testFreq = 1000.0;
        auto testSignal = SVFTestSignalGenerator::generateSineWave(testFreq, 0.1, 2.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, testSignal.size());
        for (size_t i = 0; i < testSignal.size(); ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        // Process with rapid parameter changes
        int samplesPerChange = 1000; // Change every ~23ms
        std::vector<double> parameterJumps;
        
        for (int i = 0; i < buffer.getNumSamples(); i += samplesPerChange) {
            // Alternate between two different settings
            float freqParam = (i / samplesPerChange) % 2 == 0 ? 0.2f : 0.4f;
            float resParam = (i / samplesPerChange) % 2 == 0 ? 0.3f : 0.7f;
            
            std::map<int, float> params = {
                {0, freqParam}, // Alternating frequency
                {1, resParam}, // Alternating resonance
                {2, 0.1f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f},
                {6, 0.01f}, {7, 0.1f}, {8, 0.0f}, {9, 1.0f}
            };
            filter->updateParameters(params);
            
            // Process this chunk
            int endSample = std::min(i + samplesPerChange, buffer.getNumSamples());
            for (int j = i; j < endSample; ++j) {
                juce::AudioBuffer<float> sampleBuffer(2, 1);
                sampleBuffer.setSample(0, 0, testSignal[j]);
                sampleBuffer.setSample(1, 0, testSignal[j]);
                
                filter->process(sampleBuffer);
                buffer.setSample(0, j, sampleBuffer.getSample(0, 0));
                buffer.setSample(1, j, sampleBuffer.getSample(1, 0));
            }
            
            // Check for discontinuities at parameter change points
            if (i > 0 && i < buffer.getNumSamples() - 1) {
                double beforeValue = buffer.getSample(0, i - 1);
                double afterValue = buffer.getSample(0, i);
                double jump = std::abs(afterValue - beforeValue);
                parameterJumps.push_back(jump);
            }
        }
        
        // Analyze smoothing effectiveness
        double maxJump = 0.0;
        double avgJump = 0.0;
        for (double jump : parameterJumps) {
            maxJump = std::max(maxJump, jump);
            avgJump += jump;
        }
        avgJump /= parameterJumps.size();
        
        logFile << "Parameter smoothing analysis:" << std::endl;
        logFile << "  Number of parameter changes: " << parameterJumps.size() << std::endl;
        logFile << "  Maximum jump: " << maxJump << std::endl;
        logFile << "  Average jump: " << avgJump << std::endl;
        
        // Parameter changes should be smooth
        assert(maxJump < 0.1); // No excessive discontinuities
        assert(avgJump < 0.01); // Generally smooth
        
        logFile << "✓ Parameter smoothing tests passed" << std::endl;
    }
    
    void testFrequencyTracking() {
        logFile << "\n--- Frequency Tracking Tests ---" << std::endl;
        
        // Test frequency tracking accuracy across range
        std::vector<double> testFreqs = {100.0, 440.0, 1000.0, 4000.0, 8000.0};
        
        for (double targetFreq : testFreqs) {
            if (targetFreq < TEST_SAMPLE_RATE / 2.1) {
                logFile << "\nTesting frequency tracking at " << targetFreq << " Hz:" << std::endl;
                
                std::map<int, float> params = {
                    {0, static_cast<float>(targetFreq / 20000.0)}, // Target frequency
                    {1, 0.8f}, // High resonance for clear peak
                    {2, 0.0f}, // No drive
                    {3, 0.25f}, // Bandpass mode for peak detection
                    {4, 0.0f}, // Single-pole
                    {5, 0.0f}, // No envelope
                    {6, 0.01f}, {7, 0.1f}, // Attack/release
                    {8, 0.0f}, // No analog
                    {9, 1.0f}  // Full wet
                };
                filter->updateParameters(params);
                
                // Find actual peak frequency by testing around target
                double bestFreq = targetFreq;
                double bestResponse = -100.0;
                
                for (double testRatio = 0.8; testRatio <= 1.2; testRatio += 0.05) {
                    double testFreq = targetFreq * testRatio;
                    auto response = SVFResponseAnalyzer::measureModeResponse(*filter, testFreq, 2, TEST_SAMPLE_RATE);
                    
                    if (response.magnitude_dB > bestResponse) {
                        bestResponse = response.magnitude_dB;
                        bestFreq = testFreq;
                    }
                }
                
                double freqError = std::abs(bestFreq - targetFreq) / targetFreq;
                
                logFile << "  Target frequency: " << targetFreq << " Hz" << std::endl;
                logFile << "  Actual peak frequency: " << bestFreq << " Hz" << std::endl;
                logFile << "  Frequency error: " << freqError * 100.0 << "%" << std::endl;
                logFile << "  Peak response: " << bestResponse << " dB" << std::endl;
                
                // Verify frequency tracking accuracy
                assert(freqError < FREQ_EPSILON);
                assert(bestResponse > -10.0); // Should have clear peak
            }
        }
        
        logFile << "✓ Frequency tracking tests passed" << std::endl;
    }
    
    void testStabilityAndLimits() {
        logFile << "\n--- Stability and Limits Tests ---" << std::endl;
        
        // Test extreme parameter combinations
        std::vector<std::map<int, float>> extremeSettings = {
            // Maximum everything
            {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}, {4, 1.0f}, 
             {5, 1.0f}, {6, 1.0f}, {7, 1.0f}, {8, 1.0f}, {9, 1.0f}},
            
            // Minimum everything
            {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, {4, 0.0f}, 
             {5, 0.0f}, {6, 0.0f}, {7, 0.0f}, {8, 0.0f}, {9, 1.0f}},
            
            // High frequency, high resonance
            {{0, 0.9f}, {1, 0.95f}, {2, 0.5f}, {3, 0.0f}, {4, 1.0f}, 
             {5, 0.0f}, {6, 0.01f}, {7, 0.1f}, {8, 0.5f}, {9, 1.0f}},
             
            // Random extreme combination
            {{0, 0.8f}, {1, 0.9f}, {2, 0.8f}, {3, 0.75f}, {4, 0.8f}, 
             {5, 0.7f}, {6, 0.9f}, {7, 0.8f}, {8, 0.9f}, {9, 1.0f}}
        };
        
        for (size_t i = 0; i < extremeSettings.size(); ++i) {
            logFile << "\nTesting extreme setting " << i + 1 << ":" << std::endl;
            
            filter->updateParameters(extremeSettings[i]);
            
            // Test with white noise
            auto noiseSignal = SVFTestSignalGenerator::generateWhiteNoise(0.1, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, noiseSignal.size());
            for (size_t j = 0; j < noiseSignal.size(); ++j) {
                buffer.setSample(0, j, noiseSignal[j]);
                buffer.setSample(1, j, noiseSignal[j]);
            }
            
            filter->process(buffer);
            
            // Check for stability
            bool stable = true;
            double maxOutput = 0.0;
            double avgOutput = 0.0;
            
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                float sample = buffer.getSample(0, j);
                
                if (std::isnan(sample) || std::isinf(sample)) {
                    stable = false;
                    break;
                }
                
                double absValue = std::abs(sample);
                maxOutput = std::max(maxOutput, absValue);
                avgOutput += absValue;
            }
            
            avgOutput /= buffer.getNumSamples();
            
            logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
            logFile << "  Max output: " << maxOutput << std::endl;
            logFile << "  Average output: " << avgOutput << std::endl;
            
            assert(stable);
            assert(maxOutput < 100.0); // Reasonable bounds
        }
        
        logFile << "✓ Stability and limits tests passed" << std::endl;
    }
    
    void testTransientResponse() {
        logFile << "\n--- Transient Response Tests ---" << std::endl;
        
        // Test impulse response in different modes
        std::vector<int> modes = {0, 1, 2, 3}; // LP, HP, BP, Notch
        std::vector<std::string> modeNames = {"Lowpass", "Highpass", "Bandpass", "Notch"};
        
        for (size_t i = 0; i < modes.size(); ++i) {
            logFile << "\nTesting " << modeNames[i] << " impulse response:" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.25f}, // 1kHz cutoff
                {1, 0.6f}, // Moderate resonance
                {2, 0.0f}, // No drive
                {3, static_cast<float>(modes[i]) / 8.0f}, // Mode
                {4, 0.0f}, // Single-pole
                {5, 0.0f}, // No envelope
                {6, 0.01f}, {7, 0.1f}, // Attack/release
                {8, 0.0f}, // No analog
                {9, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Generate impulse
            auto impulse = SVFTestSignalGenerator::generateImpulse(1.0, 100, static_cast<int>(0.5 * TEST_SAMPLE_RATE));
            
            juce::AudioBuffer<float> buffer(2, impulse.size());
            for (size_t j = 0; j < impulse.size(); ++j) {
                buffer.setSample(0, j, impulse[j]);
                buffer.setSample(1, j, impulse[j]);
            }
            
            filter->process(buffer);
            
            // Analyze impulse response
            double totalEnergy = 0.0;
            double peakResponse = 0.0;
            int peakPosition = 0;
            int settlingTime = 0;
            
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                double output = std::abs(buffer.getSample(0, j));
                totalEnergy += output * output;
                
                if (output > peakResponse) {
                    peakResponse = output;
                    peakPosition = j;
                }
                
                // Find settling time (when response drops below 1% of peak)
                if (settlingTime == 0 && j > peakPosition + 100 && output < peakResponse * 0.01) {
                    settlingTime = j - 100; // Account for impulse position
                }
            }
            
            logFile << "  Total energy: " << totalEnergy << std::endl;
            logFile << "  Peak response: " << peakResponse << std::endl;
            logFile << "  Peak position: " << peakPosition << " samples" << std::endl;
            logFile << "  Settling time: " << settlingTime << " samples (" 
                   << settlingTime / TEST_SAMPLE_RATE * 1000.0 << " ms)" << std::endl;
            
            // Verify reasonable impulse response
            assert(peakResponse > 0.01); // Should respond to impulse
            assert(peakPosition >= 100 && peakPosition <= 200); // Peak should occur near impulse
            if (settlingTime > 0) {
                assert(settlingTime < TEST_SAMPLE_RATE * 0.2); // Should settle within 200ms
            }
        }
        
        logFile << "✓ Transient response tests passed" << std::endl;
    }
    
    void testZeroDelayTopology() {
        logFile << "\n--- Zero-Delay Topology Tests ---" << std::endl;
        
        // Test that zero-delay feedback is working correctly
        // by comparing with expected theoretical response
        double testFreq = 1000.0;
        
        std::map<int, float> params = {
            {0, static_cast<float>(testFreq / 20000.0)}, // 1kHz cutoff
            {1, 0.7f}, // High resonance to emphasize feedback
            {2, 0.0f}, // No drive for clean test
            {3, 0.25f}, // Bandpass mode
            {4, 0.0f}, // Single-pole
            {5, 0.0f}, // No envelope
            {6, 0.01f}, {7, 0.1f}, // Attack/release
            {8, 0.0f}, // No analog
            {9, 1.0f}  // Full wet
        };
        filter->updateParameters(params);
        
        // Test with impulse to see if feedback topology is correct
        auto impulse = SVFTestSignalGenerator::generateImpulse(0.1, 0, static_cast<int>(0.2 * TEST_SAMPLE_RATE));
        
        juce::AudioBuffer<float> buffer(2, impulse.size());
        for (size_t i = 0; i < impulse.size(); ++i) {
            buffer.setSample(0, i, impulse[i]);
            buffer.setSample(1, i, impulse[i]);
        }
        
        filter->process(buffer);
        
        // Analyze feedback characteristics
        double totalEnergy = 0.0;
        double peakOutput = 0.0;
        std::vector<double> ringFreq;
        
        // Simple peak detection for ringing frequency
        for (int i = 1; i < buffer.getNumSamples() - 1; ++i) {
            double prev = buffer.getSample(0, i - 1);
            double curr = buffer.getSample(0, i);
            double next = buffer.getSample(0, i + 1);
            
            totalEnergy += curr * curr;
            peakOutput = std::max(peakOutput, std::abs(curr));
            
            // Detect zero crossings for frequency estimation
            if ((prev < 0 && curr >= 0) || (prev > 0 && curr <= 0)) {
                if (std::abs(curr) > peakOutput * 0.1) { // Only significant crossings
                    ringFreq.push_back(i);
                }
            }
        }
        
        // Estimate ringing frequency
        double avgPeriod = 0.0;
        if (ringFreq.size() > 2) {
            for (size_t i = 2; i < ringFreq.size(); ++i) {
                avgPeriod += ringFreq[i] - ringFreq[i - 2]; // Full cycle
            }
            avgPeriod /= (ringFreq.size() - 2);
            avgPeriod /= 2.0; // Convert to half-cycles to full cycles
        }
        
        double estimatedFreq = avgPeriod > 0 ? TEST_SAMPLE_RATE / avgPeriod : 0.0;
        
        logFile << "Zero-delay feedback analysis:" << std::endl;
        logFile << "  Total energy: " << totalEnergy << std::endl;
        logFile << "  Peak output: " << peakOutput << std::endl;
        logFile << "  Estimated ring frequency: " << estimatedFreq << " Hz" << std::endl;
        logFile << "  Expected frequency: " << testFreq << " Hz" << std::endl;
        
        // Verify that ringing occurs near the cutoff frequency
        if (estimatedFreq > 0) {
            double freqError = std::abs(estimatedFreq - testFreq) / testFreq;
            logFile << "  Frequency error: " << freqError * 100.0 << "%" << std::endl;
            
            assert(freqError < 0.2); // Within 20% for this simplified analysis
        }
        
        // High resonance should produce significant ringing
        assert(totalEnergy > 0.01);
        assert(peakOutput > 0.05);
        
        logFile << "✓ Zero-delay topology tests passed" << std::endl;
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
    std::cout << "Starting State Variable Filter comprehensive test suite..." << std::endl;
    
    try {
        StateVariableFilterTestSuite testSuite;
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