#include "PitchShifter.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>

// ULTRA SIMPLE TEST - Just pass audio through with mix control

struct PitchShifter::Impl {
    std::atomic<float> mixTarget{1.0f};
    float mixCurrent{1.0f};
    
    void processChannel(float* data, int numSamples) {
        // Smooth mix parameter
        const float smoothing = 0.995f;
        
        for (int i = 0; i < numSamples; ++i) {
            // Smooth the mix parameter
            mixCurrent += (mixTarget.load() - mixCurrent) * (1.0f - smoothing);
            
            // Store input
            float input = data[i];
            
            // "Process" (just pass through for now)
            float output = input;
            
            // Apply mix
            data[i] = input * (1.0f - mixCurrent) + output * mixCurrent;
        }
    }
};

PitchShifter::PitchShifter() : pimpl(std::make_unique<Impl>()) {}
PitchShifter::~PitchShifter() = default;

void PitchShifter::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/) {
    // Nothing to prepare for simple passthrough
}

void PitchShifter::reset() {
    // Nothing to reset
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        pimpl->processChannel(buffer.getWritePointer(ch), numSamples);
    }
}

void PitchShifter::updateParameters(const std::map<int, float>& params) {
    for (const auto& [index, value] : params) {
        if (index == kMix) {
            pimpl->mixTarget.store(value);
        }
        // Ignore other parameters for this test
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