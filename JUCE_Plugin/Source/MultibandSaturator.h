#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <immintrin.h>

class MultibandSaturator : public EngineBase {
public:
    MultibandSaturator();
    ~MultibandSaturator() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Multiband Saturator Ultimate"; }
    int getNumParameters() const override { return 7; }
    juce::String getParameterName(int index) const override;
    
private:
    // Saturation types
    enum class SaturationType {
        TUBE = 0,
        TAPE,
        TRANSISTOR,
        DIODE
    };
    
    // Constants
    static constexpr double LOW_CROSSOVER_FREQ = 250.0;
    static constexpr double HIGH_CROSSOVER_FREQ = 2500.0;
    static constexpr int OVERSAMPLE_FACTOR = 4;
    static constexpr size_t ALIGNMENT = 32;
    
    // Professional denormal prevention using bit manipulation
    template<typename T>
    static inline T preventDenormal(T x) noexcept {
        if constexpr (std::is_same_v<T, float>) {
            union { float f; uint32_t i; } u;
            u.f = x;
            if ((u.i & 0x7F800000) == 0) return 0.0f;
            return x;
        } else {
            union { double d; uint64_t i; } u;
            u.d = x;
            if ((u.i & 0x7FF0000000000000ULL) == 0) return 0.0;
            return x;
        }
    }
    
    // Thread-safe parameter smoothing
    struct SmoothParam {
        std::atomic<double> target{0.0};
        double current = 0.0;
        double coeff = 0.999;
        
        inline double tick() noexcept {
            double t = target.load(std::memory_order_relaxed);
            current += (t - current) * (1.0 - coeff);
            return preventDenormal(current);
        }
        
        void setImmediate(double value) {
            target.store(value, std::memory_order_relaxed);
            current = value;
        }
        
        void setSmoothingCoeff(double timeMs, double sampleRate) {
            coeff = std::exp(-1.0 / (timeMs * 0.001 * sampleRate));
        }
    };
    
    // Butterworth section for Linkwitz-Riley
    struct ButterworthSection {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        void calculateCoefficients(double freq, double sampleRate, bool highpass);
        
        inline double process(double input) noexcept {
            double w = input - a1 * y1 - a2 * y2;
            double output = b0 * w + b1 * x1 + b2 * x2;
            
            x2 = x1; x1 = w;
            y2 = y1; y1 = preventDenormal(output);
            
            return output;
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0;
        }
    };
    
    // Linkwitz-Riley filter (4th order)
    struct LinkwitzRileyFilter {
        ButterworthSection section1, section2;
        
        void setup(double freq, double sampleRate, bool highpass) {
            section1.calculateCoefficients(freq, sampleRate, highpass);
            section2.calculateCoefficients(freq, sampleRate, highpass);
        }
        
        inline double process(double input) noexcept {
            return section2.process(section1.process(input));
        }
        
        void reset() {
            section1.reset();
            section2.reset();
        }
    };
    
    // Crossover network
    struct CrossoverNetwork {
        LinkwitzRileyFilter lowLP, lowHP;
        LinkwitzRileyFilter midLP, midHP;
        
        struct BandOutputs {
            double low, mid, high;
        };
        
        void setup(double sampleRate);
        inline BandOutputs process(double input) noexcept;
        void reset();
    };
    
    // All-pass section for polyphase oversampling
    struct AllPassSection {
        double coefficient = 0.0;
        double state = 0.0;
        
        void setCoefficient(double coeff) { coefficient = coeff; }
        
        inline double process(double input) noexcept {
            double output = coefficient * (input - state) + state;
            state = preventDenormal(input);
            return output;
        }
        
        void reset() { state = 0.0; }
    };
    
    // Polyphase IIR Oversampler
    struct PolyphaseOversampler {
        std::array<AllPassSection, OVERSAMPLE_FACTOR> upPhase;
        std::array<AllPassSection, OVERSAMPLE_FACTOR> downPhase;
        
        void prepare();
        void processUpsample(const double* input, double* output, int numSamples) noexcept;
        void processDownsample(const double* input, double* output, int numSamples) noexcept;
        void reset();
    };
    
    // DC blocker
    struct DCBlocker {
        double x1 = 0.0, y1 = 0.0;
        static constexpr double R = 0.995;
        
        inline double process(double input) noexcept {
            double output = input - x1 + R * y1;
            x1 = input;
            y1 = preventDenormal(output);
            return output;
        }
        
        void reset() { x1 = y1 = 0.0; }
    };
    
    // Per-channel processor with aligned memory
    struct alignas(ALIGNMENT) ChannelProcessor {
        CrossoverNetwork crossover;
        std::array<PolyphaseOversampler, 3> oversamplers; // Low, Mid, High
        DCBlocker inputDC, outputDC;
        
        // Pre-allocated aligned buffers
        alignas(ALIGNMENT) std::vector<double> inputBuffer;
        alignas(ALIGNMENT) std::vector<double> lowBand;
        alignas(ALIGNMENT) std::vector<double> midBand;
        alignas(ALIGNMENT) std::vector<double> highBand;
        alignas(ALIGNMENT) std::vector<double> oversampledBuffer;
        alignas(ALIGNMENT) std::vector<double> oversampledOutput;
        
        // Per-channel saturation states (no statics!)
        struct SaturationStates {
            // Tube states
            double tubePreEmphState = 0.0;
            double tubeDeEmphState = 0.0;
            
            // Tape states
            double tapeHystState = 0.0;
            double tapeHighState = 0.0;
            
            // Transistor states
            double transistorCouplingState = 0.0;
            
            // Diode states
            double diodeCapState = 0.0;
            double diodeRecoveryState = 0.0;
            double diodeTempDrift = 0.0;
            
            void reset() {
                tubePreEmphState = tubeDeEmphState = 0.0;
                tapeHystState = tapeHighState = 0.0;
                transistorCouplingState = 0.0;
                diodeCapState = diodeRecoveryState = diodeTempDrift = 0.0;
            }
        } satStates;
        
        void prepare(double sampleRate, int maxBlockSize);
        void reset();
    };
    
    // Member variables
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    
    // Parameters with per-sample smoothing
    SmoothParam m_lowDrive;
    SmoothParam m_midDrive;
    SmoothParam m_highDrive;
    SmoothParam m_saturationType;
    SmoothParam m_harmonicCharacter;
    SmoothParam m_outputGain;
    SmoothParam m_mix;
    
    // Channel processors
    std::vector<std::unique_ptr<ChannelProcessor>> m_channelProcessors;
    
    // Processing methods
    void processBand(double* samples, int numSamples,
                    PolyphaseOversampler& oversampler,
                    double* oversampledBuffer,
                    double* oversampledOutput,
                    SaturationType type,
                    ChannelProcessor& processor) noexcept;
    
    // Saturation algorithms (all use channel-specific states)
    double saturateTube(double input, double drive, double harmonics,
                       ChannelProcessor::SaturationStates& states) const noexcept;
    double saturateTape(double input, double drive, double harmonics,
                       ChannelProcessor::SaturationStates& states) const noexcept;
    double saturateTransistor(double input, double drive, double harmonics,
                             ChannelProcessor::SaturationStates& states) const noexcept;
    double saturateDiode(double input, double drive, double harmonics,
                        ChannelProcessor::SaturationStates& states) const noexcept;
    
    // SIMD mixing
    void mixBandsSIMD(float* output, const double* low, const double* mid,
                     const double* high, int numSamples) noexcept;
    
    // RNG for thermal drift
    std::mt19937 m_rng{std::random_device{}()};
};