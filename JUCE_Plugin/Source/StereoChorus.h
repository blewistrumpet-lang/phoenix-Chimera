#pragma once
#include "EngineBase.h"
#include <vector>
#include <cmath>
#include <random>

class StereoChorus : public EngineBase {
public:
    StereoChorus();
    ~StereoChorus() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Stereo Chorus"; }
    int getNumParameters() const override { return 5; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters
    float m_rate = 1.0f;        // Hz
    float m_depth = 5.0f;       // ms
    float m_voices = 2.0f;      // Number of voices
    float m_feedback = 0.0f;    // Feedback amount
    float m_width = 1.0f;       // Stereo width
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // Delay line structure
    static constexpr int MAX_DELAY_MS = 50;
    static constexpr int MAX_VOICES = 4;
    
    struct DelayLine {
        std::vector<float> buffer;
        int writePos = 0;
        int size = 0;
        
        void resize(int newSize);
        void clear();
        void write(float sample);
        float readInterpolated(float delaySamples);
    };
    
    // Per-channel chorus state
    struct ChannelState {
        std::vector<DelayLine> voiceDelays;
        std::vector<float> voicePhases;
        float feedbackSample = 0.0f;
        
        void init(int numVoices, int maxDelaySamples);
        void reset();
    };
    
    std::vector<ChannelState> m_channels;
    
    // LFO state
    struct LFO {
        float phase = 0.0f;
        float phaseOffset = 0.0f;
        
        float tick(float rate, double sampleRate);
    };
    
    std::vector<std::vector<LFO>> m_lfos; // [channel][voice]
    
    // Random generator for voice phase offsets
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_phaseDist{0.0f, 1.0f};
    
    // Interpolation helper
    float interpolate(float a, float b, float t) {
        return a + (b - a) * t;
    }
    
    // Enhanced chorus processing with analog modeling
    float processVoiceWithModeling(float input, int voice, int channel, float thermalFactor, float aging);
    
    // Oversampling for cleaner modulation
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 2;
        std::vector<float> upsampleBuffer;
        std::vector<float> downsampleBuffer;
        
        // Anti-aliasing filters
        struct AAFilter {
            std::array<float, 4> x = {0.0f};
            std::array<float, 4> y = {0.0f};
            
            float process(float input) {
                // 4th order Butterworth lowpass at Nyquist/2
                const float a0 = 0.0947f, a1 = 0.3789f, a2 = 0.5684f, a3 = 0.3789f, a4 = 0.0947f;
                const float b1 = -0.0000f, b2 = 0.4860f, b3 = -0.0000f, b4 = -0.0177f;
                
                float output = a0 * input + a1 * x[0] + a2 * x[1] + a3 * x[2] + a4 * x[3]
                             - b1 * y[0] - b2 * y[1] - b3 * y[2] - b4 * y[3];
                
                // Shift delay line
                x[3] = x[2]; x[2] = x[1]; x[1] = x[0]; x[0] = input;
                y[3] = y[2]; y[2] = y[1]; y[1] = y[0]; y[0] = output;
                
                return output;
            }
        };
        
        AAFilter upsampleFilter;
        AAFilter downsampleFilter;
        
        void prepare(int blockSize) {
            upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
        }
        
        void upsample(const float* input, float* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                output[i * 2] = upsampleFilter.process(input[i] * 2.0f);
                output[i * 2 + 1] = upsampleFilter.process(0.0f);
            }
        }
        
        void downsample(const float* input, float* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                downsampleFilter.process(input[i * 2]);
                output[i] = downsampleFilter.process(input[i * 2 + 1]) * 0.5f;
            }
        }
    };
    
    Oversampler m_oversampler;
    bool m_useOversampling = true;
};