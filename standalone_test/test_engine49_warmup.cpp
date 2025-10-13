// Minimal test for Engine 49 (PhasedVocoder) warmup verification
#include "../JUCE_Plugin/Source/PhasedVocoder.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <iostream>
#include <fstream>
#include <cmath>

int main() {
    const double sampleRate = 44100.0;
    const int blockSize = 512;
    const int testDuration = 8192;  // ~185ms at 44.1kHz

    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Engine 49 (PhasedVocoder) Warmup Verification Test  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n\n";

    // Create engine
    PhasedVocoder engine;
    engine.prepareToPlay(sampleRate, blockSize);

    // Set parameters for pass-through
    std::map<int, float> params;
    params[static_cast<int>(PhasedVocoder::ParamID::Mix)] = 1.0f;  // 100% wet
    params[static_cast<int>(PhasedVocoder::ParamID::TimeStretch)] = 0.2f;  // 1.0x (pass-through)
    params[static_cast<int>(PhasedVocoder::ParamID::PitchShift)] = 0.5f;  // 1.0x (no pitch shift)
    engine.updateParameters(params);

    // Create buffer with impulse at sample 0
    juce::AudioBuffer<float> buffer(1, testDuration);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);  // Impulse

    // Process in blocks
    int samplesProcessed = 0;
    int firstNonZero = -1;
    float peakLevel = 0.0f;
    int nonZeroCount = 0;

    std::ofstream csv("impulse_engine_49.csv");
    csv << "sample,amplitude\n";

    while (samplesProcessed < testDuration) {
        int samplesToProcess = std::min(blockSize, testDuration - samplesProcessed);

        // Create a view of the current block
        juce::AudioBuffer<float> blockBuffer(1, samplesToProcess);
        for (int i = 0; i < samplesToProcess; ++i) {
            blockBuffer.setSample(0, i, buffer.getSample(0, samplesProcessed + i));
        }

        // Process the block
        engine.process(blockBuffer);

        // Copy back to main buffer
        for (int i = 0; i < samplesToProcess; ++i) {
            float sample = blockBuffer.getSample(0, i);
            buffer.setSample(0, samplesProcessed + i, sample);

            // Track first non-zero output
            if (std::abs(sample) > 1e-6f && firstNonZero == -1) {
                firstNonZero = samplesProcessed + i;
            }

            // Track peak and count non-zero samples
            if (std::abs(sample) > 1e-6f) {
                peakLevel = std::max(peakLevel, std::abs(sample));
                nonZeroCount++;
            }

            // Write to CSV
            csv << (samplesProcessed + i) << "," << sample << "\n";
        }

        samplesProcessed += samplesToProcess;
    }

    csv.close();

    // Calculate metrics
    const double firstOutputMs = (firstNonZero >= 0) ? (firstNonZero / sampleRate * 1000.0) : -1.0;
    const int expectedWarmup = 2048;  // After fix: latency only
    const double expectedMs = expectedWarmup / sampleRate * 1000.0;

    std::cout << "═══ Test Results ═══\n\n";
    std::cout << "Sample Rate:        " << sampleRate << " Hz\n";
    std::cout << "Test Duration:      " << testDuration << " samples ("
              << (testDuration / sampleRate * 1000.0) << " ms)\n\n";

    std::cout << "First Non-Zero:     ";
    if (firstNonZero >= 0) {
        std::cout << firstNonZero << " samples (" << firstOutputMs << " ms)\n";
    } else {
        std::cout << "NONE DETECTED\n";
    }

    std::cout << "Expected Warmup:    " << expectedWarmup << " samples ("
              << expectedMs << " ms)\n";
    std::cout << "Peak Level:         " << peakLevel << "\n";
    std::cout << "Non-Zero Samples:   " << nonZeroCount << "\n";

    // Verify fix
    bool fixVerified = false;
    bool outputPresent = (firstNonZero >= 0);
    bool timingCorrect = false;
    bool peakAcceptable = (peakLevel > 0.01f);  // Should have reasonable output
    bool continuousOutput = (nonZeroCount > 100);  // Should have sustained output

    if (firstNonZero >= 0) {
        // Allow ±10% tolerance
        double tolerance = expectedWarmup * 0.1;
        timingCorrect = (std::abs(firstNonZero - expectedWarmup) <= tolerance);
    }

    fixVerified = outputPresent && timingCorrect && peakAcceptable && continuousOutput;

    std::cout << "\n═══ Verification ═══\n\n";
    std::cout << "Output Present:     " << (outputPresent ? "YES" : "NO") << "\n";
    std::cout << "Timing Correct:     " << (timingCorrect ? "YES" : "NO")
              << " (within 10% of " << expectedWarmup << " samples)\n";
    std::cout << "Peak Acceptable:    " << (peakAcceptable ? "YES" : "NO")
              << " (>0.01)\n";
    std::cout << "Continuous Output:  " << (continuousOutput ? "YES" : "NO")
              << " (>100 samples)\n";
    std::cout << "\n═══════════════════\n";
    std::cout << "TEST " << (fixVerified ? "PASSED ✓" : "FAILED ✗") << "\n";
    std::cout << "═══════════════════\n\n";

    std::cout << "CSV saved to: impulse_engine_49.csv\n\n";

    return fixVerified ? 0 : 1;
}
