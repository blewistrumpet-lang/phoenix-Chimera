#pragma once

#include "../Source/EngineBase.h"
#include <vector>
#include <array>
#include <memory>

class DigitalDelay : public EngineBase {
public:
    DigitalDelay();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Digital Delay"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters with parameter smoothing
    struct SmoothParam {
        float target = 0.0f;
        float current = 0.0f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void setImmediate(float value) {
            target = value;
            current = value;
        }
    };
    
    SmoothParam m_delayTime;     // Delay time (0.0 - 1.0 = 1ms - 2000ms)
    SmoothParam m_feedback;      // Feedback amount
    SmoothParam m_mix;           // Dry/wet mix
    SmoothParam m_highCut;       // High-cut filter (0.0 - 1.0 = 1kHz - 20kHz)
    
    // DSP state
    double m_sampleRate = 44100.0;
    
    // High-precision delay line with sub-sample accuracy
    struct DelayLine {
        static constexpr size_t BUFFER_SIZE = 192000; // ~4 seconds at 48kHz
        std::array<float, BUFFER_SIZE> buffer;
        double writePos = 0.0;
        double readPos = 0.0;
        
        // Modulation for subtle pitch variation
        float modPhase = 0.0f;
        float modRate = 0.7f; // Hz
        float modDepth = 0.0f; // 0-1 samples
        
        void reset() {
            buffer.fill(0.0f);
            writePos = 0.0;
            readPos = BUFFER_SIZE * 0.5;
            modPhase = 0.0f;
        }
        
        float read(double delaySamples, double sampleRate);
        void write(float sample);
        float hermiteInterpolate(double position);
    };
    
    std::array<DelayLine, 2> m_delayLines;
    
    // High-quality filters
    struct HighCutFilter {
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
        float b1 = 0.0f, b2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
        
        void updateCoefficients(float cutoffFreq, double sampleRate);
        float process(float input);
        void reset() { x1 = x2 = y1 = y2 = 0.0f; }
    };
    
    std::array<HighCutFilter, 2> m_highCutFilters;
    
    // Crossfeed for ping-pong effect
    struct CrossfeedMatrix {
        float leftToRight = 0.0f;
        float rightToLeft = 0.0f;
        float pingPongAmount = 0.3f; // Fixed amount for stereo width
        
        void process(float& left, float& right) {
            float tempL = left + rightToLeft * pingPongAmount;
            float tempR = right + leftToRight * pingPongAmount;
            leftToRight = left;
            rightToLeft = right;
            left = tempL;
            right = tempR;
        }
    } m_crossfeed;
    
    // Soft clipper for feedback path
    float softClip(float input);
    
    // DC blocker for clean output
    struct DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        const float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
    };
    
    std::array<DCBlocker, 2> m_dcBlockers;
    
    float processSample(float input, int channel);
};