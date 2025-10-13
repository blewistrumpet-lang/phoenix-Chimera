// Diagnostic test for ConvolutionReverb Engine 41
#include "../JUCE_Plugin/Source/ConvolutionReverb.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

int main() {
    std::cout << "=== CONVOLUTION REVERB DIAGNOSTIC TEST (Engine 41) ===" << std::endl;
    std::cout << "This test includes comprehensive diagnostics to identify zero output cause." << std::endl << std::endl;

    // Create engine
    ConvolutionReverb engine;

    // Initialize
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int numChannels = 2;
    const int testDuration = 1; // 1 second

    std::cout << "Step 1: Initializing engine..." << std::endl;
    engine.prepareToPlay(sampleRate, blockSize);
    std::cout << "Engine initialized. Latency: " << engine.getLatencySamples() << " samples" << std::endl << std::endl;

    // Set up parameters for maximum effect
    std::map<int, float> params;
    params[0] = 1.0f;  // Mix = 100% wet
    params[1] = 0.0f;  // IR Select = Concert Hall (0)
    params[2] = 0.5f;  // Size = 50%
    params[3] = 0.0f;  // Pre-Delay = 0ms
    params[4] = 0.0f;  // Damping = 0
    params[5] = 0.0f;  // Reverse = off
    params[6] = 0.5f;  // Early/Late = balanced
    params[7] = 0.0f;  // Low Cut = off
    params[8] = 1.0f;  // High Cut = off
    params[9] = 1.0f;  // Width = 100%

    std::cout << "Step 2: Setting parameters (100% wet, Concert Hall IR)..." << std::endl;
    engine.updateParameters(params);
    std::cout << "Parameters set." << std::endl << std::endl;

    // Generate test signal (impulse)
    const int totalSamples = static_cast<int>(sampleRate * testDuration);
    const int numBlocks = (totalSamples + blockSize - 1) / blockSize;

    std::vector<float> inputL(totalSamples, 0.0f);
    std::vector<float> inputR(totalSamples, 0.0f);
    std::vector<float> outputL(totalSamples, 0.0f);
    std::vector<float> outputR(totalSamples, 0.0f);

    // Create impulse at start
    inputL[0] = 1.0f;
    inputR[0] = 1.0f;

    std::cout << "Step 3: Processing " << numBlocks << " blocks of audio..." << std::endl;

    int samplePos = 0;
    for (int block = 0; block < numBlocks; block++) {
        int samplesThisBlock = std::min(blockSize, totalSamples - samplePos);

        // Create buffer for this block
        juce::AudioBuffer<float> buffer(numChannels, samplesThisBlock);

        // Copy input to buffer
        for (int i = 0; i < samplesThisBlock; i++) {
            buffer.setSample(0, i, inputL[samplePos + i]);
            buffer.setSample(1, i, inputR[samplePos + i]);
        }

        // Process
        engine.process(buffer);

        // Copy output
        for (int i = 0; i < samplesThisBlock; i++) {
            outputL[samplePos + i] = buffer.getSample(0, i);
            outputR[samplePos + i] = buffer.getSample(1, i);
        }

        samplePos += samplesThisBlock;

        // Show progress
        if (block % 10 == 0) {
            std::cout << "  Processed block " << block << "/" << numBlocks << std::endl;
        }
    }

    std::cout << "Processing complete." << std::endl << std::endl;

    // Analyze output
    std::cout << "Step 4: Analyzing output..." << std::endl;

    float peakL = 0.0f;
    float peakR = 0.0f;
    float rmsL = 0.0f;
    float rmsR = 0.0f;
    int firstNonZeroL = -1;
    int firstNonZeroR = -1;
    int nonZeroCountL = 0;
    int nonZeroCountR = 0;

    for (int i = 0; i < totalSamples; i++) {
        float absL = std::abs(outputL[i]);
        float absR = std::abs(outputR[i]);

        if (absL > peakL) peakL = absL;
        if (absR > peakR) peakR = absR;

        rmsL += outputL[i] * outputL[i];
        rmsR += outputR[i] * outputR[i];

        if (absL > 0.0001f) {
            if (firstNonZeroL < 0) firstNonZeroL = i;
            nonZeroCountL++;
        }
        if (absR > 0.0001f) {
            if (firstNonZeroR < 0) firstNonZeroR = i;
            nonZeroCountR++;
        }
    }

    rmsL = std::sqrt(rmsL / totalSamples);
    rmsR = std::sqrt(rmsR / totalSamples);

    std::cout << "\n=== OUTPUT ANALYSIS ===" << std::endl;
    std::cout << "Left Channel:" << std::endl;
    std::cout << "  Peak: " << peakL << " (" << (20.0f * std::log10(peakL + 1e-10f)) << " dB)" << std::endl;
    std::cout << "  RMS: " << rmsL << " (" << (20.0f * std::log10(rmsL + 1e-10f)) << " dB)" << std::endl;
    std::cout << "  First non-zero sample: " << firstNonZeroL
              << " (" << (firstNonZeroL / sampleRate * 1000.0f) << " ms)" << std::endl;
    std::cout << "  Non-zero sample count: " << nonZeroCountL << " / " << totalSamples << std::endl;

    std::cout << "\nRight Channel:" << std::endl;
    std::cout << "  Peak: " << peakR << " (" << (20.0f * std::log10(peakR + 1e-10f)) << " dB)" << std::endl;
    std::cout << "  RMS: " << rmsR << " (" << (20.0f * std::log10(rmsR + 1e-10f)) << " dB)" << std::endl;
    std::cout << "  First non-zero sample: " << firstNonZeroR
              << " (" << (firstNonZeroR / sampleRate * 1000.0f) << " ms)" << std::endl;
    std::cout << "  Non-zero sample count: " << nonZeroCountR << " / " << totalSamples << std::endl;

    // Save first 1000 samples to CSV for inspection
    std::ofstream csv("convolution_diagnostic_output.csv");
    csv << "Sample,InputL,InputR,OutputL,OutputR\n";
    for (int i = 0; i < std::min(1000, totalSamples); i++) {
        csv << i << "," << inputL[i] << "," << inputR[i] << ","
            << outputL[i] << "," << outputR[i] << "\n";
    }
    csv.close();
    std::cout << "\nFirst 1000 samples saved to: convolution_diagnostic_output.csv" << std::endl;

    // Determine pass/fail
    std::cout << "\n=== TEST RESULT ===" << std::endl;
    bool passed = (peakL > 0.01f && peakR > 0.01f && nonZeroCountL > 100 && nonZeroCountR > 100);

    if (passed) {
        std::cout << "PASS - Convolution reverb is producing output" << std::endl;
        std::cout << "The previous damping fix appears to have resolved the issue!" << std::endl;
    } else {
        std::cout << "FAIL - Convolution reverb is still producing zero/minimal output" << std::endl;
        std::cout << "\nPlease check the debug output above for diagnostic information." << std::endl;
        std::cout << "Look for lines starting with 'ConvolutionReverb:' to identify the failure point." << std::endl;
    }

    return passed ? 0 : 1;
}
