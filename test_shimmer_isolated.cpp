#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "JUCE_Plugin/Source/ShimmerReverb.h"

void analyzeFrequencyContent(const juce::AudioBuffer<float>& buffer, float sampleRate, const std::string& label) {
    // Simple peak detection through zero-crossing analysis
    std::vector<float> zeroCrossings;
    float lastSample = 0.0f;
    int lastZeroCrossing = 0;
    
    for (int i = 1; i < buffer.getNumSamples(); ++i) {
        float sample = buffer.getSample(0, i);
        if (lastSample <= 0 && sample > 0) {
            if (lastZeroCrossing > 0) {
                int period = i - lastZeroCrossing;
                if (period > 10 && period < 1000) { // Reasonable frequency range
                    float freq = sampleRate / (2.0f * period);
                    zeroCrossings.push_back(freq);
                }
            }
            lastZeroCrossing = i;
        }
        lastSample = sample;
    }
    
    // Calculate average frequency
    if (!zeroCrossings.empty()) {
        float avgFreq = 0.0f;
        for (float f : zeroCrossings) avgFreq += f;
        avgFreq /= zeroCrossings.size();
        std::cout << label << " - Estimated frequency: " << std::fixed << std::setprecision(1) 
                 << avgFreq << " Hz" << std::endl;
    } else {
        std::cout << label << " - No clear frequency detected" << std::endl;
    }
}

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║      SHIMMER REVERB ISOLATED PITCH SHIFT TEST           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    ShimmerReverb shimmer;
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    
    shimmer.prepareToPlay(sampleRate, blockSize);
    shimmer.reset();
    
    std::cout << "Test Configuration:\n";
    std::cout << "  • Input: 440 Hz sine wave\n";
    std::cout << "  • Testing different shimmer settings\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Test 1: Dry signal (no shimmer, no reverb)
    std::cout << "TEST 1: Dry Signal (Mix=0, Shimmer=0)\n";
    std::map<int, float> params;
    for (int i = 0; i < 10; ++i) params[i] = 0.0f;
    shimmer.updateParameters(params);
    shimmer.reset();
    
    juce::AudioBuffer<float> buffer(2, blockSize * 4);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float sample = 0.7f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    juce::AudioBuffer<float> testBuffer(buffer);
    for (int block = 0; block < 4; ++block) {
        juce::AudioBuffer<float> blockBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            blockBuf.setSample(0, i, testBuffer.getSample(0, block * blockSize + i));
            blockBuf.setSample(1, i, testBuffer.getSample(1, block * blockSize + i));
        }
        shimmer.process(blockBuf);
        for (int i = 0; i < blockSize; ++i) {
            testBuffer.setSample(0, block * blockSize + i, blockBuf.getSample(0, i));
            testBuffer.setSample(1, block * blockSize + i, blockBuf.getSample(1, i));
        }
    }
    
    float maxDry = 0.0f;
    for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
        maxDry = std::max(maxDry, std::abs(testBuffer.getSample(0, i)));
    }
    std::cout << "  Max amplitude: " << maxDry << std::endl;
    analyzeFrequencyContent(testBuffer, sampleRate, "  Output");
    std::cout << "  Expected: 440 Hz (unchanged)\n\n";
    
    // Test 2: Shimmer only (octave up)
    std::cout << "TEST 2: Shimmer Active (Shimmer=1.0, Pitch=1.0 [octave up], Mix=0.5)\n";
    params[1] = 1.0f;  // Shimmer amount
    params[2] = 1.0f;  // Pitch (octave up)
    params[9] = 0.5f;  // Mix
    shimmer.updateParameters(params);
    shimmer.reset();
    
    testBuffer = buffer;
    for (int block = 0; block < 4; ++block) {
        juce::AudioBuffer<float> blockBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            blockBuf.setSample(0, i, testBuffer.getSample(0, block * blockSize + i));
            blockBuf.setSample(1, i, testBuffer.getSample(1, block * blockSize + i));
        }
        shimmer.process(blockBuf);
        for (int i = 0; i < blockSize; ++i) {
            testBuffer.setSample(0, block * blockSize + i, blockBuf.getSample(0, i));
            testBuffer.setSample(1, block * blockSize + i, blockBuf.getSample(1, i));
        }
    }
    
    float maxShimmer = 0.0f;
    for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
        maxShimmer = std::max(maxShimmer, std::abs(testBuffer.getSample(0, i)));
    }
    std::cout << "  Max amplitude: " << maxShimmer << std::endl;
    analyzeFrequencyContent(testBuffer, sampleRate, "  Output");
    std::cout << "  Expected: Should contain 880 Hz (octave up)\n\n";
    
    // Test 3: Shimmer with different pitch settings
    std::cout << "TEST 3: Various Pitch Settings (Shimmer=1.0, Mix=0.3)\n";
    params[9] = 0.3f;  // Lower mix
    
    float pitchSettings[] = {0.0f, 0.5f, 0.75f};
    const char* descriptions[] = {"Octave down (220 Hz)", "Unison (440 Hz)", "Up 6 semitones (622 Hz)"};
    
    for (int p = 0; p < 3; ++p) {
        params[2] = pitchSettings[p];
        shimmer.updateParameters(params);
        shimmer.reset();
        
        testBuffer = buffer;
        for (int block = 0; block < 4; ++block) {
            juce::AudioBuffer<float> blockBuf(2, blockSize);
            for (int i = 0; i < blockSize; ++i) {
                blockBuf.setSample(0, i, testBuffer.getSample(0, block * blockSize + i));
                blockBuf.setSample(1, i, testBuffer.getSample(1, block * blockSize + i));
            }
            shimmer.process(blockBuf);
            for (int i = 0; i < blockSize; ++i) {
                testBuffer.setSample(0, block * blockSize + i, blockBuf.getSample(0, i));
                testBuffer.setSample(1, block * blockSize + i, blockBuf.getSample(1, i));
            }
        }
        
        std::cout << "  Pitch=" << pitchSettings[p] << " - " << descriptions[p] << std::endl;
        float maxOut = 0.0f;
        float minOut = 1.0f;
        for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
            float sample = std::abs(testBuffer.getSample(0, i));
            maxOut = std::max(maxOut, sample);
            if (sample > 0.001f) minOut = std::min(minOut, sample);
        }
        std::cout << "    Amplitude range: " << minOut << " to " << maxOut << std::endl;
    }
    
    // Test 4: Shimmer with reverb
    std::cout << "\nTEST 4: Shimmer + Reverb (Full settings)\n";
    params[0] = 0.7f;  // Size
    params[1] = 0.8f;  // Shimmer
    params[2] = 1.0f;  // Pitch (octave up)
    params[3] = 0.3f;  // Damping
    params[4] = 0.5f;  // Diffusion
    params[5] = 0.3f;  // Modulation
    params[9] = 0.4f;  // Mix
    shimmer.updateParameters(params);
    shimmer.reset();
    
    // Use a short burst
    juce::AudioBuffer<float> burst(2, blockSize * 8);
    for (int i = 0; i < blockSize; ++i) {
        float sample = 0.7f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
        sample *= std::exp(-i * 0.01f); // Decay envelope
        burst.setSample(0, i, sample);
        burst.setSample(1, i, sample);
    }
    
    for (int block = 0; block < 8; ++block) {
        juce::AudioBuffer<float> blockBuf(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            blockBuf.setSample(0, i, burst.getSample(0, block * blockSize + i));
            blockBuf.setSample(1, i, burst.getSample(1, block * blockSize + i));
        }
        shimmer.process(blockBuf);
        for (int i = 0; i < blockSize; ++i) {
            burst.setSample(0, block * blockSize + i, blockBuf.getSample(0, i));
            burst.setSample(1, block * blockSize + i, blockBuf.getSample(1, i));
        }
    }
    
    // Analyze tail
    float tailEnergy = 0.0f;
    for (int i = blockSize * 4; i < burst.getNumSamples(); ++i) {
        tailEnergy += std::abs(burst.getSample(0, i));
    }
    tailEnergy /= (burst.getNumSamples() - blockSize * 4);
    
    std::cout << "  Reverb tail energy: " << std::scientific << tailEnergy << std::endl;
    std::cout << "  Status: " << (tailEnergy > 1e-4f ? "✓ Shimmer reverb active" : "✗ No reverb tail") << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "SUMMARY:\n";
    std::cout << "  The shimmer effect should:\n";
    std::cout << "  • Pass dry signal unchanged when shimmer=0\n";
    std::cout << "  • Add pitch-shifted component when shimmer>0\n";
    std::cout << "  • Blend with reverb for ethereal effect\n";
    std::cout << "  • Maintain stable amplitude\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    return 0;
}