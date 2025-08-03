#include "MidSideProcessor.h"

MidSideProcessor::MidSideProcessor() {}

void MidSideProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void MidSideProcessor::reset() {}

void MidSideProcessor::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void MidSideProcessor::updateParameters(const std::map<int, float>& params) {}

juce::String MidSideProcessor::getParameterName(int index) const {
    switch (index) {
        case 0: return "Mid Gain";
        case 1: return "Side Gain";
        case 2: return "Mid HF";
        case 3: return "Mid HF Gain";
        case 4: return "Side Low Cut";
        case 5: return "Side High Boost";
        case 6: return "Stereo Width";
        case 7: return "Bass to Mid";
        default: return "Param " + juce::String(index + 1);
    }
}