// MonoMaker_Platinum.h - Professional Frequency-Selective Mono Conversion
// Copyright (c) 2024 - Platinum DSP Series
#pragma once

#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <memory>
#include <array>

/**
 * @brief Professional mono-making tool for bass management and mix translation
 * 
 * Features:
 * - Frequency-selective mono conversion
 * - Phase-coherent processing
 * - Stereo width preservation above cutoff
 * - Mid/Side processing mode
 * - Elliptical filter for vinyl mastering
 * - Phase correlation monitoring
 * - True mono compatibility check
 * 
 * Applications:
 * - Bass frequency management
 * - Vinyl mastering preparation
 * - Club/PA system optimization
 * - Mono compatibility enhancement
 * - Phase issue correction
 * 
 * Technical Specifications:
 * - Processing: Linear-phase crossover option
 * - Frequency Range: 20Hz to 1kHz
 * - Phase Accuracy: ±0.5°
 * - Latency: 0ms (minimum phase) or 64 samples (linear phase)
 * - CPU: < 1% @ 96kHz
 */
class MonoMaker_Platinum : public ::EngineBase {
public:
    MonoMaker_Platinum();
    ~MonoMaker_Platinum() override;
    
    // Disable copying
    MonoMaker_Platinum(const MonoMaker_Platinum&) = delete;
    MonoMaker_Platinum& operator=(const MonoMaker_Platinum&) = delete;
    MonoMaker_Platinum(MonoMaker_Platinum&&) = delete;
    MonoMaker_Platinum& operator=(MonoMaker_Platinum&&) = delete;
    
    // EngineBase implementation
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Parameter info
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Mono Maker Platinum"; }
    
    // Parameter indices
    enum class ParamID : int {
        FREQUENCY = 0,      // Mono below this frequency (20Hz-1kHz)
        SLOPE = 1,          // Filter slope (6-48 dB/oct)
        MODE = 2,           // Processing mode (0=standard, 0.5=elliptical, 1=M/S)
        BASS_MONO = 3,      // Bass mono amount (0-100%)
        PRESERVE_PHASE = 4, // Phase preservation (0=minimum, 1=linear)
        DC_FILTER = 5,      // DC blocking filter (0=off, 1=on)
        WIDTH_ABOVE = 6,    // Stereo width above cutoff (0-200%)
        OUTPUT_GAIN = 7     // Output gain compensation (-6 to +6 dB)
    };
    
    // Processing modes
    enum class ProcessingMode {
        STANDARD = 0,    // Simple mono below frequency
        ELLIPTICAL,      // Elliptical EQ for vinyl
        MID_SIDE         // M/S based processing
    };
    
    // Extended API
    float getPhaseCorrelation() const;      // Current phase correlation
    float getMonoCompatibility() const;     // Mono compatibility score
    std::pair<float, float> getStereoWidth() const;  // Width below/above cutoff
    
    // Monitoring
    bool isProcessing() const;
    float getCurrentCutoff() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};