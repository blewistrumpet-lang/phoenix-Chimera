#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>

// Focused test for SpringReverb (42) and GatedReverb (43)
namespace SpringGatedReverbTests {

struct ReverbMetrics {
    float rt60;              // Decay time in seconds
    float earlyDecayTime;    // EDT (first 10dB decay)
    float lateDecayTime;     // LDT (40-60dB decay)
    float stereoWidth;       // -1 to +1 correlation
    float stereoWidthEarly;  // Stereo width in first 100ms
    float stereoWidthLate;   // Stereo width after 100ms
    float dcOffset;          // DC buildup
    float peakAmplitude;     // Peak response amplitude
    float tailAmplitude;     // Amplitude at end of buffer
    float diffusion;         // Measured diffusion quality
    float modalDensity;      // Echo density metric
    bool hasMetallicRing;    // Detects metallic artifacts
    bool hasPreDelay;        // Detects pre-delay
    float predelayMs;        // Measured pre-delay
    float frequencyResponse[10]; // Response at different frequencies
    float highFreqDecay;     // High frequency decay rate
    float lowFreqDecay;      // Low frequency decay rate
    bool hasGating;          // Detects gating behavior
    float gateThreshold;     // Gate threshold if detected
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

// Measure Late Decay Time (LDT): Time from -40dB to -60dB
float measureLDT(juce::AudioBuffer<float>& impulseResponse, float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

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

    float threshold40dB = peak * 0.01f;   // -40dB
    float threshold60dB = peak * 0.001f;  // -60dB

    int idx40 = -1, idx60 = -1;

    for (int i = peakIdx; i < numSamples; ++i) {
        float absVal = std::abs(data[i]);
        if (idx40 == -1 && absVal < threshold40dB) {
            idx40 = i;
        }
        if (idx60 == -1 && absVal < threshold60dB) {
            idx60 = i;
            break;
        }
    }

    if (idx40 != -1 && idx60 != -1) {
        return (idx60 - idx40) / sampleRate;
    }

    return 0.0f;
}

// Measure stereo width via inter-channel correlation
float measureStereoWidth(juce::AudioBuffer<float>& buffer, int startSample = 0, int numSamples = -1) {
    if (buffer.getNumChannels() < 2) return 0.0f;

    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);

    if (numSamples == -1) numSamples = buffer.getNumSamples() - startSample;

    float sumLL = 0.0f, sumRR = 0.0f, sumLR = 0.0f;

    for (int i = startSample; i < startSample + numSamples; ++i) {
        sumLL += left[i] * left[i];
        sumRR += right[i] * right[i];
        sumLR += left[i] * right[i];
    }

    float denominator = std::sqrt(sumLL * sumRR);
    if (denominator < 1e-10f) return 0.0f;

    return sumLR / denominator; // Returns -1 to +1
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
            std::abs(data[i]) > 0.05f) {
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

// Detect gating behavior
bool detectGating(juce::AudioBuffer<float>& impulseResponse, float& gateThreshold) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Find peak
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float absVal = std::abs(data[i]);
        if (absVal > peak) peak = absVal;
    }

    if (peak < 1e-6f) return false;

    // Look for sudden drops in amplitude (gating)
    float prevEnvelope = peak;
    const int envelopeWindow = 512;

    for (int i = envelopeWindow; i < numSamples; i += envelopeWindow) {
        // Calculate local RMS
        float rms = 0.0f;
        for (int j = 0; j < envelopeWindow && i + j < numSamples; ++j) {
            rms += data[i + j] * data[i + j];
        }
        rms = std::sqrt(rms / envelopeWindow);

        // Check for sudden drop (more than 20dB in one window)
        float dropDB = 20.0f * std::log10((prevEnvelope + 1e-10f) / (rms + 1e-10f));
        if (dropDB > 20.0f && rms < prevEnvelope * 0.1f) {
            gateThreshold = 20.0f * std::log10(rms / peak);
            return true; // Gating detected
        }

        prevEnvelope = rms;
    }

    gateThreshold = 0.0f;
    return false;
}

// Measure frequency response at specific frequency
float measureFrequencyResponse(EngineBase* engine, float frequency, float sampleRate, int blockSize, const std::map<int, float>& params) {
    // Re-apply parameters
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

// Comprehensive reverb test
ReverbMetrics testReverb(int engineId, float sampleRate = 48000.0f) {
    ReverbMetrics metrics = {};

    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        std::cout << "  ERROR: Failed to create engine " << engineId << "\n";
        return metrics;
    }

    const int blockSize = 512;
    const int impulseLength = static_cast<int>(sampleRate * 10); // 10 seconds

    engine->prepareToPlay(sampleRate, blockSize);

    // Set reverb parameters - 100% wet mix to measure pure reverb
    std::map<int, float> params;
    int numParams = engine->getNumParameters();

    std::cout << "  Engine has " << numParams << " parameters\n";

    // Parameter 0 is MIX (dry/wet) - set to 1.0 for 100% wet
    if (numParams > 0) params[0] = 1.0f;  // Mix = 100% wet
    if (numParams > 1) params[1] = 0.7f;  // Decay/Time/Size
    if (numParams > 2) params[2] = 0.5f;  // Damping/Feedback
    if (numParams > 3) params[3] = 0.7f;  // Additional control
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

    // Calculate basic metrics
    metrics.rt60 = measureRT60(impulseResponse, sampleRate);
    metrics.earlyDecayTime = measureEDT(impulseResponse, sampleRate);
    metrics.lateDecayTime = measureLDT(impulseResponse, sampleRate);
    metrics.stereoWidth = measureStereoWidth(impulseResponse);

    // Stereo width in different time windows
    int early100ms = static_cast<int>(0.1f * sampleRate);
    metrics.stereoWidthEarly = measureStereoWidth(impulseResponse, 0, std::min(early100ms, impulseResponse.getNumSamples()));
    if (impulseResponse.getNumSamples() > early100ms) {
        metrics.stereoWidthLate = measureStereoWidth(impulseResponse, early100ms, impulseResponse.getNumSamples() - early100ms);
    } else {
        metrics.stereoWidthLate = metrics.stereoWidth;
    }

    // Artifact detection
    metrics.hasMetallicRing = detectMetallicRing(impulseResponse);
    metrics.modalDensity = measureModalDensity(impulseResponse, sampleRate);
    metrics.predelayMs = measurePreDelay(impulseResponse, sampleRate);
    metrics.hasPreDelay = metrics.predelayMs > 1.0f;
    metrics.hasGating = detectGating(impulseResponse, metrics.gateThreshold);

    // Peak and tail amplitude
    const float* data = impulseResponse.getReadPointer(0);
    metrics.peakAmplitude = 0.0f;
    for (int i = 0; i < impulseResponse.getNumSamples(); ++i) {
        float absVal = std::abs(data[i]);
        if (absVal > metrics.peakAmplitude) {
            metrics.peakAmplitude = absVal;
        }
    }

    // Tail amplitude (last 10% of buffer)
    int tailStart = static_cast<int>(impulseResponse.getNumSamples() * 0.9f);
    float tailSum = 0.0f;
    for (int i = tailStart; i < impulseResponse.getNumSamples(); ++i) {
        tailSum += data[i] * data[i];
    }
    metrics.tailAmplitude = std::sqrt(tailSum / (impulseResponse.getNumSamples() - tailStart));

    // Measure DC offset
    float dcSum = 0.0f;
    for (int i = 0; i < impulseResponse.getNumSamples(); ++i) {
        dcSum += impulseResponse.getSample(0, i);
    }
    metrics.dcOffset = std::abs(dcSum / impulseResponse.getNumSamples());

    // Measure frequency response at different frequencies
    std::cout << "  Measuring frequency response...\n";
    float testFreqs[] = {100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 12000.0f, 16000.0f, 20000.0f};
    for (int i = 0; i < 10; ++i) {
        metrics.frequencyResponse[i] = measureFrequencyResponse(engine.get(), testFreqs[i], sampleRate, blockSize, params);
    }

    // Calculate high/low frequency decay difference
    metrics.highFreqDecay = (metrics.frequencyResponse[7] + metrics.frequencyResponse[8] + metrics.frequencyResponse[9]) / 3.0f;
    metrics.lowFreqDecay = (metrics.frequencyResponse[0] + metrics.frequencyResponse[1] + metrics.frequencyResponse[2]) / 3.0f;

    return metrics;
}

void printReverbMetrics(int engineId, const std::string& name, const ReverbMetrics& m) {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": " << std::setw(47) << std::left << name << "║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "IMPULSE RESPONSE ANALYSIS:\n";
    std::cout << "  Peak Amplitude:  " << std::fixed << std::setprecision(3) << m.peakAmplitude << "\n";
    std::cout << "  Tail Amplitude:  " << std::scientific << std::setprecision(2) << m.tailAmplitude << "\n";
    std::cout << "  Pre-delay:       " << std::fixed << std::setprecision(2) << m.predelayMs << " ms";
    std::cout << (m.hasPreDelay ? " (detected)" : " (none)") << "\n";

    std::cout << "\nDECAY CHARACTERISTICS:\n";
    std::cout << "  RT60:            " << std::fixed << std::setprecision(3) << m.rt60 << " seconds";
    if (m.rt60 < 0.1f) std::cout << " (too short)";
    else if (m.rt60 > 8.0f) std::cout << " (very long)";
    else std::cout << " (normal)";
    std::cout << "\n";

    std::cout << "  Early Decay:     " << std::fixed << std::setprecision(3) << m.earlyDecayTime << " seconds\n";
    std::cout << "  Late Decay:      " << std::fixed << std::setprecision(3) << m.lateDecayTime << " seconds\n";

    if (m.earlyDecayTime > 0) {
        float decayRatio = m.lateDecayTime / m.earlyDecayTime;
        std::cout << "  Decay Linearity: " << std::fixed << std::setprecision(2) << decayRatio;
        if (decayRatio < 0.5f) std::cout << " (fast late decay)";
        else if (decayRatio > 2.0f) std::cout << " (slow late decay)";
        else std::cout << " (linear)";
        std::cout << "\n";
    }

    std::cout << "\nSTEREO WIDTH:\n";
    std::cout << "  Overall:         " << std::fixed << std::setprecision(3) << m.stereoWidth;
    if (m.stereoWidth > 0.7f) std::cout << " (mono/narrow)";
    else if (m.stereoWidth < -0.3f) std::cout << " (inverted/very wide)";
    else if (m.stereoWidth < 0.3f) std::cout << " (good width)";
    std::cout << "\n";

    std::cout << "  Early (0-100ms): " << std::fixed << std::setprecision(3) << m.stereoWidthEarly << "\n";
    std::cout << "  Late (>100ms):   " << std::fixed << std::setprecision(3) << m.stereoWidthLate << "\n";

    std::cout << "\nARTIFACT DETECTION:\n";
    std::cout << "  Metallic Ring:   " << (m.hasMetallicRing ? "DETECTED" : "None") << "\n";
    std::cout << "  DC Offset:       " << std::scientific << std::setprecision(2) << m.dcOffset;
    if (m.dcOffset > 0.001f) std::cout << " (HIGH)";
    else std::cout << " (OK)";
    std::cout << "\n";

    std::cout << "  Echo Density:    " << std::fixed << std::setprecision(1) << m.modalDensity << " crossings/sec\n";
    std::cout << "  Gating:          " << (m.hasGating ? "DETECTED" : "None");
    if (m.hasGating) {
        std::cout << " (threshold: " << std::fixed << std::setprecision(1) << m.gateThreshold << " dB)";
    }
    std::cout << "\n";

    std::cout << "\nFREQUENCY RESPONSE:\n";
    std::cout << "  100Hz:   " << std::setw(7) << std::fixed << std::setprecision(2) << m.frequencyResponse[0] << " dB\n";
    std::cout << "  500Hz:   " << std::setw(7) << m.frequencyResponse[2] << " dB\n";
    std::cout << "  1kHz:    " << std::setw(7) << m.frequencyResponse[3] << " dB\n";
    std::cout << "  4kHz:    " << std::setw(7) << m.frequencyResponse[5] << " dB\n";
    std::cout << "  8kHz:    " << std::setw(7) << m.frequencyResponse[6] << " dB\n";
    std::cout << "  16kHz:   " << std::setw(7) << m.frequencyResponse[8] << " dB\n";

    std::cout << "  Low Freq Avg:    " << std::setw(7) << std::fixed << std::setprecision(2) << m.lowFreqDecay << " dB\n";
    std::cout << "  High Freq Avg:   " << std::setw(7) << m.highFreqDecay << " dB\n";
    float hfDamping = m.lowFreqDecay - m.highFreqDecay;
    std::cout << "  HF Damping:      " << std::setw(7) << hfDamping << " dB";
    if (hfDamping > 10.0f) std::cout << " (heavy damping)";
    else if (hfDamping > 3.0f) std::cout << " (moderate damping)";
    else std::cout << " (minimal damping)";
    std::cout << "\n";

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
    if (variance > 3.0f) std::cout << " (colored)";
    else std::cout << " (flat)";
    std::cout << "\n";

    // PASS/FAIL criteria
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST RESULTS                                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    bool passRT60 = m.rt60 > 0.05f && m.rt60 < 15.0f;
    bool passStereo = m.stereoWidth < 0.8f && m.stereoWidth > -0.8f;
    bool passArtifacts = !m.hasMetallicRing && m.dcOffset < 0.01f;
    bool passFlatness = variance < 8.0f;
    bool passPeak = m.peakAmplitude > 0.01f && m.peakAmplitude < 10.0f;

    std::cout << "  RT60 Valid:      " << (passRT60 ? "PASS" : "FAIL");
    std::cout << " (0.05s - 15s)\n";

    std::cout << "  Stereo Image:    " << (passStereo ? "PASS" : "FAIL");
    std::cout << " (-0.8 to 0.8)\n";

    std::cout << "  Artifacts:       " << (passArtifacts ? "PASS" : "FAIL");
    std::cout << " (no metallic ring, low DC)\n";

    std::cout << "  Freq Response:   " << (passFlatness ? "PASS" : "FAIL");
    std::cout << " (variance < 8 dB)\n";

    std::cout << "  Peak Amplitude:  " << (passPeak ? "PASS" : "FAIL");
    std::cout << " (0.01 - 10.0)\n";

    bool overall = passRT60 && passStereo && passArtifacts && passFlatness && passPeak;

    std::cout << "\n  ╔════════════════════════════════════════╗\n";
    std::cout << "  ║  OVERALL: " << std::setw(28) << std::left << (overall ? "PASSED" : "FAILED") << "║\n";
    std::cout << "  ╚════════════════════════════════════════╝\n\n";
}

} // namespace SpringGatedReverbTests

int main(int argc, char* argv[]) {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ChimeraPhoenix SpringReverb & GatedReverb Test Suite        ║\n";
    std::cout << "║  Engines 42-43: Impulse Response & Reverb Metrics Analysis   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    std::vector<std::pair<int, std::string>> engines = {
        {42, "Spring Reverb"},
        {43, "Gated Reverb"}
    };

    for (const auto& [id, name] : engines) {
        std::cout << "Testing Engine " << id << ": " << name << "...\n";
        auto metrics = SpringGatedReverbTests::testReverb(id);
        SpringGatedReverbTests::printReverbMetrics(id, name, metrics);
    }

    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST SUITE COMPLETE                                         ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}
