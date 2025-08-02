#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <memory>

class VocalFormantFilter : public EngineBase {
public:
    VocalFormantFilter();
    ~VocalFormantFilter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Vocal Formant Filter"; }
    
private:
    // Vowel formant frequencies (Hz) for different vowels
    struct FormantSet {
        float f1, f2, f3;  // First three formants
        float q1, q2, q3;  // Q factors
    };
    
    // Classic vowel formants
    static const FormantSet vowelFormants[5];  // A, E, I, O, U
    
    // Parameters
    float m_vowel1 = 0.0f;  // First vowel (0-4)
    float m_vowel2 = 2.0f;  // Second vowel (0-4)
    float m_morphAmount = 0.0f;  // Morphing between vowels
    float m_resonance = 0.5f;
    float m_brightness = 0.5f;
    float m_modRate = 0.0f;
    float m_modDepth = 0.0f;
    float m_mix = 1.0f;
    
    // Formant filter structure
    struct FormantFilter {
        // State variable filter for each formant
        float state1 = 0.0f;
        float state2 = 0.0f;
        
        float process(float input, float freq, float q, double sampleRate) {
            float w = 2.0f * std::sin(M_PI * freq / sampleRate);
            float q_inv = 1.0f / q;
            
            float bandpass = state2 * w + state1;
            float highpass = input - bandpass * q_inv - state1;
            state1 += state2 * w;
            state2 = highpass;
            
            return bandpass;
        }
    };
    
    struct ChannelState {
        std::array<FormantFilter, 3> formantFilters;  // 3 formants per channel
        float modulationPhase = 0.0f;
        
        // Envelope follower for dynamic response
        float envelope = 0.0f;
        
        // High shelf for brightness control
        float highShelfState = 0.0f;
    };
    
    std::vector<ChannelState> m_channelStates;
    double m_sampleRate = 44100.0;
    
    FormantSet interpolateFormants(float vowelIndex);
    float processHighShelf(float input, float& state, float frequency, float gain, float thermalFactor = 1.0f);
    
    // Nonlinear processing
    float analogSaturation(float input, float amount);
    float vintageTubeDistortion(float input, float amount);
    
    // Helper functions
    inline float softClip(float x) {
        return std::tanh(x * 0.7f) / 0.7f;
    }
};