#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>

// Forward declarations
namespace juce { 
    template<typename T> class AudioBuffer;
    class MidiBuffer;
}

/**
 * TransientShaper_Platinum - Professional-grade transient processor
 * 
 * Features:
 * - Multi-algorithm detection (Peak, RMS, Hilbert, Hybrid)
 * - Zero-latency and lookahead modes
 * - Soft-knee compression for natural dynamics
 * - Professional oversampling (2x/4x)
 * - Complete denormal protection
 * - Thread-safe parameter automation
 */
class TransientShaper_Platinum : public EngineBase {
public:
    TransientShaper_Platinum();
    ~TransientShaper_Platinum() override;
    
    // Core audio interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Parameter interface
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Transient Shaper Pro"; }
    
    // Parameter IDs
    enum ParamID : int {
        Attack = 0,
        Sustain = 1,
        AttackTime = 2,
        ReleaseTime = 3,
        Separation = 4,
        Detection = 5,
        Lookahead = 6,
        SoftKnee = 7,
        Oversampling = 8,
        Mix = 9
    };

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};