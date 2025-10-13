// ==================== DELAY ENGINE DEEP VALIDATION TEST ====================
// Comprehensive parameter validation and accuracy testing for all delay engines
#include "../JUCE_Plugin/Source/TapeEcho.h"
#include "../JUCE_Plugin/Source/DigitalDelay.h"
#include "../JUCE_Plugin/Source/MagneticDrumEcho.h"
#include "../JUCE_Plugin/Source/BucketBrigadeDelay.h"
#include "../JUCE_Plugin/Source/BufferRepeat.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>

// Test configuration
constexpr double TEST_SAMPLE_RATE = 48000.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr float IMPULSE_AMPLITUDE = 1.0f;

// Analysis utilities
struct DelayMeasurement {
    double measuredDelayMs = 0.0;
    double expectedDelayMs = 0.0;
    double errorMs = 0.0;
    double errorPercent = 0.0;
    int peakSample = 0;
    float peakAmplitude = 0.0f;
};

struct FrequencyResponse {
    std::vector<float> frequencies;
    std::vector<float> magnitudes;
    float -3dBPoint = 0.0f;
    float -6dBPoint = 0.0f;
};

struct FeedbackStability {
    bool stable = true;
    float maxPeak = 0.0f;
    float avgLevel = 0.0f;
    int oscillationStart = -1;
};

// Delay measurement function using impulse response
DelayMeasurement measureDelayTime(EngineBase* engine, float expectedMs) {
    DelayMeasurement result;
    result.expectedDelayMs = expectedMs;

    // Create impulse
    juce::AudioBuffer<float> buffer(2, 8192);
    buffer.clear();
    buffer.setSample(0, 0, IMPULSE_AMPLITUDE);
    buffer.setSample(1, 0, IMPULSE_AMPLITUDE);

    // Process
    engine->process(buffer);

    // Find peak in output (skip first few samples)
    int startSample = 10;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = startSample; i < buffer.getNumSamples(); ++i) {
            float absValue = std::abs(data[i]);
            if (absValue > result.peakAmplitude) {
                result.peakAmplitude = absValue;
                result.peakSample = i;
            }
        }
    }

    // Calculate measured delay
    result.measuredDelayMs = (result.peakSample * 1000.0) / TEST_SAMPLE_RATE;
    result.errorMs = result.measuredDelayMs - result.expectedDelayMs;
    result.errorPercent = (result.errorMs / result.expectedDelayMs) * 100.0;

    return result;
}

// Test feedback stability
FeedbackStability testFeedbackStability(EngineBase* engine, float feedbackAmount) {
    FeedbackStability result;

    // Generate test signal and run for extended period
    juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);

    // Initial impulse
    buffer.clear();
    buffer.setSample(0, 0, 0.5f);
    buffer.setSample(1, 0, 0.5f);
    engine->process(buffer);

    float runningSum = 0.0f;
    int sampleCount = 0;

    // Run for 5 seconds worth of audio
    for (int block = 0; block < (5 * TEST_SAMPLE_RATE) / TEST_BLOCK_SIZE; ++block) {
        buffer.clear();
        engine->process(buffer);

        // Check for peaks and accumulate level
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float absValue = std::abs(data[i]);
                result.maxPeak = std::max(result.maxPeak, absValue);
                runningSum += absValue;
                sampleCount++;

                // Check for oscillation (sustained level > 0.95)
                if (absValue > 0.95f && result.oscillationStart < 0) {
                    result.oscillationStart = block * TEST_BLOCK_SIZE + i;
                    result.stable = false;
                }
            }
        }

        // Early exit if unstable
        if (!result.stable && block > 10) break;
    }

    result.avgLevel = runningSum / sampleCount;
    return result;
}

// Measure frequency response through delay
FrequencyResponse measureFrequencyResponse(EngineBase* engine) {
    FrequencyResponse result;

    // Test frequencies: 20Hz to 20kHz
    std::vector<float> testFreqs = {
        20, 50, 100, 200, 500, 1000, 2000, 3000, 5000,
        7000, 10000, 12000, 15000, 18000, 20000
    };

    for (float freq : testFreqs) {
        // Generate sine wave at test frequency
        juce::AudioBuffer<float> buffer(2, 4096);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = 0.5f * std::sin(2.0f * M_PI * freq * i / TEST_SAMPLE_RATE);
            }
        }

        // Process
        engine->process(buffer);

        // Measure RMS output level
        float rmsSum = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                rmsSum += data[i] * data[i];
            }
        }
        float rms = std::sqrt(rmsSum / (buffer.getNumSamples() * buffer.getNumChannels()));

        result.frequencies.push_back(freq);
        result.magnitudes.push_back(20.0f * std::log10(rms + 1e-10f)); // dB
    }

    // Find -3dB and -6dB points
    float referenceLevel = result.magnitudes[5]; // Use 1kHz as reference
    for (size_t i = 0; i < result.frequencies.size(); ++i) {
        if (result.magnitudes[i] < referenceLevel - 3.0f && result.-3dBPoint == 0.0f) {
            result.-3dBPoint = result.frequencies[i];
        }
        if (result.magnitudes[i] < referenceLevel - 6.0f && result.-6dBPoint == 0.0f) {
            result.-6dBPoint = result.frequencies[i];
        }
    }

    return result;
}

// Test suite for each engine
void testDelayEngine(EngineBase* engine, const std::string& engineName, std::ofstream& report) {
    report << "\n" << std::string(80, '=') << "\n";
    report << "TESTING: " << engineName << "\n";
    report << std::string(80, '=') << "\n\n";

    // Prepare engine
    engine->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
    engine->reset();

    // Get parameter information
    int numParams = engine->getNumParameters();
    report << "Number of Parameters: " << numParams << "\n\n";

    report << "Parameters:\n";
    for (int i = 0; i < numParams; ++i) {
        report << "  [" << i << "] " << engine->getParameterName(i).toStdString() << "\n";
    }
    report << "\n";

    // Test 1: Delay Time Sweep (1ms - 2000ms)
    report << "TEST 1: Delay Time Accuracy\n";
    report << std::string(40, '-') << "\n";

    std::vector<float> testDelays = {10.0f, 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 1500.0f, 2000.0f};

    for (float targetMs : testDelays) {
        engine->reset();

        // Map delay time to parameter range [0, 1]
        float timeParam = (targetMs - 10.0f) / (2000.0f - 10.0f);
        std::map<int, float> params;
        params[0] = timeParam; // Assume param 0 is delay time
        params[4] = 0.0f;      // Sync off (if exists)
        params[5] = 0.0f;      // Sync off (alternate index)
        engine->updateParameters(params);

        DelayMeasurement measurement = measureDelayTime(engine, targetMs);

        report << std::fixed << std::setprecision(2);
        report << "Target: " << std::setw(8) << targetMs << "ms";
        report << " | Measured: " << std::setw(8) << measurement.measuredDelayMs << "ms";
        report << " | Error: " << std::setw(7) << measurement.errorMs << "ms";
        report << " (" << std::setw(6) << measurement.errorPercent << "%)\n";
    }

    // Test 2: Feedback Stability
    report << "\nTEST 2: Feedback Stability\n";
    report << std::string(40, '-') << "\n";

    std::vector<float> feedbackLevels = {0.0f, 0.25f, 0.5f, 0.75f, 0.9f, 0.95f, 0.98f, 0.99f};

    for (float fb : feedbackLevels) {
        engine->reset();

        std::map<int, float> params;
        params[0] = 0.25f; // Medium delay time
        params[1] = fb;    // Feedback amount
        engine->updateParameters(params);

        FeedbackStability stability = testFeedbackStability(engine, fb);

        report << "Feedback: " << std::setw(5) << (fb * 100.0f) << "%";
        report << " | Stable: " << (stability.stable ? "YES" : "NO ");
        report << " | Max Peak: " << std::setw(6) << std::setprecision(3) << stability.maxPeak;
        report << " | Avg Level: " << std::setw(6) << stability.avgLevel;
        if (!stability.stable) {
            report << " | Oscillation at sample: " << stability.oscillationStart;
        }
        report << "\n";
    }

    // Test 3: Frequency Response
    report << "\nTEST 3: Frequency Response\n";
    report << std::string(40, '-') << "\n";

    engine->reset();
    std::map<int, float> params;
    params[0] = 0.5f; // Medium delay
    params[1] = 0.0f; // No feedback
    engine->updateParameters(params);

    FrequencyResponse freqResp = measureFrequencyResponse(engine);

    report << std::setprecision(1);
    for (size_t i = 0; i < freqResp.frequencies.size(); ++i) {
        report << std::setw(7) << freqResp.frequencies[i] << " Hz: ";
        report << std::setw(7) << std::setprecision(2) << freqResp.magnitudes[i] << " dB\n";
    }

    if (freqResp.-3dBPoint > 0) {
        report << "\n-3dB Point: " << freqResp.-3dBPoint << " Hz\n";
    }
    if (freqResp.-6dBPoint > 0) {
        report << "-6dB Point: " << freqResp.-6dBPoint << " Hz\n";
    }

    // Test 4: Parameter Response
    report << "\nTEST 4: Parameter Response Test\n";
    report << std::string(40, '-') << "\n";

    for (int paramIdx = 0; paramIdx < numParams; ++paramIdx) {
        engine->reset();

        // Test parameter at min, mid, max
        std::vector<float> testValues = {0.0f, 0.5f, 1.0f};

        report << "Parameter [" << paramIdx << "] "
               << engine->getParameterName(paramIdx).toStdString() << ":\n";

        for (float value : testValues) {
            std::map<int, float> testParams;
            testParams[paramIdx] = value;
            engine->updateParameters(testParams);

            // Process a block and measure output
            juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < TEST_BLOCK_SIZE; ++i) {
                    data[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / TEST_SAMPLE_RATE);
                }
            }

            engine->process(buffer);

            // Measure RMS output
            float rms = buffer.getRMSLevel(0, 0, TEST_BLOCK_SIZE);

            report << "  Value: " << std::setw(4) << value
                   << " | RMS Output: " << std::setw(8) << std::setprecision(6) << rms << "\n";
        }
    }

    report << "\n";
}

int main() {
    std::cout << "Starting Deep Delay Engine Validation...\n\n";

    // Open report file
    std::ofstream report("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/DELAY_PARAMETER_VALIDATION_REPORT.md");

    // Write header
    report << "# DELAY ENGINE DEEP VALIDATION REPORT\n\n";
    report << "**Test Date:** " << juce::Time::getCurrentTime().toString(true, true).toStdString() << "\n";
    report << "**Sample Rate:** " << TEST_SAMPLE_RATE << " Hz\n";
    report << "**Block Size:** " << TEST_BLOCK_SIZE << " samples\n";
    report << "**Test Duration:** ~30 seconds per engine\n\n";

    report << "## Test Methodology\n\n";
    report << "1. **Delay Time Accuracy**: Impulse response measurement\n";
    report << "2. **Feedback Stability**: 5-second stability analysis\n";
    report << "3. **Frequency Response**: Sine sweep 20Hz-20kHz\n";
    report << "4. **Parameter Response**: Full parameter range validation\n\n";

    // Test each engine
    try {
        std::cout << "Testing TapeEcho...\n";
        auto tapeEcho = std::make_unique<TapeEcho>();
        testDelayEngine(tapeEcho.get(), "Tape Echo", report);
    } catch (const std::exception& e) {
        report << "ERROR testing TapeEcho: " << e.what() << "\n\n";
    }

    try {
        std::cout << "Testing DigitalDelay...\n";
        auto digitalDelay = std::make_unique<AudioDSP::DigitalDelay>();
        testDelayEngine(digitalDelay.get(), "Digital Delay Pro", report);
    } catch (const std::exception& e) {
        report << "ERROR testing DigitalDelay: " << e.what() << "\n\n";
    }

    try {
        std::cout << "Testing MagneticDrumEcho...\n";
        auto drumEcho = std::make_unique<MagneticDrumEcho>();
        testDelayEngine(drumEcho.get(), "Magnetic Drum Echo", report);
    } catch (const std::exception& e) {
        report << "ERROR testing MagneticDrumEcho: " << e.what() << "\n\n";
    }

    try {
        std::cout << "Testing BucketBrigadeDelay...\n";
        auto bbdDelay = std::make_unique<BucketBrigadeDelay>();
        testDelayEngine(bbdDelay.get(), "Bucket Brigade Delay", report);
    } catch (const std::exception& e) {
        report << "ERROR testing BucketBrigadeDelay: " << e.what() << "\n\n";
    }

    try {
        std::cout << "Testing BufferRepeat...\n";
        auto bufferRepeat = std::make_unique<BufferRepeat>();
        testDelayEngine(bufferRepeat.get(), "Buffer Repeat", report);
    } catch (const std::exception& e) {
        report << "ERROR testing BufferRepeat: " << e.what() << "\n\n";
    }

    // Write summary
    report << "\n" << std::string(80, '=') << "\n";
    report << "VALIDATION COMPLETE\n";
    report << std::string(80, '=') << "\n";

    report.close();

    std::cout << "\nValidation complete! Report saved to DELAY_PARAMETER_VALIDATION_REPORT.md\n";

    return 0;
}
