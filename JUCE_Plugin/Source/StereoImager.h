#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"#include <vector>
#include <array>
#include <random>
#include <complex>

class StereoImager : public EngineBase {
public:
    StereoImager();
    ~StereoImager() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Stereo Imager"; }
    
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
    
    SmoothParam m_width;          // Stereo Width (-1 to +2)
    SmoothParam m_lowWidth;       // Low frequency width control
    SmoothParam m_midWidth;       // Mid frequency width control
    SmoothParam m_highWidth;      // High frequency width control
    SmoothParam m_crossover1;     // Low/Mid crossover frequency
    SmoothParam m_crossover2;     // Mid/High crossover frequency
    SmoothParam m_phase;          // Phase adjustment
    SmoothParam m_mix;            // Dry/wet mix
    
    // Advanced stereo processing techniques
    enum StereoMode {
        CLASSIC_MS,      // Classic Mid-Side
        BINAURAL,        // Binaural processing
        PSEUDO_STEREO,   // Pseudo stereo from mono
        X_Y_MATRIX,      // X-Y matrix processing
        BLUMLEIN,        // Blumlein pair simulation
        MULTIBAND        // Multi-band stereo control
    };
    
    // Crossover filters for multi-band processing
    struct CrossoverFilter {
        // 4th order Linkwitz-Riley crossover
        struct BiquadFilter {
            double x1 = 0.0, x2 = 0.0;
            double y1 = 0.0, y2 = 0.0;
            double a0 = 1.0, a1 = 0.0, a2 = 0.0;
            double b1 = 0.0, b2 = 0.0;
            
            void calculateLowpass(double freq, double sampleRate) {
                double omega = 2.0 * M_PI * freq / sampleRate;
                double sin_omega = std::sin(omega);
                double cos_omega = std::cos(omega);
                double alpha = sin_omega / (2.0 * 0.707); // Q = 0.707 for LR
                
                double norm = 1.0 / (1.0 + alpha);
                a0 = (1.0 - cos_omega) * 0.5 * norm;
                a1 = (1.0 - cos_omega) * norm;
                a2 = a0;
                b1 = -2.0 * cos_omega * norm;
                b2 = (1.0 - alpha) * norm;
            }
            
            void calculateHighpass(double freq, double sampleRate) {
                double omega = 2.0 * M_PI * freq / sampleRate;
                double sin_omega = std::sin(omega);
                double cos_omega = std::cos(omega);
                double alpha = sin_omega / (2.0 * 0.707);
                
                double norm = 1.0 / (1.0 + alpha);
                a0 = (1.0 + cos_omega) * 0.5 * norm;
                a1 = -(1.0 + cos_omega) * norm;
                a2 = a0;
                b1 = -2.0 * cos_omega * norm;
                b2 = (1.0 - alpha) * norm;
            }
            
            double process(double input) {
                double output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
                
                x2 = x1; x1 = input;
                y2 = y1; y1 = output;
                
                return output;
            }
            
            void reset() {
                x1 = x2 = y1 = y2 = 0.0;
            }
        };
        
        BiquadFilter lowpass1, lowpass2;   // 4th order lowpass
        BiquadFilter highpass1, highpass2; // 4th order highpass
        
        void prepare(double lowFreq, double highFreq, double sampleRate) {
            lowpass1.calculateLowpass(lowFreq, sampleRate);
            lowpass2.calculateLowpass(lowFreq, sampleRate);
            highpass1.calculateHighpass(highFreq, sampleRate);
            highpass2.calculateHighpass(highFreq, sampleRate);
        }
        
        void process(double input, double& low, double& mid, double& high) {
            // Low band
            low = lowpass2.process(lowpass1.process(input));
            
            // High band
            high = highpass2.process(highpass1.process(input));
            
            // Mid band (difference)
            mid = input - low - high;
        }
        
        void reset() {
            lowpass1.reset();
            lowpass2.reset();
            highpass1.reset();
            highpass2.reset();
        }
    };
    
    // Binaural processor with HRTF simulation
    struct BinauralProcessor {
        static constexpr int HRTF_SIZE = 128;
        std::array<float, HRTF_SIZE> leftImpulse;
        std::array<float, HRTF_SIZE> rightImpulse;
        std::vector<float> convolutionBuffer;
        int bufferPos = 0;
        
        void prepare() {
            convolutionBuffer.resize(HRTF_SIZE);
            std::fill(convolutionBuffer.begin(), convolutionBuffer.end(), 0.0f);
            
            // Generate simplified HRTF impulses
            for (int i = 0; i < HRTF_SIZE; ++i) {
                float t = static_cast<float>(i) / HRTF_SIZE;
                
                // Left ear (slightly delayed and filtered)
                leftImpulse[i] = std::exp(-t * 8.0f) * std::sin(t * 15.0f + 0.2f) * 0.8f;
                
                // Right ear (earlier arrival, different coloration)
                rightImpulse[i] = std::exp(-t * 6.0f) * std::sin(t * 12.0f) * 0.7f;
            }
        }
        
        void process(float input, float& leftOut, float& rightOut) {
            // Store input in circular buffer
            convolutionBuffer[bufferPos] = input;
            
            // Convolve with HRTF impulses
            leftOut = rightOut = 0.0f;
            
            for (int i = 0; i < HRTF_SIZE; ++i) {
                int readPos = (bufferPos - i + HRTF_SIZE) % HRTF_SIZE;
                float sample = convolutionBuffer[readPos];
                
                leftOut += sample * leftImpulse[i];
                rightOut += sample * rightImpulse[i];
            }
            
            bufferPos = (bufferPos + 1) % HRTF_SIZE;
        }
    };
    
    // Pseudo stereo processor for mono sources
    struct PseudoStereoProcessor {
        // All-pass filters for decorrelation
        struct AllpassFilter {
            float delay1 = 0.0f, delay2 = 0.0f, delay3 = 0.0f;
            
            float process(float input, float coefficient) {
                float y = input + coefficient * delay3;
                delay3 = delay2;
                delay2 = delay1;
                delay1 = y - coefficient * input;
                return delay1;
            }
        };
        
        std::array<AllpassFilter, 4> leftFilters;
        std::array<AllpassFilter, 4> rightFilters;
        
        // Different coefficients for left and right to create decorrelation
        static constexpr std::array<float, 4> leftCoeffs = {0.7f, -0.4f, 0.6f, -0.3f};
        static constexpr std::array<float, 4> rightCoeffs = {-0.6f, 0.5f, -0.7f, 0.4f};
        
        void process(float input, float& leftOut, float& rightOut) {
            leftOut = input;
            rightOut = input;
            
            // Chain of all-pass filters with different coefficients
            for (int i = 0; i < 4; ++i) {
                leftOut = leftFilters[i].process(leftOut, leftCoeffs[i]);
                rightOut = rightFilters[i].process(rightOut, rightCoeffs[i]);
            }
            
            // Additional phase shift for more separation
            static float leftPhase = 0.0f, rightPhase = 0.0f;
            leftPhase += 0.001f;
            rightPhase += 0.0007f;
            
            leftOut *= (1.0f + 0.1f * std::sin(leftPhase));
            rightOut *= (1.0f + 0.1f * std::sin(rightPhase));
        }
    };
    
    // Phase adjustment with all-pass networks
    struct PhaseAdjuster {
        static constexpr int NUM_STAGES = 6;
        
        struct AllpassStage {
            float x1 = 0.0f, y1 = 0.0f;
            float coefficient = 0.0f;
            
            float process(float input) {
                float output = -coefficient * input + x1;
                x1 = input;
                y1 = output;
                return output + coefficient * y1;
            }
            
            void setCoefficient(float coeff) {
                coefficient = std::max(-0.9f, std::min(0.9f, coeff));
            }
        };
        
        std::array<AllpassStage, NUM_STAGES> stages;
        
        void setPhase(float phase) {
            // Distribute phase shift across stages
            float phasePerStage = phase / NUM_STAGES;
            for (auto& stage : stages) {
                stage.setCoefficient(std::tan(phasePerStage * M_PI * 0.25f));
            }
        }
        
        float process(float input) {
            float output = input;
            for (auto& stage : stages) {
                output = stage.process(output);
            }
            return output;
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
            // Slow thermal drift affecting stereo imaging
            thermalNoise += (dist(rng) * 0.0003f) / sampleRate;
            thermalNoise = std::max(-0.008f, std::min(0.008f, thermalNoise));
        }
        
        float getThermalFactor() const {
            return 1.0f + thermalNoise;
        }
    };
    
    // Component aging simulation
    struct ComponentAging {
        float age = 0.0f;
        float channelImbalance = 0.0f;
        float phaseShift = 0.0f;
        
        void update(float aging) {
            age = aging;
            channelImbalance = aging * 0.008f;  // Up to 0.8% channel imbalance
            phaseShift = aging * 0.005f;        // Slight phase shift with age
        }
        
        void applyImbalance(float& left, float& right) const {
            left *= (1.0f - channelImbalance);
            right *= (1.0f + channelImbalance);
        }
        
        float getPhaseShift() const {
            return phaseShift;
        }
    };
    
    // Channel state with enhanced processing
    struct ChannelState {
        CrossoverFilter crossover;
        BinauralProcessor binaural;
        PseudoStereoProcessor pseudoStereo;
        PhaseAdjuster phaseAdjuster;
        
        // DC blockers for input and output
        DCBlocker inputDCBlocker;
        DCBlocker outputDCBlocker;
        
        // Thermal and aging models
        ThermalModel thermalModel;
        ComponentAging componentAging;
        
        // Oversampling for high-quality processing
        struct Oversampler {
            static constexpr int OVERSAMPLE_FACTOR = 2;
            std::vector<float> upsampleBuffer;
            std::vector<float> downsampleBuffer;
            
            // Anti-aliasing filters
            struct AAFilter {
                float x1 = 0.0f, x2 = 0.0f;
                float y1 = 0.0f, y2 = 0.0f;
                
                float process(float input) {
                    // 2nd order Butterworth at Fs/4
                    const float a0 = 0.2929f, a1 = 0.5858f, a2 = 0.2929f;
                    const float b1 = 0.0000f, b2 = 0.1716f;
                    
                    float output = a0 * input + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
                    
                    x2 = x1; x1 = input;
                    y2 = y1; y1 = output;
                    
                    return output;
                }
            };
            
            AAFilter upsampleFilter;
            AAFilter downsampleFilter;
            
            void prepare(int blockSize) {
                upsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
                downsampleBuffer.resize(blockSize * OVERSAMPLE_FACTOR);
            }
            
            void upsample(const float* input, float* output, int numSamples) {
                for (int i = 0; i < numSamples; ++i) {
                    output[i * 2] = upsampleFilter.process(input[i] * 2.0f);
                    output[i * 2 + 1] = upsampleFilter.process(0.0f);
                }
            }
            
            void downsample(const float* input, float* output, int numSamples) {
                for (int i = 0; i < numSamples; ++i) {
                    downsampleFilter.process(input[i * 2]);
                    output[i] = downsampleFilter.process(input[i * 2 + 1]) * 0.5f;
                }
            }
        };
        
        Oversampler oversampler;
        bool useOversampling = true;
        
        // Delay compensation for phase-coherent processing
        static constexpr int MAX_DELAY_SAMPLES = 64;
        std::array<float, MAX_DELAY_SAMPLES> delayBuffer;
        int delayPos = 0;
        
        // Noise floor simulation
        float noiseFloor = -102.0f; // dB
        
        void prepare(double sampleRate) {
            crossover.prepare(250.0, 2500.0, sampleRate);
            binaural.prepare();
            
            inputDCBlocker.reset();
            outputDCBlocker.reset();
            
            // Initialize thermal model
            thermalModel = ThermalModel();
            
            // Initialize component aging
            componentAging.update(0.0f);
            
            // Prepare oversampler
            oversampler.prepare(512);
            
            // Clear delay buffer
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
            delayPos = 0;
        }
        
        float processDelay(float input, int delaySamples) {
            delayBuffer[delayPos] = input;
            int readPos = (delayPos - delaySamples + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
            float output = delayBuffer[readPos];
            delayPos = (delayPos + 1) % MAX_DELAY_SAMPLES;
            return output;
        }
    };
    
    ChannelState m_channelState;
    double m_sampleRate = 44100.0;
    
    // Processing mode
    StereoMode m_currentMode = MULTIBAND;
    
    // Component aging tracking
    float m_componentAge = 0.0f;
    int m_sampleCount = 0;
    
    // Enhanced processing flags
    bool m_enableThermalModeling = true;
    bool m_enableComponentAging = true;
    bool m_enableOversampling = true;
    
    // Correlation analyzer for intelligent processing
    struct CorrelationAnalyzer {
        static constexpr int ANALYSIS_SIZE = 1024;
        std::array<float, ANALYSIS_SIZE> leftHistory;
        std::array<float, ANALYSIS_SIZE> rightHistory;
        int historyPos = 0;
        float correlation = 0.0f;
        
        void update(float left, float right) {
            leftHistory[historyPos] = left;
            rightHistory[historyPos] = right;
            historyPos = (historyPos + 1) % ANALYSIS_SIZE;
            
            // Calculate correlation every 64 samples
            if (historyPos % 64 == 0) {
                float sumXY = 0.0f, sumX2 = 0.0f, sumY2 = 0.0f;
                
                for (int i = 0; i < ANALYSIS_SIZE; ++i) {
                    float x = leftHistory[i];
                    float y = rightHistory[i];
                    sumXY += x * y;
                    sumX2 += x * x;
                    sumY2 += y * y;
                }
                
                float denom = std::sqrt(sumX2 * sumY2);
                correlation = (denom > 1e-10f) ? (sumXY / denom) : 0.0f;
            }
        }
        
        float getCorrelation() const { return correlation; }
        bool isMono() const { return correlation > 0.95f; }
        bool isUncorrelated() const { return std::abs(correlation) < 0.1f; }
    };
    
    CorrelationAnalyzer m_correlationAnalyzer;
    
    // Helper methods
    void updateAllSmoothParams();
    void updateComponentAging();
    void processClassicMS(float& left, float& right, float width);
    void processMultiband(float& left, float& right);
    void processBinaural(float input, float& left, float& right);
    void processPseudoStereo(float input, float& left, float& right);
    float applyAnalogCharacter(float input, float thermalFactor, float aging);
    
    // Advanced stereo enhancement techniques
    void enhanceStereoField(float& left, float& right, float width, float correlation);
    void applyPhaseCoherence(float& left, float& right, float phase);
    void compensateChannelImbalance(float& left, float& right, float aging);
};