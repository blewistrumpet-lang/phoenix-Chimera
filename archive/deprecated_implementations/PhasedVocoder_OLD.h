#pragma once
#include "EngineBase.h"
#include <vector>
#include <complex>
#include <memory>
#include <juce_dsp/juce_dsp.h>

class PhasedVocoder : public EngineBase {
public:
    PhasedVocoder();
    ~PhasedVocoder() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Phased Vocoder"; }
    
private:
    static constexpr int FFT_SIZE = 2048;
    static constexpr int OVERLAP = 4;
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP;
    static constexpr int MAX_STRETCH = 16;
    
    // Parameters
    float m_timeStretch = 1.0f;
    float m_pitchShift = 1.0f;
    float m_spectralSmear = 0.0f;
    float m_transientPreserve = 0.5f;
    float m_phaseReset = 0.0f;
    float m_spectralGate = 0.0f;
    float m_mixAmount = 1.0f;
    float m_freeze = 0.0f;
    
    struct ChannelState {
        // Input/output buffers
        std::vector<float> inputBuffer;
        std::vector<float> outputBuffer;
        std::vector<float> grainBuffer;
        
        // FFT data
        std::vector<std::complex<float>> fftBuffer;
        std::vector<float> window;
        std::vector<float> magnitude;
        std::vector<float> phase;
        std::vector<float> lastPhase;
        std::vector<float> phaseAccum;
        std::vector<float> trueBinFreq;
        
        // Freeze buffer
        std::vector<float> freezeMagnitude;
        std::vector<float> freezePhase;
        bool isFrozen = false;
        
        // Position tracking
        float readPos = 0.0f;
        int writePos = 0;
        int outputReadPos = 0;
        int hopCounter = 0;
        
        // Transient detection
        float envelopeFollower = 0.0f;
        float lastMagnitudeSum = 0.0f;
        
        juce::dsp::FFT fft{11}; // 2^11 = 2048
    };
    
    std::vector<ChannelState> m_channelStates;
    double m_sampleRate = 44100.0;
    
    void processFrame(ChannelState& state);
    void analyzeFrame(ChannelState& state);
    void synthesizeFrame(ChannelState& state);
    void applySpectralProcessing(ChannelState& state);
    float detectTransient(ChannelState& state);
    void createWindow(std::vector<float>& window);
};