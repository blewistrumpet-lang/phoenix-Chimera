#pragma once
#include "EngineBase.h"
#include <vector>
#include <complex>
#include <memory>
#include <random>
#include <juce_dsp/juce_dsp.h>

class PitchShifter : public EngineBase {
public:
    PitchShifter();
    ~PitchShifter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Pitch Shifter"; }
    
private:
    static constexpr int FFT_SIZE = 4096;
    static constexpr int OVERLAP_FACTOR = 4;
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;
    
    // Parameters with smoothing
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
        
        void setSmoothingRate(float rate) {
            smoothing = rate;
        }
    };
    
    SmoothParam m_pitchRatio;
    SmoothParam m_formantShift;
    SmoothParam m_mixAmount;
    SmoothParam m_windowWidth;
    SmoothParam m_spectralGate;
    SmoothParam m_grainSize;
    SmoothParam m_feedback;
    SmoothParam m_stereoWidth;
    
    struct ChannelState {
        std::vector<float> inputBuffer;
        std::vector<float> outputBuffer;
        std::vector<float> windowBuffer;
        std::vector<float> analysisWindow;
        std::vector<float> synthesisWindow;
        std::vector<std::complex<float>> spectrum;
        std::vector<float> phaseLast;
        std::vector<float> phaseSum;
        std::vector<float> magnitude;
        std::vector<float> frequency;
        std::vector<float> feedbackBuffer;
        int inputPos = 0;
        int outputPos = 0;
        int feedbackPos = 0;
        
        juce::dsp::FFT fft{12}; // 2^12 = 4096
    };
    
    std::vector<ChannelState> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // DC blocking
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
    
    std::vector<DCBlocker> m_inputDCBlockers;
    std::vector<DCBlocker> m_outputDCBlockers;
    
    // Thermal modeling
    struct ThermalModel {
        float temperature = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            // Slow thermal drift
            thermalNoise += (dist(rng) * 0.001f) / sampleRate;
            thermalNoise = std::max(-0.02f, std::min(0.02f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging simulation
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    void processSpectralFrame(ChannelState& state);
    void shiftSpectrum(std::vector<std::complex<float>>& spectrum, 
                      const std::vector<float>& magnitude,
                      const std::vector<float>& frequency,
                      float pitchRatio, float formantRatio);
    float getWindow(int pos, int size);
    void createWindows(std::vector<float>& analysis, std::vector<float>& synthesis);
    
    // Oversampling for cleaner pitch shifting
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