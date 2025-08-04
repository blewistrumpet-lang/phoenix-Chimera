// DigitalDelay.h
#pragma once

#include "EngineBase.h"
#include <array>
#include <memory>
#ifdef __SSE2__
#include <immintrin.h>
#endif

namespace AudioDSP {

// Forward declarations
class DelayLine;
class ParameterSmoother;
class BiquadFilter;
class SoftClipper;
class DCBlocker;
class ModulationProcessor;

class DigitalDelay : public EngineBase {
public:
    DigitalDelay();
    ~DigitalDelay();
    
    // EngineBase interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Digital Delay Pro"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Core DSP components
    std::unique_ptr<DelayLine> m_delayLines[2];
    std::unique_ptr<BiquadFilter> m_filters[2];
    std::unique_ptr<SoftClipper> m_clipper;
    std::unique_ptr<DCBlocker> m_dcBlockers[2];
    std::unique_ptr<ModulationProcessor> m_modulator;
    
    // Parameters
    std::unique_ptr<ParameterSmoother> m_delayTime;
    std::unique_ptr<ParameterSmoother> m_feedback;
    std::unique_ptr<ParameterSmoother> m_mix;
    std::unique_ptr<ParameterSmoother> m_highCut;
    
    // State
    double m_sampleRate = 44100.0;
    bool m_sampleRateChanged = false;
    
    // Crossfeed state for ping-pong
    struct CrossfeedState {
        float leftToRight = 0.0f;
        float rightToLeft = 0.0f;
        float amount = 0.3f;
    } m_crossfeed;
    
    // Processing
    float processSample(float input, int channel);
    void processStereo(float* left, float* right, int numSamples);
    void processMono(float* data, int numSamples);
    
    // Utilities
    inline float sanitizeInput(float x) const noexcept {
        if (!std::isfinite(x)) return 0.0f;
        return std::max(-10.0f, std::min(10.0f, x));
    }
    
    static constexpr float MAX_FEEDBACK = 0.98f;
    static constexpr float MAX_OUTPUT = 0.99f;
    static constexpr int PROCESS_BLOCK_SIZE = 64;
};

// ParameterSmoother.h
class ParameterSmoother {
public:
    ParameterSmoother() = default;
    
    void setSampleRate(double sampleRate) noexcept {
        m_sampleRate = sampleRate;
    }
    
    void setSmoothingTime(float milliseconds) noexcept {
        m_rampLengthSamples = static_cast<int>(milliseconds * 0.001 * m_sampleRate);
        m_rampLengthSamples = std::max(1, m_rampLengthSamples);
    }
    
    void setTargetValue(float newTarget) noexcept {
        if (std::abs(newTarget - m_target) < 1e-8f) {
            m_current = m_target = newTarget;
            m_stepsRemaining = 0;
            return;
        }
        
        m_target = newTarget;
        m_stepSize = (m_target - m_current) / static_cast<float>(m_rampLengthSamples);
        m_stepsRemaining = m_rampLengthSamples;
    }
    
    float getNextValue() noexcept {
        if (m_stepsRemaining > 0) {
            m_current += m_stepSize;
            --m_stepsRemaining;
            
            if (m_stepsRemaining == 0) {
                m_current = m_target;
            }
        }
        
        return m_current + 1e-15f; // Denormal prevention
    }
    
    void reset(float value) noexcept {
        m_current = m_target = value;
        m_stepsRemaining = 0;
    }
    
    float getCurrentValue() const noexcept { return m_current; }
    
private:
    float m_current = 0.0f;
    float m_target = 0.0f;
    float m_stepSize = 0.0f;
    double m_sampleRate = 44100.0;
    int m_rampLengthSamples = 441;
    int m_stepsRemaining = 0;
};

// DelayLine.h
class DelayLine {
public:
    static constexpr size_t BUFFER_SIZE = 262144; // Power of 2
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;
    
    DelayLine();
    ~DelayLine() = default;
    
    void reset() noexcept;
    void write(float sample) noexcept;
    float read(double delaySamples) noexcept;
    float readModulated(double delaySamples, float modulation) noexcept;
    
private:
    // Cache-aligned buffer with ghost samples for wrap-around
    alignas(64) std::array<float, BUFFER_SIZE + 4> m_buffer;
    size_t m_writePos = 0;
    
    // Hermite interpolation
    float hermiteInterpolate(double position) const noexcept;
    
    // Denormal prevention
    static constexpr float DENORMAL_PREVENTION = 1e-25f;
};

// BiquadFilter.h
class BiquadFilter {
public:
    BiquadFilter() = default;
    
    void reset() noexcept;
    void setLowpass(double frequency, double sampleRate, double q = 0.7071) noexcept;
    float processSample(float input) noexcept;
    void processBlock(const float* input, float* output, int numSamples) noexcept;
    
    // SIMD-optimized processing
    void processBlockSIMD(const float* input, float* output, int numSamples) noexcept;
    
private:
    // Double precision for stability
    double m_a0 = 1.0, m_a1 = 0.0, m_a2 = 0.0;
    double m_b1 = 0.0, m_b2 = 0.0;
    double m_x1 = 0.0, m_x2 = 0.0;
    double m_y1 = 0.0, m_y2 = 0.0;
    
    // For SIMD processing
    #ifdef __SSE2__
    __m128d m_coeffs[5]; // a0, a1, a2, b1, b2
    __m128d m_state[4];   // x1, x2, y1, y2
    #endif
};

// SoftClipper.h
class SoftClipper {
public:
    SoftClipper();
    
    void reset() noexcept;
    float processSample(float input) noexcept;
    void processBlock(float* data, int numSamples) noexcept;
    
private:
    // 4x oversampling with polyphase FIR
    class Oversampler {
    public:
        Oversampler();
        void reset() noexcept;
        void upsample(float input, float* output) noexcept;
        float downsample(const float* input) noexcept;
        
    private:
        static constexpr int FILTER_SIZE = 32;
        static constexpr int OVERSAMPLE_FACTOR = 4;
        
        alignas(16) float m_coeffs[FILTER_SIZE];
        alignas(16) float m_state[FILTER_SIZE];
        int m_stateIndex = 0;
    };
    
    std::unique_ptr<Oversampler> m_oversampler;
    
    // Anti-aliased soft clipping curve
    float softClip(float x) const noexcept;
};

// DCBlocker.h
class DCBlocker {
public:
    DCBlocker() = default;
    
    void reset() noexcept {
        m_x1 = m_y1 = 0.0;
    }
    
    float processSample(float input) noexcept {
        double in = static_cast<double>(input);
        double out = in - m_x1 + R * m_y1;
        
        m_x1 = in;
        m_y1 = out;
        
        return static_cast<float>(out + 1e-20); // Denormal prevention
    }
    
private:
    static constexpr double R = 0.9995;
    double m_x1 = 0.0;
    double m_y1 = 0.0;
};

// ModulationProcessor.h
class ModulationProcessor {
public:
    ModulationProcessor();
    
    void setSampleRate(double sampleRate) noexcept;
    void reset() noexcept;
    float process(float rate, float depth) noexcept;
    
private:
    double m_sampleRate = 44100.0;
    float m_phase = 0.0f;
    
    // 2-pole smoothing filter
    std::unique_ptr<BiquadFilter> m_smoothingFilter;
};

} // namespace AudioDSP