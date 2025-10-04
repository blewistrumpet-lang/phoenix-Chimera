#pragma once

#include "EngineBase.h"
#include <memory>
#include <map>

namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * ShimmerReverb - Ethereal pitch-shifted reverb with octave shimmer
 * 
 * 10 Professional Parameters:
 * 0: Mix - Dry/Wet balance (0.0-1.0)
 * 1: Pitch Shift - Octave shift amount (0.0-1.0 = 0 to +12 semitones)
 * 2: Shimmer - Amount of pitched content (0.0-1.0)
 * 3: Size - Room size/decay time (0.0-1.0)
 * 4: Damping - High frequency damping (0.0-1.0)
 * 5: Feedback - Shimmer tail length (0.0-1.0)
 * 6: Pre-Delay - Pre-delay time (0.0-1.0 = 0-200ms)
 * 7: Modulation - Pitch modulation for chorus effect (0.0-1.0)
 * 8: Low Cut - High-pass filter (0.0-1.0)
 * 9: High Cut - Low-pass filter (0.0-1.0)
 */
class ShimmerReverb : public EngineBase {
public:
    ShimmerReverb();
    ~ShimmerReverb() override;
    
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