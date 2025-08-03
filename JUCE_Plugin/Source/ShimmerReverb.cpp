#include "ShimmerReverb.h"

ShimmerReverb::ShimmerReverb() {}

void ShimmerReverb::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void ShimmerReverb::reset() {}

void ShimmerReverb::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void ShimmerReverb::updateParameters(const std::map<int, float>& params) {}

juce::String ShimmerReverb::getParameterName(int index) const {
    return "Param " + juce::String(index + 1);
}
