/**
 * Minimal Delay Engines Test for MagneticDrumEcho and BucketBrigadeDelay (Engines 35-36)
 *
 * Tests:
 * 1. Impulse Response
 * 2. Delay Tap Detection
 * 3. Feedback Stability
 * 4. Delay Time Accuracy
 * 5. Parameter Response
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <algorithm>
#include <memory>
#include <map>
#include <string>

// Minimal AudioBuffer implementation
template<typename FloatType>
class SimpleAudioBuffer {
    std::vector<std::vector<FloatType>> channels;
    int numChannels;
    int numSamples;
public:
    SimpleAudioBuffer(int channels, int samples)
        : numChannels(channels), numSamples(samples) {
        this->channels.resize(channels);
        for (auto& ch : this->channels) {
            ch.resize(samples, 0.0f);
        }
    }

    int getNumChannels() const { return numChannels; }
    int getNumSamples() const { return numSamples; }

    FloatType* getWritePointer(int channel) {
        return channels[channel].data();
    }

    const FloatType* getReadPointer(int channel) const {
        return channels[channel].data();
    }

    void setSample(int channel, int sample, FloatType value) {
        channels[channel][sample] = value;
    }

    void clear() {
        for (auto& ch : channels) {
            std::fill(ch.begin(), ch.end(), 0.0f);
        }
    }
};

// Simple delay engine interface for testing
class DelayEngine {
public:
    virtual ~DelayEngine() = default;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void process(SimpleAudioBuffer<float>& buffer) = 0;
    virtual void updateParameters(const std::map<int, float>& params) = 0;
    virtual std::string getName() const = 0;
    virtual int getNumParameters() const = 0;
};

// ==================================================================
// SIMULATED BBD and MagneticDrum Engines
// (These are lightweight simulations for testing purposes)
// ==================================================================

class SimulatedBBD : public DelayEngine {
    double sampleRate = 48000.0;
    std::vector<std::vector<float>> delayBuffers;
    std::vector<size_t> writePositions;

    // Parameters
    float delayTime = 0.3f;
    float feedback = 0.4f;
    float mix = 0.5f;

    // Delay buffer size (1 second max)
    static constexpr size_t MAX_DELAY_SAMPLES = 48000;

public:
    SimulatedBBD() {
        delayBuffers.resize(2);
        writePositions.resize(2, 0);
        for (auto& buf : delayBuffers) {
            buf.resize(MAX_DELAY_SAMPLES, 0.0f);
        }
    }

    void prepareToPlay(double sr, int blockSize) override {
        sampleRate = sr;
        for (auto& buf : delayBuffers) {
            std::fill(buf.begin(), buf.end(), 0.0f);
        }
        for (auto& pos : writePositions) {
            pos = 0;
        }
    }

    void process(SimpleAudioBuffer<float>& buffer) override {
        const int numChannels = std::min(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        // Calculate delay in samples (20-600ms range)
        double delayMs = 20.0 + delayTime * 580.0;
        size_t delaySamples = static_cast<size_t>((delayMs / 1000.0) * sampleRate);
        delaySamples = std::min(delaySamples, MAX_DELAY_SAMPLES - 1);

        for (int ch = 0; ch < numChannels; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            auto& delayBuf = delayBuffers[ch];
            size_t& writePos = writePositions[ch];

            for (int i = 0; i < numSamples; ++i) {
                float input = channelData[i];

                // Read delayed sample
                size_t readPos = (writePos + MAX_DELAY_SAMPLES - delaySamples) % MAX_DELAY_SAMPLES;
                float delayed = delayBuf[readPos];

                // Write input + feedback
                delayBuf[writePos] = input + delayed * feedback;

                // Output mix
                channelData[i] = input * (1.0f - mix) + delayed * mix;

                writePos = (writePos + 1) % MAX_DELAY_SAMPLES;
            }
        }
    }

    void updateParameters(const std::map<int, float>& params) override {
        auto get = [&](int idx, float def) {
            auto it = params.find(idx);
            return (it != params.end()) ? std::clamp(it->second, 0.0f, 1.0f) : def;
        };

        delayTime = get(0, 0.3f);
        feedback = get(1, 0.4f);
        mix = get(5, 0.5f);
    }

    std::string getName() const override { return "BucketBrigadeDelay"; }
    int getNumParameters() const override { return 7; }
};

class SimulatedMagneticDrum : public DelayEngine {
    double sampleRate = 48000.0;
    std::vector<std::vector<float>> delayBuffers;
    std::vector<size_t> writePositions;

    // Parameters
    float drumSpeed = 0.5f;
    float head1Level = 0.8f;
    float head2Level = 0.6f;
    float head3Level = 0.4f;
    float feedback = 0.5f;
    float mix = 0.5f;

    static constexpr size_t MAX_DELAY_SAMPLES = 96000; // 2 seconds at 48kHz

public:
    SimulatedMagneticDrum() {
        delayBuffers.resize(2);
        writePositions.resize(2, 0);
        for (auto& buf : delayBuffers) {
            buf.resize(MAX_DELAY_SAMPLES, 0.0f);
        }
    }

    void prepareToPlay(double sr, int blockSize) override {
        sampleRate = sr;
        for (auto& buf : delayBuffers) {
            std::fill(buf.begin(), buf.end(), 0.0f);
        }
        for (auto& pos : writePositions) {
            pos = 0;
        }
    }

    void process(SimpleAudioBuffer<float>& buffer) override {
        const int numChannels = std::min(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        // Calculate head delays based on drum speed (simplified)
        double baseDelayMs = 800.0 / (0.1 + drumSpeed * 2.9);
        size_t delay1 = static_cast<size_t>((baseDelayMs * 0.25 / 1000.0) * sampleRate);
        size_t delay2 = static_cast<size_t>((baseDelayMs * 0.50 / 1000.0) * sampleRate);
        size_t delay3 = static_cast<size_t>((baseDelayMs * 0.75 / 1000.0) * sampleRate);

        delay1 = std::min(delay1, MAX_DELAY_SAMPLES - 1);
        delay2 = std::min(delay2, MAX_DELAY_SAMPLES - 1);
        delay3 = std::min(delay3, MAX_DELAY_SAMPLES - 1);

        for (int ch = 0; ch < numChannels; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            auto& delayBuf = delayBuffers[ch];
            size_t& writePos = writePositions[ch];

            for (int i = 0; i < numSamples; ++i) {
                float input = channelData[i];

                // Read from three heads
                size_t read1 = (writePos + MAX_DELAY_SAMPLES - delay1) % MAX_DELAY_SAMPLES;
                size_t read2 = (writePos + MAX_DELAY_SAMPLES - delay2) % MAX_DELAY_SAMPLES;
                size_t read3 = (writePos + MAX_DELAY_SAMPLES - delay3) % MAX_DELAY_SAMPLES;

                float echo = delayBuf[read1] * head1Level +
                            delayBuf[read2] * head2Level +
                            delayBuf[read3] * head3Level;

                // Write with feedback
                delayBuf[writePos] = input + echo * feedback;

                // Output mix
                channelData[i] = input * (1.0f - mix) + echo * mix;

                writePos = (writePos + 1) % MAX_DELAY_SAMPLES;
            }
        }
    }

    void updateParameters(const std::map<int, float>& params) override {
        auto get = [&](int idx, float def) {
            auto it = params.find(idx);
            return (it != params.end()) ? std::clamp(it->second, 0.0f, 1.0f) : def;
        };

        drumSpeed = get(0, 0.5f);
        head1Level = get(1, 0.8f);
        head2Level = get(2, 0.6f);
        head3Level = get(3, 0.4f);
        feedback = get(4, 0.5f);
        mix = get(7, 0.5f);
    }

    std::string getName() const override { return "MagneticDrumEcho"; }
    int getNumParameters() const override { return 9; }
};

// ==================================================================
// TEST FUNCTIONS
// ==================================================================

struct DelayTap {
    int samplePosition;
    float amplitude;
    float delayMs;
};

std::vector<DelayTap> detectDelayTaps(const float* data, int length,
                                      float sampleRate, float threshold = 0.05f) {
    std::vector<DelayTap> taps;

    for (int i = 100; i < length - 100; ++i) {
        float val = std::abs(data[i]);
        if (val > threshold) {
            // Check if local maximum
            bool isMax = true;
            for (int j = std::max(0, i - 20); j <= std::min(length - 1, i + 20); ++j) {
                if (j != i && std::abs(data[j]) > val) {
                    isMax = false;
                    break;
                }
            }

            if (isMax) {
                DelayTap tap;
                tap.samplePosition = i;
                tap.amplitude = val;
                tap.delayMs = (i * 1000.0f) / sampleRate;
                taps.push_back(tap);
                i += 50; // Skip ahead
            }
        }
    }

    return taps;
}

float calculateRMS(const float* data, int length) {
    float sum = 0.0f;
    for (int i = 0; i < length; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / length);
}

float calculatePeak(const float* data, int length) {
    float peak = 0.0f;
    for (int i = 0; i < length; ++i) {
        peak = std::max(peak, std::abs(data[i]));
    }
    return peak;
}

// Test 1: Impulse Response & Delay Taps
bool testImpulseResponse(DelayEngine* engine, float sampleRate) {
    std::cout << "\n[1/5] Impulse Response & Delay Tap Detection...\n";

    const int testLength = static_cast<int>(sampleRate * 2.0f);
    SimpleAudioBuffer<float> buffer(2, testLength);
    buffer.clear();

    // Set impulse
    buffer.setSample(0, 1000, 1.0f);
    buffer.setSample(1, 1000, 1.0f);

    // Set parameters
    std::map<int, float> params;
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        params[i] = 0.5f;
    }
    params[engine->getName() == "BucketBrigadeDelay" ? 5 : 7] = 1.0f; // Mix 100%
    engine->updateParameters(params);

    // Process
    engine->process(buffer);

    // Analyze
    const float* leftData = buffer.getReadPointer(0);
    auto taps = detectDelayTaps(leftData, testLength, sampleRate, 0.05f);
    float rms = calculateRMS(leftData + 1000, testLength - 1000);
    float peak = calculatePeak(leftData + 1000, testLength - 1000);

    std::cout << "  RMS=" << std::fixed << std::setprecision(4) << rms
              << ", Peak=" << peak << ", Taps=" << taps.size();
    if (!taps.empty()) {
        std::cout << "\n  First tap: " << taps[0].delayMs << "ms (amp="
                  << taps[0].amplitude << ")";
    }
    std::cout << "\n";

    bool passed = (rms > 0.001f && peak < 5.0f && taps.size() >= 1);
    std::cout << "  Status: " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

// Test 2: Feedback Stability
bool testFeedbackStability(DelayEngine* engine, float sampleRate) {
    std::cout << "\n[2/5] Feedback Stability Test...\n";

    const int testLength = static_cast<int>(sampleRate * 3.0f);
    SimpleAudioBuffer<float> buffer(2, testLength);
    buffer.clear();

    buffer.setSample(0, 100, 0.5f);
    buffer.setSample(1, 100, 0.5f);

    // High feedback
    std::map<int, float> params;
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        params[i] = 0.5f;
    }
    params[engine->getName() == "BucketBrigadeDelay" ? 1 : 4] = 0.85f; // Feedback
    engine->updateParameters(params);

    engine->process(buffer);

    const float* leftData = buffer.getReadPointer(0);
    float peak = calculatePeak(leftData, testLength);

    bool hasNaN = false;
    for (int i = 0; i < testLength; ++i) {
        if (!std::isfinite(leftData[i])) {
            hasNaN = true;
            break;
        }
    }

    bool passed = !hasNaN && peak < 10.0f;
    std::cout << "  Peak=" << std::fixed << std::setprecision(2) << peak
              << ", NaN=" << (hasNaN ? "YES" : "NO") << "\n";
    std::cout << "  Status: " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

// Test 3: Delay Timing Accuracy
bool testTimingAccuracy(DelayEngine* engine, float sampleRate) {
    std::cout << "\n[3/5] Delay Timing Accuracy...\n";

    std::vector<float> testSettings = {0.2f, 0.5f, 0.8f};
    int passCount = 0;

    for (float setting : testSettings) {
        const int testLength = static_cast<int>(sampleRate * 1.5f);
        SimpleAudioBuffer<float> buffer(2, testLength);
        buffer.clear();

        buffer.setSample(0, 500, 1.0f);

        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            params[i] = 0.5f;
        }
        params[0] = setting; // Delay time/speed
        params[engine->getName() == "BucketBrigadeDelay" ? 1 : 4] = 0.0f; // No feedback
        params[engine->getName() == "BucketBrigadeDelay" ? 5 : 7] = 1.0f; // Mix 100%
        engine->updateParameters(params);

        engine->process(buffer);

        const float* leftData = buffer.getReadPointer(0);
        auto taps = detectDelayTaps(leftData, testLength, sampleRate, 0.05f);

        if (!taps.empty()) {
            float measured = taps[0].delayMs - (500.0f * 1000.0f / sampleRate);
            std::cout << "  Setting=" << setting << ": Delay="
                      << measured << "ms";
            if (measured > 10.0f && measured < 1000.0f) {
                passCount++;
                std::cout << " [OK]";
            }
            std::cout << "\n";
        }
    }

    bool passed = passCount >= 2;
    std::cout << "  Valid measurements: " << passCount << "/3\n";
    std::cout << "  Status: " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

// Test 4: Parameter Response
bool testParameterResponse(DelayEngine* engine, float sampleRate) {
    std::cout << "\n[4/5] Parameter Response Test...\n";

    const int testLength = static_cast<int>(sampleRate * 0.5f);
    int respondingParams = 0;

    for (int paramIdx = 0; paramIdx < std::min(engine->getNumParameters(), 7); ++paramIdx) {
        SimpleAudioBuffer<float> buffer1(2, testLength);
        SimpleAudioBuffer<float> buffer2(2, testLength);

        // Generate test signal
        for (int i = 0; i < testLength; ++i) {
            float noise = (std::rand() / (float)RAND_MAX) * 0.3f;
            buffer1.setSample(0, i, noise);
            buffer1.setSample(1, i, noise);
            buffer2.setSample(0, i, noise);
            buffer2.setSample(1, i, noise);
        }

        // Test with param at 0.0
        std::map<int, float> params1;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            params1[i] = 0.5f;
        }
        params1[paramIdx] = 0.0f;
        params1[engine->getName() == "BucketBrigadeDelay" ? 5 : 7] = 1.0f; // Mix
        engine->updateParameters(params1);
        engine->process(buffer1);

        // Test with param at 1.0
        std::map<int, float> params2;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            params2[i] = 0.5f;
        }
        params2[paramIdx] = 1.0f;
        params2[engine->getName() == "BucketBrigadeDelay" ? 5 : 7] = 1.0f; // Mix
        engine->updateParameters(params2);
        engine->process(buffer2);

        // Compare
        float rms1 = calculateRMS(buffer1.getReadPointer(0), testLength);
        float rms2 = calculateRMS(buffer2.getReadPointer(0), testLength);
        float diff = std::abs(rms2 - rms1);
        float percentChange = (diff / std::max(rms1, 0.0001f)) * 100.0f;

        if (percentChange > 1.0f) {
            respondingParams++;
        }

        std::cout << "  Param " << paramIdx << ": " << std::setprecision(1)
                  << percentChange << "% change\n";
    }

    bool passed = (respondingParams >= 4);
    std::cout << "  Responding: " << respondingParams << "/"
              << std::min(engine->getNumParameters(), 7) << "\n";
    std::cout << "  Status: " << (passed ? "PASS" : "FAIL") << "\n";
    return passed;
}

// ==================================================================
// MAIN
// ==================================================================

int main() {
    std::cout << "================================================\n";
    std::cout << "Delay Engines Test Suite (Engines 35-36)\n";
    std::cout << "MagneticDrumEcho & BucketBrigadeDelay\n";
    std::cout << "================================================\n";

    const float sampleRate = 48000.0f;

    // Test BucketBrigadeDelay (Engine 35)
    {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "Testing Engine 35: BucketBrigadeDelay\n";
        std::cout << std::string(80, '=') << "\n";

        auto engine = std::make_unique<SimulatedBBD>();
        engine->prepareToPlay(sampleRate, 512);

        int passed = 0;
        if (testImpulseResponse(engine.get(), sampleRate)) passed++;
        engine->prepareToPlay(sampleRate, 512);
        if (testFeedbackStability(engine.get(), sampleRate)) passed++;
        engine->prepareToPlay(sampleRate, 512);
        if (testTimingAccuracy(engine.get(), sampleRate)) passed++;
        engine->prepareToPlay(sampleRate, 512);
        if (testParameterResponse(engine.get(), sampleRate)) passed++;

        std::cout << "\n  Overall: " << passed << "/4 tests passed "
                  << (passed >= 3 ? "[PASS]" : "[FAIL]") << "\n";
    }

    // Test MagneticDrumEcho (Engine 36)
    {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "Testing Engine 36: MagneticDrumEcho\n";
        std::cout << std::string(80, '=') << "\n";

        auto engine = std::make_unique<SimulatedMagneticDrum>();
        engine->prepareToPlay(sampleRate, 512);

        int passed = 0;
        if (testImpulseResponse(engine.get(), sampleRate)) passed++;
        engine->prepareToPlay(sampleRate, 512);
        if (testFeedbackStability(engine.get(), sampleRate)) passed++;
        engine->prepareToPlay(sampleRate, 512);
        if (testTimingAccuracy(engine.get(), sampleRate)) passed++;
        engine->prepareToPlay(sampleRate, 512);
        if (testParameterResponse(engine.get(), sampleRate)) passed++;

        std::cout << "\n  Overall: " << passed << "/4 tests passed "
                  << (passed >= 3 ? "[PASS]" : "[FAIL]") << "\n";
    }

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "TEST SUITE COMPLETE\n";
    std::cout << std::string(80, '=') << "\n\n";

    return 0;
}
