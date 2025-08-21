/*
  ==============================================================================
  
    CombResonator_Test.cpp
    Comprehensive test suite for ENGINE_COMB_RESONATOR
    
    Tests for comb resonator characteristics:
    - Comb filter frequency response accuracy
    - Resonant peak positioning and spacing
    - Delay time precision and modulation
    - Feedback amount and stability limits
    - Multi-comb configuration and interaction
    - Interpolation quality for smooth modulation
    - Frequency tracking across the spectrum
    - Harmonic enhancement and resonance
    - Stability at extreme feedback levels
    
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

// Include the engine
#include "../../Source/CombResonator.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 1.0f;
constexpr float FREQ_EPSILON = 0.05f; // 5% frequency tolerance
constexpr float DELAY_EPSILON = 0.02f; // 2% delay time tolerance

// Test signal generators for comb resonator testing
class CombTestSignalGenerator {
public:
    // Generate impulse train for comb filter testing
    static std::vector<float> generateImpulseTrain(double period, double amplitude,
                                                  int numPulses, double totalDuration,
                                                  double sampleRate) {
        int totalSamples = static_cast<int>(totalDuration * sampleRate);
        int periodSamples = static_cast<int>(period * sampleRate);
        
        std::vector<float> signal(totalSamples, 0.0f);
        
        for (int pulse = 0; pulse < numPulses; ++pulse) {
            int pulsePosition = pulse * periodSamples;
            if (pulsePosition < totalSamples) {
                signal[pulsePosition] = static_cast<float>(amplitude);
            }
        }
        
        return signal;
    }
    
    // Generate white noise for resonance testing
    static std::vector<float> generateWhiteNoise(double amplitude, double duration, 
                                                double sampleRate, uint32_t seed = 789) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
    
    // Generate harmonic series for comb interaction testing
    static std::vector<float> generateHarmonicSeries(double fundamentalFreq, 
                                                   int numHarmonics, 
                                                   double amplitude,
                                                   double duration, 
                                                   double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        for (int h = 1; h <= numHarmonics; ++h) {
            double harmonicFreq = fundamentalFreq * h;
            if (harmonicFreq < sampleRate / 2.0) {
                double harmonicAmp = amplitude / h; // 1/f rolloff
                double phase = 0.0;
                double phaseIncrement = 2.0 * M_PI * harmonicFreq / sampleRate;
                
                for (int i = 0; i < numSamples; ++i) {
                    signal[i] += static_cast<float>(harmonicAmp * std::sin(phase));
                    phase += phaseIncrement;
                }
            }
        }
        
        return signal;
    }
    
    // Generate single impulse for impulse response testing
    static std::vector<float> generateImpulse(double amplitude, int position, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate frequency sweep
    static std::vector<float> generateFrequencySweep(double startFreq, double endFreq,
                                                   double amplitude, double duration,
                                                   double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double phase = 0.0;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = i / duration / numSamples;
            double freq = startFreq + (endFreq - startFreq) * t;
            double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
            
            signal[i] = static_cast<float>(amplitude * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    // Generate pink noise (1/f spectrum)
    static std::vector<float> generatePinkNoise(double amplitude, double duration,
                                               double sampleRate, uint32_t seed = 987) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        // Simple pink noise approximation using multiple octave filters
        std::vector<float> octaveValues(7, 0.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            float white = dist(gen);
            
            // Update octave generators
            octaveValues[0] = 0.99886f * octaveValues[0] + white * 0.0555179f;
            octaveValues[1] = 0.99332f * octaveValues[1] + white * 0.0750759f;
            octaveValues[2] = 0.96900f * octaveValues[2] + white * 0.1538520f;
            octaveValues[3] = 0.86650f * octaveValues[3] + white * 0.3104856f;
            octaveValues[4] = 0.55000f * octaveValues[4] + white * 0.5329522f;
            octaveValues[5] = -0.7616f * octaveValues[5] - white * 0.0168980f;
            
            float pink = octaveValues[0] + octaveValues[1] + octaveValues[2] + 
                        octaveValues[3] + octaveValues[4] + octaveValues[5] + white * 0.5362f;
            
            signal[i] = static_cast<float>(amplitude * pink * 0.11f); // Scale down
        }
        
        return signal;
    }
};

// Comb filter analysis tools
class CombAnalyzer {
public:
    struct CombPeak {
        double frequency;
        double magnitude_dB;
        double q_factor;
        bool isResonantPeak;
    };
    
    struct CombResponse {
        std::vector<CombPeak> peaks;
        std::vector<CombPeak> notches;
        double fundamentalFreq;
        double harmonicSpacing;
        double averagePeakLevel;
        double averageNotchLevel;
    };
    
    // Analyze comb filter response
    static CombResponse analyzeCombResponse(const std::vector<float>& inputSignal,
                                          const std::vector<float>& outputSignal,
                                          double sampleRate) {
        CombResponse response;
        
        // Test frequencies across spectrum
        std::vector<double> testFreqs;
        for (double f = 50; f <= sampleRate / 2.1; f *= 1.1) {
            testFreqs.push_back(f);
        }
        
        std::vector<double> magnitudes;
        
        // Calculate frequency response
        for (double freq : testFreqs) {
            double inputMag = calculateMagnitudeAtFreq(inputSignal, freq, sampleRate);
            double outputMag = calculateMagnitudeAtFreq(outputSignal, freq, sampleRate);
            
            double response_dB = 20.0 * std::log10(outputMag / (inputMag + 1e-15));
            magnitudes.push_back(response_dB);
        }
        
        // Find peaks and notches
        for (size_t i = 2; i < magnitudes.size() - 2; ++i) {
            bool isPeak = true;
            bool isNotch = true;
            
            for (int j = -2; j <= 2; ++j) {
                if (j != 0) {
                    if (magnitudes[i + j] > magnitudes[i]) isPeak = false;
                    if (magnitudes[i + j] < magnitudes[i]) isNotch = false;
                }
            }
            
            if (isPeak && magnitudes[i] > -10.0) { // Minimum peak level
                CombPeak peak;
                peak.frequency = testFreqs[i];
                peak.magnitude_dB = magnitudes[i];
                peak.q_factor = estimateQFactor(magnitudes, i, testFreqs);
                peak.isResonantPeak = magnitudes[i] > 3.0; // Above 3dB considered resonant
                response.peaks.push_back(peak);
            }
            
            if (isNotch && magnitudes[i] < -3.0) { // Maximum notch level
                CombPeak notch;
                notch.frequency = testFreqs[i];
                notch.magnitude_dB = magnitudes[i];
                notch.q_factor = estimateQFactor(magnitudes, i, testFreqs);
                notch.isResonantPeak = false;
                response.notches.push_back(notch);
            }
        }
        
        // Estimate fundamental frequency and harmonic spacing
        if (response.peaks.size() >= 2) {
            std::vector<double> spacings;
            for (size_t i = 1; i < response.peaks.size(); ++i) {
                spacings.push_back(response.peaks[i].frequency - response.peaks[i-1].frequency);
            }
            
            // Find most common spacing (fundamental)
            std::sort(spacings.begin(), spacings.end());
            response.harmonicSpacing = spacings[spacings.size() / 2]; // Median
            response.fundamentalFreq = response.peaks[0].frequency;
        }
        
        // Calculate average levels
        if (!response.peaks.empty()) {
            double sum = 0.0;
            for (const auto& peak : response.peaks) {
                sum += peak.magnitude_dB;
            }
            response.averagePeakLevel = sum / response.peaks.size();
        }
        
        if (!response.notches.empty()) {
            double sum = 0.0;
            for (const auto& notch : response.notches) {
                sum += notch.magnitude_dB;
            }
            response.averageNotchLevel = sum / response.notches.size();
        }
        
        return response;
    }
    
private:
    static double calculateMagnitudeAtFreq(const std::vector<float>& signal, 
                                         double frequency, double sampleRate) {
        double real = 0.0, imag = 0.0;
        double omega = 2.0 * M_PI * frequency / sampleRate;
        
        for (size_t i = 0; i < signal.size(); ++i) {
            double phase = omega * i;
            real += signal[i] * std::cos(phase);
            imag += signal[i] * std::sin(phase);
        }
        
        return std::sqrt(real * real + imag * imag) / signal.size();
    }
    
    static double estimateQFactor(const std::vector<double>& magnitudes, 
                                int peakIndex, const std::vector<double>& frequencies) {
        if (peakIndex < 2 || peakIndex >= static_cast<int>(magnitudes.size()) - 2) {
            return 1.0;
        }
        
        double peakMag = magnitudes[peakIndex];
        double halfPowerLevel = peakMag - 3.0; // -3dB point
        
        // Find -3dB points
        int lowerIndex = peakIndex, upperIndex = peakIndex;
        
        for (int i = peakIndex - 1; i >= 0; --i) {
            if (magnitudes[i] <= halfPowerLevel) {
                lowerIndex = i;
                break;
            }
        }
        
        for (int i = peakIndex + 1; i < static_cast<int>(magnitudes.size()); ++i) {
            if (magnitudes[i] <= halfPowerLevel) {
                upperIndex = i;
                break;
            }
        }
        
        if (lowerIndex != peakIndex && upperIndex != peakIndex) {
            double bandwidth = frequencies[upperIndex] - frequencies[lowerIndex];
            return frequencies[peakIndex] / (bandwidth + 1e-15);
        }
        
        return 1.0;
    }
};

// Main test suite for Comb Resonator
class CombResonatorTestSuite {
private:
    std::unique_ptr<CombResonator> resonator;
    std::ofstream logFile;
    
public:
    CombResonatorTestSuite() : resonator(std::make_unique<CombResonator>()) {
        logFile.open("CombResonator_TestResults.txt");
        logFile << "=== Comb Resonator Test Results ===" << std::endl;
        logFile << "Test started at: " << getCurrentTime() << std::endl << std::endl;
    }
    
    ~CombResonatorTestSuite() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void runAllTests() {
        logFile << "Starting comprehensive Comb Resonator test suite..." << std::endl;
        
        // Initialize resonator
        resonator->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        resonator->reset();
        
        // Run test categories
        testBasicFunctionality();
        testCombFrequencyResponse();
        testResonantPeaks();
        testDelayTimePrecision();
        testFeedbackStability();
        testHarmonicEnhancement();
        testInterpolationQuality();
        testFrequencyTracking();
        testMultiCombInteraction();
        testModulationEffects();
        testStabilityLimits();
        testMusicalApplications();
        
        logFile << "\n=== Comb Resonator Test Suite Complete ===" << std::endl;
        std::cout << "Comb Resonator test results written to CombResonator_TestResults.txt" << std::endl;
    }
    
private:
    void testBasicFunctionality() {
        logFile << "\n--- Basic Functionality Tests ---" << std::endl;
        
        // Test parameter count
        int numParams = resonator->getNumParameters();
        logFile << "Number of parameters: " << numParams << std::endl;
        assert(numParams == 8);
        
        // Test parameter names
        for (int i = 0; i < numParams; ++i) {
            juce::String paramName = resonator->getParameterName(i);
            logFile << "Parameter " << i << ": " << paramName << std::endl;
        }
        
        // Test engine name
        juce::String engineName = resonator->getName();
        logFile << "Engine name: " << engineName << std::endl;
        assert(engineName == "Comb Resonator");
        
        logFile << "✓ Basic functionality tests passed" << std::endl;
    }
    
    void testCombFrequencyResponse() {
        logFile << "\n--- Comb Frequency Response Tests ---" << std::endl;
        
        // Test different fundamental frequencies
        std::vector<double> fundamentalFreqs = {110.0, 220.0, 440.0, 880.0};
        
        for (double fundamental : fundamentalFreqs) {
            logFile << "\nTesting fundamental frequency: " << fundamental << " Hz" << std::endl;
            
            // Calculate expected delay time
            double delayTime = 1.0 / fundamental;
            float delayParam = static_cast<float>(delayTime / 0.1); // Assuming 0.1s max delay
            delayParam = std::min(1.0f, std::max(0.0f, delayParam));
            
            std::map<int, float> params = {
                {0, delayParam}, // Delay time (targeting fundamental)
                {1, 0.7f}, // Moderate feedback
                {2, 0.5f}, // Moderate resonance
                {3, 0.0f}, // No modulation rate
                {4, 0.0f}, // No modulation depth
                {5, 0.5f}, // Medium frequency
                {6, 0.5f}, // Medium damping
                {7, 1.0f}  // Full wet
            };
            resonator->updateParameters(params);
            
            // Test with white noise input
            auto noiseInput = CombTestSignalGenerator::generateWhiteNoise(0.1, 2.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, noiseInput.size());
            for (size_t i = 0; i < noiseInput.size(); ++i) {
                buffer.setSample(0, i, noiseInput[i]);
                buffer.setSample(1, i, noiseInput[i]);
            }
            
            resonator->process(buffer);
            
            // Extract output signal
            std::vector<float> outputSignal(noiseInput.size());
            for (size_t i = 0; i < noiseInput.size(); ++i) {
                outputSignal[i] = buffer.getSample(0, i);
            }
            
            // Analyze comb response
            auto combResponse = CombAnalyzer::analyzeCombResponse(noiseInput, outputSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Detected peaks: " << combResponse.peaks.size() << std::endl;
            logFile << "  Detected notches: " << combResponse.notches.size() << std::endl;
            logFile << "  Estimated fundamental: " << combResponse.fundamentalFreq << " Hz" << std::endl;
            logFile << "  Harmonic spacing: " << combResponse.harmonicSpacing << " Hz" << std::endl;
            logFile << "  Average peak level: " << combResponse.averagePeakLevel << " dB" << std::endl;
            
            // Verify comb characteristics
            assert(combResponse.peaks.size() >= 3); // Should have multiple peaks
            assert(combResponse.notches.size() >= 2); // Should have notches between peaks
            
            // Check fundamental frequency accuracy
            if (combResponse.fundamentalFreq > 0) {
                double freqError = std::abs(combResponse.fundamentalFreq - fundamental) / fundamental;
                logFile << "  Fundamental frequency error: " << freqError * 100.0 << "%" << std::endl;
                assert(freqError < FREQ_EPSILON);
            }
            
            // Check harmonic spacing
            if (combResponse.harmonicSpacing > 0) {
                double spacingError = std::abs(combResponse.harmonicSpacing - fundamental) / fundamental;
                logFile << "  Harmonic spacing error: " << spacingError * 100.0 << "%" << std::endl;
                assert(spacingError < FREQ_EPSILON);
            }
        }
        
        logFile << "✓ Comb frequency response tests passed" << std::endl;
    }
    
    void testResonantPeaks() {
        logFile << "\n--- Resonant Peaks Tests ---" << std::endl;
        
        // Test different resonance levels
        std::vector<float> resonanceValues = {0.2f, 0.5f, 0.8f};
        
        for (float resonance : resonanceValues) {
            logFile << "\nTesting resonance level: " << resonance << std::endl;
            
            std::map<int, float> params = {
                {0, 0.3f}, // ~147Hz fundamental (1/147 ≈ 0.0068s, normalized)
                {1, 0.6f}, // Moderate feedback
                {2, resonance}, // Variable resonance
                {3, 0.0f}, // No modulation
                {4, 0.0f}, // No modulation
                {5, 0.5f}, // Medium frequency
                {6, 0.3f}, // Low damping for clear peaks
                {7, 1.0f}  // Full wet
            };
            resonator->updateParameters(params);
            
            // Test with impulse to excite resonances
            auto impulseInput = CombTestSignalGenerator::generateImpulse(1.0, 100, 
                                                                       static_cast<int>(1.5 * TEST_SAMPLE_RATE));
            
            juce::AudioBuffer<float> buffer(2, impulseInput.size());
            for (size_t i = 0; i < impulseInput.size(); ++i) {
                buffer.setSample(0, i, impulseInput[i]);
                buffer.setSample(1, i, impulseInput[i]);
            }
            
            resonator->process(buffer);
            
            // Extract output and analyze
            std::vector<float> outputSignal(impulseInput.size());
            for (size_t i = 0; i < impulseInput.size(); ++i) {
                outputSignal[i] = buffer.getSample(0, i);
            }
            
            auto combResponse = CombAnalyzer::analyzeCombResponse(impulseInput, outputSignal, TEST_SAMPLE_RATE);
            
            // Count resonant peaks
            int resonantPeaks = 0;
            for (const auto& peak : combResponse.peaks) {
                if (peak.isResonantPeak) {
                    resonantPeaks++;
                    logFile << "    Resonant peak at " << peak.frequency 
                           << " Hz, " << peak.magnitude_dB << " dB, Q=" << peak.q_factor << std::endl;
                }
            }
            
            logFile << "  Resonant peaks found: " << resonantPeaks << std::endl;
            logFile << "  Average peak level: " << combResponse.averagePeakLevel << " dB" << std::endl;
            
            // Higher resonance should produce more/stronger resonant peaks
            if (resonance > 0.7f) {
                assert(resonantPeaks >= 2); // Should have multiple resonant peaks
                assert(combResponse.averagePeakLevel > 5.0); // Strong peaks
            }
            
            // Verify Q factors are reasonable
            for (const auto& peak : combResponse.peaks) {
                assert(peak.q_factor > 0.5 && peak.q_factor < 100.0); // Reasonable Q range
            }
        }
        
        logFile << "✓ Resonant peaks tests passed" << std::endl;
    }
    
    void testDelayTimePrecision() {
        logFile << "\n--- Delay Time Precision Tests ---" << std::endl;
        
        // Test specific delay times
        std::vector<double> targetDelayTimes = {0.001, 0.005, 0.01, 0.02, 0.05}; // seconds
        
        for (double targetDelay : targetDelayTimes) {
            if (targetDelay < 0.1) { // Within reasonable range
                logFile << "\nTesting delay time: " << targetDelay * 1000.0 << " ms" << std::endl;
                
                float delayParam = static_cast<float>(targetDelay / 0.1); // Normalize to 0-1
                
                std::map<int, float> params = {
                    {0, delayParam}, // Target delay
                    {1, 0.0f}, // No feedback (for clean delay measurement)
                    {2, 0.0f}, // No resonance
                    {3, 0.0f}, // No modulation
                    {4, 0.0f}, // No modulation
                    {5, 0.5f}, // Medium frequency
                    {6, 0.8f}, // High damping
                    {7, 1.0f}  // Full wet
                };
                resonator->updateParameters(params);
                
                // Use impulse to measure delay
                auto impulseInput = CombTestSignalGenerator::generateImpulse(1.0, 1000, 
                                                                           static_cast<int>(2.0 * TEST_SAMPLE_RATE));
                
                juce::AudioBuffer<float> buffer(2, impulseInput.size());
                for (size_t i = 0; i < impulseInput.size(); ++i) {
                    buffer.setSample(0, i, impulseInput[i]);
                    buffer.setSample(1, i, impulseInput[i]);
                }
                
                resonator->process(buffer);
                
                // Find delayed impulse in output
                double maxOutput = 0.0;
                int delayedPeakPosition = -1;
                
                for (int i = 1000; i < buffer.getNumSamples(); ++i) {
                    double output = std::abs(buffer.getSample(0, i));
                    if (output > maxOutput) {
                        maxOutput = output;
                        delayedPeakPosition = i;
                    }
                }
                
                if (delayedPeakPosition > 1000) {
                    double measuredDelay = (delayedPeakPosition - 1000) / TEST_SAMPLE_RATE;
                    double delayError = std::abs(measuredDelay - targetDelay) / targetDelay;
                    
                    logFile << "  Target delay: " << targetDelay * 1000.0 << " ms" << std::endl;
                    logFile << "  Measured delay: " << measuredDelay * 1000.0 << " ms" << std::endl;
                    logFile << "  Delay error: " << delayError * 100.0 << "%" << std::endl;
                    
                    // Verify delay accuracy
                    assert(delayError < DELAY_EPSILON);
                } else {
                    logFile << "  Warning: Could not detect delayed impulse" << std::endl;
                }
            }
        }
        
        logFile << "✓ Delay time precision tests passed" << std::endl;
    }
    
    void testFeedbackStability() {
        logFile << "\n--- Feedback Stability Tests ---" << std::endl;
        
        // Test increasing feedback levels
        std::vector<float> feedbackValues = {0.1f, 0.3f, 0.6f, 0.8f, 0.95f, 0.99f};
        
        for (float feedback : feedbackValues) {
            logFile << "\nTesting feedback level: " << feedback << std::endl;
            
            std::map<int, float> params = {
                {0, 0.2f}, // Short delay
                {1, feedback}, // Variable feedback
                {2, 0.3f}, // Low resonance
                {3, 0.0f}, // No modulation
                {4, 0.0f}, // No modulation
                {5, 0.5f}, // Medium frequency
                {6, 0.5f}, // Medium damping
                {7, 1.0f}  // Full wet
            };
            resonator->updateParameters(params);
            
            // Test with impulse
            auto impulseInput = CombTestSignalGenerator::generateImpulse(0.1, 100, 
                                                                       static_cast<int>(3.0 * TEST_SAMPLE_RATE));
            
            juce::AudioBuffer<float> buffer(2, impulseInput.size());
            for (size_t i = 0; i < impulseInput.size(); ++i) {
                buffer.setSample(0, i, impulseInput[i]);
                buffer.setSample(1, i, impulseInput[i]);
            }
            
            resonator->process(buffer);
            
            // Check for stability
            bool stable = true;
            double maxOutput = 0.0;
            double avgOutput = 0.0;
            double finalOutput = 0.0;
            
            // Check last portion for stability
            int checkStart = buffer.getNumSamples() * 3 / 4;
            
            for (int i = checkStart; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(0, i);
                
                if (std::isnan(sample) || std::isinf(sample)) {
                    stable = false;
                    break;
                }
                
                double absValue = std::abs(sample);
                maxOutput = std::max(maxOutput, absValue);
                avgOutput += absValue;
                
                if (i == buffer.getNumSamples() - 1) {
                    finalOutput = absValue;
                }
            }
            
            avgOutput /= (buffer.getNumSamples() - checkStart);
            
            // Calculate decay characteristics
            double decayTime = -1.0;
            if (feedback < 0.99f) {
                // Find when output drops to 1% of peak
                double peakValue = 0.0;
                int peakPosition = 0;
                
                for (int i = 0; i < buffer.getNumSamples() / 2; ++i) {
                    double value = std::abs(buffer.getSample(0, i));
                    if (value > peakValue) {
                        peakValue = value;
                        peakPosition = i;
                    }
                }
                
                double threshold = peakValue * 0.01;
                for (int i = peakPosition; i < buffer.getNumSamples(); ++i) {
                    if (std::abs(buffer.getSample(0, i)) < threshold) {
                        decayTime = (i - peakPosition) / TEST_SAMPLE_RATE;
                        break;
                    }
                }
            }
            
            logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
            logFile << "  Max output: " << maxOutput << std::endl;
            logFile << "  Average output: " << avgOutput << std::endl;
            logFile << "  Final output: " << finalOutput << std::endl;
            
            if (decayTime > 0) {
                logFile << "  Decay time to 1%: " << decayTime << " seconds" << std::endl;
            }
            
            // Verify stability
            assert(stable);
            
            // Higher feedback should produce longer decay times
            if (feedback > 0.8f) {
                assert(maxOutput > avgOutput * 2.0); // Should have initial transient
            }
            
            // Very high feedback should still be stable but with long decay
            if (feedback > 0.95f) {
                assert(maxOutput < 100.0); // Should not explode
            }
        }
        
        logFile << "✓ Feedback stability tests passed" << std::endl;
    }
    
    void testHarmonicEnhancement() {
        logFile << "\n--- Harmonic Enhancement Tests ---" << std::endl;
        
        double fundamentalFreq = 110.0; // A2
        
        // Test with harmonic series input
        auto harmonicInput = CombTestSignalGenerator::generateHarmonicSeries(
            fundamentalFreq, 10, 0.1, 2.0, TEST_SAMPLE_RATE);
        
        // Test different comb settings tuned to fundamental
        double delayTime = 1.0 / fundamentalFreq;
        float delayParam = static_cast<float>(delayTime / 0.1);
        delayParam = std::min(1.0f, std::max(0.0f, delayParam));
        
        std::map<int, float> params = {
            {0, delayParam}, // Tuned to fundamental
            {1, 0.7f}, // High feedback
            {2, 0.6f}, // Moderate resonance
            {3, 0.0f}, // No modulation
            {4, 0.0f}, // No modulation
            {5, 0.5f}, // Medium frequency
            {6, 0.2f}, // Low damping
            {7, 1.0f}  // Full wet
        };
        resonator->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, harmonicInput.size());
        for (size_t i = 0; i < harmonicInput.size(); ++i) {
            buffer.setSample(0, i, harmonicInput[i]);
            buffer.setSample(1, i, harmonicInput[i]);
        }
        
        resonator->process(buffer);
        
        // Extract output
        std::vector<float> outputSignal(harmonicInput.size());
        for (size_t i = 0; i < harmonicInput.size(); ++i) {
            outputSignal[i] = buffer.getSample(0, i);
        }
        
        // Analyze harmonic enhancement
        logFile << "\nHarmonic enhancement analysis:" << std::endl;
        
        for (int h = 1; h <= 8; ++h) {
            double harmonicFreq = fundamentalFreq * h;
            
            double inputLevel = measureHarmonicLevel(harmonicInput, harmonicFreq, TEST_SAMPLE_RATE);
            double outputLevel = measureHarmonicLevel(outputSignal, harmonicFreq, TEST_SAMPLE_RATE);
            
            double enhancement_dB = 20.0 * std::log10(outputLevel / (inputLevel + 1e-15));
            
            logFile << "  Harmonic " << h << " (" << harmonicFreq << " Hz): " 
                   << enhancement_dB << " dB enhancement" << std::endl;
            
            // Fundamental and its harmonics should be enhanced
            if (h <= 6) { // Within reasonable range
                assert(enhancement_dB > -10.0); // Should not be severely attenuated
            }
        }
        
        // Test with inharmonic content
        logFile << "\nInharmonic content suppression:" << std::endl;
        
        std::vector<double> inharmonicFreqs = {
            fundamentalFreq * 1.5, // Between 1st and 2nd harmonic
            fundamentalFreq * 2.7, // Between 2nd and 3rd harmonic
            fundamentalFreq * 4.3  // Between 4th and 5th harmonic
        };
        
        for (double freq : inharmonicFreqs) {
            if (freq < TEST_SAMPLE_RATE / 2.1) {
                double inputLevel = measureHarmonicLevel(harmonicInput, freq, TEST_SAMPLE_RATE);
                double outputLevel = measureHarmonicLevel(outputSignal, freq, TEST_SAMPLE_RATE);
                
                double suppression_dB = 20.0 * std::log10(outputLevel / (inputLevel + 1e-15));
                
                logFile << "  " << freq << " Hz (inharmonic): " 
                       << suppression_dB << " dB change" << std::endl;
                
                // Inharmonic content should be less enhanced or suppressed
                // (This depends on the comb filter design)
            }
        }
        
        logFile << "✓ Harmonic enhancement tests passed" << std::endl;
    }
    
    void testInterpolationQuality() {
        logFile << "\n--- Interpolation Quality Tests ---" << std::endl;
        
        // Test with modulated delay time
        std::map<int, float> params = {
            {0, 0.3f}, // Base delay
            {1, 0.5f}, // Moderate feedback
            {2, 0.4f}, // Moderate resonance
            {3, 0.5f}, // Medium modulation rate
            {4, 0.3f}, // Moderate modulation depth
            {5, 0.5f}, // Medium frequency
            {6, 0.5f}, // Medium damping
            {7, 1.0f}  // Full wet
        };
        resonator->updateParameters(params);
        
        // Test with sine wave to detect interpolation artifacts
        auto sineInput = CombTestSignalGenerator::generateHarmonicSeries(
            440.0, 1, 0.2, 3.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, sineInput.size());
        for (size_t i = 0; i < sineInput.size(); ++i) {
            buffer.setSample(0, i, sineInput[i]);
            buffer.setSample(1, i, sineInput[i]);
        }
        
        resonator->process(buffer);
        
        // Extract output
        std::vector<float> outputSignal(sineInput.size());
        for (size_t i = 0; i < sineInput.size(); ++i) {
            outputSignal[i] = buffer.getSample(0, i);
        }
        
        // Analyze for artifacts
        double totalEnergy = 0.0;
        double fundamentalEnergy = 0.0;
        double artifactEnergy = 0.0;
        
        // Measure energy at fundamental and potential artifact frequencies
        fundamentalEnergy = measureHarmonicLevel(outputSignal, 440.0, TEST_SAMPLE_RATE);
        fundamentalEnergy *= fundamentalEnergy;
        
        // Check for aliasing and intermodulation artifacts
        std::vector<double> artifactFreqs = {
            440.0 + 50.0,  // Sideband
            440.0 - 50.0,  // Sideband
            880.0,         // 2nd harmonic
            1320.0,        // 3rd harmonic
            220.0          // Subharmonic
        };
        
        for (double freq : artifactFreqs) {
            if (freq > 0 && freq < TEST_SAMPLE_RATE / 2.1) {
                double energy = measureHarmonicLevel(outputSignal, freq, TEST_SAMPLE_RATE);
                artifactEnergy += energy * energy;
            }
        }
        
        // Calculate total energy
        for (float sample : outputSignal) {
            totalEnergy += sample * sample;
        }
        
        double thd = std::sqrt(artifactEnergy) / std::sqrt(fundamentalEnergy + 1e-15);
        double signalToNoise = 10.0 * std::log10(fundamentalEnergy / (totalEnergy - fundamentalEnergy + 1e-15));
        
        logFile << "Interpolation quality analysis:" << std::endl;
        logFile << "  THD: " << thd * 100.0 << "%" << std::endl;
        logFile << "  Signal-to-noise ratio: " << signalToNoise << " dB" << std::endl;
        
        // Verify good interpolation quality
        assert(thd < 0.1); // Less than 10% THD
        assert(signalToNoise > 30.0); // At least 30dB SNR
        
        logFile << "✓ Interpolation quality tests passed" << std::endl;
    }
    
    void testFrequencyTracking() {
        logFile << "\n--- Frequency Tracking Tests ---" << std::endl;
        
        // Test across different frequencies
        std::vector<double> trackingFreqs = {55.0, 110.0, 220.0, 440.0, 880.0};
        
        for (double freq : trackingFreqs) {
            logFile << "\nTesting frequency tracking at " << freq << " Hz:" << std::endl;
            
            // Set comb to resonate at this frequency
            double delayTime = 1.0 / freq;
            float delayParam = static_cast<float>(delayTime / 0.1);
            delayParam = std::min(1.0f, std::max(0.0f, delayParam));
            
            std::map<int, float> params = {
                {0, delayParam}, // Tuned delay
                {1, 0.6f}, // Moderate feedback
                {2, 0.7f}, // High resonance
                {3, 0.0f}, // No modulation
                {4, 0.0f}, // No modulation
                {5, 0.5f}, // Medium frequency
                {6, 0.3f}, // Low damping
                {7, 1.0f}  // Full wet
            };
            resonator->updateParameters(params);
            
            // Test with frequency sweep around target
            auto sweepInput = CombTestSignalGenerator::generateFrequencySweep(
                freq * 0.8, freq * 1.2, 0.1, 2.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, sweepInput.size());
            for (size_t i = 0; i < sweepInput.size(); ++i) {
                buffer.setSample(0, i, sweepInput[i]);
                buffer.setSample(1, i, sweepInput[i]);
            }
            
            resonator->process(buffer);
            
            // Extract output
            std::vector<float> outputSignal(sweepInput.size());
            for (size_t i = 0; i < sweepInput.size(); ++i) {
                outputSignal[i] = buffer.getSample(0, i);
            }
            
            // Find peak response frequency
            double maxResponse = 0.0;
            double peakFreq = 0.0;
            
            for (double testFreq = freq * 0.9; testFreq <= freq * 1.1; testFreq += freq * 0.01) {
                double response = measureHarmonicLevel(outputSignal, testFreq, TEST_SAMPLE_RATE);
                if (response > maxResponse) {
                    maxResponse = response;
                    peakFreq = testFreq;
                }
            }
            
            double trackingError = std::abs(peakFreq - freq) / freq;
            
            logFile << "  Target frequency: " << freq << " Hz" << std::endl;
            logFile << "  Peak response at: " << peakFreq << " Hz" << std::endl;
            logFile << "  Tracking error: " << trackingError * 100.0 << "%" << std::endl;
            logFile << "  Peak response level: " << maxResponse << std::endl;
            
            // Verify tracking accuracy
            assert(trackingError < FREQ_EPSILON);
            assert(maxResponse > 0.01); // Should have clear peak
        }
        
        logFile << "✓ Frequency tracking tests passed" << std::endl;
    }
    
    void testMultiCombInteraction() {
        logFile << "\n--- Multi-Comb Interaction Tests ---" << std::endl;
        
        // Test with multiple comb settings (simulating multiple internal combs)
        logFile << "Testing multi-comb resonance:" << std::endl;
        
        std::map<int, float> params = {
            {0, 0.25f}, // Primary delay
            {1, 0.6f}, // Moderate feedback
            {2, 0.8f}, // High resonance (may engage multiple combs)
            {3, 0.1f}, // Slow modulation
            {4, 0.2f}, // Light modulation depth
            {5, 0.6f}, // Higher frequency range
            {6, 0.3f}, // Low damping
            {7, 1.0f}  // Full wet
        };
        resonator->updateParameters(params);
        
        // Test with complex harmonic input
        auto complexInput = CombTestSignalGenerator::generateHarmonicSeries(
            147.0, 8, 0.1, 3.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, complexInput.size());
        for (size_t i = 0; i < complexInput.size(); ++i) {
            buffer.setSample(0, i, complexInput[i]);
            buffer.setSample(1, i, complexInput[i]);
        }
        
        resonator->process(buffer);
        
        // Extract output
        std::vector<float> outputSignal(complexInput.size());
        for (size_t i = 0; i < complexInput.size(); ++i) {
            outputSignal[i] = buffer.getSample(0, i);
        }
        
        // Analyze multi-comb response
        auto combResponse = CombAnalyzer::analyzeCombResponse(complexInput, outputSignal, TEST_SAMPLE_RATE);
        
        logFile << "  Total peaks detected: " << combResponse.peaks.size() << std::endl;
        logFile << "  Total notches detected: " << combResponse.notches.size() << std::endl;
        logFile << "  Peak/notch ratio: " << static_cast<double>(combResponse.peaks.size()) / 
                                            (combResponse.notches.size() + 1) << std::endl;
        
        // Analyze peak distribution
        std::vector<double> peakSpacings;
        for (size_t i = 1; i < combResponse.peaks.size(); ++i) {
            peakSpacings.push_back(combResponse.peaks[i].frequency - combResponse.peaks[i-1].frequency);
        }
        
        if (!peakSpacings.empty()) {
            double avgSpacing = 0.0;
            for (double spacing : peakSpacings) {
                avgSpacing += spacing;
            }
            avgSpacing /= peakSpacings.size();
            
            logFile << "  Average peak spacing: " << avgSpacing << " Hz" << std::endl;
            
            // Calculate spacing variance
            double variance = 0.0;
            for (double spacing : peakSpacings) {
                variance += (spacing - avgSpacing) * (spacing - avgSpacing);
            }
            variance /= peakSpacings.size();
            
            logFile << "  Peak spacing variance: " << variance << std::endl;
        }
        
        // Multi-comb should produce complex resonance patterns
        assert(combResponse.peaks.size() >= 4); // Should have multiple peaks
        assert(combResponse.notches.size() >= 2); // Should have notches
        
        logFile << "✓ Multi-comb interaction tests passed" << std::endl;
    }
    
    void testModulationEffects() {
        logFile << "\n--- Modulation Effects Tests ---" << std::endl;
        
        // Test different modulation settings
        std::vector<std::pair<float, float>> modSettings = {
            {0.2f, 0.1f}, // Slow, shallow
            {0.5f, 0.3f}, // Medium, medium
            {0.8f, 0.6f}  // Fast, deep
        };
        
        for (const auto& setting : modSettings) {
            float modRate = setting.first;
            float modDepth = setting.second;
            
            logFile << "\nTesting modulation - Rate: " << modRate 
                   << ", Depth: " << modDepth << std::endl;
            
            std::map<int, float> params = {
                {0, 0.3f}, // Base delay
                {1, 0.5f}, // Moderate feedback
                {2, 0.6f}, // Moderate resonance
                {3, modRate}, // Variable mod rate
                {4, modDepth}, // Variable mod depth
                {5, 0.5f}, // Medium frequency
                {6, 0.5f}, // Medium damping
                {7, 1.0f}  // Full wet
            };
            resonator->updateParameters(params);
            
            // Test with sustained tone
            auto sustainedInput = CombTestSignalGenerator::generateHarmonicSeries(
                220.0, 3, 0.15, 4.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, sustainedInput.size());
            for (size_t i = 0; i < sustainedInput.size(); ++i) {
                buffer.setSample(0, i, sustainedInput[i]);
                buffer.setSample(1, i, sustainedInput[i]);
            }
            
            resonator->process(buffer);
            
            // Analyze amplitude modulation
            std::vector<double> amplitudeEnvelope;
            int windowSize = static_cast<int>(TEST_SAMPLE_RATE * 0.01); // 10ms windows
            
            for (int start = 0; start < buffer.getNumSamples() - windowSize; start += windowSize / 2) {
                double rms = 0.0;
                for (int j = start; j < start + windowSize; ++j) {
                    double sample = buffer.getSample(0, j);
                    rms += sample * sample;
                }
                amplitudeEnvelope.push_back(std::sqrt(rms / windowSize));
            }
            
            // Calculate modulation depth
            double minAmp = *std::min_element(amplitudeEnvelope.begin(), amplitudeEnvelope.end());
            double maxAmp = *std::max_element(amplitudeEnvelope.begin(), amplitudeEnvelope.end());
            double modDepthMeasured = (maxAmp - minAmp) / (maxAmp + minAmp + 1e-15);
            
            logFile << "  Measured modulation depth: " << modDepthMeasured * 100.0 << "%" << std::endl;
            
            // Higher modulation settings should produce more variation
            if (modDepth > 0.4f) {
                assert(modDepthMeasured > 0.05); // At least 5% modulation
            }
            
            // Check for stability during modulation
            bool stable = true;
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(0, i);
                if (std::isnan(sample) || std::isinf(sample) || std::abs(sample) > 10.0f) {
                    stable = false;
                    break;
                }
            }
            
            assert(stable);
        }
        
        logFile << "✓ Modulation effects tests passed" << std::endl;
    }
    
    void testStabilityLimits() {
        logFile << "\n--- Stability Limits Tests ---" << std::endl;
        
        // Test extreme parameter combinations
        std::vector<std::map<int, float>> extremeSettings = {
            // Maximum feedback and resonance
            {{0, 0.5f}, {1, 0.99f}, {2, 1.0f}, {3, 0.0f}, 
             {4, 0.0f}, {5, 1.0f}, {6, 0.0f}, {7, 1.0f}},
            
            // Minimum everything
            {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, 
             {4, 0.0f}, {5, 0.0f}, {6, 1.0f}, {7, 1.0f}},
            
            // Maximum modulation
            {{0, 0.3f}, {1, 0.7f}, {2, 0.8f}, {3, 1.0f}, 
             {4, 1.0f}, {5, 0.8f}, {6, 0.2f}, {7, 1.0f}}
        };
        
        for (size_t i = 0; i < extremeSettings.size(); ++i) {
            logFile << "\nTesting extreme setting " << i + 1 << ":" << std::endl;
            
            resonator->updateParameters(extremeSettings[i]);
            
            // Test with high-level noise
            auto noiseInput = CombTestSignalGenerator::generateWhiteNoise(0.5, 2.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, noiseInput.size());
            for (size_t j = 0; j < noiseInput.size(); ++j) {
                buffer.setSample(0, j, noiseInput[j]);
                buffer.setSample(1, j, noiseInput[j]);
            }
            
            resonator->process(buffer);
            
            // Check for stability
            bool stable = true;
            double maxOutput = 0.0;
            
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                float sample = buffer.getSample(0, j);
                
                if (std::isnan(sample) || std::isinf(sample)) {
                    stable = false;
                    break;
                }
                
                maxOutput = std::max(maxOutput, static_cast<double>(std::abs(sample)));
            }
            
            logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
            logFile << "  Max output: " << maxOutput << std::endl;
            
            assert(stable);
            assert(maxOutput < 100.0); // Should not explode
        }
        
        logFile << "✓ Stability limits tests passed" << std::endl;
    }
    
    void testMusicalApplications() {
        logFile << "\n--- Musical Applications Tests ---" << std::endl;
        
        // Test pitch resonance (guitar string simulation)
        logFile << "\nTesting guitar string resonance:" << std::endl;
        
        double stringFreq = 329.63; // E4
        double delayTime = 1.0 / stringFreq;
        float delayParam = static_cast<float>(delayTime / 0.1);
        delayParam = std::min(1.0f, std::max(0.0f, delayParam));
        
        std::map<int, float> params = {
            {0, delayParam}, // Tuned to string frequency
            {1, 0.8f}, // High feedback for sustain
            {2, 0.7f}, // Good resonance
            {3, 0.15f}, // Slight vibrato
            {4, 0.1f}, // Light vibrato depth
            {5, 0.6f}, // Higher frequency range
            {6, 0.4f}, // Moderate damping
            {7, 0.9f}  // Mostly wet
        };
        resonator->updateParameters(params);
        
        // Simulate pluck with short burst
        auto pluckInput = CombTestSignalGenerator::generateBurst(
            stringFreq, 0.8, 0.01, 3.0, 0.001, 0.5, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, pluckInput.size());
        for (size_t i = 0; i < pluckInput.size(); ++i) {
            buffer.setSample(0, i, pluckInput[i]);
            buffer.setSample(1, i, pluckInput[i]);
        }
        
        resonator->process(buffer);
        
        // Analyze string-like characteristics
        std::vector<float> outputSignal(pluckInput.size());
        for (size_t i = 0; i < pluckInput.size(); ++i) {
            outputSignal[i] = buffer.getSample(0, i);
        }
        
        // Measure fundamental and harmonics
        double fundamentalLevel = measureHarmonicLevel(outputSignal, stringFreq, TEST_SAMPLE_RATE);
        double secondHarmonic = measureHarmonicLevel(outputSignal, stringFreq * 2.0, TEST_SAMPLE_RATE);
        double thirdHarmonic = measureHarmonicLevel(outputSignal, stringFreq * 3.0, TEST_SAMPLE_RATE);
        
        logFile << "  Fundamental (" << stringFreq << " Hz): " << fundamentalLevel << std::endl;
        logFile << "  2nd harmonic: " << secondHarmonic << std::endl;
        logFile << "  3rd harmonic: " << thirdHarmonic << std::endl;
        
        // Test vocal formant enhancement
        logFile << "\nTesting vocal formant enhancement:" << std::endl;
        
        // Set up for vocal formant frequencies
        double formantFreq = 800.0; // Typical vocal formant
        delayTime = 1.0 / formantFreq;
        delayParam = static_cast<float>(delayTime / 0.1);
        delayParam = std::min(1.0f, std::max(0.0f, delayParam));
        
        params[0] = delayParam; // Tuned to formant
        params[1] = 0.6f; // Moderate feedback
        params[2] = 0.9f; // High resonance for formant
        resonator->updateParameters(params);
        
        // Test with vocal-like harmonic content
        auto vocalInput = CombTestSignalGenerator::generateHarmonicSeries(
            120.0, 12, 0.1, 2.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> vocalBuffer(2, vocalInput.size());
        for (size_t i = 0; i < vocalInput.size(); ++i) {
            vocalBuffer.setSample(0, i, vocalInput[i]);
            vocalBuffer.setSample(1, i, vocalInput[i]);
        }
        
        resonator->process(vocalBuffer);
        
        // Measure formant enhancement
        std::vector<float> vocalOutput(vocalInput.size());
        for (size_t i = 0; i < vocalInput.size(); ++i) {
            vocalOutput[i] = vocalBuffer.getSample(0, i);
        }
        
        double formantInputLevel = measureHarmonicLevel(vocalInput, formantFreq, TEST_SAMPLE_RATE);
        double formantOutputLevel = measureHarmonicLevel(vocalOutput, formantFreq, TEST_SAMPLE_RATE);
        double formantEnhancement = 20.0 * std::log10(formantOutputLevel / (formantInputLevel + 1e-15));
        
        logFile << "  Formant frequency (" << formantFreq << " Hz)" << std::endl;
        logFile << "  Input level: " << formantInputLevel << std::endl;
        logFile << "  Output level: " << formantOutputLevel << std::endl;
        logFile << "  Enhancement: " << formantEnhancement << " dB" << std::endl;
        
        // Verify musical effectiveness
        assert(fundamentalLevel > 0.05); // Strong fundamental response
        assert(formantEnhancement > 5.0); // Good formant enhancement
        
        logFile << "✓ Musical applications tests passed" << std::endl;
    }
    
    // Helper function to measure harmonic level
    double measureHarmonicLevel(const std::vector<float>& signal, double frequency, double sampleRate) {
        double real = 0.0, imag = 0.0;
        double omega = 2.0 * M_PI * frequency / sampleRate;
        
        for (size_t i = 0; i < signal.size(); ++i) {
            double phase = omega * i;
            real += signal[i] * std::cos(phase);
            imag += signal[i] * std::sin(phase);
        }
        
        return std::sqrt(real * real + imag * imag) / signal.size();
    }
    
    // Helper function to generate burst signal
    std::vector<float> generateBurst(double freq, double amp, double burstDur, double totalDur, 
                                   double attack, double release, double sampleRate) {
        int totalSamples = static_cast<int>(totalDur * sampleRate);
        int burstSamples = static_cast<int>(burstDur * sampleRate);
        int attackSamples = static_cast<int>(attack * sampleRate);
        int releaseSamples = static_cast<int>(release * sampleRate);
        
        std::vector<float> signal(totalSamples, 0.0f);
        
        double phase = 0.0;
        double phaseIncrement = 2.0 * M_PI * freq / sampleRate;
        
        for (int i = 0; i < burstSamples && i < totalSamples; ++i) {
            double envelope = 1.0;
            if (i < attackSamples) {
                envelope = static_cast<double>(i) / attackSamples;
            } else if (i > burstSamples - releaseSamples) {
                int releasePos = i - (burstSamples - releaseSamples);
                envelope = 1.0 - static_cast<double>(releasePos) / releaseSamples;
            }
            
            signal[i] = static_cast<float>(amp * envelope * std::sin(phase));
            phase += phaseIncrement;
        }
        
        return signal;
    }
    
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// Main test runner
int main() {
    std::cout << "Starting Comb Resonator comprehensive test suite..." << std::endl;
    
    try {
        CombResonatorTestSuite testSuite;
        testSuite.runAllTests();
        
        std::cout << "All tests completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}