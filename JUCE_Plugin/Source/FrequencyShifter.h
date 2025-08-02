#pragma once
#include "EngineBase.h"
#include <vector>
#include <complex>
#include <memory>
#include <array>
#include <random>

class FrequencyShifter : public EngineBase {
public:
    FrequencyShifter();
    ~FrequencyShifter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Frequency Shifter"; }
    
private:
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
    
    SmoothParam m_shiftAmount;
    SmoothParam m_feedback;
    SmoothParam m_mix;
    SmoothParam m_spread;
    SmoothParam m_resonance;
    SmoothParam m_modDepth;
    SmoothParam m_modRate;
    SmoothParam m_direction;
    
    // Hilbert transformer for analytic signal
    struct HilbertTransformer {
        static constexpr int HILBERT_LENGTH = 65;
        std::vector<float> coefficients;
        std::vector<float> delayBuffer;
        int delayIndex = 0;
        
        void initialize();
        std::complex<float> process(float input);
    };
    
    struct ChannelState {
        HilbertTransformer hilbert;
        float oscillatorPhase = 0.0f;
        float modulatorPhase = 0.0f;
        std::vector<float> feedbackBuffer;
        int feedbackIndex = 0;
        
        // Resonant filter state
        float resonatorReal = 0.0f;
        float resonatorImag = 0.0f;
        
        // Component aging
        float componentDrift = 0.0f;
        float thermalFactor = 1.0f;
    };
    
    std::array<ChannelState, 2> m_channelStates;
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
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
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
    
    // Random generator for aging effects
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_distribution{-1.0f, 1.0f};
    
    // Oversampling for cleaner frequency shifting
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
    
    std::complex<float> processFrequencyShift(std::complex<float> analytic, 
                                              float shiftFreq, 
                                              float& phase);
    std::complex<float> processFrequencyShiftWithAging(std::complex<float> analytic, 
                                                       float shiftFreq, 
                                                       float& phase, 
                                                       float aging);
    void processResonator(std::complex<float>& signal, 
                         ChannelState& state, 
                         float frequency);
    void processResonatorWithAging(std::complex<float>& signal, 
                                   ChannelState& state, 
                                   float frequency, 
                                   float aging);
    float softClip(float input);
    float softClipWithAging(float input, float aging);
    
    // Sample processing function
    float processFrequencyShifterSample(float input, float channelShift, ChannelState& state, bool isOversampled);
};