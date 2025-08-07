#pragma once
#include "EngineBase.h"
#include <array>
#include <memory>
#include <atomic>
#include <cmath>
// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>  // For SIMD
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

class ClassicTremolo : public EngineBase {
public:
    ClassicTremolo();
    ~ClassicTremolo() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Classic Tremolo"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    
private:
    // Professional constants
    static constexpr double DENORMAL_PREVENTION = 1e-30;
    static constexpr int OVERSAMPLE_FACTOR = 4;
    static constexpr int MAX_BLOCK_SIZE = 2048;
    static constexpr int NUM_CHANNELS = 2;
    
    // Tremolo types
    enum class TremoloType {
        SINE_AMPLITUDE,      // Classic sine wave amplitude modulation
        TRIANGLE_AMPLITUDE,  // Triangle wave amplitude modulation
        SQUARE_AMPLITUDE,    // Square wave (choppy) tremolo
        HARMONIC_TREMOLO,    // Pitch vibrato + amplitude (Fender style)
        BIAS_TREMOLO,        // Tube bias modulation (Vox/Marshall style)
        OPTICAL_TREMOLO,     // Photocell/LED simulation
        ROTARY_SPEAKER       // Leslie-style with doppler
    };
    
    // Cached parameters for block processing
    struct CachedParams {
        double rate;
        double depth;
        double shape;
        double stereoPhase;
        TremoloType type;
        double symmetry;
        double volume;
        double mix;
    };
    
    // Professional parameter smoothing
    class ParameterSmoother {
        std::atomic<double> targetValue{0.0};
        double currentValue = 0.0;
        double smoothingCoeff = 0.0;
        
    public:
        void setSampleRate(double sr, double smoothingTimeMs = 20.0) {
            double fc = 1000.0 / (2.0 * M_PI * smoothingTimeMs);
            smoothingCoeff = std::exp(-2.0 * M_PI * fc / sr);
        }
        
        void setTarget(double value) {
            targetValue.store(value, std::memory_order_relaxed);
        }
        
        double process() {
            double target = targetValue.load(std::memory_order_relaxed);
            currentValue = target + (currentValue - target) * smoothingCoeff;
            currentValue += DENORMAL_PREVENTION;
            currentValue -= DENORMAL_PREVENTION;
            return currentValue;
        }
        
        void reset(double value) {
            targetValue.store(value, std::memory_order_relaxed);
            currentValue = value;
        }
        
        double getCurrent() const { return currentValue; }
    };
    
    // Professional LFO with phase accumulator
    class ProfessionalLFO {
        double phase = 0.0;
        double phaseIncrement = 0.0;
        double sampleRate = 48000.0;
        double pulseWidth = 0.5;
        double skew = 0.0;
        
    public:
        void setSampleRate(double sr) { sampleRate = sr; }
        void setFrequency(double freq) { phaseIncrement = freq / sampleRate; }
        void setPulseWidth(double pw) { pulseWidth = std::clamp(pw, 0.01, 0.99); }
        void setSkew(double s) { skew = std::clamp(s, -0.99, 0.99); }
        void reset(double startPhase = 0.0) { phase = std::fmod(startPhase, 1.0); }
        double getPhase() const { return phase; }
        
        void tick() {
            phase += phaseIncrement;
            if (phase >= 1.0) phase -= 1.0;
        }
        
        // Generate LFO values for entire block
        void generateBlock(double* output, int numSamples, double shape);
        
        double sine() const { return std::sin(2.0 * M_PI * phase); }
        double triangle() const;
        double square() const { return phase < pulseWidth ? 1.0 : -1.0; }
        double sawUp() const { return 2.0 * phase - 1.0; }
    };
    
    // Optical tremolo model
    class OpticalTremoloModel {
        double ledBrightness = 0.0;
        double cellResistance = 1.0;
        double attackCoeff = 0.0;
        double decayCoeff = 0.0;
        
    public:
        void setSampleRate(double sr);
        double process(double lfoValue);
        void processBlock(const double* lfoValues, double* output, int numSamples);
        void reset() { ledBrightness = 0.0; cellResistance = 1.0; }
    };
    
    // Harmonic tremolo (pitch shifting + amplitude)
    class HarmonicTremolo {
        static constexpr int DELAY_SIZE = 4096;
        alignas(16) std::array<double, DELAY_SIZE> delayLine{};
        int writePos = 0;
        double sampleRate = 48000.0;
        
        struct AllPassFilter {
            double x1 = 0, y1 = 0;
            double coefficient = 0;
            
            void setFrequency(double freq, double sr);
            double process(double input);
            void reset() { x1 = y1 = 0; }
        };
        
        std::array<AllPassFilter, 4> phaseNetwork;
        
    public:
        void setSampleRate(double sr);
        double process(double input, double lfoValue, double depth);
        void processBlock(const double* input, double* output, const double* lfoValues, 
                         int numSamples, double depth);
        void reset();
    };
    
    // Tube bias tremolo model V2
    class TubeBiasTremoloV2 {
        double sampleRate = 48000.0;
        double rcTimeConstant = 0.0;
        double couplingState = 0.0;
        
    public:
        void setSampleRate(double sr);
        double process(double input, double lfoValue, double depth);
        void processBlock(const double* input, double* output, const double* lfoValues,
                         int numSamples, double depth);
        void reset() { couplingState = 0.0; }
    };
    
    // Professional Rotary Speaker
    class ProfessionalRotarySpkr {
        struct Rotor {
            double angle = 0.0;
            double speed = 0.0;
            double targetSpeed = 0.0;
            double inertia = 0.95;
            
            void update(double speedHz, double sampleRate);
            double getSine() const { return std::sin(2.0 * M_PI * angle); }
            double getCosine() const { return std::cos(2.0 * M_PI * angle); }
        };
        
        struct LinkwitzRiley {
            double b0, b1, b2, a1, a2;
            double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            
            void setFrequency(double freq, double sr, bool highpass);
            double process(double input);
            void reset() { x1 = x2 = y1 = y2 = 0; }
        };
        
        struct DopplerDelay {
            static constexpr int DELAY_SIZE = 2048;
            alignas(16) std::array<double, DELAY_SIZE> buffer{};
            int writePos = 0;
            
            double process(double input, double delaySamples);
            void reset() { buffer.fill(0); writePos = 0; }
        };
        
        Rotor hornRotor, drumRotor;
        LinkwitzRiley lowpass, highpass;
        DopplerDelay hornDelay, drumDelay;
        double sampleRate = 48000.0;
        bool fastSpeed = false;
        
    public:
        void setSampleRate(double sr);
        void setSpeed(bool fast);
        double process(double input, double depth);
        void processBlock(const double* input, double* output, int numSamples, double depth);
        void reset();
    };
    
    // Optimized oversampler
    class OptimizedOversampler {
        struct Biquad {
            double b0, b1, b2, a1, a2;
            double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            
            inline double process(double in) {
                double out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = in;
                y2 = y1; y1 = out;
                return out;
            }
            
            void reset() { x1 = x2 = y1 = y2 = 0; }
        };
        
        std::array<Biquad, 4> upsampleStages;
        std::array<Biquad, 4> downsampleStages;
        
    public:
        void prepare(double sampleRate);
        void processUpsample(const double* input, double* output, int numSamples, int factor);
        void processDownsample(double* data, int numSamples, int factor);
        void reset();
    };
    
    // DC blocking filter
    class DCBlocker {
        double x1 = 0, y1 = 0;
        double cutoff = 0.995;
        
    public:
        void setCutoff(double freqHz, double sampleRate) {
            cutoff = 1.0 - std::exp(-2.0 * M_PI * freqHz / sampleRate);
        }
        
        double process(double input) {
            y1 = input - x1 + y1 * (1.0 - cutoff);
            x1 = input;
            y1 += DENORMAL_PREVENTION;
            y1 -= DENORMAL_PREVENTION;
            return y1;
        }
        
        void processBlock(double* data, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                data[i] = process(data[i]);
            }
        }
        
        void reset() { x1 = y1 = 0; }
    };
    
    // Core DSP members
    double m_sampleRate = 48000.0;
    
    // Parameters
    std::unique_ptr<ParameterSmoother> m_rate;
    std::unique_ptr<ParameterSmoother> m_depth;
    std::unique_ptr<ParameterSmoother> m_shape;
    std::unique_ptr<ParameterSmoother> m_stereoPhase;
    std::unique_ptr<ParameterSmoother> m_type;
    std::unique_ptr<ParameterSmoother> m_symmetry;
    std::unique_ptr<ParameterSmoother> m_volume;
    std::unique_ptr<ParameterSmoother> m_mix;
    
    // Processing components
    std::array<ProfessionalLFO, NUM_CHANNELS> m_lfos;
    std::array<OpticalTremoloModel, NUM_CHANNELS> m_opticalModels;
    std::array<std::unique_ptr<HarmonicTremolo>, NUM_CHANNELS> m_harmonicTremolos;
    std::array<std::unique_ptr<TubeBiasTremoloV2>, NUM_CHANNELS> m_tubeTremolos;
    std::array<std::unique_ptr<ProfessionalRotarySpkr>, NUM_CHANNELS> m_rotarySpeakers;
    std::array<std::unique_ptr<OptimizedOversampler>, NUM_CHANNELS> m_oversamplers;
    std::array<DCBlocker, NUM_CHANNELS> m_inputDCBlockers;
    std::array<DCBlocker, NUM_CHANNELS> m_outputDCBlockers;
    
    // Work buffers (aligned for SIMD)
    alignas(16) std::array<double, MAX_BLOCK_SIZE> m_workBuffers[NUM_CHANNELS];
    alignas(16) std::array<double, MAX_BLOCK_SIZE> m_lfoBuffers[NUM_CHANNELS];
    alignas(16) std::array<double, MAX_BLOCK_SIZE * OVERSAMPLE_FACTOR> m_oversampledBuffers[NUM_CHANNELS];
    
    // Processing methods
    void processChannelOptimized(float* data, int numSamples, int channel,
                                const CachedParams& params, bool needsOversampling);
    void processSimpleTremoloSIMD(float* data, int numSamples, 
                                  const double* lfoValues, double depth);
    double generateLFOValue(int channel, double shape, double symmetry);
};