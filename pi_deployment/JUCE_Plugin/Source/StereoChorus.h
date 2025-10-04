#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <vector>
#include <array>

class StereoChorus : public EngineBase {
public:
    StereoChorus();
    ~StereoChorus() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 6; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "StereoChorus"; }
    
private:
    double m_sampleRate = 44100.0;
    
    // Smoothed parameters
    struct SmoothParam {
        float current = 0.0f;
        float target = 0.0f;
        float smoothing = 0.99f;
        
        void reset(float value) {
            current = target = value;
        }
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void setSmoothingTime(float ms, double sampleRate) {
            float samples = ms * 0.001f * sampleRate;
            smoothing = std::exp(-1.0f / samples);
        }
    };
    
    SmoothParam m_rate;
    SmoothParam m_depth;
    SmoothParam m_feedback;
    SmoothParam m_delay;
    SmoothParam m_width;
    SmoothParam m_mix;
    
    // Delay lines for each channel
    std::array<std::vector<float>, 2> m_delayLines;
    std::array<int, 2> m_writePos = {0, 0};
    
    // LFO state for each channel
    std::array<float, 2> m_lfoPhase = {0.0f, 0.0f};
    
    // Feedback state
    std::array<float, 2> m_feedbackState = {0.0f, 0.0f};
    
    // Simple one-pole filters for feedback path
    class SimpleFilter {
    public:
        SimpleFilter(bool hp = false) : isHighpass(hp) {}
        
        void reset();
        float process(float input, float cutoff, double sampleRate);
        
    private:
        float state = 0.0f;
        bool isHighpass;
    };
    
    std::array<SimpleFilter, 2> m_highpass = {SimpleFilter(true), SimpleFilter(true)};
    std::array<SimpleFilter, 2> m_lowpass = {SimpleFilter(false), SimpleFilter(false)};
};