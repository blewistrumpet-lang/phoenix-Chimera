#pragma once

#include "EngineBase.h"
#include <memory>
#include <map>

namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * GatedReverb - Dynamic gated reverb with envelope control
 * 
 * 10 Professional Parameters:
 * 0: Mix - Dry/Wet balance (0.0-1.0)
 * 1: Threshold - Gate threshold level (0.0-1.0)
 * 2: Hold - Gate hold time (0.0-1.0 = 10ms to 500ms)
 * 3: Release - Gate release time (0.0-1.0 = 10ms to 1000ms)
 * 4: Attack - Gate attack time (0.0-1.0 = 0.1ms to 100ms)
 * 5: Size - Room size before gating (0.0-1.0)
 * 6: Damping - High frequency damping (0.0-1.0)
 * 7: Pre-Delay - Pre-delay time (0.0-1.0)
 * 8: Low Cut - High-pass filter (0.0-1.0)
 * 9: High Cut - Low-pass filter (0.0-1.0)
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
    
    // Parameter information
    int getNumParameters() const override;
    juce::String getParameterName(int index) const override;
    juce::String getName() const override;
    
private:
    // Implementation class (Pimpl idiom)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};