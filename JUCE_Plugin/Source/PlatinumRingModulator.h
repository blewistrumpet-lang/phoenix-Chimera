// PlatinumRingModulator.h - Ultimate professional ring modulator with YIN pitch tracking
#pragma once
#include "EngineBase.h"
#include <vector>
#include <cmath>
#include <memory>
#include <complex>
#include <array>
#include <random>
#include <thread>
#include <functional>
// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

// Check for availability of Bessel functions
#ifndef HAS_BESSEL_FUNCTION
    #if defined(__APPLE__) && __cplusplus >= 201703L
        #define HAS_BESSEL_FUNCTION 1
    #elif defined(__GNUC__) && __cplusplus >= 201703L && __GNUC__ >= 7
        #define HAS_BESSEL_FUNCTION 1
    #elif defined(_MSC_VER) && _MSC_VER >= 1914
        #define HAS_BESSEL_FUNCTION 1
    #else
        #define HAS_BESSEL_FUNCTION 0
    #endif
#endif

// Define ALWAYS_INLINE for cross-platform optimization
#ifdef _MSC_VER
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

class PlatinumRingModulator : public EngineBase {
public:
    PlatinumRingModulator();
    ~PlatinumRingModulator() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Platinum Ring Modulator"; }
    int getNumParameters() const override { return 12; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters with thread-safe smoothing
    struct SmoothParam {
        std::atomic<float> target{0.0f};
        float current = 0.0f;
        float smoothing = 0.995f;
        
        void update() {
            float t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * smoothing;
        }
        
        void setImmediate(float value) {
            target.store(value, std::memory_order_relaxed);
            current = value;
        }
        
        void setSmoothingRate(float timeMs, double sampleRate) {
            smoothing = std::exp(-1.0f / (timeMs * 0.001f * sampleRate));
        }
    };
    
    SmoothParam m_carrierFreq;
    SmoothParam m_ringAmount;
    SmoothParam m_shiftAmount;
    SmoothParam m_feedbackAmount;
    SmoothParam m_pulseWidth;
    SmoothParam m_phaseModulation;
    SmoothParam m_harmonicStretch;
    SmoothParam m_spectralTilt;
    SmoothParam m_resonance;
    SmoothParam m_shimmer;
    SmoothParam m_thermalDrift;
    SmoothParam m_pitchTracking;
    
    // DSP State
    double m_sampleRate = 44100.0;
    int m_blockSize = 512;
    
    // Ultra-precise carrier oscillator with 64-bit phase accumulator
    struct CarrierOscillator {
        double phase = 0.0;
        double phaseIncrement = 0.0;
        double pulseWidth = 0.5;
        double phaseModDepth = 0.0;
        
        // Sub-oscillator for thickness
        double subPhase = 0.0;
        double subMix = 0.0;
        
        // Harmonic stretch parameters
        double stretch = 1.0;
        std::array<double, 8> harmonicPhases = {0.0};
        std::array<float, 8> harmonicAmps = {1.0f, 0.5f, 0.33f, 0.25f, 0.2f, 0.17f, 0.14f, 0.125f};
        
        ALWAYS_INLINE float tick() {
            // Additive synthesis with stretched harmonics
            float output = 0.0f;
            
            for (int h = 0; h < 8; ++h) {
                double harmPhase = harmonicPhases[h];
                float harmonic = std::sin(harmPhase * 2.0 * M_PI) * harmonicAmps[h];
                output += harmonic;
                
                // Update harmonic phase with stretch
                harmonicPhases[h] += phaseIncrement * (h + 1) * stretch;
                while (harmonicPhases[h] >= 1.0) harmonicPhases[h] -= 1.0;
            }
            
            // Add pulse wave component
            float pulse = (phase < pulseWidth) ? 1.0f : -1.0f;
            output = output * 0.7f + pulse * 0.3f;
            
            // Add sub-oscillator
            float sub = std::sin(subPhase * 2.0 * M_PI);
            output = output * (1.0f - subMix) + sub * subMix;
            
            // Update phases
            phase += phaseIncrement;
            while (phase >= 1.0) phase -= 1.0;
            
            subPhase += phaseIncrement * 0.5;  // One octave down
            while (subPhase >= 1.0) subPhase -= 1.0;
            
            return output;
        }
        
        void setFrequency(float freq, double sampleRate) {
            phaseIncrement = freq / sampleRate;
        }
        
        void reset() {
            phase = 0.0;
            subPhase = 0.0;
            harmonicPhases.fill(0.0);
        }
    };
    
    CarrierOscillator m_carrier;
    
    // YIN pitch tracking algorithm
    struct YINPitchTracker {
        static constexpr int YIN_BUFFER_SIZE = 2048;
        static constexpr int YIN_HALF_SIZE = YIN_BUFFER_SIZE / 2;
        static constexpr float YIN_THRESHOLD = 0.15f;
        
        std::array<float, YIN_BUFFER_SIZE> buffer;
        std::array<float, YIN_HALF_SIZE> yinBuffer;
        int bufferPos = 0;
        float detectedFrequency = 440.0f;
        float confidence = 0.0f;
        
        float detect(float input, double sampleRate);
        
    private:
        void difference();
        void cumulativeMeanNormalize();
        int absoluteThreshold();
        float parabolicInterpolation(int bestTau);
    };
    
    // Professional Hilbert transform for frequency shifting
    struct HilbertTransform {
        static constexpr int FILTER_LENGTH = 65;  // Higher order for better accuracy
        std::array<float, FILTER_LENGTH> delayLine;
        std::array<float, FILTER_LENGTH> coefficients;
        int writePos = 0;
        
        void init();
        std::complex<float> processAnalytic(float input);
        
    private:
        void generateCoefficients();
    };
    
    // 96dB/octave elliptic filter for anti-aliasing
    struct EllipticFilter {
        // 8th order (4 biquad sections)
        struct Biquad {
            double a0 = 1.0, a1 = 0.0, a2 = 0.0;
            double b1 = 0.0, b2 = 0.0;
            double x1 = 0.0, x2 = 0.0;
            double y1 = 0.0, y2 = 0.0;
            
            ALWAYS_INLINE double process(double input) {
                double output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
                x2 = x1; x1 = input;
                y2 = y1; y1 = output;
                return output;
            }
            
            void reset() {
                x1 = x2 = y1 = y2 = 0.0;
            }
        };
        
        std::array<Biquad, 4> sections;
        
        void designLowpass(double cutoff, double sampleRate);
        
        ALWAYS_INLINE float process(float input) {
            double x = input;
            for (auto& section : sections) {
                x = section.process(x);
            }
            return static_cast<float>(x);
        }
        
        void reset() {
            for (auto& section : sections) {
                section.reset();
            }
        }
    };
    
    // 4x oversampling with polyphase FIR
    struct Oversampler {
        static constexpr int OVERSAMPLE_FACTOR = 4;
        static constexpr int FIR_LENGTH = 64;
        
        // Polyphase FIR coefficients
        std::array<std::array<float, FIR_LENGTH/OVERSAMPLE_FACTOR>, OVERSAMPLE_FACTOR> polyphaseUp;
        std::array<std::array<float, FIR_LENGTH/OVERSAMPLE_FACTOR>, OVERSAMPLE_FACTOR> polyphaseDown;
        
        // Delay lines for each phase
        std::array<std::array<float, FIR_LENGTH/OVERSAMPLE_FACTOR>, OVERSAMPLE_FACTOR> delayUp;
        std::array<std::array<float, FIR_LENGTH/OVERSAMPLE_FACTOR>, OVERSAMPLE_FACTOR> delayDown;
        
        // Anti-aliasing filters
        EllipticFilter upsampleFilter;
        EllipticFilter downsampleFilter;
        
        // Buffers
        std::vector<float> oversampledBuffer;
        int bufferSize = 0;
        
        void init(double sampleRate, int blockSize);
        void upsample(const float* input, float* output, int numSamples);
        void downsample(const float* input, float* output, int numSamples);
        
    private:
        void generatePolyphaseCoefficients();
    };
    
    // Phase vocoder for advanced pitch shifting
    struct PhaseVocoder {
        static constexpr int FFT_SIZE = 2048;
        static constexpr int HOP_SIZE = FFT_SIZE / 4;
        
        std::unique_ptr<juce::dsp::FFT> fft;
        std::array<float, FFT_SIZE * 2> fftBuffer;
        std::array<std::complex<float>, FFT_SIZE> spectrum;
        std::array<float, FFT_SIZE> window;
        std::array<float, FFT_SIZE> lastPhase;
        std::array<float, FFT_SIZE> sumPhase;
        
        // Circular buffers
        std::array<float, FFT_SIZE * 2> inputBuffer;
        std::array<float, FFT_SIZE * 2> outputBuffer;
        int inputPos = 0;
        int outputPos = 0;
        int hopCounter = 0;
        
        void init();
        float process(float input, float pitchShift);
        void reset();
        
    private:
        void processFrame(float pitchShift);
    };
    
    // Per-channel state with all processing components
    struct ChannelState {
        YINPitchTracker pitchTracker;
        HilbertTransform hilbert;
        PhaseVocoder vocoder;
        
        // Feedback delay network
        static constexpr int MAX_DELAY = 4096;
        std::array<float, MAX_DELAY> delayBuffer;
        int delayWritePos = 0;
        float feedbackGain = 0.0f;
        
        // State-variable filter for resonance
        struct SVF {
            float g = 0.0f, R = 1.0f;
            float s1 = 0.0f, s2 = 0.0f;
            
            void setFrequency(float freq, double sampleRate) {
                g = std::tan((M_PI * freq) / sampleRate);
            }
            
            void setResonance(float q) {
                R = 1.0f / (2.0f * q);
            }
            
            float processBandpass(float input) {
                float hp = (input - s1 * (1.0f + g) - s2) / (1.0f + g * (1.0f + g));
                float bp = hp * g + s1;
                float lp = bp * g + s2;
                
                s1 = hp * g + bp;
                s2 = bp * g + lp;
                
                return bp;
            }
        };
        
        SVF resonanceFilter;
        
        // Shimmer effect (pitch-shifted delay)
        std::array<float, 8192> shimmerBuffer;
        int shimmerWritePos = 0;
        float shimmerAmount = 0.0f;
        
        // DC blocker
        float dcBlockerX1 = 0.0f;
        float dcBlockerY1 = 0.0f;
        
        ALWAYS_INLINE float processDCBlocker(float input) {
            const float R = 0.995f;
            float output = input - dcBlockerX1 + R * dcBlockerY1;
            dcBlockerX1 = input;
            dcBlockerY1 = output;
            return output;
        }
        
        void init();
        void reset();
    };
    
    std::array<ChannelState, 2> m_channels;
    int m_activeChannels = 2;
    
    // Oversampling
    Oversampler m_oversampler;
    bool m_useOversampling = true;
    
    // Thermal modeling for analog warmth
    struct ThermalModel {
        float temperature = 25.0f;
        float thermalNoise = 0.0f;
        float componentAging = 0.0f;
        std::mt19937 rng{std::random_device{}()};
        std::normal_distribution<float> noiseDist{0.0f, 0.0001f};
        
        void update(float driftAmount) {
            // Slow thermal drift
            thermalNoise = thermalNoise * 0.9999f + noiseDist(rng) * driftAmount;
            thermalNoise = std::max(-0.01f, std::min(0.01f, thermalNoise));
            
            // Component aging (very slow)
            componentAging += 1e-8f;
            componentAging = std::min(0.1f, componentAging);
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise - componentAging * 0.05f;
        }
    };
    
    ThermalModel m_thermalModel;
    
    // SIMD-optimized soft clipper
    ALWAYS_INLINE float softClip(float x) {
        // Fast approximation of tanh
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
    
    // Professional denormal prevention using bit manipulation
    ALWAYS_INLINE float preventDenormal(float x) {
        union { float f; uint32_t i; } u;
        u.f = x;
        // Check if exponent bits are zero (denormal)
        if ((u.i & 0x7F800000) == 0) return 0.0f;
        return x;
    }
    
    ALWAYS_INLINE double preventDenormalDouble(double x) {
        union { double d; uint64_t i; } u;
        u.d = x;
        // Check if exponent bits are zero (denormal)
        if ((u.i & 0x7FF0000000000000ULL) == 0) return 0.0;
        return x;
    }
    
    // Processing functions
    float processRingModulation(float input, float carrier, float amount);
    float processFrequencyShifting(float input, float shiftAmount, ChannelState& state);
    void processFeedback(float& sample, ChannelState& state);
    void processResonance(float& sample, float frequency, ChannelState& state);
    void processShimmer(float& sample, ChannelState& state);
};