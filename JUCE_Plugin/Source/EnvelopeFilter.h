// EnvelopeFilter.h - Platinum-spec header (minimal interface)
#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>

// Forward declarations
namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

class EnvelopeFilter final : public EngineBase {
public:
    EnvelopeFilter();
    ~EnvelopeFilter() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Envelope Filter"; }
    
    // Parameter IDs
    enum class ParamID : int {
        Sensitivity = 0,
        Attack = 1,
        Release = 2,
        Range = 3,
        Resonance = 4,
        FilterType = 5,
        Direction = 6,
        Mix = 7
    };
    
    // Filter modes
    enum class FilterMode {
        Lowpass,
        Bandpass,
        Highpass,
        Notch,
        Allpass
    };
    
    // Quality settings
    void setAnalogMode(bool enable);
    void setOversamplingFactor(int factor); // 1, 2, 4, 8
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};