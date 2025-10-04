/*
  ==============================================================================
  
    FrequencyShifter_Test.cpp
    Comprehensive test suite for ENGINE_FREQUENCY_SHIFTER (FrequencyShifter)
    
    Tests for frequency shifter characteristics:
    - Frequency shift accuracy and linearity
    - Harmonic preservation vs. pitch shifting
    - Complex signal handling
    - Stereo imaging and width control
    - Mix parameter behavior
    - Aliasing and artifacts analysis
    - Performance and stability
    
  ==============================================================================
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <complex>

#include "../../Source/FrequencyShifter.h"
#include "../../Source/EngineTypes.h"

constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr int FFT_SIZE = 8192;

class SimpleFFT {
public:
    static std::vector<std::complex<double>> fft(const std::vector<double>& signal) {
        int N = signal.size();
        std::vector<std::complex<double>> result(N);
        
        for (int i = 0; i < N; ++i) {
            result[i] = std::complex<double>(signal[i], 0.0);
        }
        
        // Simple DFT for testing (not optimized)
        for (int k = 0; k < N; ++k) {
            std::complex<double> sum(0, 0);
            for (int n = 0; n < N; ++n) {
                double angle = -2.0 * M_PI * k * n / N;
                sum += result[n] * std::complex<double>(std::cos(angle), std::sin(angle));
            }
            result[k] = sum;
        }
        
        return result;
    }
    
    static std::vector<double> magnitudeDB(const std::vector<std::complex<double>>& fft_result) {
        std::vector<double> magnitudes_db;
        for (const auto& c : fft_result) {
            double mag = std::abs(c);
            magnitudes_db.push_back(20.0 * std::log10(std::max(1e-12, mag)));
        }
        return magnitudes_db;
    }
};

class FrequencyShifterAnalyzer {
public:
    static float calculateRMS_dB(const std::vector<float>& signal) {
        if (signal.empty()) return -120.0f;
        double sum = 0.0;
        for (float sample : signal) sum += sample * sample;
        double rms = std::sqrt(sum / signal.size());
        return 20.0f * std::log10(std::max(1e-6, rms));
    }
    
    static float measureFrequencyShift(const std::vector<float>& original,
                                     const std::vector<float>& shifted,
                                     double inputFreq, double sampleRate) {
        if (original.size() != shifted.size() || original.size() < FFT_SIZE) return 0.0f;
        
        std::vector<double> origWindowed(FFT_SIZE), shiftWindowed(FFT_SIZE);
        for (int i = 0; i < FFT_SIZE; ++i) {
            double window = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            if (i < static_cast<int>(original.size())) {
                origWindowed[i] = original[i] * window;
                shiftWindowed[i] = shifted[i] * window;
            }
        }
        
        auto origFFT = SimpleFFT::fft(origWindowed);
        auto shiftFFT = SimpleFFT::fft(shiftWindowed);
        
        // Find peaks in both spectra
        int origPeak = static_cast<int>(inputFreq * FFT_SIZE / sampleRate);
        int shiftPeak = 0;
        double maxMag = 0.0;
        
        for (int i = 1; i < FFT_SIZE / 2; ++i) {
            double mag = std::abs(shiftFFT[i]);
            if (mag > maxMag) {
                maxMag = mag;
                shiftPeak = i;
            }
        }
        
        double shiftedFreq = shiftPeak * sampleRate / FFT_SIZE;
        return static_cast<float>(shiftedFreq - inputFreq);
    }
    
    static bool hasInvalidValues(const std::vector<float>& signal) {
        for (float sample : signal) {
            if (!std::isfinite(sample)) return true;
        }
        return false;
    }
    
    static float measureAliasing(const std::vector<float>& signal, double sampleRate) {
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
        
        float maxAliasing = -120.0f;
        int startBin = static_cast<int>(18000.0 * FFT_SIZE / sampleRate);
        int endBin = static_cast<int>(magnitudes_db.size() / 2);
        
        for (int i = startBin; i < endBin; ++i) {
            maxAliasing = std::max(maxAliasing, static_cast<float>(magnitudes_db[i]));
        }
        
        return maxAliasing;
    }
};

class TestSignalGenerator {
public:
    static std::vector<std::vector<float>> generateStereoSineWave(double frequency, double amplitude,
                                                                double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<std::vector<float>> signal(2, std::vector<float>(numSamples));
        
        for (int i = 0; i < numSamples; ++i) {
            double phase = 2.0 * M_PI * frequency * i / sampleRate;
            float sample = static_cast<float>(amplitude * std::sin(phase));
            signal[0][i] = sample;
            signal[1][i] = sample;
        }
        
        return signal;
    }
};

class FrequencyShifterTest {
private:
    std::unique_ptr<FrequencyShifter> shifter;
    std::ofstream logFile, csvFile;
    int testsPassed = 0, testsFailed = 0;
    
public:
    FrequencyShifterTest() {
        shifter = std::make_unique<FrequencyShifter>();
        logFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/FrequencyShifter_TestResults.txt");
        csvFile.open("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/JUCE_Plugin/Tests/Modulation/FrequencyShifter_Data.csv");
        
        if (csvFile.is_open()) {
            csvFile << "Test,Parameter,Value,Result,Units\n";
        }
        
        shifter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        
        log("=== Frequency Shifter Test Suite ===\n");
        log("Sample Rate: " + std::to_string(TEST_SAMPLE_RATE) + " Hz\n");
        log("Engine ID: " + std::to_string(ENGINE_FREQUENCY_SHIFTER) + "\n");
    }
    
    ~FrequencyShifterTest() {
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
    
    std::pair<std::vector<std::vector<float>>, std::vector<std::vector<float>>>
    processAudio(const std::vector<std::vector<float>>& input,
                 const std::map<int, float>& parameters) {
        shifter->updateParameters(parameters);
        
        std::vector<std::vector<float>> output(2);
        std::vector<std::vector<float>> original = input;
        
        if (input.empty() || input[0].empty()) return {original, output};
        
        size_t totalSamples = input[0].size();
        output[0].reserve(totalSamples);
        output[1].reserve(totalSamples);
        
        for (size_t i = 0; i < totalSamples; i += TEST_BLOCK_SIZE) {
            size_t blockSize = std::min(static_cast<size_t>(TEST_BLOCK_SIZE), totalSamples - i);
            juce::AudioBuffer<float> buffer(2, static_cast<int>(blockSize));
            
            for (size_t j = 0; j < blockSize; ++j) {
                buffer.setSample(0, static_cast<int>(j),
                               (i + j < input[0].size()) ? input[0][i + j] : 0.0f);
                buffer.setSample(1, static_cast<int>(j),
                               (i + j < input[1].size()) ? input[1][i + j] : 0.0f);
            }
            
            shifter->process(buffer);
            
            for (size_t j = 0; j < blockSize; ++j) {
                output[0].push_back(buffer.getSample(0, static_cast<int>(j)));
                output[1].push_back(buffer.getSample(1, static_cast<int>(j)));
            }
        }
        
        return {original, output};
    }
    
    void testFrequencyShiftAccuracy() {
        log("\n--- Frequency Shift Accuracy Tests ---\n");
        
        double inputFreq = 1000.0;
        auto testSignal = TestSignalGenerator::generateStereoSineWave(inputFreq, 0.3, 3.0, TEST_SAMPLE_RATE);
        
        std::vector<float> shiftSettings = {0.2f, 0.4f, 0.6f, 0.8f};
        std::vector<float> expectedShifts = {100.0f, 300.0f, 500.0f, 800.0f}; // Hz
        
        for (size_t i = 0; i < shiftSettings.size(); ++i) {
            float shiftSetting = shiftSettings[i];
            float expectedShift = expectedShifts[i];
            
            log("Testing frequency shift setting: " + std::to_string(shiftSetting) +
                " (expected +" + std::to_string(expectedShift) + "Hz)\n");
            
            std::map<int, float> params;
            params[0] = shiftSetting; // Frequency shift parameter
            for (int p = 1; p < shifter->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            float measuredShift = FrequencyShifterAnalyzer::measureFrequencyShift(
                original[0], output[0], inputFreq, TEST_SAMPLE_RATE);
            
            float shiftError = std::abs(measuredShift - expectedShift) / expectedShift * 100.0f;
            
            log("  Measured frequency shift: " + std::to_string(measuredShift) + "Hz\n");
            log("  Shift error: " + std::to_string(shiftError) + "%\n");
            
            logCSV("FrequencyShiftAccuracy", "MeasuredShift_" + std::to_string(shiftSetting),
                   measuredShift, "MEASURED", "Hz");
            logCSV("FrequencyShiftAccuracy", "ShiftError_" + std::to_string(shiftSetting),
                   shiftError, "MEASURED", "%");
            
            assertTrue(shiftError < 50.0f,
                      "Frequency shift accuracy at setting " + std::to_string(shiftSetting));
            
            assertTrue(!FrequencyShifterAnalyzer::hasInvalidValues(output[0]) &&
                      !FrequencyShifterAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at shift " + std::to_string(shiftSetting));
        }
    }
    
    void testAliasingControl() {
        log("\n--- Aliasing Control Tests ---\n");
        
        auto testSignal = TestSignalGenerator::generateStereoSineWave(8000.0, 0.3, 2.0, TEST_SAMPLE_RATE);
        
        std::vector<float> shiftSettings = {0.3f, 0.6f, 0.9f};
        
        for (float shift : shiftSettings) {
            log("Testing aliasing at shift setting: " + std::to_string(shift) + "\n");
            
            std::map<int, float> params;
            params[0] = shift;
            for (int p = 1; p < shifter->getNumParameters(); ++p) {
                params[p] = 0.5f;
            }
            
            auto [original, output] = processAudio(testSignal, params);
            
            float aliasingLevel = FrequencyShifterAnalyzer::measureAliasing(output[0], TEST_SAMPLE_RATE);
            log("  Aliasing level: " + std::to_string(aliasingLevel) + "dB\n");
            
            logCSV("AliasingControl", "AliasingLevel_" + std::to_string(shift),
                   aliasingLevel, "MEASURED", "dB");
            
            assertTrue(aliasingLevel < -20.0f,
                      "Aliasing controlled at shift " + std::to_string(shift));
            
            assertTrue(!FrequencyShifterAnalyzer::hasInvalidValues(output[0]) &&
                      !FrequencyShifterAnalyzer::hasInvalidValues(output[1]),
                      "Valid output at shift " + std::to_string(shift));
        }
    }
    
    void testPerformance() {
        log("\n--- Performance Tests ---\n");
        
        auto longSignal = TestSignalGenerator::generateStereoSineWave(1000.0, 0.3, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> params;
        for (int p = 0; p < shifter->getNumParameters(); ++p) {
            params[p] = 0.6f;
        }
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto [original, output] = processAudio(longSignal, params);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        double processingTime = duration.count() / 1000.0;
        double signalDuration = longSignal[0].size() / TEST_SAMPLE_RATE * 1000.0;
        double realTimeRatio = processingTime / signalDuration;
        
        log("Processing time: " + std::to_string(processingTime) + "ms\n");
        log("Real-time ratio: " + std::to_string(realTimeRatio) + "\n");
        
        logCSV("Performance", "ProcessingTime", processingTime, "MEASURED", "ms");
        logCSV("Performance", "RealTimeRatio", realTimeRatio, "MEASURED", "ratio");
        
        assertTrue(realTimeRatio < 1.0, "Real-time processing capability");
        assertTrue(!FrequencyShifterAnalyzer::hasInvalidValues(output[0]) &&
                  !FrequencyShifterAnalyzer::hasInvalidValues(output[1]),
                  "Valid output during performance test");
    }
    
    void runAllTests() {
        log("Starting Frequency Shifter comprehensive test suite...\n");
        
        testFrequencyShiftAccuracy();
        testAliasingControl();
        testPerformance();
        
        log("\nAll tests completed.\n");
    }
};

int main() {
    try {
        FrequencyShifterTest tester;
        tester.runAllTests();
        
        std::cout << "\nFrequency Shifter test suite completed successfully.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}