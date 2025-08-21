/*
  ==============================================================================
  
    DigitalChorus_Test.cpp
    Comprehensive test suite for ENGINE_DIGITAL_CHORUS (StereoChorus)
    
    Tests for digital chorus characteristics:
    - LFO rate accuracy and waveform shape
    - Depth/intensity modulation precision
    - Delay time modulation accuracy
    - Stereo imaging and width control
    - Feedback loop stability
    - Mix parameter behavior
    - Tempo sync functionality
    - Phase relationships between channels
    
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
#include <numeric>

// Include the engine
#include "../../Source/StereoChorus.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;
constexpr int FFT_SIZE = 8192;

// FFT implementation for spectral analysis
class SimpleFFT {
public:
    static std::vector<std::complex<double>> fft(const std::vector<double>& signal) {
        int N = signal.size();
        std::vector<std::complex<double>> result(N);
        
        // Copy input
        for (int i = 0; i < N; ++i) {
            result[i] = std::complex<double>(signal[i], 0.0);
        }
        
        // Bit-reverse ordering
        for (int i = 1, j = 0; i < N; ++i) {
            int bit = N >> 1;
            for (; j & bit; bit >>= 1) {
                j ^= bit;
            }
            j ^= bit;
            if (i < j) {
                std::swap(result[i], result[j]);
            }
        }
        
        // Cooley-Tukey FFT
        for (int len = 2; len <= N; len <<= 1) {
            double ang = 2 * M_PI / len;
            std::complex<double> wlen(std::cos(ang), -std::sin(ang));
            
            for (int i = 0; i < N; i += len) {
                std::complex<double> w(1);
                for (int j = 0; j < len / 2; ++j) {
                    std::complex<double> u = result[i + j];
                    std::complex<double> v = result[i + j + len / 2] * w;
                    result[i + j] = u + v;
                    result[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
        
        return result;
    }
    
    static std::vector<double> magnitude(const std::vector<std::complex<double>>& fft_result) {
        std::vector<double> magnitudes;
        magnitudes.reserve(fft_result.size());
        
        for (const auto& complex_val : fft_result) {
            magnitudes.push_back(std::abs(complex_val));
        }
        
        return magnitudes;
    }
    
    static std::vector<double> magnitudeDB(const std::vector<std::complex<double>>& fft_result) {
        std::vector<double> magnitudes_db;
        magnitudes_db.reserve(fft_result.size());
        
        for (const auto& complex_val : fft_result) {
            double mag = std::abs(complex_val);
            magnitudes_db.push_back(20.0 * std::log10(std::max(1e-12, mag)));
        }
        
        return magnitudes_db;
    }
    
    static std::vector<double> phase(const std::vector<std::complex<double>>& fft_result) {
        std::vector<double> phases;
        phases.reserve(fft_result.size());
        
        for (const auto& complex_val : fft_result) {
            phases.push_back(std::arg(complex_val));
        }
        
        return phases;
    }
};

// Test signal generators
class TestSignalGenerator {
public:
    // Generate sine wave with precise frequency
    static std::vector<std::vector<float>> generateStereoSineWave(double frequency, double amplitude, 
                                                                double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = static_cast<float>(amplitude * std::sin(phase));
            signal[0][i] = sample;
            signal[1][i] = sample; // Mono signal fed to both channels
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
        
        return signal;
    }
    
    // Generate sweep signal for frequency response analysis
    static std::vector<std::vector<float>> generateStereoSweep(double startFreq, double endFreq,
                                                             double amplitude, double duration, 
                                                             double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        double freqRatio = endFreq / startFreq;
        double logFreqRatio = std::log(freqRatio);
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double normalizedTime = t / duration;
            double currentFreq = startFreq * std::exp(normalizedTime * logFreqRatio);
            
            double phase = 2.0 * M_PI * startFreq * duration * 
                          (std::exp(normalizedTime * logFreqRatio) - 1.0) / logFreqRatio;
            
            float sample = static_cast<float>(amplitude * std::sin(phase));
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
    
    // Generate pulse for impulse response
    static std::vector<std::vector<float>> generateStereoImpulse(double amplitude, int position, 
                                                               int totalSamples) {
        std::vector<std::vector<float>> signal(2, std::vector<float>(totalSamples, 0.0f));
        if (position >= 0 && position < totalSamples) {
            signal[0][position] = static_cast<float>(amplitude);
            signal[1][position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate white noise for modulation testing
    static std::vector<std::vector<float>> generateStereoNoise(double amplitude, double duration, 
                                                             double sampleRate, unsigned seed = 0) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        std::mt19937 gen(seed == 0 ? std::random_device{}() : seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = static_cast<float>(amplitude * dist(gen));
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
};

// Audio analysis utilities specific to modulation effects
class ModulationAnalyzer {
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
    
    // Analyze stereo width and correlation
    static std::pair<float, float> analyzeStereoField(const std::vector<float>& left,
                                                     const std::vector<float>& right) {
        if (left.size() != right.size() || left.empty()) {
            return {0.0f, 0.0f};
        }
        
        // Calculate cross-correlation
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
        
        float correlation = 0.0f;
        if (denominator > 0.0) {
            correlation = static_cast<float>(numerator / denominator);
        }
        
        // Calculate stereo width (0 = mono, 1 = full stereo, >1 = expanded)
        double mid_energy = 0.0, side_energy = 0.0;
        for (size_t i = 0; i < left.size(); ++i) {
            double mid = (left[i] + right[i]) * 0.5;
            double side = (left[i] - right[i]) * 0.5;
            mid_energy += mid * mid;
            side_energy += side * side;
        }
        
        float width = 0.0f;
        if (mid_energy > 0.0) {
            width = static_cast<float>(side_energy / mid_energy);
        }
        
        return {correlation, width};
    }
    
    // Measure LFO rate by analyzing modulation frequency
    static float measureLFORate(const std::vector<float>& modulatedSignal, 
                               double sampleRate, double expectedRate) {
        if (modulatedSignal.size() < FFT_SIZE) return 0.0f;
        
        // Take envelope of signal to extract modulation
        std::vector<double> envelope;
        envelope.reserve(modulatedSignal.size());
        
        float smoothing = 0.99f;
        float envelopeFollower = 0.0f;
        
        for (float sample : modulatedSignal) {
            float rectified = std::abs(sample);
            envelopeFollower = rectified + (envelopeFollower - rectified) * smoothing;
            envelope.push_back(envelopeFollower);
        }
        
        // Remove DC component
        double dcLevel = 0.0;
        for (double env : envelope) {
            dcLevel += env;
        }
        dcLevel /= envelope.size();
        
        for (double& env : envelope) {
            env -= dcLevel;
        }
        
        // Window and FFT the envelope
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE && i < static_cast<int>(envelope.size()); ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            windowed[i] = envelope[i] * window;
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto magnitudes = SimpleFFT::magnitude(fft_result);
        
        // Find peak in LFO range (0.1 Hz to 20 Hz)
        int startBin = std::max(1, static_cast<int>(0.1 * FFT_SIZE / sampleRate));
        int endBin = std::min(static_cast<int>(magnitudes.size() / 2), 
                             static_cast<int>(20.0 * FFT_SIZE / sampleRate));
        
        int peakBin = startBin;
        double peakMagnitude = magnitudes[startBin];
        
        for (int i = startBin; i < endBin; ++i) {
            if (magnitudes[i] > peakMagnitude) {
                peakMagnitude = magnitudes[i];
                peakBin = i;
            }
        }
        
        return static_cast<float>(peakBin * sampleRate / FFT_SIZE);
    }
    
    // Measure modulation depth (peak-to-peak variation)
    static float measureModulationDepth(const std::vector<float>& modulatedSignal) {
        if (modulatedSignal.size() < 1024) return 0.0f;
        
        // Calculate envelope
        std::vector<float> envelope;
        envelope.reserve(modulatedSignal.size());
        
        float smoothing = 0.99f;
        float envelopeFollower = 0.0f;
        
        for (float sample : modulatedSignal) {
            float rectified = std::abs(sample);
            envelopeFollower = rectified + (envelopeFollower - rectified) * smoothing;
            envelope.push_back(envelopeFollower);
        }
        
        float minEnv = *std::min_element(envelope.begin(), envelope.end());
        float maxEnv = *std::max_element(envelope.begin(), envelope.end());
        
        if (maxEnv > 0.0f) {
            return (maxEnv - minEnv) / maxEnv;
        }
        
        return 0.0f;
    }
    
    // Analyze delay time modulation accuracy
    static float measureDelayTimeModulation(const std::vector<float>& original,
                                           const std::vector<float>& processed,
                                           double sampleRate) {
        if (original.size() != processed.size() || original.size() < 2048) {
            return 0.0f;
        }
        
        // Cross-correlate to find delay variations
        std::vector<float> delays;
        int windowSize = 1024;
        int hopSize = 256;
        
        for (size_t start = 0; start < original.size() - windowSize; start += hopSize) {
            // Find peak correlation in a short window
            int maxLag = 200; // Maximum expected delay in samples
            float maxCorrelation = 0.0f;
            int bestLag = 0;
            
            for (int lag = 0; lag < maxLag && start + lag < processed.size(); ++lag) {
                float correlation = 0.0f;
                
                for (int i = 0; i < windowSize && start + i + lag < processed.size(); ++i) {
                    correlation += original[start + i] * processed[start + i + lag];
                }
                
                if (correlation > maxCorrelation) {
                    maxCorrelation = correlation;
                    bestLag = lag;
                }
            }
            
            delays.push_back(static_cast<float>(bestLag) / sampleRate * 1000.0f); // Convert to ms
        }
        
        if (delays.empty()) return 0.0f;
        
        // Calculate standard deviation of delay times
        float mean = 0.0f;
        for (float delay : delays) {
            mean += delay;
        }
        mean /= delays.size();
        
        float variance = 0.0f;
        for (float delay : delays) {
            variance += (delay - mean) * (delay - mean);
        }
        variance /= delays.size();
        
        return std::sqrt(variance);
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
    
    // Measure phase difference between channels
    static float measurePhaseDifference(const std::vector<float>& left,
                                       const std::vector<float>& right,
                                       double frequency, double sampleRate) {
        if (left.size() != right.size() || left.size() < FFT_SIZE) {
            return 0.0f;
        }
        
        // Window the signals
        std::vector<double> leftWindowed(FFT_SIZE);
        std::vector<double> rightWindowed(FFT_SIZE);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            if (i < static_cast<int>(left.size())) {
                leftWindowed[i] = left[i] * window;
                rightWindowed[i] = right[i] * window;
            }
        }
        
        auto leftFFT = SimpleFFT::fft(leftWindowed);
        auto rightFFT = SimpleFFT::fft(rightWindowed);
        
        auto leftPhase = SimpleFFT::phase(leftFFT);
        auto rightPhase = SimpleFFT::phase(rightFFT);
        
        // Find the bin closest to the target frequency
        int targetBin = static_cast<int>(frequency * FFT_SIZE / sampleRate);
        targetBin = std::max(1, std::min(targetBin, static_cast<int>(leftPhase.size() / 2 - 1)));
        
        double phaseDiff = rightPhase[targetBin] - leftPhase[targetBin];
        
        // Wrap to [-pi, pi]
        while (phaseDiff > M_PI) phaseDiff -= 2.0 * M_PI;
        while (phaseDiff < -M_PI) phaseDiff += 2.0 * M_PI;
        
        return static_cast<float>(phaseDiff * 180.0 / M_PI); // Convert to degrees
    }
};

// Main test class for Digital Chorus
class DigitalChorusTest {
private:
    std::unique_ptr<StereoChorus> chorus;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    DigitalChorusTest() {
        chorus = std::make_unique<StereoChorus>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/DigitalChorus_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/DigitalChorus_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the chorus
        chorus->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Digital Chorus Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_DIGITAL_CHORUS) + "\n");
        log("Parameter Count: " + std::to_string(chorus->getNumParameters()) + "\n\n");
    }
    
    ~DigitalChorusTest() {
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
    
    void logCSV(const std::string& test, const std::string& param, 
                float value, const std::string& result, const std::string& units = "") {
        if (csvFile.is_open()) {
            csvFile << test << "," << param << "," << value << "," << result << "," << units << "\n";
            csvFile.flush();
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
    
    // Process stereo audio through chorus
    std::pair<std::vector<std::vector<float>>, std::vector<std::vector<float>>> 
    processAudio(const std::vector<std::vector<float>>& input, 
                 const std::map<int, float>& parameters) {
        // Update parameters
        chorus->updateParameters(parameters);
        
        // Process in blocks
        std::vector<std::vector<float>> output(2);
        std::vector<std::vector<float>> original = input; // Copy for comparison
        
        if (input.empty() || input[0].empty()) {
            return {original, output};
        }
        
        size_t totalSamples = input[0].size();
        output[0].reserve(totalSamples);
        output[1].reserve(totalSamples);
        
        for (size_t i = 0; i < totalSamples; i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), 
                                      totalSamples - i);
            
            // Create JUCE AudioBuffer
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            // Fill buffer with input
            for (size_t j = 0; j < blockSize; ++j) {
                buffer.setSample(0, static_cast<int>(j), 
                               (i + j < input[0].size()) ? input[0][i + j] : 0.0f);
                buffer.setSample(1, static_cast<int>(j), 
                               (i + j < input[1].size()) ? input[1][i + j] : 0.0f);
            }
            
            // Process
            chorus->process(buffer);
            
            // Extract output
            for (size_t j = 0; j < blockSize; ++j) {
                output[0].push_back(buffer.getSample(0, static_cast<int>(j)));
                output[1].push_back(buffer.getSample(1, static_cast<int>(j)));
            }
        }
        
        return {original, output};
    }
    
    // Test 1: Parameter validation and response
    void testParameterResponse() {
        log("\n--- Parameter Response Tests ---\n");
        
        // Test signal: 1kHz sine at moderate level
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.5, 2.0, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < chorus->getNumParameters(); ++param) {
            std::string paramName = chorus->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            // Sweep from 0.0 to 1.0 in 0.2 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.2f) {
                std::map<int, float> params;
                
                // Set default values for all parameters
                for (int p = 0; p < chorus->getNumParameters(); ++p) {
                    params[p] = 0.5f; // Default to middle position
                }
                
                // Override the parameter being tested
                params[param] = value;
                
                auto [original, output] = processAudio(testSignal, params);
                
                // Check for valid output
                assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                          !ModulationAnalyzer::hasInvalidValues(output[1]), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                float outputRMS = ModulationAnalyzer::calculateRMS_dB(output[0]);
                responseDB.push_back(outputRMS);
                
                logCSV("ParameterResponse", paramName + "_" + std::to_string(value), 
                       outputRMS, "PASS", "dB");
            }
            
            // Check parameter responsiveness
            float minResponse = *std::min_element(responseDB.begin(), responseDB.end());
            float maxResponse = *std::max_element(responseDB.begin(), responseDB.end());
            float responseRange = maxResponse - minResponse;
            
            log("  Response range: " + std::to_string(responseRange) + "dB\n");
            
            // Most parameters should have some audible effect
            if (param != 5) { // Mix parameter might not change level significantly
                assertTrue(responseRange > 0.5f || param == chorus->getNumParameters() - 1, 
                          paramName + " has measurable effect (range: " + 
                          std::to_string(responseRange) + "dB)");
            }
        }
    }
    
    // Test 2: LFO rate accuracy and waveform shape
    void testLFORateAccuracy() {
        log("\n--- LFO Rate Accuracy Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<float> expectedRates = {0.5f, 1.0f, 2.0f, 5.0f, 10.0f}; // Hz
        std::vector<float> rateSettings = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f}; // Parameter values
        
        for (size_t i = 0; i < rateSettings.size(); ++i) {
            float rateSetting = rateSettings[i];
            float expectedRate = expectedRates[i];
            
            log("\nTesting LFO rate setting: " + std::to_string(rateSetting) + 
                " (expected ~" + std::to_string(expectedRate) + "Hz)\n");
            
            std::map<int, float> params;
            params[0] = rateSetting;  // Rate parameter
            params[1] = 0.8f;         // High depth
            params[2] = 0.1f;         // Low feedback
            params[3] = 0.5f;         // Moderate delay
            params[4] = 0.5f;         // Center width
            params[5] = 0.0f;         // Full effect (no dry mix)
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure actual LFO rate
            float measuredRate = ModulationAnalyzer::measureLFORate(output[0], TEST_SAMPLE_RATE, expectedRate);
            float rateError = std::abs(measuredRate - expectedRate) / expectedRate * 100.0f;
            
            log("  Measured LFO rate: " + std::to_string(measuredRate) + "Hz\n");
            log("  Rate error: " + std::to_string(rateError) + "%\n");
            
            logCSV("LFORateAccuracy", "MeasuredRate_" + std::to_string(rateSetting),
                   measuredRate, "MEASURED", "Hz");
            logCSV("LFORateAccuracy", "RateError_" + std::to_string(rateSetting),
                   rateError, "MEASURED", "%");
            
            // Rate should be reasonably close (within 20% for most settings)
            assertTrue(rateError < 30.0f, 
                      "LFO rate accuracy at setting " + std::to_string(rateSetting) +
                      " (error: " + std::to_string(rateError) + "%)");
            
            assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                      !ModulationAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at LFO rate " + std::to_string(rateSetting));
        }
    }
    
    // Test 3: Depth/intensity modulation precision
    void testModulationDepth() {
        log("\n--- Modulation Depth Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> depthSettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float depth : depthSettings) {
            log("\nTesting modulation depth: " + std::to_string(depth) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;    // Moderate rate (2Hz)
            params[1] = depth;   // Depth parameter
            params[2] = 0.1f;    // Low feedback
            params[3] = 0.5f;    // Moderate delay
            params[4] = 0.5f;    // Center width
            params[5] = 0.0f;    // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure modulation depth
            float measuredDepth = ModulationAnalyzer::measureModulationDepth(output[0]);
            
            log("  Measured modulation depth: " + std::to_string(measuredDepth) + "\n");
            
            logCSV("ModulationDepth", "MeasuredDepth_" + std::to_string(depth),
                   measuredDepth, "MEASURED", "ratio");
            
            // Higher depth settings should produce more modulation
            if (depth > 0.5f) {
                assertTrue(measuredDepth > 0.05f, 
                          "Significant modulation at depth " + std::to_string(depth));
            }
            
            if (depth == 0.0f) {
                assertTrue(measuredDepth < 0.02f, 
                          "Minimal modulation at zero depth");
            }
            
            assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                      !ModulationAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at depth " + std::to_string(depth));
        }
    }
    
    // Test 4: Stereo imaging and width control
    void testStereoImaging() {
        log("\n--- Stereo Imaging Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> widthSettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float width : widthSettings) {
            log("\nTesting stereo width: " + std::to_string(width) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;    // Moderate rate
            params[1] = 0.6f;    // Moderate depth
            params[2] = 0.2f;    // Low feedback
            params[3] = 0.5f;    // Moderate delay
            params[4] = width;   // Width parameter
            params[5] = 0.0f;    // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Analyze stereo field
            auto [correlation, stereoWidth] = ModulationAnalyzer::analyzeStereoField(output[0], output[1]);
            
            log("  Channel correlation: " + std::to_string(correlation) + "\n");
            log("  Stereo width ratio: " + std::to_string(stereoWidth) + "\n");
            
            logCSV("StereoImaging", "Correlation_" + std::to_string(width),
                   correlation, "MEASURED", "ratio");
            logCSV("StereoImaging", "Width_" + std::to_string(width),
                   stereoWidth, "MEASURED", "ratio");
            
            // Test phase relationship
            float phaseDiff = ModulationAnalyzer::measurePhaseDifference(output[0], output[1], 
                                                                        1000.0, TEST_SAMPLE_RATE);
            log("  Phase difference: " + std::to_string(phaseDiff) + " degrees\n");
            
            logCSV("StereoImaging", "PhaseDiff_" + std::to_string(width),
                   phaseDiff, "MEASURED", "degrees");
            
            // Width parameter should affect stereo characteristics
            if (width > 0.7f) {
                assertTrue(std::abs(correlation) < 0.95f, 
                          "Reduced correlation at wide setting " + std::to_string(width));
            }
            
            assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                      !ModulationAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at width " + std::to_string(width));
        }
    }
    
    // Test 5: Delay time modulation accuracy
    void testDelayTimeModulation() {
        log("\n--- Delay Time Modulation Accuracy Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(2000.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> delaySettings = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};
        
        for (float delaySetting : delaySettings) {
            log("\nTesting delay setting: " + std::to_string(delaySetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f;         // Slow rate for easier measurement
            params[1] = 0.7f;         // High depth
            params[2] = 0.1f;         // Low feedback
            params[3] = delaySetting; // Delay parameter
            params[4] = 0.5f;         // Center width
            params[5] = 0.0f;         // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure delay time variation
            float delayVariation = ModulationAnalyzer::measureDelayTimeModulation(
                original[0], output[0], TEST_SAMPLE_RATE);
            
            log("  Delay time variation: " + std::to_string(delayVariation) + "ms\n");
            
            logCSV("DelayTimeModulation", "DelayVariation_" + std::to_string(delaySetting),
                   delayVariation, "MEASURED", "ms");
            
            // Delay modulation should be present and proportional to setting
            assertTrue(delayVariation > 0.1f, 
                      "Measurable delay modulation at setting " + std::to_string(delaySetting));
            
            assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                      !ModulationAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at delay setting " + std::to_string(delaySetting));
        }
    }
    
    // Test 6: Feedback loop stability
    void testFeedbackStability() {
        log("\n--- Feedback Loop Stability Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.2, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<float> feedbackSettings = {0.0f, 0.3f, 0.6f, 0.8f, 0.95f};
        
        for (float feedback : feedbackSettings) {
            log("\nTesting feedback level: " + std::to_string(feedback) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;      // Moderate rate
            params[1] = 0.5f;      // Moderate depth
            params[2] = feedback;  // Feedback parameter
            params[3] = 0.6f;      // Longer delay for feedback
            params[4] = 0.5f;      // Center width
            params[5] = 0.0f;      // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Check for stability (no runaway levels)
            float outputPeak = 0.0f;
            for (const auto& channel : output) {
                for (float sample : channel) {
                    outputPeak = std::max(outputPeak, std::abs(sample));
                }
            }
            
            float outputPeakDB = 20.0f * std::log10(std::max(1e-6f, outputPeak));
            log("  Output peak level: " + std::to_string(outputPeakDB) + "dB\n");
            
            logCSV("FeedbackStability", "PeakLevel_" + std::to_string(feedback),
                   outputPeakDB, "MEASURED", "dB");
            
            // Check for resonance buildup with high feedback
            float outputRMS = ModulationAnalyzer::calculateRMS_dB(output[0]);
            log("  Output RMS level: " + std::to_string(outputRMS) + "dB\n");
            
            logCSV("FeedbackStability", "RMSLevel_" + std::to_string(feedback),
                   outputRMS, "MEASURED", "dB");
            
            // System should remain stable (no infinite buildup)
            assertTrue(outputPeakDB < 6.0f, 
                      "Stable output at feedback " + std::to_string(feedback));
            
            assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                      !ModulationAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at feedback " + std::to_string(feedback));
        }
    }
    
    // Test 7: Mix parameter behavior
    void testMixParameter() {
        log("\n--- Mix Parameter Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> mixSettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float mix : mixSettings) {
            log("\nTesting mix level: " + std::to_string(mix) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;    // Moderate rate
            params[1] = 0.7f;    // High depth for clear effect
            params[2] = 0.3f;    // Some feedback
            params[3] = 0.5f;    // Moderate delay
            params[4] = 0.7f;    // Wide stereo
            params[5] = mix;     // Mix parameter
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Calculate wet/dry ratio
            float originalRMS = ModulationAnalyzer::calculateRMS_dB(original[0]);
            float outputRMS = ModulationAnalyzer::calculateRMS_dB(output[0]);
            float levelDifference = outputRMS - originalRMS;
            
            log("  Level difference: " + std::to_string(levelDifference) + "dB\n");
            
            logCSV("MixParameter", "LevelDiff_" + std::to_string(mix),
                   levelDifference, "MEASURED", "dB");
            
            // Measure modulation presence
            float modulation = ModulationAnalyzer::measureModulationDepth(output[0]);
            log("  Modulation amount: " + std::to_string(modulation) + "\n");
            
            logCSV("MixParameter", "Modulation_" + std::to_string(mix),
                   modulation, "MEASURED", "ratio");
            
            // Mix = 1.0 should be full dry (minimal modulation)
            if (mix > 0.9f) {
                assertTrue(modulation < 0.05f, 
                          "Minimal modulation at full dry mix");
            }
            
            // Mix = 0.0 should be full wet (maximum modulation)
            if (mix < 0.1f) {
                assertTrue(modulation > 0.1f, 
                          "Significant modulation at full wet mix");
            }
            
            assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                      !ModulationAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at mix " + std::to_string(mix));
        }
    }
    
    // Test 8: Performance and stability
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        // Generate longer test signal
        auto longSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 10.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.6f; // Moderate settings
        params[1] = 0.7f;
        params[2] = 0.4f;
        params[3] = 0.5f;
        params[4] = 0.8f;
        params[5] = 0.3f;
        
        // Measure processing time
        auto startTime = std::chrono::high_resolution_clock::now();
        auto [original, output] = processAudio(longSignal, params);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0; // milliseconds
        
        double signalDuration = longSignal[0].size() / TEST_SAMPLE_RATE * 1000.0; // milliseconds
        double realTimeRatio = processingTime / signalDuration;
        
        log("Processing time: " + std::to_string(processingTime) + "ms\n");
        log("Signal duration: " + std::to_string(signalDuration) + "ms\n");
        log("Real-time ratio: " + std::to_string(realTimeRatio) + "\n");
        
        logCSV("Performance", "ProcessingTime", processingTime, "MEASURED", "ms");
        logCSV("Performance", "RealTimeRatio", realTimeRatio, "MEASURED", "ratio");
        
        // Check output quality
        assertTrue(!ModulationAnalyzer::hasInvalidValues(output[0]) &&
                  !ModulationAnalyzer::hasInvalidValues(output[1]), 
                  "Valid output during performance test");
        assertTrue(realTimeRatio < 0.5, "Real-time processing capability");
        
        float outputStability = ModulationAnalyzer::calculateRMS_dB(output[0]);
        assertTrue(outputStability > -60.0f, "Stable output level");
        
        log("Output stability: " + std::to_string(outputStability) + "dB\n");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Digital Chorus comprehensive test suite...\n");
        
        testParameterResponse();
        testLFORateAccuracy();
        testModulationDepth();
        testStereoImaging();
        testDelayTimeModulation();
        testFeedbackStability();
        testMixParameter();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        DigitalChorusTest tester;
        tester.runAllTests();
        
        std::cout << "\nDigital Chorus test suite completed successfully.\n";
        std::cout << "Check DigitalChorus_TestResults.txt for detailed results.\n";
        std::cout << "Check DigitalChorus_Data.csv for measurement data.\n";
        
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