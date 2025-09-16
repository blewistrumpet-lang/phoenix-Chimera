#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <complex>

/**
 * Advanced Spring Dispersion Model
 * 
 * Implements frequency-dependent wave propagation characteristics of real springs:
 * - Frequency-dependent propagation speed (dispersion)
 * - Mode-dependent damping
 * - Nonlinear spring tension effects
 * - Chirp generation from transients
 * 
 * Based on physical modeling of helical springs
 */
class AdvancedSpringDispersion {
public:
    AdvancedSpringDispersion();
    
    // Configuration
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    
    // Parameters
    void setSpringTension(float tension);     // 0-1: affects dispersion curve
    void setSpringDiameter(float diameter);   // 0-1: affects mode frequencies
    void setMaterialDamping(float damping);   // 0-1: frequency-dependent damping
    void setNonlinearity(float amount);       // 0-1: nonlinear effects strength
    
    // Processing
    float process(float input);
    void processBlock(float* buffer, int numSamples);
    
private:
    // Physical spring parameters
    struct SpringPhysics {
        float youngsModulus = 200e9f;      // Steel: 200 GPa
        float density = 7850.0f;            // Steel: 7850 kg/m³
        float wireRadius = 0.001f;         // 1mm wire
        float coilRadius = 0.01f;          // 10mm coil radius
        float length = 0.3f;                // 30cm spring
        int numCoils = 100;                 // Number of coils
        
        // Derived parameters
        float calculateWaveSpeed(float frequency) const;
        float calculateDispersion(float frequency) const;
        float calculateModeDamping(int modeNumber) const;
    };
    
    // Dispersive delay line using allpass chains
    class DispersiveDelayLine {
    public:
        void prepare(int maxDelay);
        void setDispersion(float amount);
        float process(float input, float delayTime);
        void reset();
        
    private:
        // Allpass filter for frequency-dependent delay
        struct AllpassSection {
            float buffer = 0.0f;
            float coefficient = 0.5f;
            
            float process(float input) {
                float delayed = buffer;
                float output = -input + delayed;
                buffer = input + delayed * coefficient;
                return output;
            }
        };
        
        static constexpr int NUM_ALLPASS = 8;
        std::array<AllpassSection, NUM_ALLPASS> allpassChain;
        std::vector<float> delayBuffer;
        int writePos = 0;
        float dispersionAmount = 0.0f;
    };
    
    // Modal synthesis for spring modes
    class SpringMode {
    public:
        void setFrequency(float freq, double sampleRate);
        void setDamping(float damping);
        void setAmplitude(float amp);
        float process(float excitation);
        void reset();
        
    private:
        // State-variable filter for modal resonance
        float freq = 440.0f;
        float resonance = 0.99f;
        float amplitude = 1.0f;
        float state1 = 0.0f;
        float state2 = 0.0f;
        double sampleRate = 48000.0;
    };
    
    // Nonlinear processor for spring tension effects
    class NonlinearProcessor {
    public:
        void setAmount(float amount);
        float process(float input);
        
    private:
        float amount = 0.0f;
        float prevSample = 0.0f;
        
        // Tension-modulated waveshaping
        float tensionCurve(float x) const {
            // Asymmetric curve simulating spring tension
            if (x > 0) {
                return x * (1.0f + amount * x * 0.3f);
            } else {
                return x * (1.0f - amount * x * 0.5f);
            }
        }
    };
    
    // Chirp generator for transient response
    class ChirpGenerator {
    public:
        void trigger(float intensity);
        float generate();
        void setSweepRate(float rate);
        
    private:
        float phase = 0.0f;
        float frequency = 2000.0f;
        float sweepRate = 0.995f;
        float amplitude = 0.0f;
        float decay = 0.999f;
    };
    
    // Member variables
    double m_sampleRate = 48000.0;
    SpringPhysics m_physics;
    
    // Processing components
    static constexpr int NUM_MODES = 10;
    std::array<SpringMode, NUM_MODES> m_modes;
    std::array<DispersiveDelayLine, 3> m_dispersiveLines;
    NonlinearProcessor m_nonlinearProc;
    ChirpGenerator m_chirpGen;
    
    // Parameters
    float m_tension = 0.5f;
    float m_diameter = 0.5f;
    float m_damping = 0.3f;
    float m_nonlinearity = 0.2f;
    
    // Transient detection for chirp triggering
    float m_envelope = 0.0f;
    float m_prevEnvelope = 0.0f;
    static constexpr float TRANSIENT_THRESHOLD = 0.1f;
    
    // Helper functions
    void updateModes();
    void detectTransient(float input);
};

/**
 * Spring Coupling Matrix
 * Models the mechanical coupling between multiple springs in a reverb tank
 */
class SpringCouplingMatrix {
public:
    static constexpr int MAX_SPRINGS = 4;
    
    SpringCouplingMatrix();
    
    // Set coupling coefficients between springs
    void setCoupling(int spring1, int spring2, float coefficient);
    
    // Process coupled spring system
    void process(std::array<float, MAX_SPRINGS>& springStates,
                const std::array<float, MAX_SPRINGS>& inputs);
    
    // Get feedback for a specific spring including coupling
    float getCoupledFeedback(int springIndex, 
                            const std::array<float, MAX_SPRINGS>& states) const;
    
private:
    // Coupling matrix (symmetric)
    std::array<std::array<float, MAX_SPRINGS>, MAX_SPRINGS> m_couplingMatrix;
    
    // Energy conservation scaling
    void normalizeMatrix();
};