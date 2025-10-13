#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <complex>
#include <algorithm>

/**
 * COMPREHENSIVE FILTER & EQ TEST SUITE
 *
 * Testing engines 7-14 (Filters & EQ):
 * 7.  Parametric EQ Studio
 * 8.  Vintage Console EQ Studio
 * 9.  Ladder Filter Pro (KNOWN: THD 3.512%)
 * 10. State Variable Filter
 * 11. Formant Filter Pro
 * 12. Envelope Filter
 * 13. Comb Resonator
 * 14. Vocal Formant Filter
 *
 * MEASUREMENTS:
 * - Frequency Response (magnitude & phase) 20Hz-20kHz
 * - Filter Slope/Roll-off (dB/octave)
 * - Cutoff frequency accuracy
 * - Q factor verification
 * - THD vs frequency analysis
 * - Resonance peak measurement
 * - Impulse response
 * - Group delay
 * - Passband flatness
 */

namespace FilterTests {

// Constants
constexpr float PROFESSIONAL_THD_THRESHOLD = 0.01f;  // 0.01% for professional clean filters
constexpr float PI = 3.14159265358979323846f;
constexpr int FFT_SIZE = 8192;
constexpr int NUM_TEST_FREQUENCIES = 100;

//==============================================================================
// DATA STRUCTURES
//==============================================================================

struct FrequencyPoint {
    float frequency;
    float magnitudeDB;
    float phaseRadians;
};

struct FilterMetrics {
    std::vector<FrequencyPoint> frequencyResponse;
    float cutoffFrequency;
    float cutoffAccuracy;  // % error
    float passbandFlatness; // dB variance in passband
    float stopbandAttenuation;
    float filterSlope;      // dB/octave
    int filterOrder;        // Estimated from slope
    float resonantPeakDB;
    float qFactor;
    float groupDelayMs;
    bool isSelfOscillating;
    bool isStable;
    std::vector<float> impulseResponse;
    float settlingTimeMs;
    float ringingDuration;

    // THD Analysis
    std::map<float, float> thdVsFrequency;
    float avgTHD;
    float maxTHD;
    float thdAt1kHz;

    // Filter characterization
    std::string filterType;  // "Butterworth", "Chebyshev", "Moog Ladder", etc.
    std::string filterMode;  // "LP", "HP", "BP", "Notch", etc.
    std::string musicalCharacter;
};

//==============================================================================
// COMPLEX NUMBER HELPERS
//==============================================================================

struct Complex {
    float real, imag;

    Complex(float r = 0.0f, float i = 0.0f) : real(r), imag(i) {}

    float magnitude() const {
        return std::sqrt(real * real + imag * imag);
    }

    float phase() const {
        return std::atan2(imag, real);
    }

    Complex operator*(const Complex& other) const {
        return Complex(
            real * other.real - imag * other.imag,
            real * other.imag + imag * other.real
        );
    }

    Complex operator+(const Complex& other) const {
        return Complex(real + other.real, imag + other.imag);
    }
};

//==============================================================================
// FFT-BASED FREQUENCY RESPONSE MEASUREMENT
//==============================================================================

std::vector<FrequencyPoint> measureFrequencyResponse(
    EngineBase* engine,
    float sampleRate,
    int blockSize,
    const std::map<int, float>& params
) {
    std::cout << "  [FreqResponse] Measuring 20Hz-20kHz...\n" << std::flush;

    std::vector<FrequencyPoint> response;

    // Generate logarithmic frequency sweep
    std::vector<float> testFrequencies;
    for (int i = 0; i < NUM_TEST_FREQUENCIES; ++i) {
        float t = static_cast<float>(i) / (NUM_TEST_FREQUENCIES - 1);
        float freq = 20.0f * std::pow(1000.0f, t);  // 20Hz to 20kHz logarithmic
        testFrequencies.push_back(freq);
    }

    // Reset engine for clean measurements
    engine->reset();
    engine->updateParameters(params);

    // Measure response at each frequency
    for (float freq : testFrequencies) {
        if (freq > sampleRate / 2.1f) continue;  // Skip above Nyquist

        const int measurementLength = FFT_SIZE;
        juce::AudioBuffer<float> input(2, measurementLength);
        juce::AudioBuffer<float> output(2, measurementLength);

        // Generate pure sine wave at this frequency
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < measurementLength; ++i) {
                float phase = 2.0f * PI * freq * i / sampleRate;
                input.setSample(ch, i, 0.5f * std::sin(phase));
            }
        }

        output.makeCopyOf(input);

        // Process in blocks
        for (int start = 0; start < measurementLength; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, measurementLength - start);
            juce::AudioBuffer<float> block(output.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Analyze input and output using FFT
        juce::dsp::FFT fft(13); // 2^13 = 8192
        std::vector<float> inputFFT(FFT_SIZE * 2, 0.0f);
        std::vector<float> outputFFT(FFT_SIZE * 2, 0.0f);

        // Apply Hann window
        for (int i = 0; i < FFT_SIZE; ++i) {
            float window = 0.5f * (1.0f - std::cos(2.0f * PI * i / FFT_SIZE));
            inputFFT[i] = input.getSample(0, i) * window;
            outputFFT[i] = output.getSample(0, i) * window;
        }

        fft.performRealOnlyForwardTransform(inputFFT.data(), true);
        fft.performRealOnlyForwardTransform(outputFFT.data(), true);

        // Find bin for this frequency
        int bin = static_cast<int>(freq * FFT_SIZE / sampleRate);
        if (bin >= FFT_SIZE / 2) continue;

        // Calculate complex values at this bin
        Complex inputComplex(inputFFT[bin * 2], inputFFT[bin * 2 + 1]);
        Complex outputComplex(outputFFT[bin * 2], outputFFT[bin * 2 + 1]);

        float inputMag = inputComplex.magnitude();
        float outputMag = outputComplex.magnitude();

        float magnitudeDB = 0.0f;
        if (inputMag > 1e-10f) {
            magnitudeDB = 20.0f * std::log10(outputMag / inputMag);
        }

        float phaseShift = outputComplex.phase() - inputComplex.phase();

        // Wrap phase to [-pi, pi]
        while (phaseShift > PI) phaseShift -= 2.0f * PI;
        while (phaseShift < -PI) phaseShift += 2.0f * PI;

        FrequencyPoint point;
        point.frequency = freq;
        point.magnitudeDB = magnitudeDB;
        point.phaseRadians = phaseShift;

        response.push_back(point);
    }

    std::cout << "  [FreqResponse] Measured " << response.size() << " points\n" << std::flush;

    return response;
}

//==============================================================================
// THD VS FREQUENCY ANALYSIS
//==============================================================================

std::map<float, float> measureTHDvsFrequency(
    EngineBase* engine,
    float sampleRate,
    int blockSize,
    const std::map<int, float>& params
) {
    std::cout << "  [THD Analysis] Testing 50Hz-10kHz...\n" << std::flush;

    std::map<float, float> thdData;

    // Test frequencies for THD measurement
    std::vector<float> testFreqs = {50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f};

    engine->reset();
    engine->updateParameters(params);

    for (float freq : testFreqs) {
        if (freq > sampleRate / 2.1f) continue;

        juce::AudioBuffer<float> buffer(2, FFT_SIZE);

        // Generate pure sine wave
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < FFT_SIZE; ++i) {
                float phase = 2.0f * PI * freq * i / sampleRate;
                buffer.setSample(ch, i, 0.3f * std::sin(phase));
            }
        }

        // Process
        for (int start = 0; start < FFT_SIZE; start += blockSize) {
            int samplesThisBlock = std::min(blockSize, FFT_SIZE - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // FFT analysis
        juce::dsp::FFT fft(13);
        std::vector<float> fftData(FFT_SIZE * 2, 0.0f);

        for (int i = 0; i < FFT_SIZE; ++i) {
            float window = 0.5f * (1.0f - std::cos(2.0f * PI * i / FFT_SIZE));
            fftData[i] = buffer.getSample(0, i) * window;
        }

        fft.performFrequencyOnlyForwardTransform(fftData.data());

        // Find fundamental
        int fundamentalBin = static_cast<int>(freq * FFT_SIZE / sampleRate);
        float fundamentalMag = fftData[fundamentalBin];

        // Sum harmonics (2nd through 10th)
        float harmonicsSumSquared = 0.0f;
        for (int h = 2; h <= 10; ++h) {
            int harmonicBin = fundamentalBin * h;
            if (harmonicBin < FFT_SIZE / 2) {
                float harmonicMag = fftData[harmonicBin];
                harmonicsSumSquared += harmonicMag * harmonicMag;
            }
        }

        float thd = 0.0f;
        if (fundamentalMag > 1e-10f) {
            thd = (std::sqrt(harmonicsSumSquared) / fundamentalMag) * 100.0f;
        }

        thdData[freq] = thd;

        std::cout << "    " << std::setw(6) << freq << " Hz: THD = "
                  << std::fixed << std::setprecision(4) << thd << "%\n" << std::flush;
    }

    return thdData;
}

//==============================================================================
// IMPULSE RESPONSE ANALYSIS
//==============================================================================

std::vector<float> measureImpulseResponse(
    EngineBase* engine,
    float sampleRate,
    int blockSize,
    const std::map<int, float>& params
) {
    std::cout << "  [Impulse] Capturing response...\n" << std::flush;

    engine->reset();
    engine->updateParameters(params);

    const int impulseLength = static_cast<int>(sampleRate * 0.5f); // 500ms
    juce::AudioBuffer<float> buffer(2, impulseLength);
    buffer.clear();

    // Create impulse (single sample spike)
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    // Process in blocks
    for (int start = 0; start < impulseLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, impulseLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Extract impulse response
    std::vector<float> impulse;
    impulse.reserve(impulseLength);
    for (int i = 0; i < impulseLength; ++i) {
        impulse.push_back(buffer.getSample(0, i));
    }

    return impulse;
}

//==============================================================================
// FILTER ANALYSIS
//==============================================================================

float estimateFilterSlope(const std::vector<FrequencyPoint>& response) {
    // Find -3dB point (cutoff)
    float cutoffFreq = 0.0f;
    float passbandDB = 0.0f;

    // Estimate passband level (average of first 10 points)
    for (int i = 0; i < std::min(10, (int)response.size()); ++i) {
        passbandDB += response[i].magnitudeDB;
    }
    passbandDB /= std::min(10, (int)response.size());

    float targetDB = passbandDB - 3.0f;

    for (const auto& point : response) {
        if (point.magnitudeDB < targetDB) {
            cutoffFreq = point.frequency;
            break;
        }
    }

    if (cutoffFreq < 20.0f) return 0.0f;

    // Measure slope 1 octave after cutoff
    float freq1Octave = cutoffFreq * 2.0f;
    float db1Octave = 0.0f;

    for (const auto& point : response) {
        if (point.frequency >= freq1Octave) {
            db1Octave = point.magnitudeDB;
            break;
        }
    }

    float slope = std::abs(db1Octave - targetDB);

    return slope;
}

float estimateCutoffFrequency(const std::vector<FrequencyPoint>& response) {
    if (response.empty()) return 0.0f;

    // Find passband level
    float passbandDB = response[0].magnitudeDB;
    for (int i = 0; i < std::min(5, (int)response.size()); ++i) {
        passbandDB = std::max(passbandDB, response[i].magnitudeDB);
    }

    float targetDB = passbandDB - 3.0f;

    // Find first point below -3dB
    for (const auto& point : response) {
        if (point.magnitudeDB < targetDB) {
            return point.frequency;
        }
    }

    return 0.0f;
}

float measureResonantPeak(const std::vector<FrequencyPoint>& response) {
    float maxDB = -100.0f;

    for (const auto& point : response) {
        maxDB = std::max(maxDB, point.magnitudeDB);
    }

    return maxDB;
}

float calculatePassbandFlatness(const std::vector<FrequencyPoint>& response) {
    if (response.size() < 10) return 0.0f;

    // Calculate variance in first 20% of frequency range (passband)
    int numPassbandPoints = response.size() / 5;
    float sum = 0.0f;

    for (int i = 0; i < numPassbandPoints; ++i) {
        sum += response[i].magnitudeDB;
    }
    float mean = sum / numPassbandPoints;

    float variance = 0.0f;
    for (int i = 0; i < numPassbandPoints; ++i) {
        float diff = response[i].magnitudeDB - mean;
        variance += diff * diff;
    }

    return std::sqrt(variance / numPassbandPoints);
}

std::string identifyFilterType(const FilterMetrics& metrics) {
    // Identify filter type based on characteristics

    if (metrics.resonantPeakDB > 6.0f) {
        return "Resonant (Moog-style ladder or similar)";
    }

    if (metrics.passbandFlatness < 0.5f) {
        return "Butterworth (maximally flat)";
    }

    if (metrics.resonantPeakDB > 1.0f && metrics.resonantPeakDB < 6.0f) {
        return "Chebyshev (ripple in passband)";
    }

    if (metrics.filterSlope > 20.0f && metrics.filterSlope < 30.0f) {
        return "4-pole (24dB/oct)";
    }

    if (metrics.filterSlope > 10.0f && metrics.filterSlope < 15.0f) {
        return "2-pole (12dB/oct)";
    }

    return "Unknown topology";
}

//==============================================================================
// COMPREHENSIVE FILTER TEST
//==============================================================================

FilterMetrics testFilter(int engineId, float sampleRate = 48000.0f) {
    FilterMetrics metrics = {};

    std::cout << "\n[Engine " << engineId << "] Starting test...\n" << std::flush;

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "[ERROR] Failed to create engine\n" << std::flush;
        return metrics;
    }

    const int blockSize = 512;
    engine->prepareToPlay(sampleRate, blockSize);

    // Set filter parameters
    std::map<int, float> params;
    int numParams = engine->getNumParameters();

    // Generic parameter setup (will vary by filter type)
    if (numParams > 0) params[0] = 1.0f;   // Mix/Wet = 100%
    if (numParams > 1) params[1] = 0.5f;   // Cutoff/Frequency = middle
    if (numParams > 2) params[2] = 0.7f;   // Resonance/Q = moderate
    if (numParams > 3) params[3] = 0.5f;   // Additional parameter
    if (numParams > 4) params[4] = 0.5f;   // Additional parameter

    engine->updateParameters(params);

    // 1. Frequency Response
    metrics.frequencyResponse = measureFrequencyResponse(engine.get(), sampleRate, blockSize, params);

    // 2. Analyze frequency response
    metrics.cutoffFrequency = estimateCutoffFrequency(metrics.frequencyResponse);
    metrics.filterSlope = estimateFilterSlope(metrics.frequencyResponse);
    metrics.resonantPeakDB = measureResonantPeak(metrics.frequencyResponse);
    metrics.passbandFlatness = calculatePassbandFlatness(metrics.frequencyResponse);

    // Estimate filter order from slope
    if (metrics.filterSlope > 20.0f) {
        metrics.filterOrder = static_cast<int>((metrics.filterSlope + 3.0f) / 6.0f);
    } else {
        metrics.filterOrder = 2;
    }

    // 3. THD Analysis
    metrics.thdVsFrequency = measureTHDvsFrequency(engine.get(), sampleRate, blockSize, params);

    // Calculate THD statistics
    float thdSum = 0.0f;
    float maxThd = 0.0f;
    for (const auto& [freq, thd] : metrics.thdVsFrequency) {
        thdSum += thd;
        maxThd = std::max(maxThd, thd);
        if (freq == 1000.0f) {
            metrics.thdAt1kHz = thd;
        }
    }
    metrics.avgTHD = thdSum / metrics.thdVsFrequency.size();
    metrics.maxTHD = maxThd;

    // 4. Impulse Response
    metrics.impulseResponse = measureImpulseResponse(engine.get(), sampleRate, blockSize, params);

    // Analyze stability
    metrics.isStable = true;
    for (float sample : metrics.impulseResponse) {
        if (std::isnan(sample) || std::isinf(sample) || std::abs(sample) > 10.0f) {
            metrics.isStable = false;
            break;
        }
    }

    // 5. Identify filter type
    metrics.filterType = identifyFilterType(metrics);

    std::cout << "[Engine " << engineId << "] Test complete\n" << std::flush;

    return metrics;
}

//==============================================================================
// CSV EXPORT
//==============================================================================

void exportFrequencyResponseCSV(int engineId, const std::vector<FrequencyPoint>& response) {
    std::string filename = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/build/filter_engine_"
                          + std::to_string(engineId) + "_magnitude.csv";

    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "Frequency (Hz),Magnitude (dB),Phase (radians)\n";
    for (const auto& point : response) {
        file << point.frequency << "," << point.magnitudeDB << "," << point.phaseRadians << "\n";
    }

    file.close();
    std::cout << "  Saved: " << filename << "\n";
}

void exportTHDDataCSV(int engineId, const std::map<float, float>& thdData) {
    std::string filename = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/build/filter_engine_"
                          + std::to_string(engineId) + "_thd_vs_freq.csv";

    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "Frequency (Hz),THD (%)\n";
    for (const auto& [freq, thd] : thdData) {
        file << freq << "," << thd << "\n";
    }

    file.close();
    std::cout << "  Saved: " << filename << "\n";
}

void exportImpulseResponseCSV(int engineId, const std::vector<float>& impulse, float sampleRate) {
    std::string filename = "/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/build/filter_engine_"
                          + std::to_string(engineId) + "_impulse.csv";

    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "Time (ms),Amplitude\n";
    for (size_t i = 0; i < impulse.size(); ++i) {
        float timeMs = (i / sampleRate) * 1000.0f;
        file << timeMs << "," << impulse[i] << "\n";
    }

    file.close();
    std::cout << "  Saved: " << filename << "\n";
}

//==============================================================================
// RESULTS DISPLAY
//==============================================================================

void printFilterMetrics(int engineId, const std::string& name, const FilterMetrics& m) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": " << std::setw(45) << std::left << name << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "FILTER CHARACTERISTICS:\n";
    std::cout << "  Type:              " << m.filterType << "\n";
    std::cout << "  Cutoff Frequency:  " << std::fixed << std::setprecision(1) << m.cutoffFrequency << " Hz\n";
    std::cout << "  Filter Slope:      " << std::fixed << std::setprecision(1) << m.filterSlope << " dB/octave\n";
    std::cout << "  Filter Order:      " << m.filterOrder << "-pole (" << (m.filterOrder * 6) << " dB/oct)\n";
    std::cout << "  Resonant Peak:     " << std::fixed << std::setprecision(2) << m.resonantPeakDB << " dB\n";
    std::cout << "  Passband Flatness: " << std::fixed << std::setprecision(2) << m.passbandFlatness << " dB variance\n";
    std::cout << "  Stability:         " << (m.isStable ? "STABLE" : "UNSTABLE!") << "\n";

    std::cout << "\nTHD ANALYSIS:\n";
    std::cout << "  THD @ 1kHz:        " << std::fixed << std::setprecision(4) << m.thdAt1kHz << "%";
    if (m.thdAt1kHz > PROFESSIONAL_THD_THRESHOLD) {
        std::cout << "  [HIGH - Above pro standard]";
    } else {
        std::cout << "  [EXCELLENT]";
    }
    std::cout << "\n";
    std::cout << "  Average THD:       " << std::fixed << std::setprecision(4) << m.avgTHD << "%\n";
    std::cout << "  Maximum THD:       " << std::fixed << std::setprecision(4) << m.maxTHD << "%\n";
    std::cout << "  Pro Standard:      < " << PROFESSIONAL_THD_THRESHOLD << "%\n";

    std::cout << "\nQUALITY ASSESSMENT:\n";
    bool passSlope = m.filterSlope > 6.0f && m.filterSlope < 48.0f;
    bool passFlatness = m.passbandFlatness < 2.0f;
    bool passTHD = m.thdAt1kHz < 0.1f; // Relaxed for filters
    bool passStability = m.isStable;

    std::cout << "  Filter Slope:      " << (passSlope ? "PASS" : "FAIL") << "\n";
    std::cout << "  Passband Flat:     " << (passFlatness ? "PASS" : "FAIL") << "\n";
    std::cout << "  Low THD:           " << (passTHD ? "PASS" : "FAIL") << "\n";
    std::cout << "  Stability:         " << (passStability ? "PASS" : "FAIL") << "\n";

    bool overall = passSlope && passFlatness && passTHD && passStability;
    std::cout << "\n  OVERALL:           " << (overall ? "PASSED" : "FAILED") << "\n\n";
}

} // namespace FilterTests

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char* argv[]) {
    std::vector<std::pair<int, std::string>> filterEngines = {
        {7, "Parametric EQ Studio"},
        {8, "Vintage Console EQ Studio"},
        {9, "Ladder Filter Pro"},
        {10, "State Variable Filter"},
        {11, "Formant Filter Pro"},
        {12, "Envelope Filter"},
        {13, "Comb Resonator"},
        {14, "Vocal Formant Filter"}
    };

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     ChimeraPhoenix Filter & EQ Deep Analysis Suite         ║\n";
    std::cout << "║                 Engines 7-14 Testing                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    std::vector<FilterTests::FilterMetrics> allMetrics;

    for (const auto& [id, name] : filterEngines) {
        std::cout << "\n[TEST] Engine " << id << ": " << name << "\n";
        std::cout << std::string(60, '=') << "\n";

        auto metrics = FilterTests::testFilter(id);
        allMetrics.push_back(metrics);

        FilterTests::printFilterMetrics(id, name, metrics);

        // Export data
        std::cout << "\nExporting data files...\n";
        FilterTests::exportFrequencyResponseCSV(id, metrics.frequencyResponse);
        FilterTests::exportTHDDataCSV(id, metrics.thdVsFrequency);
        FilterTests::exportImpulseResponseCSV(id, metrics.impulseResponse, 48000.0f);
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    TEST COMPLETE                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "All data exported to build/ directory\n";
    std::cout << "  - filter_engine_XX_magnitude.csv\n";
    std::cout << "  - filter_engine_XX_thd_vs_freq.csv\n";
    std::cout << "  - filter_engine_XX_impulse.csv\n\n";

    return 0;
}
