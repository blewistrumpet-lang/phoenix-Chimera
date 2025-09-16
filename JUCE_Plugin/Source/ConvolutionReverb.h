#pragma once

#include "EngineBase.h"
#include "../JuceLibraryCode/JuceHeader.h"
#include <memory>
#include <map>

/**
 * ConvolutionReverb - Professional FFT-based convolution reverb
 * 
 * 10 Professional Parameters:
 * 0: Mix - Dry/Wet balance (0.0-1.0)
 * 1: IR Select - Choose impulse response (0.0-1.0 = 4 IRs)
 * 2: Size - IR playback size/length (0.0-1.0)
 * 3: Pre-Delay - Pre-delay time (0.0-1.0 = 0-200ms)
 * 4: Damping - High frequency damping (0.0-1.0)
 * 5: Reverse - Reverse IR for special effects (0.0-1.0, >0.5 = reversed)
 * 6: Early/Late - Balance of early vs late reflections (0.0-1.0)
 * 7: Low Cut - High-pass filter (0.0-1.0 = 20Hz to 1kHz)
 * 8: High Cut - Low-pass filter (0.0-1.0 = 1kHz to 20kHz)
 * 9: Width - Stereo spread (0.0-1.0)
 * 
 * IRs included:
 * - Concert Hall (large natural space)
 * - EMT 250 Plate (vintage digital plate)
 * - Stairwell (characterful real space)
 * - Cloud Chamber (abstract ambient texture)
 */
class ConvolutionReverb : public EngineBase {
public:
    ConvolutionReverb();
    ~ConvolutionReverb() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Parameter information
    int getNumParameters() const override;
    juce::String getParameterName(int index) const override;
    juce::String getName() const override;
    
    // Report latency for PDC
    int getLatencySamples() const noexcept override;
    
private:
    // Implementation class
    class Impl;
    std::unique_ptr<Impl> pImpl;
};