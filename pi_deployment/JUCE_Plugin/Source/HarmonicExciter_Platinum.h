#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <memory>
#include <array>

// Forward declarations
namespace juce { template<typename T> class AudioBuffer; }

/**
 * HarmonicExciter_Platinum - Professional multiband harmonic enhancement
 * 
 * Features:
 * - Three-band processing with Linkwitz-Riley crossovers
 * - Tube/transistor harmonic modeling
 * - Phase-coherent processing
 * - Professional oversampling (2x/4x)
 * - Complete denormal protection
 * - Thread-safe parameter automation
 */
class HarmonicExciter_Platinum : public EngineBase {
public:
    HarmonicExciter_Platinum();
    ~HarmonicExciter_Platinum() override;
    
    // Core audio interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Parameter interface
    static constexpr int NumParams = 8;
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Harmonic Exciter Pro"; }
    
    // Strongly-typed parameter IDs with inline documentation
    enum class ParamID : int {
        Frequency = 0,  // Target frequency range: 0.0 = 1kHz, 1.0 = 10kHz (normalized)
        Drive = 1,      // Harmonic generation amount: 0.0 = bypass, 1.0 = maximum (normalized)
        Harmonics = 2,  // Even vs odd harmonic balance: 0.0 = even, 1.0 = odd (normalized)
        Clarity = 3,    // Phase coherence: 0.0 = none, 1.0 = maximum (normalized)
        Warmth = 4,     // Low frequency enhancement: 0.0 = flat, 1.0 = +6dB @ 100Hz (normalized)
        Presence = 5,   // High frequency enhancement: 0.0 = flat, 1.0 = +6dB @ 8kHz (normalized)
        Color = 6,      // Character: 0.0 = tube (even harmonics), 1.0 = transistor (odd harmonics)
        Mix = 7,        // Dry/wet mix: 0.0 = 100% dry, 1.0 = 100% wet (normalized)
        
        // Keep count in sync automatically
        NumParams = 8
    };
    
    // Helper to convert strongly-typed enum to int for map lookups
    static constexpr int toInt(ParamID id) noexcept {
        return static_cast<int>(id);
    }

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};