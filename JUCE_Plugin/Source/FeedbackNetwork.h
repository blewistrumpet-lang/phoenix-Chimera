#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <memory>
#include <vector>
#include <map>
#include <atomic>
#include <cmath>

class FeedbackNetwork : public EngineBase {
public:
    FeedbackNetwork();
    ~FeedbackNetwork() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Feedback Network"; }
    int getLatencySamples() const noexcept override { return latencySamples; }

    enum ParamID {
        kDelayTime = 0,
        kFeedback,
        kCrossFeed,
        kDiffusion,
        kModulation,
        kFreeze,
        kShimmer,
        kMix
    };

private:
    // Delay lines for feedback network
    struct DelayLine {
        std::vector<float> buffer;
        size_t writeIndex = 0;
        void setSize(size_t size) { buffer.assign(size, 0.0f); writeIndex = 0; }
        void clear() { std::fill(buffer.begin(), buffer.end(), 0.0f); }
        inline float read(size_t delaySamples) const {
            size_t idx = (writeIndex + buffer.size() - delaySamples) % buffer.size();
            return buffer[idx];
        }
        inline void write(float sample) {
            buffer[writeIndex] = sample;
            writeIndex = (writeIndex + 1) % buffer.size();
        }
    };

    double fs = 44100.0;
    int latencySamples = 0;
    DelayLine delayL, delayR;

    // Parameters (smoothed)
    float delayTimeSec = 0.25f;
    float feedback = 0.5f;
    float crossFeed = 0.0f;
    float diffusion = 0.0f;
    float modulationDepth = 0.0f;
    float freeze = 0.0f;
    float shimmer = 0.0f;
    float mix = 1.0f;

    // Modulation
    double modPhaseL = 0.0;
    double modPhaseR = 0.0;
    double modRate = 0.1; // Hz

    inline float sanitize(float x) {
        return DSPUtils::flushDenorm(std::isfinite(x) ? x : 0.0f);
    }
};