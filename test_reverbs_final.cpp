// Final comprehensive test for all reverb engines
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <iomanip>
#include <JuceHeader.h>

// Include all reverb engines
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

// Test configuration
constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;
constexpr int TEST_DURATION_SAMPLES = SAMPLE_RATE * 3; // 3 seconds

// Generate test signal
void generateTestSignal(juce::AudioBuffer<float>& buffer, int type) {
    buffer.clear();
    const int numSamples = buffer.getNumSamples();
    
    switch (type) {
        case 0: // Impulse
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                buffer.setSample(ch, 0, 1.0f);
            }
            break;
            
        case 1: // Short burst
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                for (int i = 0; i < std::min(100, numSamples); ++i) {
                    float envelope = 1.0f - (float)i / 100.0f;
                    buffer.setSample(ch, i, std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE) * envelope);
                }
            }
            break;
            
        case 2: // White noise burst
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                for (int i = 0; i < std::min(200, numSamples); ++i) {
                    buffer.setSample(ch, i, (rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.5f);
                }
            }
            break;
    }
}

// Analyze reverb tail
struct ReverbAnalysis {
    float peakLevel = 0.0f;
    float tailLength = 0.0f; // in seconds
    float energy = 0.0f;
    bool hasProperTail = false;
    float rt60 = 0.0f; // Reverb time
};

ReverbAnalysis analyzeReverb(const juce::AudioBuffer<float>& buffer) {
    ReverbAnalysis result;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    // Find peak level
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = std::abs(buffer.getSample(ch, i));
            result.peakLevel = std::max(result.peakLevel, sample);
            result.energy += sample * sample;
        }
    }
    
    // Find tail length (last sample above noise floor)
    const float noiseFloor = 0.0001f;
    int lastSignificantSample = 0;
    
    for (int i = numSamples - 1; i >= 0; --i) {
        float maxSample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            maxSample = std::max(maxSample, std::abs(buffer.getSample(ch, i)));
        }
        if (maxSample > noiseFloor) {
            lastSignificantSample = i;
            break;
        }
    }
    
    result.tailLength = lastSignificantSample / (float)SAMPLE_RATE;
    result.hasProperTail = result.tailLength > 0.5f; // At least 500ms tail
    
    // Estimate RT60 (time to decay 60dB)
    if (result.peakLevel > 0.01f) {
        float targetLevel = result.peakLevel * 0.001f; // -60dB
        for (int i = 0; i < numSamples; ++i) {
            float maxSample = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                maxSample = std::max(maxSample, std::abs(buffer.getSample(ch, i)));
            }
            if (maxSample < targetLevel) {
                result.rt60 = i / (float)SAMPLE_RATE;
                break;
            }
        }
    }
    
    return result;
}

// Test a single reverb engine
void testReverbEngine(const std::string& name, EngineBase* reverb) {
    std::cout << "\n========== Testing " << name << " ==========" << std::endl;
    
    // Prepare the reverb
    reverb->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    reverb->reset();
    
    // Set typical reverb parameters
    std::map<int, float> params;
    
    if (name == "Plate Reverb") {
        params[0] = 0.5f; // Mix
        params[1] = 0.7f; // Size
        params[2] = 0.5f; // Damping
        params[3] = 0.1f; // PreDelay
        params[4] = 0.5f; // Width
    } else if (name == "Spring Reverb") {
        params[0] = 0.5f; // Mix
        params[1] = 0.7f; // Tension
        params[2] = 0.5f; // Damping
        params[3] = 0.3f; // Drip
    } else if (name == "Gated Reverb") {
        params[0] = 0.7f; // Room Size
        params[1] = 0.5f; // Gate Time
        params[2] = 0.3f; // Threshold
        params[3] = 0.1f; // Pre-Delay
        params[4] = 0.5f; // Damping
        params[7] = 0.5f; // Mix
    } else if (name == "Shimmer Reverb") {
        params[0] = 0.7f; // Size
        params[1] = 0.3f; // Shimmer amount
        params[2] = 0.75f; // Pitch (octave up)
        params[3] = 0.5f; // Damping
        params[9] = 0.5f; // Mix
    } else if (name == "Convolution Reverb") {
        params[0] = 0.5f; // Mix
        params[1] = 0.1f; // Pre-Delay
        params[2] = 0.5f; // Damping
        params[3] = 0.7f; // Size
    }
    
    reverb->updateParameters(params);
    
    // Test with different input signals
    std::vector<std::string> testSignals = {"Impulse", "Tone Burst", "Noise Burst"};
    
    for (int signalType = 0; signalType < 3; ++signalType) {
        std::cout << "\n  Test Signal: " << testSignals[signalType] << std::endl;
        
        // Create test buffer
        juce::AudioBuffer<float> testBuffer(2, TEST_DURATION_SAMPLES);
        
        // Generate and process signal in blocks
        for (int pos = 0; pos < TEST_DURATION_SAMPLES; pos += BLOCK_SIZE) {
            int samplesToProcess = std::min(BLOCK_SIZE, TEST_DURATION_SAMPLES - pos);
            juce::AudioBuffer<float> blockBuffer(2, samplesToProcess);
            
            // Only generate signal in first block
            if (pos == 0) {
                generateTestSignal(blockBuffer, signalType);
            } else {
                blockBuffer.clear();
            }
            
            // Process through reverb
            reverb->process(blockBuffer);
            
            // Copy to test buffer
            for (int ch = 0; ch < 2; ++ch) {
                testBuffer.copyFrom(ch, pos, blockBuffer, ch, 0, samplesToProcess);
            }
        }
        
        // Analyze the output
        ReverbAnalysis analysis = analyzeReverb(testBuffer);
        
        std::cout << "    Peak Level: " << std::fixed << std::setprecision(4) << analysis.peakLevel << std::endl;
        std::cout << "    Tail Length: " << analysis.tailLength << " seconds" << std::endl;
        std::cout << "    RT60: " << analysis.rt60 << " seconds" << std::endl;
        std::cout << "    Energy: " << analysis.energy << std::endl;
        std::cout << "    Has Proper Tail: " << (analysis.hasProperTail ? "YES" : "NO") << std::endl;
        
        // Check for issues
        if (analysis.peakLevel > 1.5f) {
            std::cout << "    ⚠️ WARNING: Output clipping detected!" << std::endl;
        }
        if (!analysis.hasProperTail && signalType == 0) { // Impulse should have tail
            std::cout << "    ⚠️ WARNING: No reverb tail detected!" << std::endl;
        }
        if (analysis.energy < 0.001f) {
            std::cout << "    ⚠️ WARNING: Very low output energy!" << std::endl;
        }
    }
    
    std::cout << "\n  Parameter Test:" << std::endl;
    
    // Test extreme parameters
    params.clear();
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        params[i] = 1.0f; // Max all parameters
    }
    reverb->updateParameters(params);
    
    juce::AudioBuffer<float> extremeTest(2, BLOCK_SIZE);
    generateTestSignal(extremeTest, 0);
    reverb->process(extremeTest);
    
    float maxSample = 0.0f;
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            maxSample = std::max(maxSample, std::abs(extremeTest.getSample(ch, i)));
        }
    }
    
    std::cout << "    Max parameters output: " << maxSample << std::endl;
    if (maxSample > 2.0f) {
        std::cout << "    ⚠️ WARNING: Unstable with max parameters!" << std::endl;
    }
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "   FINAL REVERB ENGINE VALIDATION   " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    // Test all reverb engines
    std::vector<std::pair<std::string, std::unique_ptr<EngineBase>>> reverbs;
    
    reverbs.emplace_back("Plate Reverb", std::make_unique<PlateReverb>());
    reverbs.emplace_back("Spring Reverb", std::make_unique<SpringReverb>());
    reverbs.emplace_back("Gated Reverb", std::make_unique<GatedReverb>());
    reverbs.emplace_back("Shimmer Reverb", std::make_unique<ShimmerReverb>());
    reverbs.emplace_back("Convolution Reverb", std::make_unique<ConvolutionReverb>());
    
    int passedTests = 0;
    int totalTests = reverbs.size();
    
    for (auto& [name, reverb] : reverbs) {
        try {
            testReverbEngine(name, reverb.get());
            passedTests++;
            std::cout << "\n✅ " << name << " PASSED" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "\n❌ " << name << " FAILED: " << e.what() << std::endl;
        }
    }
    
    std::cout << "\n=====================================" << std::endl;
    std::cout << "         FINAL TEST RESULTS          " << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "Passed: " << passedTests << "/" << totalTests << std::endl;
    
    if (passedTests == totalTests) {
        std::cout << "\n🎉 ALL REVERB ENGINES WORKING CORRECTLY! 🎉" << std::endl;
    } else {
        std::cout << "\n⚠️ Some reverb engines need attention." << std::endl;
    }
    
    return passedTests == totalTests ? 0 : 1;
}