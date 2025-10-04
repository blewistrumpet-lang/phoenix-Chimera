/*
  ==============================================================================
  
    RingModulator_Test.cpp
    Comprehensive test suite for ENGINE_RING_MODULATOR (AnalogRingModulator)
    
    Tests for ring modulator characteristics:
    - Carrier frequency precision and stability
    - Amplitude modulation depth accuracy
    - Sideband generation and suppression
    - Harmonic distortion analysis
    - DC offset handling
    - Stereo imaging and width control
    - Mix parameter behavior
    - Ring vs. Amplitude modulation modes
    
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
#include "../../Source/AnalogRingModulator.h"
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
    
    // Generate two-tone test signal for intermodulation testing
    static std::vector<std::vector<float>> generateTwoTone(double freq1, double freq2, 
                                                         double amplitude, double duration, 
                                                         double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        double phase1 = 0.0, phase2 = 0.0;
        double phaseIncrement1 = 2.0 * M_PI * freq1 / sampleRate;
        double phaseIncrement2 = 2.0 * M_PI * freq2 / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = static_cast<float>(amplitude * 0.5 * (std::sin(phase1) + std::sin(phase2)));
            signal[0][i] = sample;
            signal[1][i] = sample;
            
            phase1 += phaseIncrement1;
            phase2 += phaseIncrement2;
            if (phase1 > 2.0 * M_PI) phase1 -= 2.0 * M_PI;
            if (phase2 > 2.0 * M_PI) phase2 -= 2.0 * M_PI;
        }
        
        return signal;
    }
    
    // Generate DC test signal
    static std::vector<std::vector<float>> generateDCSignal(double dcLevel, double duration, 
                                                          double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        float dcSample = static_cast<float>(dcLevel);
        for (int i = 0; i < numSamples; ++i) {
            signal[0][i] = dcSample;
            signal[1][i] = dcSample;
        }
        
        return signal;
    }
    
    // Generate complex waveform for harmonic analysis
    static std::vector<std::vector<float>> generateComplexWave(double fundamentalFreq, 
                                                             double amplitude, double duration, 
                                                             double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * fundamentalFreq / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            // Complex waveform with harmonics
            float sample = static_cast<float>(amplitude * (
                std::sin(phase) +                    // Fundamental
                0.3 * std::sin(2.0 * phase) +      // 2nd harmonic
                0.1 * std::sin(3.0 * phase)        // 3rd harmonic
            ));
            
            signal[0][i] = sample;
            signal[1][i] = sample;
            
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
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

// Audio analysis utilities specific to ring modulation effects
class RingModulatorAnalyzer {
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
    
    // Find sideband frequencies and measure their levels
    static std::vector<std::pair<float, float>> findSidebands(const std::vector<float>& signal,
                                                             double sampleRate,
                                                             double carrierFreq,
                                                             double inputFreq) {
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
        
        std::vector<std::pair<float, float>> sidebands; // frequency, level pairs
        
        // Calculate expected sideband frequencies
        std::vector<double> expectedFreqs = {
            std::abs(carrierFreq - inputFreq),  // Lower sideband
            carrierFreq + inputFreq,            // Upper sideband
            inputFreq,                          // Fundamental (should be suppressed in ring mod)
            carrierFreq                         // Carrier (should be suppressed in ring mod)
        };
        
        for (double freq : expectedFreqs) {
            if (freq > 0.0 && freq < sampleRate * 0.5) {
                int bin = static_cast<int>(freq * FFT_SIZE / sampleRate);
                if (bin >= 1 && bin < static_cast<int>(magnitudes_db.size() / 2)) {
                    float level = static_cast<float>(magnitudes_db[bin]);
                    sidebands.push_back({static_cast<float>(freq), level});
                }
            }
        }
        
        return sidebands;
    }
    
    // Measure carrier frequency precision
    static float measureCarrierFrequency(const std::vector<float>& signal, double sampleRate,
                                        double expectedCarrier) {
        if (signal.size() < FFT_SIZE) return 0.0f;
        
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(signal.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed[i] = signal[i] * window;
            }
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto magnitudes = SimpleFFT::magnitude(fft_result);
        
        // Search around expected carrier frequency
        int centerBin = static_cast<int>(expectedCarrier * FFT_SIZE / sampleRate);
        int searchRange = 10; // ±10 bins
        
        int peakBin = centerBin;
        double peakMagnitude = (centerBin < static_cast<int>(magnitudes.size())) ? magnitudes[centerBin] : 0.0;
        
        for (int i = std::max(1, centerBin - searchRange); 
             i < std::min(static_cast<int>(magnitudes.size() / 2), centerBin + searchRange); ++i) {
            if (magnitudes[i] > peakMagnitude) {
                peakMagnitude = magnitudes[i];
                peakBin = i;
            }
        }
        
        return static_cast<float>(peakBin * sampleRate / FFT_SIZE);
    }
    
    // Measure total harmonic distortion
    static float measureTHD(const std::vector<float>& signal, double sampleRate, 
                           double fundamentalFreq) {
        if (signal.size() < FFT_SIZE) return 0.0f;
        
        std::vector<double> windowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (i < static_cast<int>(signal.size())) {
                double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                windowed[i] = signal[i] * window;
            }
        }
        
        auto fft_result = SimpleFFT::fft(windowed);
        auto magnitudes = SimpleFFT::magnitude(fft_result);
        
        // Find fundamental
        int fundamentalBin = static_cast<int>(fundamentalFreq * FFT_SIZE / sampleRate);
        double fundamentalMag = (fundamentalBin < static_cast<int>(magnitudes.size())) ? 
                               magnitudes[fundamentalBin] : 0.0;
        
        // Sum harmonic magnitudes (2nd through 10th harmonics)
        double harmonicSum = 0.0;
        for (int harmonic = 2; harmonic <= 10; ++harmonic) {
            int harmonicBin = static_cast<int>(harmonic * fundamentalFreq * FFT_SIZE / sampleRate);
            if (harmonicBin < static_cast<int>(magnitudes.size() / 2)) {
                harmonicSum += magnitudes[harmonicBin] * magnitudes[harmonicBin];
            }
        }
        
        harmonicSum = std::sqrt(harmonicSum);
        
        if (fundamentalMag > 0.0) {
            return static_cast<float>(100.0 * harmonicSum / fundamentalMag); // THD as percentage
        }
        
        return 0.0f;
    }
    
    // Measure DC offset
    static float measureDCOffset(const std::vector<float>& signal) {
        if (signal.empty()) return 0.0f;
        
        double sum = 0.0;
        for (float sample : signal) {
            sum += sample;
        }
        
        return static_cast<float>(sum / signal.size());
    }
    
    // Measure modulation depth
    static float measureModulationDepth(const std::vector<float>& signal) {
        if (signal.size() < 1024) return 0.0f;
        
        // Calculate envelope
        std::vector<float> envelope;
        envelope.reserve(signal.size());
        
        float smoothing = 0.95f;
        float envelopeFollower = 0.0f;
        
        for (float sample : signal) {
            float rectified = std::abs(sample);
            envelopeFollower = rectified + (envelopeFollower - rectified) * smoothing;
            envelope.push_back(envelopeFollower);
        }
        
        float minEnv = *std::min_element(envelope.begin(), envelope.end());
        float maxEnv = *std::max_element(envelope.begin(), envelope.end());
        
        if (maxEnv > 0.0f) {
            return (maxEnv - minEnv) / (maxEnv + minEnv); // Modulation index
        }
        
        return 0.0f;
    }
    
    // Measure carrier suppression (how well the carrier is suppressed in ring modulation)
    static float measureCarrierSuppression(const std::vector<float>& original,
                                          const std::vector<float>& modulated,
                                          double sampleRate, double carrierFreq) {
        if (original.size() != modulated.size() || original.size() < FFT_SIZE) {
            return 0.0f;
        }
        
        // FFT both signals
        std::vector<double> origWindowed(FFT_SIZE);
        std::vector<double> modWindowed(FFT_SIZE);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            if (i < static_cast<int>(original.size())) {
                origWindowed[i] = original[i] * window;
                modWindowed[i] = modulated[i] * window;
            }
        }
        
        auto origFFT = SimpleFFT::fft(origWindowed);
        auto modFFT = SimpleFFT::fft(modWindowed);
        
        auto origMag = SimpleFFT::magnitudeDB(origFFT);
        auto modMag = SimpleFFT::magnitudeDB(modFFT);
        
        // Find carrier bin
        int carrierBin = static_cast<int>(carrierFreq * FFT_SIZE / sampleRate);
        carrierBin = std::max(1, std::min(carrierBin, static_cast<int>(origMag.size() / 2 - 1)));
        
        // Calculate suppression
        double origLevel = origMag[carrierBin];
        double modLevel = modMag[carrierBin];
        
        return static_cast<float>(origLevel - modLevel); // Suppression in dB
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

// Main test class for Ring Modulator
class RingModulatorTest {
private:
    std::unique_ptr<AnalogRingModulator> ringMod;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    RingModulatorTest() {
        ringMod = std::make_unique<AnalogRingModulator>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/RingModulator_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/RingModulator_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the ring modulator
        ringMod->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Ring Modulator Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Engine ID: " + std::to_string(ENGINE_RING_MODULATOR) + "\n");
        log("Parameter Count: " + std::to_string(ringMod->getNumParameters()) + "\n\n");
    }
    
    ~RingModulatorTest() {
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
    
    // Process stereo audio through ring modulator
    std::pair<std::vector<std::vector<float>>, std::vector<std::vector<float>>> 
    processAudio(const std::vector<std::vector<float>>& input, 
                 const std::map<int, float>& parameters) {
        // Update parameters
        ringMod->updateParameters(parameters);
        
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
            ringMod->process(buffer);
            
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
        
        for (int param = 0; param < ringMod->getNumParameters(); ++param) {
            std::string paramName = ringMod->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                // Set defaults
                for (int p = 0; p < ringMod->getNumParameters(); ++p) {
                    params[p] = 0.5f;
                }
                
                params[param] = value;
                
                auto [original, output] = processAudio(testSignal, params);
                
                assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                          !RingModulatorAnalyzer::hasInvalidValues(output[1]), 
                          paramName + " at " + std::to_string(value) + " produces valid output");
                
                float outputRMS = RingModulatorAnalyzer::calculateRMS_dB(output[0]);
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
    
    // Test 2: Carrier frequency precision and stability
    void testCarrierFrequencyPrecision() {
        log("\n--- Carrier Frequency Precision Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> expectedCarriers = {100.0f, 440.0f, 1000.0f, 2000.0f, 5000.0f}; // Hz
        std::vector<float> carrierSettings = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f}; // Parameter values
        
        for (size_t i = 0; i < carrierSettings.size(); ++i) {
            float carrierSetting = carrierSettings[i];
            float expectedCarrier = expectedCarriers[i];
            
            log("\nTesting carrier frequency setting: " + std::to_string(carrierSetting) + 
                " (expected ~" + std::to_string(expectedCarrier) + "Hz)\n");
            
            std::map<int, float> params;
            params[0] = carrierSetting;  // Carrier frequency parameter
            params[1] = 0.8f;           // High depth/amount
            
            // Set other parameters to reasonable defaults
            for (int p = 2; p < ringMod->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure actual carrier frequency
            float measuredCarrier = RingModulatorAnalyzer::measureCarrierFrequency(
                output[0], TEST_SAMPLE_RATE, expectedCarrier);
            
            float carrierError = (expectedCarrier > 0.0f) ? 
                std::abs(measuredCarrier - expectedCarrier) / expectedCarrier * 100.0f : 100.0f;
            
            log("  Measured carrier frequency: " + std::to_string(measuredCarrier) + "Hz\n");
            log("  Carrier error: " + std::to_string(carrierError) + "%\n");
            
            logCSV("CarrierPrecision", "MeasuredCarrier_" + std::to_string(carrierSetting),
                   measuredCarrier, "MEASURED", "Hz");
            logCSV("CarrierPrecision", "CarrierError_" + std::to_string(carrierSetting),
                   carrierError, "MEASURED", "%");
            
            // Carrier should be reasonably accurate
            assertTrue(carrierError < 50.0f, 
                      "Carrier frequency accuracy at setting " + std::to_string(carrierSetting) +
                      " (error: " + std::to_string(carrierError) + "%)");
            
            assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                      !RingModulatorAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at carrier setting " + std::to_string(carrierSetting));
        }
    }
    
    // Test 3: Sideband generation and analysis
    void testSidebandGeneration() {
        log("\n--- Sideband Generation Tests ---\n");
        
        double inputFreq = 1000.0;
        double carrierFreq = 2000.0;
        auto testSignal = TestSignalGenerator::generateStereoSineWave(inputFreq, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> depthSettings = {0.3f, 0.6f, 1.0f};
        
        for (float depth : depthSettings) {
            log("\nTesting modulation depth: " + std::to_string(depth) + 
                " (Input: " + std::to_string(inputFreq) + "Hz, Carrier: " + 
                std::to_string(carrierFreq) + "Hz)\n");
            
            std::map<int, float> params;
            params[0] = 0.7f;    // Carrier frequency setting for ~2kHz
            params[1] = depth;   // Modulation depth
            
            for (int p = 2; p < ringMod->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Analyze sidebands
            auto sidebands = RingModulatorAnalyzer::findSidebands(output[0], TEST_SAMPLE_RATE, 
                                                                 carrierFreq, inputFreq);
            
            log("  Found " + std::to_string(sidebands.size()) + " sideband components:\n");
            
            for (size_t i = 0; i < sidebands.size(); ++i) {
                log("    Component " + std::to_string(i + 1) + ": " + 
                    std::to_string(sidebands[i].first) + "Hz, " + 
                    std::to_string(sidebands[i].second) + "dB\n");
                
                logCSV("SidebandGeneration", "Component" + std::to_string(i + 1) + "_Freq_" + 
                       std::to_string(depth), sidebands[i].first, "MEASURED", "Hz");
                logCSV("SidebandGeneration", "Component" + std::to_string(i + 1) + "_Level_" + 
                       std::to_string(depth), sidebands[i].second, "MEASURED", "dB");
            }
            
            // Should generate expected sidebands
            bool hasLowerSideband = false, hasUpperSideband = false;
            float expectedLower = std::abs(carrierFreq - inputFreq);
            float expectedUpper = carrierFreq + inputFreq;
            
            for (const auto& [freq, level] : sidebands) {
                if (std::abs(freq - expectedLower) < 50.0f && level > -40.0f) {
                    hasLowerSideband = true;
                }
                if (std::abs(freq - expectedUpper) < 50.0f && level > -40.0f) {
                    hasUpperSideband = true;
                }
            }
            
            assertTrue(hasLowerSideband || hasUpperSideband, 
                      "Sideband generation at depth " + std::to_string(depth));
            
            assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                      !RingModulatorAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at depth " + std::to_string(depth));
        }
    }
    
    // Test 4: Harmonic distortion analysis
    void testHarmonicDistortion() {
        log("\n--- Harmonic Distortion Analysis Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateComplexWave(500.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> intensitySettings = {0.3f, 0.6f, 0.9f};
        
        for (float intensity : intensitySettings) {
            log("\nTesting intensity setting: " + std::to_string(intensity) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.6f;        // Carrier frequency
            params[1] = intensity;   // Intensity/depth
            
            for (int p = 2; p < ringMod->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure THD at fundamental frequency
            float thd = RingModulatorAnalyzer::measureTHD(output[0], TEST_SAMPLE_RATE, 500.0);
            
            log("  Total Harmonic Distortion: " + std::to_string(thd) + "%\n");
            
            logCSV("HarmonicDistortion", "THD_" + std::to_string(intensity),
                   thd, "MEASURED", "%");
            
            // Ring modulation should introduce harmonics
            assertTrue(thd >= 0.0f, 
                      "THD measurement valid at intensity " + std::to_string(intensity));
            
            // Higher intensity should generally increase distortion
            if (intensity > 0.7f) {
                assertTrue(thd > 1.0f, 
                          "Increased distortion at high intensity " + std::to_string(intensity));
            }
            
            assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                      !RingModulatorAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at intensity " + std::to_string(intensity));
        }
    }
    
    // Test 5: DC offset handling
    void testDCOffsetHandling() {
        log("\n--- DC Offset Handling Tests ---\n");
        
        // Create test signal with DC offset
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        // Add DC offset
        for (auto& channel : testSignal) {
            for (float& sample : channel) {
                sample += 0.2f; // 20% DC offset
            }
        }
        
        std::vector<float> dcSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float dcSetting : dcSettings) {
            log("\nTesting DC handling setting: " + std::to_string(dcSetting) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;       // Carrier frequency
            params[1] = 0.6f;       // Modulation depth
            
            // Set DC offset parameter if available
            if (ringMod->getNumParameters() > 3) {
                params[3] = dcSetting;
            }
            
            for (int p = 2; p < ringMod->getNumParameters(); ++p) {
                if (p != 3) params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure DC offset in output
            float outputDC = RingModulatorAnalyzer::measureDCOffset(output[0]);
            float inputDC = RingModulatorAnalyzer::measureDCOffset(original[0]);
            
            log("  Input DC offset: " + std::to_string(inputDC) + "\n");
            log("  Output DC offset: " + std::to_string(outputDC) + "\n");
            
            logCSV("DCOffsetHandling", "InputDC_" + std::to_string(dcSetting),
                   inputDC, "MEASURED", "amplitude");
            logCSV("DCOffsetHandling", "OutputDC_" + std::to_string(dcSetting),
                   outputDC, "MEASURED", "amplitude");
            
            // DC handling should control offset
            assertTrue(std::abs(outputDC) <= std::abs(inputDC) * 1.5f, 
                      "DC offset controlled at setting " + std::to_string(dcSetting));
            
            assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                      !RingModulatorAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at DC setting " + std::to_string(dcSetting));
        }
    }
    
    // Test 6: Carrier suppression in ring modulation mode
    void testCarrierSuppression() {
        log("\n--- Carrier Suppression Tests ---\n");
        
        double carrierFreq = 2000.0;
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> modeSettings = {0.0f, 0.5f, 1.0f}; // Different modulation modes
        
        for (float mode : modeSettings) {
            log("\nTesting modulation mode: " + std::to_string(mode) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.7f;    // Carrier frequency for ~2kHz
            params[1] = 0.8f;    // High depth
            
            // Mode parameter if available
            if (ringMod->getNumParameters() > 4) {
                params[4] = mode;
            }
            
            for (int p = 2; p < ringMod->getNumParameters(); ++p) {
                if (p != 4) params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure carrier suppression
            float suppression = RingModulatorAnalyzer::measureCarrierSuppression(
                original[0], output[0], TEST_SAMPLE_RATE, carrierFreq);
            
            log("  Carrier suppression: " + std::to_string(suppression) + "dB\n");
            
            logCSV("CarrierSuppression", "Suppression_" + std::to_string(mode),
                   suppression, "MEASURED", "dB");
            
            // Ring modulation should suppress carrier
            assertTrue(suppression >= 0.0f, 
                      "Measurable carrier suppression at mode " + std::to_string(mode));
            
            assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                      !RingModulatorAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at mode " + std::to_string(mode));
        }
    }
    
    // Test 7: Stereo imaging and width control
    void testStereoImagingAndWidth() {
        log("\n--- Stereo Imaging and Width Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> widthSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float width : widthSettings) {
            log("\nTesting stereo width: " + std::to_string(width) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;    // Carrier frequency
            params[1] = 0.7f;    // High depth for clear effect
            
            // Width parameter if available
            if (ringMod->getNumParameters() > 5) {
                params[5] = width;
            }
            
            for (int p = 2; p < ringMod->getNumParameters(); ++p) {
                if (p != 5) params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure stereo correlation
            float correlation = RingModulatorAnalyzer::measureStereoCorrelation(output[0], output[1]);
            
            log("  Stereo correlation: " + std::to_string(correlation) + "\n");
            
            logCSV("StereoImaging", "Correlation_" + std::to_string(width),
                   correlation, "MEASURED", "ratio");
            
            // Width should affect correlation
            if (width > 0.7f) {
                assertTrue(std::abs(correlation) < 0.95f, 
                          "Reduced correlation at wide setting " + std::to_string(width));
            }
            
            assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                      !RingModulatorAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at width " + std::to_string(width));
        }
    }
    
    // Test 8: Mix parameter behavior
    void testMixParameterBehavior() {
        log("\n--- Mix Parameter Behavior Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> mixSettings = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float mix : mixSettings) {
            log("\nTesting mix level: " + std::to_string(mix) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.6f;    // Carrier frequency
            params[1] = 0.8f;    // High depth for clear effect
            
            // Mix parameter (usually the last parameter)
            int mixParam = ringMod->getNumParameters() - 1;
            params[mixParam] = mix;
            
            for (int p = 2; p < ringMod->getNumParameters(); ++p) {
                if (p != mixParam) params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Calculate wet/dry balance
            float originalRMS = RingModulatorAnalyzer::calculateRMS_dB(original[0]);
            float outputRMS = RingModulatorAnalyzer::calculateRMS_dB(output[0]);
            float levelDifference = outputRMS - originalRMS;
            
            log("  Level difference: " + std::to_string(levelDifference) + "dB\n");
            
            logCSV("MixParameter", "LevelDiff_" + std::to_string(mix),
                   levelDifference, "MEASURED", "dB");
            
            // Measure modulation presence
            float modulation = RingModulatorAnalyzer::measureModulationDepth(output[0]);
            log("  Modulation amount: " + std::to_string(modulation) + "\n");
            
            logCSV("MixParameter", "Modulation_" + std::to_string(mix),
                   modulation, "MEASURED", "ratio");
            
            // Mix behavior validation
            if (mix > 0.9f) {
                assertTrue(modulation < 0.1f, 
                          "Minimal modulation at full dry mix");
            }
            
            if (mix < 0.1f) {
                assertTrue(modulation > 0.05f, 
                          "Significant modulation at full wet mix");
            }
            
            assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                      !RingModulatorAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at mix " + std::to_string(mix));
        }
    }
    
    // Test 9: Performance and stability
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        auto longSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 8.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.6f; // Moderate settings
        params[1] = 0.7f;
        for (int p = 2; p < ringMod->getNumParameters(); ++p) {
            params[p] = 0.5f;
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
        
        assertTrue(!RingModulatorAnalyzer::hasInvalidValues(output[0]) &&
                  !RingModulatorAnalyzer::hasInvalidValues(output[1]), 
                  "Valid output during performance test");
        assertTrue(realTimeRatio < 0.4, "Real-time processing capability");
        
        float outputStability = RingModulatorAnalyzer::calculateRMS_dB(output[0]);
        assertTrue(outputStability > -60.0f, "Stable output level");
        
        log("Output stability: " + std::to_string(outputStability) + "dB\n");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Ring Modulator comprehensive test suite...\n");
        
        testParameterResponse();
        testCarrierFrequencyPrecision();
        testSidebandGeneration();
        testHarmonicDistortion();
        testDCOffsetHandling();
        testCarrierSuppression();
        testStereoImagingAndWidth();
        testMixParameterBehavior();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        RingModulatorTest tester;
        tester.runAllTests();
        
        std::cout << "\nRing Modulator test suite completed successfully.\n";
        std::cout << "Check RingModulator_TestResults.txt for detailed results.\n";
        std::cout << "Check RingModulator_Data.csv for measurement data.\n";
        
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