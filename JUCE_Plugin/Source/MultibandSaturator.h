#pragma once
#include "EngineBase.h"
#include <vector>
#include <cmath>
#include <random>
#include <array>

class MultibandSaturator : public EngineBase {
public:
    MultibandSaturator();
    ~MultibandSaturator() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Multiband Saturator"; }
    int getNumParameters() const override { return 5; }
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
    
    SmoothParam m_lowDrive;           // Low band drive
    SmoothParam m_midDrive;           // Mid band drive  
    SmoothParam m_highDrive;          // High band drive
    SmoothParam m_saturationType;     // Saturation character selector
    SmoothParam m_harmonicCharacter;  // Even/odd harmonic balance
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // Crossover frequencies
    static constexpr float LOW_CROSSOVER = 200.0f;   // Hz
    static constexpr float HIGH_CROSSOVER = 2000.0f; // Hz
    
    // Linkwitz-Riley crossover filter
    struct LinkwitzRileyFilter {
        // 2nd order butterworth sections (LR4 = 2x butterworth)
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
        float b1 = 0.0f, b2 = 0.0f;
        
        // State variables for two cascaded sections
        float x1_1 = 0.0f, x2_1 = 0.0f;
        float y1_1 = 0.0f, y2_1 = 0.0f;
        float x1_2 = 0.0f, x2_2 = 0.0f;
        float y1_2 = 0.0f, y2_2 = 0.0f;
        
        void calculateCoefficients(float frequency, double sampleRate, bool highpass);
        float process(float input);
        float processWithAging(float input, float aging);
        void reset();
    };
    
    // Per-channel processing state
    struct ChannelState {
        // Crossover filters
        LinkwitzRileyFilter lowpass1;   // For low band
        LinkwitzRileyFilter highpass1;  // For mid+high
        LinkwitzRileyFilter lowpass2;   // For mid band
        LinkwitzRileyFilter highpass2;  // For high band
        
        // Band signals
        float lowBand = 0.0f;
        float midBand = 0.0f;
        float highBand = 0.0f;
        
        // Component aging simulation
        float componentDrift = 0.0f;
        
        // Noise floor for analog realism
        float noiseFloor = 0.0f;
        
        void init(double sampleRate);
        void reset();
        void updateAging(float aging);
    };
    
    std::vector<ChannelState> m_channels;
    
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
    
    std::vector<DCBlocker> m_inputDCBlockers;
    std::vector<DCBlocker> m_outputDCBlockers;
    
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
    
    // Saturation types
    enum SaturationType {
        TUBE = 0,
        TAPE,
        TRANSISTOR,
        DIGITAL
    };
    
    // Saturation functions
    float saturateTube(float input, float drive, float harmonics);
    float saturateTape(float input, float drive, float harmonics);
    float saturateTransistor(float input, float drive, float harmonics);
    float saturateDigital(float input, float drive, float harmonics);
    
    // Apply selected saturation
    float applySaturation(float input, float drive, SaturationType type);
    
    // Harmonic shaping
    float shapeHarmonics(float x, float evenOddBalance);
    
    // Oversampling for cleaner saturation
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
    
    // Enhanced saturation with component modeling
    float processComponentModeling(float input, float drive, SaturationType type, float thermalFactor, float aging);
};