// StereoChorus_Reference.h
// Reference implementation demonstrating studio-grade DSP practices
// Uses DspEngineUtilities for consistent guardrails across all engines

#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <array>
#include <memory>

class StereoChorus_Reference : public EngineBase {
public:
    StereoChorus_Reference();
    ~StereoChorus_Reference() override;
    
    // ========== Core EngineBase API ==========
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 7; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Stereo Chorus (Reference)"; }
    
    // ========== Extended API Implementation ==========
    int getLatencySamples() const noexcept override;
    void setMaxBlockSizeHint(int maxBlockSize) override;
    void setTransportInfo(const TransportInfo& info) override;
    void setBypassed(bool shouldBypass) override;
    bool supportsFeature(Feature f) const noexcept override;
    
private:
    // Parameter IDs
    enum ParamID {
        kRate = 0,      // LFO rate in Hz (0.1 - 10 Hz)
        kDepth,         // Modulation depth in ms (0 - 20ms)
        kDelay,         // Base delay time in ms (5 - 50ms)
        kFeedback,      // Feedback amount (-0.95 to 0.95)
        kWidth,         // Stereo width (0 = mono, 1 = wide)
        kMix,           // Dry/wet mix (0 = dry, 1 = wet)
        kSync           // Tempo sync on/off
    };
    
    // Internal implementation using PIMPL for ABI stability
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};