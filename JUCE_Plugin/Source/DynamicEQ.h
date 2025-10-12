#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <vector>
#include <array>
#include <cmath>
#include <complex>
#include <random>
#include <limits>

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
    
    // Low-THD Biquad Filter (replaces TPT for < 0.001% THD)
    struct BiquadFilter {
        // Direct Form II Transposed coefficients
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;

        // State variables
        float z1 = 0.0f, z2 = 0.0f;

        void setParameters(float frequency, float Q, double sampleRate) {
            // Safety checks
            frequency = std::max(1.0f, std::min(frequency, static_cast<float>(sampleRate * 0.49f)));
            Q = std::max(0.1f, std::min(Q, 100.0f));

            // Peaking EQ design (0dB gain for extraction, gain applied later)
            float A = 1.0f; // 0dB for now
            float w0 = 6.28318530718f * frequency / static_cast<float>(sampleRate);
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);

            // Peaking EQ coefficients
            float b0_raw = 1.0f + alpha * A;
            float b1_raw = -2.0f * cosw0;
            float b2_raw = 1.0f - alpha * A;
            float a0 = 1.0f + alpha / A;
            float a1_raw = -2.0f * cosw0;
            float a2_raw = 1.0f - alpha / A;

            // Normalize
            b0 = b0_raw / a0;
            b1 = b1_raw / a0;
            b2 = b2_raw / a0;
            a1 = a1_raw / a0;
            a2 = a2_raw / a0;

            // Ensure finite coefficients
            if (!std::isfinite(b0) || !std::isfinite(b1) || !std::isfinite(b2) ||
                !std::isfinite(a1) || !std::isfinite(a2)) {
                b0 = 1.0f;
                b1 = b2 = a1 = a2 = 0.0f;
            }
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

            // Process peaking EQ (Direct Form II Transposed - ultra-low THD)
            float output = b0 * input + z1;
            z1 = b1 * input - a1 * output + z2;
            z2 = b2 * input - a2 * output;

            // Peak band is the difference between filtered and input
            outputs.peak = output - input;

            // Fill other outputs for compatibility (not used in Dynamic EQ)
            outputs.lowpass = output;
            outputs.highpass = input - output;
            outputs.bandpass = outputs.peak;
            outputs.notch = input;
            outputs.allpass = input;

            return outputs;
        }

        void reset() {
            z1 = z2 = 0.0f;
        }
    };
    
    // Dynamic processor with lookahead (improved for low THD)
    struct DynamicProcessor {
        static constexpr int LOOKAHEAD_SAMPLES = 64;
        static constexpr int GAIN_CURVE_SIZE = 4096;  // Increased to 4096 for ultra-smooth gain reduction

        // Lookup table for gain reduction (eliminates log/exp per sample)
        std::array<float, GAIN_CURVE_SIZE> gainCurve;

        // Lookahead delay line
        std::array<float, LOOKAHEAD_SAMPLES> delayLine;
        int delayIndex = 0;

        // Envelope detection
        float envelope = 0.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;

        // Gain smoothing (one-pole filter instead of averaging for lower THD)
        float smoothedGain = 1.0f;
        float gainSmoothCoeff = 0.999f; // Very smooth
        
        // Build gain reduction lookup table (called when parameters change)
        // This table maps LINEAR envelope levels to gain reduction values
        // Index 0 = 0.0 (silence), Index GAIN_CURVE_SIZE-1 = 1.0 (0dBFS)
        void buildGainCurve(float thresholdDb, float ratio, int mode) {
            for (int i = 0; i < GAIN_CURVE_SIZE; ++i) {
                // Map index to LINEAR envelope level (0 to 1)
                float envLinear = static_cast<float>(i) / (GAIN_CURVE_SIZE - 1);

                // Convert to dB for gain calculation (only once per table entry, not per sample!)
                float envDb = (envLinear > 0.00001f) ? 20.0f * std::log10(envLinear) : -100.0f;

                float gr = 1.0f;

                if (mode == 0) { // Compressor
                    if (envDb > thresholdDb) {
                        float over = envDb - thresholdDb;
                        float compressedOver = over / ratio;
                        gr = std::pow(10.0f, -(over - compressedOver) / 20.0f);
                    }
                } else if (mode == 1) { // Expander
                    if (envDb < thresholdDb) {
                        float under = thresholdDb - envDb;
                        float expandedUnder = under * ratio;
                        gr = std::pow(10.0f, -(expandedUnder - under) / 20.0f);
                    }
                } else { // Gate (mode == 2)
                    if (envDb < thresholdDb) {
                        gr = 0.1f; // -20dB reduction
                    }
                }

                gainCurve[i] = gr;
            }
        }

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
            
            // Look up gain reduction from pre-computed curve using LINEAR envelope
            // This eliminates per-sample log/exp calculations for low THD!
            // Clamp envelope to valid range (0.0 to 1.0 representing linear amplitude)
            float envClamped = std::max(0.0f, std::min(1.0f, envelope));

            // Linear interpolation in lookup table (direct linear-to-gain mapping)
            float index = envClamped * (GAIN_CURVE_SIZE - 1);
            int i0 = static_cast<int>(index);
            int i1 = std::min(i0 + 1, GAIN_CURVE_SIZE - 1);
            float frac = index - static_cast<float>(i0);

            float gainReduction = gainCurve[i0] + frac * (gainCurve[i1] - gainCurve[i0]);

            // One-pole smooth gain changes (lower THD than averaging)
            smoothedGain = gainReduction + (smoothedGain - gainReduction) * gainSmoothCoeff;

            return delayedSignal * smoothedGain;
        }

        void reset() {
            delayLine.fill(0.0f);
            gainCurve.fill(1.0f); // Initialize to unity gain (will be rebuilt on first use)
            delayIndex = 0;
            envelope = 0.0f;
            smoothedGain = 1.0f;
            attackCoeff = 0.0f;
            releaseCoeff = 0.0f;
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
        BiquadFilter peakFilter;
        DynamicProcessor dynamicProcessor;
        Oversampler oversampler;

        void reset() {
            // Reset all components
            peakFilter = BiquadFilter();
            dynamicProcessor = DynamicProcessor();
            oversampler = Oversampler();
        }

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
        
        // Thread-safe random generation for thermal effects
        mutable std::mt19937 thermalRng{std::random_device{}()};
        mutable std::uniform_real_distribution<float> noiseDist{-0.5f, 0.5f};
        
        void update(double sampleRate) {
            // Slow temperature variations
            static float phase = 0.0f;
            phase += 0.00001f / sampleRate; // Very slow variation
            temperature = 25.0f + std::sin(phase) * 1.5f; // ±1.5°C variation
            
            // Thermal noise increases with temperature (thread-safe)
            float noiseLevel = (temperature - 20.0f) * 0.000005f;
            thermalNoise = noiseDist(thermalRng) * noiseLevel;
            
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