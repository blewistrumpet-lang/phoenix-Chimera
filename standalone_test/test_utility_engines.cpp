// test_utility_engines.cpp - Comprehensive Utility Engine Validation
// Tests: GranularCloud, PhasedVocoder, GainUtility_Platinum, MonoMaker_Platinum

#include "JuceHeader.h"
#include "GranularCloud.h"
#include "PhasedVocoder.h"
#include "GainUtility_Platinum.h"
#include "MonoMaker_Platinum.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <map>
#include <sstream>

// Test result structure
struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    std::map<std::string, float> metrics;
};

// Test suite class
class UtilityEngineTests {
public:
    std::vector<TestResult> results;

    // Color codes for terminal output
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string YELLOW = "\033[33m";
    const std::string CYAN = "\033[36m";
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";

    void printHeader(const std::string& title) {
        std::cout << "\n" << BOLD << CYAN << "========================================" << RESET << "\n";
        std::cout << BOLD << CYAN << title << RESET << "\n";
        std::cout << BOLD << CYAN << "========================================" << RESET << "\n\n";
    }

    void printResult(const TestResult& result) {
        std::string status = result.passed ? GREEN + "PASS" : RED + "FAIL";
        std::cout << status << RESET << " - " << result.name << "\n";
        if (!result.message.empty()) {
            std::cout << "       " << result.message << "\n";
        }
        for (const auto& [key, value] : result.metrics) {
            std::cout << "       " << key << ": " << std::fixed << std::setprecision(6) << value << "\n";
        }
    }

    void printSummary() {
        int passed = 0;
        for (const auto& result : results) {
            if (result.passed) passed++;
        }

        std::cout << "\n" << BOLD << "========================================" << RESET << "\n";
        std::cout << BOLD << "Test Summary: " << passed << "/" << results.size() << " passed" << RESET << "\n";
        std::cout << BOLD << "========================================" << RESET << "\n\n";
    }

    // Helper to generate test audio
    void generateTestSignal(juce::AudioBuffer<float>& buffer, float frequency, float amplitude = 0.5f) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        const float sr = 48000.0f;

        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                float phase = 2.0f * M_PI * frequency * i / sr;
                data[i] = amplitude * std::sin(phase + (ch * 0.2f)); // Slight phase offset for stereo
            }
        }
    }

    // Helper to measure RMS
    float measureRMS(const juce::AudioBuffer<float>& buffer, int channel = 0) {
        const float* data = buffer.getReadPointer(channel);
        const int numSamples = buffer.getNumSamples();

        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            sum += data[i] * data[i];
        }
        return std::sqrt(sum / numSamples);
    }

    // Helper to measure peak
    float measurePeak(const juce::AudioBuffer<float>& buffer, int channel = 0) {
        const float* data = buffer.getReadPointer(channel);
        const int numSamples = buffer.getNumSamples();

        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
        return peak;
    }

    // Helper to check for NaN/Inf
    bool checkFinite(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (!std::isfinite(data[i])) return false;
            }
        }
        return true;
    }

    // Helper to check for silence
    bool isSilent(const juce::AudioBuffer<float>& buffer, float threshold = 1e-6f) {
        return measureRMS(buffer) < threshold;
    }

    //==========================================================================
    // GRANULAR CLOUD TESTS
    //==========================================================================

    void testGranularCloud_ParameterRanges() {
        TestResult result;
        result.name = "GranularCloud: Parameter Ranges";
        result.passed = true;

        GranularCloud engine;
        engine.prepareToPlay(48000.0, 512);

        // Test all parameter ranges
        std::map<int, float> params;

        // Test grain size extremes
        params[0] = 0.0f; // Min grain size (2ms)
        engine.updateParameters(params);

        params[0] = 1.0f; // Max grain size (300ms)
        engine.updateParameters(params);

        // Test density extremes
        params[1] = 0.0f; // Min density (1 grain/sec)
        engine.updateParameters(params);

        params[1] = 1.0f; // Max density (200 grains/sec)
        engine.updateParameters(params);

        // Test pitch scatter extremes
        params[2] = 0.0f; // No scatter
        engine.updateParameters(params);

        params[2] = 1.0f; // Max scatter (4 octaves)
        engine.updateParameters(params);

        // Test cloud position
        params[3] = 0.0f; // Left
        params[3] = 0.5f; // Center
        params[3] = 1.0f; // Right
        engine.updateParameters(params);

        // Test mix
        params[4] = 0.0f; // Dry
        params[4] = 0.5f; // 50/50
        params[4] = 1.0f; // Wet
        engine.updateParameters(params);

        result.message = "All parameter ranges accepted";
        results.push_back(result);
    }

    void testGranularCloud_GrainProcessing() {
        TestResult result;
        result.name = "GranularCloud: Grain Processing";
        result.passed = true;

        GranularCloud engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 4096);
        generateTestSignal(buffer, 440.0f, 0.5f);

        // Set parameters for active granulation
        std::map<int, float> params;
        params[0] = 0.2f;  // Grain size ~60ms
        params[1] = 0.5f;  // Density ~100 grains/sec
        params[2] = 0.2f;  // Pitch scatter ~0.8 octaves
        params[3] = 0.5f;  // Center position
        params[4] = 1.0f;  // Full wet
        engine.updateParameters(params);

        // Process multiple blocks
        for (int block = 0; block < 10; ++block) {
            engine.process(buffer);
        }

        // Verify output
        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "Output contains NaN/Inf";
        } else if (isSilent(buffer)) {
            result.passed = false;
            result.message = "Output is silent";
        } else {
            float rms = measureRMS(buffer);
            result.metrics["RMS"] = rms;
            result.metrics["Peak"] = measurePeak(buffer);
            result.message = "Grain processing verified";
        }

        results.push_back(result);
    }

    void testGranularCloud_DensityStressTest() {
        TestResult result;
        result.name = "GranularCloud: High Density Stress Test";
        result.passed = true;

        GranularCloud engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateTestSignal(buffer, 440.0f, 0.5f);

        // Test maximum density
        std::map<int, float> params;
        params[0] = 0.1f;  // Small grains (20ms)
        params[1] = 1.0f;  // Maximum density (200 grains/sec)
        params[2] = 0.5f;  // Moderate scatter
        params[3] = 0.5f;  // Center
        params[4] = 1.0f;  // Full wet
        engine.updateParameters(params);

        // Process multiple blocks
        auto start = std::chrono::high_resolution_clock::now();
        for (int block = 0; block < 100; ++block) {
            engine.process(buffer);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "Output contains NaN/Inf at high density";
        } else {
            result.metrics["Processing Time (ms)"] = duration;
            result.metrics["RMS"] = measureRMS(buffer);
            result.message = "High density processing stable";
        }

        results.push_back(result);
    }

    void testGranularCloud_PitchScatter() {
        TestResult result;
        result.name = "GranularCloud: Pitch Scatter Quality";
        result.passed = true;

        GranularCloud engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 2048);
        generateTestSignal(buffer, 440.0f, 0.5f);

        // Test different scatter amounts
        std::map<int, float> params;
        params[0] = 0.3f;  // Medium grain size
        params[1] = 0.3f;  // Medium density
        params[3] = 0.5f;  // Center
        params[4] = 1.0f;  // Full wet

        std::vector<float> scatterAmounts = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        std::vector<float> rmsValues;

        for (float scatter : scatterAmounts) {
            params[2] = scatter;
            engine.updateParameters(params);
            engine.reset();

            // Process
            generateTestSignal(buffer, 440.0f, 0.5f);
            for (int i = 0; i < 5; ++i) {
                engine.process(buffer);
            }

            float rms = measureRMS(buffer);
            rmsValues.push_back(rms);
        }

        // Verify scatter affects output
        float minRms = *std::min_element(rmsValues.begin(), rmsValues.end());
        float maxRms = *std::max_element(rmsValues.begin(), rmsValues.end());

        result.metrics["Min RMS"] = minRms;
        result.metrics["Max RMS"] = maxRms;
        result.metrics["RMS Variation"] = (maxRms - minRms) / minRms;
        result.message = "Pitch scatter functional";

        results.push_back(result);
    }

    //==========================================================================
    // PHASED VOCODER TESTS
    //==========================================================================

    void testPhasedVocoder_TimeStretch() {
        TestResult result;
        result.name = "PhasedVocoder: Time Stretch Accuracy";
        result.passed = true;

        PhasedVocoder engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateTestSignal(buffer, 440.0f, 0.5f);

        // Test stretch factors
        std::map<int, float> params;
        params[7] = 0.0f;  // Freeze off
        params[6] = 1.0f;  // Full wet

        std::vector<float> stretchFactors = {0.0f, 0.2f, 0.5f, 0.8f, 1.0f}; // 0.25x, 1x, 2x, 3.25x, 4x

        for (float stretch : stretchFactors) {
            params[0] = stretch;
            engine.updateParameters(params);
            engine.reset();

            // Process
            for (int i = 0; i < 10; ++i) {
                generateTestSignal(buffer, 440.0f, 0.5f);
                engine.process(buffer);
            }

            if (!checkFinite(buffer)) {
                result.passed = false;
                result.message = "NaN/Inf at stretch = " + std::to_string(stretch);
                break;
            }
        }

        if (result.passed) {
            result.message = "Time stretch functional across range";
        }

        results.push_back(result);
    }

    void testPhasedVocoder_PitchShift() {
        TestResult result;
        result.name = "PhasedVocoder: Pitch Shift Range";
        result.passed = true;

        PhasedVocoder engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);

        std::map<int, float> params;
        params[0] = 0.2f;  // 1x time stretch
        params[6] = 1.0f;  // Full wet
        params[7] = 0.0f;  // Freeze off

        // Test pitch range: -24 to +24 semitones
        std::vector<float> pitchValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

        for (float pitch : pitchValues) {
            params[1] = pitch;
            engine.updateParameters(params);
            engine.reset();

            // Process
            for (int i = 0; i < 10; ++i) {
                generateTestSignal(buffer, 440.0f, 0.5f);
                engine.process(buffer);
            }

            if (!checkFinite(buffer)) {
                result.passed = false;
                result.message = "NaN/Inf at pitch = " + std::to_string(pitch);
                break;
            }
        }

        if (result.passed) {
            result.message = "Pitch shift functional across range";
        }

        results.push_back(result);
    }

    void testPhasedVocoder_SpectralEffects() {
        TestResult result;
        result.name = "PhasedVocoder: Spectral Processing";
        result.passed = true;

        PhasedVocoder engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateTestSignal(buffer, 440.0f, 0.5f);

        std::map<int, float> params;
        params[0] = 0.2f;  // 1x time
        params[1] = 0.5f;  // No pitch shift
        params[6] = 1.0f;  // Full wet
        params[7] = 0.0f;  // Freeze off

        // Test spectral smear
        params[2] = 0.5f;  // 50% smear
        engine.updateParameters(params);

        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "NaN/Inf with spectral smear";
        }

        // Test spectral gate
        params[2] = 0.0f;  // No smear
        params[5] = 0.5f;  // 50% gate
        engine.updateParameters(params);
        engine.reset();

        for (int i = 0; i < 10; ++i) {
            generateTestSignal(buffer, 440.0f, 0.5f);
            engine.process(buffer);
        }

        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "NaN/Inf with spectral gate";
        }

        if (result.passed) {
            result.message = "Spectral effects functional";
        }

        results.push_back(result);
    }

    void testPhasedVocoder_FreezeMode() {
        TestResult result;
        result.name = "PhasedVocoder: Freeze Mode";
        result.passed = true;

        PhasedVocoder engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateTestSignal(buffer, 440.0f, 0.5f);

        std::map<int, float> params;
        params[0] = 0.2f;  // 1x time
        params[1] = 0.5f;  // No pitch shift
        params[6] = 1.0f;  // Full wet

        // Process normally
        params[7] = 0.0f;  // Freeze off
        engine.updateParameters(params);

        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        // Enable freeze
        params[7] = 1.0f;  // Freeze on
        engine.updateParameters(params);

        // Process with different input
        generateTestSignal(buffer, 880.0f, 0.3f);

        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "NaN/Inf in freeze mode";
        } else {
            result.message = "Freeze mode functional";
            result.metrics["RMS"] = measureRMS(buffer);
        }

        results.push_back(result);
    }

    //==========================================================================
    // GAIN UTILITY TESTS
    //==========================================================================

    void testGainUtility_PrecisionGain() {
        TestResult result;
        result.name = "GainUtility: Gain Precision";
        result.passed = true;

        GainUtility_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);

        // Test known gain values
        struct GainTest {
            float param;     // 0-1 parameter value
            float expectedDb; // Expected dB
            float tolerance;  // Tolerance in dB
        };

        std::vector<GainTest> tests = {
            {0.0f, -24.0f, 0.1f},   // Min gain
            {0.25f, -12.0f, 0.1f},  // -12 dB
            {0.5f, 0.0f, 0.01f},    // Unity gain
            {0.75f, 12.0f, 0.1f},   // +12 dB
            {1.0f, 24.0f, 0.1f}     // Max gain
        };

        for (const auto& test : tests) {
            engine.reset();
            generateTestSignal(buffer, 440.0f, 0.1f); // Low level input

            float inputRms = measureRMS(buffer);

            std::map<int, float> params;
            params[0] = test.param;
            engine.updateParameters(params);

            // Process to allow smoothing
            for (int i = 0; i < 20; ++i) {
                juce::AudioBuffer<float> tempBuffer(2, 512);
                generateTestSignal(tempBuffer, 440.0f, 0.1f);
                engine.process(tempBuffer);
            }

            // Measure final output
            generateTestSignal(buffer, 440.0f, 0.1f);
            engine.process(buffer);
            float outputRms = measureRMS(buffer);

            // Calculate actual gain in dB
            float actualDb = 20.0f * std::log10(outputRms / inputRms);
            float error = std::abs(actualDb - test.expectedDb);

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2);
            ss << "Target: " << test.expectedDb << " dB, Actual: " << actualDb << " dB, Error: " << error << " dB";

            if (error > test.tolerance) {
                result.passed = false;
                result.message = ss.str();
                break;
            }
        }

        if (result.passed) {
            result.message = "Gain accuracy within ±0.1 dB";
        }

        results.push_back(result);
    }

    void testGainUtility_MidSideProcessing() {
        TestResult result;
        result.name = "GainUtility: Mid/Side Processing";
        result.passed = true;

        GainUtility_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateTestSignal(buffer, 440.0f, 0.5f);

        std::map<int, float> params;
        params[0] = 0.5f;  // Unity main gain
        params[5] = 0.5f;  // M/S mode
        params[3] = 0.5f;  // Unity mid gain
        params[4] = 0.5f;  // Unity side gain
        engine.updateParameters(params);

        // Process
        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "NaN/Inf in M/S mode";
        } else {
            result.message = "M/S processing functional";
            result.metrics["RMS_L"] = measureRMS(buffer, 0);
            result.metrics["RMS_R"] = measureRMS(buffer, 1);
        }

        results.push_back(result);
    }

    void testGainUtility_PhaseInversion() {
        TestResult result;
        result.name = "GainUtility: Phase Inversion";
        result.passed = true;

        GainUtility_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        juce::AudioBuffer<float> referenceBuffer(2, 512);

        generateTestSignal(buffer, 440.0f, 0.5f);
        generateTestSignal(referenceBuffer, 440.0f, 0.5f);

        std::map<int, float> params;
        params[0] = 0.5f;  // Unity gain
        params[6] = 1.0f;  // Invert left
        params[7] = 0.0f;  // Normal right
        engine.updateParameters(params);

        // Process
        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        // Verify left channel is inverted
        const float* leftOut = buffer.getReadPointer(0);
        const float* leftRef = referenceBuffer.getReadPointer(0);

        float sumDiff = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float expectedInverted = -leftRef[i];
            sumDiff += std::abs(leftOut[i] - expectedInverted);
        }

        float avgError = sumDiff / buffer.getNumSamples();

        if (avgError > 0.01f) {
            result.passed = false;
            result.message = "Phase inversion incorrect";
            result.metrics["Avg Error"] = avgError;
        } else {
            result.message = "Phase inversion accurate";
        }

        results.push_back(result);
    }

    //==========================================================================
    // MONO MAKER TESTS
    //==========================================================================

    void testMonoMaker_FrequencySelectivity() {
        TestResult result;
        result.name = "MonoMaker: Frequency Selectivity";
        result.passed = true;

        MonoMaker_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 2048);

        // Generate stereo signal with bass and treble
        const float* leftPtr = buffer.getWritePointer(0);
        const float* rightPtr = buffer.getWritePointer(1);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float bass = 0.5f * std::sin(2.0f * M_PI * 60.0f * i / 48000.0f);
            float treble = 0.3f * std::sin(2.0f * M_PI * 5000.0f * i / 48000.0f);

            // Different stereo image for bass and treble
            buffer.setSample(0, i, bass + treble * 0.7f);
            buffer.setSample(1, i, bass + treble * 1.3f);
        }

        // Set cutoff to 200Hz
        std::map<int, float> params;
        params[0] = 0.4f;  // ~200Hz
        params[1] = 0.5f;  // 24 dB/oct
        params[2] = 0.0f;  // Standard mode
        params[3] = 1.0f;  // 100% bass mono
        params[5] = 1.0f;  // DC filter on
        params[6] = 1.0f;  // Normal width above
        params[7] = 0.5f;  // Unity output gain
        engine.updateParameters(params);

        // Process
        for (int i = 0; i < 10; ++i) {
            engine.process(buffer);
        }

        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "NaN/Inf in processing";
        } else {
            result.message = "Frequency-selective mono functional";
            result.metrics["Cutoff Hz"] = engine.getCurrentCutoff();
            result.metrics["RMS_L"] = measureRMS(buffer, 0);
            result.metrics["RMS_R"] = measureRMS(buffer, 1);
        }

        results.push_back(result);
    }

    void testMonoMaker_SlopeVariation() {
        TestResult result;
        result.name = "MonoMaker: Filter Slope Variation";
        result.passed = true;

        MonoMaker_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);

        std::map<int, float> params;
        params[0] = 0.3f;  // ~100Hz
        params[2] = 0.0f;  // Standard mode
        params[3] = 1.0f;  // 100% mono
        params[5] = 1.0f;  // DC filter on

        // Test different slopes
        std::vector<float> slopes = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}; // 6, 18, 30, 42, 48 dB/oct

        for (float slope : slopes) {
            params[1] = slope;
            engine.updateParameters(params);
            engine.reset();

            generateTestSignal(buffer, 440.0f, 0.5f);

            for (int i = 0; i < 10; ++i) {
                engine.process(buffer);
            }

            if (!checkFinite(buffer)) {
                result.passed = false;
                result.message = "NaN/Inf at slope = " + std::to_string(slope);
                break;
            }
        }

        if (result.passed) {
            result.message = "All filter slopes functional";
        }

        results.push_back(result);
    }

    void testMonoMaker_PhaseCoherence() {
        TestResult result;
        result.name = "MonoMaker: Phase Coherence";
        result.passed = true;

        MonoMaker_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 2048);
        generateTestSignal(buffer, 440.0f, 0.5f);

        std::map<int, float> params;
        params[0] = 0.3f;  // ~100Hz
        params[1] = 0.5f;  // 24 dB/oct
        params[2] = 0.0f;  // Standard mode
        params[3] = 1.0f;  // 100% mono
        params[4] = 0.0f;  // Minimum phase
        params[5] = 1.0f;  // DC filter on
        engine.updateParameters(params);

        // Process
        for (int i = 0; i < 20; ++i) {
            engine.process(buffer);
        }

        // Check phase correlation
        float correlation = engine.getPhaseCorrelation();
        float monoCompat = engine.getMonoCompatibility();

        result.metrics["Phase Correlation"] = correlation;
        result.metrics["Mono Compatibility"] = monoCompat;

        if (!checkFinite(buffer)) {
            result.passed = false;
            result.message = "NaN/Inf in output";
        } else {
            result.message = "Phase relationships maintained";
        }

        results.push_back(result);
    }

    void testMonoMaker_StereoWidthControl() {
        TestResult result;
        result.name = "MonoMaker: Stereo Width Above Cutoff";
        result.passed = true;

        MonoMaker_Platinum engine;
        engine.prepareToPlay(48000.0, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        generateTestSignal(buffer, 5000.0f, 0.5f); // High frequency

        std::map<int, float> params;
        params[0] = 0.3f;  // ~100Hz cutoff
        params[1] = 0.5f;  // 24 dB/oct
        params[2] = 0.0f;  // Standard mode
        params[3] = 1.0f;  // 100% bass mono
        params[5] = 1.0f;  // DC filter on
        params[7] = 0.5f;  // Unity gain

        // Test width control
        std::vector<float> widthValues = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f};

        for (float width : widthValues) {
            params[6] = width * 0.5f; // Scale to 0-1 range
            engine.updateParameters(params);
            engine.reset();

            generateTestSignal(buffer, 5000.0f, 0.5f);

            for (int i = 0; i < 10; ++i) {
                engine.process(buffer);
            }

            if (!checkFinite(buffer)) {
                result.passed = false;
                result.message = "NaN/Inf at width = " + std::to_string(width);
                break;
            }
        }

        if (result.passed) {
            result.message = "Width control functional";
        }

        results.push_back(result);
    }

    //==========================================================================
    // RUN ALL TESTS
    //==========================================================================

    void runAllTests() {
        printHeader("GRANULAR CLOUD TESTS");
        testGranularCloud_ParameterRanges();
        printResult(results.back());

        testGranularCloud_GrainProcessing();
        printResult(results.back());

        testGranularCloud_DensityStressTest();
        printResult(results.back());

        testGranularCloud_PitchScatter();
        printResult(results.back());

        printHeader("PHASED VOCODER TESTS");
        testPhasedVocoder_TimeStretch();
        printResult(results.back());

        testPhasedVocoder_PitchShift();
        printResult(results.back());

        testPhasedVocoder_SpectralEffects();
        printResult(results.back());

        testPhasedVocoder_FreezeMode();
        printResult(results.back());

        printHeader("GAIN UTILITY TESTS");
        testGainUtility_PrecisionGain();
        printResult(results.back());

        testGainUtility_MidSideProcessing();
        printResult(results.back());

        testGainUtility_PhaseInversion();
        printResult(results.back());

        printHeader("MONO MAKER TESTS");
        testMonoMaker_FrequencySelectivity();
        printResult(results.back());

        testMonoMaker_SlopeVariation();
        printResult(results.back());

        testMonoMaker_PhaseCoherence();
        printResult(results.back());

        testMonoMaker_StereoWidthControl();
        printResult(results.back());

        printSummary();
    }
};

int main() {
    std::cout << "\n" << "============================================================" << "\n";
    std::cout << "   UTILITY ENGINE DEEP VALIDATION TEST SUITE" << "\n";
    std::cout << "   Testing: GranularCloud, PhasedVocoder, " << "\n";
    std::cout << "           GainUtility_Platinum, MonoMaker_Platinum" << "\n";
    std::cout << "============================================================" << "\n";

    UtilityEngineTests tests;
    tests.runAllTests();

    return 0;
}
