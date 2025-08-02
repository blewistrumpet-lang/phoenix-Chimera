#pragma once
#include "EngineBase.h"
#include <vector>
#include <memory>
#include <array>
#include <random>

class WaveFolder : public EngineBase {
public:
    WaveFolder();
    ~WaveFolder() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Wave Folder"; }
    
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
    
    SmoothParam m_foldAmount;
    SmoothParam m_asymmetry;
    SmoothParam m_dcOffset;
    SmoothParam m_preGain;
    SmoothParam m_postGain;
    SmoothParam m_smoothing;
    SmoothParam m_harmonics;
    SmoothParam m_mix;
    
    struct ChannelState {
        float lastInput = 0.0f;
        float lastOutput = 0.0f;
        float dcBlockerState = 0.0f;
        float smoothState = 0.0f;
        
        // Harmonic emphasis filters
        float harmonicFilter1 = 0.0f;
        float harmonicFilter2 = 0.0f;
        float harmonicFilter3 = 0.0f;
        
        // Component aging
        float componentDrift = 0.0f;
        float thermalFactor = 1.0f;
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
    
    // Random generator for aging effects
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_distribution{-1.0f, 1.0f};
    
    // Oversampling for cleaner wave folding
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 4; // Higher oversampling for wave folding
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        // Anti-aliasing filters
        struct AAFilter {
            std::array<float, 4> x = {0.0f};
            std::array<float, 4> y = {0.0f};
            
            float process(float input) {
                // 4th order Butterworth lowpass at Nyquist/4 for 4x oversampling
                const float a0 = 0.0067f, a1 = 0.0268f, a2 = 0.0402f, a3 = 0.0268f, a4 = 0.0067f;
                const float b1 = -2.3741f, b2 = 2.3139f, b3 = -1.0547f, b4 = 0.1874f;
                
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
                output[i * OVERSAMPLE_FACTOR] = upsampleFilter.process(input[i] * OVERSAMPLE_FACTOR);
                for (int j = 1; j < OVERSAMPLE_FACTOR; ++j) {
                    output[i * OVERSAMPLE_FACTOR + j] = upsampleFilter.process(0.0f);
                }
            }
        }
        
        void downsample(const float* input, float* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                float sum = 0.0f;
                for (int j = 0; j < OVERSAMPLE_FACTOR; ++j) {
                    sum += downsampleFilter.process(input[i * OVERSAMPLE_FACTOR + j]);
                }
                output[i] = sum / OVERSAMPLE_FACTOR;
            }
        }
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
    
    float processWavefolding(float input, float amount, float asymmetry);
    float processWavefoldingWithAging(float input, float amount, float asymmetry, float aging);
    float smoothTransition(float input, float lastInput, float smoothing);
    float processHarmonicEmphasis(float input, ChannelState& state);
    float processHarmonicEmphasisWithAging(float input, ChannelState& state, float aging);
    float processDCBlocker(float input, ChannelState& state);
    float softClip(float input);
    float softClipWithAging(float input, float aging);
};