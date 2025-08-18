#pragma once
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>

/**
 * Simple Engine Diagnostic
 * 
 * Add this directly to PluginProcessor.cpp to debug engine processing.
 * This is a minimal, self-contained diagnostic that can be easily integrated.
 */

// Add this function directly to PluginProcessor.cpp
inline void runSimpleEngineDiagnostic(double sampleRate = 44100.0, int blockSize = 512) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "SIMPLE ENGINE DIAGNOSTIC\n";
    std::cout << "Sample Rate: " << sampleRate << " Hz, Block Size: " << blockSize << "\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    // Create test buffer with 1kHz tone at -6dB
    juce::AudioBuffer<float> testBuffer(2, blockSize);
    const double twoPi = 2.0 * juce::MathConstants<double>::pi;
    const double phaseIncrement = twoPi * 1000.0 / sampleRate;
    const float amplitude = 0.5f; // -6dB
    
    // Generate test tone
    for (int ch = 0; ch < 2; ++ch) {
        float* channelData = testBuffer.getWritePointer(ch);
        double phase = 0.0;
        for (int s = 0; s < blockSize; ++s) {
            channelData[s] = amplitude * std::sin(phase);
            phase += phaseIncrement;
            if (phase >= twoPi) phase -= twoPi;
        }
    }
    
    // Helper lambda to calculate RMS
    auto calculateRMS = [](const juce::AudioBuffer<float>& buffer) -> float {
        float sumSquares = 0.0f;
        int totalSamples = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s) {
                sumSquares += data[s] * data[s];
                totalSamples++;
            }
        }
        return totalSamples > 0 ? std::sqrt(sumSquares / totalSamples) : 0.0f;
    };
    
    // Helper lambda to check if buffers are different
    auto buffersAreDifferent = [](const juce::AudioBuffer<float>& buf1,
                                 const juce::AudioBuffer<float>& buf2) -> bool {
        if (buf1.getNumChannels() != buf2.getNumChannels() ||
            buf1.getNumSamples() != buf2.getNumSamples()) return true;
            
        for (int ch = 0; ch < buf1.getNumChannels(); ++ch) {
            const float* data1 = buf1.getReadPointer(ch);
            const float* data2 = buf2.getReadPointer(ch);
            for (int s = 0; s < buf1.getNumSamples(); ++s) {
                if (std::abs(data1[s] - data2[s]) > 0.0001f) return true;
            }
        }
        return false;
    };
    
    // Test PlateReverb
    {
        std::cout << "--- Testing PlateReverb ---\n";
        
        juce::AudioBuffer<float> inputBuffer, outputBuffer;
        inputBuffer.makeCopyOf(testBuffer);
        outputBuffer.makeCopyOf(testBuffer);
        
        float inputRMS = calculateRMS(inputBuffer);
        
        try {
            auto engine = std::make_unique<PlateReverb>();
            engine->prepareToPlay(sampleRate, blockSize);
            
            // Set test parameters
            std::map<int, float> params = {
                {0, 0.7f},  // Size
                {1, 0.4f},  // Damping  
                {2, 0.1f},  // Predelay
                {3, 0.5f}   // Mix
            };
            engine->updateParameters(params);
            
            engine->process(outputBuffer);
            
            float outputRMS = calculateRMS(outputBuffer);
            bool modified = buffersAreDifferent(inputBuffer, outputBuffer);
            float gainChange_dB = (inputRMS > 0.0001f) ? 20.0f * std::log10(outputRMS / inputRMS) : 0.0f;
            
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "  Status: WORKING\n";
            std::cout << "  Audio Modified: " << (modified ? "YES" : "NO") << "\n";
            std::cout << "  Input RMS: " << inputRMS << "\n";
            std::cout << "  Output RMS: " << outputRMS << "\n";
            std::cout << "  Gain Change: " << gainChange_dB << " dB\n";
            std::cout << "  Mix Parameter: " << params[3] << "\n";
            std::cout << "  Expected: Should add reverb, mix at 50%\n\n";
            
        } catch (...) {
            std::cout << "  Status: ERROR - Failed to process\n\n";
        }
    }
    
    // Test ClassicCompressor
    {
        std::cout << "--- Testing ClassicCompressor ---\n";
        
        juce::AudioBuffer<float> inputBuffer, outputBuffer;
        inputBuffer.makeCopyOf(testBuffer);
        outputBuffer.makeCopyOf(testBuffer);
        
        float inputRMS = calculateRMS(inputBuffer);
        
        try {
            auto engine = std::make_unique<ClassicCompressor>();
            engine->prepareToPlay(sampleRate, blockSize);
            
            // Set test parameters for compression
            std::map<int, float> params = {
                {0, 0.3f},  // Threshold (-18dB)
                {1, 0.6f},  // Ratio (8:1)
                {2, 0.1f},  // Attack (fast)
                {3, 0.3f},  // Release (moderate)
                {4, 0.2f},  // Knee (soft)
                {5, 0.5f},  // Makeup gain
                {6, 1.0f},  // Mix (100% wet)
                {7, 0.0f},  // Lookahead (off)
                {8, 0.5f},  // Auto Release
                {9, 0.0f}   // Sidechain (off)
            };
            engine->updateParameters(params);
            
            engine->process(outputBuffer);
            
            float outputRMS = calculateRMS(outputBuffer);
            bool modified = buffersAreDifferent(inputBuffer, outputBuffer);
            float gainChange_dB = (inputRMS > 0.0001f) ? 20.0f * std::log10(outputRMS / inputRMS) : 0.0f;
            
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "  Status: WORKING\n";
            std::cout << "  Audio Modified: " << (modified ? "YES" : "NO") << "\n";
            std::cout << "  Input RMS: " << inputRMS << "\n";
            std::cout << "  Output RMS: " << outputRMS << "\n";
            std::cout << "  Gain Change: " << gainChange_dB << " dB\n";
            std::cout << "  Mix Parameter: " << params[6] << "\n";
            std::cout << "  Expected: Should compress dynamics, reduce level\n\n";
            
        } catch (...) {
            std::cout << "  Status: ERROR - Failed to process\n\n";
        }
    }
    
    // Test RodentDistortion
    {
        std::cout << "--- Testing RodentDistortion ---\n";
        
        juce::AudioBuffer<float> inputBuffer, outputBuffer;
        inputBuffer.makeCopyOf(testBuffer);
        outputBuffer.makeCopyOf(testBuffer);
        
        float inputRMS = calculateRMS(inputBuffer);
        
        try {
            auto engine = std::make_unique<RodentDistortion>();
            engine->prepareToPlay(sampleRate, blockSize);
            
            // Set test parameters for distortion
            std::map<int, float> params = {
                {0, 0.7f},  // Gain (high)
                {1, 0.5f},  // Filter (moderate)
                {2, 0.6f},  // Clipping (significant)
                {3, 0.5f},  // Tone (neutral)
                {4, 0.8f},  // Output (boosted)
                {5, 1.0f},  // Mix (100% wet)
                {6, 0.0f},  // Mode (RAT)
                {7, 0.4f}   // Presence (moderate)
            };
            engine->updateParameters(params);
            
            engine->process(outputBuffer);
            
            float outputRMS = calculateRMS(outputBuffer);
            bool modified = buffersAreDifferent(inputBuffer, outputBuffer);
            float gainChange_dB = (inputRMS > 0.0001f) ? 20.0f * std::log10(outputRMS / inputRMS) : 0.0f;
            
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "  Status: WORKING\n";
            std::cout << "  Audio Modified: " << (modified ? "YES" : "NO") << "\n";
            std::cout << "  Input RMS: " << inputRMS << "\n";
            std::cout << "  Output RMS: " << outputRMS << "\n";
            std::cout << "  Gain Change: " << gainChange_dB << " dB\n";
            std::cout << "  Mix Parameter: " << params[5] << "\n";
            std::cout << "  Expected: Should add distortion, likely boost level\n\n";
            
        } catch (...) {
            std::cout << "  Status: ERROR - Failed to process\n\n";
        }
    }
    
    std::cout << std::string(60, '=') << "\n";
    std::cout << "DIAGNOSTIC COMPLETE\n";
    std::cout << "If engines show 'Audio Modified: NO', check:\n";
    std::cout << "1. Mix parameter is not 0 (dry only)\n";
    std::cout << "2. Parameters are in expected ranges\n"; 
    std::cout << "3. Engine is actually processing the audio\n";
    std::cout << std::string(60, '=') << "\n\n";
}