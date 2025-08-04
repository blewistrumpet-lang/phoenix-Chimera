#pragma once

#include "../Source/EngineBase.h"
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <map>
#include <random>
#include <algorithm>

/**
 * KStyleOverdrive - High-quality tube overdrive emulation
 *
 * Features:
 * - Authentic tube-style saturation with even/odd harmonic control
 * - Multi-stage analog circuit modeling
 * - Interactive tone stack based on classic amplifier designs  
 * - 4× oversampling for minimal aliasing
 * - Analog component modeling (capacitor drift, thermal effects)
 * - Phase-preserving filters for transparent tone shaping
 *
 * Technical notes:
 * - Uses State Variable Filters (SVF) for stability
 * - Implements Fender/Marshall-style tone stack interaction
 * - Models 12AX7 tube characteristics with bias and sag
 * - Thread-safe parameter smoothing
 *
 * @param Drive [0,1] - Amount of tube saturation
 * @param Tone [0,1] - EQ balance from dark (0) to bright (1)
 * @param Level [0,1] - Output level with makeup gain
 * @param Mix [0,1] - Dry/wet balance
 */
class KStyleOverdrive : public EngineBase
{
public:
    KStyleOverdrive();
    ~KStyleOverdrive() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;

    juce::String getName() const override           { return "K-Style Overdrive"; }
    int          getNumParameters() const override  { return 4; }
    juce::String getParameterName(int index) const override;

private:
    // Constants
    static constexpr int   OVERSAMPLE_FACTOR           = 4;
    static constexpr int   DEFAULT_BLOCK_SIZE          = 2048;
    static constexpr float DC_BLOCK_FC                 = 10.0f;
    static constexpr float PRE_EMPHASIS_FC             = 720.0f;
    static constexpr float TUBE_BIAS_12AX7             = 0.15f;
    static constexpr float SAFETY_LIMITER_THRESHOLD    = 0.95f;
    static constexpr float SAFETY_LIMITER_KNEE         = 0.7f;
    static constexpr float INPUT_HEADROOM              = 0.7f;

    // Parameter smoothing
    struct SmoothParam
    {
        float target    = 0.5f;
        float current   = 0.5f;
        float smoothing = 0.995f;

        void update()                     { current = target + (current - target) * smoothing; }
        void reset(float value)          { target = current = value; }
        void setSmoothingTime(float ms, double sr)
        {
            float samples   = ms * 0.001f * float(sr);
            smoothing       = std::exp(-2.0f * M_PI / samples);
        }
    };

    SmoothParam m_drive, m_tone, m_level, m_mix;
    float       m_lastTone         = -1.0f;
    double      m_sampleRate       = 44100.0;
    double      m_oversampledRate  = m_sampleRate * OVERSAMPLE_FACTOR;
    int         m_maxBlockSize     = DEFAULT_BLOCK_SIZE;

    // DC blocker
    struct DCBlocker
    {
        float x1 = 0.0f, y1 = 0.0f;
        float R  = 0.995f;

        void setCutoff(float hz, double sr)  { R = std::exp(-2.0f * M_PI * hz / float(sr)); }
        float process(float in)              { float out = in - x1 + R * y1; x1 = in; y1 = out; return out; }
        void reset()                         { x1 = y1 = 0.0f; }
    };

    // State-variable filter
    struct SVFilter
    {
        float g = 0.0f, k = 1.0f, s1 = 0.0f, s2 = 0.0f;
        void setFrequency(float f, double sr)  { g = std::tan(M_PI * f / float(sr)); }
        void setResonance(float q)             { k = 1.0f / q; }
        void reset()                           { s1 = s2 = 0.0f; }

        float processLowpass(float in)
        {
            float gp1 = 1.0f / (1.0f + g * (g + k));
            float v   = (in - s2 - k*s1)*gp1;
            float lp  = v + s1;
            s1        = lp + v;
            s2        = s1 + lp;
            return lp;
        }

        float processHighpass(float in)
        {
            float gp1 = 1.0f / (1.0f + g * (g + k));
            float v   = (in - s2 - k*s1)*gp1;
            float hp  = in - k*v - s2;
            s1        = v + s1;
            s2        = s1 + v;  // Fixed: was s1 + v + s1
            return hp;
        }

        float processBandpass(float in)
        {
            float gp1 = 1.0f / (1.0f + g * (g + k));
            float v   = (in - s2 - k*s1)*gp1;
            float bp  = k*v;
            s1        = v + s1;
            s2        = s1 + v;
            return bp;
        }
    };

    // Filter stage
    struct FilterStage
    {
        SVFilter inputHighpass, preEmphasis, antiAliasUp, antiAliasDown;
        SVFilter toneStackLow, toneStackMid, toneStackHigh;
        DCBlocker dcBlocker;
        float upsampleHistory = 0.0f;
        float toneFeedback    = 0.0f;
        void prepare(double sr);
        void reset();
    };
    std::array<FilterStage,2> m_filterStages;

    // Tube model
    struct TubeStage
    {
        float bias       = TUBE_BIAS_12AX7;
        float saturation = 0.7f, warmth = 0.3f;
        float currentSag = 0.0f, gridCurrent = 0.0f;
        std::mt19937 rng{ std::random_device{}() };
        std::normal_distribution<float> noise{0.0f,1.0f};
        float process(float input, float drive);
        void reset();
    } m_tubeStage;

    // Processing helpers
    void processBlock(float* data, int numSamples, FilterStage& st);
    float processTubeStage(float input, float drive);
    float applyToneStack(float in, FilterStage& st, float tone);
    float softLimit(float in);
    void  updateFilterCoefficients();
};