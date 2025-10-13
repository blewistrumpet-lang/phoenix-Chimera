// ==================== REAL-WORLD DELAY ENGINE TESTING ====================
// Comprehensive musical testing of delay engines with real-world materials
// Tests: Digital Delay (35) & Bucket Brigade Delay (37)

#include "../JUCE_Plugin/Source/DigitalDelay.h"
#include "../JUCE_Plugin/Source/BucketBrigadeDelay.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>
#include <map>
#include <string>

// Test configuration
constexpr double TEST_SAMPLE_RATE = 48000.0;
constexpr int TEST_BLOCK_SIZE = 512;
constexpr int TEST_DURATION_SECONDS = 10;

// Musical material generators
class MusicalMaterialGenerator {
public:
    // Generate clean picked guitar (fingerstyle pattern)
    static juce::AudioBuffer<float> generateGuitar(double sampleRate, int lengthSeconds) {
        int numSamples = lengthSeconds * sampleRate;
        juce::AudioBuffer<float> buffer(2, numSamples);
        buffer.clear();

        // Fingerstyle pattern: thumb bass + fingerpicks
        std::vector<double> pickTimes = {0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875};
        std::vector<float> baseFreqs = {82.4f, 110.0f, 146.8f, 196.0f}; // E2, A2, D3, G3

        float time = 0.0f;
        int beatCount = 0;

        for (int i = 0; i < numSamples; ++i) {
            float sample = 0.0f;

            // Check if we should trigger a note
            double currentBeat = (time / sampleRate) * 2.0; // 120 BPM
            for (double pickTime : pickTimes) {
                double beatPosition = currentBeat - floor(currentBeat);
                if (std::abs(beatPosition - pickTime) < 0.001) {
                    // Trigger note
                }
            }

            // Simple plucked string synthesis with harmonics
            float freq = baseFreqs[beatCount % 4];
            float t = time / sampleRate;

            // Fundamental + harmonics with decay
            float envelope = std::exp(-3.0f * fmod(t, 0.5f));
            sample = envelope * (
                0.6f * std::sin(2.0f * M_PI * freq * t) +
                0.3f * std::sin(4.0f * M_PI * freq * t) +
                0.15f * std::sin(6.0f * M_PI * freq * t) +
                0.08f * std::sin(8.0f * M_PI * freq * t)
            );

            // Add pick attack transient
            if (fmod(t, 0.5f) < 0.01f) {
                sample += 0.3f * std::sin(2.0f * M_PI * 2000.0f * fmod(t, 0.01f) / 0.01f);
            }

            buffer.setSample(0, i, sample * 0.7f);
            buffer.setSample(1, i, sample * 0.7f);

            time++;
            if (i % (int)(sampleRate * 0.5) == 0) beatCount++;
        }

        return buffer;
    }

    // Generate rhythmic vocal phrase
    static juce::AudioBuffer<float> generateVocals(double sampleRate, int lengthSeconds) {
        int numSamples = lengthSeconds * sampleRate;
        juce::AudioBuffer<float> buffer(2, numSamples);
        buffer.clear();

        // Simulate vocal phrase with formants
        float formant1 = 800.0f;  // "ah" sound
        float formant2 = 1150.0f;
        float formant3 = 2800.0f;
        float fundamental = 130.0f; // Male vocal range

        for (int i = 0; i < numSamples; ++i) {
            float t = i / (float)sampleRate;

            // Rhythmic phrasing (4 beats per bar)
            float phrase = fmod(t, 2.0f); // 2 seconds per phrase
            bool isActive = (phrase < 0.4f) || (phrase > 0.6f && phrase < 1.0f) ||
                           (phrase > 1.2f && phrase < 1.6f);

            if (isActive) {
                // Generate vocal formants with vibrato
                float vibrato = 1.0f + 0.02f * std::sin(2.0f * M_PI * 5.5f * t);
                float f0 = fundamental * vibrato;

                // Envelope for natural attack/release
                float phrasePos = fmod(phrase, 0.4f);
                float envelope = 0.0f;
                if (phrasePos < 0.05f) {
                    envelope = phrasePos / 0.05f; // Attack
                } else if (phrasePos > 0.35f) {
                    envelope = 1.0f - ((phrasePos - 0.35f) / 0.05f); // Release
                } else {
                    envelope = 1.0f;
                }

                // Synthesize formants
                float sample = envelope * (
                    0.5f * std::sin(2.0f * M_PI * f0 * t) + // Fundamental
                    0.3f * std::sin(2.0f * M_PI * formant1 * t) +
                    0.2f * std::sin(2.0f * M_PI * formant2 * t) +
                    0.1f * std::sin(2.0f * M_PI * formant3 * t)
                );

                buffer.setSample(0, i, sample * 0.6f);
                buffer.setSample(1, i, sample * 0.6f);
            }
        }

        return buffer;
    }

    // Generate drum pattern (kick, snare, hi-hat)
    static juce::AudioBuffer<float> generateDrums(double sampleRate, int lengthSeconds) {
        int numSamples = lengthSeconds * sampleRate;
        juce::AudioBuffer<float> buffer(2, numSamples);
        buffer.clear();

        float bpm = 120.0f;
        float samplesPerBeat = (60.0f / bpm) * sampleRate;

        for (int i = 0; i < numSamples; ++i) {
            float beatPosition = (i % (int)samplesPerBeat) / samplesPerBeat;
            float sample = 0.0f;

            // Kick drum (beats 1 and 3)
            int beat = (int)(i / samplesPerBeat) % 4;
            if (beat == 0 || beat == 2) {
                if (beatPosition < 0.05f) {
                    float t = beatPosition / 0.05f;
                    float freq = 60.0f * (1.0f - t * 0.8f); // Pitch drop
                    sample += 0.8f * std::sin(2.0f * M_PI * freq * t) * (1.0f - t);
                }
            }

            // Snare (beats 2 and 4)
            if (beat == 1 || beat == 3) {
                if (beatPosition < 0.08f) {
                    float t = beatPosition / 0.08f;
                    // Snare = tone + noise
                    float tone = 0.3f * std::sin(2.0f * M_PI * 200.0f * t);
                    float noise = 0.5f * ((rand() / (float)RAND_MAX) * 2.0f - 1.0f);
                    sample += (tone + noise) * (1.0f - t);
                }
            }

            // Hi-hat (8th notes)
            if (((int)(i / (samplesPerBeat / 2))) != ((int)((i - 1) / (samplesPerBeat / 2)))) {
                float hhPos = fmod(i / (samplesPerBeat / 2.0f), 1.0f);
                if (hhPos < 0.03f) {
                    float noise = 0.15f * ((rand() / (float)RAND_MAX) * 2.0f - 1.0f);
                    sample += noise * (1.0f - hhPos / 0.03f);
                }
            }

            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        return buffer;
    }
};

// Timing accuracy measurement
struct TimingAccuracy {
    float targetMs;
    float measuredMs;
    float errorMs;
    float errorPercent;
    bool withinTolerance; // ±1ms
};

// Feedback stability result
struct FeedbackStability {
    float feedbackLevel;
    bool stable;
    float maxPeak;
    float avgEnergy;
    bool hasRunaway;
};

// Test results structure
struct DelayTestResults {
    std::string engineName;
    std::vector<TimingAccuracy> timingTests;
    std::vector<FeedbackStability> feedbackTests;
    bool clicksOnParameterChange;
    float stereoWidth;
    std::string filterCharacter;
    char grade;
    std::string productionReadiness;
};

// Measure delay timing accuracy
TimingAccuracy measureTiming(EngineBase* engine, float delayMs) {
    TimingAccuracy result;
    result.targetMs = delayMs;

    // Create impulse
    juce::AudioBuffer<float> buffer(2, 96000); // 2 seconds at 48kHz
    buffer.clear();
    buffer.setSample(0, 100, 1.0f); // Impulse at sample 100
    buffer.setSample(1, 100, 1.0f);

    // Process
    engine->reset();

    // Set delay time parameter
    float timeParam = (delayMs / 2000.0f); // Assume 0-2000ms range
    std::map<int, float> params;
    params[0] = timeParam;
    params[1] = 0.0f; // Zero feedback
    engine->updateParameters(params);

    engine->process(buffer);

    // Find echo peak
    float maxPeak = 0.0f;
    int peakSample = 0;

    for (int i = 200; i < buffer.getNumSamples(); ++i) {
        float absVal = std::abs(buffer.getSample(0, i));
        if (absVal > maxPeak) {
            maxPeak = absVal;
            peakSample = i;
        }
    }

    // Calculate measured delay
    result.measuredMs = ((peakSample - 100) * 1000.0f) / TEST_SAMPLE_RATE;
    result.errorMs = result.measuredMs - result.targetMs;
    result.errorPercent = (result.errorMs / result.targetMs) * 100.0f;
    result.withinTolerance = std::abs(result.errorMs) <= 1.0f;

    return result;
}

// Test feedback stability
FeedbackStability testFeedback(EngineBase* engine, float feedbackLevel) {
    FeedbackStability result;
    result.feedbackLevel = feedbackLevel;
    result.stable = true;
    result.maxPeak = 0.0f;
    result.avgEnergy = 0.0f;
    result.hasRunaway = false;

    engine->reset();

    // Set feedback
    std::map<int, float> params;
    params[0] = 0.25f; // 250ms delay
    params[1] = feedbackLevel;
    engine->updateParameters(params);

    // Initial impulse
    juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);
    buffer.clear();
    buffer.setSample(0, 0, 0.5f);
    buffer.setSample(1, 0, 0.5f);
    engine->process(buffer);

    // Run for 5 seconds
    int numBlocks = (5 * TEST_SAMPLE_RATE) / TEST_BLOCK_SIZE;
    float energySum = 0.0f;
    int sampleCount = 0;

    for (int block = 0; block < numBlocks; ++block) {
        buffer.clear();
        engine->process(buffer);

        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < TEST_BLOCK_SIZE; ++i) {
                float sample = buffer.getSample(ch, i);
                float absVal = std::abs(sample);

                result.maxPeak = std::max(result.maxPeak, absVal);
                energySum += sample * sample;
                sampleCount++;

                // Check for runaway (clipping)
                if (absVal > 0.99f) {
                    result.hasRunaway = true;
                    result.stable = false;
                }
            }
        }

        // Early exit if runaway detected
        if (result.hasRunaway) break;
    }

    result.avgEnergy = std::sqrt(energySum / sampleCount);

    return result;
}

// Test for clicks on parameter changes
bool testParameterClicks(EngineBase* engine) {
    engine->reset();

    juce::AudioBuffer<float> buffer(2, TEST_BLOCK_SIZE);

    // Generate continuous tone
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < TEST_BLOCK_SIZE; ++i) {
            float t = i / (float)TEST_SAMPLE_RATE;
            buffer.setSample(ch, i, 0.5f * std::sin(2.0f * M_PI * 440.0f * t));
        }
    }

    // Set initial parameters
    std::map<int, float> params;
    params[0] = 0.5f;
    params[1] = 0.3f;
    engine->updateParameters(params);

    // Process a few blocks
    for (int i = 0; i < 10; ++i) {
        engine->process(buffer);
    }

    // Sudden parameter change
    params[0] = 0.1f;
    params[1] = 0.8f;
    engine->updateParameters(params);

    // Process and check for discontinuities
    juce::AudioBuffer<float> testBuffer(2, TEST_BLOCK_SIZE);
    testBuffer.makeCopyOf(buffer);
    engine->process(testBuffer);

    // Look for large sample-to-sample jumps (clicks)
    float maxJump = 0.0f;
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 1; i < TEST_BLOCK_SIZE; ++i) {
            float jump = std::abs(testBuffer.getSample(ch, i) - testBuffer.getSample(ch, i - 1));
            maxJump = std::max(maxJump, jump);
        }
    }

    // Click detection: jump > 0.5 is likely a click
    return maxJump > 0.5f;
}

// Measure stereo width
float measureStereoWidth(EngineBase* engine) {
    engine->reset();

    // Set ping-pong or stereo mode if available
    std::map<int, float> params;
    params[0] = 0.25f; // 250ms
    params[1] = 0.5f;  // 50% feedback
    engine->updateParameters(params);

    // Generate mono input
    juce::AudioBuffer<float> buffer(2, TEST_SAMPLE_RATE * 2); // 2 seconds
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / TEST_SAMPLE_RATE);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }

    engine->process(buffer);

    // Calculate correlation between L and R
    float sumL = 0.0f, sumR = 0.0f, sumLR = 0.0f;
    float sumL2 = 0.0f, sumR2 = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float l = buffer.getSample(0, i);
        float r = buffer.getSample(1, i);

        sumL += l;
        sumR += r;
        sumLR += l * r;
        sumL2 += l * l;
        sumR2 += r * r;
    }

    int n = buffer.getNumSamples();
    float correlation = (n * sumLR - sumL * sumR) /
                       std::sqrt((n * sumL2 - sumL * sumL) * (n * sumR2 - sumR * sumR));

    // Width = 1 - correlation (0 = mono, 1 = full stereo)
    return 1.0f - std::abs(correlation);
}

// Process musical material through delay
void processMusicalMaterial(EngineBase* engine, const juce::AudioBuffer<float>& input,
                           juce::AudioBuffer<float>& output, const std::string& materialType,
                           float delayMs, float feedback) {
    output.makeCopyOf(input);

    engine->reset();

    // Set delay parameters
    float timeParam = delayMs / 2000.0f;
    std::map<int, float> params;
    params[0] = timeParam;
    params[1] = feedback;
    params[2] = 0.5f; // 50% mix
    engine->updateParameters(params);

    // Process in blocks
    int numSamples = output.getNumSamples();
    int pos = 0;

    while (pos < numSamples) {
        int blockSize = std::min(TEST_BLOCK_SIZE, numSamples - pos);

        juce::AudioBuffer<float> block(output.getArrayOfWritePointers(), 2, pos, blockSize);
        engine->process(block);

        pos += blockSize;
    }
}

// Grade the engine
char gradeEngine(const DelayTestResults& results) {
    int score = 0;

    // Timing accuracy (40 points)
    int accurateTimings = 0;
    for (const auto& timing : results.timingTests) {
        if (timing.withinTolerance) accurateTimings++;
    }
    score += (accurateTimings * 40) / results.timingTests.size();

    // Feedback stability (30 points)
    int stableTests = 0;
    for (const auto& fb : results.feedbackTests) {
        if (fb.stable && !fb.hasRunaway) stableTests++;
    }
    score += (stableTests * 30) / results.feedbackTests.size();

    // No clicks (15 points)
    if (!results.clicksOnParameterChange) score += 15;

    // Stereo width (15 points)
    if (results.stereoWidth > 0.3f) score += 15;
    else if (results.stereoWidth > 0.1f) score += 10;

    // Assign grade
    if (score >= 90) return 'A';
    if (score >= 80) return 'B';
    if (score >= 70) return 'C';
    if (score >= 60) return 'D';
    return 'F';
}

// Comprehensive delay engine test
DelayTestResults testDelayEngine(EngineBase* engine, const std::string& engineName) {
    DelayTestResults results;
    results.engineName = engineName;

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "TESTING: " << engineName << "\n";
    std::cout << std::string(80, '=') << "\n\n";

    // Prepare engine
    engine->prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);

    // Test 1: Timing Accuracy
    std::cout << "Test 1: Timing Accuracy\n";
    std::cout << std::string(40, '-') << "\n";

    std::vector<float> delayTimes = {50.0f, 250.0f, 500.0f, 1000.0f, 2000.0f};

    for (float delayMs : delayTimes) {
        TimingAccuracy timing = measureTiming(engine, delayMs);
        results.timingTests.push_back(timing);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Target: " << std::setw(7) << delayMs << "ms | ";
        std::cout << "Measured: " << std::setw(7) << timing.measuredMs << "ms | ";
        std::cout << "Error: " << std::setw(6) << timing.errorMs << "ms ";
        std::cout << "(" << std::setw(5) << timing.errorPercent << "%) ";
        std::cout << (timing.withinTolerance ? "[PASS]" : "[FAIL]") << "\n";
    }

    // Test 2: Feedback Stability
    std::cout << "\nTest 2: Feedback Stability\n";
    std::cout << std::string(40, '-') << "\n";

    std::vector<float> feedbackLevels = {0.0f, 0.25f, 0.5f, 0.75f, 0.9f, 0.95f};

    for (float fb : feedbackLevels) {
        FeedbackStability fbTest = testFeedback(engine, fb);
        results.feedbackTests.push_back(fbTest);

        std::cout << "Feedback: " << std::setw(5) << (fb * 100) << "% | ";
        std::cout << "Stable: " << (fbTest.stable ? "YES" : "NO ") << " | ";
        std::cout << "Max Peak: " << std::setw(5) << std::setprecision(3) << fbTest.maxPeak << " | ";
        std::cout << "Avg Energy: " << std::setw(5) << fbTest.avgEnergy;
        if (fbTest.hasRunaway) std::cout << " [RUNAWAY]";
        std::cout << "\n";
    }

    // Test 3: Parameter Click Detection
    std::cout << "\nTest 3: Parameter Change Smoothness\n";
    std::cout << std::string(40, '-') << "\n";

    results.clicksOnParameterChange = testParameterClicks(engine);
    std::cout << "Clicks detected: " << (results.clicksOnParameterChange ? "YES [FAIL]" : "NO [PASS]") << "\n";

    // Test 4: Stereo Width
    std::cout << "\nTest 4: Stereo Width\n";
    std::cout << std::string(40, '-') << "\n";

    results.stereoWidth = measureStereoWidth(engine);
    std::cout << "Stereo width: " << std::setprecision(3) << results.stereoWidth;
    if (results.stereoWidth > 0.3f) std::cout << " [EXCELLENT]";
    else if (results.stereoWidth > 0.1f) std::cout << " [GOOD]";
    else std::cout << " [NARROW]";
    std::cout << "\n";

    // Test 5: Musical Material Processing
    std::cout << "\nTest 5: Musical Material Processing\n";
    std::cout << std::string(40, '-') << "\n";

    // Generate materials
    auto guitar = MusicalMaterialGenerator::generateGuitar(TEST_SAMPLE_RATE, 8);
    auto vocals = MusicalMaterialGenerator::generateVocals(TEST_SAMPLE_RATE, 8);
    auto drums = MusicalMaterialGenerator::generateDrums(TEST_SAMPLE_RATE, 8);

    // Process and save
    juce::AudioBuffer<float> guitarProcessed(2, guitar.getNumSamples());
    juce::AudioBuffer<float> vocalsProcessed(2, vocals.getNumSamples());
    juce::AudioBuffer<float> drumsProcessed(2, drums.getNumSamples());

    processMusicalMaterial(engine, guitar, guitarProcessed, "guitar", 375.0f, 0.5f);
    processMusicalMaterial(engine, vocals, vocalsProcessed, "vocals", 250.0f, 0.6f);
    processMusicalMaterial(engine, drums, drumsProcessed, "drums", 500.0f, 0.4f);

    // Save audio files
    juce::File outputDir("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/delay_audio_tests");
    outputDir.createDirectory();

    auto saveAudio = [&](const juce::AudioBuffer<float>& buffer, const std::string& name) {
        juce::File outputFile = outputDir.getChildFile(engineName + "_" + name + ".wav");
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

        if (fileStream != nullptr) {
            std::unique_ptr<juce::AudioFormatWriter> writer;
            writer.reset(wavFormat.createWriterFor(fileStream.get(), TEST_SAMPLE_RATE,
                                                   buffer.getNumChannels(), 24, {}, 0));
            if (writer != nullptr) {
                fileStream.release();
                writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
            }
        }
    };

    saveAudio(guitarProcessed, "guitar");
    saveAudio(vocalsProcessed, "vocals");
    saveAudio(drumsProcessed, "drums");

    std::cout << "Audio files saved to: " << outputDir.getFullPathName() << "\n";

    // Grade engine
    results.grade = gradeEngine(results);

    // Production readiness assessment
    if (results.grade == 'A') {
        results.productionReadiness = "PRODUCTION READY - Excellent performance";
    } else if (results.grade == 'B') {
        results.productionReadiness = "PRODUCTION READY - Good performance with minor issues";
    } else if (results.grade == 'C') {
        results.productionReadiness = "USABLE - Acceptable but needs improvement";
    } else {
        results.productionReadiness = "NOT RECOMMENDED - Significant issues detected";
    }

    std::cout << "\n" << std::string(40, '-') << "\n";
    std::cout << "GRADE: " << results.grade << "\n";
    std::cout << "STATUS: " << results.productionReadiness << "\n";

    return results;
}

// Generate comprehensive report
void generateReport(const std::vector<DelayTestResults>& allResults) {
    std::ofstream report("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/DELAY_REALWORLD_TEST_REPORT.md");

    report << "# REAL-WORLD DELAY ENGINE TEST REPORT\n\n";
    report << "**Test Date:** " << juce::Time::getCurrentTime().toString(true, true).toStdString() << "\n";
    report << "**Sample Rate:** " << TEST_SAMPLE_RATE << " Hz\n";
    report << "**Test Duration:** " << TEST_DURATION_SECONDS << " seconds per engine\n\n";

    report << "## Executive Summary\n\n";
    report << "Comprehensive real-world testing of delay engines with musical materials.\n\n";

    report << "| Engine | Grade | Timing | Feedback | Stereo | Status |\n";
    report << "|--------|-------|--------|----------|--------|--------|\n";

    for (const auto& result : allResults) {
        int timingPass = 0;
        for (const auto& t : result.timingTests) {
            if (t.withinTolerance) timingPass++;
        }

        int feedbackPass = 0;
        for (const auto& f : result.feedbackTests) {
            if (f.stable) feedbackPass++;
        }

        report << "| " << result.engineName << " | ";
        report << result.grade << " | ";
        report << timingPass << "/" << result.timingTests.size() << " | ";
        report << feedbackPass << "/" << result.feedbackTests.size() << " | ";
        report << std::fixed << std::setprecision(2) << result.stereoWidth << " | ";
        report << result.productionReadiness << " |\n";
    }

    report << "\n## Detailed Results\n\n";

    for (const auto& result : allResults) {
        report << "### " << result.engineName << "\n\n";
        report << "**Grade:** " << result.grade << "\n\n";
        report << "**Production Readiness:** " << result.productionReadiness << "\n\n";

        report << "#### Timing Accuracy\n\n";
        report << "| Target (ms) | Measured (ms) | Error (ms) | Error (%) | Status |\n";
        report << "|-------------|---------------|------------|-----------|--------|\n";

        for (const auto& timing : result.timingTests) {
            report << "| " << std::fixed << std::setprecision(1) << timing.targetMs << " | ";
            report << timing.measuredMs << " | ";
            report << std::setprecision(2) << timing.errorMs << " | ";
            report << timing.errorPercent << " | ";
            report << (timing.withinTolerance ? "PASS" : "FAIL") << " |\n";
        }

        report << "\n#### Feedback Stability\n\n";
        report << "| Feedback (%) | Stable | Max Peak | Avg Energy | Notes |\n";
        report << "|--------------|--------|----------|------------|-------|\n";

        for (const auto& fb : result.feedbackTests) {
            report << "| " << (fb.feedbackLevel * 100) << " | ";
            report << (fb.stable ? "YES" : "NO") << " | ";
            report << std::setprecision(3) << fb.maxPeak << " | ";
            report << fb.avgEnergy << " | ";
            if (fb.hasRunaway) report << "RUNAWAY";
            report << " |\n";
        }

        report << "\n#### Audio Quality\n\n";
        report << "- **Stereo Width:** " << std::setprecision(3) << result.stereoWidth << "\n";
        report << "- **Parameter Clicks:** " << (result.clicksOnParameterChange ? "Detected" : "None") << "\n";
        report << "- **Filter Character:** " << result.filterCharacter << "\n\n";

        report << "---\n\n";
    }

    report << "## Audio Test Files\n\n";
    report << "Audio test files saved to: `/standalone_test/delay_audio_tests/`\n\n";
    report << "Files generated for each engine:\n";
    report << "- `[engine]_guitar.wav` - Clean picked guitar with delay\n";
    report << "- `[engine]_vocals.wav` - Rhythmic vocals with delay\n";
    report << "- `[engine]_drums.wav` - Drum pattern with delay\n\n";

    report << "## Conclusions\n\n";

    char bestGrade = 'F';
    std::string bestEngine;

    for (const auto& result : allResults) {
        if (result.grade < bestGrade) {
            bestGrade = result.grade;
            bestEngine = result.engineName;
        }
    }

    report << "**Best Performing Engine:** " << bestEngine << " (Grade: " << bestGrade << ")\n\n";
    report << "**Key Findings:**\n";
    report << "- Timing accuracy is critical for musical applications\n";
    report << "- Feedback stability must be rock-solid up to 95%\n";
    report << "- Parameter smoothing prevents clicks\n";
    report << "- Stereo imaging enhances spatial perception\n\n";

    report.close();

    std::cout << "\n\nReport saved to: DELAY_REALWORLD_TEST_REPORT.md\n";
}

int main() {
    std::cout << "==================== REAL-WORLD DELAY ENGINE TESTING ====================\n";
    std::cout << "Testing delay engines with musical materials...\n\n";

    std::vector<DelayTestResults> allResults;

    // Test Digital Delay
    try {
        std::cout << "Creating Digital Delay engine...\n";
        auto digitalDelay = std::make_unique<AudioDSP::DigitalDelay>();
        auto results = testDelayEngine(digitalDelay.get(), "Digital_Delay");
        allResults.push_back(results);
    } catch (const std::exception& e) {
        std::cerr << "ERROR testing Digital Delay: " << e.what() << "\n";
    }

    // Test Bucket Brigade Delay
    try {
        std::cout << "\n\nCreating Bucket Brigade Delay engine...\n";
        auto bbdDelay = std::make_unique<BucketBrigadeDelay>();
        auto results = testDelayEngine(bbdDelay.get(), "Bucket_Brigade_Delay");
        allResults.push_back(results);
    } catch (const std::exception& e) {
        std::cerr << "ERROR testing Bucket Brigade Delay: " << e.what() << "\n";
    }

    // Generate comprehensive report
    generateReport(allResults);

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ALL TESTS COMPLETE\n";
    std::cout << std::string(80, '=') << "\n\n";

    std::cout << "Summary:\n";
    for (const auto& result : allResults) {
        std::cout << "  " << result.engineName << ": Grade " << result.grade
                  << " - " << result.productionReadiness << "\n";
    }

    std::cout << "\nFiles generated:\n";
    std::cout << "  - DELAY_REALWORLD_TEST_REPORT.md\n";
    std::cout << "  - delay_audio_tests/*.wav\n";

    return 0;
}
