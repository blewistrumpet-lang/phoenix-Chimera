#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class HarmonicExciter : public EngineBase {
public:
    HarmonicExciter();
    ~HarmonicExciter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Harmonic Exciter"; }
    
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
    
    SmoothParam m_frequency;  // Target frequency range
    SmoothParam m_drive;      // Amount of harmonic generation
    SmoothParam m_harmonics;  // Even vs odd harmonics balance
    SmoothParam m_clarity;    // Phase coherence
    SmoothParam m_warmth;     // Low frequency enhancement
    SmoothParam m_presence;   // High frequency enhancement
    SmoothParam m_color;      // Tube vs transistor character
    SmoothParam m_mix;        // Dry/wet mix
    
    // Multiband processing
    struct Band {
        // Linkwitz-Riley crossover filters
        struct CrossoverFilter {
            float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
            float b1 = 0.0f, b2 = 0.0f;
            float s1 = 0.0f, s2 = 0.0f;
            
            void setFrequency(float freq, double sampleRate) {
                float omega = 2.0f * M_PI * freq / sampleRate;
                float cosOmega = std::cos(omega);
                float sinOmega = std::sin(omega);
                float q = 0.7071f; // Butterworth Q
                
                float alpha = sinOmega / (2.0f * q);
                
                // Lowpass coefficients
                b1 = 2.0f * (alpha * alpha - 1.0f);
                b2 = 1.0f - 2.0f * alpha;
                a0 = alpha * alpha;
                a1 = 2.0f * alpha * alpha;
                a2 = alpha * alpha;
                
                float norm = 1.0f / (1.0f + 2.0f * alpha + alpha * alpha);
                a0 *= norm;
                a1 *= norm;
                a2 *= norm;
                b1 *= norm;
                b2 *= norm;
            }
            
            float processLowpass(float input) {
                float output = a0 * input + a1 * s1 + a2 * s2 - b1 * s1 - b2 * s2;
                s2 = s1;
                s1 = input;
                return output;
            }
            
            float processHighpass(float input, float lowpass) {
                return input - lowpass;
            }
        };
        
        CrossoverFilter filter1, filter2; // 4th order
        
        float processLowpass(float input) {
            return filter2.processLowpass(filter1.processLowpass(input));
        }
        
        float processHighpass(float input) {
            float lp1 = filter1.processLowpass(input);
            float hp1 = filter1.processHighpass(input, lp1);
            float lp2 = filter2.processLowpass(hp1);
            return filter2.processHighpass(hp1, lp2);
        }
    };
    
    // Harmonic generator
    struct HarmonicGenerator {
        float lastSample = 0.0f;
        float integrator = 0.0f;
        
        float generateTubeHarmonics(float input, float drive) {
            // Tube-style saturation (even harmonics)
            float biased = input + drive * 0.1f; // Asymmetric bias
            float saturated = std::tanh(biased * (1.0f + drive * 3.0f));
            
            // Add 2nd harmonic emphasis
            float squared = input * input * (input > 0 ? 1.0f : -1.0f);
            return saturated * 0.8f + squared * drive * 0.2f;
        }
        
        float generateTubeHarmonicsWithAging(float input, float drive, float aging) {
            float basic = generateTubeHarmonics(input, drive);
            
            // Aging affects tube characteristics
            if (aging > 0.01f) {
                // Tubes become more asymmetric with age
                float asymmetry = aging * 0.15f;
                if (basic > 0) {
                    basic *= (1.0f + asymmetry);
                } else {
                    basic *= (1.0f - asymmetry * 0.7f);
                }
                
                // Add more even harmonics with aging
                basic += aging * 0.1f * input * input * (input > 0 ? 1.0f : -1.0f);
            }
            
            return basic;
        }
        
        float generateTransistorHarmonics(float input, float drive) {
            // Transistor-style saturation (odd harmonics)
            float clipped = std::tanh(input * (1.0f + drive * 4.0f));
            
            // Add crossover distortion for odd harmonics
            float crossover = input;
            if (std::abs(input) < 0.1f) {
                crossover *= 0.5f + drive * 0.5f;
            }
            
            return clipped * 0.7f + crossover * 0.3f;
        }
        
        float generateTransistorHarmonicsWithAging(float input, float drive, float aging) {
            float basic = generateTransistorHarmonics(input, drive);
            
            // Aging affects transistor characteristics
            if (aging > 0.01f) {
                // Transistors develop more crossover distortion with age
                float crossoverThreshold = 0.1f * (1.0f + aging * 2.0f);
                if (std::abs(input) < crossoverThreshold) {
                    basic *= (0.5f - aging * 0.2f);
                }
                
                // Add more odd harmonics with aging
                basic += aging * 0.08f * input * input * input;
            }
            
            return basic;
        }
        
        float process(float input, float drive, float color) {
            // Blend between tube and transistor characteristics
            float tube = generateTubeHarmonics(input, drive);
            float transistor = generateTransistorHarmonics(input, drive);
            
            return tube * (1.0f - color) + transistor * color;
        }
        
        float processWithAging(float input, float drive, float color, float aging) {
            // Blend between tube and transistor characteristics with aging
            float tube = generateTubeHarmonicsWithAging(input, drive, aging);
            float transistor = generateTransistorHarmonicsWithAging(input, drive, aging);
            
            return tube * (1.0f - color) + transistor * color;
        }
    };
    
    // Channel state
    struct ChannelState {
        // Three-band processing
        Band lowBand, midBand, highBand;
        HarmonicGenerator lowGen, midGen, highGen;
        
        // Enhancement filters
        float presenceState = 0.0f;
        float warmthState = 0.0f;
        
        // Phase alignment
        float phaseHistory[4] = {0};
        int phaseIndex = 0;
        
        // DC blocker
        float dcBlockerState = 0.0f;
        
        // Component aging and thermal effects
        float componentDrift = 0.0f;
        float thermalNoise = 0.0f;
        float noiseLevel = 0.0f;
        
        void updateAging(float aging) {
            componentDrift = aging * 0.02f; // 2% max drift
            thermalNoise = aging * 0.003f;  // Thermal fluctuations
            noiseLevel = aging * 0.001f;    // Subtle noise floor
        }
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
    
    // Helper functions
    float processPresenceFilter(float input, float& state);
    float processWarmthFilter(float input, float& state);
    float processDCBlocker(float input, float& state);
    
    // Enhanced processing with aging and thermal effects
    float processPresenceFilterWithAging(float input, float& state, float aging, float thermalFactor);
    float processWarmthFilterWithAging(float input, float& state, float aging, float thermalFactor);
    
    // Oversampling for cleaner harmonic generation
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