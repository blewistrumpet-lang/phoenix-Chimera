#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <memory>
#include <juce_core/juce_core.h>

// Forward declarations
namespace juce { 
    template<typename T> class AudioBuffer; 
}

/**
 * Platinum-spec wave folder with anti-aliasing
 * 
 * Features:
 * - Real-time safe with zero allocations
 * - 4x oversampling for alias-free folding
 * - Lock-free parameter updates
 * - Comprehensive denormal prevention
 * - < 1ms latency @ 48kHz
 * - > 120dB dynamic range
 */
class WaveFolder final : public EngineBase {
public:
    WaveFolder();
    ~WaveFolder() override;
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Wave Folder"; }
    
    // Parameter indices
    enum ParamIndex {
        kFoldAmount = 0,
        kAsymmetry = 1,
        kDCOffset = 2,
        kPreGain = 3,
        kPostGain = 4,
        kSmoothing = 5,
        kHarmonics = 6,
        kMix = 7
    };
    
    // Quality metrics access (thread-safe)
    float getCPUUsage() const;
    float getDynamicRangeDB() const;
    std::string getQualityReport() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};