// PhasedVocoder_Reference.cpp - Team C: Reference Implementation
// Simplified, working phase vocoder based on standard STFT algorithm
// Focus: Correctness over performance, educational clarity, proven parameters

#include "PhasedVocoder_Reference.h"
#include "DspEngineUtilities.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace {
    // Standard phase vocoder parameters (proven to work well)
    constexpr int FFT_ORDER = 11;           // 2^11 = 2048
    constexpr int FFT_SIZE = 1 << FFT_ORDER;
    constexpr int HOP_SIZE = 512;            // 75% overlap (FFT_SIZE / 4)
    constexpr float TWO_PI = 6.28318530718f;
    constexpr float PI = 3.14159265359f;
    
    // Simple modulo wrapper for circular buffers
    inline int wrapIndex(int index, int bufferSize) {
        while (index >= bufferSize) index -= bufferSize;
        while (index < 0) index += bufferSize;
        return index;
    }
}

// Minimal implementation focused on algorithm clarity
struct PhasedVocoder_Reference::Impl {
    // Simple parameter storage (no atomics for reference)
    struct Parameters {
        float timeStretch = 1.0f;    // 0.5x to 2.0x
        float pitchShift = 0.0f;     // -12 to +12 semitones
        float mix = 1.0f;            // 0.0 to 1.0
    } params;
    
    // Per-channel processing state
    struct ChannelState {
        // Audio buffers
        std::vector<float> inputBuffer;
        std::vector<float> outputBuffer;
        std::vector<float> windowedFrame;
        
        // FFT data - JUCE format
        std::vector<std::complex<float>> fftBuffer;
        std::vector<float> hanningWindow;
        
        // Spectral processing arrays
        std::vector<float> magnitude;
        std::vector<float> phase;
        std::vector<float> lastPhase;
        std::vector<float> phaseAccumulator;
        
        // Position tracking
        int writePos = 0;
        int readPos = 0;
        int hopCounter = 0;
        
        // JUCE FFT object
        std::unique_ptr<juce::dsp::FFT> fft;
        
        // Constructor
        ChannelState() : 
            inputBuffer(FFT_SIZE * 4, 0.0f),      // Circular input buffer
            outputBuffer(FFT_SIZE * 4, 0.0f),     // Circular output buffer  
            windowedFrame(FFT_SIZE, 0.0f),
            fftBuffer(FFT_SIZE),
            hanningWindow(FFT_SIZE),
            magnitude(FFT_SIZE / 2 + 1, 0.0f),
            phase(FFT_SIZE / 2 + 1, 0.0f),
            lastPhase(FFT_SIZE / 2 + 1, 0.0f),
            phaseAccumulator(FFT_SIZE / 2 + 1, 0.0f),
            fft(std::make_unique<juce::dsp::FFT>(FFT_ORDER))
        {
            initHanningWindow();
        }
        
        void initHanningWindow() {
            for (int i = 0; i < FFT_SIZE; ++i) {
                hanningWindow[i] = 0.5f * (1.0f - std::cos(TWO_PI * i / (FFT_SIZE - 1)));
            }
        }
        
        void reset() {
            std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
            std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
            std::fill(lastPhase.begin(), lastPhase.end(), 0.0f);
            std::fill(phaseAccumulator.begin(), phaseAccumulator.end(), 0.0f);
            writePos = readPos = hopCounter = 0;
        }
    };
    
    std::vector<std::unique_ptr<ChannelState>> channels;
    double sampleRate = 44100.0;
    
    // Core processing methods
    void processFrame(ChannelState& state);
    void analyzeFrame(ChannelState& state);
    void synthesizeFrame(ChannelState& state);
};

// Constructor/Destructor
PhasedVocoder_Reference::PhasedVocoder_Reference() : pimpl(std::make_unique<Impl>()) {}
PhasedVocoder_Reference::~PhasedVocoder_Reference() = default;

void PhasedVocoder_Reference::prepareToPlay(double sampleRate, int samplesPerBlock) {
    pimpl->sampleRate = sampleRate;
    
    // Prepare stereo processing
    pimpl->channels.clear();
    for (int ch = 0; ch < 2; ++ch) {
        pimpl->channels.push_back(std::make_unique<Impl::ChannelState>());
    }
}

void PhasedVocoder_Reference::reset() {
    for (auto& channel : pimpl->channels) {
        channel->reset();
    }
}

void PhasedVocoder_Reference::process(juce::AudioBuffer<float>& buffer) {
    // Basic denormal protection
    DenormalGuard guard;
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Process each channel independently
    for (int ch = 0; ch < numChannels && ch < pimpl->channels.size(); ++ch) {
        auto& state = *pimpl->channels[ch];
        float* channelData = buffer.getWritePointer(ch);
        
        // Store original for dry/wet mixing
        std::vector<float> originalData(channelData, channelData + numSamples);
        
        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample) {
            // Store input sample in circular buffer
            state.inputBuffer[state.writePos] = channelData[sample];
            state.writePos = wrapIndex(state.writePos + 1, state.inputBuffer.size());
            
            // Process frame when we reach hop boundary
            if (++state.hopCounter >= HOP_SIZE) {
                state.hopCounter = 0;
                pimpl->processFrame(state);
            }
            
            // Read processed output
            channelData[sample] = state.outputBuffer[state.readPos];
            state.outputBuffer[state.readPos] = 0.0f; // Clear after reading
            state.readPos = wrapIndex(state.readPos + 1, state.outputBuffer.size());
        }
        
        // Apply dry/wet mix
        const float mix = pimpl->params.mix;
        for (int sample = 0; sample < numSamples; ++sample) {
            channelData[sample] = originalData[sample] * (1.0f - mix) + 
                                channelData[sample] * mix;
        }
    }
    
    // Safety scrub for NaN/Inf
    scrubBuffer(buffer);
}

void PhasedVocoder_Reference::Impl::processFrame(ChannelState& state) {
    // Fill windowed frame from input buffer
    const int startPos = wrapIndex(state.writePos - FFT_SIZE, state.inputBuffer.size());
    
    for (int i = 0; i < FFT_SIZE; ++i) {
        const int bufferIndex = wrapIndex(startPos + i, state.inputBuffer.size());
        state.windowedFrame[i] = state.inputBuffer[bufferIndex] * state.hanningWindow[i];
    }
    
    // Spectral analysis
    analyzeFrame(state);
    
    // Apply time/pitch modifications
    const float timeStretch = params.timeStretch;
    const float pitchShiftRatio = std::pow(2.0f, params.pitchShift / 12.0f);
    
    // Phase vocoder processing
    const float expectedPhaseIncrement = TWO_PI * HOP_SIZE / FFT_SIZE;
    
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        // Phase unwrapping
        float phaseDiff = state.phase[bin] - state.lastPhase[bin];
        state.lastPhase[bin] = state.phase[bin];
        
        // Wrap phase difference to [-π, π]
        while (phaseDiff > PI) phaseDiff -= TWO_PI;
        while (phaseDiff < -PI) phaseDiff += TWO_PI;
        
        // Compute true frequency
        const float deviation = phaseDiff - expectedPhaseIncrement * bin;
        const float trueFreq = (bin + deviation / expectedPhaseIncrement) * sampleRate / FFT_SIZE;
        
        // Apply pitch shifting
        const float shiftedFreq = trueFreq * pitchShiftRatio;
        
        // Phase accumulation with time stretching
        const float phaseIncrement = TWO_PI * shiftedFreq * HOP_SIZE / (timeStretch * sampleRate);
        state.phaseAccumulator[bin] += phaseIncrement;
        
        // Wrap accumulated phase
        while (state.phaseAccumulator[bin] > PI) state.phaseAccumulator[bin] -= TWO_PI;
        while (state.phaseAccumulator[bin] < -PI) state.phaseAccumulator[bin] += TWO_PI;
    }
    
    // Spectral synthesis
    synthesizeFrame(state);
}

void PhasedVocoder_Reference::Impl::analyzeFrame(ChannelState& state) {
    // Copy windowed audio to FFT buffer (JUCE format: interleaved real/imag)
    for (int i = 0; i < FFT_SIZE; ++i) {
        state.fftBuffer[i] = std::complex<float>(state.windowedFrame[i], 0.0f);
    }
    
    // Forward FFT
    state.fft->perform(reinterpret_cast<float*>(state.fftBuffer.data()), 
                       reinterpret_cast<float*>(state.fftBuffer.data()), false);
    
    // Extract magnitude and phase (positive frequencies only)
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        const float real = state.fftBuffer[bin].real();
        const float imag = state.fftBuffer[bin].imag();
        
        state.magnitude[bin] = std::sqrt(real * real + imag * imag);
        state.phase[bin] = std::atan2(imag, real);
    }
}

void PhasedVocoder_Reference::Impl::synthesizeFrame(ChannelState& state) {
    // Reconstruct spectrum from modified magnitude/phase
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        const float mag = state.magnitude[bin];
        const float ph = state.phaseAccumulator[bin];
        
        state.fftBuffer[bin] = std::complex<float>(mag * std::cos(ph), mag * std::sin(ph));
        
        // Hermitian symmetry for real-valued output
        if (bin > 0 && bin < FFT_SIZE / 2) {
            state.fftBuffer[FFT_SIZE - bin] = std::conj(state.fftBuffer[bin]);
        }
    }
    
    // Ensure DC and Nyquist are real
    state.fftBuffer[0] = std::complex<float>(state.fftBuffer[0].real(), 0.0f);
    if (FFT_SIZE % 2 == 0) {
        state.fftBuffer[FFT_SIZE / 2] = std::complex<float>(state.fftBuffer[FFT_SIZE / 2].real(), 0.0f);
    }
    
    // Inverse FFT
    state.fft->perform(reinterpret_cast<float*>(state.fftBuffer.data()), 
                       reinterpret_cast<float*>(state.fftBuffer.data()), true);
    
    // Overlap-add with correct scaling
    // JUCE FFT is unnormalized, so divide by FFT_SIZE
    // Window normalization for 75% overlap with Hann window ≈ 1.5
    const float scale = 1.0f / (FFT_SIZE * 1.5f);
    
    const int outputStart = wrapIndex(state.writePos - FFT_SIZE + HOP_SIZE, state.outputBuffer.size());
    
    for (int i = 0; i < FFT_SIZE; ++i) {
        const int outputIndex = wrapIndex(outputStart + i, state.outputBuffer.size());
        const float windowedSample = state.fftBuffer[i].real() * state.hanningWindow[i] * scale;
        state.outputBuffer[outputIndex] += windowedSample;
    }
}

void PhasedVocoder_Reference::updateParameters(const std::map<int, float>& params) {
    for (const auto& [id, value] : params) {
        switch (static_cast<ParamID>(id)) {
            case ParamID::TimeStretch:
                // Map 0-1 to 0.5x-2.0x stretch
                pimpl->params.timeStretch = 0.5f + value * 1.5f;
                break;
                
            case ParamID::PitchShift:
                // Map 0-1 to -12 to +12 semitones
                pimpl->params.pitchShift = (value - 0.5f) * 24.0f;
                break;
                
            case ParamID::Mix:
                pimpl->params.mix = value;
                break;
        }
    }
}

juce::String PhasedVocoder_Reference::getParameterName(int index) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::TimeStretch: return "Time Stretch";
        case ParamID::PitchShift:  return "Pitch Shift";
        case ParamID::Mix:         return "Mix";
        default:                   return "";
    }
}

juce::String PhasedVocoder_Reference::getParameterDisplayString(int index, float value) const {
    switch (static_cast<ParamID>(index)) {
        case ParamID::TimeStretch: {
            float stretch = 0.5f + value * 1.5f;
            return juce::String(stretch, 2) + "x";
        }
        case ParamID::PitchShift: {
            float semitones = (value - 0.5f) * 24.0f;
            if (std::abs(semitones) < 0.1f) return "0 st";
            return juce::String(semitones, 1) + " st";
        }
        case ParamID::Mix: {
            return juce::String(static_cast<int>(value * 100)) + "%";
        }
        default:
            return "";
    }
}

juce::String PhasedVocoder_Reference::getName() const {
    return "Phase Vocoder Reference";
}