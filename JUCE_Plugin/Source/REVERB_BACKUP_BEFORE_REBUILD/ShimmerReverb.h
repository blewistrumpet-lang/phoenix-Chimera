#pragma once
#include <JuceHeader.h>
#include "EngineBase.h"
#include <vector>
#include <memory>

class ShimmerReverb : public EngineBase {
public:
    ShimmerReverb();
    ~ShimmerReverb() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getParameterName(int index) const override;
    int getNumParameters() const override;
    juce::String getName() const override;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};