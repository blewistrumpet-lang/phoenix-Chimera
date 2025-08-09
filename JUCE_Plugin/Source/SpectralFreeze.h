// SpectralFreeze_Ultimate.h
#pragma once
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <vector>
#include <complex>
#include <cmath>
#include <memory>
#include <array>
#include <atomic>
#include <random>

// Platform-specific SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>  // For SIMD on x86/x64
    #define HAS_SIMD 1
#else
    #define HAS_SIMD 0
#endif

class SpectralFreeze : public EngineBase {
public:
    SpectralFreeze();
    ~SpectralFreeze() override = default;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    void updateParameters(const std::map<int, float>& params) override;
    
    int getNumParameters() const override { return 8; }
    juce::String getParameterName(int index) const override;
    juce::String getName() const override { return "Spectral Freeze Ultimate"; }
    
private:
    // FFT Configuration
    static constexpr int FFT_ORDER = 11;  // 2048 samples
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int HALF_FFT_SIZE = FFT_SIZE / 2;
    static constexpr int HOP_SIZE = FFT_SIZE / 4;  // 75% overlap
    static constexpr int MAX_CHANNELS = 8;  // Support up to 8 channels
    
    // Alignment for SIMD
    static constexpr size_t SIMD_ALIGNMENT = 32;  // AVX alignment
    
    // Parameters with thread-safe one-pole smoothing
    struct SmoothParam {
        std::atomic<float> target{0.0f};
        float current = 0.0f;
        float smoothing = 0.999f;
        
        void update();
        void setImmediate(float value);
        void setSmoothingRate(float timeMs, double sampleRate);
    };
    
    SmoothParam m_freezeAmount;
    SmoothParam m_spectralSmear;
    SmoothParam m_spectralShift;
    SmoothParam m_resonance;
    SmoothParam m_decay;
    SmoothParam m_brightness;
    SmoothParam m_density;
    SmoothParam m_shimmer;
    
    // DSP State
    double m_sampleRate = 44100.0;
    int m_blockSize = 512;
    
    // Pre-computed window with normalization baked in
    alignas(SIMD_ALIGNMENT) std::array<float, FFT_SIZE> m_windowNormalized;
    alignas(SIMD_ALIGNMENT) std::array<float, FFT_SIZE> m_overlapCompensation;
    
    // FFT Processing with pre-allocated buffers
    struct FFTProcessor {
        std::unique_ptr<juce::dsp::FFT> fft;
        
        // Aligned pre-allocated buffers
        alignas(SIMD_ALIGNMENT) std::array<float, FFT_SIZE * 2> fftBuffer;
        alignas(SIMD_ALIGNMENT) std::array<std::complex<float>, FFT_SIZE> spectrum;
        alignas(SIMD_ALIGNMENT) std::array<std::complex<float>, FFT_SIZE> frozenSpectrum;
        
        // Decay state with leak prevention
        float decayState = 1.0f;
        static constexpr float DECAY_LEAK = 0.995f;
        static constexpr float DECAY_GAIN = 0.005f;
        
        void init(int fftOrder);
        void reset();
        void unpackRealFFT();
        void packToRealFFT();
    };
    
    // Per-channel processing with all buffers pre-allocated
    struct alignas(SIMD_ALIGNMENT) ChannelState {
        FFTProcessor fftProcessor;
        
        // Aligned circular buffers
        alignas(SIMD_ALIGNMENT) std::array<float, FFT_SIZE> inputBuffer;
        alignas(SIMD_ALIGNMENT) std::array<float, FFT_SIZE> outputBuffer;
        alignas(SIMD_ALIGNMENT) std::array<float, FFT_SIZE> windowedFrame;
        
        // Each channel needs its own temp buffer for thread safety
        alignas(SIMD_ALIGNMENT) std::array<std::complex<float>, FFT_SIZE> tempSpectrum;
        
        int inputPos = 0;
        int outputPos = 0;
        int hopCounter = 0;
        
        // Spectral freeze state
        bool isFrozen = false;
        int freezeCounter = 0;
        
        // Phase randomizer with incremental jitter
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> phaseDist{-0.1f, 0.1f};  // Small jitter
        std::array<float, FFT_SIZE> phaseAccumulator;  // Track phase changes
        
        // Processing mode flags (updated per hop)
        bool enableSmear = false;
        bool enableShift = false;
        bool enableResonance = false;
        bool enableDensity = false;
        bool enableShimmer = false;
        int shiftBins = 0;
        
        void init(int fftSize);
        void reset();
    };
    
    // Fixed-size channel array
    std::array<ChannelState, MAX_CHANNELS> m_channels;
    int m_activeChannels = 2;
    
    // Sub-block parameter smoothing
    static constexpr int SMOOTH_INTERVAL = 32;  // Samples between smoothing updates
    int m_smoothCounter = 0;
    
    // Using DspEngineUtilities DenormalGuard instead of custom implementation
    
    // Window generation with exact overlap compensation
    void generateWindowWithCompensation();
    
    // Validation helper for testing unity gain
    float validateUnityGain();
    
    // Optimized spectral processing functions
    void processSpectrum(ChannelState& state);
    void applySpectralSmear(std::complex<float>* spectrum, float amount, ChannelState& state);
    void applySpectralShift(std::complex<float>* spectrum, int shiftBins, ChannelState& state);
    void applyResonance(std::complex<float>* spectrum, float resonance);
    void applyBrightness(std::complex<float>* spectrum, float brightness);
    void applyDensity(std::complex<float>* spectrum, float density);
    void applyShimmer(std::complex<float>* spectrum, float shimmer, ChannelState& state);
    
    // Using DspEngineUtilities DSPUtils::flushDenorm instead of custom denormal prevention
};