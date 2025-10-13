// Standalone test without full JUCE GUI dependencies
#define JUCE_STANDALONE_APPLICATION 1
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1

#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <string>

// Minimal JUCE forward declarations
namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

// Include only what we need
#include "../JUCE_Plugin/Source/TransientShaper_Platinum.h"

// Minimal AudioBuffer implementation for testing
namespace juce {
    class String {
    public:
        String() = default;
        String(const char* s) : text(s) {}
        String(const std::string& s) : text(s) {}
        const char* toRawUTF8() const { return text.c_str(); }
        operator const char*() const { return text.c_str(); }
    private:
        std::string text;
    };

    template<typename FloatType>
    class AudioBuffer {
    public:
        AudioBuffer(int numChannels, int numSamples)
            : channels(numChannels), samples(numSamples) {
            data.resize(numChannels * numSamples, 0.0f);
        }

        int getNumChannels() const { return channels; }
        int getNumSamples() const { return samples; }

        FloatType* getWritePointer(int channel) {
            return &data[channel * samples];
        }

        FloatType getSample(int channel, int sample) const {
            return data[channel * samples + sample];
        }

        void setSample(int channel, int sample, FloatType value) {
            data[channel * samples + sample] = value;
        }

        void clear() {
            std::fill(data.begin(), data.end(), 0.0f);
        }

    private:
        int channels;
        int samples;
        std::vector<FloatType> data;
    };
}

// Test signal generators
std::vector<float> generateDrumHit(int sampleRate, float durationSec) {
    int numSamples = static_cast<int>(sampleRate * durationSec);
    std::vector<float> signal(numSamples);

    // Drum hit: sharp attack + exponential decay
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;

        // Attack envelope (first 5ms)
        float attack = std::min(1.0f, t / 0.005f);

        // Exponential decay
        float decay = std::exp(-t * 8.0f);

        // Mix sine wave with noise for realistic drum
        float sine = std::sin(2.0f * M_PI * 150.0f * t); // 150Hz fundamental
        float noise = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.3f;

        signal[i] = (sine * 0.7f + noise * 0.3f) * attack * decay * 0.5f;
    }

    return signal;
}

std::vector<float> generateTransientRich(int sampleRate, float durationSec) {
    int numSamples = static_cast<int>(sampleRate * durationSec);
    std::vector<float> signal(numSamples);

    // Series of sharp transients with sustain
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;

        // Multiple transient hits
        float transients = 0.0f;
        for (int hit = 0; hit < 5; ++hit) {
            float hitTime = hit * 0.1f; // Every 100ms
            float dt = t - hitTime;
            if (dt > 0 && dt < 0.05f) {
                transients += std::exp(-dt * 30.0f) * std::sin(2.0f * M_PI * 1000.0f * dt);
            }
        }

        // Sustained tone
        float sustain = std::sin(2.0f * M_PI * 200.0f * t) * 0.3f;

        signal[i] = transients * 0.6f + sustain;
    }

    return signal;
}

// Utility to measure peak level in dB
float measurePeakDB(const std::vector<float>& signal) {
    float peak = 0.0f;
    for (float sample : signal) {
        peak = std::max(peak, std::abs(sample));
    }

    if (peak < 1e-10f) return -100.0f; // Silence
    return 20.0f * std::log10(peak);
}

// Test the transient shaper at various sustain levels
void testSustainLevels() {
    const int sampleRate = 48000;
    const int blockSize = 512;

    std::cout << "\n=== TRANSIENT SHAPER SUSTAIN PARAMETER TEST ===\n";
    std::cout << "Testing sustain from 0% to 100% in 10% steps\n";
    std::cout << "Target: All outputs should stay below +20dB\n\n";

    // Test at different sustain levels
    std::vector<float> sustainLevels = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};

    std::cout << "Sustain% | Input Peak | Output Peak | Gain (dB) | Status\n";
    std::cout << "---------|------------|-------------|-----------|--------\n";

    for (float sustainParam : sustainLevels) {
        // Create engine
        TransientShaper_Platinum shaper;
        shaper.prepareToPlay(sampleRate, blockSize);

        // Set parameters - max attack, variable sustain
        std::map<int, float> params;
        params[TransientShaper_Platinum::Attack] = 0.5f;    // Unity gain (0dB)
        params[TransientShaper_Platinum::Sustain] = sustainParam;
        params[TransientShaper_Platinum::AttackTime] = 0.1f;
        params[TransientShaper_Platinum::ReleaseTime] = 0.3f;
        params[TransientShaper_Platinum::Mix] = 1.0f;       // 100% wet
        shaper.updateParameters(params);

        // Generate test signal (drum hit)
        auto inputSignal = generateDrumHit(sampleRate, 0.5f);
        float inputPeakDB = measurePeakDB(inputSignal);

        // Process in blocks
        std::vector<float> outputSignal;
        for (size_t pos = 0; pos < inputSignal.size(); pos += blockSize) {
            int samples = std::min(blockSize, static_cast<int>(inputSignal.size() - pos));

            juce::AudioBuffer<float> buffer(2, samples);
            buffer.clear();

            // Copy input to both channels
            for (int i = 0; i < samples; ++i) {
                buffer.setSample(0, i, inputSignal[pos + i]);
                buffer.setSample(1, i, inputSignal[pos + i]);
            }

            // Process
            shaper.process(buffer);

            // Extract output
            for (int i = 0; i < samples; ++i) {
                outputSignal.push_back(buffer.getSample(0, i));
            }
        }

        // Measure output
        float outputPeakDB = measurePeakDB(outputSignal);
        float gainDB = outputPeakDB - inputPeakDB;

        // Check for runaway
        bool pass = (outputPeakDB < 20.0f); // Must stay below +20dB

        printf("%6.0f%% | %9.2f dB | %10.2f dB | %8.2f dB | %s\n",
               sustainParam * 100.0f,
               inputPeakDB,
               outputPeakDB,
               gainDB,
               pass ? "PASS" : "FAIL - RUNAWAY!");

        if (!pass) {
            std::cout << "  ERROR: Output exceeded +20dB safety limit!\n";
        }
    }
}

// Test with transient-rich material
void testTransientRichMaterial() {
    const int sampleRate = 48000;
    const int blockSize = 512;

    std::cout << "\n=== TRANSIENT-RICH MATERIAL TEST ===\n";
    std::cout << "Testing with multiple sharp transients + sustained tone\n\n";

    TransientShaper_Platinum shaper;
    shaper.prepareToPlay(sampleRate, blockSize);

    // Extreme settings
    std::map<int, float> params;
    params[TransientShaper_Platinum::Attack] = 1.0f;    // Max attack boost
    params[TransientShaper_Platinum::Sustain] = 1.0f;   // Max sustain boost
    params[TransientShaper_Platinum::AttackTime] = 0.0f;
    params[TransientShaper_Platinum::ReleaseTime] = 0.0f;
    params[TransientShaper_Platinum::Mix] = 1.0f;
    shaper.updateParameters(params);

    // Generate test signal
    auto inputSignal = generateTransientRich(sampleRate, 0.5f);
    float inputPeakDB = measurePeakDB(inputSignal);

    // Process
    std::vector<float> outputSignal;
    for (size_t pos = 0; pos < inputSignal.size(); pos += blockSize) {
        int samples = std::min(blockSize, static_cast<int>(inputSignal.size() - pos));

        juce::AudioBuffer<float> buffer(2, samples);
        buffer.clear();

        for (int i = 0; i < samples; ++i) {
            buffer.setSample(0, i, inputSignal[pos + i]);
            buffer.setSample(1, i, inputSignal[pos + i]);
        }

        shaper.process(buffer);

        for (int i = 0; i < samples; ++i) {
            outputSignal.push_back(buffer.getSample(0, i));
        }
    }

    float outputPeakDB = measurePeakDB(outputSignal);
    float gainDB = outputPeakDB - inputPeakDB;

    std::cout << "Input Peak:  " << inputPeakDB << " dB\n";
    std::cout << "Output Peak: " << outputPeakDB << " dB\n";
    std::cout << "Gain:        " << gainDB << " dB\n";

    if (outputPeakDB < 20.0f) {
        std::cout << "Status: PASS - Output within safe limits\n";
    } else {
        std::cout << "Status: FAIL - Output exceeded +20dB!\n";
    }
}

// Stress test: Maximum everything
void stressTest() {
    const int sampleRate = 48000;
    const int blockSize = 512;

    std::cout << "\n=== STRESS TEST: MAXIMUM PARAMETERS ===\n";
    std::cout << "All parameters set to maximum (1.0)\n\n";

    TransientShaper_Platinum shaper;
    shaper.prepareToPlay(sampleRate, blockSize);

    // ALL PARAMETERS TO MAX
    std::map<int, float> params;
    params[TransientShaper_Platinum::Attack] = 1.0f;
    params[TransientShaper_Platinum::Sustain] = 1.0f;
    params[TransientShaper_Platinum::AttackTime] = 1.0f;
    params[TransientShaper_Platinum::ReleaseTime] = 1.0f;
    params[TransientShaper_Platinum::Separation] = 1.0f;
    params[TransientShaper_Platinum::Mix] = 1.0f;
    shaper.updateParameters(params);

    // Test with both signal types
    std::vector<std::pair<std::string, std::vector<float>>> testSignals = {
        {"Drum Hit", generateDrumHit(sampleRate, 0.5f)},
        {"Transient Rich", generateTransientRich(sampleRate, 0.5f)}
    };

    for (const auto& [name, inputSignal] : testSignals) {
        float inputPeakDB = measurePeakDB(inputSignal);

        std::vector<float> outputSignal;
        for (size_t pos = 0; pos < inputSignal.size(); pos += blockSize) {
            int samples = std::min(blockSize, static_cast<int>(inputSignal.size() - pos));

            juce::AudioBuffer<float> buffer(2, samples);
            buffer.clear();

            for (int i = 0; i < samples; ++i) {
                buffer.setSample(0, i, inputSignal[pos + i]);
                buffer.setSample(1, i, inputSignal[pos + i]);
            }

            shaper.process(buffer);

            for (int i = 0; i < samples; ++i) {
                outputSignal.push_back(buffer.getSample(0, i));
            }
        }

        float outputPeakDB = measurePeakDB(outputSignal);
        float gainDB = outputPeakDB - inputPeakDB;
        bool pass = (outputPeakDB < 20.0f);

        std::cout << name << ":\n";
        std::cout << "  Input:  " << inputPeakDB << " dB\n";
        std::cout << "  Output: " << outputPeakDB << " dB\n";
        std::cout << "  Gain:   " << gainDB << " dB\n";
        std::cout << "  Status: " << (pass ? "PASS" : "FAIL") << "\n\n";
    }
}

int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║  TRANSIENT SHAPER - RUNAWAY GAIN FIX VERIFICATION   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";

    testSustainLevels();
    testTransientRichMaterial();
    stressTest();

    std::cout << "\n=== TEST COMPLETE ===\n";
    std::cout << "All tests should show output peaks below +20dB\n";
    std::cout << "If any test shows 'FAIL - RUNAWAY!', the fix is incomplete\n\n";

    return 0;
}
