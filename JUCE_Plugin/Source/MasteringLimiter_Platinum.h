// MasteringLimiter_Platinum.h - Professional Minimal Header
// Copyright (c) 2024 - Platinum DSP Series
// Broadcast-compliant, studio-grade mastering limiter
#pragma once

#include "EngineBase.h"
#include <memory>
#include <atomic>

/**
 * @brief Professional mastering limiter with true peak detection
 * 
 * Features:
 * - ITU-R BS.1770-4 compliant true peak detection
 * - 16x linear-phase oversampling
 * - Predictive lookahead with 3rd-order analysis
 * - Adaptive program-dependent release
 * - AVX2/SSE2 optimized processing
 * - Lock-free parameter updates
 * - Zero allocations in audio thread
 * 
 * Performance targets:
 * - Latency: < 3ms @ 64 samples
 * - CPU: < 35% single core (Apple M2/Intel i7)
 * - THD+N: < 0.001%
 * - Dynamic Range: > 144dB
 */
class MasteringLimiter_Platinum : public EngineBase {
public:
    MasteringLimiter_Platinum();
    ~MasteringLimiter_Platinum() override;
    
    // Core EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Engine info
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Mastering Limiter Platinum"; }
    
    // Parameter indices
    enum ParamID {
        kThreshold = 0,
        kCeiling,
        kRelease,
        kLookahead,
        kKnee,
        kMakeup,
        kSaturation,
        kStereoLink,
        kTruePeak,
        kMix
    };
    
    // Metering getters (atomic reads, returns dB)
    float getGainReduction() const noexcept;
    float getInputLevel() const noexcept;
    float getOutputLevel() const noexcept;
    float getTruePeakLevel() const noexcept;
    
private:
    // PIMPL to hide implementation details
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    
    // Only atomic meters in header for lock-free access
    mutable std::atomic<float> m_grMeter{0.0f};
    mutable std::atomic<float> m_inputMeter{0.0f};
    mutable std::atomic<float> m_outputMeter{0.0f};
    mutable std::atomic<float> m_truePeakMeter{0.0f};
};