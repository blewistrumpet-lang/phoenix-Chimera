#pragma once

#include "../Source/EngineBase.h"
#include <array>
#include <vector>
#include <cmath>

class LadderFilter : public EngineBase {
public:
    LadderFilter();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Ladder Filter"; }
    int getNumParameters() const override { return 7; }
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
    
    SmoothParam m_cutoffFreq;
    SmoothParam m_resonance;
    SmoothParam m_drive;
    SmoothParam m_filterType;
    SmoothParam m_asymmetry;
    SmoothParam m_vintageMode;
    SmoothParam m_mix;
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Enhanced Moog ladder filter with accurate modeling
    struct LadderStage {
        // State variables for Huovilainen's improved model
        float state = 0.0f;          // Main integrator state
        float delay = 0.0f;          // Unit delay for feedback calculation
        
        // Nonlinear elements
        float saturation = 0.0f;     // Transistor saturation state
        float thermal = 0.0f;        // Thermal voltage variation
        
        // Component modeling
        float capacitorLeakage = 0.0f;  // Capacitor leakage current
        float transistorBeta = 100.0f;   // Transistor current gain
        
        void reset() {
            state = delay = saturation = thermal = capacitorLeakage = 0.0f;
            transistorBeta = 100.0f;
        }
    };
    
    // Per-channel filter state
    struct ChannelState {
        std::array<LadderStage, 4> stages;
        
        // Enhanced feedback calculation
        float feedbackState = 0.0f;
        float feedbackDelay = 0.0f;
        
        // Input stage
        float inputSaturation = 0.0f;
        float inputDCBlock = 0.0f;
        
        // Compensation filter (for self-oscillation)
        float compFilterState = 0.0f;
        
        // Vintage characteristics
        float componentDrift = 0.0f;
        float temperatureCoeff = 1.0f;
        
        // Oversampling state
        struct Oversampler {
            static constexpr int OVERSAMPLE_FACTOR = 4;
            
            // Anti-aliasing filters (Butterworth 8th order equivalent)
            struct AAFilter {
                std::array<float, 4> x = {0.0f};
                std::array<float, 4> y = {0.0f};
                
                float process(float input, const float* coeffs);
            };
            
            AAFilter upsampleFilter;
            AAFilter downsampleFilter;
            
            void reset() {
                upsampleFilter = AAFilter{};
                downsampleFilter = AAFilter{};
            }
        };
        
        Oversampler oversampler;
        
        void prepare() {
            for (auto& stage : stages) {
                stage.reset();
            }
            feedbackState = feedbackDelay = 0.0f;
            inputSaturation = inputDCBlock = 0.0f;
            compFilterState = 0.0f;
            componentDrift = 0.0f;
            temperatureCoeff = 1.0f;
            oversampler.reset();
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    
    // Filter coefficients (Huovilainen's model)
    struct FilterCoefficients {
        float g = 0.0f;          // Cutoff coefficient
        float k = 0.0f;          // Feedback coefficient
        float alpha = 0.0f;      // Feedback compensation
        
        // Nonlinear coefficient tables
        std::array<float, 256> saturationLUT;
        std::array<float, 256> resonanceLUT;
        
        void updateCoefficients(float cutoff, float resonance, float asymmetry, 
                              bool vintageMode, double sampleRate);
        void generateLUTs();
    };
    
    FilterCoefficients m_coeffs;
    
    // Thermal modeling
    struct ThermalModel {
        float ambientTemp = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        float tempCoefficient = 0.003f; // 0.3% per degree
        
        void update(double sampleRate) {
            // Slow thermal drift
            thermalNoise += (((rand() % 1000) / 1000.0f - 0.5f) * 0.001f) / sampleRate;
            thermalNoise = std::max(-0.05f, std::min(0.05f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + (ambientTemp - 25.0f) * tempCoefficient + thermalNoise;
        }
    };
    
    ThermalModel m_thermalModel;
    
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
    
    // Processing functions
    float processSample(float input, int channel);
    float processLadderCore(float input, int channel);
    float processLadderStage(float input, LadderStage& stage, float g, float drive, bool isFirst = false);
    float calculateFeedback(const ChannelState& state, float k, float alpha);
    float processOversampled(float input, int channel);
    
    // Nonlinear functions
    float transistorSaturation(float input, float drive, float asymmetry);
    float vintageSaturation(float input, float drive);
    
    // Filter response calculation for different types
    float calculateFilterResponse(const ChannelState& state, float input, float filterType);
    
    // Utility functions
    inline float softClip(float x) {
        return std::tanh(x * 0.8f) / 0.8f;
    }
    
    inline float asymmetricClip(float x, float asymmetry) {
        if (x > 0) {
            return std::tanh(x * (1.0f + asymmetry)) / (1.0f + asymmetry);
        } else {
            return std::tanh(x * (1.0f - asymmetry * 0.5f)) / (1.0f - asymmetry * 0.5f);
        }
    }
};