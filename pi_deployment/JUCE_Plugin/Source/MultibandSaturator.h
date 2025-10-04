#pragma once
#include "EngineBase.h"
#include <array>
#include <memory>

class MultibandSaturator : public EngineBase {
public:
    MultibandSaturator();
    ~MultibandSaturator() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Multiband Saturator Ultimate"; }
    int getNumParameters() const override { return 7; }
    juce::String getParameterName(int index) const override;
    
    enum ParamID {
        kLowDrive = 0,
        kMidDrive,
        kHighDrive,
        kSaturationType,
        kHarmonicCharacter,
        kOutputGain,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};