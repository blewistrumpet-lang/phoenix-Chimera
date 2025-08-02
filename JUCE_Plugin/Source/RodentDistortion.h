#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class RodentDistortion : public EngineBase {
public:
    RodentDistortion();
    ~RodentDistortion() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Rodent Distortion"; }
    int getNumParameters() const override { return 8; }
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
    
    SmoothParam m_gain;           // 1-50
    SmoothParam m_filter;         // 100-5000 Hz
    SmoothParam m_clipping;       // 0-1 (hard/soft)
    SmoothParam m_tone;           // 1000-10000 Hz
    SmoothParam m_output;         // 0-1
    SmoothParam m_mix;            // 0-1
    SmoothParam m_distortionType; // 0-1 (vintage modes)
    SmoothParam m_presence;       // High-frequency emphasis
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // Enhanced Zero-Delay Feedback Filters
    struct ZDFFilter {
        float s1 = 0.0f, s2 = 0.0f;  // State variables
        float g = 0.0f, k = 0.0f;    // Filter coefficients
        
        void updateCoefficients(float freq, float resonance, double sampleRate) {
            float wc = 2.0f * M_PI * freq;
            g = std::tan(wc * 0.5f / sampleRate);
            k = 2.0f * resonance;
        }
        
        float processHighpass(float input) {
            float v0 = input;
            float v1 = (s1 + g * (v0 - k * s1)) / (1.0f + g * (g + k));
            float v2 = s2 + g * v1;
            s1 = 2.0f * v1 - s1;
            s2 = 2.0f * v2 - s2;
            return v0 - k * v1 - v2;  // High-pass output
        }
        
        float processLowpass(float input) {
            float v0 = input;
            float v1 = (s1 + g * (v0 - k * s1)) / (1.0f + g * (g + k));
            float v2 = s2 + g * v1;
            s1 = 2.0f * v1 - s1;
            s2 = 2.0f * v2 - s2;
            return v2;  // Low-pass output
        }
        
        void reset() { s1 = s2 = 0.0f; }
    };
    
    std::array<ZDFFilter, 2> m_inputFilters;
    std::array<ZDFFilter, 2> m_toneFilters;
    
    // Per-channel presence state
    std::array<float, 2> m_presenceState = {0.0f, 0.0f};
    
    // DC Blocking
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
        
        void reset() { x1 = y1 = 0.0f; }
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
    
    // Oversampling for cleaner distortion
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 4;
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        // Anti-aliasing filters
        struct AAFilter {
            std::array<float, 4> x = {0.0f};
            std::array<float, 4> y = {0.0f};
            
            float process(float input) {
                // 4th order Butterworth at Nyquist/4
                float output = input * 0.0625f + x[0] * 0.25f + x[1] * 0.375f + 
                              x[2] * 0.25f + x[3] * 0.0625f;
                
                // Shift buffer
                x[3] = x[2]; x[2] = x[1]; x[1] = x[0]; x[0] = input;
                return output;
            }
        };
        
        AAFilter upsampleFilter;
        AAFilter downsampleFilter;
        
        void prepare(int blockSize) {
            upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
        }
        
        void upsample(const float* input, float* output, int numSamples);
        void downsample(const float* input, float* output, int numSamples);
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
    
    // Enhanced distortion algorithms
    struct VintageDistortion {
        enum Type {
            RAT_STYLE,      // Classic RAT distortion
            TUBE_SCREAMER,  // Tube Screamer style
            BIG_MUFF,       // Big Muff style
            FUZZ_FACE       // Fuzz Face style
        };
        
        // Asymmetric soft clipping with component modeling
        float processAsymmetric(float x, float drive, float asymmetry) {
            float positive = x > 0.0f ? x : 0.0f;
            float negative = x < 0.0f ? -x : 0.0f;
            
            // Different clipping curves for positive/negative
            positive = std::tanh(positive * drive * (1.0f + asymmetry));
            negative = std::tanh(negative * drive * (1.0f - asymmetry * 0.5f));
            
            return positive - negative;
        }
        
        // Op-amp distortion (RAT-style)
        float processOpAmp(float x, float gain, float threshold) {
            float amplified = x * gain;
            
            // Op-amp saturation modeling
            if (std::abs(amplified) > threshold) {
                float excess = std::abs(amplified) - threshold;
                float compressed = threshold + std::atan(excess * 2.0f) * 0.318f;
                return compressed * (amplified > 0.0f ? 1.0f : -1.0f);
            }
            return amplified;
        }
        
        // Diode clipping (Tube Screamer style)
        float processDiodeClipping(float x, float threshold) {
            const float diodeVf = 0.7f;  // Forward voltage
            
            if (x > diodeVf) {
                float excess = x - diodeVf;
                return diodeVf + excess * std::exp(-excess * threshold);
            } else if (x < -diodeVf) {
                float excess = -x - diodeVf;
                return -(diodeVf + excess * std::exp(-excess * threshold));
            }
            return x;
        }
    };
    
    VintageDistortion m_distortion;
    
    // Enhanced tone shaping with component tolerances
    struct ComponentTolerances {
        float capacitorTolerance = 0.0f;  // ±20% typical
        float resistorTolerance = 0.0f;   // ±5% typical
        std::mt19937 rng;
        
        ComponentTolerances() : rng(std::random_device{}()) {
            std::uniform_real_distribution<float> capDist(-0.2f, 0.2f);
            std::uniform_real_distribution<float> resDist(-0.05f, 0.05f);
            capacitorTolerance = capDist(rng);
            resistorTolerance = resDist(rng);
        }
        
        float adjustFrequency(float freq) const {
            return freq * (1.0f + capacitorTolerance + resistorTolerance);
        }
        
        float adjustGain(float gain) const {
            return gain * (1.0f + resistorTolerance * 0.5f);
        }
    };
    
    ComponentTolerances m_componentTolerances;
    
    // Helper functions
    float softClip(float x);
    float hardClip(float x);
    float processVintageMode(float input, int mode, float drive);
};