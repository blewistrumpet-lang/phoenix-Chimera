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
        
        // Laroche-Dolson phase locking arrays
        alignas(32) std::array<bool, FFT_SIZE/2 + 1> isPeak{};
        alignas(32) std::array<int, FFT_SIZE/2 + 1> closestPeak{};
        
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
            isPeak.fill(false);
            closestPeak.fill(-1);
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
        formantShift.setImmediate(0.5f);  // 0.5 is neutral for formant
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
        // Proper scaling for overlap-add with Hann window
        // For 75% overlap (4x), the scaling is 1/2 due to Hann window sum
        outputScale = 0.5f / OVERLAP_FACTOR;  // 0.5/4 = 0.125
        
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
        // Check for true bypass condition first
        const float currentPitch = pitchRatio.getValue();
        const float currentFormant = formantShift.getValue();
        const float currentMix = mixAmount.getValue();
        
        // Complete bypass if pitch is 1.0, formant is 0.5, and other params are neutral
        const bool canBypass = (std::abs(currentPitch - 1.0f) < 0.001f) && 
                               (std::abs(currentFormant - 0.5f) < 0.001f) &&
                               (std::abs(feedback.getValue()) < 0.001f) &&
                               (std::abs(spectralGate.getValue()) < 0.001f);
        
        if (canBypass && std::abs(currentMix - 1.0f) < 0.001f) {
            // Complete bypass - no processing at all
            return;
        }
        
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
        if (std::abs(pitch - 1.0f) < 0.001f && std::abs(formant - 0.5f) < 0.001f) {
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
            
            // Detect peaks for Laroche-Dolson phase locking
            detectPeaks(ch);
            
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
    
    void detectPeaks(ChannelState& ch) {
        // Reset peak detection
        ch.isPeak.fill(false);
        ch.closestPeak.fill(-1);
        
        // Find spectral peaks (local maxima)
        for (int k = 2; k < FFT_SIZE/2 - 2; ++k) {
            const float mag = ch.magnitude[k];
            
            // Local maximum detection
            if (mag > ch.magnitude[k-1] * 1.1f &&
                mag > ch.magnitude[k+1] * 1.1f &&
                mag > ch.magnitude[k-2] * 1.05f &&
                mag > ch.magnitude[k+2] * 1.05f &&
                mag > 0.0001f) {
                ch.isPeak[k] = true;
            }
        }
        
        // Assign each bin to its closest peak for phase locking
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            if (ch.isPeak[k]) {
                ch.closestPeak[k] = k;
            } else {
                // Find closest peak within reasonable range
                int closestDist = FFT_SIZE;
                int closestIdx = -1;
                
                for (int p = std::max(0, k - 50); p <= std::min(FFT_SIZE/2, k + 50); ++p) {
                    if (ch.isPeak[p]) {
                        int dist = std::abs(p - k);
                        if (dist < closestDist) {
                            closestDist = dist;
                            closestIdx = p;
                        }
                    }
                }
                
                ch.closestPeak[k] = closestIdx;
            }
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
        
        // Laroche-Dolson vertical phase coherence
        // Only apply to clear peaks to avoid over-processing
        if (std::abs(pitch - 1.0f) > 0.1f) {  // Only for significant pitch shifts
            for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
                if (!ch.isPeak[bin] && ch.closestPeak[bin] >= 0) {
                    int peakBin = ch.closestPeak[bin];
                    
                    // Only lock if peak is strong enough
                    if (ch.magnitude[peakBin] > 0.01f) {
                        // Calculate expected phase relationship
                        double expectedPhaseDiff = 2.0 * M_PI * (bin - peakBin) * HOP_SIZE / FFT_SIZE;
                        
                        // Lock phase to peak with gentle blending
                        double lockedPhase = ch.phaseSum[peakBin] + expectedPhaseDiff;
                        lockedPhase = lockedPhase - 2.0 * M_PI * std::round(lockedPhase / (2.0 * M_PI));
                        
                        // Blend 50% locked phase with 50% original for stability
                        ch.phaseSum[bin] = 0.5 * lockedPhase + 0.5 * ch.phaseSum[bin];
                        ch.phaseSum[bin] = ch.phaseSum[bin] - 2.0 * M_PI * std::round(ch.phaseSum[bin] / (2.0 * M_PI));
                    }
                }
            }
        }
        
        // Remove redundant second phase locking - already handled above
        
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
                    
                    // Skip spectral smoothing - it causes more artifacts than it solves
                    
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
            
            // FORMANT/BRIGHTNESS: Spectral envelope shifting
            // formant = 0.5 is neutral, < 0.5 darker, > 0.5 brighter
            // At 0.5, there should be NO change to the spectrum
            if (mag > 0 && std::abs(formant - 0.5f) > 0.001f) {
                // Formant shift by spectral envelope warping
                // This shifts the spectral envelope without changing pitch
                float formantFactor = std::pow(2.0f, (formant - 0.5f) * 2.0f);  // 0.5 to 2.0 range
                
                // Find the warped source bin for this frequency's envelope
                float envelopeSourceBin = bin / formantFactor;
                
                if (envelopeSourceBin >= 0 && envelopeSourceBin <= FFT_SIZE/2) {
                    int srcBin = static_cast<int>(envelopeSourceBin);
                    float frac = envelopeSourceBin - srcBin;
                    
                    // Get envelope magnitude from warped position
                    float envMag = 0;
                    if (srcBin < FFT_SIZE/2) {
                        float mag1 = originalMags[srcBin];
                        float mag2 = (srcBin + 1 <= FFT_SIZE/2) ? originalMags[srcBin + 1] : mag1;
                        envMag = mag1 + frac * (mag2 - mag1);
                    } else {
                        envMag = originalMags[FFT_SIZE/2];
                    }
                    
                    // Apply envelope scaling
                    if (originalMags[bin] > 1e-6f) {
                        mag = mag * (envMag / originalMags[bin]);
                    }
                }
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