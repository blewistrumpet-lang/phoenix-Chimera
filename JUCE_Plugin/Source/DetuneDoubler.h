#pragma once

#include "../Source/EngineBase.h"
#include <vector>

class DetuneDoubler : public EngineBase {
public:
    DetuneDoubler();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Detune Doubler"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters
    float m_detuneAmount = 0.3f;      // Detune amount in cents (0.0 - 1.0 = 0 - 50 cents)
    float m_delayTime = 0.15f;        // Base delay time (0.0 - 1.0 = 5ms - 50ms)
    float m_feedback = 0.1f;          // Subtle feedback for thickness
    float m_stereoWidth = 0.7f;       // Stereo spread of doubled voices
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // Variable delay lines for pitch shifting (ADT technique)
    std::vector<std::vector<float>> m_delayBuffers;
    std::vector<float> m_writePositions;
    std::vector<float> m_readPositions;
    
    // LFO for subtle modulation (simulates tape speed variations)
    std::vector<float> m_lfoPhase;
    float m_lfoRate = 0.1f; // Hz
    
    // Crossfading for smooth pitch shifts
    std::vector<float> m_crossfadePhase;
    static const int CROSSFADE_SAMPLES = 1024;
    
    // All-pass filters for phase decoration
    struct AllPassFilter {
        float delay = 0.0f;
        float gain = 0.7f;
        std::vector<float> buffer;
        int writePos = 0;
        int delayLength = 0;
    };
    
    std::vector<std::vector<AllPassFilter>> m_allPassChains;
    
    // Tape-style filtering
    std::vector<float> m_lowpassState;
    std::vector<float> m_highpassState;
    
    float processSample(float input, int channel);
    float variableDelay(float input, int channel, float delaySamples);
    float allPassChain(float input, int channel, int chainIndex);
    float tapeFilter(float input, int channel);
    float generateLFO(int channel);
    void updateDelayTimes();
};