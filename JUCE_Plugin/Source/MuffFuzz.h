#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class MuffFuzz : public EngineBase {
public:
    MuffFuzz();
    ~MuffFuzz() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Muff Fuzz"; }
    int getNumParameters() const override { return 7; }
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
    
    SmoothParam m_sustain;       // 0-1 (fuzz intensity)
    SmoothParam m_tone;          // 0-1 (tone control)
    SmoothParam m_volume;        // 0-1 (output level)
    SmoothParam m_gate;          // 0-1 (noise gate)
    SmoothParam m_mids;          // 0-1 (midrange scoop)
    SmoothParam m_fuzzType;      // 0-1 (modern fuzz variations)
    SmoothParam m_mix;           // 0-1 (dry/wet mix)
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // Enhanced multi-stage filter architecture
    struct ModernBiquadFilter {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;
        
        float process(float input) {
            double output = b0 * input + b1 * z1 + b2 * z2 - a1 * z1 - a2 * z2;
            z2 = z1;
            z1 = output;
            return static_cast<float>(output);
        }
        
        void setLowShelf(double freq, double gain, double Q, double sampleRate);
        void setHighShelf(double freq, double gain, double Q, double sampleRate);
        void setBandpass(double freq, double Q, double sampleRate);
        void setNotch(double freq, double Q, double sampleRate);
        
        void reset() { z1 = z2 = 0.0; }
    };
    
    struct ChannelState {
        ModernBiquadFilter inputHighpass;    // Remove DC and low mud
        ModernBiquadFilter inputLowShelf;    // Shape low-end character
        ModernBiquadFilter midScoop;         // Characteristic mid scoop
        ModernBiquadFilter toneFilter;       // Final tone shaping
        ModernBiquadFilter presenceFilter;   // High-frequency presence
        
        // Enhanced envelope follower
        float envelope = 0.0f;
        float peakEnvelope = 0.0f;
        float rmsEnvelope = 0.0f;
        
        // Component aging effects
        float componentDrift = 0.0f;
        float thermalFactor = 1.0f;
        
        void reset() {
            inputHighpass.reset();
            inputLowShelf.reset();
            midScoop.reset();
            toneFilter.reset();
            presenceFilter.reset();
            envelope = peakEnvelope = rmsEnvelope = 0.0f;
            componentDrift = thermalFactor = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    
    // DC Blocking
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        const float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // Thermal modeling
    struct ThermalModel {
        float temperature = 25.0f;
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            thermalNoise += (dist(rng) * 0.0008f) / sampleRate;
            thermalNoise = std::max(-0.025f, std::min(0.025f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    // Random generator for component variations
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_distribution{-1.0f, 1.0f};
    
    // Oversampling for pristine distortion
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 4;
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        struct AAFilter {
            std::array<float, 6> x = {0.0f};
            std::array<float, 6> y = {0.0f};
            
            float process(float input) {
                // 6th order Butterworth for better anti-aliasing
                float output = input * 0.0156f;
                for (int i = 0; i < 6; ++i) {
                    output += x[i] * (0.09375f - i * 0.01562f);
                }
                
                // Shift buffers
                for (int i = 5; i > 0; --i) {
                    x[i] = x[i-1];
                }
                x[0] = input;
                return output;
            }
        };
        
        AAFilter upsampleFilter, downsampleFilter;
        
        void prepare(int blockSize) {
            upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
        }
        
        void upsample(const float* input, float* output, int numSamples);
        void downsample(const float* input, float* output, int numSamples);
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
    
    // Modern fuzz variations
    struct ModernFuzzEngine {
        enum FuzzType {
            SILICON_TRANSISTOR,  // Clean, compressed fuzz
            GERMANIUM_VINTAGE,   // Warm, musical fuzz
            DIGITAL_MODERN,      // Precise, controlled
            HYBRID_TUBE         // Tube-like warmth
        };
        
        // Advanced diode modeling
        float processModernDiodeClipping(float x, float threshold, FuzzType type) {
            switch (type) {
                case SILICON_TRANSISTOR:
                    return processSiliconFuzz(x, threshold);
                case GERMANIUM_VINTAGE:
                    return processGermaniumFuzz(x, threshold);
                case DIGITAL_MODERN:
                    return processDigitalFuzz(x, threshold);
                case HYBRID_TUBE:
                    return processHybridFuzz(x, threshold);
                default:
                    return x;
            }
        }
        
        private:
            float processSiliconFuzz(float x, float threshold);
            float processGermaniumFuzz(float x, float threshold);
            float processDigitalFuzz(float x, float threshold);
            float processHybridFuzz(float x, float threshold);
    };
    
    ModernFuzzEngine m_fuzzEngine;
    
    // Component tolerances for vintage behavior
    struct ComponentTolerances {
        float capacitorDrift = 0.0f;
        float resistorDrift = 0.0f;
        float transistorBeta = 1.0f;
        
        ComponentTolerances() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> capDist(-0.15f, 0.15f);
            std::uniform_real_distribution<float> resDist(-0.03f, 0.03f);
            std::uniform_real_distribution<float> betaDist(0.8f, 1.2f);
            
            capacitorDrift = capDist(gen);
            resistorDrift = resDist(gen);
            transistorBeta = betaDist(gen);
        }
        
        float adjustFrequency(float freq) const {
            return freq * (1.0f + capacitorDrift + resistorDrift);
        }
        
        float adjustGain(float gain) const {
            return gain * transistorBeta * (1.0f + resistorDrift);
        }
    };
    
    ComponentTolerances m_componentTolerances;
    
    // Helper functions
    float processDiodeClipping(float x, float threshold);
    float processGate(float input, float& envelope, float threshold);
    float processModernFuzz(float input, int fuzzType, float intensity);
};