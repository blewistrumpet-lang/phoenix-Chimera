// PhasedVocoder.h - Platinum-spec header (minimal interface)
#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>

// Forward declarations only
namespace juce { 
    template<typename T> class AudioBuffer; 
    class String;
}

class PhasedVocoder final : public EngineBase {
public:
    PhasedVocoder();
    ~PhasedVocoder() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 10; } // Added attack/release
    juce::String getParameterName(int index) const override;
    juce::String getName() const override;
    
    // Parameter IDs for type safety
    enum class ParamID : int {
        TimeStretch = 0,
        PitchShift = 1,
        SpectralSmear = 2,
        TransientPreserve = 3,
        PhaseReset = 4,
        SpectralGate = 5,
        Mix = 6,
        Freeze = 7,
        TransientAttack = 8,
        TransientRelease = 9
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};