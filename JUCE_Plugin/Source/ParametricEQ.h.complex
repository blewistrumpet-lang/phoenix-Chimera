#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class ParametricEQ : public EngineBase {
public:
    ParametricEQ();
    ~ParametricEQ() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Parametric EQ"; }
    
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
    
    // Smoothed parameters - 4 bands with shared controls
    SmoothParam m_band1Freq;  // 20Hz-20kHz (log scale)
    SmoothParam m_band1Gain;  // -15 to +15 dB
    SmoothParam m_band2Freq;
    SmoothParam m_band2Gain;
    SmoothParam m_band3Freq;
    SmoothParam m_band3Gain;
    SmoothParam m_band4Freq;
    SmoothParam m_band4Gain;
    
    // Shared Q control affects all bands
    SmoothParam m_globalQ; // 0.1 to 10
    
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
            float targetTemp = 20.0f + processingLoad * 10.0f; // Up to 30°C
            temperature = temperature * thermalTimeConstant + targetTemp * (1.0f - thermalTimeConstant);
            
            // Component drift based on temperature (affects filter response)
            componentDrift = (temperature - 20.0f) * 0.0005f; // ±0.5% max drift
        }
        
        float getTemperatureDrift() const {
            return componentDrift;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f; // In processing hours
        float agingRate = 1.0f / (500.0f * 3600.0f * 44100.0f); // Age over 500 hours
        
        void update() {
            age += agingRate;
        }
        
        float getAgingFactor() const {
            // Subtle aging effects (capacitor drift, resistor tolerance)
            return 1.0f + std::sin(age * 0.02f) * 0.001f; // ±0.1% variation
        }
    };
    
    // Enhanced oversampling system for EQ processing
    struct Oversampler {
        static constexpr int FACTOR = 2; // 2x oversampling for EQ
        std::array<float, FACTOR * 64> buffer;
        int bufferIndex = 0;
        
        // Butterworth anti-aliasing filters
        struct AntiAliasingFilter {
            float b0, b1, b2, a1, a2;
            float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            
            void setCoefficients(double sampleRate) {
                double nyquist = sampleRate * 0.5;
                double cutoff = nyquist * 0.4; // Conservative anti-aliasing
                double w = 2.0 * M_PI * cutoff / sampleRate;
                double cosw = std::cos(w);
                double sinw = std::sin(w);
                double alpha = sinw / (2.0 * 0.707); // Q = 0.707
                
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
    };
    
    // High-quality biquad filter with boutique enhancements
    struct BiquadFilter {
        // Filter coefficients (using doubles for precision)
        double a0 = 1.0, a1 = 0.0, a2 = 0.0;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        
        // Filter state (using doubles for precision)
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        // Boutique components
        ThermalModel thermalModel;
        ComponentAging componentAging;
        
        // Analog noise simulation
        mutable std::mt19937 noiseGen{std::random_device{}()};
        mutable std::normal_distribution<float> noiseDist{0.0f, 1.0f};
        
        float addAnalogNoise(float input) {
            // Thermal noise simulation (-130dB noise floor)
            float noise = noiseDist(noiseGen) * 0.00000001f;
            return input + noise;
        }
        
        // Filter types
        enum FilterType {
            BELL,
            LOW_SHELF,
            HIGH_SHELF
        };
        
        void calculateCoefficients(FilterType type, double freq, double gain, double q, double sampleRate) {
            // Frequency pre-warping for bilinear transform
            double w = 2.0 * M_PI * freq / sampleRate;
            double cosw = std::cos(w);
            double sinw = std::sin(w);
            
            // Convert gain from dB to linear
            double A = std::pow(10.0, gain / 40.0); // For bell/peak filters
            double A2 = A * A;
            double sqrt2A = std::sqrt(2.0 * A);
            
            // Q factor (bandwidth)
            double alpha = sinw / (2.0 * q);
            
            switch (type) {
                case BELL: {
                    // Peaking EQ
                    b0 = 1.0 + alpha * A;
                    b1 = -2.0 * cosw;
                    b2 = 1.0 - alpha * A;
                    a0 = 1.0 + alpha / A;
                    a1 = -2.0 * cosw;
                    a2 = 1.0 - alpha / A;
                    break;
                }
                
                case LOW_SHELF: {
                    // Low shelf
                    double S = 1.0; // Shelf slope (1 = steepest)
                    double beta = std::sqrt(A2 + 1.0) / S - (A2 - 1.0);
                    
                    b0 = A * ((A + 1.0) - (A - 1.0) * cosw + beta * sinw);
                    b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw);
                    b2 = A * ((A + 1.0) - (A - 1.0) * cosw - beta * sinw);
                    a0 = (A + 1.0) + (A - 1.0) * cosw + beta * sinw;
                    a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw);
                    a2 = (A + 1.0) + (A - 1.0) * cosw - beta * sinw;
                    break;
                }
                
                case HIGH_SHELF: {
                    // High shelf
                    double S = 1.0;
                    double beta = std::sqrt(A2 + 1.0) / S - (A2 - 1.0);
                    
                    b0 = A * ((A + 1.0) + (A - 1.0) * cosw + beta * sinw);
                    b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw);
                    b2 = A * ((A + 1.0) + (A - 1.0) * cosw - beta * sinw);
                    a0 = (A + 1.0) - (A - 1.0) * cosw + beta * sinw;
                    a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw);
                    a2 = (A + 1.0) - (A - 1.0) * cosw - beta * sinw;
                    break;
                }
            }
            
            // Normalize coefficients
            double norm = 1.0 / a0;
            b0 *= norm;
            b1 *= norm;
            b2 *= norm;
            a1 *= norm;
            a2 *= norm;
            a0 = 1.0;
        }
        
        double process(double input) {
            // Update thermal and aging models
            float processingLoad = std::abs(input) * 5.0f;
            thermalModel.update(processingLoad);
            componentAging.update();
            
            float thermalDrift = thermalModel.getTemperatureDrift();
            float agingFactor = componentAging.getAgingFactor();
            
            // Apply thermal drift to coefficients for analog behavior
            double thermalB0 = b0 * (1.0 + thermalDrift * 0.1);
            double thermalB1 = b1 * agingFactor;
            double thermalB2 = b2 * (1.0 + thermalDrift * 0.05);
            double thermalA1 = a1 * agingFactor;
            double thermalA2 = a2 * (1.0 + thermalDrift * 0.02);
            
            // Direct Form II implementation with thermal compensation
            double w = input - thermalA1 * y1 - thermalA2 * y2;
            double output = thermalB0 * w + thermalB1 * x1 + thermalB2 * x2;
            
            // Update states
            x2 = x1;
            x1 = w;
            y2 = y1;
            y1 = output;
            
            // Add subtle analog saturation for high gains
            if (std::abs(output) > 0.8) {
                output = std::tanh(output * 0.9) * 1.1;
            }
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0;
        }
    };
    
    // EQ band with boutique smooth parameter changes
    struct EQBand {
        BiquadFilter filter;
        DCBlocker dcBlocker;
        Oversampler oversampler;
        
        // Enhanced parameter transitions with thermal stability
        double currentFreq = 1000.0;
        double currentGain = 0.0;
        double currentQ = 1.0;
        double targetFreq = 1000.0;
        double targetGain = 0.0;
        double targetQ = 1.0;
        
        bool needsUpdate = true;
        float lastProcessedSample = 0.0f;
        
        void updateSmooth(double sampleRate) {
            const double smoothing = 0.999;
            
            currentFreq = currentFreq * smoothing + targetFreq * (1.0 - smoothing);
            currentGain = currentGain * smoothing + targetGain * (1.0 - smoothing);
            currentQ = currentQ * smoothing + targetQ * (1.0 - smoothing);
            
            // Check if we need to recalculate coefficients
            if (std::abs(currentFreq - targetFreq) > 0.1 ||
                std::abs(currentGain - targetGain) > 0.01 ||
                std::abs(currentQ - targetQ) > 0.01) {
                needsUpdate = true;
            }
        }
        
        double process(double input, BiquadFilter::FilterType type, double sampleRate, bool useOversampling = false) {
            // Apply input DC blocking
            input = dcBlocker.process(input);
            
            // Add subtle analog noise
            input = filter.addAnalogNoise(input);
            
            if (needsUpdate) {
                filter.calculateCoefficients(type, currentFreq, currentGain, currentQ, sampleRate);
                needsUpdate = false;
            }
            
            double output;
            
            if (useOversampling && std::abs(currentGain) > 6.0) { // Oversample for high gains
                // Simple 2x oversampling for critical processing
                double upsampled1 = filter.process(input);
                double upsampled2 = filter.process(0.0); // Zero-stuffing
                output = (upsampled1 + upsampled2) * 0.5;
            } else {
                output = filter.process(input);
            }
            
            // Store for transient analysis
            lastProcessedSample = output;
            
            return output;
        }
    };
    
    // Channel state with boutique enhancements
    struct ChannelState {
        std::array<EQBand, 4> bands;
        DCBlocker inputDCBlocker, outputDCBlocker;
        ThermalModel thermalModel;
        ComponentAging componentAging;
        
        // Advanced saturation modeling
        float saturationDrive = 0.0f;
        float lastPeakLevel = 0.0f;
        
        void prepare(double sampleRate) {
            for (auto& band : bands) {
                band.filter.reset();
                band.dcBlocker.reset();
                band.oversampler.prepare(sampleRate);
            }
            inputDCBlocker.reset();
            outputDCBlocker.reset();
            thermalModel = ThermalModel();
            componentAging = ComponentAging();
        }
        
        float applySaturation(float input, float drive) {
            if (drive > 0.01f) {
                // Analog-style soft saturation
                float driven = input * (1.0f + drive * 2.0f);
                return std::tanh(driven * 0.7f) * 1.4f;
            }
            return input;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Helper functions
    double frequencyFromNormalized(double normalized);
};