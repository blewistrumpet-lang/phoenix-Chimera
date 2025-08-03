#pragma once
#include "EngineBase.h"

class StateVariableFilter : public EngineBase {
public:
    StateVariableFilter();
    ~StateVariableFilter() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "StateVariableFilter"; }
    
private:
    double m_sampleRate = 44100.0;
};
