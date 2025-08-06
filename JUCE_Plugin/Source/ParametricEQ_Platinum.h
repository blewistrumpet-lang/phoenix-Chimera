#pragma once
#include "EngineBase.h"
#include <memory>

class ParametricEQ_Platinum : public EngineBase {
public:
    ParametricEQ_Platinum();
    ~ParametricEQ_Platinum() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 9; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Parametric EQ Platinum"; }
    
    // Parameter indices
    enum ParamID {
        kLowGain = 0,
        kLowFreq,
        kMidGain,
        kMidFreq,
        kMidQ,
        kHighGain,
        kHighFreq,
        kOutputGain,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};