#pragma once
#include "EngineBase.h"
#include <array>
#include <memory>

class SpectralGate : public EngineBase {
public:
    SpectralGate();
    ~SpectralGate() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void setNumChannels(int numIn, int numOut) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "SpectralGate"; }
    int getLatencySamples() const noexcept override { return m_latency; }
    float getCpuUsage() const noexcept override { return m_cpuUsage; }
    bool supportsFeature(Feature f) const noexcept override;
    
    // Parameter indices
    enum Parameters {
        Threshold = 0,
        Ratio = 1,
        Attack = 2,
        Release = 3,
        Range = 4,
        Lookahead = 5,
        Frequency = 6,
        Mix = 7
    };
    
private:
    // STFT parameters
    static constexpr int FFT_SIZE = 1024;
    static constexpr int HOP_SIZE = 256;
    static constexpr int OVERLAP_FACTOR = FFT_SIZE / HOP_SIZE; // 4x overlap
    static constexpr int SPECTRUM_SIZE = FFT_SIZE / 2 + 1;
    
    // Processing state for one channel
    struct ChannelState {
        // Circular buffers for overlap-add
        std::array<float, FFT_SIZE> inputBuffer{};
        std::array<float, FFT_SIZE> outputBuffer{};
        std::array<float, FFT_SIZE> overlapBuffer{};
        
        // FFT data
        std::array<float, FFT_SIZE * 2> fftData{}; // Interleaved complex
        std::array<float, FFT_SIZE * 2> ifftData{}; // Interleaved complex
        
        // Spectral processing
        std::array<float, SPECTRUM_SIZE> magnitude{};
        std::array<float, SPECTRUM_SIZE> phase{};
        std::array<float, SPECTRUM_SIZE> gateMask{};
        std::array<float, SPECTRUM_SIZE> smoothedMask{};
        std::array<float, SPECTRUM_SIZE> prevMagnitude{};
        
        // Gate state per frequency bin
        std::array<bool, SPECTRUM_SIZE> gateOpen{};
        std::array<float, SPECTRUM_SIZE> attackFrames{};
        std::array<float, SPECTRUM_SIZE> releaseFrames{};
        
        int writePos = 0;
        int readPos = 0;
        bool isReady = false;
    };
    
    // Core components
    std::unique_ptr<juce::dsp::FFT> m_fft;
    std::array<ChannelState, 2> m_channels; // Stereo
    std::array<float, FFT_SIZE> m_window; // Hann window
    
    // Parameters (with smoothing)
    juce::SmoothedValue<float> m_threshold{-30.0f};
    juce::SmoothedValue<float> m_ratio{10.0f};
    juce::SmoothedValue<float> m_attack{10.0f};
    juce::SmoothedValue<float> m_release{100.0f};
    juce::SmoothedValue<float> m_range{60.0f};
    juce::SmoothedValue<float> m_lookahead{0.0f};
    juce::SmoothedValue<float> m_frequency{1.0f};
    juce::SmoothedValue<float> m_mix{1.0f};
    
    // State
    double m_sampleRate = 44100.0;
    int m_samplesPerBlock = 512;
    int m_numChannels = 2;
    int m_latency = FFT_SIZE - HOP_SIZE; // 768 samples
    
    // Processing helpers
    void processFrame(ChannelState& channel);
    void computeSpectralGate(ChannelState& channel);
    void applyFrequencySmoothing(std::array<float, SPECTRUM_SIZE>& mask);
    float medianFilter3(float a, float b, float c);
    void generateHannWindow();
    
    // Thread safety
    juce::CriticalSection m_parameterLock;
    
    // Performance monitoring
    float m_cpuUsage = 0.0f;
    juce::Time m_lastProcessTime;
};
