#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <cmath>
#include <random>
#include <limits>

class VintageOptoCompressor : public EngineBase {
public:
    VintageOptoCompressor();
    ~VintageOptoCompressor() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Vintage Opto"; }
    
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
    SmoothParam m_gain;         // Input gain (0 to 40dB)
    SmoothParam m_peakReduction; // Amount of compression (0 to 100%)
    SmoothParam m_emphasis;      // High frequency emphasis (off/on)
    SmoothParam m_outputGain;    // Output gain (-20 to +20dB)
    SmoothParam m_mix;          // Dry/wet mix
    SmoothParam m_knee;         // Compression knee softness
    SmoothParam m_harmonics;    // Tube harmonic coloration
    SmoothParam m_stereoLink;   // Stereo linking amount
    
    // Opto cell simulation
    struct OptoCell {
        // Light-dependent resistor characteristics
        float brightness = 0.0f;          // Current light brightness
        float resistance = 1000000.0f;    // Current resistance (ohms)
        
        // Time constants (LA-2A characteristics) with thermal modeling
        float attackTime = 10.0f;         // ~10ms attack
        float releaseTime = 60.0f;        // Initial release ~60ms
        float releaseMultiplier = 1.0f;   // Increases with compression
        float thermalTimeFactor = 1.0f;   // Temperature affects timing
        
        // Memory effect (program dependent release)
        float compressionMemory = 0.0f;
        float memoryDecay = 0.999f;
        
        void updateBrightness(float targetBrightness, double sampleRate) {
            float rate;
            
            if (targetBrightness > brightness) {
                // Attack (light getting brighter) with thermal factor
                float thermalAttack = attackTime * thermalTimeFactor;
                rate = 1.0f - std::exp(-1.0f / (thermalAttack * 0.001f * sampleRate));
            } else {
                // Release (light dimming) - program dependent with thermal factor
                float effectiveRelease = releaseTime * (1.0f + compressionMemory * 4.0f) * thermalTimeFactor;
                rate = 1.0f - std::exp(-1.0f / (effectiveRelease * 0.001f * sampleRate));
            }
            
            brightness += (targetBrightness - brightness) * rate;
            
            // Update compression memory
            if (targetBrightness > 0.5f) {
                compressionMemory = targetBrightness;
            } else {
                compressionMemory *= memoryDecay;
            }
            
            // Convert brightness to resistance (inverse relationship)
            // Typical LDR: 10k ohms in light, 1M ohms in dark
            resistance = 10000.0f + (990000.0f * (1.0f - brightness));
        }
        
        float getGainReduction() {
            // Voltage divider ratio simulating the T4 opto cell
            float ratio = 100000.0f / (100000.0f + resistance);
            return 1.0f - ratio;  // Invert for gain reduction
        }
    };
    
    // Tube stage simulation
    struct TubeStage {
        // Simple tube transfer curve
        float process(float input, float drive, VintageOptoCompressor* parent) {
            if (drive < 0.01f) return input;
            
            // Safety check input
            input = parent->safeFloat(input);
            drive = std::clamp(drive, 0.0f, 1.0f);
            
            // Asymmetric clipping (tube-like)
            float positive = input > 0.0f ? input : 0.0f;
            float negative = input < 0.0f ? -input : 0.0f;
            
            // Different curves for positive and negative
            positive = parent->safeFloat(std::tanh(parent->safeFloat(positive * (1.0f + drive * 2.0f))));
            negative = parent->safeFloat(std::tanh(parent->safeFloat(negative * (1.0f + drive * 1.5f))));
            
            float output = parent->safeFloat(positive - negative);
            
            // Add even harmonics
            float harmonic2 = output * output * (output > 0 ? 1.0f : -1.0f);
            output += parent->safeFloat(harmonic2 * drive * 0.05f);
            
            // Soft saturation
            output = parent->safeFloat(std::tanh(parent->safeFloat(output * 0.7f)) * 1.43f);
            
            return parent->safeFloat(output);
        }
    };
    
    // High frequency emphasis (pre/de-emphasis)
    struct EmphasisFilter {
        // Pre-emphasis boost high frequencies before compression
        // De-emphasis cuts them after compression
        float state = 0.0f;
        float cutoff = 0.15f;  // ~1kHz at 44.1kHz
        
        float processPreEmphasis(float input) {
            // High-pass characteristic
            float output = input - state;
            state += output * cutoff;
            return input + output * 0.5f;  // Boost highs
        }
        
        float processDeEmphasis(float input) {
            // Low-pass characteristic
            state += (input - state) * cutoff;
            return state;
        }
        
        void reset() {
            state = 0.0f;
        }
    };
    
    // Smooth gain changes
    struct GainSmoother {
        float currentGain = 1.0f;
        
        float process(float targetGain) {
            // Very smooth gain changes for vintage feel
            const float smoothing = 0.9995f;
            currentGain = currentGain * smoothing + targetGain * (1.0f - smoothing);
            return currentGain;
        }
    };
    
    // Peak detector with RMS characteristics
    struct PeakDetector {
        static constexpr int RMS_WINDOW = 128;
        std::array<float, RMS_WINDOW> buffer;
        int index = 0;
        float sum = 0.0f;
        
        float detect(float input) {
            // Remove old value from sum
            sum -= buffer[index];
            
            // Add new value
            float squared = input * input;
            buffer[index] = squared;
            sum += squared;
            
            // Advance index
            index = (index + 1) % RMS_WINDOW;
            
            // Return RMS value
            return std::sqrt(sum / RMS_WINDOW);
        }
        
        void reset() {
            buffer.fill(0.0f);
            sum = 0.0f;
            index = 0;
        }
    };
    
    // Channel state
    struct ChannelState {
        OptoCell optoCell;
        TubeStage tubeStage;
        EmphasisFilter preEmphasis;
        EmphasisFilter deEmphasis;
        GainSmoother gainSmoother;
        PeakDetector peakDetector;
        
        float prevSample = 0.0f;
        
        void prepare() {
            optoCell = OptoCell();
            preEmphasis.reset();
            deEmphasis.reset();
            peakDetector.reset();
            prevSample = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Shared stereo reduction
    float m_stereoReduction = 0.0f;
    
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
        
        void update(double sampleRate, std::mt19937& randomEngine, std::uniform_real_distribution<float>& uniformDist) {
            // Slow temperature variations
            static float phase = 0.0f;
            phase += 0.00001f / sampleRate; // Very slow variation
            temperature = 25.0f + std::sin(phase) * 2.0f; // ±2°C variation
            
            // Thermal noise increases with temperature
            float noiseLevel = (temperature - 20.0f) * 0.00001f;
            thermalNoise = uniformDist(randomEngine) * 0.5f * noiseLevel;
            
            // Thermal drift affects parameters
            thermalDrift = (temperature - 25.0f) * 0.001f;
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalDrift;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // Thread-safe random number generation
    mutable std::mt19937 m_randomEngine;
    mutable std::uniform_real_distribution<float> m_uniformDist;
    
    // Component aging simulation
    float m_componentAge = 0.0f; // In hours of operation
    
    void updateComponentAging(double sampleRate) {
        // Age components very slowly (1 hour = 3600 seconds)
        m_componentAge += 1.0f / (sampleRate * 3600.0f);
    }
    
    // Helper functions
    float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
    float linearToDb(float linear) { return 20.0f * std::log10(std::max(0.00001f, linear)); }
    float softKnee(float input, float threshold, float knee);
    float safeFloat(float value) const;
    bool isChannelValid(int channel, int maxChannels) const;
    
    // Analog modeling
    float applyAnalogNoise(float input);
    float applyVintageWarmth(float input, float amount);
};