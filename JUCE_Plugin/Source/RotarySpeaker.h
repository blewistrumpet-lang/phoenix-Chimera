// RotarySpeaker.h
#pragma once

#include "EngineBase.h"
#include <array>
#include <memory>
#include <cmath>

namespace AudioDSP {

// Forward declarations
class CrossoverFilter;
class DopplerProcessor;
class AmplitudeModulator;
class CabinetSimulator;
class TubePreamp;
class ParameterSmoother;

class RotarySpeaker : public ::EngineBase {
public:
    RotarySpeaker();
    ~RotarySpeaker();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Rotary Speaker Pro"; }
    int getNumParameters() const override { return 6; }
    juce::String getParameterName(int index) const override;
    
private:
    // Core components
    std::unique_ptr<CrossoverFilter> m_crossover[2];
    std::unique_ptr<DopplerProcessor> m_hornDoppler[2];
    std::unique_ptr<DopplerProcessor> m_drumDoppler[2];
    std::unique_ptr<AmplitudeModulator> m_hornAM[2];
    std::unique_ptr<AmplitudeModulator> m_drumAM[2];
    std::unique_ptr<CabinetSimulator> m_cabinet;
    std::unique_ptr<TubePreamp> m_preamp[2];
    
    // Parameter smoothers
    std::unique_ptr<ParameterSmoother> m_speedParam;
    std::unique_ptr<ParameterSmoother> m_mixParam;
    std::unique_ptr<ParameterSmoother> m_driveParam;
    std::unique_ptr<ParameterSmoother> m_micDistanceParam;
    std::unique_ptr<ParameterSmoother> m_stereoWidthParam;
    
    // Rotation state
    struct RotorState {
        double angle = 0.0;
        double velocity = 0.0;
        double targetVelocity = 0.0;
        double acceleration = 2.5; // rad/s²
        
        void update(double deltaTime) noexcept {
            // Smooth acceleration/deceleration
            if (std::abs(velocity - targetVelocity) > 0.01) {
                double diff = targetVelocity - velocity;
                double maxChange = acceleration * deltaTime;
                
                if (std::abs(diff) > maxChange) {
                    velocity += (diff > 0) ? maxChange : -maxChange;
                } else {
                    velocity = targetVelocity;
                }
            }
            
            // Update angle
            angle += velocity * deltaTime;
            
            // Wrap to [0, 2π]
            while (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;
            while (angle < 0.0) angle += 2.0 * M_PI;
        }
    };
    
    RotorState m_hornRotor;
    RotorState m_drumRotor;
    
    // State
    double m_sampleRate = 44100.0;
    bool m_isStopped = false;
    
    // Constants
    static constexpr double HORN_RADIUS = 0.15;  // meters
    static constexpr double DRUM_RADIUS = 0.20;  // meters
    static constexpr double SPEED_OF_SOUND = 343.0; // m/s
    
    // Processing
    void processStereo(float* left, float* right, int numSamples);
    void updateRotorSpeeds();
    
    // Utility
    inline float sanitizeInput(float x) const noexcept {
        if (!std::isfinite(x)) return 0.0f;
        return std::max(-10.0f, std::min(10.0f, x));
    }
    
    float getCurrentValue() const { return 0.0f; } // Placeholder for smoothed params
};

// CrossoverFilter.h - 4th order Linkwitz-Riley
class CrossoverFilter {
public:
    CrossoverFilter() = default;
    
    void setSampleRate(double sampleRate) noexcept {
        m_sampleRate = sampleRate;
    }
    
    void setCrossoverFrequency(double frequency) noexcept {
        if (std::abs(frequency - m_frequency) < 0.1) return;
        
        m_frequency = frequency;
        updateCoefficients();
    }
    
    void reset() noexcept {
        for (auto& stage : m_lowpassStages) stage.reset();
        for (auto& stage : m_highpassStages) stage.reset();
    }
    
    void process(float input, float& lowOut, float& highOut) noexcept {
        // Process through cascaded stages
        float low = input;
        float high = input;
        
        for (auto& stage : m_lowpassStages) {
            low = stage.processLowpass(low);
        }
        
        for (auto& stage : m_highpassStages) {
            high = stage.processHighpass(high);
        }
        
        lowOut = low;
        highOut = high;
    }
    
private:
    struct BiquadStage {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        void reset() noexcept {
            x1 = x2 = y1 = y2 = 0.0;
        }
        
        float processLowpass(float input) noexcept {
            double in = static_cast<double>(input);
            double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            
            x2 = x1; x1 = in;
            y2 = y1; y1 = out;
            
            // Denormal prevention
            return static_cast<float>(out + 1e-20);
        }
        
        float processHighpass(float input) noexcept {
            // Same topology, different coefficients
            return processLowpass(input);
        }
    };
    
    std::array<BiquadStage, 2> m_lowpassStages;
    std::array<BiquadStage, 2> m_highpassStages;
    double m_sampleRate = 44100.0;
    double m_frequency = 800.0;
    
    void updateCoefficients() noexcept;
};

// DopplerProcessor.h - Accurate Doppler shift simulation
class DopplerProcessor {
public:
    static constexpr size_t BUFFER_SIZE = 8192;
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;
    
    DopplerProcessor() {
        reset();
    }
    
    void setSampleRate(double sampleRate) noexcept {
        m_sampleRate = sampleRate;
    }
    
    void reset() noexcept {
        std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
        m_writePos = 0;
        m_prevDelay = 0.0;
        m_delaySmooth = 0.0;
    }
    
    float process(float input, double rotorAngle, double rotorVelocity, 
                  double rotorRadius, double micAngle, double micDistance) noexcept;
    
private:
    alignas(64) std::array<float, BUFFER_SIZE> m_buffer;
    size_t m_writePos = 0;
    double m_sampleRate = 44100.0;
    double m_prevDelay = 0.0;
    double m_delaySmooth = 0.0;
    
    float cubicInterpolate(double position) const noexcept;
};

// AmplitudeModulator.h - Handles amplitude modulation from rotating speaker
class AmplitudeModulator {
public:
    void reset() noexcept {
        m_smoothState = 0.0f;
    }
    
    float process(float input, double rotorAngle, double micAngle, double depth) noexcept;
    
private:
    float m_smoothState = 1.0f;
};

// TubePreamp.h - Authentic tube saturation
class TubePreamp {
public:
    void reset() noexcept;
    void setSampleRate(double sampleRate) noexcept;
    float process(float input, float drive) noexcept;
    
private:
    struct DCBlocker {
        double x1 = 0.0, y1 = 0.0;
        void reset() noexcept { x1 = y1 = 0.0; }
        float process(float input) noexcept;
    };
    
    struct ToneFilter {
        double b0 = 1.0, b1 = 0.0, a1 = 0.0;
        double x1 = 0.0, y1 = 0.0;
        void reset() noexcept { x1 = y1 = 0.0; }
        float process(float input) noexcept;
    };
    
    DCBlocker m_dcBlockerIn, m_dcBlockerOut;
    ToneFilter m_toneFilter;
    double m_sampleRate = 44100.0;
    
    float tubeSaturate(float x) const noexcept;
    void updateToneFilter() noexcept;
};

// CabinetSimulator.h - Leslie cabinet resonances
class CabinetSimulator {
public:
    CabinetSimulator();
    void setSampleRate(double sampleRate) noexcept;
    void reset() noexcept;
    float process(float input) noexcept;
    
private:
    struct Resonance {
        double frequency = 100.0;
        double q = 8.0;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        void reset() noexcept;
        void updateCoefficients(double freq, double sampleRate, double q) noexcept;
        float process(float input) noexcept;
    };
    
    struct SimpleDelay {
        static constexpr size_t MAX_DELAY = 512;
        std::array<float, MAX_DELAY> buffer;
        size_t writePos = 0;
        size_t delayLength = 100;
        
        void reset() noexcept;
        float process(float input) noexcept;
    };
    
    std::array<Resonance, 4> m_resonances;
    SimpleDelay m_reflectionDelay;
    double m_sampleRate = 44100.0;
    
    void updateResonances() noexcept;
};

// ParameterSmoother implementation
class ParameterSmoother {
public:
    void setSampleRate(double sampleRate) noexcept;
    void setSmoothingTime(float milliseconds) noexcept;
    void setTargetValue(float newTarget) noexcept;
    float getNextValue() noexcept;
    void reset(float value) noexcept;
    float getCurrentValue() const noexcept { return m_current; }
    
private:
    float m_current = 0.0f;
    float m_target = 0.0f;
    float m_stepSize = 0.0f;
    double m_sampleRate = 44100.0;
    int m_rampLengthSamples = 441;
    int m_stepsRemaining = 0;
};

} // namespace AudioDSP