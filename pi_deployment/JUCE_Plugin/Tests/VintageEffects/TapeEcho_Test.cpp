/*
  ==============================================================================

    TapeEcho_Test.cpp
    Comprehensive test suite for ENGINE_TAPE_ECHO
    
    Tests for tape echo characteristics:
    - Delay timing accuracy and sync precision
    - Feedback stability and coloration
    - Wow/flutter modulation characteristics
    - Tape saturation modeling accuracy
    - EQ stages (pre-emphasis, head bump, gap loss)
    - DC offset handling and safety measures
    - Real-time performance and stability
    
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
#include "../../Source/TapeEcho.h"
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
    
    // Generate impulse for delay time measurement
    static std::vector<float> generateImpulse(double amplitude, int position, 
                                            int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate sweep for frequency response testing
    static std::vector<float> generateChirp(double startFreq, double endFreq,
                                          double amplitude, double duration,
                                          double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double k = (endFreq - startFreq) / duration;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / sampleRate;
            double instantFreq = startFreq + k * t;
            double phase = 2.0 * M_PI * (startFreq * t + 0.5 * k * t * t);
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
        }
        
        return signal;
    }
    
    // Generate white noise
    static std::vector<float> generateWhiteNoise(double amplitude, double duration, 
                                                double sampleRate, unsigned int seed = 42) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
    
    // Generate multi-tone test signal
    static std::vector<float> generateMultiTone(const std::vector<double>& frequencies,
                                               const std::vector<double>& amplitudes,
                                               double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        for (size_t freqIdx = 0; freqIdx < frequencies.size() && freqIdx < amplitudes.size(); ++freqIdx) {
            double phase = 0.0;
            double phaseIncrement = 2.0 * M_PI * frequencies[freqIdx] / sampleRate;
            
            for (int i = 0; i < numSamples; ++i) {
                signal[i] += static_cast<float>(amplitudes[freqIdx] * std::sin(phase));
                phase += phaseIncrement;
                if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
            }
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
    
    // Measure delay time by finding peak cross-correlation
    static float measureDelayTime(const std::vector<float>& input,
                                 const std::vector<float>& output,
                                 double sampleRate) {
        if (input.size() != output.size() || input.size() < 512) {
            return 0.0f;
        }
        
        int maxLag = static_cast<int>(input.size() / 4); // Search up to 1/4 of signal length
        float maxCorrelation = 0.0f;
        int bestLag = 0;
        
        for (int lag = 1; lag < maxLag; ++lag) {
            double correlation = 0.0;
            int count = 0;
            
            for (int i = 0; i < static_cast<int>(input.size()) - lag; ++i) {
                correlation += input[i] * output[i + lag];
                count++;
            }
            
            if (count > 0) {
                correlation /= count;
                if (std::abs(correlation) > std::abs(maxCorrelation)) {
                    maxCorrelation = static_cast<float>(correlation);
                    bestLag = lag;
                }
            }
        }
        
        return bestLag / static_cast<float>(sampleRate) * 1000.0f; // Convert to milliseconds
    }
    
    // Measure feedback buildup over time
    static std::vector<float> measureFeedbackEvolution(const std::vector<float>& signal,
                                                      int windowSize) {
        std::vector<float> evolution;
        
        for (int i = 0; i < static_cast<int>(signal.size()) - windowSize; i += windowSize / 4) {
            float rms = 0.0f;
            for (int j = 0; j < windowSize; ++j) {
                if (i + j < static_cast<int>(signal.size())) {
                    rms += signal[i + j] * signal[i + j];
                }
            }
            rms = std::sqrt(rms / windowSize);
            evolution.push_back(20.0f * std::log10(std::max(1e-6f, rms)));
        }
        
        return evolution;
    }
    
    // Analyze wow/flutter modulation by detecting amplitude/frequency variations
    static float analyzeModulationDepth(const std::vector<float>& signal, 
                                       double sampleRate,
                                       double targetFreq = 1000.0) {
        if (signal.size() < FFT_SIZE) return 0.0f;
        
        // Window the signal for FFT
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
        auto magnitudes = SimpleFFT::magnitude(fft_result);
        
        // Find the target frequency bin
        int targetBin = static_cast<int>(targetFreq * FFT_SIZE / sampleRate);
        if (targetBin >= static_cast<int>(magnitudes.size() / 2)) return 0.0f;
        
        // Look for sidebands around the target frequency (wow/flutter signatures)
        float mainPeak = static_cast<float>(magnitudes[targetBin]);
        float totalSidebandEnergy = 0.0f;
        
        int searchRange = 10; // Look ±10 bins around main peak
        for (int offset = 1; offset <= searchRange; ++offset) {
            if (targetBin - offset >= 0) {
                totalSidebandEnergy += static_cast<float>(magnitudes[targetBin - offset]);
            }
            if (targetBin + offset < static_cast<int>(magnitudes.size()) / 2) {
                totalSidebandEnergy += static_cast<float>(magnitudes[targetBin + offset]);
            }
        }
        
        if (mainPeak > 0.0f) {
            return 20.0f * std::log10(totalSidebandEnergy / mainPeak);
        }
        
        return -120.0f;
    }
    
    // Measure tape saturation characteristics
    static float measureTHD(const std::vector<float>& signal, double sampleRate,
                           double fundamentalFreq) {
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
        auto magnitudes = SimpleFFT::magnitude(fft_result);
        
        int fundamentalBin = static_cast<int>(fundamentalFreq * FFT_SIZE / sampleRate);
        if (fundamentalBin >= static_cast<int>(magnitudes.size() / 2)) return 0.0f;
        
        float fundamentalMag = static_cast<float>(magnitudes[fundamentalBin]);
        float harmonicEnergy = 0.0f;
        
        // Sum energy of harmonics (2nd through 10th)
        for (int harmonic = 2; harmonic <= 10; ++harmonic) {
            int harmonicBin = fundamentalBin * harmonic;
            if (harmonicBin < static_cast<int>(magnitudes.size()) / 2) {
                harmonicEnergy += static_cast<float>(magnitudes[harmonicBin] * magnitudes[harmonicBin]);
            }
        }
        
        harmonicEnergy = std::sqrt(harmonicEnergy);
        
        if (fundamentalMag > 0.0f) {
            return 20.0f * std::log10(harmonicEnergy / fundamentalMag);
        }
        
        return -120.0f;
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
    
    // Analyze frequency response at specific frequency
    static float analyzeFrequencyResponse(const std::vector<float>& input,
                                        const std::vector<float>& output,
                                        double frequency, double sampleRate) {
        if (input.size() != output.size() || input.size() < FFT_SIZE) return 0.0f;
        
        // FFT both signals
        std::vector<double> input_windowed(FFT_SIZE);
        std::vector<double> output_windowed(FFT_SIZE);
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            input_windowed[i] = (i < static_cast<int>(input.size())) ? input[i] * window : 0.0;
            output_windowed[i] = (i < static_cast<int>(output.size())) ? output[i] * window : 0.0;
        }
        
        auto input_fft = SimpleFFT::fft(input_windowed);
        auto output_fft = SimpleFFT::fft(output_windowed);
        
        int targetBin = static_cast<int>(frequency * FFT_SIZE / sampleRate);
        if (targetBin >= static_cast<int>(input_fft.size()) / 2) return 0.0f;
        
        double input_mag = std::abs(input_fft[targetBin]);
        double output_mag = std::abs(output_fft[targetBin]);
        
        if (input_mag > 1e-12) {
            return 20.0f * std::log10(output_mag / input_mag);
        }
        
        return -120.0f;
    }
};

// Main test class for Tape Echo
class TapeEchoTest {
private:
    std::unique_ptr<TapeEcho> tapeEcho;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    TapeEchoTest() {
        tapeEcho = std::make_unique<TapeEcho>();
        
        // Open log files
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/VintageEffects/TapeEcho_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/VintageEffects/TapeEcho_Data.csv");
        
        if (!logFile.is_open()) {
            std::cerr << "Warning: Could not open log file for writing\n";
        }
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        // Prepare the tape echo
        tapeEcho->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Tape Echo Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Parameter Count: " + std::to_string(tapeEcho->getNumParameters()) + "\n\n");
    }
    
    ~TapeEchoTest() {
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
    
    // Process audio through tape echo
    std::pair<std::vector<float>, std::vector<float>> processAudio(const std::vector<float>& input, 
                                                                  const std::map<int, float>& parameters) {
        // Update parameters
        tapeEcho->updateParameters(parameters);
        
        std::vector<float> output;
        std::vector<float> original = input;
        output.reserve(input.size());
        
        for (size_t i = 0; i < input.size(); i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), 
                                      input.size() - i);
            
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            // Fill buffer with input (mono to stereo)
            for (size_t j = 0; j < blockSize; ++j) {
                float sample = (i + j < input.size()) ? input[i + j] : 0.0f;
                buffer.setSample(0, static_cast<int>(j), sample);
                buffer.setSample(1, static_cast<int>(j), sample);
            }
            
            // Process
            tapeEcho->process(buffer);
            
            // Extract output (left channel)
            for (size_t j = 0; j < blockSize; ++j) {
                output.push_back(buffer.getSample(0, static_cast<int>(j)));
            }
        }
        
        return {original, output};
    }
    
    // Test 1: Delay timing accuracy
    void testDelayTimingAccuracy() {
        log("\n--- Delay Timing Accuracy Tests ---\n");
        
        // Test impulse response for precise timing measurement
        auto impulseSignal = TestSignalGenerator::generateImpulse(0.5, 1000, 
                                                                 static_cast<int>(TEST_SAMPLE_RATE * 3.0));
        
        std::vector<float> delaySettings = {0.2f, 0.4f, 0.6f, 0.8f}; // Various delay times
        
        for (float delayTime : delaySettings) {
            log("\nTesting delay time setting: " + std::to_string(delayTime) + "\n");
            
            std::map<int, float> params;
            params[0] = delayTime;  // Time parameter
            params[1] = 0.0f;       // No feedback
            params[2] = 0.0f;       // No wow/flutter
            params[3] = 0.0f;       // No saturation
            params[4] = 1.0f;       // Full wet
            params[5] = 0.0f;       // No sync
            
            auto [original, output] = processAudio(impulseSignal, params);
            
            // Measure actual delay time
            float measuredDelay = AudioAnalyzer::measureDelayTime(original, output, TEST_SAMPLE_RATE);
            
            log("  Measured delay: " + std::to_string(measuredDelay) + "ms\n");
            
            logCSV("DelayTiming", "DelayTime_" + std::to_string(delayTime),
                   measuredDelay, "MEASURED", "ms");
            
            // Expected delay time (assuming 10-2000ms range)
            float expectedDelay = 10.0f + (2000.0f - 10.0f) * delayTime;
            float tolerance = expectedDelay * 0.05f; // 5% tolerance
            
            assertTrue(std::abs(measuredDelay - expectedDelay) < tolerance,
                      "Delay timing accuracy for setting " + std::to_string(delayTime));
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output for delay setting " + std::to_string(delayTime));
        }
    }
    
    // Test 2: Feedback stability and coloration
    void testFeedbackStability() {
        log("\n--- Feedback Stability Tests ---\n");
        
        // Use white noise for feedback stability testing
        auto noiseSignal = TestSignalGenerator::generateWhiteNoise(0.1, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> feedbackSettings = {0.3f, 0.6f, 0.9f, 0.95f}; // Various feedback levels
        
        for (float feedback : feedbackSettings) {
            log("\nTesting feedback setting: " + std::to_string(feedback) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;     // Moderate delay time
            params[1] = feedback; // Feedback parameter
            params[2] = 0.0f;     // No wow/flutter
            params[3] = 0.0f;     // No saturation
            params[4] = 0.8f;     // Mix
            params[5] = 0.0f;     // No sync
            
            auto [original, output] = processAudio(noiseSignal, params);
            
            // Analyze feedback evolution
            auto evolution = AudioAnalyzer::measureFeedbackEvolution(output, 
                                                                    static_cast<int>(TEST_SAMPLE_RATE * 0.1)); // 100ms windows
            
            if (!evolution.empty()) {
                float initialLevel = evolution[0];
                float finalLevel = evolution.back();
                float levelChange = finalLevel - initialLevel;
                
                log("  Initial level: " + std::to_string(initialLevel) + "dB\n");
                log("  Final level: " + std::to_string(finalLevel) + "dB\n");
                log("  Level change: " + std::to_string(levelChange) + "dB\n");
                
                logCSV("FeedbackStability", "LevelChange_" + std::to_string(feedback),
                       levelChange, "MEASURED", "dB");
                
                // Check for stability (no runaway feedback)
                assertTrue(levelChange < 6.0f, 
                          "Feedback stability at " + std::to_string(feedback));
            }
            
            // Check output validity
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with feedback " + std::to_string(feedback));
            
            // Measure peak level to ensure no clipping
            float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
            log("  Peak level: " + std::to_string(peakLevel) + "dB\n");
            
            assertTrue(peakLevel < 0.0f, 
                      "No clipping with feedback " + std::to_string(feedback));
        }
    }
    
    // Test 3: Wow/Flutter modulation characteristics
    void testWowFlutterModulation() {
        log("\n--- Wow/Flutter Modulation Tests ---\n");
        
        // Use pure tone for wow/flutter analysis
        auto toneSignal = TestSignalGenerator::generateSineWave(1000.0, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> wowFlutterSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float wowFlutter : wowFlutterSettings) {
            log("\nTesting wow/flutter setting: " + std::to_string(wowFlutter) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;        // Delay time
            params[1] = 0.0f;        // No feedback
            params[2] = wowFlutter;  // Wow/flutter parameter
            params[3] = 0.0f;        // No saturation
            params[4] = 1.0f;        // Full wet
            params[5] = 0.0f;        // No sync
            
            auto [original, output] = processAudio(toneSignal, params);
            
            // Analyze modulation depth
            float modulationDepth = AudioAnalyzer::analyzeModulationDepth(output, TEST_SAMPLE_RATE, 1000.0);
            
            log("  Modulation depth: " + std::to_string(modulationDepth) + "dB\n");
            
            logCSV("WowFlutter", "ModulationDepth_" + std::to_string(wowFlutter),
                   modulationDepth, "MEASURED", "dB");
            
            // Expect more modulation with higher settings
            if (wowFlutter > 0.5f) {
                assertTrue(modulationDepth > -40.0f, 
                          "Audible wow/flutter at setting " + std::to_string(wowFlutter));
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with wow/flutter " + std::to_string(wowFlutter));
        }
    }
    
    // Test 4: Tape saturation modeling
    void testTapeSaturation() {
        log("\n--- Tape Saturation Tests ---\n");
        
        // Use sine wave for THD measurement
        auto toneSignal = TestSignalGenerator::generateSineWave(1000.0, 0.5, 1.5, TEST_SAMPLE_RATE);
        
        std::vector<float> saturationSettings = {0.0f, 0.3f, 0.6f, 1.0f};
        
        for (float saturation : saturationSettings) {
            log("\nTesting saturation setting: " + std::to_string(saturation) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f;       // Short delay
            params[1] = 0.2f;       // Light feedback
            params[2] = 0.0f;       // No wow/flutter
            params[3] = saturation; // Saturation parameter
            params[4] = 0.8f;       // Mix
            params[5] = 0.0f;       // No sync
            
            auto [original, output] = processAudio(toneSignal, params);
            
            // Measure THD
            float thd = AudioAnalyzer::measureTHD(output, TEST_SAMPLE_RATE, 1000.0);
            
            log("  THD: " + std::to_string(thd) + "dB\n");
            
            logCSV("TapeSaturation", "THD_" + std::to_string(saturation),
                   thd, "MEASURED", "dB");
            
            // Expect higher THD with more saturation
            if (saturation > 0.7f) {
                assertTrue(thd > -60.0f, 
                          "Audible saturation at setting " + std::to_string(saturation));
            }
            
            // Check for soft limiting (no hard clipping)
            float peakLevel = AudioAnalyzer::calculatePeak_dB(output);
            assertTrue(peakLevel < 6.0f, 
                      "Soft limiting with saturation " + std::to_string(saturation));
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output with saturation " + std::to_string(saturation));
        }
    }
    
    // Test 5: EQ stages frequency response
    void testEQStagesResponse() {
        log("\n--- EQ Stages Frequency Response Tests ---\n");
        
        // Test frequencies for EQ analysis
        std::vector<double> testFreqs = {100.0, 1000.0, 3000.0, 8000.0, 15000.0};
        
        for (double freq : testFreqs) {
            log("\nTesting frequency response at " + std::to_string(freq) + "Hz\n");
            
            auto testSignal = TestSignalGenerator::generateSineWave(freq, 0.2, 1.0, TEST_SAMPLE_RATE);
            
            std::map<int, float> params;
            params[0] = 0.4f;  // Delay time
            params[1] = 0.3f;  // Feedback
            params[2] = 0.0f;  // No wow/flutter
            params[3] = 0.5f;  // Moderate saturation
            params[4] = 1.0f;  // Full wet
            params[5] = 0.0f;  // No sync
            
            auto [original, output] = processAudio(testSignal, params);
            
            // Measure frequency response
            float response = AudioAnalyzer::analyzeFrequencyResponse(original, output, freq, TEST_SAMPLE_RATE);
            
            log("  Frequency response: " + std::to_string(response) + "dB\n");
            
            logCSV("EQStages", "Response_" + std::to_string(freq) + "Hz",
                   response, "MEASURED", "dB");
            
            // Expect tape-like frequency shaping
            if (freq > 10000.0) {
                assertTrue(response < 0.0f, 
                          "High frequency rolloff at " + std::to_string(freq) + "Hz");
            }
            
            assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                      "Valid output at " + std::to_string(freq) + "Hz");
        }
    }
    
    // Test 6: DC offset handling
    void testDCOffsetHandling() {
        log("\n--- DC Offset Handling Tests ---\n");
        
        // Create signal with DC offset
        auto testSignal = TestSignalGenerator::generateSineWave(1000.0, 0.3, 1.0, TEST_SAMPLE_RATE);
        
        // Add DC offset
        for (float& sample : testSignal) {
            sample += 0.1f; // 10% DC offset
        }
        
        std::map<int, float> params;
        params[0] = 0.5f;  // Delay time
        params[1] = 0.4f;  // Feedback
        params[2] = 0.2f;  // Slight wow/flutter
        params[3] = 0.3f;  // Light saturation
        params[4] = 0.8f;  // Mix
        params[5] = 0.0f;  // No sync
        
        auto [original, output] = processAudio(testSignal, params);
        
        // Measure DC offset in output
        float inputDC = AudioAnalyzer::calculateDCOffset(original);
        float outputDC = AudioAnalyzer::calculateDCOffset(output);
        
        log("Input DC offset: " + std::to_string(inputDC) + "\n");
        log("Output DC offset: " + std::to_string(outputDC) + "\n");
        
        logCSV("DCOffset", "InputDC", inputDC, "MEASURED", "amplitude");
        logCSV("DCOffset", "OutputDC", outputDC, "MEASURED", "amplitude");
        
        // DC should be controlled/filtered
        assertTrue(std::abs(outputDC) < std::abs(inputDC) + 0.05f, 
                  "DC offset controlled");
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                  "Valid output with DC offset");
    }
    
    // Test 7: Transport sync functionality
    void testTransportSync() {
        log("\n--- Transport Sync Tests ---\n");
        
        // Set up transport info
        EngineBase::TransportInfo transport;
        transport.bpm = 120.0;
        transport.timeSigNumerator = 4.0;
        transport.timeSigDenominator = 4.0;
        transport.isPlaying = true;
        
        tapeEcho->setTransportInfo(transport);
        
        auto testSignal = TestSignalGenerator::generateImpulse(0.5, 1000, 
                                                              static_cast<int>(TEST_SAMPLE_RATE * 2.0));
        
        // Test with sync enabled
        std::map<int, float> params;
        params[0] = 0.5f;  // Time parameter (should be overridden by sync)
        params[1] = 0.0f;  // No feedback
        params[2] = 0.0f;  // No wow/flutter
        params[3] = 0.0f;  // No saturation
        params[4] = 1.0f;  // Full wet
        params[5] = 1.0f;  // Sync enabled
        
        auto [original, output] = processAudio(testSignal, params);
        
        // Measure synced delay time
        float syncedDelay = AudioAnalyzer::measureDelayTime(original, output, TEST_SAMPLE_RATE);
        
        log("Synced delay time: " + std::to_string(syncedDelay) + "ms\n");
        
        logCSV("TransportSync", "SyncedDelayTime", syncedDelay, "MEASURED", "ms");
        
        // Calculate expected delay for quarter note at 120 BPM
        float expectedSyncDelay = 60000.0f / (120.0f * 1.0f); // Quarter note = 500ms at 120 BPM
        float tolerance = expectedSyncDelay * 0.1f; // 10% tolerance
        
        assertTrue(std::abs(syncedDelay - expectedSyncDelay) < tolerance,
                  "Transport sync accuracy");
        
        assertTrue(!AudioAnalyzer::hasInvalidValues(output),
                  "Valid output with transport sync");
    }
    
    // Test 8: Performance and stability
    void testPerformanceAndStability() {
        log("\n--- Performance and Stability Tests ---\n");
        
        // Generate longer test signal
        auto longSignal = TestSignalGenerator::generateMultiTone({440.0, 880.0, 1320.0}, 
                                                                {0.2, 0.2, 0.2}, 
                                                                5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        params[0] = 0.6f;  // Delay time
        params[1] = 0.5f;  // Feedback
        params[2] = 0.4f;  // Wow/flutter
        params[3] = 0.3f;  // Saturation
        params[4] = 0.7f;  // Mix
        params[5] = 0.0f;  // No sync
        
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
        assertTrue(!AudioAnalyzer::hasInvalidValues(output), "Valid output during stress test");
        assertTrue(realTimeRatio < 0.5, "Real-time processing capability");
        
        float outputStability = AudioAnalyzer::calculateRMS_dB(output);
        assertTrue(outputStability > -80.0f, "Stable output level");
        
        log("Output stability: " + std::to_string(outputStability) + "dB\n");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Tape Echo comprehensive test suite...\n");
        
        testDelayTimingAccuracy();
        testFeedbackStability();
        testWowFlutterModulation();
        testTapeSaturation();
        testEQStagesResponse();
        testDCOffsetHandling();
        testTransportSync();
        testPerformanceAndStability();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        TapeEchoTest tester;
        tester.runAllTests();
        
        std::cout << "\nTape Echo test suite completed successfully.\n";
        std::cout << "Check TapeEcho_TestResults.txt for detailed results.\n";
        std::cout << "Check TapeEcho_Data.csv for measurement data.\n";
        
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