#pragma once

#include "../Source/EngineBase.h"
#include <vector>

class RotarySpeaker : public EngineBase {
public:
    RotarySpeaker();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Rotary Speaker"; }
    int getNumParameters() const override { return 5; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters
    float m_speed = 0.5f;             // Rotation speed (0.0 - 1.0 = chorale to tremolo)
    float m_acceleration = 0.3f;      // Speed change acceleration
    float m_micDistance = 0.6f;       // Microphone distance (affects Doppler intensity)
    float m_stereoWidth = 0.8f;       // Stereo spread
    float m_mix = 1.0f;               // Dry/wet mix (0.0 = dry, 1.0 = wet)
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Rotation state
    float m_hornRotation = 0.0f;      // Horn rotation angle (radians)
    float m_drumRotation = 0.0f;      // Drum rotation angle (radians)
    float m_hornVelocity = 0.0f;      // Current horn speed
    float m_drumVelocity = 0.0f;      // Current drum speed
    float m_targetHornSpeed = 1.0f;   // Target horn speed
    float m_targetDrumSpeed = 0.6f;   // Target drum speed
    
    // Crossover filters (horn/drum separation)
    struct CrossoverFilters {
        float horn_z1 = 0.0f;
        float horn_z2 = 0.0f;
        float drum_z1 = 0.0f;
        float drum_z2 = 0.0f;
    };
    
    std::vector<CrossoverFilters> m_crossover;
    
    // Doppler delay lines
    std::vector<std::vector<float>> m_hornDelayBuffers;
    std::vector<std::vector<float>> m_drumDelayBuffers;
    std::vector<int> m_hornWritePos;
    std::vector<int> m_drumWritePos;
    
    // Tube preamp simulation
    std::vector<float> m_preampState;
    
    // Leslie cabinet resonance
    std::vector<float> m_cabinetResonance;
    
    float processSample(float input, int channel);
    void processCrossover(float input, int channel, float& hornBand, float& drumBand);
    float processDoppler(float input, int channel, bool isHorn, float rotation, float velocity);
    float tubePreamp(float input, int channel);
    float cabinetResonance(float input, int channel);
    float interpolatedRead(const std::vector<float>& buffer, float position);
    void updateRotationSpeed();
};