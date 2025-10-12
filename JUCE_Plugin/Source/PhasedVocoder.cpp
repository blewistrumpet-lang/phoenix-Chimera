// PhasedVocoder.cpp - Platinum-spec implementation with all refinements
#include "PhasedVocoder.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  #include <immintrin.h>
#endif
#include <atomic>
#include <cmath>
#include <algorithm>
#include <iostream>

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

// Constants in anonymous namespace
namespace {
    constexpr int FFT_ORDER = 11;  // 2^11 = 2048
    constexpr int FFT_SIZE = 1 << FFT_ORDER;
    constexpr int OVERLAP = 4;
    constexpr int HOP_SIZE = FFT_SIZE / OVERLAP;
    constexpr int MAX_STRETCH = 16;
    constexpr double TWO_PI_D = 6.283185307179586476925286766559;
    constexpr double PI_D = 3.1415926535897932384626433832795;
    constexpr float TWO_PI = static_cast<float>(TWO_PI_D);
    
    // Use DSPUtils for denormal handling
    template<typename T>
    ALWAYS_INLINE T flushDenorm(T v) noexcept {
        return DSPUtils::flushDenorm(v);
    }
    
    // Optimized circular buffer indexing (no modulo)
    ALWAYS_INLINE size_t wrapIndex(size_t idx, size_t bufferSize) noexcept {
        return idx >= bufferSize ? idx - bufferSize : idx;
    }
}

// Thread-safe parameter smoother
class AtomicSmoother {
    float current;
    float coeff;
    std::atomic<float>& target;
    
public:
    AtomicSmoother(std::atomic<float>& t, float smoothTimeMs, double sampleRate)
        : target(t)
        , current(t.load(std::memory_order_relaxed))
    {
        const double tc = smoothTimeMs * 0.001;
        coeff = static_cast<float>(std::exp(-TWO_PI_D / (tc * sampleRate)));
    }
    
    ALWAYS_INLINE float tick() noexcept {
        const float tgt = target.load(std::memory_order_relaxed);
        current += (1.0f - coeff) * (tgt - current);
        return flushDenorm(current);
    }
    
    void reset(float value) noexcept {
        current = value;
    }
    
    void updateCoeff(float smoothTimeMs, double sampleRate) noexcept {
        const double tc = smoothTimeMs * 0.001;
        coeff = static_cast<float>(std::exp(-TWO_PI_D / (tc * sampleRate)));
    }
};

// Crossfade helper for freeze transitions
class CrossfadeState {
    int counter{0};
    int duration{0};
    
public:
    void trigger(int fadeFrames) noexcept {
        counter = duration = fadeFrames;
    }
    
    ALWAYS_INLINE float getWeight() noexcept {
        if (counter <= 0) return 1.0f;
        const float weight = static_cast<float>(counter) / duration;
        counter--;
        return weight;
    }
    
    ALWAYS_INLINE bool isActive() const noexcept {
        return counter > 0;
    }
    
    void reset() noexcept {
        counter = 0;
    }
};

// Enhanced transient detector with configurable response
class TransientDetector {
    float envelope{0.0f};
    float lastSum{0.0f};
    float attackCoeff{0.001f};
    float releaseCoeff{0.01f};
    
public:
    void prepare(double sampleRate, float attackMs, float releaseMs) noexcept {
        attackCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (attackMs * 0.001 * sampleRate)));
        releaseCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (releaseMs * 0.001 * sampleRate)));
    }
    
    ALWAYS_INLINE float process(float magnitudeSum) noexcept {
        const float flux = std::max(0.0f, magnitudeSum - lastSum);
        lastSum = magnitudeSum;
        
        const float target = flux * 10.0f;
        const float coeff = (target > envelope) ? attackCoeff : releaseCoeff;
        
        envelope += coeff * (target - envelope);
        envelope = flushDenorm(envelope);
        
        return std::min(1.0f, envelope);
    }
    
    void reset() noexcept {
        envelope = 0.0f;
        lastSum = 0.0f;
    }
};

// Silence detector for fast-path optimization
class SilenceDetector {
    static constexpr float SILENCE_THRESHOLD = 1e-6f;
    static constexpr int SILENCE_FRAMES = 512;
    int silenceCounter{0};
    bool isSilent{false};
    
public:
    ALWAYS_INLINE bool process(float rms) noexcept {
        if (rms < SILENCE_THRESHOLD) {
            if (++silenceCounter >= SILENCE_FRAMES) {
                isSilent = true;
            }
        } else {
            silenceCounter = 0;
            isSilent = false;
        }
        return isSilent;
    }
    
    void reset() noexcept {
        silenceCounter = 0;
        isSilent = false;
    }
};

// Implementation details
struct PhasedVocoder::Impl {
    // Thread-safe parameters with atomic access
    struct Parameters {
        std::atomic<float> timeStretch{1.0f};
        std::atomic<float> pitchShift{1.0f};
        std::atomic<float> spectralSmear{0.0f};
        std::atomic<float> transientPreserve{0.5f};
        std::atomic<float> phaseReset{0.0f};
        std::atomic<float> spectralGate{0.0f};
        std::atomic<float> mixAmount{1.0f};
        std::atomic<float> freeze{0.0f};
        std::atomic<float> transientAttack{1.0f};
        std::atomic<float> transientRelease{100.0f};
    } params;
    
    // Parameter smoothers
    std::unique_ptr<AtomicSmoother> timeStretchSmoother;
    std::unique_ptr<AtomicSmoother> pitchShiftSmoother;
    std::unique_ptr<AtomicSmoother> mixSmoother;
    
    // Per-channel processing state
    struct alignas(32) ChannelState {
        // Pre-allocated circular buffers (no dynamic allocation)
        static constexpr size_t BUFFER_SIZE = FFT_SIZE * 8;  // Larger for safety
        alignas(32) std::array<float, BUFFER_SIZE> inputBuffer{};
        alignas(32) std::array<float, BUFFER_SIZE> outputBuffer{};
        alignas(32) std::array<float, BUFFER_SIZE> normBuffer{};  // NEW: normalization buffer
        alignas(32) std::array<float, FFT_SIZE> grainBuffer{};
        
        // FFT workspace (JUCE uses interleaved real/imag format)
        alignas(32) std::array<float, FFT_SIZE * 2> fftRI{};  // [Re0, Im0, Re1, Im1, ...]
        alignas(32) std::array<float, FFT_SIZE * 2> savedRI{}; // For STFT identity test
        alignas(32) std::array<float, FFT_SIZE> window{};
        
        // Spectral data (double precision for phase accumulation)
        alignas(32) std::array<float, FFT_SIZE/2 + 1> magnitude{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> phase{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> lastPhase{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> instFreq{};  // instantaneous frequency
        alignas(32) std::array<double, FFT_SIZE/2 + 1> synthPhase{};  // synthesis phase accumulator
        bool firstFrame{true};  // Flag for first frame initialization
        alignas(32) std::array<double, FFT_SIZE/2 + 1> omega{};  // bin frequencies
        
        // Freeze state
        alignas(32) std::array<float, FFT_SIZE/2 + 1> freezeMagnitude{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> freezePhase{};
        std::atomic<bool> isFrozen{false};
        
        // Position tracking
        size_t inputWritePos{0};
        size_t outputWritePos{0};
        size_t outputReadPos{0};
        int accumulated{0};  // samples accumulated for next hop
        int latency{FFT_SIZE};
        size_t warmupSamples{0};  // Priming counter
        
        // Transient detection
        TransientDetector transientDetector;
        
        // Denormal flush counter
        int denormFlushCounter{0};
        
        // FFT scaling detection
        float invIFFTRoundtrip{1.0f};  // 1 / (forward*inverse scale)
        
        // Crossfade state
        CrossfadeState freezeCrossfade;
        
        // Silence detection
        SilenceDetector silenceDetector;
        
        juce::dsp::FFT fft{FFT_ORDER};
    };
    
    std::vector<std::unique_ptr<ChannelState>> channelStates;
    double sampleRate{44100.0};
    float invFFTSize{1.0f / FFT_SIZE};
    float windowSum{0.0f};
    
    // Processing methods
    void processFrame(ChannelState& state) noexcept;
    void analyzeFrame(ChannelState& state) noexcept;
    void synthesizeFrame(ChannelState& state) noexcept;
    void applySpectralProcessing(ChannelState& state) noexcept;
    void initializeWindow(std::array<float, FFT_SIZE>& window) noexcept;
    void flushAllDenormals(ChannelState& state) noexcept;
};

// Constructor/Destructor
PhasedVocoder::PhasedVocoder() : pimpl(std::make_unique<Impl>()) {}
PhasedVocoder::~PhasedVocoder() = default;

// Public interface implementation
void PhasedVocoder::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    pimpl->invFFTSize = 1.0f / FFT_SIZE;
    
    // Initialize parameter smoothers
    pimpl->timeStretchSmoother = std::make_unique<AtomicSmoother>(
        pimpl->params.timeStretch, 5.0f, sampleRate);
    pimpl->pitchShiftSmoother = std::make_unique<AtomicSmoother>(
        pimpl->params.pitchShift, 5.0f, sampleRate);
    pimpl->mixSmoother = std::make_unique<AtomicSmoother>(
        pimpl->params.mixAmount, 2.0f, sampleRate);
    
    // Pre-allocate for 2 channels (stereo)
    pimpl->channelStates.clear();
    for (int i = 0; i < 2; ++i) {
        pimpl->channelStates.push_back(std::make_unique<Impl::ChannelState>());
    }
    
    // Initialize windows and calculate normalization
    pimpl->windowSum = 0.0f;
    for (auto& statePtr : pimpl->channelStates) {
        pimpl->initializeWindow(statePtr->window);
        
        // Calculate proper window sum for overlap-add normalization
        // For Hann window with 75% overlap (4x), the overlapping windows should sum to ~1.5
        pimpl->windowSum = 0.0f;
        for (int offset = 0; offset < HOP_SIZE; ++offset) {
            float sum = 0.0f;
            for (int frame = 0; frame < OVERLAP; ++frame) {
                int idx = offset + frame * HOP_SIZE;
                if (idx < FFT_SIZE) {
                    sum += statePtr->window[idx];
                }
            }
            if (sum > pimpl->windowSum) {
                pimpl->windowSum = sum; // Use peak overlap sum
            }
        }
        
        // For Hann window with 75% overlap, this should be ~1.5
        // If calculation is off, use the theoretical value
        if (pimpl->windowSum < 1.0f || pimpl->windowSum > 2.0f) {
            pimpl->windowSum = 1.5f; // Theoretical value for Hann with 75% overlap
        }
        
        // Clear all buffers
        std::fill(statePtr->inputBuffer.begin(), statePtr->inputBuffer.end(), 0.0f);
        std::fill(statePtr->outputBuffer.begin(), statePtr->outputBuffer.end(), 0.0f);
        std::fill(statePtr->normBuffer.begin(), statePtr->normBuffer.end(), 0.0f);
        std::fill(statePtr->lastPhase.begin(), statePtr->lastPhase.end(), 0.0);
        std::fill(statePtr->synthPhase.begin(), statePtr->synthPhase.end(), 0.0);
        std::fill(statePtr->instFreq.begin(), statePtr->instFreq.end(), 0.0);
        
        // Initialize omega (bin frequencies in radians/sample)
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            statePtr->omega[k] = 2.0 * M_PI * k / FFT_SIZE;
        }
        
        
        // Auto-detect FFT round-trip scaling
        {
            // Impulse in time domain -> FFT -> IFFT -> measure y[0]
            std::vector<float> tmp(2 * FFT_SIZE, 0.0f);   // interleaved re/im
            tmp[0] = 1.0f;                                 // x[0] = 1, others 0

            statePtr->fft.perform(reinterpret_cast<std::complex<float>*>(tmp.data()),
                                reinterpret_cast<std::complex<float>*>(tmp.data()), false);
            statePtr->fft.perform(reinterpret_cast<std::complex<float>*>(tmp.data()),
                                reinterpret_cast<std::complex<float>*>(tmp.data()), true);

            // After a forward+inverse roundtrip on an impulse, tmp[0] equals the overall scale
            const float roundtrip = tmp[0];
            // Guard tiny/NaN
            statePtr->invIFFTRoundtrip = (std::abs(roundtrip) > 1e-12f) ? (1.0f / roundtrip) : 1.0f;
            
        }
        
        // Initialize positions with proper write head offset
        statePtr->outputReadPos = 0;
        statePtr->latency = FFT_SIZE;  // safe lead
        statePtr->outputWritePos = (statePtr->outputReadPos + statePtr->latency) % statePtr->outputBuffer.size();
        statePtr->inputWritePos = 0;
        statePtr->accumulated = 0;
        statePtr->firstFrame = true;
        statePtr->warmupSamples = statePtr->latency + HOP_SIZE;  // Need latency + one hop for proper initialization
        
        statePtr->isFrozen = false;
        statePtr->denormFlushCounter = 0;
        
        // Initialize transient detector
        const float attack = pimpl->params.transientAttack.load(std::memory_order_relaxed);
        const float release = pimpl->params.transientRelease.load(std::memory_order_relaxed);
        statePtr->transientDetector.prepare(sampleRate, attack, release);
        
        statePtr->freezeCrossfade.reset();
        statePtr->silenceDetector.reset();
    }
}

void PhasedVocoder::reset() {
    for (auto& statePtr : pimpl->channelStates) {
        auto& state = *statePtr;
        // Zero all audio buffers
        std::fill(state.inputBuffer.begin(), state.inputBuffer.end(), 0.0f);
        std::fill(state.outputBuffer.begin(), state.outputBuffer.end(), 0.0f);
        std::fill(state.normBuffer.begin(), state.normBuffer.end(), 0.0f);
        std::fill(state.grainBuffer.begin(), state.grainBuffer.end(), 0.0f);
        
        // Reset phase accumulators
        std::fill(state.phase.begin(), state.phase.end(), 0.0);
        std::fill(state.lastPhase.begin(), state.lastPhase.end(), 0.0);
        std::fill(state.synthPhase.begin(), state.synthPhase.end(), 0.0);
        std::fill(state.instFreq.begin(), state.instFreq.end(), 0.0);
        
        // Auto-detect FFT round-trip scaling
        {
            std::vector<float> tmp(2 * FFT_SIZE, 0.0f);
            tmp[0] = 1.0f;
            
            state.fft.perform(reinterpret_cast<std::complex<float>*>(tmp.data()),
                            reinterpret_cast<std::complex<float>*>(tmp.data()), false);
            state.fft.perform(reinterpret_cast<std::complex<float>*>(tmp.data()),
                            reinterpret_cast<std::complex<float>*>(tmp.data()), true);
            
            const float roundtrip = tmp[0];
            state.invIFFTRoundtrip = (std::abs(roundtrip) > 1e-12f) ? (1.0f / roundtrip) : 1.0f;
        }
        
        // Reset positions with proper write head offset
        state.outputReadPos = 0;
        state.latency = FFT_SIZE;  // safe lead
        state.outputWritePos = (state.outputReadPos + state.latency) % state.outputBuffer.size();
        state.inputWritePos = 0;
        state.accumulated = 0;
        state.firstFrame = true;
        state.warmupSamples = state.latency + HOP_SIZE;  // Need latency + one hop for proper initialization
        
        // Reset detection states
        state.transientDetector.reset();
        state.silenceDetector.reset();
        state.freezeCrossfade.reset();
        
        // Clear freeze state
        state.isFrozen = false;
        state.denormFlushCounter = 0;
    }
    
    // Reset smoothers
    if (pimpl->timeStretchSmoother) {
        pimpl->timeStretchSmoother->reset(pimpl->params.timeStretch.load());
        pimpl->pitchShiftSmoother->reset(pimpl->params.pitchShift.load());
        pimpl->mixSmoother->reset(pimpl->params.mixAmount.load());
    }
}

void PhasedVocoder::process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    
    // Get smoothed parameters
    const float smoothMix = pimpl->mixSmoother->tick();
    const float freeze = pimpl->params.freeze.load(std::memory_order_relaxed);
    const bool shouldFreeze = freeze > 0.5f;
    
    // Early bypass check for mix parameter
    if (smoothMix < 0.001f) {
        // Completely dry - no processing needed, just advance smoothers for smooth operation
        for (int i = 1; i < numSamples; ++i) { // Skip first since we already called tick()
            pimpl->mixSmoother->tick();
        }
        return;
    }
    
    for (int ch = 0; ch < numChannels && ch < pimpl->channelStates.size(); ++ch) {
        auto& state = *pimpl->channelStates[ch];
        float* channelData = buffer.getWritePointer(ch);
        
        // REMOVED: Silence detection - it was killing valid audio
        
        // Update freeze state with crossfade
        bool wasFrozen = state.isFrozen.load(std::memory_order_relaxed);
        if (shouldFreeze && !wasFrozen) {
            state.freezeCrossfade.trigger(HOP_SIZE);
            state.freezeMagnitude = state.magnitude;
            state.freezePhase = state.phase;
            state.isFrozen.store(true, std::memory_order_relaxed);
        } else if (!shouldFreeze && wasFrozen) {
            state.freezeCrossfade.trigger(HOP_SIZE);
            state.isFrozen.store(false, std::memory_order_relaxed);
        }
        
        // Process samples with proper frame scheduling
        for (int i = 0; i < numSamples; ++i) {
            // Store input sample in circular buffer
            state.inputBuffer[state.inputWritePos] = channelData[i];
            state.inputWritePos = (state.inputWritePos + 1) % state.inputBuffer.size();
            
            // Trigger every needed frame (use while loop for large blocks)
            state.accumulated += 1;
            while (state.accumulated >= HOP_SIZE) {
                state.accumulated -= HOP_SIZE;
                pimpl->processFrame(state);
            }
            
            // Read path: consume one sample, no latency math here
            const size_t readIdx = state.outputReadPos;
            float y = 0.0f;
            
            bool priming = state.warmupSamples > 0;
            if (!priming) {
                const float g = state.normBuffer[readIdx];
                if (g > 1e-9f) {
                    y = state.outputBuffer[readIdx] / g;
                }
                
            } else {
                --state.warmupSamples;
            }
            
            // consume
            state.outputBuffer[readIdx] = 0.0f;
            state.normBuffer[readIdx] = 0.0f;
            state.outputReadPos = (state.outputReadPos + 1) % state.outputBuffer.size();
            
            // mix
            channelData[i] = flushDenorm(channelData[i] * (1.0f - smoothMix) + y * smoothMix);
        }
    }
    
    // Scrub buffer for NaN/Inf protection
    scrubBuffer(buffer);
}

// Implementation methods
void PhasedVocoder::Impl::processFrame(ChannelState& state) noexcept {
    // Get smoothed parameters
    const float smoothTimeStretch = timeStretchSmoother->tick();
    const float smoothPitchShift = pitchShiftSmoother->tick();
    
    // Read analysis frame from the input buffer
    // Frame ends at current write position minus one
    const size_t frameEnd = (state.inputWritePos + state.inputBuffer.size() - 1) % state.inputBuffer.size();
    const size_t frameStart = (frameEnd + state.inputBuffer.size() - FFT_SIZE + 1) % state.inputBuffer.size();
    
    // Fill grain buffer with windowed input
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        size_t idx = (frameStart + i) % state.inputBuffer.size();
        state.grainBuffer[i] = state.inputBuffer[idx] * state.window[i];
    }
    
    // Process spectral data
    analyzeFrame(state);
    applySpectralProcessing(state);
    synthesizeFrame(state);
    
    // Comprehensive denormal flush
    flushAllDenormals(state);
}

void PhasedVocoder::Impl::analyzeFrame(ChannelState& state) noexcept {
    // Copy to FFT buffer in JUCE's interleaved format
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        state.fftRI[2*i] = state.grainBuffer[i];  // Real part
        state.fftRI[2*i + 1] = 0.0f;              // Imaginary part
    }
    
    // Forward FFT (JUCE expects complex<float>* but we have interleaved floats)
    state.fft.perform(reinterpret_cast<std::complex<float>*>(state.fftRI.data()), 
                      reinterpret_cast<std::complex<float>*>(state.fftRI.data()), false);
    
    // Save spectrum for STFT identity test
    std::memcpy(state.savedRI.data(), state.fftRI.data(), sizeof(float) * 2 * FFT_SIZE);
    
    // Extract magnitude and phase with phase vocoder analysis
    const double Ha = static_cast<double>(HOP_SIZE);
    
    for (size_t k = 0; k <= FFT_SIZE/2; ++k) {
        const float real = state.fftRI[2*k];
        const float imag = state.fftRI[2*k + 1];

        // Magnitude
        state.magnitude[k] = std::sqrt(real * real + imag * imag);

        // Phase
        const double currentPhase = std::atan2(static_cast<double>(imag),
                                              static_cast<double>(real));
        state.phase[k] = currentPhase;

        // CRITICAL FIX: Proper phase vocoder analysis with phase unwrapping
        const double omega_k = 2.0 * M_PI * static_cast<double>(k) / static_cast<double>(FFT_SIZE);

        // Special case for DC bin (k=0): no phase advance needed
        if (k == 0) {
            state.instFreq[k] = 0.0;
            state.lastPhase[k] = currentPhase;
            continue;
        }

        double delta = currentPhase - state.lastPhase[k] - omega_k * Ha;

        // Principal value in (-pi, pi] using std::remainder
        delta = std::remainder(delta, 2.0 * M_PI);

        // Instantaneous frequency (rad/sample)
        state.instFreq[k] = omega_k + delta / Ha;

        // CRITICAL FIX: Clamp instantaneous frequency to reasonable range
        // to prevent runaway phase accumulation - but allow for extreme pitch shifts
        // Max frequency is Nyquist (pi rad/sample)
        const double maxFreq = M_PI * 0.95;  // Up to ~95% of Nyquist
        state.instFreq[k] = std::max(-maxFreq, std::min(maxFreq, state.instFreq[k]));

        // Update last phase for next frame
        state.lastPhase[k] = currentPhase;
    }
}

void PhasedVocoder::Impl::applySpectralProcessing(ChannelState& state) noexcept {
    const float spectralGate = params.spectralGate.load(std::memory_order_relaxed);
    const float spectralSmear = params.spectralSmear.load(std::memory_order_relaxed);
    const bool isFrozen = state.isFrozen.load(std::memory_order_relaxed);
    
    // Spectral gate with smooth threshold
    if (spectralGate > 0.0f) {
        const float threshold = spectralGate * spectralGate * 0.01f;
        for (size_t bin = 0; bin <= FFT_SIZE/2; ++bin) {
            if (state.magnitude[bin] < threshold) {
                state.magnitude[bin] = 0.0f;
            }
        }
    }
    
    // Spectral smearing with SIMD
    if (spectralSmear > 0.0f) {
        alignas(32) std::array<float, FFT_SIZE/2 + 1> smearedMag;
        const int smearWidth = static_cast<int>(spectralSmear * 10.0f + 1.0f);
        
        for (size_t bin = 0; bin <= FFT_SIZE/2; ++bin) {
            float sum = 0.0f;
            int count = 0;
            
            const size_t start = (bin > smearWidth) ? bin - smearWidth : 0;
            const size_t end = std::min(bin + smearWidth + 1, static_cast<size_t>(FFT_SIZE/2 + 1));
            
            for (size_t idx = start; idx < end; ++idx) {
                sum += state.magnitude[idx];
                count++;
            }
            
            smearedMag[bin] = sum / std::max(1, count);
        }
        
        state.magnitude = smearedMag;
    }
    
    // Freeze processing with crossfade
    if (state.freezeCrossfade.isActive()) {
        const float weight = state.freezeCrossfade.getWeight();
        
        if (isFrozen) {
            // Crossfade TO frozen state
            for (size_t bin = 0; bin <= FFT_SIZE/2; ++bin) {
                state.magnitude[bin] = state.magnitude[bin] * weight + 
                                      state.freezeMagnitude[bin] * (1.0f - weight);
                state.phase[bin] = state.phase[bin] * weight + 
                                  state.freezePhase[bin] * (1.0 - weight);
            }
        } else {
            // Crossfade FROM frozen state
            for (size_t bin = 0; bin <= FFT_SIZE/2; ++bin) {
                state.magnitude[bin] = state.freezeMagnitude[bin] * weight + 
                                      state.magnitude[bin] * (1.0f - weight);
                state.phase[bin] = state.freezePhase[bin] * weight + 
                                  state.phase[bin] * (1.0 - weight);
            }
        }
    } else if (isFrozen) {
        // Fully frozen
        state.magnitude = state.freezeMagnitude;
        
        const float phaseReset = params.phaseReset.load(std::memory_order_relaxed);
        if (phaseReset > 0.0f) {
            for (size_t bin = 0; bin <= FFT_SIZE/2; ++bin) {
                state.phase[bin] = state.freezePhase[bin] * (1.0 - phaseReset) + 
                                  state.phase[bin] * phaseReset;
            }
        } else {
            state.phase = state.freezePhase;
        }
    }
}

void PhasedVocoder::Impl::synthesizeFrame(ChannelState& state) noexcept {
    const float timeStretch = timeStretchSmoother->tick();
    const float pitchShift = pitchShiftSmoother->tick();
    
    // Calculate synthesis hop size (H_s) - must be integer rounded and bounded
    const double Ha = static_cast<double>(HOP_SIZE);
    double Hs = std::round(HOP_SIZE * timeStretch);

    // CRITICAL FIX: Ensure Hs is always valid (at least 1, max reasonable)
    Hs = std::max(1.0, std::min(Hs, static_cast<double>(HOP_SIZE * MAX_STRETCH)));
    
    // Initialize synthesis phase on first frame
    if (state.firstFrame) {
        for (size_t k = 0; k <= FFT_SIZE/2; ++k) {
            state.synthPhase[k] = state.phase[k];
        }
        state.firstFrame = false;
    }

    // Standard per-bin phase vocoder synthesis with proper phase accumulation
    for (size_t k = 0; k <= FFT_SIZE/2; ++k) {
        // CRITICAL FIX: Guard against invalid instantaneous frequency
        double instFreqClamped = state.instFreq[k];
        if (std::isnan(instFreqClamped) || std::isinf(instFreqClamped)) {
            instFreqClamped = state.omega[k];  // Fall back to bin center frequency
        }

        // Advance synthesis phase based on instantaneous frequency
        state.synthPhase[k] += instFreqClamped * Hs * pitchShift;

        // Wrap phase to avoid accumulation overflow
        state.synthPhase[k] = std::remainder(state.synthPhase[k], 2.0 * M_PI);

        // CRITICAL FIX: Guard against NaN/Inf in magnitude
        float mag = state.magnitude[k];
        if (std::isnan(mag) || std::isinf(mag) || mag < 0.0f) {
            mag = 0.0f;
        }

        const float ph = static_cast<float>(state.synthPhase[k]);
        state.fftRI[2*k]     = mag * std::cos(ph);
        state.fftRI[2*k + 1] = mag * std::sin(ph);
    }

    // CRITICAL FIX: Proper Hermitian symmetry for real IFFT
    // Mirror positive frequencies to negative frequencies (conjugate symmetry)
    // For real signal: X[N-k] = conj(X[k]) for k = 1..N/2-1
    for (size_t k = 1; k < FFT_SIZE/2; ++k) {
        state.fftRI[2*(FFT_SIZE - k)]     =  state.fftRI[2*k];       // real part same
        state.fftRI[2*(FFT_SIZE - k) + 1] = -state.fftRI[2*k + 1];   // imag part negated
    }

    // CRITICAL FIX: Ensure DC (k=0) and Nyquist (k=N/2) bins are purely real
    state.fftRI[1] = 0.0f;                      // DC imaginary = 0
    state.fftRI[2*(FFT_SIZE/2) + 1] = 0.0f;     // Nyquist imaginary = 0
    
    // IFFT on fftRI
    state.fft.perform(reinterpret_cast<std::complex<float>*>(state.fftRI.data()),
                      reinterpret_cast<std::complex<float>*>(state.fftRI.data()), true);
    
    // Scale exactly once according to runtime detection
    const float postIFFTScale = state.invIFFTRoundtrip;   // replaces 1.0f / FFT_SIZE
    
    
    // OLA (writer head) + normalizer
    const size_t wBase = state.outputWritePos;
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        const size_t idx = (wBase + i) % state.outputBuffer.size();
        const float  w   = state.window[i];
        const float  s   = state.fftRI[2*i] * postIFFTScale;  // real part
        state.outputBuffer[idx] += s * w;
        state.normBuffer[idx]   += w * w;
    }
    
    // advance writer by synthesis hop (use the same Hs computed at top)
    state.outputWritePos = (state.outputWritePos + static_cast<size_t>(Hs)) % state.outputBuffer.size();
}

void PhasedVocoder::Impl::initializeWindow(std::array<float, FFT_SIZE>& window) noexcept {
    // Hann window with exact normalization
    constexpr float norm = 1.0f / (FFT_SIZE - 1);
    
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(TWO_PI * i * norm));
    }
}

void PhasedVocoder::Impl::flushAllDenormals(ChannelState& state) noexcept {
    if (++state.denormFlushCounter >= 256) {
        state.denormFlushCounter = 0;
        
        // Flush ALL feedback paths
        for (auto& p : state.synthPhase) p = flushDenorm(p);
        for (auto& p : state.lastPhase) p = flushDenorm(p);
        for (auto& f : state.instFreq) f = flushDenorm(f);
        for (auto& m : state.magnitude) m = flushDenorm(m);
        
        // Also flush transient detector state
        state.transientDetector.process(0.0f); // Forces internal flush
    }
}

// Parameter updates (thread-safe)
void PhasedVocoder::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (static_cast<ParamID>(id)) {
            case ParamID::TimeStretch:
                // CRITICAL FIX: Map 0-1 to 0.25x-4x with proper clamping
                // Snap zone around 1.0x for easier neutral positioning
                {
                    float stretch;
                    if (std::abs(value - 0.2f) < 0.02f) {
                        stretch = 1.0f;  // Snap to 1x for pass-through (wider tolerance)
                    } else {
                        stretch = 0.25f + value * 3.75f;
                        stretch = std::max(0.25f, std::min(4.0f, stretch));  // Ensure bounds
                    }
                    pimpl->params.timeStretch.store(stretch, std::memory_order_relaxed);
                }
                break;
            case ParamID::PitchShift:
                // CRITICAL FIX: Map 0-1 to 0.5x-2x with proper clamping
                {
                    float pitch = 0.5f + value * 1.5f;
                    pitch = std::max(0.5f, std::min(2.0f, pitch));  // Ensure bounds
                    pimpl->params.pitchShift.store(pitch, std::memory_order_relaxed);
                }
                break;
            case ParamID::SpectralSmear:
                pimpl->params.spectralSmear.store(value, std::memory_order_relaxed);
                break;
            case ParamID::TransientPreserve:
                pimpl->params.transientPreserve.store(value, std::memory_order_relaxed);
                break;
            case ParamID::PhaseReset:
                pimpl->params.phaseReset.store(value, std::memory_order_relaxed);
                break;
            case ParamID::SpectralGate:
                pimpl->params.spectralGate.store(value, std::memory_order_relaxed);
                break;
            case ParamID::Mix:
                pimpl->params.mixAmount.store(value, std::memory_order_relaxed);
                break;
            case ParamID::Freeze:
                pimpl->params.freeze.store(value, std::memory_order_relaxed);
                break;
            case ParamID::TransientAttack:
                {
                    const float attackMs = 0.1f + value * 9.9f; // 0.1-10ms
                    pimpl->params.transientAttack.store(attackMs, std::memory_order_relaxed);
                    
                    // Update transient detectors
                    const float releaseMs = pimpl->params.transientRelease.load();
                    for (auto& statePtr : pimpl->channelStates) {
                        statePtr->transientDetector.prepare(pimpl->sampleRate, attackMs, releaseMs);
                    }
                }
                break;
            case ParamID::TransientRelease:
                {
                    const float releaseMs = 10.0f + value * 490.0f; // 10-500ms
                    pimpl->params.transientRelease.store(releaseMs, std::memory_order_relaxed);
                    
                    // Update transient detectors
                    const float attackMs = pimpl->params.transientAttack.load();
                    for (auto& statePtr : pimpl->channelStates) {
                        statePtr->transientDetector.prepare(pimpl->sampleRate, attackMs, releaseMs);
                    }
                }
                break;
        }
    }
}

juce::String PhasedVocoder::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::TimeStretch:       return "Stretch";
        case ParamID::PitchShift:        return "Pitch";
        case ParamID::SpectralSmear:     return "Smear";
        case ParamID::TransientPreserve: return "Transient";
        case ParamID::PhaseReset:        return "Phase";
        case ParamID::SpectralGate:      return "Gate";
        case ParamID::Mix:               return "Mix";
        case ParamID::Freeze:            return "Freeze";
        case ParamID::TransientAttack:   return "Attack";
        case ParamID::TransientRelease:  return "Release";
        default:                         return "";
    }
}

juce::String PhasedVocoder::getParameterDisplayString(int index, float value) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::TimeStretch: {
            // Time stretch from 0.25x to 4x
            float stretch = 0.25f + value * 3.75f;
            return juce::String(stretch, 2) + "x";
        }
        case ParamID::PitchShift: {
            // Pitch shift from -24 to +24 semitones
            float semitones = (value - 0.5f) * 48.0f;
            if (std::abs(semitones) < 0.1f) return "0 st";
            return juce::String(semitones, 1) + " st";
        }
        case ParamID::SpectralSmear: {
            // Spectral smear 0-100%
            return juce::String(static_cast<int>(value * 100)) + "%";
        }
        case ParamID::TransientPreserve: {
            // Transient preservation 0-100%
            return juce::String(static_cast<int>(value * 100)) + "%";
        }
        case ParamID::PhaseReset: {
            // Phase reset 0-100%
            return juce::String(static_cast<int>(value * 100)) + "%";
        }
        case ParamID::SpectralGate: {
            // Spectral gate threshold 0-100%
            return juce::String(static_cast<int>(value * 100)) + "%";
        }
        case ParamID::Mix: {
            // Mix 0-100%
            return juce::String(static_cast<int>(value * 100)) + "%";
        }
        case ParamID::Freeze: {
            // Freeze on/off
            return value > 0.5f ? "ON" : "OFF";
        }
        case ParamID::TransientAttack: {
            // Attack time 0.1-100ms
            float ms = 0.1f + value * 99.9f;
            return juce::String(ms, 1) + " ms";
        }
        case ParamID::TransientRelease: {
            // Release time 1-500ms
            float ms = 1.0f + value * 499.0f;
            return juce::String(ms, 0) + " ms";
        }
        default:
            return "";
    }
}

juce::String PhasedVocoder::getName() const { 
    return "Phased Vocoder"; 
}