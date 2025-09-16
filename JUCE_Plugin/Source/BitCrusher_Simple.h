#pragma once
#include "EngineBase.h"
#include <array>

class BitCrusher_Simple : public EngineBase {
public:
    BitCrusher_Simple();
    ~BitCrusher_Simple() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Bit Crusher"; }
    
private:
    // Direct parameter values - no smoothing for bit depth!
    float m_bitDepth = 16.0f;
    float m_sampleRateReduction = 1.0f;
    float m_aliasing = 0.0f;
    float m_jitter = 0.0f;
    float m_dcOffset = 0.0f;
    float m_gateThreshold = 0.0f;
    float m_dither = 0.0f;
    float m_mix = 1.0f;
    
    // Per-channel state for sample rate reduction
    struct ChannelState {
        float heldSample = 0.0f;
        float sampleCounter = 0.0f;
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Simple quantization - the core of bit crushing
    float quantize(float input, float bits);
};