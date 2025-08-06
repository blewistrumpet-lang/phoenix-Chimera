#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>

/**
 * Professional Spectral Noise Gate - Platinum Edition
 * 
 * Features:
 * - 2048-point FFT with 75% overlap
 * - Per-frequency bin gating with individual envelopes
 * - Double precision envelope tracking
 * - Lookahead processing (0-10ms)
 * - Perfect reconstruction windowing
 * - SIMD-optimized spectral processing
 * - Complete denormal protection
 * 
 * @version 1.0.0 - Production Ready
 */
class SpectralGate_Platinum : public EngineBase {
public:
    SpectralGate_Platinum();
    ~SpectralGate_Platinum() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Spectral Gate Platinum"; }
    
    // Parameter indices
    enum ParamID {
        kThreshold = 0,
        kRatio,
        kAttack,
        kRelease,
        kFreqLow,
        kFreqHigh,
        kLookahead,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};