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
#include <complex>

/**
 * DEEP VALIDATION MISSION - Modulation Engines
 *
 * This test suite performs comprehensive validation of modulation parameters:
 * - LFO rate accuracy (Hz measurement)
 * - Depth parameter linearity (0-100%)
 * - Stereo width measurements
 * - Feedback stability limits
 * - Waveform shape analysis
 * - Phase relationships L/R
 */

namespace DeepModulationValidation {

constexpr double SAMPLE_RATE = 48000.0;
constexpr int BLOCK_SIZE = 512;
constexpr double PI = 3.14159265358979323846;
constexpr double TWO_PI = 2.0 * PI;

struct LFOAnalysis {
    double measuredFrequency = 0.0;
    double amplitudeModulation = 0.0;
    double waveformDistortion = 0.0;
    std::string waveformShape = "Unknown";
    bool passed = false;
};

struct StereoAnalysis {
    double correlation = 0.0;
    double stereoWidth = 0.0;
    double phaseOffset = 0.0;  // degrees
    double leftRMS = 0.0;
    double rightRMS = 0.0;
    bool passed = false;
};

struct DepthAnalysis {
    std::vector<double> depthSettings;  // 0.0 to 1.0
    std::vector<double> measuredModulation;  // actual modulation amount
    double linearity = 0.0;  // correlation coefficient
    bool passed = false;
};

struct FeedbackAnalysis {
    double maxStableFeedback = 0.0;
    double oscillationThreshold = 0.0;
    bool stable = false;
    bool passed = false;
};

struct EngineValidationReport {
    int engineId;
    std::string engineName;

    // Parameter documentation
    std::vector<std::string> parameterNames;
    std::vector<std::pair<double, double>> parameterRanges;

    // Analysis results
    LFOAnalysis lfoAnalysis;
    StereoAnalysis stereoAnalysis;
    DepthAnalysis depthAnalysis;
    FeedbackAnalysis feedbackAnalysis;

    bool overallPass() const {
        return lfoAnalysis.passed && stereoAnalysis.passed &&
               depthAnalysis.passed && feedbackAnalysis.passed;
    }
};

//==============================================================================
// FFT-based frequency detection
//==============================================================================
double detectDominantFrequency(const std::vector<float>& signal, double sampleRate) {
    const int fftSize = 8192;
    std::vector<float> fftInput(fftSize, 0.0f);

    // Copy signal with Hann window
    int copySize = std::min((int)signal.size(), fftSize);
    for (int i = 0; i < copySize; ++i) {
        double window = 0.5 * (1.0 - std::cos(TWO_PI * i / (copySize - 1)));
        fftInput[i] = signal[i] * window;
    }

    // Perform FFT using JUCE
    juce::dsp::FFT fft(13);  // 2^13 = 8192
    std::vector<std::complex<float>> fftOutput(fftSize);
    fft.perform(fftInput.data(), reinterpret_cast<std::complex<float>*>(fftOutput.data()), false);

    // Find peak
    double maxMag = 0.0;
    int maxBin = 0;
    for (int i = 1; i < fftSize / 2; ++i) {
        double mag = std::abs(fftOutput[i]);
        if (mag > maxMag) {
            maxMag = mag;
            maxBin = i;
        }
    }

    double frequency = maxBin * sampleRate / fftSize;
    return frequency;
}

//==============================================================================
// Envelope detection for LFO rate measurement
//==============================================================================
double detectLFORate(const std::vector<float>& signal, double sampleRate) {
    // Compute RMS envelope
    const int windowSize = 256;
    const int hopSize = 64;
    std::vector<float> envelope;

    for (size_t i = 0; i + windowSize < signal.size(); i += hopSize) {
        float rms = 0.0f;
        for (int j = 0; j < windowSize; ++j) {
            rms += signal[i + j] * signal[i + j];
        }
        envelope.push_back(std::sqrt(rms / windowSize));
    }

    if (envelope.size() < 100) return 0.0;

    // Remove DC
    float mean = std::accumulate(envelope.begin(), envelope.end(), 0.0f) / envelope.size();
    for (auto& val : envelope) {
        val -= mean;
    }

    // Use autocorrelation to find period
    int maxLag = envelope.size() / 2;
    std::vector<float> autocorr(maxLag);

    for (int lag = 0; lag < maxLag; ++lag) {
        float sum = 0.0f;
        for (size_t i = 0; i + lag < envelope.size(); ++i) {
            sum += envelope[i] * envelope[i + lag];
        }
        autocorr[lag] = sum;
    }

    // Find first significant peak after zero
    float threshold = autocorr[0] * 0.5f;
    int peakLag = 0;
    for (int lag = 5; lag < maxLag; ++lag) {
        if (autocorr[lag] > threshold &&
            autocorr[lag] > autocorr[lag-1] &&
            autocorr[lag] > autocorr[lag+1]) {
            peakLag = lag;
            break;
        }
    }

    if (peakLag == 0) return 0.0;

    double period = peakLag * hopSize / sampleRate;
    return 1.0 / period;
}

//==============================================================================
// Test 1: LFO Rate Accuracy Test
//==============================================================================
LFOAnalysis testLFORate(EngineBase* engine) {
    LFOAnalysis result;

    // Generate constant tone
    const int testLength = static_cast<int>(SAMPLE_RATE * 6.0);  // 6 seconds
    juce::AudioBuffer<float> buffer(2, testLength);

    // 440Hz sine wave input
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testLength; ++i) {
            float phase = TWO_PI * 440.0 * i / SAMPLE_RATE;
            buffer.setSample(ch, i, 0.3f * std::sin(phase));
        }
    }

    // Process in blocks
    for (int start = 0; start < testLength; start += BLOCK_SIZE) {
        int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Extract envelope and detect LFO rate
    std::vector<float> leftChannel(testLength);
    for (int i = 0; i < testLength; ++i) {
        leftChannel[i] = buffer.getSample(0, i);
    }

    result.measuredFrequency = detectLFORate(leftChannel, SAMPLE_RATE);

    // Calculate amplitude modulation depth
    float maxAmp = *std::max_element(leftChannel.begin(), leftChannel.end());
    float minAmp = *std::min_element(leftChannel.begin(), leftChannel.end());
    result.amplitudeModulation = (maxAmp - minAmp) / (maxAmp + minAmp + 1e-10);

    // Pass criteria: LFO rate between 0.01 and 20 Hz
    result.passed = (result.measuredFrequency >= 0.01 && result.measuredFrequency <= 20.0);

    return result;
}

//==============================================================================
// Test 2: Depth Parameter Linearity
//==============================================================================
DepthAnalysis testDepthLinearity(EngineBase* engine, int depthParamIndex) {
    DepthAnalysis result;

    // Test at multiple depth settings
    std::vector<double> testDepths = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0};

    for (double depth : testDepths) {
        engine->reset();

        std::map<int, float> params;
        for (int i = 0; i < 10; ++i) {
            params[i] = 0.5f;  // Default
        }
        params[depthParamIndex] = depth;
        engine->updateParameters(params);

        // Generate test signal
        const int testLength = static_cast<int>(SAMPLE_RATE * 2.0);
        juce::AudioBuffer<float> buffer(2, testLength);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testLength; ++i) {
                float phase = TWO_PI * 440.0 * i / SAMPLE_RATE;
                buffer.setSample(ch, i, 0.3f * std::sin(phase));
            }
        }

        // Process
        for (int start = 0; start < testLength; start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Measure modulation amount
        float maxAmp = -1000.0f;
        float minAmp = 1000.0f;
        for (int i = 0; i < testLength; ++i) {
            float sample = buffer.getSample(0, i);
            maxAmp = std::max(maxAmp, std::abs(sample));
            minAmp = std::min(minAmp, std::abs(sample));
        }

        double modAmount = (maxAmp - minAmp) / (maxAmp + 1e-10);

        result.depthSettings.push_back(depth);
        result.measuredModulation.push_back(modAmount);
    }

    // Calculate linearity (correlation coefficient)
    if (result.depthSettings.size() >= 3) {
        double meanX = std::accumulate(result.depthSettings.begin(), result.depthSettings.end(), 0.0) / result.depthSettings.size();
        double meanY = std::accumulate(result.measuredModulation.begin(), result.measuredModulation.end(), 0.0) / result.measuredModulation.size();

        double numerator = 0.0;
        double denomX = 0.0;
        double denomY = 0.0;

        for (size_t i = 0; i < result.depthSettings.size(); ++i) {
            double dx = result.depthSettings[i] - meanX;
            double dy = result.measuredModulation[i] - meanY;
            numerator += dx * dy;
            denomX += dx * dx;
            denomY += dy * dy;
        }

        result.linearity = numerator / (std::sqrt(denomX * denomY) + 1e-10);
    }

    result.passed = (result.linearity > 0.8);  // Good linearity

    return result;
}

//==============================================================================
// Test 3: Stereo Width Analysis
//==============================================================================
StereoAnalysis testStereoWidth(EngineBase* engine) {
    StereoAnalysis result;

    // Generate mono test signal
    const int testLength = static_cast<int>(SAMPLE_RATE * 3.0);
    juce::AudioBuffer<float> buffer(2, testLength);

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < testLength; ++i) {
            float phase = TWO_PI * 440.0 * i / SAMPLE_RATE;
            buffer.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    // Process
    for (int start = 0; start < testLength; start += BLOCK_SIZE) {
        int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Calculate correlation
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
    for (int i = 0; i < testLength; ++i) {
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    result.leftRMS = std::sqrt(sumLL / testLength);
    result.rightRMS = std::sqrt(sumRR / testLength);

    double denom = std::sqrt(sumLL * sumRR);
    if (denom > 1e-10) {
        result.correlation = sumLR / denom;
    }

    result.stereoWidth = 1.0 - std::abs(result.correlation);

    // Calculate phase offset using cross-correlation
    int maxLag = 1000;
    double maxCorr = -1.0;
    int bestLag = 0;

    for (int lag = -maxLag; lag <= maxLag; ++lag) {
        double corr = 0.0;
        int count = 0;
        for (int i = std::max(0, -lag); i < std::min(testLength, testLength - lag); ++i) {
            corr += left[i] * right[i + lag];
            count++;
        }
        if (count > 0) {
            corr /= count;
            if (std::abs(corr) > std::abs(maxCorr)) {
                maxCorr = corr;
                bestLag = lag;
            }
        }
    }

    result.phaseOffset = (bestLag * 360.0 * 440.0) / SAMPLE_RATE;

    result.passed = (result.stereoWidth > 0.01);  // Some stereo effect

    return result;
}

//==============================================================================
// Test 4: Feedback Stability
//==============================================================================
FeedbackAnalysis testFeedbackStability(EngineBase* engine, int feedbackParamIndex) {
    FeedbackAnalysis result;
    result.stable = true;

    // Test increasing feedback levels
    std::vector<double> feedbackLevels = {0.0, 0.2, 0.4, 0.6, 0.7, 0.8, 0.85, 0.9, 0.95, 0.98, 1.0};

    for (double feedback : feedbackLevels) {
        engine->reset();

        std::map<int, float> params;
        for (int i = 0; i < 10; ++i) {
            params[i] = 0.5f;
        }
        params[feedbackParamIndex] = feedback;
        engine->updateParameters(params);

        // Generate test signal
        const int testLength = static_cast<int>(SAMPLE_RATE * 1.0);
        juce::AudioBuffer<float> buffer(2, testLength);

        // Impulse
        buffer.clear();
        buffer.setSample(0, 100, 0.5f);
        buffer.setSample(1, 100, 0.5f);

        // Process
        for (int start = 0; start < testLength; start += BLOCK_SIZE) {
            int samplesThisBlock = std::min(BLOCK_SIZE, testLength - start);
            juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samplesThisBlock);
            engine->process(block);
        }

        // Check for instability
        float maxAmp = 0.0f;
        for (int i = 0; i < testLength; ++i) {
            maxAmp = std::max(maxAmp, std::abs(buffer.getSample(0, i)));
            maxAmp = std::max(maxAmp, std::abs(buffer.getSample(1, i)));
        }

        if (maxAmp > 10.0f) {
            // Unstable - oscillating
            result.oscillationThreshold = feedback;
            result.stable = false;
            break;
        }

        result.maxStableFeedback = feedback;
    }

    result.passed = (result.maxStableFeedback >= 0.7);  // Should handle at least 70% feedback

    return result;
}

//==============================================================================
// Engine Test Runner
//==============================================================================
EngineValidationReport validateEngine(int engineId, const std::string& name) {
    EngineValidationReport report;
    report.engineId = engineId;
    report.engineName = name;

    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": " << std::setw(47) << std::left << name << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "ERROR: Failed to create engine\n";
        return report;
    }

    engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);

    // Document parameters
    int numParams = engine->getNumParameters();
    std::cout << "\nParameters (" << numParams << " total):\n";
    for (int i = 0; i < numParams; ++i) {
        std::string paramName = engine->getParameterName(i).toStdString();
        report.parameterNames.push_back(paramName);
        report.parameterRanges.push_back({0.0, 1.0});  // Normalized
        std::cout << "  [" << i << "] " << paramName << " (0.0 - 1.0)\n";
    }

    // Set default parameters
    std::map<int, float> defaultParams;
    for (int i = 0; i < numParams; ++i) {
        defaultParams[i] = 0.5f;
    }
    engine->updateParameters(defaultParams);

    // Test 1: LFO Rate
    std::cout << "\n[1/4] Testing LFO Rate... " << std::flush;
    report.lfoAnalysis = testLFORate(engine.get());
    std::cout << (report.lfoAnalysis.passed ? "PASS" : "FAIL") << "\n";
    std::cout << "  Measured: " << std::fixed << std::setprecision(3)
              << report.lfoAnalysis.measuredFrequency << " Hz\n";
    std::cout << "  Modulation: " << (report.lfoAnalysis.amplitudeModulation * 100.0) << "%\n";

    // Test 2: Depth Linearity
    engine->reset();
    engine->updateParameters(defaultParams);

    std::cout << "\n[2/4] Testing Depth Linearity... " << std::flush;
    int depthParamIdx = 1;  // Usually parameter 1
    report.depthAnalysis = testDepthLinearity(engine.get(), depthParamIdx);
    std::cout << (report.depthAnalysis.passed ? "PASS" : "FAIL") << "\n";
    std::cout << "  Linearity: " << std::fixed << std::setprecision(3)
              << report.depthAnalysis.linearity << "\n";

    // Test 3: Stereo Width
    engine->reset();
    engine->updateParameters(defaultParams);

    std::cout << "\n[3/4] Testing Stereo Width... " << std::flush;
    report.stereoAnalysis = testStereoWidth(engine.get());
    std::cout << (report.stereoAnalysis.passed ? "PASS" : "FAIL") << "\n";
    std::cout << "  Correlation: " << std::fixed << std::setprecision(3)
              << report.stereoAnalysis.correlation << "\n";
    std::cout << "  Width: " << (report.stereoAnalysis.stereoWidth * 100.0) << "%\n";
    std::cout << "  Phase Offset: " << report.stereoAnalysis.phaseOffset << " degrees\n";

    // Test 4: Feedback Stability
    engine->reset();
    engine->updateParameters(defaultParams);

    std::cout << "\n[4/4] Testing Feedback Stability... " << std::flush;
    int feedbackParamIdx = 2;  // Usually parameter 2
    report.feedbackAnalysis = testFeedbackStability(engine.get(), feedbackParamIdx);
    std::cout << (report.feedbackAnalysis.passed ? "PASS" : "FAIL") << "\n";
    std::cout << "  Max Stable: " << std::fixed << std::setprecision(1)
              << (report.feedbackAnalysis.maxStableFeedback * 100.0) << "%\n";
    if (!report.feedbackAnalysis.stable) {
        std::cout << "  Oscillation at: " << (report.feedbackAnalysis.oscillationThreshold * 100.0) << "%\n";
    }

    return report;
}

} // namespace DeepModulationValidation

//==============================================================================
// Main
//==============================================================================
int main() {
    using namespace DeepModulationValidation;

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   DEEP VALIDATION MISSION - Modulation Engines (24-33)        ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   Tests:                                                       ║\n";
    std::cout << "║   • LFO rate accuracy (Hz measurement)                         ║\n";
    std::cout << "║   • Depth parameter linearity                                  ║\n";
    std::cout << "║   • Stereo width & phase analysis                              ║\n";
    std::cout << "║   • Feedback stability limits                                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";

    // Test key modulation engines
    std::vector<std::pair<int, std::string>> engines = {
        {23, "Stereo Chorus"},
        {24, "Resonant Chorus Platinum"},
        {25, "Analog Phaser"},
        {26, "Platinum Ring Modulator"},
        {27, "Frequency Shifter"},
        {28, "Harmonic Tremolo"},
        {29, "Classic Tremolo"},
        {46, "Dimension Expander"},
        {14, "Vocal Formant Filter"},
        {12, "Envelope Filter"}
    };

    std::vector<EngineValidationReport> allReports;

    for (const auto& [id, name] : engines) {
        auto report = validateEngine(id, name);
        allReports.push_back(report);
    }

    // Generate summary report
    std::cout << "\n\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      VALIDATION SUMMARY                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left << std::setw(6) << "ID"
              << std::setw(30) << "Engine"
              << std::setw(8) << "LFO"
              << std::setw(8) << "Depth"
              << std::setw(8) << "Stereo"
              << std::setw(8) << "Fdbk"
              << "Overall\n";
    std::cout << std::string(78, '-') << "\n";

    int passCount = 0;
    for (const auto& report : allReports) {
        bool overall = report.overallPass();
        if (overall) passCount++;

        std::cout << std::left << std::setw(6) << report.engineId
                  << std::setw(30) << report.engineName
                  << std::setw(8) << (report.lfoAnalysis.passed ? "PASS" : "FAIL")
                  << std::setw(8) << (report.depthAnalysis.passed ? "PASS" : "FAIL")
                  << std::setw(8) << (report.stereoAnalysis.passed ? "PASS" : "FAIL")
                  << std::setw(8) << (report.feedbackAnalysis.passed ? "PASS" : "FAIL")
                  << (overall ? "PASS" : "FAIL") << "\n";
    }

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Final Score: " << passCount << "/" << allReports.size() << " engines passed all tests"
              << std::string(32 - std::to_string(passCount).length() - std::to_string(allReports.size()).length(), ' ') << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    // Save detailed CSV reports
    std::ofstream csvLFO("modulation_lfo_rates.csv");
    csvLFO << "Engine ID,Engine Name,LFO Rate (Hz),Amplitude Modulation (%)\n";
    for (const auto& r : allReports) {
        csvLFO << r.engineId << "," << r.engineName << ","
               << r.lfoAnalysis.measuredFrequency << ","
               << (r.lfoAnalysis.amplitudeModulation * 100.0) << "\n";
    }
    csvLFO.close();

    std::ofstream csvStereo("modulation_stereo_analysis.csv");
    csvStereo << "Engine ID,Engine Name,Correlation,Width (%),Phase (deg),L RMS,R RMS\n";
    for (const auto& r : allReports) {
        csvStereo << r.engineId << "," << r.engineName << ","
                  << r.stereoAnalysis.correlation << ","
                  << (r.stereoAnalysis.stereoWidth * 100.0) << ","
                  << r.stereoAnalysis.phaseOffset << ","
                  << r.stereoAnalysis.leftRMS << ","
                  << r.stereoAnalysis.rightRMS << "\n";
    }
    csvStereo.close();

    std::cout << "Detailed reports saved:\n";
    std::cout << "  • modulation_lfo_rates.csv\n";
    std::cout << "  • modulation_stereo_analysis.csv\n\n";

    return (passCount == allReports.size()) ? 0 : 1;
}
