// GainUtility_Platinum.h - Professional Gain Control and Metering
// Copyright (c) 2024 - Platinum DSP Series
#pragma once

#include "EngineBase.h"
#include <memory>
#include <array>

/**
 * @brief Professional gain control utility with precision metering
 * 
 * Features:
 * - Precision gain control (-∞ to +24dB)
 * - Peak, RMS, and LUFS metering
 * - True peak detection
 * - Stereo/mid-side gain adjustment
 * - Automatic gain compensation
 * - Phase inversion and channel swap
 * - Gain matching between A/B states
 * - Loudness history tracking
 * 
 * Applications:
 * - Gain staging optimization
 * - Level matching for A/B comparison
 * - Loudness normalization
 * - Mix bus gain control
 * - Mastering level adjustment
 * 
 * Technical Specifications:
 * - Processing: 64-bit internal precision
 * - Metering: True peak with 4x oversampling
 * - Dynamic Range: 144dB
 * - Gain Accuracy: ±0.01dB
 * - CPU: < 0.5% @ 96kHz
 */
class GainUtility_Platinum : public ::EngineBase {
public:
    GainUtility_Platinum();
    ~GainUtility_Platinum() override;
    
    // Disable copying
    GainUtility_Platinum(const GainUtility_Platinum&) = delete;
    GainUtility_Platinum& operator=(const GainUtility_Platinum&) = delete;
    GainUtility_Platinum(GainUtility_Platinum&&) = delete;
    GainUtility_Platinum& operator=(GainUtility_Platinum&&) = delete;
    
    // EngineBase implementation
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    // Parameter info
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Gain Utility Platinum"; }
    
    // Parameter indices
    enum class ParamID : int {
        GAIN = 0,            // Main gain control (-∞ to +24dB)
        GAIN_L = 1,          // Left channel gain (-12 to +12dB)
        GAIN_R = 2,          // Right channel gain (-12 to +12dB)
        GAIN_MID = 3,        // Mid (M) gain (-12 to +12dB)
        GAIN_SIDE = 4,       // Side (S) gain (-12 to +12dB)
        MODE = 5,            // Mode (0=stereo, 0.5=M/S, 1=mono)
        PHASE_L = 6,         // Left channel phase invert
        PHASE_R = 7,         // Right channel phase invert
        CHANNEL_SWAP = 8,    // Swap L/R channels
        AUTO_GAIN = 9        // Auto gain compensation
    };
    
    // Metering modes
    enum class MeterType {
        PEAK,
        RMS,
        LUFS_M,     // Momentary loudness
        LUFS_S,     // Short-term loudness
        LUFS_I      // Integrated loudness
    };
    
    // Extended API
    struct MeteringData {
        float peakL, peakR;
        float rmsL, rmsR;
        float lufsM, lufsS, lufsI;
        float truePeakL, truePeakR;
        float gainReduction;
        float outputGain;
    };
    
    MeteringData getMetering() const;
    
    // A/B state management
    void saveState(int slot);      // Save current state to slot (0 or 1)
    void recallState(int slot);    // Recall state from slot
    void matchGain(int toSlot);    // Match gain to saved state
    
    // Utility functions
    float getIntegratedLoudness() const;  // Get integrated LUFS
    void resetLoudnessMeters();           // Reset LUFS integration
    std::array<float, 2> getPhaseCorrelation() const;  // L/R correlation
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};