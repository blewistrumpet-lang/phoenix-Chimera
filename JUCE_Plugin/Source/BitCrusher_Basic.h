#pragma once
#include "EngineBase.h"

class BitCrusher_Basic : public EngineBase {
public:
    BitCrusher_Basic() = default;
    ~BitCrusher_Basic() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 3; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Bit Crusher"; }
    
private:
    // Just the essentials
    float m_bits = 16.0f;
    float m_downsample = 1.0f;
    float m_mix = 1.0f;
    
    // Sample and hold state
    float m_heldSampleL = 0.0f;
    float m_heldSampleR = 0.0f;
    float m_counterL = 0.0f;
    float m_counterR = 0.0f;
};