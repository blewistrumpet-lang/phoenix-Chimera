#pragma once
#include "EngineBase.h"

class ShimmerReverb : public EngineBase {
public:
    ShimmerReverb();
    ~ShimmerReverb() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Shimmer Reverb"; }
    
private:
    double m_sampleRate = 44100.0;
};