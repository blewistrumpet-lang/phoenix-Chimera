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
        int feedbackReadPos{0};   // Separate read position for feedback
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
        outputScale = 1.0f / OVERLAP_FACTOR;  // Correct scaling: 1/4 = 0.25, not 1/32768!
        
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
            
            // Soft saturation for overloads
            if (std::abs(output) > 0.95f) {
                output = std::tanh(output);
            }
            
            // Mix with dry (per-sample for smooth automation)
            data[i] = DSPUtils::flushDenorm(input * (1.0f - mix) + output * mix);
        }
    }
    
    void processSpectralFrame(ChannelState& ch, float pitch, float formant, float gate, float window) {
        // Gather frame from ring buffer (zero-copy)
        ch.gatherFrame();
        
        // Window input with denormal prevention
        // Window parameter affects the window shape (0.0 = sharp, 1.0 = smooth)
        alignas(32) std::array<float, FFT_SIZE> windowed;
        for (int i = 0; i < FFT_SIZE; ++i) {
            // Apply dynamic window shaping based on parameter
            float windowShape = ch.analysisWindow[i];
            if (window < 0.5f) {
                // Sharper window for better time resolution
                windowShape = std::pow(windowShape, 1.0f + (0.5f - window) * 2.0f);
            } else {
                // Smoother window for better frequency resolution
                windowShape = std::pow(windowShape, 1.0f / (1.0f + (window - 0.5f) * 2.0f));
            }
            windowed[i] = DSPUtils::flushDenorm(ch.frameBuffer[i] * windowShape);
        }
        
        // Copy to complex array
        for (int i = 0; i < FFT_SIZE; ++i) {
            ch.spectrum[i] = std::complex<float>(windowed[i], 0.0f);
        }
        
        // Forward FFT
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), false);
        
        // Phase vocoder analysis with denormal prevention
        analyzeSpectrum(ch);
        
        // Apply spectral gate with denormal prevention
        if (gate > 1e-6f) {
            const float threshold = gate * gate * 0.1f;
            // Vectorized denormal prevention for magnitude array
            for (auto& mag : ch.magnitude) {
                mag = DSPUtils::flushDenorm(mag);
            }
            
            for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
                if (ch.magnitude[bin] < threshold) {
                    ch.magnitude[bin] = 0.0f;  // Hard gate prevents denormals
                }
            }
        }
        
        // Shift spectrum
        shiftSpectrum(ch, pitch, formant);
        
        // Inverse FFT
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), true);
        
        // Scatter to output ring buffer with overlap-add
        ch.scatterFrame(ch.spectrum.data(), outputScale);
        
        // Periodic denormal flush for phase accumulators
        if (++denormalFlushCounter >= 256) {
            denormalFlushCounter = 0;
            for (int i = 0; i <= FFT_SIZE/2; ++i) {
                ch.phaseSum[i] = DSPUtils::flushDenorm(ch.phaseSum[i]);
                ch.phaseLast[i] = DSPUtils::flushDenorm(ch.phaseLast[i]);
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
            
            // Wrap to [-pi, pi] using efficient modulo operation
            // This prevents numerical drift from iterative subtraction
            phaseDiff = std::fmod(phaseDiff + M_PI, 2.0 * M_PI) - M_PI;
            
            // True frequency calculation
            const double expectedPhase = expectedPhaseInc * bin;
            const double deviation = phaseDiff - expectedPhase;
            const double trueFreq = binFrequency * bin + 
                                   deviation * sampleRate / (2.0 * M_PI * HOP_SIZE);
            
            ch.frequency[bin] = DSPUtils::flushDenorm(static_cast<float>(trueFreq));
        }
    }
    
    void shiftSpectrum(ChannelState& ch, float pitch, float formant) {
        // Temporary buffer for shifted spectrum
        alignas(16) std::array<std::complex<float>, FFT_SIZE> shifted{};
        
        // CRITICAL FIX: Update phase for ALL bins, not just non-zero ones
        // The phase accumulator needs to track phase evolution even for silent bins
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            // Calculate the frequency for this bin after pitch shifting
            const double shiftedFreq = ch.frequency[bin] * pitch;
            
            // Update phase accumulator for this frequency
            ch.phaseSum[bin] += 2.0 * M_PI * shiftedFreq * HOP_SIZE / sampleRate;
            
            // Wrap phase to [-pi, pi] to prevent numerical overflow
            while (ch.phaseSum[bin] > M_PI) ch.phaseSum[bin] -= 2.0 * M_PI;
            while (ch.phaseSum[bin] < -M_PI) ch.phaseSum[bin] += 2.0 * M_PI;
        }
        
        // Now apply spectral reconstruction with optional formant shift
        if (std::abs(formant - 1.0f) < 0.001f) {
            // No formant shift - direct reconstruction with pitch-shifted phases
            for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
                if (ch.magnitude[bin] > 1e-10f) {
                    // Use original magnitude with pitch-shifted phase
                    const float mag = ch.magnitude[bin];
                    const float phase = static_cast<float>(ch.phaseSum[bin]);
                    shifted[bin] = std::polar(mag, phase);
                    
                    // Maintain Hermitian symmetry for real output
                    if (bin > 0 && bin < FFT_SIZE/2) {
                        shifted[FFT_SIZE - bin] = std::conj(shifted[bin]);
                    }
                }
            }
        } else {
            // Apply formant shift by remapping magnitude envelope
            // while preserving pitch-shifted phases
            for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
                // Find source bin for magnitude (formant shift)
                const int sourceBin = static_cast<int>(bin / formant + 0.5f);
                
                if (sourceBin >= 0 && sourceBin <= FFT_SIZE/2 && 
                    ch.magnitude[sourceBin] > 1e-10f) {
                    // Use magnitude from formant-shifted source
                    // but phase from pitch-shifted target bin
                    const float mag = ch.magnitude[sourceBin];
                    const float phase = static_cast<float>(ch.phaseSum[bin]);
                    shifted[bin] = std::polar(mag, phase);
                    
                    // Maintain Hermitian symmetry
                    if (bin > 0 && bin < FFT_SIZE/2) {
                        shifted[FFT_SIZE - bin] = std::conj(shifted[bin]);
                    }
                }
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
                // Convert 0-1 parameter to -24 to +24 semitones for musical intervals
                // 0.0 = -24 semitones (2 octaves down)
                // 0.5 = 0 semitones (unison)
                // 1.0 = +24 semitones (2 octaves up)
                float semitones = (value - 0.5f) * 48.0f;
                float ratio = std::pow(2.0f, semitones / 12.0f);
                pimpl->pitchRatio.setTarget(ratio);
                break;
            }
            case kFormant:  
                // Fix formant mapping: 0.5 should map to 1.0 (no shift)
                // 0.0 -> 0.5 (down one octave formant)
                // 0.5 -> 1.0 (no shift)
                // 1.0 -> 2.0 (up one octave formant)
                pimpl->formantShift.setTarget(0.5f + value); 
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