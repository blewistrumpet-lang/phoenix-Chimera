// VintageTubePreamp_Ultimate.h - Absolute Highest Quality Studio Implementation
#pragma once

#include "EngineBase.h"
#include <array>
#include <memory>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <vector>
#include <complex>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Professional denormal protection using bit manipulation
inline float flushDenormalFloat(float value) {
    union { float f; uint32_t i; } u;
    u.f = value;
    if ((u.i & 0x7F800000) == 0) return 0.0f;
    return value;
}

inline double flushDenormalDouble(double value) {
    union { double d; uint64_t i; } u;
    u.d = value;
    if ((u.i & 0x7FF0000000000000ULL) == 0) return 0.0;
    return value;
}

class VintageTubePreamp : public EngineBase {
public:
    VintageTubePreamp();
    ~VintageTubePreamp() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Vintage Tube Preamp Ultimate"; }
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    
private:
    // Professional constants
    static constexpr int OVERSAMPLE_FACTOR = 8;  // 8x for extreme quality
    static constexpr int MAX_BLOCK_SIZE = 2048;
    static constexpr int WAVESHAPER_POINTS = 4096;
    static constexpr double THERMAL_NOISE_FLOOR = 1e-12;  // -120dB
    
    // Thread-safe parameter smoothing with sample-accurate automation
    class SmoothedParameter {
        std::atomic<double> m_targetValue{0.0};
        double m_currentValue = 0.0;
        double m_smoothingCoeff = 0.995;
        double m_sampleRate = 44100.0;
        
    public:
        void setSampleRate(double sr, double smoothingMs = 20.0) {
            m_sampleRate = sr;
            double fc = 1000.0 / (2.0 * M_PI * smoothingMs);
            m_smoothingCoeff = std::exp(-2.0 * M_PI * fc / sr);
        }
        
        void setTarget(double value) {
            m_targetValue.store(value, std::memory_order_relaxed);
        }
        
        double getNextValue() {
            double target = m_targetValue.load(std::memory_order_relaxed);
            m_currentValue = target + (m_currentValue - target) * m_smoothingCoeff;
            return flushDenormalDouble(m_currentValue);
        }
        
        void reset(double value) {
            m_targetValue.store(value, std::memory_order_relaxed);
            m_currentValue = value;
        }
    };
    
    // Advanced SPICE-based tube model with multiple tube types
    class AdvancedTubeModel {
    public:
        enum class TubeType {
            ECC83_12AX7,    // High gain preamp
            ECC82_12AU7,    // Medium mu
            ECC81_12AT7,    // High transconductance
            EF86,           // Pentode
            E88CC_6922,     // Low noise
            EL34,           // Power pentode
            EL84,           // Power pentode
            KT88,           // Beam tetrode
            MODEL_300B,     // Triode power
            MODEL_2A3       // Direct heated triode
        };
        
        struct TubeParams {
            double mu;          // Amplification factor
            double ex;          // Exponent
            double kg1;         // Grid current constant
            double kp;          // Plate current constant
            double kvb;         // Plate knee constant
            double rp;          // Plate resistance
            double gm;          // Transconductance
            double cgk;         // Grid-cathode capacitance
            double cpk;         // Plate-cathode capacitance
            double cgp;         // Miller capacitance
            double heaterNoise; // Heater-induced noise
            double shotNoise;   // Shot noise coefficient
        };
        
    private:
        TubeParams m_params;
        double m_plateVoltage = 250.0;
        double m_cathodeVoltage = 1.5;
        double m_gridBias = -1.5;
        double m_heaterModulation = 0.0;
        double m_thermalState = 0.0;
        double m_cathodeBypass = 0.0;
        double m_millerCapState = 0.0;
        std::mt19937 m_noiseGen{std::random_device{}()};
        std::normal_distribution<double> m_noiseDist{0.0, 1.0};
        
    public:
        void setTubeType(TubeType type) {
            switch (type) {
                case TubeType::ECC83_12AX7:
                    m_params = {100.0, 1.4, 1.0e-6, 1.32e-3, 300.0, 62500.0, 1.6e-3, 
                               1.6e-12, 11e-12, 1.7e-12, 1e-9, 2e-10};
                    break;
                case TubeType::ECC82_12AU7:
                    m_params = {17.0, 1.3, 1.0e-6, 2.4e-3, 250.0, 7700.0, 2.2e-3,
                               1.5e-12, 12e-12, 1.5e-12, 0.8e-9, 1.8e-10};
                    break;
                case TubeType::ECC81_12AT7:
                    m_params = {60.0, 1.35, 1.0e-6, 1.8e-3, 270.0, 10900.0, 5.5e-3,
                               1.55e-12, 10e-12, 1.6e-12, 0.9e-9, 1.9e-10};
                    break;
                case TubeType::EF86:
                    m_params = {2000.0, 1.4, 0.5e-6, 0.8e-3, 350.0, 2.5e6, 2.0e-3,
                               2.8e-12, 5.5e-12, 0.05e-12, 0.7e-9, 1.5e-10};
                    break;
                case TubeType::E88CC_6922:
                    m_params = {33.0, 1.35, 0.8e-6, 2.1e-3, 260.0, 12500.0, 2.6e-3,
                               1.4e-12, 10.5e-12, 1.4e-12, 0.5e-9, 1.2e-10};
                    break;
                case TubeType::EL34:
                    m_params = {11.0, 1.35, 3e-6, 8e-3, 450.0, 900.0, 11e-3,
                               15e-12, 20e-12, 8e-12, 2e-9, 3e-10};
                    break;
                case TubeType::EL84:
                    m_params = {19.0, 1.4, 2e-6, 5e-3, 380.0, 2300.0, 8.3e-3,
                               12e-12, 18e-12, 6e-12, 1.8e-9, 2.8e-10};
                    break;
                case TubeType::KT88:
                    m_params = {8.0, 1.35, 4e-6, 10e-3, 500.0, 670.0, 12e-3,
                               18e-12, 25e-12, 10e-12, 2.5e-9, 3.5e-10};
                    break;
                case TubeType::MODEL_300B:
                    m_params = {3.85, 1.4, 5e-6, 15e-3, 400.0, 700.0, 5.5e-3,
                               20e-12, 30e-12, 15e-12, 3e-9, 4e-10};
                    break;
                case TubeType::MODEL_2A3:
                    m_params = {4.2, 1.4, 4.5e-6, 12e-3, 350.0, 800.0, 5.25e-3,
                               18e-12, 28e-12, 14e-12, 2.8e-9, 3.8e-10};
                    break;
            }
        }
        
        double process(double input, double drive, double bias, double sampleRate) {
            // Grid voltage with bias and Miller effect
            double gridVoltage = input * (1.0 + drive * 10.0) + m_gridBias + (bias - 0.5) * 5.0;
            
            // Miller capacitance effect (frequency-dependent)
            double millerCutoff = 1.0 / (2.0 * M_PI * m_params.cgp * m_params.rp);
            double millerAlpha = std::exp(-millerCutoff / sampleRate);
            m_millerCapState = gridVoltage + (m_millerCapState - gridVoltage) * millerAlpha;
            gridVoltage = m_millerCapState;
            
            // Advanced plate current calculation with secondary emission
            double plateCurrent = calculatePlateCurrent(gridVoltage, m_plateVoltage, m_cathodeVoltage);
            
            // Thermal effects and drift
            m_thermalState += (plateCurrent * 0.001 - m_thermalState) * 0.0001;
            plateCurrent *= (1.0 + m_thermalState * 0.02);
            
            // Heater-induced hum and microphonics
            m_heaterModulation = std::sin(2.0 * M_PI * 60.0 / sampleRate) * m_params.heaterNoise;
            plateCurrent += m_heaterModulation;
            
            // Shot noise and thermal noise
            double shotNoise = std::sqrt(std::abs(plateCurrent)) * m_params.shotNoise * m_noiseDist(m_noiseGen);
            double thermalNoise = THERMAL_NOISE_FLOOR * m_noiseDist(m_noiseGen);
            
            // Output with noise components
            double output = (plateCurrent + shotNoise + thermalNoise) * m_params.rp * 0.001;
            
            // Cathode bypass capacitor effect
            double cathodeAlpha = std::exp(-10.0 / sampleRate);
            m_cathodeBypass = output + (m_cathodeBypass - output) * cathodeAlpha;
            output += (output - m_cathodeBypass) * 0.3;  // Frequency-dependent gain
            
            return flushDenormalDouble(output);
        }
        
    private:
        double calculatePlateCurrent(double vg, double vp, double vk) {
            double vgk = vg - vk;
            double vpk = vp - vk;
            
            // Grid current for positive grid
            double gridCurrent = 0.0;
            if (vgk > -0.5) {
                gridCurrent = m_params.kg1 * std::pow(std::max(0.0, vgk + 0.5), 1.5);
                vgk -= gridCurrent * 10000.0;
            }
            
            // Child-Langmuir with Koren corrections
            double E1 = vpk / m_params.mu + vgk;
            if (E1 <= 0.0) return 0.0;
            
            // Space charge effects
            double spaceCharge = 1.0 / (1.0 + std::exp(-E1 * 0.1));
            
            // Plate current with knee and secondary emission
            double denominator = 1.0 + std::pow(E1 / m_params.kvb, m_params.ex);
            double plateCurrent = m_params.kp * std::pow(E1, 1.5) / denominator * spaceCharge;
            
            // Secondary emission at high plate voltages
            if (vpk > 300.0) {
                plateCurrent *= (1.0 + (vpk - 300.0) * 0.0001);
            }
            
            return std::max(0.0, plateCurrent);
        }
        
    public:
        void reset() {
            m_thermalState = 0.0;
            m_cathodeBypass = 0.0;
            m_millerCapState = 0.0;
            m_heaterModulation = 0.0;
        }
    };
    
    // Professional output transformer model
    class TransformerModel {
        // Primary inductance and core saturation
        double m_primaryInductance = 10.0;  // Henries
        double m_coreFlux = 0.0;
        double m_saturationFlux = 1.5;      // Tesla
        double m_hysteresisState = 0.0;
        
        // Frequency-dependent losses
        double m_eddyCurrentLoss = 0.0;
        double m_copperLoss = 0.0;
        
        // Resonances
        struct Resonance {
            double freq;
            double q;
            double gain;
            double state1 = 0.0;
            double state2 = 0.0;
        };
        std::array<Resonance, 3> m_resonances;
        
    public:
        void prepare(double sampleRate) {
            // Typical transformer resonances
            m_resonances[0] = {80.0, 2.0, 1.2, 0.0, 0.0};    // Low resonance
            m_resonances[1] = {3000.0, 4.0, 1.1, 0.0, 0.0};  // Mid resonance  
            m_resonances[2] = {12000.0, 3.0, 0.9, 0.0, 0.0}; // High resonance
            
            for (auto& res : m_resonances) {
                double omega = 2.0 * M_PI * res.freq / sampleRate;
                // Calculate coefficients for resonant filter
                double sinOmega = std::sin(omega);
                double cosOmega = std::cos(omega);
                double alpha = sinOmega / (2.0 * res.q);
                // Store pre-calculated values
                res.state1 = cosOmega;
                res.state2 = alpha;
            }
        }
        
        double process(double input, double sampleRate) {
            // Core flux with saturation
            double fluxRate = input * m_primaryInductance;
            m_coreFlux += fluxRate / sampleRate;
            
            // Soft saturation using tanh
            double saturatedFlux = m_saturationFlux * std::tanh(m_coreFlux / m_saturationFlux);
            
            // Hysteresis modeling
            double hysteresis = saturatedFlux - m_coreFlux;
            m_hysteresisState = flushDenormalDouble(m_hysteresisState * 0.95 + hysteresis * 0.05);
            
            // Frequency-dependent losses
            m_eddyCurrentLoss = fluxRate * fluxRate * 0.0001;  // Proportional to (df/dt)²
            m_copperLoss = input * input * 0.001;              // I²R losses
            
            // Apply losses
            double output = saturatedFlux - m_eddyCurrentLoss - m_copperLoss + m_hysteresisState;
            
            // Apply resonances
            for (auto& res : m_resonances) {
                double filtered = output * res.gain;
                // Simple resonant filter processing
                output += filtered * res.state2;
            }
            
            // Leakage flux reduction
            m_coreFlux *= 0.9999;
            
            return flushDenormalDouble(output);
        }
        
        void reset() {
            m_coreFlux = 0.0;
            m_hysteresisState = 0.0;
            m_eddyCurrentLoss = 0.0;
            m_copperLoss = 0.0;
            for (auto& res : m_resonances) {
                res.state1 = res.state2 = 0.0;
            }
        }
    };
    
    // Studio-grade passive EQ (Pultec-style)
    class PultecEQ {
        struct Band {
            enum Type { SHELF, BELL, HIGH_SHELF };
            Type type;
            double freq;
            double q;
            double boost = 0.0;
            double cut = 0.0;
            
            // State variables
            double x1 = 0.0, x2 = 0.0;
            double y1 = 0.0, y2 = 0.0;
            double b0 = 1.0, b1 = 0.0, b2 = 0.0;
            double a1 = 0.0, a2 = 0.0;
            
            void updateCoefficients(double sampleRate) {
                double omega = 2.0 * M_PI * freq / sampleRate;
                double sinw = std::sin(omega);
                double cosw = std::cos(omega);
                
                // Simultaneous boost and cut (Pultec trick)
                double gainBoost = std::pow(10.0, boost / 20.0);
                double gainCut = std::pow(10.0, -cut / 20.0);
                double A = std::sqrt(gainBoost * gainCut);
                
                double alpha = sinw / (2.0 * q);
                double beta = std::sqrt(A) / q;
                
                switch (type) {
                    case SHELF: {
                        // Low shelf with Pultec curve
                        double S = 1.0;
                        b0 = A * ((A + 1) - (A - 1) * cosw + beta * sinw);
                        b1 = 2 * A * ((A - 1) - (A + 1) * cosw);
                        b2 = A * ((A + 1) - (A - 1) * cosw - beta * sinw);
                        double a0 = (A + 1) + (A - 1) * cosw + beta * sinw;
                        a1 = -2 * ((A - 1) + (A + 1) * cosw);
                        a2 = (A + 1) + (A - 1) * cosw - beta * sinw;
                        
                        // Normalize
                        b0 /= a0; b1 /= a0; b2 /= a0;
                        a1 /= a0; a2 /= a0;
                        break;
                    }
                    case BELL: {
                        // Parametric bell
                        b0 = 1 + alpha * A;
                        b1 = -2 * cosw;
                        b2 = 1 - alpha * A;
                        double a0 = 1 + alpha / A;
                        a1 = -2 * cosw;
                        a2 = 1 - alpha / A;
                        
                        // Normalize
                        b0 /= a0; b1 /= a0; b2 /= a0;
                        a1 /= a0; a2 /= a0;
                        break;
                    }
                    case HIGH_SHELF: {
                        // High shelf with air band
                        b0 = A * ((A + 1) + (A - 1) * cosw + beta * sinw);
                        b1 = -2 * A * ((A - 1) + (A + 1) * cosw);
                        b2 = A * ((A + 1) + (A - 1) * cosw - beta * sinw);
                        double a0 = (A + 1) - (A - 1) * cosw + beta * sinw;
                        a1 = 2 * ((A - 1) - (A + 1) * cosw);
                        a2 = (A + 1) - (A - 1) * cosw - beta * sinw;
                        
                        // Normalize
                        b0 /= a0; b1 /= a0; b2 /= a0;
                        a1 /= a0; a2 /= a0;
                        break;
                    }
                }
            }
            
            double process(double input) {
                double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = flushDenormalDouble(x1);
                x1 = flushDenormalDouble(input);
                y2 = flushDenormalDouble(y1);
                y1 = flushDenormalDouble(output);
                return output;
            }
            
            void reset() {
                x1 = x2 = y1 = y2 = 0.0;
            }
        };
        
        std::array<Band, 5> m_bands;
        
    public:
        void prepare(double sampleRate) {
            // Classic Pultec frequencies
            m_bands[0] = {Band::SHELF, 60.0, 0.7, 0.0, 0.0};      // Low shelf
            m_bands[1] = {Band::BELL, 200.0, 1.0, 0.0, 0.0};      // Low mid
            m_bands[2] = {Band::BELL, 700.0, 1.5, 0.0, 0.0};      // Mid
            m_bands[3] = {Band::BELL, 3000.0, 2.0, 0.0, 0.0};     // High mid
            m_bands[4] = {Band::HIGH_SHELF, 10000.0, 0.7, 0.0, 0.0}; // Air
            
            for (auto& band : m_bands) {
                band.updateCoefficients(sampleRate);
            }
        }
        
        void setParams(double low, double lowMid, double mid, double highMid, double high, double sampleRate) {
            // Convert 0-1 to ±12dB with center at 0.5
            m_bands[0].boost = std::max(0.0, (low - 0.5) * 24.0);
            m_bands[0].cut = std::max(0.0, (0.5 - low) * 24.0);
            
            m_bands[1].boost = std::max(0.0, (lowMid - 0.5) * 24.0);
            m_bands[1].cut = std::max(0.0, (0.5 - lowMid) * 24.0);
            
            m_bands[2].boost = std::max(0.0, (mid - 0.5) * 24.0);
            m_bands[2].cut = std::max(0.0, (0.5 - mid) * 24.0);
            
            m_bands[3].boost = std::max(0.0, (highMid - 0.5) * 24.0);
            m_bands[3].cut = std::max(0.0, (0.5 - highMid) * 24.0);
            
            m_bands[4].boost = std::max(0.0, (high - 0.5) * 24.0);
            m_bands[4].cut = std::max(0.0, (0.5 - high) * 24.0);
            
            for (auto& band : m_bands) {
                band.updateCoefficients(sampleRate);
            }
        }
        
        double process(double input) {
            double output = input;
            for (auto& band : m_bands) {
                output = band.process(output);
            }
            return output;
        }
        
        void reset() {
            for (auto& band : m_bands) {
                band.reset();
            }
        }
    };
    
    // Professional 8x oversampling with linear phase FIR
    class Oversampler8x {
        static constexpr int FIR_LENGTH = 256;
        
        struct LinearPhaseFIR {
            alignas(64) std::array<double, FIR_LENGTH> coeffs;
            alignas(64) std::array<double, FIR_LENGTH> buffer{0};
            int bufferIndex = 0;
            
            void designKaiser(double cutoff, double sampleRate, double ripple = 0.0001) {
                // Kaiser window FIR design for linear phase
                double beta = 0.0;
                if (ripple < 0.00001) beta = 10.0;
                else if (ripple < 0.0001) beta = 8.0;
                else if (ripple < 0.001) beta = 6.0;
                else beta = 4.0;
                
                double sum = 0.0;
                for (int i = 0; i < FIR_LENGTH; ++i) {
                    double n = i - (FIR_LENGTH - 1) / 2.0;
                    
                    // Sinc function
                    double sinc = (n == 0) ? 1.0 : std::sin(M_PI * cutoff * n / sampleRate) / (M_PI * n);
                    
                    // Kaiser window
                    double x = 2.0 * i / (FIR_LENGTH - 1) - 1.0;
                    double kaiser = besselI0(beta * std::sqrt(1.0 - x * x)) / besselI0(beta);
                    
                    coeffs[i] = sinc * kaiser;
                    sum += coeffs[i];
                }
                
                // Normalize
                for (double& c : coeffs) c /= sum;
            }
            
            double process(double input) {
                buffer[bufferIndex] = input;
                
                double output = 0.0;
                // Optimized convolution (vectorizable)
                for (int i = 0; i < FIR_LENGTH; ++i) {
                    int idx = (bufferIndex - i + FIR_LENGTH) % FIR_LENGTH;
                    output += buffer[idx] * coeffs[i];
                }
                
                bufferIndex = (bufferIndex + 1) % FIR_LENGTH;
                return flushDenormalDouble(output);
            }
            
            void reset() {
                buffer.fill(0.0);
                bufferIndex = 0;
            }
            
        private:
            double besselI0(double x) {
                double sum = 1.0;
                double term = 1.0;
                double x2 = x * x / 4.0;
                
                for (int k = 1; k < 50; ++k) {
                    term *= x2 / (k * k);
                    sum += term;
                    if (term < 1e-15) break;
                }
                
                return sum;
            }
        };
        
        std::array<LinearPhaseFIR, 4> m_upsampleStages;
        std::array<LinearPhaseFIR, 4> m_downsampleStages;
        
    public:
        void prepare(double sampleRate) {
            // Multi-stage filtering for better stopband
            double cutoffs[4] = {
                sampleRate * 0.45,
                sampleRate * 0.9,
                sampleRate * 1.8,
                sampleRate * 3.6
            };
            
            for (int i = 0; i < 4; ++i) {
                m_upsampleStages[i].designKaiser(cutoffs[i], sampleRate * 8, 0.00001);
                m_downsampleStages[i].designKaiser(cutoffs[i], sampleRate * 8, 0.00001);
            }
        }
        
        void processUpsample(const double* input, double* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                // Zero-stuff and filter
                for (int j = 0; j < 8; ++j) {
                    double sample = (j == 0) ? input[i] * 8.0 : 0.0;
                    
                    // Multi-stage filtering
                    for (int stage = 0; stage < 4; ++stage) {
                        sample = m_upsampleStages[stage].process(sample);
                    }
                    
                    output[i * 8 + j] = sample;
                }
            }
        }
        
        void processDownsample(const double* input, double* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                // Filter and decimate
                double accumulator = 0.0;
                
                for (int j = 0; j < 8; ++j) {
                    double sample = input[i * 8 + j];
                    
                    // Multi-stage filtering
                    for (int stage = 0; stage < 4; ++stage) {
                        sample = m_downsampleStages[stage].process(sample);
                    }
                    
                    if (j == 0) accumulator = sample;
                }
                
                output[i] = accumulator;
            }
        }
        
        void reset() {
            for (auto& stage : m_upsampleStages) stage.reset();
            for (auto& stage : m_downsampleStages) stage.reset();
        }
    };
    
    // Power supply modeling
    class PowerSupply {
        double m_voltage = 300.0;
        double m_ripple = 0.0;
        double m_sag = 0.0;
        double m_recovery = 0.0;
        
    public:
        double process(double current, double sampleRate) {
            // Ripple at 120Hz (full-wave rectified)
            m_ripple = std::sin(2.0 * M_PI * 120.0 / sampleRate) * 2.0;
            
            // Voltage sag under load
            m_sag += (current * 50.0 - m_sag) * 0.001;
            
            // Recovery time constant
            m_recovery += (m_sag - m_recovery) * 0.0001;
            
            return flushDenormalDouble(m_voltage - m_sag + m_recovery + m_ripple);
        }
        
        void reset() {
            m_ripple = 0.0;
            m_sag = 0.0;
            m_recovery = 0.0;
        }
    };
    
    // Input stage modeling (guitar pickup interaction)
    class InputStage {
        double m_inputImpedance = 1e6;  // 1M ohm
        double m_cableCapacitance = 500e-12;  // 500pF
        double m_pickupInductance = 4.0;  // 4 Henries
        double m_resonantFreq = 3000.0;
        double m_resonantQ = 2.0;
        double m_state1 = 0.0;
        double m_state2 = 0.0;
        
    public:
        void prepare(double sampleRate) {
            // Calculate pickup resonance
            m_resonantFreq = 1.0 / (2.0 * M_PI * std::sqrt(m_pickupInductance * m_cableCapacitance));
            
            // Loading effect on Q
            double Rp = m_inputImpedance;
            m_resonantQ = Rp * std::sqrt(m_cableCapacitance / m_pickupInductance);
        }
        
        double process(double input, double sampleRate) {
            // Resonant peak from pickup
            double omega = 2.0 * M_PI * m_resonantFreq / sampleRate;
            double sinw = std::sin(omega);
            double cosw = std::cos(omega);
            double alpha = sinw / (2.0 * m_resonantQ);
            
            // State variable filter for resonance
            double hp = input - m_resonantQ * m_state1 - m_state2;
            double bp = hp * omega + m_state1;
            double lp = bp * omega + m_state2;
            
            m_state1 = flushDenormalDouble(bp);
            m_state2 = flushDenormalDouble(lp);
            
            // Mix for resonant peak
            return input + bp * 0.3;
        }
        
        void reset() {
            m_state1 = m_state2 = 0.0;
        }
    };
    
    // Member variables
    double m_sampleRate = 44100.0;
    
    // Parameters
    std::unique_ptr<SmoothedParameter> m_inputGain;
    std::unique_ptr<SmoothedParameter> m_drive;
    std::unique_ptr<SmoothedParameter> m_bias;
    std::unique_ptr<SmoothedParameter> m_bass;
    std::unique_ptr<SmoothedParameter> m_mid;
    std::unique_ptr<SmoothedParameter> m_treble;
    std::unique_ptr<SmoothedParameter> m_presence;
    std::unique_ptr<SmoothedParameter> m_outputGain;
    std::unique_ptr<SmoothedParameter> m_tubeType;
    std::unique_ptr<SmoothedParameter> m_mix;
    
    // DSP Components
    InputStage m_inputStage;
    std::array<AdvancedTubeModel, 2> m_tubeModels;  // Stereo
    std::array<TransformerModel, 2> m_transformers;
    std::array<PultecEQ, 2> m_eqs;
    std::array<PowerSupply, 2> m_powerSupplies;
    std::array<Oversampler8x, 2> m_oversamplers;
    
    // Work buffers
    alignas(64) std::array<double, MAX_BLOCK_SIZE * OVERSAMPLE_FACTOR> m_oversampledBuffers[2];
    
    // Cabinet simulation IRs (simplified)
    std::array<std::array<float, 512>, 2> m_cabinetIRs;
    std::array<int, 2> m_convolutionIndex{0, 0};
    
    // Processing methods
    void processStereo(float* left, float* right, int numSamples);
    void processChannel(double* buffer, int channel, int numSamples);
    AdvancedTubeModel::TubeType getTubeTypeFromParam(float param) const;
};