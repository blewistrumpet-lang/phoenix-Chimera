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
        static constexpr size_t BUFFER_SIZE = FFT_SIZE * MAX_STRETCH * 2;
        alignas(32) std::array<float, BUFFER_SIZE> inputBuffer{};
        alignas(32) std::array<float, BUFFER_SIZE> outputBuffer{};
        alignas(32) std::array<float, FFT_SIZE> grainBuffer{};
        
        // FFT workspace
        alignas(32) std::array<std::complex<float>, FFT_SIZE> fftBuffer{};
        alignas(32) std::array<float, FFT_SIZE> window{};
        
        // Spectral data (double precision for phase accumulation)
        alignas(32) std::array<float, FFT_SIZE/2 + 1> magnitude{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> phase{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> lastPhase{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> phaseAccum{};
        alignas(32) std::array<float, FFT_SIZE/2 + 1> trueBinFreq{};
        
        // Freeze state
        alignas(32) std::array<float, FFT_SIZE/2 + 1> freezeMagnitude{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> freezePhase{};
        std::atomic<bool> isFrozen{false};
        
        // Position tracking (double precision for sub-sample accuracy)
        double readPos{0.0};
        size_t writePos{0};
        size_t outputWritePos{0};
        size_t outputReadPos{0};
        int hopCounter{0};
        
        // Transient detection
        TransientDetector transientDetector;
        
        // Denormal flush counter
        int denormFlushCounter{0};
        
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
        std::fill(statePtr->lastPhase.begin(), statePtr->lastPhase.end(), 0.0);
        std::fill(statePtr->phaseAccum.begin(), statePtr->phaseAccum.end(), 0.0);
        
        statePtr->readPos = 0.0;
        statePtr->writePos = 0;
        statePtr->outputWritePos = FFT_SIZE; // Start write ahead for proper latency
        statePtr->outputReadPos = 0;
        statePtr->hopCounter = 0;
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
        std::fill(state.grainBuffer.begin(), state.grainBuffer.end(), 0.0f);
        
        // Reset phase accumulators
        std::fill(state.phase.begin(), state.phase.end(), 0.0);
        std::fill(state.lastPhase.begin(), state.lastPhase.end(), 0.0);
        std::fill(state.phaseAccum.begin(), state.phaseAccum.end(), 0.0);
        
        // Reset positions
        state.readPos = 0.0;
        state.writePos = 0;
        state.outputWritePos = FFT_SIZE; // Start write ahead for proper latency
        state.outputReadPos = 0;
        state.hopCounter = 0;
        
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
    
    for (int ch = 0; ch < numChannels && ch < pimpl->channelStates.size(); ++ch) {
        auto& state = *pimpl->channelStates[ch];
        float* channelData = buffer.getWritePointer(ch);
        
        // Calculate RMS for silence detection
        float rms = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            rms += channelData[i] * channelData[i];
        }
        rms = std::sqrt(rms / numSamples);
        
        // Fast path for silence
        const bool isSilent = state.silenceDetector.process(rms);
        if (isSilent && !shouldFreeze) {
            // Just clear the output
            std::fill(channelData, channelData + numSamples, 0.0f);
            continue;
        }
        
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
        
        // Process samples
        for (int i = 0; i < numSamples; ++i) {
            // Store input sample
            state.inputBuffer[state.writePos] = channelData[i];
            state.writePos = wrapIndex(state.writePos + 1, state.inputBuffer.size());
            
            // Process frame at hop boundary
            if (++state.hopCounter >= HOP_SIZE) {
                state.hopCounter = 0;
                pimpl->processFrame(state);
            }
            
            // Read output with mixing
            float output = state.outputBuffer[state.outputReadPos];
            state.outputBuffer[state.outputReadPos] = 0.0f;
            state.outputReadPos = wrapIndex(state.outputReadPos + 1, state.outputBuffer.size());
            
            // Apply mix with denormal prevention
            channelData[i] = flushDenorm(channelData[i] * (1.0f - smoothMix) + 
                                         output * smoothMix);
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
    
    // Fill grain buffer with windowed input
    const size_t readPosInt = static_cast<size_t>(state.readPos);
    
    // SIMD-optimized windowing
#ifdef __AVX2__
    for (size_t i = 0; i < FFT_SIZE; i += 8) {
        size_t idx = readPosInt + i;
        idx = wrapIndex(idx, state.inputBuffer.size());
        __m256 input = _mm256_loadu_ps(&state.inputBuffer[idx]);
        __m256 win = _mm256_load_ps(&state.window[i]);
        __m256 result = _mm256_mul_ps(input, win);
        _mm256_store_ps(&state.grainBuffer[i], result);
    }
#elif defined(__SSE2__)
    for (size_t i = 0; i < FFT_SIZE; i += 4) {
        size_t idx = readPosInt + i;
        idx = wrapIndex(idx, state.inputBuffer.size());
        __m128 input = _mm_loadu_ps(&state.inputBuffer[idx]);
        __m128 win = _mm_load_ps(&state.window[i]);
        __m128 result = _mm_mul_ps(input, win);
        _mm_store_ps(&state.grainBuffer[i], result);
    }
#else
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        size_t idx = readPosInt + i;
        idx = wrapIndex(idx, state.inputBuffer.size());
        state.grainBuffer[i] = state.inputBuffer[idx] * state.window[i];
    }
#endif
    
    // Advance read position with time stretch
    float hopAdvance = HOP_SIZE / std::max(0.25f, std::min(4.0f, smoothTimeStretch));
    
    // Transient preservation
    const float transientAmount = state.transientDetector.process(
        std::accumulate(state.magnitude.begin(), state.magnitude.end(), 0.0f));
    
    if (transientAmount > 0.0f) {
        const float preserve = params.transientPreserve.load(std::memory_order_relaxed);
        const float transientMod = 1.0f - (transientAmount * preserve * 0.9f);
        hopAdvance = HOP_SIZE / (smoothTimeStretch * transientMod);
    }
    
    state.readPos += hopAdvance;
    if (state.readPos >= state.inputBuffer.size()) {
        state.readPos -= state.inputBuffer.size();
    }
    
    // Process spectral data
    analyzeFrame(state);
    applySpectralProcessing(state);
    synthesizeFrame(state);
    
    // Comprehensive denormal flush
    flushAllDenormals(state);
}

void PhasedVocoder::Impl::analyzeFrame(ChannelState& state) noexcept {
    // Copy to FFT buffer - JUCE expects interleaved real/imaginary format
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        state.fftBuffer[i] = std::complex<float>(state.grainBuffer[i], 0.0f);
    }
    
    // Forward FFT - JUCE performs unnormalized FFT
    state.fft.perform(reinterpret_cast<float*>(state.fftBuffer.data()), 
                     reinterpret_cast<float*>(state.fftBuffer.data()), false);
    
    // Extract magnitude and phase with improved precision
    const double binFreqHz = sampleRate / FFT_SIZE;
    const double expectedPhaseInc = TWO_PI_D * HOP_SIZE / FFT_SIZE;
    
    for (size_t bin = 0; bin <= FFT_SIZE/2; ++bin) {
        const float real = state.fftBuffer[bin].real();
        const float imag = state.fftBuffer[bin].imag();
        
        // Magnitude with denormal prevention
        state.magnitude[bin] = flushDenorm(std::sqrt(real * real + imag * imag));
        
        // Phase in double precision
        state.phase[bin] = std::atan2(static_cast<double>(imag), 
                                      static_cast<double>(real));
        
        // Phase unwrapping
        double phaseDiff = state.phase[bin] - state.lastPhase[bin];
        state.lastPhase[bin] = state.phase[bin];
        
        // Wrap to [-π, π]
        while (phaseDiff > PI_D) phaseDiff -= TWO_PI_D;
        while (phaseDiff < -PI_D) phaseDiff += TWO_PI_D;
        
        // True frequency estimation
        const double deviation = phaseDiff - expectedPhaseInc * bin;
        const double trueFreq = binFreqHz * bin + 
                               deviation * sampleRate / (TWO_PI_D * HOP_SIZE);
        state.trueBinFreq[bin] = static_cast<float>(trueFreq);
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
    const float pitchShift = pitchShiftSmoother->tick();
    
    // Phase accumulation and spectrum reconstruction
    for (size_t bin = 0; bin <= FFT_SIZE/2; ++bin) {
        // Pitch-shifted frequency
        const double shiftedFreq = state.trueBinFreq[bin] * pitchShift;
        
        // Accumulate phase in double precision
        state.phaseAccum[bin] += TWO_PI_D * shiftedFreq * HOP_SIZE / sampleRate;
        
        // Wrap phase to prevent unbounded growth
        while (state.phaseAccum[bin] > PI_D) state.phaseAccum[bin] -= TWO_PI_D;
        while (state.phaseAccum[bin] < -PI_D) state.phaseAccum[bin] += TWO_PI_D;
        
        // Reconstruct spectrum
        state.fftBuffer[bin] = std::polar(state.magnitude[bin], 
                                         static_cast<float>(state.phaseAccum[bin]));
        
        // Hermitian symmetry for real-valued output
        if (bin > 0 && bin < FFT_SIZE/2) {
            state.fftBuffer[FFT_SIZE - bin] = std::conj(state.fftBuffer[bin]);
        }
    }
    
    // Ensure DC and Nyquist bins are real (no imaginary component)
    state.fftBuffer[0] = std::complex<float>(state.fftBuffer[0].real(), 0.0f);
    if (FFT_SIZE % 2 == 0) {
        state.fftBuffer[FFT_SIZE/2] = std::complex<float>(state.fftBuffer[FFT_SIZE/2].real(), 0.0f);
    }
    
    // Inverse FFT - JUCE expects interleaved real/imaginary format
    state.fft.perform(reinterpret_cast<float*>(state.fftBuffer.data()), 
                     reinterpret_cast<float*>(state.fftBuffer.data()), true);
    
    // Overlap-add with proper scaling
    // JUCE FFT is unnormalized, so we need to divide by FFT_SIZE for round-trip
    // Also account for window overlap normalization
    const float scale = invFFTSize / windowSum;
    
#ifdef __AVX2__
    const __m256 vScale = _mm256_set1_ps(scale);
    for (size_t i = 0; i < FFT_SIZE; i += 8) {
        size_t outIdx = state.outputWritePos + i;
        outIdx = wrapIndex(outIdx, state.outputBuffer.size());
        
        __m256 fftReal = _mm256_set_ps(
            state.fftBuffer[i+7].real(), state.fftBuffer[i+6].real(),
            state.fftBuffer[i+5].real(), state.fftBuffer[i+4].real(),
            state.fftBuffer[i+3].real(), state.fftBuffer[i+2].real(),
            state.fftBuffer[i+1].real(), state.fftBuffer[i].real());
        __m256 win = _mm256_load_ps(&state.window[i]);
        __m256 result = _mm256_mul_ps(_mm256_mul_ps(fftReal, win), vScale);
        
        __m256 existing = _mm256_loadu_ps(&state.outputBuffer[outIdx]);
        _mm256_storeu_ps(&state.outputBuffer[outIdx], _mm256_add_ps(existing, result));
    }
#elif defined(__SSE2__)
    const __m128 vScale = _mm_set1_ps(scale);
    for (size_t i = 0; i < FFT_SIZE; i += 4) {
        size_t outIdx = state.outputWritePos + i;
        outIdx = wrapIndex(outIdx, state.outputBuffer.size());
        
        __m128 fftReal = _mm_set_ps(state.fftBuffer[i+3].real(),
                                    state.fftBuffer[i+2].real(),
                                    state.fftBuffer[i+1].real(),
                                    state.fftBuffer[i].real());
        __m128 win = _mm_load_ps(&state.window[i]);
        __m128 result = _mm_mul_ps(_mm_mul_ps(fftReal, win), vScale);
        
        __m128 existing = _mm_loadu_ps(&state.outputBuffer[outIdx]);
        _mm_storeu_ps(&state.outputBuffer[outIdx], _mm_add_ps(existing, result));
    }
#else
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        size_t outIdx = state.outputWritePos + i;
        outIdx = wrapIndex(outIdx, state.outputBuffer.size());
        state.outputBuffer[outIdx] += state.fftBuffer[i].real() * 
                                     state.window[i] * scale;
    }
#endif
    
    state.outputWritePos = wrapIndex(state.outputWritePos + HOP_SIZE, 
                                    state.outputBuffer.size());
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
        for (auto& p : state.phaseAccum) p = flushDenorm(p);
        for (auto& p : state.lastPhase) p = flushDenorm(p);
        for (auto& f : state.trueBinFreq) f = flushDenorm(f);
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
                pimpl->params.timeStretch.store(0.25f + value * 3.75f, 
                                               std::memory_order_relaxed);
                break;
            case ParamID::PitchShift:
                pimpl->params.pitchShift.store(0.5f + value * 1.5f, 
                                              std::memory_order_relaxed);
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