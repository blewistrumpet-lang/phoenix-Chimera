#include "SpectralFreeze.h"

SpectralFreeze::SpectralFreeze() {}

void SpectralFreeze::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void SpectralFreeze::reset() {}

void SpectralFreeze::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void SpectralFreeze::updateParameters(const std::map<int, float>& params) {}

juce::String SpectralFreeze::getParameterName(int index) const {
    return "Param " + juce::String(index + 1);
}
