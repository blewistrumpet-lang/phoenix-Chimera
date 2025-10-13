// Deep diagnostic test for ConvolutionReverb Engine 41 zero output issue
// This test adds extensive IR generation diagnostics and buffer tracking

#include "../JUCE_Plugin/Source/ConvolutionReverb.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>

// Helper to display IR statistics
void analyzeIR(const juce::AudioBuffer<float>& ir, const char* label) {
    std::cout << "\n=== IR ANALYSIS: " << label << " ===" << std::endl;
    std::cout << "  Channels: " << ir.getNumChannels() << std::endl;
    std::cout << "  Length: " << ir.getNumSamples() << " samples" << std::endl;

    for (int ch = 0; ch < ir.getNumChannels(); ch++) {
        float peak = 0.0f;
        float rms = 0.0f;
        int nonZeroCount = 0;
        int firstNonZero = -1;
        int lastNonZero = -1;

        for (int i = 0; i < ir.getNumSamples(); i++) {
            float sample = ir.getSample(ch, i);
            float absSample = std::abs(sample);

            if (absSample > peak) peak = absSample;
            rms += sample * sample;

            if (absSample > 0.0001f) {
                if (firstNonZero < 0) firstNonZero = i;
                lastNonZero = i;
                nonZeroCount++;
            }
        }

        rms = std::sqrt(rms / ir.getNumSamples());

        std::cout << "  Channel " << ch << ":" << std::endl;
        std::cout << "    Peak: " << peak << " (" << (20.0f * std::log10(peak + 1e-10f)) << " dB)" << std::endl;
        std::cout << "    RMS: " << rms << " (" << (20.0f * std::log10(rms + 1e-10f)) << " dB)" << std::endl;
        std::cout << "    Non-zero samples: " << nonZeroCount << " / " << ir.getNumSamples()
                  << " (" << (100.0f * nonZeroCount / ir.getNumSamples()) << "%)" << std::endl;
        std::cout << "    First non-zero: " << firstNonZero << std::endl;
        std::cout << "    Last non-zero: " << lastNonZero << std::endl;

        // Show first 10 samples
        std::cout << "    First 10 samples: ";
        for (int i = 0; i < std::min(10, ir.getNumSamples()); i++) {
            std::cout << std::fixed << std::setprecision(6) << ir.getSample(ch, i);
            if (i < 9 && i < ir.getNumSamples() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "=== DEEP CONVOLUTION REVERB DIAGNOSTIC (Engine 41) ===" << std::endl;
    std::cout << "This test tracks IR generation, buffer allocation, and convolution processing\n" << std::endl;

    // Create engine
    ConvolutionReverb engine;

    // Initialize
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int numChannels = 2;

    std::cout << "Step 1: Initializing engine with sampleRate=" << sampleRate
              << ", blockSize=" << blockSize << std::endl;
    engine.prepareToPlay(sampleRate, blockSize);
    std::cout << "Engine initialized. Latency: " << engine.getLatencySamples() << " samples" << std::endl;

    // Test different parameter configurations
    std::vector<std::tuple<std::string, float, float, float>> testCases = {
        {"Default (Concert Hall, no damping)", 0.0f, 1.0f, 0.0f},      // IR=0, Size=100%, Damping=0
        {"Half size, no damping", 0.0f, 0.5f, 0.0f},                  // IR=0, Size=50%, Damping=0
        {"Full size, medium damping", 0.0f, 1.0f, 0.5f},              // IR=0, Size=100%, Damping=50%
        {"Full size, HEAVY damping (BUG)", 0.0f, 1.0f, 1.0f},        // IR=0, Size=100%, Damping=100%
        {"EMT Plate", 0.33f, 1.0f, 0.0f},                             // IR=1, Size=100%, Damping=0
    };

    for (const auto& testCase : testCases) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "TEST CASE: " << std::get<0>(testCase) << std::endl;
        std::cout << "========================================" << std::endl;

        float irSelect = std::get<1>(testCase);
        float size = std::get<2>(testCase);
        float damping = std::get<3>(testCase);

        // Set parameters
        std::map<int, float> params;
        params[0] = 1.0f;      // Mix = 100% wet
        params[1] = irSelect;  // IR Select
        params[2] = size;      // Size
        params[3] = 0.0f;      // Pre-Delay = 0ms
        params[4] = damping;   // Damping
        params[5] = 0.0f;      // Reverse = off
        params[6] = 0.5f;      // Early/Late = balanced
        params[7] = 0.0f;      // Low Cut = off
        params[8] = 1.0f;      // High Cut = off
        params[9] = 1.0f;      // Width = 100%

        std::cout << "Parameters: IR=" << irSelect << ", Size=" << size << ", Damping=" << damping << std::endl;
        engine.updateParameters(params);
        std::cout << "Parameters updated. Latency now: " << engine.getLatencySamples() << " samples" << std::endl;

        // Process impulse - 3 blocks to account for latency
        std::cout << "\nProcessing impulse through 3 blocks (accounting for latency)..." << std::endl;

        std::vector<float> outputL, outputR;

        for (int block = 0; block < 3; block++) {
            juce::AudioBuffer<float> buffer(numChannels, blockSize);
            buffer.clear();

            // Impulse in first block only
            if (block == 0) {
                buffer.setSample(0, 0, 1.0f);
                buffer.setSample(1, 0, 1.0f);
                std::cout << "  Block " << block << ": Input impulse (1.0 at sample 0)" << std::endl;
            } else {
                std::cout << "  Block " << block << ": Silence" << std::endl;
            }

            // Process
            engine.process(buffer);

            // Analyze output
            float peakL = 0.0f, peakR = 0.0f;
            int nonZeroL = 0, nonZeroR = 0;

            for (int i = 0; i < blockSize; i++) {
                float l = buffer.getSample(0, i);
                float r = buffer.getSample(1, i);
                outputL.push_back(l);
                outputR.push_back(r);

                if (std::abs(l) > peakL) peakL = std::abs(l);
                if (std::abs(r) > peakR) peakR = std::abs(r);
                if (std::abs(l) > 0.0001f) nonZeroL++;
                if (std::abs(r) > 0.0001f) nonZeroR++;
            }

            std::cout << "    Output: PeakL=" << peakL << ", PeakR=" << peakR
                      << ", NonZeroL=" << nonZeroL << ", NonZeroR=" << nonZeroR << std::endl;
        }

        // Overall analysis
        std::cout << "\nOverall output analysis (all " << outputL.size() << " samples):" << std::endl;

        float peakL = 0.0f, peakR = 0.0f;
        float rmsL = 0.0f, rmsR = 0.0f;
        int nonZeroL = 0, nonZeroR = 0;
        int firstNonZeroL = -1, firstNonZeroR = -1;

        for (size_t i = 0; i < outputL.size(); i++) {
            float absL = std::abs(outputL[i]);
            float absR = std::abs(outputR[i]);

            if (absL > peakL) peakL = absL;
            if (absR > peakR) peakR = absR;

            rmsL += outputL[i] * outputL[i];
            rmsR += outputR[i] * outputR[i];

            if (absL > 0.0001f) {
                if (firstNonZeroL < 0) firstNonZeroL = i;
                nonZeroL++;
            }
            if (absR > 0.0001f) {
                if (firstNonZeroR < 0) firstNonZeroR = i;
                nonZeroR++;
            }
        }

        rmsL = std::sqrt(rmsL / outputL.size());
        rmsR = std::sqrt(rmsR / outputR.size());

        std::cout << "  Left:  Peak=" << peakL << " dB=" << (20.0f * std::log10(peakL + 1e-10f))
                  << ", RMS=" << rmsL << ", NonZero=" << nonZeroL << " ("
                  << (100.0f * nonZeroL / outputL.size()) << "%)" << std::endl;
        std::cout << "  Right: Peak=" << peakR << " dB=" << (20.0f * std::log10(peakR + 1e-10f))
                  << ", RMS=" << rmsR << ", NonZero=" << nonZeroR << " ("
                  << (100.0f * nonZeroR / outputR.size()) << "%)" << std::endl;
        std::cout << "  First non-zero: L=" << firstNonZeroL << ", R=" << firstNonZeroR << std::endl;

        // Verdict
        bool hasOutput = (peakL > 0.01f && peakR > 0.01f && nonZeroL > 10 && nonZeroR > 10);
        std::cout << "\n  Result: " << (hasOutput ? "PASS - Has output" : "FAIL - Zero/minimal output") << std::endl;

        if (!hasOutput && damping > 0.9f) {
            std::cout << "  >> This is the DAMPING BUG - damping parameter kills the IR!" << std::endl;
        } else if (!hasOutput && nonZeroL <= 1 && nonZeroR <= 1) {
            std::cout << "  >> IR appears to have only one sample - not generating properly!" << std::endl;
        }
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "DIAGNOSTIC COMPLETE" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nCheck the debug output above (lines starting with 'ConvolutionReverb:') " << std::endl;
    std::cout << "to identify the exact failure point in IR generation." << std::endl;

    return 0;
}
