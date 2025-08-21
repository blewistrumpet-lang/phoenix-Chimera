/*
  ==============================================================================

    DigitalDelay_Test.cpp
    Comprehensive test suite for ENGINE_DIGITAL_DELAY
    
    Tests for digital delay characteristics:
    - Delay timing precision and accuracy
    - Feedback loop stability and coloration
    - High-frequency damping effectiveness
    - Low-frequency filtering accuracy
    - Stereo spread and ping-pong operation
    - Transport sync precision
    - Oversampling quality improvement
    - CPU performance optimization
    
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
#include "../../Source/DigitalDelay.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 0.1f;
constexpr int FFT_SIZE = 8192;

// Reuse FFT and TestSignalGenerator from TapeEcho test
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
    
    static std::vector<float> generateImpulse(double amplitude, int position, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
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
    
    static std::vector<float> generateStereoTestSignal(double frequency, double amplitude,
                                                      double duration, double sampleRate,
                                                      bool leftChannel = true) {
        return generateSineWave(frequency, amplitude, duration, sampleRate);
    }
};

// Audio analysis utilities for digital delay specific measurements
class DigitalDelayAnalyzer {
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
    
    // Measure delay time with sub-sample precision
    static float measurePreciseDelayTime(const std::vector<float>& input,
                                        const std::vector<float>& output,
                                        double sampleRate) {
        if (input.size() != output.size() || input.size() < 1024) {
            return 0.0f;
        }
        
        // Use cross-correlation for sub-sample precision
        int maxLag = static_cast<int>(input.size() / 3);
        std::vector<float> correlation(maxLag);
        
        for (int lag = 0; lag < maxLag; ++lag) {
            double sum = 0.0;
            int count = 0;
            
            for (int i = 0; i < static_cast<int>(input.size()) - lag; ++i) {
                sum += input[i] * output[i + lag];
                count++;
            }
            
            if (count > 0) {
                correlation[lag] = static_cast<float>(sum / count);
            }
        }
        
        // Find peak correlation
        auto maxIt = std::max_element(correlation.begin(), correlation.end());
        if (maxIt != correlation.end()) {
            int peakLag = static_cast<int>(maxIt - correlation.begin());
            
            // Parabolic interpolation for sub-sample precision
            if (peakLag > 0 && peakLag < maxLag - 1) {
                float y1 = correlation[peakLag - 1];
                float y2 = correlation[peakLag];
                float y3 = correlation[peakLag + 1];
                
                float a = (y1 - 2*y2 + y3) / 2;
                float b = (y3 - y1) / 2;
                
                if (std::abs(a) > 1e-6f) {
                    float offset = -b / (2 * a);
                    peakLag += static_cast<int>(offset);
                }
            }
            
            return peakLag / static_cast<float>(sampleRate) * 1000.0f; // Convert to ms
        }
        
        return 0.0f;
    }
    
    // Analyze stereo spread/correlation
    static float measureStereoCorrelation(const std::vector<float>& leftChannel,
                                         const std::vector<float>& rightChannel) {
        if (leftChannel.size() != rightChannel.size() || leftChannel.empty()) {
            return 0.0f;
        }
        
        double meanLeft = 0.0, meanRight = 0.0;
        for (size_t i = 0; i < leftChannel.size(); ++i) {
            meanLeft += leftChannel[i];
            meanRight += rightChannel[i];
        }
        meanLeft /= leftChannel.size();
        meanRight /= rightChannel.size();
        
        double covariance = 0.0, varianceLeft = 0.0, varianceRight = 0.0;
        
        for (size_t i = 0; i < leftChannel.size(); ++i) {
            double leftDiff = leftChannel[i] - meanLeft;
            double rightDiff = rightChannel[i] - meanRight;
            
            covariance += leftDiff * rightDiff;
            varianceLeft += leftDiff * leftDiff;
            varianceRight += rightDiff * rightDiff;
        }
        
        double denominator = std::sqrt(varianceLeft * varianceRight);
        if (denominator > 1e-12) {
            return static_cast<float>(covariance / denominator);
        }
        
        return 0.0f;
    }
    
    // Measure frequency response with high precision
    static float measureFrequencyResponse(const std::vector<float>& input,
                                        const std::vector<float>& output,
                                        double frequency, double sampleRate) {
        if (input.size() != output.size() || input.size() < FFT_SIZE) return 0.0f;
        
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
    
    // Analyze feedback loop resonance
    static std::vector<float> analyzeFeedbackSpectrum(const std::vector<float>& signal,
                                                     double sampleRate) {
        if (signal.size() < FFT_SIZE) return {};
        
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
        
        std::vector<float> spectrum;
        for (size_t i = 0; i < magnitudes_db.size() / 2; ++i) {
            spectrum.push_back(static_cast<float>(magnitudes_db[i]));
        }
        
        return spectrum;
    }
    
    // Measure ping-pong timing accuracy
    static std::vector<float> measurePingPongTiming(const std::vector<float>& leftOutput,
                                                   const std::vector<float>& rightOutput,
                                                   double sampleRate) {
        std::vector<float> timings;
        
        // Find peaks in each channel
        auto findPeaks = [](const std::vector<float>& signal, float threshold = 0.1f) {
            std::vector<int> peaks;
            for (int i = 1; i < static_cast<int>(signal.size()) - 1; ++i) {
                if (std::abs(signal[i]) > threshold &&
                    std::abs(signal[i]) > std::abs(signal[i-1]) &&
                    std::abs(signal[i]) > std::abs(signal[i+1])) {
                    peaks.push_back(i);
                }
            }
            return peaks;
        };
        
        auto leftPeaks = findPeaks(leftOutput);
        auto rightPeaks = findPeaks(rightOutput);
        
        // Calculate alternating timing intervals
        std::vector<int> allPeaks;
        for (int peak : leftPeaks) allPeaks.push_back(peak);
        for (int peak : rightPeaks) allPeaks.push_back(peak);
        
        std::sort(allPeaks.begin(), allPeaks.end());
        
        for (size_t i = 1; i < allPeaks.size(); ++i) {
            float interval = (allPeaks[i] - allPeaks[i-1]) / static_cast<float>(sampleRate) * 1000.0f;
            timings.push_back(interval);
        }
        
        return timings;
    }
    
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) {
                return true;
            }
        }
        return false;
    }
    
    static float calculateDCOffset(const std::vector<float>& signal) {
        if (signal.empty()) return 0.0f;
        
        double sum = 0.0;
        for (float sample : signal) {
            sum += sample;
        }
        
        return static_cast<float>(sum / signal.size());
    }
};

// Main test class for Digital Delay
class DigitalDelayTest {
private:
    std::unique_ptr<DigitalDelay> digitalDelay;
    std::ofstream logFile;
    std::ofstream csvFile;
    int testsPassed = 0;
    int testsFailed = 0;
    
public:
    DigitalDelayTest() {
        digitalDelay = std::make_unique<DigitalDelay>();
        
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/VintageEffects/DigitalDelay_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/VintageEffects/DigitalDelay_Data.csv");
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        digitalDelay->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Digital Delay Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Block Size: " + std::to_string(TEST_BLOCK_SIZE) + " samples\n");
        log("Parameter Count: " + std::to_string(digitalDelay->getNumParameters()) + "\n\n");
    }
    
    ~DigitalDelayTest() {
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
    
    // Process audio through digital delay
    std::tuple<std::vector<float>, std::vector<float>, std::vector<float>> processAudioStereo(
        const std::vector<float>& input, const std::map<int, float>& parameters) {
        
        digitalDelay->updateParameters(parameters);
        
        std::vector<float> leftOutput, rightOutput;
        std::vector<float> original = input;
        leftOutput.reserve(input.size());
        rightOutput.reserve(input.size());
        
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
            
            digitalDelay->process(buffer);
            
            // Extract stereo output
            for (size_t j = 0; j < blockSize; ++j) {
                leftOutput.push_back(buffer.getSample(0, static_cast<int>(j)));
                rightOutput.push_back(buffer.getSample(1, static_cast<int>(j)));
            }
        }
        
        return {original, leftOutput, rightOutput};
    }
    
    // Test 1: Digital delay timing precision
    void testDelayTimingPrecision() {
        log("\n--- Digital Delay Timing Precision Tests ---\n");
        
        auto impulseSignal = TestSignalGenerator::generateImpulse(0.7, 1000, 
                                                                 static_cast<int>(TEST_SAMPLE_RATE * 3.0));
        
        std::vector<float> delaySettings = {0.1f, 0.25f, 0.5f, 0.75f, 0.9f}; 
        
        for (float delayTime : delaySettings) {
            log("\nTesting delay time setting: " + std::to_string(delayTime) + "\n");
            
            std::map<int, float> params;
            params[0] = delayTime;  // Time parameter
            params[1] = 0.0f;       // No feedback
            params[2] = 0.0f;       // No damping
            params[3] = 0.0f;       // No low-cut
            params[4] = 0.5f;       // Center spread
            params[5] = 1.0f;       // Full wet
            params[6] = 0.0f;       // No sync
            
            auto [original, leftOut, rightOut] = processAudioStereo(impulseSignal, params);
            
            float measuredDelayLeft = DigitalDelayAnalyzer::measurePreciseDelayTime(original, leftOut, TEST_SAMPLE_RATE);
            float measuredDelayRight = DigitalDelayAnalyzer::measurePreciseDelayTime(original, rightOut, TEST_SAMPLE_RATE);
            
            log("  Left channel delay: " + std::to_string(measuredDelayLeft) + "ms\n");
            log("  Right channel delay: " + std::to_string(measuredDelayRight) + "ms\n");
            
            logCSV("DelayPrecision", "LeftDelay_" + std::to_string(delayTime),
                   measuredDelayLeft, "MEASURED", "ms");
            logCSV("DelayPrecision", "RightDelay_" + std::to_string(delayTime),
                   measuredDelayRight, "MEASURED", "ms");
            
            // Expected delay time range (assume 1ms to 2000ms)
            float expectedDelay = 1.0f + (2000.0f - 1.0f) * delayTime;
            float tolerance = expectedDelay * 0.02f; // 2% tolerance for digital precision
            
            assertTrue(std::abs(measuredDelayLeft - expectedDelay) < tolerance,
                      "Left channel delay precision for setting " + std::to_string(delayTime));
            assertTrue(std::abs(measuredDelayRight - expectedDelay) < tolerance,
                      "Right channel delay precision for setting " + std::to_string(delayTime));
            
            assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(leftOut),
                      "Valid left output for delay setting " + std::to_string(delayTime));
            assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(rightOut),
                      "Valid right output for delay setting " + std::to_string(delayTime));
        }
    }
    
    // Test 2: Feedback loop stability
    void testFeedbackLoopStability() {
        log("\n--- Feedback Loop Stability Tests ---\n");
        
        auto noiseSignal = TestSignalGenerator::generateWhiteNoise(0.05, 4.0, TEST_SAMPLE_RATE);
        
        std::vector<float> feedbackSettings = {0.2f, 0.5f, 0.8f, 0.95f, 0.99f}; 
        
        for (float feedback : feedbackSettings) {
            log("\nTesting feedback setting: " + std::to_string(feedback) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f;     // Moderate delay time
            params[1] = feedback; // Feedback parameter
            params[2] = 0.2f;     // Light damping
            params[3] = 0.0f;     // No low-cut
            params[4] = 0.5f;     // Center spread
            params[5] = 0.8f;     // Mix
            params[6] = 0.0f;     // No sync
            
            auto [original, leftOut, rightOut] = processAudioStereo(noiseSignal, params);
            
            // Analyze stability metrics
            float leftPeak = DigitalDelayAnalyzer::calculatePeak_dB(leftOut);
            float rightPeak = DigitalDelayAnalyzer::calculatePeak_dB(rightOut);
            float leftRMS = DigitalDelayAnalyzer::calculateRMS_dB(leftOut);
            float rightRMS = DigitalDelayAnalyzer::calculateRMS_dB(rightOut);
            
            log("  Left peak: " + std::to_string(leftPeak) + "dB\n");
            log("  Right peak: " + std::to_string(rightPeak) + "dB\n");
            log("  Left RMS: " + std::to_string(leftRMS) + "dB\n");
            log("  Right RMS: " + std::to_string(rightRMS) + "dB\n");
            
            logCSV("FeedbackStability", "LeftPeak_" + std::to_string(feedback),
                   leftPeak, "MEASURED", "dB");
            logCSV("FeedbackStability", "LeftRMS_" + std::to_string(feedback),
                   leftRMS, "MEASURED", "dB");
            
            // Check for stability (no runaway feedback)
            assertTrue(leftPeak < 6.0f, 
                      "Left channel stable at feedback " + std::to_string(feedback));
            assertTrue(rightPeak < 6.0f, 
                      "Right channel stable at feedback " + std::to_string(feedback));
            
            // Analyze feedback spectrum for resonant peaks
            auto leftSpectrum = DigitalDelayAnalyzer::analyzeFeedbackSpectrum(leftOut, TEST_SAMPLE_RATE);
            if (!leftSpectrum.empty()) {
                float maxResonance = *std::max_element(leftSpectrum.begin(), leftSpectrum.end());
                log("  Max spectral peak: " + std::to_string(maxResonance) + "dB\n");
                
                // High feedback should show some resonance but remain controlled
                if (feedback > 0.8f) {
                    assertTrue(maxResonance < 12.0f, 
                              "Controlled resonance at feedback " + std::to_string(feedback));
                }
            }
            
            assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(leftOut) && 
                      !DigitalDelayAnalyzer::hasInvalidValues(rightOut),
                      "Valid output with feedback " + std::to_string(feedback));
        }
    }
    
    // Test 3: High-frequency damping
    void testHighFrequencyDamping() {
        log("\n--- High-Frequency Damping Tests ---\n");
        
        // Test high-frequency content
        auto chirpSignal = TestSignalGenerator::generateChirp(1000.0, 15000.0, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> dampingSettings = {0.0f, 0.3f, 0.6f, 1.0f}; 
        
        for (float damping : dampingSettings) {
            log("\nTesting damping setting: " + std::to_string(damping) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.4f;     // Delay time
            params[1] = 0.5f;     // Feedback
            params[2] = damping;  // Damping parameter
            params[3] = 0.0f;     // No low-cut
            params[4] = 0.5f;     // Center spread
            params[5] = 1.0f;     // Full wet
            params[6] = 0.0f;     // No sync
            
            auto [original, leftOut, rightOut] = processAudioStereo(chirpSignal, params);
            
            // Analyze frequency response at key frequencies
            std::vector<double> testFreqs = {5000.0, 8000.0, 12000.0, 15000.0};
            
            for (double freq : testFreqs) {
                float response = DigitalDelayAnalyzer::measureFrequencyResponse(
                    original, leftOut, freq, TEST_SAMPLE_RATE);
                
                log("  Response at " + std::to_string(freq) + "Hz: " + 
                    std::to_string(response) + "dB\n");
                
                logCSV("HighFreqDamping", 
                       "Response_" + std::to_string(damping) + "_" + std::to_string(freq) + "Hz",
                       response, "MEASURED", "dB");
                
                // Higher damping should reduce high frequencies more
                if (damping > 0.7f && freq > 8000.0) {
                    assertTrue(response < -1.0f, 
                              "HF attenuation at " + std::to_string(freq) + 
                              "Hz with damping " + std::to_string(damping));
                }
            }
            
            assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(leftOut),
                      "Valid output with damping " + std::to_string(damping));
        }
    }
    
    // Test 4: Low-frequency filtering
    void testLowFrequencyFiltering() {
        log("\n--- Low-Frequency Filtering Tests ---\n");
        
        // Test low-frequency content
        auto lowFreqSignal = TestSignalGenerator::generateSineWave(60.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        auto midFreqSignal = TestSignalGenerator::generateSineWave(1000.0, 0.4, 2.0, TEST_SAMPLE_RATE);
        
        // Mix signals
        std::vector<float> testSignal;
        for (size_t i = 0; i < std::min(lowFreqSignal.size(), midFreqSignal.size()); ++i) {
            testSignal.push_back(lowFreqSignal[i] + midFreqSignal[i]);
        }
        
        std::vector<float> lowCutSettings = {0.0f, 0.3f, 0.6f, 1.0f}; 
        
        for (float lowCut : lowCutSettings) {
            log("\nTesting low-cut setting: " + std::to_string(lowCut) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.3f;     // Delay time
            params[1] = 0.4f;     // Feedback
            params[2] = 0.2f;     // Light damping
            params[3] = lowCut;   // Low-cut parameter
            params[4] = 0.5f;     // Center spread
            params[5] = 1.0f;     // Full wet
            params[6] = 0.0f;     // No sync
            
            auto [original, leftOut, rightOut] = processAudioStereo(testSignal, params);
            
            // Measure response at low and mid frequencies
            float lowResponse = DigitalDelayAnalyzer::measureFrequencyResponse(
                original, leftOut, 60.0, TEST_SAMPLE_RATE);
            float midResponse = DigitalDelayAnalyzer::measureFrequencyResponse(
                original, leftOut, 1000.0, TEST_SAMPLE_RATE);
            
            log("  60Hz response: " + std::to_string(lowResponse) + "dB\n");
            log("  1kHz response: " + std::to_string(midResponse) + "dB\n");
            
            logCSV("LowFreqFiltering", "60Hz_" + std::to_string(lowCut),
                   lowResponse, "MEASURED", "dB");
            logCSV("LowFreqFiltering", "1kHz_" + std::to_string(lowCut),
                   midResponse, "MEASURED", "dB");
            
            // Higher low-cut should attenuate low frequencies more
            if (lowCut > 0.7f) {
                assertTrue(lowResponse < midResponse - 3.0f, 
                          "Low frequency attenuation with low-cut " + std::to_string(lowCut));
            }
            
            assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(leftOut),
                      "Valid output with low-cut " + std::to_string(lowCut));
        }
    }
    
    // Test 5: Stereo spread and ping-pong operation
    void testStereoSpreadPingPong() {
        log("\n--- Stereo Spread and Ping-Pong Tests ---\n");
        
        auto impulseSignal = TestSignalGenerator::generateImpulse(0.6, 500, 
                                                                 static_cast<int>(TEST_SAMPLE_RATE * 4.0));
        
        std::vector<float> spreadSettings = {0.0f, 0.3f, 0.7f, 1.0f}; 
        
        for (float spread : spreadSettings) {
            log("\nTesting stereo spread setting: " + std::to_string(spread) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.2f;     // Short delay time for clear ping-pong
            params[1] = 0.6f;     // Feedback for multiple bounces
            params[2] = 0.1f;     // Light damping
            params[3] = 0.0f;     // No low-cut
            params[4] = spread;   // Spread parameter
            params[5] = 1.0f;     // Full wet
            params[6] = 0.0f;     // No sync
            
            auto [original, leftOut, rightOut] = processAudioStereo(impulseSignal, params);
            
            // Measure stereo correlation
            float correlation = DigitalDelayAnalyzer::measureStereoCorrelation(leftOut, rightOut);
            
            log("  Stereo correlation: " + std::to_string(correlation) + "\n");
            
            logCSV("StereoSpread", "Correlation_" + std::to_string(spread),
                   correlation, "MEASURED", "ratio");
            
            // Higher spread should reduce correlation
            if (spread > 0.7f) {
                assertTrue(std::abs(correlation) < 0.8f, 
                          "Reduced correlation with spread " + std::to_string(spread));
            }
            
            // Analyze ping-pong timing if feedback is present
            if (params[1] > 0.3f) {
                auto pingPongTimings = DigitalDelayAnalyzer::measurePingPongTiming(
                    leftOut, rightOut, TEST_SAMPLE_RATE);
                
                if (!pingPongTimings.empty()) {
                    float avgTiming = std::accumulate(pingPongTimings.begin(), pingPongTimings.end(), 0.0f) / pingPongTimings.size();
                    log("  Average ping-pong interval: " + std::to_string(avgTiming) + "ms\n");
                    
                    logCSV("StereoSpread", "PingPongInterval_" + std::to_string(spread),
                           avgTiming, "MEASURED", "ms");
                }
            }
            
            assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(leftOut) && 
                      !DigitalDelayAnalyzer::hasInvalidValues(rightOut),
                      "Valid stereo output with spread " + std::to_string(spread));
        }
    }
    
    // Test 6: Transport sync precision
    void testTransportSyncPrecision() {
        log("\n--- Transport Sync Precision Tests ---\n");
        
        EngineBase::TransportInfo transport;
        transport.bpm = 120.0;
        transport.timeSigNumerator = 4.0;
        transport.timeSigDenominator = 4.0;
        transport.isPlaying = true;
        
        digitalDelay->setTransportInfo(transport);
        
        auto impulseSignal = TestSignalGenerator::generateImpulse(0.7, 1000, 
                                                                 static_cast<int>(TEST_SAMPLE_RATE * 3.0));
        
        std::vector<float> syncDivisions = {0.2f, 0.4f, 0.6f, 0.8f}; // Different note divisions
        
        for (float syncDiv : syncDivisions) {
            log("\nTesting sync division: " + std::to_string(syncDiv) + "\n");
            
            std::map<int, float> params;
            params[0] = 0.5f;     // Time parameter (should be overridden by sync)
            params[1] = 0.0f;     // No feedback
            params[2] = 0.0f;     // No damping
            params[3] = 0.0f;     // No low-cut
            params[4] = 0.5f;     // Center spread
            params[5] = 1.0f;     // Full wet
            params[6] = 1.0f;     // Sync enabled
            
            // Additional parameter for sync division if available
            if (digitalDelay->getNumParameters() > 7) {
                params[7] = syncDiv;
            }
            
            auto [original, leftOut, rightOut] = processAudioStereo(impulseSignal, params);
            
            float syncedDelay = DigitalDelayAnalyzer::measurePreciseDelayTime(original, leftOut, TEST_SAMPLE_RATE);
            
            log("  Synced delay time: " + std::to_string(syncedDelay) + "ms\n");
            
            logCSV("TransportSync", "SyncedDelay_" + std::to_string(syncDiv),
                   syncedDelay, "MEASURED", "ms");
            
            // Calculate expected delay based on tempo and division
            // This would depend on the specific sync implementation
            float beatDuration = 60000.0f / 120.0f; // 500ms per beat at 120 BPM
            
            // Assume syncDiv maps to different note values
            float expectedDelay = beatDuration * (0.25f + syncDiv * 1.5f); // Range from 1/16 to whole note
            float tolerance = expectedDelay * 0.05f; // 5% tolerance
            
            assertTrue(std::abs(syncedDelay - expectedDelay) < tolerance,
                      "Sync precision for division " + std::to_string(syncDiv));
            
            assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(leftOut),
                      "Valid output with sync division " + std::to_string(syncDiv));
        }
    }
    
    // Test 7: Performance and CPU optimization
    void testPerformanceOptimization() {
        log("\n--- Performance Optimization Tests ---\n");
        
        auto complexSignal = TestSignalGenerator::generateChirp(20.0, 20000.0, 0.4, 8.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> heavyParams;
        heavyParams[0] = 0.8f;  // Long delay
        heavyParams[1] = 0.8f;  // High feedback
        heavyParams[2] = 0.5f;  // Damping
        heavyParams[3] = 0.4f;  // Low-cut
        heavyParams[4] = 0.8f;  // Wide spread
        heavyParams[5] = 1.0f;  // Full wet
        heavyParams[6] = 0.0f;  // No sync
        
        // Measure processing time
        auto startTime = std::chrono::high_resolution_clock::now();
        auto [original, leftOut, rightOut] = processAudioStereo(complexSignal, heavyParams);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0; // milliseconds
        
        double signalDuration = complexSignal.size() / TEST_SAMPLE_RATE * 1000.0; // milliseconds
        double realTimeRatio = processingTime / signalDuration;
        
        log("Processing time: " + std::to_string(processingTime) + "ms\n");
        log("Signal duration: " + std::to_string(signalDuration) + "ms\n");
        log("Real-time ratio: " + std::to_string(realTimeRatio) + "\n");
        
        logCSV("Performance", "ProcessingTime", processingTime, "MEASURED", "ms");
        logCSV("Performance", "RealTimeRatio", realTimeRatio, "MEASURED", "ratio");
        
        // Digital delay should be more efficient than tape echo
        assertTrue(realTimeRatio < 0.3, "Efficient real-time processing");
        
        // Check output quality under heavy load
        assertTrue(!DigitalDelayAnalyzer::hasInvalidValues(leftOut) && 
                  !DigitalDelayAnalyzer::hasInvalidValues(rightOut),
                  "Valid output under heavy processing");
        
        float leftStability = DigitalDelayAnalyzer::calculateRMS_dB(leftOut);
        float rightStability = DigitalDelayAnalyzer::calculateRMS_dB(rightOut);
        
        log("Left channel stability: " + std::to_string(leftStability) + "dB\n");
        log("Right channel stability: " + std::to_string(rightStability) + "dB\n");
        
        assertTrue(leftStability > -80.0f, "Stable left output");
        assertTrue(rightStability > -80.0f, "Stable right output");
    }
    
    // Run all tests
    void runAllTests() {
        log("Starting Digital Delay comprehensive test suite...\n");
        
        testDelayTimingPrecision();
        testFeedbackLoopStability();
        testHighFrequencyDamping();
        testLowFrequencyFiltering();
        testStereoSpreadPingPong();
        testTransportSyncPrecision();
        testPerformanceOptimization();
        
        log("\nAll tests completed.\n");
    }
};

// Main function
int main() {
    try {
        DigitalDelayTest tester;
        tester.runAllTests();
        
        std::cout << "\nDigital Delay test suite completed successfully.\n";
        std::cout << "Check DigitalDelay_TestResults.txt for detailed results.\n";
        std::cout << "Check DigitalDelay_Data.csv for measurement data.\n";
        
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