// DetuneDoubler.h - Complete Professional Implementation
#pragma once

#include "../Source/EngineBase.h"
#include "DspEngineUtilities.h"
#include <array>
#include <memory>
#include <cmath>
#include <random>
#include <algorithm>
#include <cstdlib>
#include <map>

// Define PI for portability
#ifndef M_PI
static constexpr double M_PI = 3.141592653589793;
#endif

namespace AudioDSP {

// Forward declarations for classes in global AudioDSP namespace
class PitchShifter;

// Forward declarations for DetuneDoublerImpl namespace
namespace DetuneDoublerImpl {
    class DelayLine;
    class BiquadFilter;
    class ParameterSmoother;
    class AllPassNetwork;
    class ModulationGenerator;
}

class DetuneDoubler : public ::EngineBase {
public:
    DetuneDoubler();
    ~DetuneDoubler();
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Detune Doubler"; }
    int getNumParameters() const override { return 5; }
    juce::String getParameterName(int index) const override;
    juce::String getParameterDisplayString(int index, float value) const;
    
private:
    // Core components for each voice
    struct Voice {
        std::unique_ptr<PitchShifter> pitchShifter;
        std::unique_ptr<DetuneDoublerImpl::DelayLine> delay;
        std::unique_ptr<DetuneDoublerImpl::AllPassNetwork> phaseNetwork;
        std::unique_ptr<DetuneDoublerImpl::ModulationGenerator> modulator;
        std::unique_ptr<DetuneDoublerImpl::BiquadFilter> tapeFilter;
        
        void reset();
    };
    
    // Two voices per channel for doubling
    std::array<Voice, 4> m_voices; // L1, L2, R1, R2
    
    // Parameter smoothers
    std::unique_ptr<DetuneDoublerImpl::ParameterSmoother> m_detuneParam;
    std::unique_ptr<DetuneDoublerImpl::ParameterSmoother> m_delayParam;
    std::unique_ptr<DetuneDoublerImpl::ParameterSmoother> m_widthParam;
    std::unique_ptr<DetuneDoublerImpl::ParameterSmoother> m_thicknessParam;
    std::unique_ptr<DetuneDoublerImpl::ParameterSmoother> m_mixParam;
    
    // State
    double m_sampleRate = 44100.0;
    std::mt19937 m_randomGen{42}; // Shared RNG for reproducible results
    
    // Processing
    void processStereo(float* left, float* right, int numSamples);
    
    // Constants
    static constexpr float MAX_DETUNE_CENTS = 50.0f;
    static constexpr float MIN_DELAY_MS = 10.0f;
    static constexpr float MAX_DELAY_MS = 60.0f;
    
    // Utility functions
    inline float clamp01(float value) const noexcept {
        return std::max(0.0f, std::min(1.0f, value));
    }
};

// PitchShifter.h - High-quality pitch shifting using grain overlap
class PitchShifter {
public:
    PitchShifter(std::mt19937& rng) : m_randomGen(rng) {
        reset();
    }
    
    void setSampleRate(double sampleRate) {
        m_sampleRate = sampleRate;
        // Grain size could be adjusted based on sample rate
        // For now, keep it fixed for consistency
    }
    
    void reset() {
        m_buffer.fill(0.0f);
        m_writePos = 0;
        m_grain1Pos = 0.0;
        m_grain2Pos = GRAIN_SIZE * 0.5;
        m_windowPhase = 0.0;
        
        // Pre-fill the buffer with silence to ensure we have valid data to read
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            m_buffer[i] = 0.0f;
        }
        m_writePos = GRAIN_SIZE + 128; // Start writing ahead of the grain read positions
    }
    
    void setPitchShift(float cents) {
        float ratio = std::pow(2.0f, cents / 1200.0f);
        m_pitchRatio = ratio;
        m_readSpeed = ratio;  // Read faster for higher pitch
    }
    
    float process(float input) {
        // Write to circular buffer
        m_buffer[m_writePos] = input;
        m_writePos = (m_writePos + 1) & BUFFER_MASK;
        
        // Process two overlapping grains
        float grain1 = processGrain(m_grain1Pos, 0.0);
        float grain2 = processGrain(m_grain2Pos, 0.5);
        
        // Update grain positions
        m_grain1Pos += 1.0;
        m_grain2Pos += 1.0;
        
        // Wrap grain positions
        if (m_grain1Pos >= GRAIN_SIZE) {
            m_grain1Pos -= GRAIN_SIZE;
            randomizeGrain(m_grain1Pos);
        }
        if (m_grain2Pos >= GRAIN_SIZE) {
            m_grain2Pos -= GRAIN_SIZE;
            randomizeGrain(m_grain2Pos);
        }
        
        return grain1 + grain2;
    }
    
private:
    static constexpr size_t BUFFER_SIZE = 8192;
    static constexpr size_t BUFFER_MASK = BUFFER_SIZE - 1;
    static constexpr size_t GRAIN_SIZE = 2048;
    
    alignas(64) std::array<float, BUFFER_SIZE> m_buffer;
    size_t m_writePos = 0;
    double m_grain1Pos = 0.0;
    double m_grain2Pos = 0.0;
    float m_pitchRatio = 1.0f;
    float m_readSpeed = 1.0f;
    double m_windowPhase = 0.0;
    double m_sampleRate = 44100.0;
    std::mt19937& m_randomGen;
    std::uniform_real_distribution<float> m_grainDist{-2.0f, 2.0f};
    
    float processGrain(double& grainPos, double phaseOffset) {
        // Calculate read position with minimum delay to ensure we're reading valid data
        // Add a small offset to avoid reading data that was just written
        double minDelay = 64.0; // Minimum samples behind write position
        double readPos = m_writePos - GRAIN_SIZE - minDelay + grainPos * m_readSpeed;
        while (readPos < 0) readPos += BUFFER_SIZE;

        // Cubic interpolation for better anti-aliasing
        int idx0 = static_cast<int>(readPos) & BUFFER_MASK;
        int idxm1 = (idx0 - 1) & BUFFER_MASK;
        int idx1 = (idx0 + 1) & BUFFER_MASK;
        int idx2 = (idx0 + 2) & BUFFER_MASK;

        float frac = static_cast<float>(readPos - std::floor(readPos));

        // 4-point cubic interpolation (Hermite)
        float ym1 = m_buffer[idxm1];
        float y0 = m_buffer[idx0];
        float y1 = m_buffer[idx1];
        float y2 = m_buffer[idx2];

        float c0 = y0;
        float c1 = 0.5f * (y1 - ym1);
        float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        float sample = ((c3 * frac + c2) * frac + c1) * frac + c0;

        // Apply improved Hann-Poisson window for better spectral characteristics
        double windowPos = grainPos / GRAIN_SIZE + phaseOffset;
        windowPos = windowPos - std::floor(windowPos);

        // Hann-Poisson window (alpha = 2.0) - reduced sidelobes compared to Hann
        float hann = 0.5f * (1.0f - std::cos(2.0f * M_PI * windowPos));
        float poisson = std::exp(-2.0f * std::abs(2.0f * windowPos - 1.0f));
        float window = hann * poisson;

        // Constant gain regardless of pitch ratio
        // Adjusted scaling for Hann-Poisson window overlap
        return sample * window * 0.85f; // Scale for two overlapping windows with Hann-Poisson
    }
    
    void randomizeGrain(double& grainPos) {
        // Add slight randomization to prevent periodicity
        grainPos += m_grainDist(m_randomGen);
        if (grainPos < 0) grainPos += GRAIN_SIZE;
        if (grainPos >= GRAIN_SIZE) grainPos -= GRAIN_SIZE;
    }
};

namespace DetuneDoublerImpl {

// DelayLine.h - Simple fractional delay
class DelayLine {
public:
    static constexpr size_t MAX_DELAY_SAMPLES = 8192;
    
    DelayLine() {
        reset();
    }
    
    void reset() {
        m_buffer.fill(0.0f);
        m_writePos = 0;
    }
    
    void setDelay(float delaySamples) {
        m_delaySamples = std::max(1.0f, std::min(delaySamples, MAX_DELAY_SAMPLES - 4.0f));
    }
    
    float process(float input) {
        // Write to buffer
        m_buffer[m_writePos] = input + 1e-25f;
        
        // Calculate read position
        float readPos = m_writePos - m_delaySamples;
        while (readPos < 0) readPos += MAX_DELAY_SAMPLES;
        
        // Cubic interpolation
        int idx = static_cast<int>(readPos);
        float frac = readPos - idx;
        
        float y0 = m_buffer[(idx - 1 + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES];
        float y1 = m_buffer[idx % MAX_DELAY_SAMPLES];
        float y2 = m_buffer[(idx + 1) % MAX_DELAY_SAMPLES];
        float y3 = m_buffer[(idx + 2) % MAX_DELAY_SAMPLES];
        
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        
        float output = ((c3 * frac + c2) * frac + c1) * frac + c0;
        
        // Update write position
        m_writePos = (m_writePos + 1) % MAX_DELAY_SAMPLES;
        
        return output;
    }
    
private:
    alignas(64) std::array<float, MAX_DELAY_SAMPLES> m_buffer;
    size_t m_writePos = 0;
    float m_delaySamples = 1000.0f;
};

// AllPassNetwork.h - Phase decorrelation network
class AllPassNetwork {
public:
    AllPassNetwork(std::mt19937& rng) : m_randomGen(rng) {
        // Initialize with prime number delays to avoid periodicity
        m_stages[0].setDelay(83, 0.7f);
        m_stages[1].setDelay(97, -0.7f);
        m_stages[2].setDelay(103, 0.6f);
        m_stages[3].setDelay(109, -0.6f);
    }
    
    void reset() {
        for (auto& stage : m_stages) {
            stage.reset();
        }
    }
    
    float process(float input) {
        float output = input;
        for (auto& stage : m_stages) {
            output = stage.process(output);
        }
        return output;
    }
    
    void randomize() {
        // Slightly randomize coefficients for variation
        std::uniform_real_distribution<float> dist(0.5f, 0.8f);
        
        for (int i = 0; i < 4; ++i) {
            float gain = dist(m_randomGen);
            if (i % 2 == 1) gain = -gain;
            m_stages[i].setGain(gain);
        }
    }
    
private:
    struct AllPassStage {
        static constexpr size_t MAX_DELAY = 128;
        std::array<float, MAX_DELAY> buffer;
        size_t writePos = 0;
        size_t delayLength = 50;
        float gain = 0.7f;
        
        void reset() {
            buffer.fill(0.0f);
            writePos = 0;
        }
        
        void setDelay(size_t delay, float g) {
            delayLength = std::min(delay, MAX_DELAY - 1);
            gain = g;
        }
        
        void setGain(float g) {
            gain = g;
        }
        
        float process(float input) {
            float delayed = buffer[writePos];
            float output = -gain * input + delayed;
            buffer[writePos] = input + gain * output;
            writePos = (writePos + 1) % delayLength;
            return output;
        }
    };
    
    std::array<AllPassStage, 4> m_stages;
    std::mt19937& m_randomGen;
};

// ModulationGenerator.h - Multi-rate modulation for natural movement
class ModulationGenerator {
public:
    ModulationGenerator(std::mt19937& rng) : m_randomGen(rng) {
        reset();
    }
    
    void setSampleRate(double sampleRate) {
        m_sampleRate = sampleRate;
    }
    
    void reset() {
        m_phase1 = 0.0;
        m_phase2 = 0.0;
        m_phase3 = 0.0;
        m_noiseState = 0.0f;
    }
    
    void setRates(float baseRate) {
        m_rate1 = baseRate;
        m_rate2 = baseRate * 1.71f;  // Non-harmonic ratios
        m_rate3 = baseRate * 2.89f;
    }
    
    float generate() {
        // Three non-harmonic LFOs
        float lfo1 = std::sin(m_phase1);
        float lfo2 = std::sin(m_phase2) * 0.7f;
        float lfo3 = std::sin(m_phase3) * 0.3f;
        
        // Update phases
        m_phase1 += 2.0 * M_PI * m_rate1 / m_sampleRate;
        m_phase2 += 2.0 * M_PI * m_rate2 / m_sampleRate;
        m_phase3 += 2.0 * M_PI * m_rate3 / m_sampleRate;
        
        // Wrap phases
        while (m_phase1 >= 2.0 * M_PI) m_phase1 -= 2.0 * M_PI;
        while (m_phase2 >= 2.0 * M_PI) m_phase2 -= 2.0 * M_PI;
        while (m_phase3 >= 2.0 * M_PI) m_phase3 -= 2.0 * M_PI;
        
        // Add filtered noise for tape-like flutter using proper RNG
        float noise = m_noiseDist(m_randomGen);
        m_noiseState = noise * 0.01f + m_noiseState * 0.99f;
        
        return (lfo1 + lfo2 + lfo3) * 0.333f + m_noiseState * 0.1f;
    }
    
private:
    double m_sampleRate = 44100.0;
    double m_phase1 = 0.0;
    double m_phase2 = 0.0;
    double m_phase3 = 0.0;
    float m_rate1 = 0.1f;
    float m_rate2 = 0.171f;
    float m_rate3 = 0.289f;
    float m_noiseState = 0.0f;
    std::mt19937& m_randomGen;
    std::uniform_real_distribution<float> m_noiseDist{-1.0f, 1.0f};
};

// BiquadFilter.h - High-quality filtering
class BiquadFilter {
public:
    void reset() noexcept {
        m_x1 = m_x2 = m_y1 = m_y2 = 0.0;
    }
    
    void setHighShelf(double frequency, double sampleRate, double gainDB) noexcept {
        double A = std::pow(10.0, gainDB / 40.0);
        double omega = 2.0 * M_PI * frequency / sampleRate;
        double sin_w = std::sin(omega);
        double cos_w = std::cos(omega);
        double S = 1.0; // Shelf slope
        double alpha = sin_w / 2.0 * std::sqrt((A + 1.0/A) * (1.0/S - 1.0) + 2.0);
        
        double norm = 1.0 / ((A + 1.0) - (A - 1.0) * cos_w + alpha);
        
        m_b0 = A * ((A + 1.0) + (A - 1.0) * cos_w + alpha) * norm;
        m_b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w) * norm;
        m_b2 = A * ((A + 1.0) + (A - 1.0) * cos_w - alpha) * norm;
        m_a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w) * norm;
        m_a2 = ((A + 1.0) - (A - 1.0) * cos_w - alpha) * norm;
    }
    
    float processSample(float input) noexcept {
        double in = static_cast<double>(input);
        double out = m_b0 * in + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
        
        m_x2 = m_x1; m_x1 = in;
        m_y2 = m_y1; m_y1 = out;
        
        return static_cast<float>(out + 1e-20);
    }
    
private:
    double m_b0 = 1.0, m_b1 = 0.0, m_b2 = 0.0;
    double m_a1 = 0.0, m_a2 = 0.0;
    double m_x1 = 0.0, m_x2 = 0.0;
    double m_y1 = 0.0, m_y2 = 0.0;
};

// ParameterSmoother.h - Enhanced with reset functionality
class ParameterSmoother {
public:
    void setSampleRate(double sampleRate) noexcept {
        m_sampleRate = sampleRate;
        updateCoefficient();
    }
    
    void setSmoothingTime(float milliseconds) noexcept {
        m_smoothingTime = milliseconds;
        updateCoefficient();
    }
    
    void setTargetValue(float newTarget) noexcept {
        m_target = newTarget;
    }
    
    float getNextValue() noexcept {
        m_current = m_target + (m_current - m_target) * m_smoothingCoeff;
        return m_current + 1e-15f;
    }
    
    void reset(float value) noexcept {
        m_current = m_target = value;
    }
    
    float getCurrentValue() const noexcept {
        return m_current;
    }
    
private:
    float m_current = 0.0f;
    float m_target = 0.0f;
    float m_smoothingCoeff = 0.99f;
    float m_smoothingTime = 20.0f;
    double m_sampleRate = 44100.0;
    
    void updateCoefficient() noexcept {
        float a = std::exp(-1.0f / (m_smoothingTime * 0.001f * m_sampleRate));
        m_smoothingCoeff = a;
    }
};

} // namespace DetuneDoublerImpl

// Implementation of Voice::reset() after all class definitions
inline void DetuneDoubler::Voice::reset() {
    if (pitchShifter) pitchShifter->reset();
    if (delay) delay->reset();
    if (phaseNetwork) phaseNetwork->reset();
    if (modulator) modulator->reset();
    if (tapeFilter) tapeFilter->reset();
}

} // namespace AudioDSP