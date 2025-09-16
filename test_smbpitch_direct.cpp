#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "JUCE_Plugin/Source/SMBPitchShiftFixed.h"

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║        DIRECT SMBPitchShiftFixed ALGORITHM TEST         ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";
    
    const float sampleRate = 44100.0f;
    const int blockSize = 2048;
    
    SMBPitchShiftFixed pitchShifter;
    pitchShifter.prepare(sampleRate, blockSize);
    
    // Test 1: Generate 440 Hz sine wave
    std::cout << "Test 1: Processing 440 Hz sine wave\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    std::vector<float> input(blockSize);
    std::vector<float> output(blockSize);
    
    // Generate input signal
    for (int i = 0; i < blockSize; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    
    // Test different pitch ratios
    float ratios[] = {0.5f, 1.0f, 2.0f};
    const char* descriptions[] = {"Octave down", "Unison", "Octave up"};
    
    for (int r = 0; r < 3; ++r) {
        std::cout << "\nPitch ratio: " << ratios[r] << " (" << descriptions[r] << ")\n";
        
        // Process
        pitchShifter.process(input.data(), output.data(), blockSize, ratios[r]);
        
        // Analyze output
        float maxOut = 0.0f;
        float avgOut = 0.0f;
        int nonZeroSamples = 0;
        
        for (int i = 0; i < blockSize; ++i) {
            float sample = std::abs(output[i]);
            maxOut = std::max(maxOut, sample);
            avgOut += sample;
            if (sample > 0.001f) nonZeroSamples++;
        }
        avgOut /= blockSize;
        
        std::cout << "  Max output: " << maxOut << std::endl;
        std::cout << "  Average output: " << avgOut << std::endl;
        std::cout << "  Non-zero samples: " << nonZeroSamples << "/" << blockSize << std::endl;
        
        // Simple frequency detection via zero crossings
        int zeroCrossings = 0;
        for (int i = 1; i < blockSize; ++i) {
            if (output[i-1] <= 0 && output[i] > 0) {
                zeroCrossings++;
            }
        }
        
        if (zeroCrossings > 0) {
            float estimatedFreq = (zeroCrossings * sampleRate) / (2.0f * blockSize);
            float expectedFreq = 440.0f * ratios[r];
            std::cout << "  Estimated frequency: " << estimatedFreq << " Hz" << std::endl;
            std::cout << "  Expected frequency: " << expectedFreq << " Hz" << std::endl;
            float error = std::abs(estimatedFreq - expectedFreq) / expectedFreq * 100.0f;
            std::cout << "  Error: " << error << "%" << std::endl;
            
            if (error < 10.0f) {
                std::cout << "  Status: ✓ PASS" << std::endl;
            } else {
                std::cout << "  Status: ✗ FAIL" << std::endl;
            }
        } else {
            std::cout << "  No zero crossings detected - no output signal!" << std::endl;
            std::cout << "  Status: ✗ FAIL" << std::endl;
        }
    }
    
    // Test 2: Check for latency/delay
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Test 2: Latency check with impulse\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    // Clear input and add impulse
    std::fill(input.begin(), input.end(), 0.0f);
    input[100] = 1.0f;  // Impulse at sample 100
    
    pitchShifter.process(input.data(), output.data(), blockSize, 1.0f);  // Unison (no pitch change)
    
    // Find first significant output
    int firstOutput = -1;
    for (int i = 0; i < blockSize; ++i) {
        if (std::abs(output[i]) > 0.01f) {
            firstOutput = i;
            break;
        }
    }
    
    if (firstOutput >= 0) {
        int latency = firstOutput - 100;
        std::cout << "  First output at sample: " << firstOutput << std::endl;
        std::cout << "  Latency: " << latency << " samples (" 
                  << (latency * 1000.0f / sampleRate) << " ms)" << std::endl;
    } else {
        std::cout << "  No output detected from impulse!" << std::endl;
    }
    
    // Test 3: Multiple consecutive blocks
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Test 3: Processing multiple consecutive blocks\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    float totalEnergy = 0.0f;
    for (int block = 0; block < 5; ++block) {
        // Generate continuous sine wave
        for (int i = 0; i < blockSize; ++i) {
            input[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * (block * blockSize + i) / sampleRate);
        }
        
        pitchShifter.process(input.data(), output.data(), blockSize, 2.0f);  // Octave up
        
        float blockEnergy = 0.0f;
        for (int i = 0; i < blockSize; ++i) {
            blockEnergy += output[i] * output[i];
        }
        totalEnergy += blockEnergy;
        
        std::cout << "  Block " << block << " energy: " << blockEnergy / blockSize << std::endl;
    }
    
    std::cout << "\n  Average energy: " << totalEnergy / (5.0f * blockSize) << std::endl;
    std::cout << "  Status: " << (totalEnergy > 0.01f ? "✓ Continuous processing works" : 
                                                        "✗ No output") << std::endl;
    
    std::cout << "\n══════════════════════════════════════════════════════════\n";
    std::cout << "DIRECT TEST COMPLETE\n";
    std::cout << "══════════════════════════════════════════════════════════\n\n";
    
    return 0;
}