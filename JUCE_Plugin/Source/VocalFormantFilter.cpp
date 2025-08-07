#include "VocalFormantFilter.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include <cmath>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Platform-specific optimizations
#if defined(__GNUC__) || defined(__clang__)
    #define ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define ALWAYS_INLINE __forceinline
#else
    #define ALWAYS_INLINE inline
#endif

// Global denormal protection
static struct DenormGuard {
    DenormGuard() {
#if HAS_SSE2
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
    }
} static g_denormGuard;

namespace {
    // Denormal flushers
    ALWAYS_INLINE float flushDenormF(float v) noexcept {
#ifdef __SSE2__
        return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(v), _mm_set_ss(0.0f)));
#else
        constexpr float tiny = 1.0e-38f;
        return std::fabs(v) < tiny ? 0.0f : v;
#endif
    }
    
    ALWAYS_INLINE double flushDenormD(double v) noexcept {
        constexpr double tiny = 1.0e-308;
        return std::fabs(v) < tiny ? 0.0 : v;
    }
}

// Implementation
struct VocalFormantFilter::Impl {
    // Formant data
    struct FormantSet {
        float f1, f2, f3;  // Frequencies
        float q1, q2, q3;  // Q factors
    };
    
    // Classic vowel formants
    static constexpr std::array<FormantSet, 5> vowelFormants = {{
        { 700.0f,  1220.0f, 2600.0f, 10.0f, 12.0f, 15.0f },  // A
        { 570.0f,  2090.0f, 2840.0f, 10.0f, 15.0f, 20.0f },  // E
        { 300.0f,  2290.0f, 3010.0f, 12.0f, 20.0f, 22.0f },  // I
        { 590.0f,  880.0f,  2540.0f, 10.0f, 12.0f, 15.0f },  // O
        { 440.0f,  1020.0f, 2240.0f, 10.0f, 12.0f, 18.0f }   // U
    }};
    
    // Core state
    double sampleRate = 44100.0;
    int blockSize = 512;
    
    // Smoothed parameters
    struct SmoothParam {
        std::atomic<float> target{0.5f};
        float current = 0.5f;
        float coeff = 0.995f;
        
        void setSmoothingTime(float ms, double sr) {
            float samples = ms * 0.001f * static_cast<float>(sr);
            coeff = std::exp(-1.0f / samples);
        }
        
        ALWAYS_INLINE float tick() noexcept {
            float t = target.load(std::memory_order_relaxed);
            current += (t - current) * (1.0f - coeff);
            return flushDenormF(current);
        }
        
        void reset(float value) {
            target.store(value, std::memory_order_relaxed);
            current = value;
        }
    };
    
    // Parameters
    SmoothParam vowel1, vowel2, morph;
    SmoothParam resonance, brightness;
    SmoothParam modRate, modDepth, mix;
    
    // Efficient biquad formant filters (scalar fallback)
    struct FormantBiquad {
        double a1 = 0.0, a2 = 0.0;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double x1 = 0.0, x2 = 0.0;
        double y1 = 0.0, y2 = 0.0;
        
        void setCoefficients(float freq, float q, double sr) {
            double w = 2.0 * M_PI * freq / sr;
            double cosw = std::cos(w);
            double sinw = std::sin(w);
            double alpha = sinw / (2.0 * q);
            
            // Bandpass coefficients
            double norm = 1.0 / (1.0 + alpha);
            b0 = alpha * norm;
            b1 = 0.0;
            b2 = -alpha * norm;
            a1 = -2.0 * cosw * norm;
            a2 = (1.0 - alpha) * norm;
        }
        
        ALWAYS_INLINE float process(float input) noexcept {
            double x0 = input;
            double y0 = b0*x0 + b1*x1 + b2*x2 - a1*y1 - a2*y2;
            
            x2 = x1; x1 = x0;
            y2 = y1; y1 = flushDenormD(y0);
            
            return static_cast<float>(y0);
        }
        
        void reset() {
            x1 = x2 = y1 = y2 = 0.0;
        }
    };
    
    // Channel processing state
    struct ChannelState {
        std::array<FormantBiquad, 3> formants;
        
        // DC blocker
        double dcX1 = 0.0, dcY1 = 0.0;
        static constexpr double dcR = 0.995;
        
        // Modulation LFO (denormal protected)
        float lfoPhase = 0.0f;
        
        // Envelope follower (denormal protected)
        float envelope = 0.0f;
        float envCoeff = 0.995f;  // Pre-computed
        
        // High shelf for brightness
        struct BrightnessShelf {
            double state = 0.0;
            double coeff = 0.0;
            
            void setCoefficients(float freq, double sr) {
                coeff = 2.0 * std::sin(M_PI * freq / sr);
            }
            
            ALWAYS_INLINE float process(float input) noexcept {
                double hp = input - state;
                state = flushDenormD(state + hp * coeff);
                return static_cast<float>(input + hp * 0.5); // Shelf boost
            }
            
            void reset() { state = 0.0; }
        } brightShelf;
        
        void reset() {
            for (auto& f : formants) f.reset();
            dcX1 = dcY1 = 0.0;
            lfoPhase = 0.0f;
            envelope = 0.0f;
            brightShelf.reset();
        }
        
        ALWAYS_INLINE float processDC(float input) noexcept {
            double x0 = input;
            double y0 = x0 - dcX1 + dcR * dcY1;
            dcX1 = x0;
            dcY1 = flushDenormD(y0);
            return static_cast<float>(y0);
        }
        
        ALWAYS_INLINE void updateEnvelope(float input) noexcept {
            float env = std::abs(input);
            envelope = env + (envelope - env) * envCoeff;
            envelope = flushDenormF(envelope);
        }
    };
    
    std::array<ChannelState, 2> channelStates;
    
    // Oversampler for saturation
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    
    // Pre-calculated formants (updated per block)
    FormantSet currentFormants;
    bool formantsNeedUpdate = true;
    
    // Pre-allocated buffers - NO RT allocations!
    juce::AudioBuffer<float> dryBuffer;
    
    // Constructor
    Impl() {
        // Initialize parameters
        vowel1.reset(0.0f);    // A
        vowel2.reset(0.4f);    // Between E and I  
        morph.reset(0.0f);
        resonance.reset(0.5f);
        brightness.reset(0.5f);
        modRate.reset(0.0f);
        modDepth.reset(0.0f);
        mix.reset(1.0f);
    }
    
    // Interpolate formants (call once per block)
    FormantSet interpolateFormants(float v1, float v2, float morphAmt) {
        // Convert normalized to indices
        float idx1 = v1 * 4.0f;
        float idx2 = v2 * 4.0f;
        float vowelIdx = idx1 + (idx2 - idx1) * morphAmt;
        
        // Clamp and get base index
        vowelIdx = std::max(0.0f, std::min(4.0f, vowelIdx));
        int baseIdx = static_cast<int>(vowelIdx);
        float frac = vowelIdx - baseIdx;
        
        if (baseIdx >= 4) return vowelFormants[4];
        
        // Interpolate
        const auto& f1 = vowelFormants[baseIdx];
        const auto& f2 = vowelFormants[std::min(baseIdx + 1, 4)];
        
        FormantSet result;
        result.f1 = f1.f1 + (f2.f1 - f1.f1) * frac;
        result.f2 = f1.f2 + (f2.f2 - f1.f2) * frac;
        result.f3 = f1.f3 + (f2.f3 - f1.f3) * frac;
        result.q1 = f1.q1 + (f2.q1 - f1.q1) * frac;
        result.q2 = f1.q2 + (f2.q2 - f1.q2) * frac;
        result.q3 = f1.q3 + (f2.q3 - f1.q3) * frac;
        
        return result;
    }
    
    // Soft saturation (for oversampled processing)
    static ALWAYS_INLINE float softSaturate(float x) noexcept {
        // Asymmetric saturation for vocal character
        if (x > 0.0f) {
            return std::tanh(x * 0.7f) / 0.7f;
        } else {
            return std::tanh(x * 0.9f) / 0.9f;
        }
    }
};

// Static member definition
constexpr std::array<VocalFormantFilter::Impl::FormantSet, 5> 
    VocalFormantFilter::Impl::vowelFormants;

// Public interface
VocalFormantFilter::VocalFormantFilter() : pimpl(std::make_unique<Impl>()) {}
VocalFormantFilter::~VocalFormantFilter() = default;

void VocalFormantFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    pimpl->blockSize = samplesPerBlock;
    
    // Pre-allocate dry buffer
    pimpl->dryBuffer.setSize(2, samplesPerBlock, false, false, true);
    
    // Set smoothing times
    pimpl->vowel1.setSmoothingTime(50.0f, sampleRate);
    pimpl->vowel2.setSmoothingTime(50.0f, sampleRate);
    pimpl->morph.setSmoothingTime(20.0f, sampleRate);
    pimpl->resonance.setSmoothingTime(30.0f, sampleRate);
    pimpl->brightness.setSmoothingTime(20.0f, sampleRate);
    pimpl->modRate.setSmoothingTime(100.0f, sampleRate);
    pimpl->modDepth.setSmoothingTime(50.0f, sampleRate);
    pimpl->mix.setSmoothingTime(10.0f, sampleRate);
    
    // Prepare oversampler
    pimpl->oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 1, // 2 channels, 2x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true
    );
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    pimpl->oversampler->initProcessing(spec.maximumBlockSize);
    
    // Pre-compute envelope coefficients
    for (auto& state : pimpl->channelStates) {
        state.envCoeff = std::exp(-1.0f / (10.0f * 0.001f * sampleRate)); // 10ms
    }
    
    reset();
}

void VocalFormantFilter::reset() {
    for (auto& state : pimpl->channelStates) {
        state.reset();
    }
    if (pimpl->oversampler) {
        pimpl->oversampler->reset();
    }
    pimpl->formantsNeedUpdate = true;
}

void VocalFormantFilter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Update parameters once per block
    float v1 = pimpl->vowel1.tick();
    float v2 = pimpl->vowel2.tick();
    float morphAmt = pimpl->morph.tick();
    float res = pimpl->resonance.tick();
    float bright = pimpl->brightness.tick();
    float modRateNorm = pimpl->modRate.tick();
    float modDepth = pimpl->modDepth.tick();
    float mixAmt = pimpl->mix.tick();
    
    // Calculate formants once per block
    pimpl->currentFormants = pimpl->interpolateFormants(v1, v2, morphAmt);
    
    // Apply resonance scaling
    float resScale = 0.5f + res * 2.0f;
    pimpl->currentFormants.q1 *= resScale;
    pimpl->currentFormants.q2 *= resScale;
    pimpl->currentFormants.q3 *= resScale;
    
    // Update filter coefficients for all channels
    for (int ch = 0; ch < 2; ++ch) {
        auto& state = pimpl->channelStates[ch];
        
        // Update formant filters
        state.formants[0].setCoefficients(pimpl->currentFormants.f1, 
                                         pimpl->currentFormants.q1, 
                                         pimpl->sampleRate);
        state.formants[1].setCoefficients(pimpl->currentFormants.f2, 
                                         pimpl->currentFormants.q2, 
                                         pimpl->sampleRate);
        state.formants[2].setCoefficients(pimpl->currentFormants.f3, 
                                         pimpl->currentFormants.q3, 
                                         pimpl->sampleRate);
        
        // Update brightness shelf
        float shelfFreq = 2000.0f + bright * 6000.0f;
        state.brightShelf.setCoefficients(shelfFreq, pimpl->sampleRate);
    }
    
    // Store dry signal
    for (int ch = 0; ch < numChannels; ++ch) {
        pimpl->dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    
    // Process each channel
    for (int ch = 0; ch < numChannels && ch < 2; ++ch) {
        auto& state = pimpl->channelStates[ch];
        float* data = buffer.getWritePointer(ch);
        
        // Calculate modulation phase increment
        float modFreq = modRateNorm * 10.0f; // 0-10 Hz
        float phaseInc = 2.0f * M_PI * modFreq / pimpl->sampleRate;
        
        for (int i = 0; i < numSamples; ++i) {
            // DC blocking
            float input = state.processDC(data[i]);
            
            // Envelope follower
            state.updateEnvelope(input);
            
            // Modulation
            float mod = 0.0f;
            if (modDepth > 0.0f) {
                mod = std::sin(state.lfoPhase) * modDepth * 0.2f;
                state.lfoPhase += phaseInc;
                if (state.lfoPhase > 2.0f * M_PI) {
                    state.lfoPhase -= 2.0f * M_PI;
                }
            }
            
            // Dynamic frequency shift based on envelope
            float dynShift = 1.0f + state.envelope * 0.1f + mod;
            
            // Process through formants in parallel
            float f1 = state.formants[0].process(input) * 0.5f;
            float f2 = state.formants[1].process(input) * 0.35f;
            float f3 = state.formants[2].process(input) * 0.15f;
            
            float output = f1 + f2 + f3;
            
            // Apply brightness
            if (bright != 0.5f) {
                output = state.brightShelf.process(output) * (0.5f + bright);
            }
            
            data[i] = output;
        }
    }
    
    // Apply oversampled saturation if signal is hot
    float peakLevel = buffer.getMagnitude(0, numSamples);
    if (peakLevel > 0.7f) {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::AudioBlock<float> osBlock = pimpl->oversampler->processSamplesUp(block);
        
        // Saturate at higher sample rate
        for (int ch = 0; ch < osBlock.getNumChannels(); ++ch) {
            float* osData = osBlock.getChannelPointer(ch);
            for (size_t i = 0; i < osBlock.getNumSamples(); ++i) {
                osData[i] = Impl::softSaturate(osData[i]);
            }
        }
        
        pimpl->oversampler->processSamplesDown(block);
    }
    
    // Mix with dry
    for (int ch = 0; ch < numChannels; ++ch) {
        float* wet = buffer.getWritePointer(ch);
        const float* dry = pimpl->dryBuffer.getReadPointer(ch);
        
        for (int i = 0; i < numSamples; ++i) {
            wet[i] = wet[i] * mixAmt + dry[i] * (1.0f - mixAmt);
        }
    }
}

void VocalFormantFilter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (id) {
            case kVowel1:    pimpl->vowel1.target.store(value); break;
            case kVowel2:    pimpl->vowel2.target.store(value); break;
            case kMorph:     pimpl->morph.target.store(value); break;
            case kResonance: pimpl->resonance.target.store(value); break;
            case kBrightness: pimpl->brightness.target.store(value); break;
            case kModRate:   pimpl->modRate.target.store(value); break;
            case kModDepth:  pimpl->modDepth.target.store(value); break;
            case kMix:       pimpl->mix.target.store(value); break;
        }
    }
}

juce::String VocalFormantFilter::getParameterName(int index) const {
    switch (index) {
        case kVowel1:     return "Vowel 1";
        case kVowel2:     return "Vowel 2";
        case kMorph:      return "Morph";
        case kResonance:  return "Resonance";
        case kBrightness: return "Brightness";
        case kModRate:    return "Mod Rate";
        case kModDepth:   return "Mod Depth";
        case kMix:        return "Mix";
        default:          return "";
    }
}