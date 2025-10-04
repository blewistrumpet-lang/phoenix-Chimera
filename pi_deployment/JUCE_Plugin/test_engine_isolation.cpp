// Standalone engine test harness
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include <JuceHeader.h>

// Include all engines
#include "Source/EngineFactory.h"
#include "Source/EngineBase.h"
#include "Source/EngineIDs.h"

// Test signal generators
void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float sampleRate) {
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            float phase = (2.0f * M_PI * frequency * i) / sampleRate;
            channelData[i] = 0.5f * std::sin(phase);
        }
    }
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer) {
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    juce::Random random;
    
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            channelData[i] = 0.3f * (2.0f * random.nextFloat() - 1.0f);
        }
    }
}

void generateImpulse(juce::AudioBuffer<float>& buffer, int position = 100) {
    buffer.clear();
    auto numChannels = buffer.getNumChannels();
    
    if (position < buffer.getNumSamples()) {
        for (int ch = 0; ch < numChannels; ++ch) {
            buffer.setSample(ch, position, 0.8f);
        }
    }
}

// Analysis functions
float calculateRMS(const juce::AudioBuffer<float>& buffer) {
    float sum = 0.0f;
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        const auto* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            sum += channelData[i] * channelData[i];
        }
    }
    
    return std::sqrt(sum / (numChannels * numSamples));
}

float calculatePeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        const auto* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            peak = std::max(peak, std::abs(channelData[i]));
        }
    }
    
    return peak;
}

bool hasSignificantChange(const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed) {
    float originalRMS = calculateRMS(original);
    float processedRMS = calculateRMS(processed);
    float difference = std::abs(processedRMS - originalRMS);
    
    // Check if RMS changed by more than 1%
    return difference > (originalRMS * 0.01f);
}

// Test a single engine
void testEngine(int engineID, const juce::String& engineName) {
    std::cout << "\n========================================\n";
    std::cout << "Testing Engine " << engineID << ": " << engineName.toStdString() << "\n";
    std::cout << "========================================\n";
    
    // Create engine
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        std::cout << "❌ FAILED to create engine!\n";
        return;
    }
    
    // Setup
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const int numChannels = 2;
    
    engine->prepareToPlay(sampleRate, blockSize);
    
    // Test different input signals
    std::vector<std::pair<juce::String, std::function<void(juce::AudioBuffer<float>&)>>> testSignals = {
        {"440Hz Sine", [sampleRate](auto& buf) { generateSineWave(buf, 440.0f, sampleRate); }},
        {"White Noise", [](auto& buf) { generateWhiteNoise(buf); }},
        {"Impulse", [](auto& buf) { generateImpulse(buf); }}
    };
    
    // Get parameter count and set defaults
    int numParams = engine->getNumParameters();
    std::cout << "Number of parameters: " << numParams << "\n";
    
    // Test with each signal type
    for (const auto& [signalName, generateSignal] : testSignals) {
        std::cout << "\nTest Signal: " << signalName.toStdString() << "\n";
        
        // Create buffers
        juce::AudioBuffer<float> testBuffer(numChannels, blockSize);
        juce::AudioBuffer<float> originalBuffer(numChannels, blockSize);
        
        // Generate test signal
        generateSignal(testBuffer);
        originalBuffer.makeCopyOf(testBuffer);
        
        // Analyze input
        float inputRMS = calculateRMS(originalBuffer);
        float inputPeak = calculatePeak(originalBuffer);
        std::cout << "  Input - RMS: " << inputRMS << ", Peak: " << inputPeak << "\n";
        
        // Test 1: Default parameters
        {
            std::map<int, float> params;
            // Set reasonable defaults based on engine type
            if (engineName.contains("Distortion") || engineName.contains("Overdrive")) {
                params[0] = 0.7f; // Drive
                params[1] = 0.5f; // Tone
                params[2] = 0.5f; // Output
                params[3] = 1.0f; // Mix
            } else if (engineName.contains("Reverb")) {
                params[0] = 0.5f; // Room Size
                params[1] = 0.7f; // Damping
                params[2] = 0.5f; // Width
                params[3] = 0.3f; // Dry/Wet
                params[5] = 0.8f; // Mix (for some reverbs)
            } else if (engineName.contains("Delay")) {
                params[0] = 0.3f; // Time
                params[1] = 0.5f; // Feedback
                params[2] = 0.5f; // Filter
                params[3] = 0.8f; // Mix
            } else if (engineName.contains("Compressor")) {
                params[0] = 0.5f; // Threshold
                params[1] = 0.3f; // Ratio
                params[2] = 0.1f; // Attack
                params[3] = 0.3f; // Release
                params[6] = 1.0f; // Mix (for some compressors)
            } else {
                // Generic defaults
                for (int i = 0; i < numParams; ++i) {
                    params[i] = 0.5f;
                }
                // Set Mix to 100%
                if (numParams > 3) params[3] = 1.0f;
                if (numParams > 5) params[5] = 1.0f;
                if (numParams > 6) params[6] = 1.0f;
                if (numParams > 7) params[7] = 1.0f;
            }
            
            engine->updateParameters(params);
            engine->process(testBuffer);
            
            float outputRMS = calculateRMS(testBuffer);
            float outputPeak = calculatePeak(testBuffer);
            bool changed = hasSignificantChange(originalBuffer, testBuffer);
            
            std::cout << "  Output - RMS: " << outputRMS << ", Peak: " << outputPeak;
            std::cout << " | " << (changed ? "✅ MODIFIED" : "❌ NO CHANGE") << "\n";
        }
        
        // Test 2: Maximum effect
        {
            testBuffer.makeCopyOf(originalBuffer);
            std::map<int, float> params;
            
            // Max out key parameters
            if (engineName.contains("Distortion") || engineName.contains("Overdrive")) {
                params[0] = 1.0f; // Max drive
                params[3] = 1.0f; // Full wet
            } else if (engineName.contains("Filter")) {
                params[0] = 0.2f; // Low cutoff
                params[1] = 0.9f; // High resonance
                params[3] = 1.0f; // Full wet
            } else {
                params[0] = 1.0f; // Max first param
                for (int i = 3; i <= 7; ++i) {
                    if (i < numParams) params[i] = 1.0f; // Max all possible Mix params
                }
            }
            
            engine->updateParameters(params);
            engine->process(testBuffer);
            
            float outputRMS = calculateRMS(testBuffer);
            bool changed = hasSignificantChange(originalBuffer, testBuffer);
            
            std::cout << "  Max Effect - RMS: " << outputRMS;
            std::cout << " | " << (changed ? "✅ MODIFIED" : "❌ NO CHANGE") << "\n";
        }
    }
    
    // Test parameter response
    std::cout << "\nParameter Names:\n";
    for (int i = 0; i < numParams; ++i) {
        juce::String paramName = engine->getParameterName(i);
        std::cout << "  [" << i << "] " << paramName.toStdString() << "\n";
    }
    
    std::cout << "\n";
}

int main() {
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    std::cout << "Chimera Engine Isolation Test\n";
    std::cout << "============================\n\n";
    
    // Test specific engines or all
    std::vector<int> enginesToTest;
    
    // Option 1: Test all engines
    for (int i = 0; i <= 56; ++i) {
        enginesToTest.push_back(i);
    }
    
    // Option 2: Test specific problem engines
    // enginesToTest = {1, 2, 11, 21, 31}; // Rodent, Vintage, Tape, Plate, Spring
    
    // Run tests
    for (int engineID : enginesToTest) {
        juce::String engineName = getEngineDisplayName(engineID);
        if (engineName.isEmpty()) {
            engineName = "Unknown Engine " + juce::String(engineID);
        }
        
        testEngine(engineID, engineName);
    }
    
    std::cout << "\n\nTest Complete!\n";
    return 0;
}