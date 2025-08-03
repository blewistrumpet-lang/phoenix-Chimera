#pragma once
#include <JuceHeader.h>
#include <cmath>

/**
 * Test Signal Generator for Engine Testing
 * Generates various test signals for validating audio processing
 */
class TestSignalGenerator {
public:
    TestSignalGenerator() = default;
    
    // Core test signals
    static juce::AudioBuffer<float> generateSineWave(float frequency, float duration, float sampleRate, float amplitude = 0.5f);
    static juce::AudioBuffer<float> generateWhiteNoise(float duration, float sampleRate, float amplitude = 0.5f);
    static juce::AudioBuffer<float> generatePinkNoise(float duration, float sampleRate, float amplitude = 0.5f);
    static juce::AudioBuffer<float> generateImpulse(float sampleRate, float amplitude = 1.0f);
    static juce::AudioBuffer<float> generateSweep(float startFreq, float endFreq, float duration, float sampleRate, float amplitude = 0.5f);
    static juce::AudioBuffer<float> generateSquareWave(float frequency, float duration, float sampleRate, float amplitude = 0.5f);
    static juce::AudioBuffer<float> generateSilence(float duration, float sampleRate);
    
    // Musical test signals
    static juce::AudioBuffer<float> generateDrumHit(float sampleRate);
    static juce::AudioBuffer<float> generateChord(float fundamentalFreq, float duration, float sampleRate);
    static juce::AudioBuffer<float> generateBurst(float onTime, float offTime, float totalDuration, float sampleRate);
    static juce::AudioBuffer<float> generateTwoTone(float freq1, float freq2, float duration, float sampleRate);
    
    // Utility functions
    static float dBToLinear(float dB) { return std::pow(10.0f, dB / 20.0f); }
    static float linearTodB(float linear) { return 20.0f * std::log10(std::max(0.00001f, linear)); }
    static void scaleSignal(juce::AudioBuffer<float>& buffer, float scale);
    static void normalizeSignal(juce::AudioBuffer<float>& buffer);
    
private:
    // Pink noise filter state
    struct PinkNoiseFilter {
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
        
        float process(float white) {
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f;
            b4 = 0.55000f * b4 + white * 0.5329522f;
            b5 = -0.7616f * b5 - white * 0.0168980f;
            float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
            b6 = white * 0.115926f;
            return pink * 0.11f; // Normalize
        }
    };
};