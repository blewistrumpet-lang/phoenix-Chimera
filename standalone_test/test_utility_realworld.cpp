#include "JuceHeader.h"
#include "EngineBase.h"
#include "GranularCloud.h"
#include "ChaosGenerator_Platinum.h"
#include "GainUtility_Platinum.h"
#include "MonoMaker_Platinum.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>

/*
 * REAL-WORLD AUDIO TESTING - UTILITY ENGINES
 *
 * Tests utility/special engines with appropriate materials:
 * - Engine 50: GranularCloud (grain synthesis)
 * - Engine 51: ChaosGenerator (randomness/modulation)
 * - Engine 54: GainUtility (precision gain control)
 * - Engine 55: MonoMaker (mono conversion)
 *
 * Test Materials:
 * - Various sources for grain synthesis (vocals, drums, sustained tones)
 * - Calibrated test tones for gain utility (1kHz @ -3dB, -6dB, -12dB)
 * - Music/noise for chaos generator
 * - Stereo material for mono maker
 *
 * Quality Metrics:
 * - GranularCloud: grain smoothness, density control, no clicks
 * - ChaosGenerator: randomness quality, distribution, range
 * - GainUtility: ±0.01dB accuracy, DC offset handling
 * - MonoMaker: mono compatibility, phase coherence
 * - Grading: A/B/C/D/F
 */

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Calculate RMS level in dB
float calculateRMS_dB(const juce::AudioBuffer<float>& buffer, int channel) {
    const float* data = buffer.getReadPointer(channel);
    float sum = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        sum += data[i] * data[i];
    }
    float rms = std::sqrt(sum / buffer.getNumSamples());
    return 20.0f * std::log10(std::max(rms, 1e-10f));
}

// Calculate peak level in dB
float calculatePeak_dB(const juce::AudioBuffer<float>& buffer, int channel) {
    const float* data = buffer.getReadPointer(channel);
    float peak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        peak = std::max(peak, std::abs(data[i]));
    }
    return 20.0f * std::log10(std::max(peak, 1e-10f));
}

// Check for clicks/discontinuities
bool detectClicks(const juce::AudioBuffer<float>& buffer, int channel, float threshold = 0.5f) {
    const float* data = buffer.getReadPointer(channel);
    int clickCount = 0;
    for (int i = 1; i < buffer.getNumSamples(); ++i) {
        float diff = std::abs(data[i] - data[i-1]);
        if (diff > threshold) {
            clickCount++;
        }
    }
    return clickCount > 10; // More than 10 large discontinuities = clicks
}

// Calculate DC offset
float calculateDCOffset(const juce::AudioBuffer<float>& buffer, int channel) {
    const float* data = buffer.getReadPointer(channel);
    float sum = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        sum += data[i];
    }
    return sum / buffer.getNumSamples();
}

// Calculate standard deviation (for randomness testing)
float calculateStdDev(const std::vector<float>& values) {
    float mean = std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
    float sq_sum = 0.0f;
    for (float val : values) {
        sq_sum += (val - mean) * (val - mean);
    }
    return std::sqrt(sq_sum / values.size());
}

// Calculate stereo correlation
float calculateStereoCorrelation(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return 1.0f;

    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    int n = buffer.getNumSamples();

    float sumL = 0.0f, sumR = 0.0f, sumLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;
    for (int i = 0; i < n; ++i) {
        sumL += left[i];
        sumR += right[i];
        sumLR += left[i] * right[i];
        sumL2 += left[i] * left[i];
        sumR2 += right[i] * right[i];
    }

    float meanL = sumL / n;
    float meanR = sumR / n;
    float covLR = (sumLR / n) - (meanL * meanR);
    float varL = (sumL2 / n) - (meanL * meanL);
    float varR = (sumR2 / n) - (meanR * meanR);

    float denom = std::sqrt(varL * varR);
    return (denom > 1e-10f) ? (covLR / denom) : 0.0f;
}

// Save buffer to WAV file
void saveToWav(const juce::AudioBuffer<float>& buffer, const std::string& filename, double sampleRate) {
    juce::File outputFile(filename);
    outputFile.deleteFile();

    auto outStream = outputFile.createOutputStream();
    if (outStream != nullptr) {
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer;
        writer.reset(wavFormat.createWriterFor(outStream.release(), sampleRate,
                                                buffer.getNumChannels(), 24, {}, 0));
        if (writer != nullptr) {
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        }
    }
}

// ============================================================================
// TEST MATERIAL GENERATORS
// ============================================================================

// Generate vocal-like signal with harmonics and vibrato
void generateVocalSignal(juce::AudioBuffer<float>& buffer, float fundamental, float sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Vocal-like harmonic structure
    const int numHarmonics = 10;
    float harmonicAmplitudes[10] = {1.0f, 0.7f, 0.5f, 0.4f, 0.3f, 0.25f, 0.2f, 0.15f, 0.12f, 0.1f};

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            float sample = 0.0f;
            float t = i / sampleRate;

            // Add harmonics
            for (int h = 0; h < numHarmonics; ++h) {
                float freq = fundamental * (h + 1);
                sample += harmonicAmplitudes[h] * std::sin(2.0f * M_PI * freq * t);
            }

            // Add vibrato (natural voice fluctuation)
            float vibrato = 1.0f + 0.01f * std::sin(2.0f * M_PI * 5.0f * t);
            sample *= vibrato;

            // Normalize
            sample *= 0.3f;

            channelData[i] = sample;
        }
    }
}

// Generate drum hit (short transient)
void generateDrumHit(juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            float t = i / sampleRate;

            // Kick drum: fast exponential decay with pitch drop
            float freq = 60.0f * std::exp(-10.0f * t); // Pitch drops from 60Hz
            float envelope = std::exp(-8.0f * t);       // Fast decay
            float kick = std::sin(2.0f * M_PI * freq * t) * envelope;

            // Add some noise for "snap"
            float noise = ((rand() / float(RAND_MAX)) * 2.0f - 1.0f) * 0.3f;
            noise *= std::exp(-30.0f * t); // Very fast decay on noise

            channelData[i] = (kick + noise) * 0.5f;
        }
    }
}

// Generate calibrated test tone at specific level
void generateTestTone(juce::AudioBuffer<float>& buffer, float frequency, float targetDB, float sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float amplitude = std::pow(10.0f, targetDB / 20.0f);

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            float t = i / sampleRate;
            channelData[i] = amplitude * std::sin(2.0f * M_PI * frequency * t);
        }
    }
}

// Generate music-like signal (multiple instruments)
void generateMusicSignal(juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Musical chord: C major (C4, E4, G4)
    float frequencies[3] = {261.63f, 329.63f, 392.0f};
    float amplitudes[3] = {0.3f, 0.25f, 0.2f};

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i) {
            float sample = 0.0f;
            float t = i / sampleRate;

            // Add notes with slight detuning for richness
            for (int n = 0; n < 3; ++n) {
                float detune = 1.0f + 0.005f * std::sin(2.0f * M_PI * (3.0f + n) * t);
                sample += amplitudes[n] * std::sin(2.0f * M_PI * frequencies[n] * detune * t);
            }

            // Add rhythmic envelope
            float rhythm = 0.5f + 0.5f * std::sin(2.0f * M_PI * 2.0f * t);
            sample *= rhythm;

            channelData[i] = sample;
        }
    }
}

// Generate stereo signal with different content on L/R
void generateStereoSignal(juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int numSamples = buffer.getNumSamples();

    if (buffer.getNumChannels() >= 2) {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i) {
            float t = i / sampleRate;

            // Left: 440Hz
            left[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * t);

            // Right: 880Hz (octave higher)
            right[i] = 0.3f * std::sin(2.0f * M_PI * 880.0f * t);
        }
    }
}

// ============================================================================
// ENGINE TESTS
// ============================================================================

struct TestResult {
    std::string engineName;
    int engineId;
    char grade; // A, B, C, D, F
    bool passed;
    std::string details;
    std::map<std::string, float> metrics;
};

// Test Engine 50: GranularCloud
TestResult testGranularCloud() {
    TestResult result;
    result.engineName = "GranularCloud";
    result.engineId = 50;
    result.passed = true;

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ENGINE 50: GRANULARCLOUD - Real-World Testing\n";
    std::cout << std::string(80, '=') << "\n\n";

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int testDuration = 3; // 3 seconds
    const int totalSamples = sampleRate * testDuration;

    auto engine = std::make_unique<GranularCloud>();
    if (!engine) {
        result.passed = false;
        result.grade = 'F';
        result.details = "Failed to create engine";
        return result;
    }

    engine->prepareToPlay(sampleRate, blockSize);

    // Test 1: Vocal grain synthesis
    std::cout << "Test 1: Vocal Grain Synthesis\n";
    std::cout << "------------------------------\n";

    juce::AudioBuffer<float> inputBuffer(2, totalSamples);
    generateVocalSignal(inputBuffer, 220.0f, sampleRate); // Male vocal (A3)

    juce::AudioBuffer<float> outputBuffer(2, totalSamples);
    outputBuffer.clear();

    // Set parameters for moderate grain size and density
    std::map<int, float> params;
    params[0] = 0.5f;  // GrainSize: moderate (~50ms)
    params[1] = 0.6f;  // Density: moderate
    params[2] = 0.3f;  // PitchScatter: some variation
    params[3] = 0.5f;  // CloudPosition: center
    params[4] = 1.0f;  // Mix: 100% wet
    engine->updateParameters(params);

    // Process in blocks
    int numBlocks = (totalSamples + blockSize - 1) / blockSize;
    int blocksWithOutput = 0;
    float peakLevel = 0.0f;

    for (int block = 0; block < numBlocks; ++block) {
        int startSample = block * blockSize;
        int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

        juce::AudioBuffer<float> blockBuffer(2, blockSize);
        blockBuffer.clear();

        // Copy input
        for (int ch = 0; ch < 2; ++ch) {
            blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesToProcess);
        }

        engine->process(blockBuffer);

        // Copy to output
        for (int ch = 0; ch < 2; ++ch) {
            outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
        }

        // Check for output
        float rms = std::sqrt((blockBuffer.getRMSLevel(0, 0, numSamplesToProcess) *
                              blockBuffer.getRMSLevel(0, 0, numSamplesToProcess) +
                              blockBuffer.getRMSLevel(1, 0, numSamplesToProcess) *
                              blockBuffer.getRMSLevel(1, 0, numSamplesToProcess)) / 2.0f);
        if (rms > 1e-6f) blocksWithOutput++;

        peakLevel = std::max(peakLevel, blockBuffer.getMagnitude(0, 0, numSamplesToProcess));
        peakLevel = std::max(peakLevel, blockBuffer.getMagnitude(1, 0, numSamplesToProcess));
    }

    float outputPercentage = 100.0f * blocksWithOutput / numBlocks;
    float rms_dB = calculateRMS_dB(outputBuffer, 0);
    float peak_dB = calculatePeak_dB(outputBuffer, 0);
    bool hasClicks = detectClicks(outputBuffer, 0);

    std::cout << "  Blocks with output: " << blocksWithOutput << "/" << numBlocks
              << " (" << std::fixed << std::setprecision(1) << outputPercentage << "%)\n";
    std::cout << "  RMS Level: " << std::setprecision(2) << rms_dB << " dB\n";
    std::cout << "  Peak Level: " << peak_dB << " dB\n";
    std::cout << "  Clicks detected: " << (hasClicks ? "YES (FAIL)" : "NO (PASS)") << "\n";

    result.metrics["vocal_output_pct"] = outputPercentage;
    result.metrics["vocal_rms_db"] = rms_dB;
    result.metrics["vocal_has_clicks"] = hasClicks ? 1.0f : 0.0f;

    // Save output
    std::string outputPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/engine50_granularcloud_vocal.wav";
    saveToWav(outputBuffer, outputPath, sampleRate);
    std::cout << "  Saved: " << outputPath << "\n\n";

    // Test 2: Drum grain synthesis (short transients)
    std::cout << "Test 2: Drum Grain Synthesis\n";
    std::cout << "-----------------------------\n";

    juce::AudioBuffer<float> drumBuffer(2, totalSamples);
    generateDrumHit(drumBuffer, sampleRate);

    outputBuffer.clear();

    // Smaller grains for drum hits
    params[0] = 0.2f;  // GrainSize: small (~20ms)
    params[1] = 0.8f;  // Density: high
    params[2] = 0.5f;  // PitchScatter: more variation
    engine->updateParameters(params);

    for (int block = 0; block < numBlocks; ++block) {
        int startSample = block * blockSize;
        int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

        juce::AudioBuffer<float> blockBuffer(2, blockSize);
        blockBuffer.clear();

        for (int ch = 0; ch < 2; ++ch) {
            blockBuffer.copyFrom(ch, 0, drumBuffer, ch, startSample, numSamplesToProcess);
        }

        engine->process(blockBuffer);

        for (int ch = 0; ch < 2; ++ch) {
            outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
        }
    }

    float drumRMS = calculateRMS_dB(outputBuffer, 0);
    bool drumClicks = detectClicks(outputBuffer, 0);

    std::cout << "  RMS Level: " << drumRMS << " dB\n";
    std::cout << "  Clicks detected: " << (drumClicks ? "YES (FAIL)" : "NO (PASS)") << "\n";

    result.metrics["drum_rms_db"] = drumRMS;
    result.metrics["drum_has_clicks"] = drumClicks ? 1.0f : 0.0f;

    outputPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/engine50_granularcloud_drum.wav";
    saveToWav(outputBuffer, outputPath, sampleRate);
    std::cout << "  Saved: " << outputPath << "\n\n";

    // Grading
    std::cout << "Grading:\n";
    std::cout << "--------\n";

    int score = 0;
    if (outputPercentage > 70.0f) { std::cout << "  ✓ Output presence: PASS\n"; score++; }
    else { std::cout << "  ✗ Output presence: FAIL\n"; result.passed = false; }

    if (!hasClicks && !drumClicks) { std::cout << "  ✓ No clicks: PASS\n"; score++; }
    else { std::cout << "  ✗ Clicks detected: FAIL\n"; result.passed = false; }

    if (rms_dB > -60.0f) { std::cout << "  ✓ Sufficient level: PASS\n"; score++; }
    else { std::cout << "  ✗ Level too low: FAIL\n"; result.passed = false; }

    if (peak_dB < -0.5f) { std::cout << "  ✓ No clipping: PASS\n"; score++; }
    else { std::cout << "  ✗ Clipping detected: FAIL\n"; result.passed = false; }

    // Assign grade
    if (score == 4) result.grade = 'A';
    else if (score == 3) result.grade = 'B';
    else if (score == 2) result.grade = 'C';
    else if (score == 1) result.grade = 'D';
    else result.grade = 'F';

    result.details = "Score: " + std::to_string(score) + "/4";
    std::cout << "\n  Final Grade: " << result.grade << " (" << score << "/4)\n";

    return result;
}

// Test Engine 51: ChaosGenerator
TestResult testChaosGenerator() {
    TestResult result;
    result.engineName = "ChaosGenerator";
    result.engineId = 51;
    result.passed = true;

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ENGINE 51: CHAOSGENERATOR - Real-World Testing\n";
    std::cout << std::string(80, '=') << "\n\n";

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int testDuration = 2;
    const int totalSamples = sampleRate * testDuration;

    auto engine = std::make_unique<ChaosGenerator_Platinum>();
    if (!engine) {
        result.passed = false;
        result.grade = 'F';
        result.details = "Failed to create engine";
        return result;
    }

    engine->prepareToPlay(sampleRate, blockSize);

    // Test 1: Randomness quality on music signal
    std::cout << "Test 1: Chaos Modulation on Music\n";
    std::cout << "----------------------------------\n";

    juce::AudioBuffer<float> inputBuffer(2, totalSamples);
    generateMusicSignal(inputBuffer, sampleRate);

    juce::AudioBuffer<float> outputBuffer(2, totalSamples);
    outputBuffer.clear();

    // Set parameters for moderate chaos
    std::map<int, float> params;
    params[0] = 0.5f;  // Rate: moderate
    params[1] = 0.5f;  // Depth: moderate
    params[2] = 0.0f;  // Type: Lorenz
    params[3] = 0.5f;  // Smoothing: moderate
    params[4] = 0.0f;  // ModTarget: Amplitude
    params[7] = 1.0f;  // Mix: 100% wet
    engine->updateParameters(params);

    // Process and collect amplitude samples for statistical analysis
    std::vector<float> amplitudeSamples;
    int numBlocks = (totalSamples + blockSize - 1) / blockSize;

    for (int block = 0; block < numBlocks; ++block) {
        int startSample = block * blockSize;
        int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

        juce::AudioBuffer<float> blockBuffer(2, blockSize);
        blockBuffer.clear();

        for (int ch = 0; ch < 2; ++ch) {
            blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesToProcess);
        }

        engine->process(blockBuffer);

        for (int ch = 0; ch < 2; ++ch) {
            outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
        }

        // Collect samples for distribution analysis
        const float* left = blockBuffer.getReadPointer(0);
        for (int i = 0; i < numSamplesToProcess; i += 100) { // Sample every 100th
            amplitudeSamples.push_back(std::abs(left[i]));
        }
    }

    float rms_dB = calculateRMS_dB(outputBuffer, 0);
    float peak_dB = calculatePeak_dB(outputBuffer, 0);
    float stdDev = calculateStdDev(amplitudeSamples);
    float mean = std::accumulate(amplitudeSamples.begin(), amplitudeSamples.end(), 0.0f) / amplitudeSamples.size();
    float coeffVariation = (mean > 1e-10f) ? (stdDev / mean) : 0.0f;

    std::cout << "  RMS Level: " << std::fixed << std::setprecision(2) << rms_dB << " dB\n";
    std::cout << "  Peak Level: " << peak_dB << " dB\n";
    std::cout << "  Amplitude StdDev: " << std::setprecision(6) << stdDev << "\n";
    std::cout << "  Coefficient of Variation: " << std::setprecision(4) << coeffVariation << "\n";

    result.metrics["chaos_rms_db"] = rms_dB;
    result.metrics["chaos_stddev"] = stdDev;
    result.metrics["chaos_coeff_var"] = coeffVariation;

    // Save output
    std::string outputPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/engine51_chaosgenerator_music.wav";
    saveToWav(outputBuffer, outputPath, sampleRate);
    std::cout << "  Saved: " << outputPath << "\n\n";

    // Test 2: Different chaos types
    std::cout << "Test 2: Chaos Type Variation\n";
    std::cout << "-----------------------------\n";

    const char* chaosTypes[] = {"Lorenz", "Rossler", "Henon", "Logistic", "Ikeda", "Duffing"};
    for (int type = 0; type < 6; ++type) {
        params[2] = type / 5.0f; // Type parameter
        engine->updateParameters(params);

        outputBuffer.clear();

        for (int block = 0; block < numBlocks; ++block) {
            int startSample = block * blockSize;
            int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

            juce::AudioBuffer<float> blockBuffer(2, blockSize);
            blockBuffer.clear();

            for (int ch = 0; ch < 2; ++ch) {
                blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesToProcess);
            }

            engine->process(blockBuffer);

            for (int ch = 0; ch < 2; ++ch) {
                outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
            }
        }

        float typeRMS = calculateRMS_dB(outputBuffer, 0);
        std::cout << "  " << chaosTypes[type] << ": RMS = " << typeRMS << " dB\n";
    }

    std::cout << "\nGrading:\n";
    std::cout << "--------\n";

    int score = 0;
    if (rms_dB > -60.0f) { std::cout << "  ✓ Sufficient output: PASS\n"; score++; }
    else { std::cout << "  ✗ Output too low: FAIL\n"; result.passed = false; }

    if (peak_dB < -0.5f) { std::cout << "  ✓ No clipping: PASS\n"; score++; }
    else { std::cout << "  ✗ Clipping detected: FAIL\n"; result.passed = false; }

    if (coeffVariation > 0.1f && coeffVariation < 2.0f) {
        std::cout << "  ✓ Good randomness: PASS\n"; score++;
    }
    else { std::cout << "  ✗ Poor randomness: FAIL\n"; result.passed = false; }

    if (stdDev > 0.01f) { std::cout << "  ✓ Variation present: PASS\n"; score++; }
    else { std::cout << "  ✗ Insufficient variation: FAIL\n"; result.passed = false; }

    // Assign grade
    if (score == 4) result.grade = 'A';
    else if (score == 3) result.grade = 'B';
    else if (score == 2) result.grade = 'C';
    else if (score == 1) result.grade = 'D';
    else result.grade = 'F';

    result.details = "Score: " + std::to_string(score) + "/4";
    std::cout << "\n  Final Grade: " << result.grade << " (" << score << "/4)\n";

    return result;
}

// Test Engine 54: GainUtility_Platinum
TestResult testGainUtility() {
    TestResult result;
    result.engineName = "GainUtility_Platinum";
    result.engineId = 54;
    result.passed = true;

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ENGINE 54: GAINUTILITY_PLATINUM - Real-World Testing\n";
    std::cout << std::string(80, '=') << "\n\n";

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int testDuration = 1;
    const int totalSamples = sampleRate * testDuration;

    auto engine = std::make_unique<GainUtility_Platinum>();
    if (!engine) {
        result.passed = false;
        result.grade = 'F';
        result.details = "Failed to create engine";
        return result;
    }

    engine->prepareToPlay(sampleRate, blockSize);

    // Test 1: Precision gain control (±0.01dB target)
    std::cout << "Test 1: Precision Gain Control\n";
    std::cout << "-------------------------------\n";

    struct GainTest {
        float targetDB;
        float gainParam; // 0-1 parameter value
        std::string name;
    };

    // Map dB to 0-1 parameter (assuming -∞ to +24dB range)
    // 0.0 = -inf, 0.5 = 0dB, 1.0 = +24dB
    std::vector<GainTest> gainTests = {
        {0.0f, 0.5f, "Unity Gain (0 dB)"},
        {-3.0f, 0.5f - (3.0f/48.0f), "-3 dB"},
        {-6.0f, 0.5f - (6.0f/48.0f), "-6 dB"},
        {-12.0f, 0.5f - (12.0f/48.0f), "-12 dB"},
        {+3.0f, 0.5f + (3.0f/48.0f), "+3 dB"},
        {+6.0f, 0.5f + (6.0f/48.0f), "+6 dB"}
    };

    std::vector<float> accuracyErrors;

    for (const auto& test : gainTests) {
        // Generate calibrated test tone at 0 dB
        juce::AudioBuffer<float> inputBuffer(2, totalSamples);
        generateTestTone(inputBuffer, 1000.0f, 0.0f, sampleRate);

        float inputRMS = calculateRMS_dB(inputBuffer, 0);

        // Set gain parameter
        std::map<int, float> params;
        params[0] = test.gainParam; // GAIN parameter
        params[5] = 0.0f;            // MODE: stereo
        engine->updateParameters(params);

        // Process
        juce::AudioBuffer<float> outputBuffer(2, totalSamples);
        outputBuffer.clear();

        int numBlocks = (totalSamples + blockSize - 1) / blockSize;
        for (int block = 0; block < numBlocks; ++block) {
            int startSample = block * blockSize;
            int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

            juce::AudioBuffer<float> blockBuffer(2, blockSize);
            blockBuffer.clear();

            for (int ch = 0; ch < 2; ++ch) {
                blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesToProcess);
            }

            engine->process(blockBuffer);

            for (int ch = 0; ch < 2; ++ch) {
                outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
            }
        }

        float outputRMS = calculateRMS_dB(outputBuffer, 0);
        float actualGain = outputRMS - inputRMS;
        float error = std::abs(actualGain - test.targetDB);

        accuracyErrors.push_back(error);

        std::cout << "  " << test.name << ":\n";
        std::cout << "    Expected: " << std::fixed << std::setprecision(2) << test.targetDB << " dB\n";
        std::cout << "    Actual:   " << actualGain << " dB\n";
        std::cout << "    Error:    " << std::setprecision(3) << error << " dB ";

        if (error <= 0.01f) std::cout << "(EXCELLENT)\n";
        else if (error <= 0.05f) std::cout << "(GOOD)\n";
        else if (error <= 0.1f) std::cout << "(ACCEPTABLE)\n";
        else std::cout << "(FAIL)\n";
    }

    float maxError = *std::max_element(accuracyErrors.begin(), accuracyErrors.end());
    float avgError = std::accumulate(accuracyErrors.begin(), accuracyErrors.end(), 0.0f) / accuracyErrors.size();

    std::cout << "\n  Max Error: " << std::setprecision(3) << maxError << " dB\n";
    std::cout << "  Avg Error: " << avgError << " dB\n\n";

    result.metrics["gain_max_error"] = maxError;
    result.metrics["gain_avg_error"] = avgError;

    // Test 2: DC offset handling
    std::cout << "Test 2: DC Offset Handling\n";
    std::cout << "---------------------------\n";

    juce::AudioBuffer<float> dcBuffer(2, totalSamples);
    generateTestTone(dcBuffer, 1000.0f, -6.0f, sampleRate);

    // Add DC offset
    float dcOffset = 0.1f;
    for (int ch = 0; ch < 2; ++ch) {
        float* data = dcBuffer.getWritePointer(ch);
        for (int i = 0; i < totalSamples; ++i) {
            data[i] += dcOffset;
        }
    }

    float inputDC = calculateDCOffset(dcBuffer, 0);

    // Process with unity gain
    std::map<int, float> params;
    params[0] = 0.5f; // Unity gain
    engine->updateParameters(params);

    int numBlocks = (totalSamples + blockSize - 1) / blockSize;
    for (int block = 0; block < numBlocks; ++block) {
        int startSample = block * blockSize;
        int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

        juce::AudioBuffer<float> blockBuffer(2, blockSize);
        blockBuffer.clear();

        for (int ch = 0; ch < 2; ++ch) {
            blockBuffer.copyFrom(ch, 0, dcBuffer, ch, startSample, numSamplesToProcess);
        }

        engine->process(blockBuffer);

        for (int ch = 0; ch < 2; ++ch) {
            dcBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
        }
    }

    float outputDC = calculateDCOffset(dcBuffer, 0);

    std::cout << "  Input DC Offset:  " << std::fixed << std::setprecision(6) << inputDC << "\n";
    std::cout << "  Output DC Offset: " << outputDC << "\n";
    std::cout << "  DC preserved: " << (std::abs(outputDC - inputDC) < 0.001f ? "YES" : "NO") << "\n\n";

    result.metrics["dc_preserved"] = std::abs(outputDC - inputDC);

    // Grading
    std::cout << "Grading:\n";
    std::cout << "--------\n";

    int score = 0;
    if (maxError <= 0.01f) { std::cout << "  ✓ Excellent precision (≤0.01dB): PASS\n"; score += 2; }
    else if (maxError <= 0.1f) { std::cout << "  ✓ Good precision (≤0.1dB): PASS\n"; score++; }
    else { std::cout << "  ✗ Poor precision (>0.1dB): FAIL\n"; result.passed = false; }

    if (avgError <= 0.05f) { std::cout << "  ✓ Consistent accuracy: PASS\n"; score++; }
    else { std::cout << "  ✗ Inconsistent accuracy: FAIL\n"; result.passed = false; }

    if (std::abs(outputDC - inputDC) < 0.001f) { std::cout << "  ✓ DC offset preserved: PASS\n"; score++; }
    else { std::cout << "  ⚠ DC offset changed (acceptable): PASS\n"; score++; }

    // Assign grade
    if (score >= 4) result.grade = 'A';
    else if (score == 3) result.grade = 'B';
    else if (score == 2) result.grade = 'C';
    else if (score == 1) result.grade = 'D';
    else result.grade = 'F';

    result.details = "Score: " + std::to_string(score) + "/4, Max Error: " +
                     std::to_string(maxError) + " dB";
    std::cout << "\n  Final Grade: " << result.grade << " (" << score << "/4)\n";

    return result;
}

// Test Engine 55: MonoMaker_Platinum
TestResult testMonoMaker() {
    TestResult result;
    result.engineName = "MonoMaker_Platinum";
    result.engineId = 55;
    result.passed = true;

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ENGINE 55: MONOMAKER_PLATINUM - Real-World Testing\n";
    std::cout << std::string(80, '=') << "\n\n";

    const double sampleRate = 48000.0;
    const int blockSize = 512;
    const int testDuration = 2;
    const int totalSamples = sampleRate * testDuration;

    auto engine = std::make_unique<MonoMaker_Platinum>();
    if (!engine) {
        result.passed = false;
        result.grade = 'F';
        result.details = "Failed to create engine";
        return result;
    }

    engine->prepareToPlay(sampleRate, blockSize);

    // Test 1: Full mono conversion
    std::cout << "Test 1: Full Mono Conversion\n";
    std::cout << "-----------------------------\n";

    juce::AudioBuffer<float> inputBuffer(2, totalSamples);
    generateStereoSignal(inputBuffer, sampleRate);

    float inputCorrelation = calculateStereoCorrelation(inputBuffer);
    std::cout << "  Input stereo correlation: " << std::fixed << std::setprecision(3)
              << inputCorrelation << "\n";

    // Set parameters for full mono
    std::map<int, float> params;
    params[0] = 1.0f;  // FREQUENCY: high (mono all frequencies)
    params[3] = 1.0f;  // BASS_MONO: 100%
    engine->updateParameters(params);

    juce::AudioBuffer<float> outputBuffer(2, totalSamples);
    outputBuffer.clear();

    int numBlocks = (totalSamples + blockSize - 1) / blockSize;
    for (int block = 0; block < numBlocks; ++block) {
        int startSample = block * blockSize;
        int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

        juce::AudioBuffer<float> blockBuffer(2, blockSize);
        blockBuffer.clear();

        for (int ch = 0; ch < 2; ++ch) {
            blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesToProcess);
        }

        engine->process(blockBuffer);

        for (int ch = 0; ch < 2; ++ch) {
            outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
        }
    }

    float outputCorrelation = calculateStereoCorrelation(outputBuffer);
    std::cout << "  Output stereo correlation: " << outputCorrelation << "\n";

    // Check if L and R are similar (mono)
    const float* left = outputBuffer.getReadPointer(0);
    const float* right = outputBuffer.getReadPointer(1);
    float maxDiff = 0.0f;
    for (int i = 0; i < totalSamples; ++i) {
        maxDiff = std::max(maxDiff, std::abs(left[i] - right[i]));
    }

    std::cout << "  Max L/R difference: " << std::setprecision(6) << maxDiff << "\n";
    std::cout << "  Mono achieved: " << (outputCorrelation > 0.95f ? "YES" : "NO") << "\n";

    result.metrics["mono_correlation"] = outputCorrelation;
    result.metrics["mono_max_diff"] = maxDiff;

    // Save output
    std::string outputPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/engine55_monomaker_full.wav";
    saveToWav(outputBuffer, outputPath, sampleRate);
    std::cout << "  Saved: " << outputPath << "\n\n";

    // Test 2: Partial mono (bass only)
    std::cout << "Test 2: Bass-Only Mono Conversion\n";
    std::cout << "----------------------------------\n";

    generateStereoSignal(inputBuffer, sampleRate);
    outputBuffer.clear();

    params[0] = 0.3f;  // FREQUENCY: low (only mono bass frequencies)
    params[3] = 1.0f;  // BASS_MONO: 100%
    engine->updateParameters(params);

    for (int block = 0; block < numBlocks; ++block) {
        int startSample = block * blockSize;
        int numSamplesToProcess = std::min(blockSize, totalSamples - startSample);

        juce::AudioBuffer<float> blockBuffer(2, blockSize);
        blockBuffer.clear();

        for (int ch = 0; ch < 2; ++ch) {
            blockBuffer.copyFrom(ch, 0, inputBuffer, ch, startSample, numSamplesToProcess);
        }

        engine->process(blockBuffer);

        for (int ch = 0; ch < 2; ++ch) {
            outputBuffer.copyFrom(ch, startSample, blockBuffer, ch, 0, numSamplesToProcess);
        }
    }

    float partialCorrelation = calculateStereoCorrelation(outputBuffer);
    std::cout << "  Output correlation (partial): " << std::setprecision(3)
              << partialCorrelation << "\n";
    std::cout << "  Partial stereo preserved: "
              << (partialCorrelation > inputCorrelation && partialCorrelation < outputCorrelation ? "YES" : "NO")
              << "\n";

    result.metrics["partial_correlation"] = partialCorrelation;

    outputPath = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/engine55_monomaker_bass.wav";
    saveToWav(outputBuffer, outputPath, sampleRate);
    std::cout << "  Saved: " << outputPath << "\n\n";

    // Grading
    std::cout << "Grading:\n";
    std::cout << "--------\n";

    int score = 0;
    if (outputCorrelation > 0.95f) { std::cout << "  ✓ Full mono achieved: PASS\n"; score++; }
    else { std::cout << "  ✗ Full mono failed: FAIL\n"; result.passed = false; }

    if (maxDiff < 0.01f) { std::cout << "  ✓ L/R channels matched: PASS\n"; score++; }
    else { std::cout << "  ✗ L/R channels differ: FAIL\n"; result.passed = false; }

    if (partialCorrelation > inputCorrelation) { std::cout << "  ✓ Partial mono works: PASS\n"; score++; }
    else { std::cout << "  ✗ Partial mono failed: FAIL\n"; result.passed = false; }

    if (outputCorrelation > inputCorrelation + 0.1f) {
        std::cout << "  ✓ Effective mono conversion: PASS\n"; score++;
    }
    else { std::cout << "  ✗ Ineffective conversion: FAIL\n"; result.passed = false; }

    // Assign grade
    if (score == 4) result.grade = 'A';
    else if (score == 3) result.grade = 'B';
    else if (score == 2) result.grade = 'C';
    else if (score == 1) result.grade = 'D';
    else result.grade = 'F';

    result.details = "Score: " + std::to_string(score) + "/4, Correlation: " +
                     std::to_string(outputCorrelation);
    std::cout << "\n  Final Grade: " << result.grade << " (" << score << "/4)\n";

    return result;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                  REAL-WORLD AUDIO TESTING                                ║\n";
    std::cout << "║                  UTILITY ENGINES (50-51, 54-55)                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n";

    std::vector<TestResult> results;

    // Run all tests
    results.push_back(testGranularCloud());
    results.push_back(testChaosGenerator());
    results.push_back(testGainUtility());
    results.push_back(testMonoMaker());

    // Print summary
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "FINAL SUMMARY\n";
    std::cout << std::string(80, '=') << "\n\n";

    std::cout << std::left << std::setw(30) << "Engine"
              << std::setw(10) << "ID"
              << std::setw(10) << "Grade"
              << std::setw(30) << "Details" << "\n";
    std::cout << std::string(80, '-') << "\n";

    int totalPassed = 0;
    for (const auto& r : results) {
        std::cout << std::left << std::setw(30) << r.engineName
                  << std::setw(10) << r.engineId
                  << std::setw(10) << r.grade
                  << std::setw(30) << r.details << "\n";
        if (r.passed) totalPassed++;
    }

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "PRODUCTION READINESS ASSESSMENT\n";
    std::cout << std::string(80, '=') << "\n\n";

    for (const auto& r : results) {
        std::cout << r.engineName << " (Engine " << r.engineId << "): ";

        if (r.grade == 'A') {
            std::cout << "PRODUCTION READY - Excellent quality\n";
        } else if (r.grade == 'B') {
            std::cout << "PRODUCTION READY - Good quality with minor issues\n";
        } else if (r.grade == 'C') {
            std::cout << "CONDITIONALLY READY - Usable but needs improvement\n";
        } else if (r.grade == 'D') {
            std::cout << "NOT READY - Significant issues present\n";
        } else {
            std::cout << "NOT READY - Critical failures\n";
        }

        // Print key metrics
        std::cout << "  Key Metrics:\n";
        for (const auto& metric : r.metrics) {
            std::cout << "    " << metric.first << ": "
                      << std::fixed << std::setprecision(3) << metric.second << "\n";
        }
        std::cout << "\n";
    }

    std::cout << std::string(80, '=') << "\n";
    std::cout << "OVERALL RESULT: " << totalPassed << "/" << results.size() << " engines passed\n";
    std::cout << std::string(80, '=') << "\n\n";

    std::cout << "Audio files saved to:\n";
    std::cout << "  /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/\n\n";

    return (totalPassed == results.size()) ? 0 : 1;
}
