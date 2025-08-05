#pragma once
#include "EngineBase.h"
#include <memory>
#include <map>

// Forward declarations
namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * Platinum-spec Vocal Formant Filter
 * - Thread-safe with lock-free parameter updates
 * - Full denormal protection
 * - SIMD-optimized processing
 * - Oversampled saturation stages
 */
class VocalFormantFilter : public EngineBase {
public:
    VocalFormantFilter();
    ~VocalFormantFilter() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Vocal Formant Filter"; }
    
    // Parameter indices
    enum ParamID {
        kVowel1 = 0,
        kVowel2,
        kMorph,
        kResonance,
        kBrightness,
        kModRate,
        kModDepth,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};