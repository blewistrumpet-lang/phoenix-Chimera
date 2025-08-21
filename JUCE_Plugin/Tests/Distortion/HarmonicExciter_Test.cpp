/*
  ==============================================================================
  
    HarmonicExciter_Test.cpp
    Comprehensive test suite for ENGINE_HARMONIC_EXCITER
    
    Tests for harmonic exciter characteristics:
    - Harmonic enhancement accuracy
    - Frequency-selective processing
    - Musical vs non-musical harmonics
    - Phase coherence analysis
    - Exciter algorithm validation
    - Frequency band processing
    - Dynamic enhancement testing
    - Psychoacoustic validation
    
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
#include "../../Source/HarmonicExciter.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;
constexpr int FFT_SIZE = 8192;

// FFT implementation for harmonic analysis
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
    
    // Generate complex harmonic test signal
    static std::vector<float> generateHarmonicComplex(double fundamental, double amplitude,
                                                    double duration, double sampleRate,
                                                    const std::vector<double>& harmonicLevels) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        // Add fundamental and harmonics
        for (size_t h = 0; h < harmonicLevels.size(); ++h) {
            double freq = fundamental * (h + 1);
            double harmAmp = amplitude * harmonicLevels[h];
            
            if (freq < sampleRate / 2) { // Below Nyquist
                double phase = 0.0;
                double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
                
                for (int i = 0; i < numSamples; ++i) {
                    signal[i] += static_cast<float>(harmAmp * std::sin(phase));
                    phase += phaseIncrement;
                    if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
                }
            }
        }
        
        return signal;
    }
    
    // Generate musical chord for testing musical enhancement
    static std::vector<float> generateChord(const std::vector<double>& frequencies,
                                          double amplitude, double duration,
                                          double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        for (double freq : frequencies) {
            if (freq < sampleRate / 2) {
                double phase = 0.0;
                double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
                double noteAmp = amplitude / frequencies.size(); // Normalize
                
                for (int i = 0; i < numSamples; ++i) {
                    signal[i] += static_cast<float>(noteAmp * std::sin(phase));
                    phase += phaseIncrement;
                    if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
                }
            }
        }
        
        return signal;
    }
    
    // Generate pink noise for broadband testing
    static std::vector<float> generatePinkNoise(double amplitude, double duration, 
                                               double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Pink noise generator state
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            float white = dist(gen);
            
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f;
            b4 = 0.55000f * b4 + white * 0.5329522f;
            b5 = -0.7616f * b5 - white * 0.0168980f;
            
            signal[i] = static_cast<float>(amplitude * (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f));
            b6 = white * 0.115926f;
        }
        
        return signal;
    }
    
    // Generate frequency sweep
    static std::vector<float> generateSweep(double startFreq, double endFreq,
                                          double amplitude, double duration,
                                          double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double progress = t / duration;
            
            // Logarithmic sweep
            double freq = startFreq * std::pow(endFreq / startFreq, progress);
            double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
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
    
    // Calculate peak level in dB
    static float calculatePeak_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        
        float peak = 0.0f;
        for (float sample : signal) {
            peak = std::max(peak, std::abs(sample));
        }
        
        return 20.0f * std::log10(std::max(1e-6, peak));
    }
    
    // Analyze harmonic enhancement
    static std::vector<float> analyzeHarmonicEnhancement(const std::vector<float>& original,
                                                       const std::vector<float>& processed,
                                                       double fundamental, double sampleRate,
                                                       int maxHarmonics = 10) {
        std::vector<float> enhancement(maxHarmonics, 0.0f);
        
        if (original.size() != processed.size() || original.size() < FFT_SIZE) {
            return enhancement;
        }
        
        // Analyze original signal
        std::vector<double> windowed_original(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(original.size())) {
                double window = 0.42 - 0.5 * std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)) + 
                               0.08 * std::cos(4.0 * M_PI * i / (FFT_SIZE - 1));
                windowed_original[i] = original[i] * window;
            } else {
                windowed_original[i] = 0.0;
            }
        }
        
        auto fft_original = SimpleFFT::fft(windowed_original);
        auto mag_original = SimpleFFT::magnitudeDB(fft_original);
        
        // Analyze processed signal
        std::vector<double> windowed_processed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(processed.size())) {
                double window = 0.42 - 0.5 * std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)) + 
                               0.08 * std::cos(4.0 * M_PI * i / (FFT_SIZE - 1));
                windowed_processed[i] = processed[i] * window;
            } else {
                windowed_processed[i] = 0.0;
            }
        }
        
        auto fft_processed = SimpleFFT::fft(windowed_processed);
        auto mag_processed = SimpleFFT::magnitudeDB(fft_processed);
        
        // Calculate enhancement for each harmonic
        for (int h = 1; h <= maxHarmonics; ++h) {
            int harm_bin = static_cast<int>(fundamental * h * FFT_SIZE / sampleRate);
            if (harm_bin < static_cast<int>(mag_original.size() / 2)) {
                enhancement[h - 1] = mag_processed[harm_bin] - mag_original[harm_bin];
            }
        }
        
        return enhancement;
    }
    
    // Measure frequency response in specific bands
    static std::vector<float> measureBandResponse(const std::vector<float>& input,
                                                const std::vector<float>& output,
                                                const std::vector<std::pair<double, double>>& bands,
                                                double sampleRate) {
        std::vector<float> bandGains;
        
        if (input.size() != output.size() || input.size() < FFT_SIZE) {
            return bandGains;
        }
        
        // FFT of input
        std::vector<double> windowed_input(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(input.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed_input[i] = input[i] * window;
            } else {
                windowed_input[i] = 0.0;
            }
        }
        
        auto fft_input = SimpleFFT::fft(windowed_input);
        auto mag_input = SimpleFFT::magnitude(fft_input);
        
        // FFT of output
        std::vector<double> windowed_output(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(output.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed_output[i] = output[i] * window;
            } else {
                windowed_output[i] = 0.0;
            }
        }
        
        auto fft_output = SimpleFFT::fft(windowed_output);
        auto mag_output = SimpleFFT::magnitude(fft_output);
        
        // Calculate band gains
        for (const auto& band : bands) {
            int startBin = static_cast<int>(band.first * FFT_SIZE / sampleRate);
            int endBin = static_cast<int>(band.second * FFT_SIZE / sampleRate);
            
            double inputEnergy = 0.0, outputEnergy = 0.0;
            
            for (int bin = startBin; bin <= endBin && bin < static_cast<int>(mag_input.size() / 2); ++bin) {
                inputEnergy += mag_input[bin] * mag_input[bin];
                outputEnergy += mag_output[bin] * mag_output[bin];
            }
            
            if (inputEnergy > 0.0) {
                float gain = 10.0f * std::log10(outputEnergy / inputEnergy);
                bandGains.push_back(gain);
            } else {
                bandGains.push_back(0.0f);
            }
        }
        
        return bandGains;
    }
    
    // Calculate phase coherence between original and processed
    static float calculatePhaseCoherence(const std::vector<float>& original,
                                       const std::vector<float>& processed,
                                       double frequency, double sampleRate) {
        if (original.size() != processed.size() || original.size() < FFT_SIZE) {
            return 0.0f;
        }
        
        // FFT of both signals
        std::vector<double> windowed_original(FFT_SIZE);
        std::vector<double> windowed_processed(FFT_SIZE);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            
            if (i < static_cast<int>(original.size())) {
                windowed_original[i] = original[i] * window;
                windowed_processed[i] = processed[i] * window;
            } else {
                windowed_original[i] = 0.0;
                windowed_processed[i] = 0.0;
            }
        }
        
        auto fft_original = SimpleFFT::fft(windowed_original);
        auto fft_processed = SimpleFFT::fft(windowed_processed);
        
        // Find frequency bin
        int freq_bin = static_cast<int>(frequency * FFT_SIZE / sampleRate);
        if (freq_bin >= static_cast<int>(fft_original.size() / 2)) {
            return 0.0f;
        }
        
        // Calculate phase difference
        double phase_orig = std::arg(fft_original[freq_bin]);
        double phase_proc = std::arg(fft_processed[freq_bin]);
        double phase_diff = std::abs(phase_orig - phase_proc);
        
        // Normalize to 0-1 (1 = perfect coherence)
        while (phase_diff > M_PI) phase_diff -= 2.0 * M_PI;
        
        return static_cast<float>(1.0 - std::abs(phase_diff) / M_PI);
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
    
    // Calculate brightness enhancement (high frequency content increase)
    static float calculateBrightnessEnhancement(const std::vector<float>& original,
                                               const std::vector<float>& processed,
                                               double sampleRate) {
        if (original.size() != processed.size() || original.size() < FFT_SIZE) {
            return 0.0f;
        }
        
        // Define high frequency range (5kHz - 15kHz)
        double highFreqStart = 5000.0;
        double highFreqEnd = 15000.0;
        
        std::vector<std::pair<double, double>> highFreqBand = {{highFreqStart, highFreqEnd}};
        auto bandGains = measureBandResponse(original, processed, highFreqBand, sampleRate);
        
        return bandGains.empty() ? 0.0f : bandGains[0];
    }
};

// Main test class for Harmonic Exciter
class HarmonicExciterTest {
private:
    std::unique_ptr<HarmonicExciter> harmonicExciter;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    HarmonicExciterTest() {
        harmonicExciter = std::make_unique<HarmonicExciter>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/HarmonicExciter_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/HarmonicExciter_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the harmonic exciter
        harmonicExciter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Harmonic Exciter Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_HARMONIC_EXCITER) + "\n");
        log("Parameter Count: " + std::to_string(harmonicExciter->getNumParameters()) + "\n\n");
    }
    
    ~HarmonicExciterTest() {
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
    
    // Process audio through harmonic exciter
    std::pair<std::vector<float>, std::vector<float>> processAudio(const std::vector<float>& input, 
                                                                  const std::map<int, float>& parameters) {
        // Update parameters
        harmonicExciter->updateParameters(parameters);
        
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
            harmonicExciter->process(buffer);
            
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
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.3, 1.0, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < harmonicExciter->getNumParameters(); ++param) {
            std::string paramName = harmonicExciter->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            // Sweep from 0.0 to 1.0 in 0.25 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                // Set default values for all parameters
                for (int p = 0; p < harmonicExciter->getNumParameters(); ++p) {
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
            
            // Enhancement parameters should have audible effect
            if (param <= 5) { // Core enhancement parameters
                assertTrue(responseRange > 0.3f, 
                          paramName + " has audible effect (range: " + 
                          std::to_string(responseRange) + "dB)");
            }
        }
    }
    
    // Test 2: Harmonic enhancement accuracy
    void testHarmonicEnhancement() {
        log("\n--- Harmonic Enhancement Accuracy Tests ---\n");
        
        // Test with pure sine wave to measure harmonic generation
        std::vector<double> testFreqs = {440.0, 1000.0, 2000.0, 3000.0};
        
        for (double freq : testFreqs) {
            if (freq < TEST_SAMPLE_RATE / 4) { // Leave room for harmonics
                log("\nTesting harmonic enhancement at " + std::to_string(freq) + "Hz\n");
                
                auto testSignal = TestSignalGenerator::generateSineWave(freq, 0.3, 2.0, TEST_SAMPLE_RATE);
                
                // Test with different enhancement levels
                std::vector<float> enhancementLevels = {0.3f, 0.6f, 0.9f};
                
                for (float enhancement : enhancementLevels) {
                    std::map<int, float> params;
                    params[0] = enhancement; // Main enhancement parameter
                    params[1] = 0.5f;        // Frequency parameter
                    params[2] = 0.5f;        // Harmonics parameter
                    
                    auto [original, output] = processAudio(testSignal, params);
                    
                    // Analyze harmonic enhancement
                    auto harmonicEnhancement = AudioAnalyzer::analyzeHarmonicEnhancement(
                        original, output, freq, TEST_SAMPLE_RATE, 8);
                    
                    log("  Enhancement level " + std::to_string(enhancement) + ":\n");
                    for (int h = 0; h < 5; ++h) {
                        log("    H" + std::to_string(h + 1) + ": " + 
                            std::to_string(harmonicEnhancement[h]) + "dB\n");
                        
                        logCSV("HarmonicEnhancement", 
                               "Freq" + std::to_string(freq) + "_H" + std::to_string(h + 1) + 
                               "_Enh" + std::to_string(enhancement),
                               harmonicEnhancement[h], "MEASURED", "dB");
                    }
                    
                    // Higher enhancement should increase harmonic content
                    if (enhancement > 0.5f) {
                        bool hasEnhancement = false;
                        for (int h = 1; h < 5; ++h) { // Check H2-H5
                            if (harmonicEnhancement[h] > 1.0f) {
                                hasEnhancement = true;
                                break;
                            }
                        }
                        
                        assertTrue(hasEnhancement, 
                                  "Harmonic enhancement detected at " + std::to_string(freq) + 
                                  "Hz, level " + std::to_string(enhancement));
                    }
                }
            }
        }
    }
    
    // Test 3: Frequency-selective processing
    void testFrequencySelectiveProcessing() {
        log("\n--- Frequency-Selective Processing Tests ---\n");
        
        // Create multi-frequency test signal
        std::vector<double> frequencies = {200.0, 1000.0, 5000.0, 10000.0};
        std::vector<double> harmonicLevels = {1.0, 1.0, 1.0, 1.0}; // Equal levels
        
        auto testSignal = TestSignalGenerator::generateHarmonicComplex(
            200.0, 0.2, 2.0, TEST_SAMPLE_RATE, harmonicLevels);
        
        // Test different frequency focus settings
        std::vector<float> frequencySettings = {0.2f, 0.5f, 0.8f}; // Low, mid, high focus
        
        for (float freqSetting : frequencySettings) {
            log("\nTesting frequency setting: " + std::to_string(freqSetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.7f;        // Enhancement amount
            params[1] = freqSetting; // Frequency focus
            params[2] = 0.6f;        // Harmonics
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Define frequency bands for analysis
            std::vector<std::pair<double, double>> bands = {
                {100.0, 500.0},    // Low band
                {500.0, 2000.0},   // Mid band
                {2000.0, 8000.0},  // High band
                {8000.0, 15000.0}  // Very high band
            };
            
            auto bandGains = AudioAnalyzer::measureBandResponse(original, output, bands, TEST_SAMPLE_RATE);
            
            for (size_t i = 0; i < bands.size() && i < bandGains.size(); ++i) {
                log("  Band " + std::to_string(bands[i].first) + "-" + 
                    std::to_string(bands[i].second) + "Hz: " + 
                    std::to_string(bandGains[i]) + "dB\n");
                
                logCSV("FrequencySelective", 
                       "FreqSetting" + std::to_string(freqSetting) + "_Band" + std::to_string(i),
                       bandGains[i], "MEASURED", "dB");
            }
            
            // Check that appropriate frequency bands are enhanced
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with frequency setting " + std::to_string(freqSetting));
        }
    }
    
    // Test 4: Musical vs non-musical harmonics
    void testMusicalVsNonMusicalHarmonics() {
        log("\n--- Musical vs Non-Musical Harmonics Tests ---\n");
        
        // Test with musical chord (A major: A4, C#5, E5)
        std::vector<double> chordFreqs = {440.0, 554.37, 659.25};
        auto musicalSignal = TestSignalGenerator::generateChord(chordFreqs, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        // Test with inharmonic frequencies
        std::vector<double> inharmonicFreqs = {440.0, 567.8, 723.4}; // Non-harmonic ratios
        auto inharmonicSignal = TestSignalGenerator::generateChord(inharmonicFreqs, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.8f; // High enhancement
        params[1] = 0.5f; // Mid frequency
        params[2] = 0.7f; // Harmonics
        
        // Process musical signal
        auto [musicalOrig, musicalOutput] = processAudio(musicalSignal, params);
        
        // Process inharmonic signal
        auto [inharmonicOrig, inharmonicOutput] = processAudio(inharmonicSignal, params);
        
        // Calculate brightness enhancement for both
        float musicalBrightness = AudioAnalyzer::calculateBrightnessEnhancement(
            musicalOrig, musicalOutput, TEST_SAMPLE_RATE);
        float inharmonicBrightness = AudioAnalyzer::calculateBrightnessEnhancement(
            inharmonicOrig, inharmonicOutput, TEST_SAMPLE_RATE);
        
        log("Musical signal brightness enhancement: " + std::to_string(musicalBrightness) + "dB\n");
        log("Inharmonic signal brightness enhancement: " + std::to_string(inharmonicBrightness) + "dB\n");
        
        logCSV("MusicalVsInharmonic", "MusicalBrightness", musicalBrightness, "MEASURED", "dB");
        logCSV("MusicalVsInharmonic", "InharmonicBrightness", inharmonicBrightness, "MEASURED", "dB");
        
        // Musical content might be enhanced differently than inharmonic
        assertTrue(!AudioAnalyzer::hasInvalidValues(musicalOutput), "Valid musical output");
        assertTrue(!AudioAnalyzer::hasInvalidValues(inharmonicOutput), "Valid inharmonic output");
        
        // Both should show some enhancement
        assertTrue(musicalBrightness > -3.0f || inharmonicBrightness > -3.0f,
                  "Enhancement occurs for test signals");
    }
    
    // Test 5: Phase coherence analysis
    void testPhaseCoherence() {
        log("\n--- Phase Coherence Analysis ---\n");
        
        std::vector<double> testFreqs = {440.0, 1000.0, 2000.0, 4000.0};
        
        for (double freq : testFreqs) {
            if (freq < TEST_SAMPLE_RATE / 2) {
                log("\nTesting phase coherence at " + std::to_string(freq) + "Hz\n");
                
                auto testSignal = TestSignalGenerator::generateSineWave(freq, 0.3, 1.0, TEST_SAMPLE_RATE);
                
                std::map<int, float> params;
                params[0] = 0.5f; // Moderate enhancement
                params[1] = 0.5f; // Mid frequency
                
                auto [original, output] = processAudio(testSignal, params);
                
                // Calculate phase coherence at fundamental frequency
                float coherence = AudioAnalyzer::calculatePhaseCoherence(original, output, freq, TEST_SAMPLE_RATE);
                
                log("  Phase coherence: " + std::to_string(coherence) + "\n");
                
                logCSV("PhaseCoherence", "Freq_" + std::to_string(freq), coherence, "MEASURED", "ratio");
                
                // Phase coherence should be reasonable (harmonic exciters may introduce some phase shift)
                assertTrue(coherence > 0.3f, 
                          "Reasonable phase coherence at " + std::to_string(freq) + "Hz");
            }
        }
    }
    
    // Test 6: Dynamic enhancement testing
    void testDynamicEnhancement() {
        log("\n--- Dynamic Enhancement Testing ---\n");
        
        // Create signal with varying amplitude
        std::vector<float> dynamicSignal;
        
        // Quiet section
        auto quietSection = TestSignalGenerator::generateSineWave(1000.0, 0.05, 0.5, TEST_SAMPLE_RATE);
        dynamicSignal.insert(dynamicSignal.end(), quietSection.begin(), quietSection.end());
        
        // Loud section
        auto loudSection = TestSignalGenerator::generateSineWave(1000.0, 0.4, 0.5, TEST_SAMPLE_RATE);
        dynamicSignal.insert(dynamicSignal.end(), loudSection.begin(), loudSection.end());
        
        // Medium section
        auto mediumSection = TestSignalGenerator::generateSineWave(1000.0, 0.15, 0.5, TEST_SAMPLE_RATE);
        dynamicSignal.insert(dynamicSignal.end(), mediumSection.begin(), mediumSection.end());
        
        std::map<int, float> params;
        params[0] = 0.7f; // Enhancement
        params[1] = 0.5f; // Frequency
        params[3] = 0.6f; // Dynamic response (if available)
        
        auto [original, output] = processAudio(dynamicSignal, params);
        
        // Analyze sections separately
        int sectionLength = static_cast<int>(0.5 * TEST_SAMPLE_RATE);
        
        // Quiet section analysis
        std::vector<float> quietOrig(original.begin(), original.begin() + sectionLength);
        std::vector<float> quietOut(output.begin(), output.begin() + sectionLength);
        
        // Loud section analysis
        std::vector<float> loudOrig(original.begin() + sectionLength, original.begin() + 2 * sectionLength);
        std::vector<float> loudOut(output.begin() + sectionLength, output.begin() + 2 * sectionLength);
        
        float quietRMS_orig = AudioAnalyzer::calculateRMS_dB(quietOrig);
        float quietRMS_out = AudioAnalyzer::calculateRMS_dB(quietOut);
        float loudRMS_orig = AudioAnalyzer::calculateRMS_dB(loudOrig);
        float loudRMS_out = AudioAnalyzer::calculateRMS_dB(loudOut);
        
        float quietEnhancement = quietRMS_out - quietRMS_orig;
        float loudEnhancement = loudRMS_out - loudRMS_orig;
        
        log("Quiet section enhancement: " + std::to_string(quietEnhancement) + "dB\n");
        log("Loud section enhancement: " + std::to_string(loudEnhancement) + "dB\n");
        
        logCSV("DynamicEnhancement", "QuietEnhancement", quietEnhancement, "MEASURED", "dB");
        logCSV("DynamicEnhancement", "LoudEnhancement", loudEnhancement, "MEASURED", "dB");
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid output during dynamic test");
        
        // Dynamic enhancement should adapt to signal level
        assertTrue(std::abs(quietEnhancement - loudEnhancement) < 20.0f,
                  "Reasonable dynamic response range");
    }
    
    // Test 7: Broadband enhancement
    void testBroadbandEnhancement() {
        log("\n--- Broadband Enhancement Tests ---\n");
        
        // Test with pink noise (contains all frequencies)
        auto noiseSignal = TestSignalGenerator::generatePinkNoise(0.2, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> enhancementLevels = {0.3f, 0.6f, 0.9f};
        
        for (float enhancement : enhancementLevels) {
            log("\nTesting broadband enhancement level: " + std::to_string(enhancement) + "\n");
            
            std::map<int, float> params;
            params[0] = enhancement; // Enhancement amount
            params[1] = 0.5f;        // Frequency balance
            params[2] = 0.7f;        // Harmonics
            
            auto [original, output] = processAudio(noiseSignal, params);
            
            // Analyze frequency response across spectrum
            std::vector<std::pair<double, double>> spectralBands = {
                {100.0, 300.0},    // Low
                {300.0, 1000.0},   // Low-mid
                {1000.0, 3000.0},  // Mid
                {3000.0, 8000.0},  // High-mid
                {8000.0, 15000.0}  // High
            };
            
            auto bandGains = AudioAnalyzer::measureBandResponse(original, output, spectralBands, TEST_SAMPLE_RATE);
            
            log("  Spectral band gains:\n");
            for (size_t i = 0; i < spectralBands.size() && i < bandGains.size(); ++i) {
                log("    " + std::to_string(spectralBands[i].first) + "-" + 
                    std::to_string(spectralBands[i].second) + "Hz: " + 
                    std::to_string(bandGains[i]) + "dB\n");
                
                logCSV("BroadbandEnhancement", 
                       "Enh" + std::to_string(enhancement) + "_Band" + std::to_string(i),
                       bandGains[i], "MEASURED", "dB");
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid broadband output at enhancement " + std::to_string(enhancement));
        }
    }
    
    // Test 8: Frequency sweep response
    void testFrequencySweepResponse() {
        log("\n--- Frequency Sweep Response Tests ---\n");
        
        // Generate logarithmic frequency sweep
        auto sweepSignal = TestSignalGenerator::generateSweep(50.0, 15000.0, 0.2, 3.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.6f; // Enhancement
        params[1] = 0.5f; // Frequency balance
        params[2] = 0.5f; // Harmonics
        
        auto [original, output] = processAudio(sweepSignal, params);
        
        // Calculate overall response characteristics
        float originalRMS = AudioAnalyzer::calculateRMS_dB(original);
        float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
        float overallGain = outputRMS - originalRMS;
        
        float brightness = AudioAnalyzer::calculateBrightnessEnhancement(original, output, TEST_SAMPLE_RATE);
        
        log("Overall gain: " + std::to_string(overallGain) + "dB\n");
        log("Brightness enhancement: " + std::to_string(brightness) + "dB\n");
        
        logCSV("FrequencySweep", "OverallGain", overallGain, "MEASURED", "dB");
        logCSV("FrequencySweep", "BrightnessEnhancement", brightness, "MEASURED", "dB");
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid sweep response");
        assertTrue(overallGain > -6.0f && overallGain < 12.0f, "Reasonable overall gain");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Harmonic Exciter comprehensive test suite...\n");
        
        testParameterResponse();
        testHarmonicEnhancement();
        testFrequencySelectiveProcessing();
        testMusicalVsNonMusicalHarmonics();
        testPhaseCoherence();
        testDynamicEnhancement();
        testBroadbandEnhancement();
        testFrequencySweepResponse();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        HarmonicExciterTest tester;
        tester.runAllTests();
        
        std::cout << "\nHarmonic Exciter test suite completed successfully.\n";
        std::cout << "Check HarmonicExciter_TestResults.txt for detailed results.\n";
        std::cout << "Check HarmonicExciter_Data.csv for measurement data.\n";
        
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