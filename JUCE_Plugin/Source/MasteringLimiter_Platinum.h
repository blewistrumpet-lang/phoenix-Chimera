// ===============================================================
// MasteringLimiter_Platinum.h - Rewritten (API-compatible)
// ===============================================================
#pragma once

#include "EngineBase.h"
#include <memory>
#include <atomic>

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