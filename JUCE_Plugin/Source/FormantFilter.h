#pragma once

#include "../Source/EngineBase.h"
#include <vector>
#include <array>
#include <cmath>

class FormantFilter : public EngineBase {
public:
    FormantFilter();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Formant Filter"; }
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
    
    SmoothParam m_vowelPosition;     // Vowel position (0=A, 0.25=E, 0.5=I, 0.75=O, 1=U)
    SmoothParam m_formantShift;      // Overall formant frequency shift
    SmoothParam m_resonance;         // Formant peak sharpness
    SmoothParam m_morph;             // Morph between adjacent vowels
    SmoothParam m_drive;             // Input drive for analog character
    SmoothParam m_vintageMode;       // Vintage modeling amount
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Formant frequencies for different vowels (F1, F2, F3 in Hz)
    struct FormantData {
        float f1, f2, f3;  // Formant frequencies
        float a1, a2, a3;  // Formant amplitudes
    };
    
    // Vowel formant data (male voice averages)
    static const FormantData VOWEL_A;
    static const FormantData VOWEL_E;
    static const FormantData VOWEL_I;
    static const FormantData VOWEL_O;
    static const FormantData VOWEL_U;
    
    // Enhanced bandpass filters for each formant with analog modeling
    struct FormantBandpass {
        float state1 = 0.0f;  // Filter state 1
        float state2 = 0.0f;  // Filter state 2
        float freq = 1000.0f; // Center frequency
        float q = 2.0f;       // Quality factor
        float gain = 1.0f;    // Gain
        
        // Filter coefficients
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        
        // Analog modeling
        float saturationState = 0.0f;
        float componentDrift = 0.0f;
        float thermalNoise = 0.0f;
        
        // Oversampling for nonlinear processing
        float oversampleState1 = 0.0f;
        float oversampleState2 = 0.0f;
        
        void reset() {
            state1 = state2 = 0.0f;
            saturationState = 0.0f;
            componentDrift = 0.0f;
            thermalNoise = 0.0f;
            oversampleState1 = oversampleState2 = 0.0f;
        }
    };
    
    std::array<std::array<FormantBandpass, 3>, 2> m_formantFilters; // [channel][formant]
    
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
    
    float processSample(float input, int channel);
    FormantData interpolateVowels(float vowelPos, float morph);
    void updateFormantFilters(int channel, const FormantData& formant);
    float processFormantFilter(float input, FormantBandpass& filter, float drive, bool vintageMode);
    void calculateFilterCoefficients(FormantBandpass& filter, float thermalFactor);
    
    // Nonlinear processing
    float analogSaturation(float input, float amount);
    float vintageDistortion(float input, float amount);
    
    // Helper functions
    inline float softClip(float x) {
        return std::tanh(x * 0.7f) / 0.7f;
    }
};