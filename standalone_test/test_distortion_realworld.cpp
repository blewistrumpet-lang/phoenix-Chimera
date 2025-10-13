/**
 * REAL-WORLD DISTORTION ENGINES TEST (15-22)
 *
 * Comprehensive testing of distortion engines with real-world audio:
 * - 15: VintageTubePreamp_Studio (Tube saturation)
 * - 16: WaveFolder (Wave folding)
 * - 17: HarmonicExciter_Platinum (Harmonic enhancement)
 * - 18: BitCrusher (Digital degradation)
 * - 19: MultibandSaturator (Multiband saturation)
 * - 20: MuffFuzz (Big Muff fuzz)
 * - 21: RodentDistortion (RAT-style distortion)
 * - 22: KStyleOverdrive (Tube Screamer-style)
 *
 * Tests:
 * - Drive parameter sweeps (clean to extreme)
 * - Harmonic richness analysis (even/odd balance)
 * - Digital artifact detection
 * - Gain staging validation
 * - DC offset measurement
 * - THD at various drive levels
 * - Frequency response analysis
 */

#include "DistortionEngineFactory.h"
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

struct HarmonicAnalysis {
    double fundamental;
    double h2;  // 2nd harmonic (even)
    double h3;  // 3rd harmonic (odd)
    double h4;  // 4th harmonic
    double h5;  // 5th harmonic
    double evenHarmonics;  // Total even
    double oddHarmonics;   // Total odd
    double thd;
    double thdPlusNoise;
    std::string character;  // "warm", "harsh", "smooth", "aggressive"
};

struct DistortionMetrics {
    double peakLevel;
    double rmsLevel;
    double dcOffset;
    double crestFactor;
    bool hasClipping;
    bool hasAliasing;
    bool hasHarshness;
    int artifactCount;
    double gainCompensation;  // Output level vs input
    HarmonicAnalysis harmonics;
};

struct DriveTestResult {
    float driveLevel;
    DistortionMetrics metrics;
    char grade;  // A/B/C/D/F
    std::string notes;
};

struct EngineTestResult {
    int engineID;
    std::string engineName;
    std::string materialName;
    std::vector<DriveTestResult> driveSweep;
    char overallGrade;
    std::string character;
    std::string gainStagingAdvice;
    bool productionReady;
};

class DistortionRealWorldTester {
private:
    std::vector<AudioFile> testMaterials;
    std::vector<EngineTestResult> results;
    int sampleRate = 48000;
    int bufferSize = 512;

    // FFT helper for harmonic analysis
    void computeFFT(const std::vector<float>& signal, std::vector<double>& magnitudes) {
        int N = signal.size();
        magnitudes.resize(N / 2);

        // Simple DFT (not optimized, but sufficient for analysis)
        for (int k = 0; k < N / 2; ++k) {
            double real = 0.0, imag = 0.0;
            for (int n = 0; n < N; ++n) {
                double angle = 2.0 * M_PI * k * n / N;
                real += signal[n] * std::cos(angle);
                imag -= signal[n] * std::sin(angle);
            }
            magnitudes[k] = std::sqrt(real * real + imag * imag) / N;
        }
    }

    // Analyze harmonic content
    HarmonicAnalysis analyzeHarmonics(const std::vector<float>& signal, int sampleRate) {
        HarmonicAnalysis analysis;

        // Find fundamental frequency (assume dominant frequency)
        std::vector<double> magnitudes;
        computeFFT(signal, magnitudes);

        // Find peak (fundamental)
        double maxMag = 0.0;
        int fundamentalBin = 0;
        int minBin = 20;  // Ignore DC and very low frequencies

        for (size_t i = minBin; i < magnitudes.size() / 2; ++i) {
            if (magnitudes[i] > maxMag) {
                maxMag = magnitudes[i];
                fundamentalBin = i;
            }
        }

        analysis.fundamental = (fundamentalBin * sampleRate) / (double)signal.size();

        // Measure harmonics
        double binWidth = sampleRate / (double)signal.size();
        int h2Bin = fundamentalBin * 2;
        int h3Bin = fundamentalBin * 3;
        int h4Bin = fundamentalBin * 4;
        int h5Bin = fundamentalBin * 5;

        auto getBinMagnitude = [&](int bin) {
            if (bin >= 0 && bin < (int)magnitudes.size()) {
                // Average nearby bins for robustness
                double sum = 0.0;
                int count = 0;
                for (int i = bin - 2; i <= bin + 2; ++i) {
                    if (i >= 0 && i < (int)magnitudes.size()) {
                        sum += magnitudes[i];
                        count++;
                    }
                }
                return sum / count;
            }
            return 0.0;
        };

        analysis.h2 = getBinMagnitude(h2Bin);
        analysis.h3 = getBinMagnitude(h3Bin);
        analysis.h4 = getBinMagnitude(h4Bin);
        analysis.h5 = getBinMagnitude(h5Bin);

        // Calculate even/odd ratios
        analysis.evenHarmonics = analysis.h2 + analysis.h4;
        analysis.oddHarmonics = analysis.h3 + analysis.h5;

        // THD calculation
        double harmonicPower = analysis.h2*analysis.h2 + analysis.h3*analysis.h3 +
                               analysis.h4*analysis.h4 + analysis.h5*analysis.h5;
        double fundamentalPower = maxMag * maxMag;

        if (fundamentalPower > 0.0) {
            analysis.thd = std::sqrt(harmonicPower / fundamentalPower) * 100.0;
        } else {
            analysis.thd = 0.0;
        }

        // Calculate total noise including harmonics
        double totalPower = 0.0;
        for (const auto& mag : magnitudes) {
            totalPower += mag * mag;
        }
        analysis.thdPlusNoise = std::sqrt((totalPower - fundamentalPower) / fundamentalPower) * 100.0;

        // Characterize distortion type
        if (analysis.evenHarmonics > analysis.oddHarmonics * 1.5) {
            analysis.character = "warm (even-dominant)";
        } else if (analysis.oddHarmonics > analysis.evenHarmonics * 1.5) {
            analysis.character = "aggressive (odd-dominant)";
        } else if (analysis.thd < 5.0) {
            analysis.character = "smooth (low THD)";
        } else if (analysis.thd > 20.0) {
            analysis.character = "harsh (high THD)";
        } else {
            analysis.character = "balanced";
        }

        return analysis;
    }

    // Load RAW stereo float32 file (new format)
    bool loadRAW(const std::string& filename, AudioFile& audio) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open: " << filename << std::endl;
            return false;
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        int numSamples = fileSize / (2 * sizeof(float));  // Stereo
        audio.leftChannel.resize(numSamples);
        audio.rightChannel.resize(numSamples);
        audio.sampleRate = sampleRate;
        audio.filename = filename;

        // Read interleaved stereo float32
        for (int i = 0; i < numSamples; ++i) {
            file.read(reinterpret_cast<char*>(&audio.leftChannel[i]), sizeof(float));
            file.read(reinterpret_cast<char*>(&audio.rightChannel[i]), sizeof(float));
        }

        return true;
    }

    // Load WAV file (legacy)
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

    // Analyze distortion quality
    DistortionMetrics analyzeDistortion(const AudioFile& audio, const AudioFile& input) {
        DistortionMetrics metrics;

        // Peak and RMS levels
        float peakL = 0.0f, peakR = 0.0f;
        double sumL = 0.0, sumR = 0.0;

        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            peakL = std::max(peakL, std::abs(audio.leftChannel[i]));
            peakR = std::max(peakR, std::abs(audio.rightChannel[i]));
            sumL += audio.leftChannel[i] * audio.leftChannel[i];
            sumR += audio.rightChannel[i] * audio.rightChannel[i];
        }

        metrics.peakLevel = std::max(peakL, peakR);
        metrics.rmsLevel = std::sqrt((sumL + sumR) / (2.0 * audio.leftChannel.size()));

        // Crest factor (dynamic range)
        metrics.crestFactor = metrics.peakLevel / (metrics.rmsLevel + 1e-10);

        // DC offset
        double dcL = 0.0, dcR = 0.0;
        for (size_t i = 0; i < audio.leftChannel.size(); ++i) {
            dcL += audio.leftChannel[i];
            dcR += audio.rightChannel[i];
        }
        metrics.dcOffset = std::max(std::abs(dcL), std::abs(dcR)) / audio.leftChannel.size();

        // Clipping detection
        metrics.hasClipping = (metrics.peakLevel >= 0.99f);

        // Aliasing detection (check for energy above Nyquist/2)
        std::vector<double> magnitudes;
        computeFFT(audio.leftChannel, magnitudes);
        double highFreqEnergy = 0.0;
        int highFreqStart = magnitudes.size() * 3 / 4;  // Upper quarter
        for (size_t i = highFreqStart; i < magnitudes.size(); ++i) {
            highFreqEnergy += magnitudes[i] * magnitudes[i];
        }
        metrics.hasAliasing = (highFreqEnergy > 0.01);  // Threshold

        // Harshness detection (excessive high-frequency content)
        double midFreqEnergy = 0.0;
        int midStart = magnitudes.size() / 4;
        int midEnd = magnitudes.size() / 2;
        for (int i = midStart; i < midEnd; ++i) {
            midFreqEnergy += magnitudes[i] * magnitudes[i];
        }
        metrics.hasHarshness = (highFreqEnergy / (midFreqEnergy + 1e-10) > 0.5);

        // Artifact detection (discontinuities)
        metrics.artifactCount = 0;
        float threshold = 0.5f;
        for (size_t i = 1; i < audio.leftChannel.size(); ++i) {
            float diff = std::abs(audio.leftChannel[i] - audio.leftChannel[i-1]);
            if (diff > threshold) {
                metrics.artifactCount++;
            }
        }

        // Gain compensation analysis
        double inputRMS = 0.0;
        for (size_t i = 0; i < input.leftChannel.size(); ++i) {
            inputRMS += input.leftChannel[i] * input.leftChannel[i];
        }
        inputRMS = std::sqrt(inputRMS / input.leftChannel.size());
        metrics.gainCompensation = metrics.rmsLevel / (inputRMS + 1e-10);

        // Harmonic analysis
        metrics.harmonics = analyzeHarmonics(audio.leftChannel, audio.sampleRate);

        return metrics;
    }

    // Grade a drive test
    char gradeDistortion(const DistortionMetrics& metrics, float driveLevel) {
        int score = 100;

        // Clipping penalty (unless at extreme drive)
        if (metrics.hasClipping && driveLevel < 0.8f) {
            score -= 30;
        }

        // Aliasing penalty (digital artifacts)
        if (metrics.hasAliasing) {
            score -= 25;
        }

        // Harsh high-frequency penalty
        if (metrics.hasHarshness && driveLevel < 0.5f) {
            score -= 20;
        }

        // DC offset penalty
        if (metrics.dcOffset > 0.01) {
            score -= 15;
        }

        // Discontinuity penalty
        if (metrics.artifactCount > 50) {
            score -= 20;
        }

        // Bad gain staging
        if (metrics.gainCompensation > 2.0 || metrics.gainCompensation < 0.3) {
            score -= 10;
        }

        if (score >= 90) return 'A';
        if (score >= 80) return 'B';
        if (score >= 70) return 'C';
        if (score >= 60) return 'D';
        return 'F';
    }

    // Get engine name
    std::string getEngineName(int engineID) {
        switch (engineID) {
            case 15: return "Vintage Tube Preamp Studio";
            case 16: return "Wave Folder";
            case 17: return "Harmonic Exciter Platinum";
            case 18: return "Bit Crusher";
            case 19: return "Multiband Saturator";
            case 20: return "Muff Fuzz";
            case 21: return "Rodent Distortion";
            case 22: return "K-Style Overdrive";
            default: return "Unknown";
        }
    }

    // Save spectral plot CSV
    void saveSpectralData(int engineID, const std::string& materialName,
                          float driveLevel, const std::vector<double>& magnitudes) {
        std::string filename = "distortion_spectrum_" + std::to_string(engineID) +
                               "_drive_" + std::to_string(int(driveLevel * 100)) + ".csv";
        std::ofstream file(filename);

        file << "Frequency,Magnitude\n";
        double freqStep = sampleRate / (2.0 * magnitudes.size());

        for (size_t i = 0; i < magnitudes.size(); ++i) {
            file << (i * freqStep) << "," << magnitudes[i] << "\n";
        }
    }

public:
    // Load test materials (guitar, bass, drums, synth)
    bool loadTestMaterials(const std::string& materialsDir = ".") {
        std::cout << "\nLoading distortion test materials...\n";
        std::cout << "============================================================\n";

        std::vector<std::string> filenames = {
            "distortion_test_guitar_di.raw",
            "distortion_test_bass.raw",
            "distortion_test_drums.raw",
            "distortion_test_synth.raw"
        };

        std::vector<std::string> descriptions = {
            "Guitar DI",
            "Bass Guitar",
            "Drums",
            "Synth Lead"
        };

        for (size_t i = 0; i < filenames.size(); ++i) {
            AudioFile audio;
            std::string fullPath = (materialsDir == "." ? "" : materialsDir + "/") + filenames[i];

            if (loadRAW(fullPath, audio)) {
                audio.description = descriptions[i];
                testMaterials.push_back(audio);
                std::cout << "  ✓ Loaded: " << descriptions[i] << " ("
                          << audio.leftChannel.size() << " samples)\n";
            } else {
                std::cerr << "  ✗ Failed: " << fullPath << "\n";
            }
        }

        std::cout << "\nLoaded " << testMaterials.size() << " materials\n";
        return !testMaterials.empty();
    }

    // Test all distortion engines (15-22)
    void testDistortionEngines() {
        std::cout << "\n============================================================\n";
        std::cout << "TESTING DISTORTION ENGINES 15-22\n";
        std::cout << "============================================================\n\n";

        // Drive levels to test (clean to extreme)
        std::vector<float> driveLevels = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

        for (int engineID = 15; engineID <= 22; ++engineID) {
            std::string engineName = getEngineName(engineID);
            std::cout << "\n[Engine " << engineID << "] " << engineName << "\n";
            std::cout << std::string(60, '=') << "\n";

            for (const auto& material : testMaterials) {
                std::cout << "\n  Material: " << material.description << "\n";
                std::cout << "  " << std::string(50, '-') << "\n";

                EngineTestResult result;
                result.engineID = engineID;
                result.engineName = engineName;
                result.materialName = material.description;

                // Create engine
                auto engine = DistortionEngineFactory::createEngine(engineID);
                if (!engine) {
                    std::cout << "  ERROR: Failed to create engine\n";
                    continue;
                }

                engine->prepareToPlay(sampleRate, bufferSize);

                // Test each drive level
                for (float drive : driveLevels) {
                    std::cout << "    Drive " << std::fixed << std::setprecision(0)
                              << (drive * 100.0f) << "%... ";

                    // Set parameters with drive level
                    std::map<int, float> driveParams;
                    driveParams[0] = drive;  // Drive/gain parameter
                    for (int p = 1; p < 8; ++p) {
                        driveParams[p] = 0.5f;  // Neutral for others
                    }
                    engine->updateParameters(driveParams);

                    // Process audio
                    AudioFile processed;
                    processed.leftChannel = material.leftChannel;
                    processed.rightChannel = material.rightChannel;
                    processed.sampleRate = material.sampleRate;

                    size_t numSamples = material.leftChannel.size();
                    for (size_t pos = 0; pos < numSamples; pos += bufferSize) {
                        size_t chunkSize = std::min(bufferSize, static_cast<int>(numSamples - pos));

                        // Create JUCE AudioBuffer for processing
                        juce::AudioBuffer<float> buffer(2, chunkSize);
                        for (size_t i = 0; i < chunkSize; ++i) {
                            buffer.setSample(0, i, processed.leftChannel[pos + i]);
                            buffer.setSample(1, i, processed.rightChannel[pos + i]);
                        }

                        engine->process(buffer);

                        // Copy back to processed audio
                        for (size_t i = 0; i < chunkSize; ++i) {
                            processed.leftChannel[pos + i] = buffer.getSample(0, i);
                            processed.rightChannel[pos + i] = buffer.getSample(1, i);
                        }
                    }

                    // Analyze
                    DriveTestResult driveResult;
                    driveResult.driveLevel = drive;
                    driveResult.metrics = analyzeDistortion(processed, material);
                    driveResult.grade = gradeDistortion(driveResult.metrics, drive);

                    // Add notes
                    if (driveResult.metrics.hasClipping) {
                        driveResult.notes += "Clipping; ";
                    }
                    if (driveResult.metrics.hasAliasing) {
                        driveResult.notes += "Aliasing detected; ";
                    }
                    if (driveResult.metrics.dcOffset > 0.01) {
                        driveResult.notes += "DC offset; ";
                    }

                    result.driveSweep.push_back(driveResult);

                    std::cout << "Grade: " << driveResult.grade
                              << " | THD: " << std::fixed << std::setprecision(1)
                              << driveResult.metrics.harmonics.thd << "%"
                              << " | " << driveResult.metrics.harmonics.character << "\n";

                    // Save spectrum at key drive levels
                    if (drive == 0.5f || drive == 1.0f) {
                        std::vector<double> magnitudes;
                        computeFFT(processed.leftChannel, magnitudes);
                        saveSpectralData(engineID, material.description, drive, magnitudes);
                    }

                    // Save audio for extreme distortion
                    if (drive == 1.0f) {
                        std::string filename = "distortion_output_" + std::to_string(engineID) +
                                               "_" + material.description + "_drive100.wav";
                        std::replace(filename.begin(), filename.end(), ' ', '_');
                        saveWAV(filename, processed);
                    }
                }

                // Calculate overall grade for this material
                int totalScore = 0;
                for (const auto& dr : result.driveSweep) {
                    switch (dr.grade) {
                        case 'A': totalScore += 95; break;
                        case 'B': totalScore += 85; break;
                        case 'C': totalScore += 75; break;
                        case 'D': totalScore += 65; break;
                        case 'F': totalScore += 45; break;
                    }
                }
                totalScore /= result.driveSweep.size();

                if (totalScore >= 90) result.overallGrade = 'A';
                else if (totalScore >= 80) result.overallGrade = 'B';
                else if (totalScore >= 70) result.overallGrade = 'C';
                else if (totalScore >= 60) result.overallGrade = 'D';
                else result.overallGrade = 'F';

                // Characterize distortion
                auto& lastMetrics = result.driveSweep.back().metrics;
                result.character = lastMetrics.harmonics.character;

                // Gain staging advice
                if (lastMetrics.gainCompensation > 1.5) {
                    result.gainStagingAdvice = "Reduce output gain";
                } else if (lastMetrics.gainCompensation < 0.5) {
                    result.gainStagingAdvice = "Increase output gain";
                } else {
                    result.gainStagingAdvice = "Good gain staging";
                }

                result.productionReady = (result.overallGrade != 'F' &&
                                          !lastMetrics.hasAliasing);

                results.push_back(result);

                engine->reset();
            }
        }

        std::cout << "\n============================================================\n";
        std::cout << "DISTORTION TESTING COMPLETE\n";
        std::cout << "============================================================\n";
    }

    // Generate comprehensive report
    void generateReport(const std::string& filename) {
        std::ofstream report(filename);

        report << "# DISTORTION ENGINES REAL-WORLD TESTING REPORT\n\n";
        report << "**Test Date**: " << __DATE__ << " " << __TIME__ << "\n";
        report << "**Engines Tested**: 15-22 (8 distortion engines)\n";
        report << "**Test Materials**: Guitar, Bass, Drums\n";
        report << "**Drive Levels**: 0%, 25%, 50%, 75%, 100%\n\n";
        report << "---\n\n";

        // Summary
        report << "## EXECUTIVE SUMMARY\n\n";

        int productionReady = 0;
        for (const auto& result : results) {
            if (result.productionReady) productionReady++;
        }

        report << "**Production Ready**: " << productionReady << "/" << results.size()
               << " (" << (productionReady * 100 / results.size()) << "%)\n\n";

        report << "| Engine | Grade | Character | Gain Staging | Status |\n";
        report << "|--------|-------|-----------|--------------|--------|\n";

        for (const auto& result : results) {
            report << "| " << result.engineID << ": " << result.engineName
                   << " | " << result.overallGrade
                   << " | " << result.character
                   << " | " << result.gainStagingAdvice
                   << " | " << (result.productionReady ? "✅ Ready" : "⚠️ Issues")
                   << " |\n";
        }

        report << "\n---\n\n";

        // Detailed results
        report << "## DETAILED ANALYSIS BY ENGINE\n\n";

        for (int engineID = 15; engineID <= 22; ++engineID) {
            std::string engineName = getEngineName(engineID);
            report << "### Engine " << engineID << ": " << engineName << "\n\n";

            // Filter results for this engine
            std::vector<EngineTestResult*> engineResults;
            for (auto& result : results) {
                if (result.engineID == engineID) {
                    engineResults.push_back(&result);
                }
            }

            if (engineResults.empty()) continue;

            // Overall assessment
            int avgScore = 0;
            for (const auto* result : engineResults) {
                switch (result->overallGrade) {
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

            // Drive sweep results
            for (const auto* result : engineResults) {
                report << "#### " << result->materialName << "\n\n";
                report << "| Drive | THD | Character | Grade | Issues |\n";
                report << "|-------|-----|-----------|-------|--------|\n";

                for (const auto& dr : result->driveSweep) {
                    report << "| " << int(dr.driveLevel * 100) << "% "
                           << "| " << std::fixed << std::setprecision(1)
                           << dr.metrics.harmonics.thd << "% "
                           << "| " << dr.metrics.harmonics.character
                           << " | " << dr.grade
                           << " | " << (dr.notes.empty() ? "None" : dr.notes)
                           << " |\n";
                }

                report << "\n**Harmonic Balance** (at 100% drive):\n";
                auto& lastDrive = result->driveSweep.back();
                report << "- Even harmonics: " << std::fixed << std::setprecision(3)
                       << lastDrive.metrics.harmonics.evenHarmonics << "\n";
                report << "- Odd harmonics: " << lastDrive.metrics.harmonics.oddHarmonics << "\n";
                report << "- 2nd harmonic: " << lastDrive.metrics.harmonics.h2 << "\n";
                report << "- 3rd harmonic: " << lastDrive.metrics.harmonics.h3 << "\n\n";

                report << "**Gain Staging**: " << result->gainStagingAdvice << "\n";
                report << "**DC Offset**: " << (lastDrive.metrics.dcOffset * 100) << "%\n\n";
            }

            report << "---\n\n";
        }

        // Recommendations
        report << "## RECOMMENDATIONS\n\n";

        report << "### Production-Ready Distortions\n\n";
        bool hasReady = false;
        for (const auto& result : results) {
            if (result.productionReady && result.overallGrade >= 'B') {
                hasReady = true;
                report << "- **" << result.engineName << "** (Grade "
                       << result.overallGrade << "): "
                       << result.character << "\n";
            }
        }
        if (!hasReady) {
            report << "_No engines meet production-ready criteria_\n";
        }

        report << "\n### Needs Improvement\n\n";
        bool hasIssues = false;
        for (const auto& result : results) {
            if (!result.productionReady || result.overallGrade < 'C') {
                hasIssues = true;
                report << "- **" << result.engineName << "** (Grade "
                       << result.overallGrade << "): ";

                // Identify specific issues
                auto& lastDrive = result.driveSweep.back();
                if (lastDrive.metrics.hasAliasing) report << "Aliasing detected; ";
                if (lastDrive.metrics.hasClipping) report << "Poor gain staging; ";
                if (lastDrive.metrics.dcOffset > 0.01) report << "DC offset present; ";
                report << "\n";
            }
        }
        if (!hasIssues) {
            report << "_All engines performing well!_\n";
        }

        report << "\n---\n\n";

        report << "## AUDIO FILE OUTPUTS\n\n";
        report << "Generated audio files for analysis:\n\n";
        report << "- `distortion_output_[ID]_[material]_drive100.wav` - Full drive examples\n";
        report << "- `distortion_spectrum_[ID]_drive_[level].csv` - Spectral data\n\n";

        report << "---\n\n";

        report << "## CONCLUSION\n\n";
        report << "Distortion engines tested with real-world guitar, bass, and drum materials.\n";
        report << "Key evaluation criteria:\n\n";
        report << "- ✅ **Harmonic richness** - Musical harmonic content\n";
        report << "- ✅ **No aliasing** - Proper oversampling/filtering\n";
        report << "- ✅ **Gain compensation** - Appropriate output levels\n";
        report << "- ✅ **DC offset control** - Centered signal\n";
        report << "- ✅ **Character** - Warm, smooth, or aggressive as intended\n\n";

        report << "Engines with Grade B or better are suitable for production use.\n\n";

        report.close();
        std::cout << "\nReport generated: " << filename << "\n";
    }
};

int main() {
    std::cout << "============================================================\n";
    std::cout << "DISTORTION ENGINES REAL-WORLD TESTING\n";
    std::cout << "Engines 15-22\n";
    std::cout << "============================================================\n";

    DistortionRealWorldTester tester;

    // Load materials (from current directory)
    if (!tester.loadTestMaterials(".")) {
        std::cerr << "\nERROR: Failed to load test materials!\n";
        std::cerr << "Make sure to run generate_distortion_test_materials.py first!\n";
        return 1;
    }

    // Test distortion engines
    tester.testDistortionEngines();

    // Generate report
    tester.generateReport("DISTORTION_REALWORLD_TEST_REPORT.md");

    std::cout << "\n============================================================\n";
    std::cout << "✅ TESTING COMPLETE\n";
    std::cout << "============================================================\n";
    std::cout << "\nCheck:\n";
    std::cout << "  - DISTORTION_REALWORLD_TEST_REPORT.md (main report)\n";
    std::cout << "  - distortion_output_*.wav (audio files)\n";
    std::cout << "  - distortion_spectrum_*.csv (spectral data)\n\n";

    return 0;
}
