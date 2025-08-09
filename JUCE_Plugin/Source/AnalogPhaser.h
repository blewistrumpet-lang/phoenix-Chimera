#pragma once
#include "EngineBase.h"
#include <memory>
#include <juce_core/juce_core.h>

// Forward declarations
namespace juce {
    template<typename T> class AudioBuffer;
}

/**
 * Analog Phaser (RT-safe, stable)
 *
 * - TPT one-pole all-pass ladder (2/4/6/8 stages)
 * - No per-sample dynamic allocation, no locks, FTZ/DAZ
 * - Coefficients clamped to keep all-pass stable
 * - Feedback hard-capped and soft-limited
 * - Parameter smoothing throughout
 */
class AnalogPhaser final : public EngineBase {
public:
    AnalogPhaser();
    ~AnalogPhaser() override;

    // EngineBase
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Analog Phaser"; }

    // Parameter indices (unchanged)
    enum ParamIndex {
        kRate = 0,
        kDepth = 1,
        kFeedback = 2,
        kStages = 3,
        kStereoSpread = 4,
        kCenterFreq = 5,
        kResonance = 6,
        kMix = 7
    };

    // Optional quality metrics (if your project provides them)
    float getCPUUsage() const;
    float getDynamicRangeDB() const;
    std::string getQualityReport() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};