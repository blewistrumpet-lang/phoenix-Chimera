// Comprehensive test suite for all reverb engines
// Tests functionality, parameter response, audio quality, and edge cases

#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>
#include <numeric>
#include <random>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 512;
const int LONG_BUFFER = 88200; // 2 seconds

// Test result structure
struct TestResult {
    bool passed = false;
    std::string testName;
    std::string details;
    float score = 0.0f; // 0-100
};

// Analysis utilities
class AudioAnalyzer {
public:
    static float calculateRMS(const juce::AudioBuffer<float>& buffer, int startSample = 0, int numSamples = -1) {
        if (numSamples < 0) numSamples = buffer.getNumSamples() - startSample;
        float sum = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = startSample; i < startSample + numSamples; ++i) {
                float sample = buffer.getSample(ch, i);
                sum += sample * sample;
            }
        }
        return std::sqrt(sum / (numSamples * buffer.getNumChannels()));
    }
    
    static float calculatePeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));
            }
        }
        return peak;
    }
    
    static float calculateDecayTime(const juce::AudioBuffer<float>& buffer, float thresholdDB = -60.0f) {
        // Find peak
        float peak = calculatePeak(buffer);
        if (peak < 0.001f) return 0.0f;
        
        float threshold = peak * std::pow(10.0f, thresholdDB / 20.0f);
        
        // Find last sample above threshold
        int lastSample = 0;
        for (int i = buffer.getNumSamples() - 1; i >= 0; --i) {
            if (std::abs(buffer.getSample(0, i)) > threshold) {
                lastSample = i;
                break;
            }
        }
        
        return (float)lastSample / SAMPLE_RATE;
    }
    
    static float calculateStereoWidth(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() < 2) return 0.0f;
        
        float correlation = 0.0f;
        float leftPower = 0.0f;
        float rightPower = 0.0f;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float left = buffer.getSample(0, i);
            float right = buffer.getSample(1, i);
            correlation += left * right;
            leftPower += left * left;
            rightPower += right * right;
        }
        
        if (leftPower > 0 && rightPower > 0) {
            float normalizedCorr = correlation / std::sqrt(leftPower * rightPower);
            return 1.0f - std::abs(normalizedCorr); // 0 = mono, 1 = wide stereo
        }
        return 0.0f;
    }
    
    static bool hasNaNOrInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample) || std::isinf(sample)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    static float calculateNoiseFloor(const juce::AudioBuffer<float>& buffer) {
        // Calculate noise floor in quiet sections
        const int windowSize = 1024;
        float minRMS = 1.0f;
        
        for (int i = 0; i < buffer.getNumSamples() - windowSize; i += windowSize/2) {
            float windowRMS = calculateRMS(buffer, i, windowSize);
            if (windowRMS > 0.0f) {
                minRMS = std::min(minRMS, windowRMS);
            }
        }
        
        return 20.0f * std::log10(minRMS + 1e-10f); // Return in dB
    }
};

// Test signal generators
class TestSignalGenerator {
public:
    static void generateImpulse(juce::AudioBuffer<float>& buffer, int position = 0, float amplitude = 1.0f) {
        buffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.setSample(ch, position, amplitude);
        }
    }
    
    static void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude = 0.5f) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = amplitude * std::sin(2.0f * M_PI * frequency * i / SAMPLE_RATE);
                buffer.setSample(ch, i, sample);
            }
        }
    }
    
    static void generateChirp(juce::AudioBuffer<float>& buffer, float startFreq, float endFreq, float amplitude = 0.5f) {
        float duration = (float)buffer.getNumSamples() / SAMPLE_RATE;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float t = (float)i / SAMPLE_RATE;
                float freq = startFreq + (endFreq - startFreq) * (t / duration);
                float phase = 2.0f * M_PI * (startFreq * t + (endFreq - startFreq) * t * t / (2.0f * duration));
                buffer.setSample(ch, i, amplitude * std::sin(phase));
            }
        }
    }
    
    static void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.3f) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-amplitude, amplitude);
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                buffer.setSample(ch, i, dis(gen));
            }
        }
    }
    
    static void generateTransient(juce::AudioBuffer<float>& buffer, int attackSamples = 10, int decaySamples = 100) {
        buffer.clear();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            // Attack
            for (int i = 0; i < attackSamples && i < buffer.getNumSamples(); ++i) {
                float envelope = (float)i / attackSamples;
                buffer.setSample(ch, i, envelope * 0.8f);
            }
            // Decay
            for (int i = 0; i < decaySamples && (i + attackSamples) < buffer.getNumSamples(); ++i) {
                float envelope = 1.0f - (float)i / decaySamples;
                buffer.setSample(ch, i + attackSamples, envelope * 0.8f);
            }
        }
    }
};

// Comprehensive test class for reverb engines
template<typename ReverbType>
class ReverbTester {
private:
    ReverbType& reverb;
    std::string reverbName;
    std::vector<TestResult> results;
    
public:
    ReverbTester(ReverbType& r, const std::string& name) : reverb(r), reverbName(name) {}
    
    // Test 1: Basic functionality
    TestResult testBasicFunctionality() {
        TestResult result;
        result.testName = "Basic Functionality";
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        reverb.reset();
        
        // Set to 100% wet
        std::map<int, float> params = getDefaultParams();
        setMixParameter(params, 1.0f);
        reverb.updateParameters(params);
        
        // Process impulse
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE);
        TestSignalGenerator::generateImpulse(buffer);
        
        float inputEnergy = AudioAnalyzer::calculateRMS(buffer);
        reverb.process(buffer);
        float outputEnergy = AudioAnalyzer::calculateRMS(buffer);
        
        // Check for NaN/Inf
        if (AudioAnalyzer::hasNaNOrInf(buffer)) {
            result.details = "CRITICAL: Output contains NaN or Inf values!";
            result.score = 0.0f;
            return result;
        }
        
        // Check if reverb produces output
        result.passed = outputEnergy > inputEnergy * 0.1f;
        result.score = result.passed ? 100.0f : 0.0f;
        result.details = "Input RMS: " + std::to_string(inputEnergy) + 
                        ", Output RMS: " + std::to_string(outputEnergy);
        
        return result;
    }
    
    // Test 2: Reverb tail generation
    TestResult testReverbTail() {
        TestResult result;
        result.testName = "Reverb Tail Generation";
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        reverb.reset();
        
        std::map<int, float> params = getDefaultParams();
        setMixParameter(params, 1.0f);
        reverb.updateParameters(params);
        
        // Process impulse then empty buffers
        juce::AudioBuffer<float> impulse(2, BUFFER_SIZE);
        TestSignalGenerator::generateImpulse(impulse);
        reverb.process(impulse);
        
        // Collect tail energy over time
        std::vector<float> tailEnergies;
        for (int i = 0; i < 10; ++i) {
            juce::AudioBuffer<float> empty(2, BUFFER_SIZE);
            empty.clear();
            reverb.process(empty);
            float energy = AudioAnalyzer::calculateRMS(empty);
            tailEnergies.push_back(energy);
        }
        
        // Check if tail exists and decays
        bool hasTail = false;
        bool decays = true;
        float totalTailEnergy = 0.0f;
        
        for (size_t i = 0; i < tailEnergies.size(); ++i) {
            totalTailEnergy += tailEnergies[i];
            if (tailEnergies[i] > 0.001f) hasTail = true;
            
            // Check for proper decay (allow some fluctuation)
            if (i > 2 && tailEnergies[i] > tailEnergies[i-1] * 1.5f) {
                decays = false;
            }
        }
        
        result.passed = hasTail && decays;
        result.score = hasTail ? (decays ? 100.0f : 50.0f) : 0.0f;
        result.details = "Total tail energy: " + std::to_string(totalTailEnergy) +
                        ", Has tail: " + (hasTail ? "Yes" : "No") +
                        ", Decays properly: " + (decays ? "Yes" : "No");
        
        return result;
    }
    
    // Test 3: Mix parameter
    TestResult testMixParameter() {
        TestResult result;
        result.testName = "Mix Parameter";
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        juce::AudioBuffer<float> testSignal(2, BUFFER_SIZE);
        TestSignalGenerator::generateSineWave(testSignal, 440.0f);
        
        // Test dry (0% mix)
        reverb.reset();
        std::map<int, float> params = getDefaultParams();
        setMixParameter(params, 0.0f);
        reverb.updateParameters(params);
        
        juce::AudioBuffer<float> dry(testSignal);
        reverb.process(dry);
        float dryRMS = AudioAnalyzer::calculateRMS(dry);
        
        // Test wet (100% mix)
        reverb.reset();
        setMixParameter(params, 1.0f);
        reverb.updateParameters(params);
        
        juce::AudioBuffer<float> wet(testSignal);
        reverb.process(wet);
        float wetRMS = AudioAnalyzer::calculateRMS(wet);
        
        // Test 50% mix
        reverb.reset();
        setMixParameter(params, 0.5f);
        reverb.updateParameters(params);
        
        juce::AudioBuffer<float> mixed(testSignal);
        reverb.process(mixed);
        float mixedRMS = AudioAnalyzer::calculateRMS(mixed);
        
        // Verify mix behavior
        bool dryCorrect = std::abs(dryRMS - AudioAnalyzer::calculateRMS(testSignal)) < 0.1f;
        bool wetDifferent = std::abs(wetRMS - dryRMS) > 0.01f;
        bool mixInBetween = mixedRMS > std::min(dryRMS, wetRMS) * 0.8f && 
                            mixedRMS < std::max(dryRMS, wetRMS) * 1.2f;
        
        result.passed = dryCorrect && wetDifferent && mixInBetween;
        result.score = (dryCorrect ? 33.0f : 0.0f) + 
                      (wetDifferent ? 33.0f : 0.0f) + 
                      (mixInBetween ? 34.0f : 0.0f);
        
        result.details = "Dry RMS: " + std::to_string(dryRMS) +
                        ", Wet RMS: " + std::to_string(wetRMS) +
                        ", Mixed RMS: " + std::to_string(mixedRMS);
        
        return result;
    }
    
    // Test 4: Parameter response
    TestResult testParameterResponse() {
        TestResult result;
        result.testName = "Parameter Response";
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        
        std::map<int, float> params = getDefaultParams();
        setMixParameter(params, 1.0f);
        
        // Test with different size/decay settings
        std::vector<float> decayTimes;
        
        for (float size : {0.2f, 0.5f, 0.8f}) {
            reverb.reset();
            setSizeParameter(params, size);
            reverb.updateParameters(params);
            
            juce::AudioBuffer<float> buffer(2, LONG_BUFFER);
            TestSignalGenerator::generateImpulse(buffer);
            
            // Process in chunks
            for (int i = 0; i < LONG_BUFFER / BUFFER_SIZE; ++i) {
                juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(), 
                                              buffer.getNumChannels(),
                                              i * BUFFER_SIZE, BUFFER_SIZE);
                reverb.process(chunk);
            }
            
            float decay = AudioAnalyzer::calculateDecayTime(buffer);
            decayTimes.push_back(decay);
        }
        
        // Check if decay times increase with size
        bool responsive = true;
        for (size_t i = 1; i < decayTimes.size(); ++i) {
            if (decayTimes[i] <= decayTimes[i-1]) {
                responsive = false;
                break;
            }
        }
        
        result.passed = responsive && decayTimes.back() > 0.1f;
        result.score = result.passed ? 100.0f : (decayTimes.back() > 0.0f ? 50.0f : 0.0f);
        result.details = "Decay times (small/med/large): " +
                        std::to_string(decayTimes[0]) + "s / " +
                        std::to_string(decayTimes[1]) + "s / " +
                        std::to_string(decayTimes[2]) + "s";
        
        return result;
    }
    
    // Test 5: Stability
    TestResult testStability() {
        TestResult result;
        result.testName = "Stability (No Feedback/Explosion)";
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        reverb.reset();
        
        // Set aggressive parameters
        std::map<int, float> params = getDefaultParams();
        setMixParameter(params, 1.0f);
        setSizeParameter(params, 1.0f);
        setFeedbackParameter(params, 0.95f);
        reverb.updateParameters(params);
        
        // Process loud transient
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE * 20);
        TestSignalGenerator::generateTransient(buffer);
        
        float maxLevel = 0.0f;
        bool exploded = false;
        
        for (int i = 0; i < 20; ++i) {
            juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(),
                                          buffer.getNumChannels(),
                                          i * BUFFER_SIZE, BUFFER_SIZE);
            reverb.process(chunk);
            
            float peak = AudioAnalyzer::calculatePeak(chunk);
            maxLevel = std::max(maxLevel, peak);
            
            if (peak > 10.0f || AudioAnalyzer::hasNaNOrInf(chunk)) {
                exploded = true;
                break;
            }
        }
        
        result.passed = !exploded && maxLevel < 2.0f;
        result.score = exploded ? 0.0f : (maxLevel < 1.0f ? 100.0f : 50.0f);
        result.details = "Max level: " + std::to_string(maxLevel) +
                        ", Exploded: " + (exploded ? "Yes" : "No");
        
        return result;
    }
    
    // Test 6: Stereo imaging
    TestResult testStereoImaging() {
        TestResult result;
        result.testName = "Stereo Imaging";
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        reverb.reset();
        
        std::map<int, float> params = getDefaultParams();
        setMixParameter(params, 1.0f);
        reverb.updateParameters(params);
        
        // Process mono input (same on both channels)
        juce::AudioBuffer<float> buffer(2, BUFFER_SIZE * 4);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        
        // Process
        for (int i = 0; i < 4; ++i) {
            juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(),
                                          buffer.getNumChannels(),
                                          i * BUFFER_SIZE, BUFFER_SIZE);
            reverb.process(chunk);
        }
        
        float stereoWidth = AudioAnalyzer::calculateStereoWidth(buffer);
        
        result.passed = stereoWidth > 0.1f; // Should have some stereo spread
        result.score = std::min(100.0f, stereoWidth * 200.0f); // Score based on width
        result.details = "Stereo width: " + std::to_string(stereoWidth * 100.0f) + "%";
        
        return result;
    }
    
    // Test 7: Frequency response
    TestResult testFrequencyResponse() {
        TestResult result;
        result.testName = "Frequency Response";
        
        reverb.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
        reverb.reset();
        
        std::map<int, float> params = getDefaultParams();
        setMixParameter(params, 1.0f);
        reverb.updateParameters(params);
        
        // Test with different frequencies
        std::vector<float> frequencies = {100.0f, 500.0f, 1000.0f, 5000.0f};
        std::vector<float> responses;
        
        for (float freq : frequencies) {
            reverb.reset();
            
            juce::AudioBuffer<float> buffer(2, BUFFER_SIZE * 4);
            TestSignalGenerator::generateSineWave(buffer, freq, 0.5f);
            
            float inputRMS = AudioAnalyzer::calculateRMS(buffer);
            
            for (int i = 0; i < 4; ++i) {
                juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(),
                                              buffer.getNumChannels(),
                                              i * BUFFER_SIZE, BUFFER_SIZE);
                reverb.process(chunk);
            }
            
            float outputRMS = AudioAnalyzer::calculateRMS(buffer);
            responses.push_back(outputRMS / inputRMS);
        }
        
        // Check for reasonable frequency response
        bool balanced = true;
        for (size_t i = 1; i < responses.size(); ++i) {
            // No frequency should be more than 6dB different
            if (responses[i] > responses[0] * 2.0f || responses[i] < responses[0] * 0.5f) {
                balanced = false;
            }
        }
        
        result.passed = balanced;
        result.score = balanced ? 100.0f : 50.0f;
        result.details = "Frequency responses (100/500/1k/5k Hz): ";
        for (float resp : responses) {
            result.details += std::to_string(20.0f * std::log10(resp)) + "dB ";
        }
        
        return result;
    }
    
    // Run all tests
    void runAllTests() {
        std::cout << "\n=== Testing " << reverbName << " ===" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        results.clear();
        results.push_back(testBasicFunctionality());
        results.push_back(testReverbTail());
        results.push_back(testMixParameter());
        results.push_back(testParameterResponse());
        results.push_back(testStability());
        results.push_back(testStereoImaging());
        results.push_back(testFrequencyResponse());
        
        // Print results
        float totalScore = 0.0f;
        int passedTests = 0;
        
        for (const auto& result : results) {
            std::cout << std::setw(30) << std::left << result.testName << ": ";
            
            if (result.passed) {
                std::cout << "✓ PASS";
                passedTests++;
            } else {
                std::cout << "✗ FAIL";
            }
            
            std::cout << " (Score: " << std::fixed << std::setprecision(1) 
                     << result.score << "/100)" << std::endl;
            std::cout << "  Details: " << result.details << std::endl;
            
            totalScore += result.score;
        }
        
        float averageScore = totalScore / results.size();
        std::cout << "\nOverall: " << passedTests << "/" << results.size() 
                 << " tests passed" << std::endl;
        std::cout << "Average Score: " << std::fixed << std::setprecision(1) 
                 << averageScore << "/100" << std::endl;
        
        if (averageScore >= 80.0f) {
            std::cout << "Status: ✓ WORKING WELL" << std::endl;
        } else if (averageScore >= 50.0f) {
            std::cout << "Status: ⚠ PARTIALLY WORKING" << std::endl;
        } else {
            std::cout << "Status: ✗ NOT WORKING" << std::endl;
        }
    }
    
private:
    // Helper functions to set parameters (customize per reverb type)
    std::map<int, float> getDefaultParams() {
        std::map<int, float> params;
        
        if (reverbName == "PlateReverb") {
            params[0] = 0.5f; // Size
            params[1] = 0.5f; // Damping
            params[2] = 0.5f; // Mix
        } else if (reverbName == "SpringReverb") {
            params[0] = 0.5f; // Tension
            params[1] = 0.5f; // Damping
            params[2] = 0.5f; // Springs
            params[3] = 0.5f; // Diffusion
            params[4] = 0.5f; // Brightness
            params[5] = 0.0f; // Drip
            params[6] = 0.5f; // Mix
        } else if (reverbName == "ConvolutionReverb") {
            params[0] = 0.5f; // Mix
        } else if (reverbName == "ShimmerReverb") {
            params[0] = 0.5f; // Size
            params[1] = 0.5f; // Damping
            params[2] = 0.3f; // Shimmer
            params[3] = 0.5f; // Pitch
            params[4] = 0.3f; // Modulation
            params[5] = 0.3f; // Low cut
            params[6] = 0.7f; // High cut
            params[7] = 0.0f; // Freeze
            params[8] = 0.5f; // Mix
        } else if (reverbName == "GatedReverb") {
            params[0] = 0.5f; // Size
            params[1] = 0.5f; // Gate time
            params[2] = 0.1f; // Pre-delay
            params[3] = 0.5f; // Damping
            params[4] = 0.5f; // Diffusion
            params[5] = 0.3f; // Hold
            params[6] = 0.5f; // Mix
        }
        
        return params;
    }
    
    void setMixParameter(std::map<int, float>& params, float value) {
        if (reverbName == "PlateReverb") params[2] = value;
        else if (reverbName == "SpringReverb") params[6] = value;
        else if (reverbName == "ConvolutionReverb") params[0] = value;
        else if (reverbName == "ShimmerReverb") params[8] = value;
        else if (reverbName == "GatedReverb") params[6] = value;
    }
    
    void setSizeParameter(std::map<int, float>& params, float value) {
        if (reverbName == "PlateReverb") params[0] = value;
        else if (reverbName == "SpringReverb") params[0] = value; // Tension affects decay
        else if (reverbName == "ShimmerReverb") params[0] = value;
        else if (reverbName == "GatedReverb") params[0] = value;
    }
    
    void setFeedbackParameter(std::map<int, float>& params, float value) {
        if (reverbName == "PlateReverb") params[0] = value; // Size affects feedback
        else if (reverbName == "SpringReverb") params[0] = value; // Tension
        else if (reverbName == "ShimmerReverb") params[0] = value; // Size
        else if (reverbName == "GatedReverb") params[0] = value; // Size
    }
};

int main() {
    std::cout << "=== COMPREHENSIVE REVERB ENGINE TEST SUITE ===" << std::endl;
    std::cout << "Testing all reverb engines for functionality and quality" << std::endl;
    
    // Test PlateReverb
    {
        PlateReverb reverb;
        ReverbTester<PlateReverb> tester(reverb, "PlateReverb");
        tester.runAllTests();
    }
    
    // Test SpringReverb
    {
        SpringReverb reverb;
        ReverbTester<SpringReverb> tester(reverb, "SpringReverb");
        tester.runAllTests();
    }
    
    // Test ConvolutionReverb
    {
        ConvolutionReverb reverb;
        ReverbTester<ConvolutionReverb> tester(reverb, "ConvolutionReverb");
        tester.runAllTests();
    }
    
    // Test ShimmerReverb
    {
        ShimmerReverb reverb;
        ReverbTester<ShimmerReverb> tester(reverb, "ShimmerReverb");
        tester.runAllTests();
    }
    
    // Test GatedReverb
    {
        GatedReverb reverb;
        ReverbTester<GatedReverb> tester(reverb, "GatedReverb");
        tester.runAllTests();
    }
    
    std::cout << "\n=== TEST SUITE COMPLETE ===" << std::endl;
    std::cout << "\nSummary:" << std::endl;
    std::cout << "- Check each reverb's overall score and status" << std::endl;
    std::cout << "- Focus on fixing reverbs with scores below 50" << std::endl;
    std::cout << "- Pay special attention to CRITICAL errors (NaN/Inf)" << std::endl;
    
    return 0;
}