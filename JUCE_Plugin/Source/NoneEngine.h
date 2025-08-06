#pragma once

#include "EngineBase.h"

/**
 * NoneEngine - A passthrough engine for empty slots
 * 
 * This engine does nothing except pass audio through unchanged.
 * It represents an empty slot in the signal chain and has no parameters.
 * 
 * IMPORTANT: This engine should always have ID 0 to represent "no selection"
 */
class NoneEngine : public EngineBase {
public:
    NoneEngine() = default;
    ~NoneEngine() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // No preparation needed for passthrough
        (void)sampleRate;
        (void)samplesPerBlock;
    }
    
    void process(juce::AudioBuffer<float>& buffer) override {
        // Pure passthrough - do nothing to the buffer
        (void)buffer;
    }
    
    void reset() override {
        // No state to reset
    }
    
    void updateParameters(const std::map<int, float>& params) override {
        // No parameters to update
        (void)params;
    }
    
    juce::String getName() const override { 
        return "None"; 
    }
    
    int getNumParameters() const override { 
        return 0; 
    }
    
    juce::String getParameterName(int index) const override { 
        (void)index;
        return ""; 
    }
};