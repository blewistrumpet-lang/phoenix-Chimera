#pragma once
#include "EngineBase.h"
#include <memory>
#include <juce_core/juce_core.h>

// Forward declarations
namespace juce { 
    template<typename T> class AudioBuffer; 
}

/**
 * Platinum-spec analog phaser with thermal modeling
 * 
 * Features:
 * - 2/4/6/8 all-pass stages
 * - Real-time safe with zero allocations
 * - Lock-free parameter updates
 * - Comprehensive denormal prevention
 * - < 1ms latency @ 48kHz
 * - Thermal drift modeling
 */
class AnalogPhaser final : public EngineBase {
public:
    AnalogPhaser();
    ~AnalogPhaser() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Analog Phaser"; }
    
    // Parameter indices
    enum ParamIndex {
        kRate = 0,
        kDepth = 1,
        kFeedback = 2,
        kStages = 3,
        kStereoSpread = 4,
        kCenterFreq = 5,
        kResonance = 6,
        kMix = 7
    };
    
    // Quality metrics access (thread-safe)
    float getCPUUsage() const;
    float getDynamicRangeDB() const;
    std::string getQualityReport() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};