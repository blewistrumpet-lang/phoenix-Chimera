#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <complex>

// Pitch Shifting and Time-based Effects Test Suite
namespace PitchTimeTests {

struct PitchMetrics {
    // Pitch accuracy
    std::vector<float> inputFreqs;
    std::vector<float> outputFreqs;
    std::vector<float> expectedFreqs;
    std::vector<float> pitchErrors;      // In cents (1/100 semitone)
    float maxPitchError;                 // Maximum error in cents
    float avgPitchError;                 // Average error in cents

    // Formant analysis
    std::vector<float> inputFormants;
    std::vector<float> outputFormants;
    bool formantsPreserved;

    // Artifact analysis
    float thd;                           // Total harmonic distortion
    float artifactLevel;                 // Spectral artifacts in dB
    bool hasPreEcho;
    bool hasPostEcho;
    float transientSmearing;             // Transient degradation
    bool hasChorus;                      // Unwanted chorus/phasing
    bool hasAliasing;                    // Aliasing artifacts

    // Latency
    int latencySamples;
    float latencyMs;
    bool constantLatency;

    // Algorithm identification
    std::string algorithmType;           // PSOLA, Phase Vocoder, Hybrid, etc.
    int estimatedWindowSize;
    float estimatedOverlap;

    // Quality score
    float qualityScore;                  // 0-100
    std::string qualityRating;           // Poor/Fair/Good/Excellent/Professional
};

struct DelayMetrics {
    // Timing accuracy
    std::vector<float> setDelayTimes;    // In milliseconds
    std::vector<float> measuredDelayTimes;
    std::vector<float> timingErrors;
    float maxTimingError;
    float avgTimingError;

    // Feedback quality
    float feedbackTHD;
    bool feedbackStable;
    float maxStableFeedback;             // Maximum feedback before instability

    // Modulation characteristics
    bool hasModulation;
    float modulationRate;                // Hz
    float modulationDepth;               // Percentage
    float wowFlutter;                    // Wow & flutter in %
    std::string modulationWaveform;      // sine, triangle, random, etc.

    // Character
    bool hasSaturation;
    bool hasFiltering;
    float toneCharacter;                 // -1 (dark) to +1 (bright)
    std::string emulationType;           // Tape, BBD, Digital, etc.

    // Quality
    float qualityScore;
    std::string qualityRating;
};

//==============================================================================
// FFT-based Frequency Detection
//==============================================================================
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
    for (int i = 20; i < fftSize / 2; ++i) { // Skip DC and very low frequencies
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

//==============================================================================
// Autocorrelation-based Pitch Detection (more accurate for complex signals)
//==============================================================================
float detectPitchAutocorrelation(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);

    // Calculate autocorrelation
    std::vector<float> autocorr(numSamples / 2);
    for (int lag = 0; lag < numSamples / 2; ++lag) {
        float sum = 0.0f;
        for (int i = 0; i < numSamples - lag; ++i) {
            sum += data[i] * data[i + lag];
        }
        autocorr[lag] = sum;
    }

    // Find first peak after zero crossing
    int minLag = static_cast<int>(sampleRate / 2000.0f); // Min 2000 Hz
    int maxLag = static_cast<int>(sampleRate / 50.0f);   // Max 50 Hz

    int peakLag = 0;
    float maxCorr = 0.0f;
    for (int lag = minLag; lag < maxLag && lag < autocorr.size(); ++lag) {
        if (autocorr[lag] > maxCorr) {
            maxCorr = autocorr[lag];
            peakLag = lag;
        }
    }

    if (peakLag == 0) return 0.0f;
    return sampleRate / peakLag;
}

//==============================================================================
// Formant Detection (finds spectral peaks)
//==============================================================================
std::vector<float> detectFormants(const juce::AudioBuffer<float>& buffer, float sampleRate, int numFormants = 3) {
    const int fftSize = 8192;
    std::vector<float> formants;
    if (buffer.getNumSamples() < fftSize) return formants;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    // Copy with window
    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Find spectral peaks
    std::vector<std::pair<int, float>> peaks;
    for (int i = 50; i < fftSize / 2 - 1; ++i) { // Start at ~300 Hz
        if (fftData[i] > fftData[i-1] && fftData[i] > fftData[i+1]) {
            peaks.push_back({i, fftData[i]});
        }
    }

    // Sort by magnitude
    std::sort(peaks.begin(), peaks.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Take top formants
    for (int i = 0; i < std::min(numFormants, (int)peaks.size()); ++i) {
        formants.push_back(peaks[i].first * sampleRate / fftSize);
    }

    return formants;
}

//==============================================================================
// Measure THD (Total Harmonic Distortion)
//==============================================================================
float measureTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate) {
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

    // Find fundamental
    int fundamentalBin = static_cast<int>(fundamentalFreq * fftSize / sampleRate);
    float fundamentalMag = fftData[fundamentalBin];

    // Sum harmonics
    float harmonicsSumSquared = 0.0f;
    for (int h = 2; h <= 10; ++h) {
        int harmonicBin = fundamentalBin * h;
        if (harmonicBin < fftSize / 2) {
            float harmonicMag = fftData[harmonicBin];
            harmonicsSumSquared += harmonicMag * harmonicMag;
        }
    }

    if (fundamentalMag < 1e-10f) return 0.0f;
    return (std::sqrt(harmonicsSumSquared) / fundamentalMag) * 100.0f;
}

//==============================================================================
// Measure Latency (using impulse response)
//==============================================================================
int measureLatency(EngineBase* engine, float sampleRate, int blockSize) {
    const int maxLatency = static_cast<int>(sampleRate * 0.5); // Max 500ms
    juce::AudioBuffer<float> buffer(2, maxLatency);
    buffer.clear();

    // Create impulse at start
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    // Process in blocks
    for (int start = 0; start < maxLatency; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, maxLatency - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Find first sample above threshold
    const float threshold = 0.01f;
    for (int i = 0; i < maxLatency; ++i) {
        if (std::abs(buffer.getSample(0, i)) > threshold) {
            return i;
        }
    }

    return 0;
}

//==============================================================================
// Detect Aliasing
//==============================================================================
bool detectAliasing(const juce::AudioBuffer<float>& buffer, float sampleRate) {
    const int fftSize = 8192;
    if (buffer.getNumSamples() < fftSize) return false;

    juce::dsp::FFT fft(13);
    std::vector<float> fftData(fftSize * 2, 0.0f);

    const float* inputData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / fftSize));
        fftData[i] = inputData[i] * window;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Check for energy above Nyquist/2 that shouldn't be there
    float highFreqEnergy = 0.0f;
    float totalEnergy = 0.0f;
    int nyquistBin = fftSize / 2;

    for (int i = 0; i < nyquistBin; ++i) {
        totalEnergy += fftData[i] * fftData[i];
        if (i > nyquistBin * 0.75f) { // Check top quarter of spectrum
            highFreqEnergy += fftData[i] * fftData[i];
        }
    }

    // If more than 10% of energy is in top quarter, likely aliasing
    return (highFreqEnergy / totalEnergy) > 0.1f;
}

//==============================================================================
// Measure Delay Time Accuracy
//==============================================================================
float measureDelayTime(EngineBase* engine, float sampleRate, int blockSize) {
    const int bufferSize = static_cast<int>(sampleRate * 2.0); // 2 seconds
    juce::AudioBuffer<float> buffer(2, bufferSize);
    buffer.clear();

    // Create impulse
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    // Process
    for (int start = 0; start < bufferSize; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, bufferSize - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Find first echo (skip direct signal in first 10ms)
    int skipSamples = static_cast<int>(sampleRate * 0.01);
    const float threshold = 0.1f;

    for (int i = skipSamples; i < bufferSize; ++i) {
        if (std::abs(buffer.getSample(0, i)) > threshold) {
            return (i * 1000.0f) / sampleRate; // Return in milliseconds
        }
    }

    return 0.0f;
}

//==============================================================================
// Test Pitch Shifter
//==============================================================================
PitchMetrics testPitchShifter(int engineId, float sampleRate = 48000.0f) {
    PitchMetrics metrics = {};

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) return metrics;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    std::cout << "  Testing pitch shifter engine " << engineId << ": " << engine->getName() << "\n";

    // Test frequencies
    std::vector<float> testFreqs = {100.0f, 220.0f, 440.0f, 880.0f, 1760.0f, 3520.0f};

    // Semitone shifts to test
    std::vector<int> semitoneShifts = {-12, -7, -5, -2, 0, +2, +5, +7, +12};

    // Measure latency first
    metrics.latencySamples = measureLatency(engine.get(), sampleRate, blockSize);
    metrics.latencyMs = (metrics.latencySamples * 1000.0f) / sampleRate;

    std::cout << "    Latency: " << metrics.latencySamples << " samples ("
              << std::fixed << std::setprecision(2) << metrics.latencyMs << " ms)\n";

    float totalError = 0.0f;
    int errorCount = 0;

    // Test each combination of frequency and shift
    for (float inputFreq : testFreqs) {
        for (int semitones : semitoneShifts) {
            // Set pitch shift parameter (assuming param 0 is pitch shift)
            std::map<int, float> params;

            // Map semitones to 0-1 range (assuming -12 to +12 semitone range)
            float normalizedShift = (semitones + 12.0f) / 24.0f;
            params[0] = normalizedShift;

            if (engine->getNumParameters() > 1) {
                params[1] = 1.0f; // Mix at 100%
            }

            engine->reset();
            engine->updateParameters(params);

            // Generate test signal
            const int testLength = 16384;
            juce::AudioBuffer<float> testBuffer(2, testLength);

            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < testLength; ++i) {
                    float phase = 2.0f * M_PI * inputFreq * i / sampleRate;
                    testBuffer.setSample(ch, i, 0.5f * std::sin(phase));
                }
            }

            // Process
            for (int start = 0; start < testLength; start += blockSize) {
                int samplesThisBlock = std::min(blockSize, testLength - start);
                juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
                engine->process(block);
            }

            // Skip latency samples
            juce::AudioBuffer<float> analysisBuffer(2, testLength - metrics.latencySamples);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < analysisBuffer.getNumSamples(); ++i) {
                    analysisBuffer.setSample(ch, i, testBuffer.getSample(ch, i + metrics.latencySamples));
                }
            }

            // Detect output frequency
            float outputFreq = detectFundamentalFrequency(analysisBuffer, sampleRate);
            float expectedFreq = inputFreq * std::pow(2.0f, semitones / 12.0f);

            // Calculate error in cents (1 cent = 1/100 semitone)
            float cents = 1200.0f * std::log2(outputFreq / expectedFreq);

            metrics.inputFreqs.push_back(inputFreq);
            metrics.outputFreqs.push_back(outputFreq);
            metrics.expectedFreqs.push_back(expectedFreq);
            metrics.pitchErrors.push_back(cents);

            totalError += std::abs(cents);
            errorCount++;

            // Measure THD for this shift
            if (semitones == 0) { // Only at unity pitch for fair comparison
                metrics.thd = measureTHD(analysisBuffer, expectedFreq, sampleRate);
            }

            // Check for aliasing
            if (!metrics.hasAliasing) {
                metrics.hasAliasing = detectAliasing(analysisBuffer, sampleRate);
            }
        }
    }

    // Calculate statistics
    if (!metrics.pitchErrors.empty()) {
        metrics.maxPitchError = *std::max_element(metrics.pitchErrors.begin(), metrics.pitchErrors.end(),
                                                   [](float a, float b) { return std::abs(a) < std::abs(b); });
        metrics.avgPitchError = totalError / errorCount;
    }

    // Formant test (using 440 Hz as test signal)
    engine->reset();
    std::map<int, float> unityParams;
    unityParams[0] = 0.5f; // Unity pitch
    if (engine->getNumParameters() > 1) unityParams[1] = 1.0f;
    engine->updateParameters(unityParams);

    juce::AudioBuffer<float> formantTest(2, 16384);
    // Generate signal with formant-like structure
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 16384; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            float sig = std::sin(phase);
            // Add formants at 800, 1200, 2400 Hz
            sig += 0.5f * std::sin(2.0f * M_PI * 800.0f * i / sampleRate);
            sig += 0.3f * std::sin(2.0f * M_PI * 1200.0f * i / sampleRate);
            sig += 0.2f * std::sin(2.0f * M_PI * 2400.0f * i / sampleRate);
            formantTest.setSample(ch, i, sig * 0.3f);
        }
    }

    juce::AudioBuffer<float> inputCopy = formantTest;

    for (int start = 0; start < 16384; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, 16384 - start);
        juce::AudioBuffer<float> block(formantTest.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    metrics.inputFormants = detectFormants(inputCopy, sampleRate);
    metrics.outputFormants = detectFormants(formantTest, sampleRate);

    // Check if formants are preserved (within 10%)
    metrics.formantsPreserved = true;
    if (metrics.inputFormants.size() == metrics.outputFormants.size()) {
        for (size_t i = 0; i < metrics.inputFormants.size(); ++i) {
            float diff = std::abs(metrics.inputFormants[i] - metrics.outputFormants[i]);
            if (diff > metrics.inputFormants[i] * 0.1f) {
                metrics.formantsPreserved = false;
                break;
            }
        }
    }

    // Algorithm identification
    if (metrics.latencySamples < 512) {
        metrics.algorithmType = "Time-domain (PSOLA/Granular)";
    } else if (metrics.latencySamples > 2048) {
        metrics.algorithmType = "Frequency-domain (Phase Vocoder)";
    } else {
        metrics.algorithmType = "Hybrid";
    }

    // Quality scoring
    metrics.qualityScore = 100.0f;
    if (metrics.avgPitchError > 1.0f) metrics.qualityScore -= 20.0f;
    if (metrics.avgPitchError > 5.0f) metrics.qualityScore -= 30.0f;
    if (metrics.thd > 1.0f) metrics.qualityScore -= 15.0f;
    if (metrics.thd > 5.0f) metrics.qualityScore -= 25.0f;
    if (metrics.hasAliasing) metrics.qualityScore -= 20.0f;
    if (!metrics.formantsPreserved) metrics.qualityScore -= 10.0f;

    if (metrics.qualityScore >= 90.0f) metrics.qualityRating = "Professional";
    else if (metrics.qualityScore >= 75.0f) metrics.qualityRating = "Excellent";
    else if (metrics.qualityScore >= 60.0f) metrics.qualityRating = "Good";
    else if (metrics.qualityScore >= 40.0f) metrics.qualityRating = "Fair";
    else metrics.qualityRating = "Poor";

    return metrics;
}

//==============================================================================
// Test Delay/Time-based Effect
//==============================================================================
DelayMetrics testDelayEngine(int engineId, float sampleRate = 48000.0f) {
    DelayMetrics metrics = {};

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) return metrics;

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    std::cout << "  Testing delay engine " << engineId << ": " << engine->getName() << "\n";

    // Test delay times
    std::vector<float> testDelays = {50.0f, 100.0f, 250.0f, 500.0f, 1000.0f};

    for (float targetDelay : testDelays) {
        // Set delay time parameter
        std::map<int, float> params;

        // Map delay time to parameter range (assume 0-1 = 0-2000ms)
        params[0] = targetDelay / 2000.0f;
        if (engine->getNumParameters() > 1) params[1] = 0.0f; // Feedback = 0
        if (engine->getNumParameters() > 2) params[2] = 1.0f; // Mix = 100%

        engine->reset();
        engine->updateParameters(params);

        float measuredDelay = measureDelayTime(engine.get(), sampleRate, blockSize);
        float error = std::abs(measuredDelay - targetDelay);

        metrics.setDelayTimes.push_back(targetDelay);
        metrics.measuredDelayTimes.push_back(measuredDelay);
        metrics.timingErrors.push_back(error);

        std::cout << "    Target: " << std::fixed << std::setprecision(1) << targetDelay
                  << " ms, Measured: " << measuredDelay << " ms, Error: " << error << " ms\n";
    }

    // Calculate statistics
    if (!metrics.timingErrors.empty()) {
        metrics.maxTimingError = *std::max_element(metrics.timingErrors.begin(), metrics.timingErrors.end());
        float sum = std::accumulate(metrics.timingErrors.begin(), metrics.timingErrors.end(), 0.0f);
        metrics.avgTimingError = sum / metrics.timingErrors.size();
    }

    // Test feedback stability
    std::map<int, float> feedbackParams;
    feedbackParams[0] = 0.25f; // 250ms delay
    metrics.feedbackStable = true;

    for (float feedback = 0.5f; feedback <= 0.95f; feedback += 0.1f) {
        if (engine->getNumParameters() > 1) feedbackParams[1] = feedback;
        if (engine->getNumParameters() > 2) feedbackParams[2] = 1.0f;

        engine->reset();
        engine->updateParameters(feedbackParams);

        // Generate test signal
        juce::AudioBuffer<float> testBuffer(2, static_cast<int>(sampleRate * 2.0));
        testBuffer.clear();
        testBuffer.setSample(0, 0, 1.0f);
        testBuffer.setSample(1, 0, 1.0f);

        // Process
        for (int start = 0; start < testBuffer.getNumSamples(); start += blockSize) {
            int samplesThisBlock = std::min(blockSize, testBuffer.getNumSamples() - start);
            juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);

            // Check for instability
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < samplesThisBlock; ++i) {
                    float sample = testBuffer.getSample(ch, start + i);
                    if (std::abs(sample) > 10.0f || std::isnan(sample) || std::isinf(sample)) {
                        metrics.feedbackStable = false;
                        metrics.maxStableFeedback = feedback - 0.1f;
                        goto feedback_done;
                    }
                }
            }
        }
    }
    feedback_done:

    if (metrics.feedbackStable) {
        metrics.maxStableFeedback = 0.95f;
    }

    // Determine delay type from name
    std::string name = engine->getName().toStdString();
    if (name.find("Tape") != std::string::npos) {
        metrics.emulationType = "Tape Echo";
        metrics.hasModulation = true;
        metrics.hasSaturation = true;
    } else if (name.find("BBD") != std::string::npos || name.find("Bucket") != std::string::npos) {
        metrics.emulationType = "Bucket Brigade (BBD)";
        metrics.hasModulation = true;
        metrics.hasFiltering = true;
    } else if (name.find("Digital") != std::string::npos) {
        metrics.emulationType = "Digital Delay";
        metrics.hasModulation = false;
        metrics.hasSaturation = false;
    } else if (name.find("Magnetic") != std::string::npos || name.find("Drum") != std::string::npos) {
        metrics.emulationType = "Magnetic Drum";
        metrics.hasModulation = true;
        metrics.hasSaturation = true;
    } else {
        metrics.emulationType = "Unknown";
    }

    // Quality scoring
    metrics.qualityScore = 100.0f;
    if (metrics.avgTimingError > 1.0f) metrics.qualityScore -= 10.0f;
    if (metrics.avgTimingError > 5.0f) metrics.qualityScore -= 20.0f;
    if (!metrics.feedbackStable) metrics.qualityScore -= 30.0f;

    if (metrics.qualityScore >= 90.0f) metrics.qualityRating = "Excellent";
    else if (metrics.qualityScore >= 75.0f) metrics.qualityRating = "Good";
    else if (metrics.qualityScore >= 60.0f) metrics.qualityRating = "Fair";
    else metrics.qualityRating = "Poor";

    return metrics;
}

//==============================================================================
// Print Pitch Metrics
//==============================================================================
void printPitchMetrics(int engineId, const std::string& name, const PitchMetrics& m) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": " << std::setw(45) << std::left << name << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "PITCH ACCURACY:\n";
    std::cout << "  Average Error:   " << std::fixed << std::setprecision(2) << m.avgPitchError << " cents";
    if (m.avgPitchError < 1.0f) std::cout << " ✓ EXCELLENT";
    else if (m.avgPitchError < 5.0f) std::cout << " ✓ GOOD";
    else if (m.avgPitchError < 10.0f) std::cout << " ⚠ FAIR";
    else std::cout << " ✗ POOR";
    std::cout << "\n";

    std::cout << "  Maximum Error:   " << std::fixed << std::setprecision(2) << m.maxPitchError << " cents\n";
    std::cout << "  Target:          ±1.0 cents (professional standard)\n";

    std::cout << "\nFORMANT PRESERVATION:\n";
    std::cout << "  Preserved:       " << (m.formantsPreserved ? "✓ YES" : "✗ NO") << "\n";
    if (!m.inputFormants.empty() && !m.outputFormants.empty()) {
        std::cout << "  Input Formants:  ";
        for (float f : m.inputFormants) std::cout << static_cast<int>(f) << " Hz  ";
        std::cout << "\n  Output Formants: ";
        for (float f : m.outputFormants) std::cout << static_cast<int>(f) << " Hz  ";
        std::cout << "\n";
    }

    std::cout << "\nARTIFACTS:\n";
    std::cout << "  THD:             " << std::fixed << std::setprecision(3) << m.thd << "%";
    if (m.thd < 0.5f) std::cout << " ✓ EXCELLENT";
    else if (m.thd < 1.0f) std::cout << " ✓ GOOD";
    else if (m.thd < 5.0f) std::cout << " ⚠ FAIR";
    else std::cout << " ✗ POOR";
    std::cout << "\n";

    std::cout << "  Aliasing:        " << (m.hasAliasing ? "⚠ DETECTED" : "✓ None") << "\n";

    std::cout << "\nLATENCY:\n";
    std::cout << "  Samples:         " << m.latencySamples << "\n";
    std::cout << "  Milliseconds:    " << std::fixed << std::setprecision(2) << m.latencyMs << " ms\n";

    std::cout << "\nALGORITHM:\n";
    std::cout << "  Type:            " << m.algorithmType << "\n";

    std::cout << "\nOVERALL QUALITY:\n";
    std::cout << "  Score:           " << std::fixed << std::setprecision(1) << m.qualityScore << "/100\n";
    std::cout << "  Rating:          " << m.qualityRating << "\n\n";
}

//==============================================================================
// Print Delay Metrics
//==============================================================================
void printDelayMetrics(int engineId, const std::string& name, const DelayMetrics& m) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": " << std::setw(45) << std::left << name << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "TIMING ACCURACY:\n";
    std::cout << "  Average Error:   " << std::fixed << std::setprecision(2) << m.avgTimingError << " ms";
    if (m.avgTimingError < 1.0f) std::cout << " ✓ EXCELLENT";
    else if (m.avgTimingError < 5.0f) std::cout << " ✓ GOOD";
    else std::cout << " ⚠ FAIR";
    std::cout << "\n";

    std::cout << "  Maximum Error:   " << std::fixed << std::setprecision(2) << m.maxTimingError << " ms\n";
    std::cout << "  Target:          ±1.0 ms\n";

    std::cout << "\nFEEDBACK:\n";
    std::cout << "  Stable:          " << (m.feedbackStable ? "✓ YES" : "✗ NO") << "\n";
    std::cout << "  Max Stable:      " << std::fixed << std::setprecision(0) << (m.maxStableFeedback * 100.0f) << "%\n";

    std::cout << "\nCHARACTER:\n";
    std::cout << "  Type:            " << m.emulationType << "\n";
    std::cout << "  Modulation:      " << (m.hasModulation ? "YES" : "NO") << "\n";
    std::cout << "  Saturation:      " << (m.hasSaturation ? "YES" : "NO") << "\n";
    std::cout << "  Filtering:       " << (m.hasFiltering ? "YES" : "NO") << "\n";

    std::cout << "\nQUALITY:\n";
    std::cout << "  Score:           " << std::fixed << std::setprecision(1) << m.qualityScore << "/100\n";
    std::cout << "  Rating:          " << m.qualityRating << "\n\n";
}

//==============================================================================
// Save CSV Results
//==============================================================================
void savePitchCSV(int engineId, const PitchMetrics& m) {
    std::string filename = "pitch_engine_" + std::to_string(engineId) + "_accuracy.csv";
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "InputFreq,OutputFreq,ExpectedFreq,ErrorCents\n";
    for (size_t i = 0; i < m.inputFreqs.size(); ++i) {
        file << m.inputFreqs[i] << "," << m.outputFreqs[i] << ","
             << m.expectedFreqs[i] << "," << m.pitchErrors[i] << "\n";
    }
    file.close();
}

void saveDelayCSV(int engineId, const DelayMetrics& m) {
    std::string filename = "delay_engine_" + std::to_string(engineId) + "_timing.csv";
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "TargetMs,MeasuredMs,ErrorMs\n";
    for (size_t i = 0; i < m.setDelayTimes.size(); ++i) {
        file << m.setDelayTimes[i] << "," << m.measuredDelayTimes[i] << ","
             << m.timingErrors[i] << "\n";
    }
    file.close();
}

} // namespace PitchTimeTests

//==============================================================================
// Main
//==============================================================================
int main(int argc, char* argv[]) {
    using namespace PitchTimeTests;

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     ChimeraPhoenix Pitch & Time Effects Analysis          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    const float sampleRate = 48000.0f;

    // Pitch shifting engines
    std::vector<std::pair<int, std::string>> pitchEngines = {
        {31, "Detune Doubler"},
        {32, "Pitch Shifter (CRITICAL: THD 8.673%)"},
        {33, "Intelligent Harmonizer (KNOWN CRASH)"},
        {49, "Pitch Shifter (duplicate?)"}
    };

    // Delay engines
    std::vector<std::pair<int, std::string>> delayEngines = {
        {34, "Tape Echo"},
        {35, "Digital Delay"},
        {36, "Magnetic Drum Echo"},
        {37, "Bucket Brigade Delay (BBD)"},
        {38, "Buffer Repeat Platinum"}
    };

    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  PITCH SHIFTING ENGINES\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    std::vector<PitchMetrics> pitchResults;
    for (const auto& [id, name] : pitchEngines) {
        try {
            std::cout << "\nTesting Engine " << id << ": " << name << "...\n";
            auto metrics = testPitchShifter(id, sampleRate);
            printPitchMetrics(id, name, metrics);
            savePitchCSV(id, metrics);
            pitchResults.push_back(metrics);
        } catch (const std::exception& e) {
            std::cout << "  ✗ CRASHED: " << e.what() << "\n";
        } catch (...) {
            std::cout << "  ✗ CRASHED: Unknown error\n";
        }
    }

    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "  DELAY / TIME-BASED ENGINES\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    std::vector<DelayMetrics> delayResults;
    for (const auto& [id, name] : delayEngines) {
        try {
            std::cout << "\nTesting Engine " << id << ": " << name << "...\n";
            auto metrics = testDelayEngine(id, sampleRate);
            printDelayMetrics(id, name, metrics);
            saveDelayCSV(id, metrics);
            delayResults.push_back(metrics);
        } catch (const std::exception& e) {
            std::cout << "  ✗ ERROR: " << e.what() << "\n";
        } catch (...) {
            std::cout << "  ✗ ERROR: Unknown error\n";
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                   TESTING COMPLETE                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Results saved to CSV files in current directory.\n\n";

    return 0;
}
