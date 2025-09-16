#pragma once
#include "EngineBase.h"
#include "IPitchShiftStrategy.h"
#include "SMBPitchShift.h"
#include <memory>
#include <array>

/**
 * SimpleDetuner - A simplified detune doubler using SMBPitchShift
 * 
 * Creates a stereo-widened effect by detuning left and right channels
 * in opposite directions.
 */
class SimpleDetuner : public EngineBase {
public:
    SimpleDetuner() {
        // Create pitch shifters for each channel
        for (int i = 0; i < 2; ++i) {
            pitchShifters[i] = std::make_unique<SMBPitchShift>();
        }
    }
    
    ~SimpleDetuner() = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        this->sampleRate = sampleRate;
        
        // Prepare pitch shifters
        for (auto& shifter : pitchShifters) {
            if (shifter) {
                shifter->prepare(sampleRate, samplesPerBlock);
            }
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer) override {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        if (numChannels == 0 || numSamples == 0) return;
        
        // Process each channel with opposite detuning
        for (int ch = 0; ch < std::min(numChannels, 2); ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            
            // Calculate pitch ratio based on detune amount
            // Channel 0: detune down, Channel 1: detune up
            float cents = detuneAmount * (ch == 0 ? -1.0f : 1.0f);
            float pitchRatio = std::pow(2.0f, cents / 1200.0f);
            
            // Process with pitch shifter
            if (pitchShifters[ch]) {
                std::vector<float> output(numSamples);
                pitchShifters[ch]->process(channelData, output.data(), numSamples, pitchRatio);
                
                // Mix with dry signal
                for (int i = 0; i < numSamples; ++i) {
                    channelData[i] = channelData[i] * (1.0f - mixAmount) + output[i] * mixAmount;
                }
            }
        }
    }
    
    void reset() override {
        for (auto& shifter : pitchShifters) {
            if (shifter) shifter->reset();
        }
    }
    
    void updateParameters(const std::map<int, float>& params) override {
        for (const auto& [index, value] : params) {
            switch (index) {
                case 0: // Mix
                    mixAmount = value;
                    break;
                case 1: // Detune amount in cents (0-50 cents)
                    detuneAmount = value * 50.0f;
                    break;
            }
        }
    }
    
    juce::String getName() const override { return "Simple Detuner"; }
    int getNumParameters() const override { return 2; }
    
    juce::String getParameterName(int index) const override {
        switch (index) {
            case 0: return "Mix";
            case 1: return "Detune";
            default: return "";
        }
    }
    
    int getLatencySamples() const noexcept override {
        if (pitchShifters[0]) {
            return pitchShifters[0]->getLatencySamples();
        }
        return 0;
    }
    
private:
    std::array<std::unique_ptr<IPitchShiftStrategy>, 2> pitchShifters;
    double sampleRate = 44100.0;
    float mixAmount = 0.5f;
    float detuneAmount = 10.0f; // Cents
};