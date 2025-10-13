/**
 * Real-World Audio Testing Suite
 * Processes realistic musical materials through all 57 engines
 * Performs subjective quality assessment and artifact detection
 */

#include "ComprehensiveTHDEngineFactory.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <sstream>

// WAV file structures
struct WAVHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];           // "data"
    uint32_t dataSize;
};

struct AudioFile {
    std::string filename;
    std::vector<float> leftChannel;
    std::vector<float> rightChannel;
    int sampleRate;
    std::string description;
};

struct QualityMetrics {
    double peakLevel;
    double rmsLevel;
    double dynamicRange;
    double thd;
    double noiseFloor;
    double dcOffset;
    bool hasClipping;
    bool hasArtifacts;
    bool hasSilence;
    double correlationLR;
    int artifactCount;
    std::string artifactDescription;
};

struct EngineTestResult {
    int engineID;
    std::string engineName;
    std::string materialName;
    QualityMetrics inputMetrics;
    QualityMetrics outputMetrics;
    char subjectiveGrade;  // A/B/C/D/F
    std::string issues;
    std::string notes;
    bool passed;
};

class RealWorldAudioTester {
private:
    std::vector<AudioFile> testMaterials;
    std::vector<EngineTestResult> results;
    int sampleRate = 48000;
    int bufferSize = 512;

    // Load WAV file
    bool loadWAV(const std::string& filename, AudioFile& audio) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open: " << filename << std::endl;
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

        // Validate WAV format
        if (std::string(header.riff, 4) != "RIFF" ||
            std::string(header.wave, 4) != "WAVE") {
            std::cerr << "Invalid WAV file: " << filename << std::endl;
            return false;
        }

        audio.sampleRate = header.sampleRate;
        audio.filename = filename;

        // Read samples
        int numSamples = header.dataSize / (header.numChannels * (header.bitsPerSample / 8));

        audio.leftChannel.resize(numSamples);
        audio.rightChannel.resize(numSamples);

        if (header.bitsPerSample == 16) {
            std::vector<int16_t> buffer(header.numChannels);
            for (int i = 0; i < numSamples; ++i) {
                file.read(reinterpret_cast<char*>(buffer.data()),
                         header.numChannels * sizeof(int16_t));

                audio.leftChannel[i] = buffer[0] / 32768.0f;
                audio.rightChannel[i] = (header.numChannels > 1) ?
                                        buffer[1] / 32768.0f : buffer[0] / 32768.0f;
            }
        }

        std::cout << "Loaded: " << filename << " (" << numSamples << " samples)" << std::endl;
        return true;
    }

    // Save processed audio
    bool saveWAV(const std::string& filename, const AudioFile& audio) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            return false;
        }

        WAVHeader header;
        std::memcpy(header.riff, "RIFF", 4);
        std::memcpy(header.wave, "WAVE", 4);
        std::memcpy(header.fmt, "fmt ", 4);
        std::memcpy(header.data, "data", 4);

        header.fmtSize = 16;
        header.audioFormat = 1;  // PCM
        header.numChannels = 2;
        header.sampleRate = audio.sampleRate;
        header.bitsPerSample = 16;
        header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
        header.byteRate = header.sampleRate * header.blockAlign;
        header.dataSize = audio.leftChannel.size() * header.blockAlign;
        header.fileSize = 36 + header.dataSize;

        file.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));

        // Write samples
        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            int16_t left = static_cast<int16_t>(std::clamp(audio.leftChannel[i] * 32767.0f,
                                                           -32768.0f, 32767.0f));
            int16_t right = static_cast<int16_t>(std::clamp(audio.rightChannel[i] * 32767.0f,
                                                            -32768.0f, 32767.0f));

            file.write(reinterpret_cast<const char*>(&left), sizeof(int16_t));
            file.write(reinterpret_cast<const char*>(&right), sizeof(int16_t));
        }

        return true;
    }

    // Calculate quality metrics
    QualityMetrics analyzeAudio(const AudioFile& audio) {
        QualityMetrics metrics;

        // Peak level
        float peakL = 0.0f, peakR = 0.0f;
        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            peakL = std::max(peakL, std::abs(audio.leftChannel[i]));
            peakR = std::max(peakR, std::abs(audio.rightChannel[i]));
        }
        metrics.peakLevel = std::max(peakL, peakR);

        // RMS level
        double sumL = 0.0, sumR = 0.0;
        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            sumL += audio.leftChannel[i] * audio.leftChannel[i];
            sumR += audio.rightChannel[i] * audio.rightChannel[i];
        }
        double rmsL = std::sqrt(sumL / audio.leftChannel.size());
        double rmsR = std::sqrt(sumR / audio.rightChannel.size());
        metrics.rmsLevel = (rmsL + rmsR) / 2.0;

        // Dynamic range (peak to RMS ratio in dB)
        metrics.dynamicRange = 20.0 * std::log10(metrics.peakLevel / (metrics.rmsLevel + 1e-10));

        // DC offset
        double dcL = 0.0, dcR = 0.0;
        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            dcL += audio.leftChannel[i];
            dcR += audio.rightChannel[i];
        }
        dcL /= audio.leftChannel.size();
        dcR /= audio.rightChannel.size();
        metrics.dcOffset = std::max(std::abs(dcL), std::abs(dcR));

        // Clipping detection
        metrics.hasClipping = (metrics.peakLevel >= 0.99f);

        // Silence detection
        metrics.hasSilence = (metrics.rmsLevel < 1e-6);

        // Stereo correlation
        double correlation = 0.0;
        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            correlation += audio.leftChannel[i] * audio.rightChannel[i];
        }
        correlation /= (rmsL * rmsR * audio.leftChannel.size() + 1e-10);
        metrics.correlationLR = correlation;

        // Artifact detection (simple high-frequency noise check)
        metrics.artifactCount = 0;
        metrics.hasArtifacts = false;

        // Check for sudden discontinuities
        int discontinuities = 0;
        float threshold = 0.5f;
        for (size_t i = 1; i < audio.leftChannel.size(); ++i) {
            float diff = std::abs(audio.leftChannel[i] - audio.leftChannel[i-1]);
            if (diff > threshold) {
                discontinuities++;
            }
        }

        if (discontinuities > 10) {
            metrics.hasArtifacts = true;
            metrics.artifactCount = discontinuities;
            metrics.artifactDescription = "Discontinuities detected";
        }

        // Noise floor estimation (RMS of quietest 10%)
        std::vector<float> amplitudes;
        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            amplitudes.push_back(std::abs(audio.leftChannel[i]));
        }
        std::sort(amplitudes.begin(), amplitudes.end());
        size_t quietSamples = amplitudes.size() / 10;
        double noiseSum = 0.0;
        for (size_t i = 0; i < quietSamples; ++i) {
            noiseSum += amplitudes[i] * amplitudes[i];
        }
        metrics.noiseFloor = std::sqrt(noiseSum / quietSamples);

        // THD estimation (crude - assumes signal is mostly fundamental)
        metrics.thd = 0.0;  // Would need FFT for accurate THD

        return metrics;
    }

    // Assign subjective grade based on metrics
    char assignGrade(const QualityMetrics& input, const QualityMetrics& output) {
        int score = 100;

        // Penalty for clipping
        if (output.hasClipping && !input.hasClipping) {
            score -= 30;
        }

        // Penalty for artifacts
        if (output.hasArtifacts && !input.hasArtifacts) {
            score -= 25;
        } else if (output.artifactCount > input.artifactCount * 2) {
            score -= 15;
        }

        // Penalty for excessive DC offset
        if (output.dcOffset > input.dcOffset * 2.0 && output.dcOffset > 0.01) {
            score -= 10;
        }

        // Penalty for unexpected silence
        if (output.hasSilence && !input.hasSilence) {
            score -= 50;  // Critical failure
        }

        // Penalty for dynamic range loss
        double drLoss = input.dynamicRange - output.dynamicRange;
        if (drLoss > 10.0) {
            score -= 15;
        } else if (drLoss > 5.0) {
            score -= 10;
        }

        // Penalty for noise floor increase
        double noiseFactor = output.noiseFloor / (input.noiseFloor + 1e-10);
        if (noiseFactor > 5.0) {
            score -= 20;
        } else if (noiseFactor > 2.0) {
            score -= 10;
        }

        // Grade assignment
        if (score >= 90) return 'A';
        if (score >= 80) return 'B';
        if (score >= 70) return 'C';
        if (score >= 60) return 'D';
        return 'F';
    }

    // Get issues description
    std::string getIssues(const QualityMetrics& input, const QualityMetrics& output) {
        std::vector<std::string> issues;

        if (output.hasClipping && !input.hasClipping) {
            issues.push_back("Introduced clipping");
        }

        if (output.hasArtifacts) {
            issues.push_back("Audio artifacts detected (" +
                           std::to_string(output.artifactCount) + " discontinuities)");
        }

        if (output.hasSilence && !input.hasSilence) {
            issues.push_back("Output is silent");
        }

        if (output.dcOffset > 0.01) {
            issues.push_back("DC offset: " +
                           std::to_string(output.dcOffset * 100.0) + "%");
        }

        double drLoss = input.dynamicRange - output.dynamicRange;
        if (drLoss > 5.0) {
            issues.push_back("Dynamic range loss: " +
                           std::to_string(drLoss) + " dB");
        }

        double noiseFactor = output.noiseFloor / (input.noiseFloor + 1e-10);
        if (noiseFactor > 2.0) {
            issues.push_back("Noise floor increased " +
                           std::to_string(noiseFactor) + "x");
        }

        if (issues.empty()) {
            return "None";
        }

        std::string result = issues[0];
        for (size_t i = 1; i < issues.size(); ++i) {
            result += "; " + issues[i];
        }
        return result;
    }

    // Get engine name
    std::string getEngineName(int engineID) {
        const char* names[] = {
            "Bypass",
            "Vintage Opto Compressor",
            "Classic VCA Compressor",
            "Modern FET Compressor",
            "Multiband Compressor",
            "De-esser",
            "Transient Shaper",
            "Parametric EQ",
            "State Variable Filter",
            "Ladder Filter",
            "Comb Filter",
            "Formant Filter",
            "Graphic EQ",
            "Shelving EQ",
            "Resonant Filter",
            "Tube Distortion",
            "Transistor Distortion",
            "Tape Saturation",
            "Bit Crusher",
            "Wave Shaper",
            "Rodent Distortion",
            "Tube Screamer",
            "Muff Fuzz",
            "ProCo RAT",
            "Chorus",
            "Flanger",
            "Phaser",
            "Tremolo",
            "Vibrato",
            "Ring Modulator",
            "Auto-Pan",
            "Rotary Speaker",
            "Detune Doubler",
            "Intelligent Harmonizer",
            "Simple Delay",
            "Ping-Pong Delay",
            "Tape Delay",
            "Diffusion Chorus",
            "Ensemble Chorus",
            "Hall Reverb",
            "Shimmer Reverb",
            "Convolution Reverb",
            "Plate Reverb",
            "Spring Reverb",
            "Stereo Width",
            "Mid-Side Processor",
            "Haas Effect",
            "Binaural Panner",
            "Surround Panner",
            "Stereo Enhancer",
            "Spectral Gate",
            "Transient Designer",
            "Spectral Delay",
            "Pitch Shifter (SMB)",
            "Phase Vocoder",
            "Granular Processor",
            "Utility (Gain/Pan)"
        };

        if (engineID >= 0 && engineID <= 56) {
            return names[engineID];
        }
        return "Unknown";
    }

public:
    // Load all test materials
    bool loadTestMaterials(const std::string& materialsDir) {
        std::cout << "\nLoading test materials from: " << materialsDir << std::endl;
        std::cout << "============================================================\n";

        std::vector<std::string> filenames = {
            "drum_loop_120bpm.wav",
            "bass_line_e1_e2.wav",
            "vocal_sample_formants.wav",
            "guitar_chord_emajor.wav",
            "piano_notes_c1_c4_c7.wav",
            "white_noise_burst.wav",
            "pink_noise_sustained.wav"
        };

        std::vector<std::string> descriptions = {
            "120 BPM drum loop with kick, snare, hi-hats",
            "Bass line (E1-E2 range, 40-80Hz fundamental)",
            "Vocal sample with formants and vibrato",
            "Acoustic guitar E major chord",
            "Piano notes (C1, C4, C7)",
            "White noise burst (0.5s)",
            "Pink noise sustained (3s)"
        };

        for (size_t i = 0; i < filenames.size(); ++i) {
            AudioFile audio;
            std::string fullPath = materialsDir + "/" + filenames[i];

            if (loadWAV(fullPath, audio)) {
                audio.description = descriptions[i];
                testMaterials.push_back(audio);
            } else {
                std::cerr << "Warning: Could not load " << fullPath << std::endl;
            }
        }

        std::cout << "\nLoaded " << testMaterials.size() << " test materials successfully\n";
        return !testMaterials.empty();
    }

    // Test all engines with all materials
    void testAllEngines() {
        std::cout << "\n============================================================\n";
        std::cout << "TESTING ALL 57 ENGINES WITH REAL-WORLD AUDIO\n";
        std::cout << "============================================================\n\n";

        int totalTests = 57 * testMaterials.size();
        int currentTest = 0;

        for (int engineID = 0; engineID <= 56; ++engineID) {
            std::string engineName = getEngineName(engineID);
            std::cout << "\n[Engine " << engineID << "] " << engineName << std::endl;
            std::cout << std::string(60, '-') << std::endl;

            // Create engine
            auto engine = ComprehensiveTHDEngineFactory::createEngine(engineID);
            if (!engine) {
                std::cout << "  ERROR: Failed to create engine " << engineID << std::endl;
                continue;
            }

            engine->setSampleRate(sampleRate);
            engine->prepareToPlay(sampleRate, bufferSize);

            // Set moderate parameters (0.5 for all)
            for (int p = 0; p < 8; ++p) {
                engine->setParameter(p, 0.5f);
            }

            // Test with each material
            for (const auto& material : testMaterials) {
                currentTest++;
                float progress = (currentTest * 100.0f) / totalTests;
                std::cout << "  [" << std::fixed << std::setprecision(1) << progress
                          << "%] Testing with: " << material.description << "... ";

                EngineTestResult result;
                result.engineID = engineID;
                result.engineName = engineName;
                result.materialName = material.filename;

                // Analyze input
                result.inputMetrics = analyzeAudio(material);

                // Process audio
                AudioFile processed;
                processed.leftChannel = material.leftChannel;
                processed.rightChannel = material.rightChannel;
                processed.sampleRate = material.sampleRate;

                // Process in chunks
                size_t numSamples = material.leftChannel.size();
                for (size_t pos = 0; pos < numSamples; pos += bufferSize) {
                    size_t chunkSize = std::min(bufferSize, static_cast<int>(numSamples - pos));

                    float* buffers[2] = {
                        processed.leftChannel.data() + pos,
                        processed.rightChannel.data() + pos
                    };

                    engine->processBlock(buffers, 2, chunkSize);
                }

                // Analyze output
                result.outputMetrics = analyzeAudio(processed);

                // Grade
                result.subjectiveGrade = assignGrade(result.inputMetrics, result.outputMetrics);
                result.issues = getIssues(result.inputMetrics, result.outputMetrics);
                result.passed = (result.subjectiveGrade != 'F');

                // Add notes for exceptional cases
                if (result.subjectiveGrade == 'A') {
                    result.notes = "Excellent transparency";
                } else if (result.subjectiveGrade == 'F') {
                    result.notes = "Critical issues detected";
                }

                results.push_back(result);

                std::cout << "Grade: " << result.subjectiveGrade;
                if (!result.passed) {
                    std::cout << " ⚠️  FAIL";
                }
                std::cout << std::endl;

                // Save processed audio for critical failures
                if (result.subjectiveGrade == 'F') {
                    std::string outputFilename = "output_engine_" + std::to_string(engineID) +
                                                "_" + material.filename;
                    saveWAV(outputFilename, processed);
                }
            }

            engine->releaseResources();
        }

        std::cout << "\n============================================================\n";
        std::cout << "TESTING COMPLETE\n";
        std::cout << "============================================================\n";
        std::cout << "Total tests: " << results.size() << std::endl;
    }

    // Generate report
    void generateReport(const std::string& filename) {
        std::ofstream report(filename);

        report << "# REAL-WORLD AUDIO TESTING REPORT\n\n";
        report << "**Test Date**: " << __DATE__ << " " << __TIME__ << "\n";
        report << "**Total Engines Tested**: 57 (ID 0-56)\n";
        report << "**Test Materials**: " << testMaterials.size() << "\n";
        report << "**Total Tests**: " << results.size() << "\n\n";

        report << "---\n\n";

        // Summary statistics
        int gradeA = 0, gradeB = 0, gradeC = 0, gradeD = 0, gradeF = 0;
        for (const auto& result : results) {
            switch (result.subjectiveGrade) {
                case 'A': gradeA++; break;
                case 'B': gradeB++; break;
                case 'C': gradeC++; break;
                case 'D': gradeD++; break;
                case 'F': gradeF++; break;
            }
        }

        report << "## SUMMARY STATISTICS\n\n";
        report << "| Grade | Count | Percentage |\n";
        report << "|-------|-------|------------|\n";
        report << "| A (Excellent) | " << gradeA << " | "
               << std::fixed << std::setprecision(1)
               << (gradeA * 100.0 / results.size()) << "% |\n";
        report << "| B (Good) | " << gradeB << " | "
               << (gradeB * 100.0 / results.size()) << "% |\n";
        report << "| C (Acceptable) | " << gradeC << " | "
               << (gradeC * 100.0 / results.size()) << "% |\n";
        report << "| D (Poor) | " << gradeD << " | "
               << (gradeD * 100.0 / results.size()) << "% |\n";
        report << "| F (Failed) | " << gradeF << " | "
               << (gradeF * 100.0 / results.size()) << "% |\n\n";

        int passed = results.size() - gradeF;
        report << "**Pass Rate**: " << (passed * 100.0 / results.size()) << "%\n\n";

        report << "---\n\n";

        // Detailed results by engine
        report << "## DETAILED RESULTS BY ENGINE\n\n";

        for (int engineID = 0; engineID <= 56; ++engineID) {
            std::string engineName = getEngineName(engineID);
            report << "### Engine " << engineID << ": " << engineName << "\n\n";

            // Filter results for this engine
            std::vector<EngineTestResult*> engineResults;
            for (auto& result : results) {
                if (result.engineID == engineID) {
                    engineResults.push_back(&result);
                }
            }

            if (engineResults.empty()) {
                report << "_No test results available_\n\n";
                continue;
            }

            // Calculate average grade
            int avgScore = 0;
            for (const auto* result : engineResults) {
                switch (result->subjectiveGrade) {
                    case 'A': avgScore += 95; break;
                    case 'B': avgScore += 85; break;
                    case 'C': avgScore += 75; break;
                    case 'D': avgScore += 65; break;
                    case 'F': avgScore += 45; break;
                }
            }
            avgScore /= engineResults.size();

            char overallGrade;
            if (avgScore >= 90) overallGrade = 'A';
            else if (avgScore >= 80) overallGrade = 'B';
            else if (avgScore >= 70) overallGrade = 'C';
            else if (avgScore >= 60) overallGrade = 'D';
            else overallGrade = 'F';

            report << "**Overall Grade**: " << overallGrade << "\n\n";

            // Results table
            report << "| Material | Grade | Issues |\n";
            report << "|----------|-------|--------|\n";

            for (const auto* result : engineResults) {
                // Extract material name from path
                std::string matName = result->materialName;
                size_t lastSlash = matName.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    matName = matName.substr(lastSlash + 1);
                }

                report << "| " << matName << " | "
                       << result->subjectiveGrade << " | "
                       << result->issues << " |\n";
            }

            report << "\n";

            // Note any critical issues
            bool hasCritical = false;
            for (const auto* result : engineResults) {
                if (result->subjectiveGrade == 'F') {
                    if (!hasCritical) {
                        report << "⚠️ **Critical Issues**:\n";
                        hasCritical = true;
                    }
                    std::string matName = result->materialName;
                    size_t lastSlash = matName.find_last_of("/\\");
                    if (lastSlash != std::string::npos) {
                        matName = matName.substr(lastSlash + 1);
                    }
                    report << "- " << matName << ": " << result->issues << "\n";
                }
            }

            if (hasCritical) {
                report << "\n";
            }

            report << "---\n\n";
        }

        // Recommendations
        report << "## RECOMMENDATIONS\n\n";

        report << "### Engines Requiring Attention (Grade D or F)\n\n";
        bool hasIssues = false;
        for (int engineID = 0; engineID <= 56; ++engineID) {
            std::vector<char> grades;
            for (const auto& result : results) {
                if (result.engineID == engineID) {
                    grades.push_back(result.subjectiveGrade);
                }
            }

            if (grades.empty()) continue;

            // Count failures
            int failures = std::count_if(grades.begin(), grades.end(),
                                        [](char g) { return g == 'D' || g == 'F'; });

            if (failures > 0) {
                hasIssues = true;
                report << "- **Engine " << engineID << ": " << getEngineName(engineID)
                       << "** - " << failures << "/" << grades.size()
                       << " tests failed\n";
            }
        }

        if (!hasIssues) {
            report << "_All engines performing well!_\n";
        }

        report << "\n### Top Performing Engines (All A grades)\n\n";
        bool hasExcellent = false;
        for (int engineID = 0; engineID <= 56; ++engineID) {
            std::vector<char> grades;
            for (const auto& result : results) {
                if (result.engineID == engineID) {
                    grades.push_back(result.subjectiveGrade);
                }
            }

            if (grades.empty()) continue;

            bool allA = std::all_of(grades.begin(), grades.end(),
                                   [](char g) { return g == 'A'; });

            if (allA) {
                hasExcellent = true;
                report << "- **Engine " << engineID << ": " << getEngineName(engineID)
                       << "** ⭐\n";
            }
        }

        if (!hasExcellent) {
            report << "_No engines achieved perfect scores across all materials_\n";
        }

        report << "\n---\n\n";
        report << "## CONCLUSION\n\n";
        report << "This report provides subjective quality assessment based on:\n";
        report << "- Dynamic range preservation\n";
        report << "- Artifact detection\n";
        report << "- Clipping/distortion analysis\n";
        report << "- Noise floor measurement\n";
        report << "- DC offset detection\n\n";
        report << "Engines with grade C or better are suitable for production use.\n";
        report << "Engines with grade D or F require investigation and fixes.\n\n";

        report.close();
        std::cout << "\nReport generated: " << filename << std::endl;
    }
};

int main() {
    std::cout << "============================================================\n";
    std::cout << "REAL-WORLD AUDIO TESTING SUITE\n";
    std::cout << "Project Chimera Phoenix v3.0\n";
    std::cout << "============================================================\n";

    RealWorldAudioTester tester;

    // Load test materials
    if (!tester.loadTestMaterials("real_world_test_materials")) {
        std::cerr << "\nERROR: Failed to load test materials!\n";
        std::cerr << "Run: python3 generate_musical_materials.py first\n";
        return 1;
    }

    // Test all engines
    tester.testAllEngines();

    // Generate report
    tester.generateReport("REAL_WORLD_AUDIO_TESTING_REPORT.md");

    std::cout << "\n============================================================\n";
    std::cout << "TESTING COMPLETE - Check REAL_WORLD_AUDIO_TESTING_REPORT.md\n";
    std::cout << "============================================================\n";

    return 0;
}
