#pragma once
#include "EngineBase.h"
#include <vector>
#include <memory>
#include <array>
#include <random>
#include <juce_dsp/juce_dsp.h>

class ShimmerReverb : public EngineBase {
public:
    ShimmerReverb();
    ~ShimmerReverb() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Shimmer Reverb"; }
    
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
    
    SmoothParam m_roomSize;
    SmoothParam m_damping;
    SmoothParam m_shimmerAmount;
    SmoothParam m_shimmerPitch; // semitones
    SmoothParam m_predelay;
    SmoothParam m_modulation;
    SmoothParam m_highCut;
    SmoothParam m_mix;
    
    // Freeverb-style reverb structure
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_ALLPASS = 4;
    
    struct DelayLine {
        std::vector<float> buffer;
        int writePos = 0;
        float feedback = 0.0f;
        float damp = 0.0f;
        float dampState = 0.0f;
        
        void setSize(int size) {
            buffer.resize(size);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
        
        float process(float input) {
            float output = buffer[writePos];
            float filtered = output * (1.0f - damp) + dampState * damp;
            dampState = filtered;
            buffer[writePos] = input + filtered * feedback;
            writePos = (writePos + 1) % buffer.size();
            return output;
        }
        
        float processWithAging(float input, float aging) {
            float output = process(input);
            
            // Aging affects delay line precision and adds slight modulation
            if (aging > 0.01f) {
                // Add jitter from aging components
                float jitter = aging * 0.03f * ((rand() % 1000) / 1000.0f - 0.5f);
                output *= (1.0f + jitter);
                
                // Add slight high frequency roll-off due to aging
                static float hfState = 0.0f;
                float cutoff = 0.15f * (1.0f - aging * 0.3f);
                hfState += (output - hfState) * cutoff;
                output = output * 0.8f + hfState * 0.2f;
            }
            
            return output;
        }
    };
    
    struct PitchShifter {
        static constexpr int GRAIN_SIZE = 4096;
        static constexpr int NUM_GRAINS = 4; // Increased for smoother shimmer
        
        std::array<std::vector<float>, NUM_GRAINS> grainBuffers;
        std::array<int, NUM_GRAINS> grainPos;
        std::array<float, NUM_GRAINS> grainPhase;
        std::vector<float> inputBuffer;
        int inputPos = 0;
        float pitchRatio = 2.0f;
        
        void prepare(int maxSamples) {
            for (auto& grain : grainBuffers) {
                grain.resize(GRAIN_SIZE);
                std::fill(grain.begin(), grain.end(), 0.0f);
            }
            inputBuffer.resize(GRAIN_SIZE * 2);
            std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
            std::fill(grainPos.begin(), grainPos.end(), 0);
            std::fill(grainPhase.begin(), grainPhase.end(), 0.0f);
            
            // Distribute grains evenly in phase for smoother overlap
            for (int i = 0; i < NUM_GRAINS; ++i) {
                grainPhase[i] = static_cast<float>(i) / NUM_GRAINS;
            }
        }
        
        float process(float input);
        float processWithOversampling(float input, Oversampler& oversampler);
    };
    
    struct ChannelState {
        std::array<DelayLine, NUM_COMBS> combFilters;
        std::array<DelayLine, NUM_ALLPASS> allpassFilters;
        PitchShifter shifter;
        float preDelayBuffer[44100]; // 1 second max at 44.1kHz
        int preDelayPos = 0;
        int preDelaySamples = 0;
        float modulationPhase = 0.0f;
        
        // High cut filter
        float highCutState = 0.0f;
        
        // Aging effects per channel
        float componentDrift = 0.0f;
        float thermalNoise = 0.0f;
        float noiseLevel = 0.0f;
        
        void updateAging(float aging) {
            componentDrift = aging * 0.012f; // 1.2% max drift
            thermalNoise = aging * 0.0015f;   // Thermal fluctuations
            noiseLevel = aging * 0.0006f;     // Very subtle noise floor
        }
    };
    
    std::vector<ChannelState> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Tuning values (in samples at 44.1kHz)
    static const int combTuning[NUM_COMBS];
    static const int allpassTuning[NUM_ALLPASS];
    
    void updateInternalParameters();
    
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
    
    // Enhanced reverb processing with analog modeling
    float processReverbWithModeling(float input, int channel, float thermalFactor, float aging);
    
    // Oversampling for shimmer pitch shifting
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