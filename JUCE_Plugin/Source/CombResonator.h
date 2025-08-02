#pragma once
#include "EngineBase.h"
#include <vector>
#include <cmath>
#include <array>

class CombResonator : public EngineBase {
public:
    CombResonator();
    ~CombResonator() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Comb Resonator"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters
    float m_rootFrequency = 110.0f;    // Fundamental pitch (Hz)
    float m_resonance = 0.9f;          // Feedback amount
    float m_harmonicSpread = 1.0f;     // Overtone spacing
    float m_decayTime = 1.0f;          // Resonance length (seconds)
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // Comb filter structure
    struct CombFilter {
        std::vector<float> delayLine;
        int writePos = 0;
        int delayLength = 0;
        float feedback = 0.9f;
        float feedforward = 0.5f;
        
        void init(int maxDelay);
        void setDelay(int samples);
        float process(float input);
        void reset();
    };
    
    // Array of 8 comb filters per channel
    static constexpr int NUM_COMBS = 8;
    struct ChannelState {
        std::array<CombFilter, NUM_COMBS> combs;
        
        // Stereo decorrelation
        float detuneAmount = 0.0f;
        
        void init(int maxDelay);
        void reset();
    };
    
    std::vector<ChannelState> m_channels;
    
    // Harmonic series ratios
    const float HARMONIC_RATIOS[NUM_COMBS] = {
        1.0f,    // Fundamental
        2.0f,    // Octave
        3.0f,    // Perfect fifth
        4.0f,    // Two octaves
        5.0f,    // Major third
        6.0f,    // Fifth + octave
        7.0f,    // Minor seventh
        8.0f     // Three octaves
    };
    
    // Calculate delay time from frequency
    int frequencyToDelay(float freq, double sampleRate) {
        return static_cast<int>(sampleRate / freq);
    }
    
    // Calculate feedback from decay time
    float decayTimeToFeedback(float decaySeconds, float delaySamples, double sampleRate);
    
    // DC blocker for stability
    struct DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        float process(float input);
    };
    
    std::vector<DCBlocker> m_dcBlockers;
};