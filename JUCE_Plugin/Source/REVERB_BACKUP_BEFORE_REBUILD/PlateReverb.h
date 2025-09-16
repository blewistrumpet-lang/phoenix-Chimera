#pragma once

#include "EngineBase.h"
#include <memory>
#include <map>

namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * PlateReverb - High-quality plate reverb using Freeverb algorithm
 * 
 * Parameters:
 * 0: Size (Room Size) - Controls the size of the reverb space
 * 1: Damping - Controls high frequency damping
 * 2: Predelay - Pre-delay time (reserved for future use)
 * 3: Mix - Dry/Wet mix
 */
class PlateReverb : public EngineBase {
public:
    PlateReverb();
    ~PlateReverb() override;
    
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