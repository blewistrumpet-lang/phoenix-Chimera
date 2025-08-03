#pragma once
#include "EngineBase.h"
#include <array>
#include <cmath>
#include <random>

class MidSideProcessor : public EngineBase {
public:
    MidSideProcessor();
    ~MidSideProcessor() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Mid/Side Processor"; }
    
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
    SmoothParam m_midGain;
    SmoothParam m_sideGain;
    SmoothParam m_midHighFreq;
    SmoothParam m_midHighGain;
    SmoothParam m_sideLowCut;
    SmoothParam m_sideHighBoost;
    SmoothParam m_stereoWidth;
    SmoothParam m_bassToMid;
    
    // Enhanced filter with boutique features
    struct EnhancedFilter {
        float state1 = 0.0f;
        float state2 = 0.0f;
        float componentDrift = 0.0f;
        
        float processHighpass(float input, float cutoff, float thermalFactor = 1.0f) {
            // Apply thermal compensation
            cutoff *= thermalFactor;
            cutoff = std::max(0.0001f, std::min(cutoff, 0.49f));
            
            float output = input - state1;
            state1 += output * cutoff;
            return output * (1.0f + componentDrift * 0.1f);
        }
        
        float processLowpass(float input, float cutoff, float thermalFactor = 1.0f) {
            cutoff *= thermalFactor;
            cutoff = std::max(0.0001f, std::min(cutoff, 0.49f));
            
            state1 += (input - state1) * cutoff;
            return state1 * (1.0f + componentDrift * 0.05f);
        }
        
        float processShelf(float input, float freq, float gain, float thermalFactor = 1.0f) {
            // High/low shelf filter with thermal compensation
            freq *= thermalFactor;
            freq = std::max(0.001f, std::min(freq, 0.45f));
            
            float g = std::tan(juce::MathConstants<float>::pi * freq);
            float k = gain > 1.0f ? gain : 1.0f / gain;
            
            float v = (input - state2) * g / (1.0f + g);
            float lp = v + state2;
            state2 = lp + v;
            
            float hp = input - lp;
            
            if (gain > 1.0f) {
                return lp + hp / k; // Low shelf boost
            } else {
                return lp * k + hp; // High shelf boost
            }
        }
        
        void updateComponentDrift(double sampleRate) {
            componentDrift += ((rand() % 1000) / 1000000.0f - 0.0005f) / sampleRate;
            componentDrift = std::max(-0.02f, std::min(0.02f, componentDrift));
        }
        
        void reset() {
            state1 = state2 = 0.0f;
            componentDrift = 0.0f;
        }
    };
    
    std::array<EnhancedFilter, 4> m_filters; // Expanded for more processing
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
            temperature = 25.0f + std::sin(phase) * 1.5f; // ±1.5°C variation
            
            // Thermal noise increases with temperature
            float noiseLevel = (temperature - 20.0f) * 0.000003f;
            thermalNoise = ((rand() % 1000) / 1000.0f - 0.5f) * noiseLevel;
            
            // Thermal drift affects filter parameters
            thermalDrift = (temperature - 25.0f) * 0.0006f;
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalDrift;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging simulation
    float m_componentAge = 0.0f; // In hours of operation
    
    void updateComponentAging(double sampleRate) {
        // Age components very slowly (1 hour = 3600 seconds)
        m_componentAge += 1.0f / (sampleRate * 3600.0f);
    }
    
    // Analog modeling functions
    float analogSaturation(float input, float amount);
    float applyVintageWarmth(float input, float thermalFactor);
    
    // Enhanced MS processing
    struct MSProcessor {
        float midEnhancement = 0.0f;
        float sideEnhancement = 0.0f;
        
        void processMidSide(float& mid, float& side, float midGain, float sideGain, 
                           float width, float thermalFactor) {
            // Apply analog-style gain scaling
            mid *= midGain * (1.0f + thermalFactor * 0.01f);
            side *= sideGain * width * (1.0f + thermalFactor * 0.015f);
            
            // Add subtle harmonic enhancement
            if (std::abs(mid) > 0.1f) {
                float harmonic = mid * mid * (mid > 0 ? 1.0f : -1.0f);
                mid += harmonic * 0.02f;
            }
            
            if (std::abs(side) > 0.05f) {
                float harmonic = side * side * (side > 0 ? 1.0f : -1.0f);
                side += harmonic * 0.01f;
            }
        }
    };
    
    MSProcessor m_msProcessor;
};