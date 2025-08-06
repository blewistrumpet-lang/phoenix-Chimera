#include "SpectralGate_Platinum.h"
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <vector>
#include <array>

// Platform-specific includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE2 1
#else
    #define HAS_SSE2 0
#endif

// Platform-specific denormal prevention
#if HAS_SSE2
static struct DenormGuard {
    DenormGuard() {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    }
} s_denormGuard;
#endif

namespace {
    // Constants for professional quality
    constexpr int kFFTSize = 2048;
    constexpr int kFFTBins = kFFTSize / 2 + 1;
    constexpr int kOverlap = 4;  // 75% overlap
    constexpr int kHopSize = kFFTSize / kOverlap;
    constexpr float kSilenceThreshold = 1e-6f;
    constexpr double kPI = 3.14159265358979323846;
    
    // Inline denormal flushing
    template<typename T>
    inline T flushDenorm(T v) noexcept {
        #if HAS_SSE2
            if constexpr (std::is_same_v<T, float>) {
                return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(v), _mm_set_ss(0.0f)));
            }
        #endif
        constexpr T tiny = static_cast<T>(1.0e-38);
        return std::fabs(v) < tiny ? static_cast<T>(0) : v;
    }
    
    // Fast approximation for envelope detection
    inline float fastExp(float x) noexcept {
        x = 1.0f + x / 256.0f;
        x *= x; x *= x; x *= x; x *= x;
        x *= x; x *= x; x *= x; x *= x;
        return x;
    }
}

// Parameter smoother for click-free automation
class ParamSmoother {
public:
    void setSampleRate(double sr) noexcept {
        m_sampleRate = sr;
        updateCoeff();
    }
    
    void setTime(float ms) noexcept {
        m_timeMs = std::max(0.001f, ms);
        updateCoeff();
    }
    
    void setValue(float v) noexcept {
        m_target.store(v, std::memory_order_relaxed);
    }
    
    void snap(float v) noexcept {
        m_current = m_target = v;
    }
    
    inline float tick() noexcept {
        float t = m_target.load(std::memory_order_relaxed);
        m_current += m_coeff * (t - m_current);
        return flushDenorm(m_current);
    }
    
    float getCurrent() const noexcept { return m_current; }
    
private:
    void updateCoeff() noexcept {
        if (m_sampleRate > 0 && m_timeMs > 0) {
            m_coeff = 1.0f - std::exp(-1000.0f / (m_timeMs * m_sampleRate));
            m_coeff = std::min(1.0f, std::max(0.0f, m_coeff));
        }
    }
    
    std::atomic<float> m_target{0.0f};
    float m_current{0.0f};
    float m_coeff{0.01f};
    float m_timeMs{5.0f};
    double m_sampleRate{44100.0};
};

// Spectral bin processor with per-frequency gating (double precision state)
class SpectralBinProcessor {
public:
    void prepare(double sampleRate) noexcept {
        m_sampleRate = sampleRate;
        m_binFreqHz = sampleRate / kFFTSize;
        
        // Calculate gate smoothing coefficient (5ms default)
        m_gateTimeMs = 5.0f;
        m_gateCoeff = std::exp(-1.0f / (m_gateTimeMs * sampleRate * 0.001f));
        
        reset();
    }
    
    void setGateSmoothTime(float ms) noexcept {
        m_gateTimeMs = std::max(0.1f, ms);
        m_gateCoeff = std::exp(-1.0f / (m_gateTimeMs * m_sampleRate * 0.001f));
    }
    
    void reset() noexcept {
        std::fill(m_binEnvelopes.begin(), m_binEnvelopes.end(), 0.0);
        std::fill(m_binGates.begin(), m_binGates.end(), 0.0f);
    }
    
    void processSpectrum(float* real, float* imag, 
                        float threshold, float ratio,
                        float attack, float release,
                        float freqLow, float freqHigh) noexcept {
        
        // Convert freq range to bin indices
        int binLow = std::max(1, static_cast<int>(freqLow / m_binFreqHz));
        int binHigh = std::min(kFFTBins - 1, static_cast<int>(freqHigh / m_binFreqHz));
        
        // Calculate attack/release coefficients
        double attackCoeff = std::exp(-1.0 / (attack * m_sampleRate * 0.001));
        double releaseCoeff = std::exp(-1.0 / (release * m_sampleRate * 0.001));
        
        // Process each bin with double precision envelopes
        for (int bin = 0; bin < kFFTBins; ++bin) {
            double mag = std::sqrt(real[bin] * real[bin] + imag[bin] * imag[bin]);
            mag = flushDenorm(mag);
            
            // Envelope follower with asymmetric attack/release (double precision)
            double& env = m_binEnvelopes[bin];
            double coeff = (mag > env) ? attackCoeff : releaseCoeff;
            env = mag + coeff * (env - mag);
            env = flushDenorm(env);
            
            // Gate calculation
            float gate = 1.0f;
            if (bin >= binLow && bin <= binHigh) {
                if (env < threshold) {
                    gate = 0.0f;
                } else {
                    float excess = static_cast<float>(env - threshold);
                    gate = threshold + excess / ratio;
                    gate = std::min(1.0f, gate / static_cast<float>(env));
                }
            }
            
            // Sample-rate aware gate smoothing
            float& prevGate = m_binGates[bin];
            prevGate = prevGate * m_gateCoeff + gate * (1.0f - m_gateCoeff);
            prevGate = flushDenorm(prevGate);
            
            // Apply gating
            real[bin] *= prevGate;
            imag[bin] *= prevGate;
        }
    }
    
private:
    double m_sampleRate{44100.0};
    double m_binFreqHz{21.53};
    float m_gateTimeMs{5.0f};
    float m_gateCoeff{0.99f};
    std::array<double, kFFTBins> m_binEnvelopes;  // Double precision for stability
    std::array<float, kFFTBins> m_binGates;
};

// Implementation structure with all DSP components
struct SpectralGate_Platinum::Impl {
    // Core DSP state
    double sampleRate{44100.0};
    int blockSize{512};
    
    // FFT components (using JUCE's FFT for compatibility)
    juce::dsp::FFT fft{static_cast<int>(std::log2(kFFTSize))};
    
    // Buffers aligned for SIMD
    alignas(32) std::array<float, kFFTSize> fftBuffer;
    alignas(32) std::array<float, kFFTSize> window;
    alignas(32) std::array<float, kFFTSize> inputBuffer;
    alignas(32) std::array<float, kFFTSize> outputBuffer;
    
    // Overlap-add state per channel
    struct ChannelState {
        alignas(32) std::array<float, kFFTSize> overlapBuffer;
        int writePos{0};
        int readPos{0};
        float inputGain{1.0f};
        float outputGain{1.0f};
    };
    std::vector<ChannelState> channels;
    
    // Lookahead delay line
    struct DelayLine {
        std::vector<float> buffer;
        int writePos{0};
        int size{0};
        
        void prepare(int samples) {
            size = samples;
            buffer.resize(size, 0.0f);
            writePos = 0;
        }
        
        void reset() {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }
        
        float processSample(float in) noexcept {
            if (size == 0) return in;
            float out = buffer[writePos];
            buffer[writePos] = in;
            writePos = (writePos + 1) % size;
            return out;
        }
    };
    std::vector<DelayLine> lookaheadDelays;
    
    // Spectral processor
    SpectralBinProcessor spectralProcessor;
    
    // Window normalization factor for perfect reconstruction
    float windowNormFactor{1.0f};
    
    // Parameter smoothers (using double precision for accumulation)
    ParamSmoother thresholdSmooth;
    ParamSmoother ratioSmooth;
    ParamSmoother attackSmooth;
    ParamSmoother releaseSmooth;
    ParamSmoother freqLowSmooth;
    ParamSmoother freqHighSmooth;
    ParamSmoother lookaheadSmooth;
    ParamSmoother mixSmooth;
    ParamSmoother gateSmooth;  // For per-bin gate smoothing
    
    // Silence detection
    float silenceCounter{0};
    bool isSilent{false};
    
    // Constructor
    Impl() {
        initWindow();
        setupSmoothers();
    }
    
    void initWindow() {
        // Hann window for smooth overlap-add
        for (int i = 0; i < kFFTSize; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * kPI * i / (kFFTSize - 1)));
        }
        
        // Calculate exact normalization factor for perfect reconstruction
        double sum = 0.0;
        for (int i = 0; i < kFFTSize; i += kHopSize) {
            sum += window[i];
        }
        windowNormFactor = static_cast<float>(1.0 / sum);
    }
    
    void setupSmoothers() {
        thresholdSmooth.setTime(5.0f);
        ratioSmooth.setTime(5.0f);
        attackSmooth.setTime(5.0f);
        releaseSmooth.setTime(5.0f);
        freqLowSmooth.setTime(10.0f);
        freqHighSmooth.setTime(10.0f);
        lookaheadSmooth.setTime(20.0f);
        mixSmooth.setTime(10.0f);
        gateSmooth.setTime(5.0f);  // 5ms gate smoothing
    }
    
    void prepare(double sr, int bs) {
        sampleRate = sr;
        blockSize = bs;
        
        // Update smoothers
        thresholdSmooth.setSampleRate(sr);
        ratioSmooth.setSampleRate(sr);
        attackSmooth.setSampleRate(sr);
        releaseSmooth.setSampleRate(sr);
        freqLowSmooth.setSampleRate(sr);
        freqHighSmooth.setSampleRate(sr);
        lookaheadSmooth.setSampleRate(sr);
        mixSmooth.setSampleRate(sr);
        
        // Prepare spectral processor
        spectralProcessor.prepare(sr);
        
        // Clear buffers
        reset();
    }
    
    void reset() {
        std::fill(fftBuffer.begin(), fftBuffer.end(), 0.0f);
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        
        for (auto& ch : channels) {
            std::fill(ch.overlapBuffer.begin(), ch.overlapBuffer.end(), 0.0f);
            ch.writePos = 0;
            ch.readPos = 0;
        }
        
        for (auto& delay : lookaheadDelays) {
            delay.reset();
        }
        
        spectralProcessor.reset();
        silenceCounter = 0;
        isSilent = false;
    }
    
    void processChannel(float* data, int numSamples, int chIdx) {
        auto& ch = channels[chIdx];
        auto& delay = lookaheadDelays[chIdx];
        
        for (int s = 0; s < numSamples; ++s) {
            float input = data[s];
            
            // Apply lookahead delay
            float delayed = delay.processSample(input);
            
            // Fill input buffer
            inputBuffer[ch.writePos] = delayed * ch.inputGain;
            ch.writePos = (ch.writePos + 1) % kFFTSize;
            
            // Process when we have enough samples
            if (ch.writePos % kHopSize == 0) {
                processFrame(chIdx);
            }
            
            // Read from output buffer
            float output = outputBuffer[ch.readPos] * ch.outputGain;
            ch.readPos = (ch.readPos + 1) % kFFTSize;
            
            // Mix dry/wet
            float mix = mixSmooth.tick();
            data[s] = input * (1.0f - mix) + output * mix;
            data[s] = flushDenorm(data[s]);
        }
    }
    
    void processFrame(int chIdx) {
        auto& ch = channels[chIdx];
        
        // Prefetch next cache line for better memory access
        #if HAS_SSE2
        _mm_prefetch(reinterpret_cast<const char*>(&inputBuffer[(ch.writePos + 64) % kFFTSize]), _MM_HINT_T0);
        #endif
        
        // Copy input to FFT buffer with windowing
        int readStart = (ch.writePos - kFFTSize + kFFTSize) % kFFTSize;
        for (int i = 0; i < kFFTSize; ++i) {
            int idx = (readStart + i) % kFFTSize;
            fftBuffer[i] = inputBuffer[idx] * window[i];
        }
        
        // Forward FFT
        fft.performRealOnlyForwardTransform(fftBuffer.data());
        
        // Extract real and imaginary parts
        alignas(32) float real[kFFTBins];
        alignas(32) float imag[kFFTBins];
        
        for (int i = 0; i < kFFTBins; ++i) {
            real[i] = fftBuffer[i * 2];
            imag[i] = (i > 0 && i < kFFTBins - 1) ? fftBuffer[i * 2 + 1] : 0.0f;
        }
        
        // Apply spectral gating
        float threshold = thresholdSmooth.tick();
        float ratio = ratioSmooth.tick();
        float attack = attackSmooth.tick();
        float release = releaseSmooth.tick();
        float freqLow = freqLowSmooth.tick();
        float freqHigh = freqHighSmooth.tick();
        
        // Update gate smoothing time if needed
        float gateSmoothMs = gateSmooth.tick();
        spectralProcessor.setGateSmoothTime(gateSmoothMs);
        
        spectralProcessor.processSpectrum(real, imag, 
                                         threshold, ratio,
                                         attack, release,
                                         freqLow, freqHigh);
        
        // Pack back for inverse FFT
        for (int i = 0; i < kFFTBins; ++i) {
            fftBuffer[i * 2] = real[i];
            if (i > 0 && i < kFFTBins - 1) {
                fftBuffer[i * 2 + 1] = imag[i];
            }
        }
        
        // Inverse FFT
        fft.performRealOnlyInverseTransform(fftBuffer.data());
        
        // Overlap-add with exact windowing normalization
        for (int i = 0; i < kFFTSize; ++i) {
            int outIdx = (ch.readPos + i) % kFFTSize;
            float windowed = fftBuffer[i] * window[i] * windowNormFactor / kFFTSize;
            
            if (i < kFFTSize - kHopSize) {
                ch.overlapBuffer[i] += windowed;
                outputBuffer[outIdx] = ch.overlapBuffer[i];
                ch.overlapBuffer[i] = ch.overlapBuffer[i + kHopSize];
            } else {
                ch.overlapBuffer[i - kFFTSize + kHopSize] = windowed;
            }
        }
    }
    
    bool detectSilence(const juce::AudioBuffer<float>& buffer) {
        float rms = 0.0f;
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int s = 0; s < numSamples; ++s) {
                rms += data[s] * data[s];
            }
        }
        
        rms = std::sqrt(rms / (numChannels * numSamples));
        
        if (rms < kSilenceThreshold) {
            silenceCounter += numSamples;
            if (silenceCounter > sampleRate * 0.1) { // 100ms of silence
                return true;
            }
        } else {
            silenceCounter = 0;
        }
        
        return false;
    }
};

// Public interface implementation
SpectralGate_Platinum::SpectralGate_Platinum() : pImpl(std::make_unique<Impl>()) {}

SpectralGate_Platinum::~SpectralGate_Platinum() = default;

void SpectralGate_Platinum::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pImpl->prepare(sampleRate, samplesPerBlock);
    
    // Prepare channel states
    int numChannels = 2; // Assume stereo, adjust as needed
    pImpl->channels.resize(numChannels);
    pImpl->lookaheadDelays.resize(numChannels);
    
    // Setup lookahead delays (5ms default)
    int lookaheadSamples = static_cast<int>(0.005 * sampleRate);
    for (auto& delay : pImpl->lookaheadDelays) {
        delay.prepare(lookaheadSamples);
    }
    
    // Initialize default parameters (converted to dB/Hz values)
    pImpl->thresholdSmooth.snap(0.01f);    // Linear threshold
    pImpl->ratioSmooth.snap(4.0f);         // 4:1
    pImpl->attackSmooth.snap(10.0f);       // ms
    pImpl->releaseSmooth.snap(100.0f);     // ms
    pImpl->freqLowSmooth.snap(20.0f);      // Hz
    pImpl->freqHighSmooth.snap(20000.0f);  // Hz
    pImpl->lookaheadSmooth.snap(5.0f);     // ms
    pImpl->mixSmooth.snap(1.0f);           // 100% wet
}

void SpectralGate_Platinum::process(juce::AudioBuffer<float>& buffer) {
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    // Ensure we have the right number of channels
    if (numChannels != static_cast<int>(pImpl->channels.size())) {
        pImpl->channels.resize(numChannels);
        pImpl->lookaheadDelays.resize(numChannels);
        for (int i = 0; i < numChannels; ++i) {
            int lookaheadSamples = static_cast<int>(
                pImpl->lookaheadSmooth.getCurrent() * 0.001 * pImpl->sampleRate
            );
            pImpl->lookaheadDelays[i].prepare(lookaheadSamples);
        }
    }
    
    // Fast path for silence
    pImpl->isSilent = pImpl->detectSilence(buffer);
    if (pImpl->isSilent) {
        buffer.clear();
        return;
    }
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        pImpl->processChannel(data, numSamples, ch);
    }
}

void SpectralGate_Platinum::reset() {
    pImpl->reset();
}

void SpectralGate_Platinum::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (id) {
            case kThreshold:
                // Convert from 0-1 to linear threshold
                {
                    float db = -80.0f + value * 80.0f;  // -80 to 0 dB
                    float linear = std::pow(10.0f, db / 20.0f);
                    pImpl->thresholdSmooth.setValue(linear);
                }
                break;
            case kRatio:
                // Convert from 0-1 to 1:1 to 100:1
                pImpl->ratioSmooth.setValue(1.0f + value * 99.0f);
                break;
            case kAttack:
                // Convert from 0-1 to 0.1-100 ms (log scale)
                pImpl->attackSmooth.setValue(0.1f * std::pow(1000.0f, value));
                break;
            case kRelease:
                // Convert from 0-1 to 1-1000 ms (log scale)
                pImpl->releaseSmooth.setValue(1.0f * std::pow(1000.0f, value));
                break;
            case kFreqLow:
                // Convert from 0-1 to 20-2000 Hz (log scale)
                pImpl->freqLowSmooth.setValue(20.0f * std::pow(100.0f, value));
                break;
            case kFreqHigh:
                // Convert from 0-1 to 200-20000 Hz (log scale)
                pImpl->freqHighSmooth.setValue(200.0f * std::pow(100.0f, value));
                break;
            case kLookahead:
                // Convert from 0-1 to 0-10 ms
                {
                    float ms = value * 10.0f;
                    pImpl->lookaheadSmooth.setValue(ms);
                    int samples = static_cast<int>(ms * 0.001 * pImpl->sampleRate);
                    for (auto& delay : pImpl->lookaheadDelays) {
                        delay.prepare(samples);
                    }
                }
                break;
            case kMix:
                // Direct 0-1 for dry/wet mix
                pImpl->mixSmooth.setValue(value);
                break;
        }
    }
}

juce::String SpectralGate_Platinum::getParameterName(int index) const {
    switch (index) {
        case kThreshold: return "Threshold";
        case kRatio:     return "Ratio";
        case kAttack:    return "Attack";
        case kRelease:   return "Release";
        case kFreqLow:   return "Freq Low";
        case kFreqHigh:  return "Freq High";
        case kLookahead: return "Lookahead";
        case kMix:       return "Mix";
        default: return "Unknown";
    }
}