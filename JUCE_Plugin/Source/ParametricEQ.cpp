#include "ParametricEQ.h"

ParametricEQ::ParametricEQ() {}

void ParametricEQ::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void ParametricEQ::reset() {}

void ParametricEQ::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void ParametricEQ::updateParameters(const std::map<int, float>& params) {}

juce::String ParametricEQ::getParameterName(int index) const {
    return "Param " + juce::String(index + 1);
}
