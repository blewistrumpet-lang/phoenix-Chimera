#pragma once

#include "EngineBase.h"
#include <memory>
#include <map>

namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * PlateReverb - Professional plate reverb using Schroeder-Moorer architecture
 * 
 * 10 Professional Parameters:
 * 0: Mix - Dry/Wet balance (0.0-1.0)
 * 1: Size - Room size/decay time (0.0-1.0 = 0.2s to 10s)
 * 2: Damping - High frequency damping (0.0-1.0)
 * 3: Pre-Delay - Pre-delay time (0.0-1.0 = 0-200ms)
 * 4: Diffusion - Smearing/density of reflections (0.0-1.0)
 * 5: Modulation Rate - LFO rate for vintage wobble (0.0-1.0 = 0.1-5 Hz)
 * 6: Modulation Depth - Amount of pitch modulation (0.0-1.0)
 * 7: Low Cut - High-pass filter (0.0-1.0 = 20Hz to 1kHz)
 * 8: High Cut - Low-pass filter (0.0-1.0 = 1kHz to 20kHz)
 * 9: Width - Stereo spread (0.0-1.0, 0=mono, 1=wide)
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
    // Implementation class (Pimpl idiom for ABI stability)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};