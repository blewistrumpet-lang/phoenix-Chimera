#include "TransientShaper.h"

TransientShaper::TransientShaper() {}

void TransientShaper::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void TransientShaper::reset() {}

void TransientShaper::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void TransientShaper::updateParameters(const std::map<int, float>& params) {}

juce::String TransientShaper::getParameterName(int index) const {
    return "Param " + juce::String(index + 1);
}
