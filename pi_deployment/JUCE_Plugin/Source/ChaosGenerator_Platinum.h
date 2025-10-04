#pragma once
#include "EngineBase.h"
#include <memory>
#include <array>
#include <atomic>
#include <cmath>

// Forward declare JUCE bits lightly
namespace juce {
    template<typename T> class AudioBuffer;
    class String;
}

/**
 * Professional Chaos Generator - Platinum Edition (hardened)
 *
 * - 6 chaos algorithms (Lorenz, Rossler, Henon, Logistic, Ikeda, Duffing)
 * - Double precision state + clamped RK4 / semi-implicit steps
 * - Iteration/step caps (per sample) to prevent hangs
 * - Full NaN/Inf/denormal protection
 * - Lock-free parameter smoothing (no std::map on audio thread)
 * - Thread-safe, allocation-free process() path
 */
class ChaosGenerator_Platinum : public EngineBase {
public:
    ChaosGenerator_Platinum();
    ~ChaosGenerator_Platinum() override;

    // EngineBase
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Chaos Generator Platinum"; }

    // Parameter indices (preserved exactly)
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