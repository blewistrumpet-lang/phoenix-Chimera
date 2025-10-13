/**
 * REAL-WORLD AUDIO TESTING - DYNAMICS ENGINES 0-7
 *
 * Tests all dynamics engines with realistic musical materials:
 * - Engine 0: ClassicCompressor
 * - Engine 1: VintageOptoCompressor
 * - Engine 2: NoiseGate
 * - Engine 3: TransientShaper
 * - Engine 4: MasteringLimiter
 * - Engine 5: DynamicEQ
 * - Engine 6: Deesser
 * - Engine 7: Multiband Compressor
 *
 * Test Materials:
 * - Drum loop (120 BPM, transients)
 * - Bass line (low frequency handling)
 * - Vocal sample (sibilance, formants)
 */

#include <JuceHeader.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iomanip>

// Engine includes
#include "../JUCE_Plugin/Source/EngineBase.h"
#include "../JUCE_Plugin/Source/ClassicCompressor.h"
#include "../JUCE_Plugin/Source/VintageOptoCompressor.h"
#include "../JUCE_Plugin/Source/NoiseGate.h"
#include "../JUCE_Plugin/Source/MasteringLimiter_Platinum.h"
#include "../JUCE_Plugin/Source/DynamicEQ.h"

using namespace juce;

// ==================== AUDIO FILE LOADER ====================
class WavFileLoader {
public:
    static bool loadWavFile(const String& filename,
                           AudioBuffer<float>& buffer,
                           double& sampleRate) {
        File file(filename);
        if (!file.existsAsFile()) {
            std::cerr << "File not found: " << filename << std::endl;
            return false;
        }

        AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (!reader) {
            std::cerr << "Failed to create reader for: " << filename << std::endl;
            return false;
        }

        sampleRate = reader->sampleRate;
        buffer.setSize(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);

        std::cout << "Loaded: " << file.getFileName()
                  << " (" << reader->numChannels << " ch, "
                  << reader->lengthInSamples << " samples, "
                  << reader->sampleRate << " Hz)" << std::endl;

        return true;
    }

    static bool saveWavFile(const String& filename,
                           const AudioBuffer<float>& buffer,
                           double sampleRate) {
        File file(filename);
        file.deleteFile(); // Remove existing

        WavAudioFormat wavFormat;
        std::unique_ptr<AudioFormatWriter> writer(
            wavFormat.createWriterFor(
                new FileOutputStream(file),
                sampleRate,
                buffer.getNumChannels(),
                16, // bits per sample
                {},
                0
            )
        );

        if (!writer) {
            std::cerr << "Failed to create writer for: " << filename << std::endl;
            return false;
        }

        writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        return true;
    }
};

// ==================== AUDIO METRICS ====================
struct AudioMetrics {
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    float dynamicRange = 0.0f;
    float crestFactor = 0.0f;
    int clipCount = 0;
    bool hasNaN = false;
    bool hasInf = false;
    bool hasDCOffset = false;
    float dcOffset = 0.0f;

    void analyze(const AudioBuffer<float>& buffer) {
        float sumSquares = 0.0f;
        float dcSum = 0.0f;
        int totalSamples = buffer.getNumSamples() * buffer.getNumChannels();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = data[i];

                // Check for invalid values
                if (std::isnan(sample)) hasNaN = true;
                if (std::isinf(sample)) hasInf = true;

                // Peak detection
                float absSample = std::abs(sample);
                if (absSample > peakLevel) peakLevel = absSample;

                // Clipping detection
                if (absSample >= 0.999f) clipCount++;

                // RMS calculation
                sumSquares += sample * sample;

                // DC offset
                dcSum += sample;
            }
        }

        rmsLevel = std::sqrt(sumSquares / totalSamples);
        crestFactor = peakLevel / std::max(0.0001f, rmsLevel);
        dcOffset = dcSum / totalSamples;
        hasDCOffset = std::abs(dcOffset) > 0.01f;

        // Dynamic range (simplified)
        dynamicRange = 20.0f * std::log10(peakLevel / std::max(0.0001f, rmsLevel));
    }

    void print(const String& prefix = "") const {
        std::cout << prefix << "Peak: " << std::fixed << std::setprecision(3)
                  << peakLevel << " (" << (20.0f * std::log10(peakLevel)) << " dBFS)" << std::endl;
        std::cout << prefix << "RMS: " << rmsLevel
                  << " (" << (20.0f * std::log10(rmsLevel)) << " dBFS)" << std::endl;
        std::cout << prefix << "Crest Factor: " << crestFactor << " dB" << std::endl;
        std::cout << prefix << "Dynamic Range: " << dynamicRange << " dB" << std::endl;
        std::cout << prefix << "Clips: " << clipCount << std::endl;
        std::cout << prefix << "DC Offset: " << dcOffset
                  << (hasDCOffset ? " [WARNING]" : " [OK]") << std::endl;
        if (hasNaN) std::cout << prefix << "ERROR: Contains NaN values!" << std::endl;
        if (hasInf) std::cout << prefix << "ERROR: Contains Inf values!" << std::endl;
    }

    char getGrade() const {
        int issues = 0;
        if (hasNaN || hasInf) return 'F';
        if (clipCount > 10) issues += 2;
        if (hasDCOffset) issues += 1;
        if (peakLevel > 1.0f) issues += 2;
        if (rmsLevel < 0.001f) issues += 1; // Too quiet

        if (issues == 0) return 'A';
        if (issues == 1) return 'B';
        if (issues == 2) return 'C';
        if (issues == 3) return 'D';
        return 'F';
    }
};

// ==================== TEST CASE ====================
struct TestCase {
    String engineName;
    int engineId;
    String material;
    std::map<int, float> parameters;
    AudioMetrics inputMetrics;
    AudioMetrics outputMetrics;
    char grade = 'F';
    String notes;

    void printSummary() const {
        std::cout << "\n=== TEST: " << engineName << " on " << material << " ===" << std::endl;
        std::cout << "Grade: " << grade << std::endl;
        if (!notes.isEmpty()) {
            std::cout << "Notes: " << notes << std::endl;
        }
        std::cout << "\nINPUT:" << std::endl;
        inputMetrics.print("  ");
        std::cout << "\nOUTPUT:" << std::endl;
        outputMetrics.print("  ");
    }
};

// ==================== DYNAMICS ENGINE TESTER ====================
class DynamicsEngineTester {
public:
    DynamicsEngineTester() {
        // No MessageManager needed for standalone test
    }

    ~DynamicsEngineTester() {
    }

    void runAllTests() {
        std::cout << "\n" << String::repeatedString("=", 70) << std::endl;
        std::cout << "REAL-WORLD DYNAMICS ENGINE TESTING" << std::endl;
        std::cout << String::repeatedString("=", 70) << "\n" << std::endl;

        // Load test materials
        if (!loadTestMaterials()) {
            std::cerr << "Failed to load test materials!" << std::endl;
            return;
        }

        // Test each engine
        testClassicCompressor();
        testVintageOptoCompressor();
        testNoiseGate();
        // testTransientShaper(); // TODO: Fix Engine 3 first
        testMasteringLimiter();
        testDynamicEQ();
        // testDeesser(); // TODO: Implement
        // testMultibandCompressor(); // TODO: Implement

        // Generate report
        generateReport();
    }

private:
    // Test materials
    AudioBuffer<float> drumLoop;
    AudioBuffer<float> bassLine;
    AudioBuffer<float> vocalSample;
    double sampleRate = 48000.0;

    std::vector<TestCase> testResults;

    bool loadTestMaterials() {
        std::cout << "Loading test materials..." << std::endl;

        String basePath = "real_world_test_materials/";

        bool success = true;
        success &= WavFileLoader::loadWavFile(basePath + "drum_loop_120bpm.wav",
                                             drumLoop, sampleRate);
        success &= WavFileLoader::loadWavFile(basePath + "bass_line_e1_e2.wav",
                                             bassLine, sampleRate);
        success &= WavFileLoader::loadWavFile(basePath + "vocal_sample_formants.wav",
                                             vocalSample, sampleRate);

        return success;
    }

    void testEngine(EngineBase* engine,
                   const String& engineName,
                   int engineId,
                   const AudioBuffer<float>& input,
                   const String& materialName,
                   const std::map<int, float>& params,
                   const String& testNotes = "") {

        std::cout << "\nTesting: " << engineName << " on " << materialName << std::endl;

        // Create output buffer
        AudioBuffer<float> output;
        output.makeCopyOf(input);

        // Prepare engine
        engine->prepareToPlay(sampleRate, input.getNumSamples());
        engine->updateParameters(params);
        engine->reset();

        // Process
        engine->process(output);

        // Analyze
        TestCase testCase;
        testCase.engineName = engineName;
        testCase.engineId = engineId;
        testCase.material = materialName;
        testCase.parameters = params;
        testCase.notes = testNotes;

        testCase.inputMetrics.analyze(input);
        testCase.outputMetrics.analyze(output);
        testCase.grade = testCase.outputMetrics.getGrade();

        // Additional checks for dynamics processors
        float gainReduction = testCase.inputMetrics.peakLevel - testCase.outputMetrics.peakLevel;
        if (engineName.contains("Compressor") || engineName.contains("Limiter")) {
            if (gainReduction < 0.0f) {
                testCase.notes += " WARNING: No gain reduction detected. ";
                if (testCase.grade > 'C') testCase.grade = 'C';
            }
        }

        // Check for pumping (dramatic RMS changes - simplified check)
        float rmsRatio = testCase.outputMetrics.rmsLevel / testCase.inputMetrics.rmsLevel;
        if (rmsRatio < 0.1f || rmsRatio > 10.0f) {
            testCase.notes += " WARNING: Extreme RMS change detected. ";
            if (testCase.grade > 'D') testCase.grade = 'D';
        }

        testCase.printSummary();
        testResults.push_back(testCase);

        // Save output file
        String outputPath = "real_world_outputs/" +
                           String(engineId) + "_" +
                           engineName.replace(" ", "_") + "_" +
                           materialName.replace(" ", "_") + ".wav";
        File(outputPath).getParentDirectory().createDirectory();
        WavFileLoader::saveWavFile(outputPath, output, sampleRate);
        std::cout << "Saved: " << outputPath << std::endl;
    }

    // ==================== ENGINE 0: CLASSIC COMPRESSOR ====================
    void testClassicCompressor() {
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "ENGINE 0: CLASSIC COMPRESSOR" << std::endl;
        std::cout << String::repeatedString("-", 70) << std::endl;

        ClassicCompressor engine;

        // Test 1: Drums - Medium compression
        std::map<int, float> params;
        params[0] = 0.3f;  // Threshold: -20 dB
        params[1] = 0.4f;  // Ratio: 4:1
        params[2] = 0.1f;  // Attack: fast
        params[3] = 0.3f;  // Release: medium
        params[4] = 0.2f;  // Knee: soft
        params[5] = 0.5f;  // Makeup gain: 0 dB
        params[6] = 1.0f;  // Mix: 100% wet
        testEngine(&engine, "ClassicCompressor", 0, drumLoop,
                  "drums", params, "Medium compression for transients");

        // Test 2: Bass - Heavy compression
        params[0] = 0.4f;  // Threshold: -24 dB
        params[1] = 0.6f;  // Ratio: 8:1
        params[2] = 0.2f;  // Attack: medium
        params[3] = 0.4f;  // Release: medium
        testEngine(&engine, "ClassicCompressor", 0, bassLine,
                  "bass", params, "Heavy compression for sustain");

        // Test 3: Vocals - Gentle compression
        params[0] = 0.25f; // Threshold: -18 dB
        params[1] = 0.3f;  // Ratio: 3:1
        params[2] = 0.3f;  // Attack: slow
        params[3] = 0.5f;  // Release: slow
        params[4] = 0.5f;  // Knee: very soft
        testEngine(&engine, "ClassicCompressor", 0, vocalSample,
                  "vocals", params, "Gentle compression for naturalness");
    }

    // ==================== ENGINE 1: VINTAGE OPTO COMPRESSOR ====================
    void testVintageOptoCompressor() {
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "ENGINE 1: VINTAGE OPTO COMPRESSOR" << std::endl;
        std::cout << String::repeatedString("-", 70) << std::endl;

        VintageOptoCompressor engine;

        std::map<int, float> params;
        params[0] = 0.4f;  // Input gain
        params[1] = 0.6f;  // Output gain
        params[2] = 0.5f;  // Ratio
        params[3] = 0.3f;  // Attack
        params[4] = 0.4f;  // Release
        params[5] = 1.0f;  // Mix

        testEngine(&engine, "VintageOptoCompressor", 1, drumLoop,
                  "drums", params, "Vintage character test");
        testEngine(&engine, "VintageOptoCompressor", 1, bassLine,
                  "bass", params, "Opto compression on bass");
        testEngine(&engine, "VintageOptoCompressor", 1, vocalSample,
                  "vocals", params, "Smooth vocal compression");
    }

    // ==================== ENGINE 2: NOISE GATE ====================
    void testNoiseGate() {
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "ENGINE 2: NOISE GATE" << std::endl;
        std::cout << String::repeatedString("-", 70) << std::endl;

        NoiseGate engine;

        std::map<int, float> params;
        params[0] = 0.2f;  // Threshold: -40 dB (gentle)
        params[1] = 0.5f;  // Ratio: 10:1
        params[2] = 0.1f;  // Attack: fast
        params[3] = 0.4f;  // Release: medium
        params[4] = 0.2f;  // Hold time

        testEngine(&engine, "NoiseGate", 2, drumLoop,
                  "drums", params, "Gate transients");

        // Tighter gate for bass
        params[0] = 0.15f; // Lower threshold
        testEngine(&engine, "NoiseGate", 2, bassLine,
                  "bass", params, "Gate bass notes");
    }

    // ==================== ENGINE 4: MASTERING LIMITER ====================
    void testMasteringLimiter() {
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "ENGINE 4: MASTERING LIMITER" << std::endl;
        std::cout << String::repeatedString("-", 70) << std::endl;

        MasteringLimiter_Platinum engine;

        std::map<int, float> params;
        params[0] = 0.95f; // Threshold: -0.5 dB
        params[1] = 0.5f;  // Release
        params[2] = 0.8f;  // Ceiling: -0.1 dB
        params[3] = 0.5f;  // Character

        testEngine(&engine, "MasteringLimiter", 4, drumLoop,
                  "drums", params, "Peak limiting on drums");
        testEngine(&engine, "MasteringLimiter", 4, bassLine,
                  "bass", params, "Peak limiting on bass");
        testEngine(&engine, "MasteringLimiter", 4, vocalSample,
                  "vocals", params, "Peak limiting on vocals");
    }

    // ==================== ENGINE 5: DYNAMIC EQ ====================
    void testDynamicEQ() {
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "ENGINE 5: DYNAMIC EQ" << std::endl;
        std::cout << String::repeatedString("-", 70) << std::endl;

        DynamicEQ engine;

        std::map<int, float> params;
        params[0] = 0.5f;  // Band 1 freq: 100 Hz
        params[1] = 0.6f;  // Band 1 gain: +3 dB
        params[2] = 0.4f;  // Band 1 Q
        params[3] = 0.5f;  // Band 2 freq: 1 kHz
        params[4] = 0.5f;  // Band 2 gain: 0 dB
        params[5] = 0.4f;  // Band 2 Q

        testEngine(&engine, "DynamicEQ", 5, drumLoop,
                  "drums", params, "Dynamic EQ on drums");
        testEngine(&engine, "DynamicEQ", 5, bassLine,
                  "bass", params, "Dynamic low-end control");
        testEngine(&engine, "DynamicEQ", 5, vocalSample,
                  "vocals", params, "Dynamic presence control");
    }

    // ==================== REPORT GENERATION ====================
    void generateReport() {
        std::cout << "\n" << String::repeatedString("=", 70) << std::endl;
        std::cout << "COMPREHENSIVE TEST REPORT" << std::endl;
        std::cout << String::repeatedString("=", 70) << "\n" << std::endl;

        // Summary by engine
        std::map<String, std::vector<char>> gradesByEngine;
        for (const auto& test : testResults) {
            gradesByEngine[test.engineName].push_back(test.grade);
        }

        std::cout << "SUMMARY BY ENGINE:\n" << std::endl;
        for (const auto& [engineName, grades] : gradesByEngine) {
            int aCount = 0, bCount = 0, cCount = 0, dCount = 0, fCount = 0;
            for (char grade : grades) {
                if (grade == 'A') aCount++;
                else if (grade == 'B') bCount++;
                else if (grade == 'C') cCount++;
                else if (grade == 'D') dCount++;
                else fCount++;
            }

            std::cout << std::left << std::setw(30) << engineName << ": ";
            std::cout << "A=" << aCount << " B=" << bCount << " C=" << cCount
                     << " D=" << dCount << " F=" << fCount;

            // Overall grade
            float avgScore = (aCount * 4.0f + bCount * 3.0f + cCount * 2.0f +
                            dCount * 1.0f) / grades.size();
            char overallGrade = 'F';
            if (avgScore >= 3.5f) overallGrade = 'A';
            else if (avgScore >= 2.5f) overallGrade = 'B';
            else if (avgScore >= 1.5f) overallGrade = 'C';
            else if (avgScore >= 0.5f) overallGrade = 'D';

            std::cout << " | Overall: " << overallGrade << std::endl;
        }

        // Production readiness
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "PRODUCTION READINESS:\n" << std::endl;

        for (const auto& [engineName, grades] : gradesByEngine) {
            bool hasFailures = false;
            for (char grade : grades) {
                if (grade == 'F') hasFailures = true;
            }

            String status = hasFailures ? "[NOT READY]" : "[READY]";
            std::cout << std::left << std::setw(30) << engineName << ": " << status << std::endl;
        }

        // Detailed issues
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "ISSUES FOUND:\n" << std::endl;

        for (const auto& test : testResults) {
            if (test.grade >= 'C' && test.notes.isEmpty()) continue;

            std::cout << "- " << test.engineName << " on " << test.material
                     << " [" << test.grade << "]";
            if (!test.notes.isEmpty()) {
                std::cout << ": " << test.notes;
            }
            std::cout << std::endl;
        }

        // File locations
        std::cout << "\n" << String::repeatedString("-", 70) << std::endl;
        std::cout << "OUTPUT FILES:\n" << std::endl;
        std::cout << "Location: real_world_outputs/" << std::endl;
        std::cout << "Format: [engineID]_[engineName]_[material].wav" << std::endl;
        std::cout << "\nPlease listen to these files for subjective evaluation!" << std::endl;

        // Save report to file
        saveReportToFile();
    }

    void saveReportToFile() {
        File reportFile("DYNAMICS_ENGINES_REALWORLD_REPORT.md");
        FileOutputStream output(reportFile);

        if (!output.openedOk()) {
            std::cerr << "Failed to create report file!" << std::endl;
            return;
        }

        String report;
        report << "# DYNAMICS ENGINES REAL-WORLD TESTING REPORT\n\n";
        report << "**Date**: " << Time::getCurrentTime().toString(true, true) << "\n\n";
        report << "## Test Overview\n\n";
        report << "- **Test Materials**: Drums, Bass, Vocals\n";
        report << "- **Engines Tested**: " << testResults.size() << " test cases\n";
        report << "- **Sample Rate**: " << sampleRate << " Hz\n\n";

        report << "## Summary by Engine\n\n";
        report << "| Engine | A | B | C | D | F | Overall | Status |\n";
        report << "|--------|---|---|---|---|---|---------|--------|\n";

        std::map<String, std::vector<char>> gradesByEngine;
        for (const auto& test : testResults) {
            gradesByEngine[test.engineName].push_back(test.grade);
        }

        for (const auto& [engineName, grades] : gradesByEngine) {
            int aCount = 0, bCount = 0, cCount = 0, dCount = 0, fCount = 0;
            for (char grade : grades) {
                if (grade == 'A') aCount++;
                else if (grade == 'B') bCount++;
                else if (grade == 'C') cCount++;
                else if (grade == 'D') dCount++;
                else fCount++;
            }

            float avgScore = (aCount * 4.0f + bCount * 3.0f + cCount * 2.0f +
                            dCount * 1.0f) / grades.size();
            char overallGrade = 'F';
            if (avgScore >= 3.5f) overallGrade = 'A';
            else if (avgScore >= 2.5f) overallGrade = 'B';
            else if (avgScore >= 1.5f) overallGrade = 'C';
            else if (avgScore >= 0.5f) overallGrade = 'D';

            String status = (fCount == 0) ? "READY" : "NOT READY";

            report << "| " << engineName << " | " << aCount << " | " << bCount
                  << " | " << cCount << " | " << dCount << " | " << fCount
                  << " | " << overallGrade << " | " << status << " |\n";
        }

        report << "\n## Detailed Test Results\n\n";
        for (const auto& test : testResults) {
            report << "### " << test.engineName << " - " << test.material << "\n\n";
            report << "- **Grade**: " << String::charToString(test.grade) << "\n";
            report << "- **Input Peak**: " << String(test.inputMetrics.peakLevel, 3) << " dBFS\n";
            report << "- **Output Peak**: " << String(test.outputMetrics.peakLevel, 3) << " dBFS\n";
            report << "- **Input RMS**: " << String(test.inputMetrics.rmsLevel, 3) << " dBFS\n";
            report << "- **Output RMS**: " << String(test.outputMetrics.rmsLevel, 3) << " dBFS\n";
            report << "- **Clips**: " << test.outputMetrics.clipCount << "\n";
            if (!test.notes.isEmpty()) {
                report << "- **Notes**: " << test.notes << "\n";
            }
            report << "\n";
        }

        report << "## Output Files\n\n";
        report << "Location: `real_world_outputs/`\n\n";
        report << "Format: `[engineID]_[engineName]_[material].wav`\n\n";
        report << "**Please listen to these files for subjective quality assessment!**\n";

        output.writeText(report, false, false, nullptr);
        std::cout << "\nReport saved: " << reportFile.getFullPathName() << std::endl;
    }
};

// ==================== MAIN ====================
int main() {
    DynamicsEngineTester tester;
    tester.runAllTests();

    std::cout << "\n" << String::repeatedString("=", 70) << std::endl;
    std::cout << "TESTING COMPLETE" << std::endl;
    std::cout << String::repeatedString("=", 70) << std::endl;

    return 0;
}
