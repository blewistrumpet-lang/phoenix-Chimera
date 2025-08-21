#include "PitchShifter.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <array>
#include <algorithm>
#include <iostream>

// DEBUG VERSION WITH LOGGING

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

// Main implementation
struct PitchShifter::Impl {
    static constexpr int FFT_ORDER = 12;  // 2^12 = 4096
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int OVERLAP_FACTOR = 4;
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;
    static constexpr int MAX_CHANNELS = 8;
    
    // Debug counters
    int processCounter{0};
    int frameCounter{0};
    bool debugLogged{false};
    
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
    struct alignas(64) ChannelState {
        alignas(32) std::array<float, FFT_SIZE * 2> inputRing{};
        alignas(32) std::array<float, FFT_SIZE * 2> outputRing{};
        alignas(32) std::array<std::complex<float>, FFT_SIZE> spectrum{};
        alignas(32) std::array<float, FFT_SIZE> frameBuffer{};
        
        alignas(32) std::array<double, FFT_SIZE/2 + 1> phaseLast{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> phaseSum{};
        
        alignas(32) std::array<float, FFT_SIZE/2 + 1> magnitude{};
        alignas(32) std::array<float, FFT_SIZE/2 + 1> frequency{};
        
        std::array<float, 8192> feedbackBuffer{};
        
        alignas(32) std::array<float, FFT_SIZE> analysisWindow{};
        alignas(32) std::array<float, FFT_SIZE> synthesisWindow{};
        
        int inputWriteIdx{0};
        int inputReadIdx{0};
        int outputWriteIdx{0};
        int outputReadIdx{0};
        int feedbackWritePos{0};
        int feedbackReadPos{0};
        int hopCounter{0};
        
        std::unique_ptr<juce::dsp::FFT> fft;
        
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
        
        ALWAYS_INLINE void writeSample(float sample) noexcept {
            inputRing[inputWriteIdx] = sample;
            inputWriteIdx = (inputWriteIdx + 1) & (FFT_SIZE * 2 - 1);
        }
        
        ALWAYS_INLINE float readOutput() noexcept {
            float out = outputRing[outputReadIdx];
            outputRing[outputReadIdx] = 0.0f;
            outputReadIdx = (outputReadIdx + 1) & (FFT_SIZE * 2 - 1);
            return out;
        }
        
        ALWAYS_INLINE void gatherFrame() noexcept {
            int idx = inputReadIdx;
            for (int i = 0; i < FFT_SIZE; ++i) {
                frameBuffer[i] = inputRing[idx];
                idx = (idx + 1) & (FFT_SIZE * 2 - 1);
            }
            inputReadIdx = (inputReadIdx + HOP_SIZE) & (FFT_SIZE * 2 - 1);
        }
        
        ALWAYS_INLINE void scatterFrame(const std::complex<float>* fftOut, float scale) noexcept {
            int idx = outputWriteIdx;
            for (int i = 0; i < FFT_SIZE; ++i) {
                outputRing[idx] += fftOut[i].real() * synthesisWindow[i] * scale;
                idx = (idx + 1) & (FFT_SIZE * 2 - 1);
            }
            outputWriteIdx = (outputWriteIdx + HOP_SIZE) & (FFT_SIZE * 2 - 1);
        }
    };
    
    std::array<ChannelState, MAX_CHANNELS> channels;
    int activeChannels{0};
    double sampleRate{44100.0};
    
    float binFrequency{0.0f};
    float expectedPhaseInc{0.0f};
    float outputScale{0.0f};
    
    int denormalFlushCounter{0};
    
    Impl() {
        pitchRatio.setImmediate(1.0f);
        formantShift.setImmediate(1.0f);
        mixAmount.setImmediate(1.0f);
        windowWidth.setImmediate(0.5f);
        spectralGate.setImmediate(0.0f);
        grainSize.setImmediate(0.5f);
        feedback.setImmediate(0.0f);
        stereoWidth.setImmediate(0.5f);
        
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
        
        binFrequency = static_cast<float>(sampleRate / FFT_SIZE);
        expectedPhaseInc = 2.0f * static_cast<float>(M_PI) * HOP_SIZE / FFT_SIZE;
        
        // TEST DIFFERENT SCALING VALUES
        outputScale = 1.0f / FFT_SIZE;
        std::cout << "\n=== PITCHSHIFTER DEBUG INIT ===\n";
        std::cout << "FFT_SIZE: " << FFT_SIZE << "\n";
        std::cout << "HOP_SIZE: " << HOP_SIZE << "\n";
        std::cout << "OVERLAP_FACTOR: " << OVERLAP_FACTOR << "\n";
        std::cout << "outputScale: " << outputScale << "\n";
        
        for (auto& ch : channels) {
            ch.fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
            createWindows(ch.analysisWindow, ch.synthesisWindow);
            ch.reset();
        }
    }
    
    void createWindows(std::array<float, FFT_SIZE>& analysis,
                      std::array<float, FFT_SIZE>& synthesis) {
        for (int i = 0; i < FFT_SIZE; ++i) {
            const float t = static_cast<float>(i) / (FFT_SIZE - 1);
            analysis[i] = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * t);
        }
        
        synthesis = analysis;
        
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
        float inputRMS = 0, outputRMS = 0;
        
        for (int i = 0; i < numSamples; ++i) {
            const float pitch = pitchRatio.tick();
            const float formant = formantShift.tick();
            const float mix = mixAmount.tick();
            const float gate = spectralGate.tick();
            const float fbAmount = feedback.tick() * 0.7f;
            const float window = windowWidth.tick();
            
            float input = ch.inputDC.process(data[i]);
            inputRMS += input * input;
            
            if (fbAmount > 1e-6f) {
                input += DSPUtils::flushDenorm(ch.feedbackBuffer[ch.feedbackReadPos] * fbAmount);
                ch.feedbackReadPos = (ch.feedbackReadPos + 1) % ch.feedbackBuffer.size();
            }
            
            ch.writeSample(input);
            ch.hopCounter++;
            
            if (ch.hopCounter >= HOP_SIZE) {
                ch.hopCounter = 0;
                processSpectralFrame(ch, pitch, formant, gate, window);
            }
            
            float output = ch.readOutput();
            outputRMS += output * output;
            
            if (fbAmount > 1e-6f) {
                ch.feedbackBuffer[ch.feedbackWritePos] = output;
                ch.feedbackWritePos = (ch.feedbackWritePos + 1) % ch.feedbackBuffer.size();
            }
            
            output = DSPUtils::flushDenorm(ch.outputDC.process(output));
            
            if (std::abs(output) > 0.95f) {
                output = std::tanh(output);
            }
            
            data[i] = DSPUtils::flushDenorm(input * (1.0f - mix) + output * mix);
        }
        
        // Log debug info periodically
        processCounter++;
        if (processCounter % 100 == 0 && !debugLogged) {
            inputRMS = std::sqrt(inputRMS / numSamples);
            outputRMS = std::sqrt(outputRMS / numSamples);
            std::cout << "\n=== PROCESS DEBUG (block " << processCounter << ") ===\n";
            std::cout << "Input RMS: " << inputRMS << "\n";
            std::cout << "Output RMS (before mix): " << outputRMS << "\n";
            std::cout << "Mix: " << mixAmount.getValue() << "\n";
            std::cout << "Pitch: " << pitchRatio.getValue() << "\n";
            std::cout << "Frames processed: " << frameCounter << "\n";
            
            if (outputRMS < 0.001f && inputRMS > 0.01f) {
                std::cout << "❌ OUTPUT IS ZERO!\n";
                debugLogged = true;
            }
        }
    }
    
    void processSpectralFrame(ChannelState& ch, float pitch, float formant, float gate, float window) {
        frameCounter++;
        
        ch.gatherFrame();
        
        // Calculate input RMS
        float inputRMS = 0;
        for (float s : ch.frameBuffer) {
            inputRMS += s * s;
        }
        inputRMS = std::sqrt(inputRMS / FFT_SIZE);
        
        // Apply window
        alignas(32) std::array<float, FFT_SIZE> windowed;
        for (int i = 0; i < FFT_SIZE; ++i) {
            float windowShape = ch.analysisWindow[i];
            if (window < 0.5f) {
                windowShape = std::pow(windowShape, 1.0f + (0.5f - window) * 2.0f);
            } else {
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
        
        // Calculate spectrum RMS
        float spectrumRMS = 0;
        for (const auto& c : ch.spectrum) {
            spectrumRMS += std::norm(c);
        }
        spectrumRMS = std::sqrt(spectrumRMS / FFT_SIZE);
        
        // BYPASS TEST - Skip phase vocoder
        // Just do FFT -> IFFT to test if that works
        
        // Inverse FFT
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), true);
        
        // Calculate output RMS
        float outputRMS = 0;
        for (const auto& c : ch.spectrum) {
            outputRMS += c.real() * c.real();
        }
        outputRMS = std::sqrt(outputRMS / FFT_SIZE);
        
        // Log first frame
        if (frameCounter == 1) {
            std::cout << "\n=== FIRST FRAME DEBUG ===\n";
            std::cout << "Input RMS: " << inputRMS << "\n";
            std::cout << "Spectrum RMS after FFT: " << spectrumRMS << "\n";
            std::cout << "Output RMS after IFFT: " << outputRMS << "\n";
            std::cout << "Output scale: " << outputScale << "\n";
            std::cout << "Scaled output: " << (outputRMS * outputScale) << "\n";
        }
        
        ch.scatterFrame(ch.spectrum.data(), outputScale);
        
        if (++denormalFlushCounter >= 256) {
            denormalFlushCounter = 0;
            for (int i = 0; i <= FFT_SIZE/2; ++i) {
                ch.phaseSum[i] = DSPUtils::flushDenorm(ch.phaseSum[i]);
                ch.phaseLast[i] = DSPUtils::flushDenorm(ch.phaseLast[i]);
            }
            for (auto& sample : ch.outputRing) {
                sample = DSPUtils::flushDenorm(sample);
            }
        }
    }
};

// Public interface
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
    
    for (int ch = 0; ch < numChannels; ++ch) {
        pimpl->processChannel(pimpl->channels[ch], buffer.getWritePointer(ch), numSamples);
    }
    
    if (numChannels >= 2) {
        pimpl->processStereoWidth(buffer.getWritePointer(0), 
                                 buffer.getWritePointer(1), 
                                 numSamples);
    }
    
    scrubBuffer(buffer);
}

void PitchShifter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        switch (index) {
            case kPitch: {
                float semitones = (value - 0.5f) * 48.0f;
                float ratio = std::pow(2.0f, semitones / 12.0f);
                pimpl->pitchRatio.setTarget(ratio);
                break;
            }
            case kFormant:  
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

void PitchShifter::processStereoWidth(float* left, float* right, int numSamples) {
    pimpl->processStereoWidth(left, right, numSamples);
}