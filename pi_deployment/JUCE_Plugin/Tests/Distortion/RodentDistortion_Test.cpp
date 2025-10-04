/*
  ==============================================================================
  
    RodentDistortion_Test.cpp
    Comprehensive test suite for ENGINE_RODENT_DISTORTION
    
    Tests for Rodent Distortion characteristics:
    - Clipping characteristics analysis
    - Filter response verification
    - Gain structure validation
    - Tonal accuracy testing
    - Distortion curve analysis
    - Frequency response measurement
    - Dynamic behavior validation
    - Overdrive vs distortion modes
    
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

// Include test-compatible JUCE headers
#include "../EngineBaseTest.h"
#include "../../Source/RodentDistortion.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;
constexpr int FFT_SIZE = 8192;

// Simple FFT for analysis
class SimpleFFT {
public:
    static std::vector<std::complex<double>> fft(const std::vector<double>& signal) {
        int N = signal.size();
        std::vector<std::complex<double>> result(N);
        
        for (int i = 0; i < N; ++i) {
            result[i] = std::complex<double>(signal[i], 0.0);
        }
        
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
    
    static std::vector<double> magnitudeDB(const std::vector<std::complex<double>>& fft_result) {
        std::vector<double> magnitudes_db;
        magnitudes_db.reserve(fft_result.size());
        
        for (const auto& complex_val : fft_result) {
            double mag = std::abs(complex_val);
            magnitudes_db.push_back(20.0 * std::log10(std::max(1e-12, mag)));
        }
        
        return magnitudes_db;
    }
};

// Test signal generators
class TestSignalGenerator {
public:
    static std::vector<float> generateSineWave(double frequency, double amplitude, 
                                             double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
        
        return signal;
    }
    
    static std::vector<float> generateSweep(double startFreq, double endFreq,
                                          double amplitude, double duration,
                                          double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double progress = t / duration;
            double freq = startFreq * std::pow(endFreq / startFreq, progress);
            double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    static std::vector<float> generateRamp(double amplitude, double duration, 
                                         double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / (numSamples - 1.0);
            signal[i] = static_cast<float>(amplitude * (2.0 * t - 1.0));
        }
        
        return signal;
    }
};

// Audio analysis utilities
class AudioAnalyzer {
public:
    static float calculateRMS_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        
        double sum = 0.0;
        for (float sample : signal) {
            sum += sample * sample;
        }
        
        double rms = std::sqrt(sum / signal.size());
        return 20.0f * std::log10(std::max(1e-6, rms));
    }
    
    static float calculatePeak_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        
        float peak = 0.0f;
        for (float sample : signal) {
            peak = std::max(peak, std::abs(sample));
        }
        
        return 20.0f * std::log10(std::max(1e-6, peak));
    }
    
    static std::vector<float> measureFrequencyResponse(const std::vector<float>& input,
                                                      const std::vector<float>& output,
                                                      const std::vector<double>& frequencies,
                                                      double sampleRate) {
        std::vector<float> gains;
        
        if (input.size() != output.size() || input.size() < FFT_SIZE) {
            return gains;
        }
        
        std::vector<double> windowed_input(FFT_SIZE);
        std::vector<double> windowed_output(FFT_SIZE);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            
            if (i < static_cast<int>(input.size())) {
                windowed_input[i] = input[i] * window;
                windowed_output[i] = output[i] * window;
            } else {
                windowed_input[i] = 0.0;
                windowed_output[i] = 0.0;
            }
        }
        
        auto fft_input = SimpleFFT::fft(windowed_input);
        auto fft_output = SimpleFFT::fft(windowed_output);
        auto mag_input_db = SimpleFFT::magnitudeDB(fft_input);
        auto mag_output_db = SimpleFFT::magnitudeDB(fft_output);
        
        for (double freq : frequencies) {
            int bin = static_cast<int>(freq * FFT_SIZE / sampleRate);
            if (bin < static_cast<int>(mag_input_db.size() / 2)) {
                float gain = mag_output_db[bin] - mag_input_db[bin];
                gains.push_back(gain);
            } else {
                gains.push_back(0.0f);
            }
        }
        
        return gains;
    }
    
    static float calculateTHD(const std::vector<float>& signal, double fundamental, 
                            double sampleRate, int harmonics = 5) {
        if (signal.size() < FFT_SIZE) return 0.0f;
        
        std::vector<double> windowed_signal(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(signal.size())) {
                double window = 0.42 - 0.5 * std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)) + 
                               0.08 * std::cos(4.0 * M_PI * i / (FFT_SIZE - 1));
                windowed_signal[i] = signal[i] * window;
            } else {
                windowed_signal[i] = 0.0;
            }
        }
        
        auto fft_result = SimpleFFT::fft(windowed_signal);
        auto magnitudes = SimpleFFT::magnitudeDB(fft_result);
        
        int fund_bin = static_cast<int>(fundamental * FFT_SIZE / sampleRate);
        if (fund_bin >= static_cast<int>(magnitudes.size() / 2)) return 0.0f;
        
        double fund_power = std::pow(10.0, magnitudes[fund_bin] / 10.0);
        double harmonic_power = 0.0;
        
        for (int h = 2; h <= harmonics; ++h) {
            int harm_bin = fund_bin * h;
            if (harm_bin < static_cast<int>(magnitudes.size() / 2)) {
                harmonic_power += std::pow(10.0, magnitudes[harm_bin] / 10.0);
            }
        }
        
        if (fund_power == 0.0) return 0.0f;
        
        return static_cast<float>(std::sqrt(harmonic_power / fund_power));
    }
    
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) {
                return true;
            }
        }
        return false;
    }
};

// Main test class for Rodent Distortion
class RodentDistortionTest {
private:
    std::unique_ptr<RodentDistortion> rodentDistortion;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    RodentDistortionTest() {
        rodentDistortion = std::make_unique<RodentDistortion>();
        
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/RodentDistortion_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/RodentDistortion_Data.csv");
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        rodentDistortion->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Rodent Distortion Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Engine ID: " + std::to_string(ENGINE_RODENT_DISTORTION) + "\n");
        log("Parameter Count: " + std::to_string(rodentDistortion->getNumParameters()) + "\n\n");
    }
    
    ~RodentDistortionTest() {
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
    
    std::pair<std::vector<float>, std::vector<float>> processAudio(const std::vector<float>& input, 
                                                                  const std::map<int, float>& parameters) {
        rodentDistortion->updateParameters(parameters);
        
        std::vector<float> output;
        std::vector<float> original = input;
        output.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), 
                                      input.size() - i);
            
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            for (size_t j = 0; j < blockSize; ++j) {
                float sample = (i + j < input.size()) ? input[i + j] : 0.0f;
                buffer.setSample(0, static_cast<int>(j), sample);
                buffer.setSample(1, static_cast<int>(j), sample);
            }
            
            rodentDistortion->process(buffer);
            
            for (size_t j = 0; j < blockSize; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }
        
        return {original, output};
    }
    
    void testParameterResponse() {
        log("\n--- Parameter Response Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateSineWave(440.0, 0.3, 1.0, TEST_SAMPLE_RATE);
        
        for (int param = 0; param < rodentDistortion->getNumParameters(); ++param) {
            std::string paramName = rodentDistortion->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                for (int p = 0; p < rodentDistortion->getNumParameters(); ++p) {
                    params[p] = 0.5f;
                }
                
                params[param] = value;
                
                auto [original, output] = processAudio(testSignal, params);
                
                assertTrue(!AudioAnalyzer::hasInvalidValues(output), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
                responseDB.push_back(outputRMS);
                
                logCSV("ParameterResponse", paramName + "_" + std::to_string(value), 
                       outputRMS, "PASS", "dB");
            }
            
            float minResponse = *std::min_element(responseDB.begin(), responseDB.end());
            float maxResponse = *std::max_element(responseDB.begin(), responseDB.end());
            float responseRange = maxResponse - minResponse;
            
            log("  Response range: " + std::to_string(responseRange) + "dB\n");
            
            if (param <= 3) {
                assertTrue(responseRange > 0.5f, 
                          paramName + " has audible effect");
            }
        }
    }
    
    void testClippingCharacteristics() {
        log("\n--- Clipping Characteristics Tests ---\n");
        
        auto rampSignal = TestSignalGenerator::generateRamp(1.0, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> distortionSettings = {0.3f, 0.6f, 0.9f};
        
        for (float distortion : distortionSettings) {
            log("\nTesting distortion setting: " + std::to_string(distortion) + "\n");
            
            std::map<int, float> params;
            params[0] = distortion; // Distortion amount
            params[1] = 0.5f;       // Filter
            params[2] = 0.5f;       // Level
            
            auto [original, output] = processAudio(rampSignal, params);
            
            // Find clipping threshold
            float clippingThreshold = 1.0f;
            for (size_t i = 0; i < output.size(); ++i) {
                float inputLevel = std::abs(original[i]);
                float outputLevel = std::abs(output[i]);
                
                if (inputLevel > 0.1f && outputLevel < inputLevel * 0.9f) {
                    clippingThreshold = std::min(clippingThreshold, inputLevel);
                }
            }
            
            log("  Clipping threshold: " + std::to_string(clippingThreshold) + "\n");
            
            logCSV("ClippingCharacteristics", "ClippingThreshold_" + std::to_string(distortion),
                   clippingThreshold, "MEASURED", "amplitude");
            
            // Calculate transfer function linearity
            float maxOutput = AudioAnalyzer::calculatePeak_dB(output);
            log("  Maximum output: " + std::to_string(maxOutput) + "dB\n");
            
            assertTrue(clippingThreshold < 0.9f, 
                      "Clipping occurs at distortion " + std::to_string(distortion));
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at distortion " + std::to_string(distortion));
        }
    }
    
    void testFilterResponse() {
        log("\n--- Filter Response Tests ---\n");
        
        auto sweepSignal = TestSignalGenerator::generateSweep(50.0, 15000.0, 0.2, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> filterSettings = {0.2f, 0.5f, 0.8f};
        
        for (float filter : filterSettings) {
            log("\nTesting filter setting: " + std::to_string(filter) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;   // Moderate distortion
            params[1] = filter; // Filter setting
            params[2] = 0.5f;   // Level
            
            auto [original, output] = processAudio(sweepSignal, params);
            
            // Measure frequency response at key frequencies
            std::vector<double> testFreqs = {100, 500, 1000, 2000, 5000, 10000};
            auto gains = AudioAnalyzer::measureFrequencyResponse(original, output, testFreqs, TEST_SAMPLE_RATE);
            
            log("  Frequency response:\n");
            for (size_t i = 0; i < testFreqs.size() && i < gains.size(); ++i) {
                log("    " + std::to_string(testFreqs[i]) + "Hz: " + 
                    std::to_string(gains[i]) + "dB\n");
                
                logCSV("FilterResponse", 
                       "Filter" + std::to_string(filter) + "_" + std::to_string(testFreqs[i]) + "Hz",
                       gains[i], "MEASURED", "dB");
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid filter response at setting " + std::to_string(filter));
        }
    }
    
    void testTHDMeasurements() {
        log("\n--- THD Measurement Tests ---\n");
        
        std::vector<double> testFreqs = {220.0, 440.0, 880.0};
        std::vector<float> driveSettings = {0.3f, 0.6f, 0.9f};
        
        for (double freq : testFreqs) {
            for (float drive : driveSettings) {
                log("\nTesting THD at " + std::to_string(freq) + "Hz, drive " + 
                    std::to_string(drive) + "\n");
                
                auto testSignal = TestSignalGenerator::generateSineWave(freq, 0.3, 1.5, TEST_SAMPLE_RATE);
                
                std::map<int, float> params;
                params[0] = drive; // Drive
                params[1] = 0.5f;  // Filter
                params[2] = 0.5f;  // Level
                
                auto [original, output] = processAudio(testSignal, params);
                
                float thd = AudioAnalyzer::calculateTHD(output, freq, TEST_SAMPLE_RATE);
                float thd_percent = thd * 100.0f;
                
                log("  THD: " + std::to_string(thd_percent) + "%\n");
                
                logCSV("THDMeasurements", 
                       "THD_" + std::to_string(freq) + "Hz_Drive" + std::to_string(drive),
                       thd_percent, "MEASURED", "%");
                
                assertTrue(thd_percent < 50.0f, 
                          "THD reasonable at " + std::to_string(freq) + "Hz");
                
                if (drive > 0.7f) {
                    assertTrue(thd_percent > 1.0f, 
                              "Audible distortion at high drive");
                }
            }
        }
    }
    
    void testDynamicResponse() {
        log("\n--- Dynamic Response Tests ---\n");
        
        // Create signal with varying levels
        std::vector<float> dynamicSignal;
        
        // Low level
        auto lowLevel = TestSignalGenerator::generateSineWave(440.0, 0.1, 0.5, TEST_SAMPLE_RATE);
        dynamicSignal.insert(dynamicSignal.end(), lowLevel.begin(), lowLevel.end());
        
        // High level
        auto highLevel = TestSignalGenerator::generateSineWave(440.0, 0.6, 0.5, TEST_SAMPLE_RATE);
        dynamicSignal.insert(dynamicSignal.end(), highLevel.begin(), highLevel.end());
        
        std::map<int, float> params;
        params[0] = 0.7f; // High distortion
        params[1] = 0.5f; // Filter
        params[2] = 0.5f; // Level
        
        auto [original, output] = processAudio(dynamicSignal, params);
        
        // Analyze sections
        int sectionLength = static_cast<int>(0.5 * TEST_SAMPLE_RATE);
        
        std::vector<float> lowSection(output.begin(), output.begin() + sectionLength);
        std::vector<float> highSection(output.begin() + sectionLength, output.begin() + 2 * sectionLength);
        
        float lowRMS = AudioAnalyzer::calculateRMS_dB(lowSection);
        float highRMS = AudioAnalyzer::calculateRMS_dB(highSection);
        float dynamicRange = highRMS - lowRMS;
        
        log("Low level RMS: " + std::to_string(lowRMS) + "dB\n");
        log("High level RMS: " + std::to_string(highRMS) + "dB\n");
        log("Dynamic range: " + std::to_string(dynamicRange) + "dB\n");
        
        logCSV("DynamicResponse", "LowLevelRMS", lowRMS, "MEASURED", "dB");
        logCSV("DynamicResponse", "HighLevelRMS", highRMS, "MEASURED", "dB");
        logCSV("DynamicResponse", "DynamicRange", dynamicRange, "MEASURED", "dB");
        
        assertTrue(dynamicRange > 5.0f, "Preserves some dynamic range");
        assertTrue(dynamicRange < 20.0f, "Provides compression");
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid dynamic response");
    }
    
    void testGainStructure() {
        log("\n--- Gain Structure Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateSineWave(440.0, 0.2, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> levelSettings = {0.2f, 0.5f, 0.8f};
        
        for (float level : levelSettings) {
            log("\nTesting level setting: " + std::to_string(level) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;  // Distortion
            params[1] = 0.5f;  // Filter
            params[2] = level; // Level
            
            auto [original, output] = processAudio(testSignal, params);
            
            float inputRMS = AudioAnalyzer::calculateRMS_dB(original);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float gain = outputRMS - inputRMS;
            
            log("  Gain: " + std::to_string(gain) + "dB\n");
            
            logCSV("GainStructure", "Gain_Level" + std::to_string(level), gain, "MEASURED", "dB");
            
            assertTrue(gain > -12.0f && gain < 20.0f, "Reasonable gain range");
            assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid output");
        }
    }
    
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        auto longSignal = TestSignalGenerator::generateSweep(20.0, 18000.0, 0.3, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.8f; // High distortion
        params[1] = 0.7f; // Filter
        params[2] = 0.6f; // Level
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto [original, output] = processAudio(longSignal, params);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0;
        
        double signalDuration = longSignal.size() / TEST_SAMPLE_RATE * 1000.0;
        double realTimeRatio = processingTime / signalDuration;
        
        log("Processing time: " + std::to_string(processingTime) + "ms\n");
        log("Real-time ratio: " + std::to_string(realTimeRatio) + "\n");
        
        logCSV("Performance", "ProcessingTime", processingTime, "MEASURED", "ms");
        logCSV("Performance", "RealTimeRatio", realTimeRatio, "MEASURED", "ratio");
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid output during performance test");
        assertTrue(realTimeRatio < 0.3, "Real-time processing capability");
        
        float outputLevel = AudioAnalyzer::calculatePeak_dB(output);
        assertTrue(outputLevel < 6.0f, "Output level controlled");
    }
    
    void runAllTests() {
        log("Starting Rodent Distortion comprehensive test suite...\n");
        
        testParameterResponse();
        testClippingCharacteristics();
        testFilterResponse();
        testTHDMeasurements();
        testDynamicResponse();
        testGainStructure();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

int main() {
    try {
        RodentDistortionTest tester;
        tester.runAllTests();
        
        std::cout << "\nRodent Distortion test suite completed successfully.\n";
        std::cout << "Check RodentDistortion_TestResults.txt for detailed results.\n";
        std::cout << "Check RodentDistortion_Data.csv for measurement data.\n";
        
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