#pragma once
#include "EngineBase.h"
#include <vector>
#include <complex>
#include <cmath>
#include <random>

class SpectralFreeze : public EngineBase {
public:
    SpectralFreeze();
    ~SpectralFreeze() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    juce::String getName() const override { return "Spectral Freeze"; }
    int getNumParameters() const override { return 4; }
    juce::String getParameterName(int index) const override;
    
private:
    // Parameters
    float m_freezeTrigger = 0.0f;      // 0-1 (momentary/latch)
    float m_resolution = 0.5f;         // FFT size selector
    float m_crystalline = 0.5f;        // Spectral filtering intensity
    float m_morph = 0.0f;             // Spectral content blending
    
    // DSP State
    double m_sampleRate = 44100.0;
    
    // FFT Configuration
    static constexpr int MAX_FFT_SIZE = 8192;
    static constexpr int MIN_FFT_SIZE = 512;
    int m_fftSize = 2048;
    int m_hopSize = 512;
    
    // Per-channel processing state
    struct ChannelState {
        // Input/output buffers
        std::vector<float> inputBuffer;
        std::vector<float> outputBuffer;
        int inputPos = 0;
        int outputPos = 0;
        
        // FFT data
        std::vector<std::complex<float>> fftData;
        std::vector<std::complex<float>> frozenSpectrum;
        std::vector<float> window;
        
        // Overlap-add buffer
        std::vector<float> overlapBuffer;
        
        // State
        bool isFrozen = false;
        float freezeFadeIn = 0.0f;
        
        void init(int maxFftSize);
        void reset();
    };
    
    std::vector<ChannelState> m_channels;
    
    // Previous freeze state for edge detection
    float m_prevFreezeTrigger = 0.0f;
    
    // Random phase generator
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_phaseDist{0.0f, 2.0f * M_PI};
    
    // FFT processing
    void performFFT(std::vector<std::complex<float>>& data, bool inverse);
    void applyWindow(std::vector<float>& buffer, const std::vector<float>& window);
    void processSpectralFrame(ChannelState& state);
    
    // Window generation
    void generateHannWindow(std::vector<float>& window, int size);
    
    // Spectral manipulation
    void applyCrystallineFilter(std::vector<std::complex<float>>& spectrum);
    void morphSpectra(std::vector<std::complex<float>>& result, 
                     const std::vector<std::complex<float>>& a,
                     const std::vector<std::complex<float>>& b,
                     float amount);
    
    // Utility
    int getFFTSizeFromParameter(float param);
};