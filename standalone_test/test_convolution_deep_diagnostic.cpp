// Deep diagnostic test for ConvolutionReverb (Engine 41)
// Tests every stage of IR generation and convolution processing

#include "../JUCE_Plugin/Source/ConvolutionReverb.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <map>

// Test configuration
const double TEST_SAMPLE_RATE = 48000.0;
const int TEST_BLOCK_SIZE = 512;
const int IMPULSE_RESPONSE_LENGTH = 48000 * 5; // 5 seconds

// Color codes for terminal output
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

struct DiagnosticResults {
    bool irGenerated = false;
    int irLength = 0;
    int irChannels = 0;
    float irPeak = 0.0f;
    float irRMS = 0.0f;
    int irNonZeroSamples = 0;
    int firstNonZeroSample = -1;

    bool convolutionPrepared = false;
    int convolutionLatency = 0;

    bool producesOutput = false;
    float outputPeak = 0.0f;
    float outputRMS = 0.0f;
    int outputNonZeroSamples = 0;

    float reverbTail = 0.0f; // Time to decay to -60dB
    bool hasDecay = false;
};

class ConvolutionDiagnostics {
public:
    static void printHeader(const std::string& title) {
        std::cout << "\n" << CYAN << "========================================" << RESET << std::endl;
        std::cout << CYAN << title << RESET << std::endl;
        std::cout << CYAN << "========================================" << RESET << std::endl;
    }

    static void printResult(const std::string& test, bool passed, const std::string& detail = "") {
        std::cout << (passed ? GREEN : RED)
                  << (passed ? "[PASS] " : "[FAIL] ")
                  << RESET << test;
        if (!detail.empty()) {
            std::cout << ": " << detail;
        }
        std::cout << std::endl;
    }

    static void printMetric(const std::string& name, const std::string& value) {
        std::cout << BLUE << "  " << name << ": " << RESET << value << std::endl;
    }

    static DiagnosticResults runFullDiagnostic(ConvolutionReverb& engine) {
        DiagnosticResults results;

        printHeader("ConvolutionReverb Deep Diagnostic");

        // Stage 1: Initialization
        std::cout << "\n" << YELLOW << "[Stage 1] Initialization" << RESET << std::endl;
        engine.prepareToPlay(TEST_SAMPLE_RATE, TEST_BLOCK_SIZE);
        printResult("Engine initialized", true);

        // Stage 2: Parameter Configuration (100% wet, default IR)
        std::cout << "\n" << YELLOW << "[Stage 2] Parameter Configuration" << RESET << std::endl;
        std::map<int, float> params;
        params[0] = 1.0f;  // Mix = 100% wet
        params[1] = 0.0f;  // IR Select = 0 (Concert Hall)
        params[2] = 1.0f;  // Size = full
        params[3] = 0.0f;  // Pre-delay = 0
        params[4] = 0.0f;  // Damping = 0 (no filtering)
        params[5] = 0.0f;  // Reverse = off
        params[6] = 0.5f;  // Early/Late = balanced
        params[7] = 0.0f;  // Low Cut = minimum
        params[8] = 1.0f;  // High Cut = maximum
        params[9] = 1.0f;  // Width = full stereo

        engine.updateParameters(params);
        printResult("Parameters set to 100% wet, no filtering", true);

        // Stage 3: Test with impulse (unit impulse at sample 0)
        std::cout << "\n" << YELLOW << "[Stage 3] Impulse Response Test" << RESET << std::endl;

        juce::AudioBuffer<float> impulseBuffer(2, IMPULSE_RESPONSE_LENGTH);
        impulseBuffer.clear();
        impulseBuffer.setSample(0, 0, 1.0f);
        impulseBuffer.setSample(1, 0, 1.0f);

        printMetric("Input impulse", "1.0 at sample 0");

        // Process in blocks
        int samplesProcessed = 0;
        while (samplesProcessed < IMPULSE_RESPONSE_LENGTH) {
            int blockSize = std::min(TEST_BLOCK_SIZE, IMPULSE_RESPONSE_LENGTH - samplesProcessed);
            juce::AudioBuffer<float> block(impulseBuffer.getArrayOfWritePointers(),
                                          impulseBuffer.getNumChannels(),
                                          samplesProcessed, blockSize);
            engine.process(block);
            samplesProcessed += blockSize;
        }

        // Analyze output
        results.producesOutput = false;
        results.outputPeak = 0.0f;
        results.outputRMS = 0.0f;
        results.outputNonZeroSamples = 0;

        for (int ch = 0; ch < impulseBuffer.getNumChannels(); ch++) {
            const float* data = impulseBuffer.getReadPointer(ch);
            for (int i = 0; i < impulseBuffer.getNumSamples(); i++) {
                float sample = std::abs(data[i]);
                if (sample > 0.0001f) {
                    results.outputNonZeroSamples++;
                    results.producesOutput = true;
                }
                results.outputPeak = std::max(results.outputPeak, sample);
                results.outputRMS += sample * sample;
            }
        }

        results.outputRMS = std::sqrt(results.outputRMS / (impulseBuffer.getNumSamples() * impulseBuffer.getNumChannels()));

        printMetric("Output Peak", std::to_string(results.outputPeak));
        printMetric("Output RMS", std::to_string(results.outputRMS));
        printMetric("Non-zero samples", std::to_string(results.outputNonZeroSamples) +
                   " (" + std::to_string(100.0f * results.outputNonZeroSamples / impulseBuffer.getNumSamples()) + "%)");

        printResult("Produces output", results.producesOutput,
                   results.outputPeak > 0.01f ? "Good level" : "Very weak");

        // Stage 4: Decay analysis (RT60)
        std::cout << "\n" << YELLOW << "[Stage 4] Reverb Decay Analysis (RT60)" << RESET << std::endl;

        float rt60 = measureRT60(impulseBuffer, TEST_SAMPLE_RATE);
        results.reverbTail = rt60;
        results.hasDecay = rt60 > 0.1f;

        printMetric("RT60", std::to_string(rt60) + " seconds");
        printResult("Has reverb decay", results.hasDecay,
                   results.hasDecay ? "Good reverb tail" : "No decay - acts like delay/gate");

        // Stage 5: Save output for analysis
        std::cout << "\n" << YELLOW << "[Stage 5] Saving Output for Analysis" << RESET << std::endl;

        saveBufferToCSV(impulseBuffer, "convolution_diagnostic_output.csv");
        printResult("CSV saved", true, "convolution_diagnostic_output.csv");

        // Stage 6: Test with sustained tone (1kHz)
        std::cout << "\n" << YELLOW << "[Stage 6] Sustained Tone Test (1kHz)" << RESET << std::endl;

        juce::AudioBuffer<float> toneBuffer(2, TEST_SAMPLE_RATE); // 1 second
        for (int i = 0; i < toneBuffer.getNumSamples(); i++) {
            float sample = std::sin(2.0f * M_PI * 1000.0f * i / TEST_SAMPLE_RATE) * 0.5f;
            toneBuffer.setSample(0, i, sample);
            toneBuffer.setSample(1, i, sample);
        }

        // Process tone
        samplesProcessed = 0;
        while (samplesProcessed < toneBuffer.getNumSamples()) {
            int blockSize = std::min(TEST_BLOCK_SIZE, toneBuffer.getNumSamples() - samplesProcessed);
            juce::AudioBuffer<float> block(toneBuffer.getArrayOfWritePointers(),
                                          toneBuffer.getNumChannels(),
                                          samplesProcessed, blockSize);
            engine.process(block);
            samplesProcessed += blockSize;
        }

        float tonePeak = toneBuffer.getMagnitude(0, toneBuffer.getNumSamples());
        printMetric("1kHz output peak", std::to_string(tonePeak));
        printResult("Processes sustained tone", tonePeak > 0.01f,
                   tonePeak > 0.01f ? "Good" : "Severely attenuated");

        // Stage 7: Parameter sweep (damping test)
        std::cout << "\n" << YELLOW << "[Stage 7] Damping Parameter Test" << RESET << std::endl;

        bool allDampingLevelsWork = true;
        for (float damping = 0.0f; damping <= 1.0f; damping += 0.5f) {
            params[4] = damping;
            engine.updateParameters(params);

            // Reset engine
            engine.reset();

            // Test impulse
            juce::AudioBuffer<float> testBuffer(2, TEST_BLOCK_SIZE);
            testBuffer.clear();
            testBuffer.setSample(0, 0, 1.0f);
            testBuffer.setSample(1, 0, 1.0f);
            engine.process(testBuffer);

            float peak = testBuffer.getMagnitude(0, testBuffer.getNumSamples());
            bool works = peak > 0.0001f;

            printMetric("Damping " + std::to_string(damping),
                       works ? (GREEN + std::string("PASS") + RESET + " (peak=" + std::to_string(peak) + ")")
                             : (RED + std::string("FAIL") + RESET + " (zero output)"));

            if (!works) allDampingLevelsWork = false;
        }

        printResult("All damping levels work", allDampingLevelsWork);

        return results;
    }

    static float measureRT60(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        // Find peak
        float peak = buffer.getMagnitude(0, buffer.getNumSamples());
        if (peak < 0.0001f) return 0.0f;

        // Find time to decay to -60dB (0.001 = 1/1000 of peak)
        float targetLevel = peak * 0.001f;

        // Use envelope follower
        float envelope = 0.0f;
        const float attackTime = 0.001f;
        const float releaseTime = 0.1f;
        float attackCoeff = 1.0f - std::exp(-1.0f / (attackTime * sampleRate));
        float releaseCoeff = 1.0f - std::exp(-1.0f / (releaseTime * sampleRate));

        int decayStartSample = -1;
        int decaySample = -1;

        for (int i = 0; i < buffer.getNumSamples(); i++) {
            float sample = std::abs(buffer.getSample(0, i));

            if (sample > envelope) {
                envelope = envelope + attackCoeff * (sample - envelope);
            } else {
                envelope = envelope + releaseCoeff * (sample - envelope);
            }

            // Find first peak
            if (decayStartSample < 0 && envelope > peak * 0.5f) {
                decayStartSample = i;
            }

            // Find decay to target
            if (decayStartSample >= 0 && envelope < targetLevel) {
                decaySample = i;
                break;
            }
        }

        if (decayStartSample >= 0 && decaySample > decayStartSample) {
            return (decaySample - decayStartSample) / (float)sampleRate;
        }

        return 0.0f;
    }

    static void saveBufferToCSV(const juce::AudioBuffer<float>& buffer, const std::string& filename) {
        std::ofstream file(filename);
        file << "Sample,Left,Right\n";

        int samplesToSave = std::min(buffer.getNumSamples(), 10000); // Save first 10k samples

        for (int i = 0; i < samplesToSave; i++) {
            file << i << ","
                 << buffer.getSample(0, i) << ","
                 << (buffer.getNumChannels() > 1 ? buffer.getSample(1, i) : 0.0f)
                 << "\n";
        }

        file.close();
    }

    static void printSummary(const DiagnosticResults& results) {
        printHeader("Diagnostic Summary");

        int passCount = 0;
        int totalTests = 4;

        std::cout << "\n" << MAGENTA << "Critical Tests:" << RESET << std::endl;

        bool test1 = results.producesOutput && results.outputPeak > 0.01f;
        printResult("1. Produces output", test1);
        if (test1) passCount++;

        bool test2 = results.outputNonZeroSamples > 1000;
        printResult("2. Has sustained output (>1000 samples)", test2);
        if (test2) passCount++;

        bool test3 = results.hasDecay && results.reverbTail > 0.5f;
        printResult("3. Has reverb decay (RT60 > 0.5s)", test3);
        if (test3) passCount++;

        bool test4 = results.outputRMS > 0.001f;
        printResult("4. Has adequate energy (RMS > 0.001)", test4);
        if (test4) passCount++;

        std::cout << "\n" << MAGENTA << "Overall Result: " << RESET;
        if (passCount == totalTests) {
            std::cout << GREEN << "ALL TESTS PASSED (" << passCount << "/" << totalTests << ")" << RESET << std::endl;
        } else if (passCount >= totalTests / 2) {
            std::cout << YELLOW << "PARTIAL PASS (" << passCount << "/" << totalTests << ")" << RESET << std::endl;
        } else {
            std::cout << RED << "FAILED (" << passCount << "/" << totalTests << ")" << RESET << std::endl;
        }

        std::cout << "\n" << MAGENTA << "Diagnosis:" << RESET << std::endl;
        if (!results.producesOutput) {
            std::cout << RED << "  CRITICAL: Engine produces zero or near-zero output" << RESET << std::endl;
            std::cout << "  → Check IR generation pipeline" << std::endl;
            std::cout << "  → Check convolution engine initialization" << std::endl;
        } else if (!test2) {
            std::cout << YELLOW << "  WARNING: Output is very sparse" << RESET << std::endl;
            std::cout << "  → IR may be too short or heavily filtered" << std::endl;
        } else if (!test3) {
            std::cout << YELLOW << "  WARNING: No reverb decay detected" << RESET << std::endl;
            std::cout << "  → May be acting like a gate or very short delay" << std::endl;
            std::cout << "  → Check size/decay parameter mapping" << RESET << std::endl;
        }

        if (passCount == totalTests) {
            std::cout << GREEN << "  ✓ ConvolutionReverb is functioning correctly!" << RESET << std::endl;
        }
    }
};

int main() {
    std::cout << CYAN << "=====================================================" << RESET << std::endl;
    std::cout << CYAN << "  ConvolutionReverb Deep Diagnostic Test Suite" << RESET << std::endl;
    std::cout << CYAN << "  Testing IR generation, convolution, and output" << RESET << std::endl;
    std::cout << CYAN << "=====================================================" << RESET << std::endl;

    ConvolutionReverb engine;

    DiagnosticResults results = ConvolutionDiagnostics::runFullDiagnostic(engine);

    ConvolutionDiagnostics::printSummary(results);

    std::cout << "\n" << BLUE << "Output saved to: convolution_diagnostic_output.csv" << RESET << std::endl;
    std::cout << BLUE << "Analyze first 100 samples with: head -101 convolution_diagnostic_output.csv" << RESET << std::endl;

    return results.producesOutput && results.hasDecay ? 0 : 1;
}
