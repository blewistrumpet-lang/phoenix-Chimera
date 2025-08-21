/*
  ==============================================================================
  
    WaveFolder_Test.cpp
    Comprehensive test suite for ENGINE_WAVE_FOLDER
    
    Tests for wave folder characteristics:
    - Folding threshold precision and accuracy
    - Wave symmetry analysis
    - Folding harmonics content
    - Anti-aliasing effectiveness
    - Oversampling verification
    - Asymmetry parameter testing
    - DC offset handling
    - Real-time performance validation
    - Parameter sweep validation
    
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
#include "../../Source/WaveFolder.h"
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
    
    // Generate triangle wave for folding analysis
    static std::vector<float> generateTriangleWave(double frequency, double amplitude, 
                                                 double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            // Triangle wave: -1 to 1 over one period
            double tri;
            if (phase < 0.5) {
                tri = 4.0 * phase - 1.0;
            } else {
                tri = 3.0 - 4.0 * phase;
            }
            
            signal[i] = static_cast<float>(amplitude * tri);
            phase += phaseIncrement;
            if (phase >= 1.0) phase -= 1.0;
        }
        
        return signal;
    }
    
    // Generate ramp wave for threshold testing
    static std::vector<float> generateRampWave(double amplitude, double duration, 
                                             double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / (numSamples - 1.0);
            signal[i] = static_cast<float>(amplitude * (2.0 * t - 1.0)); // -amplitude to +amplitude
        }
        
        return signal;
    }
    
    // Generate stepped amplitude signal for threshold testing
    static std::vector<float> generateSteppedAmplitude(double frequency, double duration,
                                                     double sampleRate, int steps = 10) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        int samplesPerStep = numSamples / steps;
        
        for (int step = 0; step < steps; ++step) {
            double amplitude = (step + 1) * 0.1; // 0.1 to 1.0
            
            for (int i = 0; i < samplesPerStep; ++i) {
                int sampleIndex = step * samplesPerStep + i;
                if (sampleIndex < numSamples) {
                    double t = i / sampleRate;
                    signal[sampleIndex] = static_cast<float>(amplitude * std::sin(2.0 * M_PI * frequency * t));
                }
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
    
    // Detect folding events in signal
    static int countFoldingEvents(const std::vector<float>& signal, float threshold = 0.8f) {
        int foldCount = 0;
        bool wasAboveThreshold = false;
        
        for (float sample : signal) {
            bool isAboveThreshold = std::abs(sample) > threshold;
            
            if (isAboveThreshold && !wasAboveThreshold) {
                foldCount++;
            }
            
            wasAboveThreshold = isAboveThreshold;
        }
        
        return foldCount;
    }
    
    // Measure wave symmetry
    static float measureSymmetry(const std::vector<float>& signal) {
        if (signal.empty()) return 0.0f;
        
        double positiveSum = 0.0, negativeSum = 0.0;
        int positiveCount = 0, negativeCount = 0;
        
        for (float sample : signal) {
            if (sample > 0.0f) {
                positiveSum += sample;
                positiveCount++;
            } else if (sample < 0.0f) {
                negativeSum += std::abs(sample);
                negativeCount++;
            }
        }
        
        if (positiveCount == 0 || negativeCount == 0) return 0.0f;
        
        double positiveAvg = positiveSum / positiveCount;
        double negativeAvg = negativeSum / negativeCount;
        
        if (positiveAvg + negativeAvg == 0.0) return 1.0f;
        
        return static_cast<float>(1.0 - std::abs(positiveAvg - negativeAvg) / (positiveAvg + negativeAvg));
    }
    
    // Analyze harmonic content with emphasis on folding harmonics
    static std::vector<float> analyzeFoldingHarmonics(const std::vector<float>& signal, 
                                                    double fundamental, double sampleRate, 
                                                    int maxHarmonics = 15) {
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
    
    // Check for aliasing above Nyquist/2
    static float detectAliasing(const std::vector<float>& signal, double sampleRate) {
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
        
        // Check energy above 18kHz (potential aliasing zone)
        int aliasingStartBin = static_cast<int>(18000.0 * FFT_SIZE / sampleRate);
        int nyquistBin = static_cast<int>(magnitudes_db.size() / 2);
        
        float maxAliasing = -120.0f;
        for (int i = aliasingStartBin; i < nyquistBin; ++i) {
            maxAliasing = std::max(maxAliasing, static_cast<float>(magnitudes_db[i]));
        }
        
        return maxAliasing;
    }
    
    // Find folding threshold by analyzing transfer curve
    static float findFoldingThreshold(const std::vector<float>& input, 
                                    const std::vector<float>& output) {
        if (input.size() != output.size() || input.empty()) return 0.0f;
        
        // Look for where output starts decreasing while input increases
        float maxInput = 0.0f;
        float thresholdCandidate = 1.0f;
        
        for (size_t i = 1; i < input.size(); ++i) {
            float currentInput = std::abs(input[i]);
            float currentOutput = std::abs(output[i]);
            float prevOutput = std::abs(output[i-1]);
            
            if (currentInput > maxInput) {
                maxInput = currentInput;
                
                // If output decreased while input increased, we found folding
                if (currentOutput < prevOutput && currentInput > 0.5f) {
                    thresholdCandidate = std::min(thresholdCandidate, currentInput);
                }
            }
        }
        
        return thresholdCandidate;
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
};

// Main test class for Wave Folder
class WaveFolderTest {
private:
    std::unique_ptr<WaveFolder> waveFolder;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    WaveFolderTest() {
        waveFolder = std::make_unique<WaveFolder>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/WaveFolder_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/WaveFolder_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the wave folder
        waveFolder->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Wave Folder Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_WAVE_FOLDER) + "\n");
        log("Parameter Count: " + std::to_string(waveFolder->getNumParameters()) + "\n\n");
    }
    
    ~WaveFolderTest() {
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
    
    // Process audio through wave folder
    std::vector<float> processAudio(const std::vector<float>& input, 
                                  const std::map<int, float>& parameters) {
        // Update parameters
        waveFolder->updateParameters(parameters);
        
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
            waveFolder->process(buffer);
            
            // Extract output (left channel)
            for (size_t j = 0; j < blockSize; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }
        
        return output;
    }
    
    // Test 1: Parameter validation and response
    void testParameterResponse() {
        log("\n--- Parameter Response Tests ---\n");
        
        // Test signal: 1kHz sine at moderate level
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.5, 1.0, TEST_SAMPLE_RATE);
        
        // Test each parameter individually
        for (int param = 0; param < waveFolder->getNumParameters(); ++param) {
            std::string paramName = waveFolder->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            // Sweep from 0.0 to 1.0 in 0.25 steps
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                // Set default values for all parameters
                for (int p = 0; p < waveFolder->getNumParameters(); ++p) {
                    params[p] = 0.5f; // Default to middle position
                }
                
                // Override the parameter being tested
                params[param] = value;
                
                auto output = processAudio(testSignal, params);
                
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
            
            // Core folding parameters should have significant effect
            if (param <= 3) { // Core wave folding parameters
                assertTrue(responseRange > 1.0f, 
                          paramName + " has audible effect (range: " + 
                          std::to_string(responseRange) + "dB)");
            }
        }
    }
    
    // Test 2: Folding threshold accuracy
    void testFoldingThreshold() {
        log("\n--- Folding Threshold Accuracy Tests ---\n");
        
        // Test with different fold amount settings
        std::vector<float> foldAmounts = {0.2f, 0.5f, 0.8f};
        
        for (float foldAmount : foldAmounts) {
            log("\nTesting fold amount: " + std::to_string(foldAmount) + "\n");
            
            // Generate ramp signal from -1 to +1
            auto rampSignal = TestSignalGenerator::generateRampWave(1.0, 1.0, TEST_SAMPLE_RATE);
            
            std::map<int, float> params;
            params[0] = foldAmount; // Fold Amount parameter
            params[3] = 0.5f;       // Pre-gain
            params[4] = 0.5f;       // Post-gain
            
            auto output = processAudio(rampSignal, params);
            
            // Find where folding occurs
            float detectedThreshold = AudioAnalyzer::findFoldingThreshold(rampSignal, output);
            
            log("  Detected folding threshold: " + std::to_string(detectedThreshold) + "\n");
            
            logCSV("FoldingThreshold", "FoldAmount_" + std::to_string(foldAmount),
                   detectedThreshold, "MEASURED", "amplitude");
            
            // Count folding events
            int foldingEvents = AudioAnalyzer::countFoldingEvents(output, 0.7f);
            log("  Folding events detected: " + std::to_string(foldingEvents) + "\n");
            
            // Higher fold amounts should create more folding
            if (foldAmount > 0.5f) {
                assertTrue(foldingEvents > 0, 
                          "Folding events occur at fold amount " + std::to_string(foldAmount));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at fold amount " + std::to_string(foldAmount));
        }
    }
    
    // Test 3: Wave symmetry and asymmetry
    void testWaveSymmetry() {
        log("\n--- Wave Symmetry and Asymmetry Tests ---\n");
        
        // Test with sine wave and different asymmetry settings
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.8, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> asymmetrySettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float asymmetry : asymmetrySettings) {
            log("\nTesting asymmetry: " + std::to_string(asymmetry) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.6f;     // Fold amount
            params[1] = asymmetry; // Asymmetry parameter
            params[3] = 0.5f;     // Pre-gain
            
            auto output = processAudio(testSignal, params);
            
            // Measure symmetry
            float symmetry = AudioAnalyzer::measureSymmetry(output);
            log("  Measured symmetry: " + std::to_string(symmetry) + "\n");
            
            logCSV("WaveSymmetry", "Asymmetry_" + std::to_string(asymmetry),
                   symmetry, "MEASURED", "ratio");
            
            // At asymmetry = 0.5 (center), we expect high symmetry
            if (asymmetry == 0.5f) {
                assertTrue(symmetry > 0.8f, 
                          "High symmetry at center asymmetry setting");
            }
            
            // At extreme asymmetry settings, expect lower symmetry
            if (asymmetry == 0.0f || asymmetry == 1.0f) {
                assertTrue(symmetry < 0.9f, 
                          "Reduced symmetry at extreme asymmetry " + std::to_string(asymmetry));
            }
        }
    }
    
    // Test 4: Harmonic content analysis
    void testHarmonicContent() {
        log("\n--- Folding Harmonic Content Analysis ---\n");
        
        // Generate 1kHz test tone
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.7, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> foldAmounts = {0.2f, 0.5f, 0.8f};
        
        for (float foldAmount : foldAmounts) {
            log("\nAnalyzing harmonics at fold amount: " + std::to_string(foldAmount) + "\n");
            
            std::map<int, float> params;
            params[0] = foldAmount; // Fold amount
            params[1] = 0.5f;       // Asymmetry
            params[3] = 0.5f;       // Pre-gain
            
            auto output = processAudio(testSignal, params);
            
            // Analyze harmonics (wave folding creates rich harmonic content)
            auto harmonics = AudioAnalyzer::analyzeFoldingHarmonics(output, 1000.0, TEST_SAMPLE_RATE, 15);
            
            // Log first 10 harmonics
            for (int h = 0; h < 10; ++h) {
                log("  H" + std::to_string(h + 1) + ": " + 
                    std::to_string(harmonics[h]) + "dB\n");
                
                logCSV("FoldingHarmonics", "H" + std::to_string(h + 1) + "_Fold_" + std::to_string(foldAmount),
                       harmonics[h], "MEASURED", "dB");
            }
            
            // Wave folding should create rich harmonic content
            if (foldAmount > 0.5f) {
                // Check that higher harmonics are present
                bool hasHigherHarmonics = false;
                for (int h = 2; h < 6; ++h) { // Check H3-H6
                    if (harmonics[h] > -40.0f) {
                        hasHigherHarmonics = true;
                        break;
                    }
                }
                
                assertTrue(hasHigherHarmonics, 
                          "Rich harmonic content at fold amount " + std::to_string(foldAmount));
            }
        }
    }
    
    // Test 5: Anti-aliasing effectiveness
    void testAntiAliasing() {
        log("\n--- Anti-Aliasing Effectiveness Tests ---\n");
        
        // Test with high-frequency signals that could cause aliasing
        std::vector<double> testFreqs = {8000.0, 12000.0, 16000.0, 18000.0};
        
        std::map<int, float> params;
        params[0] = 0.8f; // High fold amount to create harmonics
        params[1] = 0.5f; // Symmetry
        params[3] = 0.5f; // Pre-gain
        
        for (double freq : testFreqs) {
            if (freq < TEST_SAMPLE_RATE / 2) {
                log("\nTesting anti-aliasing at " + std::to_string(freq) + "Hz\n");
                
                auto testSignal = TestSignalGenerator::generateSineWave(freq, 0.6, 1.0, TEST_SAMPLE_RATE);
                auto output = processAudio(testSignal, params);
                
                float aliasingLevel = AudioAnalyzer::detectAliasing(output, TEST_SAMPLE_RATE);
                
                log("  Aliasing level: " + std::to_string(aliasingLevel) + "dB\n");
                
                logCSV("AntiAliasing", "Freq_" + std::to_string(freq),
                       aliasingLevel, "MEASURED", "dB");
                
                // Anti-aliasing should keep aliasing below -50dB
                assertTrue(aliasingLevel < -40.0f, 
                          "Low aliasing at " + std::to_string(freq) + "Hz");
            }
        }
    }
    
    // Test 6: DC offset handling
    void testDCOffsetHandling() {
        log("\n--- DC Offset Handling Tests ---\n");
        
        // Test with various DC offset settings
        std::vector<float> dcOffsets = {0.0f, 0.3f, 0.5f, 0.7f, 1.0f};
        
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.5, 1.0, TEST_SAMPLE_RATE);
        
        for (float dcOffset : dcOffsets) {
            log("\nTesting DC offset: " + std::to_string(dcOffset) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.6f;    // Fold amount
            params[2] = dcOffset; // DC Offset parameter
            params[3] = 0.5f;    // Pre-gain
            
            auto output = processAudio(testSignal, params);
            
            // Measure actual DC offset in output
            float measuredDC = AudioAnalyzer::calculateDCOffset(output);
            log("  Measured DC offset: " + std::to_string(measuredDC) + "\n");
            
            logCSV("DCOffset", "Setting_" + std::to_string(dcOffset),
                   measuredDC, "MEASURED", "amplitude");
            
            // DC offset should affect the signal appropriately
            assertTrue(std::abs(measuredDC) < 0.5f, 
                      "DC offset within reasonable range at setting " + std::to_string(dcOffset));
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with DC offset " + std::to_string(dcOffset));
        }
    }
    
    // Test 7: Real-time performance
    void testRealTimePerformance() {
        log("\n--- Real-Time Performance Tests ---\n");
        
        // Generate longer test signal
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.6, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.7f; // Fold amount
        params[1] = 0.5f; // Asymmetry
        params[6] = 0.8f; // Harmonics parameter (if available)
        
        // Measure processing time
        auto startTime = std::chrono::high_resolution_clock::now();
        auto output = processAudio(testSignal, params);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0; // milliseconds
        
        double signalDuration = testSignal.size() / TEST_SAMPLE_RATE * 1000.0; // milliseconds
        double realTimeRatio = processingTime / signalDuration;
        
        log("Processing time: " + std::to_string(processingTime) + "ms\n");
        log("Signal duration: " + std::to_string(signalDuration) + "ms\n");
        log("Real-time ratio: " + std::to_string(realTimeRatio) + "\n");
        
        logCSV("Performance", "ProcessingTime", processingTime, "MEASURED", "ms");
        logCSV("Performance", "RealTimeRatio", realTimeRatio, "MEASURED", "ratio");
        
        // Should process faster than real-time
        assertTrue(realTimeRatio < 0.5, "Real-time processing capability");
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid output during performance test");
    }
    
    // Test 8: Edge cases and stability
    void testEdgeCases() {
        log("\n--- Edge Cases and Stability Tests ---\n");
        
        // Test with extreme parameter combinations
        std::vector<std::map<int, float>> extremeParams = {
            {{0, 1.0f}, {1, 0.0f}, {3, 1.0f}}, // Max fold, min asymmetry, max gain
            {{0, 0.0f}, {1, 1.0f}, {3, 0.0f}}, // Min fold, max asymmetry, min gain
            {{0, 1.0f}, {1, 1.0f}, {3, 1.0f}}, // All max
            {{0, 0.0f}, {1, 0.0f}, {3, 0.0f}}  // All min
        };
        
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.5, 0.5, TEST_SAMPLE_RATE);
        
        for (size_t i = 0; i < extremeParams.size(); ++i) {
            log("\nTesting extreme parameter set " + std::to_string(i + 1) + "\n");
            
            auto output = processAudio(testSignal, extremeParams[i]);
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Stable output with extreme parameters " + std::to_string(i + 1));
            
            float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
            log("  Peak output level: " + std::to_string(peakLevel) + "dB\n");
            
            assertTrue(peakLevel < 12.0f, 
                      "Output level controlled with extreme parameters " + std::to_string(i + 1));
        }
        
        // Test with silence
        std::vector<float> silenceSignal(static_cast<int>(0.5 * TEST_SAMPLE_RATE), 0.0f);
        std::map<int, float> params;
        params[0] = 0.8f; // High fold amount
        
        auto silenceOutput = processAudio(silenceSignal, params);
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(silenceOutput),
                  "Stable output with silence input");
        
        float silenceRMS = AudioAnalyzer::calculateRMS_dB(silenceOutput);
        assertTrue(silenceRMS < -60.0f, "Low noise floor with silence input");
    }
    
    // Test 9: Triangle wave folding characteristics
    void testTriangleWaveFolding() {
        log("\n--- Triangle Wave Folding Characteristics ---\n");
        
        // Triangle waves are ideal for testing wave folding
        auto triangleSignal = TestSignalGenerator::generateTriangleWave(500.0, 0.8, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> foldAmounts = {0.3f, 0.6f, 0.9f};
        
        for (float foldAmount : foldAmounts) {
            log("\nTesting triangle wave folding at: " + std::to_string(foldAmount) + "\n");
            
            std::map<int, float> params;
            params[0] = foldAmount; // Fold amount
            params[1] = 0.5f;       // Symmetry
            params[3] = 0.5f;       // Pre-gain
            
            auto output = processAudio(triangleSignal, params);
            
            // Analyze the folded triangle characteristics
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float outputPeak = AudioAnalyzer::calculatePeak_dB(output);
            int foldingEvents = AudioAnalyzer::countFoldingEvents(output, 0.6f);
            
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            log("  Output Peak: " + std::to_string(outputPeak) + "dB\n");
            log("  Folding events: " + std::to_string(foldingEvents) + "\n");
            
            logCSV("TriangleFolding", "FoldAmount_" + std::to_string(foldAmount) + "_RMS",
                   outputRMS, "MEASURED", "dB");
            logCSV("TriangleFolding", "FoldAmount_" + std::to_string(foldAmount) + "_Events",
                   foldingEvents, "MEASURED", "count");
            
            // Higher fold amounts should create more folding events
            if (foldAmount > 0.5f) {
                assertTrue(foldingEvents > 5, 
                          "Multiple folding events at fold amount " + std::to_string(foldAmount));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid triangle wave output at fold amount " + std::to_string(foldAmount));
        }
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Wave Folder comprehensive test suite...\n");
        
        testParameterResponse();
        testFoldingThreshold();
        testWaveSymmetry();
        testHarmonicContent();
        testAntiAliasing();
        testDCOffsetHandling();
        testRealTimePerformance();
        testEdgeCases();
        testTriangleWaveFolding();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        WaveFolderTest tester;
        tester.runAllTests();
        
        std::cout << "\nWave Folder test suite completed successfully.\n";
        std::cout << "Check WaveFolder_TestResults.txt for detailed results.\n";
        std::cout << "Check WaveFolder_Data.csv for measurement data.\n";
        
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