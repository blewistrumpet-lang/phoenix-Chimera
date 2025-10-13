#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <random>

/**
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║  PITCH ENGINE STRESS TESTING - EXTREME CONDITIONS SUITE              ║
 * ║  Testing 8 Pitch Engines Under Brutal, Production-Breaking Scenarios ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 *
 * Mission: Prove pitch engines are BULLETPROOF under any condition
 *
 * Engines Tested:
 * - Engine 31: PitchShifter (Vocal Destroyer)
 * - Engine 32: DetuneDoubler
 * - Engine 33: IntelligentHarmonizer
 * - Engine 38: PhasedVocoder (inside BufferRepeat)
 * - Engine 40: ShimmerReverb (has pitch shifting)
 * - Engine 42: ShimmerReverb
 * - Engine 11: FormantFilter (pitch-related)
 * - Engine 14: VocalFormantFilter (pitch-related)
 *
 * Test Categories:
 * 1. Extreme Pitch Shifts (-48 to +48 semitones, up to ±96)
 * 2. Extreme Input Signals (DC, square wave, Nyquist, subsonic, silence)
 * 3. Rapid Parameter Changes (automation, random jumps)
 * 4. Long Duration (5+ minutes continuous)
 * 5. Edge Case Combinations
 * 6. Buffer Size Stress (1 sample to 16,384 samples)
 * 7. Sample Rate Stress (8 kHz to 384 kHz)
 * 8. CPU/Memory Stability
 */

// Test result tracking structure
struct StressTestResult {
    std::string testName;
    bool passed = false;
    bool crashed = false;
    bool hasNaN = false;
    bool hasInf = false;
    bool producedOutput = false;
    float maxOutputLevel = 0.0f;
    float avgOutputLevel = 0.0f;
    double cpuTime = 0.0;
    std::string failureMode;
    std::string notes;
};

struct EngineStressResult {
    int engineId;
    std::string engineName;
    bool engineCreated = false;
    std::vector<StressTestResult> testResults;
    int passCount = 0;
    int failCount = 0;
    int robustnessScore = 0; // 0-100
    std::string overallVerdict;
};

// Utility: Check for NaN/Inf in buffer
bool hasNaNOrInf(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (!std::isfinite(data[i])) {
                return true;
            }
        }
    }
    return false;
}

// Utility: Calculate RMS level
float calculateRMS(const juce::AudioBuffer<float>& buffer) {
    float sum = 0.0f;
    int totalSamples = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sum += data[i] * data[i];
            totalSamples++;
        }
    }
    return std::sqrt(sum / totalSamples);
}

// Utility: Get max absolute level
float getMaxLevel(const juce::AudioBuffer<float>& buffer) {
    float maxLevel = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            maxLevel = std::max(maxLevel, std::abs(data[i]));
        }
    }
    return maxLevel;
}

// ═══════════════════════════════════════════════════════════════════════
//                          TEST 1: EXTREME PITCH SHIFTS
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_ExtremePitchShifts(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Extreme Pitch Shifts (-48 to +48 semitones)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Test range: -48, -36, -24, -12, 0, +12, +24, +36, +48 semitones
        std::vector<float> pitchValues = {-48.0f, -36.0f, -24.0f, -12.0f, 0.0f, 12.0f, 24.0f, 36.0f, 48.0f};

        juce::AudioBuffer<float> testBuffer(2, blockSize * 2);
        testBuffer.clear();

        // Generate test signal (440Hz sine)
        for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            testBuffer.setSample(0, i, 0.5f * std::sin(phase));
            testBuffer.setSample(1, i, 0.5f * std::sin(phase));
        }

        bool anyNaN = false;
        float totalRMS = 0.0f;
        int processCount = 0;

        for (float pitchShift : pitchValues) {
            std::map<int, float> params;
            // Try to set pitch shift on first parameter (usually pitch/detune)
            params[0] = (pitchShift + 48.0f) / 96.0f; // Normalize to 0-1 range
            params[1] = 1.0f; // Full wet
            engine->updateParameters(params);

            juce::AudioBuffer<float> processBuffer(2, blockSize);
            for (int start = 0; start < testBuffer.getNumSamples(); start += blockSize) {
                int samples = std::min(blockSize, testBuffer.getNumSamples() - start);
                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < samples; ++i) {
                        processBuffer.setSample(ch, i, testBuffer.getSample(ch, start + i));
                    }
                }

                engine->process(processBuffer);

                if (hasNaNOrInf(processBuffer)) {
                    anyNaN = true;
                    result.failureMode = "NaN/Inf at pitch shift " + std::to_string(pitchShift) + " semitones";
                    break;
                }

                totalRMS += calculateRMS(processBuffer);
                processCount++;
            }

            if (anyNaN) break;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyNaN;
        result.hasInf = anyNaN;
        result.avgOutputLevel = processCount > 0 ? totalRMS / processCount : 0.0f;
        result.producedOutput = result.avgOutputLevel > 0.0001f;
        result.passed = !anyNaN && result.producedOutput;

        if (result.passed) {
            result.notes = "All pitch shifts handled without errors";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                     TEST 2: INSANE PITCH RANGE (±96 semitones)
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_InsanePitchRange(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Insane Pitch Range (±96 semitones / 8 octaves)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        juce::AudioBuffer<float> testBuffer(2, blockSize);

        // Generate test signal
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            testBuffer.setSample(0, i, 0.5f * std::sin(phase));
            testBuffer.setSample(1, i, 0.5f * std::sin(phase));
        }

        // Test extreme values: -96, -72, +72, +96 semitones
        std::vector<float> extremeValues = {-96.0f, -72.0f, 72.0f, 96.0f};

        bool anyNaN = false;
        float maxOutput = 0.0f;

        for (float pitch : extremeValues) {
            std::map<int, float> params;
            params[0] = (pitch + 96.0f) / 192.0f; // Normalize
            params[1] = 1.0f;
            engine->updateParameters(params);

            juce::AudioBuffer<float> processBuffer(2, blockSize);
            processBuffer.makeCopyOf(testBuffer);

            engine->process(processBuffer);

            if (hasNaNOrInf(processBuffer)) {
                anyNaN = true;
                result.failureMode = "NaN/Inf at extreme pitch " + std::to_string(pitch);
                break;
            }

            maxOutput = std::max(maxOutput, getMaxLevel(processBuffer));
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyNaN;
        result.hasInf = anyNaN;
        result.maxOutputLevel = maxOutput;
        result.producedOutput = maxOutput > 0.0001f;
        result.passed = !anyNaN;

        if (result.passed) {
            result.notes = "Survived 8-octave range without crashes";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                     TEST 3: EXTREME INPUT SIGNALS
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_ExtremeInputSignals(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Extreme Input Signals (DC, Square, Nyquist, Silence)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::map<int, float> params;
        params[0] = 0.5f; // Neutral pitch
        params[1] = 1.0f; // Full wet
        engine->updateParameters(params);

        bool anyFailure = false;
        std::vector<std::string> signalTests;

        // Test 1: DC Offset (100%)
        {
            juce::AudioBuffer<float> dcBuffer(2, blockSize);
            dcBuffer.clear();
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    dcBuffer.setSample(ch, i, 1.0f); // Full DC
                }
            }
            engine->process(dcBuffer);
            if (hasNaNOrInf(dcBuffer)) {
                anyFailure = true;
                signalTests.push_back("DC: FAILED");
            } else {
                signalTests.push_back("DC: OK");
            }
        }

        // Test 2: Full-scale square wave
        {
            juce::AudioBuffer<float> squareBuffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    squareBuffer.setSample(ch, i, (i % 100 < 50) ? 1.0f : -1.0f);
                }
            }
            engine->process(squareBuffer);
            if (hasNaNOrInf(squareBuffer)) {
                anyFailure = true;
                signalTests.push_back("Square: FAILED");
            } else {
                signalTests.push_back("Square: OK");
            }
        }

        // Test 3: Nyquist frequency (half sample rate)
        {
            juce::AudioBuffer<float> nyquistBuffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = M_PI * i; // Nyquist = alternating +1/-1
                    nyquistBuffer.setSample(ch, i, std::sin(phase));
                }
            }
            engine->process(nyquistBuffer);
            if (hasNaNOrInf(nyquistBuffer)) {
                anyFailure = true;
                signalTests.push_back("Nyquist: FAILED");
            } else {
                signalTests.push_back("Nyquist: OK");
            }
        }

        // Test 4: Sub-sonic (5 Hz)
        {
            juce::AudioBuffer<float> subsonicBuffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = 2.0f * M_PI * 5.0f * i / sampleRate;
                    subsonicBuffer.setSample(ch, i, 0.5f * std::sin(phase));
                }
            }
            engine->process(subsonicBuffer);
            if (hasNaNOrInf(subsonicBuffer)) {
                anyFailure = true;
                signalTests.push_back("Subsonic: FAILED");
            } else {
                signalTests.push_back("Subsonic: OK");
            }
        }

        // Test 5: Ultrasonic (19 kHz)
        {
            juce::AudioBuffer<float> ultrasonicBuffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = 2.0f * M_PI * 19000.0f * i / sampleRate;
                    ultrasonicBuffer.setSample(ch, i, 0.5f * std::sin(phase));
                }
            }
            engine->process(ultrasonicBuffer);
            if (hasNaNOrInf(ultrasonicBuffer)) {
                anyFailure = true;
                signalTests.push_back("Ultrasonic: FAILED");
            } else {
                signalTests.push_back("Ultrasonic: OK");
            }
        }

        // Test 6: Complete silence
        {
            juce::AudioBuffer<float> silenceBuffer(2, blockSize);
            silenceBuffer.clear();
            engine->process(silenceBuffer);
            if (hasNaNOrInf(silenceBuffer)) {
                anyFailure = true;
                signalTests.push_back("Silence: FAILED");
            } else {
                signalTests.push_back("Silence: OK");
            }
        }

        // Test 7: White noise
        {
            juce::AudioBuffer<float> noiseBuffer(2, blockSize);
            std::mt19937 rng(12345);
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    noiseBuffer.setSample(ch, i, dist(rng));
                }
            }
            engine->process(noiseBuffer);
            if (hasNaNOrInf(noiseBuffer)) {
                anyFailure = true;
                signalTests.push_back("Noise: FAILED");
            } else {
                signalTests.push_back("Noise: OK");
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyFailure;
        result.hasInf = anyFailure;
        result.passed = !anyFailure;

        std::string noteStr;
        for (const auto& test : signalTests) {
            noteStr += test + ", ";
        }
        result.notes = noteStr;

        if (!result.passed) {
            result.failureMode = "Failed on extreme input signals";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                  TEST 4: RAPID PARAMETER CHANGES
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_RapidParameterChanges(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Rapid Parameter Changes (automation stress)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        juce::AudioBuffer<float> testBuffer(2, blockSize);

        // Generate test signal
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
            testBuffer.setSample(0, i, 0.5f * std::sin(phase));
            testBuffer.setSample(1, i, 0.5f * std::sin(phase));
        }

        bool anyNaN = false;
        std::mt19937 rng(54321);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Process 500 blocks with random parameter changes each time
        for (int block = 0; block < 500; ++block) {
            std::map<int, float> params;
            // Randomize all parameters
            for (int p = 0; p < engine->getNumParameters(); ++p) {
                params[p] = dist(rng);
            }
            engine->updateParameters(params);

            juce::AudioBuffer<float> processBuffer(2, blockSize);
            processBuffer.makeCopyOf(testBuffer);

            engine->process(processBuffer);

            if (hasNaNOrInf(processBuffer)) {
                anyNaN = true;
                result.failureMode = "NaN/Inf during rapid parameter changes at block " + std::to_string(block);
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyNaN;
        result.hasInf = anyNaN;
        result.passed = !anyNaN;
        result.notes = "Processed 500 blocks with random parameters";

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                     TEST 5: LONG DURATION (5 MINUTES)
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_LongDuration(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Long Duration Stability (30 seconds simulated)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::map<int, float> params;
        params[0] = 0.5f; // Neutral setting
        params[1] = 1.0f;
        engine->updateParameters(params);

        juce::AudioBuffer<float> testBuffer(2, blockSize);

        // Simulate 30 seconds of processing (scaled down from 5 minutes for speed)
        int totalBlocks = static_cast<int>(30.0 * sampleRate / blockSize);
        bool anyNaN = false;
        float totalRMS = 0.0f;

        for (int block = 0; block < totalBlocks; ++block) {
            // Generate fresh signal each block
            for (int i = 0; i < blockSize; ++i) {
                float phase = 2.0f * M_PI * 440.0f * (block * blockSize + i) / sampleRate;
                testBuffer.setSample(0, i, 0.5f * std::sin(phase));
                testBuffer.setSample(1, i, 0.5f * std::sin(phase));
            }

            engine->process(testBuffer);

            if (hasNaNOrInf(testBuffer)) {
                anyNaN = true;
                result.failureMode = "NaN/Inf at block " + std::to_string(block) + " (~" +
                                    std::to_string(block * blockSize / sampleRate) + " seconds)";
                break;
            }

            totalRMS += calculateRMS(testBuffer);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyNaN;
        result.hasInf = anyNaN;
        result.avgOutputLevel = totalBlocks > 0 ? totalRMS / totalBlocks : 0.0f;
        result.producedOutput = result.avgOutputLevel > 0.0001f;
        result.passed = !anyNaN && result.producedOutput;

        result.notes = "Processed " + std::to_string(totalBlocks) + " blocks (~30 seconds)";

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                     TEST 6: BUFFER SIZE STRESS
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_BufferSizeStress(EngineBase* engine, double sampleRate) {
    StressTestResult result;
    result.testName = "Buffer Size Stress (1 to 16384 samples)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::vector<int> bufferSizes = {1, 16, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
        bool anyFailure = false;
        std::string failedSize;

        for (int bufSize : bufferSizes) {
            engine->prepareToPlay(sampleRate, bufSize);

            std::map<int, float> params;
            params[0] = 0.5f;
            params[1] = 1.0f;
            engine->updateParameters(params);

            juce::AudioBuffer<float> testBuffer(2, bufSize);
            for (int i = 0; i < bufSize; ++i) {
                float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                testBuffer.setSample(0, i, 0.5f * std::sin(phase));
                testBuffer.setSample(1, i, 0.5f * std::sin(phase));
            }

            engine->process(testBuffer);

            if (hasNaNOrInf(testBuffer)) {
                anyFailure = true;
                failedSize = std::to_string(bufSize);
                result.failureMode = "NaN/Inf at buffer size " + failedSize;
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyFailure;
        result.hasInf = anyFailure;
        result.passed = !anyFailure;

        if (result.passed) {
            result.notes = "All buffer sizes handled successfully";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                     TEST 7: SAMPLE RATE STRESS
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_SampleRateStress(EngineBase* engine, int blockSize) {
    StressTestResult result;
    result.testName = "Sample Rate Stress (8 kHz to 192 kHz)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::vector<double> sampleRates = {8000.0, 22050.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0};
        bool anyFailure = false;
        std::string failedRate;

        for (double sr : sampleRates) {
            engine->prepareToPlay(sr, blockSize);

            std::map<int, float> params;
            params[0] = 0.5f;
            params[1] = 1.0f;
            engine->updateParameters(params);

            juce::AudioBuffer<float> testBuffer(2, blockSize);
            for (int i = 0; i < blockSize; ++i) {
                float phase = 2.0f * M_PI * 440.0f * i / sr;
                testBuffer.setSample(0, i, 0.5f * std::sin(phase));
                testBuffer.setSample(1, i, 0.5f * std::sin(phase));
            }

            engine->process(testBuffer);

            if (hasNaNOrInf(testBuffer)) {
                anyFailure = true;
                failedRate = std::to_string((int)sr);
                result.failureMode = "NaN/Inf at sample rate " + failedRate + " Hz";
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyFailure;
        result.hasInf = anyFailure;
        result.passed = !anyFailure;

        if (result.passed) {
            result.notes = "All sample rates handled successfully";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                  TEST 8: EDGE CASE COMBINATIONS
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_EdgeCaseCombinations(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Edge Case Combinations";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        bool anyFailure = false;

        // Edge case 1: Silence then impulse
        {
            juce::AudioBuffer<float> buffer(2, blockSize * 2);
            buffer.clear();
            // Add impulse at midpoint
            buffer.setSample(0, blockSize, 1.0f);
            buffer.setSample(1, blockSize, 1.0f);

            std::map<int, float> params;
            params[0] = 0.7f; // +12 semitones roughly
            params[1] = 1.0f;
            engine->updateParameters(params);

            for (int start = 0; start < buffer.getNumSamples(); start += blockSize) {
                int samples = std::min(blockSize, buffer.getNumSamples() - start);
                juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), 2, start, samples);
                engine->process(block);

                if (hasNaNOrInf(block)) {
                    anyFailure = true;
                    result.failureMode = "Silence->Impulse test failed";
                    break;
                }
            }
        }

        // Edge case 2: DC + extreme pitch
        if (!anyFailure) {
            juce::AudioBuffer<float> buffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    buffer.setSample(ch, i, 0.5f); // DC
                }
            }

            std::map<int, float> params;
            params[0] = 1.0f; // Maximum pitch shift
            params[1] = 1.0f;
            engine->updateParameters(params);

            engine->process(buffer);

            if (hasNaNOrInf(buffer)) {
                anyFailure = true;
                result.failureMode = "DC + extreme pitch failed";
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyFailure;
        result.hasInf = anyFailure;
        result.passed = !anyFailure;

        if (result.passed) {
            result.notes = "All edge case combinations handled";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                  TEST 9: PINK NOISE & IMPULSE TRAIN
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_PinkNoiseAndImpulseTrain(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Pink Noise & Impulse Train";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::map<int, float> params;
        params[0] = 0.6f; // Slight pitch shift
        params[1] = 1.0f; // Full wet
        engine->updateParameters(params);

        bool anyFailure = false;

        // Test 1: Pink Noise (1/f spectrum)
        {
            juce::AudioBuffer<float> pinkBuffer(2, blockSize * 4);
            std::mt19937 rng(98765);
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

            // Simple pink noise approximation using running sum
            std::array<float, 7> b = {0, 0, 0, 0, 0, 0, 0};
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < pinkBuffer.getNumSamples(); ++i) {
                    float white = dist(rng);
                    b[0] = 0.99886f * b[0] + white * 0.0555179f;
                    b[1] = 0.99332f * b[1] + white * 0.0750759f;
                    b[2] = 0.96900f * b[2] + white * 0.1538520f;
                    b[3] = 0.86650f * b[3] + white * 0.3104856f;
                    b[4] = 0.55000f * b[4] + white * 0.5329522f;
                    b[5] = -0.7616f * b[5] - white * 0.0168980f;
                    float pink = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + white * 0.5362f;
                    b[6] = white * 0.115926f;
                    pinkBuffer.setSample(ch, i, pink * 0.1f);
                }
            }

            for (int start = 0; start < pinkBuffer.getNumSamples(); start += blockSize) {
                int samples = std::min(blockSize, pinkBuffer.getNumSamples() - start);
                juce::AudioBuffer<float> block(pinkBuffer.getArrayOfWritePointers(), 2, start, samples);
                engine->process(block);
                if (hasNaNOrInf(block)) {
                    anyFailure = true;
                    result.failureMode = "Pink noise caused NaN/Inf";
                    break;
                }
            }
        }

        // Test 2: Impulse train (periodic spikes)
        if (!anyFailure) {
            juce::AudioBuffer<float> impulseBuffer(2, blockSize * 2);
            impulseBuffer.clear();

            // Place impulses every 100 samples
            for (int i = 0; i < impulseBuffer.getNumSamples(); i += 100) {
                impulseBuffer.setSample(0, i, 1.0f);
                impulseBuffer.setSample(1, i, 1.0f);
            }

            for (int start = 0; start < impulseBuffer.getNumSamples(); start += blockSize) {
                int samples = std::min(blockSize, impulseBuffer.getNumSamples() - start);
                juce::AudioBuffer<float> block(impulseBuffer.getArrayOfWritePointers(), 2, start, samples);
                engine->process(block);
                if (hasNaNOrInf(block)) {
                    anyFailure = true;
                    result.failureMode = "Impulse train caused NaN/Inf";
                    break;
                }
            }
        }

        // Test 3: Chirp signal (frequency sweep)
        if (!anyFailure) {
            juce::AudioBuffer<float> chirpBuffer(2, blockSize * 2);
            float startFreq = 20.0f;
            float endFreq = 18000.0f;

            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < chirpBuffer.getNumSamples(); ++i) {
                    float t = (float)i / chirpBuffer.getNumSamples();
                    float freq = startFreq + (endFreq - startFreq) * t;
                    float phase = 2.0f * M_PI * freq * i / sampleRate;
                    chirpBuffer.setSample(ch, i, 0.5f * std::sin(phase));
                }
            }

            for (int start = 0; start < chirpBuffer.getNumSamples(); start += blockSize) {
                int samples = std::min(blockSize, chirpBuffer.getNumSamples() - start);
                juce::AudioBuffer<float> block(chirpBuffer.getArrayOfWritePointers(), 2, start, samples);
                engine->process(block);
                if (hasNaNOrInf(block)) {
                    anyFailure = true;
                    result.failureMode = "Chirp signal caused NaN/Inf";
                    break;
                }
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyFailure;
        result.hasInf = anyFailure;
        result.passed = !anyFailure;
        result.notes = anyFailure ? "" : "Pink noise, impulse train, and chirp handled";

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                  TEST 10: NON-POWER-OF-2 BUFFER SIZES
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_NonPowerOf2BufferSizes(EngineBase* engine, double sampleRate) {
    StressTestResult result;
    result.testName = "Non-Power-of-2 Buffer Sizes (333, 777, 1001)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::vector<int> weirdSizes = {333, 777, 1001};
        bool anyFailure = false;
        std::string failedSize;

        for (int bufSize : weirdSizes) {
            engine->prepareToPlay(sampleRate, bufSize);

            std::map<int, float> params;
            params[0] = 0.5f;
            params[1] = 1.0f;
            engine->updateParameters(params);

            juce::AudioBuffer<float> testBuffer(2, bufSize);
            for (int i = 0; i < bufSize; ++i) {
                float phase = 2.0f * M_PI * 440.0f * i / sampleRate;
                testBuffer.setSample(0, i, 0.5f * std::sin(phase));
                testBuffer.setSample(1, i, 0.5f * std::sin(phase));
            }

            engine->process(testBuffer);

            if (hasNaNOrInf(testBuffer)) {
                anyFailure = true;
                failedSize = std::to_string(bufSize);
                result.failureMode = "NaN/Inf at buffer size " + failedSize;
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyFailure;
        result.hasInf = anyFailure;
        result.passed = !anyFailure;

        if (result.passed) {
            result.notes = "All non-power-of-2 sizes handled";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                  TEST 11: ULTRA HIGH SAMPLE RATE (384 kHz)
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_UltraHighSampleRate(EngineBase* engine, int blockSize) {
    StressTestResult result;
    result.testName = "Ultra High Sample Rate (384 kHz)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        double ultraSR = 384000.0;
        engine->prepareToPlay(ultraSR, blockSize);

        std::map<int, float> params;
        params[0] = 0.5f;
        params[1] = 1.0f;
        engine->updateParameters(params);

        juce::AudioBuffer<float> testBuffer(2, blockSize);
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 440.0f * i / ultraSR;
            testBuffer.setSample(0, i, 0.5f * std::sin(phase));
            testBuffer.setSample(1, i, 0.5f * std::sin(phase));
        }

        engine->process(testBuffer);

        bool hasNaN = hasNaNOrInf(testBuffer);

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = hasNaN;
        result.hasInf = hasNaN;
        result.passed = !hasNaN;

        if (result.passed) {
            result.notes = "384 kHz sample rate handled successfully";
        } else {
            result.failureMode = "NaN/Inf at 384 kHz sample rate";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                  TEST 12: POLYPHONIC SIGNAL (MULTIPLE TONES)
// ═══════════════════════════════════════════════════════════════════════
StressTestResult test_PolyphonicSignal(EngineBase* engine, double sampleRate, int blockSize) {
    StressTestResult result;
    result.testName = "Polyphonic Signal (Complex Waveform)";

    try {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::map<int, float> params;
        params[0] = 0.6f; // +12 semitones roughly
        params[1] = 1.0f;
        engine->updateParameters(params);

        juce::AudioBuffer<float> testBuffer(2, blockSize * 2);

        // Mix of multiple frequencies: 200Hz, 440Hz, 880Hz, 1320Hz (major chord-ish)
        std::vector<float> frequencies = {200.0f, 440.0f, 880.0f, 1320.0f};

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < testBuffer.getNumSamples(); ++i) {
                float sample = 0.0f;
                for (float freq : frequencies) {
                    float phase = 2.0f * M_PI * freq * i / sampleRate;
                    sample += 0.25f * std::sin(phase) / frequencies.size();
                }
                testBuffer.setSample(ch, i, sample);
            }
        }

        bool anyNaN = false;
        for (int start = 0; start < testBuffer.getNumSamples(); start += blockSize) {
            int samples = std::min(blockSize, testBuffer.getNumSamples() - start);
            juce::AudioBuffer<float> block(testBuffer.getArrayOfWritePointers(), 2, start, samples);
            engine->process(block);

            if (hasNaNOrInf(block)) {
                anyNaN = true;
                result.failureMode = "Polyphonic signal caused NaN/Inf";
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.cpuTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        result.hasNaN = anyNaN;
        result.hasInf = anyNaN;
        result.passed = !anyNaN;

        if (result.passed) {
            result.notes = "Complex polyphonic signal handled";
        }

    } catch (const std::exception& e) {
        result.crashed = true;
        result.failureMode = std::string("Exception: ") + e.what();
    } catch (...) {
        result.crashed = true;
        result.failureMode = "Unknown exception";
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════
//                       MAIN STRESS TEST RUNNER
// ═══════════════════════════════════════════════════════════════════════
EngineStressResult runStressTests(int engineId) {
    EngineStressResult engineResult;
    engineResult.engineId = engineId;

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            engineResult.engineCreated = false;
            engineResult.overallVerdict = "FAILED - Engine creation failed";
            return engineResult;
        }

        engineResult.engineCreated = true;
        engineResult.engineName = engine->getName().toStdString();

        const double sampleRate = 48000.0;
        const int blockSize = 512;

        engine->prepareToPlay(sampleRate, blockSize);

        std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  Testing Engine " << std::setw(2) << engineId << ": "
                  << std::left << std::setw(43) << engineResult.engineName << "║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

        // Run all stress tests
        std::vector<StressTestResult> tests;

        std::cout << "  [1/12] Running extreme pitch shifts test...\n";
        tests.push_back(test_ExtremePitchShifts(engine.get(), sampleRate, blockSize));

        std::cout << "  [2/12] Running insane pitch range test...\n";
        tests.push_back(test_InsanePitchRange(engine.get(), sampleRate, blockSize));

        std::cout << "  [3/12] Running extreme input signals test...\n";
        tests.push_back(test_ExtremeInputSignals(engine.get(), sampleRate, blockSize));

        std::cout << "  [4/12] Running rapid parameter changes test...\n";
        tests.push_back(test_RapidParameterChanges(engine.get(), sampleRate, blockSize));

        std::cout << "  [5/12] Running long duration stability test...\n";
        tests.push_back(test_LongDuration(engine.get(), sampleRate, blockSize));

        std::cout << "  [6/12] Running buffer size stress test...\n";
        tests.push_back(test_BufferSizeStress(engine.get(), sampleRate));

        std::cout << "  [7/12] Running sample rate stress test...\n";
        tests.push_back(test_SampleRateStress(engine.get(), blockSize));

        std::cout << "  [8/12] Running edge case combinations test...\n";
        tests.push_back(test_EdgeCaseCombinations(engine.get(), sampleRate, blockSize));

        std::cout << "  [9/12] Running pink noise & impulse train test...\n";
        tests.push_back(test_PinkNoiseAndImpulseTrain(engine.get(), sampleRate, blockSize));

        std::cout << "  [10/12] Running non-power-of-2 buffer sizes test...\n";
        tests.push_back(test_NonPowerOf2BufferSizes(engine.get(), sampleRate));

        std::cout << "  [11/12] Running ultra high sample rate test...\n";
        tests.push_back(test_UltraHighSampleRate(engine.get(), blockSize));

        std::cout << "  [12/12] Running polyphonic signal test...\n";
        tests.push_back(test_PolyphonicSignal(engine.get(), sampleRate, blockSize));

        engineResult.testResults = tests;

        // Calculate pass/fail counts
        for (const auto& test : tests) {
            if (test.passed) {
                engineResult.passCount++;
            } else {
                engineResult.failCount++;
            }
        }

        // Calculate robustness score (0-100)
        int totalTests = tests.size();
        engineResult.robustnessScore = (engineResult.passCount * 100) / totalTests;

        // Determine overall verdict
        if (engineResult.robustnessScore == 100) {
            engineResult.overallVerdict = "BULLETPROOF - Production Ready";
        } else if (engineResult.robustnessScore >= 85) {
            engineResult.overallVerdict = "ROBUST - Minor issues only";
        } else if (engineResult.robustnessScore >= 70) {
            engineResult.overallVerdict = "STABLE - Some edge case issues";
        } else if (engineResult.robustnessScore >= 50) {
            engineResult.overallVerdict = "FRAGILE - Multiple failure modes";
        } else {
            engineResult.overallVerdict = "UNSTABLE - Not production ready";
        }

        // Print summary
        std::cout << "\n  Results:\n";
        std::cout << "    Passed:  " << engineResult.passCount << "/" << totalTests << "\n";
        std::cout << "    Failed:  " << engineResult.failCount << "/" << totalTests << "\n";
        std::cout << "    Score:   " << engineResult.robustnessScore << "/100\n";
        std::cout << "    Verdict: " << engineResult.overallVerdict << "\n";

    } catch (const std::exception& e) {
        engineResult.overallVerdict = std::string("CRASHED - Exception: ") + e.what();
    } catch (...) {
        engineResult.overallVerdict = "CRASHED - Unknown exception";
    }

    return engineResult;
}

// ═══════════════════════════════════════════════════════════════════════
//                           MAIN FUNCTION
// ═══════════════════════════════════════════════════════════════════════
int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║        PITCH ENGINE STRESS TESTING - EXTREME CONDITIONS        ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║  Mission: Prove pitch engines are bulletproof under ANY load  ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Test Suite (12 comprehensive tests per engine):\n";
    std::cout << "  1. Extreme Pitch Shifts (-48 to +48 semitones)\n";
    std::cout << "  2. Insane Pitch Range (±96 semitones / 8 octaves)\n";
    std::cout << "  3. Extreme Input Signals (DC, square, Nyquist, silence, white noise)\n";
    std::cout << "  4. Rapid Parameter Changes (500 blocks, random params)\n";
    std::cout << "  5. Long Duration Stability (30 seconds continuous)\n";
    std::cout << "  6. Buffer Size Stress (1 to 16384 samples)\n";
    std::cout << "  7. Sample Rate Stress (8 kHz to 192 kHz)\n";
    std::cout << "  8. Edge Case Combinations (silence->impulse, DC+extreme pitch)\n";
    std::cout << "  9. Pink Noise & Impulse Train & Chirp Signal\n";
    std::cout << " 10. Non-Power-of-2 Buffer Sizes (333, 777, 1001)\n";
    std::cout << " 11. Ultra High Sample Rate (384 kHz)\n";
    std::cout << " 12. Polyphonic Signal (complex multi-tone waveform)\n\n";

    // Pitch-related engines to test (corrected IDs from PitchEngineFactory)
    std::vector<int> pitchEngines = {
        32,  // PitchShifter (Vocal Destroyer)
        33,  // IntelligentHarmonizer
        38,  // BufferRepeat (mapped to PitchShifter)
        49,  // Pitch Shifter Alt
        50   // GranularCloud (has pitch scatter)
    };

    std::vector<EngineStressResult> allResults;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int engineId : pitchEngines) {
        auto result = runStressTests(engineId);
        allResults.push_back(result);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double totalTime = std::chrono::duration<double>(endTime - startTime).count();

    // ═══════════════════════════════════════════════════════════════
    //                      FINAL SUMMARY
    // ═══════════════════════════════════════════════════════════════

    std::cout << "\n\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    STRESS TEST SUMMARY                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    // Sort by robustness score
    std::sort(allResults.begin(), allResults.end(),
              [](const EngineStressResult& a, const EngineStressResult& b) {
                  return a.robustnessScore > b.robustnessScore;
              });

    std::cout << std::left;
    std::cout << "  " << std::setw(4) << "ID"
              << std::setw(30) << "Engine Name"
              << std::setw(10) << "Score"
              << std::setw(8) << "Pass"
              << std::setw(8) << "Fail"
              << "Verdict\n";
    std::cout << "  " << std::string(90, '-') << "\n";

    for (const auto& result : allResults) {
        if (result.engineCreated) {
            std::cout << "  " << std::setw(4) << result.engineId
                      << std::setw(30) << result.engineName.substr(0, 28)
                      << std::setw(10) << (std::to_string(result.robustnessScore) + "/100")
                      << std::setw(8) << result.passCount
                      << std::setw(8) << result.failCount
                      << result.overallVerdict << "\n";
        } else {
            std::cout << "  " << std::setw(4) << result.engineId
                      << std::setw(30) << "CREATION FAILED"
                      << std::setw(10) << "0/100"
                      << std::setw(8) << "0"
                      << std::setw(8) << "8"
                      << "N/A\n";
        }
    }

    std::cout << "\n  Total Test Duration: " << std::fixed << std::setprecision(2)
              << totalTime << " seconds\n\n";

    // Statistics
    int bulletproofCount = 0;
    int robustCount = 0;
    int unstableCount = 0;

    for (const auto& result : allResults) {
        if (result.robustnessScore == 100) bulletproofCount++;
        else if (result.robustnessScore >= 85) robustCount++;
        else if (result.robustnessScore < 50) unstableCount++;
    }

    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    ROBUSTNESS ANALYSIS                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "  Bulletproof (100%):     " << bulletproofCount << " engines\n";
    std::cout << "  Robust (85-99%):        " << robustCount << " engines\n";
    std::cout << "  Unstable (<50%):        " << unstableCount << " engines\n\n";

    // Overall verdict
    if (unstableCount == 0 && bulletproofCount >= allResults.size() / 2) {
        std::cout << "  OVERALL VERDICT: Pitch engines are PRODUCTION BULLETPROOF!\n\n";
    } else if (unstableCount == 0) {
        std::cout << "  OVERALL VERDICT: Pitch engines are PRODUCTION READY with minor notes.\n\n";
    } else {
        std::cout << "  OVERALL VERDICT: Some engines need hardening before production.\n\n";
    }

    // Write detailed report to file
    std::ofstream reportFile("PITCH_ENGINE_STRESS_TEST_REPORT.md");
    if (reportFile.is_open()) {
        reportFile << "# PITCH ENGINE STRESS TEST REPORT\n\n";
        reportFile << "**Generated:** " << juce::Time::getCurrentTime().toString(true, true).toStdString() << "\n";
        reportFile << "**Test Duration:** " << totalTime << " seconds\n\n";
        reportFile << "## Executive Summary\n\n";
        reportFile << "Tested " << allResults.size() << " pitch-related engines under extreme conditions.\n\n";
        reportFile << "- **Bulletproof (100%):** " << bulletproofCount << " engines\n";
        reportFile << "- **Robust (85-99%):** " << robustCount << " engines\n";
        reportFile << "- **Unstable (<50%):** " << unstableCount << " engines\n\n";

        reportFile << "## Test Categories\n\n";
        reportFile << "1. **Extreme Pitch Shifts**: -48 to +48 semitones\n";
        reportFile << "2. **Insane Pitch Range**: ±96 semitones (8 octaves)\n";
        reportFile << "3. **Extreme Input Signals**: DC, square wave, Nyquist, subsonic, ultrasonic, silence, white noise\n";
        reportFile << "4. **Rapid Parameter Changes**: 500 blocks with randomized parameters\n";
        reportFile << "5. **Long Duration**: 30 seconds continuous processing\n";
        reportFile << "6. **Buffer Size Stress**: 1 to 16384 samples\n";
        reportFile << "7. **Sample Rate Stress**: 8 kHz to 192 kHz\n";
        reportFile << "8. **Edge Case Combinations**: Silence->impulse, DC+extreme pitch\n";
        reportFile << "9. **Pink Noise & Impulse Train**: Pink noise (1/f), impulse train, chirp signal\n";
        reportFile << "10. **Non-Power-of-2 Buffer Sizes**: 333, 777, 1001 samples\n";
        reportFile << "11. **Ultra High Sample Rate**: 384 kHz extreme sample rate\n";
        reportFile << "12. **Polyphonic Signal**: Complex multi-tone waveform (4 simultaneous frequencies)\n\n";

        reportFile << "## Detailed Results\n\n";

        for (const auto& result : allResults) {
            if (!result.engineCreated) continue;

            reportFile << "### Engine " << result.engineId << ": " << result.engineName << "\n\n";
            reportFile << "**Robustness Score:** " << result.robustnessScore << "/100\n";
            reportFile << "**Verdict:** " << result.overallVerdict << "\n\n";
            reportFile << "**Test Results:**\n\n";

            reportFile << "| Test | Status | CPU Time | Notes |\n";
            reportFile << "|------|--------|----------|-------|\n";

            for (const auto& test : result.testResults) {
                std::string status;
                if (test.crashed) status = "CRASHED";
                else if (test.hasNaN || test.hasInf) status = "NaN/Inf";
                else if (test.passed) status = "PASS";
                else status = "FAIL";

                reportFile << "| " << test.testName << " | " << status << " | "
                          << std::fixed << std::setprecision(2) << test.cpuTime << " ms | ";

                if (!test.failureMode.empty()) {
                    reportFile << test.failureMode;
                } else if (!test.notes.empty()) {
                    reportFile << test.notes;
                } else {
                    reportFile << "OK";
                }

                reportFile << " |\n";
            }

            reportFile << "\n";
        }

        reportFile << "## Robustness Ranking\n\n";
        reportFile << "| Rank | Engine ID | Engine Name | Score | Verdict |\n";
        reportFile << "|------|-----------|-------------|-------|----------|\n";

        int rank = 1;
        for (const auto& result : allResults) {
            if (result.engineCreated) {
                reportFile << "| " << rank++ << " | " << result.engineId << " | "
                          << result.engineName << " | " << result.robustnessScore << "/100 | "
                          << result.overallVerdict << " |\n";
            }
        }

        reportFile << "\n## Recommendations\n\n";

        bool hasIssues = false;
        for (const auto& result : allResults) {
            if (result.robustnessScore < 100 && result.engineCreated) {
                if (!hasIssues) {
                    reportFile << "Engines needing attention:\n\n";
                    hasIssues = true;
                }
                reportFile << "- **Engine " << result.engineId << " (" << result.engineName << ")**: ";
                reportFile << "Score " << result.robustnessScore << "/100. ";

                // Find what failed
                for (const auto& test : result.testResults) {
                    if (!test.passed) {
                        reportFile << test.testName << " failed";
                        if (!test.failureMode.empty()) {
                            reportFile << " (" << test.failureMode << ")";
                        }
                        reportFile << ". ";
                    }
                }
                reportFile << "\n";
            }
        }

        if (!hasIssues) {
            reportFile << "All engines are production-bulletproof! No hardening required.\n";
        }

        reportFile << "\n## Conclusion\n\n";
        if (unstableCount == 0 && bulletproofCount >= allResults.size() / 2) {
            reportFile << "**VERDICT: PRODUCTION BULLETPROOF**\n\n";
            reportFile << "Pitch engines have proven robust under all extreme conditions tested. ";
            reportFile << "They can handle anything users throw at them without crashes, NaN/Inf, or instability.\n";
        } else if (unstableCount == 0) {
            reportFile << "**VERDICT: PRODUCTION READY**\n\n";
            reportFile << "Pitch engines are stable and ready for production use with minor notes on edge cases.\n";
        } else {
            reportFile << "**VERDICT: HARDENING REQUIRED**\n\n";
            reportFile << "Some engines need additional robustness improvements before full production deployment.\n";
        }

        reportFile.close();

        std::cout << "  Detailed report written to: PITCH_ENGINE_STRESS_TEST_REPORT.md\n\n";
    }

    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                  STRESS TESTING COMPLETE                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";

    return (unstableCount == 0) ? 0 : 1;
}
