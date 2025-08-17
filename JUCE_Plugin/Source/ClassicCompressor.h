#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <cmath>
#include <array>
#include <atomic>
#include <algorithm>
#include <cstring>
// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SIMD 1
    #ifdef __SSE__
    #include <xmmintrin.h>
    #endif
#else
    #define HAS_SIMD 0
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


class ClassicCompressor : public EngineBase {
public:
    ClassicCompressor();
    ~ClassicCompressor() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Classic Compressor Pro"; }
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    
    // Professional metering
    float getGainReduction() const { return m_currentGainReduction.load(std::memory_order_relaxed); }
    float getPeakReduction() const { return m_peakGainReduction.load(std::memory_order_relaxed); }
    void resetMeters() { 
        m_currentGainReduction.store(0.0f, std::memory_order_relaxed);
        m_peakGainReduction.store(0.0f, std::memory_order_relaxed);
    }
    
#ifdef JUCE_DEBUG
    // Test method for verifying chunked processing fix
    bool testChunkedProcessing();
#endif
    
private:
    // Constants
    static constexpr int SUBBLOCK_SIZE = 32;
    static constexpr int MAX_BLOCK_SIZE = 2048;
    static constexpr int RMS_WINDOW_SIZE = 512;
    static constexpr int MAX_LOOKAHEAD_SAMPLES = 512;
    
    // Aligned allocation for SIMD
    template<typename T, size_t Alignment = 32>
    struct AlignedArray {
        alignas(Alignment) std::array<T, MAX_BLOCK_SIZE> data;
        T& operator[](size_t i) { return data[i]; }
        const T& operator[](size_t i) const { return data[i]; }
        T* ptr() { return data.data(); }
        const T* ptr() const { return data.data(); }
    };
    
    // ============== PARAMETER SMOOTHER (Thread-Safe) ==============
    class ParameterSmoother {
        std::atomic<float> m_target{0.0f};
        double m_current = 0.0;
        double m_smoothingCoeff = 0.99;
        
    public:
        void setSampleRate(double sr, float smoothingMs = 20.0f) {
            double tau = smoothingMs * 0.001;
            m_smoothingCoeff = std::exp(-1.0 / (tau * sr));
        }
        
        void setTarget(float value) { 
            m_target.store(value, std::memory_order_relaxed); 
        }
        
        double process() {
            double target = static_cast<double>(m_target.load(std::memory_order_relaxed));
            m_current = target + (m_current - target) * m_smoothingCoeff;
            return DSPUtils::flushDenorm(m_current);
        }
        
        double processSubBlock(int numSamples) {
            // Process only once per sub-block
            return process();
        }
        
        void reset(float value) { 
            m_target.store(value, std::memory_order_relaxed);
            m_current = value;
        }
        
        double getCurrentValue() const { return m_current; }
    };
    
    // ============== OPTIMIZED ENVELOPE FOLLOWER ==============
    class EnvelopeFollower {
        // All internal state in double precision
        double m_envelope = 0.0;
        
        // Optimized RMS detection with O(1) updates
        alignas(32) std::array<double, RMS_WINDOW_SIZE> m_rmsWindow;
        int m_rmsIndex = 0;
        double m_rmsSum = 0.0;
        
        // Pre-computed coefficients
        double m_attackCoeff = 0.0;
        double m_releaseCoeff = 0.0;
        
    public:
        enum class Mode { PEAK, RMS };
        
        void reset() {
            m_envelope = 0.0;
            std::fill(m_rmsWindow.begin(), m_rmsWindow.end(), 0.0);
            m_rmsIndex = 0;
            m_rmsSum = 0.0;
        }
        
        void updateCoefficients(double attackMs, double releaseMs, double sampleRate) {
            double attackTau = attackMs * 0.001;
            double releaseTau = releaseMs * 0.001;
            m_attackCoeff = 1.0 - std::exp(-1.0 / (attackTau * sampleRate));
            m_releaseCoeff = 1.0 - std::exp(-1.0 / (releaseTau * sampleRate));
        }
        
        double processPeak(float input) {
            double rectified = std::abs(static_cast<double>(input));
            
            if (rectified > m_envelope) {
                m_envelope += (rectified - m_envelope) * m_attackCoeff;
            } else {
                m_envelope += (rectified - m_envelope) * m_releaseCoeff;
            }
            
            return DSPUtils::flushDenorm(m_envelope);
        }
        
        double processRMS(float input) {
            // O(1) RMS update
            double squared = static_cast<double>(input) * static_cast<double>(input);
            
            // Remove oldest sample from sum
            m_rmsSum -= m_rmsWindow[m_rmsIndex];
            
            // Add new sample
            m_rmsWindow[m_rmsIndex] = squared;
            m_rmsSum += squared;
            
            // Advance circular buffer
            m_rmsIndex = (m_rmsIndex + 1) % RMS_WINDOW_SIZE;
            
            // Calculate RMS
            double rms = std::sqrt(m_rmsSum / RMS_WINDOW_SIZE);
            
            // Apply envelope
            if (rms > m_envelope) {
                m_envelope += (rms - m_envelope) * m_attackCoeff;
            } else {
                m_envelope += (rms - m_envelope) * m_releaseCoeff;
            }
            
            return DSPUtils::flushDenorm(m_envelope);
        }
        
        // SIMD-optimized batch processing
        void processBlockRMS(const float* input, double* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                output[i] = processRMS(input[i]);
            }
        }
    };
    
    // ============== TPT SVF SIDECHAIN FILTER ==============
    class SidechainProcessor {
        // Double precision state
        double m_s1 = 0.0, m_s2 = 0.0;
        double m_g = 0.0, m_k = 0.0, m_a0 = 0.0;
        
        // Pre-allocated lookahead buffer
        alignas(32) std::array<float, MAX_LOOKAHEAD_SAMPLES> m_lookaheadBuffer;
        int m_writeIndex = 0;
        int m_lookaheadSamples = 0;
        
        // Optimized peak detection with monotonic deque
        struct PeakDetector {
            struct Sample { float value; int index; };
            std::array<Sample, MAX_LOOKAHEAD_SAMPLES> m_deque;
            int m_front = 0, m_back = 0;
            
            void push(float value, int index) {
                // Remove samples that are smaller than current
                while (m_back > m_front && m_deque[m_back-1].value <= value) {
                    m_back--;
                }
                m_deque[m_back++] = {value, index};
            }
            
            void removeOld(int oldestValid) {
                while (m_front < m_back && m_deque[m_front].index < oldestValid) {
                    m_front++;
                }
            }
            
            float getPeak() const {
                return (m_front < m_back) ? m_deque[m_front].value : 0.0f;
            }
            
            void reset() { m_front = m_back = 0; }
        } m_peakDetector;
        
    public:
        void prepare(double sampleRate) {
            reset();
            setHighpass(80.0, sampleRate);
        }
        
        void setHighpass(double freq, double sampleRate) {
            // TPT highpass design
            m_g = std::tan(M_PI * freq / sampleRate);
            m_k = std::sqrt(2.0);
            m_a0 = 1.0 / (1.0 + m_g * (m_g + m_k));
        }
        
        void setLookahead(double ms, double sampleRate) {
            m_lookaheadSamples = std::clamp(
                static_cast<int>(ms * sampleRate * 0.001),
                0,
                MAX_LOOKAHEAD_SAMPLES - 1
            );
        }
        
        double processHighpass(double input) {
            // TPT SVF highpass (stable at all frequencies)
            double hp = (input - (2.0 * m_k + m_g) * m_s1 - m_s2) * m_a0;
            double bp = m_g * hp + m_s1;
            double lp = m_g * bp + m_s2;
            
            m_s1 = DSPUtils::flushDenorm(2.0 * bp - m_s1);
            m_s2 = DSPUtils::flushDenorm(2.0 * lp - m_s2);
            
            return hp;
        }
        
        float processLookahead(float input, float& delayedOutput) {
            // Write to circular buffer
            m_lookaheadBuffer[m_writeIndex] = input;
            
            // Get delayed sample
            int delayIndex = (m_writeIndex - m_lookaheadSamples + MAX_LOOKAHEAD_SAMPLES) 
                           % MAX_LOOKAHEAD_SAMPLES;
            delayedOutput = m_lookaheadBuffer[delayIndex];
            
            // Update peak detector (O(1) amortized)
            float absInput = std::abs(input);
            m_peakDetector.push(absInput, m_writeIndex);
            
            // Remove samples outside lookahead window
            int oldestValid = (m_writeIndex - m_lookaheadSamples + MAX_LOOKAHEAD_SAMPLES) 
                            % MAX_LOOKAHEAD_SAMPLES;
            m_peakDetector.removeOld(oldestValid);
            
            // Advance write position
            m_writeIndex = (m_writeIndex + 1) % MAX_LOOKAHEAD_SAMPLES;
            
            return m_peakDetector.getPeak();
        }
        
        void reset() {
            m_s1 = m_s2 = 0.0;
            std::fill(m_lookaheadBuffer.begin(), m_lookaheadBuffer.end(), 0.0f);
            m_writeIndex = 0;
            m_peakDetector.reset();
        }
    };
    
    // ============== GAIN COMPUTER ==============
    class GainComputer {
        double m_threshold = -12.0;
        double m_ratio = 4.0;
        double m_kneeWidth = 2.0;
        
        // Pre-computed knee coefficients
        double m_kneeStart = 0.0;
        double m_kneeEnd = 0.0;
        double m_kneeCoeff = 0.0;
        
    public:
        void updateParameters(double threshold, double ratio, double knee) {
            m_threshold = threshold;
            m_ratio = ratio;
            m_kneeWidth = knee;
            
            // Pre-compute knee boundaries
            m_kneeStart = threshold - knee * 0.5;
            m_kneeEnd = threshold + knee * 0.5;
            m_kneeCoeff = 1.0 / std::max(0.01, knee);
        }
        
        double computeGainReduction(double inputDb) const {
            if (m_kneeWidth < 0.1) {
                // Hard knee
                if (inputDb <= m_threshold) return 0.0;
                return (inputDb - m_threshold) * (1.0 - 1.0 / m_ratio);
            }
            
            // Soft knee with pre-computed coefficients
            if (inputDb <= m_kneeStart) {
                return 0.0;
            } else if (inputDb >= m_kneeEnd) {
                return (inputDb - m_threshold) * (1.0 - 1.0 / m_ratio);
            } else {
                // Optimized hermite interpolation
                double x = (inputDb - m_kneeStart) * m_kneeCoeff;
                double x2 = x * x;
                double h01 = x2 * (3.0 - 2.0 * x);
                double endGain = (m_kneeEnd - m_threshold) * (1.0 - 1.0 / m_ratio);
                return h01 * endGain;
            }
        }
    };
    
    // ============== GAIN SMOOTHER ==============
    class GainSmoother {
        double m_currentGain = 1.0;
        double m_attackCoeff = 0.0;
        double m_releaseCoeff = 0.0;
        double m_autoReleaseAmount = 0.0;
        double m_peakMemory = -60.0;
        double m_peakDecayCoeff = 0.0;
        
    public:
        void setTimes(double attackMs, double releaseMs, double autoRelease, double sampleRate) {
            double attackTau = attackMs * 0.001;
            double releaseTau = releaseMs * 0.001;
            
            m_attackCoeff = 1.0 - std::exp(-1.0 / (attackTau * sampleRate));
            m_releaseCoeff = 1.0 - std::exp(-1.0 / (releaseTau * sampleRate));
            m_autoReleaseAmount = autoRelease;
            
            // Peak memory decay (1 second time constant)
            m_peakDecayCoeff = std::exp(-1.0 / sampleRate);
        }
        
        double process(double targetGain, double inputLevel) {
            // Program-dependent release
            double releaseCoeff = m_releaseCoeff;
            
            if (m_autoReleaseAmount > 0.0) {
                double levelDb = 20.0 * std::log10(std::max(1e-6, inputLevel));
                
                // Update peak memory with proper time constant
                if (levelDb > m_peakMemory) {
                    m_peakMemory = levelDb;
                } else {
                    m_peakMemory = DSPUtils::flushDenorm(levelDb + (m_peakMemory - levelDb) * m_peakDecayCoeff);
                }
                
                // Adjust release based on peak memory
                if (levelDb > m_peakMemory - 3.0) {
                    releaseCoeff *= (1.0 + m_autoReleaseAmount * 2.0);
                }
            }
            
            // Apply smoothing
            if (targetGain < m_currentGain) {
                m_currentGain += (targetGain - m_currentGain) * m_attackCoeff;
            } else {
                m_currentGain += (targetGain - m_currentGain) * releaseCoeff;
            }
            
            return DSPUtils::flushDenorm(m_currentGain);
        }
        
        void reset() { 
            m_currentGain = 1.0; 
            m_peakMemory = -60.0;
        }
    };
    
    // Use DCBlocker from DspEngineUtilities
    
    // ============== MEMBER VARIABLES ==============
    double m_sampleRate = 44100.0;
    
    // Parameters (thread-safe)
    ParameterSmoother m_threshold;
    ParameterSmoother m_ratio;
    ParameterSmoother m_attack;
    ParameterSmoother m_release;
    ParameterSmoother m_knee;
    ParameterSmoother m_makeupGain;
    ParameterSmoother m_mix;
    ParameterSmoother m_lookahead;
    ParameterSmoother m_autoRelease;
    ParameterSmoother m_sidechain;
    
    // DSP components
    std::array<EnvelopeFollower, 2> m_envelopes;
    std::array<SidechainProcessor, 2> m_sidechains;
    std::array<GainComputer, 2> m_gainComputers;
    std::array<GainSmoother, 2> m_gainSmoothers;
    std::array<DCBlocker, 2> m_dcBlockers;
    
    // Pre-allocated work buffers
    AlignedArray<float> m_workBuffer1;
    AlignedArray<float> m_workBuffer2;
    AlignedArray<double> m_envelopeBuffer;
    
    // Metering (thread-safe)
    std::atomic<float> m_currentGainReduction{0.0f};
    std::atomic<float> m_peakGainReduction{0.0f};
    
    // Processing state
    enum class StereoMode { DUAL_MONO, STEREO_LINK };
    StereoMode m_stereoMode = StereoMode::STEREO_LINK;
    
    // Helper methods
    inline double dbToLinear(double db) const { 
        return std::pow(10.0, db * 0.05); 
    }
    
    inline double linearToDb(double linear) const { 
        return linear > 1e-6 ? 20.0 * std::log10(linear) : -120.0; 
    }
    
    void enableDenormalPrevention();
    void processSubBlock(float* left, float* right, int startSample, int numSamples);
};