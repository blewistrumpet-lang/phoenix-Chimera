#pragma once
#include "EngineBase.h"
#include <memory>

class DimensionExpander : public EngineBase {
public:
    DimensionExpander();
    ~DimensionExpander() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Dimension Expander"; }
    
    // Parameter indices
    enum ParamID {
        kWidth = 0,
        kDepth,
        kCrossfeed,
        kBassRetention,
        kAmbience,
        kMovement,
        kClarity,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};