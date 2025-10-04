// PhasedVocoder_Reference.h - Team C: Reference Implementation
// Simplified, working phase vocoder implementation for correctness validation
// Based on standard STFT phase vocoder algorithm with proven parameters

#pragma once
#include "EngineBase.h"
#include <memory>
#include <vector>
#include <complex>

// Forward declarations only
namespace juce { 
    template<typename T> class AudioBuffer; 
    class String;
    namespace dsp { class FFT; }
}

class PhasedVocoder_Reference final : public EngineBase {
public:
    PhasedVocoder_Reference();
    ~PhasedVocoder_Reference() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 3; }
    juce::String getParameterName(int index) const override;
    juce::String getParameterDisplayString(int index, float value) const;
    juce::String getName() const override;
    
    // Parameter IDs for reference implementation (minimal set)
    enum class ParamID : int {
        TimeStretch = 0,  // Time stretching factor
        PitchShift = 1,   // Pitch shifting in semitones
        Mix = 2           // Dry/wet mix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};