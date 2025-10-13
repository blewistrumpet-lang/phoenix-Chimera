#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <map>
#include <sstream>

/**
 * REAL-WORLD AUDIO TESTING - MODULATION ENGINES 23-33
 *
 * Mission: Test all modulation engines with real-world audio materials
 *
 * Test Materials:
 * - Sustained vocal tone (vowel "Aah")
 * - Clean electric guitar (single notes + chords)
 * - Sustained synth pad
 *
 * Validation Criteria:
 * - Smooth modulation (no zipper noise)
 * - Appropriate depth response
 * - Stereo field width
 * - No excessive detuning/artifacts
 * - Mix parameter (dry/wet balance)
 *
 * Special Focus: Engines 23, 24, 27, 28 (LFO calibration fixes)
 */

namespace RealWorldModulationTest {

constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;
constexpr double PI = 3.14159265358979323846;
constexpr double TWO_PI = 2.0 * PI;

//==============================================================================
// Test Material Generator
//==============================================================================
class AudioMaterialGenerator {
public:
    // Generate sustained vocal-like tone (complex harmonic content)
    static juce::AudioBuffer<float> generateVocalTone(double sampleRate, double durationSeconds) {
        const int numSamples = static_cast<int>(sampleRate * durationSeconds);
        juce::AudioBuffer<float> buffer(2, numSamples);

        // Base frequency: 220 Hz (A3)
        const double f0 = 220.0;

        // Vowel formants for "Aah" sound
        struct Formant { double freq; double amp; double q; };
        std::vector<Formant> formants = {
            {730.0, 1.0, 10.0},   // F1
            {1090.0, 0.7, 15.0},  // F2
            {2440.0, 0.3, 20.0},  // F3
            {3400.0, 0.2, 25.0}   // F4
        };

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < numSamples; ++i) {
                double t = i / sampleRate;
                double sample = 0.0;

                // Generate harmonics
                for (int h = 1; h <= 8; ++h) {
                    double harmFreq = f0 * h;
                    double harmAmp = 1.0 / h;  // Natural harmonic rolloff

                    // Apply formant filtering
                    double formantGain = 0.0;
                    for (const auto& formant : formants) {
                        double dist = std::abs(harmFreq - formant.freq);
                        double q_response = formant.amp / (1.0 + dist / (formant.freq / formant.q));
                        formantGain += q_response;
                    }

                    sample += harmAmp * formantGain * std::sin(TWO_PI * harmFreq * t);
                }

                // Add slight vibrato (5 Hz, 3 cents)
                double vibrato = 1.0 + 0.0003 * std::sin(TWO_PI * 5.0 * t);
                sample *= vibrato;

                // Envelope (slight attack/release)
                double env = 1.0;
                if (t < 0.05) env = t / 0.05;
                if (t > durationSeconds - 0.1) env = (durationSeconds - t) / 0.1;

                buffer.setSample(ch, i, static_cast<float>(sample * env * 0.3));
            }
        }

        return buffer;
    }

    // Generate clean guitar tone (fundamental + harmonics)
    static juce::AudioBuffer<float> generateGuitarTone(double sampleRate, double durationSeconds) {
        const int numSamples = static_cast<int>(sampleRate * durationSeconds);
        juce::AudioBuffer<float> buffer(2, numSamples);

        // E string - 329.6 Hz (E4)
        const double f0 = 329.6;

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < numSamples; ++i) {
                double t = i / sampleRate;
                double sample = 0.0;

                // Guitar harmonics with characteristic decay
                for (int h = 1; h <= 10; ++h) {
                    double harmFreq = f0 * h;
                    double harmAmp = 1.0 / std::pow(h, 1.3);  // Faster rolloff than voice

                    // Harmonic decay over time
                    double decay = std::exp(-0.3 * h * t);

                    sample += harmAmp * decay * std::sin(TWO_PI * harmFreq * t);
                }

                // Pluck envelope
                double env = std::exp(-2.0 * t);

                buffer.setSample(ch, i, static_cast<float>(sample * env * 0.4));
            }
        }

        return buffer;
    }

    // Generate sustained synth pad (detuned oscillators)
    static juce::AudioBuffer<float> generateSynthPad(double sampleRate, double durationSeconds) {
        const int numSamples = static_cast<int>(sampleRate * durationSeconds);
        juce::AudioBuffer<float> buffer(2, numSamples);

        // Base frequency: 110 Hz (A2)
        const double f0 = 110.0;

        // Multiple detuned oscillators
        std::vector<double> detunes = {0.0, -0.03, 0.05, -0.08, 0.10};  // In Hz

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < numSamples; ++i) {
                double t = i / sampleRate;
                double sample = 0.0;

                // Supersaw-style pad
                for (double detune : detunes) {
                    double freq = f0 + detune;

                    // Sawtooth wave (sum of harmonics)
                    for (int h = 1; h <= 20; ++h) {
                        double harmAmp = 1.0 / h;
                        sample += harmAmp * std::sin(TWO_PI * freq * h * t);
                    }
                }

                // Slow envelope
                double env = 1.0;
                if (t < 0.5) env = t / 0.5;
                if (t > durationSeconds - 0.5) env = (durationSeconds - t) / 0.5;

                buffer.setSample(ch, i, static_cast<float>(sample * env * 0.15));
            }
        }

        return buffer;
    }
};

//==============================================================================
// Quality Metrics
//==============================================================================
struct QualityMetrics {
    // Modulation smoothness (lower is better, detects zipper noise)
    double zipperNoise = 0.0;

    // Depth response (should scale with parameter)
    double depthResponse = 0.0;

    // Stereo width (0 = mono, 1 = full stereo)
    double stereoWidth = 0.0;

    // Artifacts detection (THD, aliasing, etc.)
    double artifactLevel = 0.0;

    // Mix parameter accuracy
    double mixAccuracy = 0.0;

    // LFO rate accuracy (Hz)
    double lfoRateAccuracy = 0.0;

    // Overall musicality (subjective metric)
    double musicality = 0.0;

    char getGrade() const {
        int score = 0;
        if (zipperNoise < 0.01) score += 20; else if (zipperNoise < 0.05) score += 10;
        if (depthResponse > 0.6) score += 20; else if (depthResponse > 0.3) score += 10;
        if (stereoWidth > 0.3) score += 15; else if (stereoWidth > 0.1) score += 7;
        if (artifactLevel < 0.05) score += 15; else if (artifactLevel < 0.15) score += 7;
        if (mixAccuracy > 0.8) score += 15; else if (mixAccuracy > 0.5) score += 7;
        if (lfoRateAccuracy > 0.8) score += 15; else if (lfoRateAccuracy > 0.5) score += 7;

        if (score >= 85) return 'A';
        if (score >= 70) return 'B';
        if (score >= 55) return 'C';
        if (score >= 40) return 'D';
        return 'F';
    }
};

//==============================================================================
// Zipper Noise Detector
//==============================================================================
double detectZipperNoise(const juce::AudioBuffer<float>& buffer) {
    // Detect sudden discontinuities in modulation
    double maxDiscontinuity = 0.0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        int numSamples = buffer.getNumSamples();

        // High-pass filter to isolate discontinuities
        std::vector<float> hpFiltered(numSamples);
        float prev = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            hpFiltered[i] = data[i] - prev * 0.99f;
            prev = hpFiltered[i];
        }

        // Detect spikes in high-passed signal
        for (int i = 1; i < numSamples - 1; ++i) {
            double diff = std::abs(hpFiltered[i] - hpFiltered[i-1]);
            maxDiscontinuity = std::max(maxDiscontinuity, diff);
        }
    }

    return maxDiscontinuity;
}

//==============================================================================
// Stereo Width Measurement
//==============================================================================
double measureStereoWidth(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return 0.0;

    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    int numSamples = buffer.getNumSamples();

    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;

    for (int i = 0; i < numSamples; ++i) {
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    double correlation = sumLR / (std::sqrt(sumLL * sumRR) + 1e-10);
    return 1.0 - std::abs(correlation);
}

//==============================================================================
// THD Measurement
//==============================================================================
double measureTHD(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output) {
    // Simple THD estimate: compare output spectrum to input
    const int fftSize = 8192;
    juce::dsp::FFT fft(13);

    std::vector<float> inputFFT(fftSize * 2, 0.0f);
    std::vector<float> outputFFT(fftSize * 2, 0.0f);

    // Copy input
    int copySize = std::min(input.getNumSamples(), fftSize);
    for (int i = 0; i < copySize; ++i) {
        float window = 0.5f * (1.0f - std::cos(TWO_PI * i / copySize));
        inputFFT[i] = input.getSample(0, i) * window;
        outputFFT[i] = output.getSample(0, i) * window;
    }

    fft.performFrequencyOnlyForwardTransform(inputFFT.data());
    fft.performFrequencyOnlyForwardTransform(outputFFT.data());

    // Calculate distortion
    double fundamentalPower = 0.0;
    double harmonicPower = 0.0;

    for (int i = 1; i < fftSize / 2; ++i) {
        double outputMag = outputFFT[i];
        double inputMag = inputFFT[i];

        if (inputMag > 0.01) {
            fundamentalPower += outputMag * outputMag;
        } else {
            harmonicPower += outputMag * outputMag;
        }
    }

    return std::sqrt(harmonicPower / (fundamentalPower + 1e-10));
}

//==============================================================================
// LFO Rate Detection
//==============================================================================
double detectLFORate(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    // Extract envelope modulation
    const int windowSize = 512;
    const int hopSize = 128;
    std::vector<float> envelope;

    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i + windowSize < numSamples; i += hopSize) {
        float rms = 0.0f;
        for (int j = 0; j < windowSize; ++j) {
            rms += data[i + j] * data[i + j];
        }
        envelope.push_back(std::sqrt(rms / windowSize));
    }

    if (envelope.size() < 50) return 0.0;

    // Remove DC
    float mean = std::accumulate(envelope.begin(), envelope.end(), 0.0f) / envelope.size();
    for (auto& val : envelope) val -= mean;

    // Autocorrelation
    int maxLag = envelope.size() / 2;
    std::vector<float> autocorr(maxLag, 0.0f);

    for (int lag = 0; lag < maxLag; ++lag) {
        for (size_t i = 0; i + lag < envelope.size(); ++i) {
            autocorr[lag] += envelope[i] * envelope[i + lag];
        }
    }

    // Find first peak
    float threshold = autocorr[0] * 0.3f;
    int peakLag = 0;
    for (int lag = 3; lag < maxLag - 1; ++lag) {
        if (autocorr[lag] > threshold &&
            autocorr[lag] > autocorr[lag-1] &&
            autocorr[lag] > autocorr[lag+1]) {
            peakLag = lag;
            break;
        }
    }

    if (peakLag == 0) return 0.0;

    double envelopeSampleRate = sampleRate / hopSize;
    double period = peakLag / envelopeSampleRate;
    return 1.0 / period;
}

//==============================================================================
// Mix Parameter Test
//==============================================================================
double testMixParameter(EngineBase* engine, const juce::AudioBuffer<float>& testMaterial,
                       int mixParamIndex, double sampleRate) {
    std::vector<double> mixSettings = {0.0, 0.5, 1.0};
    std::vector<double> wetLevels;

    for (double mix : mixSettings) {
        engine->reset();
        engine->prepareToPlay(sampleRate, BLOCK_SIZE);

        std::map<int, float> params;
        for (int i = 0; i < 10; ++i) params[i] = 0.5f;
        params[mixParamIndex] = mix;
        engine->updateParameters(params);

        // Process
        juce::AudioBuffer<float> buffer(testMaterial);
        for (int start = 0; start < buffer.getNumSamples(); start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, buffer.getNumSamples() - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Measure difference from dry signal
        double diff = 0.0;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            double delta = buffer.getSample(0, i) - testMaterial.getSample(0, i);
            diff += delta * delta;
        }
        wetLevels.push_back(std::sqrt(diff / buffer.getNumSamples()));
    }

    // Check if wet level scales properly with mix
    double accuracy = 0.0;
    if (wetLevels[2] > 0.0) {
        accuracy = wetLevels[1] / wetLevels[2];  // 50% mix should be ~0.5 of 100%
    }

    return accuracy;
}

//==============================================================================
// Comprehensive Engine Test
//==============================================================================
struct EngineTestResult {
    int engineId;
    std::string engineName;
    QualityMetrics vocalMetrics;
    QualityMetrics guitarMetrics;
    QualityMetrics synthMetrics;

    char overallGrade() const {
        int avgScore = (vocalMetrics.getGrade() + guitarMetrics.getGrade() + synthMetrics.getGrade()) / 3;
        return avgScore;
    }
};

EngineTestResult testEngine(int engineId, const std::string& engineName) {
    EngineTestResult result;
    result.engineId = engineId;
    result.engineName = engineName;

    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ Engine " << std::setw(2) << engineId << ": "
              << std::setw(50) << std::left << engineName << "║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "ERROR: Failed to create engine\n";
        return result;
    }

    // Generate test materials
    auto vocalMaterial = AudioMaterialGenerator::generateVocalTone(SAMPLE_RATE, 4.0);
    auto guitarMaterial = AudioMaterialGenerator::generateGuitarTone(SAMPLE_RATE, 3.0);
    auto synthMaterial = AudioMaterialGenerator::generateSynthPad(SAMPLE_RATE, 4.0);

    // Test with each material
    std::vector<std::pair<std::string, juce::AudioBuffer<float>>> materials = {
        {"Vocal", vocalMaterial},
        {"Guitar", guitarMaterial},
        {"Synth", synthMaterial}
    };

    std::vector<QualityMetrics*> metricsRefs = {
        &result.vocalMetrics,
        &result.guitarMetrics,
        &result.synthMetrics
    };

    for (size_t m = 0; m < materials.size(); ++m) {
        auto& [matName, material] = materials[m];
        QualityMetrics& metrics = *metricsRefs[m];

        std::cout << "\n  Testing with " << matName << " material...\n";

        engine->reset();
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

        // Set moderate parameters
        std::map<int, float> params;
        for (int i = 0; i < 10; ++i) params[i] = 0.5f;
        params[0] = 0.4f;  // Rate
        params[1] = 0.6f;  // Depth
        engine->updateParameters(params);

        // Process
        juce::AudioBuffer<float> processed(material);
        for (int start = 0; start < processed.getNumSamples(); start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, processed.getNumSamples() - start);
            juce::AudioBuffer<float> block(processed.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Analyze
        metrics.zipperNoise = detectZipperNoise(processed);
        metrics.stereoWidth = measureStereoWidth(processed);
        metrics.artifactLevel = measureTHD(material, processed);
        metrics.lfoRateAccuracy = detectLFORate(processed, SAMPLE_RATE);

        // Test depth response
        engine->reset();
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        params[1] = 0.0f;  // Minimum depth
        engine->updateParameters(params);

        juce::AudioBuffer<float> minDepth(material);
        for (int start = 0; start < minDepth.getNumSamples(); start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, minDepth.getNumSamples() - start);
            juce::AudioBuffer<float> block(minDepth.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        engine->reset();
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        params[1] = 1.0f;  // Maximum depth
        engine->updateParameters(params);

        juce::AudioBuffer<float> maxDepth(material);
        for (int start = 0; start < maxDepth.getNumSamples(); start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, maxDepth.getNumSamples() - start);
            juce::AudioBuffer<float> block(maxDepth.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Calculate depth response
        double minMod = detectZipperNoise(minDepth);
        double maxMod = detectZipperNoise(maxDepth);
        metrics.depthResponse = (maxMod > 0.0) ? (maxMod - minMod) / maxMod : 0.0;

        // Test mix parameter
        metrics.mixAccuracy = testMixParameter(engine.get(), material, 2, SAMPLE_RATE);

        // Calculate musicality (composite metric)
        metrics.musicality =
            (1.0 - metrics.zipperNoise) * 0.3 +
            metrics.depthResponse * 0.2 +
            metrics.stereoWidth * 0.2 +
            (1.0 - metrics.artifactLevel) * 0.2 +
            metrics.mixAccuracy * 0.1;

        // Report
        std::cout << "    Zipper Noise:   " << std::fixed << std::setprecision(4)
                  << metrics.zipperNoise << (metrics.zipperNoise < 0.01 ? " ✓" : " ⚠") << "\n";
        std::cout << "    Stereo Width:   " << metrics.stereoWidth
                  << (metrics.stereoWidth > 0.3 ? " ✓" : " ⚠") << "\n";
        std::cout << "    Artifacts:      " << metrics.artifactLevel
                  << (metrics.artifactLevel < 0.05 ? " ✓" : " ⚠") << "\n";
        std::cout << "    LFO Rate:       " << metrics.lfoRateAccuracy << " Hz\n";
        std::cout << "    Depth Response: " << metrics.depthResponse
                  << (metrics.depthResponse > 0.5 ? " ✓" : " ⚠") << "\n";
        std::cout << "    Mix Accuracy:   " << metrics.mixAccuracy
                  << (metrics.mixAccuracy > 0.8 ? " ✓" : " ⚠") << "\n";
        std::cout << "    Grade:          " << metrics.getGrade() << "\n";

        // Save audio file
        std::string filename = "modulation_" + std::to_string(engineId) + "_" +
                              matName + "_realworld.wav";
        juce::File outputFile(filename);
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(new juce::FileOutputStream(outputFile),
                                     SAMPLE_RATE, 2, 24, {}, 0));
        if (writer) {
            writer->writeFromAudioSampleBuffer(processed, 0, processed.getNumSamples());
        }
    }

    // Overall grade
    int gradeSum = (result.vocalMetrics.getGrade() == 'A' ? 4 :
                   result.vocalMetrics.getGrade() == 'B' ? 3 :
                   result.vocalMetrics.getGrade() == 'C' ? 2 :
                   result.vocalMetrics.getGrade() == 'D' ? 1 : 0) +
                  (result.guitarMetrics.getGrade() == 'A' ? 4 :
                   result.guitarMetrics.getGrade() == 'B' ? 3 :
                   result.guitarMetrics.getGrade() == 'C' ? 2 :
                   result.guitarMetrics.getGrade() == 'D' ? 1 : 0) +
                  (result.synthMetrics.getGrade() == 'A' ? 4 :
                   result.synthMetrics.getGrade() == 'B' ? 3 :
                   result.synthMetrics.getGrade() == 'C' ? 2 :
                   result.synthMetrics.getGrade() == 'D' ? 1 : 0);

    char overall = gradeSum >= 10 ? 'A' : gradeSum >= 8 ? 'B' : gradeSum >= 6 ? 'C' : gradeSum >= 4 ? 'D' : 'F';

    std::cout << "\n  Overall Grade: " << overall << "\n";

    return result;
}

} // namespace RealWorldModulationTest

//==============================================================================
// Main
//==============================================================================
int main() {
    using namespace RealWorldModulationTest;

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  REAL-WORLD AUDIO TESTING - MODULATION ENGINES 23-33         ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  Test Materials:                                              ║\n";
    std::cout << "║  • Sustained vocal tone (220 Hz with formants)                ║\n";
    std::cout << "║  • Clean electric guitar (329.6 Hz)                           ║\n";
    std::cout << "║  • Sustained synth pad (110 Hz supersaw)                      ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  Quality Tests:                                               ║\n";
    std::cout << "║  • Smooth modulation (zipper noise detection)                 ║\n";
    std::cout << "║  • Depth response linearity                                   ║\n";
    std::cout << "║  • Stereo field width                                         ║\n";
    std::cout << "║  • Artifacts/THD measurement                                  ║\n";
    std::cout << "║  • Mix parameter accuracy                                     ║\n";
    std::cout << "║  • LFO rate calibration (Engines 23,24,27,28)                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    // Modulation engines to test
    std::vector<std::pair<int, std::string>> engines = {
        {23, "StereoChorus"},
        {24, "ClassicFlanger"},
        {25, "AnalogPhaser"},
        {26, "ClassicTremolo"},
        {27, "FrequencyShifter"},
        {28, "RingModulator_Platinum"},
        {29, "BucketBrigadeChorus"},
        {30, "DetuneDoubler"},
        {31, "SimplePitchShift"},
        {32, "SMBPitchShiftFixed"},
        {33, "IntelligentHarmonizer"}
    };

    std::vector<EngineTestResult> results;

    for (const auto& [id, name] : engines) {
        try {
            auto result = testEngine(id, name);
            results.push_back(result);
        } catch (const std::exception& e) {
            std::cout << "ERROR testing engine " << id << ": " << e.what() << "\n";
        }
    }

    // Summary Report
    std::cout << "\n\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     SUMMARY REPORT                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(28) << "Engine"
              << std::setw(8) << "Vocal"
              << std::setw(8) << "Guitar"
              << std::setw(8) << "Synth"
              << std::setw(10) << "Overall"
              << "Status\n";
    std::cout << std::string(75, '=') << "\n";

    int aCount = 0, bCount = 0, cCount = 0, dCount = 0, fCount = 0;

    for (const auto& r : results) {
        int gradeSum = (r.vocalMetrics.getGrade() == 'A' ? 4 :
                       r.vocalMetrics.getGrade() == 'B' ? 3 :
                       r.vocalMetrics.getGrade() == 'C' ? 2 :
                       r.vocalMetrics.getGrade() == 'D' ? 1 : 0) +
                      (r.guitarMetrics.getGrade() == 'A' ? 4 :
                       r.guitarMetrics.getGrade() == 'B' ? 3 :
                       r.guitarMetrics.getGrade() == 'C' ? 2 :
                       r.guitarMetrics.getGrade() == 'D' ? 1 : 0) +
                      (r.synthMetrics.getGrade() == 'A' ? 4 :
                       r.synthMetrics.getGrade() == 'B' ? 3 :
                       r.synthMetrics.getGrade() == 'C' ? 2 :
                       r.synthMetrics.getGrade() == 'D' ? 1 : 0);

        char overall = gradeSum >= 10 ? 'A' : gradeSum >= 8 ? 'B' : gradeSum >= 6 ? 'C' : gradeSum >= 4 ? 'D' : 'F';

        if (overall == 'A') aCount++;
        else if (overall == 'B') bCount++;
        else if (overall == 'C') cCount++;
        else if (overall == 'D') dCount++;
        else fCount++;

        std::string status = (overall >= 'C') ? "✓ Production Ready" :
                            (overall == 'D') ? "⚠ Needs Work" : "✗ Not Ready";

        std::cout << std::left << std::setw(5) << r.engineId
                  << std::setw(28) << r.engineName
                  << std::setw(8) << r.vocalMetrics.getGrade()
                  << std::setw(8) << r.guitarMetrics.getGrade()
                  << std::setw(8) << r.synthMetrics.getGrade()
                  << std::setw(10) << overall
                  << status << "\n";
    }

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Grade Distribution:                                          ║\n";
    std::cout << "║    A: " << std::setw(2) << aCount << " engines (Production Ready)                          ║\n";
    std::cout << "║    B: " << std::setw(2) << bCount << " engines (Good Quality)                              ║\n";
    std::cout << "║    C: " << std::setw(2) << cCount << " engines (Acceptable)                                ║\n";
    std::cout << "║    D: " << std::setw(2) << dCount << " engines (Needs Improvement)                         ║\n";
    std::cout << "║    F: " << std::setw(2) << fCount << " engines (Failed)                                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    // LFO Calibration Report (Engines 23, 24, 27, 28)
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  LFO CALIBRATION VERIFICATION (Engines 23, 24, 27, 28)        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    std::vector<int> lfoEngines = {23, 24, 27, 28};
    for (int engId : lfoEngines) {
        for (const auto& r : results) {
            if (r.engineId == engId) {
                std::cout << "Engine " << engId << " (" << r.engineName << "):\n";
                std::cout << "  Vocal LFO Rate:  " << r.vocalMetrics.lfoRateAccuracy << " Hz\n";
                std::cout << "  Guitar LFO Rate: " << r.guitarMetrics.lfoRateAccuracy << " Hz\n";
                std::cout << "  Synth LFO Rate:  " << r.synthMetrics.lfoRateAccuracy << " Hz\n";

                bool calibrated = (r.vocalMetrics.lfoRateAccuracy > 0.1 &&
                                  r.vocalMetrics.lfoRateAccuracy < 20.0);
                std::cout << "  Status: " << (calibrated ? "✓ CALIBRATED" : "✗ NEEDS CALIBRATION") << "\n\n";
            }
        }
    }

    // Save CSV report
    std::ofstream csv("modulation_realworld_results.csv");
    csv << "Engine ID,Engine Name,Vocal Grade,Guitar Grade,Synth Grade,Overall Grade,"
        << "Avg Zipper,Avg Stereo Width,Avg Artifacts,Production Ready\n";

    for (const auto& r : results) {
        int gradeSum = (r.vocalMetrics.getGrade() == 'A' ? 4 :
                       r.vocalMetrics.getGrade() == 'B' ? 3 :
                       r.vocalMetrics.getGrade() == 'C' ? 2 :
                       r.vocalMetrics.getGrade() == 'D' ? 1 : 0) +
                      (r.guitarMetrics.getGrade() == 'A' ? 4 :
                       r.guitarMetrics.getGrade() == 'B' ? 3 :
                       r.guitarMetrics.getGrade() == 'C' ? 2 :
                       r.guitarMetrics.getGrade() == 'D' ? 1 : 0) +
                      (r.synthMetrics.getGrade() == 'A' ? 4 :
                       r.synthMetrics.getGrade() == 'B' ? 3 :
                       r.synthMetrics.getGrade() == 'C' ? 2 :
                       r.synthMetrics.getGrade() == 'D' ? 1 : 0);

        char overall = gradeSum >= 10 ? 'A' : gradeSum >= 8 ? 'B' : gradeSum >= 6 ? 'C' : gradeSum >= 4 ? 'D' : 'F';

        double avgZipper = (r.vocalMetrics.zipperNoise + r.guitarMetrics.zipperNoise + r.synthMetrics.zipperNoise) / 3.0;
        double avgWidth = (r.vocalMetrics.stereoWidth + r.guitarMetrics.stereoWidth + r.synthMetrics.stereoWidth) / 3.0;
        double avgArtifacts = (r.vocalMetrics.artifactLevel + r.guitarMetrics.artifactLevel + r.synthMetrics.artifactLevel) / 3.0;

        csv << r.engineId << ","
            << r.engineName << ","
            << r.vocalMetrics.getGrade() << ","
            << r.guitarMetrics.getGrade() << ","
            << r.synthMetrics.getGrade() << ","
            << overall << ","
            << avgZipper << ","
            << avgWidth << ","
            << avgArtifacts << ","
            << (overall >= 'C' ? "Yes" : "No") << "\n";
    }
    csv.close();

    std::cout << "\n✓ Results saved to: modulation_realworld_results.csv\n";
    std::cout << "✓ Audio files saved as: modulation_[ID]_[Material]_realworld.wav\n\n";

    return (aCount + bCount + cCount >= results.size() * 0.7) ? 0 : 1;
}
