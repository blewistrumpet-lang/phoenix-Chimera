#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <deque>
#include <random>

class MasteringLimiter : public EngineBase {
public:
    MasteringLimiter();
    ~MasteringLimiter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Mastering Limiter"; }
    
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
    SmoothParam m_threshold;     // -12 to 0 dB
    SmoothParam m_release;       // 1ms to 1000ms
    SmoothParam m_lookahead;     // 0 to 10ms
    SmoothParam m_ceiling;       // Output ceiling (-3 to 0 dB)
    SmoothParam m_softKnee;      // Knee width (0 = hard, 1 = soft)
    SmoothParam m_truePeak;      // True peak detection on/off
    SmoothParam m_character;     // Clean to colored
    SmoothParam m_makeupGain;    // Auto makeup gain on/off
    
    // Lookahead delay buffer
    struct DelayBuffer {
        std::vector<float> buffer;
        int writeIndex = 0;
        int size = 0;
        
        void prepare(int delaySize) {
            size = delaySize;
            buffer.resize(size);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writeIndex = 0;
        }
        
        void write(float sample) {
            buffer[writeIndex] = sample;
            writeIndex = (writeIndex + 1) % size;
        }
        
        float read(int samplesAgo) {
            int readIndex = (writeIndex - samplesAgo + size) % size;
            return buffer[readIndex];
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
            // Simulate thermal buildup from limiting activity
            float targetTemp = 20.0f + processingLoad * 25.0f; // Up to 45°C
            temperature = temperature * thermalTimeConstant + targetTemp * (1.0f - thermalTimeConstant);
            
            // Component drift affects threshold stability
            componentDrift = (temperature - 20.0f) * 0.0002f; // ±0.5% max drift
        }
        
        float getTemperatureDrift() const {
            return componentDrift;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f; // In processing hours
        float agingRate = 1.0f / (1000.0f * 3600.0f * 44100.0f); // Age over 1000 hours
        
        void update() {
            age += agingRate;
        }
        
        float getAgingFactor() const {
            // Subtle aging effects (capacitor drift, VCA characteristics)
            return 1.0f + std::sin(age * 0.01f) * 0.0005f; // ±0.05% variation
        }
    };
    
    // Enhanced true peak detection with advanced oversampling
    struct TruePeakDetector {
        static constexpr int OVERSAMPLE_FACTOR = 8; // Increased for better accuracy
        std::array<float, OVERSAMPLE_FACTOR * 8> upsampleBuffer;
        int bufferIndex = 0;
        
        // Lagrange interpolation for superior quality
        struct LagrangeInterpolator {
            float interpolate(const std::array<float, 4>& samples, float position) {
                float result = 0.0f;
                for (int i = 0; i < 4; ++i) {
                    float term = samples[i];
                    for (int j = 0; j < 4; ++j) {
                        if (i != j) {
                            term *= (position - j) / (i - j);
                        }
                    }
                    result += term;
                }
                return result;
            }
        };
        
        LagrangeInterpolator interpolator;
        
        float detectTruePeak(float input) {
            // Store input sample
            upsampleBuffer[bufferIndex] = input;
            bufferIndex = (bufferIndex + 1) % 8;
            
            float peak = std::abs(input);
            
            // Advanced interpolation for true peak detection
            if (bufferIndex >= 4) {
                std::array<float, 4> samples;
                for (int i = 0; i < 4; ++i) {
                    samples[i] = upsampleBuffer[(bufferIndex - 4 + i + 8) % 8];
                }
                
                // Check interpolated values between samples
                for (int i = 1; i < OVERSAMPLE_FACTOR; ++i) {
                    float position = static_cast<float>(i) / OVERSAMPLE_FACTOR + 1.0f;
                    float interpolated = interpolator.interpolate(samples, position);
                    peak = std::max(peak, std::abs(interpolated));
                }
            }
            
            return peak;
        }
    };
    
    // Smooth gain computer
    struct GainComputer {
        float currentGain = 1.0f;
        float attackTime = 0.0f;
        float releaseTime = 50.0f;
        
        float process(float targetGain, double sampleRate) {
            float rate;
            
            if (targetGain < currentGain) {
                // Attack (gain reduction)
                rate = 1.0f - std::exp(-1.0f / (attackTime * 0.001f * sampleRate));
            } else {
                // Release (gain recovery)
                rate = 1.0f - std::exp(-1.0f / (releaseTime * 0.001f * sampleRate));
            }
            
            currentGain = targetGain + (currentGain - targetGain) * (1.0f - rate);
            return currentGain;
        }
        
        void reset() {
            currentGain = 1.0f;
        }
    };
    
    // Soft knee curve
    struct SoftKnee {
        float process(float input, float threshold, float kneeWidth) {
            if (kneeWidth <= 0.0f) {
                // Hard knee
                return (input > threshold) ? threshold : input;
            }
            
            float kneeStart = threshold - kneeWidth * 0.5f;
            float kneeEnd = threshold + kneeWidth * 0.5f;
            
            if (input <= kneeStart) {
                return input;
            } else if (input >= kneeEnd) {
                return threshold;
            } else {
                // Smooth transition in knee region
                float kneePosition = (input - kneeStart) / kneeWidth;
                float kneeCurve = kneePosition * kneePosition;
                return kneeStart + kneeCurve * (threshold - kneeStart);
            }
        }
    };
    
    // Channel state with boutique enhancements
    struct ChannelState {
        DelayBuffer lookaheadBuffer;
        TruePeakDetector truePeakDetector;
        GainComputer gainComputer;
        
        // Peak hold for lookahead
        std::deque<float> peakHistory;
        float currentPeak = 0.0f;
        
        // Boutique components
        DCBlocker inputDCBlocker, outputDCBlocker;
        ThermalModel thermalModel;
        ComponentAging componentAging;
        
        // Enhanced harmonic generation
        float prevSample = 0.0f;
        float evenHarmonicState = 0.0f;
        float oddHarmonicState = 0.0f;
        
        // Advanced envelope analysis
        float envelopeFollower = 0.0f;
        float spectralCentroid = 0.0f;
        
        // Analog noise simulation
        mutable std::mt19937 noiseGen{std::random_device{}()};
        mutable std::normal_distribution<float> noiseDist{0.0f, 1.0f};
        
        float addAnalogNoise(float input) {
            // Thermal noise simulation (-135dB noise floor)
            float noise = noiseDist(noiseGen) * 0.000000001f;
            return input + noise;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Shared stereo linking
    float m_stereoLinkGain = 1.0f;
    
    SoftKnee m_softKneeProcessor;
    
    // Enhanced helper functions
    float calculateGainReduction(float peak, float threshold, float ceiling, float thermalDrift);
    float addAdvancedHarmonicColor(ChannelState& state, float input, float amount, double sampleRate);
    float applyAnalogSaturation(float input, float drive, float temperature);
    void updateSpectralAnalysis(ChannelState& state, float input, double sampleRate);
};