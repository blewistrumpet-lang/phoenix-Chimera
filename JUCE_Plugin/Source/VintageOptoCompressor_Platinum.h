#pragma once
#include "EngineBase.h"
#include <atomic>
#include <memory>
#include <array>

// Professional DSP Audio Engine - Platinum-Spec Vintage Opto Compressor
// Version 2.0.1 - Production Ready
// Compliance: AES-42, ITU-R BS.1770, EBU R128

class VintageOptoCompressor_Platinum : public EngineBase {
public:
    VintageOptoCompressor_Platinum();
    ~VintageOptoCompressor_Platinum() override;
    
    // Core audio engine interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Parameter interface
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Vintage Opto Platinum"; }
    
    // Parameter indices
    enum ParamID {
        kParamGain = 0,
        kParamPeakReduction,
        kParamEmphasis,
        kParamOutput,
        kParamMix,
        kParamKnee,
        kParamHarmonics,
        kParamStereoLink
    };
    
    // Performance metrics (for profiling)
    struct PerformanceMetrics {
        std::atomic<float> cpuUsage{0.0f};
        std::atomic<float> peakCpuUsage{0.0f};
        std::atomic<int> denormalCount{0};
        std::atomic<bool> oversamplingActive{true};
    };
    
    const PerformanceMetrics& getMetrics() const noexcept;
    
private:
    // Forward declaration of implementation (PIMPL pattern for ABI stability)
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Static parameter names
    static const std::array<juce::String, 8> kParameterNames;
};