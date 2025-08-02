#pragma once

#include "../Source/EngineBase.h"
#include <vector>

class HarmonicTremolo : public EngineBase {
public:
    HarmonicTremolo();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Harmonic Tremolo"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters
    float m_rate = 0.3f;              // Tremolo rate (0.0 - 1.0 = 0.1Hz - 20Hz)
    float m_depth = 0.5f;             // Tremolo depth
    float m_harmonics = 0.4f;         // Harmonic content (crossover frequency)
    float m_stereoPhase = 0.25f;      // Stereo phase offset
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // LFO state
    std::vector<float> m_lfoPhase;
    
    // Crossover filters for harmonic tremolo
    struct CrossoverFilters {
        // High-pass for treble band
        float highpass_z1 = 0.0f;
        float highpass_z2 = 0.0f;
        
        // Low-pass for bass band  
        float lowpass_z1 = 0.0f;
        float lowpass_z2 = 0.0f;
    };
    
    std::vector<CrossoverFilters> m_crossover;
    
    // Tube-style waveshaping for vintage character
    std::vector<float> m_tubeState;
    
    float processSample(float input, int channel);
    void processCrossover(float input, int channel, float& lowBand, float& highBand);
    float tubeWaveshape(float input, int channel);
    float calculateLFO(int channel);
};