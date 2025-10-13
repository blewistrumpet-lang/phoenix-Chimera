#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>

// Reverb-specific test suite
namespace ReverbTests {

struct ReverbMetrics {
    float rt60;              // Decay time in seconds
    float earlyDecayTime;    // EDT (first 10dB decay)
    float stereoWidth;       // -1 to +1 correlation
    float dcOffset;          // DC buildup
    float frequencyResponse[10]; // Response at different frequencies
    float diffusion;         // Measured diffusion quality
    float modalDensity;      // Echo density metric
    bool hasMetallicRing;    // Detects metallic artifacts
    bool hasPreDelay;        // Detects pre-delay
    float predelayMs;        // Measured pre-delay
    float mixLinearity;      // How linear is the mix control
};

// Measure RT60: Time for signal to decay 60dB
float measureRT60(juce::AudioBuffer<float>& impulseResponse, float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Find peak
    float peak = 0.0f;
    int peakIdx = 0;
    for (int i = 0; i < numSamples; ++i) {
        float absVal = std::abs(data[i]);
        if (absVal > peak) {
            peak = absVal;
            peakIdx = i;
        }
    }

    if (peak < 1e-6f) return 0.0f; // No signal

    // Find where signal drops to -60dB (1/1000 of peak)
    float threshold60dB = peak * 0.001f;

    for (int i = peakIdx; i < numSamples; ++i) {
        if (std::abs(data[i]) < threshold60dB) {
            return (i - peakIdx) / sampleRate;
        }
    }

    return (numSamples - peakIdx) / sampleRate; // Hit buffer limit
}

// Measure Early Decay Time (EDT): Time for first 10dB drop
float measureEDT(juce::AudioBuffer<float>& impulseResponse, float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Find peak
    float peak = 0.0f;
    int peakIdx = 0;
    for (int i = 0; i < numSamples; ++i) {
        float absVal = std::abs(data[i]);
        if (absVal > peak) {
            peak = absVal;
            peakIdx = i;
        }
    }

    if (peak < 1e-6f) return 0.0f;

    // Find where signal drops to -10dB (0.316 of peak)
    float threshold10dB = peak * 0.316f;

    for (int i = peakIdx; i < numSamples; ++i) {
        if (std::abs(data[i]) < threshold10dB) {
            return (i - peakIdx) / sampleRate;
        }
    }

    return 0.0f;
}

// Measure stereo width via inter-channel correlation
float measureStereoWidth(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return 0.0f;

    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    int numSamples = buffer.getNumSamples();

    float sumLL = 0.0f, sumRR = 0.0f, sumLR = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    float denominator = std::sqrt(sumLL * sumRR);
    if (denominator < 1e-10f) return 0.0f;

    return sumLR / denominator; // Returns -1 to +1
}

// Measure frequency response at specific frequency
float measureFrequencyResponse(EngineBase* engine, float frequency, float sampleRate, int blockSize, const std::map<int, float>& params) {
    // Re-apply parameters without resetting (reset clears parameters!)
    engine->updateParameters(params);

    // Generate sine sweep at this frequency
    juce::AudioBuffer<float> input(2, blockSize * 4);
    juce::AudioBuffer<float> output(2, blockSize * 4);

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < input.getNumSamples(); ++i) {
            float phase = 2.0f * M_PI * frequency * i / sampleRate;
            input.setSample(ch, i, 0.5f * std::sin(phase));
        }
    }

    output.makeCopyOf(input);
    engine->process(output);

    // Measure RMS of output vs input at this frequency
    float inputRMS = 0.0f, outputRMS = 0.0f;
    for (int i = 0; i < output.getNumSamples(); ++i) {
        inputRMS += input.getSample(0, i) * input.getSample(0, i);
        outputRMS += output.getSample(0, i) * output.getSample(0, i);
    }

    inputRMS = std::sqrt(inputRMS / output.getNumSamples());
    outputRMS = std::sqrt(outputRMS / output.getNumSamples());

    if (inputRMS < 1e-10f) return 0.0f;
    return 20.0f * std::log10(outputRMS / inputRMS); // Return in dB
}

// Detect metallic ringing by looking for strong resonances
bool detectMetallicRing(juce::AudioBuffer<float>& impulseResponse) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Look for periodic peaks that indicate modal resonances
    std::vector<int> peakIndices;
    for (int i = 100; i < numSamples - 1; ++i) {
        if (std::abs(data[i]) > std::abs(data[i-1]) &&
            std::abs(data[i]) > std::abs(data[i+1]) &&
            std::abs(data[i]) > 0.1f) {
            peakIndices.push_back(i);
        }
    }

    // If we have very regular peaks, it's metallic
    if (peakIndices.size() > 5) {
        std::vector<int> intervals;
        for (size_t i = 1; i < peakIndices.size(); ++i) {
            intervals.push_back(peakIndices[i] - peakIndices[i-1]);
        }

        // Check if intervals are very similar (within 20%)
        if (intervals.size() > 3) {
            float avgInterval = std::accumulate(intervals.begin(), intervals.end(), 0.0f) / intervals.size();
            int similarCount = 0;
            for (int interval : intervals) {
                if (std::abs(interval - avgInterval) < avgInterval * 0.2f) {
                    similarCount++;
                }
            }
            if (similarCount > intervals.size() * 0.7f) {
                return true; // Metallic!
            }
        }
    }

    return false;
}

// Measure echo density (how many discrete echoes vs smooth decay)
float measureModalDensity(juce::AudioBuffer<float>& impulseResponse, float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Count zero crossings in tail (after 50ms)
    int startIdx = static_cast<int>(0.05f * sampleRate);
    int zeroCrossings = 0;

    for (int i = startIdx + 1; i < numSamples; ++i) {
        if ((data[i-1] < 0 && data[i] >= 0) || (data[i-1] >= 0 && data[i] < 0)) {
            zeroCrossings++;
        }
    }

    float duration = (numSamples - startIdx) / sampleRate;
    return zeroCrossings / duration; // Crossings per second
}

// Measure pre-delay
float measurePreDelay(juce::AudioBuffer<float>& impulseResponse, float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Find first sample above threshold
    float threshold = 0.001f;
    for (int i = 0; i < numSamples; ++i) {
        if (std::abs(data[i]) > threshold) {
            return i / sampleRate * 1000.0f; // Return in milliseconds
        }
    }

    return 0.0f;
}

// Comprehensive reverb test
ReverbMetrics testReverb(int engineId, float sampleRate = 48000.0f) {
    ReverbMetrics metrics = {};

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) return metrics;

    const int blockSize = 512;
    const int impulseLength = static_cast<int>(sampleRate * 10); // 10 seconds

    engine->prepareToPlay(sampleRate, blockSize);

    // Set reverb parameters based on comprehensive parameter analysis
    std::map<int, float> params;
    int numParams = engine->getNumParameters();

    // CRITICAL: Parameter 0 is MIX (dry/wet) for ALL reverb engines
    // Must set to 1.0 (100% wet) to measure actual reverb characteristics
    if (numParams > 0) params[0] = 1.0f;  // Mix = 100% wet (was incorrectly 0.5)
    if (numParams > 1) params[1] = 0.7f;  // Decay/Time/Size
    if (numParams > 2) params[2] = 0.5f;  // Damping/Feedback
    if (numParams > 3) params[3] = 0.7f;  // Additional decay control
    if (numParams > 4) params[4] = 1.0f;  // Width (full stereo)

    engine->updateParameters(params);

    // Generate impulse response
    juce::AudioBuffer<float> impulseResponse(2, impulseLength);
    impulseResponse.clear();

    // Create impulse (single sample spike)
    impulseResponse.setSample(0, 0, 1.0f);
    impulseResponse.setSample(1, 0, 1.0f);

    // Process in blocks
    for (int start = 0; start < impulseLength; start += blockSize) {
        int samplesThisBlock = std::min(blockSize, impulseLength - start);
        juce::AudioBuffer<float> block(impulseResponse.getArrayOfWritePointers(), 2, start, samplesThisBlock);
        engine->process(block);
    }

    // Calculate metrics
    metrics.rt60 = measureRT60(impulseResponse, sampleRate);
    metrics.earlyDecayTime = measureEDT(impulseResponse, sampleRate);
    metrics.stereoWidth = measureStereoWidth(impulseResponse);
    metrics.hasMetallicRing = detectMetallicRing(impulseResponse);
    metrics.modalDensity = measureModalDensity(impulseResponse, sampleRate);
    metrics.predelayMs = measurePreDelay(impulseResponse, sampleRate);
    metrics.hasPreDelay = metrics.predelayMs > 1.0f;

    // Measure frequency response at different frequencies
    float testFreqs[] = {100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 12000.0f, 16000.0f, 20000.0f};
    for (int i = 0; i < 10; ++i) {
        std::cout << "[DEBUG] Measuring freq response at " << testFreqs[i] << " Hz...\n" << std::flush;
        metrics.frequencyResponse[i] = measureFrequencyResponse(engine.get(), testFreqs[i], sampleRate, blockSize, params);
    }

    // Measure DC offset
    float dcSum = 0.0f;
    for (int i = 0; i < impulseLength; ++i) {
        dcSum += impulseResponse.getSample(0, i);
    }
    metrics.dcOffset = std::abs(dcSum / impulseLength);

    return metrics;
}

void printReverbMetrics(int engineId, const std::string& name, const ReverbMetrics& m) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": " << std::setw(45) << std::left << name << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "DECAY CHARACTERISTICS:\n";
    std::cout << "  RT60:            " << std::fixed << std::setprecision(2) << m.rt60 << " seconds\n";
    std::cout << "  Early Decay:     " << std::fixed << std::setprecision(3) << m.earlyDecayTime << " seconds\n";
    std::cout << "  Pre-delay:       " << (m.hasPreDelay ? "YES" : "NO") << " (" << m.predelayMs << " ms)\n";

    std::cout << "\nSPATIAL QUALITY:\n";
    std::cout << "  Stereo Width:    " << std::fixed << std::setprecision(3) << m.stereoWidth;
    if (m.stereoWidth > 0.7f) std::cout << " (too narrow/mono)";
    else if (m.stereoWidth < -0.3f) std::cout << " (inverted/wide)";
    else std::cout << " (good)";
    std::cout << "\n";

    std::cout << "\nARTIFACTS:\n";
    std::cout << "  Metallic Ring:   " << (m.hasMetallicRing ? "⚠️  DETECTED" : "✓ None") << "\n";
    std::cout << "  DC Offset:       " << std::scientific << m.dcOffset;
    if (m.dcOffset > 0.001f) std::cout << " ⚠️  HIGH";
    std::cout << "\n";
    std::cout << "  Echo Density:    " << std::fixed << std::setprecision(1) << m.modalDensity << " crossings/sec\n";

    std::cout << "\nFREQUENCY RESPONSE:\n";
    std::cout << "  100Hz:   " << std::setw(6) << std::fixed << std::setprecision(1) << m.frequencyResponse[0] << " dB\n";
    std::cout << "  500Hz:   " << std::setw(6) << m.frequencyResponse[2] << " dB\n";
    std::cout << "  1kHz:    " << std::setw(6) << m.frequencyResponse[3] << " dB\n";
    std::cout << "  4kHz:    " << std::setw(6) << m.frequencyResponse[5] << " dB\n";
    std::cout << "  16kHz:   " << std::setw(6) << m.frequencyResponse[8] << " dB\n";

    // Calculate frequency response variance
    float avgResponse = 0.0f;
    for (int i = 0; i < 10; ++i) avgResponse += m.frequencyResponse[i];
    avgResponse /= 10.0f;

    float variance = 0.0f;
    for (int i = 0; i < 10; ++i) {
        float diff = m.frequencyResponse[i] - avgResponse;
        variance += diff * diff;
    }
    variance = std::sqrt(variance / 10.0f);

    std::cout << "  Flatness:        " << std::fixed << std::setprecision(2) << variance << " dB variance";
    if (variance > 3.0f) std::cout << " ⚠️  COLORED";
    std::cout << "\n";

    // Overall quality assessment
    std::cout << "\nQUALITY ASSESSMENT:\n";
    bool passRT60 = m.rt60 > 0.1f && m.rt60 < 15.0f;
    bool passStereo = m.stereoWidth < 0.5f && m.stereoWidth > -0.5f;
    bool passArtifacts = !m.hasMetallicRing && m.dcOffset < 0.01f;
    bool passFlatness = variance < 5.0f;

    std::cout << "  Decay Time:      " << (passRT60 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "  Stereo Image:    " << (passStereo ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "  Artifacts:       " << (passArtifacts ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "  Freq Response:   " << (passFlatness ? "✓ PASS" : "✗ FAIL") << "\n";

    bool overall = passRT60 && passStereo && passArtifacts && passFlatness;
    std::cout << "\n  OVERALL:         " << (overall ? "✓ PASSED" : "✗ FAILED") << "\n\n";
}

} // namespace ReverbTests

int main(int argc, char* argv[]) {
    // Reverb engine IDs: 34-43
    std::vector<std::pair<int, std::string>> reverbEngines = {
        {34, "Tape Echo"},
        {35, "Digital Delay"},
        {36, "Magnetic Drum Echo"},
        {37, "Bucket Brigade Delay"},
        {38, "Buffer Repeat Platinum"},
        {39, "Convolution Reverb"},
        {40, "Shimmer Reverb"},
        {41, "Plate Reverb"},
        {42, "Spring Reverb"},
        {43, "Gated Reverb"}
    };

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║     ChimeraPhoenix Reverb Deep Analysis Suite             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    for (const auto& [id, name] : reverbEngines) {
        std::cout << "\n[DEBUG] Testing engine " << id << ": " << name << "...\n" << std::flush;
        auto metrics = ReverbTests::testReverb(id);
        ReverbTests::printReverbMetrics(id, name, metrics);
    }

    return 0;
}
