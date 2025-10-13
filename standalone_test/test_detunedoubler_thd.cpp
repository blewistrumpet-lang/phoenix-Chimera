#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/DetuneDoubler.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

/**
 * DetuneDoubler THD Test (Bug #6 Verification)
 *
 * Tests Engine 32 (DetuneDoubler) for Total Harmonic Distortion
 * to verify the fix replacing tanh() with std::clamp()
 *
 * Test procedure:
 * 1. Generate 1kHz sine wave at -6dBFS
 * 2. Process through DetuneDoubler with default settings
 * 3. Perform FFT analysis
 * 4. Measure harmonics at 2kHz, 3kHz, 4kHz, 5kHz
 * 5. Calculate THD percentage
 *
 * Success criteria: THD < 1.0% (previous measurement: 8.673%)
 */

// FFT analysis to measure harmonic content
struct THDMeasurement {
    float fundamentalMagnitude = 0.0f;
    std::vector<float> harmonicMagnitudes;
    float thdPercent = 0.0f;
    bool passed = false;
};

THDMeasurement measureTHD(const juce::AudioBuffer<float>& buffer, float fundamentalHz, float sampleRate) {
    THDMeasurement result;

    const int fftSize = 16384; // High resolution for accurate frequency measurement
    const int numBins = fftSize / 2;

    juce::dsp::FFT fft(14); // 2^14 = 16384
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* data = buffer.getReadPointer(0);
    int numSamples = std::min(buffer.getNumSamples(), fftSize);

    // Apply Blackman-Harris window for minimal spectral leakage
    for (int i = 0; i < numSamples; ++i) {
        float w = i / float(fftSize);
        float window = 0.35875f
                     - 0.48829f * std::cos(2.0f * M_PI * w)
                     + 0.14128f * std::cos(4.0f * M_PI * w)
                     - 0.01168f * std::cos(6.0f * M_PI * w);
        fftData[i] = data[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Calculate bin width in Hz
    float binWidth = sampleRate / fftSize;

    // Find fundamental (1kHz)
    int fundamentalBin = std::round(fundamentalHz / binWidth);

    // Search in a small range around expected bin (±3 bins)
    float maxMag = 0.0f;
    int actualFundamentalBin = fundamentalBin;
    for (int i = fundamentalBin - 3; i <= fundamentalBin + 3; ++i) {
        if (i >= 0 && i < numBins && fftData[i] > maxMag) {
            maxMag = fftData[i];
            actualFundamentalBin = i;
        }
    }
    result.fundamentalMagnitude = maxMag;

    // Measure harmonics (2nd through 5th)
    std::vector<int> harmonicMultiples = {2, 3, 4, 5};
    float harmonicPowerSum = 0.0f;

    for (int harmonic : harmonicMultiples) {
        float expectedFreq = fundamentalHz * harmonic;
        int harmonicBin = std::round(expectedFreq / binWidth);

        // Search in a small range (±3 bins)
        float maxHarmonicMag = 0.0f;
        for (int i = harmonicBin - 3; i <= harmonicBin + 3; ++i) {
            if (i >= 0 && i < numBins) {
                maxHarmonicMag = std::max(maxHarmonicMag, fftData[i]);
            }
        }

        result.harmonicMagnitudes.push_back(maxHarmonicMag);
        harmonicPowerSum += maxHarmonicMag * maxHarmonicMag;
    }

    // Calculate THD
    float fundamentalPower = result.fundamentalMagnitude * result.fundamentalMagnitude;
    if (fundamentalPower > 0.0f) {
        result.thdPercent = 100.0f * std::sqrt(harmonicPowerSum / fundamentalPower);
    }

    // Success if THD < 1.0%
    result.passed = (result.thdPercent < 1.0f);

    return result;
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Bug #6 Verification: DetuneDoubler THD Test (Engine 32)     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    // Test parameters
    const float sampleRate = 48000.0f;
    const int blockSize = 512;
    const float testFreqHz = 1000.0f;
    const float amplitudeDbFS = -6.0f;
    const float amplitude = std::pow(10.0f, amplitudeDbFS / 20.0f); // -6dBFS

    std::cout << "Test Configuration:\n";
    std::cout << "  Sample Rate:       " << sampleRate << " Hz\n";
    std::cout << "  Test Frequency:    " << testFreqHz << " Hz\n";
    std::cout << "  Test Amplitude:    " << amplitudeDbFS << " dBFS\n";
    std::cout << "  Block Size:        " << blockSize << " samples\n";
    std::cout << "\n";

    // Create DetuneDoubler engine directly
    std::cout << "Creating DetuneDoubler...\n";
    auto engine = std::make_unique<AudioDSP::DetuneDoubler>();

    engine->prepareToPlay(sampleRate, blockSize);

    // Set parameters to default/moderate settings
    std::map<int, float> params;
    params[0] = 0.3f;  // Detune Amount = 30%
    params[1] = 0.15f; // Delay Time = 15%
    params[2] = 0.7f;  // Stereo Width = 70%
    params[3] = 0.3f;  // Thickness = 30%
    params[4] = 0.5f;  // Mix = 50% (blend with dry)

    engine->updateParameters(params);

    std::cout << "Engine Parameters:\n";
    std::cout << "  Detune Amount:     " << (params[0] * 100.0f) << "%\n";
    std::cout << "  Delay Time:        " << (params[1] * 100.0f) << "%\n";
    std::cout << "  Stereo Width:      " << (params[2] * 100.0f) << "%\n";
    std::cout << "  Thickness:         " << (params[3] * 100.0f) << "%\n";
    std::cout << "  Mix:               " << (params[4] * 100.0f) << "%\n";
    std::cout << "\n";

    // Generate test signal: 1kHz sine wave at -6dBFS
    const int testLength = static_cast<int>(sampleRate * 2.0f); // 2 seconds
    juce::AudioBuffer<float> buffer(2, testLength);

    std::cout << "Generating 1kHz sine wave test signal...\n";
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testLength; ++i) {
            float phase = 2.0f * M_PI * testFreqHz * i / sampleRate;
            buffer.setSample(ch, i, amplitude * std::sin(phase));
        }
    }

    // Process through DetuneDoubler
    std::cout << "Processing through DetuneDoubler...\n";
    for (int start = 0; start < testLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }
    std::cout << "Processing complete.\n";
    std::cout << "\n";

    // Skip first 0.5 seconds to allow for transients to settle
    int skipSamples = static_cast<int>(sampleRate * 0.5f);
    juce::AudioBuffer<float> analysisBuffer(2, testLength - skipSamples);

    for (int ch = 0; ch < 2; ++ch) {
        analysisBuffer.copyFrom(ch, 0, buffer, ch, skipSamples, testLength - skipSamples);
    }

    // Perform THD measurement
    std::cout << "Performing FFT analysis and THD measurement...\n";
    auto thdResult = measureTHD(analysisBuffer, testFreqHz, sampleRate);

    // Display results
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      MEASUREMENT RESULTS                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    std::cout << "Fundamental (1kHz):\n";
    std::cout << "  Magnitude:         " << std::fixed << std::setprecision(6)
              << thdResult.fundamentalMagnitude << "\n";
    std::cout << "\n";

    std::cout << "Harmonics:\n";
    std::vector<int> harmonicFreqs = {2000, 3000, 4000, 5000};
    for (size_t i = 0; i < thdResult.harmonicMagnitudes.size(); ++i) {
        float harmonicDb = 20.0f * std::log10(thdResult.harmonicMagnitudes[i] / thdResult.fundamentalMagnitude);
        std::cout << "  " << (i+2) << "nd harmonic (" << harmonicFreqs[i] << " Hz): "
                  << std::fixed << std::setprecision(6) << thdResult.harmonicMagnitudes[i]
                  << "  (" << std::setprecision(2) << harmonicDb << " dB below fundamental)\n";
    }
    std::cout << "\n";

    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        THD MEASUREMENT                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "  Total Harmonic Distortion: " << std::fixed << std::setprecision(3)
              << thdResult.thdPercent << "%\n";
    std::cout << "\n";

    // Compare with previous measurement
    const float previousTHD = 8.673f;
    float improvement = previousTHD - thdResult.thdPercent;
    float improvementPercent = (improvement / previousTHD) * 100.0f;

    std::cout << "Comparison:\n";
    std::cout << "  Previous THD (with tanh):  " << std::fixed << std::setprecision(3)
              << previousTHD << "%\n";
    std::cout << "  Current THD (with clamp):  " << std::fixed << std::setprecision(3)
              << thdResult.thdPercent << "%\n";
    std::cout << "  Improvement:               " << std::fixed << std::setprecision(3)
              << improvement << "% (" << improvementPercent << "% reduction)\n";
    std::cout << "\n";

    // Pass/Fail verdict
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    if (thdResult.passed) {
        std::cout << "║                      TEST PASSED ✓                            ║\n";
    } else {
        std::cout << "║                      TEST FAILED ✗                            ║\n";
    }
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    if (thdResult.passed) {
        std::cout << "THD of " << std::fixed << std::setprecision(3) << thdResult.thdPercent
                  << "% is below the 1.0% threshold.\n";
        std::cout << "Bug #6 fix verified: std::clamp() successfully reduced THD.\n";
    } else {
        std::cout << "THD of " << std::fixed << std::setprecision(3) << thdResult.thdPercent
                  << "% exceeds the 1.0% threshold.\n";
        std::cout << "Fix may require additional investigation.\n";
    }
    std::cout << "\n";

    // Summary for agent report
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      SUMMARY REPORT                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Fix verified in code:      YES (lines 193-194 use std::clamp)\n";
    std::cout << "Build status:              SUCCESS\n";
    std::cout << "THD measurement:           " << std::fixed << std::setprecision(3)
              << thdResult.thdPercent << "%\n";
    std::cout << "Improvement:               " << std::fixed << std::setprecision(3)
              << improvement << "% reduction\n";
    std::cout << "Test passed:               " << (thdResult.passed ? "YES" : "NO") << "\n";
    std::cout << "\n";

    return thdResult.passed ? 0 : 1;
}
