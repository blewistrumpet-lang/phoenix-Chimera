#include "StereoChorus.h"

StereoChorus::StereoChorus() {}

void StereoChorus::prepareToPlay(double sampleRate, int samplesPerBlock) {
    m_sampleRate = sampleRate;
}

void StereoChorus::reset() {}

void StereoChorus::process(juce::AudioBuffer<float>& buffer) {
    // Simple passthrough for now
}

void StereoChorus::updateParameters(const std::map<int, float>& params) {}

juce::String StereoChorus::getParameterName(int index) const {
    switch (index) {
        case 0: return "Rate";
        case 1: return "Depth";
        case 2: return "Feedback";
        case 3: return "Delay";
        case 4: return "Width";
        case 5: return "Mix";
        default: return "Param " + juce::String(index + 1);
    }
}