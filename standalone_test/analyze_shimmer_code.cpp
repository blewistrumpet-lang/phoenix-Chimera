// Code analysis tool - traces through SimplePitchShifter logic
#include <iostream>
#include <vector>
#include <cmath>

const int pitchBufferSize = 4096;
const int grainSize = 1024;
const int numGrains = 2;

int main() {
    std::cout << "SimplePitchShifter Analysis\n";
    std::cout << "============================\n\n";

    // Simulate initialization
    std::vector<float> buffer(pitchBufferSize, 0.0f);
    std::vector<float> grainEnvelope(grainSize);

    // Create Hann window
    for (int i = 0; i < grainSize; i++) {
        float phase = static_cast<float>(i) / static_cast<float>(grainSize - 1);
        grainEnvelope[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
    }

    int writePos = 0;
    float grainPos[numGrains] = {0};
    float phaseOffset = 0.0f;

    // Reset logic (FIXED VERSION)
    for (int i = 0; i < numGrains; i++) {
        // Start grains at 1/4 window position where envelope has good amplitude
        float basePos = grainSize * 0.25f;  // Start at 25% through window
        grainPos[i] = basePos + (i * grainSize / numGrains) + (phaseOffset * grainSize);
        // Wrap around if needed
        while (grainPos[i] >= grainSize) grainPos[i] -= grainSize;
    }

    std::cout << "After reset:\n";
    std::cout << "  grainPos[0] = " << grainPos[0] << "\n";
    std::cout << "  grainPos[1] = " << grainPos[1] << "\n";
    std::cout << "  writePos = " << writePos << "\n\n";

    // Simulate first sample processing with pitch ratio 2.0
    float pitchRatio = 2.0f;
    float input = 1.0f;  // Impulse

    std::cout << "Processing first sample (impulse = 1.0, pitchRatio = 2.0):\n";

    // Write to circular buffer
    buffer[writePos] = input;
    std::cout << "  buffer[" << writePos << "] = " << input << "\n";

    float output = 0.0f;

    // Process grains
    for (int g = 0; g < numGrains; g++) {
        std::cout << "\n  Grain " << g << ":\n";

        // Read position with pitch shift
        float readPos = grainPos[g];
        std::cout << "    readPos (grainPos[" << g << "]) = " << readPos << "\n";

        // Get sample with linear interpolation
        int readIdx = static_cast<int>(readPos) % pitchBufferSize;
        int readIdx2 = (readIdx + 1) % pitchBufferSize;
        float frac = readPos - std::floor(readPos);

        std::cout << "    readIdx = " << readIdx << ", readIdx2 = " << readIdx2 << ", frac = " << frac << "\n";
        std::cout << "    buffer[" << readIdx << "] = " << buffer[readIdx] << "\n";
        std::cout << "    buffer[" << readIdx2 << "] = " << buffer[readIdx2] << "\n";

        float sample = buffer[readIdx] * (1.0f - frac) + buffer[readIdx2] * frac;
        std::cout << "    interpolated sample = " << sample << "\n";

        // Apply grain envelope
        int envPos = static_cast<int>(grainPos[g]) % grainSize;
        std::cout << "    envPos = " << envPos << ", envelope = " << grainEnvelope[envPos] << "\n";
        sample *= grainEnvelope[envPos];
        std::cout << "    sample after envelope = " << sample << "\n";

        output += sample;

        // Update grain position
        grainPos[g] += pitchRatio;
        if (grainPos[g] >= grainSize) {
            grainPos[g] -= grainSize;
        }
        std::cout << "    new grainPos[" << g << "] = " << grainPos[g] << "\n";
    }

    writePos = (writePos + 1) % pitchBufferSize;
    output = output / numGrains;

    std::cout << "\n  Final output = " << output << "\n";
    std::cout << "  New writePos = " << writePos << "\n";

    std::cout << "\n===================\n";
    std::cout << "ANALYSIS:\n";
    std::cout << "===================\n\n";

    std::cout << "Issue: Grain 0 starts at position 0, reading from buffer[0].\n";
    std::cout << "       The impulse was just written to buffer[0].\n";
    std::cout << "       BUT buffer[1] is still 0!\n\n";

    std::cout << "       Grain 1 starts at position 512, reading from buffer[512].\n";
    std::cout << "       buffer[512] and buffer[513] are both 0!\n\n";

    std::cout << "       This means on the FIRST sample, the pitch shifter reads from\n";
    std::cout << "       mostly empty buffer locations!\n\n";

    std::cout << "Expected behavior: Pitch shifter needs warmup/latency.\n";
    std::cout << "                   OR: Should read backwards from writePos.\n\n";

    std::cout << "Current behavior: Reads from arbitrary positions that may be empty.\n";
    std::cout << "                  With high pitch ratios (2.0), this is worse.\n\n";

    return 0;
}
