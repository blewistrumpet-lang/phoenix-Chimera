#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <memory>
#include <map>

// Forward declarations
namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * Platinum-spec Gated Reverb
 * - Thread-safe with lock-free parameters
 * - Zero allocations in audio thread
 * - Full denormal protection
 * - SIMD-optimized processing
 */
class GatedReverb : public EngineBase {
public:
    GatedReverb();
    ~GatedReverb() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Gated Reverb"; }
    
    // Parameter indices
    enum ParamID {
        kRoomSize = 0,
        kGateTime,
        kThreshold,
        kPreDelay,
        kDamping,
        kGateShape,
        kBrightness,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};