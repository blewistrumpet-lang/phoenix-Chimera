#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/PhasedVocoder.h"
#include <iostream>
#include <cmath>

// Minimal test that only requires PhasedVocoder
int main() {
    std::cout << "=== PhasedVocoder Warmup Test ===\n\n";

    PhasedVocoder engine;

    const float sampleRate = 48000.0f;
    const int blockSize = 512;

    engine.prepareToPlay(sampleRate, blockSize);

    std::cout << "Engine Name: " << engine.getName() << "\n";
    std::cout << "Num Parameters: " << engine.getNumParameters() << "\n\n";

    // Set correct parameters for PhasedVocoder
    std::map<int, float> params;
    params[0] = 0.2f;   // TimeStretch = 1.0x
    params[1] = 0.5f;   // PitchShift = 0 semitones
    params[6] = 1.0f;   // Mix = 100% wet

    engine.updateParameters(params);

    std::cout << "Processing blocks to test warmup period...\n";

    // Process 10 blocks (5120 samples total)
    // Old warmup: 4096 samples (8 blocks)
    // New warmup: 2048 samples (4 blocks)
    for (int block = 0; block < 10; ++block) {
        juce::AudioBuffer<float> buffer(2, blockSize);

        // Generate 1kHz sine
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < blockSize; ++i) {
                int globalSample = block * blockSize + i;
                float phase = 2.0f * M_PI * 1000.0f * globalSample / sampleRate;
                buffer.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        engine.process(buffer);

        // Calculate RMS of output
        float rms = 0.0f;
        for (int i = 0; i < blockSize; ++i) {
            float sample = buffer.getSample(0, i);
            rms += sample * sample;
        }
        rms = std::sqrt(rms / blockSize);

        std::cout << "Block " << block << " (samples "
                  << (block * blockSize) << "-" << ((block + 1) * blockSize - 1)
                  << "): RMS = " << rms;

        if (rms > 0.001f) {
            std::cout << " [OUTPUT DETECTED]";
        } else {
            std::cout << " [SILENT - warmup]";
        }
        std::cout << "\n";
    }

    std::cout << "\nExpected behavior:\n";
    std::cout << "  OLD: Blocks 0-7 silent (4096 samples)\n";
    std::cout << "  NEW: Blocks 0-3 silent (2048 samples)\n";

    return 0;
}
