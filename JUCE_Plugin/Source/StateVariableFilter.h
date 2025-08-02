#pragma once
#include "EngineBase.h"
#include <array>
#include <vector>
#include <cmath>

class StateVariableFilter : public EngineBase {
public:
    StateVariableFilter();
    ~StateVariableFilter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "State Variable Filter"; }
    int getNumParameters() const override { return 6; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters with smoothing
    struct SmoothParam {
        float target = 0.5f;
        float current = 0.5f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void reset(float value) {
            target = current = value;
        }
        
        void setSmoothingTime(float timeMs, float sampleRate) {
            float samples = timeMs * 0.001f * sampleRate;
            smoothing = std::exp(-1.0f / samples);
        }
    };
    
    SmoothParam m_frequency;
    SmoothParam m_resonance;
    SmoothParam m_mode;
    SmoothParam m_drive;
    SmoothParam m_nonlinearity;
    SmoothParam m_vintageMode;

    // DSP State
    double m_sampleRate = 44100.0;
    
    // Enhanced state variable filter with multiple topologies
    struct SVFState {
        // Zavalishin's topology
        float ic1eq = 0.0f;  // Integrator 1 state
        float ic2eq = 0.0f;  // Integrator 2 state
        
        // Additional states for enhanced topology
        float v0z = 0.0f;    // Unit delay for zero-delay feedback
        float v1 = 0.0f;     // Internal node 1
        float v2 = 0.0f;     // Internal node 2
        float v3 = 0.0f;     // Internal node 3
        
        // Filter outputs
        float lowpass = 0.0f;
        float bandpass = 0.0f;
        float highpass = 0.0f;
        float notch = 0.0f;
        float allpass = 0.0f;
        float peak = 0.0f;   // Peaking response
        
        // Nonlinear processing states
        float saturationState = 0.0f;
        float asymmetryState = 0.0f;
        
        // Component aging simulation
        float componentDrift = 0.0f;
        
        void reset() {
            ic1eq = ic2eq = 0.0f;
            v0z = v1 = v2 = v3 = 0.0f;
            lowpass = bandpass = highpass = notch = allpass = peak = 0.0f;
            saturationState = asymmetryState = 0.0f;
            componentDrift = 0.0f;
        }
        
        void processZavalishin(float input, float g, float k, float a1, float a2, float a3);
        void processVintage(float input, float g, float k, float nonlinearity);
        float getMorphedOutput(float mode, float vintageAmount);
    };
    
    std::array<SVFState, 2> m_filters;
    
    // Enhanced coefficient calculation
    struct FilterCoefficients {
        float g = 0.0f;      // Cutoff coefficient
        float k = 0.0f;      // Damping coefficient
        float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;  // Zavalishin coeffs
        
        // Vintage modeling
        float thermalDrift = 0.0f;
        float componentTolerance = 1.0f;
        
        void updateCoefficients(float frequency, float resonance, bool vintageMode, 
                              double sampleRate, float componentDrift = 0.0f);
    };
    
    FilterCoefficients m_coeffs;
    
    // Oversampling for high-quality nonlinear processing
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 2;
        
        // Simple anti-aliasing filters
        struct AAFilter {
            float z1 = 0.0f;
            float z2 = 0.0f;
            
            float process(float input, float cutoff) {
                z1 += cutoff * (input - z1);
                z2 += cutoff * (z1 - z2);
                return z2;
            }
            
            void reset() { z1 = z2 = 0.0f; }
        };
        
        AAFilter upsampleFilter;
        AAFilter downsampleFilter;
        
        void reset() {
            upsampleFilter.reset();
            downsampleFilter.reset();
        }
    };
    
    std::array<Oversampler, 2> m_oversamplers;
    
    // DC blocking
    struct DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        
        float process(float input) {
            const float R = 0.995f;
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
    };
    
    std::array<DCBlocker, 2> m_dcBlockers;
    
    // Thermal modeling for vintage behavior
    struct ThermalModel {
        float temperature = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        
        void update(double sampleRate) {
            // Slow thermal drift
            thermalNoise += (((rand() % 1000) / 1000.0f - 0.5f) * 0.001f) / sampleRate;
            thermalNoise = std::max(-0.02f, std::min(0.02f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Processing functions
    float processSample(float input, int channel);
    float processOversampled(float input, int channel);
    
    // Nonlinear functions
    float analogSaturation(float input, float amount, float asymmetry);
    float vintageSaturation(float input, float amount);
    
    // Helper functions
    inline float softClip(float x) {
        return std::tanh(x * 0.7f) / 0.7f;
    }
    
    inline float hardClip(float x, float threshold = 0.95f) {
        return std::max(-threshold, std::min(threshold, x));
    }
    
    // Enhanced frequency to g coefficient conversion
    inline float frequencyToG(float freq, double sampleRate, float drift = 0.0f) {
        float adjustedFreq = freq * (1.0f + drift);
        float wc = 2.0f * M_PI * adjustedFreq / sampleRate;
        return std::tan(wc * 0.5f);
    }
};