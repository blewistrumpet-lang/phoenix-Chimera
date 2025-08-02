#pragma once

#include "EngineBase.h"

class BypassEngine : public EngineBase {
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
    
    void process(juce::AudioBuffer<float>& buffer) override {
        // Bypass - do nothing
    }
    
    void updateParameters(const std::map<int, float>& params) override {}
    
    juce::String getName() const override { return "Bypass"; }
    
    int getNumParameters() const override { return 0; }
    
    juce::String getParameterName(int index) const override { return ""; }
};