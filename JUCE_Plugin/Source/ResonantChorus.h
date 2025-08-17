#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <array>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class ResonantChorus : public EngineBase {
public:
    ResonantChorus();
    ~ResonantChorus() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 5; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "ResonantChorus"; }
    
private:
    static constexpr int NUM_VOICES = 6;
    
    // Simple LFO with sine wave generation
    class LFO {
    public:
        void setSampleRate(double sr) { sampleRate = sr; }
        void setFrequency(float freq) { frequency = freq; }
        void setPhase(float ph) { phase = ph; }
        void reset() { currentPhase = phase; }
        
        float process() {
            float output = std::sin(currentPhase);
            currentPhase += 2.0f * M_PI * frequency / sampleRate;
            if (currentPhase >= 2.0f * M_PI) {
                currentPhase -= 2.0f * M_PI;
            }
            return output;
        }
        
    private:
        double sampleRate = 44100.0;
        float frequency = 1.0f;
        float phase = 0.0f;
        float currentPhase = 0.0f;
    };
    
    // SVF filter from existing StateVariableFilter
    class SVFFilter {
    public:
        void setSampleRate(double sr) { sampleRate = sr; }
        void reset() { s1 = s2 = 0.0f; }
        
        float processLowpass(float input, float frequency, float resonance) {
            float g = std::tan(M_PI * frequency / sampleRate);
            float k = 1.0f / resonance;
            float a1 = 1.0f / (1.0f + g * (g + k));
            float a2 = g * a1;
            float a3 = g * a2;
            
            float v3 = input - s2;
            float v1 = a1 * s1 + a2 * v3;
            float v2 = s2 + a2 * s1 + a3 * v3;
            
            s1 = 2.0f * v1 - s1;
            s2 = 2.0f * v2 - s2;
            
            return v2; // lowpass output
        }
        
    private:
        double sampleRate = 44100.0;
        float s1 = 0.0f;
        float s2 = 0.0f;
    };
    
    // Individual delay voice with modulation and filtering
    struct DelayVoice {
        CircularBuffer<float> delayBuffer;
        LFO lfo;
        SVFFilter filter;
        float baseDelay = 0.0f;
        
        void prepare(double sampleRate, float baseDelayMs, float lfoPhaseOffset) {
            int maxDelaySamples = (int)(sampleRate * 0.030); // 30ms max
            delayBuffer.setSize(maxDelaySamples);
            lfo.setSampleRate(sampleRate);
            lfo.setPhase(lfoPhaseOffset);
            filter.setSampleRate(sampleRate);
            baseDelay = baseDelayMs * 0.001f * sampleRate; // Convert to samples
        }
        
        void reset() {
            delayBuffer.clear();
            lfo.reset();
            filter.reset();
        }
        
        float process(float input, float lfoRate, float depth, float resonance) {
            // Write input to delay
            delayBuffer.write(input);
            
            // Generate LFO modulation
            lfo.setFrequency(lfoRate);
            float lfoValue = lfo.process();
            
            // Calculate modulated delay time
            float modulatedDelay = baseDelay + (lfoValue * depth);
            modulatedDelay = std::max(1.0f, modulatedDelay);
            
            // Read from delay with interpolation
            float delayed = delayBuffer.readInterpolated(modulatedDelay);
            
            // Apply SVF lowpass filtering with resonance
            float cutoff = 8000.0f; // Fixed cutoff around 8kHz
            return filter.processLowpass(delayed, cutoff, resonance);
        }
    };
    
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    
    // Parameters with smoothing
    ParamSmoother m_rateParam;
    ParamSmoother m_depthParam;
    ParamSmoother m_resonanceParam;
    ParamSmoother m_mixParam;
    ParamSmoother m_widthParam;
    
    // Parameter target values
    float m_rateTarget = 0.8f;
    float m_depthTarget = 0.4f;
    float m_resonanceTarget = 0.7f;
    float m_mixTarget = 0.5f;
    float m_widthTarget = 1.0f;
    
    // Delay voices (stereo)
    std::array<DelayVoice, NUM_VOICES> m_leftVoices;
    std::array<DelayVoice, NUM_VOICES> m_rightVoices;
    
    // Base delay times for each voice (in milliseconds)
    static constexpr std::array<float, NUM_VOICES> BASE_DELAYS = {
        12.0f, 15.5f, 19.0f, 22.5f, 26.0f, 25.0f
    };
    
    // LFO phase offsets for each voice (in radians)
    static constexpr std::array<float, NUM_VOICES> LFO_PHASES = {
        0.0f, M_PI/3.0f, 2.0f*M_PI/3.0f, M_PI, 4.0f*M_PI/3.0f, 5.0f*M_PI/3.0f
    };
};
