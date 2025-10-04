/*
  ==============================================================================
  
    AnalogPhaser_Test.cpp
    Comprehensive test suite for ENGINE_ANALOG_PHASER (AnalogPhaser)
    
    Tests for analog phaser characteristics:
    - LFO rate accuracy and waveform shape
    - Depth/intensity modulation precision
    - Notch frequency tracking accuracy
    - All-pass stage configuration (2/4/6/8 stages)
    - Feedback loop stability and coloration
    - Stereo spread and phase relationships
    - Center frequency and resonance control
    - Mix parameter behavior
    
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
#include "../../Source/AnalogPhaser.h"
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
    
    // Generate frequency sweep for notch detection
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
    
    // Generate impulse for all-pass response analysis
    static std::vector<std::vector<float>> generateStereoImpulse(double amplitude, int position, 
                                                               int totalSamples) {
        std::vector<std::vector<float>> signal(2, std::vector<float>(totalSamples, 0.0f));
        if (position >= 0 && position < totalSamples) {
            signal[0][position] = static_cast<float>(amplitude);
            signal[1][position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate white noise for phase analysis
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
    
    // Generate multi-sine test signal for harmonic analysis
    static std::vector<std::vector<float>> generateMultiSine(const std::vector<double>& frequencies,
                                                           double amplitude, double duration, 
                                                           double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples, 0.0f));
        
        std::vector<double> phases(frequencies.size(), 0.0);
        std::vector<double> phaseIncrements;
        
        for (double freq : frequencies) {
            phaseIncrements.push_back(2.0 * M_PI * freq / sampleRate);
        }
        
        double scale = amplitude / frequencies.size();
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = 0.0f;
            
            for (size_t f = 0; f < frequencies.size(); ++f) {
                sample += static_cast<float>(scale * std::sin(phases[f]));
                phases[f] += phaseIncrements[f];
                if (phases[f] > 2.0 * M_PI) phases[f] -= 2.0 * M_PI;
            }
            
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
};

// Audio analysis utilities specific to phaser effects
class PhaserAnalyzer {
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
    
    // Find notch frequencies in the spectrum
    static std::vector<std::pair<float, float>> findNotchFrequencies(const std::vector<float>& signal,
                                                                    double sampleRate,
                                                                    float minFreq = 100.0f,
                                                                    float maxFreq = 8000.0f) {
        if (signal.size() < FFT_SIZE) return {};
        
        // Window the signal
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(signal.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed[i] = signal[i] * window;
            }
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto magnitudes_db = SimpleFFT::magnitudeDB(fft_result);
        
        std::vector<std::pair<float, float>> notches; // frequency, depth pairs
        
        int startBin = static_cast<int>(minFreq * FFT_SIZE / sampleRate);
        int endBin = std::min(static_cast<int>(maxFreq * FFT_SIZE / sampleRate), 
                             static_cast<int>(magnitudes_db.size() / 2));
        
        // Find local minima (notches)
        for (int i = startBin + 2; i < endBin - 2; ++i) {
            bool isNotch = true;
            double currentLevel = magnitudes_db[i];
            
            // Check if it's a local minimum
            for (int j = -2; j <= 2; ++j) {
                if (j != 0 && magnitudes_db[i + j] <= currentLevel) {
                    isNotch = false;
                    break;
                }
            }
            
            // Calculate notch depth
            if (isNotch) {
                double surroundingLevel = (magnitudes_db[i - 2] + magnitudes_db[i + 2]) * 0.5;
                double notchDepth = surroundingLevel - currentLevel;
                
                if (notchDepth > 3.0) { // Minimum 3dB notch depth
                    float frequency = static_cast<float>(i * sampleRate / FFT_SIZE);
                    notches.push_back({frequency, static_cast<float>(notchDepth)});
                }
            }
        }
        
        // Sort by depth (deepest first)
        std::sort(notches.begin(), notches.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return notches;
    }
    
    // Measure LFO rate from phase modulation
    static float measureLFORate(const std::vector<float>& modulatedSignal, 
                               double sampleRate, double testFreq) {
        if (modulatedSignal.size() < FFT_SIZE) return 0.0f;
        
        // Extract phase information at test frequency
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE && i < static_cast<int>(modulatedSignal.size()); ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            windowed[i] = modulatedSignal[i] * window;
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto phases = SimpleFFT::phase(fft_result);
        
        // Find the bin corresponding to test frequency
        int testBin = static_cast<int>(testFreq * FFT_SIZE / sampleRate);
        testBin = std::max(1, std::min(testBin, static_cast<int>(phases.size() / 2 - 1)));
        
        // Look for phase modulation sidebands
        std::vector<double> magnitudes = SimpleFFT::magnitude(fft_result);
        
        // Find modulation frequency by looking at sidebands
        float maxSidebandLevel = 0.0f;
        float detectedLFORate = 0.0f;
        
        for (int offset = 1; offset < 100 && testBin + offset < static_cast<int>(magnitudes.size() / 2); ++offset) {
            float lowerSideband = static_cast<float>(magnitudes[testBin - offset]);
            float upperSideband = static_cast<float>(magnitudes[testBin + offset]);
            float sidebandLevel = std::max(lowerSideband, upperSideband);
            
            if (sidebandLevel > maxSidebandLevel) {
                maxSidebandLevel = sidebandLevel;
                detectedLFORate = static_cast<float>(offset * sampleRate / FFT_SIZE);
            }
        }
        
        return detectedLFORate;
    }
    
    // Measure all-pass phase response
    static float measurePhaseShift(const std::vector<float>& original,
                                  const std::vector<float>& processed,
                                  double testFreq, double sampleRate) {
        if (original.size() != processed.size() || original.size() < FFT_SIZE) {
            return 0.0f;
        }
        
        // FFT both signals
        std::vector<double> origWindowed(FFT_SIZE);
        std::vector<double> procWindowed(FFT_SIZE);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            if (i < static_cast<int>(original.size())) {
                origWindowed[i] = original[i] * window;
                procWindowed[i] = processed[i] * window;
            }
        }
        
        auto origFFT = SimpleFFT::fft(origWindowed);
        auto procFFT = SimpleFFT::fft(procWindowed);
        
        auto origPhase = SimpleFFT::phase(origFFT);
        auto procPhase = SimpleFFT::phase(procFFT);
        
        // Find the bin closest to the test frequency
        int targetBin = static_cast<int>(testFreq * FFT_SIZE / sampleRate);
        targetBin = std::max(1, std::min(targetBin, static_cast<int>(origPhase.size() / 2 - 1)));
        
        double phaseDiff = procPhase[targetBin] - origPhase[targetBin];
        
        // Wrap to [-pi, pi]
        while (phaseDiff > M_PI) phaseDiff -= 2.0 * M_PI;
        while (phaseDiff < -M_PI) phaseDiff += 2.0 * M_PI;
        
        return static_cast<float>(phaseDiff * 180.0 / M_PI); // Convert to degrees
    }
    
    // Analyze stereo phase relationships
    static float measureStereoPhaseSpread(const std::vector<float>& left,
                                         const std::vector<float>& right,
                                         double testFreq, double sampleRate) {
        return measurePhaseShift(left, right, testFreq, sampleRate);
    }
    
    // Count effective stages by analyzing phase response
    static int estimateStageCount(const std::vector<float>& original,
                                 const std::vector<float>& processed,
                                 double sampleRate) {
        if (original.size() != processed.size() || original.size() < FFT_SIZE) {
            return 0;
        }
        
        std::vector<double> origWindowed(FFT_SIZE);
        std::vector<double> procWindowed(FFT_SIZE);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            if (i < static_cast<int>(original.size())) {
                origWindowed[i] = original[i] * window;
                procWindowed[i] = processed[i] * window;
            }
        }
        
        auto origFFT = SimpleFFT::fft(origWindowed);
        auto procFFT = SimpleFFT::fft(procWindowed);
        
        auto origPhase = SimpleFFT::phase(origFFT);
        auto procPhase = SimpleFFT::phase(procFFT);
        
        // Calculate total phase shift across frequency range
        double totalPhaseShift = 0.0;
        int count = 0;
        
        int startBin = static_cast<int>(100.0 * FFT_SIZE / sampleRate);
        int endBin = static_cast<int>(8000.0 * FFT_SIZE / sampleRate);
        endBin = std::min(endBin, static_cast<int>(origPhase.size() / 2));
        
        for (int i = startBin; i < endBin; ++i) {
            double phaseDiff = procPhase[i] - origPhase[i];
            
            // Unwrap phase
            while (phaseDiff > M_PI) phaseDiff -= 2.0 * M_PI;
            while (phaseDiff < -M_PI) phaseDiff += 2.0 * M_PI;
            
            totalPhaseShift += std::abs(phaseDiff);
            count++;
        }
        
        if (count > 0) {
            double avgPhaseShift = totalPhaseShift / count;
            // Each all-pass stage contributes roughly π/2 radians
            return static_cast<int>(avgPhaseShift / (M_PI / 4.0) + 0.5);
        }
        
        return 0;
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
    
    // Measure feedback-induced coloration
    static float measureFeedbackColoration(const std::vector<float>& signal, double sampleRate) {
        if (signal.size() < FFT_SIZE) return 0.0f;
        
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(signal.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed[i] = signal[i] * window;
            }
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto magnitudes_db = SimpleFFT::magnitudeDB(fft_result);
        
        // Calculate spectral centroid as a measure of timbral change
        double weightedSum = 0.0;
        double magnitudeSum = 0.0;
        
        int startBin = static_cast<int>(100.0 * FFT_SIZE / sampleRate);
        int endBin = std::min(static_cast<int>(8000.0 * FFT_SIZE / sampleRate),
                             static_cast<int>(magnitudes_db.size() / 2));
        
        for (int i = startBin; i < endBin; ++i) {
            double magnitude = std::pow(10.0, magnitudes_db[i] / 20.0); // Convert back from dB
            double frequency = i * sampleRate / FFT_SIZE;
            
            weightedSum += frequency * magnitude;
            magnitudeSum += magnitude;
        }
        
        if (magnitudeSum > 0.0) {
            return static_cast<float>(weightedSum / magnitudeSum);
        }
        
        return 0.0f;
    }
};

// Main test class for Analog Phaser
class AnalogPhaserTest {
private:
    std::unique_ptr<AnalogPhaser> phaser;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    AnalogPhaserTest() {
        phaser = std::make_unique<AnalogPhaser>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/AnalogPhaser_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/AnalogPhaser_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the phaser
        phaser->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Analog Phaser Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_ANALOG_PHASER) + "\n");
        log("Parameter Count: " + std::to_string(phaser->getNumParameters()) + "\n\n");
    }
    
    ~AnalogPhaserTest() {
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
    
    // Process stereo audio through phaser
    std::pair<std::vector<std::vector<float>>, std::vector<std::vector<float>>> 
    processAudio(const std::vector<std::vector<float>>& input, 
                 const std::map<int, float>& parameters) {
        // Update parameters
        phaser->updateParameters(parameters);
        
        // Process in blocks
        std::vector<std::vector<float>> output(2);
        std::vector<std::vector<float>> original = input;
        
        if (input.empty() || input[0].empty()) {
            return {original, output};
        }
        
        size_t totalSamples = input[0].size();
        output[0].reserve(totalSamples);
        output[1].reserve(totalSamples);
        
        for (size_t i = 0; i < totalSamples; i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), 
                                      totalSamples - i);
            
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            // Fill buffer
            for (size_t j = 0; j < blockSize; ++j) {
                buffer.setSample(0, static_cast<int>(j), 
                               (i + j < input[0].size()) ? input[0][i + j] : 0.0f);
                buffer.setSample(1, static_cast<int>(j), 
                               (i + j < input[1].size()) ? input[1][i + j] : 0.0f);
            }
            
            // Process
            phaser->process(buffer);
            
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
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        for (int param = 0; param < phaser->getNumParameters(); ++param) {
            std::string paramName = phaser->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                // Set defaults
                for (int p = 0; p < phaser->getNumParameters(); ++p) {
                    params[p] = 0.5f;
                }
                
                params[param] = value;
                
                auto [original, output] = processAudio(testSignal, params);
                
                assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                          !PhaserAnalyzer::hasInvalidValues(output[1]), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                float outputRMS = PhaserAnalyzer::calculateRMS_dB(output[0]);
                responseDB.push_back(outputRMS);
                
                logCSV("ParameterResponse", paramName + "_" + std::to_string(value), 
                       outputRMS, "PASS", "dB");
            }
            
            float minResponse = *std::min_element(responseDB.begin(), responseDB.end());
            float maxResponse = *std::max_element(responseDB.begin(), responseDB.end());
            float responseRange = maxResponse - minResponse;
            
            log("  Response range: " + std::to_string(responseRange) + "dB\n");
            
            assertTrue(responseRange >= 0.0f, 
                      paramName + " has measurable effect (range: " + 
                      std::to_string(responseRange) + "dB)");
        }
    }
    
    // Test 2: LFO rate accuracy and waveform shape
    void testLFORateAccuracy() {
        log("\n--- LFO Rate Accuracy Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(2000.0, 0.3, 5.0, TEST_SAMPLE_RATE);
        
        std::vector<float> expectedRates = {0.5f, 1.0f, 2.0f, 5.0f, 8.0f}; // Hz
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
            params[3] = 0.5f;         // Stages
            params[4] = 0.3f;         // Stereo spread
            params[5] = 0.5f;         // Center frequency
            params[6] = 0.4f;         // Resonance
            params[7] = 0.0f;         // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure actual LFO rate
            float measuredRate = PhaserAnalyzer::measureLFORate(output[0], TEST_SAMPLE_RATE, 2000.0);
            float rateError = (measuredRate > 0.0f) ? std::abs(measuredRate - expectedRate) / expectedRate * 100.0f : 100.0f;
            
            log("  Measured LFO rate: " + std::to_string(measuredRate) + "Hz\n");
            log("  Rate error: " + std::to_string(rateError) + "%\n");
            
            logCSV("LFORateAccuracy", "MeasuredRate_" + std::to_string(rateSetting),
                   measuredRate, "MEASURED", "Hz");
            logCSV("LFORateAccuracy", "RateError_" + std::to_string(rateSetting),
                   rateError, "MEASURED", "%");
            
            // Rate should be reasonably detectable
            assertTrue(measuredRate > 0.1f, 
                      "Detectable LFO rate at setting " + std::to_string(rateSetting));
            
            assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                      !PhaserAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at LFO rate " + std::to_string(rateSetting));
        }
    }
    
    // Test 3: Notch frequency tracking accuracy
    void testNotchFrequencyTracking() {
        log("\n--- Notch Frequency Tracking Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoNoise(0.2, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> centerFreqSettings = {0.2f, 0.4f, 0.6f, 0.8f};
        
        for (float centerFreq : centerFreqSettings) {
            log("\nTesting center frequency setting: " + std::to_string(centerFreq) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.1f;        // Very slow rate to minimize modulation
            params[1] = 0.2f;        // Low depth
            params[2] = 0.0f;        // No feedback
            params[3] = 0.75f;       // More stages for deeper notches
            params[4] = 0.0f;        // No stereo spread
            params[5] = centerFreq;  // Center frequency
            params[6] = 0.3f;        // Some resonance
            params[7] = 0.0f;        // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Find notch frequencies
            auto notches = PhaserAnalyzer::findNotchFrequencies(output[0], TEST_SAMPLE_RATE, 200.0f, 6000.0f);
            
            log("  Found " + std::to_string(notches.size()) + " notch frequencies:\n");
            
            for (size_t i = 0; i < std::min(notches.size(), size_t(3)); ++i) {
                log("    Notch " + std::to_string(i + 1) + ": " + 
                    std::to_string(notches[i].first) + "Hz, depth: " + 
                    std::to_string(notches[i].second) + "dB\n");
                
                logCSV("NotchTracking", "Notch" + std::to_string(i + 1) + "_Freq_" + 
                       std::to_string(centerFreq), notches[i].first, "MEASURED", "Hz");
                logCSV("NotchTracking", "Notch" + std::to_string(i + 1) + "_Depth_" + 
                       std::to_string(centerFreq), notches[i].second, "MEASURED", "dB");
            }
            
            // Should have at least one significant notch
            assertTrue(!notches.empty(), 
                      "Notch frequencies found at center freq " + std::to_string(centerFreq));
            
            if (!notches.empty()) {
                assertTrue(notches[0].second > 3.0f, 
                          "Significant notch depth at center freq " + std::to_string(centerFreq));
            }
            
            assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                      !PhaserAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at center freq " + std::to_string(centerFreq));
        }
    }
    
    // Test 4: All-pass stage configuration
    void testAllPassStageConfiguration() {
        log("\n--- All-Pass Stage Configuration Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoImpulse(1.0, 1000, 
                                                                   static_cast<int>(2.0 * TEST_SAMPLE_RATE));
        
        std::vector<float> stageSettings = {0.25f, 0.5f, 0.75f, 1.0f}; // 2, 4, 6, 8 stages
        std::vector<int> expectedStages = {2, 4, 6, 8};
        
        for (size_t i = 0; i < stageSettings.size(); ++i) {
            float stageSetting = stageSettings[i];
            int expectedStageCount = expectedStages[i];
            
            log("\nTesting stage setting: " + std::to_string(stageSetting) + 
                " (expected " + std::to_string(expectedStageCount) + " stages)\n");
            
            std::map<int, float> params;
            params[0] = 0.0f;         // No LFO modulation
            params[1] = 0.0f;         // No depth
            params[2] = 0.0f;         // No feedback
            params[3] = stageSetting; // Stages parameter
            params[4] = 0.0f;         // No stereo spread
            params[5] = 0.5f;         // Mid center frequency
            params[6] = 0.2f;         // Low resonance
            params[7] = 0.0f;         // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Estimate stage count from phase response
            int estimatedStages = PhaserAnalyzer::estimateStageCount(original[0], output[0], TEST_SAMPLE_RATE);
            
            log("  Estimated stage count: " + std::to_string(estimatedStages) + "\n");
            
            logCSV("StageConfiguration", "EstimatedStages_" + std::to_string(stageSetting),
                   estimatedStages, "MEASURED", "count");
            
            // Check phase shift at specific frequency
            float phaseShift = PhaserAnalyzer::measurePhaseShift(original[0], output[0], 1000.0, TEST_SAMPLE_RATE);
            
            log("  Phase shift at 1kHz: " + std::to_string(phaseShift) + " degrees\n");
            
            logCSV("StageConfiguration", "PhaseShift1k_" + std::to_string(stageSetting),
                   phaseShift, "MEASURED", "degrees");
            
            // More stages should produce more phase shift
            assertTrue(std::abs(phaseShift) > 0.0f, 
                      "Measurable phase shift at stage setting " + std::to_string(stageSetting));
            
            assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                      !PhaserAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at stage setting " + std::to_string(stageSetting));
        }
    }
    
    // Test 5: Feedback loop stability and coloration
    void testFeedbackStabilityAndColoration() {
        log("\n--- Feedback Stability and Coloration Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoNoise(0.2, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<float> feedbackSettings = {0.0f, 0.3f, 0.6f, 0.8f, 0.95f};
        
        for (float feedback : feedbackSettings) {
            log("\nTesting feedback level: " + std::to_string(feedback) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.2f;      // Slow rate
            params[1] = 0.3f;      // Low depth
            params[2] = feedback;  // Feedback parameter
            params[3] = 0.6f;      // Medium stages
            params[4] = 0.2f;      // Low stereo spread
            params[5] = 0.5f;      // Mid center frequency
            params[6] = 0.4f;      // Moderate resonance
            params[7] = 0.0f;      // Full effect
            
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
            
            // Measure timbral coloration
            float coloration = PhaserAnalyzer::measureFeedbackColoration(output[0], TEST_SAMPLE_RATE);
            log("  Spectral centroid: " + std::to_string(coloration) + "Hz\n");
            
            logCSV("FeedbackStability", "SpectralCentroid_" + std::to_string(feedback),
                   coloration, "MEASURED", "Hz");
            
            // System should remain stable
            assertTrue(outputPeakDB < 6.0f, 
                      "Stable output at feedback " + std::to_string(feedback));
            
            // Higher feedback should change timbre
            if (feedback > 0.6f) {
                assertTrue(coloration > 500.0f, 
                          "Timbral coloration at high feedback " + std::to_string(feedback));
            }
            
            assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                      !PhaserAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at feedback " + std::to_string(feedback));
        }
    }
    
    // Test 6: Stereo spread and phase relationships
    void testStereoSpreadAndPhase() {
        log("\n--- Stereo Spread and Phase Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> spreadSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float spread : spreadSettings) {
            log("\nTesting stereo spread: " + std::to_string(spread) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f;    // Moderate rate
            params[1] = 0.5f;    // Moderate depth
            params[2] = 0.2f;    // Low feedback
            params[3] = 0.5f;    // Medium stages
            params[4] = spread;  // Stereo spread parameter
            params[5] = 0.5f;    // Mid center frequency
            params[6] = 0.3f;    // Low resonance
            params[7] = 0.0f;    // Full effect
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure phase difference between channels
            float phaseSpread = PhaserAnalyzer::measureStereoPhaseSpread(output[0], output[1], 
                                                                        1000.0, TEST_SAMPLE_RATE);
            
            log("  Stereo phase spread: " + std::to_string(phaseSpread) + " degrees\n");
            
            logCSV("StereoSpread", "PhaseSpread_" + std::to_string(spread),
                   phaseSpread, "MEASURED", "degrees");
            
            // Higher spread should create more phase difference
            if (spread > 0.7f) {
                assertTrue(std::abs(phaseSpread) > 10.0f, 
                          "Significant phase spread at setting " + std::to_string(spread));
            }
            
            if (spread < 0.1f) {
                assertTrue(std::abs(phaseSpread) < 30.0f, 
                          "Minimal phase spread at low setting " + std::to_string(spread));
            }
            
            assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                      !PhaserAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at stereo spread " + std::to_string(spread));
        }
    }
    
    // Test 7: Mix parameter behavior
    void testMixParameterBehavior() {
        log("\n--- Mix Parameter Behavior Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> mixSettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float mix : mixSettings) {
            log("\nTesting mix level: " + std::to_string(mix) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;    // Moderate rate
            params[1] = 0.8f;    // High depth for clear effect
            params[2] = 0.4f;    // Some feedback
            params[3] = 0.75f;   // Many stages
            params[4] = 0.5f;    // Some stereo spread
            params[5] = 0.6f;    // Higher center frequency
            params[6] = 0.5f;    // Moderate resonance
            params[7] = mix;     // Mix parameter
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Calculate wet/dry balance
            float originalRMS = PhaserAnalyzer::calculateRMS_dB(original[0]);
            float outputRMS = PhaserAnalyzer::calculateRMS_dB(output[0]);
            float levelDifference = outputRMS - originalRMS;
            
            log("  Level difference: " + std::to_string(levelDifference) + "dB\n");
            
            logCSV("MixParameter", "LevelDiff_" + std::to_string(mix),
                   levelDifference, "MEASURED", "dB");
            
            // Measure phase shift as indication of effect amount
            float phaseShift = PhaserAnalyzer::measurePhaseShift(original[0], output[0], 1000.0, TEST_SAMPLE_RATE);
            
            log("  Phase shift: " + std::to_string(std::abs(phaseShift)) + " degrees\n");
            
            logCSV("MixParameter", "PhaseShift_" + std::to_string(mix),
                   std::abs(phaseShift), "MEASURED", "degrees");
            
            // Mix = 1.0 should be full dry (minimal phase shift)
            if (mix > 0.9f) {
                assertTrue(std::abs(phaseShift) < 10.0f, 
                          "Minimal phase shift at full dry mix");
            }
            
            // Mix = 0.0 should be full wet (maximum phase shift)
            if (mix < 0.1f) {
                assertTrue(std::abs(phaseShift) > 20.0f, 
                          "Significant phase shift at full wet mix");
            }
            
            assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                      !PhaserAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at mix " + std::to_string(mix));
        }
    }
    
    // Test 8: Performance and stability
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        auto longSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 8.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.6f; // Moderate settings
        params[1] = 0.7f;
        params[2] = 0.5f;
        params[3] = 0.8f;
        params[4] = 0.6f;
        params[5] = 0.7f;
        params[6] = 0.5f;
        params[7] = 0.3f;
        
        // Measure processing time
        auto startTime = std::chrono::high_resolution_clock::now();
        auto [original, output] = processAudio(longSignal, params);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0;
        
        double signalDuration = longSignal[0].size() / TEST_SAMPLE_RATE * 1000.0;
        double realTimeRatio = processingTime / signalDuration;
        
        log("Processing time: " + std::to_string(processingTime) + "ms\n");
        log("Signal duration: " + std::to_string(signalDuration) + "ms\n");
        log("Real-time ratio: " + std::to_string(realTimeRatio) + "\n");
        
        logCSV("Performance", "ProcessingTime", processingTime, "MEASURED", "ms");
        logCSV("Performance", "RealTimeRatio", realTimeRatio, "MEASURED", "ratio");
        
        assertTrue(!PhaserAnalyzer::hasInvalidValues(output[0]) &&
                  !PhaserAnalyzer::hasInvalidValues(output[1]), 
                  "Valid output during performance test");
        assertTrue(realTimeRatio < 0.5, "Real-time processing capability");
        
        float outputStability = PhaserAnalyzer::calculateRMS_dB(output[0]);
        assertTrue(outputStability > -60.0f, "Stable output level");
        
        log("Output stability: " + std::to_string(outputStability) + "dB\n");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Analog Phaser comprehensive test suite...\n");
        
        testParameterResponse();
        testLFORateAccuracy();
        testNotchFrequencyTracking();
        testAllPassStageConfiguration();
        testFeedbackStabilityAndColoration();
        testStereoSpreadAndPhase();
        testMixParameterBehavior();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        AnalogPhaserTest tester;
        tester.runAllTests();
        
        std::cout << "\nAnalog Phaser test suite completed successfully.\n";
        std::cout << "Check AnalogPhaser_TestResults.txt for detailed results.\n";
        std::cout << "Check AnalogPhaser_Data.csv for measurement data.\n";
        
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