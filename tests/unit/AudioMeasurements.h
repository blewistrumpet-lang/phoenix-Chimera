#pragma once
#include <JuceHeader.h>
#include <complex>
#include <vector>

/**
 * Audio Measurement Functions for Engine Testing
 * Provides analysis tools for measuring audio characteristics
 */
class AudioMeasurements {
public:
    // Core measurements
    static float measureRMS(const juce::AudioBuffer<float>& buffer);
    static float measurePeak(const juce::AudioBuffer<float>& buffer);
    static float measureTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate);
    static float measureSNR(const juce::AudioBuffer<float>& signal, const juce::AudioBuffer<float>& noise);
    
    // Dynamics measurements
    static float measureGainReduction(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output);
    static std::pair<float, float> measureEnvelopeTiming(const juce::AudioBuffer<float>& buffer, float sampleRate);
    
    // Frequency analysis
    struct FrequencyResponse {
        std::vector<float> frequencies;
        std::vector<float> magnitudes;
        std::vector<float> phases;
    };
    static FrequencyResponse computeFrequencyResponse(const juce::AudioBuffer<float>& buffer, float sampleRate);
    
    // Time-based measurements
    static float measureDelayTime(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output, float sampleRate);
    static float measureRT60(const juce::AudioBuffer<float>& impulseResponse, float sampleRate);
    
    // Modulation analysis
    struct ModulationProfile {
        float rate;     // Hz
        float depth;    // 0-1
        float phase;    // radians
    };
    static ModulationProfile extractModulationProfile(const juce::AudioBuffer<float>& buffer, float sampleRate);
    
    // Harmonic analysis
    struct HarmonicContent {
        std::vector<float> harmonicAmplitudes;  // Amplitude of each harmonic
        std::vector<float> harmonicFrequencies; // Frequency of each harmonic
        float thd;                              // Total harmonic distortion
    };
    static HarmonicContent measureHarmonicContent(const juce::AudioBuffer<float>& buffer, float fundamentalFreq, float sampleRate);
    
    // Intermodulation distortion
    static float measureIMD(const juce::AudioBuffer<float>& buffer, float freq1, float freq2, float sampleRate);
    
    // Utility functions
    static bool detectSustainedOscillation(const juce::AudioBuffer<float>& buffer, float sampleRate);
    static float measureLatency(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output, float sampleRate);
    static float measureNoiseFloor(const juce::AudioBuffer<float>& buffer);
    
    // FFT utilities
    static std::vector<std::complex<float>> performFFT(const juce::AudioBuffer<float>& buffer, int fftSize = 2048);
    static std::vector<float> computeMagnitudeSpectrum(const std::vector<std::complex<float>>& fftData);
    static std::vector<float> computePhaseSpectrum(const std::vector<std::complex<float>>& fftData);
    
private:
    // Helper functions
    static float findPeakFrequency(const std::vector<float>& magnitudeSpectrum, float sampleRate);
    static float correlate(const float* signal1, const float* signal2, int length);
    static int findDelayUsingCorrelation(const float* input, const float* output, int length);
};