// PhaseAlign_Platinum.h - Professional Multi-band Phase Alignment Tool
// Copyright (c) 2024 - Platinum DSP Series
#pragma once

#include "EngineBase.h"
#include <memory>
#include <array>

/**
 * @brief Professional phase alignment tool for multi-mic and multi-track scenarios
 * 
 * Features:
 * - All-pass filter based phase rotation
 * - Frequency-dependent phase adjustment
 * - Automatic phase correlation detection
 * - Multi-band phase control (4 bands)
 * - Linear phase mode option
 * - Polarity inversion per band
 * - Phase meter and correlation display
 * 
 * Applications:
 * - Multi-mic drum recording alignment
 * - Bass DI and amp alignment
 * - Stereo imaging correction
 * - Mix bus phase optimization
 * - Parallel processing alignment
 * 
 * Technical Specifications:
 * - Processing: Zero-latency all-pass filters
 * - Bands: 4 (Low, Low-Mid, High-Mid, High)
 * - Phase Range: -180° to +180° per band
 * - Correlation: Real-time measurement
 * - CPU: < 2% @ 96kHz
 */
class PhaseAlign_Platinum : public ::EngineBase {
public:
    PhaseAlign_Platinum();
    ~PhaseAlign_Platinum() override;
    
    // Disable copying
    PhaseAlign_Platinum(const PhaseAlign_Platinum&) = delete;
    PhaseAlign_Platinum& operator=(const PhaseAlign_Platinum&) = delete;
    PhaseAlign_Platinum(PhaseAlign_Platinum&&) = delete;
    PhaseAlign_Platinum& operator=(PhaseAlign_Platinum&&) = delete;
    
    // EngineBase implementation
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Parameter info
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Phase Align Platinum"; }
    
    // Parameter indices
    enum class ParamID : int {
        // Global controls
        AUTO_ALIGN = 0,      // Automatic phase alignment (0=off, 1=on)
        REFERENCE = 1,       // Reference channel (0=left, 1=right, 0.5=sum)
        
        // Band controls
        LOW_PHASE = 2,       // Low band phase (-180° to +180°)
        LOW_MID_PHASE = 3,   // Low-mid band phase (-180° to +180°)
        HIGH_MID_PHASE = 4,  // High-mid band phase (-180° to +180°)
        HIGH_PHASE = 5,      // High band phase (-180° to +180°)
        
        // Band frequencies
        LOW_FREQ = 6,        // Low/Low-mid crossover (20Hz-500Hz)
        MID_FREQ = 7,        // Low-mid/High-mid crossover (200Hz-5kHz)
        HIGH_FREQ = 8,       // High-mid/High crossover (1kHz-15kHz)
        
        // Output
        MIX = 9              // Dry/wet mix (0-100%)
    };
    
    // Extended API
    float getPhaseCorrelation() const;  // Get current phase correlation (-1 to +1)
    std::array<float, 4> getBandPhases() const;  // Get current phase shifts per band
    void triggerAutoAlign();  // Manually trigger auto-alignment
    
    // Band solo/mute (for setup)
    void setBandSolo(int band, bool solo);
    void setBandMute(int band, bool mute);
    void setGlobalPolarity(bool invert);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};