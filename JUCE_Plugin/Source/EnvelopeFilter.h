#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <cmath>

class EnvelopeFilter : public EngineBase {
public:
    EnvelopeFilter();
    ~EnvelopeFilter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Envelope Filter"; }
    
private:
    // Smoothed parameters for boutique quality
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
    
    // Parameters with smoothing
    SmoothParam m_sensitivity;
    SmoothParam m_attack; 
    SmoothParam m_release;
    SmoothParam m_range;
    SmoothParam m_resonance;
    SmoothParam m_filterType; // 0=LP, 0.5=BP, 1=HP
    SmoothParam m_direction; // 0=down, 1=up
    SmoothParam m_mix;
    
    // Enhanced State variable filter with boutique features
    struct SVFilter {
        float ic1eq = 0.0f;
        float ic2eq = 0.0f;
        float componentDrift = 0.0f; // Component aging simulation
        
        struct Output {
            float lowpass;
            float bandpass;
            float highpass;
            float notch;
            float allpass;
        };
        
        Output process(float input, float cutoff, float resonance, double sampleRate, 
                      float drive = 0.0f, bool vintageMode = false);
        
        void reset() {
            ic1eq = 0.0f;
            ic2eq = 0.0f;
            componentDrift = 0.0f;
        }
    };
    
    // Enhanced Envelope follower with boutique features
    struct EnvelopeFollower {
        float envelope = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        
        // Enhanced detection modes
        std::array<float, 64> rmsBuffer = {0.0f};
        float rmsSum = 0.0f;
        int rmsIndex = 0;
        
        // Peak hold for punchier envelope
        float peakHold = 0.0f;
        int peakTimer = 0;
        
        // Smoothing filter
        float smoothingState = 0.0f;
        
        void setAttackRelease(float attackMs, float releaseMs, double sampleRate) {
            attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
            releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
        }
        
        float process(float input, float sensitivity = 1.0f, bool useRMS = false);
    };
    
    // Channel state with boutique features
    struct ChannelState {
        SVFilter filter;
        EnvelopeFollower envelope;
        
        // Smoothing
        float currentCutoff = 0.0f;
        float targetCutoff = 0.0f;
        
        // Component aging simulation
        float componentAge = 0.0f;
        
        void reset() {
            filter.reset();
            currentCutoff = 0.0f;
            targetCutoff = 0.0f;
            componentAge = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // DC Blocking for boutique quality
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        static constexpr float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    std::array<DCBlocker, 2> m_dcBlockers;
    
    // Thermal modeling for analog warmth
    struct ThermalModel {
        float temperature = 25.0f; // Room temperature in Celsius
        float thermalNoise = 0.0f;
        float thermalDrift = 0.0f;
        
        void update(double sampleRate) {
            // Slow temperature variations
            static float phase = 0.0f;
            phase += 0.00001f / sampleRate; // Very slow variation
            temperature = 25.0f + std::sin(phase) * 1.0f; // ±1°C variation
            
            // Thermal noise increases with temperature
            float noiseLevel = (temperature - 20.0f) * 0.000003f;
            thermalNoise = ((rand() % 1000) / 1000.0f - 0.5f) * noiseLevel;
            
            // Thermal drift affects filter parameters
            thermalDrift = (temperature - 25.0f) * 0.0005f;
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalDrift;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Helper functions
    float calculateCutoff(float envelope, float thermalFactor = 1.0f);
    float getFilterMix(const SVFilter::Output& filterOut);
    
    // Analog modeling
    float analogSaturation(float input, float amount);
    
    // Helper functions
    inline float softClip(float x) {
        return std::tanh(x * 0.7f) / 0.7f;
    }
};