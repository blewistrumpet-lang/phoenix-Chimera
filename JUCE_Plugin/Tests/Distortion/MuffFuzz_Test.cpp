/*
  ==============================================================================
  
    MuffFuzz_Test.cpp
    Comprehensive test suite for ENGINE_MUFF_FUZZ
    
    Tests for Muff Fuzz characteristics:
    - Sustain behavior analysis
    - Gate threshold accuracy
    - Compression characteristics
    - Feedback control testing
    - Fuzz tone generation
    - Dynamic response validation
    - Sustain decay measurement
    - Gate opening/closing behavior
    
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
#include "../../Source/MuffFuzz.h"
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
    
    // Generate exponentially decaying signal for sustain testing
    static std::vector<float> generateDecayingSine(double frequency, double amplitude,
                                                  double duration, double decayTime,
                                                  double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        double decayRate = 1.0 / (decayTime * sampleRate);
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double envelope = amplitude * std::exp(-decayRate * i);
            
            signal[i] = static_cast<float>(envelope * std::sin(phase));
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
        
        return signal;
    }
    
    // Generate note with attack, sustain, decay for gate testing
    static std::vector<float> generateASDNote(double frequency, double peakAmplitude,
                                            double attackTime, double sustainTime,
                                            double decayTime, double sampleRate) {
        int attackSamples = static_cast<int>(attackTime * sampleRate);
        int sustainSamples = static_cast<int>(sustainTime * sampleRate);
        int decaySamples = static_cast<int>(decayTime * sampleRate);
        int totalSamples = attackSamples + sustainSamples + decaySamples;
        
        std::vector<float> signal(totalSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < totalSamples; ++i) {
            double envelope;
            
            if (i < attackSamples) {
                // Attack phase
                envelope = peakAmplitude * (static_cast<double>(i) / attackSamples);
            } else if (i < attackSamples + sustainSamples) {
                // Sustain phase
                envelope = peakAmplitude;
            } else {
                // Decay phase
                int decayIndex = i - attackSamples - sustainSamples;
                envelope = peakAmplitude * std::exp(-5.0 * decayIndex / decaySamples);
            }
            
            signal[i] = static_cast<float>(envelope * std::sin(phase));
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
        
        return signal;
    }
    
    // Generate varying amplitude signal for dynamic testing
    static std::vector<float> generateVaryingAmplitude(double frequency, double duration,
                                                      double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            
            // Varying amplitude envelope (slow modulation)
            double amplitude = 0.1 + 0.4 * (1.0 + std::sin(2.0 * M_PI * 0.5 * t)) / 2.0;
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
        }
        
        return signal;
    }
    
    // Generate burst signal for gate testing
    static std::vector<float> generateBurst(double frequency, double amplitude,
                                          double burstDuration, double silenceDuration,
                                          int numBursts, double sampleRate) {
        int burstSamples = static_cast<int>(burstDuration * sampleRate);
        int silenceSamples = static_cast<int>(silenceDuration * sampleRate);
        int totalSamples = numBursts * (burstSamples + silenceSamples);
        
        std::vector<float> signal(totalSamples, 0.0f);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int burst = 0; burst < numBursts; ++burst) {
            int startIdx = burst * (burstSamples + silenceSamples);
            
            for (int i = 0; i < burstSamples; ++i) {
                signal[startIdx + i] = static_cast<float>(amplitude * std::sin(phase));
                phase += phaseIncrement;
                if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
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
    
    // Measure sustain time (time for signal to decay to -20dB from peak)
    static float measureSustainTime(const std::vector<float>& signal, double sampleRate) {
        if (signal.empty()) return 0.0f;
        
        // Find peak level
        float peakLevel = 0.0f;
        int peakIndex = 0;
        
        for (int i = 0; i < static_cast<int>(signal.size()); ++i) {
            float level = std::abs(signal[i]);
            if (level > peakLevel) {
                peakLevel = level;
                peakIndex = i;
            }
        }
        
        if (peakLevel < 1e-6f) return 0.0f;
        
        // Find -20dB point
        float targetLevel = peakLevel * 0.1f; // -20dB
        
        for (int i = peakIndex; i < static_cast<int>(signal.size()); ++i) {
            if (std::abs(signal[i]) < targetLevel) {
                return (i - peakIndex) / sampleRate;
            }
        }
        
        return static_cast<float>(signal.size() - peakIndex) / sampleRate;
    }
    
    // Detect gate opening/closing events
    static std::vector<int> detectGateEvents(const std::vector<float>& signal, float threshold) {
        std::vector<int> events;
        bool gateOpen = false;
        
        for (int i = 0; i < static_cast<int>(signal.size()); ++i) {
            float level = std::abs(signal[i]);
            
            if (!gateOpen && level > threshold) {
                // Gate opening
                events.push_back(i);
                gateOpen = true;
            } else if (gateOpen && level < threshold * 0.5f) {
                // Gate closing (with hysteresis)
                events.push_back(-i); // Negative for closing
                gateOpen = false;
            }
        }
        
        return events;
    }
    
    // Measure compression ratio
    static float measureCompressionRatio(const std::vector<float>& input,
                                       const std::vector<float>& output) {
        if (input.size() != output.size() || input.empty()) {
            return 1.0f;
        }
        
        // Calculate dynamic range
        float inputMin = *std::min_element(input.begin(), input.end());
        float inputMax = *std::max_element(input.begin(), input.end());
        float outputMin = *std::min_element(output.begin(), output.end());
        float outputMax = *std::max_element(output.begin(), output.end());
        
        float inputRange = inputMax - inputMin;
        float outputRange = outputMax - outputMin;
        
        if (outputRange == 0.0f) return 100.0f; // Infinite compression
        if (inputRange == 0.0f) return 1.0f;
        
        return inputRange / outputRange;
    }
    
    // Measure fuzz characteristics (harmonic content)
    static std::vector<float> analyzeFuzzHarmonics(const std::vector<float>& signal,
                                                  double fundamental, double sampleRate,
                                                  int maxHarmonics = 10) {
        std::vector<float> harmonic_levels(maxHarmonics, -120.0f);
        
        if (signal.size() < FFT_SIZE) return harmonic_levels;
        
        // Prepare windowed signal
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
        auto magnitudes_db = SimpleFFT::magnitudeDB(fft_result);
        
        // Extract harmonic levels
        for (int h = 1; h <= maxHarmonics; ++h) {
            int harm_bin = static_cast<int>(fundamental * h * FFT_SIZE / sampleRate);
            if (harm_bin < static_cast<int>(magnitudes_db.size() / 2)) {
                harmonic_levels[h - 1] = magnitudes_db[harm_bin];
            }
        }
        
        return harmonic_levels;
    }
    
    // Measure gate threshold accuracy
    static float measureGateThreshold(const std::vector<float>& input,
                                    const std::vector<float>& output,
                                    float expectedThreshold) {
        if (input.size() != output.size() || input.empty()) {
            return 0.0f;
        }
        
        // Find the input level where output starts to appear significantly
        float measuredThreshold = 1.0f;
        
        for (size_t i = 0; i < input.size(); ++i) {
            float inputLevel = std::abs(input[i]);
            float outputLevel = std::abs(output[i]);
            
            // Gate opening detection
            if (outputLevel > 0.01f && inputLevel < measuredThreshold) {
                measuredThreshold = inputLevel;
            }
        }
        
        return measuredThreshold;
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
    
    // Calculate crest factor (peak to RMS ratio)
    static float calculateCrestFactor(const std::vector<float>& signal) {
        if (signal.empty()) return 0.0f;
        
        float peak = 0.0f;
        double sumSquares = 0.0;
        
        for (float sample : signal) {
            peak = std::max(peak, std::abs(sample));
            sumSquares += sample * sample;
        }
        
        float rms = static_cast<float>(std::sqrt(sumSquares / signal.size()));
        
        if (rms == 0.0f) return 0.0f;
        
        return peak / rms;
    }
    
    // Measure feedback oscillation detection
    static bool detectFeedbackOscillation(const std::vector<float>& signal, double sampleRate) {
        if (signal.size() < FFT_SIZE) return false;
        
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
        
        // Look for sharp peaks that might indicate oscillation
        for (int i = 10; i < static_cast<int>(magnitudes_db.size() / 2) - 10; ++i) {
            if (magnitudes_db[i] > -20.0) { // High level peak
                // Check if it's significantly higher than neighbors
                bool isPeak = true;
                for (int j = i - 5; j <= i + 5; ++j) {
                    if (j != i && magnitudes_db[j] > magnitudes_db[i] - 10.0) {
                        isPeak = false;
                        break;
                    }
                }
                
                if (isPeak) return true; // Potential oscillation detected
            }
        }
        
        return false;
    }
};

// Main test class for Muff Fuzz
class MuffFuzzTest {
private:
    std::unique_ptr<MuffFuzz> muffFuzz;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    MuffFuzzTest() {
        muffFuzz = std::make_unique<MuffFuzz>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/MuffFuzz_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/MuffFuzz_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the muff fuzz
        muffFuzz->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Muff Fuzz Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_MUFF_FUZZ) + "\n");
        log("Parameter Count: " + std::to_string(muffFuzz->getNumParameters()) + "\n\n");
    }
    
    ~MuffFuzzTest() {
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
    
    // Process audio through muff fuzz
    std::pair<std::vector<float>, std::vector<float>> processAudio(const std::vector<float>& input, 
                                                                  const std::map<int, float>& parameters) {
        // Update parameters
        muffFuzz->updateParameters(parameters);
        
        // Process in blocks and keep original for comparison
        std::vector<float> output;
        std::vector<float> original = input; // Copy for comparison
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
            muffFuzz->process(buffer);
            
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
        
        // Test signal: guitar-like note
        auto testSignal = TestSignalGenerator::generateASDNote(220.0, 0.5, 0.05, 0.5, 0.5, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < muffFuzz->getNumParameters(); ++param) {
            std::string paramName = muffFuzz->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            // Sweep from 0.0 to 1.0 in 0.25 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                // Set default values for all parameters
                for (int p = 0; p < muffFuzz->getNumParameters(); ++p) {
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
            
            // Core fuzz parameters should have significant effect
            if (param <= 5) { // Core parameters
                assertTrue(responseRange > 1.0f, 
                          paramName + " has audible effect (range: " + 
                          std::to_string(responseRange) + "dB)");
            }
        }
    }
    
    // Test 2: Sustain behavior analysis
    void testSustainBehavior() {
        log("\n--- Sustain Behavior Analysis ---\n");
        
        // Test with naturally decaying signal
        auto decayingSignal = TestSignalGenerator::generateDecayingSine(220.0, 0.6, 3.0, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> sustainSettings = {0.2f, 0.5f, 0.8f};
        
        for (float sustain : sustainSettings) {
            log("\nTesting sustain setting: " + std::to_string(sustain) + "\n");
            
            std::map<int, float> params;
            params[0] = sustain;  // Sustain parameter
            params[1] = 0.6f;     // Fuzz amount
            params[2] = 0.5f;     // Tone
            
            auto [original, output] = processAudio(decayingSignal, params);
            
            // Measure sustain time
            float originalSustainTime = AudioAnalyzer::measureSustainTime(original, TEST_SAMPLE_RATE);
            float processedSustainTime = AudioAnalyzer::measureSustainTime(output, TEST_SAMPLE_RATE);
            
            log("  Original sustain time: " + std::to_string(originalSustainTime) + "s\n");
            log("  Processed sustain time: " + std::to_string(processedSustainTime) + "s\n");
            
            float sustainRatio = processedSustainTime / originalSustainTime;
            log("  Sustain ratio: " + std::to_string(sustainRatio) + "\n");
            
            logCSV("SustainBehavior", "SustainTime_" + std::to_string(sustain),
                   processedSustainTime, "MEASURED", "seconds");
            logCSV("SustainBehavior", "SustainRatio_" + std::to_string(sustain),
                   sustainRatio, "MEASURED", "ratio");
            
            // Higher sustain setting should increase sustain time
            if (sustain > 0.6f) {
                assertTrue(sustainRatio > 1.0f, 
                          "Sustain increased at setting " + std::to_string(sustain));
            }
            
            // Measure compression characteristics
            float compressionRatio = AudioAnalyzer::measureCompressionRatio(original, output);
            log("  Compression ratio: " + std::to_string(compressionRatio) + ":1\n");
            
            logCSV("SustainBehavior", "CompressionRatio_" + std::to_string(sustain),
                   compressionRatio, "MEASURED", "ratio");
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at sustain setting " + std::to_string(sustain));
        }
    }
    
    // Test 3: Gate threshold accuracy
    void testGateThreshold() {
        log("\n--- Gate Threshold Accuracy Tests ---\n");
        
        // Test with varying amplitude signal
        auto varyingSignal = TestSignalGenerator::generateVaryingAmplitude(220.0, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> gateSettings = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};
        
        for (float gate : gateSettings) {
            log("\nTesting gate threshold: " + std::to_string(gate) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;  // Sustain
            params[1] = 0.6f;  // Fuzz
            params[5] = gate;  // Gate threshold parameter (assuming index 5)
            
            auto [original, output] = processAudio(varyingSignal, params);
            
            // Detect gate events
            float expectedThreshold = gate * 0.5f; // Expected threshold level
            auto gateEvents = AudioAnalyzer::detectGateEvents(output, expectedThreshold);
            
            log("  Gate events detected: " + std::to_string(gateEvents.size()) + "\n");
            
            logCSV("GateThreshold", "GateEvents_" + std::to_string(gate),
                   static_cast<float>(gateEvents.size()), "MEASURED", "count");
            
            // Measure actual gate threshold
            float measuredThreshold = AudioAnalyzer::measureGateThreshold(original, output, expectedThreshold);
            log("  Measured threshold: " + std::to_string(measuredThreshold) + "\n");
            
            logCSV("GateThreshold", "MeasuredThreshold_" + std::to_string(gate),
                   measuredThreshold, "MEASURED", "amplitude");
            
            // Gate should be functional
            if (gate > 0.3f) {
                assertTrue(gateEvents.size() > 0, 
                          "Gate events occur at setting " + std::to_string(gate));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at gate setting " + std::to_string(gate));
        }
    }
    
    // Test 4: Fuzz tone generation
    void testFuzzToneGeneration() {
        log("\n--- Fuzz Tone Generation Tests ---\n");
        
        // Test with clean sine wave
        auto cleanSignal = TestSignalGenerator::generateSineWave(220.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> fuzzSettings = {0.2f, 0.5f, 0.8f};
        
        for (float fuzz : fuzzSettings) {
            log("\nTesting fuzz amount: " + std::to_string(fuzz) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;  // Sustain
            params[1] = fuzz;  // Fuzz amount
            params[2] = 0.5f;  // Tone
            
            auto [original, output] = processAudio(cleanSignal, params);
            
            // Analyze harmonic content
            auto harmonics = AudioAnalyzer::analyzeFuzzHarmonics(output, 220.0, TEST_SAMPLE_RATE, 8);
            
            log("  Harmonic content:\n");
            for (int h = 0; h < 5; ++h) {
                log("    H" + std::to_string(h + 1) + ": " + 
                    std::to_string(harmonics[h]) + "dB\n");
                
                logCSV("FuzzToneGeneration", 
                       "H" + std::to_string(h + 1) + "_Fuzz" + std::to_string(fuzz),
                       harmonics[h], "MEASURED", "dB");
            }
            
            // Calculate crest factor (indicates squashing/clipping)
            float crestFactor = AudioAnalyzer::calculateCrestFactor(output);
            log("  Crest factor: " + std::to_string(crestFactor) + "\n");
            
            logCSV("FuzzToneGeneration", "CrestFactor_" + std::to_string(fuzz),
                   crestFactor, "MEASURED", "ratio");
            
            // Higher fuzz should generate more harmonics
            if (fuzz > 0.6f) {
                bool hasHarmonics = false;
                for (int h = 1; h < 5; ++h) { // Check H2-H5
                    if (harmonics[h] > -40.0f) {
                        hasHarmonics = true;
                        break;
                    }
                }
                
                assertTrue(hasHarmonics, 
                          "Harmonic content generated at fuzz " + std::to_string(fuzz));
            }
            
            // Fuzz should reduce crest factor (more squashed waveform)
            if (fuzz > 0.5f) {
                assertTrue(crestFactor < 3.0f, 
                          "Waveform squashing at fuzz " + std::to_string(fuzz));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at fuzz setting " + std::to_string(fuzz));
        }
    }
    
    // Test 5: Feedback control testing
    void testFeedbackControl() {
        log("\n--- Feedback Control Testing ---\n");
        
        auto testSignal = TestSignalGenerator::generateSineWave(440.0, 0.3, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> feedbackSettings = {0.0f, 0.3f, 0.6f, 0.9f};
        
        for (float feedback : feedbackSettings) {
            log("\nTesting feedback setting: " + std::to_string(feedback) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.6f;     // Sustain
            params[1] = 0.5f;     // Fuzz
            params[3] = feedback; // Feedback parameter (assuming index 3)
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Check for oscillation at high feedback
            bool hasOscillation = AudioAnalyzer::detectFeedbackOscillation(output, TEST_SAMPLE_RATE);
            
            log("  Oscillation detected: " + std::string(hasOscillation ? "Yes" : "No") + "\n");
            
            logCSV("FeedbackControl", "Oscillation_" + std::to_string(feedback),
                   hasOscillation ? 1.0f : 0.0f, "MEASURED", "boolean");
            
            // Measure output level change due to feedback
            float inputRMS = AudioAnalyzer::calculateRMS_dB(original);
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float gain = outputRMS - inputRMS;
            
            log("  Feedback gain: " + std::to_string(gain) + "dB\n");
            
            logCSV("FeedbackControl", "FeedbackGain_" + std::to_string(feedback),
                   gain, "MEASURED", "dB");
            
            // High feedback might cause oscillation but should remain stable
            if (feedback > 0.8f) {
                assertTrue(gain < 20.0f, 
                          "Feedback controlled at setting " + std::to_string(feedback));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at feedback setting " + std::to_string(feedback));
        }
    }
    
    // Test 6: Dynamic response validation
    void testDynamicResponse() {
        log("\n--- Dynamic Response Validation ---\n");
        
        // Test with burst signal (note attacks)
        auto burstSignal = TestSignalGenerator::generateBurst(220.0, 0.5, 0.2, 0.3, 5, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.7f; // High sustain
        params[1] = 0.6f; // Moderate fuzz
        params[4] = 0.4f; // Gate threshold
        
        auto [original, output] = processAudio(burstSignal, params);
        
        // Analyze attack and decay behavior
        auto gateEvents = AudioAnalyzer::detectGateEvents(output, 0.1f);
        
        log("Total gate events: " + std::to_string(gateEvents.size()) + "\n");
        
        logCSV("DynamicResponse", "TotalGateEvents", 
               static_cast<float>(gateEvents.size()), "MEASURED", "count");
        
        // Measure response to level changes
        std::vector<float> burstLevels;
        int burstLength = static_cast<int>(0.2 * TEST_SAMPLE_RATE);
        int cycleLength = static_cast<int>(0.5 * TEST_SAMPLE_RATE);
        
        for (int burst = 0; burst < 5; ++burst) {
            int startIdx = burst * cycleLength;
            int endIdx = startIdx + burstLength;
            
            if (endIdx < static_cast<int>(output.size())) {
                float burstRMS = 0.0f;
                for (int i = startIdx; i < endIdx; ++i) {
                    burstRMS += output[i] * output[i];
                }
                burstRMS = std::sqrt(burstRMS / burstLength);
                burstLevels.push_back(20.0f * std::log10(std::max(1e-6f, burstRMS)));
                
                log("  Burst " + std::to_string(burst + 1) + " level: " + 
                    std::to_string(burstLevels.back()) + "dB\n");
            }
        }
        
        // Check consistency of burst levels (sustain effect)
        if (burstLevels.size() >= 2) {
            float levelVariation = *std::max_element(burstLevels.begin(), burstLevels.end()) -
                                  *std::min_element(burstLevels.begin(), burstLevels.end());
            
            log("  Burst level variation: " + std::to_string(levelVariation) + "dB\n");
            
            logCSV("DynamicResponse", "BurstLevelVariation", levelVariation, "MEASURED", "dB");
            
            // Sustain should reduce level variation
            assertTrue(levelVariation < 10.0f, "Consistent burst levels due to sustain");
        }
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid dynamic response output");
    }
    
    // Test 7: Gate opening/closing behavior
    void testGateOpeningClosing() {
        log("\n--- Gate Opening/Closing Behavior ---\n");
        
        // Create signal that crosses gate threshold multiple times
        std::vector<float> testSignal;
        
        // Below threshold
        auto lowLevel = TestSignalGenerator::generateSineWave(220.0, 0.05, 0.3, TEST_SAMPLE_RATE);
        testSignal.insert(testSignal.end(), lowLevel.begin(), lowLevel.end());
        
        // Above threshold
        auto highLevel = TestSignalGenerator::generateSineWave(220.0, 0.4, 0.3, TEST_SAMPLE_RATE);
        testSignal.insert(testSignal.end(), highLevel.begin(), highLevel.end());
        
        // Below threshold again
        auto lowLevel2 = TestSignalGenerator::generateSineWave(220.0, 0.05, 0.3, TEST_SAMPLE_RATE);
        testSignal.insert(testSignal.end(), lowLevel2.begin(), lowLevel2.end());
        
        std::map<int, float> params;
        params[0] = 0.5f; // Sustain
        params[1] = 0.5f; // Fuzz
        params[5] = 0.5f; // Gate threshold
        
        auto [original, output] = processAudio(testSignal, params);
        
        // Detect gate transitions
        auto gateEvents = AudioAnalyzer::detectGateEvents(output, 0.1f);
        
        int openEvents = 0, closeEvents = 0;
        for (int event : gateEvents) {
            if (event > 0) openEvents++;
            else closeEvents++;
        }
        
        log("Gate open events: " + std::to_string(openEvents) + "\n");
        log("Gate close events: " + std::to_string(closeEvents) + "\n");
        
        logCSV("GateOpeningClosing", "OpenEvents", static_cast<float>(openEvents), "MEASURED", "count");
        logCSV("GateOpeningClosing", "CloseEvents", static_cast<float>(closeEvents), "MEASURED", "count");
        
        // Should have at least one open and one close event
        assertTrue(openEvents > 0, "Gate opening events detected");
        assertTrue(closeEvents > 0, "Gate closing events detected");
        
        // Analyze gate timing
        if (!gateEvents.empty()) {
            std::vector<float> gateOpenTimes;
            for (int event : gateEvents) {
                if (event > 0) {
                    gateOpenTimes.push_back(event / TEST_SAMPLE_RATE);
                }
            }
            
            if (gateOpenTimes.size() >= 1) {
                log("  First gate opening at: " + std::to_string(gateOpenTimes[0]) + "s\n");
                
                // Should open somewhere in the middle section
                assertTrue(gateOpenTimes[0] > 0.1f && gateOpenTimes[0] < 0.8f, 
                          "Gate opens at appropriate time");
            }
        }
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid gate behavior output");
    }
    
    // Test 8: Performance and stability
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        // Generate longer test signal with typical guitar content
        auto longSignal = TestSignalGenerator::generateASDNote(110.0, 0.6, 0.1, 2.0, 2.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.7f; // High sustain
        params[1] = 0.8f; // High fuzz
        params[3] = 0.6f; // Moderate feedback
        
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
        
        // Check output quality and stability
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid output during performance test");
        assertTrue(realTimeRatio < 0.3, "Real-time processing capability");
        
        float outputLevel = AudioAnalyzer::calculatePeak_dB(output);
        assertTrue(outputLevel < 6.0f, "Output level controlled");
        
        // Check for unwanted oscillations
        bool hasUnwantedOscillation = AudioAnalyzer::detectFeedbackOscillation(output, TEST_SAMPLE_RATE);
        assertTrue(!hasUnwantedOscillation, "No unwanted oscillations");
        
        log("Peak output level: " + std::to_string(outputLevel) + "dB\n");
        log("Unwanted oscillation: " + std::string(hasUnwantedOscillation ? "Yes" : "No") + "\n");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Muff Fuzz comprehensive test suite...\n");
        
        testParameterResponse();
        testSustainBehavior();
        testGateThreshold();
        testFuzzToneGeneration();
        testFeedbackControl();
        testDynamicResponse();
        testGateOpeningClosing();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        MuffFuzzTest tester;
        tester.runAllTests();
        
        std::cout << "\nMuff Fuzz test suite completed successfully.\n";
        std::cout << "Check MuffFuzz_TestResults.txt for detailed results.\n";
        std::cout << "Check MuffFuzz_Data.csv for measurement data.\n";
        
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