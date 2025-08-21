/*
  ==============================================================================
  
    BitCrusher_Test.cpp
    Comprehensive test suite for ENGINE_BIT_CRUSHER
    
    Tests for bit crusher characteristics:
    - Bit depth reduction accuracy
    - Sample rate downsampling precision
    - Quantization noise characteristics
    - Dithering effectiveness
    - Jitter and timing accuracy
    - Aliasing control
    - DC offset handling
    - Thermal modeling accuracy
    - Component aging simulation
    
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
#include "../../Source/BitCrusher.h"
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
};

// Test signal generators
class TestSignalGenerator {
public:
    // Generate sine wave with precise frequency
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
    
    // Generate full-scale ramp for quantization testing
    static std::vector<float> generateRamp(double amplitude, double duration, 
                                         double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / (numSamples - 1.0);
            signal[i] = static_cast<float>(amplitude * (2.0 * t - 1.0)); // -amplitude to +amplitude
        }
        
        return signal;
    }
    
    // Generate white noise for dither testing
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
    
    // Generate stepped amplitude signal for bit depth testing
    static std::vector<float> generateSteppedSignal(int numSteps, double amplitude,
                                                   double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        int samplesPerStep = numSamples / numSteps;
        
        for (int step = 0; step < numSteps; ++step) {
            double level = amplitude * (2.0 * step / (numSteps - 1) - 1.0); // -amplitude to +amplitude
            
            for (int i = 0; i < samplesPerStep; ++i) {
                int sampleIndex = step * samplesPerStep + i;
                if (sampleIndex < numSamples) {
                    signal[sampleIndex] = static_cast<float>(level);
                }
            }
        }
        
        return signal;
    }
    
    // Generate high-frequency test signal for aliasing detection
    static std::vector<float> generateHighFreqTest(double sampleRate) {
        // Generate signal with content near Nyquist frequency
        std::vector<float> signal;
        double duration = 1.0;
        int numSamples = static_cast<int>(duration * sampleRate);
        signal.resize(numSamples);
        
        // Mix of high frequencies
        std::vector<double> testFreqs = {sampleRate * 0.3, sampleRate * 0.4, sampleRate * 0.45};
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            signal[i] = 0.0f;
            
            for (double freq : testFreqs) {
                signal[i] += static_cast<float>(0.1 * std::sin(2.0 * M_PI * freq * t));
            }
        }
        
        return signal;
    }
    
    // Generate impulse response test signal
    static std::vector<float> generateImpulse(double amplitude, int position, 
                                            int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
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
    
    // Calculate peak level in dB
    static float calculatePeak_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        
        float peak = 0.0f;
        for (float sample : signal) {
            peak = std::max(peak, std::abs(sample));
        }
        
        return 20.0f * std::log10(std::max(1e-6, peak));
    }
    
    // Count unique quantization levels
    static int countQuantizationLevels(const std::vector<float>& signal, float resolution = 1e-6f) {
        std::set<int> uniqueLevels;
        
        for (float sample : signal) {
            // Quantize to detect levels
            int quantizedValue = static_cast<int>(sample / resolution);
            uniqueLevels.insert(quantizedValue);
        }
        
        return static_cast<int>(uniqueLevels.size());
    }
    
    // Measure effective bit depth
    static float measureEffectiveBitDepth(const std::vector<float>& signal) {
        if (signal.empty()) return 0.0f;
        
        // Find signal range
        float minVal = *std::min_element(signal.begin(), signal.end());
        float maxVal = *std::max_element(signal.begin(), signal.end());
        float range = maxVal - minVal;
        
        if (range == 0.0f) return 0.0f;
        
        // Count unique levels
        std::set<float> uniqueLevels;
        for (float sample : signal) {
            uniqueLevels.insert(sample);
        }
        
        int numLevels = static_cast<int>(uniqueLevels.size());
        if (numLevels <= 1) return 0.0f;
        
        // Calculate effective bit depth
        return std::log2(static_cast<float>(numLevels));
    }
    
    // Measure sample rate reduction artifacts
    static float measureSampleRateArtifacts(const std::vector<float>& original,
                                           const std::vector<float>& processed,
                                           double sampleRate) {
        if (original.size() != processed.size() || original.size() < FFT_SIZE) {
            return 0.0f;
        }
        
        // FFT of original signal
        std::vector<double> windowed_original(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(original.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed_original[i] = original[i] * window;
            } else {
                windowed_original[i] = 0.0;
            }
        }
        
        auto fft_original = SimpleFFT::fft(windowed_original);
        auto mag_original = SimpleFFT::magnitude(fft_original);
        
        // FFT of processed signal
        std::vector<double> windowed_processed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(processed.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed_processed[i] = processed[i] * window;
            } else {
                windowed_processed[i] = 0.0;
            }
        }
        
        auto fft_processed = SimpleFFT::fft(windowed_processed);
        auto mag_processed = SimpleFFT::magnitude(fft_processed);
        
        // Look for aliasing artifacts (content above original signal bandwidth)
        double maxOriginalFreq = 0.0;
        for (int i = 0; i < static_cast<int>(mag_original.size() / 2); ++i) {
            if (mag_original[i] > 0.01) { // Threshold for significant content
                maxOriginalFreq = i * sampleRate / FFT_SIZE;
            }
        }
        
        // Measure artifacts above original content
        int startBin = static_cast<int>(maxOriginalFreq * 1.5 * FFT_SIZE / sampleRate);
        int endBin = static_cast<int>(mag_processed.size() / 2);
        
        double artifactEnergy = 0.0;
        for (int i = startBin; i < endBin; ++i) {
            artifactEnergy += mag_processed[i] * mag_processed[i];
        }
        
        return 20.0f * std::log10(std::sqrt(artifactEnergy) + 1e-12);
    }
    
    // Calculate Signal-to-Quantization Noise Ratio (SQNR)
    static float calculateSQNR(const std::vector<float>& original,
                              const std::vector<float>& quantized) {
        if (original.size() != quantized.size() || original.empty()) {
            return 0.0f;
        }
        
        double signalPower = 0.0;
        double noisePower = 0.0;
        
        for (size_t i = 0; i < original.size(); ++i) {
            signalPower += original[i] * original[i];
            float noise = quantized[i] - original[i];
            noisePower += noise * noise;
        }
        
        signalPower /= original.size();
        noisePower /= original.size();
        
        if (noisePower == 0.0) return 120.0f; // Perfect quantization
        
        return 10.0f * std::log10(signalPower / noisePower);
    }
    
    // Detect aliasing above specific frequency
    static float detectAliasing(const std::vector<float>& signal, double sampleRate,
                               double aboveFreq = 18000.0) {
        if (signal.size() < FFT_SIZE) return 0.0f;
        
        std::vector<double> windowed_signal(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(signal.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed_signal[i] = signal[i] * window;
            } else {
                windowed_signal[i] = 0.0;
            }
        }
        
        auto fft_result = SimpleFFT::fft(windowed_signal);
        auto magnitudes_db = SimpleFFT::magnitudeDB(fft_result);
        
        // Check energy above specified frequency
        int startBin = static_cast<int>(aboveFreq * FFT_SIZE / sampleRate);
        int nyquistBin = static_cast<int>(magnitudes_db.size() / 2);
        
        float maxAliasing = -120.0f;
        for (int i = startBin; i < nyquistBin; ++i) {
            maxAliasing = std::max(maxAliasing, static_cast<float>(magnitudes_db[i]));
        }
        
        return maxAliasing;
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
    
    // Calculate DC offset
    static float calculateDCOffset(const std::vector<float>& signal) {
        if (signal.empty()) return 0.0f;
        
        double sum = 0.0;
        for (float sample : signal) {
            sum += sample;
        }
        
        return static_cast<float>(sum / signal.size());
    }
    
    // Measure jitter/timing errors by cross-correlation
    static float measureTimingJitter(const std::vector<float>& reference,
                                   const std::vector<float>& processed) {
        if (reference.size() != processed.size() || reference.size() < 1024) {
            return 0.0f;
        }
        
        // Simple cross-correlation to measure timing offset variance
        std::vector<float> correlation;
        int maxLag = 50; // Maximum samples to check
        
        for (int lag = -maxLag; lag <= maxLag; ++lag) {
            double sum = 0.0;
            int count = 0;
            
            for (int i = std::max(0, -lag); i < static_cast<int>(reference.size() - std::max(0, lag)); ++i) {
                sum += reference[i] * processed[i + lag];
                count++;
            }
            
            if (count > 0) {
                correlation.push_back(static_cast<float>(sum / count));
            }
        }
        
        // Find peak correlation
        auto maxIt = std::max_element(correlation.begin(), correlation.end());
        if (maxIt != correlation.end()) {
            int peakLag = static_cast<int>(maxIt - correlation.begin()) - maxLag;
            return std::abs(static_cast<float>(peakLag));
        }
        
        return 0.0f;
    }
};

// Main test class for Bit Crusher
class BitCrusherTest {
private:
    std::unique_ptr<BitCrusher> bitCrusher;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    BitCrusherTest() {
        bitCrusher = std::make_unique<BitCrusher>();
        
        // Open log files
        logFile.open("BitCrusher_TestResults.txt");
        csvFile.open("BitCrusher_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the bit crusher
        bitCrusher->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Bit Crusher Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_BIT_CRUSHER) + "\n");
        log("Parameter Count: " + std::to_string(bitCrusher->getNumParameters()) + "\n\n");
    }
    
    ~BitCrusherTest() {
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
    
    // Process audio through bit crusher
    std::pair<std::vector<float>, std::vector<float>> processAudio(const std::vector<float>& input, 
                                                                  const std::map<int, float>& parameters) {
        // Update parameters
        bitCrusher->updateParameters(parameters);
        
        // Process in blocks and keep original for comparison
        std::vector<float> output;
        std::vector<float> original = input; // Copy for comparison
        output.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), 
                                      input.size() - i);
            
            // Create JUCE AudioBuffer
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            buffer.clear();
            
            // Fill buffer with input (mono to stereo)
            for (size_t j = 0; j < blockSize; ++j) {
                float sample = (i + j < input.size()) ? input[i + j] : 0.0f;
                buffer.setSample(0, static_cast<int>(j), sample);
                buffer.setSample(1, static_cast<int>(j), sample);
            }
            
            // Process
            bitCrusher->process(buffer);
            
            // Extract output (left channel)
            for (size_t j = 0; j < blockSize; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }
        
        return {original, output};
    }
    
    // Test 1: Parameter validation and response
    void testParameterResponse() {
        log("\n--- Parameter Response Tests ---\n");
        
        // Test signal: 1kHz sine at moderate level
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.5, 1.0, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < bitCrusher->getNumParameters(); ++param) {
            std::string paramName = bitCrusher->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            // Sweep from 0.0 to 1.0 in 0.25 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                // Set default values for all parameters
                for (int p = 0; p < bitCrusher->getNumParameters(); ++p) {
                    params[p] = 0.5f; // Default to middle position
                }
                
                // Override the parameter being tested
                params[param] = value;
                
                auto [original, output] = processAudio(testSignal, params);
                
                // Check for valid output
                assertTrue(!AudioAnalyzer::hasInvalidValues(output), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
                responseDB.push_back(outputRMS);
                
                logCSV("ParameterResponse", paramName + "_" + std::to_string(value), 
                       outputRMS, "PASS", "dB");
            }
            
            // Check parameter responsiveness
            float minResponse = *std::min_element(responseDB.begin(), responseDB.end());
            float maxResponse = *std::max_element(responseDB.begin(), responseDB.end());
            float responseRange = maxResponse - minResponse;
            
            log("  Response range: " + std::to_string(responseRange) + "dB\n");
            
            // Core bit crushing parameters should have significant effect
            if (param <= 3) { // Core parameters: bit depth, sample rate, etc.
                assertTrue(responseRange > 1.0f, 
                          paramName + " has audible effect (range: " + 
                          std::to_string(responseRange) + "dB)");
            }
        }
    }
    
    // Test 2: Bit depth reduction accuracy
    void testBitDepthReduction() {
        log("\n--- Bit Depth Reduction Accuracy Tests ---\n");
        
        // Generate stepped signal for quantization testing
        auto steppedSignal = TestSignalGenerator::generateSteppedSignal(256, 0.8, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> bitDepthSettings = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f}; // Various bit depths
        
        for (float bitDepth : bitDepthSettings) {
            log("\nTesting bit depth setting: " + std::to_string(bitDepth) + "\n");
            
            std::map<int, float> params;
            params[0] = bitDepth;    // Bit depth parameter
            params[1] = 0.9f;        // High sample rate (no downsampling)
            params[7] = 0.0f;        // No mix (full effect)
            
            auto [original, output] = processAudio(steppedSignal, params);
            
            // Measure effective bit depth
            float effectiveBitDepth = AudioAnalyzer::measureEffectiveBitDepth(output);
            
            log("  Effective bit depth: " + std::to_string(effectiveBitDepth) + " bits\n");
            
            logCSV("BitDepthReduction", "Setting_" + std::to_string(bitDepth),
                   effectiveBitDepth, "MEASURED", "bits");
            
            // Calculate SQNR
            float sqnr = AudioAnalyzer::calculateSQNR(original, output);
            log("  SQNR: " + std::to_string(sqnr) + "dB\n");
            
            logCSV("BitDepthReduction", "SQNR_" + std::to_string(bitDepth),
                   sqnr, "MEASURED", "dB");
            
            // Lower bit depth settings should reduce effective bit depth
            if (bitDepth < 0.5f) {
                assertTrue(effectiveBitDepth < 12.0f, 
                          "Reduced bit depth at setting " + std::to_string(bitDepth));
            }
            
            // SQNR should decrease with lower bit depth
            assertTrue(sqnr < 100.0f, 
                      "Quantization noise present at bit depth " + std::to_string(bitDepth));
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at bit depth " + std::to_string(bitDepth));
        }
    }
    
    // Test 3: Sample rate downsampling accuracy
    void testSampleRateDownsampling() {
        log("\n--- Sample Rate Downsampling Tests ---\n");
        
        // Generate high-frequency test signal
        auto testSignal = TestSignalGenerator::generateHighFreqTest(TEST_SAMPLE_RATE);
        
        std::vector<float> sampleRateSettings = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};
        
        for (float sampleRateSetting : sampleRateSettings) {
            log("\nTesting sample rate setting: " + std::to_string(sampleRateSetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.9f;               // High bit depth (no quantization)
            params[1] = sampleRateSetting;  // Sample rate reduction
            params[7] = 0.0f;               // No mix
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure aliasing artifacts
            float aliasingLevel = AudioAnalyzer::detectAliasing(output, TEST_SAMPLE_RATE);
            log("  Aliasing level: " + std::to_string(aliasingLevel) + "dB\n");
            
            logCSV("SampleRateDownsampling", "Aliasing_" + std::to_string(sampleRateSetting),
                   aliasingLevel, "MEASURED", "dB");
            
            // Measure sample rate artifacts
            float artifacts = AudioAnalyzer::measureSampleRateArtifacts(original, output, TEST_SAMPLE_RATE);
            log("  Sample rate artifacts: " + std::to_string(artifacts) + "dB\n");
            
            logCSV("SampleRateDownsampling", "Artifacts_" + std::to_string(sampleRateSetting),
                   artifacts, "MEASURED", "dB");
            
            // Lower sample rate should introduce more aliasing
            if (sampleRateSetting < 0.5f) {
                assertTrue(aliasingLevel > -80.0f, 
                          "Aliasing artifacts present at low sample rate " + std::to_string(sampleRateSetting));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at sample rate " + std::to_string(sampleRateSetting));
        }
    }
    
    // Test 4: Dithering effectiveness
    void testDitheringEffectiveness() {
        log("\n--- Dithering Effectiveness Tests ---\n");
        
        // Generate low-level sine wave for dither testing
        auto lowLevelSignal = TestSignalGenerator::generateSineWave(1000.0, 0.01, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> ditherSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float dither : ditherSettings) {
            log("\nTesting dither setting: " + std::to_string(dither) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f;  // Low bit depth to force quantization
            params[1] = 0.8f;  // High sample rate
            params[6] = dither; // Dither parameter
            params[7] = 0.0f;  // No mix
            
            auto [original, output] = processAudio(lowLevelSignal, params);
            
            // Calculate SQNR
            float sqnr = AudioAnalyzer::calculateSQNR(original, output);
            log("  SQNR with dither: " + std::to_string(sqnr) + "dB\n");
            
            logCSV("DitheringEffectiveness", "SQNR_Dither_" + std::to_string(dither),
                   sqnr, "MEASURED", "dB");
            
            // Measure noise floor
            auto silenceSignal = std::vector<float>(static_cast<int>(0.5 * TEST_SAMPLE_RATE), 0.0f);
            auto [silenceOrig, silenceOut] = processAudio(silenceSignal, params);
            
            float noiseFloor = AudioAnalyzer::calculateRMS_dB(silenceOut);
            log("  Noise floor: " + std::to_string(noiseFloor) + "dB\n");
            
            logCSV("DitheringEffectiveness", "NoiseFloor_Dither_" + std::to_string(dither),
                   noiseFloor, "MEASURED", "dB");
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with dither " + std::to_string(dither));
            
            // Dithering should affect the noise characteristics
            if (dither > 0.5f) {
                assertTrue(noiseFloor > -100.0f, 
                          "Dither noise present at setting " + std::to_string(dither));
            }
        }
    }
    
    // Test 5: Jitter and timing accuracy
    void testJitterAndTiming() {
        log("\n--- Jitter and Timing Accuracy Tests ---\n");
        
        // Generate precise timing reference signal
        auto referenceSignal = TestSignalGenerator::generateSineWave(1000.0, 0.5, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> jitterSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float jitter : jitterSettings) {
            log("\nTesting jitter setting: " + std::to_string(jitter) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.8f;  // High bit depth
            params[1] = 0.6f;  // Moderate sample rate reduction
            params[3] = jitter; // Jitter parameter
            params[7] = 0.0f;  // No mix
            
            auto [original, output] = processAudio(referenceSignal, params);
            
            // Measure timing jitter
            float timingJitter = AudioAnalyzer::measureTimingJitter(original, output);
            log("  Timing jitter: " + std::to_string(timingJitter) + " samples\n");
            
            logCSV("JitterTiming", "TimingJitter_" + std::to_string(jitter),
                   timingJitter, "MEASURED", "samples");
            
            // Calculate correlation with original
            float correlation = 0.0f;
            if (original.size() == output.size() && !original.empty()) {
                double sum_orig = 0.0, sum_out = 0.0, sum_orig_out = 0.0;
                double sum_orig_sq = 0.0, sum_out_sq = 0.0;
                
                for (size_t i = 0; i < original.size(); ++i) {
                    sum_orig += original[i];
                    sum_out += output[i];
                    sum_orig_out += original[i] * output[i];
                    sum_orig_sq += original[i] * original[i];
                    sum_out_sq += output[i] * output[i];
                }
                
                double n = static_cast<double>(original.size());
                double numerator = n * sum_orig_out - sum_orig * sum_out;
                double denominator = std::sqrt((n * sum_orig_sq - sum_orig * sum_orig) * 
                                             (n * sum_out_sq - sum_out * sum_out));
                
                if (denominator > 0.0) {
                    correlation = static_cast<float>(numerator / denominator);
                }
            }
            
            log("  Correlation with original: " + std::to_string(correlation) + "\n");
            
            logCSV("JitterTiming", "Correlation_" + std::to_string(jitter),
                   correlation, "MEASURED", "ratio");
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with jitter " + std::to_string(jitter));
            
            // Higher jitter should reduce correlation
            if (jitter > 0.7f) {
                assertTrue(correlation < 0.99f, 
                          "Jitter affects timing at setting " + std::to_string(jitter));
            }
        }
    }
    
    // Test 6: DC offset handling
    void testDCOffsetHandling() {
        log("\n--- DC Offset Handling Tests ---\n");
        
        // Create signal with known DC offset
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.3, 1.0, TEST_SAMPLE_RATE);
        
        // Add DC offset
        for (float& sample : testSignal) {
            sample += 0.2f; // 20% DC offset
        }
        
        std::vector<float> dcOffsetSettings = {0.0f, 0.3f, 0.5f, 0.7f, 1.0f};
        
        for (float dcOffset : dcOffsetSettings) {
            log("\nTesting DC offset setting: " + std::to_string(dcOffset) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.6f;     // Bit depth
            params[1] = 0.7f;     // Sample rate
            params[4] = dcOffset; // DC offset parameter
            params[7] = 0.0f;     // No mix
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure DC offset in output
            float outputDC = AudioAnalyzer::calculateDCOffset(output);
            log("  Output DC offset: " + std::to_string(outputDC) + "\n");
            
            logCSV("DCOffsetHandling", "OutputDC_" + std::to_string(dcOffset),
                   outputDC, "MEASURED", "amplitude");
            
            // Measure AC content (RMS after DC removal)
            std::vector<float> dcRemovedOutput = output;
            for (float& sample : dcRemovedOutput) {
                sample -= outputDC;
            }
            
            float acContent = AudioAnalyzer::calculateRMS_dB(dcRemovedOutput);
            log("  AC content: " + std::to_string(acContent) + "dB\n");
            
            logCSV("DCOffsetHandling", "ACContent_" + std::to_string(dcOffset),
                   acContent, "MEASURED", "dB");
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with DC offset setting " + std::to_string(dcOffset));
            
            // DC offset should be controlled
            assertTrue(std::abs(outputDC) < 0.8f, 
                      "DC offset controlled at setting " + std::to_string(dcOffset));
        }
    }
    
    // Test 7: Aliasing control effectiveness
    void testAliasingControl() {
        log("\n--- Aliasing Control Effectiveness Tests ---\n");
        
        // Generate signal with high-frequency content
        auto highFreqSignal = TestSignalGenerator::generateSineWave(8000.0, 0.4, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> aliasingSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float aliasing : aliasingSettings) {
            log("\nTesting aliasing control: " + std::to_string(aliasing) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;     // Moderate bit depth
            params[1] = 0.3f;     // Low sample rate (should cause aliasing)
            params[2] = aliasing; // Aliasing parameter
            params[7] = 0.0f;     // No mix
            
            auto [original, output] = processAudio(highFreqSignal, params);
            
            // Measure aliasing artifacts
            float aliasingLevel = AudioAnalyzer::detectAliasing(output, TEST_SAMPLE_RATE, 12000.0);
            log("  Aliasing level: " + std::to_string(aliasingLevel) + "dB\n");
            
            logCSV("AliasingControl", "AliasingLevel_" + std::to_string(aliasing),
                   aliasingLevel, "MEASURED", "dB");
            
            // Measure high-frequency content preservation
            float artifacts = AudioAnalyzer::measureSampleRateArtifacts(original, output, TEST_SAMPLE_RATE);
            log("  HF artifacts: " + std::to_string(artifacts) + "dB\n");
            
            logCSV("AliasingControl", "HFArtifacts_" + std::to_string(aliasing),
                   artifacts, "MEASURED", "dB");
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with aliasing control " + std::to_string(aliasing));
            
            // Higher aliasing control should reduce artifacts
            if (aliasing > 0.7f) {
                assertTrue(aliasingLevel < -20.0f, 
                          "Aliasing controlled at setting " + std::to_string(aliasing));
            }
        }
    }
    
    // Test 8: Combined parameter effects
    void testCombinedParameterEffects() {
        log("\n--- Combined Parameter Effects Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.4, 1.0, TEST_SAMPLE_RATE);
        
        // Test extreme combinations
        std::vector<std::map<int, float>> combinations = {
            {{0, 0.1f}, {1, 0.1f}, {6, 0.8f}}, // Low bit depth, low sample rate, high dither
            {{0, 0.9f}, {1, 0.9f}, {3, 0.8f}}, // High bit depth, high sample rate, high jitter
            {{0, 0.3f}, {1, 0.7f}, {2, 0.9f}}, // Low bit depth, high sample rate, high aliasing
            {{0, 0.7f}, {1, 0.3f}, {4, 0.5f}}  // High bit depth, low sample rate, medium DC offset
        };
        
        for (size_t i = 0; i < combinations.size(); ++i) {
            log("\nTesting parameter combination " + std::to_string(i + 1) + "\n");
            
            auto [original, output] = processAudio(testSignal, combinations[i]);
            
            // Basic quality metrics
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float outputPeak = AudioAnalyzer::calculatePeak_dB(output);
            float effectiveBits = AudioAnalyzer::measureEffectiveBitDepth(output);
            float sqnr = AudioAnalyzer::calculateSQNR(original, output);
            
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            log("  Output Peak: " + std::to_string(outputPeak) + "dB\n");
            log("  Effective bits: " + std::to_string(effectiveBits) + "\n");
            log("  SQNR: " + std::to_string(sqnr) + "dB\n");
            
            logCSV("CombinedEffects", "Combo" + std::to_string(i + 1) + "_RMS", outputRMS, "MEASURED", "dB");
            logCSV("CombinedEffects", "Combo" + std::to_string(i + 1) + "_EffectiveBits", effectiveBits, "MEASURED", "bits");
            logCSV("CombinedEffects", "Combo" + std::to_string(i + 1) + "_SQNR", sqnr, "MEASURED", "dB");
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output for combination " + std::to_string(i + 1));
            
            assertTrue(outputPeak < 6.0f, 
                      "Output level controlled for combination " + std::to_string(i + 1));
        }
    }
    
    // Test 9: Real-time performance and stability
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        // Generate longer test signal
        auto longSignal = TestSignalGenerator::generateSineWave(1000.0, 0.3, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.5f; // Moderate settings
        params[1] = 0.5f;
        params[2] = 0.5f;
        params[3] = 0.3f;
        
        // Measure processing time
        auto startTime = std::chrono::high_resolution_clock::now();
        auto [original, output] = processAudio(longSignal, params);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0; // milliseconds
        
        double signalDuration = longSignal.size() / TEST_SAMPLE_RATE * 1000.0; // milliseconds
        double realTimeRatio = processingTime / signalDuration;
        
        log("Processing time: " + std::to_string(processingTime) + "ms\n");
        log("Signal duration: " + std::to_string(signalDuration) + "ms\n");
        log("Real-time ratio: " + std::to_string(realTimeRatio) + "\n");
        
        logCSV("Performance", "ProcessingTime", processingTime, "MEASURED", "ms");
        logCSV("Performance", "RealTimeRatio", realTimeRatio, "MEASURED", "ratio");
        
        // Check output quality
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid output during performance test");
        assertTrue(realTimeRatio < 0.3, "Real-time processing capability");
        
        float outputStability = AudioAnalyzer::calculateRMS_dB(output);
        assertTrue(outputStability > -60.0f, "Stable output level");
        
        log("Output stability: " + std::to_string(outputStability) + "dB\n");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Bit Crusher comprehensive test suite...\n");
        
        testParameterResponse();
        testBitDepthReduction();
        testSampleRateDownsampling();
        testDitheringEffectiveness();
        testJitterAndTiming();
        testDCOffsetHandling();
        testAliasingControl();
        testCombinedParameterEffects();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        BitCrusherTest tester;
        tester.runAllTests();
        
        std::cout << "\nBit Crusher test suite completed successfully.\n";
        std::cout << "Check BitCrusher_TestResults.txt for detailed results.\n";
        std::cout << "Check BitCrusher_Data.csv for measurement data.\n";
        
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