#include "SpectralGate.h"

SpectralGate::SpectralGate() {}

void SpectralGate::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void SpectralGate::reset() {}

void SpectralGate::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void SpectralGate::updateParameters(const std::map<int, float>& params) {}

juce::String SpectralGate::getParameterName(int index) const {
    switch (index) {
        case 0: return "Threshold";
        case 1: return "Ratio";
        case 2: return "Attack";
        case 3: return "Release";
        case 4: return "Frequency Range";
        case 5: return "Spectral Tilt";
        case 6: return "Lookahead";
        case 7: return "Mix";
        default: return "Param " + juce::String(index + 1);
    }
}