#pragma once

#include "EngineBase.h"
#include <memory>
#include <map>

namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * SpringReverb - Authentic spring reverb with physical modeling
 * 
 * 10 Professional Parameters:
 * 0: Mix - Dry/Wet balance (0.0-1.0)
 * 1: Tension - Spring tension/character (0.0-1.0)
 * 2: Damping - High frequency damping (0.0-1.0)
 * 3: Decay - Decay time (0.0-1.0 = 0.5s to 5s)
 * 4: Pre-Delay - Pre-delay time (0.0-1.0 = 0-100ms)
 * 5: Drive - Input saturation for authentic spring character (0.0-1.0)
 * 6: Chirp - Amount of spring "boing" character (0.0-1.0)
 * 7: Low Cut - High-pass filter (0.0-1.0 = 20Hz to 500Hz)
 * 8: High Cut - Low-pass filter (0.0-1.0 = 2kHz to 10kHz)
 * 9: Width - Stereo spread (0.0-1.0)
 */
class SpringReverb : public EngineBase {
public:
    SpringReverb();
    ~SpringReverb() override;
    
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