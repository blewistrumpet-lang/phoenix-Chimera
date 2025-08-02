#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <cmath>

class StereoWidener : public EngineBase {
public:
    StereoWidener();
    ~StereoWidener() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Stereo Widener"; }
    
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
    SmoothParam m_width;          // Stereo width (0% to 200%)
    SmoothParam m_bassMonoFreq;   // Bass mono frequency (20Hz to 500Hz)
    SmoothParam m_highShelfFreq;  // High shelf frequency (1kHz to 20kHz)
    SmoothParam m_highShelfGain;  // High shelf gain for sides (-20dB to +20dB)
    SmoothParam m_delayTime;      // Haas delay time (0ms to 30ms)
    SmoothParam m_delayGain;      // Delay channel gain (0% to 100%)
    SmoothParam m_correlation;    // Inter-channel correlation (0% to 100%)
    SmoothParam m_mix;            // Dry/wet mix (0% to 100%)
    
    // High-quality all-pass filter for phase decorrelation
    struct AllPassFilter {
        static constexpr int MAX_DELAY = 512;
        std::array<float, MAX_DELAY> buffer;
        int index = 0;
        int delay = 0;
        float coefficient = 0.0f;
        
        void setDelay(int delaySamples) {
            delay = std::max(1, std::min(delaySamples, MAX_DELAY - 1));
        }
        
        void setCoefficient(float coeff) {
            coefficient = std::max(-0.99f, std::min(0.99f, coeff));
        }
        
        float process(float input) {
            int readIndex = (index - delay + MAX_DELAY) % MAX_DELAY;
            float delayed = buffer[readIndex];
            
            float output = -coefficient * input + delayed;
            buffer[index] = input + coefficient * delayed;
            
            index = (index + 1) % MAX_DELAY;
            return output;
        }
        
        void reset() {
            buffer.fill(0.0f);
            index = 0;
        }
    };
    
    // Haas effect delay line
    struct HaasDelay {
        static constexpr int MAX_DELAY_MS = 35;
        std::vector<float> buffer;
        int writeIndex = 0;
        int size = 0;
        
        void prepare(double sampleRate) {
            size = static_cast<int>(MAX_DELAY_MS * 0.001 * sampleRate) + 1;
            buffer.resize(size);
            buffer.assign(size, 0.0f);
            writeIndex = 0;
        }
        
        void write(float sample) {
            buffer[writeIndex] = sample;
            writeIndex = (writeIndex + 1) % size;
        }
        
        float read(float delayMs, double sampleRate) {
            float delaySamples = delayMs * 0.001f * sampleRate;
            delaySamples = std::max(0.0f, std::min(delaySamples, static_cast<float>(size - 1)));
            
            int readIndex1 = writeIndex - static_cast<int>(delaySamples);
            if (readIndex1 < 0) readIndex1 += size;
            
            int readIndex2 = (readIndex1 - 1 + size) % size;
            float frac = delaySamples - static_cast<int>(delaySamples);
            
            return buffer[readIndex1] * (1.0f - frac) + buffer[readIndex2] * frac;
        }
        
        void reset() {
            buffer.assign(size, 0.0f);
            writeIndex = 0;
        }
    };
    
    // High-quality shelving filter
    struct ShelfFilter {
        float state1 = 0.0f;
        float state2 = 0.0f;
        
        float processHighShelf(float input, float freq, float gain, double sampleRate) {
            float w = juce::MathConstants<float>::twoPi * freq / sampleRate;
            float cosw = std::cos(w);
            float sinw = std::sin(w);
            float A = std::pow(10.0f, gain / 40.0f); // Convert dB to linear for shelving
            float beta = std::sqrt(A) / 1.0f; // Q = 1 for shelf
            
            float b0 = A * ((A + 1) + (A - 1) * cosw + beta * sinw);
            float b1 = -2 * A * ((A - 1) + (A + 1) * cosw);
            float b2 = A * ((A + 1) + (A - 1) * cosw - beta * sinw);
            float a0 = (A + 1) - (A - 1) * cosw + beta * sinw;
            float a1 = 2 * ((A - 1) - (A + 1) * cosw);
            float a2 = (A + 1) - (A - 1) * cosw - beta * sinw;
            
            // Normalize coefficients
            b0 /= a0; b1 /= a0; b2 /= a0;
            a1 /= a0; a2 /= a0;
            
            float output = b0 * input + b1 * state1 + b2 * state2 - a1 * state1 - a2 * state2;
            
            // Update states
            state2 = state1;
            state1 = input;
            
            return output;
        }
        
        void reset() {
            state1 = state2 = 0.0f;
        }
    };
    
    // Bass mono filter (high-pass for sides)
    struct BassMonoFilter {
        float state = 0.0f;
        
        float processHighPass(float input, float cutoff) {
            float output = input - state;
            state += output * cutoff;
            return output;
        }
        
        void reset() {
            state = 0.0f;
        }
    };
    
    // Channel state
    struct ChannelState {
        AllPassFilter allPass1;
        AllPassFilter allPass2;
        HaasDelay haasDelay;
        ShelfFilter shelfFilter;
        BassMonoFilter bassMonoFilter;
        
        void prepare(double sampleRate) {
            allPass1.reset();
            allPass2.reset();
            haasDelay.prepare(sampleRate);
            shelfFilter.reset();
            bassMonoFilter.reset();
            
            // Set up all-pass filters with different delays for decorrelation
            allPass1.setDelay(47);  // Prime numbers for inharmonic relationships
            allPass2.setDelay(97);
            allPass1.setCoefficient(0.7f);
            allPass2.setCoefficient(-0.6f);
        }
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
            float noiseLevel = (temperature - 20.0f) * 0.000004f;
            thermalNoise = ((rand() % 1000) / 1000.0f - 0.5f) * noiseLevel;
            
            // Thermal drift affects timing and gain
            thermalDrift = (temperature - 25.0f) * 0.0007f;
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
    
    // Analog modeling functions
    float analogSaturation(float input, float amount);
    float applyVintageWarmth(float input, float thermalFactor);
    
    // Stereo processing utilities
    void processMidSide(float& left, float& right, float width);
    float calculateCorrelation(float left, float right, float amount);
};