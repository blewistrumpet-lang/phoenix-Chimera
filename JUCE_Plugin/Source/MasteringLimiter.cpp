#include "MasteringLimiter.h"

MasteringLimiter::MasteringLimiter() {}

void MasteringLimiter::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void MasteringLimiter::reset() {}

void MasteringLimiter::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void MasteringLimiter::updateParameters(const std::map<int, float>& params) {}

juce::String MasteringLimiter::getParameterName(int index) const {
    return "Param " + juce::String(index + 1);
}
