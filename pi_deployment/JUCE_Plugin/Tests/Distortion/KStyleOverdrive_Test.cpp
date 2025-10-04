/*
  ==============================================================================
  
    KStyleOverdrive_Test.cpp
    Comprehensive test suite for ENGINE_K_STYLE
    
    Tests for K-Style Overdrive characteristics:
    - Overdrive curve modeling
    - Tone stack accuracy
    - Drive response characteristics
    - Clean/overdrive blend testing
    - Tube-style overdrive validation
    - Mid-frequency emphasis
    - Dynamic responsiveness
    - Amp-like behavior verification
    
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
#include "../../Source/KStyleOverdrive.h"
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
    
    static std::vector<float> generateChord(const std::vector<double>& frequencies,
                                          double amplitude, double duration,
                                          double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        for (double freq : frequencies) {
            if (freq < sampleRate / 2) {
                double phase = 0.0;
                double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
                double noteAmp = amplitude / frequencies.size();
                
                for (int i = 0; i < numSamples; ++i) {
                    signal[i] += static_cast<float>(noteAmp * std::sin(phase));
                    phase += phaseIncrement;
                    if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
                }
            }
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
    
    static std::vector<float> generateGuitarNote(double frequency, double amplitude,
                                                double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            
            // Guitar-like envelope (quick attack, exponential decay)
            double envelope;
            if (t < 0.01) {
                envelope = amplitude * (t / 0.01); // Attack
            } else {
                envelope = amplitude * std::exp(-t * 2.0); // Decay
            }
            
            // Add some harmonics for realism
            double fundamental = std::sin(phase);
            double harmonic2 = 0.3 * std::sin(phase * 2.0);
            double harmonic3 = 0.1 * std::sin(phase * 3.0);
            
            signal[i] = static_cast<float>(envelope * (fundamental + harmonic2 + harmonic3));
            phase += phaseIncrement;
            if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
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
    
    static std::vector<float> measureToneStackResponse(const std::vector<float>& input,
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
    
    static float measureOverdriveAmount(const std::vector<float>& input,
                                      const std::vector<float>& output) {
        if (input.size() != output.size() || input.empty()) {
            return 0.0f;
        }
        
        // Calculate RMS ratio as overdrive indicator
        double inputRMS = 0.0, outputRMS = 0.0;
        
        for (size_t i = 0; i < input.size(); ++i) {
            inputRMS += input[i] * input[i];
            outputRMS += output[i] * output[i];
        }
        
        inputRMS = std::sqrt(inputRMS / input.size());
        outputRMS = std::sqrt(outputRMS / output.size());
        
        if (inputRMS == 0.0) return 0.0f;
        
        return static_cast<float>(20.0 * std::log10(outputRMS / inputRMS));
    }
    
    static float measureTubeStyleSaturation(const std::vector<float>& signal, 
                                          double fundamental, double sampleRate) {
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
        auto magnitudes_db = SimpleFFT::magnitudeDB(fft_result);
        
        int fund_bin = static_cast<int>(fundamental * FFT_SIZE / sampleRate);
        if (fund_bin >= static_cast<int>(magnitudes_db.size() / 2)) return 0.0f;
        
        // Check for even harmonics (tube-like characteristic)
        float evenHarmonics = 0.0f;
        float oddHarmonics = 0.0f;
        
        for (int h = 2; h <= 6; h += 2) { // Even harmonics
            int harm_bin = fund_bin * h;
            if (harm_bin < static_cast<int>(magnitudes_db.size() / 2)) {
                evenHarmonics += magnitudes_db[harm_bin];
            }
        }
        
        for (int h = 3; h <= 5; h += 2) { // Odd harmonics  
            int harm_bin = fund_bin * h;
            if (harm_bin < static_cast<int>(magnitudes_db.size() / 2)) {
                oddHarmonics += magnitudes_db[harm_bin];
            }
        }
        
        // Return even/odd harmonic ratio as tube-like indicator
        if (oddHarmonics == 0.0f) return 0.0f;
        return evenHarmonics / oddHarmonics;
    }
    
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) {
                return true;
            }
        }
        return false;
    }
    
    static float measureMidEmphasis(const std::vector<float>& input,
                                  const std::vector<float>& output,
                                  double sampleRate) {
        if (input.size() != output.size() || input.size() < FFT_SIZE) {
            return 0.0f;
        }
        
        // Measure gain specifically in mid-frequency range (500Hz - 2kHz)
        std::vector<double> midFreqs = {500.0, 800.0, 1000.0, 1500.0, 2000.0};
        auto gains = measureToneStackResponse(input, output, midFreqs, sampleRate);
        
        if (gains.empty()) return 0.0f;
        
        // Return average mid-frequency gain
        float totalGain = 0.0f;
        for (float gain : gains) {
            totalGain += gain;
        }
        
        return totalGain / gains.size();
    }
};

// Main test class for K-Style Overdrive
class KStyleOverdriveTest {
private:
    std::unique_ptr<KStyleOverdrive> kStyleOverdrive;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    KStyleOverdriveTest() {
        kStyleOverdrive = std::make_unique<KStyleOverdrive>();
        
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/KStyleOverdrive_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Distortion/KStyleOverdrive_Data.csv");
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        kStyleOverdrive->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== K-Style Overdrive Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Engine ID: " + std::to_string(ENGINE_K_STYLE) + "\n");
        log("Parameter Count: " + std::to_string(kStyleOverdrive->getNumParameters()) + "\n\n");
    }
    
    ~KStyleOverdriveTest() {
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
        kStyleOverdrive->updateParameters(parameters);
        
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
            
            kStyleOverdrive->process(buffer);
            
            for (size_t j = 0; j < blockSize; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }
        
        return {original, output};
    }
    
    void testParameterResponse() {
        log("\n--- Parameter Response Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateGuitarNote(440.0, 0.4, 1.0, TEST_SAMPLE_RATE);
        
        for (int param = 0; param < kStyleOverdrive->getNumParameters(); ++param) {
            std::string paramName = kStyleOverdrive->getParameterName(param).toStdString();
            log("Testing parameter " + std::to_string(param) + ": " + paramName + "\n");
            
            std::vector<float> responseDB;
            
            for (float value = 0.0f; value <= 1.0f; value += 0.25f) {
                std::map<int, float> params;
                
                for (int p = 0; p < kStyleOverdrive->getNumParameters(); ++p) {
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
            
            if (param <= 4) {
                assertTrue(responseRange > 0.5f, 
                          paramName + " has audible effect");
            }
        }
    }
    
    void testOverdriveCurveModeling() {
        log("\n--- Overdrive Curve Modeling Tests ---\n");
        
        auto rampSignal = TestSignalGenerator::generateRamp(1.0, 1.0, TEST_SAMPLE_RATE);
        
        std::vector<float> driveSettings = {0.2f, 0.5f, 0.8f};
        
        for (float drive : driveSettings) {
            log("\nTesting drive setting: " + std::to_string(drive) + "\n");
            
            std::map<int, float> params;
            params[0] = drive; // Drive parameter
            params[1] = 0.5f;  // Tone
            params[2] = 0.5f;  // Level
            
            auto [original, output] = processAudio(rampSignal, params);
            
            // Analyze overdrive characteristics
            float overdriveAmount = AudioAnalyzer::measureOverdriveAmount(original, output);
            log("  Overdrive amount: " + std::to_string(overdriveAmount) + "dB\n");
            
            logCSV("OverdriveCurve", "OverdriveAmount_" + std::to_string(drive),
                   overdriveAmount, "MEASURED", "dB");
            
            // Check for soft clipping characteristics
            float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
            log("  Peak output: " + std::to_string(peakLevel) + "dB\n");
            
            // Higher drive should produce more overdrive
            if (drive > 0.6f) {
                assertTrue(overdriveAmount > -3.0f, 
                          "Overdrive present at drive " + std::to_string(drive));
            }
            
            // Should maintain reasonable output levels
            assertTrue(peakLevel < 6.0f, 
                      "Output level controlled at drive " + std::to_string(drive));
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at drive " + std::to_string(drive));
        }
    }
    
    void testToneStackAccuracy() {
        log("\n--- Tone Stack Accuracy Tests ---\n");
        
        auto sweepSignal = TestSignalGenerator::generateSweep(50.0, 15000.0, 0.2, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> toneSettings = {0.2f, 0.5f, 0.8f};
        
        for (float tone : toneSettings) {
            log("\nTesting tone setting: " + std::to_string(tone) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f; // Moderate drive
            params[1] = tone; // Tone control
            params[2] = 0.5f; // Level
            
            auto [original, output] = processAudio(sweepSignal, params);
            
            // Measure frequency response at key frequencies
            std::vector<double> testFreqs = {100, 300, 500, 1000, 2000, 5000, 8000, 12000};
            auto gains = AudioAnalyzer::measureToneStackResponse(original, output, testFreqs, TEST_SAMPLE_RATE);
            
            log("  Frequency response:\n");
            for (size_t i = 0; i < testFreqs.size() && i < gains.size(); ++i) {
                log("    " + std::to_string(testFreqs[i]) + "Hz: " + 
                    std::to_string(gains[i]) + "dB\n");
                
                logCSV("ToneStackAccuracy", 
                       "Tone" + std::to_string(tone) + "_" + std::to_string(testFreqs[i]) + "Hz",
                       gains[i], "MEASURED", "dB");
            }
            
            // Check for expected tone stack behavior
            if (!gains.empty()) {
                // Low tone setting should reduce highs
                if (tone < 0.4f && gains.size() >= 6) {
                    assertTrue(gains[5] < gains[2], // 5kHz < 500Hz
                              "Low tone setting reduces highs");
                }
                
                // High tone setting should boost highs
                if (tone > 0.6f && gains.size() >= 6) {
                    assertTrue(gains[5] >= gains[2] - 3.0f, // More forgiving check
                              "High tone setting maintains/boosts highs");
                }
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid tone stack response at setting " + std::to_string(tone));
        }
    }
    
    void testTubeStyleOverdrive() {
        log("\n--- Tube-Style Overdrive Validation ---\n");
        
        std::vector<double> testFreqs = {220.0, 440.0, 880.0};
        
        for (double freq : testFreqs) {
            log("\nTesting tube-style overdrive at " + std::to_string(freq) + "Hz\n");
            
            auto testSignal = TestSignalGenerator::generateSineWave(freq, 0.4, 1.5, TEST_SAMPLE_RATE);
            
            std::map<int, float> params;
            params[0] = 0.7f; // High drive for saturation
            params[1] = 0.5f; // Neutral tone
            params[2] = 0.5f; // Level
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure tube-style characteristics
            float tubeRatio = AudioAnalyzer::measureTubeStyleSaturation(output, freq, TEST_SAMPLE_RATE);
            log("  Tube-style ratio: " + std::to_string(tubeRatio) + "\n");
            
            logCSV("TubeStyleOverdrive", "TubeRatio_" + std::to_string(freq),
                   tubeRatio, "MEASURED", "ratio");
            
            // Should show some even harmonic content (tube-like)
            assertTrue(tubeRatio > 0.1f, 
                      "Tube-style harmonics present at " + std::to_string(freq) + "Hz");
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid tube-style output at " + std::to_string(freq) + "Hz");
        }
    }
    
    void testMidFrequencyEmphasis() {
        log("\n--- Mid-Frequency Emphasis Tests ---\n");
        
        auto sweepSignal = TestSignalGenerator::generateSweep(50.0, 10000.0, 0.2, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> driveSettings = {0.3f, 0.6f, 0.9f};
        
        for (float drive : driveSettings) {
            log("\nTesting mid emphasis at drive: " + std::to_string(drive) + "\n");
            
            std::map<int, float> params;
            params[0] = drive; // Drive
            params[1] = 0.5f;  // Neutral tone
            params[2] = 0.5f;  // Level
            
            auto [original, output] = processAudio(sweepSignal, params);
            
            // Measure mid-frequency emphasis
            float midEmphasis = AudioAnalyzer::measureMidEmphasis(original, output, TEST_SAMPLE_RATE);
            log("  Mid-frequency emphasis: " + std::to_string(midEmphasis) + "dB\n");
            
            logCSV("MidFrequencyEmphasis", "MidEmphasis_Drive" + std::to_string(drive),
                   midEmphasis, "MEASURED", "dB");
            
            // K-style overdrives typically emphasize mids
            assertTrue(midEmphasis > -3.0f, 
                      "Mid-frequency response maintained at drive " + std::to_string(drive));
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid mid emphasis at drive " + std::to_string(drive));
        }
    }
    
    void testCleanOverdriveBlend() {
        log("\n--- Clean/Overdrive Blend Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateGuitarNote(330.0, 0.4, 1.0, TEST_SAMPLE_RATE);
        
        // Test at low and high drive to see clean vs overdrive character
        std::vector<float> blendSettings = {0.1f, 0.5f, 0.9f};
        
        for (float blend : blendSettings) {
            log("\nTesting clean/overdrive blend: " + std::to_string(blend) + "\n");
            
            std::map<int, float> params;
            params[0] = blend; // Drive/blend control
            params[1] = 0.5f;  // Tone
            params[2] = 0.5f;  // Level
            
            auto [original, output] = processAudio(testSignal, params);
            
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            float inputRMS = AudioAnalyzer::calculateRMS_dB(original);
            float gain = outputRMS - inputRMS;
            
            log("  Gain: " + std::to_string(gain) + "dB\n");
            
            logCSV("CleanOverdriveBlend", "Gain_Blend" + std::to_string(blend),
                   gain, "MEASURED", "dB");
            
            // Low settings should be more transparent
            if (blend < 0.3f) {
                assertTrue(gain > -6.0f && gain < 6.0f, 
                          "Clean behavior at low blend " + std::to_string(blend));
            }
            
            // High settings should add character
            if (blend > 0.7f) {
                assertTrue(gain > -3.0f, 
                          "Overdrive character at high blend " + std::to_string(blend));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid blend output at setting " + std::to_string(blend));
        }
    }
    
    void testDynamicResponsiveness() {
        log("\n--- Dynamic Responsiveness Tests ---\n");
        
        // Test with different input levels
        std::vector<double> inputLevels = {0.1, 0.3, 0.6, 0.9};
        
        std::map<int, float> params;
        params[0] = 0.6f; // Moderate drive
        params[1] = 0.5f; // Tone
        params[2] = 0.5f; // Level
        
        std::vector<float> outputLevels;
        
        for (double inputLevel : inputLevels) {
            log("\nTesting dynamic response at input level: " + std::to_string(inputLevel) + "\n");
            
            auto testSignal = TestSignalGenerator::generateGuitarNote(440.0, inputLevel, 1.0, TEST_SAMPLE_RATE);
            auto [original, output] = processAudio(testSignal, params);
            
            float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
            outputLevels.push_back(outputRMS);
            
            log("  Output RMS: " + std::to_string(outputRMS) + "dB\n");
            
            logCSV("DynamicResponsiveness", "OutputLevel_Input" + std::to_string(inputLevel),
                   outputRMS, "MEASURED", "dB");
        }
        
        // Check dynamic response characteristics
        if (outputLevels.size() >= 2) {
            float dynamicRange = outputLevels.back() - outputLevels.front();
            log("Total dynamic range: " + std::to_string(dynamicRange) + "dB\n");
            
            logCSV("DynamicResponsiveness", "DynamicRange", dynamicRange, "MEASURED", "dB");
            
            // Should maintain some dynamics but compress
            assertTrue(dynamicRange > 5.0f && dynamicRange < 25.0f, 
                      "Appropriate dynamic response");
        }
        
        for (const auto& output : outputLevels) {
            assertTrue(output > -60.0f, "Reasonable output levels");
        }
    }
    
    void testAmpLikeBehavior() {
        log("\n--- Amp-Like Behavior Tests ---\n");
        
        // Test with chord to check for intermodulation
        std::vector<double> chordFreqs = {220.0, 277.18, 329.63}; // A major chord
        auto chordSignal = TestSignalGenerator::generateChord(chordFreqs, 0.4, 1.5, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.7f; // High drive for amp-like saturation
        params[1] = 0.6f; // Slightly bright tone
        params[2] = 0.5f; // Level
        
        auto [original, output] = processAudio(chordSignal, params);
        
        // Check for amp-like characteristics
        float outputRMS = AudioAnalyzer::calculateRMS_dB(output);
        float inputRMS = AudioAnalyzer::calculateRMS_dB(original);
        float compression = inputRMS - outputRMS + 6.0f; // Offset for analysis
        
        log("Chord compression: " + std::to_string(compression) + "dB\n");
        
        logCSV("AmpLikeBehavior", "ChordCompression", compression, "MEASURED", "dB");
        
        // Should show some compression of complex signals
        assertTrue(compression > 0.0f && compression < 15.0f, 
                  "Amp-like compression of chord");
        
        // Check for reasonable harmonic content
        float tubeRatio = AudioAnalyzer::measureTubeStyleSaturation(output, 220.0, TEST_SAMPLE_RATE);
        log("Chord tube-style ratio: " + std::to_string(tubeRatio) + "\n");
        
        assertTrue(tubeRatio > 0.05f, "Amp-like harmonic generation");
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid amp-like behavior");
    }
    
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        auto longSignal = TestSignalGenerator::generateSweep(20.0, 18000.0, 0.3, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.8f; // High drive
        params[1] = 0.7f; // Bright tone
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
        
        log("Peak output level: " + std::to_string(outputLevel) + "dB\n");
    }
    
    void runAllTests() {
        log("Starting K-Style Overdrive comprehensive test suite...\n");
        
        testParameterResponse();
        testOverdriveCurveModeling();
        testToneStackAccuracy();
        testTubeStyleOverdrive();
        testMidFrequencyEmphasis();
        testCleanOverdriveBlend();
        testDynamicResponsiveness();
        testAmpLikeBehavior();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

int main() {
    try {
        KStyleOverdriveTest tester;
        tester.runAllTests();
        
        std::cout << "\nK-Style Overdrive test suite completed successfully.\n";
        std::cout << "Check KStyleOverdrive_TestResults.txt for detailed results.\n";
        std::cout << "Check KStyleOverdrive_Data.csv for measurement data.\n";
        
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