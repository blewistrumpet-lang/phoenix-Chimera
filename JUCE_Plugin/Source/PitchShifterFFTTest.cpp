#include "PitchShifter.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <array>
#include <iostream>

// TEST VERSION - FFT -> IFFT passthrough only (no phase vocoder)

struct PitchShifter::Impl {
    static constexpr int FFT_ORDER = 12;  // 2^12 = 4096
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int OVERLAP_FACTOR = 4;
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP_FACTOR;
    
    std::atomic<float> mixTarget{1.0f};
    float mixCurrent{1.0f};
    
    struct ChannelState {
        std::array<float, FFT_SIZE * 2> inputRing{};
        std::array<float, FFT_SIZE * 2> outputRing{};
        std::array<std::complex<float>, FFT_SIZE> spectrum{};
        std::array<float, FFT_SIZE> window{};
        
        int inputWriteIdx{0};
        int inputReadIdx{0}; 
        int outputWriteIdx{0};
        int outputReadIdx{0};
        int hopCounter{0};
        int frameCount{0};
        
        std::unique_ptr<juce::dsp::FFT> fft;
        
        void reset() {
            inputRing.fill(0.0f);
            outputRing.fill(0.0f);
            spectrum.fill(std::complex<float>(0.0f, 0.0f));
            inputWriteIdx = 0;
            inputReadIdx = 0;
            outputWriteIdx = 0;
            outputReadIdx = 0;
            hopCounter = 0;
            frameCount = 0;
        }
        
        void writeSample(float sample) {
            inputRing[inputWriteIdx] = sample;
            inputWriteIdx = (inputWriteIdx + 1) & (FFT_SIZE * 2 - 1);
        }
        
        float readOutput() {
            float out = outputRing[outputReadIdx];
            outputRing[outputReadIdx] = 0.0f;
            outputReadIdx = (outputReadIdx + 1) & (FFT_SIZE * 2 - 1);
            return out;
        }
    };
    
    std::array<ChannelState, 8> channels;
    double sampleRate{44100.0};
    bool initialized{false};
    
    void prepareToPlay(double sr) {
        sampleRate = sr;
        
        std::cout << "\n=== FFT TEST INIT ===\n";
        std::cout << "FFT_SIZE: " << FFT_SIZE << "\n";
        std::cout << "HOP_SIZE: " << HOP_SIZE << "\n";
        
        for (auto& ch : channels) {
            ch.fft = std::make_unique<juce::dsp::FFT>(FFT_ORDER);
            
            // Create Hann window
            for (int i = 0; i < FFT_SIZE; ++i) {
                ch.window[i] = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (FFT_SIZE - 1));
            }
            
            ch.reset();
        }
        
        initialized = true;
    }
    
    void processChannel(ChannelState& ch, float* data, int numSamples) {
        if (!initialized) return;
        
        const float smoothing = 0.995f;
        
        for (int i = 0; i < numSamples; ++i) {
            // Smooth mix
            mixCurrent += (mixTarget.load() - mixCurrent) * (1.0f - smoothing);
            
            float input = data[i];
            
            // Write to input ring
            ch.writeSample(input);
            ch.hopCounter++;
            
            // Process frame when ready
            if (ch.hopCounter >= HOP_SIZE) {
                ch.hopCounter = 0;
                processFrame(ch);
            }
            
            // Read output
            float output = ch.readOutput();
            
            // Apply mix
            data[i] = input * (1.0f - mixCurrent) + output * mixCurrent;
        }
    }
    
    void processFrame(ChannelState& ch) {
        ch.frameCount++;
        
        // Gather input frame
        int idx = ch.inputReadIdx;
        for (int i = 0; i < FFT_SIZE; ++i) {
            float sample = ch.inputRing[idx] * ch.window[i];
            ch.spectrum[i] = std::complex<float>(sample, 0.0f);
            idx = (idx + 1) & (FFT_SIZE * 2 - 1);
        }
        ch.inputReadIdx = (ch.inputReadIdx + HOP_SIZE) & (FFT_SIZE * 2 - 1);
        
        // TEST 1: Try different scaling approaches
        
        // Forward FFT
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), false);
        
        // Immediately inverse FFT (no processing)
        ch.fft->perform(ch.spectrum.data(), ch.spectrum.data(), true);
        
        // TEST DIFFERENT SCALINGS
        // JUCE FFT includes 1/N scaling in inverse transform
        // But we need additional scaling for overlap-add
        
        float scale = 1.0f / OVERLAP_FACTOR;  // Try 1/4 = 0.25
        
        if (ch.frameCount == 1) {
            std::cout << "\nFirst frame - testing scale: " << scale << "\n";
            
            // Check first few samples
            float sum = 0;
            for (int i = 0; i < 100; ++i) {
                sum += std::abs(ch.spectrum[i].real());
            }
            std::cout << "Sum of first 100 samples: " << sum << "\n";
            std::cout << "Average: " << (sum/100) << "\n";
        }
        
        // Scatter to output with overlap-add
        idx = ch.outputWriteIdx;
        for (int i = 0; i < FFT_SIZE; ++i) {
            ch.outputRing[idx] += ch.spectrum[i].real() * ch.window[i] * scale;
            idx = (idx + 1) & (FFT_SIZE * 2 - 1);
        }
        ch.outputWriteIdx = (ch.outputWriteIdx + HOP_SIZE) & (FFT_SIZE * 2 - 1);
    }
};

PitchShifter::PitchShifter() : pimpl(std::make_unique<Impl>()) {}
PitchShifter::~PitchShifter() = default;

void PitchShifter::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    pimpl->prepareToPlay(sampleRate);
}

void PitchShifter::reset() {
    for (auto& ch : pimpl->channels) {
        ch.reset();
    }
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels && ch < 8; ++ch) {
        pimpl->processChannel(pimpl->channels[ch], buffer.getWritePointer(ch), numSamples);
    }
}

void PitchShifter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index == kMix) {
            pimpl->mixTarget.store(value);
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