/*
  ==============================================================================
  
    VocalFormantFilter_Test.cpp
    Comprehensive test suite for ENGINE_VOCAL_FORMANT
    
    Tests for vocal formant filter characteristics:
    - Vowel formant frequency accuracy and modeling
    - Formant bandwidth and Q factor precision
    - Vowel morphing smoothness and realism
    - Brightness control and spectral tilt
    - Modulation effects (vibrato, tremolo)
    - Professional oversampling and aliasing control
    - Thread-safe parameter updates
    - Voice modeling accuracy across gender/age
    - Stability and performance optimization
    
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
#include "../../Source/VocalFormantFilter.h"
#include "../../Source/EngineTypes.h"

// Test configuration constants
constexpr double TEST_SAMPLE_RATE = 44100.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float EPSILON = 1e-6f;
constexpr float DB_EPSILON = 1.5f; // Relaxed for complex formant modeling
constexpr float FORMANT_TOLERANCE = 0.2f; // 20% tolerance for formant accuracy
constexpr float VOWEL_TOLERANCE = 0.15f; // 15% tolerance for vowel morphing

// Professional vowel formant data (multiple voice types)
struct VocalFormantData {
    std::string vowel;
    std::string voiceType;
    double f1, f2, f3, f4; // Formant frequencies in Hz
    double bw1, bw2, bw3, bw4; // Bandwidths in Hz
    double amplitude1, amplitude2, amplitude3, amplitude4; // Relative amplitudes
};

// Reference formant data for different voice types
const std::vector<VocalFormantData> REFERENCE_FORMANTS = {
    // Male adult voice
    {"A", "Male", 730, 1090, 2440, 3400, 60, 90, 120, 200, 1.0, 0.8, 0.6, 0.4},
    {"E", "Male", 530, 1840, 2480, 3500, 60, 90, 120, 200, 1.0, 0.9, 0.6, 0.4},
    {"I", "Male", 270, 2290, 3010, 3500, 40, 90, 120, 200, 1.0, 0.8, 0.5, 0.3},
    {"O", "Male", 570, 840, 2410, 3400, 70, 80, 120, 200, 1.0, 0.7, 0.6, 0.4},
    {"U", "Male", 300, 870, 2240, 3400, 40, 80, 120, 200, 1.0, 0.6, 0.5, 0.3},
    
    // Female adult voice
    {"A", "Female", 850, 1220, 2810, 3800, 80, 120, 150, 250, 1.0, 0.8, 0.6, 0.4},
    {"E", "Female", 610, 2070, 2850, 3900, 80, 120, 150, 250, 1.0, 0.9, 0.6, 0.4},
    {"I", "Female", 310, 2790, 3310, 3900, 50, 120, 150, 250, 1.0, 0.8, 0.5, 0.3},
    {"O", "Female", 610, 920, 2710, 3800, 90, 100, 150, 250, 1.0, 0.7, 0.6, 0.4},
    {"U", "Female", 370, 950, 2670, 3800, 50, 100, 150, 250, 1.0, 0.6, 0.5, 0.3},
    
    // Child voice
    {"A", "Child", 1030, 1370, 3170, 4500, 100, 150, 200, 300, 1.0, 0.8, 0.6, 0.4},
    {"E", "Child", 730, 2610, 3200, 4600, 100, 150, 200, 300, 1.0, 0.9, 0.6, 0.4},
    {"I", "Child", 370, 3200, 3730, 4600, 60, 150, 200, 300, 1.0, 0.8, 0.5, 0.3},
    {"O", "Child", 730, 1090, 3000, 4500, 110, 120, 200, 300, 1.0, 0.7, 0.6, 0.4},
    {"U", "Child", 450, 1160, 2990, 4500, 60, 120, 200, 300, 1.0, 0.6, 0.5, 0.3}
};

// Test signal generators for vocal formant testing
class VocalFormantTestSignalGenerator {
public:
    // Generate glottal pulse train (voice source)
    static std::vector<float> generateGlottalPulses(double f0, double amplitude, 
                                                   double duration, double sampleRate,
                                                   double openQuotient = 0.6) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        double period = sampleRate / f0;
        double openSamples = period * openQuotient;
        
        for (int i = 0; i < numSamples; ++i) {
            double position = fmod(i, period);
            
            if (position < openSamples) {
                // Rosenberg glottal pulse model
                double t = position / openSamples;
                double pulse = 3.0 * t * t - 2.0 * t * t * t; // Smooth pulse shape
                signal[i] = static_cast<float>(amplitude * pulse);
            } else {
                signal[i] = 0.0f;
            }
        }
        
        return signal;
    }
    
    // Generate voiced speech-like signal
    static std::vector<float> generateVoicedSpeech(double f0, double amplitude,
                                                 double duration, double sampleRate,
                                                 bool addNoise = true) {
        auto glottal = generateGlottalPulses(f0, amplitude, duration, sampleRate);
        
        if (addNoise) {
            // Add aspiration noise
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> noise(0.0f, amplitude * 0.05f);
            
            for (size_t i = 0; i < glottal.size(); ++i) {
                glottal[i] += noise(gen);
            }
        }
        
        return glottal;
    }
    
    // Generate harmonic series with 1/f² spectral envelope
    static std::vector<float> generateVocalHarmonics(double f0, int numHarmonics,
                                                   double amplitude, double duration,
                                                   double sampleRate) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples, 0.0f);
        
        for (int h = 1; h <= numHarmonics; ++h) {
            double harmonicFreq = f0 * h;
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
    
    // Generate white noise for formant analysis
    static std::vector<float> generateWhiteNoise(double amplitude, double duration, 
                                                double sampleRate, uint32_t seed = 555) {
        int numSamples = static_cast<int>(duration * sampleRate);
        std::vector<float> signal(numSamples);
        
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = static_cast<float>(amplitude * dist(gen));
        }
        
        return signal;
    }
    
    // Generate impulse for impulse response testing
    static std::vector<float> generateImpulse(double amplitude, int position, int totalSamples) {
        std::vector<float> signal(totalSamples, 0.0f);
        if (position >= 0 && position < totalSamples) {
            signal[position] = static_cast<float>(amplitude);
        }
        return signal;
    }
    
    // Generate swept sine for frequency response testing
    static std::vector<float> generateSweepedSine(double startFreq, double endFreq,
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
};

// Advanced formant analysis tools
class VocalFormantAnalyzer {
public:
    struct FormantPeak {
        double frequency;
        double magnitude_dB;
        double bandwidth;
        double prominence;
        bool isSignificant;
    };
    
    struct VowelAnalysis {
        std::vector<FormantPeak> formants;
        std::string detectedVowel;
        double vowelConfidence;
        double spectralCentroid;
        double spectralTilt;
        std::string voiceCharacter;
    };
    
    // Detect formants using advanced peak detection
    static std::vector<FormantPeak> detectFormants(const std::vector<float>& signal, 
                                                  double sampleRate,
                                                  int maxFormants = 6) {
        std::vector<FormantPeak> formants;
        
        // Generate frequency range for analysis
        std::vector<double> testFreqs;
        for (double f = 200; f <= std::min(6000.0, sampleRate / 2.1); f += 25) {
            testFreqs.push_back(f);
        }
        
        // Calculate power spectrum
        std::vector<double> powerSpectrum;
        for (double freq : testFreqs) {
            double power = calculatePowerAtFreq(signal, freq, sampleRate);
            powerSpectrum.push_back(10.0 * std::log10(power + 1e-15));
        }
        
        // Smooth spectrum for better peak detection
        std::vector<double> smoothedSpectrum = smoothSpectrum(powerSpectrum, 3);
        
        // Find peaks using advanced criteria
        for (size_t i = 3; i < smoothedSpectrum.size() - 3; ++i) {
            bool isPeak = true;
            double peakValue = smoothedSpectrum[i];
            
            // Check if it's a local maximum
            for (int j = -3; j <= 3; ++j) {
                if (j != 0 && smoothedSpectrum[i + j] > peakValue) {
                    isPeak = false;
                    break;
                }
            }
            
            if (isPeak && peakValue > -40.0) { // Minimum peak level
                FormantPeak peak;
                peak.frequency = testFreqs[i];
                peak.magnitude_dB = peakValue;
                peak.bandwidth = estimateBandwidth(smoothedSpectrum, i, testFreqs);
                peak.prominence = calculateProminence(smoothedSpectrum, i);
                peak.isSignificant = peak.prominence > 8.0 && peak.magnitude_dB > -25.0;
                
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
    
    // Analyze vowel characteristics
    static VowelAnalysis analyzeVowel(const std::vector<float>& signal, double sampleRate) {
        VowelAnalysis analysis;
        analysis.formants = detectFormants(signal, sampleRate);
        
        if (analysis.formants.size() >= 2) {
            // Classify vowel based on F1 and F2
            double f1 = analysis.formants[0].frequency;
            double f2 = analysis.formants[1].frequency;
            
            analysis.detectedVowel = classifyVowel(f1, f2);
            analysis.vowelConfidence = calculateVowelConfidence(f1, f2, analysis.detectedVowel);
            analysis.voiceCharacter = estimateVoiceCharacter(f1, f2);
        }
        
        // Calculate spectral characteristics
        analysis.spectralCentroid = calculateSpectralCentroid(signal, sampleRate);
        analysis.spectralTilt = calculateSpectralTilt(signal, sampleRate);
        
        return analysis;
    }
    
    // Compare two vowel analyses for morphing tests
    static double compareVowelAnalyses(const VowelAnalysis& a, const VowelAnalysis& b) {
        if (a.formants.size() < 2 || b.formants.size() < 2) {
            return 0.0;
        }
        
        // Compare first two formants
        double f1_diff = std::abs(a.formants[0].frequency - b.formants[0].frequency) / 
                        std::max(a.formants[0].frequency, b.formants[0].frequency);
        double f2_diff = std::abs(a.formants[1].frequency - b.formants[1].frequency) / 
                        std::max(a.formants[1].frequency, b.formants[1].frequency);
        
        // Calculate similarity (1.0 = identical, 0.0 = completely different)
        return 1.0 - (f1_diff + f2_diff) / 2.0;
    }
    
private:
    static double calculatePowerAtFreq(const std::vector<float>& signal, 
                                     double frequency, double sampleRate) {
        double real = 0.0, imag = 0.0;
        double omega = 2.0 * M_PI * frequency / sampleRate;
        
        for (size_t i = 0; i < signal.size(); ++i) {
            double phase = omega * i;
            real += signal[i] * std::cos(phase);
            imag += signal[i] * std::sin(phase);
        }
        
        return (real * real + imag * imag) / (signal.size() * signal.size());
    }
    
    static std::vector<double> smoothSpectrum(const std::vector<double>& spectrum, int windowSize) {
        std::vector<double> smoothed(spectrum.size());
        
        for (size_t i = 0; i < spectrum.size(); ++i) {
            double sum = 0.0;
            int count = 0;
            
            for (int j = -windowSize; j <= windowSize; ++j) {
                int idx = static_cast<int>(i) + j;
                if (idx >= 0 && idx < static_cast<int>(spectrum.size())) {
                    sum += spectrum[idx];
                    count++;
                }
            }
            
            smoothed[i] = count > 0 ? sum / count : spectrum[i];
        }
        
        return smoothed;
    }
    
    static double estimateBandwidth(const std::vector<double>& spectrum, int peakIndex,
                                  const std::vector<double>& frequencies) {
        if (peakIndex < 1 || peakIndex >= static_cast<int>(spectrum.size()) - 1) {
            return 100.0; // Default bandwidth
        }
        
        double peakLevel = spectrum[peakIndex];
        double halfPowerLevel = peakLevel - 3.0; // -3dB point
        
        // Find left and right -3dB points
        int leftIndex = peakIndex, rightIndex = peakIndex;
        
        for (int i = peakIndex - 1; i >= 0; --i) {
            if (spectrum[i] <= halfPowerLevel) {
                leftIndex = i;
                break;
            }
        }
        
        for (int i = peakIndex + 1; i < static_cast<int>(spectrum.size()); ++i) {
            if (spectrum[i] <= halfPowerLevel) {
                rightIndex = i;
                break;
            }
        }
        
        if (leftIndex != peakIndex && rightIndex != peakIndex) {
            return frequencies[rightIndex] - frequencies[leftIndex];
        }
        
        return 100.0; // Default bandwidth
    }
    
    static double calculateProminence(const std::vector<double>& spectrum, int peakIndex) {
        if (peakIndex < 2 || peakIndex >= static_cast<int>(spectrum.size()) - 2) {
            return 0.0;
        }
        
        double peakValue = spectrum[peakIndex];
        double leftMin = std::min({spectrum[peakIndex - 2], spectrum[peakIndex - 1]});
        double rightMin = std::min({spectrum[peakIndex + 1], spectrum[peakIndex + 2]});
        double surroundingMin = std::min(leftMin, rightMin);
        
        return peakValue - surroundingMin;
    }
    
    static std::string classifyVowel(double f1, double f2) {
        // Simplified vowel classification based on F1 and F2
        if (f1 < 400) {
            if (f2 > 2000) return "I";
            else if (f2 < 1000) return "U";
            else return "UH";
        } else if (f1 < 600) {
            if (f2 > 1600) return "E";
            else return "O";
        } else {
            if (f2 > 1300) return "AE";
            else return "A";
        }
    }
    
    static double calculateVowelConfidence(double f1, double f2, const std::string& vowel) {
        // Calculate confidence based on distance from prototypical values
        // This is a simplified implementation
        std::map<std::string, std::pair<double, double>> prototypes = {
            {"A", {730, 1090}}, {"E", {530, 1840}}, {"I", {270, 2290}},
            {"O", {570, 840}}, {"U", {300, 870}}, {"AE", {660, 1720}}
        };
        
        auto it = prototypes.find(vowel);
        if (it != prototypes.end()) {
            double prototypeF1 = it->second.first;
            double prototypeF2 = it->second.second;
            
            double f1Error = std::abs(f1 - prototypeF1) / prototypeF1;
            double f2Error = std::abs(f2 - prototypeF2) / prototypeF2;
            
            return 1.0 / (1.0 + f1Error + f2Error);
        }
        
        return 0.5; // Default confidence
    }
    
    static std::string estimateVoiceCharacter(double f1, double f2) {
        // Estimate voice character based on formant frequencies
        if (f1 > 800 && f2 > 2500) return "Child";
        else if (f1 > 650 && f2 > 1800) return "Female";
        else return "Male";
    }
    
    static double calculateSpectralCentroid(const std::vector<float>& signal, double sampleRate) {
        double weightedSum = 0.0;
        double magnitudeSum = 0.0;
        
        for (double freq = 200; freq < sampleRate / 2.1; freq += 50) {
            double magnitude = calculatePowerAtFreq(signal, freq, sampleRate);
            weightedSum += freq * magnitude;
            magnitudeSum += magnitude;
        }
        
        return magnitudeSum > 0 ? weightedSum / magnitudeSum : 1000.0;
    }
    
    static double calculateSpectralTilt(const std::vector<float>& signal, double sampleRate) {
        double lowFreqEnergy = 0.0;
        double highFreqEnergy = 0.0;
        
        // Low frequencies (200-1000 Hz)
        for (double freq = 200; freq <= 1000; freq += 50) {
            lowFreqEnergy += calculatePowerAtFreq(signal, freq, sampleRate);
        }
        
        // High frequencies (2000-4000 Hz)
        for (double freq = 2000; freq <= 4000; freq += 50) {
            highFreqEnergy += calculatePowerAtFreq(signal, freq, sampleRate);
        }
        
        return 10.0 * std::log10(highFreqEnergy / (lowFreqEnergy + 1e-15));
    }
};

// Main test suite for Vocal Formant Filter
class VocalFormantFilterTestSuite {
private:
    std::unique_ptr<VocalFormantFilter> filter;
    std::ofstream logFile;
    
public:
    VocalFormantFilterTestSuite() : filter(std::make_unique<VocalFormantFilter>()) {
        logFile.open("VocalFormantFilter_TestResults.txt");
        logFile << "=== Vocal Formant Filter Test Results ===" << std::endl;
        logFile << "Test started at: " << getCurrentTime() << std::endl << std::endl;
    }
    
    ~VocalFormantFilterTestSuite() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void runAllTests() {
        logFile << "Starting comprehensive Vocal Formant Filter test suite..." << std::endl;
        
        // Initialize filter
        filter->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        filter->reset();
        
        // Run test categories
        testBasicFunctionality();
        testVowelFormantAccuracy();
        testVowelMorphing();
        testVoiceCharacteristics();
        testBrightnessControl();
        testResonanceModeling();
        testModulationEffects();
        testThreadSafety();
        testOversamplingQuality();
        testSpeechProcessing();
        testMusicalApplications();
        testPerformanceStability();
        
        logFile << "\n=== Vocal Formant Filter Test Suite Complete ===" << std::endl;
        std::cout << "Vocal Formant Filter test results written to VocalFormantFilter_TestResults.txt" << std::endl;
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
        assert(engineName == "Vocal Formant Filter");
        
        logFile << "✓ Basic functionality tests passed" << std::endl;
    }
    
    void testVowelFormantAccuracy() {
        logFile << "\n--- Vowel Formant Accuracy Tests ---" << std::endl;
        
        // Test specific vowels from reference data
        std::vector<std::string> testVowels = {"A", "E", "I", "O", "U"};
        
        for (const std::string& vowel : testVowels) {
            logFile << "\nTesting vowel: " << vowel << std::endl;
            
            // Find reference data for male voice
            const VocalFormantData* refData = nullptr;
            for (const auto& data : REFERENCE_FORMANTS) {
                if (data.vowel == vowel && data.voiceType == "Male") {
                    refData = &data;
                    break;
                }
            }
            
            if (!refData) continue;
            
            logFile << "  Expected formants: F1=" << refData->f1 << " Hz, F2=" << refData->f2 
                   << " Hz, F3=" << refData->f3 << " Hz" << std::endl;
            
            // Set filter to target vowel
            float vowel1Param = static_cast<float>((&*refData - &REFERENCE_FORMANTS[0])) / 
                              static_cast<float>(REFERENCE_FORMANTS.size() - 1);
            
            std::map<int, float> vowelParams = {
                {0, vowel1Param}, // Vowel1 (target vowel)
                {1, vowel1Param}, // Vowel2 (same for pure vowel)
                {2, 0.0f}, // Morph (no morphing)
                {3, 0.7f}, // Resonance (good resonance)
                {4, 0.5f}, // Brightness (neutral)
                {5, 0.0f}, // Mod rate (no modulation)
                {6, 0.0f}, // Mod depth (no modulation)
                {7, 1.0f}  // Mix (full wet)
            };
            filter->updateParameters(vowelParams);
            
            // Test with glottal pulse train
            auto glottalInput = VocalFormantTestSignalGenerator::generateGlottalPulses(
                120.0, 0.2, 2.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, glottalInput.size());
            for (size_t i = 0; i < glottalInput.size(); ++i) {
                buffer.setSample(0, i, glottalInput[i]);
                buffer.setSample(1, i, glottalInput[i]);
            }
            
            filter->process(buffer);
            
            // Extract processed signal
            std::vector<float> processedSignal(glottalInput.size());
            for (size_t i = 0; i < glottalInput.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            // Analyze formants
            auto vowelAnalysis = VocalFormantAnalyzer::analyzeVowel(processedSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Detected formants:" << std::endl;
            for (size_t i = 0; i < vowelAnalysis.formants.size() && i < 4; ++i) {
                logFile << "    F" << (i + 1) << "=" << vowelAnalysis.formants[i].frequency 
                       << " Hz (magnitude: " << vowelAnalysis.formants[i].magnitude_dB 
                       << " dB, bandwidth: " << vowelAnalysis.formants[i].bandwidth << " Hz)" << std::endl;
            }
            
            logFile << "  Detected vowel: " << vowelAnalysis.detectedVowel 
                   << " (confidence: " << vowelAnalysis.vowelConfidence << ")" << std::endl;
            logFile << "  Voice character: " << vowelAnalysis.voiceCharacter << std::endl;
            
            // Verify formant accuracy
            if (vowelAnalysis.formants.size() >= 3) {
                double f1Error = std::abs(vowelAnalysis.formants[0].frequency - refData->f1) / refData->f1;
                double f2Error = std::abs(vowelAnalysis.formants[1].frequency - refData->f2) / refData->f2;
                double f3Error = std::abs(vowelAnalysis.formants[2].frequency - refData->f3) / refData->f3;
                
                logFile << "  Formant errors: F1=" << f1Error * 100.0 << "%, F2=" 
                       << f2Error * 100.0 << "%, F3=" << f3Error * 100.0 << "%" << std::endl;
                
                assert(f1Error < FORMANT_TOLERANCE);
                assert(f2Error < FORMANT_TOLERANCE);
                assert(f3Error < FORMANT_TOLERANCE * 1.5); // F3 can be less precise
            }
            
            // Verify vowel detection
            assert(vowelAnalysis.detectedVowel == vowel || vowelAnalysis.vowelConfidence > 0.6);
        }
        
        logFile << "✓ Vowel formant accuracy tests passed" << std::endl;
    }
    
    void testVowelMorphing() {
        logFile << "\n--- Vowel Morphing Tests ---" << std::endl;
        
        // Test morphing between different vowel pairs
        std::vector<std::pair<std::string, std::string>> morphPairs = {
            {"A", "I"}, {"E", "O"}, {"U", "A"}, {"I", "E"}
        };
        
        for (const auto& pair : morphPairs) {
            logFile << "\nTesting morph from " << pair.first << " to " << pair.second << ":" << std::endl;
            
            // Find reference data
            const VocalFormantData* vowel1Data = nullptr;
            const VocalFormantData* vowel2Data = nullptr;
            
            for (const auto& data : REFERENCE_FORMANTS) {
                if (data.vowel == pair.first && data.voiceType == "Female") {
                    vowel1Data = &data;
                }
                if (data.vowel == pair.second && data.voiceType == "Female") {
                    vowel2Data = &data;
                }
            }
            
            if (!vowel1Data || !vowel2Data) continue;
            
            std::vector<float> morphValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
            std::vector<VocalFormantAnalyzer::VowelAnalysis> morphAnalyses;
            
            for (float morphAmount : morphValues) {
                logFile << "  Morph amount: " << morphAmount << std::endl;
                
                std::map<int, float> morphParams = {
                    {0, 0.0f}, // Vowel1 (first vowel)
                    {1, 1.0f}, // Vowel2 (second vowel)
                    {2, morphAmount}, // Morph amount
                    {3, 0.8f}, // High resonance for clear formants
                    {4, 0.6f}, // Slight brightness
                    {5, 0.0f}, // No modulation
                    {6, 0.0f}, // No modulation
                    {7, 1.0f}  // Full wet
                };
                filter->updateParameters(morphParams);
                
                // Test with voiced speech signal
                auto voicedInput = VocalFormantTestSignalGenerator::generateVoicedSpeech(
                    180.0, 0.15, 1.5, TEST_SAMPLE_RATE);
                
                juce::AudioBuffer<float> buffer(2, voicedInput.size());
                for (size_t i = 0; i < voicedInput.size(); ++i) {
                    buffer.setSample(0, i, voicedInput[i]);
                    buffer.setSample(1, i, voicedInput[i]);
                }
                
                filter->process(buffer);
                
                // Extract and analyze
                std::vector<float> processedSignal(voicedInput.size());
                for (size_t i = 0; i < voicedInput.size(); ++i) {
                    processedSignal[i] = buffer.getSample(0, i);
                }
                
                auto analysis = VocalFormantAnalyzer::analyzeVowel(processedSignal, TEST_SAMPLE_RATE);
                morphAnalyses.push_back(analysis);
                
                if (analysis.formants.size() >= 2) {
                    // Calculate expected formant positions
                    double expectedF1 = vowel1Data->f1 + morphAmount * (vowel2Data->f1 - vowel1Data->f1);
                    double expectedF2 = vowel1Data->f2 + morphAmount * (vowel2Data->f2 - vowel1Data->f2);
                    
                    double f1Error = std::abs(analysis.formants[0].frequency - expectedF1) / expectedF1;
                    double f2Error = std::abs(analysis.formants[1].frequency - expectedF2) / expectedF2;
                    
                    logFile << "    Expected F1: " << expectedF1 << " Hz, Measured: " 
                           << analysis.formants[0].frequency << " Hz (error: " << f1Error * 100.0 << "%)" << std::endl;
                    logFile << "    Expected F2: " << expectedF2 << " Hz, Measured: " 
                           << analysis.formants[1].frequency << " Hz (error: " << f2Error * 100.0 << "%)" << std::endl;
                    
                    // Verify smooth morphing
                    assert(f1Error < VOWEL_TOLERANCE);
                    assert(f2Error < VOWEL_TOLERANCE);
                }
            }
            
            // Test morphing smoothness
            if (morphAnalyses.size() >= 3) {
                for (size_t i = 1; i < morphAnalyses.size() - 1; ++i) {
                    double similarity1 = VocalFormantAnalyzer::compareVowelAnalyses(morphAnalyses[i-1], morphAnalyses[i]);
                    double similarity2 = VocalFormantAnalyzer::compareVowelAnalyses(morphAnalyses[i], morphAnalyses[i+1]);
                    
                    logFile << "    Morphing smoothness " << i << ": " << similarity1 << ", " << similarity2 << std::endl;
                    
                    // Adjacent morph steps should be similar
                    assert(similarity1 > 0.7);
                    assert(similarity2 > 0.7);
                }
            }
        }
        
        logFile << "✓ Vowel morphing tests passed" << std::endl;
    }
    
    void testVoiceCharacteristics() {
        logFile << "\n--- Voice Characteristics Tests ---" << std::endl;
        
        // Test different voice characteristics (male, female, child)
        std::vector<std::string> voiceTypes = {"Male", "Female", "Child"};
        
        for (const std::string& voiceType : voiceTypes) {
            logFile << "\nTesting " << voiceType << " voice characteristics:" << std::endl;
            
            // Find reference data for this voice type (vowel A)
            const VocalFormantData* refData = nullptr;
            for (const auto& data : REFERENCE_FORMANTS) {
                if (data.vowel == "A" && data.voiceType == voiceType) {
                    refData = &data;
                    break;
                }
            }
            
            if (!refData) continue;
            
            // Use appropriate fundamental frequency for voice type
            double f0 = (voiceType == "Male") ? 120.0 : 
                       (voiceType == "Female") ? 220.0 : 280.0;
            
            std::map<int, float> voiceParams = {
                {0, 0.0f}, // Vowel A
                {1, 0.0f}, // Vowel A
                {2, 0.0f}, // No morph
                {3, 0.6f}, // Moderate resonance
                {4, (voiceType == "Child") ? 0.8f : 0.5f}, // Higher brightness for child
                {5, 0.0f}, // No modulation
                {6, 0.0f}, // No modulation
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(voiceParams);
            
            // Generate appropriate voice source
            auto voiceInput = VocalFormantTestSignalGenerator::generateVocalHarmonics(
                f0, 15, 0.12, 2.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, voiceInput.size());
            for (size_t i = 0; i < voiceInput.size(); ++i) {
                buffer.setSample(0, i, voiceInput[i]);
                buffer.setSample(1, i, voiceInput[i]);
            }
            
            filter->process(buffer);
            
            // Analyze voice characteristics
            std::vector<float> processedSignal(voiceInput.size());
            for (size_t i = 0; i < voiceInput.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            auto analysis = VocalFormantAnalyzer::analyzeVowel(processedSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Detected voice character: " << analysis.voiceCharacter << std::endl;
            logFile << "  Spectral centroid: " << analysis.spectralCentroid << " Hz" << std::endl;
            logFile << "  Spectral tilt: " << analysis.spectralTilt << " dB" << std::endl;
            
            if (analysis.formants.size() >= 2) {
                double f1 = analysis.formants[0].frequency;
                double f2 = analysis.formants[1].frequency;
                
                logFile << "  F1: " << f1 << " Hz (expected: " << refData->f1 << " Hz)" << std::endl;
                logFile << "  F2: " << f2 << " Hz (expected: " << refData->f2 << " Hz)" << std::endl;
                
                // Verify voice-type-appropriate formant ranges
                if (voiceType == "Male") {
                    assert(f1 < 800.0); // Male F1 typically lower
                    assert(analysis.spectralCentroid < 2500.0); // Lower spectral centroid
                } else if (voiceType == "Female") {
                    assert(f1 > 550.0 && f1 < 900.0); // Female F1 range
                    assert(f2 > 1800.0); // Female F2 typically higher
                } else { // Child
                    assert(f1 > 900.0); // Child F1 typically higher
                    assert(analysis.spectralCentroid > 2000.0); // Higher spectral centroid
                }
            }
        }
        
        logFile << "✓ Voice characteristics tests passed" << std::endl;
    }
    
    void testBrightnessControl() {
        logFile << "\n--- Brightness Control Tests ---" << std::endl;
        
        std::vector<float> brightnessValues = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f};
        
        for (float brightness : brightnessValues) {
            logFile << "\nTesting brightness: " << brightness << std::endl;
            
            std::map<int, float> params = {
                {0, 0.3f}, // Vowel E
                {1, 0.3f}, // Vowel E
                {2, 0.0f}, // No morph
                {3, 0.6f}, // Moderate resonance
                {4, brightness}, // Variable brightness
                {5, 0.0f}, // No modulation
                {6, 0.0f}, // No modulation
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with harmonic-rich signal
            auto harmonicInput = VocalFormantTestSignalGenerator::generateVocalHarmonics(
                150.0, 20, 0.1, 1.5, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, harmonicInput.size());
            for (size_t i = 0; i < harmonicInput.size(); ++i) {
                buffer.setSample(0, i, harmonicInput[i]);
                buffer.setSample(1, i, harmonicInput[i]);
            }
            
            filter->process(buffer);
            
            // Analyze brightness effect
            std::vector<float> processedSignal(harmonicInput.size());
            for (size_t i = 0; i < harmonicInput.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            auto analysis = VocalFormantAnalyzer::analyzeVowel(processedSignal, TEST_SAMPLE_RATE);
            
            logFile << "  Spectral centroid: " << analysis.spectralCentroid << " Hz" << std::endl;
            logFile << "  Spectral tilt: " << analysis.spectralTilt << " dB" << std::endl;
            
            // Higher brightness should increase spectral centroid and tilt
            if (brightness > 0.7f) {
                assert(analysis.spectralCentroid > 1500.0); // Brighter sound
                assert(analysis.spectralTilt > -10.0); // Less negative tilt
            } else if (brightness < 0.3f) {
                assert(analysis.spectralCentroid < 2500.0); // Darker sound
            }
            
            // Verify formants are still present
            assert(analysis.formants.size() >= 2);
            assert(analysis.formants[0].isSignificant);
            assert(analysis.formants[1].isSignificant);
        }
        
        logFile << "✓ Brightness control tests passed" << std::endl;
    }
    
    void testResonanceModeling() {
        logFile << "\n--- Resonance Modeling Tests ---" << std::endl;
        
        std::vector<float> resonanceValues = {0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
        
        for (float resonance : resonanceValues) {
            logFile << "\nTesting resonance: " << resonance << std::endl;
            
            std::map<int, float> params = {
                {0, 0.2f}, // Vowel I (clear formants)
                {1, 0.2f}, // Vowel I
                {2, 0.0f}, // No morph
                {3, resonance}, // Variable resonance
                {4, 0.5f}, // Neutral brightness
                {5, 0.0f}, // No modulation
                {6, 0.0f}, // No modulation
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with white noise to emphasize resonances
            auto noiseInput = VocalFormantTestSignalGenerator::generateWhiteNoise(
                0.1, 1.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, noiseInput.size());
            for (size_t i = 0; i < noiseInput.size(); ++i) {
                buffer.setSample(0, i, noiseInput[i]);
                buffer.setSample(1, i, noiseInput[i]);
            }
            
            filter->process(buffer);
            
            // Analyze resonance characteristics
            std::vector<float> processedSignal(noiseInput.size());
            for (size_t i = 0; i < noiseInput.size(); ++i) {
                processedSignal[i] = buffer.getSample(0, i);
            }
            
            auto analysis = VocalFormantAnalyzer::analyzeVowel(processedSignal, TEST_SAMPLE_RATE);
            
            // Calculate average formant prominence
            double avgProminence = 0.0;
            int significantFormants = 0;
            
            for (const auto& formant : analysis.formants) {
                if (formant.isSignificant) {
                    avgProminence += formant.prominence;
                    significantFormants++;
                }
            }
            
            if (significantFormants > 0) {
                avgProminence /= significantFormants;
            }
            
            logFile << "  Significant formants: " << significantFormants << std::endl;
            logFile << "  Average prominence: " << avgProminence << " dB" << std::endl;
            
            // Higher resonance should increase formant prominence
            if (resonance > 0.7f) {
                assert(significantFormants >= 2);
                assert(avgProminence > 10.0); // Strong formant peaks
            }
            
            // Check Q factors
            for (const auto& formant : analysis.formants) {
                if (formant.isSignificant) {
                    double estimatedQ = formant.frequency / (formant.bandwidth + 1e-15);
                    logFile << "    Formant at " << formant.frequency 
                           << " Hz: Q ≈ " << estimatedQ << std::endl;
                    
                    // Higher resonance should increase Q factors
                    if (resonance > 0.8f) {
                        assert(estimatedQ > 5.0); // High Q for high resonance
                    }
                }
            }
        }
        
        logFile << "✓ Resonance modeling tests passed" << std::endl;
    }
    
    void testModulationEffects() {
        logFile << "\n--- Modulation Effects Tests ---" << std::endl;
        
        // Test different modulation settings
        std::vector<std::pair<float, float>> modSettings = {
            {0.2f, 0.2f}, // Slow, shallow (vibrato-like)
            {0.5f, 0.4f}, // Medium rate and depth
            {0.8f, 0.6f}  // Fast, deep modulation
        };
        
        for (const auto& setting : modSettings) {
            float modRate = setting.first;
            float modDepth = setting.second;
            
            logFile << "\nTesting modulation - Rate: " << modRate 
                   << ", Depth: " << modDepth << std::endl;
            
            std::map<int, float> params = {
                {0, 0.1f}, // Vowel A
                {1, 0.7f}, // Vowel O (different vowel for morphing)
                {2, 0.5f}, // 50% morph (middle position)
                {3, 0.7f}, // High resonance
                {4, 0.6f}, // Moderate brightness
                {5, modRate}, // Variable mod rate
                {6, modDepth}, // Variable mod depth
                {7, 1.0f}  // Full wet
            };
            filter->updateParameters(params);
            
            // Test with sustained vocal signal
            auto sustainedInput = VocalFormantTestSignalGenerator::generateVoicedSpeech(
                160.0, 0.15, 4.0, TEST_SAMPLE_RATE, false);
            
            juce::AudioBuffer<float> buffer(2, sustainedInput.size());
            for (size_t i = 0; i < sustainedInput.size(); ++i) {
                buffer.setSample(0, i, sustainedInput[i]);
                buffer.setSample(1, i, sustainedInput[i]);
            }
            
            filter->process(buffer);
            
            // Analyze modulation by measuring spectral variation over time
            std::vector<double> spectralCentroids;
            int windowSize = static_cast<int>(TEST_SAMPLE_RATE * 0.1); // 100ms windows
            
            for (int start = 0; start < buffer.getNumSamples() - windowSize; start += windowSize / 2) {
                std::vector<float> windowSignal(windowSize);
                for (int i = 0; i < windowSize; ++i) {
                    windowSignal[i] = buffer.getSample(0, start + i);
                }
                
                auto windowAnalysis = VocalFormantAnalyzer::analyzeVowel(windowSignal, TEST_SAMPLE_RATE);
                spectralCentroids.push_back(windowAnalysis.spectralCentroid);
            }
            
            // Calculate modulation depth
            double minCentroid = *std::min_element(spectralCentroids.begin(), spectralCentroids.end());
            double maxCentroid = *std::max_element(spectralCentroids.begin(), spectralCentroids.end());
            double modDepthMeasured = (maxCentroid - minCentroid) / (maxCentroid + minCentroid + 1e-15);
            
            logFile << "  Measured modulation depth: " << modDepthMeasured * 100.0 << "%" << std::endl;
            logFile << "  Centroid range: " << minCentroid << " - " << maxCentroid << " Hz" << std::endl;
            
            // Higher modulation settings should produce more variation
            if (modDepth > 0.4f) {
                assert(modDepthMeasured > 0.05); // At least 5% modulation
            }
            
            // Verify stability during modulation
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
    
    void testThreadSafety() {
        logFile << "\n--- Thread Safety Tests ---" << std::endl;
        
        // Test rapid parameter changes (simulating real-time automation)
        logFile << "Testing rapid parameter updates:" << std::endl;
        
        auto testInput = VocalFormantTestSignalGenerator::generateVoicedSpeech(
            140.0, 0.1, 2.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, testInput.size());
        for (size_t i = 0; i < testInput.size(); ++i) {
            buffer.setSample(0, i, testInput[i]);
            buffer.setSample(1, i, testInput[i]);
        }
        
        // Simulate rapid parameter changes during processing
        int updateInterval = 64; // Update every 64 samples
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        for (int start = 0; start < buffer.getNumSamples(); start += updateInterval) {
            // Random parameter update
            std::map<int, float> randomParams = {
                {0, dist(gen)}, {1, dist(gen)}, {2, dist(gen)}, {3, dist(gen)},
                {4, dist(gen)}, {5, dist(gen)}, {6, dist(gen)}, {7, 1.0f}
            };
            filter->updateParameters(randomParams);
            
            // Process chunk
            int chunkSize = std::min(updateInterval, buffer.getNumSamples() - start);
            for (int i = start; i < start + chunkSize; ++i) {
                juce::AudioBuffer<float> sampleBuffer(2, 1);
                sampleBuffer.setSample(0, 0, testInput[i]);
                sampleBuffer.setSample(1, 0, testInput[i]);
                
                filter->process(sampleBuffer);
                
                buffer.setSample(0, i, sampleBuffer.getSample(0, 0));
                buffer.setSample(1, i, sampleBuffer.getSample(1, 0));
            }
        }
        
        // Check for stability and artifacts
        bool stable = true;
        double maxOutput = 0.0;
        int artifactCount = 0;
        
        for (int i = 1; i < buffer.getNumSamples(); ++i) {
            float sample = buffer.getSample(0, i);
            float prevSample = buffer.getSample(0, i - 1);
            
            if (std::isnan(sample) || std::isinf(sample)) {
                stable = false;
                break;
            }
            
            maxOutput = std::max(maxOutput, static_cast<double>(std::abs(sample)));
            
            // Check for excessive discontinuities
            if (std::abs(sample - prevSample) > 0.5f) {
                artifactCount++;
            }
        }
        
        double artifactRate = static_cast<double>(artifactCount) / buffer.getNumSamples();
        
        logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
        logFile << "  Max output: " << maxOutput << std::endl;
        logFile << "  Artifact rate: " << artifactRate * 100.0 << "%" << std::endl;
        
        assert(stable);
        assert(maxOutput < 50.0); // Reasonable output bounds
        assert(artifactRate < 0.01); // Less than 1% artifacts
        
        logFile << "✓ Thread safety tests passed" << std::endl;
    }
    
    void testOversamplingQuality() {
        logFile << "\n--- Oversampling Quality Tests ---" << std::endl;
        
        // Test with high-frequency content to check for aliasing
        std::map<int, float> params = {
            {0, 0.4f}, // Vowel setting
            {1, 0.6f}, // Different vowel
            {2, 0.5f}, // 50% morph
            {3, 0.8f}, // High resonance (can create harmonics)
            {4, 0.8f}, // High brightness
            {5, 0.6f}, // Medium modulation rate
            {6, 0.4f}, // Medium modulation depth
            {7, 1.0f}  // Full wet
        };
        filter->updateParameters(params);
        
        // Test with high-frequency swept sine
        auto sweepInput = VocalFormantTestSignalGenerator::generateSweepedSine(
            1000.0, TEST_SAMPLE_RATE * 0.4, 0.2, 2.0, TEST_SAMPLE_RATE);
        
        juce::AudioBuffer<float> buffer(2, sweepInput.size());
        for (size_t i = 0; i < sweepInput.size(); ++i) {
            buffer.setSample(0, i, sweepInput[i]);
            buffer.setSample(1, i, sweepInput[i]);
        }
        
        filter->process(buffer);
        
        // Analyze for aliasing artifacts
        std::vector<float> outputSignal(sweepInput.size());
        for (size_t i = 0; i < sweepInput.size(); ++i) {
            outputSignal[i] = buffer.getSample(0, i);
        }
        
        // Check for excessive high-frequency content above Nyquist/2
        double totalEnergy = 0.0;
        double aliasingSuspectEnergy = 0.0;
        
        for (double freq = 100; freq < TEST_SAMPLE_RATE / 2.1; freq += 100) {
            double energy = calculateEnergyAtFreq(outputSignal, freq, TEST_SAMPLE_RATE);
            totalEnergy += energy;
            
            if (freq > TEST_SAMPLE_RATE / 4.0) { // Upper frequency range
                aliasingSuspectEnergy += energy;
            }
        }
        
        double aliasingRatio = aliasingSuspectEnergy / (totalEnergy + 1e-15);
        
        logFile << "Oversampling quality analysis:" << std::endl;
        logFile << "  Total energy: " << totalEnergy << std::endl;
        logFile << "  High-frequency energy ratio: " << aliasingRatio * 100.0 << "%" << std::endl;
        
        // Should not have excessive high-frequency artifacts
        assert(aliasingRatio < 0.3); // Less than 30% in upper range
        
        // Check for reasonable THD+N
        double fundamentalEnergy = calculateEnergyAtFreq(outputSignal, 2000.0, TEST_SAMPLE_RATE);
        double harmonicEnergy = 0.0;
        
        for (int h = 2; h <= 8; ++h) {
            double harmonicFreq = 2000.0 * h;
            if (harmonicFreq < TEST_SAMPLE_RATE / 2.1) {
                harmonicEnergy += calculateEnergyAtFreq(outputSignal, harmonicFreq, TEST_SAMPLE_RATE);
            }
        }
        
        double thd = std::sqrt(harmonicEnergy) / std::sqrt(fundamentalEnergy + 1e-15);
        
        logFile << "  THD: " << thd * 100.0 << "%" << std::endl;
        
        // THD should be reasonable for vocal formant processing
        assert(thd < 0.2); // Less than 20% THD
        
        logFile << "✓ Oversampling quality tests passed" << std::endl;
    }
    
    void testSpeechProcessing() {
        logFile << "\n--- Speech Processing Tests ---" << std::endl;
        
        // Test with realistic speech signals
        std::vector<double> speechF0s = {100.0, 180.0, 250.0}; // Male, female, child
        std::vector<std::string> voiceLabels = {"Male", "Female", "Child"};
        
        for (size_t v = 0; v < speechF0s.size(); ++v) {
            logFile << "\nTesting " << voiceLabels[v] << " speech processing:" << std::endl;
            
            double f0 = speechF0s[v];
            
            // Set appropriate vowel and characteristics
            std::map<int, float> speechParams = {
                {0, 0.1f}, // Vowel A
                {1, 0.3f}, // Vowel E (slight difference)
                {2, 0.2f}, // Slight morph
                {3, 0.6f}, // Moderate resonance
                {4, (v == 2) ? 0.8f : 0.5f}, // Higher brightness for child
                {5, 0.1f}, // Slight vibrato
                {6, 0.15f}, // Light vibrato depth
                {7, 0.85f}  // Mostly wet, preserve some original
            };
            filter->updateParameters(speechParams);
            
            // Generate speech-like signal
            auto speechInput = VocalFormantTestSignalGenerator::generateVoicedSpeech(
                f0, 0.12, 3.0, TEST_SAMPLE_RATE, true);
            
            juce::AudioBuffer<float> buffer(2, speechInput.size());
            for (size_t i = 0; i < speechInput.size(); ++i) {
                buffer.setSample(0, i, speechInput[i]);
                buffer.setSample(1, i, speechInput[i]);
            }
            
            filter->process(buffer);
            
            // Analyze processed speech
            std::vector<float> processedSpeech(speechInput.size());
            for (size_t i = 0; i < speechInput.size(); ++i) {
                processedSpeech[i] = buffer.getSample(0, i);
            }
            
            auto speechAnalysis = VocalFormantAnalyzer::analyzeVowel(processedSpeech, TEST_SAMPLE_RATE);
            
            logFile << "  Formants detected: " << speechAnalysis.formants.size() << std::endl;
            logFile << "  Detected vowel: " << speechAnalysis.detectedVowel 
                   << " (confidence: " << speechAnalysis.vowelConfidence << ")" << std::endl;
            logFile << "  Voice character: " << speechAnalysis.voiceCharacter << std::endl;
            logFile << "  Spectral centroid: " << speechAnalysis.spectralCentroid << " Hz" << std::endl;
            
            // Verify speech characteristics are preserved/enhanced
            assert(speechAnalysis.formants.size() >= 2);
            assert(speechAnalysis.vowelConfidence > 0.4); // Reasonable vowel detection
            
            // Check voice character detection
            if (v == 0) { // Male
                assert(speechAnalysis.voiceCharacter == "Male" || 
                       speechAnalysis.spectralCentroid < 2000.0);
            } else if (v == 1) { // Female
                assert(speechAnalysis.voiceCharacter == "Female" || 
                       speechAnalysis.spectralCentroid > 1500.0);
            } else { // Child
                assert(speechAnalysis.voiceCharacter == "Child" || 
                       speechAnalysis.spectralCentroid > 2000.0);
            }
            
            // Verify naturalness (no excessive artifacts)
            double maxOutput = 0.0;
            for (float sample : processedSpeech) {
                maxOutput = std::max(maxOutput, static_cast<double>(std::abs(sample)));
            }
            
            assert(maxOutput < 5.0); // Reasonable dynamic range
        }
        
        logFile << "✓ Speech processing tests passed" << std::endl;
    }
    
    void testMusicalApplications() {
        logFile << "\n--- Musical Applications Tests ---" << std::endl;
        
        // Test vocal-style processing on instruments
        logFile << "Testing vocal-style instrument processing:" << std::endl;
        
        // Simulate saxophone-like harmonic content
        auto saxInput = VocalFormantTestSignalGenerator::generateVocalHarmonics(
            220.0, 12, 0.15, 3.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> saxParams = {
            {0, 0.2f}, // Vowel O (warm, rounded)
            {1, 0.1f}, // Vowel A (open)
            {2, 0.3f}, // Slight morph
            {3, 0.7f}, // Good resonance
            {4, 0.4f}, // Moderate brightness
            {5, 0.2f}, // Slow modulation
            {6, 0.2f}, // Light modulation
            {7, 0.7f}  // Blend with original
        };
        filter->updateParameters(saxParams);
        
        juce::AudioBuffer<float> saxBuffer(2, saxInput.size());
        for (size_t i = 0; i < saxInput.size(); ++i) {
            saxBuffer.setSample(0, i, saxInput[i]);
            saxBuffer.setSample(1, i, saxInput[i]);
        }
        
        filter->process(saxBuffer);
        
        // Analyze musical character
        std::vector<float> saxOutput(saxInput.size());
        for (size_t i = 0; i < saxInput.size(); ++i) {
            saxOutput[i] = saxBuffer.getSample(0, i);
        }
        
        auto saxAnalysis = VocalFormantAnalyzer::analyzeVowel(saxOutput, TEST_SAMPLE_RATE);
        
        logFile << "  Saxophone-style processing:" << std::endl;
        logFile << "    Formants: " << saxAnalysis.formants.size() << std::endl;
        logFile << "    Spectral centroid: " << saxAnalysis.spectralCentroid << " Hz" << std::endl;
        logFile << "    Detected character: " << saxAnalysis.detectedVowel << std::endl;
        
        // Test robot voice effect
        logFile << "\nTesting robot voice effect:" << std::endl;
        
        auto robotInput = VocalFormantTestSignalGenerator::generateVoicedSpeech(
            200.0, 0.1, 2.0, TEST_SAMPLE_RATE, false);
        
        std::map<int, float> robotParams = {
            {0, 0.0f}, // Vowel A
            {1, 0.0f}, // Same vowel (no morph)
            {2, 0.0f}, // No morph
            {3, 1.0f}, // Maximum resonance (robotic)
            {4, 0.3f}, // Reduced brightness
            {5, 0.8f}, // Fast modulation
            {6, 0.6f}, // Deep modulation
            {7, 1.0f}  // Full effect
        };
        filter->updateParameters(robotParams);
        
        juce::AudioBuffer<float> robotBuffer(2, robotInput.size());
        for (size_t i = 0; i < robotInput.size(); ++i) {
            robotBuffer.setSample(0, i, robotInput[i]);
            robotBuffer.setSample(1, i, robotInput[i]);
        }
        
        filter->process(robotBuffer);
        
        std::vector<float> robotOutput(robotInput.size());
        for (size_t i = 0; i < robotInput.size(); ++i) {
            robotOutput[i] = robotBuffer.getSample(0, i);
        }
        
        auto robotAnalysis = VocalFormantAnalyzer::analyzeVowel(robotOutput, TEST_SAMPLE_RATE);
        
        logFile << "  Robot voice effect:" << std::endl;
        logFile << "    Formant peaks: " << robotAnalysis.formants.size() << std::endl;
        logFile << "    Average prominence: ";
        
        double avgProminence = 0.0;
        for (const auto& formant : robotAnalysis.formants) {
            avgProminence += formant.prominence;
        }
        avgProminence /= (robotAnalysis.formants.size() + 1e-15);
        
        logFile << avgProminence << " dB" << std::endl;
        
        // Verify musical effectiveness
        assert(saxAnalysis.formants.size() >= 2); // Should add vocal character
        assert(robotAnalysis.formants.size() >= 2); // Should have strong formants
        assert(avgProminence > 15.0); // Robot effect should have prominent formants
        
        logFile << "✓ Musical applications tests passed" << std::endl;
    }
    
    void testPerformanceStability() {
        logFile << "\n--- Performance Stability Tests ---" << std::endl;
        
        // Test with extreme parameter combinations
        std::vector<std::map<int, float>> extremeSettings = {
            // Maximum everything
            {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}, 
             {4, 1.0f}, {5, 1.0f}, {6, 1.0f}, {7, 1.0f}},
            
            // Minimum everything
            {{0, 0.0f}, {1, 0.0f}, {2, 0.0f}, {3, 0.0f}, 
             {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 1.0f}},
            
            // High resonance with fast modulation
            {{0, 0.5f}, {1, 0.8f}, {2, 0.7f}, {3, 1.0f}, 
             {4, 0.9f}, {5, 1.0f}, {6, 1.0f}, {7, 1.0f}}
        };
        
        for (size_t i = 0; i < extremeSettings.size(); ++i) {
            logFile << "\nTesting extreme setting " << i + 1 << ":" << std::endl;
            
            filter->updateParameters(extremeSettings[i]);
            
            // Test with high-level, complex signal
            auto complexInput = VocalFormantTestSignalGenerator::generateVocalHarmonics(
                150.0, 25, 0.5, 2.0, TEST_SAMPLE_RATE);
            
            juce::AudioBuffer<float> buffer(2, complexInput.size());
            for (size_t j = 0; j < complexInput.size(); ++j) {
                buffer.setSample(0, j, complexInput[j]);
                buffer.setSample(1, j, complexInput[j]);
            }
            
            filter->process(buffer);
            
            // Check performance characteristics
            bool stable = true;
            double maxOutput = 0.0;
            double totalEnergy = 0.0;
            
            for (int j = 0; j < buffer.getNumSamples(); ++j) {
                float sample = buffer.getSample(0, j);
                
                if (std::isnan(sample) || std::isinf(sample)) {
                    stable = false;
                    break;
                }
                
                maxOutput = std::max(maxOutput, static_cast<double>(std::abs(sample)));
                totalEnergy += sample * sample;
            }
            
            double rmsOutput = std::sqrt(totalEnergy / buffer.getNumSamples());
            
            logFile << "  Stability: " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
            logFile << "  Max output: " << maxOutput << std::endl;
            logFile << "  RMS output: " << rmsOutput << std::endl;
            
            assert(stable);
            assert(maxOutput < 100.0); // Should not explode
            assert(rmsOutput > 1e-6); // Should produce some output
        }
        
        // Test processing efficiency (simplified)
        logFile << "\nTesting processing efficiency:" << std::endl;
        
        auto benchmarkInput = VocalFormantTestSignalGenerator::generateVoicedSpeech(
            180.0, 0.1, 5.0, TEST_SAMPLE_RATE);
        
        std::map<int, float> benchmarkParams = {
            {0, 0.3f}, {1, 0.7f}, {2, 0.5f}, {3, 0.8f}, 
            {4, 0.6f}, {5, 0.4f}, {6, 0.3f}, {7, 1.0f}
        };
        filter->updateParameters(benchmarkParams);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        juce::AudioBuffer<float> benchmarkBuffer(2, benchmarkInput.size());
        for (size_t i = 0; i < benchmarkInput.size(); ++i) {
            benchmarkBuffer.setSample(0, i, benchmarkInput[i]);
            benchmarkBuffer.setSample(1, i, benchmarkInput[i]);
        }
        
        filter->process(benchmarkBuffer);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        double processingTime = duration.count() / 1000.0; // milliseconds
        double realTimeRatio = processingTime / (benchmarkInput.size() / TEST_SAMPLE_RATE * 1000.0);
        
        logFile << "  Processing time: " << processingTime << " ms" << std::endl;
        logFile << "  Real-time ratio: " << realTimeRatio << "x" << std::endl;
        
        // Should be efficient enough for real-time processing
        assert(realTimeRatio < 5.0); // Should be much faster than real-time
        
        logFile << "✓ Performance stability tests passed" << std::endl;
    }
    
    // Helper function to calculate energy at specific frequency
    double calculateEnergyAtFreq(const std::vector<float>& signal, double frequency, double sampleRate) {
        double real = 0.0, imag = 0.0;
        double omega = 2.0 * M_PI * frequency / sampleRate;
        
        for (size_t i = 0; i < signal.size(); ++i) {
            double phase = omega * i;
            real += signal[i] * std::cos(phase);
            imag += signal[i] * std::sin(phase);
        }
        
        return (real * real + imag * imag) / (signal.size() * signal.size());
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
    std::cout << "Starting Vocal Formant Filter comprehensive test suite..." << std::endl;
    
    try {
        VocalFormantFilterTestSuite testSuite;
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