#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include <random>
#include <cmath>

// Performance-critical inline hints
#if defined(_MSC_VER)
    #define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE inline __attribute__((always_inline))
#else
    #define FORCE_INLINE inline
#endif

// Professional constants for analog modeling
namespace DistortionConstants {
    // Physical constants
    constexpr double BOLTZMANN = 1.380649e-23;    // Boltzmann constant (J/K)
    constexpr double ELECTRON_CHARGE = 1.602176634e-19; // Elementary charge (C)
    constexpr double ROOM_TEMP_KELVIN = 298.15;   // Room temperature (K)
    constexpr double THERMAL_VOLTAGE = 0.026;     // kT/q at room temperature (V)
    
    // Circuit component values (based on actual pedals)
    constexpr double RAT_OPAMP_GAIN = 100000.0;   // LM308 open-loop gain
    constexpr double RAT_SLEW_RATE = 0.5e6;       // V/s
    constexpr double TS_DIODE_IS = 1e-14;         // 1N4148 saturation current
    constexpr double TS_DIODE_N = 1.752;          // 1N4148 ideality factor
    
    // Oversampling
    constexpr int OVERSAMPLE_FACTOR = 4;
    constexpr double OVERSAMPLE_CUTOFF = 0.45;     // Normalized frequency
    
    // Parameter ranges
    constexpr double MIN_GAIN_DB = 0.0;
    constexpr double MAX_GAIN_DB = 60.0;
    constexpr double MIN_FILTER_HZ = 60.0;
    constexpr double MAX_FILTER_HZ = 5000.0;
    constexpr double MIN_TONE_HZ = 500.0;
    constexpr double MAX_TONE_HZ = 12000.0;
    
    // Denormal prevention
    constexpr double DENORMAL_PREVENTION = 1e-30;
}

class RodentDistortion : public EngineBase {
public:
    RodentDistortion();
    ~RodentDistortion() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Rodent Distortion"; }
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    
private:
    // Thread-safe parameter smoothing
    class ParameterSmoother {
        std::atomic<double> targetValue{0.0};
        double currentValue = 0.0;
        double smoothingCoeff = 0.0;
        double sampleRate = 44100.0;
        
    public:
        void setSampleRate(double sr) {
            sampleRate = sr;
            setSmoothingTime(0.01); // 10ms default
        }
        
        void setSmoothingTime(double timeSeconds) {
            double fc = 1.0 / (2.0 * M_PI * timeSeconds);
            smoothingCoeff = std::exp(-2.0 * M_PI * fc / sampleRate);
        }
        
        void setTarget(double value) {
            targetValue.store(value, std::memory_order_relaxed);
        }
        
        FORCE_INLINE double process() {
            double target = targetValue.load(std::memory_order_relaxed);
            currentValue = target + (currentValue - target) * smoothingCoeff;
            currentValue += DistortionConstants::DENORMAL_PREVENTION;
            currentValue -= DistortionConstants::DENORMAL_PREVENTION;
            return currentValue;
        }
        
        void reset(double value) {
            targetValue.store(value, std::memory_order_relaxed);
            currentValue = value;
        }
        
        double getCurrent() const { return currentValue; }
    };
    
    // Professional Zero-Delay Feedback State Variable Filter
    class ZDFStateVariable {
        double s1 = 0.0, s2 = 0.0;  // State variables
        double g = 0.0;             // Tan(pi*fc/fs)
        double k = 0.0;             // Resonance
        double a1 = 0.0, a2 = 0.0, a3 = 0.0; // Coefficients
        
    public:
        void updateCoefficients(double frequency, double resonance, double sampleRate) {
            g = std::tan(M_PI * frequency / sampleRate);
            k = 2.0 - 2.0 * resonance;
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
        
        struct Outputs {
            double lowpass;
            double highpass;
            double bandpass;
            double notch;
        };
        
        FORCE_INLINE Outputs process(double input) {
            double v3 = input - s2;
            double v1 = a1 * s1 + a2 * v3;
            double v2 = s2 + a2 * s1 + a3 * v3;
            
            s1 = 2.0 * v1 - s1 + DistortionConstants::DENORMAL_PREVENTION;
            s2 = 2.0 * v2 - s2 + DistortionConstants::DENORMAL_PREVENTION;
            
            s1 -= DistortionConstants::DENORMAL_PREVENTION;
            s2 -= DistortionConstants::DENORMAL_PREVENTION;
            
            return {
                .lowpass = v2,
                .highpass = input - k * v1 - v2,
                .bandpass = v1,
                .notch = input - k * v1
            };
        }
        
        void reset() {
            s1 = s2 = 0.0;
        }
    };
    
    // Professional 8th-order Elliptic Filter for anti-aliasing
    class EllipticFilter {
        struct BiquadCoeffs {
            double b0, b1, b2, a1, a2;
        };
        
        struct BiquadState {
            double x1 = 0.0, x2 = 0.0;
            double y1 = 0.0, y2 = 0.0;
            
            FORCE_INLINE double process(double input, const BiquadCoeffs& c) {
                double output = c.b0 * input + c.b1 * x1 + c.b2 * x2 
                               - c.a1 * y1 - c.a2 * y2;
                
                x2 = x1; x1 = input;
                y2 = y1; y1 = output;
                
                // Denormal prevention
                y1 += DistortionConstants::DENORMAL_PREVENTION;
                y1 -= DistortionConstants::DENORMAL_PREVENTION;
                
                return output;
            }
            
            void reset() {
                x1 = x2 = y1 = y2 = 0.0;
            }
        };
        
        std::array<BiquadCoeffs, 4> coeffs;
        std::array<BiquadState, 4> states;
        
    public:
        void design(double normalizedFreq, double passbandRipple = 0.1, double stopbandAtten = 80.0);
        
        FORCE_INLINE double process(double input) {
            double output = input;
            for (size_t i = 0; i < 4; ++i) {
                output = states[i].process(output, coeffs[i]);
            }
            return output;
        }
        
        void reset() {
            for (auto& state : states) {
                state.reset();
            }
        }
    };
    
    // High-quality oversampler
    class Oversampler {
        static constexpr int FACTOR = DistortionConstants::OVERSAMPLE_FACTOR;
        EllipticFilter upsampleFilter;
        EllipticFilter downsampleFilter;
        std::vector<double> oversampledBuffer;
        
    public:
        void prepare(int blockSize, double sampleRate) {
            oversampledBuffer.resize(blockSize * FACTOR);
            
            // Design steep elliptic filters
            double normalizedCutoff = DistortionConstants::OVERSAMPLE_CUTOFF / FACTOR;
            upsampleFilter.design(normalizedCutoff);
            downsampleFilter.design(normalizedCutoff);
        }
        
        void upsample(const double* input, double* output, int numSamples);
        void downsample(const double* input, double* output, int numSamples);
        
        void reset() {
            upsampleFilter.reset();
            downsampleFilter.reset();
        }
    };
    
    // Accurate analog component models
    class AnalogComponents {
    public:
        // Op-amp model (LM308 for RAT)
        struct OpAmpLM308 {
            double lastOutput = 0.0;
            double compensationCap = 30e-12; // 30pF compensation
            double slewRate = DistortionConstants::RAT_SLEW_RATE;
            
            FORCE_INLINE double process(double input, double gain, double sampleRate) {
                // Open-loop gain with frequency rolloff
                // double openLoopGain = DistortionConstants::RAT_OPAMP_GAIN; // TODO: Use for frequency response modeling
                // double gbProduct = 1e6; // 1MHz gain-bandwidth product // TODO: Use for frequency response modeling
                
                // Slew rate limiting
                double maxDelta = slewRate / sampleRate;
                double targetOutput = input * gain;
                double delta = targetOutput - lastOutput;
                
                if (std::abs(delta) > maxDelta) {
                    delta = maxDelta * (delta > 0 ? 1.0 : -1.0);
                }
                
                lastOutput += delta;
                
                // Output stage saturation
                const double vcc = 9.0; // 9V battery
                const double satVoltage = vcc - 1.5; // Headroom
                
                if (lastOutput > satVoltage) {
                    lastOutput = satVoltage - 0.1 * std::exp(-(lastOutput - satVoltage));
                } else if (lastOutput < -satVoltage) {
                    lastOutput = -satVoltage + 0.1 * std::exp(lastOutput + satVoltage);
                }
                
                return lastOutput;
            }
            
            void reset() { lastOutput = 0.0; }
        };
        
        // Diode clipping model (1N4148 / 1N914)
        struct DiodeClipper {
            double temperature = DistortionConstants::ROOM_TEMP_KELVIN;
            double thermalVoltage = DistortionConstants::THERMAL_VOLTAGE;
            
            FORCE_INLINE double process(double voltage, bool isGermanium = false) {
                // Temperature-dependent parameters
                double vt = (DistortionConstants::BOLTZMANN * temperature) / 
                           DistortionConstants::ELECTRON_CHARGE;
                
                // Diode parameters
                double is = isGermanium ? 1e-9 : DistortionConstants::TS_DIODE_IS;
                double n = isGermanium ? 1.0 : DistortionConstants::TS_DIODE_N;
                double vf = isGermanium ? 0.3 : 0.7;
                
                // Shockley diode equation with series resistance
                double rs = 10.0; // 10 ohm series resistance
                
                if (voltage > vf * 0.5) {
                    double id = is * (std::exp(voltage / (n * vt)) - 1.0);
                    double vd = voltage - id * rs;
                    return vd;
                }
                
                return voltage;
            }
        };
        
        // Transistor model for fuzz
        struct TransistorModel {
            double beta = 100.0;     // Current gain
            double vbe = 0.7;        // Base-emitter voltage
            double leakage = 1e-9;   // Leakage current
            
            FORCE_INLINE double process(double input, double bias) {
                // Simple Ebers-Moll model
                double vb = input + bias;
                
                if (vb < vbe) {
                    return leakage * vb; // Cutoff region
                }
                
                // Active region with soft transition
                double ib = (vb - vbe) / 1000.0; // Base current
                double ic = beta * ib * (1.0 - std::exp(-vb));
                
                // Saturation
                const double vcesat = 0.2;
                if (ic > 0.01) {
                    ic = 0.01 - vcesat * std::exp(-ic * 100.0);
                }
                
                return ic * 470.0; // Collector resistor
            }
        };
    };
    
    // Thermal modeling with realistic physics
    class ThermalModel {
        double junctionTemp = DistortionConstants::ROOM_TEMP_KELVIN;
        double ambientTemp = DistortionConstants::ROOM_TEMP_KELVIN;
        double thermalMass = 0.001;        // Thermal mass (J/K)
        double thermalResistance = 150.0;  // Thermal resistance (K/W)
        double dissipatedPower = 0.0;
        
    public:
        void update(double power, double deltaTime) {
            dissipatedPower = power;
            
            // Newton's law of cooling
            double tempDiff = junctionTemp - ambientTemp;
            double heatFlow = tempDiff / thermalResistance;
            double tempChange = (dissipatedPower - heatFlow) * deltaTime / thermalMass;
            
            junctionTemp += tempChange;
            junctionTemp = std::max(ambientTemp, std::min(junctionTemp, 400.0)); // Max 127°C
        }
        
        double getTemperature() const { return junctionTemp; }
        double getThermalVoltage() const {
            return (DistortionConstants::BOLTZMANN * junctionTemp) / 
                   DistortionConstants::ELECTRON_CHARGE;
        }
    };
    
    // Vintage circuit models
    enum class VintageMode {
        RAT,           // ProCo RAT
        TUBE_SCREAMER, // Ibanez TS808/TS9
        BIG_MUFF,      // Electro-Harmonix Big Muff Pi
        FUZZ_FACE      // Dallas Arbiter Fuzz Face
    };
    
    // DSP members
    double m_sampleRate = 44100.0;
    
    // Parameters
    std::unique_ptr<ParameterSmoother> m_gain;
    std::unique_ptr<ParameterSmoother> m_filter;
    std::unique_ptr<ParameterSmoother> m_clipping;
    std::unique_ptr<ParameterSmoother> m_tone;
    std::unique_ptr<ParameterSmoother> m_output;
    std::unique_ptr<ParameterSmoother> m_mix;
    std::unique_ptr<ParameterSmoother> m_distortionType;
    std::unique_ptr<ParameterSmoother> m_presence;
    
    // Filters (stereo)
    std::array<ZDFStateVariable, 2> m_inputFilters;
    std::array<ZDFStateVariable, 2> m_toneFilters;
    
    // Oversampling
    std::array<std::unique_ptr<Oversampler>, 2> m_oversamplers;
    
    // Analog models
    std::array<AnalogComponents::OpAmpLM308, 2> m_opAmps;
    std::array<AnalogComponents::DiodeClipper, 2> m_diodeClippers;
    std::array<AnalogComponents::TransistorModel, 2> m_transistors;
    
    // Thermal model
    ThermalModel m_thermalModel;
    
    // DC blocking with proper time constant
    class DCBlocker {
        double x1 = 0.0, y1 = 0.0;
        double cutoff = 0.0;
        
    public:
        void setCutoff(double freqHz, double sampleRate) {
            cutoff = 1.0 - std::exp(-2.0 * M_PI * freqHz / sampleRate);
        }
        
        FORCE_INLINE double process(double input) {
            y1 = input - x1 + y1 * (1.0 - cutoff);
            x1 = input;
            
            // Denormal prevention
            y1 += DistortionConstants::DENORMAL_PREVENTION;
            y1 -= DistortionConstants::DENORMAL_PREVENTION;
            
            return y1;
        }
        
        void reset() { x1 = y1 = 0.0; }
    };
    
    std::array<DCBlocker, 2> m_inputDCBlockers;
    std::array<DCBlocker, 2> m_outputDCBlockers;
    
    // Circuit-specific processing
    double processRATCircuit(double input, int channel);
    double processTubeScreamerCircuit(double input, int channel);
    double processBigMuffCircuit(double input, int channel);
    double processFuzzFaceCircuit(double input, int channel);
    
    // Helper functions
    static double tanhApproximation(double x);
    static double softClipAsymmetric(double x, double amount);
};