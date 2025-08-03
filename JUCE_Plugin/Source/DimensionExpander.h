#pragma once
#include "EngineBase.h"
#include <vector>
#include <memory>
#include <array>
#include <random>

class DimensionExpander : public EngineBase {
public:
    DimensionExpander();
    ~DimensionExpander() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Dimension Expander"; }
    
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
    
    SmoothParam m_width;
    SmoothParam m_depth;
    SmoothParam m_crossfeed;
    SmoothParam m_bassRetention;
    SmoothParam m_ambience;
    SmoothParam m_movement;
    SmoothParam m_clarity;
    SmoothParam m_mix;
    
    // Haas effect delays (in samples)
    static constexpr int MAX_HAAS_DELAY = 50; // ~1ms at 48kHz
    
    // Micro-pitch shift for depth
    struct MicroPitchShifter {
        static constexpr int BUFFER_SIZE = 4096;
        std::vector<float> buffer;
        float readPos = 0.0f;
        int writePos = 0;
        float pitchOffset = 0.0f;
        
        void prepare() {
            buffer.resize(BUFFER_SIZE);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            readPos = BUFFER_SIZE / 2.0f;
            writePos = 0;
        }
        
        float process(float input, float cents) {
            buffer[writePos] = input;
            writePos = (writePos + 1) % BUFFER_SIZE;
            
            // Calculate pitch ratio from cents
            float ratio = std::pow(2.0f, cents / 1200.0f);
            
            // Read with interpolation
            int readIdx = static_cast<int>(readPos);
            float frac = readPos - readIdx;
            int nextIdx = (readIdx + 1) % BUFFER_SIZE;
            int prevIdx = (readIdx - 1 + BUFFER_SIZE) % BUFFER_SIZE;
            int next2Idx = (readIdx + 2) % BUFFER_SIZE;
            
            // Hermite interpolation for better quality
            float y0 = buffer[prevIdx];
            float y1 = buffer[readIdx];
            float y2 = buffer[nextIdx];
            float y3 = buffer[next2Idx];
            
            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c3 = 1.5f * (y1 - y2) + 0.5f * (y3 - y0);
            
            float output = ((c3 * frac + c2) * frac + c1) * frac + c0;
            
            // Update read position
            readPos += ratio;
            while (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;
            while (readPos < 0) readPos += BUFFER_SIZE;
            
            return output;
        }
        
        float processWithThermal(float input, float cents, float thermalFactor) {
            // Add thermal variation to pitch
            float thermalCents = cents * thermalFactor;
            return process(input, thermalCents);
        }
    };
    
    struct ChannelState {
        // Haas delay lines
        std::array<float, MAX_HAAS_DELAY> delayBuffer;
        int delayIndex = 0;
        
        // All-pass filters for ambience
        struct AllPass {
            static constexpr int MAX_SIZE = 2048;
            std::vector<float> buffer;
            int index = 0;
            int size = 0;
            float feedback = 0.5f;
            
            void setSize(int newSize) {
                size = std::min(newSize, MAX_SIZE);
                buffer.resize(size);
                std::fill(buffer.begin(), buffer.end(), 0.0f);
                index = 0;
            }
            
            float process(float input) {
                float delayed = buffer[index];
                float output = -input + delayed;
                buffer[index] = input + delayed * feedback;
                index = (index + 1) % size;
                return output;
            }
            
            float processWithAging(float input, float aging) {
                float output = process(input);
                
                // Aging affects allpass precision and adds slight coloration
                if (aging > 0.01f) {
                    // Add jitter from aging components
                    float jitter = aging * 0.02f * ((rand() % 1000) / 1000.0f - 0.5f);
                    output *= (1.0f + jitter);
                    
                    // Add slight frequency response changes due to aging
                    static float agingState = 0.0f;
                    float cutoff = 0.2f * (1.0f - aging * 0.2f);
                    agingState += (output - agingState) * cutoff;
                    output = output * 0.9f + agingState * 0.1f;
                }
                
                return output;
            }
        };
        
        std::array<AllPass, 4> allpassFilters;
        MicroPitchShifter pitchShifter;
        
        // Movement LFO
        float lfoPhase = 0.0f;
        
        // Crossover filters for bass retention
        float lowpassState = 0.0f;
        float highpassState = 0.0f;
        
        // Aging effects per channel
        float componentDrift = 0.0f;
        float thermalNoise = 0.0f;
        float noiseLevel = 0.0f;
        
        void updateAging(float aging) {
            componentDrift = aging * 0.008f; // 0.8% max drift
            thermalNoise = aging * 0.001f;   // Thermal fluctuations
            noiseLevel = aging * 0.0004f;    // Very subtle noise floor
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    void processMidSide(float& left, float& right);
    float processAmbience(float input, ChannelState& state);
    void processCrossfeed(float& left, float& right, float amount);
    
    // DC blocking
    struct DCBlocker {
        float x1 = 0.0f;
        float y1 = 0.0f;
        float R = 0.995f;
        
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
    
    // Enhanced dimension processing with analog modeling
    float processChannelWithModeling(float input, int channel, float thermalFactor, float aging);
    
    // Multiple expansion modes
    enum class ExpansionMode {
        Classic,
        Vintage,
        Modern,
        Studio
    };
    
    ExpansionMode m_currentMode = ExpansionMode::Classic;
    
    // Oversampling for high-quality processing
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