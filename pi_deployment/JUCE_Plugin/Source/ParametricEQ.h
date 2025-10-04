#pragma once
#include "EngineBase.h"
#include <vector>

class ParametricEQ : public EngineBase {
public:
    ParametricEQ();
    ~ParametricEQ() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 9; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "ParametricEQ"; }
    
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
    
    SmoothParam m_lowGain;
    SmoothParam m_lowFreq;
    SmoothParam m_midGain;
    SmoothParam m_midFreq;
    SmoothParam m_midQ;
    SmoothParam m_highGain;
    SmoothParam m_highFreq;
    SmoothParam m_outputGain;
    SmoothParam m_mix;
    
    // BiQuad filter implementation
    class BiQuadFilter {
    public:
        enum class FilterType {
            LOW_SHELF,
            HIGH_SHELF,
            PEAK
        };
        
        BiQuadFilter(FilterType t = FilterType::PEAK) : type(t) {}
        
        void reset();
        float process(float input);
        void setCoefficients(float freq, float gainDb, float Q, double sampleRate);
        
    private:
        FilterType type;
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
    };
    
    // Filter banks for each channel
    BiQuadFilter m_lowShelf[2] = { BiQuadFilter(BiQuadFilter::FilterType::LOW_SHELF), 
                                   BiQuadFilter(BiQuadFilter::FilterType::LOW_SHELF) };
    BiQuadFilter m_midBand[2] = { BiQuadFilter(BiQuadFilter::FilterType::PEAK), 
                                  BiQuadFilter(BiQuadFilter::FilterType::PEAK) };
    BiQuadFilter m_highShelf[2] = { BiQuadFilter(BiQuadFilter::FilterType::HIGH_SHELF), 
                                    BiQuadFilter(BiQuadFilter::FilterType::HIGH_SHELF) };
    
    void updateFilterCoefficients();
};