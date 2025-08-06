#pragma once
#include "EngineBase.h"
#include <memory>

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
    
    // Get total processing latency in samples
    int getLatencySamples() const;
    
    // Parameter indices
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
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};