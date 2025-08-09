#pragma once
#include "EngineBase.h"
#include <memory>

class IntelligentHarmonizer : public EngineBase {
public:
    IntelligentHarmonizer();
    ~IntelligentHarmonizer() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Intelligent Harmonizer"; }
    
    // Get total processing latency in samples
    int getLatencySamples() const noexcept override;
    
    // Parameter indices
    enum ParamID {
        kInterval = 0,
        kKey,
        kScale,
        kVoices,
        kSpread,
        kHumanize,
        kFormant,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};