#pragma once
#include "EngineBase.h"
#include <array>
#include <atomic>
#include <cmath>

// Platform-specific SIMD headers with detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

namespace AudioDSP {

//==============================================================================
// RotarySpeaker_Platinum - Zero-Allocation, SIMD-Optimized Leslie Simulator
// Version 2.0.0 - Production Ready
//==============================================================================

class RotarySpeaker_Platinum : public ::EngineBase {
public:
    //==========================================================================
    // Constants - Single Source of Truth
    //==========================================================================
    static constexpr int MaxBlockSize = 2048;
    static constexpr int DelayBufferSize = 8192;  // Power of 2 for fast masking
    static constexpr int NumChannels = 2;
    static constexpr int NumParameters = 6;
    
    // Physical modeling constants
    static constexpr double HornRadius = 0.15;     // meters
    static constexpr double DrumRadius = 0.20;     // meters
    static constexpr double SpeedOfSound = 343.0;  // m/s
    static constexpr double CrossoverFreq = 800.0; // Hz
    
    // Compile-time validations
    static_assert((DelayBufferSize & (DelayBufferSize - 1)) == 0, 
                  "DelayBufferSize must be power of 2");
    static_assert(MaxBlockSize <= DelayBufferSize / 4, 
                  "MaxBlockSize too large for delay buffer");
    
    //==========================================================================
    // Constructor/Destructor
    //==========================================================================
    RotarySpeaker_Platinum() noexcept;
    ~RotarySpeaker_Platinum() noexcept = default;
    
    //==========================================================================
    // Core API - All Real-Time Safe
    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) noexcept override;
    void reset() noexcept override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const noexcept override { return "Rotary Speaker Platinum"; }
    int getNumParameters() const noexcept override { return NumParameters; }
    juce::String getParameterName(int index) const noexcept override;
    
    //==========================================================================
    // Parameter Control Block - Cache-Aligned
    //==========================================================================
    struct alignas(64) Parameters {
        std::atomic<float> speed{0.5f};        // 0=stop, 0.5=slow, 1=fast
        std::atomic<float> acceleration{0.5f}; // Rotor inertia
        std::atomic<float> drive{0.3f};        // Tube saturation
        std::atomic<float> micDistance{0.6f};  // Mic placement
        std::atomic<float> stereoWidth{0.8f};  // Stereo spread
        std::atomic<float> mix{1.0f};          // Dry/wet mix
        std::atomic<bool> brake{false};        // Emergency stop
    };
    
    const Parameters& getParameters() const noexcept { return m_params; }
    
    //==========================================================================
    // Performance Metrics
    //==========================================================================
    struct alignas(64) Metrics {
        std::atomic<float> cpuUsage{0.0f};
        std::atomic<int> denormalCount{0};
        std::atomic<float> hornSpeed{0.0f};
        std::atomic<float> drumSpeed{0.0f};
    };
    
    const Metrics& getMetrics() const noexcept { return m_metrics; }

private:
    //==========================================================================
    // Rotor State - Stack Allocated
    //==========================================================================
    struct RotorState {
        double angle{0.0};
        double velocity{0.0};
        double targetVelocity{0.0};
        double acceleration{2.5};
        
        void update(double deltaTime) noexcept {
            // Smooth acceleration/deceleration
            const double maxChange = acceleration * deltaTime;
            const double diff = targetVelocity - velocity;
            
            if (std::fabs(diff) > maxChange) {
                velocity += (diff > 0) ? maxChange : -maxChange;
            } else {
                velocity = targetVelocity;
            }
            
            // Update angle
            angle += velocity * deltaTime;
            
            // Wrap to [0, 2π] without branching
            const double twoPi = 2.0 * M_PI;
            angle = angle - twoPi * std::floor(angle / twoPi);
        }
        
        void reset() noexcept {
            angle = velocity = targetVelocity = 0.0;
        }
    };
    
    //==========================================================================
    // Linkwitz-Riley Crossover - Stack Allocated
    //==========================================================================
    struct alignas(16) CrossoverFilter {
        struct BiquadCoeffs {
            float b0{1}, b1{0}, b2{0}, a1{0}, a2{0};
        };
        
        struct BiquadState {
            float x1{0}, x2{0}, y1{0}, y2{0};
            
            float process(float in, const BiquadCoeffs& c) noexcept {
                float out = c.b0 * in + c.b1 * x1 + c.b2 * x2 - c.a1 * y1 - c.a2 * y2;
                x2 = x1; x1 = in;
                y2 = y1; y1 = flushDenormal(out);
                return out;
            }
            
            void reset() noexcept { x1 = x2 = y1 = y2 = 0; }
        };
        
        // 4th order = 2 cascaded 2nd order
        std::array<BiquadState, 2> lowStages;
        std::array<BiquadState, 2> highStages;
        BiquadCoeffs lowCoeffs, highCoeffs;
        
        void prepare(double sampleRate, double frequency) noexcept;
        void process(float input, float& lowOut, float& highOut) noexcept;
        void reset() noexcept;
    };
    
    //==========================================================================
    // Doppler Delay Line - Fixed Size
    //==========================================================================
    struct alignas(64) DopplerProcessor {
        std::array<float, DelayBufferSize> buffer;
        static constexpr int Mask = DelayBufferSize - 1;
        int writePos{0};
        float prevDelay{0};
        float delaySmoothCoeff{0.995f};
        
        void reset() noexcept {
            buffer.fill(0);
            writePos = 0;
            prevDelay = 0;
        }
        
        float process(float input, float delayTime) noexcept;
        
    private:
        float cubicInterpolate(float position) const noexcept;
    };
    
    //==========================================================================
    // Amplitude Modulator - Stack Allocated
    //==========================================================================
    struct AmplitudeModulator {
        float smoothState{1.0f};
        float smoothCoeff{0.99f};
        
        float process(float input, double angle, double micAngle, float depth) noexcept {
            // Cardioid pattern
            float pattern = 0.5f + 0.5f * std::cos(angle - micAngle);
            pattern += 0.1f * std::cos(2.0 * (angle - micAngle)); // Higher harmonic
            
            float modulation = 1.0f - depth * (1.0f - pattern);
            smoothState = modulation + (smoothState - modulation) * smoothCoeff;
            
            return input * smoothState;
        }
        
        void reset() noexcept { smoothState = 1.0f; }
    };
    
    //==========================================================================
    // Tube Saturation - Stack Allocated
    //==========================================================================
    struct TubeSaturator {
        struct DCBlocker {
            float x1{0}, y1{0};
            static constexpr float R = 0.995f;
            
            float process(float in) noexcept {
                float out = in - x1 + R * y1;
                x1 = in; y1 = out;
                return out;
            }
            
            void reset() noexcept { x1 = y1 = 0; }
        };
        
        DCBlocker dcIn, dcOut;
        
        float process(float input, float drive) noexcept {
            float blocked = dcIn.process(input);
            float driven = blocked * (1.0f + drive * 4.0f);
            float saturated = softClip(driven);
            return dcOut.process(saturated) / (1.0f + drive * 2.0f);
        }
        
        void reset() noexcept {
            dcIn.reset();
            dcOut.reset();
        }
        
    private:
        static float softClip(float x) noexcept {
            // Asymmetric tube saturation
            if (x > 0) {
                return (x < 0.7f) ? x : 0.7f + 0.3f * std::tanh(3.0f * (x - 0.7f));
            } else {
                return (x > -0.5f) ? x : -0.5f - 0.4f * std::tanh(2.0f * (-x - 0.5f));
            }
        }
    };
    
    //==========================================================================
    // Cabinet Resonator - Stack Allocated
    //==========================================================================
    struct CabinetResonator {
        struct Resonance {
            float freq{100}, q{8};
            float b0{1}, b1{0}, b2{0}, a1{0}, a2{0};
            float x1{0}, x2{0}, y1{0}, y2{0};
            
            void prepare(double sampleRate, float frequency, float Q) noexcept {
                freq = frequency;
                q = Q;
                
                const float omega = 2.0f * M_PI * freq / sampleRate;
                const float sinw = std::sin(omega);
                const float cosw = std::cos(omega);
                const float alpha = sinw / (2.0f * q);
                
                const float norm = 1.0f / (1.0f + alpha);
                b0 = alpha * norm;
                b1 = 0;
                b2 = -alpha * norm;
                a1 = -2.0f * cosw * norm;
                a2 = (1.0f - alpha) * norm;
            }
            
            float process(float in) noexcept {
                float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = in;
                y2 = y1; y1 = out;
                return out;
            }
            
            void reset() noexcept { x1 = x2 = y1 = y2 = 0; }
        };
        
        std::array<Resonance, 4> resonances;
        
        void prepare(double sampleRate) noexcept {
            // Leslie cabinet characteristic frequencies
            resonances[0].prepare(sampleRate, 97.0f, 12.0f);
            resonances[1].prepare(sampleRate, 185.0f, 10.0f);
            resonances[2].prepare(sampleRate, 380.0f, 8.0f);
            resonances[3].prepare(sampleRate, 760.0f, 6.0f);
        }
        
        float process(float in) noexcept {
            float out = in;
            for (auto& res : resonances) {
                out += res.process(in) * 0.05f;
            }
            return out;
        }
        
        void reset() noexcept {
            for (auto& res : resonances) res.reset();
        }
    };
    
    //==========================================================================
    // Parameter Smoother - Stack Allocated
    //==========================================================================
    struct Smoother {
        float current{0}, target{0};
        float coeff{0.999f};
        
        void setCoeff(double sampleRate, float timeMs) noexcept {
            const float samples = timeMs * 0.001f * sampleRate;
            coeff = std::exp(-1.0f / samples);
        }
        
        void setTarget(float t) noexcept { target = t; }
        
        float tick() noexcept {
            current += (target - current) * (1.0f - coeff);
            return current;
        }
        
        float getCurrentValue() const noexcept { return current; }
        
        void reset(float value) noexcept { current = target = value; }
    };
    
    //==========================================================================
    // Channel Processing State - All Stack Allocated!
    //==========================================================================
    struct alignas(64) ChannelState {
        CrossoverFilter crossover;
        DopplerProcessor hornDoppler;
        DopplerProcessor drumDoppler;
        AmplitudeModulator hornAM;
        AmplitudeModulator drumAM;
        TubeSaturator preamp;
        
        void prepare(double sampleRate) noexcept {
            crossover.prepare(sampleRate, CrossoverFreq);
            hornDoppler.reset();
            drumDoppler.reset();
            hornAM.reset();
            drumAM.reset();
            preamp.reset();
        }
        
        void reset() noexcept {
            crossover.reset();
            hornDoppler.reset();
            drumDoppler.reset();
            hornAM.reset();
            drumAM.reset();
            preamp.reset();
        }
    };
    
    //==========================================================================
    // Member Variables - All Stack Allocated!
    //==========================================================================
    
    // Processing state
    std::array<ChannelState, NumChannels> m_channels;
    CabinetResonator m_cabinet;
    
    // Rotor simulation
    RotorState m_hornRotor;
    RotorState m_drumRotor;
    
    // Parameter smoothing
    struct alignas(64) Smoothers {
        Smoother speed;
        Smoother acceleration;
        Smoother drive;
        Smoother micDistance;
        Smoother stereoWidth;
        Smoother mix;
    } m_smoothers;
    
    // Parameters and metrics
    Parameters m_params;
    Metrics m_metrics;
    
    // State
    double m_sampleRate{44100.0};
    int m_blockSize{512};
    
    // Sine/Cosine LUT for efficiency
    struct SinCosLUT {
        static constexpr int Size = 4096;
        std::array<float, Size> sinTable;
        std::array<float, Size> cosTable;
        
        void init() noexcept {
            for (int i = 0; i < Size; ++i) {
                float angle = (2.0f * M_PI * i) / Size;
                sinTable[i] = std::sin(angle);
                cosTable[i] = std::cos(angle);
            }
        }
        
        float sin(float angle) const noexcept {
            float norm = angle * (Size / (2.0f * M_PI));
            int idx = static_cast<int>(norm) & (Size - 1);
            return sinTable[idx];
        }
        
        float cos(float angle) const noexcept {
            float norm = angle * (Size / (2.0f * M_PI));
            int idx = static_cast<int>(norm) & (Size - 1);
            return cosTable[idx];
        }
    } m_lut;
    
    //==========================================================================
    // Internal Methods - All noexcept
    //==========================================================================
    void updateRotorSpeeds() noexcept;
    void processChannel(ChannelState& state, float* data, int numSamples, 
                       float micAngle) noexcept;
    float calculateDopplerDelay(double angle, double velocity, double radius,
                               double micAngle, double micDistance) const noexcept;
    
    // Denormal prevention
    template<typename T>
    static T flushDenormal(T x) noexcept {
        return (std::fabs(x) < 1e-30) ? T(0) : x;
    }
    
    // SIMD helpers
    #if HAS_SSE2
    void processBlockSSE(float* left, float* right, int numSamples) noexcept;
    #endif
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotarySpeaker_Platinum)
};

} // namespace AudioDSP