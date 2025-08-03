#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <memory>

class FeedbackNetwork : public EngineBase {
public:
    FeedbackNetwork();
    ~FeedbackNetwork() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Feedback Network"; }
    
private:
    // Smoothed parameters for boutique quality
    struct SmoothParam {
        float target = 0.5f;
        float current = 0.5f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void reset(float value) {
            target = current = value;
        }
        
        void setSmoothingTime(float timeMs, float sampleRate) {
            float samples = timeMs * 0.001f * sampleRate;
            smoothing = std::exp(-1.0f / samples);
        }
    };
    
    // Parameters with smoothing
    SmoothParam m_delayTime;
    SmoothParam m_feedback;
    SmoothParam m_crossFeed;
    SmoothParam m_diffusion;
    SmoothParam m_modulation;
    SmoothParam m_freeze;
    SmoothParam m_shimmer;
    SmoothParam m_mix;
    
    static constexpr int NUM_DELAYS = 4;
    static constexpr float MAX_DELAY_TIME = 2.0f; // seconds
    
    // Enhanced Delay line with boutique features
    struct ModulatedDelay {
        std::vector<float> buffer;
        int writeIndex = 0;
        float readIndex = 0.0f;
        int size = 0;
        
        // Modulation with thermal drift
        float lfoPhase = 0.0f;
        float lfoRate = 0.0f;
        float modDepth = 0.0f;
        float thermalDrift = 0.0f;
        
        // Component aging
        float componentAge = 0.0f;
        
        void prepare(int maxSamples) {
            size = maxSamples;
            buffer.resize(size);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writeIndex = 0;
            readIndex = size * 0.5f;
        }
        
        void write(float sample) {
            // Apply subtle tape saturation
            float processed = std::tanh(sample * 0.8f) * 1.25f;
            buffer[writeIndex] = processed;
            writeIndex = (writeIndex + 1) % size;
        }
        
        float read(float delaySamples) {
            // Apply modulation with thermal drift
            float thermalMod = thermalDrift * 0.5f;
            float modulation = (std::sin(lfoPhase) + thermalMod) * modDepth * delaySamples * 0.01f;
            float totalDelay = delaySamples + modulation;
            
            // Component aging affects delay accuracy
            totalDelay *= (1.0f + componentAge * 0.001f);
            
            // Ensure delay is within bounds
            totalDelay = std::max(1.0f, std::min(totalDelay, static_cast<float>(size - 1)));
            
            // Calculate read position
            float exactReadPos = writeIndex - totalDelay;
            while (exactReadPos < 0) exactReadPos += size;
            
            // Hermite interpolation for better quality
            int idx = static_cast<int>(exactReadPos);
            float frac = exactReadPos - idx;
            
            int idx0 = (idx - 1 + size) % size;
            int idx1 = idx;
            int idx2 = (idx + 1) % size;
            int idx3 = (idx + 2) % size;
            
            float x0 = buffer[idx0];
            float x1 = buffer[idx1];
            float x2 = buffer[idx2];
            float x3 = buffer[idx3];
            
            // Hermite interpolation
            float c0 = x1;
            float c1 = 0.5f * (x2 - x0);
            float c2 = x0 - 2.5f * x1 + 2.0f * x2 - 0.5f * x3;
            float c3 = 1.5f * (x1 - x2) + 0.5f * (x3 - x0);
            
            return ((c3 * frac + c2) * frac + c1) * frac + c0;
        }
        
        void updateModulation(double sampleRate) {
            lfoPhase += 2.0f * M_PI * lfoRate / sampleRate;
            if (lfoPhase > 2.0f * M_PI) lfoPhase -= 2.0f * M_PI;
            
            // Update thermal drift slowly
            thermalDrift += ((rand() % 1000) / 1000000.0f - 0.0005f);
            thermalDrift = std::max(-0.01f, std::min(0.01f, thermalDrift));
            
            // Update component aging
            componentAge += 0.00001f / sampleRate;
        }
    };
    
    // Diffusion all-pass
    struct DiffusionFilter {
        std::vector<float> buffer;
        int index = 0;
        int size = 0;
        float feedback = 0.618f; // Golden ratio
        
        void prepare(int delaySize) {
            size = delaySize;
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
    };
    
    // Pitch shifter for shimmer
    struct ShimmerPitchShifter {
        static constexpr int BUFFER_SIZE = 4096;
        std::array<float, BUFFER_SIZE> buffer;
        float readPos = 0.0f;
        int writePos = 0;
        
        void prepare() {
            buffer.fill(0.0f);
            readPos = BUFFER_SIZE * 0.5f;
            writePos = 0;
        }
        
        float process(float input, float pitchRatio) {
            buffer[writePos] = input;
            writePos = (writePos + 1) % BUFFER_SIZE;
            
            // Read with pitch shift
            int readIdx = static_cast<int>(readPos);
            float frac = readPos - readIdx;
            float output = buffer[readIdx] * (1.0f - frac) + 
                          buffer[(readIdx + 1) % BUFFER_SIZE] * frac;
            
            readPos += pitchRatio;
            while (readPos >= BUFFER_SIZE) readPos -= BUFFER_SIZE;
            while (readPos < 0) readPos += BUFFER_SIZE;
            
            return output;
        }
    };
    
    // Network node
    struct NetworkNode {
        ModulatedDelay delay;
        std::array<DiffusionFilter, 2> diffusers;
        ShimmerPitchShifter shimmer;
        
        // Filtering
        float lowpassState = 0.0f;
        float highpassState = 0.0f;
        
        void prepare(double sampleRate) {
            int maxSamples = static_cast<int>(MAX_DELAY_TIME * sampleRate);
            delay.prepare(maxSamples);
            
            // Different diffusion sizes for each node
            diffusers[0].prepare(static_cast<int>(0.037f * sampleRate));
            diffusers[1].prepare(static_cast<int>(0.041f * sampleRate));
            
            shimmer.prepare();
            
            lowpassState = 0.0f;
            highpassState = 0.0f;
        }
    };
    
    // Hadamard mixing matrix for decorrelation
    static constexpr float HADAMARD[4][4] = {
        { 0.5f,  0.5f,  0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f,  0.5f}
    };
    
    // Channel state
    struct ChannelState {
        std::array<NetworkNode, NUM_DELAYS> nodes;
        float inputGain = 0.0f;
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // DC Blocking for boutique quality
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        static constexpr float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    std::array<DCBlocker, 2> m_dcBlockers;
    
    // Thermal modeling for analog warmth
    struct ThermalModel {
        float temperature = 25.0f; // Room temperature in Celsius
        float thermalNoise = 0.0f;
        float thermalDrift = 0.0f;
        
        void update(double sampleRate) {
            // Slow temperature variations
            static float phase = 0.0f;
            phase += 0.00001f / sampleRate; // Very slow variation
            temperature = 25.0f + std::sin(phase) * 2.0f; // ±2°C variation
            
            // Thermal noise increases with temperature
            float noiseLevel = (temperature - 20.0f) * 0.000005f;
            thermalNoise = ((rand() % 1000) / 1000.0f - 0.5f) * noiseLevel;
            
            // Thermal drift affects timing
            thermalDrift = (temperature - 25.0f) * 0.0008f;
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalDrift;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Component aging simulation
    float m_componentAge = 0.0f; // In hours of operation
    
    void updateComponentAging(double sampleRate) {
        // Age components very slowly (1 hour = 3600 seconds)
        m_componentAge += 1.0f / (sampleRate * 3600.0f);
    }
    
    // Helper functions
    void mixMatrix(std::array<float, NUM_DELAYS>& signals);
    float softClip(float input);
    float analogSaturation(float input, float amount);
    float applyVintageWarmth(float input, float thermalFactor);
};