#include "StateVariableFilter.h"

StateVariableFilter::StateVariableFilter() {}

void StateVariableFilter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void StateVariableFilter::reset() {}

void StateVariableFilter::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void StateVariableFilter::updateParameters(const std::map<int, float>& params) {}

juce::String StateVariableFilter::getParameterName(int index) const {
    return "Param " + juce::String(index + 1);
}
