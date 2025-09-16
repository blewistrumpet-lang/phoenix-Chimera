#pragma once
#include "EngineBase.h"
#include <memory>

class ConvolutionReverb : public EngineBase {
public:
    ConvolutionReverb();
    ~ConvolutionReverb() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override;
    juce::String getParameterName(int index) const override;
    juce::String getName() const override;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};