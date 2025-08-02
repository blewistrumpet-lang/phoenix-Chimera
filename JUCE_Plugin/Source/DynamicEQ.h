#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <cmath>
#include <complex>

class DynamicEQ : public EngineBase {
public:
    DynamicEQ();
    ~DynamicEQ() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Dynamic EQ"; }
    
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
    SmoothParam m_frequency;      // Center frequency (20Hz - 20kHz)
    SmoothParam m_threshold;      // Dynamic threshold (-60dB to 0dB)
    SmoothParam m_ratio;          // Compression/expansion ratio (0.1:1 to 10:1)
    SmoothParam m_attack;         // Attack time (0.1ms to 100ms)
    SmoothParam m_release;        // Release time (10ms to 5000ms)
    SmoothParam m_gain;           // Static gain (-20dB to +20dB)
    SmoothParam m_mix;            // Dry/wet mix (0% to 100%)
    SmoothParam m_mode;           // Mode: Compressor/Expander/Gate
    
    // High-quality TPT (Topology Preserving Transform) SVF filter
    struct TPTFilter {
        // State variables
        float v0 = 0.0f, v1 = 0.0f, v2 = 0.0f;
        float ic1eq = 0.0f, ic2eq = 0.0f;
        
        // Filter coefficients
        float g = 0.0f;    // Frequency coefficient
        float k = 0.0f;    // Resonance coefficient  
        float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        
        void setParameters(float frequency, float Q, double sampleRate) {
            float w = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(sampleRate);
            g = std::tan(w * 0.5f);
            k = 1.0f / Q;
            
            float k2 = k + k;
            float gk = g * k;
            float a0 = 1.0f / (1.0f + gk + g * g);
            
            a1 = g * a0;
            a2 = g * a1;
            a3 = k2 * a1;
        }
        
        struct FilterOutputs {
            float lowpass = 0.0f;
            float highpass = 0.0f;
            float bandpass = 0.0f;
            float notch = 0.0f;
            float allpass = 0.0f;
            float peak = 0.0f;
        };
        
        FilterOutputs process(float input) {
            FilterOutputs outputs;
            
            // Apply thermal and aging effects to input
            float thermalInput = input + ((rand() % 1000) / 1000000.0f - 0.0005f); // Tiny thermal noise
            
            v0 = thermalInput;
            v1 = a1 * v0 - a2 * v1 - a3 * v2 + ic1eq;
            v2 = a1 * v1 - a2 * v2 + ic2eq;
            
            // Update integrator states
            ic1eq = 2.0f * a1 * v0 - a2 * v1 - a3 * v2 + ic1eq;
            ic2eq = 2.0f * a1 * v1 - a2 * v2 + ic2eq;
            
            // Generate all filter outputs
            outputs.lowpass = v2;
            outputs.highpass = v0 - k * v1 - v2;
            outputs.bandpass = v1;
            outputs.notch = v0 - k * v1;
            outputs.allpass = v0 - k2 * v1;
            outputs.peak = outputs.lowpass - outputs.highpass;
            
            return outputs;
        }
        
        void reset() {
            v0 = v1 = v2 = 0.0f;
            ic1eq = ic2eq = 0.0f;
        }
    };
    
    // Dynamic processor with lookahead
    struct DynamicProcessor {
        static constexpr int LOOKAHEAD_SAMPLES = 64;
        static constexpr int ENVELOPE_HISTORY = 32;
        
        // Lookahead delay line
        std::array<float, LOOKAHEAD_SAMPLES> delayLine;
        int delayIndex = 0;
        
        // Envelope detection
        float envelope = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        
        // Gain reduction history for smooth operation
        std::array<float, ENVELOPE_HISTORY> gainHistory;
        int historyIndex = 0;
        
        void setTiming(float attackMs, float releaseMs, double sampleRate) {
            attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
            releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
        }
        
        float process(float input, float threshold, float ratio, int mode) {
            // Store input in delay line
            delayLine[delayIndex] = input;
            
            // Get delayed signal
            int readIndex = (delayIndex + 1) % LOOKAHEAD_SAMPLES;
            float delayedSignal = delayLine[readIndex];
            
            // Advance delay index
            delayIndex = (delayIndex + 1) % LOOKAHEAD_SAMPLES;
            
            // Peak detection with lookahead
            float peak = 0.0f;
            for (int i = 0; i < LOOKAHEAD_SAMPLES; ++i) {
                peak = std::max(peak, std::abs(delayLine[i]));
            }
            
            // Envelope following
            float targetEnv = peak;
            if (targetEnv > envelope) {
                envelope = targetEnv + (envelope - targetEnv) * attackCoeff;
            } else {
                envelope = targetEnv + (envelope - targetEnv) * releaseCoeff;
            }
            
            // Calculate gain reduction
            float gainReduction = 1.0f;
            float envDb = 20.0f * std::log10(std::max(0.00001f, envelope));
            
            if (mode == 0) { // Compressor
                if (envDb > threshold) {
                    float over = envDb - threshold;
                    float compressedOver = over / ratio;
                    gainReduction = std::pow(10.0f, -(over - compressedOver) / 20.0f);
                }
            } else if (mode == 1) { // Expander
                if (envDb < threshold) {
                    float under = threshold - envDb;
                    float expandedUnder = under * ratio;
                    gainReduction = std::pow(10.0f, -(expandedUnder - under) / 20.0f);
                }
            } else { // Gate (mode == 2)
                if (envDb < threshold) {
                    gainReduction = 0.1f; // -20dB reduction
                }
            }
            
            // Smooth gain changes
            gainHistory[historyIndex] = gainReduction;
            historyIndex = (historyIndex + 1) % ENVELOPE_HISTORY;
            
            float smoothGain = 0.0f;
            for (auto gain : gainHistory) {
                smoothGain += gain;
            }
            smoothGain /= ENVELOPE_HISTORY;
            
            return delayedSignal * smoothGain;
        }
        
        void reset() {
            delayLine.fill(0.0f);
            gainHistory.fill(1.0f);
            delayIndex = historyIndex = 0;
            envelope = 0.0f;
        }
    };
    
    // Oversampling for anti-aliasing
    struct Oversampler {
        static constexpr int FACTOR = 2; // 2x oversampling
        static constexpr int FILTER_ORDER = 64;
        
        // Anti-aliasing filters
        std::array<float, FILTER_ORDER> upFilter;
        std::array<float, FILTER_ORDER> downFilter;
        std::array<float, FILTER_ORDER> upHistory;
        std::array<float, FILTER_ORDER> downHistory;
        
        Oversampler() {
            // Design elliptic anti-aliasing filters
            designAntiAliasingFilter();
            reset();
        }
        
        void designAntiAliasingFilter() {
            // Simple but effective windowed sinc filter
            float cutoff = 0.45f; // Slightly below Nyquist/2
            
            for (int i = 0; i < FILTER_ORDER; ++i) {
                float n = i - (FILTER_ORDER - 1) * 0.5f;
                if (n == 0.0f) {
                    upFilter[i] = downFilter[i] = 2.0f * cutoff;
                } else {
                    float sinc = std::sin(juce::MathConstants<float>::twoPi * cutoff * n) / (juce::MathConstants<float>::pi * n);
                    float window = 0.54f - 0.46f * std::cos(juce::MathConstants<float>::twoPi * i / (FILTER_ORDER - 1));
                    upFilter[i] = downFilter[i] = sinc * window;
                }
            }
        }
        
        void upsample(float input, float* output) {
            // Shift history
            for (int i = FILTER_ORDER - 1; i > 0; --i) {
                upHistory[i] = upHistory[i - 1];
            }
            upHistory[0] = input;
            
            // Generate 2 upsampled outputs
            for (int phase = 0; phase < FACTOR; ++phase) {
                float sum = 0.0f;
                for (int i = 0; i < FILTER_ORDER; ++i) {
                    int index = phase + i * FACTOR;
                    if (index < FILTER_ORDER) {
                        sum += upHistory[i] * upFilter[index];
                    }
                }
                output[phase] = sum * FACTOR;
            }
        }
        
        float downsample(const float* input) {
            // Process FACTOR samples
            float sum = 0.0f;
            for (int phase = 0; phase < FACTOR; ++phase) {
                // Shift history
                for (int i = FILTER_ORDER - 1; i > 0; --i) {
                    downHistory[i] = downHistory[i - 1];
                }
                downHistory[0] = input[phase];
                
                // Filter
                for (int i = 0; i < FILTER_ORDER; ++i) {
                    sum += downHistory[i] * downFilter[i];
                }
            }
            return sum / FACTOR;
        }
        
        void reset() {
            upHistory.fill(0.0f);
            downHistory.fill(0.0f);
        }
    };
    
    // Channel state
    struct ChannelState {
        TPTFilter peakFilter;
        DynamicProcessor dynamicProcessor;
        Oversampler oversampler;
        
        void prepare(double sampleRate) {
            peakFilter.reset();
            dynamicProcessor.reset();
            oversampler.reset();
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
            temperature = 25.0f + std::sin(phase) * 1.5f; // ±1.5°C variation
            
            // Thermal noise increases with temperature
            float noiseLevel = (temperature - 20.0f) * 0.000005f;
            thermalNoise = ((rand() % 1000) / 1000.0f - 0.5f) * noiseLevel;
            
            // Thermal drift affects filter parameters
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
    
    // Analog modeling functions
    float applyAnalogSaturation(float input);
    float applyComponentTolerance(float value, float tolerance);
    float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
    float linearToDb(float linear) { return 20.0f * std::log10(std::max(0.00001f, linear)); }
};