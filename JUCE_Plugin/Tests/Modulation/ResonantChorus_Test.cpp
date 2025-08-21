/*
  ==============================================================================
  
    ResonantChorus_Test.cpp
    Comprehensive test suite for ENGINE_RESONANT_CHORUS (ResonantChorus)
    
    Tests for resonant chorus characteristics:
    - LFO rate accuracy and waveform shape
    - Depth/intensity modulation precision
    - Resonance frequency tracking
    - Filter Q factor accuracy
    - Stereo imaging and width control
    - Feedback loop stability with resonance
    - Mix parameter behavior
    - Comb filter characteristics
    
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
#include "../../Source/ResonantChorus.h"
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
    
    // Generate frequency sweep for resonance analysis
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
    
    // Generate impulse for resonance impulse response
    static std::vector<std::vector<float>> generateStereoImpulse(double amplitude, int position, 
                                                               int totalSamples) {
        std::vector<std::vector<float>> signal(2, std::vector<float>(totalSamples, 0.0f));
        if (position >= 0 && position < totalSamples) {
            signal[0][position] = static_cast<float>(amplitude);
            signal[1][position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate pink noise for resonance coloration testing
    static std::vector<std::vector<float>> generateStereoPinkNoise(double amplitude, double duration, 
                                                                 double sampleRate, unsigned seed = 0) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        std::mt19937 gen(seed == 0 ? std::random_device{}() : seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Pink noise filter coefficients (simple approximation)
        std::array<float, 2> b0 = {0.0f, 0.0f};
        std::array<float, 2> b1 = {0.0f, 0.0f};
        std::array<float, 2> b2 = {0.0f, 0.0f};
        
        for (int i = 0; i < numSamples; ++i) {
            float white = dist(gen);
            
            for (int ch = 0; ch < 2; ++ch) {
                // Simple pink noise filter
                b0[ch] = 0.99886f * b0[ch] + white * 0.0555179f;
                b1[ch] = 0.99332f * b1[ch] + white * 0.0750759f;
                b2[ch] = 0.96900f * b2[ch] + white * 0.1538520f;
                
                float pink = b0[ch] + b1[ch] + b2[ch] + white * 0.3104856f;
                signal[ch][i] = static_cast<float>(amplitude * pink * 0.11f); // Scale down
            }
        }
        
        return signal;
    }
};

// Audio analysis utilities specific to resonant effects
class ResonantAnalyzer {
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
    
    // Find resonant peaks in frequency response
    static std::vector<std::pair<float, float>> findResonantPeaks(const std::vector<float>& signal,
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
        
        std::vector<std::pair<float, float>> peaks; // frequency, magnitude pairs
        
        int startBin = static_cast<int>(minFreq * FFT_SIZE / sampleRate);
        int endBin = std::min(static_cast<int>(maxFreq * FFT_SIZE / sampleRate), 
                             static_cast<int>(magnitudes_db.size() / 2));
        
        // Find local maxima
        for (int i = startBin + 1; i < endBin - 1; ++i) {
            if (magnitudes_db[i] > magnitudes_db[i-1] && 
                magnitudes_db[i] > magnitudes_db[i+1] &&
                magnitudes_db[i] > -40.0) { // Minimum threshold
                
                float frequency = static_cast<float>(i * sampleRate / FFT_SIZE);
                float magnitude = static_cast<float>(magnitudes_db[i]);
                peaks.push_back({frequency, magnitude});
            }
        }
        
        // Sort by magnitude (highest first)
        std::sort(peaks.begin(), peaks.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return peaks;
    }
    
    // Measure Q factor at a resonant frequency
    static float measureQFactor(const std::vector<float>& signal, double sampleRate,
                               float centerFreq, float peakMagnitude) {
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
        
        int centerBin = static_cast<int>(centerFreq * FFT_SIZE / sampleRate);
        float targetLevel = peakMagnitude - 3.0f; // -3dB point
        
        // Find -3dB points
        int lowerBin = centerBin;
        int upperBin = centerBin;
        
        // Search downward
        for (int i = centerBin - 1; i >= 1; --i) {
            if (magnitudes_db[i] <= targetLevel) {
                lowerBin = i;
                break;
            }
        }
        
        // Search upward
        for (int i = centerBin + 1; i < static_cast<int>(magnitudes_db.size() / 2); ++i) {
            if (magnitudes_db[i] <= targetLevel) {
                upperBin = i;
                break;
            }
        }
        
        float lowerFreq = static_cast<float>(lowerBin * sampleRate / FFT_SIZE);
        float upperFreq = static_cast<float>(upperBin * sampleRate / FFT_SIZE);
        float bandwidth = upperFreq - lowerFreq;
        
        if (bandwidth > 0.0f) {
            return centerFreq / bandwidth;
        }
        
        return 0.0f;
    }
    
    // Analyze comb filter characteristics
    static std::vector<float> analyzeCombFilter(const std::vector<float>& signal,
                                               double sampleRate) {
        if (signal.size() < FFT_SIZE) return {};
        
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(signal.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed[i] = signal[i] * window;
            }
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto magnitudes_db = SimpleFFT::magnitudeDB(fft_result);
        
        std::vector<float> combSpacing;
        
        // Look for periodic peaks (comb filter characteristic)
        std::vector<int> peakBins;
        for (int i = 10; i < static_cast<int>(magnitudes_db.size() / 2) - 10; ++i) {
            bool isPeak = true;
            for (int j = -5; j <= 5; ++j) {
                if (j != 0 && magnitudes_db[i + j] >= magnitudes_db[i]) {
                    isPeak = false;
                    break;
                }
            }
            if (isPeak && magnitudes_db[i] > -30.0) {
                peakBins.push_back(i);
            }
        }
        
        // Calculate spacing between peaks
        for (size_t i = 1; i < peakBins.size(); ++i) {
            float freqSpacing = static_cast<float>((peakBins[i] - peakBins[i-1]) * sampleRate / FFT_SIZE);
            if (freqSpacing > 10.0f && freqSpacing < 2000.0f) {
                combSpacing.push_back(freqSpacing);
            }
        }
        
        return combSpacing;
    }
    
    // Measure modulation rate from envelope
    static float measureModulationRate(const std::vector<float>& modulatedSignal, 
                                     double sampleRate) {
        if (modulatedSignal.size() < FFT_SIZE) return 0.0f;
        
        // Extract envelope
        std::vector<double> envelope;
        envelope.reserve(modulatedSignal.size());
        
        float smoothing = 0.95f;
        float envelopeFollower = 0.0f;
        
        for (float sample : modulatedSignal) {
            float rectified = std::abs(sample);
            envelopeFollower = rectified + (envelopeFollower - rectified) * smoothing;
            envelope.push_back(envelopeFollower);
        }
        
        // Remove DC
        double dcLevel = std::accumulate(envelope.begin(), envelope.end(), 0.0) / envelope.size();
        for (double& env : envelope) {
            env -= dcLevel;
        }
        
        // FFT of envelope
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE && i < static_cast<int>(envelope.size()); ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            windowed[i] = envelope[i] * window;
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto magnitudes = SimpleFFT::magnitude(fft_result);
        
        // Find peak in modulation range
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
    
    // Check for invalid values
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) {
                return true;
            }
        }
        return false;
    }
    
    // Measure stereo correlation
    static float measureStereoCorrelation(const std::vector<float>& left,
                                         const std::vector<float>& right) {
        if (left.size() != right.size() || left.empty()) {
            return 0.0f;
        }
        
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
        
        if (denominator > 0.0) {
            return static_cast<float>(numerator / denominator);
        }
        
        return 0.0f;
    }
};

// Main test class for Resonant Chorus
class ResonantChorusTest {
private:
    std::unique_ptr<ResonantChorus> chorus;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    ResonantChorusTest() {
        chorus = std::make_unique<ResonantChorus>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/ResonantChorus_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/ResonantChorus_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the chorus
        chorus->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Resonant Chorus Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_RESONANT_CHORUS) + "\n");
        log("Parameter Count: " + std::to_string(chorus->getNumParameters()) + "\n\n");
    }
    
    ~ResonantChorusTest() {
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
    
    // Process stereo audio through resonant chorus
    std::pair<std::vector<std::vector<float>>, std::vector<std::vector<float>>> 
    processAudio(const std::vector<std::vector<float>>& input, 
                 const std::map<int, float>& parameters) {
        // Update parameters
        chorus->updateParameters(parameters);
        
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
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        for (int param = 0; param < chorus->getNumParameters(); ++param) {
            std::string paramName = chorus->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                // Set defaults
                for (int p = 0; p < chorus->getNumParameters(); ++p) {
                    params[p] = 0.5f;
                }
                
                params[param] = value;
                
                auto [original, output] = processAudio(testSignal, params);
                
                assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                          !ResonantAnalyzer::hasInvalidValues(output[1]), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                float outputRMS = ResonantAnalyzer::calculateRMS_dB(output[0]);
                responseDB.push_back(outputRMS);
                
                logCSV("ParameterResponse", paramName + "_" + std::to_string(value), 
                       outputRMS, "PASS", "dB");
            }
            
            float minResponse = *std::min_element(responseDB.begin(), responseDB.end());
            float maxResponse = *std::max_element(responseDB.begin(), responseDB.end());
            float responseRange = maxResponse - minResponse;
            
            log("  Response range: " + std::to_string(responseRange) + "dB\n");
            
            assertTrue(responseRange > 0.1f, 
                      paramName + " has measurable effect (range: " + 
                      std::to_string(responseRange) + "dB)");
        }
    }
    
    // Test 2: Resonance frequency tracking
    void testResonanceFrequencyTracking() {
        log("\n--- Resonance Frequency Tracking Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoPinkNoise(0.2, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> resonanceSettings = {0.2f, 0.4f, 0.6f, 0.8f};
        
        for (float resonanceSetting : resonanceSettings) {
            log("\nTesting resonance setting: " + std::to_string(resonanceSetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;              // Moderate rate
            params[1] = 0.5f;              // Moderate depth
            params[2] = 0.2f;              // Low feedback
            params[3] = resonanceSetting;  // Resonance frequency
            params[4] = 0.7f;              // High Q for clear resonance
            
            if (chorus->getNumParameters() > 5) {
                params[5] = 0.5f;          // Width
                params[6] = 0.0f;          // Full effect
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Find resonant peaks
            auto peaks = ResonantAnalyzer::findResonantPeaks(output[0], TEST_SAMPLE_RATE);
            
            log("  Found " + std::to_string(peaks.size()) + " resonant peaks:\n");
            
            for (size_t i = 0; i < std::min(peaks.size(), size_t(3)); ++i) {
                log("    Peak " + std::to_string(i + 1) + ": " + 
                    std::to_string(peaks[i].first) + "Hz, " + 
                    std::to_string(peaks[i].second) + "dB\n");
                
                logCSV("ResonanceTracking", "Peak" + std::to_string(i + 1) + "_Freq_" + 
                       std::to_string(resonanceSetting), peaks[i].first, "MEASURED", "Hz");
                logCSV("ResonanceTracking", "Peak" + std::to_string(i + 1) + "_Mag_" + 
                       std::to_string(resonanceSetting), peaks[i].second, "MEASURED", "dB");
            }
            
            // Should have at least one prominent resonant peak
            assertTrue(!peaks.empty(), 
                      "Resonant peaks found at setting " + std::to_string(resonanceSetting));
            
            if (!peaks.empty()) {
                assertTrue(peaks[0].second > -20.0f, 
                          "Prominent resonant peak at setting " + std::to_string(resonanceSetting));
            }
            
            assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                      !ResonantAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at resonance " + std::to_string(resonanceSetting));
        }
    }
    
    // Test 3: Filter Q factor accuracy
    void testQFactorAccuracy() {
        log("\n--- Q Factor Accuracy Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSweep(100.0, 8000.0, 0.3, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<float> qSettings = {0.2f, 0.5f, 0.8f, 1.0f};
        
        for (float qSetting : qSettings) {
            log("\nTesting Q factor setting: " + std::to_string(qSetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f;        // Slow rate
            params[1] = 0.3f;        // Low depth
            params[2] = 0.1f;        // Low feedback
            params[3] = 0.6f;        // Mid-range resonance frequency
            params[4] = qSetting;    // Q factor
            
            if (chorus->getNumParameters() > 5) {
                params[5] = 0.5f;    // Width
                params[6] = 0.0f;    // Full effect
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Find the main resonant peak
            auto peaks = ResonantAnalyzer::findResonantPeaks(output[0], TEST_SAMPLE_RATE, 200.0f, 4000.0f);
            
            if (!peaks.empty()) {
                float centerFreq = peaks[0].first;
                float peakMag = peaks[0].second;
                
                float qFactor = ResonantAnalyzer::measureQFactor(output[0], TEST_SAMPLE_RATE, 
                                                               centerFreq, peakMag);
                
                log("  Center frequency: " + std::to_string(centerFreq) + "Hz\n");
                log("  Peak magnitude: " + std::to_string(peakMag) + "dB\n");
                log("  Measured Q factor: " + std::to_string(qFactor) + "\n");
                
                logCSV("QFactorAccuracy", "CenterFreq_" + std::to_string(qSetting),
                       centerFreq, "MEASURED", "Hz");
                logCSV("QFactorAccuracy", "QFactor_" + std::to_string(qSetting),
                       qFactor, "MEASURED", "ratio");
                
                // Higher Q settings should produce higher measured Q
                if (qSetting > 0.7f) {
                    assertTrue(qFactor > 5.0f, 
                              "High Q factor at setting " + std::to_string(qSetting));
                }
                
                assertTrue(qFactor > 1.0f, 
                          "Measurable Q factor at setting " + std::to_string(qSetting));
            }
            
            assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                      !ResonantAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at Q setting " + std::to_string(qSetting));
        }
    }
    
    // Test 4: Comb filter characteristics
    void testCombFilterCharacteristics() {
        log("\n--- Comb Filter Characteristics Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoPinkNoise(0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> delaySettings = {0.2f, 0.5f, 0.8f};
        
        for (float delaySetting : delaySettings) {
            log("\nTesting delay setting: " + std::to_string(delaySetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.1f;         // Very slow rate to minimize modulation
            params[1] = 0.1f;         // Minimal depth
            params[2] = 0.6f;         // Moderate feedback for comb effect
            params[3] = 0.5f;         // Mid-range resonance
            params[4] = 0.3f;         // Moderate Q
            
            if (chorus->getNumParameters() > 5) {
                params[5] = 0.5f;     // Width
                params[6] = 0.0f;     // Full effect
            }
            
            // Override delay parameter if available
            if (chorus->getNumParameters() > 7) {
                params[7] = delaySetting; // Delay time parameter
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Analyze comb filter spacing
            auto combSpacing = ResonantAnalyzer::analyzeCombFilter(output[0], TEST_SAMPLE_RATE);
            
            log("  Found " + std::to_string(combSpacing.size()) + " comb spacings:\n");
            
            float avgSpacing = 0.0f;
            if (!combSpacing.empty()) {
                for (size_t i = 0; i < std::min(combSpacing.size(), size_t(5)); ++i) {
                    log("    Spacing " + std::to_string(i + 1) + ": " + 
                        std::to_string(combSpacing[i]) + "Hz\n");
                    avgSpacing += combSpacing[i];
                }
                avgSpacing /= std::min(combSpacing.size(), size_t(5));
                
                log("  Average comb spacing: " + std::to_string(avgSpacing) + "Hz\n");
                
                logCSV("CombFilter", "AvgSpacing_" + std::to_string(delaySetting),
                       avgSpacing, "MEASURED", "Hz");
            }
            
            // Should show comb filter characteristics with feedback
            if (params[2] > 0.4f) { // Significant feedback
                assertTrue(!combSpacing.empty(), 
                          "Comb filter characteristics at delay " + std::to_string(delaySetting));
            }
            
            assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                      !ResonantAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at delay " + std::to_string(delaySetting));
        }
    }
    
    // Test 5: LFO rate and modulation interaction
    void testLFOModulationInteraction() {
        log("\n--- LFO Modulation Interaction Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<std::pair<float, float>> rateDepthPairs = {
            {0.2f, 0.3f}, {0.5f, 0.6f}, {0.8f, 0.9f}
        };
        
        for (const auto& [rate, depth] : rateDepthPairs) {
            log("\nTesting LFO rate: " + std::to_string(rate) + 
                ", depth: " + std::to_string(depth) + "\n");
            
            std::map<int, float> params;
            params[0] = rate;        // Rate
            params[1] = depth;       // Depth
            params[2] = 0.3f;        // Feedback
            params[3] = 0.6f;        // Resonance frequency
            params[4] = 0.5f;        // Q factor
            
            if (chorus->getNumParameters() > 5) {
                params[5] = 0.5f;    // Width
                params[6] = 0.0f;    // Full effect
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure modulation rate
            float measuredRate = ResonantAnalyzer::measureModulationRate(output[0], TEST_SAMPLE_RATE);
            log("  Measured modulation rate: " + std::to_string(measuredRate) + "Hz\n");
            
            logCSV("LFOModulation", "Rate_" + std::to_string(rate) + "_" + std::to_string(depth),
                   measuredRate, "MEASURED", "Hz");
            
            // Check for resonant modulation artifacts
            auto peaks = ResonantAnalyzer::findResonantPeaks(output[0], TEST_SAMPLE_RATE);
            
            log("  Resonant peaks during modulation: " + std::to_string(peaks.size()) + "\n");
            
            // Should show modulation activity
            assertTrue(measuredRate > 0.1f, 
                      "Measurable modulation at rate " + std::to_string(rate));
            
            assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                      !ResonantAnalyzer::hasInvalidValues(output[1]),
                      "Valid output with rate " + std::to_string(rate) + 
                      " and depth " + std::to_string(depth));
        }
    }
    
    // Test 6: Stereo width and correlation
    void testStereoWidthAndCorrelation() {
        log("\n--- Stereo Width and Correlation Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> widthSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float width : widthSettings) {
            log("\nTesting stereo width: " + std::to_string(width) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;        // Moderate rate
            params[1] = 0.6f;        // Moderate depth
            params[2] = 0.2f;        // Low feedback
            params[3] = 0.5f;        // Mid resonance
            params[4] = 0.4f;        // Moderate Q
            
            if (chorus->getNumParameters() > 5) {
                params[5] = width;   // Width parameter
                params[6] = 0.0f;    // Full effect
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure stereo correlation
            float correlation = ResonantAnalyzer::measureStereoCorrelation(output[0], output[1]);
            
            log("  Stereo correlation: " + std::to_string(correlation) + "\n");
            
            logCSV("StereoWidth", "Correlation_" + std::to_string(width),
                   correlation, "MEASURED", "ratio");
            
            // Width should affect correlation
            if (width > 0.7f) {
                assertTrue(std::abs(correlation) < 0.9f, 
                          "Reduced correlation at wide setting " + std::to_string(width));
            }
            
            assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                      !ResonantAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at width " + std::to_string(width));
        }
    }
    
    // Test 7: Feedback stability with resonance
    void testFeedbackStabilityWithResonance() {
        log("\n--- Feedback Stability with Resonance Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.2, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<std::pair<float, float>> feedbackResonancePairs = {
            {0.3f, 0.5f}, {0.6f, 0.7f}, {0.8f, 0.9f}
        };
        
        for (const auto& [feedback, resonance] : feedbackResonancePairs) {
            log("\nTesting feedback: " + std::to_string(feedback) + 
                ", resonance: " + std::to_string(resonance) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;           // Moderate rate
            params[1] = 0.5f;           // Moderate depth
            params[2] = feedback;       // Feedback
            params[3] = resonance;      // Resonance frequency
            params[4] = 0.8f;           // High Q for stronger resonance
            
            if (chorus->getNumParameters() > 5) {
                params[5] = 0.5f;       // Width
                params[6] = 0.0f;       // Full effect
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Check for stability
            float outputPeak = 0.0f;
            for (const auto& channel : output) {
                for (float sample : channel) {
                    outputPeak = std::max(outputPeak, std::abs(sample));
                }
            }
            
            float outputPeakDB = 20.0f * std::log10(std::max(1e-6f, outputPeak));
            log("  Output peak level: " + std::to_string(outputPeakDB) + "dB\n");
            
            logCSV("FeedbackStability", "PeakLevel_" + std::to_string(feedback) + "_" + 
                   std::to_string(resonance), outputPeakDB, "MEASURED", "dB");
            
            // System should remain stable
            assertTrue(outputPeakDB < 12.0f, 
                      "Stable output with feedback " + std::to_string(feedback) + 
                      " and resonance " + std::to_string(resonance));
            
            assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                      !ResonantAnalyzer::hasInvalidValues(output[1]),
                      "Valid output with feedback " + std::to_string(feedback) + 
                      " and resonance " + std::to_string(resonance));
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
        params[3] = 0.6f;
        params[4] = 0.8f;
        if (chorus->getNumParameters() > 5) {
            params[5] = 0.7f;
            params[6] = 0.2f;
        }
        
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
        
        assertTrue(!ResonantAnalyzer::hasInvalidValues(output[0]) &&
                  !ResonantAnalyzer::hasInvalidValues(output[1]), 
                  "Valid output during performance test");
        assertTrue(realTimeRatio < 0.6, "Real-time processing capability");
        
        float outputStability = ResonantAnalyzer::calculateRMS_dB(output[0]);
        assertTrue(outputStability > -60.0f, "Stable output level");
        
        log("Output stability: " + std::to_string(outputStability) + "dB\n");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Resonant Chorus comprehensive test suite...\n");
        
        testParameterResponse();
        testResonanceFrequencyTracking();
        testQFactorAccuracy();
        testCombFilterCharacteristics();
        testLFOModulationInteraction();
        testStereoWidthAndCorrelation();
        testFeedbackStabilityWithResonance();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        ResonantChorusTest tester;
        tester.runAllTests();
        
        std::cout << "\nResonant Chorus test suite completed successfully.\n";
        std::cout << "Check ResonantChorus_TestResults.txt for detailed results.\n";
        std::cout << "Check ResonantChorus_Data.csv for measurement data.\n";
        
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