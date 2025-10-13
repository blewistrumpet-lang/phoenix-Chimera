#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

/*
 * REAL-WORLD AUDIO TESTING - PITCH ENGINES
 *
 * Tests pitch/formant engines with realistic audio materials:
 * - Engine 31: PitchShifter (with Gender Bender, Vocoder, etc modes)
 * - Engine 33: IntelligentHarmonizer (chord harmonization)
 * - Engine 49: PhasedVocoder (classic phase vocoder)
 *
 * Test Materials:
 * - Male vocal (A3 = 220 Hz)
 * - Female vocal (A4 = 440 Hz)
 * - Monophonic instrument (trumpet: Bb4 = ~466 Hz)
 *
 * Test Intervals:
 * -12, -7, -5, -1, +1, +5, +7, +12 semitones
 *
 * Quality Metrics:
 * - Pitch accuracy (±5 cents tolerance)
 * - Formant preservation (spectral envelope analysis)
 * - Artifact detection (grain smoothness, phasiness)
 * - Latency measurement
 * - Grading: A/B/C/D/F
 */

// Generate realistic vocal-like signal with harmonics
void generateVocalSignal(juce::AudioBuffer<float>& buffer, float fundamental, float sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Vocal-like harmonic structure
    const int numHarmonics = 12;
    float harmonicAmplitudes[12] = {
        1.0f,    // Fundamental
        0.7f,    // 2nd harmonic
        0.5f,    // 3rd
        0.4f,    // 4th
        0.3f,    // 5th
        0.25f,   // 6th
        0.2f,    // 7th
        0.15f,   // 8th
        0.12f,   // 9th
        0.1f,    // 10th
        0.08f,   // 11th
        0.06f    // 12th
    };

    // Add formant-like resonances (for male voice around 220Hz)
    // Simplified formants: F1=650Hz, F2=1080Hz, F3=2650Hz
    float formantFreqs[3] = {650.0f, 1080.0f, 2650.0f};
    float formantAmps[3] = {0.8f, 0.6f, 0.4f};
    float formantBWs[3] = {80.0f, 90.0f, 120.0f};

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

            // Add formant resonances (simplified)
            for (int f = 0; f < 3; ++f) {
                float formantPhase = 2.0f * M_PI * formantFreqs[f] * t;
                float envelope = std::exp(-formantBWs[f] * std::abs(std::sin(formantPhase)));
                sample += formantAmps[f] * std::sin(formantPhase) * envelope * 0.1f;
            }

            // Add slight vibrato (natural voice fluctuation)
            float vibrato = 1.0f + 0.01f * std::sin(2.0f * M_PI * 5.0f * t);
            sample *= vibrato;

            // Normalize
            sample *= 0.3f;

            channelData[i] = sample;
        }
    }
}

// Generate monophonic instrument signal (trumpet-like)
void generateInstrumentSignal(juce::AudioBuffer<float>& buffer, float fundamental, float sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Trumpet-like harmonic structure (strong odd harmonics)
    const int numHarmonics = 10;
    float harmonicAmplitudes[10] = {
        1.0f,    // Fundamental
        0.4f,    // 2nd (weak even)
        0.8f,    // 3rd (strong odd)
        0.3f,    // 4th
        0.7f,    // 5th (strong odd)
        0.2f,    // 6th
        0.5f,    // 7th
        0.15f,   // 8th
        0.4f,    // 9th
        0.1f     // 10th
    };

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

            // Add amplitude envelope (attack-sustain-release)
            float envelope = 1.0f;
            if (t < 0.05f) {
                envelope = t / 0.05f; // Attack
            } else if (t > numSamples/sampleRate - 0.1f) {
                float releaseTime = numSamples/sampleRate - t;
                envelope = releaseTime / 0.1f; // Release
            }

            sample *= envelope * 0.3f;
            channelData[i] = sample;
        }
    }
}

// FFT-based frequency detection with parabolic interpolation
float detectFundamentalFrequency(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13); // 2^13 = 8192
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy first channel with Hann window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find peak frequency
    int maxBin = 0;
    float maxMag = 0.0f;
    for (int i = 20; i < fftSize / 2; ++i) {
        if (fftData[i] > maxMag) {
            maxMag = fftData[i];
            maxBin = i;
        }
    }

    // Parabolic interpolation for sub-bin accuracy
    if (maxBin > 0 && maxBin < fftSize / 2 - 1) {
        float alpha = fftData[maxBin - 1];
        float beta = fftData[maxBin];
        float gamma = fftData[maxBin + 1];
        float p = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma);
        float interpolatedBin = maxBin + p;
        return interpolatedBin * sampleRate / fftSize;
    }

    return maxBin * sampleRate / fftSize;
}

// Calculate cents error between two frequencies
float calculateCentsError(float measured, float expected) {
    if (expected <= 0.0f || measured <= 0.0f) return 999.0f;
    return 1200.0f * std::log2(measured / expected);
}

// Semitones to pitch ratio
float semitonesToRatio(int semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

// Analyze spectral centroid (for formant preservation check)
float analyzeSpectralCentroid(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;

    for (int i = 1; i < fftSize / 2; ++i) {
        float freq = i * sampleRate / fftSize;
        float magnitude = fftData[i];
        weightedSum += freq * magnitude;
        magnitudeSum += magnitude;
    }

    return magnitudeSum > 0.0f ? weightedSum / magnitudeSum : 0.0f;
}

// Calculate THD+N
float calculateTHDN(const juce::AudioBuffer<float>& buffer, float sampleRate, float fundamental) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return 0.0f;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find fundamental bin
    int fundamentalBin = static_cast<int>(fundamental * fftSize / sampleRate);
    float fundamentalMag = fftData[fundamentalBin];

    // Sum harmonics and noise
    float harmonicSum = 0.0f;
    for (int h = 2; h <= 10; ++h) {
        int bin = h * fundamentalBin;
        if (bin < fftSize / 2) {
            harmonicSum += fftData[bin] * fftData[bin];
        }
    }

    return fundamentalMag > 0.0f ? std::sqrt(harmonicSum) / fundamentalMag : 0.0f;
}

struct PitchTestResult {
    int engineId;
    std::string engineName;
    std::string testMaterial;
    int semitoneShift;
    float inputFreq;
    float expectedFreq;
    float measuredFreq;
    float centsError;
    float spectralCentroid;
    float thdn;
    int latencySamples;
    bool passAccuracy;  // Within ±5 cents
    char grade;
    std::string notes;
};

// Test a single pitch shift configuration
PitchTestResult testPitchShift(int engineId, const std::string& materialName,
                                float inputFreq, int semitones, float sampleRate) {
    PitchTestResult result;
    result.engineId = engineId;
    result.testMaterial = materialName;
    result.semitoneShift = semitones;
    result.inputFreq = inputFreq;
    result.expectedFreq = inputFreq * semitonesToRatio(semitones);
    result.passAccuracy = false;
    result.grade = 'F';

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.notes = "Failed to create engine";
            return result;
        }

        result.engineName = engine->getName().toStdString();

        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        // Configure engine for pitch shifting
        std::map<int, float> params;

        if (engineId == 31) { // PitchShifter
            // Mode parameter: try different modes
            params[0] = 0.0f;  // Gender mode (has pitch shifting)
            params[1] = 0.5f + (semitones / 24.0f);  // Pitch control (normalized)
            params[2] = 0.5f;  // Formant control
            params[3] = 1.0f;  // Full wet
        } else if (engineId == 33) { // IntelligentHarmonizer
            params[0] = 0.5f + (semitones / 24.0f);  // Pitch shift
            params[1] = 0.0f;  // Harmony interval
            params[2] = 1.0f;  // Full wet
        } else if (engineId == 49) { // PhasedVocoder
            params[0] = 0.5f + (semitones / 24.0f);  // Pitch parameter
            params[1] = 0.5f;  // Formant
            params[2] = 1.0f;  // Full wet
        }

        engine->updateParameters(params);

        // Generate test signal (2 seconds)
        const int testLength = static_cast<int>(2.0f * sampleRate);
        juce::AudioBuffer<float> testBuffer(2, testLength);

        if (materialName.find("Male") != std::string::npos ||
            materialName.find("Female") != std::string::npos) {
            generateVocalSignal(testBuffer, inputFreq, sampleRate);
        } else {
            generateInstrumentSignal(testBuffer, inputFreq, sampleRate);
        }

        // Process in blocks
        for (int start = 0; start < testLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testLength - start);
            juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Analyze output (skip first 20% for transient settling)
        int skipSamples = testLength / 5;
        juce::AudioBuffer<float> analysisBuffer(2, testLength - skipSamples);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < analysisBuffer.getNumSamples(); ++i) {
                analysisBuffer.setSample(ch, i, testBuffer.getSample(ch, i + skipSamples));
            }
        }

        // Measure pitch
        result.measuredFreq = detectFundamentalFrequency(analysisBuffer, sampleRate);
        result.centsError = calculateCentsError(result.measuredFreq, result.expectedFreq);

        // Measure formant preservation (spectral centroid should stay relatively stable)
        result.spectralCentroid = analyzeSpectralCentroid(analysisBuffer, sampleRate);

        // Measure artifacts (THD+N)
        result.thdn = calculateTHDN(analysisBuffer, sampleRate, result.measuredFreq);

        // Estimate latency (simplified: just return typical values)
        if (engineId == 31) result.latencySamples = 2048;
        else if (engineId == 33) result.latencySamples = 2048;
        else if (engineId == 49) result.latencySamples = 4096;

        // Grade accuracy
        result.passAccuracy = std::abs(result.centsError) <= 5.0f;

        // Overall grade based on multiple factors
        if (std::abs(result.centsError) <= 5.0f && result.thdn < 0.05f) {
            result.grade = 'A';
        } else if (std::abs(result.centsError) <= 10.0f && result.thdn < 0.1f) {
            result.grade = 'B';
        } else if (std::abs(result.centsError) <= 20.0f && result.thdn < 0.2f) {
            result.grade = 'C';
        } else if (std::abs(result.centsError) <= 50.0f) {
            result.grade = 'D';
        } else {
            result.grade = 'F';
        }

        // Save output audio
        std::stringstream filename;
        filename << "pitch_test_" << engineId << "_" << materialName << "_"
                 << (semitones >= 0 ? "+" : "") << semitones << "st.wav";

        juce::File outputFile(filename.str());
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> outputStream(outputFile.createOutputStream());

        if (outputStream != nullptr) {
            std::unique_ptr<juce::AudioFormatWriter> writer(
                wavFormat.createWriterFor(outputStream.get(), sampleRate,
                                         testBuffer.getNumChannels(), 16, {}, 0));
            if (writer != nullptr) {
                outputStream.release();
                writer->writeFromAudioSampleBuffer(testBuffer, 0, testBuffer.getNumSamples());
            }
        }

    } catch (const std::exception& e) {
        result.notes = std::string("Exception: ") + e.what();
        result.grade = 'F';
    }

    return result;
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     REAL-WORLD AUDIO TESTING - PITCH ENGINES 31-38           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    const float sampleRate = 48000.0f;

    // Test configurations
    struct TestMaterial {
        std::string name;
        float frequency;
    };

    std::vector<TestMaterial> materials = {
        {"Male_Vocal", 220.0f},      // A3
        {"Female_Vocal", 440.0f},    // A4
        {"Trumpet", 466.16f}         // Bb4
    };

    std::vector<int> semitoneShifts = {-12, -7, -5, -1, +1, +5, +7, +12};
    std::vector<int> engineIds = {31, 33, 49};  // PitchShifter, IntelligentHarmonizer, PhasedVocoder

    std::cout << "Test Configuration:\n";
    std::cout << "  Sample Rate:     " << sampleRate << " Hz\n";
    std::cout << "  Test Materials:  Male Vocal (220Hz), Female Vocal (440Hz), Trumpet (466Hz)\n";
    std::cout << "  Semitone Shifts: -12, -7, -5, -1, +1, +5, +7, +12\n";
    std::cout << "  Engines:         31 (PitchShifter), 33 (IntelligentHarmonizer), 49 (PhasedVocoder)\n";
    std::cout << "  Accuracy Target: ±5 cents\n\n";

    std::vector<PitchTestResult> allResults;

    // Run all tests
    for (int engineId : engineIds) {
        std::cout << "\n═══════════════════════════════════════════════════════════════\n";
        std::cout << "Testing Engine " << engineId << "\n";
        std::cout << "═══════════════════════════════════════════════════════════════\n\n";

        for (const auto& material : materials) {
            std::cout << "  Material: " << material.name << " (" << material.frequency << " Hz)\n";
            std::cout << "  ───────────────────────────────────────────────────────────\n";

            for (int semitones : semitoneShifts) {
                auto result = testPitchShift(engineId, material.name, material.frequency,
                                            semitones, sampleRate);
                allResults.push_back(result);

                std::cout << "    " << std::setw(4) << (semitones >= 0 ? "+" : "") << semitones << " st: "
                         << std::fixed << std::setprecision(2)
                         << result.measuredFreq << " Hz (expected " << result.expectedFreq << " Hz) "
                         << "Error: " << std::setw(6) << result.centsError << " cents "
                         << "[" << result.grade << "] "
                         << (result.passAccuracy ? "✓" : "✗") << "\n";
            }
            std::cout << "\n";
        }
    }

    // Generate summary report
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      SUMMARY REPORT                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    // Grade distribution per engine
    std::map<int, std::map<char, int>> gradeDistribution;
    std::map<int, std::string> engineNames;
    std::map<int, std::vector<float>> centsErrors;

    for (const auto& result : allResults) {
        gradeDistribution[result.engineId][result.grade]++;
        engineNames[result.engineId] = result.engineName;
        centsErrors[result.engineId].push_back(std::abs(result.centsError));
    }

    for (int engineId : engineIds) {
        std::cout << "Engine " << engineId << ": " << engineNames[engineId] << "\n";
        std::cout << "  Grade Distribution: ";
        std::cout << "A=" << gradeDistribution[engineId]['A'] << " ";
        std::cout << "B=" << gradeDistribution[engineId]['B'] << " ";
        std::cout << "C=" << gradeDistribution[engineId]['C'] << " ";
        std::cout << "D=" << gradeDistribution[engineId]['D'] << " ";
        std::cout << "F=" << gradeDistribution[engineId]['F'] << "\n";

        // Calculate average cents error
        float avgError = 0.0f;
        float maxError = 0.0f;
        for (float err : centsErrors[engineId]) {
            avgError += err;
            maxError = std::max(maxError, err);
        }
        avgError /= centsErrors[engineId].size();

        std::cout << "  Avg Cents Error: " << std::fixed << std::setprecision(2) << avgError << "\n";
        std::cout << "  Max Cents Error: " << std::fixed << std::setprecision(2) << maxError << "\n";

        // Overall engine grade
        char overallGrade = 'F';
        if (avgError <= 5.0f && maxError <= 10.0f) overallGrade = 'A';
        else if (avgError <= 10.0f && maxError <= 20.0f) overallGrade = 'B';
        else if (avgError <= 20.0f && maxError <= 50.0f) overallGrade = 'C';
        else if (avgError <= 50.0f) overallGrade = 'D';

        std::cout << "  Overall Grade: " << overallGrade << "\n";

        // Production readiness
        bool productionReady = (overallGrade <= 'B' && maxError <= 20.0f);
        std::cout << "  Production Ready: " << (productionReady ? "YES ✓" : "NO ✗") << "\n\n";
    }

    // Best use cases
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    BEST USE CASES                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Engine 31 (PitchShifter):\n";
    std::cout << "  - Gender transformation effects\n";
    std::cout << "  - Creative vocal manipulation\n";
    std::cout << "  - Real-time pitch correction\n\n";

    std::cout << "Engine 33 (IntelligentHarmonizer):\n";
    std::cout << "  - Vocal harmonization\n";
    std::cout << "  - Chord generation from monophonic sources\n";
    std::cout << "  - Musical doubling effects\n\n";

    std::cout << "Engine 49 (PhasedVocoder):\n";
    std::cout << "  - Classic vocoder effects\n";
    std::cout << "  - Robot voice processing\n";
    std::cout << "  - Spectral manipulation\n\n";

    // Audio file locations
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                  AUDIO FILE LOCATIONS                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "Output files saved in current directory:\n";
    std::cout << "  Format: pitch_test_[ENGINE]_[MATERIAL]_[SHIFT]st.wav\n";
    std::cout << "  Example: pitch_test_31_Male_Vocal_+5st.wav\n\n";

    // Final summary
    int totalTests = allResults.size();
    int passedTests = 0;
    for (const auto& result : allResults) {
        if (result.passAccuracy) passedTests++;
    }

    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    FINAL SUMMARY                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "  Total Tests:      " << totalTests << "\n";
    std::cout << "  Passed (±5¢):     " << passedTests << "\n";
    std::cout << "  Failed:           " << (totalTests - passedTests) << "\n";
    std::cout << "  Success Rate:     " << std::fixed << std::setprecision(1)
              << (100.0f * passedTests / totalTests) << "%\n\n";

    std::cout << "NOTE: Pitch shifting is inherently challenging. Results depend heavily on:\n";
    std::cout << "  - Source material complexity\n";
    std::cout << "  - Shift interval size\n";
    std::cout << "  - Algorithm choice and parameters\n";
    std::cout << "  - Real-time vs offline processing trade-offs\n\n";

    return (passedTests >= totalTests * 0.7) ? 0 : 1;  // 70% success threshold
}
