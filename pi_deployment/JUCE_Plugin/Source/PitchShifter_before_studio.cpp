#include "PitchShifter.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <array>
#include <algorithm>

// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

// Define ALWAYS_INLINE
#ifndef ALWAYS_INLINE
  #if defined(__GNUC__) || defined(__clang__)
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
  #elif defined(_MSC_VER)
    #define ALWAYS_INLINE __forceinline
  #else
    #define ALWAYS_INLINE inline
  #endif
#endif

// Use DspEngineUtilities for denormal protection

// Lock-free parameter with smoothing
class AtomicSmoothParam {
public:
    void setTarget(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
    }
    
    void setImmediate(float value) noexcept {
        target.store(value, std::memory_order_relaxed);
        current = value;
    }
    
    void setSmoothingCoeff(float coeff) noexcept {
        smoothing = coeff;
    }
    
    ALWAYS_INLINE float tick() noexcept {
        const float t = target.load(std::memory_order_relaxed);
        current += (t - current) * (1.0f - smoothing);
        return DSPUtils::flushDenorm(current);
    }
    
    float getValue() const noexcept {
        return current;
    }
    
private:
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float smoothing{0.995f};
};

// Use DCBlocker from DspEngineUtilities

// Main implementation
struct PitchShifter::Impl {
    static constexpr int FFT_ORDER = 12;  // 2^12 = 4096
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int OVERLAP_FACTOR = 4;
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;
    static constexpr int MAX_CHANNELS = 8;
    
    // Parameters (lock-free)
    AtomicSmoothParam pitchRatio;
    AtomicSmoothParam formantShift;
    AtomicSmoothParam mixAmount;
    AtomicSmoothParam windowWidth;
    AtomicSmoothParam spectralGate;
    AtomicSmoothParam grainSize;
    AtomicSmoothParam feedback;
    AtomicSmoothParam stereoWidth;
    
    // Store actual snapped pitch value for display
    std::atomic<float> snappedPitchValue{0.5f};
    
    // Previous pitch for artifact reduction
    float previousPitch{1.0f};
    int pitchChangeCounter{0};
    
    // Per-channel state
    struct alignas(64) ChannelState {  // Cache-line aligned
        // Ring buffers for zero-copy overlap-add
        alignas(32) std::array<float, FFT_SIZE * 2> inputRing{};
        alignas(32) std::array<float, FFT_SIZE * 2> outputRing{};
        alignas(32) std::array<std::complex<float>, FFT_SIZE> spectrum{};
        alignas(32) std::array<float, FFT_SIZE> frameBuffer{};  // Temp for FFT input
        
        // Double precision for phase coherence
        alignas(32) std::array<double, FFT_SIZE/2 + 1> phaseLast{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> phaseSum{};
        
        // Structure-of-Arrays for SIMD efficiency
        alignas(32) std::array<float, FFT_SIZE/2 + 1> magnitude{};
        alignas(32) std::array<float, FFT_SIZE/2 + 1> frequency{};
        
        std::array<float, 8192> feedbackBuffer{};  // Fixed size
        
        // Window functions (computed once, aligned)
        alignas(32) std::array<float, FFT_SIZE> analysisWindow{};
        alignas(32) std::array<float, FFT_SIZE> synthesisWindow{};
        
        // Ring buffer indices
        int inputWriteIdx{0};
        int inputReadIdx{0};
        int outputWriteIdx{0};
        int outputReadIdx{0};
        int feedbackWritePos{0};  // Separate write position for feedback
        int feedbackReadPos{4000};   // Separate read position with delay offset
        int hopCounter{0};
        
        // FFT object
        std::unique_ptr<juce::dsp::FFT> fft;
        
        // DC blockers
        DCBlocker inputDC;
        DCBlocker outputDC;
        
        void reset() noexcept {
            inputRing.fill(0.0f);
            outputRing.fill(0.0f);
            phaseLast.fill(0.0);
            phaseSum.fill(0.0);
            feedbackBuffer.fill(0.0f);
            inputWriteIdx = 0;
            inputReadIdx = 0;
            outputWriteIdx = 0;
            outputReadIdx = 0;
            feedbackWritePos = 0;
            feedbackReadPos = 0;
            hopCounter = 0;
            inputDC.reset();
            outputDC.reset();
        }
        
        // Ring buffer operations (zero-copy)
        ALWAYS_INLINE void writeSample(float sample) noexcept {
            inputRing[inputWriteIdx] = sample;
            inputWriteIdx = (inputWriteIdx + 1) & (FFT_SIZE * 2 - 1);
        }
        
        ALWAYS_INLINE float readOutput() noexcept {
            float out = outputRing[outputReadIdx];
            outputRing[outputReadIdx] = 0.0f;  // Clear after reading
            outputReadIdx = (outputReadIdx + 1) & (FFT_SIZE * 2 - 1);
            return out;
        }
        
        ALWAYS_INLINE void gatherFrame() noexcept {
            // Gather FFT_SIZE samples from ring buffer without copying
            int idx = inputReadIdx;
            for (int i = 0; i < FFT_SIZE; ++i) {
                frameBuffer[i] = inputRing[idx];
                idx = (idx + 1) & (FFT_SIZE * 2 - 1);
            }
            // Update read position by hop size
            inputReadIdx = (inputReadIdx + HOP_SIZE) & (FFT_SIZE * 2 - 1);
        }
        
        ALWAYS_INLINE void scatterFrame(const std::complex<float>* fftOut, float scale) noexcept {
            // Overlap-add into output ring buffer
            int idx = outputWriteIdx;
            for (int i = 0; i < FFT_SIZE; ++i) {
                outputRing[idx] += fftOut[i].real() * synthesisWindow[i] * scale;
                idx = (idx + 1) & (FFT_SIZE * 2 - 1);
            }
            // Update write position by hop size
            outputWriteIdx = (outputWriteIdx + HOP_SIZE) & (FFT_SIZE * 2 - 1);
        }
    };
    
    std::array<ChannelState, MAX_CHANNELS> channels;
    int activeChannels{0};
    double sampleRate{44100.0};
    
    // Pre-computed constants
    float binFrequency{0.0f};
    float expectedPhaseInc{0.0f};
    float outputScale{0.0f};
    
    // Denormal flush counter
    int denormalFlushCounter{0};
    
    Impl() {
        // Initialize parameters with default values
        pitchRatio.setImmediate(1.0f);
        formantShift.setImmediate(1.0f);
        mixAmount.setImmediate(1.0f);
        windowWidth.setImmediate(0.5f);
        spectralGate.setImmediate(0.0f);
        grainSize.setImmediate(0.5f);
        feedback.setImmediate(0.0f);
        stereoWidth.setImmediate(0.5f);
        
        // Set smoothing coefficients
        pitchRatio.setSmoothingCoeff(0.990f);
        formantShift.setSmoothingCoeff(0.992f);
        mixAmount.setSmoothingCoeff(0.995f);
        windowWidth.setSmoothingCoeff(0.998f);
        spectralGate.setSmoothingCoeff(0.995f);
        grainSize.setSmoothingCoeff(0.998f);
        feedback.setSmoothingCoeff(0.995f);
        stereoWidth.setSmoothingCoeff(0.995f);
    }
    
    void prepareToPlay(double sr, int /*samplesPerBlock*/) {
        sampleRate = sr;
        
        // Pre-compute constants
        binFrequency = static_cast<float>(sampleRate / FFT_SIZE);
        expectedPhaseInc = 2.0f * static_cast<float>(M_PI) * HOP_SIZE / FFT_SIZE;
        // Adjusted scaling - slightly higher to compensate for window overlap
        outputScale = 1.15f / OVERLAP_FACTOR;  // 1.15/4 = 0.2875 (was too low before)
        
        // Initialize FFT objects and windows
        for (auto& ch : channels) {
            ch.fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
            createWindows(ch.analysisWindow, ch.synthesisWindow);
            ch.reset();
        }
    }
    
    void createWindows(std::array<float, FFT_SIZE>& analysis,
                      std::array<float, FFT_SIZE>& synthesis) {
        // Hann window
        for (int i = 0; i < FFT_SIZE; ++i) {
            const float t = static_cast<float>(i) / (FFT_SIZE - 1);
            analysis[i] = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * t);
        }
        
        // COLA-optimized synthesis window
        synthesis = analysis;
        
        // Normalize for perfect reconstruction
        std::array<float, FFT_SIZE> sum{};
        for (int i = 0; i < OVERLAP_FACTOR; ++i) {
            const int offset = i * HOP_SIZE;
            for (int j = 0; j < FFT_SIZE; ++j) {
                const int idx = (j + offset) % FFT_SIZE;
                if (idx < FFT_SIZE) {
                    sum[idx] += synthesis[j] * synthesis[j];
                }
            }
        }
        
        for (int i = 0; i < FFT_SIZE; ++i) {
            if (sum[i] > 1e-6f) {
                synthesis[i] /= std::sqrt(sum[i]);
            }
        }
    }
    
    ALWAYS_INLINE void processChannel(ChannelState& ch, float* data, int numSamples) {
        // Process samples with per-sample parameter smoothing
        for (int i = 0; i < numSamples; ++i) {
            // Update ALL parameters per-sample for click-free automation
            const float pitch = pitchRatio.tick();
            const float formant = formantShift.tick();
            const float mix = mixAmount.tick();
            const float gate = spectralGate.tick();
            const float fbAmount = feedback.tick() * 0.7f;
            const float window = windowWidth.tick();  // Now using window parameter
            // const float grain = grainSize.tick();  // Temporarily disabled - was breaking overlap-add
            
            // DC block input
            float input = ch.inputDC.process(data[i]);
            
            // Add feedback with denormal prevention (with delay)
            if (fbAmount > 1e-6f) {
                // Read from delayed position for feedback
                input += DSPUtils::flushDenorm(ch.feedbackBuffer[ch.feedbackReadPos] * fbAmount);
                ch.feedbackReadPos = (ch.feedbackReadPos + 1) % ch.feedbackBuffer.size();
            }
            
            // Write to ring buffer
            ch.writeSample(input);
            ch.hopCounter++;
            
            // Process frame when ready (fixed hop size for correct overlap-add)
            if (ch.hopCounter >= HOP_SIZE) {
                ch.hopCounter = 0;
                // Note: grain parameter temporarily disabled - was breaking overlap-add
                processSpectralFrame(ch, pitch, formant, gate, window);
            }
            
            // Read from output ring buffer
            float output = ch.readOutput();
            
            // Store feedback (write to different position for delay)
            if (fbAmount > 1e-6f) {
                ch.feedbackBuffer[ch.feedbackWritePos] = output;
                ch.feedbackWritePos = (ch.feedbackWritePos + 1) % ch.feedbackBuffer.size();
            }
            
            // DC block output with denormal flush
            output = DSPUtils::flushDenorm(ch.outputDC.process(output));
            
            // Hard limiter to prevent extreme loudness from gate
            if (std::abs(output) > 2.0f) {
                output = output > 0 ? 2.0f : -2.0f;
            }
            
            // Soft saturation for overloads
            if (std::abs(output) > 0.95f) {
                output = std::tanh(output * 0.7f) * 1.43f;  // Softer saturation
            }
            
            // Apply crossfade on pitch changes to reduce artifacts
            if (pitchChangeCounter < 2048) {  // ~46ms crossfade at 44.1kHz for smoother transition
                float crossfade = static_cast<float>(pitchChangeCounter) / 2048.0f;
                // S-curve for even smoother transition
                crossfade = crossfade * crossfade * (3.0f - 2.0f * crossfade);
                output *= crossfade;
                pitchChangeCounter++;
            }
            
            // Mix with dry (per-sample for smooth automation)
            data[i] = DSPUtils::flushDenorm(input * (1.0f - mix) + output * mix);
        }
    }
    
    void processSpectralFrame(ChannelState& ch, float pitch, float formant, float gate, float window) {
        // Gather frame from ring buffer (zero-copy)
        ch.gatherFrame();
        
        // Window input with denormal prevention
        alignas(32) std::array<float, FFT_SIZE> windowed;
        
        // Always use Hann window for best phase vocoder performance
        for (int i = 0; i < FFT_SIZE; ++i) {
            windowed[i] = ch.frameBuffer[i] * ch.analysisWindow[i];
        }
        
        // Window parameter now controls phase coherence smoothing
        // Lower values = more phase smoothing = less artifacts but softer transients
        // Higher values = less phase smoothing = sharper transients but more artifacts
        const float phaseCoherence = 0.7f + window * 0.3f;  // 0.7 to 1.0
        
        // Copy to complex array
        for (int i = 0; i < FFT_SIZE; ++i) {
            ch.spectrum[i] = std::complex<float>(windowed[i], 0.0f);
        }
        
        // Forward FFT
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), false);
        
        // BYPASS: If no processing needed, skip phase vocoder
        if (std::abs(pitch - 1.0f) < 0.001f && std::abs(formant - 1.0f) < 0.001f) {
            // Direct passthrough when no pitch/formant shift
            // Apply gate if needed (soft gate to avoid glitches)
            if (gate > 1e-6f) {
                // Much lower threshold and soft gating
                const float threshold = gate * 0.001f;  // Much smaller threshold
                for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
                    float mag = std::abs(ch.spectrum[bin]);
                    if (mag < threshold) {
                        // Soft gate - reduce instead of zeroing
                        float reduction = mag / (threshold + 1e-10f);
                        ch.spectrum[bin] *= reduction * reduction;
                        if (bin > 0 && bin < FFT_SIZE/2) {
                            ch.spectrum[FFT_SIZE - bin] = std::conj(ch.spectrum[bin]);
                        }
                    }
                }
            }
        } else {
            // Phase vocoder processing for pitch shifting
            analyzeSpectrum(ch);
            
            // Apply spectral gate (soft gating to avoid artifacts)
            if (gate > 1e-6f) {
                // Calculate average magnitude for adaptive threshold
                float avgMag = 0;
                for (int bin = 1; bin <= FFT_SIZE/2; ++bin) {
                    avgMag += ch.magnitude[bin];
                }
                avgMag /= (FFT_SIZE/2);
                
                // Adaptive threshold based on average magnitude
                const float threshold = avgMag * gate * 0.01f;  // 1% of average at max
                
                for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
                    if (ch.magnitude[bin] < threshold) {
                        // Soft reduction instead of hard gating
                        float ratio = ch.magnitude[bin] / (threshold + 1e-10f);
                        ch.magnitude[bin] *= ratio * ratio;
                    }
                }
            }
            
            // Shift spectrum with phase vocoder
            shiftSpectrum(ch, pitch, formant);
        }
        
        // Inverse FFT
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), true);
        
        // Scatter to output ring buffer with overlap-add
        ch.scatterFrame(ch.spectrum.data(), outputScale);
        
        // Periodic denormal flush and phase coherence maintenance
        if (++denormalFlushCounter >= 256) {
            denormalFlushCounter = 0;
            
            // Apply window-based phase coherence
            const float phaseCoherence = 0.7f + window * 0.3f;
            
            for (int i = 0; i <= FFT_SIZE/2; ++i) {
                // Flush denormals
                ch.phaseSum[i] = DSPUtils::flushDenorm(ch.phaseSum[i]);
                ch.phaseLast[i] = DSPUtils::flushDenorm(ch.phaseLast[i]);
                
                // Gently reduce phase accumulation to prevent artifacts
                ch.phaseSum[i] *= phaseCoherence;
            }
            
            // Also flush output ring buffer to prevent accumulation
            for (auto& sample : ch.outputRing) {
                sample = DSPUtils::flushDenorm(sample);
            }
        }
    }
    
    void analyzeSpectrum(ChannelState& ch) {
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            const auto& c = ch.spectrum[bin];
            const float real = c.real();
            const float imag = c.imag();
            
            // Magnitude with denormal prevention
            ch.magnitude[bin] = DSPUtils::flushDenorm(std::sqrt(real * real + imag * imag + 1e-20f));
            
            // Phase in double precision
            const double phase = std::atan2(static_cast<double>(imag), 
                                          static_cast<double>(real));
            
            // Phase difference with proper wrapping
            double phaseDiff = phase - ch.phaseLast[bin];
            ch.phaseLast[bin] = phase;
            
            // Princarg wrapping function for better phase coherence
            const double twoPi = 2.0 * M_PI;
            phaseDiff = phaseDiff - twoPi * std::round(phaseDiff / twoPi);
            
            // True frequency calculation with improved precision
            const double expectedPhase = expectedPhaseInc * bin;
            const double deviation = phaseDiff - expectedPhase;
            
            // Wrap deviation as well for better accuracy
            const double wrappedDeviation = deviation - twoPi * std::round(deviation / twoPi);
            
            // Improved frequency estimation using phase vocoder formula
            const double trueFreq = (bin + wrappedDeviation / twoPi * FFT_SIZE / HOP_SIZE) * binFrequency;
            
            ch.frequency[bin] = DSPUtils::flushDenorm(static_cast<float>(trueFreq));
        }
    }
    
    void shiftSpectrum(ChannelState& ch, float pitch, float formant) {
        // Temporary buffer for shifted spectrum
        alignas(16) std::array<std::complex<float>, FFT_SIZE> shifted{};
        
        // Store original magnitudes for formant shifting
        alignas(16) std::array<float, FFT_SIZE/2 + 1> originalMags;
        for (int i = 0; i <= FFT_SIZE/2; ++i) {
            originalMags[i] = ch.magnitude[i];
        }
        
        // Improved phase vocoder with phase locking to reduce artifacts
        // 1. Update phase accumulators with phase locking
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            // True frequency was calculated in analyzeSpectrum
            double trueFreq = ch.frequency[bin];
            
            // Shift the frequency for pitch change
            double shiftedFreq = trueFreq * pitch;
            
            // Calculate expected phase advance
            double phaseAdvance = 2.0 * M_PI * shiftedFreq * HOP_SIZE / sampleRate;
            
            // Update phase accumulator with smoothing for less artifacts
            ch.phaseSum[bin] += phaseAdvance;
            
            // Princarg wrapping for better phase coherence
            const double twoPi = 2.0 * M_PI;
            ch.phaseSum[bin] = ch.phaseSum[bin] - twoPi * std::round(ch.phaseSum[bin] / twoPi);
        }
        
        // Apply phase locking to maintain relationships between harmonics
        // Peak detection with improved thresholds
        if (std::abs(pitch - 1.0f) > 0.001f) {
            // First pass: identify spectral peaks with better detection
            std::array<bool, FFT_SIZE/2 + 1> isPeak{};
            std::array<float, FFT_SIZE/2 + 1> peakMagnitude{};
            
            // Smooth magnitude spectrum for better peak detection
            for (int bin = 1; bin < FFT_SIZE/2; ++bin) {
                peakMagnitude[bin] = 0.25f * ch.magnitude[bin-1] + 0.5f * ch.magnitude[bin] + 0.25f * ch.magnitude[bin+1];
            }
            peakMagnitude[0] = ch.magnitude[0];
            peakMagnitude[FFT_SIZE/2] = ch.magnitude[FFT_SIZE/2];
            
            // Find peaks using local maxima
            for (int bin = 2; bin <= FFT_SIZE/2 - 2; ++bin) {
                // Check if this is a local maximum
                if (peakMagnitude[bin] > peakMagnitude[bin-1] * 1.1f && 
                    peakMagnitude[bin] > peakMagnitude[bin+1] * 1.1f &&
                    peakMagnitude[bin] > peakMagnitude[bin-2] * 1.05f &&
                    peakMagnitude[bin] > peakMagnitude[bin+2] * 1.05f &&
                    peakMagnitude[bin] > 0.001f) {
                    isPeak[bin] = true;
                }
            }
            
            // Second pass: phase locking for harmonic coherence
            // Only lock phases when we have clear harmonic structure
            for (int fundamentalBin = 1; fundamentalBin <= FFT_SIZE/8; ++fundamentalBin) {
                if (!isPeak[fundamentalBin]) continue;
                
                // Check for harmonic series
                int harmonicsFound = 0;
                for (int h = 2; h <= 4; ++h) {
                    int hBin = fundamentalBin * h;
                    if (hBin <= FFT_SIZE/2 && isPeak[hBin]) {
                        harmonicsFound++;
                    }
                }
                
                // If we found harmonics, lock their phases
                if (harmonicsFound >= 2) {
                    for (int harmonic = 2; harmonic <= 6; ++harmonic) {
                        int harmonicBin = std::round(fundamentalBin * harmonic * pitch);
                        if (harmonicBin > 0 && harmonicBin <= FFT_SIZE/2) {
                            // Phase locking with soft blending
                            double targetPhase = ch.phaseSum[fundamentalBin] * harmonic;
                            const double twoPi = 2.0 * M_PI;
                            targetPhase = targetPhase - twoPi * std::round(targetPhase / twoPi);
                            
                            // Blend 60% locked phase with 40% original for smoother sound
                            ch.phaseSum[harmonicBin] = 0.6 * targetPhase + 0.4 * ch.phaseSum[harmonicBin];
                            ch.phaseSum[harmonicBin] = ch.phaseSum[harmonicBin] - twoPi * std::round(ch.phaseSum[harmonicBin] / twoPi);
                        }
                    }
                }
            }
        }
        
        // 2. Reconstruct spectrum with pitch shifting
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            float mag = 0;
            float phase = 0;
            
            // PITCH SHIFTING: Resample spectrum with improved interpolation
            if (std::abs(pitch - 1.0f) > 0.001f) {
                // Find source bin for this target bin
                float sourceBinFloat = bin / pitch;
                int sourceBin = static_cast<int>(sourceBinFloat);
                float fraction = sourceBinFloat - sourceBin;
                
                if (sourceBin >= 0 && sourceBin < FFT_SIZE/2 - 1) {
                    // Linear interpolation with magnitude preservation
                    // Simpler but cleaner for real-time pitch shifting
                    int bin1 = sourceBin;
                    int bin2 = std::min(FFT_SIZE/2, sourceBin + 1);
                    
                    float mag1 = originalMags[bin1];
                    float mag2 = originalMags[bin2];
                    
                    // Linear interpolation
                    mag = mag1 + fraction * (mag2 - mag1);
                    
                    // Apply spectral smoothing to reduce artifacts
                    if (bin > 0 && bin < FFT_SIZE/2) {
                        float prevMag = (bin > 0) ? originalMags[std::max(0, sourceBin - 1)] : mag1;
                        float nextMag = (bin < FFT_SIZE/2 - 1) ? originalMags[std::min(FFT_SIZE/2, sourceBin + 2)] : mag2;
                        
                        // Smooth with neighbors
                        mag = 0.1f * prevMag + 0.8f * mag + 0.1f * nextMag;
                    }
                    
                    // Ensure non-negative magnitude
                    mag = std::max(0.0f, mag);
                    phase = static_cast<float>(ch.phaseSum[bin]);
                } else if (sourceBin == FFT_SIZE/2) {
                    // Edge case - use linear interpolation
                    mag = originalMags[sourceBin] * (1.0f - fraction);
                    phase = static_cast<float>(ch.phaseSum[bin]);
                } else {
                    continue;  // Skip bins outside range
                }
            } else {
                // No pitch shift - use original magnitude
                mag = originalMags[bin];
                phase = static_cast<float>(ch.phaseSum[bin]);
            }
            
            // FORMANT/BRIGHTNESS: VERY pronounced spectral shaping
            // formant = 0.5 is neutral, < 0.5 darker, > 0.5 brighter
            if (mag > 0) {
                float freqHz = bin * binFrequency;
                
                // Multi-band EQ approach for dramatic effect
                float gain = 1.0f;
                
                // Low frequencies (< 500 Hz) - slightly affected
                if (freqHz < 500.0f) {
                    if (formant < 0.5f) {
                        // Boost lows when dark
                        gain = 1.0f + (0.5f - formant) * 0.5f;  // Up to 1.25x
                    } else {
                        // Cut lows when bright
                        gain = 1.0f - (formant - 0.5f) * 0.3f;  // Down to 0.85x
                    }
                }
                // Mid frequencies (500-2000 Hz) - moderately affected  
                else if (freqHz < 2000.0f) {
                    if (formant < 0.5f) {
                        // Cut mids when dark
                        gain = std::pow(10.0f, (formant - 0.5f) * 2.0f);  // Up to -20dB
                    } else {
                        // Boost mids when bright
                        gain = 1.0f + (formant - 0.5f) * 4.0f;  // Up to +12dB
                    }
                }
                // High frequencies (> 2000 Hz) - heavily affected
                else {
                    if (formant < 0.5f) {
                        // Heavily cut highs when dark
                        gain = std::pow(10.0f, (formant - 0.5f) * 6.0f);  // Up to -60dB!
                    } else {
                        // Heavily boost highs when bright
                        gain = 1.0f + (formant - 0.5f) * 20.0f;  // Up to +26dB!
                    }
                }
                
                mag *= gain;
            }
            
            // Create the shifted bin
            shifted[bin] = std::polar(mag, phase);
            
            // Maintain Hermitian symmetry for real output
            if (bin > 0 && bin < FFT_SIZE/2) {
                shifted[FFT_SIZE - bin] = std::conj(shifted[bin]);
            }
        }
        
        ch.spectrum = shifted;
    }
    
    void processStereoWidth(float* left, float* right, int numSamples) {
        // Per-sample width processing for smooth automation
        for (int i = 0; i < numSamples; ++i) {
            const float width = stereoWidth.tick() * 2.0f;
            const float mid = (left[i] + right[i]) * 0.5f;
            const float side = (left[i] - right[i]) * 0.5f * width;
            left[i] = DSPUtils::flushDenorm(mid + side);
            right[i] = DSPUtils::flushDenorm(mid - side);
        }
    }
};

// Public interface implementation
PitchShifter::PitchShifter() : pimpl(std::make_unique<Impl>()) {}
PitchShifter::~PitchShifter() = default;

void PitchShifter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->prepareToPlay(sampleRate, samplesPerBlock);
}

void PitchShifter::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = std::min(buffer.getNumChannels(), Impl::MAX_CHANNELS);
    const int numSamples = buffer.getNumSamples();
    
    pimpl->activeChannels = numChannels;
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        pimpl->processChannel(pimpl->channels[ch], buffer.getWritePointer(ch), numSamples);
    }
    
    // Apply stereo width if stereo (2 channels)
    // For surround sound, only apply width to first stereo pair
    if (numChannels >= 2) {
        pimpl->processStereoWidth(buffer.getWritePointer(0), 
                                 buffer.getWritePointer(1), 
                                 numSamples);
    }
    
    // Scrub buffer for NaN/Inf protection
    scrubBuffer(buffer);
}

void PitchShifter::updateParameters(const std::map<int, float>& params) {
    // Thread-safe parameter updates
    for (const auto& [index, value] : params) {
        switch (index) {
            case kPitch: {
                // Snap to musical intervals if close
                float snappedValue = value;
                
                // Define snap points (musical intervals)
                const float snapPoints[] = {
                    0.250f,  // Octave down
                    0.354f,  // Perfect 5th down
                    0.396f,  // Perfect 4th down  
                    0.417f,  // Major 3rd down
                    0.438f,  // Minor 3rd down
                    0.479f,  // Minor 2nd down
                    0.500f,  // Unison
                    0.521f,  // Minor 2nd up
                    0.563f,  // Minor 3rd up
                    0.583f,  // Major 3rd up
                    0.604f,  // Perfect 4th up
                    0.646f,  // Perfect 5th up
                    0.750f   // Octave up
                };
                
                // Find closest snap point
                float minDistance = 1.0f;
                for (float snapPoint : snapPoints) {
                    float distance = std::abs(value - snapPoint);
                    if (distance < minDistance) {
                        minDistance = distance;
                        snappedValue = snapPoint;
                    }
                }
                
                // Store the snapped value for display
                pimpl->snappedPitchValue.store(snappedValue);
                
                // Convert to semitones and ratio
                float semitones = (snappedValue - 0.5f) * 48.0f;
                float ratio = std::pow(2.0f, semitones / 12.0f);
                
                // Check if pitch actually changed
                if (std::abs(ratio - pimpl->previousPitch) > 0.001f) {
                    pimpl->previousPitch = ratio;
                    pimpl->pitchChangeCounter = 0;  // Reset counter for crossfade
                }
                
                // Set immediately without smoothing for pitch
                pimpl->pitchRatio.setImmediate(ratio);
                break;
            }
            case kFormant:  
                // Formant/Brightness control
                // 0.0 -> Very dark
                // 0.5 -> Neutral  
                // 1.0 -> Very bright
                pimpl->formantShift.setTarget(value); 
                break;
            case kMix:      pimpl->mixAmount.setTarget(value); break;
            case kWindow:   pimpl->windowWidth.setTarget(value); break;
            case kGate:     pimpl->spectralGate.setTarget(value); break;
            case kGrain:    pimpl->grainSize.setTarget(value); break;
            case kFeedback: pimpl->feedback.setTarget(value * 0.9f); break;
            case kWidth:    pimpl->stereoWidth.setTarget(value); break;
        }
    }
}

juce::String PitchShifter::getParameterName(int index) const {
    switch (index) {
        case kPitch:    return "Pitch";
        case kFormant:  return "Formant";
        case kMix:      return "Mix";
        case kWindow:   return "Window";
        case kGate:     return "Gate";
        case kGrain:    return "Grain";
        case kFeedback: return "Feedback";
        case kWidth:    return "Width";
        default:        return "";
    }
}

juce::String PitchShifter::getParameterText(int index, float /*value*/) const {
    if (index == kPitch) {
        // Return the actual snapped value with 3 decimal places
        float snappedValue = pimpl->snappedPitchValue.load();
        return juce::String(snappedValue, 3);  // 3 decimal places
    } else {
        // Other parameters show 2 decimal places
        return "";  // Let default handle it
    }
}