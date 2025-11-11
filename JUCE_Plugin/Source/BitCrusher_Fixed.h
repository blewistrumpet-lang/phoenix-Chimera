#pragma once
#include "EngineBase.h"
#include <juce_dsp/juce_dsp.h>

class BitCrusher : public EngineBase {
public:
    BitCrusher() = default;
    ~BitCrusher() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    int getNumParameters() const override { return 3; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Bit Crusher"; }

private:
    // Parameters with smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> m_bits;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> m_downsample;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> m_mix;

    // Sample and hold state
    float m_heldSampleL = 0.0f;
    float m_heldSampleR = 0.0f;
    float m_counterL = 0.0f;
    float m_counterR = 0.0f;

    // Denormal protection
    static constexpr float DENORMAL_OFFSET = 1e-8f;

    double m_sampleRate = 44100.0;
};