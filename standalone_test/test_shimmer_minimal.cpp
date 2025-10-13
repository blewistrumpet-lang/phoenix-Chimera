// Minimal debug test for ShimmerReverb
#include <iostream>
#include <memory>
#include "ShimmerReverb.h"
#include <JuceHeader.h>

int main() {
    std::cout << "Minimal ShimmerReverb Debug Test\n";
    std::cout << "=================================\n\n";

    auto engine = std::make_unique<ShimmerReverb>();
    std::cout << "Engine: " << engine->getName().toStdString() << "\n\n";

    // Prepare
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters - pure reverb, no shimmer
    std::map<int, float> params;
    params[0] = 1.0f;   // Mix = 100% wet
    params[2] = 0.0f;   // Shimmer = 0% (DISABLED)
    params[3] = 0.7f;   // Size = 70%
    params[6] = 0.0f;   // Pre-delay = 0%
    engine->updateParameters(params);
    engine->reset();

    // Create test buffer
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();

    // Set impulse
    buffer.setSample(0, 0, 1.0f);  // Left impulse
    buffer.setSample(1, 0, 1.0f);  // Right impulse

    std::cout << "Input:\n";
    std::cout << "  buffer[0][0] = " << buffer.getSample(0, 0) << "\n";
    std::cout << "  buffer[1][0] = " << buffer.getSample(1, 0) << "\n\n";

    // Process
    engine->process(buffer);

    std::cout << "Output (first 10 samples):\n";
    std::cout << "Sample | Left           | Right\n";
    std::cout << "-------|----------------|--------------\n";
    for (int i = 0; i < 10; ++i) {
        printf("  %d    | %14.10f | %14.10f\n",
               i, buffer.getSample(0, i), buffer.getSample(1, i));
    }

    // Find first non-zero sample in first block
    int firstNonZeroL = -1, firstNonZeroR = -1;
    for (int i = 0; i < blockSize; ++i) {
        if (firstNonZeroL < 0 && std::abs(buffer.getSample(0, i)) > 1e-10f) {
            firstNonZeroL = i;
        }
        if (firstNonZeroR < 0 && std::abs(buffer.getSample(1, i)) > 1e-10f) {
            firstNonZeroR = i;
        }
        if (firstNonZeroL >= 0 && firstNonZeroR >= 0) break;
    }

    std::cout << "\nFirst non-zero sample in block 0:\n";
    std::cout << "  Left:  " << firstNonZeroL << "\n";
    std::cout << "  Right: " << firstNonZeroR << "\n";

    // Process more blocks to let reverb build up
    std::cout << "\nProcessing 10 more blocks...\n";
    bool foundOutput = false;
    for (int block = 1; block <= 10; ++block) {
        buffer.clear();
        engine->process(buffer);

        // Check for non-zero output
        for (int i = 0; i < blockSize; ++i) {
            if (std::abs(buffer.getSample(0, i)) > 1e-10f || std::abs(buffer.getSample(1, i)) > 1e-10f) {
                std::cout << "Block " << block << ": Found output at sample " << i << "\n";
                std::cout << "  L[" << i << "] = " << buffer.getSample(0, i) << "\n";
                std::cout << "  R[" << i << "] = " << buffer.getSample(1, i) << "\n";
                foundOutput = true;
                break;
            }
        }
        if (foundOutput) break;
    }

    if (!foundOutput) {
        std::cout << "\n✗ FAIL: ALL ZEROS after 11 blocks - No output produced!\n";
        return 1;
    } else {
        std::cout << "\n✓ Output detected\n";
        return 0;
    }
}
