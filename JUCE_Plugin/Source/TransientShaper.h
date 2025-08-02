#pragma once
#include "EngineBase.h"
#include <vector>
#include <memory>
#include <array>
#include <random>

class TransientShaper : public EngineBase {
public:
    TransientShaper();
    ~TransientShaper() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Transient Shaper"; }
    
private:
    // Boutique parameter smoothing system
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
    
    // Smoothed parameters
    SmoothParam m_attack;
    SmoothParam m_sustain;
    SmoothParam m_sensitivity;
    SmoothParam m_speed;
    SmoothParam m_clipper;
    SmoothParam m_punchMode;
    SmoothParam m_stereoLink;
    SmoothParam m_mix;
    
    struct EnvelopeFollower {
        float attackTime = 0.001f;
        float releaseTime = 0.01f;
        float envelope = 0.0f;
        float peakEnvelope = 0.0f;
        
        float process(float input) {
            float inputAbs = std::abs(input);
            
            // Fast peak detector
            if (inputAbs > peakEnvelope) {
                peakEnvelope = inputAbs;
            } else {
                peakEnvelope *= 0.9999f; // Slow decay
            }
            
            // Smooth envelope follower
            float target = inputAbs;
            float rate = target > envelope ? attackTime : releaseTime;
            envelope += (target - envelope) * rate;
            
            return envelope;
        }
        
        void setSpeed(float speed) {
            // Speed controls the envelope times
            attackTime = 0.0001f + (1.0f - speed) * 0.01f;
            releaseTime = 0.001f + (1.0f - speed) * 0.1f;
        }
    };
    
    // DC Blocking filter
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
    
    // Thermal modeling for analog drift simulation
    struct ThermalModel {
        float temperature = 20.0f; // Celsius
        float thermalTimeConstant = 0.99999f;
        float componentDrift = 0.0f;
        
        void update(float processingLoad) {
            // Simulate thermal buildup from processing
            float targetTemp = 20.0f + processingLoad * 15.0f; // Up to 35°C
            temperature = temperature * thermalTimeConstant + targetTemp * (1.0f - thermalTimeConstant);
            
            // Component drift based on temperature (subtle)
            componentDrift = (temperature - 20.0f) * 0.001f; // ±1.5% max drift
        }
        
        float getTemperatureDrift() const {
            return componentDrift;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f; // In processing hours
        float agingRate = 1.0f / (100.0f * 3600.0f * 44100.0f); // Age over 100 hours
        
        void update() {
            age += agingRate;
        }
        
        float getAgingFactor() const {
            // Subtle aging effects (capacitor drift, resistor tolerance)
            return 1.0f + std::sin(age * 0.1f) * 0.002f; // ±0.2% variation
        }
    };
    
    // Enhanced oversampling system
    struct Oversampler {
        static constexpr int FACTOR = 2; // 2x oversampling for transients
        std::array<float, FACTOR * 128> buffer;
        int bufferIndex = 0;
        
        // Anti-aliasing filter coefficients (Butterworth 4th order)
        struct AntiAliasingFilter {
            float b0, b1, b2, a1, a2;
            float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            
            void setCoefficients(double sampleRate) {
                double nyquist = sampleRate * 0.5;
                double cutoff = nyquist * 0.45; // Anti-aliasing at 45% Nyquist
                double w = 2.0 * M_PI * cutoff / sampleRate;
                double cosw = std::cos(w);
                double sinw = std::sin(w);
                double alpha = sinw / (2.0 * 0.707); // Q = 0.707 for Butterworth
                
                b0 = (1.0 - cosw) * 0.5;
                b1 = 1.0 - cosw;
                b2 = (1.0 - cosw) * 0.5;
                double a0 = 1.0 + alpha;
                a1 = -2.0 * cosw / a0;
                a2 = (1.0 - alpha) / a0;
                b0 /= a0; b1 /= a0; b2 /= a0;
            }
            
            float process(float input) {
                float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = input;
                y2 = y1; y1 = output;
                return output;
            }
        };
        
        AntiAliasingFilter upsampleFilter, downsampleFilter;
        
        void prepare(double sampleRate) {
            buffer.fill(0.0f);
            bufferIndex = 0;
            upsampleFilter.setCoefficients(sampleRate * FACTOR);
            downsampleFilter.setCoefficients(sampleRate * FACTOR);
        }
        
        std::array<float, FACTOR> upsample(float input) {
            std::array<float, FACTOR> upsampled;
            upsampled[0] = upsampleFilter.process(input * FACTOR);
            for (int i = 1; i < FACTOR; ++i) {
                upsampled[i] = upsampleFilter.process(0.0f);
            }
            return upsampled;
        }
        
        float downsample(const std::array<float, FACTOR>& samples) {
            float sum = 0.0f;
            for (float sample : samples) {
                sum += downsampleFilter.process(sample);
            }
            return sum / FACTOR;
        }
    };
    
    struct ChannelState {
        EnvelopeFollower signalEnvelope;
        EnvelopeFollower transientEnvelope;
        
        float lastSample = 0.0f;
        float smoothedDiff = 0.0f;
        float gain = 1.0f;
        float lastGain = 1.0f;
        
        // Boutique quality components
        DCBlocker inputDCBlocker, outputDCBlocker;
        ThermalModel thermalModel;
        ComponentAging componentAging;
        Oversampler oversampler;
        
        // Enhanced lookahead buffer with Hermite interpolation
        static constexpr int LOOKAHEAD_SIZE = 128; // Increased for better quality
        std::array<float, LOOKAHEAD_SIZE> lookaheadBuffer;
        int lookaheadIndex = 0;
        
        // Zero Delay Feedback (ZDF) filter for transient isolation
        struct ZDFHighpass {
            float s = 0.0f; // State
            float g = 0.0f; // Coefficient
            
            void setCutoff(float cutoffHz, double sampleRate) {
                float wd = 2.0f * M_PI * cutoffHz;
                float T = 1.0f / sampleRate;
                float wa = (2.0f / T) * std::tan(wd * T / 2.0f);
                g = wa * T / 2.0f;
            }
            
            float process(float input) {
                float v = (input - s) / (1.0f + g);
                float hp = v;
                s += 2.0f * g * v;
                return hp;
            }
        } zdfHighpass;
        
        // Advanced noise floor simulation
        std::mutable_thread_local std::mt19937 noiseGen{std::random_device{}()};
        std::mutable_thread_local std::normal_distribution<float> noiseDist{0.0f, 1.0f};
        
        float addAnalogNoise(float input) {
            // Thermal noise simulation (-120dB noise floor)
            float noise = noiseDist(noiseGen) * 0.000001f;
            return input + noise;
        }
    };
    
    std::vector<ChannelState> m_channelStates;
    double m_sampleRate = 44100.0;
    
    float calculateTransientGain(float transientLevel, float sustainLevel);
    float processPunchMode(float gain, float transientLevel);
    float softClip(float input, float threshold);
};