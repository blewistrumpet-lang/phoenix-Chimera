#pragma once

#include "EngineBase.h"
#include <vector>
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <functional>  // For std::function
#include <cstdint>     // For uint32_t in denormal handling

class HarmonicTremolo : public EngineBase {
public:
    HarmonicTremolo();
    ~HarmonicTremolo() = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Harmonic Tremolo Pro"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Professional-grade constants
    static constexpr int OVERSAMPLE_FACTOR = 2;  // Reduced for better performance
    static constexpr int LFO_TABLE_SIZE = 4096;
    static constexpr int BLOCK_SIZE = 64;
    static constexpr int FIR_ORDER = 32;  // Reduced for better performance/quality balance
    static constexpr float PARAMETER_SMOOTH_MS = 10.0f;
    
    // Enable/disable oversampling based on need
    std::atomic<bool> m_oversamplingEnabled{true};
    
    // Core parameters with thread-safe access
    struct SmoothedParameter {
        std::atomic<float> targetValue{0.0f};
        float currentValue = 0.0f;
        float smoothingCoeff = 0.999f;
        
        void setTarget(float value) {
            targetValue.store(value, std::memory_order_relaxed);
        }
        
        float getNextValue() {
            float target = targetValue.load(std::memory_order_relaxed);
            currentValue = target + (currentValue - target) * smoothingCoeff;
            return currentValue;
        }
        
        void setSmoothingTime(float ms, float sampleRate) {
            smoothingCoeff = std::exp(-1.0f / (ms * 0.001f * sampleRate));
        }
        
        void reset(float value) {
            currentValue = value;
            targetValue.store(value, std::memory_order_relaxed);
        }
    };
    
    // Parameters
    SmoothedParameter m_rate;
    SmoothedParameter m_depth;
    SmoothedParameter m_harmonics;
    SmoothedParameter m_stereoPhase;
    
    // DSP state
    double m_sampleRate = 44100.0;
    std::atomic<bool> m_coefficientsNeedUpdate{false};
    
    // Professional LFO implementation
    struct LFOState {
        double phase = 0.0;  // Double precision for phase accuracy
        float previousValue = 0.0f;
        
        // Anti-aliased wavetable
        std::array<float, LFO_TABLE_SIZE> triangleTable;
        
        void initializeTables();
        float process(float rateHz, float sampleRate, float phaseOffset = 0.0f);
    };
    
    std::vector<LFOState> m_lfoState;
    
    // Linkwitz-Riley 4th order crossover
    struct LinkwitzRileyCrossover {
        struct BiquadCoeffs {
            float b0, b1, b2, a1, a2;
        };
        
        struct BiquadState {
            float z1 = 0.0f, z2 = 0.0f;
            float y1 = 0.0f, y2 = 0.0f;
            
            float process(float input, const BiquadCoeffs& coeffs) {
                // Transposed Direct Form II with denormal protection
                float w = input - coeffs.a1 * z1 - coeffs.a2 * z2;
                float output = coeffs.b0 * w + coeffs.b1 * z1 + coeffs.b2 * z2;
                
                // Update states with denormal flush
                z2 = HarmonicTremolo::flushDenormal(z1);
                z1 = HarmonicTremolo::flushDenormal(w);
                
                return output;
            }
            
            void reset() { z1 = z2 = y1 = y2 = 0.0f; }
        };
        
        // Two cascaded biquads for 4th order response
        BiquadState lowpass1, lowpass2;
        BiquadState highpass1, highpass2;
        BiquadCoeffs lowCoeffs, highCoeffs;
        
        void updateCoefficients(float freq, float sampleRate);
        void process(float input, float& low, float& high);
        void reset();
    };
    
    std::vector<LinkwitzRileyCrossover> m_crossover;
    
    // Professional oversampling
    struct OversamplingProcessor {
        static constexpr int FIR_ORDER = 32;  // Match parent constant
        
        struct PolyphaseFilter {
            std::array<float, FIR_ORDER> coefficients;
            std::array<float, FIR_ORDER> delayLine;
            int writeIndex = 0;
            
            void designFilter(float cutoff, bool isUpsampler);
            float process(float input);
            void reset();
        };
        
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        PolyphaseFilter upsampler;
        PolyphaseFilter downsampler;
        
        void initialize(int factor);
        float processWithOversampling(float input, std::function<float(float)> processor);
    };
    
    std::vector<OversamplingProcessor> m_oversampling;
    
    // Tube saturation state
    struct TubeState {
        float dcBlockerState = 0.0f;
        float warmthFilterState = 0.0f;
        
        float process(float input);
        void reset() { dcBlockerState = warmthFilterState = 0.0f; }
    };
    
    std::vector<TubeState> m_tubeState;
    
    // Processing functions
    float processSample(float input, int channel);
    void processBlock(float* channelData, int numSamples, int channel);
    void processBlockSSE(float* channelData, int numSamples, int channel);
    
    // Helper functions
    static inline float flushDenormal(float value) {
        // Fast denormal check and flush
        union { float f; uint32_t i; } u;
        u.f = value;
        
        // Check if exponent is zero (denormal)
        if ((u.i & 0x7F800000) == 0) {
            return 0.0f;
        }
        
        return value;
    }
    
    void updateCrossoverCoefficients();
};