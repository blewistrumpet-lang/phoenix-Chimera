// MasteringLimiter_Ultimate.h - Absolute Highest Quality Studio Implementation
#pragma once

#include "EngineBase.h"
#include <array>
#include <memory>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <vector>
#include <deque>
#include <complex>

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

class MasteringLimiter : public EngineBase {
public:
    MasteringLimiter();
    ~MasteringLimiter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Mastering Limiter Ultimate"; }
    int getNumParameters() const override { return 10; }
    juce::String getParameterName(int index) const override;
    
private:
    // Professional constants
    static constexpr int OVERSAMPLE_FACTOR = 16;  // 16x for true peak accuracy
    static constexpr int MAX_LOOKAHEAD_MS = 20;
    static constexpr int MAX_BLOCK_SIZE = 2048;
    static constexpr double TRUE_PEAK_THRESHOLD = 0.9999;  // -0.01 dBFS
    
    // Thread-safe parameter smoothing
    class SmoothedParameter {
        std::atomic<double> m_targetValue{0.0};
        double m_currentValue = 0.0;
        double m_smoothingCoeff = 0.995;
        
    public:
        void setSampleRate(double sr, double smoothingMs = 20.0) {
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
    
    // ITU-R BS.1770-4 True Peak Detection
    class TruePeakDetector {
        static constexpr int SINC_SAMPLES = 48;
        static constexpr int SINC_PHASES = 8192;  // Sub-sample precision
        
        // Pre-calculated sinc table for efficiency
        alignas(64) std::array<std::array<double, SINC_SAMPLES>, SINC_PHASES> m_sincTable;
        std::deque<double> m_sampleHistory;
        
    public:
        TruePeakDetector() {
            // Generate windowed sinc interpolation table
            for (int phase = 0; phase < SINC_PHASES; ++phase) {
                double fractionalDelay = static_cast<double>(phase) / SINC_PHASES;
                
                for (int i = 0; i < SINC_SAMPLES; ++i) {
                    double x = i - SINC_SAMPLES/2 + fractionalDelay;
                    
                    // Sinc function
                    double sinc = (x == 0) ? 1.0 : std::sin(M_PI * x) / (M_PI * x);
                    
                    // Blackman-Harris window for minimal sidelobes
                    double n = static_cast<double>(i) / (SINC_SAMPLES - 1);
                    double window = 0.35875 - 0.48829 * std::cos(2*M_PI*n) + 
                                   0.14128 * std::cos(4*M_PI*n) - 0.01168 * std::cos(6*M_PI*n);
                    
                    m_sincTable[phase][i] = sinc * window;
                }
            }
        }
        
        void prepare(double sampleRate) {
            m_sampleHistory.clear();
            for (int i = 0; i < SINC_SAMPLES; ++i) {
                m_sampleHistory.push_back(0.0);
            }
        }
        
        double detectTruePeak(double input) {
            // Update history
            m_sampleHistory.push_back(input);
            if (m_sampleHistory.size() > SINC_SAMPLES) {
                m_sampleHistory.pop_front();
            }
            
            double truePeak = std::abs(input);
            
            // Check interpolated values between samples
            for (int phase = 1; phase < SINC_PHASES; ++phase) {
                double interpolated = 0.0;
                
                // Convolve with sinc function
                for (int i = 0; i < SINC_SAMPLES; ++i) {
                    interpolated += m_sampleHistory[i] * m_sincTable[phase][i];
                }
                
                truePeak = std::max(truePeak, std::abs(interpolated));
            }
            
            return truePeak;
        }
        
        void reset() {
            for (auto& sample : m_sampleHistory) {
                sample = 0.0;
            }
        }
    };
    
    // Psychoacoustic loudness detection (K-weighting)
    class LoudnessDetector {
        // K-weighting filter stages (ITU-R BS.1770-4)
        struct KWeightingFilter {
            // High shelf: +4dB at 1.5kHz
            double hsB0 = 1.53512, hsB1 = -2.69169, hsB2 = 1.19839;
            double hsA1 = -1.69065, hsA2 = 0.73248;
            double hsX1 = 0.0, hsX2 = 0.0, hsY1 = 0.0, hsY2 = 0.0;
            
            // High pass: 60Hz
            double hpB0 = 0.98621, hpB1 = -1.97242, hpB2 = 0.98621;
            double hpA1 = -1.97223, hpA2 = 0.97261;
            double hpX1 = 0.0, hpX2 = 0.0, hpY1 = 0.0, hpY2 = 0.0;
            
            double process(double input) {
                // High shelf stage
                double hs = hsB0 * input + hsB1 * hsX1 + hsB2 * hsX2 - hsA1 * hsY1 - hsA2 * hsY2;
                hsX2 = flushDenormalDouble(hsX1); hsX1 = flushDenormalDouble(input);
                hsY2 = flushDenormalDouble(hsY1); hsY1 = flushDenormalDouble(hs);
                
                // High pass stage
                double hp = hpB0 * hs + hpB1 * hpX1 + hpB2 * hpX2 - hpA1 * hpY1 - hpA2 * hpY2;
                hpX2 = flushDenormalDouble(hpX1); hpX1 = flushDenormalDouble(hs);
                hpY2 = flushDenormalDouble(hpY1); hpY1 = flushDenormalDouble(hp);
                
                return hp;
            }
            
            void reset() {
                hsX1 = hsX2 = hsY1 = hsY2 = 0.0;
                hpX1 = hpX2 = hpY1 = hpY2 = 0.0;
            }
        };
        
        std::array<KWeightingFilter, 2> m_kFilters;  // Stereo
        double m_momentaryWindow[4800];  // 400ms at 12kHz
        double m_shortTermWindow[36000]; // 3s at 12kHz
        int m_momentaryIndex = 0;
        int m_shortTermIndex = 0;
        
    public:
        void prepare(double sampleRate) {
            for (auto& filter : m_kFilters) {
                filter.reset();
            }
            std::fill(std::begin(m_momentaryWindow), std::end(m_momentaryWindow), 0.0);
            std::fill(std::begin(m_shortTermWindow), std::end(m_shortTermWindow), 0.0);
        }
        
        double processLoudness(double left, double right) {
            // K-weight the signals
            double kLeft = m_kFilters[0].process(left);
            double kRight = m_kFilters[1].process(right);
            
            // Mean square
            double power = (kLeft * kLeft + kRight * kRight) / 2.0;
            
            // Update windows
            m_momentaryWindow[m_momentaryIndex] = power;
            m_momentaryIndex = (m_momentaryIndex + 1) % 4800;
            
            m_shortTermWindow[m_shortTermIndex] = power;
            m_shortTermIndex = (m_shortTermIndex + 1) % 36000;
            
            // Calculate momentary loudness (400ms)
            double momentarySum = 0.0;
            for (int i = 0; i < 4800; ++i) {
                momentarySum += m_momentaryWindow[i];
            }
            double momentaryLoudness = -0.691 + 10.0 * std::log10(momentarySum / 4800.0 + 1e-15);
            
            return momentaryLoudness;
        }
        
        void reset() {
            for (auto& filter : m_kFilters) {
                filter.reset();
            }
            std::fill(std::begin(m_momentaryWindow), std::end(m_momentaryWindow), 0.0);
            std::fill(std::begin(m_shortTermWindow), std::end(m_shortTermWindow), 0.0);
            m_momentaryIndex = 0;
            m_shortTermIndex = 0;
        }
    };
    
    // Advanced lookahead with predictive analysis
    class PredictiveLookahead {
        std::deque<double> m_delayLine;
        std::deque<double> m_envelopeLine;
        int m_lookaheadSamples = 0;
        double m_attackTime = 0.0;
        double m_releaseTime = 0.0;
        
        // Predictive analysis
        double m_slope = 0.0;
        double m_acceleration = 0.0;
        double m_jerk = 0.0;
        
    public:
        void prepare(double lookaheadMs, double sampleRate) {
            m_lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * sampleRate);
            m_delayLine.clear();
            m_envelopeLine.clear();
            
            for (int i = 0; i < m_lookaheadSamples; ++i) {
                m_delayLine.push_back(0.0);
                m_envelopeLine.push_back(0.0);
            }
        }
        
        double process(double input, double& delayedOutput) {
            // Store input
            m_delayLine.push_back(input);
            delayedOutput = m_delayLine.front();
            m_delayLine.pop_front();
            
            // Analyze future samples for prediction
            double maxPeak = 0.0;
            double prevSample = 0.0;
            double prevSlope = 0.0;
            
            for (size_t i = 0; i < m_delayLine.size(); ++i) {
                double sample = std::abs(m_delayLine[i]);
                maxPeak = std::max(maxPeak, sample);
                
                // Calculate derivatives for prediction
                if (i > 0) {
                    double currentSlope = sample - prevSample;
                    if (i > 1) {
                        double currentAccel = currentSlope - prevSlope;
                        m_acceleration = m_acceleration * 0.9 + currentAccel * 0.1;
                        
                        if (i > 2) {
                            m_jerk = currentAccel - m_acceleration;
                        }
                    }
                    m_slope = m_slope * 0.9 + currentSlope * 0.1;
                    prevSlope = currentSlope;
                }
                prevSample = sample;
            }
            
            // Predictive gain reduction based on derivatives
            double prediction = maxPeak + m_slope * m_lookaheadSamples * 0.5 + 
                               m_acceleration * m_lookaheadSamples * 0.25 +
                               m_jerk * m_lookaheadSamples * 0.125;
            
            return std::max(maxPeak, prediction);
        }
        
        void reset() {
            for (auto& sample : m_delayLine) sample = 0.0;
            for (auto& sample : m_envelopeLine) sample = 0.0;
            m_slope = m_acceleration = m_jerk = 0.0;
        }
    };
    
    // Multi-band dynamics for frequency-dependent limiting
    class MultibandProcessor {
        static constexpr int NUM_BANDS = 5;
        
        struct Band {
            double freq;
            double gain = 1.0;
            double threshold = 0.95;
            double ratio = 10.0;
            double attack = 0.001;
            double release = 0.01;
            double envelope = 0.0;
            
            // Linkwitz-Riley crossover filters
            struct LR4Filter {
                double b0, b1, b2, b3, b4;
                double a1, a2, a3, a4;
                double x1 = 0, x2 = 0, x3 = 0, x4 = 0;
                double y1 = 0, y2 = 0, y3 = 0, y4 = 0;
                
                void setLowpass(double freq, double sampleRate) {
                    double omega = 2.0 * M_PI * freq / sampleRate;
                    double sinw = std::sin(omega);
                    double cosw = std::cos(omega);
                    double alpha = sinw / std::sqrt(2.0);
                    
                    // Cascade two 2nd-order Butterworth for LR4
                    double norm = 1.0 / (1.0 + alpha);
                    b0 = (1.0 - cosw) * 0.5 * norm;
                    b1 = (1.0 - cosw) * norm;
                    b2 = b0;
                    a1 = -2.0 * cosw * norm;
                    a2 = (1.0 - alpha) * norm;
                    
                    // Square for 4th order
                    b0 *= b0; b1 *= 2 * b0; b2 = b0 * b0 + b1 * b1;
                    b3 = b1; b4 = b0;
                }
                
                double process(double input) {
                    double out = b0 * input + b1 * x1 + b2 * x2 + b3 * x3 + b4 * x4
                                - a1 * y1 - a2 * y2 - a3 * y3 - a4 * y4;
                    
                    x4 = flushDenormalDouble(x3); x3 = flushDenormalDouble(x2);
                    x2 = flushDenormalDouble(x1); x1 = flushDenormalDouble(input);
                    y4 = flushDenormalDouble(y3); y3 = flushDenormalDouble(y2);
                    y2 = flushDenormalDouble(y1); y1 = flushDenormalDouble(out);
                    
                    return out;
                }
                
                void reset() {
                    x1 = x2 = x3 = x4 = y1 = y2 = y3 = y4 = 0.0;
                }
            };
            
            LR4Filter lowpass, highpass;
            
            double processEnvelope(double input, double sampleRate) {
                double rect = std::abs(input);
                
                if (rect > envelope) {
                    envelope = flushDenormalDouble(envelope + (rect - envelope) * attack);
                } else {
                    envelope = flushDenormalDouble(envelope + (rect - envelope) * release);
                }
                
                // Calculate gain reduction
                if (envelope > threshold) {
                    double excess = envelope - threshold;
                    double compressedExcess = excess / ratio;
                    gain = (threshold + compressedExcess) / envelope;
                } else {
                    gain = 1.0;
                }
                
                return gain;
            }
        };
        
        std::array<Band, NUM_BANDS> m_bands;
        
    public:
        void prepare(double sampleRate) {
            // Set crossover frequencies
            double freqs[NUM_BANDS-1] = {100.0, 500.0, 2000.0, 8000.0};
            
            for (int i = 0; i < NUM_BANDS; ++i) {
                m_bands[i].attack = 1.0 - std::exp(-1.0 / (0.1 * 0.001 * sampleRate));
                m_bands[i].release = 1.0 - std::exp(-1.0 / (50.0 * 0.001 * sampleRate));
                
                if (i < NUM_BANDS - 1) {
                    m_bands[i].lowpass.setLowpass(freqs[i], sampleRate);
                }
                if (i > 0) {
                    m_bands[i].highpass.setLowpass(freqs[i-1], sampleRate);
                }
            }
        }
        
        double process(double input, double sampleRate) {
            std::array<double, NUM_BANDS> bandSignals;
            
            // Split into bands
            for (int i = 0; i < NUM_BANDS; ++i) {
                double band = input;
                
                if (i < NUM_BANDS - 1) {
                    band = m_bands[i].lowpass.process(band);
                }
                if (i > 0) {
                    band = m_bands[i].highpass.process(band);
                }
                
                // Process dynamics
                double gain = m_bands[i].processEnvelope(band, sampleRate);
                bandSignals[i] = band * gain;
            }
            
            // Sum bands
            double output = 0.0;
            for (const auto& band : bandSignals) {
                output += band;
            }
            
            return output;
        }
        
        void reset() {
            for (auto& band : m_bands) {
                band.lowpass.reset();
                band.highpass.reset();
                band.envelope = 0.0;
                band.gain = 1.0;
            }
        }
    };
    
    // Adaptive release curve generator
    class AdaptiveRelease {
        double m_program = 0.0;
        double m_peak = 0.0;
        double m_rms = 0.0;
        double m_crest = 0.0;
        std::array<double, 1024> m_history;
        int m_historyIndex = 0;
        
    public:
        double calculateRelease(double input, double baseRelease) {
            // Update history
            m_history[m_historyIndex] = input * input;
            m_historyIndex = (m_historyIndex + 1) % 1024;
            
            // Calculate RMS
            double sum = 0.0;
            for (const auto& sample : m_history) {
                sum += sample;
            }
            m_rms = std::sqrt(sum / 1024.0);
            
            // Peak detection
            m_peak = m_peak * 0.9999 + std::abs(input) * 0.0001;
            
            // Crest factor
            m_crest = (m_rms > 0) ? m_peak / m_rms : 1.0;
            
            // Program-dependent release
            // Faster for transients, slower for sustained
            double adaptiveRelease = baseRelease;
            
            if (m_crest > 10.0) {
                // Transient material - fast release
                adaptiveRelease *= 0.1;
            } else if (m_crest > 5.0) {
                // Mixed material
                adaptiveRelease *= 0.5;
            } else {
                // Dense material - slow release
                adaptiveRelease *= 2.0;
            }
            
            return adaptiveRelease;
        }
        
        void reset() {
            m_program = m_peak = m_rms = m_crest = 0.0;
            m_history.fill(0.0);
            m_historyIndex = 0;
        }
    };
    
    // Professional 16x oversampling
    class Oversampler16x {
        static constexpr int FIR_LENGTH = 512;
        
        struct LinearPhaseFIR {
            alignas(64) std::array<double, FIR_LENGTH> coeffs;
            alignas(64) std::array<double, FIR_LENGTH> buffer{0};
            int bufferIndex = 0;
            
            void designKaiser(double cutoff, double sampleRate, double stopbandAttenuation = 120.0) {
                double beta = 0.1102 * (stopbandAttenuation - 8.7);
                double sum = 0.0;
                
                for (int i = 0; i < FIR_LENGTH; ++i) {
                    double n = i - (FIR_LENGTH - 1) / 2.0;
                    double sinc = (n == 0) ? 1.0 : std::sin(M_PI * cutoff * n / sampleRate) / (M_PI * n);
                    
                    double x = 2.0 * i / (FIR_LENGTH - 1) - 1.0;
                    double kaiser = besselI0(beta * std::sqrt(1.0 - x * x)) / besselI0(beta);
                    
                    coeffs[i] = sinc * kaiser;
                    sum += coeffs[i];
                }
                
                for (double& c : coeffs) c /= sum;
            }
            
            double process(double input) {
                buffer[bufferIndex] = input;
                
                double output = 0.0;
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
                double sum = 1.0, term = 1.0, x2 = x * x / 4.0;
                for (int k = 1; k < 100; ++k) {
                    term *= x2 / (k * k);
                    sum += term;
                    if (term < 1e-15) break;
                }
                return sum;
            }
        };
        
        std::array<LinearPhaseFIR, 8> m_upsampleStages;
        std::array<LinearPhaseFIR, 8> m_downsampleStages;
        
    public:
        void prepare(double sampleRate) {
            // Cascaded stages for extreme attenuation
            for (int i = 0; i < 8; ++i) {
                double cutoff = sampleRate * 0.45 * std::pow(2.0, i);
                m_upsampleStages[i].designKaiser(cutoff, sampleRate * 16, 150.0);
                m_downsampleStages[i].designKaiser(cutoff, sampleRate * 16, 150.0);
            }
        }
        
        void processUpsample(const double* input, double* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                for (int j = 0; j < 16; ++j) {
                    double sample = (j == 0) ? input[i] * 16.0 : 0.0;
                    
                    for (int stage = 0; stage < 8; ++stage) {
                        sample = m_upsampleStages[stage].process(sample);
                    }
                    
                    output[i * 16 + j] = sample;
                }
            }
        }
        
        void processDownsample(const double* input, double* output, int numSamples) {
            for (int i = 0; i < numSamples; ++i) {
                double accumulator = 0.0;
                
                for (int j = 0; j < 16; ++j) {
                    double sample = input[i * 16 + j];
                    
                    for (int stage = 0; stage < 8; ++stage) {
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
    
    // Soft clipping with multiple algorithms
    class SoftClipper {
    public:
        enum class Algorithm {
            TANH,
            ALGEBRAIC,
            EXPONENTIAL,
            SINE,
            CUBIC,
            ARCTANGENT,
            ERF,
            VARIABLE_MU
        };
        
    private:
        Algorithm m_algorithm = Algorithm::TANH;
        double m_knee = 0.1;
        
    public:
        void setAlgorithm(Algorithm algo) { m_algorithm = algo; }
        void setKnee(double knee) { m_knee = std::clamp(knee, 0.0, 1.0); }
        
        double process(double input, double threshold) {
            double absInput = std::abs(input);
            double sign = (input < 0) ? -1.0 : 1.0;
            
            if (absInput < threshold - m_knee) {
                return input;
            }
            
            double x = (absInput - threshold + m_knee) / m_knee;
            x = std::clamp(x, 0.0, 1.0);
            
            double softClipped = absInput;
            
            switch (m_algorithm) {
                case Algorithm::TANH:
                    softClipped = threshold * std::tanh(absInput / threshold);
                    break;
                    
                case Algorithm::ALGEBRAIC:
                    softClipped = absInput / std::sqrt(1.0 + (absInput * absInput) / (threshold * threshold));
                    break;
                    
                case Algorithm::EXPONENTIAL:
                    softClipped = threshold * (1.0 - std::exp(-absInput / threshold));
                    break;
                    
                case Algorithm::SINE:
                    if (absInput > threshold) {
                        softClipped = threshold * std::sin(M_PI * 0.5 * absInput / threshold);
                    }
                    break;
                    
                case Algorithm::CUBIC:
                    if (absInput > threshold) {
                        double over = (absInput - threshold) / (1.0 - threshold);
                        softClipped = threshold + (1.0 - threshold) * (over - over * over * over / 3.0);
                    }
                    break;
                    
                case Algorithm::ARCTANGENT:
                    softClipped = threshold * (2.0 / M_PI) * std::atan(absInput * M_PI / (2.0 * threshold));
                    break;
                    
                case Algorithm::ERF:
                    softClipped = threshold * std::erf(absInput / threshold);
                    break;
                    
                case Algorithm::VARIABLE_MU: {
                    double mu = 1.0 + 10.0 * (absInput / threshold);
                    softClipped = threshold * std::log(1.0 + mu * absInput / threshold) / std::log(1.0 + mu);
                    break;
                }
            }
            
            // Blend between hard and soft clipping based on knee
            double hardClipped = std::min(absInput, threshold);
            double output = hardClipped * (1.0 - x) + softClipped * x;
            
            return sign * output;
        }
    };
    
    // Member variables
    double m_sampleRate = 44100.0;
    
    // Parameters
    std::unique_ptr<SmoothedParameter> m_threshold;
    std::unique_ptr<SmoothedParameter> m_ceiling;
    std::unique_ptr<SmoothedParameter> m_release;
    std::unique_ptr<SmoothedParameter> m_lookahead;
    std::unique_ptr<SmoothedParameter> m_knee;
    std::unique_ptr<SmoothedParameter> m_makeup;
    std::unique_ptr<SmoothedParameter> m_saturation;
    std::unique_ptr<SmoothedParameter> m_stereoLink;
    std::unique_ptr<SmoothedParameter> m_truePeak;
    std::unique_ptr<SmoothedParameter> m_mix;
    
    // DSP Components
    std::array<TruePeakDetector, 2> m_truePeakDetectors;
    std::array<PredictiveLookahead, 2> m_lookaheads;
    std::array<MultibandProcessor, 2> m_multibandProcessors;
    std::array<AdaptiveRelease, 2> m_adaptiveReleases;
    std::array<SoftClipper, 2> m_softClippers;
    std::array<Oversampler16x, 2> m_oversamplers;
    LoudnessDetector m_loudnessDetector;
    
    // Work buffers
    alignas(64) std::array<double, MAX_BLOCK_SIZE * OVERSAMPLE_FACTOR> m_oversampledBuffers[2];
    
    // Metering
    std::atomic<double> m_inputPeak{0.0};
    std::atomic<double> m_outputPeak{0.0};
    std::atomic<double> m_gainReduction{0.0};
    std::atomic<double> m_momentaryLoudness{0.0};
    
    // Processing methods
    void processStereo(float* left, float* right, int numSamples);
    SoftClipper::Algorithm getAlgorithmFromParam(float param) const;
};