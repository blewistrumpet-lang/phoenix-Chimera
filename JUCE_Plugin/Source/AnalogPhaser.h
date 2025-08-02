#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class AnalogPhaser : public EngineBase {
public:
    AnalogPhaser();
    ~AnalogPhaser() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Analog Phaser"; }
    
private:
    // Parameters with smoothing
    struct SmoothParam {
        float target = 0.0f;
        float current = 0.0f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void setImmediate(float value) {
            target = value;
            current = value;
        }
        
        void setSmoothingRate(float rate) {
            smoothing = rate;
        }
    };
    
    SmoothParam m_rate;
    SmoothParam m_depth;
    SmoothParam m_feedback;
    SmoothParam m_stages;
    SmoothParam m_stereoSpread;
    SmoothParam m_centerFreq;
    SmoothParam m_resonance;
    SmoothParam m_mix;
    
    // Maximum number of all-pass stages
    static constexpr int MAX_STAGES = 8;
    
    // All-pass filter stage
    struct AllPassFilter {
        float state = 0.0f;
        float coefficient = 0.0f;
        
        float process(float input) {
            float output = -input + state;
            state = input + coefficient * output;
            return output;
        }
        
        void setFrequency(float freq, double sampleRate) {
            // All-pass coefficient calculation
            float tanFreq = std::tan(M_PI * freq / sampleRate);
            coefficient = (tanFreq - 1.0f) / (tanFreq + 1.0f);
        }
        
        float processWithAging(float input, float aging) {
            float output = process(input);
            // Add aging effects - slight frequency drift and increased noise
            if (aging > 0.01f) {
                float drift = aging * 0.02f * ((rand() % 1000) / 1000.0f - 0.5f);
                output *= (1.0f + drift);
            }
            return output;
        }
    };
    
    // Channel state for stereo processing
    struct ChannelState {
        std::array<AllPassFilter, MAX_STAGES> allpassFilters;
        float lfoPhase = 0.0f;
        float feedbackSample = 0.0f;
        
        // Smooth parameter changes
        float currentDepth = 0.0f;
        float targetDepth = 0.0f;
        
        // Component aging
        float componentDrift = 0.0f;
        float noiseLevel = 0.0f;
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // DC blocking
    struct DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        const float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
    };
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // Thermal modeling
    struct ThermalModel {
        float temperature = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            // Slow thermal drift
            thermalNoise += (dist(rng) * 0.001f) / sampleRate;
            thermalNoise = std::max(-0.02f, std::min(0.02f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging simulation
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    // LFO for modulation
    float generateLFO(float phase);
    int getActiveStages() const;
    
    // Analog modeling
    float softClip(float input);
    float softClipWithAging(float input, float aging);
    
    // Enhanced LFO with thermal drift
    float generateLFOWithThermal(float phase, float thermalFactor);
    
    // Oversampling for cleaner phase shifting
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 2;
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        // Anti-aliasing filters
        struct AAFilter {
            std::array<float, 4> x = {0.0f};
            std::array<float, 4> y = {0.0f};
            
            float process(float input) {
                // 4th order Butterworth lowpass at Nyquist/2
                const float a0 = 0.0947f, a1 = 0.3789f, a2 = 0.5684f, a3 = 0.3789f, a4 = 0.0947f;
                const float b1 = -0.0000f, b2 = 0.4860f, b3 = -0.0000f, b4 = -0.0177f;
                
                float output = a0 * input + a1 * x[0] + a2 * x[1] + a3 * x[2] + a4 * x[3]
                             - b1 * y[0] - b2 * y[1] - b3 * y[2] - b4 * y[3];
                
                // Shift delay line
                x[3] = x[2]; x[2] = x[1]; x[1] = x[0]; x[0] = input;
                y[3] = y[2]; y[2] = y[1]; y[1] = y[0]; y[0] = output;
                
                return output;
            }
        };
        
        AAFilter upsampleFilter;
        AAFilter downsampleFilter;
        
        void prepare(int blockSize) {
            upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
        }
        
        void upsample(const float* input, float* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                output[i * 2] = upsampleFilter.process(input[i] * 2.0f);
                output[i * 2 + 1] = upsampleFilter.process(0.0f);
            }
        }
        
        void downsample(const float* input, float* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                downsampleFilter.process(input[i * 2]);
                output[i] = downsampleFilter.process(input[i * 2 + 1]) * 0.5f;
            }
        }
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
};