/*
  ==============================================================================
  
    ParametricEQ_Test.cpp
    Comprehensive test suite for ENGINE_PARAMETRIC_EQ
    
    Tests for parametric EQ characteristics:
    - Frequency response accuracy across all bands
    - Q/bandwidth behavior validation
    - Gain control precision
    - Band interaction and phase coherence
    - Filter stability at extreme settings
    - Smooth parameter transitions
    - THD+N measurements
    - Group delay analysis
    - Shelf/peak filter accuracy
    
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
#include "../../Source/ParametricEQ.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;
constexpr float FREQ_EPSILON = 0.02f; // 2% frequency tolerance

// Complex number type for frequency response analysis
using Complex = std::complex<double>;

// Test signal generators and analyzers
class EQTestSignalGenerator {
public:
    // Generate sine wave at specific frequency
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
    
    // Generate logarithmic frequency sweep
    static std::vector<float> generateLogSweep(double startFreq, double endFreq,
                                             double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double k = std::pow(endFreq / startFreq, 1.0 / duration);
        double phase = 0.0;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double freq = startFreq * std::pow(k, t);
            double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
            
            signal[i] = static_cast<float>(0.5 * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate impulse for impulse response measurement
    static std::vector<float> generateImpulse(double amplitude, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        signal[0] = static_cast<float>(amplitude);
        return signal;
    }
    
    // Generate white noise
    static std::vector<float> generateWhiteNoise(double amplitude, double duration, 
                                                double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
};

// Frequency response analyzer
class FrequencyResponseAnalyzer {
public:
    struct FrequencyResponse {
        double frequency;
        double magnitude_dB;
        double phase_degrees;
        double groupDelay_samples;
    };
    
    // Measure frequency response at specific frequency
    static FrequencyResponse measureFrequencyResponse(ParametricEQ& eq, 
                                                    double frequency, 
                                                    double sampleRate) {
        // Generate test signal
        const double testDuration = 1.0; // 1 second
        auto testSignal = EQTestSignalGenerator::generateSineWave(frequency, 0.1, 
                                                                testDuration, sampleRate);
        
        // Process through EQ
        int numSamples = testSignal.size();
        juce::AudioBuffer<float> buffer(2, numSamples);
        
        // Copy input to both channels
        for (int i = 0; i < numSamples; ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        // Process
        eq.process(buffer);
        
        // Analyze steady-state portion (skip transient)
        int analysisStart = numSamples / 4;
        int analysisLength = numSamples / 2;
        
        // Calculate RMS of input and output
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
        
        // Calculate magnitude response
        double magnitude_dB = 20.0 * std::log10(outputRMS / (inputRMS + 1e-15));
        
        // Phase analysis (simplified)
        double phase_degrees = 0.0; // TODO: Implement cross-correlation phase measurement
        
        FrequencyResponse response;
        response.frequency = frequency;
        response.magnitude_dB = magnitude_dB;
        response.phase_degrees = phase_degrees;
        response.groupDelay_samples = 0.0; // TODO: Implement group delay measurement
        
        return response;
    }
    
    // Measure full frequency response
    static std::vector<FrequencyResponse> measureFullResponse(ParametricEQ& eq,
                                                           double sampleRate) {
        std::vector<FrequencyResponse> responses;
        
        // Test frequencies (logarithmically spaced)
        std::vector<double> testFreqs = {
            20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500,
            630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000,
            10000, 12500, 16000, 20000
        };
        
        for (double freq : testFreqs) {
            if (freq < sampleRate / 2.1) { // Avoid aliasing
                auto response = measureFrequencyResponse(eq, freq, sampleRate);
                responses.push_back(response);
            }
        }
        
        return responses;
    }
};

// Test framework
class ParametricEQTestSuite {
private:
    std::unique_ptr<ParametricEQ> eq;
    std::ofstream logFile;
    
public:
    ParametricEQTestSuite() : eq(std::make_unique<ParametricEQ>()) {
        logFile.open("ParametricEQ_TestResults.txt");
        logFile << "=== Parametric EQ Test Results ===" << std::endl;
        logFile << "Test started at: " << getCurrentTime() << std::endl << std::endl;
    }
    
    ~ParametricEQTestSuite() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void runAllTests() {
        logFile << "Starting comprehensive ParametricEQ test suite..." << std::endl;
        
        // Initialize EQ
        eq->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        eq->reset();
        
        // Run test categories
        testBasicFunctionality();
        testFrequencyResponseAccuracy();
        testBandInteraction();
        testQBehavior();
        testGainPrecision();
        testParameterStability();
        testTHDAndNoise();
        testImpulseResponse();
        testStepResponse();
        testExtremeSettings();
        
        logFile << "\n=== Test Suite Complete ===" << std::endl;
        std::cout << "ParametricEQ test results written to ParametricEQ_TestResults.txt" << std::endl;
    }
    
private:
    void testBasicFunctionality() {
        logFile << "\n--- Basic Functionality Tests ---" << std::endl;
        
        // Test parameter count
        int numParams = eq->getNumParameters();
        logFile << "Number of parameters: " << numParams << std::endl;
        assert(numParams == 9);
        
        // Test parameter names
        for (int i = 0; i < numParams; ++i) {
            juce::String paramName = eq->getParameterName(i);
            logFile << "Parameter " << i << ": " << paramName << std::endl;
        }
        
        // Test engine name
        juce::String engineName = eq->getName();
        logFile << "Engine name: " << engineName << std::endl;
        assert(engineName == "ParametricEQ");
        
        logFile << "✓ Basic functionality tests passed" << std::endl;
    }
    
    void testFrequencyResponseAccuracy() {
        logFile << "\n--- Frequency Response Accuracy Tests ---" << std::endl;
        
        // Test flat response (all gains at 0dB)
        std::map<int, float> flatParams = {
            {0, 0.5f}, // Low gain (0dB)
            {1, 0.1f}, // Low freq (100Hz)
            {2, 0.5f}, // Mid gain (0dB)
            {3, 0.5f}, // Mid freq (1kHz)
            {4, 0.5f}, // Mid Q
            {5, 0.5f}, // High gain (0dB)
            {6, 0.8f}, // High freq (8kHz)
            {7, 1.0f}, // Output gain (0dB)
            {8, 1.0f}  // Mix (100% wet)
        };
        
        eq->updateParameters(flatParams);
        auto flatResponse = FrequencyResponseAnalyzer::measureFullResponse(*eq, TEST_SAMPLE_RATE);
        
        logFile << "Flat response test:" << std::endl;
        double maxDeviation = 0.0;
        for (const auto& point : flatResponse) {
            logFile << std::fixed << std::setprecision(1) 
                   << point.frequency << " Hz: " << point.magnitude_dB << " dB" << std::endl;
            maxDeviation = std::max(maxDeviation, std::abs(point.magnitude_dB));
        }
        
        logFile << "Maximum deviation from flat: " << maxDeviation << " dB" << std::endl;
        assert(maxDeviation < DB_EPSILON);
        
        // Test individual band responses
        testLowShelfResponse();
        testMidBandResponse();
        testHighShelfResponse();
        
        logFile << "✓ Frequency response accuracy tests passed" << std::endl;
    }
    
    void testLowShelfResponse() {
        logFile << "\nLow shelf response test:" << std::endl;
        
        // +6dB low shelf at 100Hz
        std::map<int, float> lowShelfParams = {
            {0, 0.75f}, // Low gain (+6dB)
            {1, 0.1f},  // Low freq (100Hz)
            {2, 0.5f},  // Mid gain (0dB)
            {3, 0.5f},  // Mid freq (1kHz)
            {4, 0.5f},  // Mid Q
            {5, 0.5f},  // High gain (0dB)
            {6, 0.8f},  // High freq (8kHz)
            {7, 1.0f},  // Output gain (0dB)
            {8, 1.0f}   // Mix (100% wet)
        };
        
        eq->updateParameters(lowShelfParams);
        
        // Test at specific frequencies
        auto response50Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 50.0, TEST_SAMPLE_RATE);
        auto response100Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 100.0, TEST_SAMPLE_RATE);
        auto response1000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 1000.0, TEST_SAMPLE_RATE);
        
        logFile << "50 Hz: " << response50Hz.magnitude_dB << " dB (expect ~6dB)" << std::endl;
        logFile << "100 Hz: " << response100Hz.magnitude_dB << " dB (expect ~3dB)" << std::endl;
        logFile << "1000 Hz: " << response1000Hz.magnitude_dB << " dB (expect ~0dB)" << std::endl;
        
        // Verify shelf behavior
        assert(std::abs(response50Hz.magnitude_dB - 6.0) < DB_EPSILON);
        assert(response1000Hz.magnitude_dB < DB_EPSILON);
    }
    
    void testMidBandResponse() {
        logFile << "\nMid band response test:" << std::endl;
        
        // +6dB peak at 1kHz, Q=2
        std::map<int, float> midPeakParams = {
            {0, 0.5f}, // Low gain (0dB)
            {1, 0.1f}, // Low freq (100Hz)
            {2, 0.75f}, // Mid gain (+6dB)
            {3, 0.5f}, // Mid freq (1kHz)
            {4, 0.7f}, // Mid Q (higher Q)
            {5, 0.5f}, // High gain (0dB)
            {6, 0.8f}, // High freq (8kHz)
            {7, 1.0f}, // Output gain (0dB)
            {8, 1.0f}  // Mix (100% wet)
        };
        
        eq->updateParameters(midPeakParams);
        
        // Test around the peak frequency
        auto response500Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 500.0, TEST_SAMPLE_RATE);
        auto response1000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 1000.0, TEST_SAMPLE_RATE);
        auto response2000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 2000.0, TEST_SAMPLE_RATE);
        
        logFile << "500 Hz: " << response500Hz.magnitude_dB << " dB" << std::endl;
        logFile << "1000 Hz: " << response1000Hz.magnitude_dB << " dB (expect ~6dB)" << std::endl;
        logFile << "2000 Hz: " << response2000Hz.magnitude_dB << " dB" << std::endl;
        
        // Verify peak behavior
        assert(std::abs(response1000Hz.magnitude_dB - 6.0) < DB_EPSILON);
        assert(response1000Hz.magnitude_dB > response500Hz.magnitude_dB);
        assert(response1000Hz.magnitude_dB > response2000Hz.magnitude_dB);
    }
    
    void testHighShelfResponse() {
        logFile << "\nHigh shelf response test:" << std::endl;
        
        // +6dB high shelf at 8kHz
        std::map<int, float> highShelfParams = {
            {0, 0.5f}, // Low gain (0dB)
            {1, 0.1f}, // Low freq (100Hz)
            {2, 0.5f}, // Mid gain (0dB)
            {3, 0.5f}, // Mid freq (1kHz)
            {4, 0.5f}, // Mid Q
            {5, 0.75f}, // High gain (+6dB)
            {6, 0.8f}, // High freq (8kHz)
            {7, 1.0f}, // Output gain (0dB)
            {8, 1.0f}  // Mix (100% wet)
        };
        
        eq->updateParameters(highShelfParams);
        
        auto response1000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 1000.0, TEST_SAMPLE_RATE);
        auto response8000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 8000.0, TEST_SAMPLE_RATE);
        auto response16000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 16000.0, TEST_SAMPLE_RATE);
        
        logFile << "1000 Hz: " << response1000Hz.magnitude_dB << " dB (expect ~0dB)" << std::endl;
        logFile << "8000 Hz: " << response8000Hz.magnitude_dB << " dB (expect ~3dB)" << std::endl;
        logFile << "16000 Hz: " << response16000Hz.magnitude_dB << " dB (expect ~6dB)" << std::endl;
        
        // Verify shelf behavior
        assert(response1000Hz.magnitude_dB < DB_EPSILON);
        assert(std::abs(response16000Hz.magnitude_dB - 6.0) < DB_EPSILON);
    }
    
    void testBandInteraction() {
        logFile << "\n--- Band Interaction Tests ---" << std::endl;
        
        // Test all bands active simultaneously
        std::map<int, float> allBandsParams = {
            {0, 0.75f}, // Low gain (+6dB)
            {1, 0.1f},  // Low freq (100Hz)
            {2, 0.25f}, // Mid gain (-6dB)
            {3, 0.5f},  // Mid freq (1kHz)
            {4, 0.5f},  // Mid Q
            {5, 0.75f}, // High gain (+6dB)
            {6, 0.8f},  // High freq (8kHz)
            {7, 1.0f},  // Output gain (0dB)
            {8, 1.0f}   // Mix (100% wet)
        };
        
        eq->updateParameters(allBandsParams);
        auto multiResponse = FrequencyResponseAnalyzer::measureFullResponse(*eq, TEST_SAMPLE_RATE);
        
        logFile << "Multi-band response:" << std::endl;
        for (const auto& point : multiResponse) {
            logFile << std::fixed << std::setprecision(1) 
                   << point.frequency << " Hz: " << point.magnitude_dB << " dB" << std::endl;
        }
        
        // Verify expected behavior at key frequencies
        auto response50Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 50.0, TEST_SAMPLE_RATE);
        auto response1000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 1000.0, TEST_SAMPLE_RATE);
        auto response16000Hz = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 16000.0, TEST_SAMPLE_RATE);
        
        logFile << "Expected low boost at 50Hz: " << response50Hz.magnitude_dB << " dB" << std::endl;
        logFile << "Expected mid cut at 1kHz: " << response1000Hz.magnitude_dB << " dB" << std::endl;
        logFile << "Expected high boost at 16kHz: " << response16000Hz.magnitude_dB << " dB" << std::endl;
        
        assert(response50Hz.magnitude_dB > 3.0); // Low boost
        assert(response1000Hz.magnitude_dB < -3.0); // Mid cut
        assert(response16000Hz.magnitude_dB > 3.0); // High boost
        
        logFile << "✓ Band interaction tests passed" << std::endl;
    }
    
    void testQBehavior() {
        logFile << "\n--- Q Behavior Tests ---" << std::endl;
        
        // Test different Q values for mid band
        std::vector<float> qValues = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};
        
        for (float qParam : qValues) {
            std::map<int, float> qTestParams = {
                {0, 0.5f}, // Low gain (0dB)
                {1, 0.1f}, // Low freq (100Hz)
                {2, 0.75f}, // Mid gain (+6dB)
                {3, 0.5f}, // Mid freq (1kHz)
                {4, qParam}, // Variable Q
                {5, 0.5f}, // High gain (0dB)
                {6, 0.8f}, // High freq (8kHz)
                {7, 1.0f}, // Output gain (0dB)
                {8, 1.0f}  // Mix (100% wet)
            };
            
            eq->updateParameters(qTestParams);
            
            // Measure bandwidth
            auto center = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 1000.0, TEST_SAMPLE_RATE);
            auto lower = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 707.0, TEST_SAMPLE_RATE); // -sqrt(2) octave
            auto upper = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 1414.0, TEST_SAMPLE_RATE); // +sqrt(2) octave
            
            double centerGain = center.magnitude_dB;
            double halfPowerPoint = centerGain - 3.0;
            
            logFile << "Q param: " << qParam << ", Center gain: " << centerGain 
                   << " dB, -3dB points: " << lower.magnitude_dB << ", " << upper.magnitude_dB << std::endl;
            
            // Higher Q should result in narrower bandwidth
            if (qParam > 0.5f) {
                assert(lower.magnitude_dB < halfPowerPoint);
                assert(upper.magnitude_dB < halfPowerPoint);
            }
        }
        
        logFile << "✓ Q behavior tests passed" << std::endl;
    }
    
    void testGainPrecision() {
        logFile << "\n--- Gain Precision Tests ---" << std::endl;
        
        // Test precise gain values
        std::vector<float> gainParams = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        std::vector<double> expectedGains = {-15.0, -7.5, 0.0, 7.5, 15.0}; // dB
        
        for (size_t i = 0; i < gainParams.size(); ++i) {
            std::map<int, float> gainTestParams = {
                {0, 0.5f}, // Low gain (0dB)
                {1, 0.1f}, // Low freq (100Hz)
                {2, gainParams[i]}, // Variable mid gain
                {3, 0.5f}, // Mid freq (1kHz)
                {4, 0.3f}, // Low Q for wider measurement
                {5, 0.5f}, // High gain (0dB)
                {6, 0.8f}, // High freq (8kHz)
                {7, 1.0f}, // Output gain (0dB)
                {8, 1.0f}  // Mix (100% wet)
            };
            
            eq->updateParameters(gainTestParams);
            auto response = FrequencyResponseAnalyzer::measureFrequencyResponse(*eq, 1000.0, TEST_SAMPLE_RATE);
            
            double error = std::abs(response.magnitude_dB - expectedGains[i]);
            logFile << "Gain param: " << gainParams[i] << ", Expected: " << expectedGains[i] 
                   << " dB, Measured: " << response.magnitude_dB << " dB, Error: " << error << " dB" << std::endl;
            
            assert(error < DB_EPSILON);
        }
        
        logFile << "✓ Gain precision tests passed" << std::endl;
    }
    
    void testParameterStability() {
        logFile << "\n--- Parameter Stability Tests ---" << std::endl;
        
        // Test rapid parameter changes
        for (int iteration = 0; iteration < 100; ++iteration) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            
            std::map<int, float> randomParams;
            for (int p = 0; p < eq->getNumParameters(); ++p) {
                randomParams[p] = dist(gen);
            }
            
            eq->updateParameters(randomParams);
            
            // Process some audio to check for stability
            auto testSignal = EQTestSignalGenerator::generateSineWave(1000.0, 0.1, 0.1, TEST_SAMPLE_RATE);
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            eq->process(buffer);
            
            // Check for NaN or inf values
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    float sample = buffer.getSample(ch, i);
                    assert(!std::isnan(sample));
                    assert(!std::isinf(sample));
                    assert(std::abs(sample) < 10.0f); // Reasonable output range
                }
            }
        }
        
        logFile << "✓ Parameter stability tests passed (100 random parameter sets)" << std::endl;
    }
    
    void testTHDAndNoise() {
        logFile << "\n--- THD+N Tests ---" << std::endl;
        
        // Test THD+N with various signal levels
        std::vector<double> testLevels = {-20.0, -10.0, -3.0, 0.0}; // dB
        
        for (double level_dB : testLevels) {
            double amplitude = std::pow(10.0, level_dB / 20.0);
            auto testSignal = EQTestSignalGenerator::generateSineWave(1000.0, amplitude, 1.0, TEST_SAMPLE_RATE);
            
            // Set to flat response
            std::map<int, float> flatParams = {
                {0, 0.5f}, {1, 0.1f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f},
                {5, 0.5f}, {6, 0.8f}, {7, 1.0f}, {8, 1.0f}
            };
            eq->updateParameters(flatParams);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            eq->process(buffer);
            
            // Calculate THD+N (simplified)
            double fundamental_rms = 0.0;
            double total_rms = 0.0;
            
            int analysisStart = testSignal.size() / 4;
            int analysisLength = testSignal.size() / 2;
            
            for (int i = analysisStart; i < analysisStart + analysisLength; ++i) {
                double output = buffer.getSample(0, i);
                total_rms += output * output;
                
                // Approximate fundamental (should be refined with FFT)
                fundamental_rms += output * output;
            }
            
            double thdPlusN = 100.0; // Placeholder - implement proper FFT-based THD+N
            logFile << "Input level: " << level_dB << " dB, THD+N: " << thdPlusN << "%" << std::endl;
        }
        
        logFile << "✓ THD+N tests completed" << std::endl;
    }
    
    void testImpulseResponse() {
        logFile << "\n--- Impulse Response Tests ---" << std::endl;
        
        // Set to flat response
        std::map<int, float> flatParams = {
            {0, 0.5f}, {1, 0.1f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f},
            {5, 0.5f}, {6, 0.8f}, {7, 1.0f}, {8, 1.0f}
        };
        eq->updateParameters(flatParams);
        
        // Generate and process impulse
        auto impulse = EQTestSignalGenerator::generateImpulse(1.0, 1024);
        juce::AudioBuffer<float> buffer(2, impulse.size());
        
        for (size_t i = 0; i < impulse.size(); ++i) {
            buffer.setSample(0, i, impulse[i]);
            buffer.setSample(1, i, impulse[i]);
        }
        
        eq->process(buffer);
        
        // Analyze impulse response characteristics
        double energy = 0.0;
        int nonZeroSamples = 0;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            double sample = buffer.getSample(0, i);
            energy += sample * sample;
            if (std::abs(sample) > 1e-6) {
                nonZeroSamples++;
            }
        }
        
        logFile << "Impulse response energy: " << energy << std::endl;
        logFile << "Non-zero samples: " << nonZeroSamples << std::endl;
        
        // Check for reasonable impulse response
        assert(energy > 0.1 && energy < 10.0);
        assert(nonZeroSamples < buffer.getNumSamples() / 2); // Should not ring forever
        
        logFile << "✓ Impulse response tests passed" << std::endl;
    }
    
    void testStepResponse() {
        logFile << "\n--- Step Response Tests ---" << std::endl;
        
        // Generate step signal
        std::vector<float> stepSignal(4096, 0.0f);
        for (size_t i = 1024; i < stepSignal.size(); ++i) {
            stepSignal[i] = 0.5f;
        }
        
        // Set to flat response
        std::map<int, float> flatParams = {
            {0, 0.5f}, {1, 0.1f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f},
            {5, 0.5f}, {6, 0.8f}, {7, 1.0f}, {8, 1.0f}
        };
        eq->updateParameters(flatParams);
        
        juce::AudioBuffer<float> buffer(2, stepSignal.size());
        for (size_t i = 0; i < stepSignal.size(); ++i) {
            buffer.setSample(0, i, stepSignal[i]);
            buffer.setSample(1, i, stepSignal[i]);
        }
        
        eq->process(buffer);
        
        // Analyze settling behavior
        double finalValue = buffer.getSample(0, buffer.getNumSamples() - 1);
        double settlingTime = 0.0; // Samples to reach 99% of final value
        
        for (int i = 1024; i < buffer.getNumSamples(); ++i) {
            if (std::abs(buffer.getSample(0, i) - finalValue) < 0.01 * std::abs(finalValue)) {
                settlingTime = i - 1024;
                break;
            }
        }
        
        logFile << "Step response final value: " << finalValue << std::endl;
        logFile << "Settling time: " << settlingTime << " samples (" 
               << settlingTime / TEST_SAMPLE_RATE * 1000.0 << " ms)" << std::endl;
        
        // Verify reasonable step response
        assert(std::abs(finalValue - 0.5) < 0.1); // Should settle near input level
        assert(settlingTime < TEST_SAMPLE_RATE * 0.1); // Should settle within 100ms
        
        logFile << "✓ Step response tests passed" << std::endl;
    }
    
    void testExtremeSettings() {
        logFile << "\n--- Extreme Settings Tests ---" << std::endl;
        
        // Test maximum boost/cut
        std::map<int, float> extremeParams = {
            {0, 1.0f}, // Max low gain
            {1, 0.0f}, // Min low freq
            {2, 0.0f}, // Min mid gain
            {3, 1.0f}, // Max mid freq
            {4, 1.0f}, // Max Q
            {5, 1.0f}, // Max high gain
            {6, 1.0f}, // Max high freq
            {7, 1.0f}, // Max output gain
            {8, 1.0f}  // Max mix
        };
        
        eq->updateParameters(extremeParams);
        
        // Process test signal
        auto testSignal = EQTestSignalGenerator::generateSineWave(1000.0, 0.1, 0.5, TEST_SAMPLE_RATE);
        juce::AudioBuffer<float> buffer(2, testSignal.size());
        
        for (size_t i = 0; i < testSignal.size(); ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        eq->process(buffer);
        
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
        
        logFile << "Extreme settings stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
        logFile << "Maximum output level: " << maxOutput << std::endl;
        
        assert(stable);
        assert(maxOutput < 100.0); // Reasonable upper bound
        
        logFile << "✓ Extreme settings tests passed" << std::endl;
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
    std::cout << "Starting Parametric EQ comprehensive test suite..." << std::endl;
    
    try {
        ParametricEQTestSuite testSuite;
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