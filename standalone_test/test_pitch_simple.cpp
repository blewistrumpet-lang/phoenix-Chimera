/*
 * SIMPLIFIED REAL-WORLD PITCH ENGINE TEST
 *
 * Tests the core pitch shifting algorithms directly without the full engine framework.
 * Focus: SMBPitchShiftFixed and IntelligentHarmonizer standalone versions
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include "SMBPitchShiftFixed_standalone.h"
#include "IntelligentHarmonizer_standalone.h"

// Type aliases for cleaner code
using SMBPitchShift = SMBPitchShiftFixed;
using Harmonizer = IntelligentHarmonizer_Standalone;

// Generate vocal-like signal with harmonics and formants
void generateVocalSignal(std::vector<float>& buffer, float fundamental, float sampleRate) {
    const size_t numSamples = buffer.size();

    // Vocal harmonic structure
    const int numHarmonics = 12;
    float harmonicAmps[12] = {1.0f, 0.7f, 0.5f, 0.4f, 0.3f, 0.25f, 0.2f, 0.15f, 0.12f, 0.1f, 0.08f, 0.06f};

    // Simplified formants (male voice ~220Hz)
    float formantFreqs[3] = {650.0f, 1080.0f, 2650.0f};
    float formantAmps[3] = {0.8f, 0.6f, 0.4f};
    float formantBWs[3] = {80.0f, 90.0f, 120.0f};

    for (size_t i = 0; i < numSamples; ++i) {
        float sample = 0.0f;
        float t = i / sampleRate;

        // Add harmonics
        for (int h = 0; h < numHarmonics; ++h) {
            float freq = fundamental * (h + 1);
            sample += harmonicAmps[h] * std::sin(2.0f * M_PI * freq * t);
        }

        // Add formant resonances
        for (int f = 0; f < 3; ++f) {
            float formantPhase = 2.0f * M_PI * formantFreqs[f] * t;
            float envelope = std::exp(-formantBWs[f] * std::abs(std::sin(formantPhase)));
            sample += formantAmps[f] * std::sin(formantPhase) * envelope * 0.1f;
        }

        // Add vibrato
        float vibrato = 1.0f + 0.01f * std::sin(2.0f * M_PI * 5.0f * t);
        sample *= vibrato * 0.3f;

        buffer[i] = sample;
    }
}

// Generate instrument signal (trumpet-like)
void generateInstrumentSignal(std::vector<float>& buffer, float fundamental, float sampleRate) {
    const size_t numSamples = buffer.size();

    // Trumpet harmonic structure (strong odd harmonics)
    const int numHarmonics = 10;
    float harmonicAmps[10] = {1.0f, 0.4f, 0.8f, 0.3f, 0.7f, 0.2f, 0.5f, 0.15f, 0.4f, 0.1f};

    for (size_t i = 0; i < numSamples; ++i) {
        float sample = 0.0f;
        float t = i / sampleRate;

        // Add harmonics
        for (int h = 0; h < numHarmonics; ++h) {
            float freq = fundamental * (h + 1);
            sample += harmonicAmps[h] * std::sin(2.0f * M_PI * freq * t);
        }

        // Add envelope
        float envelope = 1.0f;
        float duration = numSamples / sampleRate;
        if (t < 0.05f) envelope = t / 0.05f;
        else if (t > duration - 0.1f) envelope = (duration - t) / 0.1f;

        sample *= envelope * 0.3f;
        buffer[i] = sample;
    }
}

// FFT-based frequency detection (simplified without JUCE)
float detectFundamentalFrequency(const std::vector<float>& buffer, float sampleRate) {
    // Simple autocorrelation-based pitch detection
    const size_t searchRange = static_cast<size_t>(sampleRate / 50.0f);  // ~50Hz to 2kHz
    const size_t minLag = static_cast<size_t>(sampleRate / 2000.0f);

    float bestR = -1.0f;
    size_t bestLag = 0;

    for (size_t lag = minLag; lag < searchRange && lag < buffer.size() / 2; ++lag) {
        float r = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;

        for (size_t i = 0; i < buffer.size() - lag; ++i) {
            r += buffer[i] * buffer[i + lag];
            norm1 += buffer[i] * buffer[i];
            norm2 += buffer[i + lag] * buffer[i + lag];
        }

        if (norm1 > 0.0f && norm2 > 0.0f) {
            r /= std::sqrt(norm1 * norm2);
            if (r > bestR) {
                bestR = r;
                bestLag = lag;
            }
        }
    }

    return bestLag > 0 ? sampleRate / static_cast<float>(bestLag) : 0.0f;
}

// Calculate cents error
float calculateCentsError(float measured, float expected) {
    if (expected <= 0.0f || measured <= 0.0f) return 999.0f;
    return 1200.0f * std::log2(measured / expected);
}

// Semitones to ratio
float semitonesToRatio(int semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

struct PitchTestResult {
    std::string algorithm;
    std::string material;
    int semitones;
    float inputFreq;
    float expectedFreq;
    float measuredFreq;
    float centsError;
    bool passAccuracy;  // Within ±5 cents
    char grade;
    std::string notes;
};

PitchTestResult testSMBPitchShift(const std::string& material, float inputFreq,
                                   int semitones, float sampleRate) {
    PitchTestResult result;
    result.algorithm = "SMBPitchShift";
    result.material = material;
    result.semitones = semitones;
    result.inputFreq = inputFreq;
    result.expectedFreq = inputFreq * semitonesToRatio(semitones);
    result.passAccuracy = false;
    result.grade = 'F';

    try {
        // Generate test signal (2 seconds)
        const size_t testLength = static_cast<size_t>(2.0f * sampleRate);
        std::vector<float> inputBuffer(testLength);
        std::vector<float> outputBuffer(testLength);

        if (material.find("Vocal") != std::string::npos) {
            generateVocalSignal(inputBuffer, inputFreq, sampleRate);
        } else {
            generateInstrumentSignal(inputBuffer, inputFreq, sampleRate);
        }

        // Create pitch shifter
        SMBPitchShiftFixed pitchShifter;
        pitchShifter.prepare(sampleRate, 512);

        // Calculate pitch ratio
        float pitchRatio = semitonesToRatio(semitones);

        // Process audio (frame by frame for more realistic testing)
        const size_t frameSize = 512;
        for (size_t i = 0; i < testLength; i += frameSize) {
            size_t samplesThisFrame = std::min(frameSize, testLength - i);
            pitchShifter.process(&inputBuffer[i], &outputBuffer[i],
                                samplesThisFrame, pitchRatio);
        }

        // Analyze output (skip first 20% for transient settling)
        size_t skipSamples = testLength / 5;
        std::vector<float> analysisBuffer(outputBuffer.begin() + skipSamples, outputBuffer.end());

        // Measure pitch
        result.measuredFreq = detectFundamentalFrequency(analysisBuffer, sampleRate);
        result.centsError = calculateCentsError(result.measuredFreq, result.expectedFreq);

        // Grade accuracy
        result.passAccuracy = std::abs(result.centsError) <= 5.0f;

        if (std::abs(result.centsError) <= 5.0f) result.grade = 'A';
        else if (std::abs(result.centsError) <= 10.0f) result.grade = 'B';
        else if (std::abs(result.centsError) <= 20.0f) result.grade = 'C';
        else if (std::abs(result.centsError) <= 50.0f) result.grade = 'D';
        else result.grade = 'F';

        // Save output audio as simple WAV
        std::string filename = "pitch_smb_" + material + "_" +
                              (semitones >= 0 ? "+" : "") + std::to_string(semitones) + "st.raw";
        std::ofstream outFile(filename, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(outputBuffer.data()),
                         outputBuffer.size() * sizeof(float));
            outFile.close();
        }

    } catch (const std::exception& e) {
        result.notes = std::string("Exception: ") + e.what();
        result.grade = 'F';
    }

    return result;
}

PitchTestResult testIntelligentHarmonizer(const std::string& material, float inputFreq,
                                          int semitones, float sampleRate) {
    PitchTestResult result;
    result.algorithm = "IntelligentHarmonizer";
    result.material = material;
    result.semitones = semitones;
    result.inputFreq = inputFreq;
    result.expectedFreq = inputFreq * semitonesToRatio(semitones);
    result.passAccuracy = false;
    result.grade = 'F';

    try {
        // Generate test signal (2 seconds)
        const size_t testLength = static_cast<size_t>(2.0f * sampleRate);
        std::vector<float> leftIn(testLength);
        std::vector<float> rightIn(testLength);
        std::vector<float> leftOut(testLength);
        std::vector<float> rightOut(testLength);

        if (material.find("Vocal") != std::string::npos) {
            generateVocalSignal(leftIn, inputFreq, sampleRate);
            generateVocalSignal(rightIn, inputFreq, sampleRate);
        } else {
            generateInstrumentSignal(leftIn, inputFreq, sampleRate);
            generateInstrumentSignal(rightIn, inputFreq, sampleRate);
        }

        // Create harmonizer
        Harmonizer harmonizer;
        harmonizer.prepareToPlay(sampleRate, 512);

        // Set pitch shift parameter using updateParameters
        std::map<int, float> params;
        params[Harmonizer::kTranspose] = 0.5f + (semitones / 24.0f);  // Global transpose
        params[Harmonizer::kMasterMix] = 1.0f;  // Full wet
        params[Harmonizer::kVoices] = 0.0f;  // Single voice (just transpose)

        harmonizer.updateParameters(params);

        // Process audio (mono for now - harmonizer expects mono input)
        const size_t frameSize = 512;
        for (size_t i = 0; i < testLength; i += frameSize) {
            size_t samplesThisFrame = std::min(frameSize, testLength - i);
            harmonizer.processBlock(&leftIn[i], &leftOut[i], samplesThisFrame);
        }

        // Copy to right channel
        rightOut = leftOut;

        // Analyze output (skip first 20%)
        size_t skipSamples = testLength / 5;
        std::vector<float> analysisBuffer(leftOut.begin() + skipSamples, leftOut.end());

        // Measure pitch
        result.measuredFreq = detectFundamentalFrequency(analysisBuffer, sampleRate);
        result.centsError = calculateCentsError(result.measuredFreq, result.expectedFreq);

        // Grade accuracy
        result.passAccuracy = std::abs(result.centsError) <= 5.0f;

        if (std::abs(result.centsError) <= 5.0f) result.grade = 'A';
        else if (std::abs(result.centsError) <= 10.0f) result.grade = 'B';
        else if (std::abs(result.centsError) <= 20.0f) result.grade = 'C';
        else if (std::abs(result.centsError) <= 50.0f) result.grade = 'D';
        else result.grade = 'F';

        // Save output audio
        std::string filename = "pitch_harmonizer_" + material + "_" +
                              (semitones >= 0 ? "+" : "") + std::to_string(semitones) + "st.raw";
        std::ofstream outFile(filename, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(leftOut.data()),
                         leftOut.size() * sizeof(float));
            outFile.close();
        }

    } catch (const std::exception& e) {
        result.notes = std::string("Exception: ") + e.what();
        result.grade = 'F';
    }

    return result;
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          REAL-WORLD PITCH ENGINE TEST (SIMPLIFIED)           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    const float sampleRate = 48000.0f;

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

    std::cout << "Test Configuration:\n";
    std::cout << "  Sample Rate:     " << sampleRate << " Hz\n";
    std::cout << "  Materials:       Male Vocal (220Hz), Female Vocal (440Hz), Trumpet (466Hz)\n";
    std::cout << "  Semitone Shifts: -12, -7, -5, -1, +1, +5, +7, +12\n";
    std::cout << "  Algorithms:      SMBPitchShift, IntelligentHarmonizer\n";
    std::cout << "  Accuracy Target: ±5 cents\n\n";

    std::vector<PitchTestResult> allResults;

    // Test SMBPitchShift
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << "Testing SMBPitchShift Algorithm\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n\n";

    for (const auto& material : materials) {
        std::cout << "  Material: " << material.name << " (" << material.frequency << " Hz)\n";
        std::cout << "  ───────────────────────────────────────────────────────────\n";

        for (int semitones : semitoneShifts) {
            auto result = testSMBPitchShift(material.name, material.frequency,
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

    // Test IntelligentHarmonizer
    std::cout << "\n═══════════════════════════════════════════════════════════════\n";
    std::cout << "Testing IntelligentHarmonizer Algorithm\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n\n";

    for (const auto& material : materials) {
        std::cout << "  Material: " << material.name << " (" << material.frequency << " Hz)\n";
        std::cout << "  ───────────────────────────────────────────────────────────\n";

        for (int semitones : semitoneShifts) {
            auto result = testIntelligentHarmonizer(material.name, material.frequency,
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

    // Generate summary
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      SUMMARY REPORT                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    std::map<std::string, std::map<char, int>> gradeDistribution;
    std::map<std::string, std::vector<float>> centsErrors;

    for (const auto& result : allResults) {
        gradeDistribution[result.algorithm][result.grade]++;
        centsErrors[result.algorithm].push_back(std::abs(result.centsError));
    }

    for (const auto& algo : {"SMBPitchShift", "IntelligentHarmonizer"}) {
        std::cout << "Algorithm: " << algo << "\n";
        std::cout << "  Grade Distribution: ";
        std::cout << "A=" << gradeDistribution[algo]['A'] << " ";
        std::cout << "B=" << gradeDistribution[algo]['B'] << " ";
        std::cout << "C=" << gradeDistribution[algo]['C'] << " ";
        std::cout << "D=" << gradeDistribution[algo]['D'] << " ";
        std::cout << "F=" << gradeDistribution[algo]['F'] << "\n";

        float avgError = 0.0f;
        float maxError = 0.0f;
        for (float err : centsErrors[algo]) {
            avgError += err;
            maxError = std::max(maxError, err);
        }
        avgError /= centsErrors[algo].size();

        std::cout << "  Avg Cents Error: " << std::fixed << std::setprecision(2) << avgError << "\n";
        std::cout << "  Max Cents Error: " << std::fixed << std::setprecision(2) << maxError << "\n";

        char overallGrade = 'F';
        if (avgError <= 5.0f && maxError <= 10.0f) overallGrade = 'A';
        else if (avgError <= 10.0f && maxError <= 20.0f) overallGrade = 'B';
        else if (avgError <= 20.0f && maxError <= 50.0f) overallGrade = 'C';
        else if (avgError <= 50.0f) overallGrade = 'D';

        std::cout << "  Overall Grade: " << overallGrade << "\n";
        bool productionReady = (overallGrade <= 'B' && maxError <= 20.0f);
        std::cout << "  Production Ready: " << (productionReady ? "YES ✓" : "NO ✗") << "\n\n";
    }

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

    std::cout << "Audio files saved as .raw (32-bit float, mono, 48kHz)\n";
    std::cout << "Convert with: ffmpeg -f f32le -ar 48000 -ac 1 -i file.raw file.wav\n\n";

    return (passedTests >= totalTests * 0.7) ? 0 : 1;
}
