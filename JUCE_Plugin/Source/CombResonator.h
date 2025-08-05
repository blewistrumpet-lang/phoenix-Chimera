#pragma once
#include "EngineBase.h"
#include "Denorm.hpp"
#include <vector>
#include <array>
#include <memory>
#include <cmath>
#include <algorithm>
#include <atomic>

class CombResonator : public EngineBase {
public:
    CombResonator();
    ~CombResonator() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Comb Resonator"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    
private:
    static constexpr int NUM_COMBS = 8;
    static constexpr int MAX_DELAY_SAMPLES = 96000; // 2 seconds at 48kHz
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 20000.0f;
    
    // Professional comb filter with interpolation and modulation
    class ProfessionalCombFilter {
    public:
        ProfessionalCombFilter();
        void init(int maxDelay);
        void setDelay(float samples);
        void setFeedback(float fb) { feedback = std::clamp(fb, -0.99f, 0.99f); }
        void setFeedforward(float ff) { feedforward = std::clamp(ff, -1.0f, 1.0f); }
        void setDamping(float damp) { damping = std::clamp(damp, 0.0f, 1.0f); }
        float process(float input) noexcept;
        void reset();
        
    private:
        AlignedArray<float, MAX_DELAY_SAMPLES, 64> delayLine;
        float feedback = 0.0f;
        float feedforward = 1.0f;
        float damping = 0.0f;
        float dampingState = 0.0f;
        float delayTime = 0.0f;
        int writePos = 0;
        
        // Hermite interpolation for fractional delays
        inline float interpolate(float frac, float y0, float y1, float y2, float y3) noexcept {
            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }
    };
    
    // Advanced DC blocker with improved stability
    class StableDCBlocker {
    public:
        StableDCBlocker() = default;
        
        float process(float input) noexcept {
            // Two-pole DC blocker for better low frequency response
            const float R = 0.9995f;
            
            // First stage
            float stage1 = input - x1 + R * y1;
            x1 = flushDenorm(input);
            y1 = flushDenorm(stage1);
            
            // Second stage  
            float output = stage1 - x2 + R * y2;
            x2 = flushDenorm(stage1);
            y2 = flushDenorm(output);
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0f;
        }
        
    private:
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
    };
    
    // Channel state with all processing elements
    struct ChannelState {
        std::array<ProfessionalCombFilter, NUM_COMBS> combs;
        StableDCBlocker inputDC;
        StableDCBlocker outputDC;
        
        // Modulation
        float lfoPhase = 0.0f;
        float chorusPhase = 0.0f;
        
        // Harmonic balance
        std::array<float, NUM_COMBS> harmonicGains{};
        
        // Soft clipping state
        float clipState = 0.0f;
        
        void init();
        void reset();
    };
    
    // Smooth parameter handling
    struct SmoothParam {
        std::atomic<float> target{0.0f};
        float current = 0.0f;
        float rate = 0.001f;
        
        void setRate(float r) { rate = std::clamp(r, 0.0001f, 1.0f); }
        void setImmediate(float v) { target = current = v; }
        
        float tick() noexcept {
            float t = target.load(std::memory_order_relaxed);
            if (std::abs(current - t) < 0.0001f) {
                current = t;
            } else {
                current += (t - current) * rate;
            }
            return current;
        }
    };
    
    // Member variables
    double m_sampleRate = 44100.0;
    std::vector<ChannelState> m_channels;
    
    // Parameters
    SmoothParam m_rootFrequency;
    SmoothParam m_resonance;  
    SmoothParam m_harmonicSpread;
    SmoothParam m_decayTime;
    SmoothParam m_damping;
    SmoothParam m_modDepth;
    SmoothParam m_stereoWidth;
    SmoothParam m_mix;
    
    // Harmonic series ratios (can be modified by harmonic spread)
    static constexpr std::array<float, NUM_COMBS> BASE_HARMONIC_RATIOS = {
        1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f
    };
    
    // Helper functions
    static float frequencyToDelay(float freq, double sampleRate) noexcept {
        return static_cast<float>(sampleRate) / std::max(freq, MIN_FREQ);
    }
    
    static float decayTimeToFeedback(float decaySeconds, float delaySamples, double sampleRate) noexcept;
    
    // Soft saturation for musical limiting
    static float softSaturate(float input, float& state) noexcept {
        // Asymmetric soft clipping with state
        const float threshold = 0.7f;
        float x = input;
        
        if (std::abs(x) > threshold) {
            float excess = std::abs(x) - threshold;
            float compression = 1.0f - excess / (1.0f + excess * 2.0f);
            x = (x > 0) ? threshold + excess * compression
                        : -(threshold + excess * compression * 0.9f); // Asymmetry
        }
        
        // Add subtle warmth via state variable
        state = state * 0.995f + x * 0.005f;
        state = flushDenorm(state);
        
        return x + state * 0.02f;
    }
};