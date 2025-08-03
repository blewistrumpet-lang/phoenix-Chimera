#include "ResonantChorus.h"

ResonantChorus::ResonantChorus() {}

void ResonantChorus::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void ResonantChorus::reset() {}

void ResonantChorus::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void ResonantChorus::updateParameters(const std::map<int, float>& params) {}

juce::String ResonantChorus::getParameterName(int index) const {
    return "Param " + juce::String(index + 1);
}