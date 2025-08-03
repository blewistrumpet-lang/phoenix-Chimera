#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class ClassicTremolo : public EngineBase {
public:
    ClassicTremolo();
    ~ClassicTremolo() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Classic Tremolo"; }
    int getNumParameters() const override { return 6; }
    juce::String getParameterName(int index) const override;
    
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
    
    SmoothParam m_rate;         // 0.1-20 Hz
    SmoothParam m_depth;        // 0-1
    SmoothParam m_waveform;     // 0-1 (sine to square)
    SmoothParam m_stereoPhase;  // 0-180 degrees
    SmoothParam m_volume;       // 0-1
    SmoothParam m_mix;          // 0-1 (dry/wet mix)
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // Advanced LFO with multiple waveforms
    struct AdvancedLFO {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        float thermalDrift = 0.0f;
        float agingOffset = 0.0f;
        
        float tick(float rate, double sampleRate, float thermalFactor, float aging) {
            // Update phase with thermal and aging effects
            float adjustedRate = rate * thermalFactor * (1.0f - aging * 0.02f);
            phaseIncrement = adjustedRate / sampleRate;
            phase += phaseIncrement;
            
            if (phase >= 1.0f) {
                phase -= 1.0f;
            }
            
            return phase;
        }
    };
    
    std::vector<AdvancedLFO> m_oscillators;
    
    // Helper functions
    float generateWaveform(float phase, float waveformMix);
    float smoothstep(float edge0, float edge1, float x);
    
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
    
    // Multiple tremolo modes
    enum class TremoloMode {
        Classic,
        Vintage,
        Modern,
        Tube
    };
    
    TremoloMode m_currentMode = TremoloMode::Classic;
    
    // Vintage-style tube tremolo modeling
    struct TubeModel {
        float tubeState = 0.0f;
        float bias = 0.5f;
        
        float process(float input, float modulation, float aging) {
            // Simulate tube bias modulation
            float biasModulation = bias + modulation * 0.3f;
            
            // Tube saturation with aging
            float saturation = 1.0f + aging * 0.2f;
            float saturated = std::tanh(input * saturation * biasModulation) / saturation;
            
            // Tube state tracking for bias drift
            tubeState = tubeState * 0.9999f + saturated * 0.0001f;
            
            return saturated;
        }
    };
    
    std::vector<TubeModel> m_tubeModels;
    
    // Enhanced tremolo processing with analog modeling
    float processChannelWithModeling(float input, int channel, float thermalFactor, float aging);
    
    // Oversampling for high-quality tremolo
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