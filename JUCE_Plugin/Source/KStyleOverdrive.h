#pragma once

#include "../Source/EngineBase.h"
#include <array>

class KStyleOverdrive : public EngineBase {
public:
    KStyleOverdrive();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "K-Style Overdrive"; }
    int getNumParameters() const override { return 3; }
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
    };
    
    SmoothParam m_drive;
    SmoothParam m_tone;
    SmoothParam m_level;
    
    double m_sampleRate = 44100.0;
    
    // Multi-stage filtering for authentic tube-like response
    struct FilterStage {
        // Highpass filter state (removes DC and shapes low end)
        float hp_z1 = 0.0f;
        
        // Pre-emphasis filter state
        float pre_z1 = 0.0f;
        
        // Tone stack simulation (3-band interactive EQ)
        float low_z1 = 0.0f, low_z2 = 0.0f;
        float mid_z1 = 0.0f;
        float high_z1 = 0.0f;
        
        // DC blocker
        float dc_z1_in = 0.0f;
        float dc_z1_out = 0.0f;
        
        // Oversampling states
        float up_z1 = 0.0f;
        float down_z1 = 0.0f;
        
        void reset() {
            hp_z1 = 0.0f;
            pre_z1 = 0.0f;
            low_z1 = low_z2 = 0.0f;
            mid_z1 = 0.0f;
            high_z1 = 0.0f;
            dc_z1_in = dc_z1_out = 0.0f;
            up_z1 = down_z1 = 0.0f;
        }
    };
    
    std::array<FilterStage, 2> m_filterStates;
    
    // Tube stage emulation parameters
    struct TubeStage {
        float bias = 0.15f;        // Asymmetry for even harmonics
        float saturation = 0.7f;   // Saturation amount
        float warmth = 0.3f;       // Low-mid emphasis
    };
    
    TubeStage m_tubeStage;
    
    // Processing functions
    float processSample(float input, int channel);
    float processUpsampled(float input, FilterStage& state);
    float applyTubeStage(float input, float drive);
    float applyToneStack(float input, FilterStage& state);
    
    // Oversampling for reduced aliasing
    static constexpr int OVERSAMPLE_FACTOR = 2;
    float upsample(float input, float& z1);
    float downsample(float input, float& z1);
    
    // Filter coefficient calculation
    void updateFilterCoefficients();
};