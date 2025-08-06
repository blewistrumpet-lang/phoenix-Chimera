#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>

/**
 * Professional Chaos Generator - Platinum Edition
 * 
 * Features:
 * - 6 chaos algorithms (Lorenz, Rossler, Henon, Logistic, Ikeda, Duffing)
 * - Double precision state variables for numerical stability
 * - 4th-order Runge-Kutta integration
 * - Multi-target modulation routing
 * - Phase synchronization with host tempo
 * - Anti-aliased parameter smoothing
 * - Complete denormal protection
 * 
 * @version 1.0.0 - Production Ready
 */
class ChaosGenerator_Platinum : public EngineBase {
public:
    ChaosGenerator_Platinum();
    ~ChaosGenerator_Platinum() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Chaos Generator Platinum"; }
    
    // Parameter indices
    enum ParamID {
        kRate = 0,
        kDepth,
        kType,
        kSmoothing,
        kModTarget,
        kSync,
        kSeed,
        kMix
    };
    
private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};