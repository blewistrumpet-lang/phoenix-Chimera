// BitCrusher_Studio.h - Professional studio-quality bit crusher
#pragma once
#include "EngineBase.h"
#include <vector>
#include <memory>
#include <random>
#include <array>

class BitCrusher : public EngineBase {
public:
    BitCrusher();
    ~BitCrusher() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Bit Crusher"; }
    
private:
    // ==================== Parameter Smoothing ====================
    struct SmoothParam {
        float target = 0.0f;
        float current = 0.0f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
            // Denormal protection
            if (std::abs(current - target) < 1e-6f) {
                current = target;
            }
        }
        
        void setImmediate(float value) {
            target = value;
            current = value;
        }
        
        void setSmoothingRate(float rate) {
            smoothing = std::max(0.0f, std::min(0.9999f, rate));
        }
    };
    
    // Parameters
    SmoothParam m_bitDepth;
    SmoothParam m_sampleRateReduction;
    SmoothParam m_aliasing;
    SmoothParam m_jitter;
    SmoothParam m_dcOffset;
    SmoothParam m_gateThreshold;
    SmoothParam m_dither;
    SmoothParam m_mix;
    
    // ==================== Channel State ====================
    struct ChannelState {
        float heldSample = 0.0f;
        float sampleCounter = 0.0f;
        float lastInput = 0.0f;
        float lastOutput = 0.0f;
        float ditherError = 0.0f;      // For noise shaping
        float quantizationError = 0.0f; // For error feedback
        
        // Anti-aliasing filter states (not used in new implementation)
        float lpf1State = 0.0f;
        float lpf2State = 0.0f;
        
        // Removed unnecessary complexity:
        // - dcBlockerState (using separate DCBlocker)
        // - noiseShaping (incorporated into ditherError)
        // - componentDrift (removed aging simulation)
        // - thermalFactor (removed thermal modeling)
    };
    
    std::array<ChannelState, 2> m_channelStates;
    
    // ==================== Random Number Generation ====================
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_distribution{-1.0f, 1.0f};
    
    // ==================== DC Blocking ====================
    struct DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        float R = 0.995f; // Will be set properly in prepareToPlay
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            // Denormal protection
            if (std::abs(y1) < 1e-30f) y1 = 0.0f;
            return output;
        }
    };
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // ==================== Oversampling ====================
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 4; // 4x for quality
        
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        // Butterworth anti-aliasing filters
        struct AAFilter {
            std::array<float, 4> x = {0.0f}; // Input delay line
            std::array<float, 4> y = {0.0f}; // Output delay line
        };
        
        AAFilter upsampleFilter;
        AAFilter downsampleFilter;
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
    
    // Filter coefficients for Butterworth
    struct FilterCoeffs {
        float a0, a1, a2, a3, a4; // Feedforward
        float b1, b2, b3, b4;      // Feedback
    } m_oversampleCoeffs;
    
    // ==================== Processing State ====================
    double m_sampleRate = 44100.0;
    
    // Removed unnecessary members:
    // - ThermalModel (unnecessary complexity)
    // - m_componentAge (unnecessary aging simulation)
    // - m_sampleCount (not needed without aging)
    
    // ==================== Processing Functions ====================
    void prepareOversampler(double sampleRate, int samplesPerBlock);
    void processWithOversampling(float* data, int numSamples, ChannelState& state, int channel);
    void processWithoutOversampling(float* data, int numSamples, ChannelState& state, int channel);
    
    float quantizeProperly(float input, float bits, ChannelState& state);
    float applyProperDither(float input, float bits, ChannelState& state);
    void applyButterworthFilter(float* data, int numSamples, Oversampler::AAFilter& filter);
    
    // Legacy functions (for compatibility)
    float quantize(float input, float bits);
    float quantizeWithAging(float input, float bits, float aging);
    float applyDither(float input, float ditherAmount, ChannelState& state);
    float processDCBlocker(float input, ChannelState& state);
    float softClip(float input);
    float softClipWithAging(float input, float aging);
};