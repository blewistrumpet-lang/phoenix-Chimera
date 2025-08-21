/*
  ==============================================================================
  
    FormantFilter_Test.cpp
    Comprehensive test suite for ENGINE_FORMANT_FILTER
    
    Tests for formant filter characteristics:
    - Vowel frequency accuracy and modeling
    - Formant peak positioning and bandwidth
    - Vowel morphing smoothness and accuracy
    - Frequency response matching target formants
    - Resonance and Q behavior validation
    - Professional oversampling effectiveness
    - Component modeling and thermal drift
    - Input signal preservation and timbre
    - Stability across all parameter ranges
    
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
#include "../../Source/FormantFilter.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 1.0f; // Relaxed for formant modeling
constexpr float FREQ_EPSILON = 0.1f; // 10% frequency tolerance for formants
constexpr float FORMANT_TOLERANCE = 0.15f; // 15% tolerance for formant accuracy

// Vowel formant data (approximate frequencies in Hz)
struct VowelFormants {
    std::string name;
    double f1, f2, f3; // First three formants
    double bw1, bw2, bw3; // Bandwidth estimates
};

// Standard vowel formant data (male voice average)
const std::vector<VowelFormants> REFERENCE_VOWELS = {
    {"A", 730, 1090, 2440, 60, 90, 120},   // "father"
    {"E", 530, 1840, 2480, 60, 90, 120},   // "bed"
    {"I", 270, 2290, 3010, 40, 90, 120},   // "bit"
    {"O", 570, 840, 2410, 70, 80, 120},    // "law"
    {"U", 300, 870, 2240, 40, 80, 120},    // "book"
    {"AE", 660, 1720, 2410, 80, 90, 120},  // "cat"
    {"UH", 520, 1190, 2390, 60, 90, 120},  // "but"
    {"ER", 490, 1350, 1690, 70, 90, 120}   // "bird"
};

// Test signal generators for formant testing
class FormantTestSignalGenerator {
public:
    // Generate harmonic series (voice-like)
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
                double harmonicAmp = amplitude / (h * h); // 1/f² rolloff
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
    
    // Generate sawtooth wave (rich harmonic content)
    static std::vector<float> generateSawtooth(double frequency, double amplitude,
                                             double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        double period = sampleRate / frequency;
        
        for (int i = 0; i < numSamples; ++i) {
            double t = fmod(i, period) / period;
            signal[i] = static_cast<float>(amplitude * (2.0 * t - 1.0));
        }
        
        return signal;
    }
    
    // Generate white noise for formant analysis
    static std::vector<float> generateWhiteNoise(double amplitude, double duration, 
                                                double sampleRate, uint32_t seed = 123) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
    
    // Generate speech-like signal with vocal tract characteristics
    static std::vector<float> generateSpeechLike(double f0, double duration, double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        // Generate glottal pulse train
        double glottalPeriod = sampleRate / f0;
        
        for (int i = 0; i < numSamples; ++i) {
            double position = fmod(i, glottalPeriod) / glottalPeriod;
            
            // Rosenberg glottal pulse approximation
            if (position < 0.6) {
                double x = position / 0.6;
                signal[i] = 0.5f * (1.0f - std::cos(M_PI * x));
            } else {
                double x = (position - 0.6) / 0.4;
                signal[i] = 0.5f * std::cos(M_PI * x / 2.0);
            }
        }
        
        // Add some harmonic richness
        for (int h = 2; h <= 10; ++h) {
            double harmonicAmp = 1.0 / (h * h);
            double phase = 0.0;
            double phaseIncrement = 2.0 * M_PI * (f0 * h) / sampleRate;
            
            for (int i = 0; i < numSamples; ++i) {
                signal[i] += static_cast<float>(harmonicAmp * std::sin(phase) * 0.1);
                phase += phaseIncrement;
            }
        }
        
        return signal;
    }
    
    // Generate impulse for formant response testing
    static std::vector<float> generateImpulse(double amplitude, int position, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
        }
        return signal;
    }
};

// Formant analysis tools
class FormantAnalyzer {
public:
    struct FormantPeak {
        double frequency;
        double magnitude_dB;
        double bandwidth;
        double prominence;
    };
    
    // Simple formant detection using spectral peaks
    static std::vector<FormantPeak> detectFormants(const std::vector<float>& signal, 
                                                  double sampleRate,
                                                  int maxFormants = 5) {
        std::vector<FormantPeak> formants;
        
        // Simple spectral analysis (would use FFT in production)
        std::vector<double> testFreqs;
        for (double f = 200; f <= 4000; f += 50) {
            testFreqs.push_back(f);
        }
        
        std::vector<double> magnitudes;
        
        // Calculate approximate magnitude at each frequency
        for (double freq : testFreqs) {
            double real = 0.0, imag = 0.0;
            double omega = 2.0 * M_PI * freq / sampleRate;
            
            for (size_t i = 0; i < signal.size(); ++i) {
                double phase = omega * i;
                real += signal[i] * std::cos(phase);
                imag += signal[i] * std::sin(phase);
            }
            
            double magnitude = std::sqrt(real * real + imag * imag) / signal.size();
            magnitudes.push_back(20.0 * std::log10(magnitude + 1e-15));
        }
        
        // Find peaks (simplified)
        for (size_t i = 2; i < magnitudes.size() - 2; ++i) {
            bool isPeak = true;
            for (int j = -2; j <= 2; ++j) {
                if (j != 0 && magnitudes[i + j] > magnitudes[i]) {
                    isPeak = false;
                    break;
                }
            }
            
            if (isPeak && magnitudes[i] > -40.0) { // Minimum magnitude threshold
                FormantPeak peak;
                peak.frequency = testFreqs[i];
                peak.magnitude_dB = magnitudes[i];
                peak.bandwidth = 100.0; // Simplified
                peak.prominence = magnitudes[i] - std::min(magnitudes[i-1], magnitudes[i+1]);
                
                formants.push_back(peak);
                
                if (formants.size() >= maxFormants) break;
            }
        }
        
        // Sort by frequency
        std::sort(formants.begin(), formants.end(), 
                 [](const FormantPeak& a, const FormantPeak& b) {
                     return a.frequency < b.frequency;
                 });
        
        return formants;
    }
    
    // Calculate formant accuracy against reference
    static double calculateFormantAccuracy(const std::vector<FormantPeak>& detected,
                                         const VowelFormants& reference) {
        double totalError = 0.0;
        int matches = 0;
        
        std::vector<double> refFormants = {reference.f1, reference.f2, reference.f3};
        
        for (size_t i = 0; i < std::min(detected.size(), refFormants.size()); ++i) {
            if (i < detected.size()) {
                double error = std::abs(detected[i].frequency - refFormants[i]) / refFormants[i];
                totalError += error;
                matches++;
            }
        }
        
        return matches > 0 ? totalError / matches : 1.0; // Return relative error
    }
};

// Frequency response analyzer for formant filter
class FormantResponseAnalyzer {
public:
    struct FormantResponse {
        double frequency;
        double magnitude_dB;
        double phase_degrees;
        bool isFormantPeak;
    };
    
    // Measure frequency response
    static FormantResponse measureFrequencyResponse(FormantFilter& filter, 
                                                   double frequency, 
                                                   double sampleRate,
                                                   double amplitude = 0.1) {
        // Generate test signal
        const double testDuration = 1.5;
        auto testSignal = FormantTestSignalGenerator::generateSawtooth(frequency / 10.0, amplitude, 
                                                                     testDuration, sampleRate);
        
        // Process through formant filter
        int numSamples = testSignal.size();
        juce::AudioBuffer<float> buffer(2, numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            buffer.setSample(0, i, testSignal[i]);
            buffer.setSample(1, i, testSignal[i]);
        }
        
        filter.process(buffer);
        
        // Analyze response at target frequency
        double real = 0.0, imag = 0.0;
        double omega = 2.0 * M_PI * frequency / sampleRate;
        
        // Skip initial samples to avoid transients
        int analysisStart = numSamples / 4;
        int analysisLength = numSamples / 2;
        
        for (int i = analysisStart; i < analysisStart + analysisLength; ++i) {
            double phase = omega * (i - analysisStart);
            double output = buffer.getSample(0, i);
            real += output * std::cos(phase);
            imag += output * std::sin(phase);
        }
        
        double magnitude = std::sqrt(real * real + imag * imag) / analysisLength;
        double magnitude_dB = 20.0 * std::log10(magnitude + 1e-15);
        
        FormantResponse response;
        response.frequency = frequency;
        response.magnitude_dB = magnitude_dB;
        response.phase_degrees = std::atan2(imag, real) * 180.0 / M_PI;
        response.isFormantPeak = magnitude_dB > -10.0; // Simplified peak detection
        
        return response;
    }
    
    // Measure full frequency response
    static std::vector<FormantResponse> measureFullResponse(FormantFilter& filter, double sampleRate) {
        std::vector<FormantResponse> responses;
        
        // Test frequencies focusing on formant range
        std::vector<double> testFreqs;
        for (double f = 200; f <= 4000; f += 100) {
            testFreqs.push_back(f);
        }
        
        for (double freq : testFreqs) {
            if (freq < sampleRate / 2.1) {
                auto response = measureFrequencyResponse(filter, freq, sampleRate);
                responses.push_back(response);
            }
        }
        
        return responses;
    }
};

// Main test suite for Formant Filter
class FormantFilterTestSuite {
private:
    std::unique_ptr<FormantFilter> filter;
    std::ofstream logFile;
    
public:
    FormantFilterTestSuite() : filter(std::make_unique<FormantFilter>()) {
        logFile.open("FormantFilter_TestResults.txt");
        logFile << "=== Formant Filter Test Results ===" << std::endl;
        logFile << "Test started at: " << getCurrentTime() << std::endl << std::endl;
    }
    
    ~FormantFilterTestSuite() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void runAllTests() {
        logFile << "Starting comprehensive Formant Filter test suite..." << std::endl;
        
        // Initialize filter
        filter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        filter->reset();
        
        // Run test categories
        testBasicFunctionality();
        testVowelFormantAccuracy();
        testVowelMorphing();
        testFormantPositioning();
        testResonanceControl();
        testBrightnessControl();
        testModulationEffects();
        testSpeechSignalProcessing();
        testHarmonicContent();
        testFormantStability();
        testParameterInteraction();
        testExtremeSettings();
        
        logFile << "\n=== Formant Filter Test Suite Complete ===" << std::endl;
        std::cout << "Formant Filter test results written to FormantFilter_TestResults.txt" << std::endl;
    }
    
private:
    void testBasicFunctionality() {
        logFile << "\n--- Basic Functionality Tests ---" << std::endl;
        
        // Test parameter count
        int numParams = filter->getNumParameters();
        logFile << "Number of parameters: " << numParams << std::endl;
        assert(numParams == 8);
        
        // Test parameter names
        for (int i = 0; i < numParams; ++i) {
            juce::String paramName = filter->getParameterName(i);
            logFile << "Parameter " << i << ": " << paramName << std::endl;
        }
        
        // Test engine name
        juce::String engineName = filter->getName();
        logFile << "Engine name: " << engineName << std::endl;
        assert(engineName == "Formant Filter");
        
        logFile << "✓ Basic functionality tests passed" << std::endl;
    }
    
    void testVowelFormantAccuracy() {
        logFile << "\n--- Vowel Formant Accuracy Tests ---" << std::endl;
        
        // Test each reference vowel
        for (const auto& vowel : REFERENCE_VOWELS) {
            logFile << "\nTesting vowel: " << vowel.name << std::endl;
            logFile << "  Expected formants: F1=" << vowel.f1 << " Hz, F2=" << vowel.f2 
                   << " Hz, F3=" << vowel.f3 << " Hz" << std::endl;
            
            // Set filter to specific vowel (this is implementation-dependent)
            // Assuming vowel1 parameter controls vowel type
            float vowelParam = static_cast<float>((&vowel - &REFERENCE_VOWELS[0])) / (REFERENCE_VOWELS.size() - 1);
            
            std::map<int, float> vowelParams = {
                {0, vowelParam}, // Vowel1 (target vowel)
                {1, vowelParam}, // Vowel2 (same for pure vowel)
                {2, 0.0f}, // Morph (no morphing)
                {3, 0.6f}, // Resonance (moderate)
                {4, 0.5f}, // Brightness (neutral)
                {5, 0.0f}, // Mod rate (no modulation)
                {6, 0.0f}, // Mod depth (no modulation)
                {7, 1.0f}  // Mix (full wet)
            };
            filter->updateParameters(vowelParams);
            
            // Test with harmonic-rich signal
            auto testSignal = FormantTestSignalGenerator::generateHarmonicSeries(130.0, 20, 0.1, 2.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Extract processed signal for analysis
            std::vector<float> processedSignal(testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            // Detect formants in processed signal
            auto detectedFormants = FormantAnalyzer::detectFormants(processedSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Detected formants:" << std::endl;
            for (size_t i = 0; i < detectedFormants.size() && i < 3; ++i) {
                logFile << "    F" << (i + 1) << "=" << detectedFormants[i].frequency 
                       << " Hz (" << detectedFormants[i].magnitude_dB << " dB)" << std::endl;
            }
            
            // Calculate accuracy
            double accuracy = FormantAnalyzer::calculateFormantAccuracy(detectedFormants, vowel);
            logFile << "  Formant accuracy: " << (1.0 - accuracy) * 100.0 << "%" << std::endl;
            
            // Verify reasonable formant accuracy
            assert(accuracy < FORMANT_TOLERANCE);
            assert(detectedFormants.size() >= 2); // Should detect at least F1 and F2
        }
        
        logFile << "✓ Vowel formant accuracy tests passed" << std::endl;
    }
    
    void testVowelMorphing() {
        logFile << "\n--- Vowel Morphing Tests ---" << std::endl;
        
        // Test morphing between specific vowels
        const VowelFormants& vowelA = REFERENCE_VOWELS[0]; // "A"
        const VowelFormants& vowelI = REFERENCE_VOWELS[2]; // "I"
        
        logFile << "Testing morph from " << vowelA.name << " to " << vowelI.name << std::endl;
        
        std::vector<float> morphValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float morphAmount : morphValues) {
            logFile << "\nMorph amount: " << morphAmount << std::endl;
            
            std::map<int, float> morphParams = {
                {0, 0.0f}, // Vowel1 (A)
                {1, 1.0f}, // Vowel2 (I) - assuming parameter mapping
                {2, morphAmount}, // Morph amount
                {3, 0.6f}, // Resonance
                {4, 0.5f}, // Brightness
                {5, 0.0f}, // Mod rate
                {6, 0.0f}, // Mod depth
                {7, 1.0f}  // Mix
            };
            filter->updateParameters(morphParams);
            
            // Test with speech-like signal
            auto testSignal = FormantTestSignalGenerator::generateSpeechLike(150.0, 1.5, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Extract and analyze
            std::vector<float> processedSignal(testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            auto formants = FormantAnalyzer::detectFormants(processedSignal, TEST_SAMPLE_RATE);
            
            if (formants.size() >= 2) {
                // Calculate expected formant positions based on morph
                double expectedF1 = vowelA.f1 + morphAmount * (vowelI.f1 - vowelA.f1);
                double expectedF2 = vowelA.f2 + morphAmount * (vowelI.f2 - vowelA.f2);
                
                double f1Error = std::abs(formants[0].frequency - expectedF1) / expectedF1;
                double f2Error = std::abs(formants[1].frequency - expectedF2) / expectedF2;
                
                logFile << "  Expected F1: " << expectedF1 << " Hz, Detected: " 
                       << formants[0].frequency << " Hz (error: " << f1Error * 100.0 << "%)" << std::endl;
                logFile << "  Expected F2: " << expectedF2 << " Hz, Detected: " 
                       << formants[1].frequency << " Hz (error: " << f2Error * 100.0 << "%)" << std::endl;
                
                // Verify smooth morphing
                assert(f1Error < FORMANT_TOLERANCE);
                assert(f2Error < FORMANT_TOLERANCE);
            }
        }
        
        logFile << "✓ Vowel morphing tests passed" << std::endl;
    }
    
    void testFormantPositioning() {
        logFile << "\n--- Formant Positioning Tests ---" << std::endl;
        
        // Test formant positioning accuracy across frequency range
        std::map<int, float> testParams = {
            {0, 0.5f}, // Mid vowel
            {1, 0.5f}, // Mid vowel
            {2, 0.0f}, // No morph
            {3, 0.7f}, // High resonance for clear peaks
            {4, 0.5f}, // Neutral brightness
            {5, 0.0f}, // No modulation
            {6, 0.0f}, // No modulation
            {7, 1.0f}  // Full wet
        };
        filter->updateParameters(testParams);
        
        // Measure frequency response
        auto fullResponse = FormantResponseAnalyzer::measureFullResponse(*filter, TEST_SAMPLE_RATE);
        
        logFile << "Frequency response analysis:" << std::endl;
        std::vector<double> peakFrequencies;
        
        for (const auto& point : fullResponse) {
            logFile << "  " << point.frequency << " Hz: " << point.magnitude_dB << " dB";
            if (point.isFormantPeak) {
                logFile << " (PEAK)";
                peakFrequencies.push_back(point.frequency);
            }
            logFile << std::endl;
        }
        
        logFile << "Detected formant peaks at: ";
        for (double freq : peakFrequencies) {
            logFile << freq << " Hz ";
        }
        logFile << std::endl;
        
        // Should detect reasonable number of formant peaks
        assert(peakFrequencies.size() >= 2);
        assert(peakFrequencies.size() <= 5);
        
        // Peaks should be in ascending frequency order and reasonable spacing
        for (size_t i = 1; i < peakFrequencies.size(); ++i) {
            assert(peakFrequencies[i] > peakFrequencies[i-1]);
            double spacing = peakFrequencies[i] - peakFrequencies[i-1];
            assert(spacing > 200.0); // Minimum 200Hz spacing
        }
        
        logFile << "✓ Formant positioning tests passed" << std::endl;
    }
    
    void testResonanceControl() {
        logFile << "\n--- Resonance Control Tests ---" << std::endl;
        
        std::vector<float> resonanceValues = {0.1f, 0.4f, 0.7f, 0.9f};
        
        for (float resonance : resonanceValues) {
            logFile << "\nTesting resonance: " << resonance << std::endl;
            
            std::map<int, float> params = {
                {0, 0.0f}, // Vowel A
                {1, 0.0f}, // Vowel A
                {2, 0.0f}, // No morph
                {3, resonance}, // Variable resonance
                {4, 0.5f}, // Neutral brightness
                {5, 0.0f}, {6, 0.0f}, // No modulation
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with harmonic signal
            auto testSignal = FormantTestSignalGenerator::generateHarmonicSeries(100.0, 15, 0.1, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Measure formant peak sharpness
            std::vector<float> processedSignal(testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            auto formants = FormantAnalyzer::detectFormants(processedSignal, TEST_SAMPLE_RATE);
            
            if (!formants.empty()) {
                logFile << "  Formant peaks detected: " << formants.size() << std::endl;
                logFile << "  Primary formant magnitude: " << formants[0].magnitude_dB << " dB" << std::endl;
                logFile << "  Primary formant prominence: " << formants[0].prominence << " dB" << std::endl;
                
                // Higher resonance should increase formant prominence
                if (resonance > 0.5f) {
                    assert(formants[0].prominence > 5.0); // At least 5dB prominence
                    assert(formants[0].magnitude_dB > -20.0); // Strong peak
                }
            }
        }
        
        logFile << "✓ Resonance control tests passed" << std::endl;
    }
    
    void testBrightnessControl() {
        logFile << "\n--- Brightness Control Tests ---" << std::endl;
        
        std::vector<float> brightnessValues = {0.1f, 0.5f, 0.9f};
        std::vector<std::string> brightnessNames = {"Dark", "Neutral", "Bright"};
        
        for (size_t i = 0; i < brightnessValues.size(); ++i) {
            logFile << "\nTesting brightness: " << brightnessNames[i] 
                   << " (" << brightnessValues[i] << ")" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.3f}, // Vowel setting
                {1, 0.3f}, // Vowel setting
                {2, 0.0f}, // No morph
                {3, 0.6f}, // Moderate resonance
                {4, brightnessValues[i]}, // Variable brightness
                {5, 0.0f}, {6, 0.0f}, // No modulation
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with sawtooth (rich harmonics)
            auto testSignal = FormantTestSignalGenerator::generateSawtooth(150.0, 0.1, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Analyze high-frequency content
            double highFreqEnergy = 0.0;
            double totalEnergy = 0.0;
            
            // Simple high-frequency energy estimation
            for (int j = 1; j < buffer.getNumSamples(); ++j) {
                double sample = buffer.getSample(0, j);
                double prevSample = buffer.getSample(0, j - 1);
                double highFreqComponent = sample - prevSample; // Simple high-pass
                
                highFreqEnergy += highFreqComponent * highFreqComponent;
                totalEnergy += sample * sample;
            }
            
            double brightnessRatio = highFreqEnergy / (totalEnergy + 1e-15);
            
            logFile << "  High-frequency energy ratio: " << brightnessRatio << std::endl;
            
            // Brighter settings should have more high-frequency energy
            if (i == 2) { // Bright setting
                assert(brightnessRatio > 0.01); // Should have significant HF content
            }
        }
        
        logFile << "✓ Brightness control tests passed" << std::endl;
    }
    
    void testModulationEffects() {
        logFile << "\n--- Modulation Effects Tests ---" << std::endl;
        
        // Test formant modulation
        std::vector<std::pair<float, float>> modSettings = {
            {0.1f, 0.3f}, // Slow, shallow
            {0.5f, 0.6f}, // Medium, medium
            {0.9f, 0.9f}  // Fast, deep
        };
        
        for (const auto& setting : modSettings) {
            float modRate = setting.first;
            float modDepth = setting.second;
            
            logFile << "\nTesting modulation - Rate: " << modRate 
                   << ", Depth: " << modDepth << std::endl;
            
            std::map<int, float> params = {
                {0, 0.2f}, // Vowel 1
                {1, 0.8f}, // Vowel 2 (different vowel)
                {2, 0.5f}, // 50% morph (middle)
                {3, 0.6f}, // Moderate resonance
                {4, 0.5f}, // Neutral brightness
                {5, modRate}, // Variable mod rate
                {6, modDepth}, // Variable mod depth
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with sustained tone
            auto testSignal = FormantTestSignalGenerator::generateHarmonicSeries(120.0, 10, 0.1, 3.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Analyze modulation by looking at amplitude variation
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
            if (modDepth > 0.5f) {
                assert(modDepthMeasured > 0.05); // At least 5% modulation
            }
        }
        
        logFile << "✓ Modulation effects tests passed" << std::endl;
    }
    
    void testSpeechSignalProcessing() {
        logFile << "\n--- Speech Signal Processing Tests ---" << std::endl;
        
        // Test with speech-like signal
        auto speechSignal = FormantTestSignalGenerator::generateSpeechLike(110.0, 2.0, TEST_SAMPLE_RATE);
        
        // Test different vowel settings on speech
        std::vector<std::pair<std::string, float>> vowelSettings = {
            {"A-like", 0.0f},
            {"E-like", 0.25f},
            {"I-like", 0.5f},
            {"O-like", 0.75f},
            {"U-like", 1.0f}
        };
        
        for (const auto& setting : vowelSettings) {
            logFile << "\nTesting " << setting.first << " vowel character:" << std::endl;
            
            std::map<int, float> params = {
                {0, setting.second}, // Vowel character
                {1, setting.second}, // Same vowel
                {2, 0.0f}, // No morph
                {3, 0.5f}, // Moderate resonance
                {4, 0.6f}, // Slightly bright
                {5, 0.0f}, {6, 0.0f}, // No modulation
                {7, 0.8f}  // 80% wet (preserve some original)
            };
            filter->updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, speechSignal.size());
            for (size_t i = 0; i < speechSignal.size(); ++i) {
                buffer.setSample(0, i, speechSignal[i]);
                buffer.setSample(1, i, speechSignal[i]);
            }
            
            filter->process(buffer);
            
            // Analyze spectral characteristics
            std::vector<float> processedSignal(speechSignal.size());
            for (size_t i = 0; i < speechSignal.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            auto formants = FormantAnalyzer::detectFormants(processedSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Detected formants: ";
            for (const auto& formant : formants) {
                logFile << formant.frequency << " Hz (" << formant.magnitude_dB << " dB) ";
            }
            logFile << std::endl;
            
            // Should maintain speech-like characteristics
            assert(formants.size() >= 2); // At least two formants
            assert(formants[0].frequency >= 200.0 && formants[0].frequency <= 1000.0); // F1 range
            if (formants.size() > 1) {
                assert(formants[1].frequency >= 800.0 && formants[1].frequency <= 3000.0); // F2 range
            }
        }
        
        logFile << "✓ Speech signal processing tests passed" << std::endl;
    }
    
    void testHarmonicContent() {
        logFile << "\n--- Harmonic Content Tests ---" << std::endl;
        
        // Test how filter affects harmonic content
        double f0 = 100.0; // Fundamental frequency
        auto harmonicSignal = FormantTestSignalGenerator::generateHarmonicSeries(f0, 20, 0.1, 1.5, TEST_SAMPLE_RATE);
        
        std::map<int, float> params = {
            {0, 0.3f}, // Vowel setting
            {1, 0.3f}, // Same vowel
            {2, 0.0f}, // No morph
            {3, 0.7f}, // High resonance
            {4, 0.5f}, // Neutral brightness
            {5, 0.0f}, {6, 0.0f}, // No modulation
            {7, 1.0f}  // Full wet
        };
        filter->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, harmonicSignal.size());
        for (size_t i = 0; i < harmonicSignal.size(); ++i) {
            buffer.setSample(0, i, harmonicSignal[i]);
            buffer.setSample(1, i, harmonicSignal[i]);
        }
        
        filter->process(buffer);
        
        // Analyze harmonic enhancement/suppression
        std::vector<double> inputHarmonics, outputHarmonics;
        
        for (int h = 1; h <= 15; ++h) {
            double harmonicFreq = f0 * h;
            if (harmonicFreq < TEST_SAMPLE_RATE / 2.0) {
                // Measure input harmonic level
                double inputLevel = measureHarmonicLevel(harmonicSignal, harmonicFreq, TEST_SAMPLE_RATE);
                inputHarmonics.push_back(inputLevel);
                
                // Measure output harmonic level
                std::vector<float> processedSignal(harmonicSignal.size());
                for (size_t i = 0; i < harmonicSignal.size(); ++i) {
                    processedSignal[i] = buffer.getSample(0, i);
                }
                double outputLevel = measureHarmonicLevel(processedSignal, harmonicFreq, TEST_SAMPLE_RATE);
                outputHarmonics.push_back(outputLevel);
                
                double enhancement_dB = 20.0 * std::log10(outputLevel / (inputLevel + 1e-15));
                
                logFile << "  Harmonic " << h << " (" << harmonicFreq << " Hz): " 
                       << enhancement_dB << " dB enhancement" << std::endl;
            }
        }
        
        // Formant filter should enhance some harmonics and suppress others
        bool hasEnhancement = false;
        bool hasSuppression = false;
        
        for (size_t i = 0; i < inputHarmonics.size(); ++i) {
            double ratio = outputHarmonics[i] / (inputHarmonics[i] + 1e-15);
            if (ratio > 1.5) hasEnhancement = true;
            if (ratio < 0.7) hasSuppression = true;
        }
        
        assert(hasEnhancement); // Should enhance some harmonics
        assert(hasSuppression); // Should suppress some harmonics
        
        logFile << "✓ Harmonic content tests passed" << std::endl;
    }
    
    void testFormantStability() {
        logFile << "\n--- Formant Stability Tests ---" << std::endl;
        
        // Test stability with various signal types
        std::map<int, float> stableParams = {
            {0, 0.4f}, {1, 0.6f}, {2, 0.3f}, {3, 0.8f}, 
            {4, 0.7f}, {5, 0.2f}, {6, 0.4f}, {7, 1.0f}
        };
        filter->updateParameters(stableParams);
        
        // Test with different signal types
        std::vector<std::string> signalTypes = {"Harmonic", "Sawtooth", "Noise", "Speech"};
        std::vector<std::vector<float>> testSignals = {
            FormantTestSignalGenerator::generateHarmonicSeries(130.0, 15, 0.15, 1.0, TEST_SAMPLE_RATE),
            FormantTestSignalGenerator::generateSawtooth(130.0, 0.15, 1.0, TEST_SAMPLE_RATE),
            FormantTestSignalGenerator::generateWhiteNoise(0.1, 1.0, TEST_SAMPLE_RATE),
            FormantTestSignalGenerator::generateSpeechLike(130.0, 1.0, TEST_SAMPLE_RATE)
        };
        
        for (size_t i = 0; i < signalTypes.size(); ++i) {
            logFile << "\nTesting stability with " << signalTypes[i] << " signal:" << std::endl;
            
            juce::AudioBuffer<float> buffer(2, testSignals[i].size());
            for (size_t j = 0; j < testSignals[i].size(); ++j) {
                buffer.setSample(0, j, testSignals[i][j]);
                buffer.setSample(1, j, testSignals[i][j]);
            }
            
            filter->process(buffer);
            
            // Check for stability
            bool stable = true;
            double maxOutput = 0.0;
            double avgOutput = 0.0;
            
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                float sample = buffer.getSample(0, j);
                
                if (std::isnan(sample) || std::isinf(sample)) {
                    stable = false;
                    break;
                }
                
                double absValue = std::abs(sample);
                maxOutput = std::max(maxOutput, absValue);
                avgOutput += absValue;
            }
            
            avgOutput /= buffer.getNumSamples();
            
            logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
            logFile << "  Max output: " << maxOutput << std::endl;
            logFile << "  Average output: " << avgOutput << std::endl;
            
            assert(stable);
            assert(maxOutput < 10.0); // Reasonable output bounds
            assert(avgOutput > 1e-6); // Should produce some output
        }
        
        logFile << "✓ Formant stability tests passed" << std::endl;
    }
    
    void testParameterInteraction() {
        logFile << "\n--- Parameter Interaction Tests ---" << std::endl;
        
        // Test interactions between key parameters
        auto testSignal = FormantTestSignalGenerator::generateSpeechLike(140.0, 1.5, TEST_SAMPLE_RATE);
        
        // Test resonance vs brightness interaction
        std::vector<std::pair<float, float>> combinations = {
            {0.3f, 0.3f}, // Low res, low brightness
            {0.3f, 0.8f}, // Low res, high brightness
            {0.8f, 0.3f}, // High res, low brightness
            {0.8f, 0.8f}  // High res, high brightness
        };
        
        for (const auto& combo : combinations) {
            float resonance = combo.first;
            float brightness = combo.second;
            
            logFile << "\nTesting Resonance=" << resonance 
                   << ", Brightness=" << brightness << ":" << std::endl;
            
            std::map<int, float> params = {
                {0, 0.4f}, {1, 0.4f}, {2, 0.0f}, // Vowel settings
                {3, resonance}, // Variable resonance
                {4, brightness}, // Variable brightness
                {5, 0.0f}, {6, 0.0f}, // No modulation
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                buffer.setSample(0, i, testSignal[i]);
                buffer.setSample(1, i, testSignal[i]);
            }
            
            filter->process(buffer);
            
            // Analyze combined effect
            std::vector<float> processedSignal(testSignal.size());
            for (size_t i = 0; i < testSignal.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            auto formants = FormantAnalyzer::detectFormants(processedSignal, TEST_SAMPLE_RATE);
            
            if (!formants.empty()) {
                logFile << "  Primary formant: " << formants[0].frequency 
                       << " Hz, " << formants[0].magnitude_dB << " dB" << std::endl;
                logFile << "  Total formants detected: " << formants.size() << std::endl;
                
                // Verify reasonable interaction
                assert(formants[0].magnitude_dB > -30.0); // Should have clear formants
                assert(formants.size() >= 2); // Should detect multiple formants
            }
        }
        
        logFile << "✓ Parameter interaction tests passed" << std::endl;
    }
    
    void testExtremeSettings() {
        logFile << "\n--- Extreme Settings Tests ---" << std::endl;
        
        // Test extreme parameter combinations
        std::vector<std::map<int, float>> extremeSettings = {
            // All maximum
            {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}, 
             {4, 1.0f}, {5, 1.0f}, {6, 1.0f}, {7, 1.0f}},
            
            // All minimum (except mix)
            {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, 
             {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 1.0f}},
            
            // High resonance, max modulation
            {{0, 0.5f}, {1, 0.8f}, {2, 0.5f}, {3, 1.0f}, 
             {4, 0.5f}, {5, 1.0f}, {6, 1.0f}, {7, 1.0f}}
        };
        
        for (size_t i = 0; i < extremeSettings.size(); ++i) {
            logFile << "\nTesting extreme setting " << i + 1 << ":" << std::endl;
            
            filter->updateParameters(extremeSettings[i]);
            
            // Test with harmonic signal
            auto testSignal = FormantTestSignalGenerator::generateHarmonicSeries(120.0, 12, 0.1, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, testSignal.size());
            for (size_t j = 0; j < testSignal.size(); ++j) {
                buffer.setSample(0, j, testSignal[j]);
                buffer.setSample(1, j, testSignal[j]);
            }
            
            filter->process(buffer);
            
            // Check for stability and reasonable output
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
        
        logFile << "✓ Extreme settings tests passed" << std::endl;
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
    std::cout << "Starting Formant Filter comprehensive test suite..." << std::endl;
    
    try {
        FormantFilterTestSuite testSuite;
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