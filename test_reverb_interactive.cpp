// Interactive reverb test with continuous audio input
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <iomanip>
#include <map>
#include <JuceHeader.h>

// Include reverb engines
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;

// Generate various test signals
enum class TestSignal {
    SILENCE,
    IMPULSE,
    SINE_440,
    SINE_SWEEP,
    WHITE_NOISE,
    PINK_NOISE,
    DRUM_HIT,
    VOCAL_SAMPLE
};

class SignalGenerator {
    float phase = 0;
    float sweepFreq = 100;
    float pinkState = 0;
    int sampleCounter = 0;
    
public:
    void generate(juce::AudioBuffer<float>& buffer, TestSignal type) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            
            for (int i = 0; i < numSamples; ++i) {
                float sample = 0;
                
                switch (type) {
                    case TestSignal::SILENCE:
                        sample = 0;
                        break;
                        
                    case TestSignal::IMPULSE:
                        sample = (sampleCounter % 48000 == 0) ? 1.0f : 0.0f;
                        break;
                        
                    case TestSignal::SINE_440:
                        sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * phase);
                        phase += 1.0f / SAMPLE_RATE;
                        if (phase > 1.0f) phase -= 1.0f;
                        break;
                        
                    case TestSignal::SINE_SWEEP:
                        sample = 0.5f * std::sin(2.0f * M_PI * sweepFreq * phase);
                        phase += 1.0f / SAMPLE_RATE;
                        if (phase > 1.0f) phase -= 1.0f;
                        sweepFreq *= 1.00001f;
                        if (sweepFreq > 4000) sweepFreq = 100;
                        break;
                        
                    case TestSignal::WHITE_NOISE:
                        sample = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.3f;
                        break;
                        
                    case TestSignal::PINK_NOISE:
                        {
                            float white = (rand() / (float)RAND_MAX * 2.0f - 1.0f);
                            pinkState = pinkState * 0.99f + white * 0.01f;
                            sample = pinkState * 0.3f;
                        }
                        break;
                        
                    case TestSignal::DRUM_HIT:
                        {
                            int cyclePos = sampleCounter % 24000; // 0.5 second cycle
                            if (cyclePos < 1000) {
                                float env = std::exp(-cyclePos * 0.005f);
                                float tone = std::sin(2.0f * M_PI * 60.0f * cyclePos / SAMPLE_RATE);
                                float click = (rand() / (float)RAND_MAX * 2.0f - 1.0f);
                                sample = env * (tone * 0.7f + click * 0.3f);
                            }
                        }
                        break;
                        
                    case TestSignal::VOCAL_SAMPLE:
                        {
                            // Simulated vocal formants
                            float f1 = std::sin(2.0f * M_PI * 700.0f * phase);
                            float f2 = std::sin(2.0f * M_PI * 1220.0f * phase);
                            float f3 = std::sin(2.0f * M_PI * 2600.0f * phase);
                            float env = 0.5f + 0.5f * std::sin(2.0f * M_PI * 4.0f * phase); // Vibrato
                            sample = env * 0.3f * (f1 * 0.5f + f2 * 0.3f + f3 * 0.2f);
                            phase += 1.0f / SAMPLE_RATE;
                            if (phase > 1.0f) phase -= 1.0f;
                        }
                        break;
                }
                
                data[i] = sample;
                sampleCounter++;
            }
        }
    }
};

void testReverbWithSignal(const std::string& name, EngineBase* reverb, TestSignal signalType) {
    std::cout << "\nTesting " << name << " with ";
    
    switch (signalType) {
        case TestSignal::SILENCE: std::cout << "SILENCE"; break;
        case TestSignal::IMPULSE: std::cout << "IMPULSE"; break;
        case TestSignal::SINE_440: std::cout << "SINE 440Hz"; break;
        case TestSignal::SINE_SWEEP: std::cout << "SINE SWEEP"; break;
        case TestSignal::WHITE_NOISE: std::cout << "WHITE NOISE"; break;
        case TestSignal::PINK_NOISE: std::cout << "PINK NOISE"; break;
        case TestSignal::DRUM_HIT: std::cout << "DRUM HIT"; break;
        case TestSignal::VOCAL_SAMPLE: std::cout << "VOCAL SAMPLE"; break;
    }
    std::cout << std::endl;
    
    reverb->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
    reverb->reset();
    
    SignalGenerator generator;
    
    // Test each parameter at different values
    for (int paramIdx = 0; paramIdx < reverb->getNumParameters(); ++paramIdx) {
        juce::String paramName = reverb->getParameterName(paramIdx);
        std::cout << "  Parameter " << paramIdx << " (" << paramName << "): ";
        
        std::vector<float> testValues = {0.0f, 0.5f, 1.0f};
        std::vector<float> outputLevels;
        
        for (float value : testValues) {
            // Set parameters
            std::map<int, float> params;
            for (int i = 0; i < reverb->getNumParameters(); ++i) {
                params[i] = 0.5f; // Default
            }
            params[paramIdx] = value;
            reverb->updateParameters(params);
            reverb->reset();
            
            // Process several blocks
            float totalEnergy = 0;
            for (int block = 0; block < 10; ++block) {
                juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
                generator.generate(buffer, signalType);
                
                reverb->process(buffer);
                
                // Measure output energy
                for (int ch = 0; ch < 2; ++ch) {
                    const float* data = buffer.getReadPointer(ch);
                    for (int i = 0; i < BLOCK_SIZE; ++i) {
                        totalEnergy += data[i] * data[i];
                    }
                }
            }
            
            outputLevels.push_back(std::sqrt(totalEnergy / (10 * BLOCK_SIZE * 2)));
        }
        
        // Check if parameter has effect
        float maxDiff = 0;
        for (size_t i = 1; i < outputLevels.size(); ++i) {
            maxDiff = std::max(maxDiff, std::abs(outputLevels[i] - outputLevels[0]));
        }
        
        if (maxDiff > 0.001f) {
            std::cout << "✅ Working (diff=" << maxDiff << ")" << std::endl;
        } else {
            std::cout << "❌ No effect" << std::endl;
        }
    }
}

int main() {
    std::cout << "=====================================" << std::endl;
    std::cout << "   INTERACTIVE REVERB TEST SUITE    " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    // Test each reverb with different signals
    std::vector<TestSignal> testSignals = {
        TestSignal::IMPULSE,
        TestSignal::SINE_440,
        TestSignal::WHITE_NOISE,
        TestSignal::DRUM_HIT
    };
    
    std::cout << "\n========== PLATE REVERB ==========";
    auto plate = std::make_unique<PlateReverb>();
    for (auto signal : testSignals) {
        testReverbWithSignal("PlateReverb", plate.get(), signal);
    }
    
    std::cout << "\n========== SPRING REVERB ==========";
    auto spring = std::make_unique<SpringReverb>();
    for (auto signal : testSignals) {
        testReverbWithSignal("SpringReverb", spring.get(), signal);
    }
    
    std::cout << "\n========== GATED REVERB ==========";
    auto gated = std::make_unique<GatedReverb>();
    for (auto signal : testSignals) {
        testReverbWithSignal("GatedReverb", gated.get(), signal);
    }
    
    std::cout << "\n========== SHIMMER REVERB ==========";
    auto shimmer = std::make_unique<ShimmerReverb>();
    for (auto signal : testSignals) {
        testReverbWithSignal("ShimmerReverb", shimmer.get(), signal);
    }
    
    std::cout << "\n========== CONVOLUTION REVERB ==========";
    auto convolution = std::make_unique<ConvolutionReverb>();
    for (auto signal : testSignals) {
        testReverbWithSignal("ConvolutionReverb", convolution.get(), signal);
    }
    
    std::cout << "\n=====================================" << std::endl;
    std::cout << "        TEST SUITE COMPLETE          " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    return 0;
}