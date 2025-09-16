// FormantFilter.h
#pragma once

#include "../Source/EngineBase.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <cmath>
#include <map>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Professional denormal prevention using bit manipulation
inline double preventDenormal(double x) {
    union { double d; uint64_t i; } u;
    u.d = x;
    if ((u.i & 0x7FF0000000000000ULL) == 0) return 0.0;
    return x;
}

// Fast xorshift PRNG for thermal noise
struct XorShift64 {
    uint64_t state = 88172645463325252ull;
    double next() {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return double(state & 0xFFFFFFFFull) / double(0x100000000ull) - 0.5;
    }
};

// Professional Kaiser-windowed sinc oversampler
class KaiserOversampler2x {
public:
    static constexpr int TAPS_PER_PHASE = 16;
    static constexpr int TOTAL_TAPS = TAPS_PER_PHASE * 2;
    
    KaiserOversampler2x() {
        generateCoefficients();
        reset();
    }
    
    void reset() {
        std::fill(m_upHistory.begin(), m_upHistory.end(), 0.0);
        std::fill(m_downHistory.begin(), m_downHistory.end(), 0.0);
        m_upIdx = 0;
        m_downIdx = 0;
    }
    
    void process(double input, double& out1, double& out2) {
        // Update history
        m_upHistory[m_upIdx] = input;
        
        // Phase 0 (at sample point)
        out1 = 0.0;
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            int idx = (m_upIdx - i + HISTORY_SIZE) % HISTORY_SIZE;
            out1 += m_upHistory[idx] * m_coeffsPhase0[i];
        }
        
        // Phase 1 (between samples)
        out2 = 0.0;
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            int idx = (m_upIdx - i + HISTORY_SIZE) % HISTORY_SIZE;
            out2 += m_upHistory[idx] * m_coeffsPhase1[i];
        }
        
        m_upIdx = (m_upIdx + 1) % HISTORY_SIZE;
    }
    
    double downsample(double in1, double in2) {
        // Store both samples
        m_downHistory[m_downIdx] = in1;
        m_downHistory[(m_downIdx + 1) % HISTORY_SIZE] = in2;
        
        // Apply filter
        double output = 0.0;
        for (int i = 0; i < TOTAL_TAPS; ++i) {
            int idx = (m_downIdx + i) % HISTORY_SIZE;
            output += m_downHistory[idx] * m_coeffsDown[i];
        }
        
        m_downIdx = (m_downIdx + 2) % HISTORY_SIZE;
        return output;
    }
    
private:
    static constexpr int HISTORY_SIZE = 32;
    std::array<double, HISTORY_SIZE> m_upHistory;
    std::array<double, HISTORY_SIZE> m_downHistory;
    std::array<double, TAPS_PER_PHASE> m_coeffsPhase0;
    std::array<double, TAPS_PER_PHASE> m_coeffsPhase1;
    std::array<double, TOTAL_TAPS> m_coeffsDown;
    int m_upIdx = 0;
    int m_downIdx = 0;
    
    void generateCoefficients() {
        // Kaiser window parameters for -80dB stopband
        const double beta = 7.865; // Kaiser beta for -80dB
        const double cutoff = 0.45; // Normalized cutoff
        
        // Generate phase 0 coefficients (at sample points)
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            double n = i * 2.0; // Even samples
            double x = n - (TOTAL_TAPS - 1) / 2.0;
            double sinc = (x == 0) ? 2.0 * cutoff : std::sin(2.0 * M_PI * cutoff * x) / (M_PI * x);
            double kaiser = besseli0(beta * std::sqrt(1.0 - (2.0 * x / (TOTAL_TAPS - 1)) * (2.0 * x / (TOTAL_TAPS - 1)))) / besseli0(beta);
            m_coeffsPhase0[i] = sinc * kaiser * 2.0;
        }
        
        // Generate phase 1 coefficients (between samples)
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            double n = i * 2.0 + 1.0; // Odd samples
            double x = n - (TOTAL_TAPS - 1) / 2.0;
            double sinc = std::sin(2.0 * M_PI * cutoff * x) / (M_PI * x);
            double kaiser = besseli0(beta * std::sqrt(1.0 - (2.0 * x / (TOTAL_TAPS - 1)) * (2.0 * x / (TOTAL_TAPS - 1)))) / besseli0(beta);
            m_coeffsPhase1[i] = sinc * kaiser * 2.0;
        }
        
        // Downsampling coefficients (full filter)
        for (int i = 0; i < TOTAL_TAPS; ++i) {
            double x = i - (TOTAL_TAPS - 1) / 2.0;
            double sinc = (x == 0) ? 2.0 * cutoff : std::sin(2.0 * M_PI * cutoff * x) / (M_PI * x);
            double kaiser = besseli0(beta * std::sqrt(1.0 - (2.0 * x / (TOTAL_TAPS - 1)) * (2.0 * x / (TOTAL_TAPS - 1)))) / besseli0(beta);
            m_coeffsDown[i] = sinc * kaiser;
        }
        
        // Normalize
        normalizeCoefficients();
    }
    
    void normalizeCoefficients() {
        double sum0 = 0.0, sum1 = 0.0, sumDown = 0.0;
        
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            sum0 += m_coeffsPhase0[i];
            sum1 += m_coeffsPhase1[i];
        }
        
        for (int i = 0; i < TOTAL_TAPS; ++i) {
            sumDown += m_coeffsDown[i];
        }
        
        // Normalize for unity gain
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            m_coeffsPhase0[i] /= sum0;
            m_coeffsPhase1[i] /= sum1;
        }
        
        for (int i = 0; i < TOTAL_TAPS; ++i) {
            m_coeffsDown[i] /= sumDown;
        }
    }
    
    // Modified Bessel function for Kaiser window
    double besseli0(double x) {
        double sum = 1.0;
        double term = 1.0;
        double x2 = x * x / 4.0;
        
        for (int k = 1; k < 20; ++k) {
            term *= x2 / (k * k);
            sum += term;
            if (term < 1e-10 * sum) break;
        }
        
        return sum;
    }
};

// State Variable Filter with full double precision
class SVFilter {
public:
    void reset() {
        m_ic1eq = m_ic2eq = 0.0;
    }
    
    void setParameters(double freq, double q, double sampleRate) {
        double g = std::tan(M_PI * freq / sampleRate);
        double k = 1.0 / q;
        
        m_a1 = 1.0 / (1.0 + g * (g + k));
        m_a2 = g * m_a1;
        m_a3 = g * m_a2;
    }
    
    double processBandpass(double input) {
        double v3 = input - m_ic2eq;
        double v1 = m_a1 * m_ic1eq + m_a2 * v3;
        double v2 = m_ic2eq + m_a2 * m_ic1eq + m_a3 * v3;
        
        m_ic1eq = 2.0 * v1 - m_ic1eq;
        m_ic2eq = 2.0 * v2 - m_ic2eq;
        
        // Denormal prevention
        m_ic1eq = preventDenormal(m_ic1eq);
        m_ic2eq = preventDenormal(m_ic2eq);
        
        return v1;
    }
    
private:
    double m_ic1eq = 0.0, m_ic2eq = 0.0;
    double m_a1 = 0.0, m_a2 = 0.0, m_a3 = 0.0;
};

// Parameter identifiers
enum ParamID {
    kVowelPosition = 0,
    kFormantShift,
    kResonance,
    kMorph,
    kDrive,
    kMix,
    kNumParams
};

class FormantFilter : public EngineBase {
public:
    FormantFilter();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    juce::String getName() const override { return "Formant Filter Pro"; }
    int getNumParameters() const override { return kNumParams; }
    juce::String getParameterName(int index) const override;
    juce::String getParameterDisplayString(int index, float value) const;

private:
    struct SmoothParam {
        std::atomic<float> target{0.0f};
        double current = 0.0;
        double smoothing = 0.995;
        
        void setSmoothingTime(double timeMs, double sampleRate) {
            double samples = timeMs * 0.001 * sampleRate;
            smoothing = std::exp(-1.0 / samples);
        }
        
        void updateBlock() {
            double t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * smoothing;
            // Snap to target when close
            if (std::abs(current - t) < 1e-6) {
                current = t;
            }
        }
    };

    SmoothParam m_vowelPosition, m_formantShift, m_resonance, m_morph, m_drive, m_mix;
    double m_sampleRate = 44100.0;
    int m_blockSize = 512;

    // Professional formant data with Q values
    struct FormantData { 
        double f1, f2, f3;  // Frequencies
        double q1, q2, q3;  // Q values (2-20 range)
        double a1, a2, a3;  // Amplitudes
    };
    
    static const FormantData VOWEL_A, VOWEL_E, VOWEL_I, VOWEL_O, VOWEL_U;

    struct FormantBandpass {
        SVFilter filter;
        KaiserOversampler2x oversampler;
        double freq = 1000.0;
        double q = 5.0;
        double gain = 1.0;
        
        void reset(double sampleRate) {
            filter.reset();
            oversampler.reset();
        }
    };

    // Dynamic channel allocation
    std::vector<std::array<FormantBandpass, 3>> m_formantFilters;
    
    struct DCBlocker {
        double x1 = 0.0, y1 = 0.0;
        static constexpr double R = 0.995;
        
        double process(double in) {
            double out = in - x1 + R * y1;
            x1 = preventDenormal(in);
            y1 = preventDenormal(out);
            return out;
        }
        
        void reset() {
            x1 = y1 = 0.0;
        }
    };
    
    std::vector<DCBlocker> m_dcBlockers;

    struct ThermalModel {
        XorShift64 prng;
        double thermalNoise = 0.0;
        double noiseFilter = 0.0;
        static constexpr double DECAY = 0.999;
        static constexpr double GAIN = 0.00001;  // Reduced for less noise
        
        void update(double sr) {
            // Pink noise filtering
            double white = prng.next() * 0.00001;  // Reduced from 0.001
            noiseFilter = white * 0.02 + noiseFilter * 0.98;
            
            // Leaky integrator to prevent drift
            thermalNoise = thermalNoise * DECAY + noiseFilter * (GAIN / sr);
            thermalNoise = std::clamp(thermalNoise, -0.0001, 0.0001);  // Reduced from 0.01
        }
        
        double getFactor() const { return 1.0 + thermalNoise; }
    } m_thermalModel;

    // Processing functions - all double precision
    double processSample(double input, int channel);
    FormantData interpolateVowels(double vowelPos, double morph) const;
    void updateFormantFilters(int channel, const FormantData& D);
    double processFormantBank(double in, int channel, double drive);
    double analogSaturation(double in, double amt) const;
    double asymmetricSaturation(double in, double amt) const;
    
    // Processing state
    bool m_useOversampling = false;
};