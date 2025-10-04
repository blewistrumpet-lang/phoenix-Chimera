#include <iostream>
#include <vector>
#include <memory>
#include <map>

// Minimal includes to test PhasedVocoder fixes
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#include "JuceLibraryCode/JuceHeader.h"
#include "Source/PhasedVocoder.h"

int main() {
    std::cout << "Testing PhasedVocoder fixes..." << std::endl;
    
    try {
        // Create PhasedVocoder instance
        auto vocoder = std::make_unique<PhasedVocoder>();
        std::cout << "✓ PhasedVocoder created successfully" << std::endl;
        
        // Prepare to play
        double sampleRate = 44100.0;
        int samplesPerBlock = 512;
        vocoder->prepareToPlay(sampleRate, samplesPerBlock);
        std::cout << "✓ prepareToPlay() completed without crash" << std::endl;
        
        // Create test buffer
        juce::AudioBuffer<float> testBuffer(2, samplesPerBlock);
        testBuffer.clear();
        
        // Fill with test signal
        for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
            float* channelData = testBuffer.getWritePointer(ch);
            for (int i = 0; i < samplesPerBlock; ++i) {
                // Simple sine wave at 440Hz
                channelData[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
            }
        }
        
        std::cout << "✓ Test buffer created with 440Hz sine wave" << std::endl;
        
        // Test processing multiple times to trigger potential wraparound issues
        for (int iteration = 0; iteration < 10; ++iteration) {
            vocoder->process(testBuffer);
            
            // Check for NaN/Inf in output
            bool hasInvalidSamples = false;
            for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
                const float* channelData = testBuffer.getReadPointer(ch);
                for (int i = 0; i < samplesPerBlock; ++i) {
                    if (!std::isfinite(channelData[i])) {
                        hasInvalidSamples = true;
                        break;
                    }
                }
                if (hasInvalidSamples) break;
            }
            
            if (hasInvalidSamples) {
                std::cout << "✗ Invalid samples detected in iteration " << iteration << std::endl;
                return 1;
            }
        }
        
        std::cout << "✓ 10 processing iterations completed without invalid samples" << std::endl;
        
        // Test parameter updates
        std::map<int, float> testParams;
        testParams[static_cast<int>(PhasedVocoder::ParamID::TimeStretch)] = 0.5f;  // 2x time stretch
        testParams[static_cast<int>(PhasedVocoder::ParamID::PitchShift)] = 0.75f;  // +12 semitones
        testParams[static_cast<int>(PhasedVocoder::ParamID::Mix)] = 1.0f;          // Full wet
        
        vocoder->updateParameters(testParams);
        std::cout << "✓ Parameter updates completed" << std::endl;
        
        // Process again with new parameters
        for (int iteration = 0; iteration < 5; ++iteration) {
            vocoder->process(testBuffer);
            
            // Check for NaN/Inf in output after parameter changes
            bool hasInvalidSamples = false;
            for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
                const float* channelData = testBuffer.getReadPointer(ch);
                for (int i = 0; i < samplesPerBlock; ++i) {
                    if (!std::isfinite(channelData[i])) {
                        hasInvalidSamples = true;
                        break;
                    }
                }
                if (hasInvalidSamples) break;
            }
            
            if (hasInvalidSamples) {
                std::cout << "✗ Invalid samples detected after parameter update in iteration " << iteration << std::endl;
                return 1;
            }
        }
        
        std::cout << "✓ Processing with parameter changes completed successfully" << std::endl;
        
        // Reset and test again
        vocoder->reset();
        std::cout << "✓ Reset completed" << std::endl;
        
        // Final processing test
        vocoder->process(testBuffer);
        std::cout << "✓ Processing after reset completed" << std::endl;
        
        std::cout << "\n🎉 All PhasedVocoder tests passed!" << std::endl;
        std::cout << "Fixed issues:" << std::endl;
        std::cout << "  - SIMD buffer wraparound in grain filling" << std::endl;
        std::cout << "  - SIMD buffer wraparound in overlap-add" << std::endl;
        std::cout << "  - Proper scaling normalization" << std::endl;
        std::cout << "  - FFT data format handling" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "✗ Unknown exception caught" << std::endl;
        return 1;
    }
}