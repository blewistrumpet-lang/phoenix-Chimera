#include "MultibandSaturator.h"
#include "WaveFolder.h"
#include "PitchShifter.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <iostream>
#include <cassert>

// Test channel compatibility for the fixed engines
void testChannelCompatibility() {
    std::cout << "Testing channel compatibility for fixed engines...\n";
    
    const int sampleRate = 44100;
    const int blockSize = 512;
    
    // Test cases: mono, stereo, 5.1 surround, 7.1 surround
    std::vector<int> testChannelCounts = {1, 2, 6, 8};
    
    for (int numChannels : testChannelCounts) {
        std::cout << "Testing with " << numChannels << " channels:\n";
        
        // Create test buffer
        juce::AudioBuffer<float> buffer(numChannels, blockSize);
        buffer.clear();
        
        // Fill with test signal (sine wave)
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                data[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
            }
        }
        
        // Test MultibandSaturator
        {
            MultibandSaturator saturator;
            saturator.prepareToPlay(sampleRate, blockSize);
            
            auto testBuffer = buffer;
            saturator.process(testBuffer);
            
            // Verify output is finite and not all zeros
            bool hasValidOutput = false;
            for (int ch = 0; ch < numChannels; ++ch) {
                auto* data = testBuffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    assert(std::isfinite(data[i]));
                    if (std::abs(data[i]) > 1e-10f) {
                        hasValidOutput = true;
                    }
                }
            }
            assert(hasValidOutput);
            std::cout << "  - MultibandSaturator: PASS\n";
        }
        
        // Test WaveFolder
        {
            WaveFolder folder;
            folder.prepareToPlay(sampleRate, blockSize);
            
            auto testBuffer = buffer;
            folder.process(testBuffer);
            
            // Verify output is finite and not all zeros
            bool hasValidOutput = false;
            for (int ch = 0; ch < numChannels; ++ch) {
                auto* data = testBuffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    assert(std::isfinite(data[i]));
                    if (std::abs(data[i]) > 1e-10f) {
                        hasValidOutput = true;
                    }
                }
            }
            assert(hasValidOutput);
            std::cout << "  - WaveFolder: PASS\n";
        }
        
        // Test PitchShifter
        {
            PitchShifter shifter;
            shifter.prepareToPlay(sampleRate, blockSize);
            
            auto testBuffer = buffer;
            shifter.process(testBuffer);
            
            // Verify output is finite and not all zeros
            bool hasValidOutput = false;
            for (int ch = 0; ch < numChannels; ++ch) {
                auto* data = testBuffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    assert(std::isfinite(data[i]));
                    if (std::abs(data[i]) > 1e-10f) {
                        hasValidOutput = true;
                    }
                }
            }
            assert(hasValidOutput);
            std::cout << "  - PitchShifter: PASS\n";
        }
        
        std::cout << "  All engines passed for " << numChannels << " channels\n\n";
    }
    
    std::cout << "Channel compatibility test completed successfully!\n";
    std::cout << "All engines now support up to 8 channels while maintaining backwards compatibility.\n";
}

int main() {
    testChannelCompatibility();
    return 0;
}