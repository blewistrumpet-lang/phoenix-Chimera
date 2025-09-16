#include "TransientShaper_Platinum.h"
#include <JuceHeader.h>
#include <iostream>
#include <cmath>

int main() {
    std::cout << "TransientShaper_Platinum Parameter Test\n";
    std::cout << "======================================\n";
    
    // Initialize the processor
    TransientShaper_Platinum processor;
    
    // Setup for 44.1kHz, 512 sample blocks
    const double sampleRate = 44100.0;
    const int blockSize = 512;
    processor.prepareToPlay(sampleRate, blockSize);
    
    // Create test buffer
    juce::AudioBuffer<float> buffer(2, blockSize);
    
    // Generate test signal: drum-like transient (short burst of noise)
    std::cout << "\nGenerating drum-like test signal...\n";
    
    for (int channel = 0; channel < 2; ++channel) {
        float* data = buffer.getWritePointer(channel);
        for (int i = 0; i < blockSize; ++i) {
            if (i < 50) {
                // Sharp attack (transient)
                float envelope = std::exp(-i * 0.1f);
                data[i] = envelope * ((rand() / float(RAND_MAX)) * 2.0f - 1.0f) * 0.5f;
            } else if (i < 200) {
                // Decay/sustain
                float envelope = 0.3f * std::exp(-(i-50) * 0.02f);
                data[i] = envelope * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
            } else {
                // Silence
                data[i] = 0.0f;
            }
        }
    }
    
    // Test different parameter values
    std::map<int, float> params;
    
    std::cout << "\nTesting Attack parameter:\n";
    
    // Test Attack at minimum (-15dB)
    params[TransientShaper_Platinum::Attack] = 0.0f;  // Should be -15dB
    params[TransientShaper_Platinum::Sustain] = 0.5f; // Unity gain
    processor.updateParameters(params);
    
    // Process and check RMS level
    juce::AudioBuffer<float> testBuffer1 = buffer;
    processor.process(testBuffer1);
    
    float rmsAttackMin = 0.0f;
    for (int i = 0; i < 50; ++i) {
        rmsAttackMin += testBuffer1.getSample(0, i) * testBuffer1.getSample(0, i);
    }
    rmsAttackMin = std::sqrt(rmsAttackMin / 50.0f);
    
    // Test Attack at maximum (+15dB)
    params[TransientShaper_Platinum::Attack] = 1.0f;  // Should be +15dB
    processor.updateParameters(params);
    
    juce::AudioBuffer<float> testBuffer2 = buffer;
    processor.process(testBuffer2);
    
    float rmsAttackMax = 0.0f;
    for (int i = 0; i < 50; ++i) {
        rmsAttackMax += testBuffer2.getSample(0, i) * testBuffer2.getSample(0, i);
    }
    rmsAttackMax = std::sqrt(rmsAttackMax / 50.0f);
    
    std::cout << "Attack Min (0.0): RMS = " << rmsAttackMin << std::endl;
    std::cout << "Attack Max (1.0): RMS = " << rmsAttackMax << std::endl;
    std::cout << "Ratio (should be ~5.62 for 15dB): " << (rmsAttackMax / (rmsAttackMin + 1e-10f)) << std::endl;
    
    std::cout << "\nTesting Sustain parameter:\n";
    
    // Test Sustain at minimum (-24dB)
    params[TransientShaper_Platinum::Attack] = 0.5f;  // Unity gain
    params[TransientShaper_Platinum::Sustain] = 0.0f; // Should be -24dB
    processor.updateParameters(params);
    
    juce::AudioBuffer<float> testBuffer3 = buffer;
    processor.process(testBuffer3);
    
    float rmsSustainMin = 0.0f;
    for (int i = 50; i < 200; ++i) {
        rmsSustainMin += testBuffer3.getSample(0, i) * testBuffer3.getSample(0, i);
    }
    rmsSustainMin = std::sqrt(rmsSustainMin / 150.0f);
    
    // Test Sustain at maximum (+24dB)
    params[TransientShaper_Platinum::Sustain] = 1.0f; // Should be +24dB
    processor.updateParameters(params);
    
    juce::AudioBuffer<float> testBuffer4 = buffer;
    processor.process(testBuffer4);
    
    float rmsSustainMax = 0.0f;
    for (int i = 50; i < 200; ++i) {
        rmsSustainMax += testBuffer4.getSample(0, i) * testBuffer4.getSample(0, i);
    }
    rmsSustainMax = std::sqrt(rmsSustainMax / 150.0f);
    
    std::cout << "Sustain Min (0.0): RMS = " << rmsSustainMin << std::endl;
    std::cout << "Sustain Max (1.0): RMS = " << rmsSustainMax << std::endl;
    std::cout << "Ratio (should be ~15.85 for 24dB): " << (rmsSustainMax / (rmsSustainMin + 1e-10f)) << std::endl;
    
    // Unity test (both at 0.5)
    std::cout << "\nTesting Unity (both parameters at 0.5):\n";
    params[TransientShaper_Platinum::Attack] = 0.5f;
    params[TransientShaper_Platinum::Sustain] = 0.5f;
    processor.updateParameters(params);
    
    juce::AudioBuffer<float> originalBuffer = buffer;
    juce::AudioBuffer<float> processedBuffer = buffer;
    processor.process(processedBuffer);
    
    // Compare original vs processed
    float originalRMS = 0.0f, processedRMS = 0.0f;
    for (int i = 0; i < blockSize; ++i) {
        originalRMS += originalBuffer.getSample(0, i) * originalBuffer.getSample(0, i);
        processedRMS += processedBuffer.getSample(0, i) * processedBuffer.getSample(0, i);
    }
    originalRMS = std::sqrt(originalRMS / blockSize);
    processedRMS = std::sqrt(processedRMS / blockSize);
    
    std::cout << "Original RMS: " << originalRMS << std::endl;
    std::cout << "Processed RMS (unity): " << processedRMS << std::endl;
    std::cout << "Unity ratio (should be ~1.0): " << (processedRMS / (originalRMS + 1e-10f)) << std::endl;
    
    std::cout << "\nTest completed!\n";
    std::cout << "If ratios are close to expected values, parameters are working correctly.\n";
    
    return 0;
}