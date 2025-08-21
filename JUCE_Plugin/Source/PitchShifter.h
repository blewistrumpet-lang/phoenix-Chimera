#pragma once
#include "EngineBase.h"
#include <memory>
#include <juce_core/juce_core.h>

// Forward declarations only
namespace juce { 
    template<typename T> class AudioBuffer; 
}

/**
 * Platinum-spec pitch shifter with phase vocoder architecture
 * 
 * Features:
 * - Real-time safe with zero allocations in process()
 * - Lock-free parameter updates
 * - Denormal prevention throughout
 * - < 10ms latency @ 48kHz
 * - > 120dB dynamic range
 */
class PitchShifter final : public EngineBase {
public:
    PitchShifter();
    ~PitchShifter() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Pitch Shifter"; }
    
    // Get the actual snapped value for display (3 decimal places)
    juce::String getParameterText(int index, float value) const;
    
    // Parameter indices
    enum ParamIndex {
        kPitch = 0,
        kFormant = 1,
        kMix = 2,
        kWindow = 3,
        kGate = 4,
        kGrain = 5,
        kFeedback = 6,
        kWidth = 7
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};