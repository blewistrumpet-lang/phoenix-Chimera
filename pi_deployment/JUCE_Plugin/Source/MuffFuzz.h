#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <random>
#include <cmath>

// Professional Big Muff Pi emulation with modern enhancements
class MuffFuzz : public EngineBase {
public:
    MuffFuzz();
    ~MuffFuzz() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Muff Fuzz"; }
    int getNumParameters() const override { return 7; }
    juce::String getParameterName(int index) const override;
    
private:
    // Professional constants
    static constexpr double DENORMAL_PREVENTION = 1e-30;
    static constexpr int OVERSAMPLE_FACTOR = 4;
    static constexpr double DIODE_THRESHOLD = 0.7;
    static constexpr double TRANSISTOR_VBE = 0.65;
    
    // Thread-safe parameter smoothing
    class ParameterSmoother {
        std::atomic<double> targetValue{0.0};
        double currentValue = 0.0;
        double smoothingCoeff = 0.0;
        double sampleRate = 0.0;
        
    public:
        void setSampleRate(double sr) {
            sampleRate = sr;
            setSmoothingTime(0.01);
        }
        
        void setSmoothingTime(double timeSeconds) {
            double fc = 1.0 / (2.0 * M_PI * timeSeconds);
            smoothingCoeff = std::exp(-2.0 * M_PI * fc / sampleRate);
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
    
    // Accurate Big Muff tone stack
    class BigMuffToneStack {
        // Component values from original circuit
        static constexpr double R1 = 39000.0;  // 39k
        static constexpr double R2 = 22000.0;  // 22k  
        static constexpr double R3 = 22000.0;  // 22k
        static constexpr double R4 = 100000.0; // 100k (tone pot)
        static constexpr double C1 = 10e-9;    // 10nF
        static constexpr double C2 = 4e-9;     // 4nF (some versions use 3.9nF)
        
        // Digital filter coefficients
        double b0, b1, b2, a0, a1, a2;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        
    public:
        void updateCoefficients(double tonePosition, double sampleRate);
        double process(double input);
        void reset() { x1 = x2 = y1 = y2 = 0; }
    };
    
    // Transistor clipping stage (BC239C or 2N5088)
    class TransistorClippingStage {
        double c1 = 0, c2 = 0;  // Coupling capacitors state
        double vbe = TRANSISTOR_VBE;
        double beta = 400.0;    // High gain transistor
        double collectorCurrent = 0;
        double temperature = 298.15; // Kelvin
        
    public:
        void setSampleRate(double sr) {
            // Set RC time constants for coupling caps
            double rc = 0.1;  // 100ms
            c1 = c2 = 1.0 - std::exp(-1.0 / (rc * sr));
        }
        
        double process(double input, double gain, double bias);
        void setTemperature(double tempK) { temperature = tempK; }
        void reset() { collectorCurrent = 0; }
    };
    
    // Back-to-back diode clipping
    class DiodeClipper {
        // Silicon diode parameters
        static constexpr double IS = 1e-14;   // Saturation current
        static constexpr double N = 1.5;      // Ideality factor
        static constexpr double VT = 0.026;   // Thermal voltage at room temp
        
        double temperature = 298.15;
        
    public:
        double process(double voltage);
        void setTemperature(double tempK) { temperature = tempK; }
    };
    
    // Professional oversampling
    class Oversampler {
        struct ButterworthStage {
            double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            double b0, b1, b2, a1, a2;
            
            void design(double cutoff) {
                double c = 1.0 / std::tan(M_PI * cutoff);
                double c2 = c * c;
                double sqrt2c = std::sqrt(2.0) * c;
                double a0 = c2 + sqrt2c + 1.0;
                
                b0 = 1.0 / a0;
                b1 = 2.0 / a0;
                b2 = 1.0 / a0;
                a1 = (2.0 - 2.0 * c2) / a0;
                a2 = (c2 - sqrt2c + 1.0) / a0;
            }
            
            double process(double input) {
                double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = input;
                y2 = y1; y1 = output;
                return output;
            }
            
            void reset() { x1 = x2 = y1 = y2 = 0; }
        };
        
        std::array<ButterworthStage, 4> upsampleFilters;
        std::array<ButterworthStage, 4> downsampleFilters;
        std::vector<double> oversampledBuffer;
        
    public:
        void prepare(int blockSize, double sampleRate);
        void upsample(const double* input, double* output, int numSamples);
        void downsample(const double* input, double* output, int numSamples);
        void reset();
    };
    
    // Complete Big Muff circuit model
    class BigMuffCircuit {
        // Four gain stages as in original
        TransistorClippingStage inputBuffer;
        TransistorClippingStage clippingStage1;
        TransistorClippingStage clippingStage2;
        TransistorClippingStage outputBuffer;
        
        // Diode clippers after each clipping stage
        DiodeClipper diodeClipper1;
        DiodeClipper diodeClipper2;
        
        // Tone stack between stages
        BigMuffToneStack toneStack;
        
        // Component variations
        double transistorMatching = 1.0;
        double diodeMatching = 1.0;
        
        // Store sample rate for tone stack
        double circuitSampleRate = 0.0;
        
    public:
        void prepare(double sampleRate);
        double process(double input, double sustain, double tone, double volume);
        void setTemperature(double tempK);
        void setComponentVariation(double matching);
        void reset();
    };
    
    // Modern fuzz variations
    enum class FuzzVariant {
        TRIANGLE_1971,      // Original "Triangle" Big Muff
        RAMS_HEAD_1973,     // "Ram's Head" version
        NYC_REISSUE,        // Modern NYC reissue
        RUSSIAN_SOVTEK,     // Russian "Civil War" Sovtek
        OP_AMP_VERSION,     // Op-amp based variant
        MODERN_DELUXE       // Modern enhanced version
    };
    
    // Gate with hysteresis
    class NoiseGate {
        double envelope = 0;
        double gateState = 1.0;
        double attackTime = 0.001;
        double releaseTime = 0.01;
        double hysteresis = 0.9;
        
    public:
        void setSampleRate(double sr) {
            attackTime = 1.0 - std::exp(-1.0 / (0.001 * sr));
            releaseTime = 1.0 - std::exp(-1.0 / (0.01 * sr));
        }
        
        double process(double input, double threshold);
        void reset() { envelope = 0; gateState = 1.0; }
    };
    
    // Mid scoop control (not in original but useful)
    class MidScoopFilter {
        double b0, b1, b2, a1, a2;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        
    public:
        void updateCoefficients(double frequency, double depth, double sampleRate);
        double process(double input);
        void reset() { x1 = x2 = y1 = y2 = 0; }
    };
    
    // DSP members
    double m_sampleRate = 0.0;
    
    // Parameters
    std::unique_ptr<ParameterSmoother> m_sustain;
    std::unique_ptr<ParameterSmoother> m_tone;
    std::unique_ptr<ParameterSmoother> m_volume;
    std::unique_ptr<ParameterSmoother> m_gate;
    std::unique_ptr<ParameterSmoother> m_mids;
    std::unique_ptr<ParameterSmoother> m_variant;
    std::unique_ptr<ParameterSmoother> m_mix;
    
    // Processing components (stereo)
    std::array<BigMuffCircuit, 2> m_circuits;
    std::array<Oversampler, 2> m_oversamplers;
    std::array<NoiseGate, 2> m_gates;
    std::array<MidScoopFilter, 2> m_midScoops;
    
    // DC blocking
    class DCBlocker {
        double x1 = 0, y1 = 0;
        double cutoff = 0;
        
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
        
        void reset() { x1 = y1 = 0; }
    };
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // Thermal modeling
    class ThermalModel {
        double junctionTemp = 298.15;
        double ambientTemp = 298.15;
        double thermalResistance = 200.0;  // K/W
        double dissipatedPower = 0;
        
    public:
        void update(double power, double deltaTime) {
            dissipatedPower = power;
            double tempRise = power * thermalResistance;
            double tau = 10.0; // seconds
            double alpha = 1.0 - std::exp(-deltaTime / tau);
            junctionTemp += ((ambientTemp + tempRise) - junctionTemp) * alpha;
        }
        
        double getTemperature() const { return junctionTemp; }
    };
    
    ThermalModel m_thermalModel;
    
    // Apply variant-specific modifications
    void applyVariantSettings(FuzzVariant variant);
};