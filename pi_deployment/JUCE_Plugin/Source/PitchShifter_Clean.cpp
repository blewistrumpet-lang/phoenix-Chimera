// Clean Phase Vocoder Implementation for PitchShifter
// Focuses on correctness and minimal artifacts

#include "PitchShifter.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <array>
#include <algorithm>

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
    
    inline float tick() noexcept {
        const float t = target.load(std::memory_order_relaxed);
        current += (t - current) * (1.0f - smoothing);
        return current;
    }
    
    float getValue() const noexcept {
        return current;
    }
    
private:
    std::atomic<float> target{0.0f};
    float current{0.0f};
    float smoothing{0.995f};
};

struct PitchShifter::Impl {
    static constexpr int FFT_ORDER = 11;  // 2048 points - good balance
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int OVERLAP_FACTOR = 4;  // 75% overlap
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;
    static constexpr int MAX_CHANNELS = 8;
    
    // Parameters
    AtomicSmoothParam pitchRatio;
    AtomicSmoothParam formantShift;
    AtomicSmoothParam mixAmount;
    AtomicSmoothParam windowWidth;
    AtomicSmoothParam spectralGate;
    AtomicSmoothParam grainSize;
    AtomicSmoothParam feedback;
    AtomicSmoothParam stereoWidth;
    
    std::atomic<float> snappedPitchValue{0.5f};
    
    struct ChannelState {
        // Circular buffers
        std::array<float, FFT_SIZE * 2> inputBuffer{};
        std::array<float, FFT_SIZE * 2> outputBuffer{};
        int inputPos{0};
        int outputPos{0};
        int hopCounter{0};
        
        // FFT data
        alignas(32) std::array<std::complex<float>, FFT_SIZE> fftData{};
        alignas(32) std::array<float, FFT_SIZE> window{};
        
        // Phase vocoder state - proper initialization is critical
        alignas(32) std::array<double, FFT_SIZE/2 + 1> lastPhase{};
        alignas(32) std::array<double, FFT_SIZE/2 + 1> sumPhase{};
        alignas(32) std::array<float, FFT_SIZE/2 + 1> magnitude{};
        alignas(32) std::array<float, FFT_SIZE/2 + 1> frequency{};
        
        std::unique_ptr<juce::dsp::FFT> fft;
        DCBlocker dcBlocker;
        
        void reset() {
            inputBuffer.fill(0.0f);
            outputBuffer.fill(0.0f);
            inputPos = 0;
            outputPos = 0;
            hopCounter = 0;
            
            // Critical: properly initialize phase arrays
            lastPhase.fill(0.0);
            sumPhase.fill(0.0);
            magnitude.fill(0.0f);
            frequency.fill(0.0f);
            
            dcBlocker.reset();
        }
    };
    
    std::array<ChannelState, MAX_CHANNELS> channels;
    double sampleRate{44100.0};
    
    // Pre-computed constants
    double binFrequency{0.0};
    double expPhaseInc{0.0};
    
    Impl() {
        pitchRatio.setImmediate(1.0f);
        formantShift.setImmediate(0.5f);
        mixAmount.setImmediate(1.0f);
        windowWidth.setImmediate(0.5f);
        spectralGate.setImmediate(0.0f);
        grainSize.setImmediate(0.5f);
        feedback.setImmediate(0.0f);
        stereoWidth.setImmediate(0.5f);
        
        pitchRatio.setSmoothingCoeff(0.995f);
        formantShift.setSmoothingCoeff(0.995f);
        mixAmount.setSmoothingCoeff(0.995f);
    }
    
    void prepareToPlay(double sr, int /*samplesPerBlock*/) {
        sampleRate = sr;
        binFrequency = sr / FFT_SIZE;
        expPhaseInc = 2.0 * M_PI * HOP_SIZE / FFT_SIZE;
        
        for (auto& ch : channels) {
            ch.fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
            createHannWindow(ch.window);
            ch.reset();
        }
    }
    
    void createHannWindow(std::array<float, FFT_SIZE>& window) {
        // Standard Hann window
        for (int i = 0; i < FFT_SIZE; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (FFT_SIZE - 1)));
        }
    }
    
    void processChannel(ChannelState& ch, float* data, int numSamples) {
        const float pitch = pitchRatio.getValue();
        const float formant = formantShift.getValue();
        const float mix = mixAmount.getValue();
        
        // True bypass for neutral settings
        if (std::abs(pitch - 1.0f) < 0.001f && 
            std::abs(formant - 0.5f) < 0.001f && 
            std::abs(mix - 1.0f) < 0.001f) {
            return;  // No processing needed
        }
        
        for (int i = 0; i < numSamples; ++i) {
            const float input = data[i];
            
            // Write to input buffer
            ch.inputBuffer[ch.inputPos] = input;
            ch.inputPos = (ch.inputPos + 1) % (FFT_SIZE * 2);
            ch.hopCounter++;
            
            // Process frame at hop boundary
            if (ch.hopCounter >= HOP_SIZE) {
                ch.hopCounter = 0;
                processFrame(ch, pitch, formant);
            }
            
            // Read from output buffer
            float output = ch.outputBuffer[ch.outputPos];
            ch.outputBuffer[ch.outputPos] = 0.0f;  // Clear after reading
            ch.outputPos = (ch.outputPos + 1) % (FFT_SIZE * 2);
            
            // Apply DC blocking and mix
            output = ch.dcBlocker.process(output);
            data[i] = input * (1.0f - mix) + output * mix;
        }
    }
    
    void processFrame(ChannelState& ch, float pitch, float formant) {
        // 1. Analysis: Get windowed frame
        int readPos = (ch.inputPos - FFT_SIZE + FFT_SIZE * 2) % (FFT_SIZE * 2);
        for (int i = 0; i < FFT_SIZE; ++i) {
            ch.fftData[i] = ch.inputBuffer[readPos] * ch.window[i];
            readPos = (readPos + 1) % (FFT_SIZE * 2);
        }
        
        // 2. Forward FFT
        ch.fft->perform(ch.fftData.data(), ch.fftData.data(), false);
        
        // 3. Phase analysis
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            const auto& bin = ch.fftData[k];
            
            // Magnitude
            ch.magnitude[k] = std::abs(bin);
            
            // Phase
            double phase = std::arg(bin);
            
            // Phase difference
            double phaseDiff = phase - ch.lastPhase[k];
            ch.lastPhase[k] = phase;
            
            // Wrap to [-PI, PI]
            phaseDiff -= 2.0 * M_PI * std::round(phaseDiff / (2.0 * M_PI));
            
            // Frequency deviation
            double expectedPhase = k * expPhaseInc;
            double deviation = phaseDiff - expectedPhase;
            deviation -= 2.0 * M_PI * std::round(deviation / (2.0 * M_PI));
            
            // True frequency
            ch.frequency[k] = (k + deviation / (2.0 * M_PI) * FFT_SIZE / HOP_SIZE) * binFrequency;
        }
        
        // 4. Pitch shifting
        alignas(32) std::array<std::complex<float>, FFT_SIZE> shiftedSpectrum{};
        
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            // Update phase accumulator
            double shiftedFreq = ch.frequency[k] * pitch;
            double phaseAdvance = 2.0 * M_PI * shiftedFreq * HOP_SIZE / sampleRate;
            ch.sumPhase[k] += phaseAdvance;
            
            // Wrap phase
            ch.sumPhase[k] -= 2.0 * M_PI * std::round(ch.sumPhase[k] / (2.0 * M_PI));
            
            // Get magnitude from resampled spectrum
            float mag = 0.0f;
            float sourceBin = k / pitch;
            
            if (sourceBin >= 0 && sourceBin < FFT_SIZE/2) {
                // Linear interpolation for magnitude
                int bin1 = static_cast<int>(sourceBin);
                int bin2 = std::min(bin1 + 1, FFT_SIZE/2);
                float frac = sourceBin - bin1;
                
                mag = ch.magnitude[bin1] * (1.0f - frac) + ch.magnitude[bin2] * frac;
            }
            
            // Apply formant shift if needed
            if (std::abs(formant - 0.5f) > 0.001f) {
                // Simple spectral tilt for formant
                float tilt = std::pow(static_cast<float>(k) / (FFT_SIZE/2), 2.0f * (formant - 0.5f));
                mag *= tilt;
            }
            
            // Create shifted bin
            shiftedSpectrum[k] = std::polar(mag, static_cast<float>(ch.sumPhase[k]));
        }
        
        // Maintain conjugate symmetry
        for (int k = 1; k < FFT_SIZE/2; ++k) {
            shiftedSpectrum[FFT_SIZE - k] = std::conj(shiftedSpectrum[k]);
        }
        
        // 5. Inverse FFT
        ch.fft->perform(shiftedSpectrum.data(), shiftedSpectrum.data(), true);
        
        // 6. Overlap-add with proper scaling
        // The FFT scaling in JUCE is 1/N for inverse
        const float scale = 1.0f / OVERLAP_FACTOR;
        
        int writePos = ch.outputPos;
        for (int i = 0; i < FFT_SIZE; ++i) {
            ch.outputBuffer[writePos] += shiftedSpectrum[i].real() * ch.window[i] * scale;
            writePos = (writePos + 1) % (FFT_SIZE * 2);
        }
    }
    
    void processStereoWidth(float* left, float* right, int numSamples) {
        const float width = stereoWidth.tick() * 2.0f;
        for (int i = 0; i < numSamples; ++i) {
            const float mid = (left[i] + right[i]) * 0.5f;
            const float side = (left[i] - right[i]) * 0.5f * width;
            left[i] = mid + side;
            right[i] = mid - side;
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
    const int numChannels = std::min(buffer.getNumChannels(), Impl::MAX_CHANNELS);
    const int numSamples = buffer.getNumSamples();
    
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
                float snappedValue = value;
                const float snapPoints[] = {
                    0.250f, 0.354f, 0.396f, 0.417f, 0.438f, 0.479f,
                    0.500f, 0.521f, 0.563f, 0.583f, 0.604f, 0.646f, 0.750f
                };
                
                float minDistance = 1.0f;
                for (float snapPoint : snapPoints) {
                    float distance = std::abs(value - snapPoint);
                    if (distance < minDistance) {
                        minDistance = distance;
                        snappedValue = snapPoint;
                    }
                }
                
                pimpl->snappedPitchValue.store(snappedValue);
                float semitones = (snappedValue - 0.5f) * 48.0f;
                float ratio = std::pow(2.0f, semitones / 12.0f);
                pimpl->pitchRatio.setImmediate(ratio);
                break;
            }
            case kFormant:  pimpl->formantShift.setTarget(value); break;
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
        float snappedValue = pimpl->snappedPitchValue.load();
        return juce::String(snappedValue, 3);
    }
    return "";
}