#pragma once
#include "EngineBase.h"
#include <vector>
#include <array>
#include <random>

class ChaosGenerator : public EngineBase {
public:
    ChaosGenerator();
    ~ChaosGenerator() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Chaos Generator"; }
    
private:
    // Smoothed parameters for boutique quality
    struct SmoothParam {
        float target = 0.5f;
        float current = 0.5f;
        float smoothing = 0.995f;
        
        void update() {
            current = target + (current - target) * smoothing;
        }
        
        void reset(float value) {
            target = current = value;
        }
        
        void setSmoothingTime(float timeMs, float sampleRate) {
            float samples = timeMs * 0.001f * sampleRate;
            smoothing = std::exp(-1.0f / samples);
        }
    };
    
    SmoothParam m_rate;           // Chaos update rate (0.1Hz to 100Hz)
    SmoothParam m_depth;          // Chaos intensity
    SmoothParam m_type;           // Chaos type (Lorenz/Rossler/Henon)
    SmoothParam m_smoothing;      // Output smoothing
    SmoothParam m_modTarget;      // What to modulate (amp/pitch/filter/pan)
    SmoothParam m_sync;           // Free/tempo sync
    SmoothParam m_seed;           // Random seed control
    SmoothParam m_mix;            // Dry/wet mix
    
    // Chaos system types
    enum ChaosType {
        LORENZ,
        ROSSLER,
        HENON,
        LOGISTIC,
        IKEDA,
        DUFFING
    };
    
    // Modulation targets
    enum ModTarget {
        AMPLITUDE,
        PITCH,
        FILTER,
        PAN,
        DISTORTION,
        ALL
    };
    
    // Lorenz attractor
    struct LorenzSystem {
        double x = 0.1, y = 0.0, z = 0.0;
        const double sigma = 10.0;
        const double rho = 28.0;
        const double beta = 8.0 / 3.0;
        
        float iterate(double dt) {
            double dx = sigma * (y - x);
            double dy = x * (rho - z) - y;
            double dz = x * y - beta * z;
            
            x += dx * dt;
            y += dy * dt;
            z += dz * dt;
            
            // Return normalized output
            return static_cast<float>(std::tanh(x / 30.0));
        }
    };
    
    // Rossler attractor
    struct RosslerSystem {
        double x = 0.1, y = 0.0, z = 0.0;
        const double a = 0.2;
        const double b = 0.2;
        const double c = 5.7;
        
        float iterate(double dt) {
            double dx = -y - z;
            double dy = x + a * y;
            double dz = b + z * (x - c);
            
            x += dx * dt;
            y += dy * dt;
            z += dz * dt;
            
            return static_cast<float>(std::tanh(x / 10.0));
        }
    };
    
    // Henon map (discrete)
    struct HenonMap {
        double x = 0.0, y = 0.0;
        double a = 1.4;
        double b = 0.3;
        
        float iterate() {
            double xNew = 1.0 - a * x * x + y;
            double yNew = b * x;
            
            x = xNew;
            y = yNew;
            
            return static_cast<float>(std::tanh(x));
        }
    };
    
    // Logistic map
    struct LogisticMap {
        double x = 0.5;
        double r = 3.9; // Chaos parameter (3.57 to 4.0)
        
        float iterate() {
            x = r * x * (1.0 - x);
            return static_cast<float>(x * 2.0 - 1.0);
        }
    };
    
    // Ikeda map
    struct IkedaMap {
        double x = 0.1, y = 0.1;
        double u = 0.9;
        
        float iterate() {
            double t = 0.4 - 6.0 / (1.0 + x * x + y * y);
            double xNew = 1.0 + u * (x * std::cos(t) - y * std::sin(t));
            double yNew = u * (x * std::sin(t) + y * std::cos(t));
            
            x = xNew;
            y = yNew;
            
            return static_cast<float>(std::tanh(x / 2.0));
        }
    };
    
    // Duffing oscillator
    struct DuffingOscillator {
        double x = 0.1, y = 0.0;
        double alpha = -1.0;
        double beta = 1.0;
        double gamma = 0.3;
        double delta = 0.2;
        double omega = 1.2;
        double phase = 0.0;
        
        float iterate(double dt) {
            double dx = y;
            double dy = -delta * y - alpha * x - beta * x * x * x + gamma * std::cos(omega * phase);
            
            x += dx * dt;
            y += dy * dt;
            phase += dt;
            
            return static_cast<float>(std::tanh(x));
        }
    };
    
    // Smooth interpolator for chaos values
    struct SmoothValue {
        float current = 0.0f;
        float target = 0.0f;
        float smoothing = 0.99f;
        
        float process() {
            current = current * smoothing + target * (1.0f - smoothing);
            return current;
        }
        
        void setTarget(float value) {
            target = value;
        }
        
        void setSmoothing(float smooth) {
            smoothing = std::max(0.0f, std::min(0.999f, smooth));
        }
    };
    
    // Pitch shifter for chaos-driven pitch modulation
    struct SimplePitchShift {
        std::vector<float> buffer;
        float writePos = 0.0f;
        int readPos = 0;
        static constexpr int BUFFER_SIZE = 4096;
        
        void prepare() {
            buffer.resize(BUFFER_SIZE);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0.0f;
            readPos = 0;
        }
        
        float process(float input, float pitchFactor) {
            // Write to buffer
            buffer[static_cast<int>(writePos)] = input;
            writePos = std::fmod(writePos + 1.0f, BUFFER_SIZE);
            
            // Read with pitch shift
            float output = 0.0f;
            float readPosFloat = static_cast<float>(readPos);
            
            // Linear interpolation
            int idx0 = static_cast<int>(readPosFloat) % BUFFER_SIZE;
            int idx1 = (idx0 + 1) % BUFFER_SIZE;
            float frac = readPosFloat - std::floor(readPosFloat);
            
            output = buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
            
            // Update read position based on pitch factor
            readPos = static_cast<int>(readPos + pitchFactor) % BUFFER_SIZE;
            
            return output;
        }
    };
    
    // State variable filter for chaos-driven filtering
    struct SVFilter {
        float freq = 1000.0f;
        float res = 0.5f;
        float state1 = 0.0f;
        float state2 = 0.0f;
        
        float processLowpass(float input, double sampleRate) {
            float f = 2.0f * std::sin(M_PI * freq / sampleRate);
            float q = 1.0f - res;
            
            state1 += f * (input - state1 + q * (state1 - state2));
            state2 += f * (state1 - state2);
            
            return state2;
        }
        
        void setFrequency(float f) {
            freq = std::max(20.0f, std::min(20000.0f, f));
        }
    };
    
    // DC Blocking filter
    struct DCBlocker {
        float x1 = 0.0f, y1 = 0.0f;
        static constexpr float R = 0.995f;
        
        float process(float input) {
            float output = input - x1 + R * y1;
            x1 = input;
            y1 = output;
            return output;
        }
        
        void reset() { x1 = y1 = 0.0f; }
    };
    
    // Thermal modeling for analog drift simulation
    struct ThermalModel {
        float temperature = 25.0f;  // Celsius
        float thermalNoise = 0.0f;
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist{-0.5f, 0.5f};
        
        ThermalModel() : rng(std::random_device{}()) {}
        
        void update(double sampleRate) {
            // Slow thermal drift affecting chaos parameters
            thermalNoise += (dist(rng) * 0.0005f) / sampleRate;
            thermalNoise = std::max(-0.01f, std::min(0.01f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f;
        float drift = 0.0f;
        float nonlinearity = 0.0f;
        
        void update(float aging) {
            age = aging;
            drift = aging * 0.01f;  // 1% max drift
            nonlinearity = aging * 0.008f;  // Increased nonlinearity with age
        }
        
        float applyChaosParameterDrift(float value) const {
            return value * (1.0f + drift);
        }
        
        float applyNonlinearAging(float input) const {
            if (nonlinearity > 0.001f) {
                return input + nonlinearity * input * input * input;
            }
            return input;
        }
    };
    
    // Enhanced chaos systems with thermal and aging effects
    struct EnhancedLorenzSystem : public LorenzSystem {
        float iterate(double dt, float thermalFactor, float aging) {
            // Apply thermal effects to system parameters
            double thermSigma = sigma * thermalFactor;
            double thermRho = rho * (1.0f + aging * 0.05f);  // Aging affects attractor shape
            double thermBeta = beta * thermalFactor;
            
            double dx = thermSigma * (y - x);
            double dy = x * (thermRho - z) - y;
            double dz = x * y - thermBeta * z;
            
            x += dx * dt;
            y += dy * dt;
            z += dz * dt;
            
            // Apply nonlinear aging effects
            float output = static_cast<float>(std::tanh(x / 30.0));
            if (aging > 0.01f) {
                output += aging * 0.02f * output * output * output;
            }
            
            return output;
        }
    };
    
    // Enhanced modulation processors with aging
    struct EnhancedPitchShift : public SimplePitchShift {
        mutable std::mt19937 rng{std::random_device{}()};
        mutable std::uniform_real_distribution<float> wobbleDist{-0.5f, 0.5f};
        
        float processWithAging(float input, float pitchFactor, float aging, float thermalFactor) {
            // Apply thermal drift to pitch factor
            float adjustedPitchFactor = pitchFactor * thermalFactor;
            
            // Basic pitch shift processing
            float output = process(input, adjustedPitchFactor);
            
            // Add aging artifacts
            if (aging > 0.05f) {
                // Slight pitch instability with aging (thread-safe)
                static float pitchWobble = 0.0f;
                pitchWobble += wobbleDist(rng) * aging * 0.001f;
                pitchWobble *= 0.999f;  // Slow decay
                
                float wobbleFactor = 1.0f + pitchWobble;
                output = process(output, wobbleFactor);
            }
            
            return output;
        }
    };
    
    // Enhanced filter with component modeling
    struct EnhancedSVFilter : public SVFilter {
        float processWithAging(float input, double sampleRate, float aging, float thermalFactor) {
            // Apply thermal drift to filter parameters
            float adjustedFreq = freq * thermalFactor;
            float adjustedRes = res * (1.0f + aging * 0.1f);  // Resonance changes with aging
            
            // Calculate filter coefficients with aging
            float f = 2.0f * std::sin(M_PI * adjustedFreq / sampleRate);
            float q = 1.0f - adjustedRes;
            
            // Add aging nonlinearity in feedback path
            if (aging > 0.01f) {
                state1 += aging * 0.02f * state1 * state1 * state1;
            }
            
            state1 += f * (input - state1 + q * (state1 - state2));
            state2 += f * (state1 - state2);
            
            return state2;
        }
    };
    
    // Channel state with enhanced processing
    struct ChannelState {
        // Enhanced chaos systems
        EnhancedLorenzSystem lorenz;
        RosslerSystem rossler;
        HenonMap henon;
        LogisticMap logistic;
        IkedaMap ikeda;
        DuffingOscillator duffing;
        
        // Current chaos value with enhanced smoothing
        SmoothValue chaosValue;
        
        // Enhanced modulation processors
        EnhancedPitchShift pitchShifter;
        EnhancedSVFilter filter;
        
        // DC blockers for input and output
        DCBlocker inputDCBlocker;
        DCBlocker outputDCBlocker;
        
        // Thermal and aging models
        ThermalModel thermalModel;
        ComponentAging componentAging;
        
        // Sample counter for update rate
        int sampleCounter = 0;
        int updateInterval = 441; // 100Hz at 44.1kHz
        
        // Enhanced random number generation
        std::mt19937 rng{42};
        std::uniform_real_distribution<float> dist{-1.0f, 1.0f};
        
        // Multiple chaos history for enhanced interpolation
        static constexpr int HISTORY_SIZE = 4;
        std::array<float, HISTORY_SIZE> chaosHistory;
        int historyIndex = 0;
        
        // Oversampling for cleaner modulation
        struct Oversampler {
            static constexpr int OVERSAMPLE_FACTOR = 2;
            std::vector<float> upsampleBuffer;
            std::vector<float> downsampleBuffer;
            
            void prepare(int blockSize) {
                upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
                downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            }
            
            // Simple 2x oversampling with basic anti-aliasing
            void processBlock(float* input, float* output, int numSamples, 
                            std::function<float(float)> processor) {
                // Upsample
                for (int i = 0; i < numSamples; ++i) {
                    upsampleBuffer[i * 2] = input[i];
                    upsampleBuffer[i * 2 + 1] = 0.0f;
                }
                
                // Process at higher sample rate
                for (int i = 0; i < numSamples * OVERSAMPLE_FACTOR; ++i) {
                    downsampleBuffer[i] = processor(upsampleBuffer[i]);
                }
                
                // Downsample with simple averaging
                for (int i = 0; i < numSamples; ++i) {
                    output[i] = (downsampleBuffer[i * 2] + downsampleBuffer[i * 2 + 1]) * 0.5f;
                }
            }
        };
        
        Oversampler oversampler;
        bool useOversampling = false;  // Enable for critical modulation targets
        
        // Noise floor simulation
        float noiseFloor = -84.0f; // dB
        
        void prepare(double sampleRate) {
            pitchShifter.prepare();
            filter.setFrequency(1000.0f);
            chaosValue.current = 0.0f;
            chaosValue.target = 0.0f;
            sampleCounter = 0;
            
            // Initialize DC blockers
            inputDCBlocker.reset();
            outputDCBlocker.reset();
            
            // Initialize thermal model
            thermalModel = ThermalModel();
            
            // Initialize component aging
            componentAging.update(0.0f);
            
            // Clear chaos history
            std::fill(chaosHistory.begin(), chaosHistory.end(), 0.0f);
            historyIndex = 0;
            
            // Prepare oversampler
            oversampler.prepare(512);
        }
        
        void reset(unsigned int seed) {
            // Reset chaos systems with slight randomization
            rng.seed(seed);
            
            lorenz.x = 0.1 + dist(rng) * 0.01;
            lorenz.y = dist(rng) * 0.01;
            lorenz.z = dist(rng) * 0.01;
            
            rossler.x = 0.1 + dist(rng) * 0.01;
            rossler.y = dist(rng) * 0.01;
            rossler.z = dist(rng) * 0.01;
            
            henon.x = dist(rng) * 0.1;
            henon.y = dist(rng) * 0.1;
            
            logistic.x = 0.5 + dist(rng) * 0.1;
            
            ikeda.x = 0.1 + dist(rng) * 0.01;
            ikeda.y = 0.1 + dist(rng) * 0.01;
            
            duffing.x = 0.1 + dist(rng) * 0.01;
            duffing.y = dist(rng) * 0.01;
            duffing.phase = dist(rng) * M_PI;
            
            // Clear history
            std::fill(chaosHistory.begin(), chaosHistory.end(), 0.0f);
        }
        
        // Enhanced chaos generation with interpolation
        float generateEnhancedChaos(ChaosType type, float thermalFactor, float aging) {
            float chaosOutput = 0.0f;
            
            switch (type) {
                case LORENZ:
                    chaosOutput = lorenz.iterate(0.01, thermalFactor, aging);
                    break;
                case ROSSLER:
                    chaosOutput = rossler.iterate(0.01);
                    break;
                case HENON:
                    chaosOutput = henon.iterate();
                    break;
                case LOGISTIC:
                    chaosOutput = logistic.iterate();
                    break;
                case IKEDA:
                    chaosOutput = ikeda.iterate();
                    break;
                case DUFFING:
                    chaosOutput = duffing.iterate(0.01);
                    break;
            }
            
            // Store in history for interpolation
            chaosHistory[historyIndex] = chaosOutput;
            historyIndex = (historyIndex + 1) % HISTORY_SIZE;
            
            // Apply component aging nonlinearity
            return componentAging.applyNonlinearAging(chaosOutput);
        }
    };
    
    std::array<ChannelState, 2> m_channelStates;
    double m_sampleRate = 44100.0;
    
    // Shared state
    float m_lastSeed = 0.5f;
    
    // Component aging tracking
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    // Enhanced processing flags
    bool m_enableThermalModeling = true;
    bool m_enableComponentAging = true;
    bool m_enableOversampling = false;
    
    // Helper functions
    ChaosType getChaosType() const;
    ModTarget getModTarget() const;
    float applyModulation(float input, float chaos, ModTarget target, ChannelState& state);
    
    // Enhanced helper methods
    void updateAllSmoothParams();
    void updateComponentAging();
    float applyEnhancedModulation(float input, float chaos, ModTarget target, ChannelState& state, float thermalFactor, float aging);
    float interpolateChaosHistory(const ChannelState& state, float smoothing);
    
    // Advanced chaos processing
    float processAdvancedChaos(float rawChaos, float thermalFactor, float aging);
    void applyChaosSmoothing(float& chaos, float smoothingAmount);
};