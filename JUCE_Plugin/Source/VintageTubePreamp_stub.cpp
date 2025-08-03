#include "VintageTubePreamp.h"

VintageTubePreamp::VintageTubePreamp() {}

void VintageTubePreamp::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void VintageTubePreamp::reset() {}

void VintageTubePreamp::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void VintageTubePreamp::updateParameters(const std::map<int, float>& params) {
    // Store parameters but don't process
}

juce::String VintageTubePreamp::getParameterName(int index) const {
    switch (index) {
        case 0: return "Input Gain";
        case 1: return "Warmth";
        case 2: return "Presence";
        case 3: return "Drive";
        case 4: return "Bias";
        case 5: return "Tone";
        case 6: return "Output Gain";
        case 7: return "Mix";
        default: return "Param " + juce::String(index + 1);
    }
}