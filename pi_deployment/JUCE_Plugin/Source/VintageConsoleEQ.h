#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <cmath>
#include <random>

class VintageConsoleEQ : public EngineBase {
public:
    VintageConsoleEQ();
    ~VintageConsoleEQ() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 11; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Vintage Console EQ"; }
    
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
    
    SmoothParam m_lowGain;       // Low shelf gain (+/- 15dB)
    SmoothParam m_lowFreq;       // Low shelf frequency (30-300Hz)
    SmoothParam m_midGain;       // Mid bell gain (+/- 15dB)
    SmoothParam m_midFreq;       // Mid frequency (200Hz-8kHz)
    SmoothParam m_midQ;          // Mid Q width
    SmoothParam m_highGain;      // High shelf gain (+/- 15dB)
    SmoothParam m_highFreq;      // High shelf frequency (3k-16kHz)
    SmoothParam m_drive;         // Console saturation amount
    SmoothParam m_consoleType;   // Console type selector
    SmoothParam m_vintage;       // Vintage character amount
    SmoothParam m_mix;           // Dry/wet mix
    
    // Vintage EQ characteristics
    enum ConsoleType {
        NEVE_1073,    // Musical, transformer-coupled
        API_550,      // Proportional Q, punchy
        SSL_4000,     // Clean, surgical
        PULTEC        // Passive, smooth curves
    };
    
    // Baxandall-style shelving filter
    struct ShelfFilter {
        double a0 = 1.0, a1 = 0.0, a2 = 0.0;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        void calculateLowShelf(double freq, double gain, double sampleRate) {
            double omega = 2.0 * M_PI * freq / sampleRate;
            double cos_omega = std::cos(omega);
            double sin_omega = std::sin(omega);
            
            // Vintage-style shelf with gentle slope
            double A = std::pow(10.0, gain / 40.0);
            double S = 0.7; // Shelf slope factor
            double beta = std::sqrt(A) / S;
            
            b0 = A * ((A + 1.0) - (A - 1.0) * cos_omega + beta * sin_omega);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_omega);
            b2 = A * ((A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega);
            a0 = (A + 1.0) + (A - 1.0) * cos_omega + beta * sin_omega;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_omega);
            a2 = (A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega;
            
            // Normalize
            double norm = 1.0 / a0;
            b0 *= norm;
            b1 *= norm;
            b2 *= norm;
            a1 *= norm;
            a2 *= norm;
            a0 = 1.0;
        }
        
        void calculateHighShelf(double freq, double gain, double sampleRate) {
            double omega = 2.0 * M_PI * freq / sampleRate;
            double cos_omega = std::cos(omega);
            double sin_omega = std::sin(omega);
            
            double A = std::pow(10.0, gain / 40.0);
            double S = 0.7;
            double beta = std::sqrt(A) / S;
            
            b0 = A * ((A + 1.0) + (A - 1.0) * cos_omega + beta * sin_omega);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_omega);
            b2 = A * ((A + 1.0) + (A - 1.0) * cos_omega - beta * sin_omega);
            a0 = (A + 1.0) - (A - 1.0) * cos_omega + beta * sin_omega;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_omega);
            a2 = (A + 1.0) - (A - 1.0) * cos_omega - beta * sin_omega;
            
            double norm = 1.0 / a0;
            b0 *= norm;
            b1 *= norm;
            b2 *= norm;
            a1 *= norm;
            a2 *= norm;
            a0 = 1.0;
        }
        
        double process(double input) {
            double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            
            x2 = x1;
            x1 = input;
            y2 = y1;
            y1 = output;
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0;
        }
    };
    
    // Proportional-Q bell filter (API-style)
    struct ProportionalQFilter {
        double a0 = 1.0, a1 = 0.0, a2 = 0.0;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        void calculate(double freq, double gain, double q, double sampleRate) {
            // Proportional Q - narrower at higher gains
            double actualQ = q * (1.0 + std::abs(gain) / 15.0);
            
            double omega = 2.0 * M_PI * freq / sampleRate;
            double cos_omega = std::cos(omega);
            double sin_omega = std::sin(omega);
            double alpha = sin_omega / (2.0 * actualQ);
            
            double A = std::pow(10.0, gain / 40.0);
            
            // Bell/peaking EQ
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cos_omega;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cos_omega;
            a2 = 1.0 - alpha / A;
            
            double norm = 1.0 / a0;
            b0 *= norm;
            b1 *= norm;
            b2 *= norm;
            a1 *= norm;
            a2 *= norm;
            a0 = 1.0;
        }
        
        double process(double input) {
            double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            
            x2 = x1;
            x1 = input;
            y2 = y1;
            y1 = output;
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0;
        }
    };
    
    // Console saturation (transformer/op-amp modeling)
    struct ConsoleSaturation {
        float prevSample = 0.0f;
        
        float processNeve(float input, float drive) {
            if (drive < 0.01f) return input;
            
            // Transformer saturation with hysteresis
            float diff = input - prevSample;
            prevSample = input;
            
            // Asymmetric saturation (more on positive swings)
            float saturated = input;
            if (input > 0) {
                saturated = std::tanh(input * (1.0f + drive * 2.0f)) / (1.0f + drive);
            } else {
                saturated = std::tanh(input * (1.0f + drive * 1.5f)) / (1.0f + drive);
            }
            
            // Add transformer-style harmonics
            float harmonics = saturated * saturated * (saturated > 0 ? 1.0f : -1.0f);
            saturated += harmonics * drive * 0.05f;
            
            // Frequency-dependent saturation (more on lows)
            float lowFreqEmphasis = diff * drive * 0.1f;
            saturated += lowFreqEmphasis;
            
            return saturated;
        }
        
        float processAPI(float input, float drive) {
            if (drive < 0.01f) return input;
            
            // Op-amp style saturation - cleaner, more controlled
            float threshold = 0.7f;
            float output = input;
            
            if (std::abs(input) > threshold) {
                float excess = std::abs(input) - threshold;
                float compressed = threshold + std::tanh(excess * 2.0f) * 0.3f;
                output = compressed * (input > 0 ? 1.0f : -1.0f);
            }
            
            // Add subtle odd harmonics
            output += std::sin(output * M_PI) * drive * 0.02f;
            
            return output * (1.0f + drive * 0.1f); // Slight level boost
        }
        
        float processSSL(float input, float drive) {
            if (drive < 0.01f) return input;
            
            // Clean, controlled saturation
            return std::tanh(input * (1.0f + drive)) / (1.0f + drive * 0.5f);
        }
    };
    
    // Channel state
    struct ChannelState {
        ShelfFilter lowShelf;
        ProportionalQFilter midBell;
        ShelfFilter highShelf;
        ConsoleSaturation saturation;
        
        // Enhanced component aging and thermal modeling
        float componentAge = 0.0f;
        float thermalDrift = 0.0f;
        float transformerSaturation = 0.0f;
        
        // Smooth parameter changes with component modeling
        double currentLowFreq = 100.0;
        double currentLowGain = 0.0;
        double currentMidFreq = 1000.0;
        double currentMidGain = 0.0;
        double currentMidQ = 1.0;
        double currentHighFreq = 8000.0;
        double currentHighGain = 0.0;
        
        void updateFilters(double sampleRate) {
            // Apply component aging and thermal effects
            double ageAdjustedLowFreq = currentLowFreq * (1.0 + componentAge * 0.05 + thermalDrift);
            double ageAdjustedMidFreq = currentMidFreq * (1.0 + componentAge * 0.03 + thermalDrift);
            double ageAdjustedHighFreq = currentHighFreq * (1.0 + componentAge * 0.02 + thermalDrift);
            
            lowShelf.calculateLowShelf(ageAdjustedLowFreq, currentLowGain, sampleRate);
            midBell.calculate(ageAdjustedMidFreq, currentMidGain, currentMidQ, sampleRate);
            highShelf.calculateHighShelf(ageAdjustedHighFreq, currentHighGain, sampleRate);
        }
        
        void smoothParameters(double targetLowFreq, double targetLowGain,
                            double targetMidFreq, double targetMidGain, double targetMidQ,
                            double targetHighFreq, double targetHighGain) {
            const double smoothing = 0.995;
            
            currentLowFreq = currentLowFreq * smoothing + targetLowFreq * (1.0 - smoothing);
            currentLowGain = currentLowGain * smoothing + targetLowGain * (1.0 - smoothing);
            currentMidFreq = currentMidFreq * smoothing + targetMidFreq * (1.0 - smoothing);
            currentMidGain = currentMidGain * smoothing + targetMidGain * (1.0 - smoothing);
            currentMidQ = currentMidQ * smoothing + targetMidQ * (1.0 - smoothing);
            currentHighFreq = currentHighFreq * smoothing + targetHighFreq * (1.0 - smoothing);
            currentHighGain = currentHighGain * smoothing + targetHighGain * (1.0 - smoothing);
        }
        
        void reset() {
            lowShelf.reset();
            midBell.reset();
            highShelf.reset();
            saturation.prevSample = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    ConsoleType m_currentConsoleType = NEVE_1073;
    
    // Enhanced DC blocking
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        const float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // Thermal modeling for console behavior
    struct ThermalModel {
        float temperature = 25.0f;
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            thermalNoise += (dist(rng) * 0.0006f) / sampleRate;
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
    
    // Component tolerances for vintage behavior
    struct ComponentTolerances {
        float capacitorTolerance = 0.0f;
        float resistorTolerance = 0.0f;
        float inductorTolerance = 0.0f;  // For transformer modeling
        
        ComponentTolerances() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> capDist(-0.1f, 0.1f);
            std::uniform_real_distribution<float> resDist(-0.02f, 0.02f);
            std::uniform_real_distribution<float> indDist(-0.05f, 0.05f);
            
            capacitorTolerance = capDist(gen);
            resistorTolerance = resDist(gen);
            inductorTolerance = indDist(gen);
        }
        
        float adjustFrequency(float freq) const {
            return freq * (1.0f + capacitorTolerance + resistorTolerance);
        }
        
        float adjustGain(float gain) const {
            return gain * (1.0f + resistorTolerance + inductorTolerance * 0.5f);
        }
    };
    
    ComponentTolerances m_componentTolerances;
    
    // Helper functions
    ConsoleType getConsoleType() const;
};