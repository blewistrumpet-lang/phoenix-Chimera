/**
 * REAL-WORLD AUDIO TESTING - FILTER/EQ ENGINES 7-14
 *
 * Comprehensive testing of all filter engines with realistic musical content
 * Tests for: artifacts, ringing, phase issues, resonance stability
 *
 * Engines tested:
 * 7: ParametricEQ
 * 8: VintageConsoleEQ
 * 9: LadderFilter
 * 10: StateVariableFilter
 * 11: FormantFilter
 * 12: EnvelopeFilter
 * 13: CombResonator
 * 14: VocalFormant
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
#include <complex>

// WAV file structures
struct WAVHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

struct AudioFile {
    std::string filename;
    std::vector<float> leftChannel;
    std::vector<float> rightChannel;
    int sampleRate;
    std::string description;
};

struct FilterTestMetrics {
    double peakLevel;
    double rmsLevel;
    double thd;
    double dcOffset;
    double phaseCoherence;
    double resonancePeak;
    double smoothness;
    int ringingDetected;
    int artifactCount;
    bool hasInstability;
    bool hasClipping;
    double noiseFloor;
};

struct FilterTestResult {
    int engineID;
    std::string engineName;
    std::string materialName;
    std::string testType;  // "normal", "freq_sweep", "resonance_sweep"
    FilterTestMetrics metrics;
    char grade;  // A/B/C/D/F
    std::string issues;
    std::string character;  // "transparent", "colored", "warm", etc.
    double recommendedResonanceLimit;
    bool productionReady;
};

class FilterRealWorldTester {
private:
    std::vector<AudioFile> testMaterials;
    std::vector<FilterTestResult> results;
    int sampleRate = 48000;
    int bufferSize = 512;

    const std::vector<int> filterEngineIDs = {7, 8, 9, 10, 11, 12, 13, 14};

    // Load WAV file
    bool loadWAV(const std::string& filename, AudioFile& audio) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open: " << filename << std::endl;
            return false;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

        if (std::string(header.riff, 4) != "RIFF" ||
            std::string(header.wave, 4) != "WAVE") {
            std::cerr << "Invalid WAV file: " << filename << std::endl;
            return false;
        }

        audio.sampleRate = header.sampleRate;
        audio.filename = filename;

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

        return true;
    }

    // Save processed audio
    bool saveWAV(const std::string& filename, const AudioFile& audio) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return false;

        WAVHeader header;
        std::memcpy(header.riff, "RIFF", 4);
        std::memcpy(header.wave, "WAVE", 4);
        std::memcpy(header.fmt, "fmt ", 4);
        std::memcpy(header.data, "data", 4);

        header.fmtSize = 16;
        header.audioFormat = 1;
        header.numChannels = 2;
        header.sampleRate = audio.sampleRate;
        header.bitsPerSample = 16;
        header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
        header.byteRate = header.sampleRate * header.blockAlign;
        header.dataSize = audio.leftChannel.size() * header.blockAlign;
        header.fileSize = 36 + header.dataSize;

        file.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));

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

    // Calculate THD using simple harmonic analysis
    double calculateTHD(const std::vector<float>& signal) {
        if (signal.size() < 1024) return 0.0;

        // Use only middle section to avoid transients
        size_t start = signal.size() / 4;
        size_t end = 3 * signal.size() / 4;
        size_t len = end - start;

        // Calculate RMS of harmonics vs fundamental (simplified)
        double rms = 0.0;
        for (size_t i = start; i < end; ++i) {
            rms += signal[i] * signal[i];
        }
        rms = std::sqrt(rms / len);

        // Estimate THD from signal statistics (crude but fast)
        double variance = 0.0;
        double mean = 0.0;
        for (size_t i = start; i < end; ++i) {
            mean += signal[i];
        }
        mean /= len;

        for (size_t i = start; i < end; ++i) {
            double diff = signal[i] - mean;
            variance += diff * diff;
        }
        variance /= len;

        // THD estimation
        double thd = std::sqrt(variance) / (rms + 1e-10);
        return std::min(thd * 100.0, 100.0);  // Cap at 100%
    }

    // Detect ringing (oscillations after transients)
    int detectRinging(const std::vector<float>& signal) {
        int ringingCount = 0;

        // Look for sustained oscillations after sudden changes
        for (size_t i = 100; i < signal.size() - 100; ++i) {
            float delta = std::abs(signal[i] - signal[i-1]);

            // Detect sudden change
            if (delta > 0.1f) {
                // Check for ringing in next 100 samples
                float sumOscillation = 0.0f;
                for (size_t j = i; j < i + 100 && j < signal.size(); ++j) {
                    sumOscillation += std::abs(signal[j]);
                }

                // If significant energy continues, it's ringing
                if (sumOscillation / 100.0f > 0.05f && delta < 0.5f) {
                    ringingCount++;
                    i += 100;  // Skip ahead
                }
            }
        }

        return ringingCount;
    }

    // Calculate phase coherence between L/R channels
    double calculatePhaseCoherence(const std::vector<float>& left,
                                   const std::vector<float>& right) {
        if (left.size() != right.size()) return 0.0;

        double correlation = 0.0;
        double leftPower = 0.0;
        double rightPower = 0.0;

        for (size_t i = 0; i < left.size(); ++i) {
            correlation += left[i] * right[i];
            leftPower += left[i] * left[i];
            rightPower += right[i] * right[i];
        }

        double coherence = correlation / (std::sqrt(leftPower * rightPower) + 1e-10);
        return std::abs(coherence);
    }

    // Analyze filter output
    FilterTestMetrics analyzeFilterOutput(const AudioFile& input,
                                          const AudioFile& output) {
        FilterTestMetrics metrics;

        // Peak level
        float peakL = 0.0f, peakR = 0.0f;
        for (size_t i = 0; i < output.leftChannel.size(); ++i) {
            peakL = std::max(peakL, std::abs(output.leftChannel[i]));
            peakR = std::max(peakR, std::abs(output.rightChannel[i]));
        }
        metrics.peakLevel = std::max(peakL, peakR);

        // RMS level
        double sumL = 0.0, sumR = 0.0;
        for (size_t i = 0; i < output.leftChannel.size(); ++i) {
            sumL += output.leftChannel[i] * output.leftChannel[i];
            sumR += output.rightChannel[i] * output.rightChannel[i];
        }
        metrics.rmsLevel = std::sqrt((sumL + sumR) / (2.0 * output.leftChannel.size()));

        // DC offset
        double dcL = 0.0, dcR = 0.0;
        for (size_t i = 0; i < output.leftChannel.size(); ++i) {
            dcL += output.leftChannel[i];
            dcR += output.rightChannel[i];
        }
        metrics.dcOffset = std::max(std::abs(dcL / output.leftChannel.size()),
                                    std::abs(dcR / output.rightChannel.size()));

        // THD
        metrics.thd = calculateTHD(output.leftChannel);

        // Ringing detection
        metrics.ringingDetected = detectRinging(output.leftChannel);

        // Phase coherence
        metrics.phaseCoherence = calculatePhaseCoherence(output.leftChannel,
                                                         output.rightChannel);

        // Clipping detection
        metrics.hasClipping = (metrics.peakLevel >= 0.99f);

        // Instability detection (sudden gain changes)
        metrics.hasInstability = false;
        for (size_t i = 1; i < output.leftChannel.size(); ++i) {
            float delta = std::abs(output.leftChannel[i] - output.leftChannel[i-1]);
            if (delta > 1.0f) {
                metrics.hasInstability = true;
                break;
            }
        }

        // Noise floor
        std::vector<float> amplitudes;
        for (size_t i = 0; i < output.leftChannel.size(); ++i) {
            amplitudes.push_back(std::abs(output.leftChannel[i]));
        }
        std::sort(amplitudes.begin(), amplitudes.end());
        size_t quietSamples = amplitudes.size() / 10;
        double noiseSum = 0.0;
        for (size_t i = 0; i < quietSamples; ++i) {
            noiseSum += amplitudes[i] * amplitudes[i];
        }
        metrics.noiseFloor = std::sqrt(noiseSum / quietSamples);

        // Smoothness (measure of frequency response flatness)
        // Calculate by looking at short-term RMS variations
        int windowSize = 1024;
        std::vector<double> shortTermRMS;
        for (size_t i = 0; i < output.leftChannel.size() - windowSize; i += windowSize/2) {
            double sum = 0.0;
            for (size_t j = i; j < i + windowSize; ++j) {
                sum += output.leftChannel[j] * output.leftChannel[j];
            }
            shortTermRMS.push_back(std::sqrt(sum / windowSize));
        }

        // Calculate variance of short-term RMS
        double meanRMS = 0.0;
        for (double rms : shortTermRMS) {
            meanRMS += rms;
        }
        meanRMS /= shortTermRMS.size();

        double variance = 0.0;
        for (double rms : shortTermRMS) {
            variance += (rms - meanRMS) * (rms - meanRMS);
        }
        variance /= shortTermRMS.size();
        metrics.smoothness = 1.0 / (1.0 + variance * 100.0);  // 0-1, higher is smoother

        // Resonance peak (ratio of peak to RMS)
        metrics.resonancePeak = metrics.peakLevel / (metrics.rmsLevel + 1e-10);

        // Artifact count
        metrics.artifactCount = 0;
        float artifactThreshold = 0.5f;
        for (size_t i = 1; i < output.leftChannel.size(); ++i) {
            if (std::abs(output.leftChannel[i] - output.leftChannel[i-1]) > artifactThreshold) {
                metrics.artifactCount++;
            }
        }

        return metrics;
    }

    // Assign grade based on metrics
    char assignGrade(const FilterTestMetrics& metrics) {
        int score = 100;

        // Penalties
        if (metrics.hasInstability) score -= 50;
        if (metrics.hasClipping) score -= 30;
        if (metrics.ringingDetected > 5) score -= 25;
        else if (metrics.ringingDetected > 0) score -= 10;
        if (metrics.thd > 5.0) score -= 20;
        else if (metrics.thd > 1.0) score -= 10;
        if (metrics.dcOffset > 0.01) score -= 15;
        if (metrics.phaseCoherence < 0.9) score -= 15;
        if (metrics.smoothness < 0.5) score -= 10;
        if (metrics.artifactCount > 100) score -= 20;
        else if (metrics.artifactCount > 50) score -= 10;

        // Grade assignment
        if (score >= 90) return 'A';
        if (score >= 80) return 'B';
        if (score >= 70) return 'C';
        if (score >= 60) return 'D';
        return 'F';
    }

    // Get issues description
    std::string getIssues(const FilterTestMetrics& metrics) {
        std::vector<std::string> issues;

        if (metrics.hasInstability) {
            issues.push_back("INSTABILITY DETECTED");
        }
        if (metrics.hasClipping) {
            issues.push_back("Clipping");
        }
        if (metrics.ringingDetected > 0) {
            issues.push_back("Ringing (" + std::to_string(metrics.ringingDetected) + " events)");
        }
        if (metrics.thd > 1.0) {
            issues.push_back("High THD: " + std::to_string(int(metrics.thd)) + "%");
        }
        if (metrics.dcOffset > 0.01) {
            issues.push_back("DC offset");
        }
        if (metrics.phaseCoherence < 0.9) {
            issues.push_back("Phase issues");
        }
        if (metrics.artifactCount > 50) {
            issues.push_back("Artifacts: " + std::to_string(metrics.artifactCount));
        }

        if (issues.empty()) return "None";

        std::string result;
        for (size_t i = 0; i < issues.size(); ++i) {
            if (i > 0) result += "; ";
            result += issues[i];
        }
        return result;
    }

    // Determine filter character
    std::string determineCharacter(const FilterTestMetrics& metrics, double thd) {
        if (metrics.thd < 0.1 && metrics.smoothness > 0.9) {
            return "Transparent";
        } else if (metrics.thd < 0.5 && metrics.smoothness > 0.7) {
            return "Clean";
        } else if (metrics.thd < 2.0) {
            return "Warm/Colored";
        } else {
            return "Aggressive/Distorted";
        }
    }

    // Get engine name
    std::string getEngineName(int engineID) {
        switch (engineID) {
            case 7: return "ParametricEQ";
            case 8: return "VintageConsoleEQ";
            case 9: return "LadderFilter";
            case 10: return "StateVariableFilter";
            case 11: return "FormantFilter";
            case 12: return "EnvelopeFilter";
            case 13: return "CombResonator";
            case 14: return "VocalFormant";
            default: return "Unknown";
        }
    }

    // Process audio through engine
    AudioFile processAudio(EngineBase* engine, const AudioFile& input) {
        AudioFile output;
        output.leftChannel = input.leftChannel;
        output.rightChannel = input.rightChannel;
        output.sampleRate = input.sampleRate;
        output.filename = input.filename;
        output.description = input.description;

        size_t numSamples = input.leftChannel.size();
        for (size_t pos = 0; pos < numSamples; pos += bufferSize) {
            size_t chunkSize = std::min(bufferSize, static_cast<int>(numSamples - pos));

            // Create JUCE AudioBuffer
            juce::AudioBuffer<float> buffer(2, chunkSize);

            // Copy input data to buffer
            for (size_t i = 0; i < chunkSize; ++i) {
                buffer.setSample(0, i, output.leftChannel[pos + i]);
                buffer.setSample(1, i, output.rightChannel[pos + i]);
            }

            // Process
            engine->process(buffer);

            // Copy back
            for (size_t i = 0; i < chunkSize; ++i) {
                output.leftChannel[pos + i] = buffer.getSample(0, i);
                output.rightChannel[pos + i] = buffer.getSample(1, i);
            }
        }

        return output;
    }

    // Test single engine with material
    void testEngineWithMaterial(int engineID, const AudioFile& material,
                                const std::string& testType,
                                float param0 = 0.5f, float param1 = 0.5f,
                                float param2 = 0.5f, float param3 = 0.5f) {
        auto engine = ComprehensiveTHDEngineFactory::createEngine(engineID);
        if (!engine) {
            std::cerr << "Failed to create engine " << engineID << std::endl;
            return;
        }

        engine->prepareToPlay(sampleRate, bufferSize);

        // Set parameters using updateParameters
        std::map<int, float> params;
        params[0] = param0;
        params[1] = param1;
        params[2] = param2;
        params[3] = param3;
        engine->updateParameters(params);

        // Process
        AudioFile processed = processAudio(engine.get(), material);

        // Analyze
        FilterTestMetrics metrics = analyzeFilterOutput(material, processed);

        // Create result
        FilterTestResult result;
        result.engineID = engineID;
        result.engineName = getEngineName(engineID);
        result.materialName = material.filename;
        result.testType = testType;
        result.metrics = metrics;
        result.grade = assignGrade(metrics);
        result.issues = getIssues(metrics);
        result.character = determineCharacter(metrics, metrics.thd);

        // Determine recommended resonance limit
        if (metrics.hasInstability) {
            result.recommendedResonanceLimit = 0.5;
        } else if (metrics.ringingDetected > 3) {
            result.recommendedResonanceLimit = 0.7;
        } else {
            result.recommendedResonanceLimit = 0.9;
        }

        result.productionReady = (result.grade != 'F' && !metrics.hasInstability);

        results.push_back(result);

        // Save audio for failed tests
        if (result.grade == 'F' || metrics.hasInstability) {
            std::string outputFilename = "filter_output_engine" + std::to_string(engineID) +
                                        "_" + testType + "_" + material.filename;
            saveWAV(outputFilename, processed);
        }

        engine->reset();
    }

public:
    // Load test materials
    bool loadTestMaterials(const std::string& materialsDir) {
        std::cout << "\nLoading test materials from: " << materialsDir << std::endl;

        std::vector<std::string> filenames = {
            "drum_loop_120bpm.wav",
            "bass_line_e1_e2.wav",
            "vocal_sample_formants.wav",
            "pink_noise_sustained.wav"
        };

        for (const auto& filename : filenames) {
            AudioFile audio;
            std::string fullPath = materialsDir + "/" + filename;

            if (loadWAV(fullPath, audio)) {
                audio.description = filename;
                testMaterials.push_back(audio);
                std::cout << "  Loaded: " << filename << std::endl;
            } else {
                std::cerr << "  WARNING: Could not load " << fullPath << std::endl;
            }
        }

        std::cout << "Loaded " << testMaterials.size() << " test materials\n";
        return !testMaterials.empty();
    }

    // Test all filters
    void testAllFilters() {
        std::cout << "\n============================================================\n";
        std::cout << "REAL-WORLD FILTER TESTING - ENGINES 7-14\n";
        std::cout << "============================================================\n\n";

        int testCount = 0;
        int totalTests = filterEngineIDs.size() * testMaterials.size() * 3;  // 3 test types

        for (int engineID : filterEngineIDs) {
            std::cout << "\n[Engine " << engineID << "] " << getEngineName(engineID) << std::endl;
            std::cout << std::string(60, '-') << std::endl;

            for (const auto& material : testMaterials) {
                // Test 1: Normal operation (moderate settings)
                testCount++;
                float progress = (testCount * 100.0f) / totalTests;
                std::cout << "  [" << std::fixed << std::setprecision(1) << progress
                          << "%] Normal: " << material.description << "... ";

                testEngineWithMaterial(engineID, material, "normal", 0.5f, 0.5f, 0.5f, 0.5f);
                std::cout << "Grade: " << results.back().grade << std::endl;

                // Test 2: Frequency sweep (vary cutoff/frequency)
                testCount++;
                progress = (testCount * 100.0f) / totalTests;
                std::cout << "  [" << std::fixed << std::setprecision(1) << progress
                          << "%] Freq sweep: " << material.description << "... ";

                testEngineWithMaterial(engineID, material, "freq_sweep", 0.2f, 0.5f, 0.5f, 0.5f);
                std::cout << "Grade: " << results.back().grade << std::endl;

                // Test 3: High resonance
                testCount++;
                progress = (testCount * 100.0f) / totalTests;
                std::cout << "  [" << std::fixed << std::setprecision(1) << progress
                          << "%] High resonance: " << material.description << "... ";

                testEngineWithMaterial(engineID, material, "resonance_sweep", 0.5f, 0.9f, 0.5f, 0.5f);
                std::cout << "Grade: " << results.back().grade;

                if (results.back().metrics.hasInstability) {
                    std::cout << " [INSTABILITY WARNING]";
                }
                std::cout << std::endl;
            }
        }

        std::cout << "\n============================================================\n";
        std::cout << "TESTING COMPLETE\n";
        std::cout << "============================================================\n";
    }

    // Generate comprehensive report
    void generateReport(const std::string& filename) {
        std::ofstream report(filename);

        report << "# REAL-WORLD FILTER TESTING REPORT\n";
        report << "# Engines 7-14: Comprehensive Filter/EQ Analysis\n\n";

        report << "**Test Date**: " << __DATE__ << " " << __TIME__ << "\n";
        report << "**Engines Tested**: 8 (ID 7-14)\n";
        report << "**Test Materials**: " << testMaterials.size() << "\n";
        report << "**Test Types**: Normal, Frequency Sweep, High Resonance\n";
        report << "**Total Tests**: " << results.size() << "\n\n";

        report << "---\n\n";

        // Summary by engine
        report << "## EXECUTIVE SUMMARY\n\n";
        report << "| Engine | Name | Avg Grade | Character | Prod Ready | Issues |\n";
        report << "|--------|------|-----------|-----------|------------|--------|\n";

        for (int engineID : filterEngineIDs) {
            std::vector<FilterTestResult*> engineResults;
            for (auto& result : results) {
                if (result.engineID == engineID) {
                    engineResults.push_back(&result);
                }
            }

            if (engineResults.empty()) continue;

            // Calculate average grade
            int avgScore = 0;
            int instabilityCount = 0;
            std::string character = engineResults[0]->character;

            for (const auto* result : engineResults) {
                switch (result->grade) {
                    case 'A': avgScore += 95; break;
                    case 'B': avgScore += 85; break;
                    case 'C': avgScore += 75; break;
                    case 'D': avgScore += 65; break;
                    case 'F': avgScore += 45; break;
                }
                if (result->metrics.hasInstability) instabilityCount++;
            }
            avgScore /= engineResults.size();

            char avgGrade;
            if (avgScore >= 90) avgGrade = 'A';
            else if (avgScore >= 80) avgGrade = 'B';
            else if (avgScore >= 70) avgGrade = 'C';
            else if (avgScore >= 60) avgGrade = 'D';
            else avgGrade = 'F';

            bool prodReady = (avgGrade != 'F' && instabilityCount == 0);

            std::string issues = "None";
            if (instabilityCount > 0) {
                issues = "Instability x" + std::to_string(instabilityCount);
            }

            report << "| " << engineID << " | " << getEngineName(engineID) << " | "
                   << avgGrade << " | " << character << " | "
                   << (prodReady ? "YES" : "NO") << " | " << issues << " |\n";
        }

        report << "\n---\n\n";

        // Detailed results
        report << "## DETAILED TEST RESULTS\n\n";

        for (int engineID : filterEngineIDs) {
            report << "### Engine " << engineID << ": " << getEngineName(engineID) << "\n\n";

            std::vector<FilterTestResult*> engineResults;
            for (auto& result : results) {
                if (result.engineID == engineID) {
                    engineResults.push_back(&result);
                }
            }

            if (engineResults.empty()) continue;

            // Overall assessment
            char bestGrade = 'F', worstGrade = 'A';
            double avgResonanceLimit = 0.0;
            int instabilityCount = 0;

            for (const auto* result : engineResults) {
                if (result->grade < bestGrade) bestGrade = result->grade;
                if (result->grade > worstGrade) worstGrade = result->grade;
                avgResonanceLimit += result->recommendedResonanceLimit;
                if (result->metrics.hasInstability) instabilityCount++;
            }
            avgResonanceLimit /= engineResults.size();

            report << "**Character**: " << engineResults[0]->character << "\n";
            report << "**Grade Range**: " << bestGrade << " to " << worstGrade << "\n";
            report << "**Recommended Resonance Limit**: "
                   << std::fixed << std::setprecision(2) << avgResonanceLimit << "\n";
            report << "**Instability Issues**: " << instabilityCount << "\n";
            report << "**Production Ready**: "
                   << (worstGrade != 'F' && instabilityCount == 0 ? "YES" : "NO") << "\n\n";

            // Results table
            report << "| Material | Test Type | Grade | Issues | THD% | Ringing | Phase |\n";
            report << "|----------|-----------|-------|--------|------|---------|-------|\n";

            for (const auto* result : engineResults) {
                std::string matName = result->materialName;
                size_t lastSlash = matName.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    matName = matName.substr(lastSlash + 1);
                }

                report << "| " << matName << " | " << result->testType << " | "
                       << result->grade << " | " << result->issues << " | "
                       << std::fixed << std::setprecision(1) << result->metrics.thd << " | "
                       << result->metrics.ringingDetected << " | "
                       << std::fixed << std::setprecision(2) << result->metrics.phaseCoherence << " |\n";
            }

            report << "\n";

            // Specific recommendations
            if (instabilityCount > 0) {
                report << "**WARNING**: Instability detected - requires urgent attention!\n\n";
            } else if (worstGrade == 'F') {
                report << "**Note**: Some tests failed - review recommended\n\n";
            } else if (bestGrade == 'A' && worstGrade == 'A') {
                report << "**Excellent**: Perfect performance across all tests\n\n";
            }

            report << "---\n\n";
        }

        // Final recommendations
        report << "## RECOMMENDATIONS\n\n";

        report << "### Production-Ready Filters\n\n";
        for (int engineID : filterEngineIDs) {
            bool hasFailures = false;
            bool hasInstability = false;

            for (const auto& result : results) {
                if (result.engineID == engineID) {
                    if (result.grade == 'F') hasFailures = true;
                    if (result.metrics.hasInstability) hasInstability = true;
                }
            }

            if (!hasFailures && !hasInstability) {
                report << "- Engine " << engineID << ": " << getEngineName(engineID) << "\n";
            }
        }

        report << "\n### Filters Requiring Attention\n\n";
        for (int engineID : filterEngineIDs) {
            bool hasFailures = false;
            bool hasInstability = false;

            for (const auto& result : results) {
                if (result.engineID == engineID) {
                    if (result.grade == 'F') hasFailures = true;
                    if (result.metrics.hasInstability) hasInstability = true;
                }
            }

            if (hasFailures || hasInstability) {
                report << "- Engine " << engineID << ": " << getEngineName(engineID);
                if (hasInstability) report << " [INSTABILITY]";
                if (hasFailures) report << " [FAILURES]";
                report << "\n";
            }
        }

        report << "\n### Resonance Stability Summary\n\n";
        report << "| Engine | Recommended Max Resonance | Self-Oscillation |\n";
        report << "|--------|---------------------------|------------------|\n";

        for (int engineID : filterEngineIDs) {
            double avgLimit = 0.0;
            int count = 0;
            bool hasOscillation = false;

            for (const auto& result : results) {
                if (result.engineID == engineID && result.testType == "resonance_sweep") {
                    avgLimit += result.recommendedResonanceLimit;
                    count++;
                    if (result.metrics.hasInstability) hasOscillation = true;
                }
            }

            if (count > 0) {
                avgLimit /= count;
                report << "| " << getEngineName(engineID) << " | "
                       << std::fixed << std::setprecision(2) << avgLimit << " | "
                       << (hasOscillation ? "UNSTABLE" : "Musical") << " |\n";
            }
        }

        report << "\n---\n\n";
        report << "## CONCLUSION\n\n";
        report << "This report evaluates all filter/EQ engines (7-14) with:\n";
        report << "- Real-world musical materials\n";
        report << "- Frequency sweep testing\n";
        report << "- High resonance stability testing\n";
        report << "- Ringing and artifact detection\n";
        report << "- Phase coherence analysis\n";
        report << "- THD measurement at high resonance\n\n";

        report << "**Grading Scale**:\n";
        report << "- A: Excellent (transparent, no artifacts, stable)\n";
        report << "- B: Good (minor coloration, stable)\n";
        report << "- C: Acceptable (noticeable coloration but usable)\n";
        report << "- D: Poor (significant issues)\n";
        report << "- F: Failed (instability, clipping, severe artifacts)\n\n";

        report.close();
        std::cout << "\nReport generated: " << filename << std::endl;
    }
};

int main() {
    std::cout << "============================================================\n";
    std::cout << "REAL-WORLD FILTER TESTING - ENGINES 7-14\n";
    std::cout << "Project Chimera Phoenix v3.0\n";
    std::cout << "============================================================\n";

    FilterRealWorldTester tester;

    // Load test materials
    if (!tester.loadTestMaterials("real_world_test_materials")) {
        std::cerr << "\nERROR: Failed to load test materials!\n";
        std::cerr << "Run: python3 generate_musical_materials.py first\n";
        return 1;
    }

    // Test all filters
    tester.testAllFilters();

    // Generate report
    tester.generateReport("FILTER_REALWORLD_TESTING_REPORT.md");

    std::cout << "\n============================================================\n";
    std::cout << "TESTING COMPLETE\n";
    std::cout << "Check FILTER_REALWORLD_TESTING_REPORT.md for full results\n";
    std::cout << "============================================================\n";

    return 0;
}
